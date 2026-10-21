/* SPDX-License-Identifier: GPL-2.0-only
 * Pure synthetic scalar fixtures. No radio, network, credentials or storage. */
#include <camera/network_transition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,cases;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"check=%u line=%d\n",checks,__LINE__);abort();}} while(0)
#define OK(x) CHECK((x)==CAMERA_NETWORK_OK)
#define FACTS (CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS|CAMERA_NETWORK_LOCAL_REACHABLE)
static struct camera_network_config config(void)
{
 struct camera_network_config c={0};
 c.namespace_id=101;c.committed_config_id=10;c.committed_revision=7;
 c.radio_high_water=30;c.transition_timeout_ns=100;c.rollback_timeout_ns=50;
 return c;
}
static void init(struct camera_network_transition *s)
{struct camera_network_config c=config();OK(camera_network_init(s,&c,1000));cases++;}
static struct camera_network_request request(uint64_t id)
{
 struct camera_network_request q={0};q.token.namespace_id=101;q.token.id=id;
 q.expected_revision=7;q.config_id=10+id;q.kind=CAMERA_NETWORK_SWITCH;
 q.required_facts=FACTS;return q;
}
static struct camera_network_report report(struct camera_network_transition *s,
 struct camera_network_token t)
{struct camera_network_report r;OK(camera_network_query(s,t,&r));return r;}
static struct camera_network_link link(const struct camera_network_work *w)
{
 struct camera_network_link p={0};p.token=w->token;p.config_id=w->config_id;
 p.radio_generation=w->radio_generation;p.observation_id=1;p.facts=FACTS;return p;
}
static struct camera_network_work start(struct camera_network_transition *s,
 struct camera_network_request q)
{
 struct camera_network_work w;OK(camera_network_prepare(s,&q,1000));
 OK(camera_network_receipt_ack(s,q.token,1001));OK(camera_network_next(s,1001,&w));return w;
}
static void restored(struct camera_network_transition *s,struct camera_network_token t,
 uint64_t now)
{
 struct camera_network_work w;struct camera_network_link p;
 CHECK(report(s,t).state==CAMERA_NETWORK_ROLLBACK_PENDING);
 CHECK(!report(s,t).old_link_known);OK(camera_network_next(s,now,&w));
 CHECK(w.effect==CAMERA_NETWORK_ROLLBACK&&w.config_id==10);
 CHECK(report(s,t).state==CAMERA_NETWORK_ROLLING_BACK);
 CHECK(!report(s,t).old_link_known);p=link(&w);
 OK(camera_network_complete(s,&w,1,&p,now+1));
 CHECK(report(s,t).state==CAMERA_NETWORK_RESTORED);
 CHECK(report(s,t).old_link_known&&report(s,t).committed_config_id==10);
 CHECK(report(s,t).rollback_completed&&report(s,t).rollback_succeeded&&
       !report(s,t).rollback_reason);
 CHECK(report(s,t).committed_revision==7&&!report(s,t).in_flight);
}
static void positive(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1),q2=request(2);
 struct camera_network_work w;struct camera_network_link p;struct camera_network_report r;
 init(&s);w=start(&s,q);p=link(&w);
 CHECK(w.effect==CAMERA_NETWORK_APPLY&&w.radio_generation==31&&w.deadline_ns==1100);
 CHECK(camera_network_next(&s,1002,&w)==CAMERA_NETWORK_NOT_READY);
 /* Failed next zeroes output, so get the original exact work from live. */
 w=s.live;OK(camera_network_complete(&s,&w,1,NULL,1002));
 r=report(&s,q.token);CHECK(r.state==CAMERA_NETWORK_WAIT_LINK&&!r.in_flight);
 CHECK(r.committed_config_id==10&&r.committed_revision==7&&!r.old_link_known);
 OK(camera_network_link_proof(&s,&p,1003));OK(camera_network_link_proof(&s,&p,1003));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_CONFIRM_READY);
 OK(camera_network_confirm(&s,q.token,7,1004));r=report(&s,q.token);
 CHECK(r.state==CAMERA_NETWORK_CONFIRMED&&r.committed_config_id==11&&r.committed_revision==8);
 CHECK(!r.old_link_known&&r.link_facts==FACTS);
 OK(camera_network_prepare(&s,&q,1004));
 CHECK(camera_network_confirm(&s,q.token,7,1004)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_link_proof(&s,&p,1004)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_complete(&s,&w,1,NULL,1004)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_prepare(&s,&q2,1004)==CAMERA_NETWORK_CONFLICT);
 q2.expected_revision=8;OK(camera_network_prepare(&s,&q2,1004));
 CHECK(report(&s,q2.token).apply_generation==33);
 CHECK(report(&s,q.token).committed_revision==8); /* retained historical result */
 /* Old operation callbacks/replay cannot regress the clock or expire new work. */
 CHECK(camera_network_link_proof(&s,&p,UINT64_MAX)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_cancel(&s,q.token,0)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_receipt_ack(&s,q.token,UINT64_MAX)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_confirm(&s,q.token,7,0)==CAMERA_NETWORK_ORDER);
 OK(camera_network_prepare(&s,&q,UINT64_MAX));OK(camera_network_prepare(&s,&q,0));
 CHECK(s.last_ns==1004&&!s.clock_fault&&report(&s,q2.token).state==CAMERA_NETWORK_PREPARED);
}
static void receipt_and_order(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1),b=request(2);
 struct camera_network_work w;struct camera_network_link p={0};struct camera_network_token bad;
 init(&s);OK(camera_network_prepare(&s,&q,1000));
 for(unsigned i=0;i<4;i++)CHECK(camera_network_next(&s,1000,&w)==CAMERA_NETWORK_NOT_READY);
 CHECK(!s.live_present&&report(&s,q.token).old_link_known);
 CHECK(camera_network_confirm(&s,q.token,7,1000)==CAMERA_NETWORK_ORDER);
 p.token=q.token;p.config_id=q.config_id;p.radio_generation=31;p.observation_id=1;p.facts=FACTS;
 CHECK(camera_network_link_proof(&s,&p,1000)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_forget(&s,q.token)==CAMERA_NETWORK_BUSY);
 CHECK(camera_network_prepare(&s,&b,1000)==CAMERA_NETWORK_BUSY);
 bad=q.token;bad.namespace_id++;
 CHECK(camera_network_receipt_ack(&s,bad,1000)==CAMERA_NETWORK_STALE);
 CHECK(!report(&s,q.token).receipt_acknowledged);
 OK(camera_network_prepare(&s,&q,1000));b=q;b.config_id++;
 CHECK(camera_network_prepare(&s,&b,1000)==CAMERA_NETWORK_CONFLICT);
 OK(camera_network_receipt_ack(&s,q.token,1000));
 OK(camera_network_receipt_ack(&s,q.token,1000));
 OK(camera_network_next(&s,1000,&w));
 CHECK(camera_network_receipt_ack(&s,q.token,1000)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_link_proof(&s,&p,1000)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_confirm(&s,q.token,7,1000)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_complete(&s,&w,1,&p,1000)==CAMERA_NETWORK_INVALID);
 CHECK(report(&s,q.token).in_flight);
}
static void failures_and_rollback(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1);
 struct camera_network_work w;struct camera_network_link p;
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,0,NULL,1002));
 CHECK(report(&s,q.token).reason==CAMERA_NETWORK_REASON_APPLY);
 restored(&s,q.token,1003);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,1,NULL,1002));p=link(&w);
 OK(camera_network_link_proof(&s,&p,1003));
 CHECK(camera_network_confirm(&s,q.token,8,1004)==CAMERA_NETWORK_CONFLICT);
 CHECK(report(&s,q.token).committed_revision==7);
 restored(&s,q.token,1005);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,0,NULL,1002));
 OK(camera_network_next(&s,1003,&w));p=link(&w);
 CHECK(camera_network_complete(&s,&w,1,NULL,1004)==CAMERA_NETWORK_STALE);
 CHECK(report(&s,q.token).in_flight&&!report(&s,q.token).old_link_known);
 p.radio_generation--;CHECK(camera_network_complete(&s,&w,1,&p,1004)==CAMERA_NETWORK_STALE);
 p=link(&w);p.config_id++;CHECK(camera_network_complete(&s,&w,1,&p,1004)==CAMERA_NETWORK_STALE);
 p=link(&w);p.facts=CAMERA_NETWORK_ADDRESS;
 CHECK(camera_network_complete(&s,&w,1,&p,1004)==CAMERA_NETWORK_STALE);
 p=link(&w);p.observation_id=0;
 CHECK(camera_network_complete(&s,&w,1,&p,1004)==CAMERA_NETWORK_STALE);
 OK(camera_network_complete(&s,&w,0,NULL,1004));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED);
 CHECK(report(&s,q.token).rollback_completed&&!report(&s,q.token).rollback_succeeded&&
       report(&s,q.token).rollback_reason==CAMERA_NETWORK_REASON_ROLLBACK);
 CHECK(!report(&s,q.token).old_link_known&&report(&s,q.token).committed_config_id==10);
 q=request(2);CHECK(camera_network_prepare(&s,&q,1005)==CAMERA_NETWORK_BUSY);
}
static void stale_proofs(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1);
 struct camera_network_work w,b;struct camera_network_link p,z;
 init(&s);w=start(&s,q);b=w;b.token.namespace_id++;
 CHECK(camera_network_complete(&s,&b,1,NULL,1002)==CAMERA_NETWORK_STALE);
 b=w;b.token.id++;CHECK(camera_network_complete(&s,&b,1,NULL,1002)==CAMERA_NETWORK_STALE);
 b=w;b.config_id++;CHECK(camera_network_complete(&s,&b,1,NULL,1002)==CAMERA_NETWORK_STALE);
 b=w;b.radio_generation++;CHECK(camera_network_complete(&s,&b,1,NULL,1002)==CAMERA_NETWORK_STALE);
 b=w;b.effect=CAMERA_NETWORK_ROLLBACK;
 CHECK(camera_network_complete(&s,&b,0,NULL,1002)==CAMERA_NETWORK_STALE);
 b=w;b.deadline_ns++;CHECK(camera_network_complete(&s,&b,1,NULL,1002)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_complete(&s,&w,2,NULL,1002)==CAMERA_NETWORK_INVALID);
 OK(camera_network_complete(&s,&w,1,NULL,1002));p=link(&w);
 z=p;z.token.namespace_id++;CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE);
 z=p;z.token.id++;CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE);
 z=p;z.config_id--;CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE);
 z=p;z.radio_generation++;CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE);
 z=p;z.observation_id=0;CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE);
 for(unsigned facts=0;facts<16;facts++)if((facts&FACTS)!=FACTS){
  z=p;z.observation_id=2+facts;z.facts=facts;
  CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_NOT_READY);
 }
 z=p;z.observation_id=19;z.facts|=16;
 CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_INVALID);
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_WAIT_LINK);
 p.observation_id=20;
 OK(camera_network_link_proof(&s,&p,1003));
 z=p;z.observation_id=21;z.facts|=16;
 CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_confirm(&s,q.token,7,1003)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_link_proof(&s,&p,1003)==CAMERA_NETWORK_STALE);
 p.observation_id=22;
 OK(camera_network_link_proof(&s,&p,1003));
 z=p;z.observation_id=23;z.facts=CAMERA_NETWORK_ADDRESS;
 CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_confirm(&s,q.token,7,1003)==CAMERA_NETWORK_ORDER);
 CHECK(camera_network_link_proof(&s,&p,1003)==CAMERA_NETWORK_STALE);
 p.observation_id=24;
 OK(camera_network_link_proof(&s,&p,1003));
 z=p;z.radio_generation--;
 CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_link_proof(&s,&z,0)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_link_proof(&s,&z,UINT64_MAX)==CAMERA_NETWORK_STALE);
 z=p;z.config_id--;
 CHECK(camera_network_link_proof(&s,&z,0)==CAMERA_NETWORK_STALE);
 z=p;z.observation_id--;
 CHECK(camera_network_link_proof(&s,&z,UINT64_MAX)==CAMERA_NETWORK_STALE);
 CHECK(s.last_ns==1003&&!s.clock_fault);
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_CONFIRM_READY);
 OK(camera_network_cancel(&s,q.token,1003));
 CHECK(camera_network_link_proof(&s,&p,1003)==CAMERA_NETWORK_ORDER);
 restored(&s,q.token,1004);
 q=request(2);q.config_id=11;
 CHECK(camera_network_prepare(&s,&q,1006)==CAMERA_NETWORK_CONFLICT); /* consumed instance */
 q.config_id=12;OK(camera_network_prepare(&s,&q,1006));
 CHECK(camera_network_link_proof(&s,&p,1006)==CAMERA_NETWORK_ORDER);
}
static void cancellation(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1),q2=request(2);
 struct camera_network_work w,b;struct camera_network_link p;
 init(&s);OK(camera_network_prepare(&s,&q,1000));OK(camera_network_cancel(&s,q.token,1001));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_CANCELLED&&report(&s,q.token).old_link_known);
 CHECK(camera_network_next(&s,1001,&w)==CAMERA_NETWORK_NOT_READY);
 OK(camera_network_prepare(&s,&q,1001)); /* replay never restarts cancelled request */
 CHECK(camera_network_receipt_ack(&s,q.token,1001)==CAMERA_NETWORK_ORDER);
 init(&s);w=start(&s,q);p=link(&w);OK(camera_network_cancel(&s,q.token,1002));
 OK(camera_network_cancel(&s,q.token,1002));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_CANCEL_WAIT&&report(&s,q.token).in_flight);
 CHECK(camera_network_forget(&s,q.token)==CAMERA_NETWORK_BUSY);
 CHECK(camera_network_prepare(&s,&q2,1002)==CAMERA_NETWORK_BUSY);
 CHECK(camera_network_next(&s,1002,&b)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_link_proof(&s,&p,1002)==CAMERA_NETWORK_ORDER);
 OK(camera_network_complete(&s,&w,1,NULL,1003));
 CHECK(report(&s,q.token).reason==CAMERA_NETWORK_REASON_CANCEL);
 restored(&s,q.token,1004);OK(camera_network_prepare(&s,&q2,1006));
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,1,NULL,1002));
 OK(camera_network_cancel(&s,q.token,1002));restored(&s,q.token,1003);
}
static void timeouts(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1),q2=request(2);
 struct camera_network_work w,b;struct camera_network_link p;
 init(&s);OK(camera_network_prepare(&s,&q,1000));OK(camera_network_tick(&s,1099));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_PREPARED);
 OK(camera_network_tick(&s,1100));CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED);
 CHECK(report(&s,q.token).old_link_known&&!s.live_present);
 CHECK(camera_network_receipt_ack(&s,q.token,1100)==CAMERA_NETWORK_ORDER);
 init(&s);w=start(&s,q);p=link(&w);OK(camera_network_tick(&s,1100));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_CANCEL_WAIT&&report(&s,q.token).in_flight);
 CHECK(camera_network_prepare(&s,&q2,1100)==CAMERA_NETWORK_BUSY);
 CHECK(camera_network_next(&s,1101,&b)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_link_proof(&s,&p,1101)==CAMERA_NETWORK_ORDER);
 OK(camera_network_complete(&s,&w,1,NULL,1101));
 CHECK(report(&s,q.token).reason==CAMERA_NETWORK_REASON_TIMEOUT);restored(&s,q.token,1102);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,1,NULL,1002));p=link(&w);
 CHECK(camera_network_link_proof(&s,&p,1100)==CAMERA_NETWORK_ORDER);restored(&s,q.token,1101);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,1,NULL,1002));p=link(&w);
 OK(camera_network_link_proof(&s,&p,1099));
 CHECK(camera_network_confirm(&s,q.token,7,1100)==CAMERA_NETWORK_ORDER);restored(&s,q.token,1101);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,0,NULL,1002));
 OK(camera_network_next(&s,1003,&w));p=link(&w);
 OK(camera_network_tick(&s,1053));CHECK(report(&s,q.token).in_flight);
 CHECK(report(&s,q.token).rollback_reason==CAMERA_NETWORK_REASON_TIMEOUT);
 CHECK(camera_network_next(&s,1054,&b)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_forget(&s,q.token)==CAMERA_NETWORK_BUSY);
 OK(camera_network_complete(&s,&w,1,&p,1054));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&!report(&s,q.token).old_link_known);
 CHECK(report(&s,q.token).rollback_completed&&!report(&s,q.token).rollback_succeeded);
 CHECK(camera_network_complete(&s,&w,1,&p,1054)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_prepare(&s,&q2,1054)==CAMERA_NETWORK_BUSY);
}
static void scans(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1);
 struct camera_network_work w;struct camera_network_link p;
 q.kind=CAMERA_NETWORK_SCAN;q.config_id=10;
 for(unsigned success=0;success<=1;success++){
  init(&s);w=start(&s,q);CHECK(w.effect==CAMERA_NETWORK_DISRUPTIVE_SCAN);p=link(&w);
  CHECK(camera_network_link_proof(&s,&p,1002)==CAMERA_NETWORK_ORDER);
  OK(camera_network_complete(&s,&w,success,NULL,1002));
  CHECK(report(&s,q.token).scan_completed&&report(&s,q.token).scan_succeeded==success);
  CHECK(camera_network_confirm(&s,q.token,7,1002)==CAMERA_NETWORK_ORDER);
  restored(&s,q.token,1003);
 }
 init(&s);w=start(&s,q);OK(camera_network_cancel(&s,q.token,1002));
 CHECK(report(&s,q.token).in_flight);OK(camera_network_complete(&s,&w,1,NULL,1003));
 CHECK(report(&s,q.token).scan_completed&&!report(&s,q.token).scan_succeeded);
 restored(&s,q.token,1004);
}
static void retention_and_overflow(void)
{
 struct camera_network_transition s;struct camera_network_request q;struct camera_network_report r;
 struct camera_network_config c=config();struct camera_network_work w;
 init(&s);
 for(unsigned i=1;i<=CAMERA_NETWORK_RESULTS;i++){
  q=request(i);OK(camera_network_prepare(&s,&q,1000));OK(camera_network_cancel(&s,q.token,1000));
 }
 q=request(9);CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_CAPACITY);
 CHECK(s.operation_high_water==8&&s.radio_high_water==46&&s.config_high_water==18);
 q=request(1);OK(camera_network_query(&s,q.token,&r));
 OK(camera_network_forget(&s,q.token));CHECK(camera_network_query(&s,q.token,&r)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_CONFLICT);
 q=request(9);OK(camera_network_prepare(&s,&q,1000));
 CHECK(camera_network_forget(&s,q.token)==CAMERA_NETWORK_BUSY);
 c.operation_high_water=UINT64_MAX;OK(camera_network_init(&s,&c,1000));
 CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_EXHAUSTED);
 c=config();c.radio_high_water=UINT64_MAX-1;OK(camera_network_init(&s,&c,1000));
 CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_EXHAUSTED);
 c=config();c.radio_high_water=UINT64_MAX-2;OK(camera_network_init(&s,&c,1000));
 OK(camera_network_prepare(&s,&q,1000));CHECK(report(&s,q.token).rollback_generation==UINT64_MAX);
 OK(camera_network_cancel(&s,q.token,1000));q=request(10);
 CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_EXHAUSTED);
 c=config();c.committed_revision=UINT64_MAX;OK(camera_network_init(&s,&c,1000));
 q=request(1);q.expected_revision=UINT64_MAX;
 CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_EXHAUSTED);
 c=config();OK(camera_network_init(&s,&c,UINT64_MAX-99));q=request(1);
 CHECK(camera_network_prepare(&s,&q,UINT64_MAX-99)==CAMERA_NETWORK_EXHAUSTED);
 OK(camera_network_init(&s,&c,UINT64_MAX-100));
 OK(camera_network_prepare(&s,&q,UINT64_MAX-100));
 OK(camera_network_receipt_ack(&s,q.token,UINT64_MAX-100));
 OK(camera_network_next(&s,UINT64_MAX-100,&w));
 OK(camera_network_complete(&s,&w,0,NULL,UINT64_MAX-49));
 CHECK(camera_network_next(&s,UINT64_MAX-49,&w)==CAMERA_NETWORK_EXHAUSTED);
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&!s.live_present);
 c=config();OK(camera_network_init(&s,&c,1000));q=request(1);q.token.id=UINT64_MAX;
 OK(camera_network_prepare(&s,&q,1000));OK(camera_network_cancel(&s,q.token,1000));
 OK(camera_network_prepare(&s,&q,1000));q=request(1);
 CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_EXHAUSTED);
 c=config();OK(camera_network_init(&s,&c,1000));q=request(1);q.config_id=UINT64_MAX;
 OK(camera_network_prepare(&s,&q,1000));OK(camera_network_cancel(&s,q.token,1000));q=request(2);
 CHECK(camera_network_prepare(&s,&q,1000)==CAMERA_NETWORK_CONFLICT);
}
static void clocks(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1),q2=request(2);
 struct camera_network_work w,b;struct camera_network_link p;
 init(&s);OK(camera_network_prepare(&s,&q,1000));
 CHECK(camera_network_tick(&s,999)==CAMERA_NETWORK_CLOCK);
 CHECK(s.last_ns==1000&&s.clock_fault&&report(&s,q.token).state==CAMERA_NETWORK_FAILED);
 CHECK(camera_network_prepare(&s,&q2,1000)==CAMERA_NETWORK_CLOCK);
 init(&s);w=start(&s,q);CHECK(camera_network_tick(&s,1000)==CAMERA_NETWORK_CLOCK);
 CHECK(report(&s,q.token).in_flight&&report(&s,q.token).state==CAMERA_NETWORK_CANCEL_WAIT);
 CHECK(camera_network_complete(&s,&w,1,NULL,1000)==CAMERA_NETWORK_CLOCK);
 CHECK(report(&s,q.token).in_flight);OK(camera_network_complete(&s,&w,1,NULL,1001));
 OK(camera_network_next(&s,1001,&b));p=link(&b);
 OK(camera_network_complete(&s,&b,1,&p,1002));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&!report(&s,q.token).old_link_known);
 CHECK(camera_network_prepare(&s,&q2,1002)==CAMERA_NETWORK_CLOCK);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,1,NULL,1002));p=link(&w);
 CHECK(camera_network_link_proof(&s,&p,1001)==CAMERA_NETWORK_CLOCK);
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_ROLLBACK_PENDING);
 CHECK(camera_network_link_proof(&s,&p,1002)==CAMERA_NETWORK_ORDER);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,0,NULL,1002));
 OK(camera_network_next(&s,1003,&w));p=link(&w);
 CHECK(camera_network_complete(&s,&w,1,&p,1002)==CAMERA_NETWORK_CLOCK);
 CHECK(report(&s,q.token).in_flight);OK(camera_network_complete(&s,&w,1,&p,1003));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED);
}
static void boundaries(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1),q2=request(2);
 struct camera_network_work w,b;struct camera_network_link p,z;
 /* Deadline checks happen in completion itself without relying on a prior tick. */
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,1,NULL,1100));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_ROLLBACK_PENDING);
 restored(&s,q.token,1101);
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,0,NULL,1002));
 OK(camera_network_next(&s,1003,&w));p=link(&w);
 OK(camera_network_complete(&s,&w,1,&p,1053));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&
       report(&s,q.token).rollback_reason==CAMERA_NETWORK_REASON_TIMEOUT);
 /* Cancel cannot release/replace or redispatch an in-flight rollback. */
 init(&s);w=start(&s,q);OK(camera_network_complete(&s,&w,0,NULL,1002));
 OK(camera_network_next(&s,1003,&w));p=link(&w);
 OK(camera_network_cancel(&s,q.token,1004));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_ROLLING_BACK&&report(&s,q.token).in_flight);
 CHECK(camera_network_prepare(&s,&q2,1004)==CAMERA_NETWORK_BUSY);
 CHECK(camera_network_next(&s,1004,&b)==CAMERA_NETWORK_NOT_READY);
 z=p;z.token.namespace_id++;
 CHECK(camera_network_complete(&s,&w,1,&z,1004)==CAMERA_NETWORK_STALE);
 z=p;z.token.id++;
 CHECK(camera_network_complete(&s,&w,1,&z,1004)==CAMERA_NETWORK_STALE);
 OK(camera_network_complete(&s,&w,1,&p,1005));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_RESTORED);
 /* All configured policy facts, including route when explicitly requested. */
 init(&s);q.required_facts=FACTS|CAMERA_NETWORK_ROUTE;w=start(&s,q);p=link(&w);
 OK(camera_network_complete(&s,&w,1,NULL,1002));
 CHECK(camera_network_link_proof(&s,&p,1003)==CAMERA_NETWORK_NOT_READY);
 p.facts|=CAMERA_NETWORK_ROUTE;p.observation_id++;
 OK(camera_network_link_proof(&s,&p,1003));
 CHECK(camera_network_confirm(&s,q.token,7,1002)==CAMERA_NETWORK_CLOCK);
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_ROLLBACK_PENDING);
 CHECK(report(&s,q.token).committed_config_id==10&&report(&s,q.token).committed_revision==7);
 /* Prior apply/rollback instances cannot acknowledge a newer live action. */
 q=request(1);init(&s);w=start(&s,q);OK(camera_network_cancel(&s,q.token,1002));
 OK(camera_network_complete(&s,&w,1,NULL,1003));OK(camera_network_next(&s,1004,&b));
 p=link(&b);OK(camera_network_complete(&s,&b,1,&p,1005));
 OK(camera_network_prepare(&s,&q2,1006));OK(camera_network_receipt_ack(&s,q2.token,1006));
 OK(camera_network_next(&s,1006,&b));
 CHECK(camera_network_complete(&s,&w,1,NULL,1006)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_link_proof(&s,&p,1006)==CAMERA_NETWORK_ORDER);
 CHECK(report(&s,q2.token).in_flight);
}
static void namespaces(void)
{
 struct camera_network_transition a,b;struct camera_network_config c=config();
 struct camera_network_request q=request(1),qb=q;
 struct camera_network_work wa,wb;struct camera_network_link p;
 init(&a);wa=start(&a,q);c.namespace_id=102;qb.token.namespace_id=102;
 OK(camera_network_init(&b,&c,1000));cases++;
 OK(camera_network_prepare(&b,&qb,1000));OK(camera_network_receipt_ack(&b,qb.token,1001));
 OK(camera_network_next(&b,1001,&wb));
 CHECK(wa.token.id==wb.token.id&&wa.radio_generation==wb.radio_generation);
 CHECK(camera_network_complete(&b,&wa,1,NULL,1002)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_cancel(&b,q.token,1002)==CAMERA_NETWORK_STALE);
 OK(camera_network_complete(&b,&wb,1,NULL,1002));p=link(&wa);
 CHECK(camera_network_link_proof(&b,&p,1003)==CAMERA_NETWORK_STALE);
 CHECK(report(&b,qb.token).state==CAMERA_NETWORK_WAIT_LINK&&report(&a,q.token).in_flight);
 p=link(&wb);OK(camera_network_link_proof(&b,&p,1003));
 OK(camera_network_confirm(&b,qb.token,7,1004));
 CHECK(report(&b,qb.token).state==CAMERA_NETWORK_CONFIRMED&&report(&a,q.token).in_flight);
}
static void observations(void)
{
 struct camera_network_transition s;struct camera_network_request q=request(1);
 struct camera_network_work w;struct camera_network_link p,z;
 init(&s);w=start(&s,q);p=link(&w);
 OK(camera_network_complete(&s,&w,1,NULL,1002));OK(camera_network_link_proof(&s,&p,1003));
 z=p;z.facts=0;
 CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE); /* equal-ID conflict */
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_CONFIRM_READY);
 z.observation_id=2;
 CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_link_proof(&s,&p,1003)==CAMERA_NETWORK_STALE); /* late formerly valid */
 CHECK(camera_network_confirm(&s,q.token,7,1003)==CAMERA_NETWORK_ORDER);
 p.observation_id=UINT64_MAX;OK(camera_network_link_proof(&s,&p,1003));
 z=p;z.observation_id=1;z.facts=0;
 CHECK(camera_network_link_proof(&s,&z,1003)==CAMERA_NETWORK_STALE); /* no sequence wrap */
 OK(camera_network_link_proof(&s,&p,1003));OK(camera_network_confirm(&s,q.token,7,1004));
 CHECK(report(&s,q.token).link_observation_id==UINT64_MAX);
}
static struct camera_network_work rollback_start(struct camera_network_transition *s,
 struct camera_network_request q)
{
 struct camera_network_work w;
 init(s);w=start(s,q);OK(camera_network_complete(s,&w,0,NULL,1002));
 OK(camera_network_next(s,1003,&w));CHECK(w.effect==CAMERA_NETWORK_ROLLBACK);
 CHECK(report(s,q.token).link_observation_id==0);return w;
}
static void rollback_live(struct camera_network_transition *s,
 struct camera_network_token t,uint64_t observation,uint32_t facts)
{
 struct camera_network_report r=report(s,t);
 CHECK(r.state==CAMERA_NETWORK_ROLLING_BACK&&r.in_flight&&s->live_present);
 CHECK(!r.old_link_known&&!r.rollback_completed&&!r.rollback_succeeded);
 CHECK(r.link_observation_id==observation&&r.link_facts==facts);
 CHECK(r.committed_config_id==10&&r.committed_revision==7);
 CHECK(camera_network_forget(s,t)==CAMERA_NETWORK_BUSY);
}
static void rollback_observations(void)
{
 struct camera_network_transition s,before;struct camera_network_request q=request(1),q2=request(2);
 struct camera_network_work w,out;struct camera_network_link p,loss,z;
 /* Positive then loss: the old positive cannot retire the command or restore. */
 w=rollback_start(&s,q);p=link(&w);OK(camera_network_link_proof(&s,&p,1004));
 rollback_live(&s,q.token,1,FACTS);
 loss=p;loss.observation_id=2;loss.facts=CAMERA_NETWORK_ADDRESS;
 CHECK(camera_network_link_proof(&s,&loss,1005)==CAMERA_NETWORK_NOT_READY);
 rollback_live(&s,q.token,2,CAMERA_NETWORK_ADDRESS);
 CHECK(camera_network_complete(&s,&w,1,&p,UINT64_MAX)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_complete(&s,&w,1,&p,0)==CAMERA_NETWORK_STALE);
 CHECK(s.last_ns==1005&&!s.clock_fault);
 z=p;z.observation_id=2;
 CHECK(camera_network_complete(&s,&w,1,&z,1005)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_link_proof(&s,&z,1005)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_complete(&s,&w,1,&loss,1005)==CAMERA_NETWORK_STALE);
 rollback_live(&s,q.token,2,CAMERA_NETWORK_ADDRESS);
 CHECK(camera_network_prepare(&s,&q2,1005)==CAMERA_NETWORK_BUSY);
 CHECK(camera_network_next(&s,1005,&out)==CAMERA_NETWORK_NOT_READY);
 p.observation_id=3;OK(camera_network_link_proof(&s,&p,1006));
 OK(camera_network_link_proof(&s,&p,1006));rollback_live(&s,q.token,3,FACTS);
 CHECK(camera_network_link_proof(&s,&loss,UINT64_MAX)==CAMERA_NETWORK_STALE);
 CHECK(s.last_ns==1006);OK(camera_network_complete(&s,&w,1,&p,1007));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_RESTORED&&report(&s,q.token).old_link_known);
 CHECK(report(&s,q.token).link_observation_id==3&&!report(&s,q.token).in_flight);
 /* Reordered delivery: loss arrives first; formerly observed positive arrives late. */
 w=rollback_start(&s,q);p=link(&w);loss=p;loss.observation_id=2;loss.facts=0;
 CHECK(camera_network_link_proof(&s,&loss,1004)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_link_proof(&s,&p,1005)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_complete(&s,&w,1,&p,1005)==CAMERA_NETWORK_STALE);
 rollback_live(&s,q.token,2,0);
 p.observation_id=3;OK(camera_network_complete(&s,&w,1,&p,1005)); /* fresh direct proof */
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_RESTORED&&report(&s,q.token).link_observation_id==3);
 /* Unknown bits are retained raw: duplicates are INVALID; equal-ID changes STALE. */
 w=rollback_start(&s,q);p=link(&w);loss=p;loss.observation_id=2;loss.facts=FACTS|16U;
 CHECK(camera_network_link_proof(&s,&loss,1004)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_link_proof(&s,&loss,1004)==CAMERA_NETWORK_INVALID);
 rollback_live(&s,q.token,2,FACTS|16U);
 z=loss;z.facts|=32U;
 CHECK(camera_network_link_proof(&s,&z,1004)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_complete(&s,&w,1,&p,1005)==CAMERA_NETWORK_STALE);
 p.observation_id=2;
 CHECK(camera_network_complete(&s,&w,1,&p,1005)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_link_proof(&s,&p,1005)==CAMERA_NETWORK_STALE);
 rollback_live(&s,q.token,2,FACTS|16U);
 p.observation_id=3;OK(camera_network_link_proof(&s,&p,1005));
 OK(camera_network_complete(&s,&w,1,&p,1006));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_RESTORED);
 /* A newer loss/unknown delivered in a denied completion is still retained. */
 for(unsigned unknown=0;unknown<2;unknown++){
  w=rollback_start(&s,q);p=link(&w);OK(camera_network_link_proof(&s,&p,1004));
  loss=p;loss.observation_id=2;loss.facts=unknown?FACTS|16U:0;
  CHECK(camera_network_complete(&s,&w,1,&loss,1005)==CAMERA_NETWORK_STALE);
  rollback_live(&s,q.token,2,loss.facts);
  CHECK(camera_network_complete(&s,&w,1,&p,1005)==CAMERA_NETWORK_STALE);
  rollback_live(&s,q.token,2,loss.facts);
  if(unknown){
   OK(camera_network_complete(&s,&w,0,NULL,1006));
   CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED);
  }else{
   p.observation_id=3;OK(camera_network_complete(&s,&w,1,&p,1006));
   CHECK(report(&s,q.token).state==CAMERA_NETWORK_RESTORED);
  }
 }
 /* Explicit failure acknowledges actual retirement even after loss/unknown bits. */
 for(unsigned unknown=0;unknown<2;unknown++){
  w=rollback_start(&s,q);p=link(&w);loss=p;loss.observation_id=2;
  loss.facts=unknown?FACTS|16U:0;
  CHECK(camera_network_link_proof(&s,&loss,1004)==
   (unknown?CAMERA_NETWORK_INVALID:CAMERA_NETWORK_NOT_READY));
  CHECK(camera_network_complete(&s,&w,1,&p,1005)==CAMERA_NETWORK_STALE);
  rollback_live(&s,q.token,2,loss.facts);
  OK(camera_network_complete(&s,&w,0,unknown?NULL:&p,1005));
  CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&!report(&s,q.token).old_link_known);
  CHECK(report(&s,q.token).rollback_completed&&!report(&s,q.token).rollback_succeeded);
  CHECK(report(&s,q.token).committed_config_id==10&&report(&s,q.token).committed_revision==7);
 }
 /* Wrong instance cannot invalidate good proof, nor change the monotonic clock. */
 w=rollback_start(&s,q);p=link(&w);OK(camera_network_link_proof(&s,&p,1004));
 for(unsigned field=0;field<6;field++){
  z=p;z.observation_id=999;z.facts=0;
  if(field==0)z.token.namespace_id++;
  if(field==1)z.token.id++;
  if(field==2)z.config_id=q.config_id;
  if(field==3)z.radio_generation=report(&s,q.token).apply_generation;
  if(field==4)z.observation_id=0;
  if(field==5)z.observation_id=1; /* conflicting same sequence */
  memcpy(&before,&s,sizeof(s));
  CHECK(camera_network_link_proof(&s,&z,field&1U?0:UINT64_MAX)==CAMERA_NETWORK_STALE);
  CHECK(!memcmp(&before,&s,sizeof(s)));
 }
 rollback_live(&s,q.token,1,FACTS);OK(camera_network_complete(&s,&w,1,&p,1005));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_RESTORED);
 /* Timeout retains work and observations; late fresh proof can retire but not restore. */
 w=rollback_start(&s,q);p=link(&w);loss=p;loss.observation_id=2;loss.facts=0;
 CHECK(camera_network_link_proof(&s,&loss,1004)==CAMERA_NETWORK_NOT_READY);
 OK(camera_network_tick(&s,1053));rollback_live(&s,q.token,2,0);
 CHECK(report(&s,q.token).rollback_reason==CAMERA_NETWORK_REASON_TIMEOUT);
 CHECK(camera_network_next(&s,1053,&out)==CAMERA_NETWORK_NOT_READY);
 p.observation_id=3;
 CHECK(camera_network_link_proof(&s,&p,1053)==CAMERA_NETWORK_NOT_READY);
 rollback_live(&s,q.token,3,FACTS);z=p;z.observation_id=1;
 CHECK(camera_network_complete(&s,&w,1,&z,1054)==CAMERA_NETWORK_STALE);
 rollback_live(&s,q.token,3,FACTS);OK(camera_network_complete(&s,&w,1,&p,1054));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&!report(&s,q.token).old_link_known);
 CHECK(report(&s,q.token).rollback_completed&&!report(&s,q.token).rollback_succeeded);
 /* Clock fault cannot turn fresh positive facts into restoration. */
 w=rollback_start(&s,q);p=link(&w);loss=p;loss.observation_id=2;loss.facts=0;
 CHECK(camera_network_link_proof(&s,&loss,1004)==CAMERA_NETWORK_NOT_READY);
 p.observation_id=3;CHECK(camera_network_link_proof(&s,&p,1003)==CAMERA_NETWORK_CLOCK);
 rollback_live(&s,q.token,2,0);
 CHECK(camera_network_link_proof(&s,&p,1004)==CAMERA_NETWORK_NOT_READY);
 rollback_live(&s,q.token,3,FACTS);OK(camera_network_complete(&s,&w,1,&p,1005));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&
  report(&s,q.token).rollback_reason==CAMERA_NETWORK_REASON_CLOCK);
 /* Maximum observed sequence cannot wrap to an older proof; explicit failure still retires. */
 w=rollback_start(&s,q);p=link(&w);loss=p;loss.observation_id=UINT64_MAX;loss.facts=0;
 CHECK(camera_network_link_proof(&s,&loss,1004)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_complete(&s,&w,1,&p,1005)==CAMERA_NETWORK_STALE);
 p.observation_id=UINT64_MAX;
 CHECK(camera_network_complete(&s,&w,1,&p,1005)==CAMERA_NETWORK_STALE);
 rollback_live(&s,q.token,UINT64_MAX,0);OK(camera_network_complete(&s,&w,0,NULL,1005));
 CHECK(report(&s,q.token).state==CAMERA_NETWORK_FAILED&&!report(&s,q.token).in_flight);
}

