/* SPDX-License-Identifier: GPL-2.0-only
 * Exclusive version3 derivative of span-hub-v2. See LICENSE_NOTICES.md.
 */
#include "span_hub.h"
#include <stdlib.h>
#include <string.h>

_Static_assert(CAMERA_SPAN_HUB_V1_CHUNKS*CAMERA_SPAN_HUB_V1_CHUNK_BYTES==
 CAMERA_RTP_H264_NAL_V1_BYTES,"preserve the existing NAL envelope");
_Static_assert(CAMERA_SPAN_HUB_V1_CHUNK_BYTES==131072U,"fixed allocation/destination cap");
_Static_assert(CAMERA_SPAN_HUB_V1_CHUNKS<=CAMERA_RTP_H264_SPANS_V1_MAX,
 "owned view fits the actual packetizer API");
_Static_assert(sizeof(struct camera_span_hub_v1)<4096&&
 sizeof(struct camera_span_hub_view_v1)<4096&&
 sizeof(struct camera_span_hub_writer_v3)<4096,"small metadata, not NAL backing");

static int next_generation(const struct camera_span_hub_v1 *h)
{return h->generation!=UINT64_MAX&&h->writer_generation==h->generation+1;}

/* Audit phase AND identities/exposure/error, not an editable ready Boolean.
 * Trust still requires no caller edits; these are not pointer provenance checks. */
static int writer_consistent(const struct camera_span_hub_v1 *h)
{
 if(h->writer_token!=h->issued_writer_token||h->armed_writer_token>h->writer_token)
  return 0;
 if(!h->writer_token)
  return h->writer_phase==CAMERA_SPAN_WRITER_V3_IDLE&&!h->writer_generation&&
   !h->armed_writer_token&&!h->writer_error;
 if(!h->writer_generation)return 0;
 switch(h->writer_phase){
 case CAMERA_SPAN_WRITER_V3_IDLE:
  return !h->writer_error&&
   (h->writer_generation<=h->generation||next_generation(h));
 case CAMERA_SPAN_WRITER_V3_RESERVED:
  return !h->writer_error&&next_generation(h)&&h->armed_writer_token<h->writer_token;
 case CAMERA_SPAN_WRITER_V3_ARMED:
  return !h->writer_error&&next_generation(h)&&h->armed_writer_token==h->writer_token;
 case CAMERA_SPAN_WRITER_V3_HOLD:
  return h->writer_error&&next_generation(h)&&h->armed_writer_token==h->writer_token;
 default:return 0;
 }
}

static int consumers_unheld(const struct camera_span_hub_v1 *h)
{
 if(h->acquired)return 0;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CONSUMERS;i++)if(h->active_token[i])return 0;
 return 1;
}

static int usable(const struct camera_span_hub_v1 *h)
{
 if(!h||h->self!=h||h->layout_version!=CAMERA_SPAN_HUB_LAYOUT_VERSION||
    (h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE&&h->phase!=CAMERA_SPAN_HUB_V1_STOPPED)||
    !writer_consistent(h))return 0;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)if(!h->chunks[i])return 0;
 if(h->writer_phase!=CAMERA_SPAN_WRITER_V3_IDLE&&
    (h->pending||!consumers_unheld(h)))return 0;
 return 1;
}

static int writer_idle(const struct camera_span_hub_v1 *h)
{return h->writer_phase==CAMERA_SPAN_WRITER_V3_IDLE&&!h->writer_error;}

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
 /* Only true-fresh zero-initialized owners are admitted by caller contract. */
 h->self=h;h->layout_version=CAMERA_SPAN_HUB_LAYOUT_VERSION;
 h->phase=CAMERA_SPAN_HUB_V1_INITIALIZING;
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
 if(!writer_idle(h))return CAMERA_BUSY;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE)return CAMERA_NOT_READY;
 uint32_t bit=1U<<c;
 if(h->subscribers&bit)return CAMERA_BUSY;
 h->subscribers|=bit;h->need_keyframe|=bit;
 return CAMERA_OK;
}

int camera_span_hub_resync_v1(struct camera_span_hub_v1 *h,uint32_t c)
{
 if(!consumer_valid(h,c))return CAMERA_INVALID;
 if(!writer_idle(h))return CAMERA_BUSY;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE)return CAMERA_NOT_READY;
 lose_backlog(h,1U<<c);
 return CAMERA_OK;
}

