/* SPDX-License-Identifier: GPL-2.0-only
 * One-slot lifetime/resync semantics derive from framework/src/packet_hub.c.
 * Separate CPU chunk ownership and real span connection; see LICENSE_NOTICES.md.
 */
#include "span_hub.h"
#include <stdlib.h>
#include <string.h>

_Static_assert(CAMERA_SPAN_HUB_V1_CHUNKS*CAMERA_SPAN_HUB_V1_CHUNK_BYTES==
 CAMERA_RTP_H264_NAL_V1_BYTES,"preserve the existing NAL envelope");
_Static_assert(CAMERA_SPAN_HUB_V1_CHUNK_BYTES<=131072U,"allocation request cap");
_Static_assert(CAMERA_SPAN_HUB_V1_CHUNKS<=CAMERA_RTP_H264_SPANS_V1_MAX,
 "owned view fits the actual packetizer API");
_Static_assert(sizeof(struct camera_span_hub_v1)<4096&&
 sizeof(struct camera_span_hub_view_v1)<4096,"fixed small metadata, not NAL backing");

static int usable(const struct camera_span_hub_v1 *h)
{
 if(!h||h->self!=h||(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE&&
    h->phase!=CAMERA_SPAN_HUB_V1_STOPPED))return 0;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)if(!h->chunks[i])return 0;
 return 1;
}

static int consumer_valid(const struct camera_span_hub_v1 *h,uint32_t c)
{return usable(h)&&c<CAMERA_SPAN_HUB_V1_CONSUMERS&&(h->subscribers&(1U<<c));}

static void lose_backlog(struct camera_span_hub_v1 *h,uint32_t consumers)
{
 h->pending&=~(consumers&~h->acquired);
 h->need_keyframe|=consumers;
}

int camera_span_hub_init_v1(struct camera_span_hub_v1 *h)
{
 if(!h)return CAMERA_INVALID;
 if(h->self)return h->self==h?CAMERA_NOT_READY:CAMERA_INVALID;
 if(h->phase!=CAMERA_SPAN_HUB_V1_FRESH)return CAMERA_INVALID;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)
  if(h->chunks[i])return CAMERA_INVALID;
 /* Only true-fresh zero-initialized owners are admitted by the caller contract.
  * Latch attempt/identity BEFORE allocation. Never erase an existing owner. */
 h->self=h;h->phase=CAMERA_SPAN_HUB_V1_INITIALIZING;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++){
  h->allocation_calls++;
  h->chunks[i]=malloc(CAMERA_SPAN_HUB_V1_CHUNK_BYTES);
  if(!h->chunks[i]){h->phase=CAMERA_SPAN_HUB_V1_INIT_FAILED;return CAMERA_NOT_READY;}
  h->allocated_bytes+=CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
  h->maximum_allocation=CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
 }
 h->phase=CAMERA_SPAN_HUB_V1_ACTIVE;
 return CAMERA_OK;
}

int camera_span_hub_subscribe_v1(struct camera_span_hub_v1 *h,uint32_t c)
{
 if(!usable(h)||c>=CAMERA_SPAN_HUB_V1_CONSUMERS)return CAMERA_INVALID;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE)return CAMERA_NOT_READY;
 uint32_t bit=1U<<c;
 if(h->subscribers&bit)return CAMERA_BUSY;
 h->subscribers|=bit;h->need_keyframe|=bit;
 return CAMERA_OK;
}

int camera_span_hub_resync_v1(struct camera_span_hub_v1 *h,uint32_t c)
{
 if(!consumer_valid(h,c))return CAMERA_INVALID;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE)return CAMERA_NOT_READY;
 lose_backlog(h,1U<<c);
 return CAMERA_OK;
}

int camera_span_hub_unsubscribe_v1(struct camera_span_hub_v1 *h,uint32_t c)
{
 if(!consumer_valid(h,c))return CAMERA_INVALID;
 uint32_t bit=1U<<c;
 if(h->acquired&bit)return CAMERA_BUSY;
 h->pending&=~bit;h->subscribers&=~bit;h->need_keyframe&=~bit;
 return CAMERA_OK;
}

