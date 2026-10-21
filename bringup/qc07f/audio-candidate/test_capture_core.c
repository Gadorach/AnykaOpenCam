/* SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic PCM patterns in RAM only; NO microphone/audio files/hardware. */
#include "capture_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,suites;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}} while(0)
#define BASE 1000000000ULL
struct fake {struct ac_request request[128];unsigned count,reenter;int result,reentered;struct ac_core *core;};
struct fixture {
 struct ac_core core;struct fake fake;struct ac_provider provider;struct ac_params params;struct ac_clock clock;
 _Alignas(64) ac_u8 ring[AC_RING_BYTES+64];ac_u64 now;
};
static int submit(void *ctx,const struct ac_request *q)
{
 struct fake *f=ctx;CHECK(f->count<128);f->request[f->count++]=*q;
 if(f->reenter){f->reenter=0;f->reentered=ac_close(f->core,f->core->last_ns);}
 return f->result;
}
static void setup(struct fixture *f)
{
 memset(f,0,sizeof(*f));f->now=BASE;f->fake.core=&f->core;
 f->provider.abi=AC_ABI;f->provider.struct_bytes=sizeof(f->provider);f->provider.context=&f->fake;f->provider.submit=submit;
 f->params.rate=16000;f->params.channels=1;f->params.sample_bits=16;f->params.period_bytes=512;f->params.periods=4;
 f->clock.parent_hz=98304000;f->clock.epoch=1;f->clock.ready_mask=AC_RESOURCE_REQUIRED;f->clock.mic_gain_code=6;
 memset(f->ring+AC_RING_BYTES,0xa7,64);CHECK(ac_init(&f->core,&f->provider,0x5055424c4943ULL,f->now)==AC_OK);
}
static struct ac_completion receipt(const struct ac_request *q,ac_u32 status)
{
 struct ac_completion r;memset(&r,0,sizeof(r));r.ticket=q->ticket;r.operation=q->operation;r.status=status;return r;
}
static int ack(struct fixture *f)
{
 struct ac_request q=f->fake.request[f->fake.count-1];struct ac_completion r=receipt(&q,AC_ACK);unsigned i;
 f->now++;
 if(q.operation==AC_DMA_PERIOD){
  r.bytes=512;r.timestamp_valid=1;r.sample_index=f->core.next_sample_index;
  r.first_sample_ns=BASE+(r.sample_index*1000000000ULL)/f->params.rate;
  r.irq_ns=r.first_sample_ns+(256ULL*1000000000ULL)/f->params.rate;
  if(f->now<=r.irq_ns)f->now=r.irq_ns+1;
  for(i=0;i<512;i++)f->ring[q.offset+i]=(ac_u8)(r.sample_index+i);
 }
 return ac_complete(&f->core,&r,f->now);
}
static void prepare(struct fixture *f)
{
 CHECK(ac_prepare(&f->core,&f->params,&f->clock,f->ring,AC_RING_BYTES,1,f->now)==AC_OK);
 CHECK(f->fake.request[0].operation==AC_CLAIM);CHECK(ack(f)==AC_OK);
 CHECK(f->fake.request[1].operation==AC_CONFIGURE);CHECK(ack(f)==AC_OK);
 CHECK(f->core.state==AC_PREPARED&&!f->core.native_qualified);
}
static void period(struct fixture *f,int first)
{
 CHECK((first?ac_start(&f->core,++f->now):ac_pump(&f->core,++f->now))==AC_OK);
 CHECK(f->fake.request[f->fake.count-1].operation==AC_CACHE_DEVICE);CHECK(ack(f)==AC_OK);
 CHECK(f->fake.request[f->fake.count-1].operation==AC_DMA_PERIOD);CHECK(ack(f)==AC_OK);
 CHECK(f->fake.request[f->fake.count-1].operation==AC_CACHE_CPU);CHECK(ack(f)==AC_OK);
}
static void retire(struct fixture *f)
{
 CHECK(ac_close(&f->core,++f->now)==AC_RETAINED);CHECK(f->fake.request[f->fake.count-1].operation==AC_STOP);
 CHECK(ack(f)==AC_RETAINED);CHECK(f->fake.request[f->fake.count-1].operation==AC_RELEASE);
 CHECK(ack(f)==AC_OK);CHECK(ac_can_free(&f->core));
}
static void rate_and_admission(void)
{
 struct fixture f;struct ac_rate r;struct ac_core before;unsigned i;const unsigned rates[]={8000,16000,24000,48000};
 suites++;setup(&f);
 for(i=0;i<4;i++){f.params.rate=rates[i];CHECK(ac_rate_plan(&f.params,&f.clock,&r)==AC_OK);
  CHECK(r.numerator==(ac_u64)rates[i]*r.denominator&&r.divider>=1&&r.divider<=64);}
 for(i=0;i<10;i++){
  setup(&f);memcpy(&before,&f.core,sizeof(before));
  switch(i){
  case 0:f.params.rate=256000;break;case 1:f.params.channels=2;break;
  case 2:f.params.sample_bits=8;break;case 3:f.params.period_bytes=1024;break;
  case 4:f.params.periods=128;break;case 5:f.clock.parent_hz=0;break;
  case 6:f.clock.ready_mask=AC_RESOURCE_REQUIRED-1;break;case 7:f.clock.epoch=0;break;
  case 8:f.clock.mic_gain_code=8;break;case 9:f.clock.parent_hz=160000000;break;
  }
  CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,1,f.now)<0);
  CHECK(!f.fake.count&&!memcmp(&before,&f.core,sizeof(before)));
 }
}
static void capture_periods_pts_borrow(void)
{
 struct fixture f;struct ac_span s;struct ac_span changed;ac_u64 pts;unsigned rate,i,j;
 suites++;
 for(rate=0;rate<4;rate++){
  const unsigned rates[]={8000,16000,24000,48000};setup(&f);f.params.rate=rates[rate];prepare(&f);
  for(i=0;i<16;i++){
   period(&f,i==0);CHECK(ac_borrow(&f.core,&s)==AC_OK);CHECK(ac_span_valid(&f.core,&s)==AC_OK);
   CHECK(s.bytes==512&&s.frames==256&&s.sample_index==(ac_u64)i*256&&!s.native_qualified);
   CHECK(s.monotonic_ns==BASE+((ac_u64)i*256*1000000000ULL)/rates[rate]);
   CHECK(ac_pts(&f.core,s.sample_index,&pts)==AC_OK&&pts==s.monotonic_ns);
   for(j=0;j<512;j++)CHECK(s.data[j]==(ac_u8)(s.sample_index+j));
   changed=s;changed.rate.denominator++;CHECK(ac_span_valid(&f.core,&changed)==AC_STALE);
   CHECK(ac_return(&f.core,&s.ticket,++f.now)==AC_OK);CHECK(ac_return(&f.core,&s.ticket,f.now)==AC_STALE);
   for(j=0;j<64;j++)CHECK(f.ring[AC_RING_BYTES+j]==0xa7);
  }
  retire(&f);CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,1,++f.now)==AC_STALE);
  CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,2,f.now)==AC_OK);
  CHECK(ac_close(&f.core,++f.now)==AC_RETAINED);CHECK(ack(&f)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
 }
}
static void early_dma_late_irq_lost_period(void)
{
 struct fixture f;struct ac_completion r;struct ac_core before;unsigned i;
 suites++;setup(&f);prepare(&f);CHECK(ac_start(&f.core,++f.now)==AC_OK);
 r=receipt(&f.fake.request[f.fake.count-1],AC_ACK);r.operation=AC_DMA_PERIOD;
 memcpy(&before,&f.core,sizeof(before));CHECK(ac_complete(&f.core,&r,f.now)==AC_STALE);
 CHECK(!memcmp(&before,&f.core,sizeof(before)));CHECK(ack(&f)==AC_OK);
 r=receipt(&f.fake.request[f.fake.count-1],AC_ACK);
 r.bytes=512;r.timestamp_valid=1;r.sample_index=256;r.first_sample_ns=BASE;r.irq_ns=f.now;
 CHECK(ac_complete(&f.core,&r,++f.now)==AC_RETAINED);CHECK(f.core.xrun&&f.core.retained);
 CHECK(ac_complete(&f.core,&r,f.now)==AC_STALE);CHECK(ack(&f)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
 CHECK(ac_complete(&f.core,&r,++f.now)==AC_STALE);CHECK(ac_can_free(&f.core));
 for(i=0;i<4;i++){
  setup(&f);prepare(&f);CHECK(ac_start(&f.core,++f.now)==AC_OK);CHECK(ack(&f)==AC_OK);
  r=receipt(&f.fake.request[f.fake.count-1],AC_ACK);r.bytes=512;r.timestamp_valid=1;
  r.first_sample_ns=BASE;r.irq_ns=f.now;
  if(i==0)r.bytes=510;
  if(i==1)r.timestamp_valid=0;
  if(i==2)r.irq_ns=f.now+100;
  if(i==3)r.first_sample_ns=r.irq_ns+1;
  CHECK(ac_complete(&f.core,&r,++f.now)==AC_RETAINED);CHECK(f.core.retained&&f.core.xrun);
 }
}
static void hold_close_mute_xrun(void)
{
 struct fixture f;struct ac_span s,empty;unsigned i,count;
 suites++;setup(&f);prepare(&f);period(&f,1);CHECK(ac_borrow(&f.core,&s)==AC_OK);
 CHECK(ac_mute(&f.core,1,++f.now)==AC_RETAINED);CHECK(ac_span_valid(&f.core,&s)==AC_UNAVAILABLE);
 CHECK(ac_borrow(&f.core,&empty)==AC_UNAVAILABLE);count=f.fake.count;
 CHECK(ac_close(&f.core,++f.now)==AC_RETAINED&&f.fake.count==count);
 CHECK(ack(&f)==AC_RETAINED);CHECK(f.core.state==AC_WAIT_BORROW&&!ac_can_free(&f.core));
 CHECK(f.core.ring==f.ring&&s.data[0]==0);CHECK(ac_return(&f.core,&s.ticket,++f.now)==AC_RETAINED);
 CHECK(f.fake.request[f.fake.count-1].operation==AC_RELEASE);CHECK(ack(&f)==AC_OK);
 CHECK(ac_can_free(&f.core)&&ac_mute(&f.core,0,++f.now)==AC_OK);
 setup(&f);prepare(&f);period(&f,1);CHECK(ac_borrow(&f.core,&s)==AC_OK);
 for(i=0;i<3;i++)period(&f,0);
 CHECK(ac_pump(&f.core,++f.now)==AC_RETAINED);CHECK(f.core.xrun&&!ac_can_free(&f.core));
 CHECK(ack(&f)==AC_RETAINED);CHECK(ac_return(&f.core,&s.ticket,++f.now)==AC_RETAINED);
 CHECK(ack(&f)==AC_OK);CHECK(ac_can_free(&f.core));
}
static void partial_init_uncertainty_resource_conflict(void)
{
 struct fixture f;struct ac_completion r;unsigned stage,count;
 suites++;
 for(stage=0;stage<3;stage++){
  setup(&f);
  if(stage==0)f.fake.result=AC_UNCERTAIN;
  CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,1,f.now)==(stage==0?AC_RETAINED:AC_OK));
  f.fake.result=AC_ACCEPTED;
  if(stage==1){r=receipt(&f.fake.request[0],AC_FAILED);CHECK(ac_complete(&f.core,&r,++f.now)==AC_RETAINED);}
  if(stage==2){CHECK(ack(&f)==AC_OK);r=receipt(&f.fake.request[1],AC_UNKNOWN);CHECK(ac_complete(&f.core,&r,++f.now)==AC_RETAINED);}
  count=f.fake.count;CHECK(!ac_can_free(&f.core)&&f.core.ring==f.ring);
  CHECK(ac_close(&f.core,++f.now)==AC_RETAINED&&f.fake.count==count);
  CHECK(ac_init(&f.core,&f.provider,2,f.now)==AC_INVALID);
  CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,2,f.now)==AC_BUSY);
  CHECK(ack(&f)==AC_RETAINED);CHECK(ack(&f)==AC_OK);CHECK(ac_can_free(&f.core));
 }
 setup(&f);f.fake.result=AC_NO_EFFECT;
 CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,1,f.now)==AC_FAULT);
 CHECK(ac_can_free(&f.core)&&f.fake.count==1); /* shared owner conflict, no private reset/release */
 setup(&f);prepare(&f);f.fake.result=AC_UNCERTAIN;
 CHECK(ac_close(&f.core,++f.now)==AC_RETAINED);count=f.fake.count;
 f.fake.result=AC_ACCEPTED;r=receipt(&f.fake.request[count-1],AC_UNKNOWN);
 CHECK(ac_complete(&f.core,&r,++f.now)==AC_RETAINED);CHECK(!ac_can_free(&f.core));
 CHECK(ac_close(&f.core,++f.now)==AC_RETAINED&&f.fake.count==count);
 CHECK(ack(&f)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
 setup(&f);prepare(&f);CHECK(ac_close(&f.core,++f.now)==AC_RETAINED);
 r=receipt(&f.fake.request[f.fake.count-1],AC_FAILED);CHECK(ac_complete(&f.core,&r,++f.now)==AC_RETAINED);
 r.status=AC_ACK;CHECK(ac_complete(&f.core,&r,++f.now)==AC_STALE);CHECK(!ac_can_free(&f.core));
}
static void clock_tokens_aliases_and_source_fault(void)
{
 struct fixture f;struct ac_core copied,before;struct ac_span span;struct ac_completion r;unsigned i;
 suites++;setup(&f);prepare(&f);CHECK(ac_start(&f.core,++f.now)==AC_OK);
 r=receipt(&f.fake.request[f.fake.count-1],AC_ACK);
 for(i=0;i<4;i++){
  struct ac_completion bad=r;
  if(i==0)bad.ticket.namespace_epoch++;
  if(i==1)bad.ticket.generation++;
  if(i==2)bad.ticket.resource_epoch++;
  if(i==3)bad.ticket.sequence++;
  memcpy(&before,&f.core,sizeof(before));CHECK(ac_complete(&f.core,&bad,f.now)==AC_STALE);
  CHECK(!memcmp(&before,&f.core,sizeof(before)));
 }
 CHECK(ac_complete(&f.core,&r,BASE-1)==AC_STALE);CHECK(f.core.fault&&!ac_can_free(&f.core));
 CHECK(ack(&f)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
 setup(&f);prepare(&f);f.fake.reenter=1;period(&f,1);CHECK(f.fake.reentered==AC_INVALID);
 CHECK(ac_borrow(&f.core,&span)==AC_OK);
 CHECK(ac_borrow(&f.core,(struct ac_span *)&f.core.slots)==AC_INVALID);
 CHECK(ac_return(&f.core,&f.core.slots[span.slot].ticket,f.now)==AC_INVALID);
 copied=f.core;CHECK(ac_close(&copied,f.now)==AC_INVALID);
 CHECK(ac_source_fault(&f.core,++f.now)==AC_RETAINED);CHECK(ac_span_valid(&f.core,&span)==AC_UNAVAILABLE);
 CHECK(ack(&f)==AC_RETAINED);CHECK(ac_return(&f.core,&span.ticket,++f.now)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
}
static void exhaustion_and_deadline(void)
{
 struct fixture f;ac_u64 pts;unsigned count;
 suites++;setup(&f);prepare(&f);f.core.next_sequence=AC_MAX64-2;
 CHECK(ac_start(&f.core,++f.now)==AC_EXHAUSTED);CHECK(f.fake.request[f.fake.count-1].operation==AC_STOP);
 CHECK(ack(&f)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
 setup(&f);prepare(&f);period(&f,1);CHECK(ac_pts(&f.core,AC_MAX64,&pts)==AC_EXHAUSTED);
 CHECK(ac_pump(&f.core,++f.now)==AC_OK);count=f.fake.count;
 f.now=f.core.deadline_ns;CHECK(ac_pump(&f.core,f.now)==AC_RETAINED);CHECK(f.fake.count==count+1&&f.core.xrun);
 CHECK(ack(&f)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
 setup(&f);prepare(&f);CHECK(ac_start(&f.core,++f.now)==AC_OK);
 f.core.next_sample_index=AC_MAX64-100;CHECK(ack(&f)==AC_OK);
 CHECK(ack(&f)==AC_RETAINED);CHECK(!ac_can_free(&f.core));
}
static void release_fence_and_repeated_cancellation(void)
{
 struct fixture f;struct ac_completion r,old;struct ac_span span;unsigned i,count;
 suites++;
 for(i=0;i<3;i++){
  setup(&f);prepare(&f);period(&f,1);CHECK(ac_borrow(&f.core,&span)==AC_OK);
  old=receipt(&f.fake.request[f.fake.count-1],AC_ACK);
  CHECK(ac_close(&f.core,++f.now)==AC_RETAINED);count=f.fake.count;
  CHECK(ac_complete(&f.core,&old,f.now)==AC_STALE);CHECK(ac_close(&f.core,++f.now)==AC_RETAINED);
  CHECK(f.fake.count==count);CHECK(ack(&f)==AC_RETAINED);
  f.fake.result=i==0?AC_UNCERTAIN:(i==1?AC_NO_EFFECT:AC_ACCEPTED);
  CHECK(ac_return(&f.core,&span.ticket,++f.now)==AC_RETAINED);count=f.fake.count;
  CHECK(!ac_can_free(&f.core)&&f.core.ring==f.ring);
  CHECK(ac_return(&f.core,&span.ticket,f.now)==AC_STALE);
  CHECK(ac_close(&f.core,++f.now)==AC_RETAINED&&f.fake.count==count);
  r=receipt(&f.fake.request[count-1],AC_ACK);
  if(i==1){CHECK(ac_complete(&f.core,&r,++f.now)==AC_STALE);CHECK(!ac_can_free(&f.core));}
  else{CHECK(ac_complete(&f.core,&r,++f.now)==AC_OK);CHECK(ac_can_free(&f.core));}
 }
 setup(&f);prepare(&f);period(&f,1);CHECK(ac_borrow(&f.core,&span)==AC_OK);
 f.core.track_generation=AC_MAX64;
 CHECK(ac_mute(&f.core,1,++f.now)==AC_RETAINED);CHECK(ac_span_valid(&f.core,&span)==AC_UNAVAILABLE);
 CHECK(ack(&f)==AC_RETAINED);CHECK(ac_return(&f.core,&span.ticket,++f.now)==AC_RETAINED);CHECK(ack(&f)==AC_OK);
 CHECK(f.core.track_generation==AC_MAX64); /* no wrap/reissued old track */
}
static void pending_stage(struct fixture *f,unsigned operation)
{
 setup(f);
 CHECK(ac_prepare(&f->core,&f->params,&f->clock,f->ring,AC_RING_BYTES,1,f->now)==AC_OK);
 if(operation==AC_CLAIM)return;
 CHECK(ack(f)==AC_OK);
 if(operation==AC_CONFIGURE)return;
 CHECK(ack(f)==AC_OK);
 if(operation==AC_STOP||operation==AC_RELEASE){
  CHECK(ac_close(&f->core,++f->now)==AC_RETAINED);
  if(operation==AC_RELEASE)CHECK(ack(f)==AC_RETAINED);
  return;
 }
 CHECK(ac_start(&f->core,++f->now)==AC_OK);
 if(operation==AC_CACHE_DEVICE)return;
 CHECK(ack(f)==AC_OK);
 if(operation==AC_DMA_PERIOD)return;
 CHECK(ack(f)==AC_OK);CHECK(operation==AC_CACHE_CPU);
}
static void all_stage_deadlines(void)
{
 struct fixture f;struct ac_completion old,stale,current;struct ac_core before;
 ac_u64 deadline,sequence;unsigned operation,late,definitive,count;int result;
 suites++;CHECK(AC_STAGE_BOUND_NS==100000000ULL);
 for(operation=AC_CLAIM;operation<=AC_RELEASE;operation++)for(late=0;late<2;late++)for(definitive=0;definitive<2;definitive++){
  pending_stage(&f,operation);CHECK(f.core.operation==operation);
  old=receipt(&f.fake.request[f.fake.count-1],AC_ACK);deadline=f.core.deadline_ns;
  CHECK(deadline-f.now==AC_STAGE_BOUND_NS);count=f.fake.count;sequence=f.core.next_sequence;
  f.now=deadline-1;
  CHECK(ac_pump(&f.core,f.now)==(operation>=AC_STOP?AC_RETAINED:AC_BUSY));
  CHECK(f.fake.count==count&&!f.core.fault&&f.core.retained&&f.core.ring==f.ring);
  CHECK(f.core.operation==operation&&ac_ticket_equal(&f.core.pending,&old.ticket));
  CHECK(f.core.next_sequence==sequence&&f.core.deadline_ns==deadline);
  f.now=deadline+late;result=ac_pump(&f.core,f.now);
  if(result!=AC_RETAINED)fprintf(stderr,"deadline-regression operation=%u result=%d submits=%u fault=%u\n",operation,result,f.fake.count,f.core.fault);
  CHECK(result==AC_RETAINED);
  CHECK(f.core.fault&&f.core.retained&&f.core.ring==f.ring&&!ac_can_free(&f.core));
  CHECK(f.core.next_sequence==sequence); /* only previously reserved cleanup token */
  if(operation<AC_STOP){
   CHECK(f.fake.count==count+1&&f.core.operation==AC_STOP&&f.core.stop_sent);
   CHECK(f.fake.request[count].operation==AC_STOP&&ac_ticket_equal(&f.fake.request[count].ticket,&f.core.stop));
   CHECK(ac_ticket_equal(&f.fake.request[count].work,&f.core.work));
   CHECK(f.fake.request[count].deadline_ns==f.now+AC_STAGE_BOUND_NS);
   CHECK(f.core.xrun==(operation>=AC_CACHE_DEVICE));
  }else{
   CHECK(f.fake.count==count&&f.core.state==AC_QUARANTINED);
   CHECK(f.core.operation==operation&&ac_ticket_equal(&f.core.pending,&old.ticket));
  }
  count=f.fake.count;CHECK(ac_pump(&f.core,++f.now)==AC_RETAINED);
  CHECK(ac_close(&f.core,++f.now)==AC_RETAINED&&f.fake.count==count);
  current=receipt(&f.fake.request[count-1],AC_ACK);
  stale=current;stale.ticket.sequence++;
  memcpy(&before,&f.core,sizeof(before));
  CHECK(ac_complete(&f.core,&stale,0)==AC_STALE);CHECK(!memcmp(&before,&f.core,sizeof(before)));
  if(operation<AC_STOP){
   CHECK(ac_complete(&f.core,&old,0)==AC_STALE);CHECK(!memcmp(&before,&f.core,sizeof(before)));
  }
  /* Skip well past cleanup deadline with NO callback/timer guarantee inferred.
   * The actual normal fence may still retire; definitive failure is terminal. */
  f.now=f.core.deadline_ns+1;
  if(definitive){
   current.status=AC_FAILED;CHECK(ac_complete(&f.core,&current,f.now)==AC_RETAINED);
   CHECK(f.core.terminal&&f.core.state==AC_QUARANTINED);
   current.status=AC_ACK;CHECK(ac_complete(&f.core,&current,++f.now)==AC_STALE);
   CHECK(ac_pump(&f.core,++f.now)==AC_RETAINED&&f.fake.count==count);
   CHECK(f.core.ring==f.ring&&f.core.retained&&!ac_can_free(&f.core));
  }else{
   result=ac_complete(&f.core,&current,f.now);
   if(current.operation==AC_STOP){
    CHECK(result==AC_RETAINED&&f.core.operation==AC_RELEASE&&f.fake.count==count+1);
    CHECK(ack(&f)==AC_OK);
   }else CHECK(result==AC_OK&&f.fake.count==count);
   CHECK(f.core.fault&&ac_can_free(&f.core)&&!f.core.ring);
   CHECK(ac_complete(&f.core,&current,++f.now)==AC_STALE);
   CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,2,f.now)==AC_FAULT);
  }
 }
 printf("deadline-matrix=all7 pending operations deadline-1/equality/late exact-once cleanup or quarantine; real ACK vs definitive failure\n");
}
static void timer_clock_loss_held_and_no_effect(void)
{
 struct fixture f;struct ac_span held;struct ac_completion r;unsigned operation,count;
 ac_u64 deadline;
 suites++;
 for(operation=AC_CLAIM;operation<=AC_RELEASE;operation++){
  pending_stage(&f,operation);count=f.fake.count;r=receipt(&f.fake.request[count-1],AC_ACK);
  CHECK(ac_pump(&f.core,f.core.last_ns-1)==AC_RETAINED);
  CHECK(f.core.fault&&f.core.retained&&f.core.ring==f.ring);
  if(operation<AC_STOP){
   CHECK(f.fake.count==count+1&&f.core.operation==AC_STOP);
   CHECK(ac_complete(&f.core,&r,0)==AC_STALE);
  }else CHECK(f.fake.count==count&&f.core.state==AC_QUARANTINED&&f.core.operation==operation);
  CHECK(ack(&f)==(operation==AC_RELEASE?AC_OK:AC_RETAINED));
  if(f.core.operation==AC_RELEASE)CHECK(ack(&f)==AC_OK);
  CHECK(f.core.fault&&ac_can_free(&f.core));
 }
 setup(&f);prepare(&f);period(&f,1);CHECK(ac_borrow(&f.core,&held)==AC_OK);
 CHECK(ac_close(&f.core,++f.now)==AC_RETAINED);deadline=f.core.deadline_ns;count=f.fake.count;
 f.now=deadline;CHECK(ac_pump(&f.core,f.now)==AC_RETAINED);
 CHECK(f.fake.count==count&&f.core.state==AC_QUARANTINED&&f.core.borrowed==1);
 CHECK(ack(&f)==AC_RETAINED);CHECK(f.core.state==AC_WAIT_BORROW&&f.core.operation==0);
 CHECK(f.core.ring==f.ring&&f.core.fault&&!ac_can_free(&f.core));
 f.now+=3*AC_STAGE_BOUND_NS;CHECK(ac_pump(&f.core,f.now)==AC_RETAINED);
 CHECK(f.fake.count==count&&f.core.borrowed==1&&f.core.state==AC_WAIT_BORROW);
 CHECK(ac_return(&f.core,&held.ticket,++f.now)==AC_RETAINED);
 CHECK(f.fake.count==count+1&&f.core.operation==AC_RELEASE);
 deadline=f.core.deadline_ns;f.now=deadline;CHECK(ac_pump(&f.core,f.now)==AC_RETAINED);
 CHECK(f.fake.count==count+1&&f.core.state==AC_QUARANTINED&&!ac_can_free(&f.core));
 CHECK(ack(&f)==AC_OK);CHECK(f.core.fault&&ac_can_free(&f.core));
 setup(&f);f.fake.result=AC_NO_EFFECT;
 CHECK(ac_prepare(&f.core,&f.params,&f.clock,f.ring,AC_RING_BYTES,1,f.now)==AC_FAULT);
 CHECK(f.core.operation==0&&!f.core.retained&&ac_can_free(&f.core));
 f.now+=2*AC_STAGE_BOUND_NS;CHECK(ac_pump(&f.core,f.now)==AC_RETAINED);
 CHECK(f.fake.count==1&&f.core.operation==0&&!f.core.retained&&!f.core.ring);
 CHECK(ac_close(&f.core,++f.now)==AC_OK&&f.fake.count==1);
}
int main(int argc,char **argv)
{
 if(argc==2&&!strcmp(argv[1],"--deadline-only")){
  all_stage_deadlines();timer_clock_loss_held_and_no_effect();
  printf("HOST timeout regression PASS suites=%u checks=%u\n",suites,checks);return 0;
 }
 CHECK(argc==1);
 rate_and_admission();capture_periods_pts_borrow();early_dma_late_irq_lost_period();
 hold_close_mute_xrun();partial_init_uncertainty_resource_conflict();clock_tokens_aliases_and_source_fault();exhaustion_and_deadline();
 release_fence_and_repeated_cancellation();
 all_stage_deadlines();timer_clock_loss_held_and_no_effect();
 printf("HOST SYNTHETIC PCM PASS suites=%u checks=%u core_bytes=%zu ring_bytes=%u native_qualified=0 no_audio_output_files\n",
  suites,checks,sizeof(struct ac_core),AC_RING_BYTES);return 0;
}