int camera_span_hub_unsubscribe_v1(struct camera_span_hub_v1 *h,uint32_t c)
{
 if(!consumer_valid(h,c))return CAMERA_INVALID;
 if(!writer_idle(h))return CAMERA_BUSY;
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

/* Positive bounded minima, at most16+8-1 pieces; final total is not read. */
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
 if(!usable(h))return CAMERA_INVALID;
 if(!writer_idle(h))return CAMERA_BUSY;
 if(!input_valid(p))return CAMERA_INVALID;
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
 h->keyframe=p->keyframe;h->bytes=p->bytes;h->data_offset=0;
 h->pending=eligible;h->acquired=0;
 if(p->keyframe)h->need_keyframe&=~eligible;
 return CAMERA_OK;
}

static int reservation_exact(const struct camera_span_hub_v1 *h,
 const struct camera_span_hub_reservation_v3 *r)
{return r&&r->owner==h&&r->token&&r->token==h->writer_token&&
 r->generation==h->writer_generation;}

static int writer_exact(const struct camera_span_hub_v1 *h,
 const struct camera_span_hub_writer_v3 *w)
{return w&&w->owner==h&&w->token&&w->token==h->writer_token&&
 w->generation==h->writer_generation;}

int camera_span_hub_producer_reserve_v3(struct camera_span_hub_v1 *h,
 struct camera_span_hub_reservation_v3 *r)
{
 if(!usable(h)||!r)return CAMERA_INVALID;
 if(!writer_idle(h)||h->pending||!consumers_unheld(h))return CAMERA_BUSY;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||!h->subscribers||
    h->generation==UINT64_MAX||h->issued_writer_token==UINT64_MAX)
  return CAMERA_NOT_READY;
 const struct camera_span_hub_reservation_v3 next={h,h->issued_writer_token+1,
  h->generation+1};
 h->issued_writer_token=next.token;h->writer_token=next.token;
 h->writer_generation=next.generation;h->writer_phase=CAMERA_SPAN_WRITER_V3_RESERVED;
 *r=next;
 return CAMERA_OK;
}

int camera_span_hub_producer_cancel_v3(struct camera_span_hub_v1 *h,
 const struct camera_span_hub_reservation_v3 *r)
{
 if(!usable(h))return CAMERA_INVALID;
 if(!reservation_exact(h,r))return CAMERA_STALE_LEASE;
 if(h->writer_phase!=CAMERA_SPAN_WRITER_V3_RESERVED)return CAMERA_NOT_READY;
 /* No writable alias ever exposed for THIS token; histories remain. */
 h->writer_phase=CAMERA_SPAN_WRITER_V3_IDLE;
 return CAMERA_OK;
}

int camera_span_hub_producer_arm_v3(struct camera_span_hub_v1 *h,
 const struct camera_span_hub_reservation_v3 *r,struct camera_span_hub_writer_v3 *w)
{
 if(!usable(h)||!w)return CAMERA_INVALID;
 if(!reservation_exact(h,r))return CAMERA_STALE_LEASE;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||
    h->writer_phase!=CAMERA_SPAN_WRITER_V3_RESERVED)return CAMERA_NOT_READY;
 struct camera_span_hub_writer_v3 next={.owner=h,.token=h->writer_token,
  .generation=h->writer_generation,.span_count=CAMERA_SPAN_HUB_V1_CHUNKS};
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)
  next.spans[i]=(struct camera_span_hub_write_span_v3){h->chunks[i],
   CAMERA_SPAN_HUB_V1_CHUNK_BYTES};
 h->writer_phase=CAMERA_SPAN_WRITER_V3_ARMED;
 h->armed_writer_token=h->writer_token;
 *w=next; /* only now are canonical writable aliases exposed */
 return CAMERA_OK;
}

static int hold_writer(struct camera_span_hub_v1 *h,int error)
{
 if(!h->writer_error)h->writer_error=error;
 h->writer_phase=CAMERA_SPAN_WRITER_V3_HOLD;
 return error;
}

