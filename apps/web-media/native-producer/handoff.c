/* SPDX-License-Identifier: GPL-2.0-only */
#include "handoff.h"
#include "crc-bzip2.h"
#include <string.h>

static int tick(const struct nh_backend *b,struct nh_report *r,uint64_t *out)
{
 uint64_t n=b->now(b->context);
 if(n==UINT64_MAX||n<r->last_clock){r->quarantined=1;return CAMERA_IO_ERROR;}
 r->last_clock=n;*out=n;
 if(b->cancelled(b->context)||n-r->started>=NH_SESSION_NS)return CAMERA_DISCONNECTED;
 return CAMERA_OK;
}
static int same(const struct nh_key *a,const struct nh_key *b)
{return a->source==b->source&&a->file==b->file&&a->generation==b->generation;}
static int valid(const struct camera_packet *p,const struct nh_key *k,const uint8_t *storage,size_t capacity)
{
 uintptr_t begin=(uintptr_t)p->data,hub_begin=(uintptr_t)storage;
 if(p->bytes>UINTPTR_MAX-begin||hub_begin>UINTPTR_MAX-capacity||
    (begin<hub_begin+capacity&&begin+p->bytes>hub_begin))return 0;
 return p->codec==CAMERA_H264&&p->data&&p->bytes>1&&p->bytes<=CAMERA_FMP4_MAX_NAL_BYTES&&
 p->keyframe<=1&&(p->data[0]&31)==(p->keyframe?5:1)&&p->monotonic_ns!=UINT64_MAX&&
 k->source&&k->file&&k->generation&&k->sequence&&k->sequence<=128;
}
static uint32_t rtp(const struct camera_packet *p)
{
 /* Divide first: even (UINT64_MAX-1)/1000000 *90 fits uint64_t.
  * Final narrowing is intentional RTP wrap, checked by the timeline. */
 return (uint32_t)((p->monotonic_ns/1000000ULL)*90ULL);
}
static int checksum(const struct nh_backend *b,struct nh_report *r,
                    const uint8_t *p,size_t bytes,uint32_t *crc,uint64_t *begin,uint64_t *end)
{
 int result=tick(b,r,begin);if(result)return result;
 *crc=qc_crc_bzip2(p,bytes); /* qualified full scalar oracle, no copy/table slicing */
 result=tick(b,r,end);if(result)return result;
 r->integrity_ns+=*end-*begin;return CAMERA_OK;
}
static int bytes_out(const struct nh_backend *b,struct nh_report *r,
                     const uint8_t *p,size_t n,uint64_t start,
                     const struct camera_packet_hub *hub,const struct camera_packet *lease)
{
 while(n){
  uint64_t now;size_t done=0,take=n>CAMERA_FMP4_SEND_CHUNK_BYTES?CAMERA_FMP4_SEND_CHUNK_BYTES:n;
  int e=tick(b,r,&now);if(e)return e;
  if(hub&&!camera_hub_lease_valid(hub,0,lease))return CAMERA_STALE_LEASE;
  if(now-start>=NH_OUTPUT_NS)return CAMERA_DISCONNECTED;
  e=b->write(b->context,p,take,&done);
  if(done>take)return CAMERA_IO_ERROR;
  if(e==CAMERA_BUSY&&!done)continue; /* bounded I/O progress; next attempt rechecks exact lease */
  r->accepted+=done;p+=done;n-=done;
  if(e||!done)return CAMERA_IO_ERROR;
  e=tick(b,r,&now);if(e)return e;
  if(now-start>=NH_OUTPUT_NS)return CAMERA_DISCONNECTED;
 }
 return CAMERA_OK;
}
int nh_run(const struct nh_backend *b,const struct camera_fmp4_config *config,
           uint8_t *storage,size_t capacity,struct nh_report *r)
{
 struct camera_packet_hub hub;struct camera_packet pending={0},old={0};
 struct nh_key future={0};struct camera_fmp4_timeline timeline;
 struct camera_fmp4_init init;unsigned held=0,pending_held=0,hub_ready=0;
 int result=CAMERA_INVALID;uint64_t now;unsigned fenced=0;
 if(!b||!config||!storage||capacity!=CAMERA_FMP4_MAX_NAL_BYTES||!r||
    !b->next||!b->release||!b->retire||!b->now||!b->cancelled||!b->write||!b->discard_output||!b->quarantine)return result;
 memset(r,0,sizeof(*r));r->started=b->now(b->context);r->last_clock=r->started;
 if(r->started==UINT64_MAX){r->quarantined=1;b->quarantine(b->context);return CAMERA_IO_ERROR;}
 if(b->session_start_ns){
  if(b->session_start_ns>r->started){r->quarantined=1;b->quarantine(b->context);return CAMERA_IO_ERROR;}
  r->started=b->session_start_ns;
 }
 if(camera_hub_init_count(&hub,storage,capacity,1)||camera_hub_subscribe(&hub,0))goto done;
 hub_ready=1;
 result=tick(b,r,&now);if(result)goto done;
 uint64_t first_begin=now;
 if(b->memory_gate){result=b->memory_gate(b->context,NH_MEM_PRE_NEXT);if(result)goto done;}
 result=b->next(b->context,&pending,&r->key);if(result){r->failed_next=1;goto done;}pending_held=1;r->produced++;
 if(b->memory_gate){result=b->memory_gate(b->context,NH_MEM_POST_FIRST);if(result)goto done;}
 if(!valid(&pending,&r->key,storage,capacity)||!pending.keyframe||r->key.sequence!=1||
    (config->generation&&config->generation!=r->key.generation)){result=CAMERA_STALE_LEASE;goto done;}
 result=tick(b,r,&now);if(result)goto done;
 if(now-first_begin>=NH_OUTPUT_NS){result=CAMERA_DISCONNECTED;goto done;}
 struct camera_fmp4_config bound_config=*config;bound_config.generation=r->key.generation;
 if(camera_fmp4_init_prepare(&bound_config,&init)||camera_fmp4_timeline_init(&timeline,r->key.generation,rtp(&pending))){result=CAMERA_INVALID;goto done;}
 if(b->output_begin){result=b->output_begin(b->context,now,init.bytes);if(result)goto done;}
 result=bytes_out(b,r,init.data,init.bytes,now,NULL,NULL);if(result)goto done;r->committed=init.bytes;
 for(;;){
  uint64_t copy_start,write_start,next_begin,next_end;
  if(pending.bytes>r->max_nal)r->max_nal=pending.bytes;
  result=tick(b,r,&copy_start);if(result)goto done;
  result=camera_hub_publish(&hub,&pending);if(result)goto done;
  /* Compare the copied old NAL to the producer's already-computed source CRC
   * BEFORE release: a bad copy must not discard the remaining producer view. */
  unsigned check=r->integrity_count++;
  r->integrity[check].sequence=r->key.sequence;
  r->integrity[check].source_crc=r->key.nal_crc32_bzip2;
  result=checksum(b,r,storage,pending.bytes,&r->integrity[check].before_crc,
   &r->integrity[check].before_begin_ns,&r->integrity[check].before_end_ns);if(result)goto done;
  if(r->integrity[check].before_crc!=r->key.nal_crc32_bzip2){r->quarantined=1;result=CAMERA_IO_ERROR;goto done;}
  result=b->release(b->context,&pending);if(result){r->quarantined=1;goto done;}pending_held=0;
  result=camera_hub_acquire(&hub,0,&old);if(result)goto done;held=1;
  result=tick(b,r,&now);if(result)goto done;
  r->hub_copy_ns+=r->integrity[check].before_begin_ns-copy_start+now-r->integrity[check].before_end_ns;
  next_begin=now;
  if(b->memory_gate){result=b->memory_gate(b->context,NH_MEM_PRE_NEXT);if(result)goto done;}
  int next_result=b->next(b->context,&pending,&future);
  if(!next_result){pending_held=1;r->produced++;}else r->failed_next=1;
  result=tick(b,r,&now);if(result)goto done;
  next_end=now;
  /* Read only the still-owned old hub on a failed next(); a partially returned
   * packet/key is never inspected, released or checksummed after nonzero return. */
  if(!camera_hub_lease_valid(&hub,0,&old)){r->quarantined=1;result=CAMERA_STALE_LEASE;goto done;}
  result=checksum(b,r,old.data,old.bytes,&r->integrity[check].after_crc,
   &r->integrity[check].after_begin_ns,&r->integrity[check].after_end_ns);if(result)goto done;
  r->integrity[check].after_checked=1;
  if(r->integrity[check].after_crc!=r->integrity[check].before_crc||
     r->integrity[check].after_crc!=r->integrity[check].source_crc){r->quarantined=1;result=CAMERA_IO_ERROR;goto done;}
  if(next_result){result=next_result;goto done;}
  if(next_end-next_begin>=NH_OUTPUT_NS){result=CAMERA_DISCONNECTED;goto done;}
  if(pending.bytes>r->max_nal)r->max_nal=pending.bytes;
  if(!valid(&pending,&future,storage,capacity)||!same(&r->key,&future)||future.sequence!=r->key.sequence+1||
     pending.monotonic_ns<=old.monotonic_ns||pending.monotonic_ns-old.monotonic_ns>=NH_OUTPUT_NS){result=CAMERA_STALE_LEASE;goto done;}
  struct camera_fmp4_fragment f;
  result=camera_fmp4_timeline_step(&timeline,r->key.generation,rtp(&old),rtp(&pending),old.data,old.bytes,old.keyframe,&f);if(result)goto done;
  size_t total=sizeof(f.header)+f.nal_bytes;
  if(total>NH_OUTPUT_LIMIT-r->committed){result=CAMERA_BUSY;goto done;}
  result=tick(b,r,&write_start);if(result)goto done;
  if(b->output_begin){result=b->output_begin(b->context,write_start,total);if(result)goto done;}
  for(size_t offset=0;offset<total;){
   const uint8_t *p;size_t n;
   if(!camera_hub_lease_valid(&hub,0,&old)){result=CAMERA_STALE_LEASE;goto done;}
   result=camera_fmp4_fragment_span(&f,offset,CAMERA_FMP4_SEND_CHUNK_BYTES,&p,&n);if(result)goto done;
   result=bytes_out(b,r,p,n,write_start,&hub,&old);if(result)goto done;offset+=n;
  }
  result=tick(b,r,&now);if(result)goto done;
  r->samples[r->output].key=r->key;
  r->samples[r->output].source_ns=old.monotonic_ns;r->samples[r->output].next_source_ns=pending.monotonic_ns;
  r->samples[r->output].next_begin_ns=next_begin;r->samples[r->output].next_end_ns=next_end;
  r->samples[r->output].output_begin_ns=write_start;r->samples[r->output].output_end_ns=now;
  r->samples[r->output].nal_bytes=old.bytes;r->samples[r->output].wire_bytes=total;
  r->output_ns+=now-write_start;r->committed+=total;r->output++;
  result=camera_hub_release(&hub,0,old.lease_token,old.generation);if(result){r->quarantined=1;goto done;}held=0;
  r->key=future;
  if(r->produced==NH_PRODUCED_LIMIT){
   result=b->release(b->context,&pending);if(result){r->quarantined=1;goto done;}
   pending_held=0;r->boundary_retired=1;result=CAMERA_OK;break;
  }
 }
done:
 /* Failed next() may have uncertain DMA even without a returned CPU lease.
  * Adapter retire must deny it; neither EOF nor process exit is proof. */
  /* Transport errors cannot release known CPU backing before actual safe source
   * stop. Native stop parks (does not free/copy) the one producer buffer while
   * END/QUERY/ACK runs; unknown failed-next backing is never treated as returned. */
  if(result&&!r->quarantined&&b->retire_held){
   if(b->retire_held(b->context,pending_held?&pending:NULL))r->quarantined=1;
   else {r->retired=1;pending_held=0;}
  }
   if(!r->quarantined&&held){
   if(camera_hub_release(&hub,0,old.lease_token,old.generation))r->quarantined=1;else held=0;
  }
  if(!r->quarantined&&pending_held){
   if(b->release(b->context,&pending))r->quarantined=1;else pending_held=0;
  }
  if(hub_ready){
   if(camera_hub_stop(&hub))r->quarantined=1;
   if(!r->quarantined&&(!camera_hub_drained(&hub)||camera_hub_unsubscribe(&hub,0)))r->quarantined=1;
  }
  if(result&&b->discard_output(b->context,r->committed))r->quarantined=1;
   if(!r->quarantined&&!r->retired){if(b->retire(b->context))r->quarantined=1;else r->retired=1;}
  if(r->quarantined){b->quarantine(b->context);fenced=1;}
 r->ended=b->now(b->context);
 if(r->ended==UINT64_MAX||r->ended<r->last_clock){r->quarantined=1;result=CAMERA_IO_ERROR;}
 if(r->quarantined){
  if(!fenced)b->quarantine(b->context);
  r->retained_hub=hub_ready;r->retained_producer=pending_held;
  return CAMERA_IO_ERROR;
 }
 return result;
}