/* Bounded deterministic fixture exploration: no production retry loop. */
static void explore(void)
{
 uint32_t seed=1234567;
 for(unsigned episode=0;episode<48;episode++){
  struct camera_network_transition s;struct camera_network_request q=request(1);
  struct camera_network_work w={0},out;struct camera_network_link p;
  uint64_t now=1000;init(&s);OK(camera_network_prepare(&s,&q,now));
  for(unsigned step=0;step<64;step++){
   struct camera_network_report before=report(&s,q.token),after;
   uint32_t was_live=s.live_present;struct camera_network_work old_live=s.live;
   seed=seed*1664525U+1013904223U;now+=(seed>>28)&3U;
   switch((seed>>16)%9U){
   case 0: (void)camera_network_receipt_ack(&s,q.token,now);break;
   case 1:
    if(camera_network_next(&s,now,&out)==CAMERA_NETWORK_OK){
     w=out;CHECK(before.receipt_acknowledged);
     CHECK(!was_live);
    }
    break;
   case 2: (void)camera_network_cancel(&s,q.token,now);break;
   case 3: OK(camera_network_tick(&s,now));break;
   case 4:
    if(s.live_present){
     w=s.live;p=link(&w);
     (void)camera_network_complete(&s,&w,(seed>>8)&1U,
       w.effect==CAMERA_NETWORK_ROLLBACK?&p:NULL,now);
    }
    break;
   case 5:
    p=link(&w);p.observation_id=step+1;p.facts=(seed>>8)&15U;
    (void)camera_network_link_proof(&s,&p,now);break;
   case 6: (void)camera_network_confirm(&s,q.token,7,now);break;
   case 7: OK(camera_network_prepare(&s,&q,now));break;
   case 8:
    out=w;out.token.namespace_id++;
    CHECK(camera_network_complete(&s,&out,0,NULL,now)==CAMERA_NETWORK_STALE);break;
   }
   after=report(&s,q.token);
   CHECK(after.committed_revision==7||after.state==CAMERA_NETWORK_CONFIRMED);
   CHECK(after.committed_config_id==10||after.state==CAMERA_NETWORK_CONFIRMED);
   CHECK(after.in_flight==s.live_present);
   if(was_live&&s.live_present){
    CHECK(s.live.token.id==old_live.token.id&&s.live.radio_generation==old_live.radio_generation);
    CHECK(s.live.effect==old_live.effect&&s.live.config_id==old_live.config_id);
   }
   if(after.state==CAMERA_NETWORK_CONFIRMED){
    CHECK(after.receipt_acknowledged&&(after.link_facts&FACTS)==FACTS);
    CHECK(!s.live_present&&s.config.committed_revision==8&&s.config.committed_config_id==11);
   }
   if(after.state==CAMERA_NETWORK_RESTORED){
    CHECK(!s.live_present&&after.rollback_completed&&after.rollback_succeeded&&after.old_link_known);
   }
   if(after.in_flight){
    CHECK(camera_network_forget(&s,q.token)==CAMERA_NETWORK_BUSY);
   }
  }
 }
}
static void invalid(void)
{
 struct camera_network_transition s={0};struct camera_network_config c=config(),z;
 struct camera_network_request q=request(1),b;struct camera_network_work w={0};
 struct camera_network_report r;
 CHECK(camera_network_init(NULL,&c,0)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_init(&s,NULL,0)==CAMERA_NETWORK_INVALID);
 for(unsigned i=0;i<4;i++){
  z=c;
  if(i==0)z.namespace_id=0;
  if(i==1)z.committed_config_id=0;
  if(i==2)z.transition_timeout_ns=0;
  if(i==3)z.rollback_timeout_ns=0;
  CHECK(camera_network_init(&s,&z,0)==CAMERA_NETWORK_INVALID);
 }
 CHECK(camera_network_tick(&s,0)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_query(&s,q.token,&r)==CAMERA_NETWORK_INVALID);
 init(&s);
 for(unsigned i=0;i<8;i++){
  b=q;
  if(i==0)b.token.id=0;
  if(i==1)b.token.namespace_id++;
  if(i==2)b.config_id=0;
  if(i==3)b.kind=0;
  if(i==4)b.kind=3;
  if(i==5)b.required_facts=0;
  if(i==6)b.required_facts=CAMERA_NETWORK_ADDRESS;
  if(i==7)b.required_facts=FACTS|16;
  CHECK(camera_network_prepare(&s,&b,1000)==CAMERA_NETWORK_INVALID);
 }
 b=q;b.expected_revision--;CHECK(camera_network_prepare(&s,&b,1000)==CAMERA_NETWORK_CONFLICT);
 b=q;b.kind=CAMERA_NETWORK_SCAN;CHECK(camera_network_prepare(&s,&b,1000)==CAMERA_NETWORK_CONFLICT);
 CHECK(camera_network_prepare(&s,NULL,1000)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_next(&s,1000,NULL)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_query(&s,q.token,NULL)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_link_proof(&s,NULL,1000)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_complete(&s,NULL,1,NULL,1000)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_complete(&s,&w,1,NULL,1000)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_cancel(&s,q.token,1000)==CAMERA_NETWORK_STALE);
 CHECK(camera_network_forget(&s,q.token)==CAMERA_NETWORK_STALE);
 CHECK(!s.live_present&&s.operation_high_water==0&&s.radio_high_water==30);
}
int main(void)
{
 positive();receipt_and_order();failures_and_rollback();stale_proofs();
 cancellation();timeouts();scans();retention_and_overflow();clocks();boundaries();namespaces();observations();
 rollback_observations();invalid();explore();
 printf("synthetic=1 cases=%u checks=%u passed=1\n",cases,checks);return 0;
}