int camera_span_hub_producer_hold_v3(struct camera_span_hub_v1 *h,
 const struct camera_span_hub_writer_v3 *w)
{
 if(!usable(h))return CAMERA_INVALID;
 if(!writer_exact(h,w))return CAMERA_STALE_LEASE;
 if(h->writer_phase==CAMERA_SPAN_WRITER_V3_HOLD)return CAMERA_NOT_READY;
 if(h->writer_phase!=CAMERA_SPAN_WRITER_V3_ARMED)return CAMERA_NOT_READY;
 return hold_writer(h,CAMERA_IO_ERROR);
}

/* Read ONLY a proven written prefix/header offset (<written<=1MiB). */
static uint8_t owned_byte(const struct camera_span_hub_v1 *h,size_t offset)
{return h->chunks[offset/CAMERA_SPAN_HUB_V1_CHUNK_BYTES]
 [offset%CAMERA_SPAN_HUB_V1_CHUNK_BYTES];}

static int result_valid(const struct camera_span_hub_v1 *h,
 const struct camera_span_hub_result_v3 *p)
{
 if(!p||p->codec!=CAMERA_H264||p->keyframe>1||p->monotonic_ns==UINT64_MAX||
    !p->written_bytes||p->written_bytes>CAMERA_RTP_H264_NAL_V1_BYTES||
    (p->prefix_bytes!=0&&p->prefix_bytes!=3&&p->prefix_bytes!=4)||
    p->prefix_bytes>=p->written_bytes)return 0;
 if(p->prefix_bytes){
  for(size_t i=0;i<p->prefix_bytes-1;i++)if(owned_byte(h,i)!=0)return 0;
  if(owned_byte(h,p->prefix_bytes-1)!=1)return 0;
 }
 const uint8_t header=owned_byte(h,p->prefix_bytes),type=header&31;
 return !(header&0x80)&&(type==1||type==5)&&p->keyframe==(uint32_t)(type==5);
}

int camera_span_hub_producer_commit_v3(struct camera_span_hub_v1 *h,
 const struct camera_span_hub_writer_v3 *w,const struct camera_span_hub_result_v3 *p)
{
 if(!usable(h))return CAMERA_INVALID;
 if(!writer_exact(h,w))return CAMERA_STALE_LEASE;
 if(h->writer_phase!=CAMERA_SPAN_WRITER_V3_ARMED)return CAMERA_NOT_READY;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||!next_generation(h))
  return hold_writer(h,CAMERA_NOT_READY);
 if(w->span_count!=CAMERA_SPAN_HUB_V1_CHUNKS)return hold_writer(h,CAMERA_INVALID);
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)
  if(w->spans[i].data!=h->chunks[i]||
     w->spans[i].capacity!=CAMERA_SPAN_HUB_V1_CHUNK_BYTES)
   return hold_writer(h,CAMERA_INVALID);
 if(h->pending||!consumers_unheld(h))return hold_writer(h,CAMERA_BUSY);
 if(!result_valid(h,p))return hold_writer(h,CAMERA_INVALID);
 const uint32_t eligible=p->keyframe?h->subscribers:h->subscribers&~h->need_keyframe;
 if(!eligible)return hold_writer(h,CAMERA_NOT_READY);
 /* EXTERNAL real copy0/result/release/exact END/guards are caller obligations.
  * All local refusals are above. No callbacks/copies/allocations after this point.
  * Aliases MUST cease on success. Retire writer only AFTER publication metadata. */
 h->generation++;h->monotonic_ns=p->monotonic_ns;h->codec=p->codec;
 h->keyframe=p->keyframe;h->data_offset=p->prefix_bytes;
 h->bytes=p->written_bytes-p->prefix_bytes;h->pending=eligible;h->acquired=0;
 if(p->keyframe)h->need_keyframe&=~eligible;
 h->writer_phase=CAMERA_SPAN_WRITER_V3_IDLE;
 return CAMERA_OK;
}

/* Canonical packed logical slice: physical chunk0 starts at offset0/3/4 for
 * writer commits; legacy publication is offset0. General checked bounds avoid
 * relying on that small offset for arithmetic safety. Tail descriptors are0. */