static int input_valid(const struct camera_span_hub_input_v1 *p)
{
 if(!p||p->codec!=CAMERA_H264||p->keyframe>1||!p->spans||!p->span_count||
    p->span_count>CAMERA_RTP_H264_SPANS_V1_MAX||!p->bytes||
    p->bytes>CAMERA_RTP_H264_NAL_V1_BYTES)return 0;
 size_t sum=0;
 for(size_t i=0;i<p->span_count;i++){
  const struct camera_rtp_h264_span_v1 *s=&p->spans[i];
  if(!s->data||!s->size||s->size>CAMERA_SPAN_HUB_V1_CHUNK_BYTES||
     s->size>SIZE_MAX-sum)return 0;
  sum+=s->size;
 }
 if(sum!=p->bytes)return 0;
 const uint8_t header=p->spans[0].data[0],type=header&31;
 return !(header&0x80)&&(type==1||type==5)&&p->keyframe==(uint32_t)(type==5);
}

/* Full shape is checked before entry: destination stays below total<=8 chunks,
 * every source extent is nonempty and <=one chunk. Each memcpy consumes a
 * positive minimum of source and destination remainders. At most 16+8-1 pieces;
 * final destination==total is never dereferenced after the last copy. */
static void copy_input(struct camera_span_hub_v1 *h,
 const struct camera_span_hub_input_v1 *p)
{
 size_t destination=0;
 for(size_t i=0;i<p->span_count;i++){
  size_t source=0;
  while(source<p->spans[i].size){
   const size_t chunk=destination/CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
   const size_t offset=destination%CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
   size_t take=CAMERA_SPAN_HUB_V1_CHUNK_BYTES-offset;
   if(take>p->spans[i].size-source)take=p->spans[i].size-source;
   memcpy(h->chunks[chunk]+offset,p->spans[i].data+source,take);
   destination+=take;source+=take;
  }
 }
}

int camera_span_hub_publish_v1(struct camera_span_hub_v1 *h,
 const struct camera_span_hub_input_v1 *p)
{
 if(!usable(h)||!input_valid(p))return CAMERA_INVALID;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||h->generation==UINT64_MAX)
  return CAMERA_NOT_READY;
 uint32_t eligible=p->keyframe?h->subscribers:h->subscribers&~h->need_keyframe;
 if(!eligible)return CAMERA_NOT_READY;
 if(h->acquired){lose_backlog(h,h->subscribers);return CAMERA_BUSY;}
 if(h->pending){
  lose_backlog(h,h->pending);
  eligible=p->keyframe?h->subscribers:h->subscribers&~h->need_keyframe;
  if(!eligible)return CAMERA_BUSY;
 }
 /* No refusal or callbacks after this point; no retained input pointer. */
 copy_input(h,p);
 h->generation++;h->monotonic_ns=p->monotonic_ns;h->codec=p->codec;
 h->keyframe=p->keyframe;h->bytes=p->bytes;
 h->pending=eligible;h->acquired=0;
 if(p->keyframe)h->need_keyframe&=~eligible;
 return CAMERA_OK;
}

int camera_span_hub_acquire_v1(struct camera_span_hub_v1 *h,uint32_t c,
 struct camera_span_hub_view_v1 *view)
{
 if(!consumer_valid(h,c)||!view)return CAMERA_INVALID;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE)return CAMERA_NOT_READY;
 uint32_t bit=1U<<c;
 if(h->acquired&bit)return CAMERA_BUSY;
 if(!(h->pending&bit))return CAMERA_NOT_READY;
 if(h->issued_token==UINT64_MAX)return CAMERA_NOT_READY;
 struct camera_span_hub_view_v1 v={.owner=h,.lease_token=h->issued_token+1,
  .generation=h->generation,.monotonic_ns=h->monotonic_ns,.codec=h->codec,
  .keyframe=h->keyframe,.consumer=c,.bytes=h->bytes};
 size_t remaining=h->bytes;
 while(remaining){
  size_t take=remaining;
  if(take>CAMERA_SPAN_HUB_V1_CHUNK_BYTES)take=CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
  v.spans[v.span_count]=(struct camera_rtp_h264_span_v1){h->chunks[v.span_count],take};
  v.span_count++;remaining-=take;
 }
 h->issued_token=v.lease_token;h->active_token[c]=v.lease_token;
 h->acquired|=bit;*view=v;
 return CAMERA_OK;
}

