/* SPDX-License-Identifier: GPL-2.0-only */
/* PUBLICDUMMY logical observations only: no button/device/reset executor. */
#include "reset_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,suites;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}} while(0)
#define MS RI_NS_PER_MS
#define BASE UINT64_C(1000000000)
struct fake {
 struct ri_observation observation;struct reset_input *engine;
 unsigned reads,write_mode,reenter,normalization,fixture_active_low,fixture_level;
 int result,reentrant[5];
};
struct fixture {struct reset_input engine;struct fake fake;struct ri_provider provider;struct ri_config config;};
static int read_observation(void *context,struct ri_observation *out)
{
 struct fake *f=context;struct ri_event event={0};struct ri_view v;
 f->reads++;
 if(f->reenter){
  f->reenter=0;f->reentrant[0]=reset_input_poll(f->engine,f->observation.sampled_ns);
  f->reentrant[1]=reset_input_peek(f->engine,f->observation.sampled_ns,&event);
  f->reentrant[2]=reset_input_consume(f->engine,&event,f->observation.sampled_ns);
  f->reentrant[3]=reset_input_dispose(f->engine);f->reentrant[4]=reset_input_view(f->engine,&v);
 }
 if(f->write_mode==0){
  *out=f->observation;
  /* HOST-only Boolean fixture normalization, NOT a GPIO/voltage read. */
  if(f->normalization)out->logical_pressed=(uint32_t)(f->fixture_level!=f->fixture_active_low);
 }else if(f->write_mode==2){out->valid=1;out->logical_pressed=RI_RELEASED;out->sampled_ns=f->observation.sampled_ns;}
 return f->result;
}
static struct ri_config proposed(void)
{
 struct ri_config c={20*MS,5000*MS,15000*MS,100*MS,1000*MS};return c;
}
static struct ri_config tiny(void)
{
 struct ri_config c={MS,4*MS,20*MS,2*MS,10*MS};return c;
}
static void setup_at(struct fixture *f,struct ri_config c,uint64_t start)
{
 memset(f,0,sizeof(*f));f->config=c;f->fake.engine=&f->engine;
 f->fake.observation.source_epoch=1;f->fake.observation.sampled_ns=start;
 f->fake.observation.valid=1;f->fake.observation.source_class=RI_PUBLICDUMMY_INPUT_ONLY;
 f->provider.abi=RESET_INPUT_ABI;f->provider.struct_bytes=sizeof(f->provider);
 f->provider.presence=RI_INPUT_PRESENT_UNQUALIFIED;f->provider.source_class=RI_PUBLICDUMMY_INPUT_ONLY;
 f->provider.context=&f->fake;f->provider.read=read_observation;
 CHECK(reset_input_init(&f->engine,&f->provider,&f->config,UINT64_C(0x5055424c4943),1,start)==RI_OK);
}
static void setup(struct fixture *f){setup_at(f,proposed(),BASE);}
static struct ri_view view(struct fixture *f)
{
 struct ri_view v;CHECK(reset_input_view(&f->engine,&v)==RI_OK);CHECK(!v.native_qualified);return v;
}
static int sample(struct fixture *f,uint32_t logical,uint64_t sampled,uint64_t received)
{
 f->fake.observation.logical_pressed=logical;f->fake.observation.sampled_ns=sampled;
 return reset_input_poll(&f->engine,received);
}
static void ok(struct fixture *f,uint32_t logical,uint64_t t){CHECK(sample(f,logical,t,t)==RI_OK);}
static void advance(struct fixture *f,uint32_t logical,uint64_t until)
{
 uint64_t t=f->fake.observation.sampled_ns,step=f->config.debounce_ns/2;
 CHECK(step!=0);
 while(t<until){uint64_t left=until-t;t+=left<step?left:step;ok(f,logical,t);}
}
static void arm(struct fixture *f,uint64_t start)
{
 ok(f,RI_RELEASED,start);advance(f,RI_RELEASED,start+f->config.debounce_ns);
 CHECK(view(f).desired_state==RI_ARMED);
}
static uint64_t held_threshold(struct fixture *f)
{
 uint64_t start=f->fake.observation.sampled_ns+f->config.debounce_ns/2;
 ok(f,RI_PRESSED,start);advance(f,RI_PRESSED,start+f->config.debounce_ns);
 CHECK(view(f).desired_state==RI_HELD);
 advance(f,RI_PRESSED,f->engine.press_since_ns+f->config.long_press_ns);
 CHECK(view(f).long_threshold_observed&&!view(f).event_queued);
 return f->fake.observation.sampled_ns;
}
static struct ri_event gesture(struct fixture *f,uint64_t start)
{
 struct ri_event e;uint64_t released;
 arm(f,start);held_threshold(f);released=f->fake.observation.sampled_ns+f->config.debounce_ns/2;
 ok(f,RI_RELEASED,released);advance(f,RI_RELEASED,released+f->config.debounce_ns);
 CHECK(view(f).event_queued);CHECK(reset_input_peek(&f->engine,f->fake.observation.sampled_ns,&e)==RI_OK);
 CHECK(e.kind==RI_DELIBERATE_LONG_PRESS_RELEASE&&e.evidence==RI_PROVIDER_DECLARATION_ONLY&&!e.native_qualified);
 CHECK(e.held_ns>=f->config.long_press_ns);return e;
}
static void boundaries_and_deliberate_release(void)
{
 struct fixture f;struct ri_event event={0};uint64_t press,t;
 suites++;setup(&f);
 ok(&f,RI_RELEASED,BASE);ok(&f,RI_RELEASED,BASE+20*MS-1);
 CHECK(view(&f).desired_state==RI_WAIT_RELEASE);
 ok(&f,RI_RELEASED,BASE+20*MS);CHECK(view(&f).desired_state==RI_ARMED);
 press=BASE+30*MS;ok(&f,RI_PRESSED,press);
 ok(&f,RI_PRESSED,press+20*MS-1);CHECK(view(&f).desired_state==RI_PRESS_DEBOUNCE);
 ok(&f,RI_PRESSED,press+20*MS);t=f.engine.press_since_ns;
 advance(&f,RI_PRESSED,t+5000*MS-1);CHECK(!view(&f).long_threshold_observed);
 ok(&f,RI_PRESSED,t+5000*MS);CHECK(view(&f).long_threshold_observed&&!view(&f).event_queued);
 CHECK(reset_input_peek(&f.engine,t+5000*MS,&event)==RI_EMPTY);
 ok(&f,RI_RELEASED,t+5010*MS);ok(&f,RI_RELEASED,t+5030*MS-1);
 CHECK(!view(&f).event_queued);
 ok(&f,RI_RELEASED,t+5030*MS);CHECK(reset_input_peek(&f.engine,t+5030*MS,&event)==RI_OK);
 CHECK(event.held_ns==5000*MS&&event.stable_press_ns==t&&event.stable_release_ns==t+5030*MS);
 CHECK(reset_input_consume(&f.engine,&event,t+5030*MS)==RI_OK);
 CHECK(reset_input_consume(&f.engine,&event,t+5030*MS)==RI_STALE);
 CHECK(view(&f).desired_state==RI_WAIT_RELEASE&&!view(&f).event_queued);
}
static void startup_short_bounce_stuck(void)
{
 struct fixture f;uint64_t t,press;struct ri_event e;
 suites++;setup(&f);ok(&f,RI_PRESSED,BASE);advance(&f,RI_PRESSED,BASE+20000*MS);
 CHECK(!view(&f).event_queued&&view(&f).desired_state==RI_WAIT_RELEASE);
 CHECK(view(&f).reason==RI_REASON_STUCK&&view(&f).logical_observed==RI_PRESSED);
 arm(&f,BASE+20010*MS);press=f.fake.observation.sampled_ns+10*MS;
 ok(&f,RI_PRESSED,press);ok(&f,RI_RELEASED,press+10*MS);CHECK(view(&f).desired_state==RI_ARMED);
 ok(&f,RI_PRESSED,press+20*MS);advance(&f,RI_PRESSED,press+40*MS);
 advance(&f,RI_PRESSED,press+4000*MS);ok(&f,RI_RELEASED,press+4010*MS);
 advance(&f,RI_RELEASED,press+4030*MS);CHECK(!view(&f).event_queued);
 CHECK(reset_input_peek(&f.engine,press+4030*MS,&e)==RI_EMPTY);
 setup(&f);arm(&f,BASE);held_threshold(&f);t=f.fake.observation.sampled_ns;
 ok(&f,RI_RELEASED,t+10*MS);ok(&f,RI_PRESSED,t+20*MS);
 CHECK(view(&f).desired_state==RI_HELD&&!view(&f).event_queued);
 advance(&f,RI_PRESSED,f.engine.press_since_ns+15000*MS);
 CHECK(!view(&f).event_queued&&view(&f).reason==RI_REASON_STUCK);
 t=f.fake.observation.sampled_ns;ok(&f,RI_RELEASED,t+10*MS);advance(&f,RI_RELEASED,t+30*MS);
 CHECK(!view(&f).event_queued&&view(&f).desired_state==RI_ARMED);
}
static void invalid_loss_and_sampling(void)
{
 struct fixture f;uint64_t t;unsigned mode;
 suites++;
 for(mode=0;mode<8;mode++){
  setup(&f);arm(&f,BASE);held_threshold(&f);t=f.fake.observation.sampled_ns+10*MS;
  if(mode==0)f.fake.observation.valid=0;
  if(mode==1)f.fake.observation.logical_pressed=RI_LOGICAL_UNKNOWN;
  if(mode==2)f.fake.observation.source_class=99;
  if(mode==3)f.fake.result=RI_READ_EOF;
  if(mode==4)f.fake.result=RI_READ_ERROR;
  if(mode==5)f.fake.write_mode=1;
  if(mode==6)f.fake.write_mode=2;
  if(mode==7)f.fake.result=99;
  f.fake.observation.sampled_ns=t;
  CHECK(reset_input_poll(&f.engine,t)==RI_UNAVAILABLE);
  CHECK(view(&f).logical_observed==RI_LOGICAL_UNKNOWN&&!view(&f).event_queued);
  f.fake.observation.valid=1;f.fake.observation.source_class=RI_PUBLICDUMMY_INPUT_ONLY;
  f.fake.result=RI_READ_OK;f.fake.write_mode=0;
  ok(&f,RI_RELEASED,t+10*MS);advance(&f,RI_RELEASED,t+30*MS);
  CHECK(!view(&f).event_queued);CHECK(view(&f).desired_state==RI_ARMED);
 }
 setup(&f);arm(&f,BASE);held_threshold(&f);t=f.fake.observation.sampled_ns;
 ok(&f,RI_PRESSED,t+100*MS+1);CHECK(view(&f).desired_state==RI_WAIT_RELEASE&&!view(&f).event_queued);
 advance(&f,RI_PRESSED,t+6000*MS);CHECK(!view(&f).event_queued);
 t=f.fake.observation.sampled_ns;
 CHECK(sample(&f,RI_RELEASED,t+10*MS,t+111*MS)==RI_UNAVAILABLE);
 CHECK(view(&f).observation_validity==RI_OBS_STALE&&view(&f).logical_observed==RI_LOGICAL_UNKNOWN);
 setup(&f);CHECK(sample(&f,RI_RELEASED,BASE+1,BASE)==RI_UNAVAILABLE);
 CHECK(view(&f).logical_observed==RI_LOGICAL_UNKNOWN);
}
static void timestamp_equality_and_source_epochs(void)
{
 struct fixture f;struct ri_event event;uint64_t t;unsigned reads;
 suites++;setup(&f);ok(&f,RI_RELEASED,BASE);
 ok(&f,RI_RELEASED,BASE);CHECK(view(&f).desired_state==RI_WAIT_RELEASE);
 CHECK(sample(&f,RI_PRESSED,BASE,BASE)==RI_UNAVAILABLE);CHECK(view(&f).reason==RI_REASON_CONFLICT);
 ok(&f,RI_RELEASED,BASE+MS);ok(&f,RI_RELEASED,BASE+21*MS);CHECK(view(&f).desired_state==RI_ARMED);
 setup(&f);event=gesture(&f,BASE);t=event.issued_ns;
 f.fake.observation.source_epoch=2;ok(&f,RI_RELEASED,t+MS);
 CHECK(!view(&f).event_queued&&view(&f).desired_state==RI_WAIT_RELEASE);
 CHECK(reset_input_consume(&f.engine,&event,t+MS)==RI_STALE);
 f.fake.observation.source_epoch=1;
 CHECK(sample(&f,RI_RELEASED,t+2*MS,t+2*MS)==RI_FAULT);
 CHECK(view(&f).reason==RI_REASON_SOURCE_REUSE&&!view(&f).event_queued);
 reads=f.fake.reads;f.fake.observation.source_epoch=3;
 CHECK(sample(&f,RI_RELEASED,t+3*MS,t+3*MS)==RI_FAULT);CHECK(f.fake.reads==reads);
 setup(&f);f.fake.observation.source_epoch=UINT64_MAX;arm(&f,BASE);
 f.fake.observation.source_epoch=UINT64_MAX-1;
 CHECK(sample(&f,RI_PRESSED,BASE+30*MS,BASE+30*MS)==RI_FAULT);CHECK(!view(&f).event_queued);
 setup(&f);arm(&f,BASE);
 CHECK(sample(&f,RI_PRESSED,BASE+19*MS,BASE+21*MS)==RI_FAULT);CHECK(view(&f).reason==RI_REASON_CLOCK);
 setup(&f);event=gesture(&f,BASE);reads=f.fake.reads;
 CHECK(reset_input_peek(&f.engine,event.issued_ns-1,&event)==RI_FAULT);
 CHECK(!view(&f).event_queued&&view(&f).fault);CHECK(f.fake.reads==reads);
}
static void queue_receipts_aliases_and_expiry(void)
{
 struct fixture f;struct ri_event event,copy,wrong;struct reset_input before;
 uint64_t now;unsigned i,reads;struct ri_view v;
 suites++;setup(&f);event=gesture(&f,BASE);now=event.issued_ns;reads=f.fake.reads;
 for(i=0;i<8;i++){
  CHECK(reset_input_peek(&f.engine,now,&copy)==RI_OK);CHECK(!memcmp(&event,&copy,sizeof(event)));
 }
 CHECK(f.fake.reads==reads&&f.engine.next_sequence==2);
 for(i=0;i<12;i++){
  wrong=event;
  switch(i){
  case 0:wrong.ticket.namespace_epoch++;break;
  case 1:wrong.ticket.engine_generation++;break;
  case 2:wrong.ticket.source_epoch++;break;
  case 3:wrong.ticket.sequence++;break;
  case 4:wrong.issued_ns++;break;
  case 5:wrong.expires_ns++;break;
  case 6:wrong.stable_press_ns++;break;
  case 7:wrong.stable_release_ns++;break;
  case 8:wrong.held_ns++;break;
  case 9:wrong.kind++;break;
  case 10:wrong.evidence++;break;
  case 11:wrong.native_qualified=1;break;
  default:CHECK(0);break;
  }
  memcpy(&before,&f.engine,sizeof(before));
  CHECK(reset_input_consume(&f.engine,&wrong,now)==RI_STALE);CHECK(!memcmp(&before,&f.engine,sizeof(before)));
 }
 memcpy(&before,&f.engine,sizeof(before));
 CHECK(reset_input_consume(&f.engine,&f.engine.event,now)==RI_INVALID);
 CHECK(reset_input_peek(&f.engine,now,&f.engine.event)==RI_INVALID);
 CHECK(reset_input_peek(&f.engine,now,NULL)==RI_INVALID);
 CHECK(reset_input_consume(&f.engine,NULL,now)==RI_INVALID);
 CHECK(reset_input_view(&f.engine,&f.engine.view)==RI_INVALID);
 CHECK(!memcmp(&before,&f.engine,sizeof(before)));
 CHECK(reset_input_consume(&f.engine,&event,now)==RI_OK);CHECK(f.engine.next_sequence==2);
 CHECK(reset_input_consume(&f.engine,&event,now)==RI_STALE);CHECK(!view(&f).event_queued);
 /* Consuming while a later press is already held cannot arm that press. */
 ok(&f,RI_PRESSED,now+10*MS);advance(&f,RI_PRESSED,now+6000*MS);CHECK(!view(&f).event_queued);
 setup(&f);event=gesture(&f,BASE);now=event.issued_ns;
 ok(&f,RI_PRESSED,now+10*MS);advance(&f,RI_PRESSED,now+900*MS);
 CHECK(view(&f).event_queued&&f.engine.next_sequence==2);
 CHECK(reset_input_consume(&f.engine,&event,now+900*MS)==RI_OK);
 advance(&f,RI_PRESSED,now+6000*MS);CHECK(!view(&f).event_queued&&f.engine.next_sequence==2);
 setup(&f);event=gesture(&f,BASE);advance(&f,RI_RELEASED,event.expires_ns-1);
 CHECK(reset_input_peek(&f.engine,event.expires_ns-1,&copy)==RI_OK);
 CHECK(reset_input_consume(&f.engine,&event,event.expires_ns)==RI_STALE);
 CHECK(!view(&f).event_queued&&view(&f).reason==RI_REASON_EXPIRED);
 setup(&f);event=gesture(&f,BASE);
 CHECK(reset_input_peek(&f.engine,event.issued_ns+100*MS,&copy)==RI_OK);
 CHECK(reset_input_consume(&f.engine,&event,event.issued_ns+100*MS+1)==RI_STALE);
 v=view(&f);CHECK(v.observation_validity==RI_OBS_STALE&&!v.event_queued);
}
static void absent_partial_init_and_reentry(void)
{
 struct fixture f;struct reset_input engine={0},unchanged={0},copied;struct ri_config c=proposed();
 struct ri_provider p={0};struct ri_view v;struct ri_event e;unsigned i;
 suites++;
 CHECK(reset_input_init(&engine,&p,&c,1,1,BASE)==RI_INVALID);CHECK(!memcmp(&engine,&unchanged,sizeof(engine)));
 p.abi=RESET_INPUT_ABI;p.struct_bytes=sizeof(p);p.source_class=RI_PUBLICDUMMY_INPUT_ONLY;p.presence=RI_INPUT_ABSENT;
 CHECK(reset_input_init(&engine,&p,&c,1,1,BASE)==RI_OK);
 CHECK(reset_input_poll(&engine,BASE)==RI_UNAVAILABLE);CHECK(reset_input_view(&engine,&v)==RI_OK);
 CHECK(v.presence==RI_INPUT_ABSENT&&v.observation_validity==RI_OBS_ABSENT&&v.logical_observed==RI_LOGICAL_UNKNOWN&&!v.native_qualified);
 CHECK(reset_input_peek(&engine,BASE,&e)==RI_EMPTY);
 CHECK(reset_input_init(&engine,&p,&c,2,2,BASE)==RI_INVALID);
 CHECK(reset_input_dispose(&engine)==RI_OK);CHECK(reset_input_view(&engine,&v)==RI_OK);
 CHECK(v.desired_state==RI_DISPOSED);CHECK(reset_input_poll(&engine,BASE)==RI_INVALID);
 CHECK(reset_input_init(&engine,&p,&c,2,2,BASE)==RI_INVALID);
 setup(&f);f.fake.reenter=1;ok(&f,RI_RELEASED,BASE);
 for(i=0;i<5;i++)CHECK(f.fake.reentrant[i]==RI_BUSY);
 CHECK(f.fake.reads==1&&!view(&f).event_queued);
 copied=f.engine;CHECK(reset_input_poll(&copied,BASE)==RI_INVALID);
 CHECK(reset_input_init(&f.engine,&f.provider,&f.config,2,2,BASE)==RI_INVALID);
 setup(&f);p=f.provider;p.source_class=99;memset(&engine,0,sizeof(engine));
 CHECK(reset_input_init(&engine,&p,&c,1,1,BASE)==RI_INVALID);CHECK(!memcmp(&engine,&unchanged,sizeof(engine)));
 for(i=0;i<10;i++){
  struct ri_config bad=c;memset(&engine,0,sizeof(engine));p=f.provider;
  switch(i){
  case 0:bad.debounce_ns=0;break;
  case 1:bad.debounce_ns=UINT64_MAX;break;
  case 2:bad.long_press_ns=bad.debounce_ns;break;
  case 3:bad.long_press_ns=UINT64_MAX;break;
  case 4:bad.stuck_cap_ns=bad.long_press_ns;break;
  case 5:bad.stuck_cap_ns=UINT64_MAX;break;
  case 6:bad.fresh_gap_ns=bad.debounce_ns-1;break;
  case 7:bad.fresh_gap_ns=UINT64_MAX;break;
  case 8:bad.event_ttl_ns=0;break;
  case 9:bad.event_ttl_ns=UINT64_MAX;break;
  default:CHECK(0);break;
  }
  CHECK(reset_input_init(&engine,&p,&bad,1,1,BASE)==RI_INVALID);CHECK(!memcmp(&engine,&unchanged,sizeof(engine)));
 }
}
static void exhaustion_and_polarity_contract(void)
{
 struct fixture f;uint64_t t;struct ri_event events[2];unsigned low;
 suites++;setup_at(&f,tiny(),BASE);arm(&f,BASE);held_threshold(&f);
 f.engine.next_sequence=UINT64_MAX; /* intentional white-box exhaustion boundary */
 t=f.fake.observation.sampled_ns+f.config.debounce_ns/2;ok(&f,RI_RELEASED,t);
 CHECK(sample(&f,RI_RELEASED,t+f.config.debounce_ns,t+f.config.debounce_ns)==RI_EXHAUSTED);
 CHECK(view(&f).fault&&!view(&f).event_queued);
 setup_at(&f,tiny(),UINT64_MAX-9*MS);arm(&f,UINT64_MAX-9*MS);held_threshold(&f);
 t=f.fake.observation.sampled_ns+f.config.debounce_ns/2;ok(&f,RI_RELEASED,t);
 CHECK(sample(&f,RI_RELEASED,t+f.config.debounce_ns,t+f.config.debounce_ns)==RI_EXHAUSTED);
 CHECK(!view(&f).event_queued);CHECK(sample(&f,RI_RELEASED,0,0)==RI_FAULT);
 for(low=0;low<2;low++){
  setup_at(&f,tiny(),BASE);f.fake.normalization=1;f.fake.fixture_active_low=low;
  f.fake.fixture_level=low;arm(&f,BASE);f.fake.fixture_level=!low;
  held_threshold(&f);f.fake.fixture_level=low;
  t=f.fake.observation.sampled_ns+f.config.debounce_ns/2;ok(&f,RI_RELEASED,t);
  advance(&f,RI_RELEASED,t+f.config.debounce_ns);
  CHECK(reset_input_peek(&f.engine,f.fake.observation.sampled_ns,&events[low])==RI_OK);
 }
 CHECK(!memcmp(&events[0],&events[1],sizeof(events[0])));
 /* No GPIO polarity field exists in the engine/provider ABI. */
}
struct oracle {unsigned state,anchor,pressed_since,long_seen,events;};
static void oracle_step(struct oracle *o,unsigned pressed,unsigned sample_index)
{
 if(o->events)return;
 switch(o->state){
 case 0: /* startup requires two observed releases, one ms apart */
  if(pressed){o->anchor=sample_index+1;break;}
  if(sample_index>=o->anchor+1)o->state=1;
  break;
 case 1:if(pressed){o->anchor=sample_index;o->state=2;}break;
 case 2:
  if(!pressed){o->state=1;break;}
  if(sample_index>=o->anchor+1){o->pressed_since=sample_index;o->state=3;}
  break;
 case 3:
  if(!pressed){o->anchor=sample_index;o->state=4;break;}
  if(sample_index-o->pressed_since>=4)o->long_seen=1;
  break;
 case 4:
  if(pressed){o->state=3;if(sample_index-o->pressed_since>=4)o->long_seen=1;break;}
  if(sample_index>=o->anchor+1){if(o->long_seen)o->events++;else o->state=1;}
  break;
 default:CHECK(0);break;
 }
}
static void exhaustive_patterns_and_metadata_fuzz(void)
{
 struct fixture f;unsigned pattern,i,events=0;uint64_t rng=UINT64_C(0x5245534554505542),t;
 suites++;
 for(pattern=0;pattern<4096;pattern++){
  struct oracle o={0};setup_at(&f,tiny(),0);
  for(i=0;i<12;i++){
   unsigned pressed=(pattern>>i)&1U;
   ok(&f,pressed,(uint64_t)i*MS);oracle_step(&o,pressed,i);
   CHECK(f.engine.view.event_queued==(o.events!=0));CHECK(!f.engine.view.native_qualified);
   CHECK(f.engine.next_sequence==(o.events?2U:1U));
  }
  CHECK(o.events<=1);events+=o.events;
 }
 CHECK(events>0);printf("exhaustive=4096 twelve-sample Boolean patterns oracle_matched events=%u\n",events);
 for(pattern=0;pattern<256;pattern++){
  setup(&f);t=BASE;
  for(i=0;i<64;i++){
   rng=rng*UINT64_C(6364136223846793005)+1;t+=MS;
   f.fake.observation.valid=(uint32_t)(rng&3U);
   f.fake.observation.logical_pressed=(uint32_t)((rng>>2)&3U);
   f.fake.observation.source_epoch=(rng>>4)&7U;
   f.fake.observation.source_class=(uint32_t)((rng>>7)&3U);
   f.fake.observation.sampled_ns=(rng&256U)?t:t+MS;
   f.fake.result=(int)((rng>>9)&3U);
   (void)reset_input_poll(&f.engine,t);
   CHECK(!f.engine.view.event_queued&&!f.engine.view.native_qualified);
   CHECK(f.engine.next_sequence==1); /* no five-second stable gesture possible */
  }
 }
}
int main(void)
{
 boundaries_and_deliberate_release();startup_short_bounce_stuck();invalid_loss_and_sampling();
 timestamp_equality_and_source_epochs();queue_receipts_aliases_and_expiry();absent_partial_init_and_reentry();
 exhaustion_and_polarity_contract();exhaustive_patterns_and_metadata_fuzz();
 printf("HOST PUBLICDUMMY PASS suites=%u checks=%u engine_bytes=%zu event_bytes=%zu; reset/footage executors=ABSENT\n",
  suites,checks,sizeof(struct reset_input),sizeof(struct ri_event));return 0;
}