static int slice_spans(const struct camera_span_hub_v1 *h,
 struct camera_rtp_h264_span_v1 spans[CAMERA_SPAN_HUB_V1_CHUNKS],size_t *count)
{
 if(!h->bytes||h->data_offset>=CAMERA_RTP_H264_NAL_V1_BYTES||
    h->bytes>CAMERA_RTP_H264_NAL_V1_BYTES-h->data_offset)return 0;
 size_t position=h->data_offset,remaining=h->bytes,n=0;
 while(remaining){
  const size_t index=position/CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
  const size_t offset=position%CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
  size_t take=CAMERA_SPAN_HUB_V1_CHUNK_BYTES-offset;
  if(take>remaining)take=remaining;
  spans[n++]=(struct camera_rtp_h264_span_v1){h->chunks[index]+offset,take};
  position+=take;remaining-=take;
 }
 for(size_t i=n;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)
  spans[i]=(struct camera_rtp_h264_span_v1){NULL,0};
 *count=n;
 return 1;
}

int camera_span_hub_acquire_v1(struct camera_span_hub_v1 *h,uint32_t c,
 struct camera_span_hub_view_v1 *view)
{
 if(!consumer_valid(h,c)||!view)return CAMERA_INVALID;
 if(!writer_idle(h))return CAMERA_BUSY;
 if(h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE)return CAMERA_NOT_READY;
 uint32_t bit=1U<<c;
 if(h->acquired&bit)return CAMERA_BUSY;
 if(!(h->pending&bit))return CAMERA_NOT_READY;
 if(h->issued_token==UINT64_MAX)return CAMERA_NOT_READY;
 struct camera_span_hub_view_v1 v={.owner=h,.lease_token=h->issued_token+1,
  .generation=h->generation,.monotonic_ns=h->monotonic_ns,.codec=h->codec,
  .keyframe=h->keyframe,.consumer=c,.bytes=h->bytes};
 if(!slice_spans(h,v.spans,&v.span_count))return CAMERA_INVALID;
 h->issued_token=v.lease_token;h->active_token[c]=v.lease_token;
 h->acquired|=bit;*view=v;
 return CAMERA_OK;
}

int camera_span_hub_release_v1(struct camera_span_hub_v1 *h,uint32_t c,
 uint64_t token,uint64_t generation)
{
 if(!consumer_valid(h,c)||!token||!generation)return CAMERA_INVALID;
 if(!writer_idle(h))return CAMERA_BUSY;
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
 if(!consumer_valid(h,c)||!writer_idle(h)||!v||h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||
    !(h->acquired&(1U<<c))||v->owner!=h||!v->lease_token||!v->generation||
    v->consumer!=c||v->lease_token!=h->active_token[c]||v->generation!=h->generation||
    v->codec!=h->codec||v->keyframe!=h->keyframe||v->monotonic_ns!=h->monotonic_ns||
    !h->bytes||v->bytes!=h->bytes)return 0;
 struct camera_rtp_h264_span_v1 spans[CAMERA_SPAN_HUB_V1_CHUNKS];
 size_t count;
 if(!slice_spans(h,spans,&count)||v->span_count!=count)return 0;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)
  if(v->spans[i].data!=spans[i].data||v->spans[i].size!=spans[i].size)return 0;
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
 if(h->writer_phase==CAMERA_SPAN_WRITER_V3_ARMED)
  (void)hold_writer(h,CAMERA_NOT_READY);
 return CAMERA_OK;
}

int camera_span_hub_drained_v1(const struct camera_span_hub_v1 *h)
{return usable(h)&&h->phase==CAMERA_SPAN_HUB_V1_STOPPED&&writer_idle(h)&&
 !h->pending&&consumers_unheld(h);}

int camera_span_hub_dispose_v1(struct camera_span_hub_v1 *h)
{
 if(!h||h->self!=h||h->layout_version!=CAMERA_SPAN_HUB_LAYOUT_VERSION)
  return CAMERA_INVALID;
 if(h->phase==CAMERA_SPAN_HUB_V1_INIT_FAILED){
  if(h->generation||h->pending||!consumers_unheld(h)||h->bytes||h->data_offset||
     !writer_consistent(h)||!writer_idle(h)||h->issued_writer_token)
   return CAMERA_NOT_READY;
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
