/* SPDX-License-Identifier: GPL-2.0-only */
#include "audio_packet.h"
#include <string.h>
_Static_assert(sizeof(struct am_packet)<=AM_STORAGE_CAP,"one-period static storage cap");

static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!an||!bn)return 0;
 if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
static int zero(const void *p,size_t n){const uint8_t *v=p;size_t i;for(i=0;i<n;i++)if(v[i])return 0;return 1;}
static int live(const struct am_packet *p){return p&&p->self==p&&!p->busy&&!p->disposed;}
static int disjoint(const struct am_packet *p,const void *v,size_t n)
{
 return v&&!overlap(p,sizeof(*p),v,n)&&!overlap(p->capture,sizeof(*p->capture),v,n)&&
  (!p->capture->ring||!overlap(p->capture->ring,AC_RING_BYTES,v,n))&&
  !overlap(p->admission_context,1,v,n);
}
static int key_equal(const struct am_key *a,const struct am_key *b)
{return a->stream_namespace==b->stream_namespace&&a->stream_generation==b->stream_generation&&
 a->packet_serial==b->packet_serial&&a->track_generation==b->track_generation&&ac_ticket_equal(&a->capture_ticket,&b->capture_ticket);}
static int chunk_equal(const struct am_chunk *a,const struct am_chunk *b)
{return key_equal(&a->key,&b->key)&&a->chunk_serial==b->chunk_serial&&a->offset==b->offset&&a->bytes==b->bytes&&a->effect==b->effect;}
static int deny(struct am_packet *p,int error)
{
 p->invalid=1;p->offered_valid=0;memset(p->codes,0,sizeof(p->codes));memset(p->header,0,sizeof(p->header));return error;
}
static int source_current(const struct am_packet *p)
{
 const struct ac_core *c=p->capture;uint64_t pts;
 return ac_span_valid(c,&p->span)==AC_OK&&c->params.rate==8000&&c->params.channels==1&&c->params.sample_bits==16&&
  p->span.bytes==512&&p->span.frames==256&&p->span.ticket.namespace_epoch==c->namespace_epoch&&
  p->span.ticket.generation==c->generation&&p->span.ticket.resource_epoch==c->clock.epoch&&
  p->span.track_generation==c->track_generation&&p->span.rate.numerator==c->clock.parent_hz&&
  ac_pts(c,p->span.sample_index,&pts)==AC_OK&&pts==p->span.monotonic_ns;
}
static void borrow_identity(const struct am_packet *p,struct am_borrow_identity *id)
{
 const struct ac_core *c=p->capture;
 memset(id,0,sizeof(*id));id->stream_namespace=p->key.stream_namespace;id->stream_generation=p->key.stream_generation;
 id->capture_namespace=c->namespace_epoch;id->capture_generation=c->generation;id->resource_epoch=c->clock.epoch;
 id->track_generation=c->track_generation;id->parent_hz=c->clock.parent_hz;
 id->rate_hz=c->params.rate;id->channels=c->params.channels;id->sample_bits=c->params.sample_bits;
}
static int borrow_source_current(const struct am_packet *p,const struct am_borrow_identity *id)
{
 struct am_borrow_identity current;
 const struct ac_core *c=p->capture;
 if(c->self!=c||c->busy||c->closing||c->fault||c->muted||c->state!=AC_CAPTURING||
    !c->namespace_epoch||!c->generation||!c->clock.epoch||!c->track_generation||
    c->params.rate!=8000||c->params.channels!=1||c->params.sample_bits!=16)return 0;
 borrow_identity(p,&current);return !memcmp(&current,id,sizeof(current));
}
static int check_authority(struct am_packet *p,const struct am_borrow_identity *before,uint64_t now,
 uint32_t effect,uint32_t offset,uint32_t bytes,uint64_t *checked_now)
{
 uint64_t start,middle,end,until=0;int admitted,ret=AM_DENIED;
 p->busy=1;start=p->clock_now(p->admission_context);
 if(start<now||start<p->last_ns||start<p->capture->last_ns){ret=AM_CLOCK;goto done;}
 admitted=before?p->preborrow(p->admission_context,before,start,&until):
  p->admit(p->admission_context,&p->key,start,effect,offset,bytes,&until);
 middle=p->clock_now(p->admission_context);
 if(middle<start||middle<p->capture->last_ns){ret=AM_CLOCK;goto done;}
 if(admitted!=1||middle-start>AM_CALLBACK_BOUND_NS||middle>=until||
    !(before?borrow_source_current(p,before):source_current(p)))goto done;
 /* A grant checked at callback entry is insufficient for a later effect. */
 until=0;
 admitted=before?p->preborrow(p->admission_context,before,middle,&until):
  p->admit(p->admission_context,&p->key,middle,effect,offset,bytes,&until);
 end=p->clock_now(p->admission_context);
 if(end<middle||end<p->capture->last_ns){ret=AM_CLOCK;goto done;}
 if(admitted==1&&end-start<=AM_CALLBACK_BOUND_NS&&end<until&&
    (before?borrow_source_current(p,before):source_current(p))){*checked_now=end;ret=AM_OK;}
done:p->busy=0;return ret;
}
static int guard(struct am_packet *p,uint64_t now,uint32_t effect,uint32_t offset,uint32_t bytes)
{
 int ret;uint64_t checked;
 if(now<p->last_ns||now<p->capture->last_ns){p->clock_failed=1;return deny(p,AM_CLOCK);}
 if(!p->owned||p->invalid||p->clock_failed||!source_current(p))return deny(p,AM_DENIED);
 ret=check_authority(p,NULL,now,effect,offset,bytes,&checked);
 if(ret){if(ret==AM_CLOCK)p->clock_failed=1;return deny(p,ret);}
 p->last_ns=checked;return AM_OK;
}
int am_format_select(uint32_t law,struct am_format *format)
{
 struct am_format f;
 if(!format||(law!=G711_MULAW&&law!=G711_ALAW))return AM_FORMAT;
 f.law=law;f.payload_type=law==G711_MULAW?0U:8U;f.clock_hz=8000;f.channels=1;*format=f;return AM_OK;
}
int am_init(struct am_packet *p,struct ac_core *c,am_preborrow_fn preborrow,am_admit_fn admit,am_clock_fn clock_now,void *context,
 const struct am_seed *seed,uint32_t law,uint64_t now)
{
 struct am_format format;
 if(!p||!c||!seed||!context||!preborrow||!admit||!clock_now||c->self!=c||!zero(p,sizeof(*p))||
    overlap(p,sizeof(*p),c,sizeof(*c))||overlap(p,sizeof(*p),seed,sizeof(*seed))||
    overlap(p,sizeof(*p),context,1)||!seed->stream_namespace||!seed->stream_generation||!seed->ssrc||
    am_format_select(law,&format)!=AM_OK||now<c->last_ns)return AM_INVALID;
 p->self=p;p->capture=c;p->admit=admit;p->preborrow=preborrow;p->clock_now=clock_now;p->admission_context=context;p->law=law;p->ssrc=seed->ssrc;
 p->rtp_base=seed->rtp_timestamp;p->sequence_ext=seed->sequence;p->next_chunk_serial=1;p->last_ns=now;
 p->key.stream_namespace=seed->stream_namespace;p->key.stream_generation=seed->stream_generation;return AM_OK;
}
static void put16(uint8_t *b,uint16_t v){b[0]=(uint8_t)(v>>8);b[1]=(uint8_t)v;}
static void put32(uint8_t *b,uint32_t v){b[0]=(uint8_t)(v>>24);b[1]=(uint8_t)(v>>16);b[2]=(uint8_t)(v>>8);b[3]=(uint8_t)v;}
int am_begin(struct am_packet *p,struct am_key *key,uint64_t now)
{
 uint64_t delta,pts,checked;struct am_format format;struct am_borrow_identity before;int ret;
 if(!live(p)||!disjoint(p,key,sizeof(*key)))return AM_INVALID;
 if(p->owned)return AM_BUSY;
 if(p->clock_failed||p->invalid)return AM_DENIED;
 if(p->packet_serial==UINT64_MAX||p->sequence_ext==UINT64_MAX)return AM_EXHAUSTED;
 if(p->capture->params.rate!=8000||p->capture->params.channels!=1||p->capture->params.sample_bits!=16)return AM_FORMAT;
 borrow_identity(p,&before);
 if(!borrow_source_current(p,&before))return AM_DENIED;
 ret=check_authority(p,&before,now,0,0,0,&checked);if(ret)return ret; /* no ticket/no loan/no counters */
 ret=ac_borrow(p->capture,&p->span);if(ret!=AC_OK)return AM_DENIED;
 p->last_ns=checked;
 p->owned=1;p->return_attempted=0;p->cursor=0;p->offered_valid=0;p->packet_serial++;
 p->key.packet_serial=p->packet_serial;p->key.capture_ticket=p->span.ticket;p->key.track_generation=p->span.track_generation;
 ret=guard(p,checked,AM_ENCODE,0,AC_PERIOD_BYTES);if(ret)return ret;
 if(p->span.bytes!=512||p->span.frames!=256||p->span.rate.requested_hz!=8000||
    !p->span.rate.denominator||p->span.rate.denominator>32768||
    p->span.rate.numerator!=8000ULL*p->span.rate.denominator||p->span.monotonic_ns>p->last_ns||
    p->span.sample_index>UINT64_MAX-AC_PERIOD_FRAMES)return deny(p,AM_FORMAT);
 if(!p->have_anchor){
  p->anchor_index=p->span.sample_index;p->anchor_pts=p->span.monotonic_ns;p->resource_epoch=p->span.ticket.resource_epoch;
  p->track_generation=p->span.track_generation;p->capture_generation=p->span.ticket.generation;p->expected_index=p->span.sample_index;
  p->have_anchor=1;
 }
 if(p->span.sample_index!=p->expected_index||p->span.ticket.resource_epoch!=p->resource_epoch||
    p->span.ticket.generation!=p->capture_generation||p->span.track_generation!=p->track_generation)return deny(p,AM_STALE);
 delta=p->span.sample_index-p->anchor_index;
 if(delta>UINT64_MAX/125000ULL||UINT64_MAX-p->anchor_pts<delta*125000ULL||
    UINT64_MAX-p->rtp_base<delta)return deny(p,AM_EXHAUSTED);
 pts=p->anchor_pts+delta*125000ULL;
 if(pts!=p->span.monotonic_ns)return deny(p,AM_CLOCK);
 p->rtp_ext=(uint64_t)p->rtp_base+delta;
 if(g711_encode_s16le(p->law,p->span.data,p->span.bytes,p->codes,sizeof(p->codes)))return deny(p,AM_FORMAT);
 if(am_format_select(p->law,&format)!=AM_OK)return deny(p,AM_FORMAT);
 p->header[0]=0x80;p->header[1]=(uint8_t)format.payload_type;
 put16(p->header+2,(uint16_t)p->sequence_ext);put32(p->header+4,(uint32_t)p->rtp_ext);put32(p->header+8,p->ssrc);
 *key=p->key;return AM_OK;
}
int am_offer(struct am_packet *p,const struct am_key *key,uint64_t now,uint8_t *out,size_t capacity,struct am_chunk *chunk)
{
 uint32_t remaining,bytes,effect;int ret;
 if(!live(p)||!disjoint(p,key,sizeof(*key))||!disjoint(p,chunk,sizeof(*chunk))||
    !disjoint(p,out,capacity)||!capacity||capacity>AM_EFFECT_MAX||overlap(out,capacity,key,sizeof(*key))||
    overlap(out,capacity,chunk,sizeof(*chunk)))return AM_INVALID;
 if(!p->owned||!key_equal(key,&p->key))return AM_STALE;
 if(p->offered_valid)return AM_BUSY;
 if(p->cursor==AM_WIRE_BYTES)return AM_COMPLETE;
 if(!p->next_chunk_serial||p->next_chunk_serial==UINT64_MAX)return deny(p,AM_EXHAUSTED);
 effect=p->cursor<AM_HEADER_BYTES?AM_HEADER:AM_PAYLOAD;
 remaining=(effect==AM_HEADER?AM_HEADER_BYTES:AM_WIRE_BYTES)-p->cursor;
 bytes=remaining<capacity?remaining:(uint32_t)capacity;
 ret=guard(p,now,effect,p->cursor,bytes);if(ret)return ret;
 memset(&p->offered,0,sizeof(p->offered));p->offered.key=p->key;p->offered.chunk_serial=p->next_chunk_serial++;
 p->offered.offset=p->cursor;p->offered.bytes=bytes;p->offered.effect=effect;
 memcpy(out,effect==AM_HEADER?p->header+p->cursor:p->codes+p->cursor-AM_HEADER_BYTES,bytes);
 p->offered_valid=1;*chunk=p->offered;return AM_OK;
}
int am_recheck(struct am_packet *p,const struct am_chunk *chunk,uint64_t now)
{
 if(!live(p)||!disjoint(p,chunk,sizeof(*chunk)))return AM_INVALID;
 if(!p->owned||!p->offered_valid||!chunk_equal(chunk,&p->offered))return AM_STALE;
 return guard(p,now,AM_QUEUED_SEND,chunk->offset,chunk->bytes);
}
int am_advance(struct am_packet *p,const struct am_chunk *chunk,uint32_t actual,uint64_t now)
{
 int ret;
 if(!live(p)||!disjoint(p,chunk,sizeof(*chunk)))return AM_INVALID;
 if(!p->owned||!p->offered_valid||!chunk_equal(chunk,&p->offered))return AM_STALE;
 if(actual>chunk->bytes)return AM_INVALID;
 ret=guard(p,now,AM_QUEUED_SEND,chunk->offset,actual);if(ret)return ret;
 if(!actual)return AM_OK; /* unchanged offer; next continuation still rechecks */
 p->cursor+=actual;p->offered_valid=0;
 return p->cursor==AM_WIRE_BYTES?AM_COMPLETE:AM_OK;
}
static int return_loan(struct am_packet *p,uint64_t now)
{
 int ret;
 if(!p->owned)return AM_STALE;
 if(p->return_attempted)return AM_RETAINED;
 /* A media timestamp regression must NOT invoke capture's clock-fault STOP. */
 if(p->capture->self!=p->capture||now<p->capture->last_ns)return AM_CLOCK;
 p->return_attempted=1;ret=ac_return(p->capture,&p->span.ticket,now);
 if(ret!=AC_OK&&ret!=AC_RETAINED)return deny(p,AM_RETAINED);
 p->owned=0;p->offered_valid=0;memset(p->header,0,sizeof(p->header));memset(p->codes,0,sizeof(p->codes));
 return AM_OK; /* not evidence of source DMA/code retirement */
}
int am_done(struct am_packet *p,const struct am_key *key,uint64_t now)
{
 int ret;
 if(!live(p)||!disjoint(p,key,sizeof(*key)))return AM_INVALID;
 if(!p->owned||!key_equal(key,&p->key))return AM_STALE;
 if(p->invalid||p->cursor!=AM_WIRE_BYTES||p->offered_valid)return AM_BUSY;
 ret=return_loan(p,now);if(ret)return ret;
 p->expected_index+=AC_PERIOD_FRAMES;p->sequence_ext++;return AM_OK;
}
int am_cancel(struct am_packet *p,uint64_t now)
{
 if(!live(p))return AM_INVALID;
 /* Trusted transport discards ALL queued copies before calling this. No
  * socket EOF interpretation, source STOP/RELEASE or implicit resume/retry. */
 deny(p,AM_DENIED);return return_loan(p,now);
}
int am_dispose(struct am_packet *p,uint64_t now)
{
 int ret;
 if(!live(p))return AM_INVALID;
 if(p->owned){ret=am_cancel(p,now);if(ret)return ret;}
 p->disposed=1;return AM_OK;
}