int camera_span_hub_release_v1(struct camera_span_hub_v1 *h,uint32_t c,
 uint64_t token,uint64_t generation)
{
 if(!consumer_valid(h,c)||!token||!generation)return CAMERA_INVALID;
 if(token!=h->active_token[c]||generation!=h->generation)return CAMERA_STALE_LEASE;
 uint32_t bit=1U<<c;
 if(!(h->acquired&bit))return CAMERA_INVALID;
 h->active_token[c]=0;
 h->acquired&=~bit;h->pending&=~bit;
 return CAMERA_OK;
}

int camera_span_hub_lease_valid_v1(const struct camera_span_hub_v1 *h,uint32_t c,
 const struct camera_span_hub_view_v1 *v)
{
 if(!consumer_valid(h,c)||!v||h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||
    !(h->acquired&(1U<<c))||v->owner!=h||!v->lease_token||!v->generation||
    v->consumer!=c||v->lease_token!=h->active_token[c]||v->generation!=h->generation||
    v->codec!=h->codec||v->keyframe!=h->keyframe||v->monotonic_ns!=h->monotonic_ns||
    !h->bytes||v->bytes!=h->bytes||
    v->span_count!=(h->bytes-1)/CAMERA_SPAN_HUB_V1_CHUNK_BYTES+1)return 0;
 size_t remaining=h->bytes;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++){
  size_t take=remaining;
  if(take>CAMERA_SPAN_HUB_V1_CHUNK_BYTES)take=CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
  if(v->spans[i].data!=(take?h->chunks[i]:NULL)||v->spans[i].size!=take)return 0;
  remaining-=take;
 }
 return 1;
}

int camera_span_hub_packet_v1(const struct camera_span_hub_v1 *h,uint32_t c,
 const struct camera_span_hub_view_v1 *v,const struct camera_rtp_session *session,
 uint32_t timestamp,uint32_t last,size_t offset,
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],size_t *bytes,size_t *next)
{
 if(!camera_span_hub_lease_valid_v1(h,c,v))return CAMERA_STALE_LEASE;
 return camera_rtp_h264_spans_packet_v1(session,timestamp,v->spans,v->span_count,
  v->bytes,last,offset,packet,bytes,next);
}

int camera_span_hub_stop_v1(struct camera_span_hub_v1 *h)
{
 if(!usable(h))return CAMERA_INVALID;
 h->phase=CAMERA_SPAN_HUB_V1_STOPPED;h->pending=h->acquired;
 return CAMERA_OK;
}

int camera_span_hub_drained_v1(const struct camera_span_hub_v1 *h)
{return usable(h)&&h->phase==CAMERA_SPAN_HUB_V1_STOPPED&&!h->acquired;}

int camera_span_hub_dispose_v1(struct camera_span_hub_v1 *h)
{
 if(!h||h->self!=h)return CAMERA_INVALID;
 if(h->phase==CAMERA_SPAN_HUB_V1_INIT_FAILED){
  if(h->generation||h->pending||h->acquired)return CAMERA_NOT_READY;
 }else if(!camera_span_hub_drained_v1(h))return CAMERA_NOT_READY;
 h->phase=CAMERA_SPAN_HUB_V1_DISPOSING;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++){
  uint8_t *owned=h->chunks[i];
  h->chunks[i]=NULL;
  if(owned)free(owned);
 }
 h->phase=CAMERA_SPAN_HUB_V1_DISPOSED;
 return CAMERA_OK;
}
