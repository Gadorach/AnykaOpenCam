/* SPDX-License-Identifier: GPL-2.0-only */
#include "capture_core.h"
#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
 ac_uptr x=(ac_uptr)a,y=(ac_uptr)b,maximum=~(ac_uptr)0;
 if(!an||!bn)return 0;
 if(!a||!b||an>maximum-x||bn>maximum-y)return 1;
 return x<y+bn&&y<x+an;
}
static int zero(const void *p,size_t n)
{const ac_u8 *v=p;size_t i;for(i=0;i<n;i++)if(v[i])return 0;return 1;}
static int live(const struct ac_core *c){return c&&c->self==c&&!c->busy;}
static int disjoint(const struct ac_core *c,const void *p,size_t n)
{return p&&!overlap(c,sizeof(*c),p,n)&&(!c->ring||!overlap(c->ring,AC_RING_BYTES,p,n))&&
 (!c->provider.context||!overlap(c->provider.context,1,p,n));}
int ac_ticket_equal(const struct ac_ticket *a,const struct ac_ticket *b)
{return a&&b&&a->namespace_epoch==b->namespace_epoch&&a->generation==b->generation&&
 a->sequence==b->sequence&&a->resource_epoch==b->resource_epoch;}
int ac_rate_plan(const struct ac_params *p,const struct ac_clock *clock,struct ac_rate *out)
{
 struct ac_rate plan;ac_u32 d,osr;
 if(!p||!clock||!out||overlap(out,sizeof(*out),p,sizeof(*p))||overlap(out,sizeof(*out),clock,sizeof(*clock)))return AC_INVALID;
 if(p->channels!=1||p->sample_bits!=16||p->period_bytes!=AC_PERIOD_BYTES||p->periods!=AC_SLOTS||
    (p->rate!=8000&&p->rate!=16000&&p->rate!=24000&&p->rate!=48000))return AC_INVALID;
 if(clock->ready_mask!=AC_RESOURCE_REQUIRED||!clock->parent_hz||!clock->epoch||clock->mic_gain_code>7)return AC_UNAVAILABLE;
 /* Anyka get_adc2_osr_div: <=24k OSR512, >24k OSR256; divider1..64.
  * Do NOT retune shared PLL or round an unmeasured parent into a nominal rate. */
 osr=p->rate>24000?256U:512U;
 for(d=1;d<=64;d++)if(clock->parent_hz==(ac_u64)p->rate*osr*d)break;
 if(d==65)return AC_UNAVAILABLE;
 memset(&plan,0,sizeof(plan));plan.numerator=clock->parent_hz;plan.denominator=osr*d;
 plan.osr=osr;plan.divider=d;plan.requested_hz=p->rate;*out=plan;return AC_OK;
}
int ac_init(struct ac_core *c,const struct ac_provider *p,ac_u64 space,ac_u64 now)
{
 if(!c||!p||!space||overlap(c,sizeof(*c),p,sizeof(*p))||!zero(c,sizeof(*c))||
    p->abi!=AC_ABI||p->struct_bytes!=sizeof(*p)||!p->context||!p->submit||overlap(c,sizeof(*c),p->context,1))return AC_INVALID;
 c->self=c;c->provider=*p;c->namespace_epoch=space;c->next_sequence=1;c->track_generation=1;c->last_ns=now;return AC_OK;
}
static int observe(struct ac_core *c,ac_u64 now)
{if(now<c->last_ns){c->fault=1;return AC_FAULT;}c->last_ns=now;return AC_OK;}
static int allocate_tickets(struct ac_core *c,struct ac_ticket *tickets,ac_u32 count)
{
 ac_u32 i;
 if(!c->next_sequence||c->next_sequence>AC_MAX64-count)return AC_EXHAUSTED;
 for(i=0;i<count;i++){
  tickets[i].namespace_epoch=c->namespace_epoch;tickets[i].generation=c->generation;
  tickets[i].resource_epoch=c->clock.epoch;tickets[i].sequence=c->next_sequence++;
 }
 return AC_OK;
}
static int submit(struct ac_core *c,ac_u32 op,const struct ac_ticket *ticket)
{
 struct ac_request q;int result;
 memset(&q,0,sizeof(q));q.ticket=*ticket;q.work=c->work;q.operation=op;q.resource_mask=AC_RESOURCE_REQUIRED;
 q.rate=c->rate;q.slot=c->slot_index;q.offset=c->slot_index*AC_PERIOD_BYTES;
 if(op==AC_CACHE_DEVICE||op==AC_DMA_PERIOD||op==AC_CACHE_CPU)q.bytes=AC_PERIOD_BYTES;
 q.deadline_ns=AC_MAX64-c->last_ns<AC_STAGE_BOUND_NS?AC_MAX64:c->last_ns+AC_STAGE_BOUND_NS;
 c->deadline_ns=q.deadline_ns;c->pending=*ticket;c->operation=op;c->retained=1;
 c->busy=1;result=c->provider.submit(c->provider.context,&q);c->busy=0;
 return result==AC_ACCEPTED||result==AC_NO_EFFECT?result:AC_UNCERTAIN;
}
static void finish(struct ac_core *c)
{
 c->state=AC_CLOSED;c->retained=0;c->ring=NULL;c->operation=0;
 memset(&c->pending,0,sizeof(c->pending));
}
static int release_owned(struct ac_core *c)
{
 int result;
 if(!c->stop_acked||c->borrowed)return AC_RETAINED;
 if(c->release_sent)return AC_RETAINED;
 c->release_sent=1;c->state=AC_RELEASING;result=submit(c,AC_RELEASE,&c->release);
 if(result!=AC_ACCEPTED){c->fault=1;c->state=AC_QUARANTINED;if(result==AC_NO_EFFECT)c->terminal=1;}
 return AC_RETAINED;
}
static int stop_owned(struct ac_core *c,int fault)
{
 ac_u32 i;int result;
 if(fault)c->fault=1;
 c->closing=1;
 for(i=0;i<AC_SLOTS;i++)if(c->slots[i].state==AC_SLOT_READY)c->slots[i].state=AC_SLOT_FREE;
 if(!c->retained){finish(c);return AC_OK;}
 if(c->stop_acked)return release_owned(c);
 if(c->stop_sent)return AC_RETAINED;
 c->stop_sent=1;c->state=AC_STOPPING;result=submit(c,AC_STOP,&c->stop);
 if(result!=AC_ACCEPTED){c->fault=1;c->state=AC_QUARANTINED;if(result==AC_NO_EFFECT)c->terminal=1;}
 return AC_RETAINED;
}
int ac_prepare(struct ac_core *c,const struct ac_params *p,const struct ac_clock *clock,
 ac_u8 *ring,size_t bytes,ac_u64 generation,ac_u64 now)
{
 struct ac_rate rate;struct ac_ticket t[5];int ret;
 if(!live(c)||!disjoint(c,p,sizeof(*p))||!disjoint(c,clock,sizeof(*clock))||!ring||
    overlap(c,sizeof(*c),ring,bytes)||overlap(p,sizeof(*p),ring,bytes)||overlap(clock,sizeof(*clock),ring,bytes)||
    ((ac_uptr)ring&63U)||bytes!=AC_RING_BYTES)return AC_INVALID;
 if(c->retained||c->borrowed||c->state!=AC_CLOSED)return AC_BUSY;
 if(c->fault)return AC_FAULT;
 if(!generation||generation<=c->generation)return AC_STALE;
 ret=ac_rate_plan(p,clock,&rate);if(ret)return ret;
 if(c->next_sequence>AC_MAX64-5||!c->next_sequence)return AC_EXHAUSTED;
 if(observe(c,now))return AC_FAULT;
 c->generation=generation;c->clock=*clock;c->params=*p;c->rate=rate;c->ring=ring;
 c->closing=c->stop_sent=c->stop_acked=c->release_sent=c->terminal=c->have_epoch=0;
 c->slot_index=0;c->next_sample_index=0;c->last_irq_ns=0;
 memset(c->slots,0,sizeof(c->slots));
 ret=allocate_tickets(c,t,5);if(ret)return ret;
 c->work=t[0];c->configure=t[2];c->stop=t[3];c->release=t[4];c->state=AC_CLAIMING;
 ret=submit(c,AC_CLAIM,&t[1]);
 if(ret==AC_NO_EFFECT){c->retained=0;c->fault=1;finish(c);return AC_FAULT;}
 if(ret!=AC_ACCEPTED){stop_owned(c,1);return AC_RETAINED;}
 return AC_OK;
}
int ac_pts(const struct ac_core *c,ac_u64 sample_index,ac_u64 *out)
{
 ac_u64 whole,remainder,delta;
 if(!c||c->self!=c||!out||!disjoint(c,out,sizeof(*out))||!c->have_epoch||!c->params.rate)return AC_INVALID;
 whole=sample_index/c->params.rate;remainder=sample_index%c->params.rate;
 if(whole>AC_MAX64/1000000000ULL)return AC_EXHAUSTED;
 delta=whole*1000000000ULL+(remainder*1000000000ULL)/c->params.rate;
 if(delta<whole*1000000000ULL||AC_MAX64-c->epoch_ns<delta)return AC_EXHAUSTED;
 *out=c->epoch_ns+delta;return AC_OK;
}
int ac_pump(struct ac_core *c,ac_u64 now)
{
 struct ac_ticket t[3];int ret;
 if(!live(c))return AC_INVALID;
 if(observe(c,now)){
  if(c->operation==AC_STOP||c->operation==AC_RELEASE){c->state=AC_QUARANTINED;return AC_RETAINED;}
  return stop_owned(c,1);
 }
 /* Owner-driven tick MUST cover init and teardown, not only PCM periods.
  * Exact pending operation remains live after expiry; timeout is not ACK.
  * Cleanup timeouts quarantine without retry/free. Normal late exact fences
  * may still retire uncertainty; definitive failure remains terminal. */
 if(c->operation&&now>=c->deadline_ns){
  if(c->operation==AC_STOP||c->operation==AC_RELEASE){
   c->fault=1;c->state=AC_QUARANTINED;return AC_RETAINED;
  }
  if(c->operation>=AC_CACHE_DEVICE&&c->operation<=AC_CACHE_CPU)c->xrun=1;
  return stop_owned(c,1);
 }
 if(c->closing||c->fault)return AC_RETAINED;
 if(c->state!=AC_CAPTURING)return AC_BUSY;
 if(c->operation)return AC_BUSY;
 if(c->slots[c->slot_index].state!=AC_SLOT_FREE){c->xrun=1;return stop_owned(c,1);}
 ret=allocate_tickets(c,t,3);if(ret){stop_owned(c,1);return ret;}
 c->device=t[0];c->dma=t[1];c->cpu=t[2];c->slots[c->slot_index].state=AC_SLOT_DEVICE;
 ret=submit(c,AC_CACHE_DEVICE,&c->device);
 if(ret!=AC_ACCEPTED){stop_owned(c,1);return AC_RETAINED;}
 return AC_OK;
}
int ac_start(struct ac_core *c,ac_u64 now)
{
 if(!live(c))return AC_INVALID;
 if(c->state!=AC_PREPARED||c->closing||c->fault||c->muted)return AC_UNAVAILABLE;
 c->state=AC_CAPTURING;return ac_pump(c,now);
}
int ac_complete(struct ac_core *c,const struct ac_completion *r,ac_u64 now)
{
 int ret;ac_u64 pts;struct ac_slot *slot;
 if(!live(c)||!disjoint(c,r,sizeof(*r)))return AC_INVALID;
 if(!ac_ticket_equal(&r->ticket,&c->pending)||r->operation!=c->operation||!c->operation)return AC_STALE;
 if(observe(c,now))stop_owned(c,1);
 if(!ac_ticket_equal(&r->ticket,&c->pending))return AC_STALE;
 if(r->status!=AC_ACK){if(c->operation==AC_STOP||c->operation==AC_RELEASE){
   c->fault=1;c->state=AC_QUARANTINED;if(r->status==AC_FAILED)c->terminal=1;return AC_RETAINED;
  }return stop_owned(c,1);}
 if(c->terminal)return AC_STALE;
 if(now>=c->deadline_ns)c->fault=1;
 if(c->fault&&c->operation!=AC_STOP&&c->operation!=AC_RELEASE)return stop_owned(c,1);
 switch(c->operation){
 case AC_CLAIM:
  c->state=AC_CONFIGURING;ret=submit(c,AC_CONFIGURE,&c->configure);
  if(ret!=AC_ACCEPTED)return stop_owned(c,1);
  return AC_OK;
 case AC_CONFIGURE:c->state=AC_PREPARED;c->operation=0;return c->fault?stop_owned(c,1):AC_OK;
 case AC_CACHE_DEVICE:
  if(c->fault)return stop_owned(c,1);
  c->slots[c->slot_index].state=AC_SLOT_DMA;ret=submit(c,AC_DMA_PERIOD,&c->dma);
  return ret==AC_ACCEPTED?AC_OK:stop_owned(c,1);
 case AC_DMA_PERIOD:
  if(c->fault||r->bytes!=AC_PERIOD_BYTES||r->timestamp_valid!=1||r->sample_index!=c->next_sample_index||
     r->irq_ns>now||r->irq_ns<c->last_irq_ns||r->first_sample_ns>r->irq_ns||
     AC_MAX64-c->next_sample_index<AC_PERIOD_FRAMES){c->xrun=1;return stop_owned(c,1);}
  if(!c->have_epoch){c->epoch_ns=r->first_sample_ns;c->have_epoch=1;}
  ret=ac_pts(c,c->next_sample_index,&pts);
  if(ret||pts!=r->first_sample_ns){c->xrun=1;return stop_owned(c,1);}
  c->last_irq_ns=r->irq_ns;slot=&c->slots[c->slot_index];slot->sample_index=c->next_sample_index;slot->pts_ns=pts;
  c->next_sample_index+=AC_PERIOD_FRAMES;slot->state=AC_SLOT_CPU;ret=submit(c,AC_CACHE_CPU,&c->cpu);
  return ret==AC_ACCEPTED?AC_OK:stop_owned(c,1);
 case AC_CACHE_CPU:
  if(c->fault)return stop_owned(c,1);
  slot=&c->slots[c->slot_index];slot->ticket=c->dma;slot->state=AC_SLOT_READY;
  c->slot_index=(c->slot_index+1)%AC_SLOTS;c->operation=0;return AC_OK;
 case AC_STOP:
  c->stop_acked=1;c->operation=0;c->state=AC_WAIT_BORROW;
  return release_owned(c);
 case AC_RELEASE:
  if(c->borrowed||!c->stop_acked)return stop_owned(c,1);
  finish(c);return AC_OK;
 default:return stop_owned(c,1);
 }
}
int ac_close(struct ac_core *c,ac_u64 now)
{if(!live(c))return AC_INVALID;if(observe(c,now))c->fault=1;return stop_owned(c,0);}
int ac_source_fault(struct ac_core *c,ac_u64 now)
{if(!live(c))return AC_INVALID;if(observe(c,now))c->fault=1;return stop_owned(c,1);}
int ac_mute(struct ac_core *c,ac_u32 enabled,ac_u64 now)
{
 if(!live(c)||enabled>1)return AC_INVALID;
 if(enabled==c->muted)return AC_OK;
 if(c->track_generation==AC_MAX64)return stop_owned(c,1);
 if(!enabled&&(c->retained||c->borrowed))return AC_BUSY;
 c->track_generation++;c->muted=enabled;
 return enabled?ac_close(c,now):AC_OK;
}
int ac_borrow(struct ac_core *c,struct ac_span *out)
{
 ac_u32 i,best=AC_SLOTS;struct ac_span span;
 if(!live(c)||!disjoint(c,out,sizeof(*out)))return AC_INVALID;
 if(c->closing||c->muted||c->fault)return AC_UNAVAILABLE;
 for(i=0;i<AC_SLOTS;i++)if(c->slots[i].state==AC_SLOT_READY&&(best==AC_SLOTS||c->slots[i].sample_index<c->slots[best].sample_index))best=i;
 if(best==AC_SLOTS)return AC_EMPTY;
 memset(&span,0,sizeof(span));span.ticket=c->slots[best].ticket;span.slot=best;span.data=c->ring+best*AC_PERIOD_BYTES;
 span.bytes=AC_PERIOD_BYTES;span.frames=AC_PERIOD_FRAMES;span.sample_index=c->slots[best].sample_index;
 span.monotonic_ns=c->slots[best].pts_ns;span.track_generation=c->track_generation;span.rate=c->rate;span.evidence=AC_PRESENT_UNQUALIFIED;
 c->slots[best].state=AC_SLOT_BORROWED;c->borrowed++;*out=span;return AC_OK;
}
int ac_span_valid(const struct ac_core *c,const struct ac_span *s)
{
 if(!live(c)||!disjoint(c,s,sizeof(*s))||c->closing||c->muted||c->fault||s->slot>=AC_SLOTS)return AC_UNAVAILABLE;
 return c->slots[s->slot].state==AC_SLOT_BORROWED&&ac_ticket_equal(&s->ticket,&c->slots[s->slot].ticket)&&
  s->track_generation==c->track_generation&&s->data==c->ring+s->slot*AC_PERIOD_BYTES&&
  s->bytes==AC_PERIOD_BYTES&&s->frames==AC_PERIOD_FRAMES&&s->sample_index==c->slots[s->slot].sample_index&&
  s->monotonic_ns==c->slots[s->slot].pts_ns&&s->rate.numerator==c->rate.numerator&&
  s->rate.denominator==c->rate.denominator&&s->rate.osr==c->rate.osr&&s->rate.divider==c->rate.divider&&
  s->rate.requested_hz==c->rate.requested_hz&&!s->native_qualified&&s->evidence==AC_PRESENT_UNQUALIFIED?AC_OK:AC_STALE;
}
int ac_return(struct ac_core *c,const struct ac_ticket *t,ac_u64 now)
{
 ac_u32 i;
 if(!live(c)||!disjoint(c,t,sizeof(*t)))return AC_INVALID;
 for(i=0;i<AC_SLOTS;i++)if(c->slots[i].state==AC_SLOT_BORROWED&&ac_ticket_equal(t,&c->slots[i].ticket))break;
 if(i==AC_SLOTS)return AC_STALE;
 if(observe(c,now))stop_owned(c,1);
 c->slots[i].state=AC_SLOT_FREE;c->borrowed--;
 if(c->closing&&c->stop_acked)return release_owned(c);
 return AC_OK;
}
int ac_can_free(const struct ac_core *c)
{return c&&c->self==c&&!c->retained&&!c->borrowed&&c->state==AC_CLOSED;}
