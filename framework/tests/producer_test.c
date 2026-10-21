/* SPDX-License-Identifier: GPL-2.0-only; logical ownership, not hardware proof. */
#include <camera/producer.h>
#include <camera/lease_pool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct events {struct camera_producer_event event[256];uint32_t count;};
static void event(void *context,const struct camera_producer_event *e)
{
 struct events *log=context;
 assert(log->count<256);log->event[log->count++]=*e;
}
static struct camera_producer_config config(struct events *log)
{
 struct camera_producer_config c={0};c.operation_timeout_ns=100;
 c.keyframe_supported=1;c.event=event;c.event_context=log;
 for(unsigned i=0;i<3;i++)c.controls[i].set_supported=1;
 c.controls[1].stop_supported=1;return c;
}
static void init(struct camera_producer *p,struct events *log)
{struct camera_producer_config c=config(log);assert(camera_producer_init(p,&c)==CAMERA_OK);}
static struct camera_producer_work next(struct camera_producer *p,uint64_t now,
                                       enum camera_producer_work_kind kind)
{
 struct camera_producer_work w;
 assert(camera_producer_next(p,now,CAMERA_PRODUCER_PROVED_BOUNDARY,&w)==CAMERA_OK);
 assert(w.kind==kind&&w.token.owner==p&&w.token.generation==p->generation);return w;
}
static struct camera_producer_demand start(struct camera_producer *p,uint64_t now)
{
 struct camera_producer_demand d;
 assert(camera_producer_attach(p,CAMERA_PRODUCER_RTSP,now,&d)==CAMERA_OK);
 struct camera_producer_work w=next(p,now,CAMERA_PRODUCER_START);
 assert(camera_producer_complete(p,w.token,CAMERA_OK,now+1)==CAMERA_OK);
 assert(p->state==CAMERA_PRODUCER_RUNNING);return d;
}
static struct camera_producer_command command(uint32_t key,int64_t value)
{
 struct camera_producer_command c={0};c.key=key;c.kind=CAMERA_PRODUCER_SET;
 c.value.word[0]=value;return c;
}
static struct camera_producer_token submit(struct camera_producer *p,uint32_t key,int64_t value,uint64_t now)
{
 struct camera_producer_command c=command(key,value);struct camera_producer_token t;
 assert(camera_producer_submit(p,&c,now,&t)==CAMERA_OK);return t;
}
static struct camera_producer_control_state snapshot(struct camera_producer *p,uint32_t key)
{
 struct camera_producer_control_state s;
 assert(camera_producer_control_snapshot(p,key,&s)==CAMERA_OK);return s;
}
static void shutdown(struct camera_producer *p,uint64_t now)
{
 assert(camera_producer_shutdown(p,now)==CAMERA_OK);
 assert(!camera_producer_quiescent(p));
 struct camera_producer_work w=next(p,now,CAMERA_PRODUCER_STOP);
 assert(camera_producer_shutdown(p,now)==CAMERA_OK); /* does not cancel STOP */
 int result;struct camera_producer_work active;
 assert(camera_producer_active(p,&active,&result)==CAMERA_OK&&result==CAMERA_PRODUCER_PENDING);
 assert(camera_producer_complete(p,w.token,CAMERA_OK,now+1)==CAMERA_OK);
 assert(camera_producer_quiescent(p));
}
static unsigned notifications(const struct events *log,struct camera_producer_token t)
{
 unsigned count=0;
 for(unsigned i=0;i<log->count;i++)
  if(log->event[i].work.token.owner==t.owner&&log->event[i].work.token.id==t.id)count++;
 return count;
}

/* R1: transient demand must not cancel/repeat an already dispatched cleanup.
 * Exercise ordinary/recovery STOP, repeated churn, retained demand, and real
 * failure/timeout. Assertions express the contract, not the reviewed defects.
 */
static void lifecycle_stop_churn(void)
{
 for(unsigned recovery=0;recovery<2;recovery++)
 for(unsigned retained=0;retained<2;retained++)
 for(unsigned outcome=0;outcome<3;outcome++){
  struct camera_producer p;struct events log={0};init(&p,&log);
  struct camera_producer_demand first=start(&p,0),transient,keeper;
  if(recovery){
   assert(camera_producer_source_failed(&p,CAMERA_IO_ERROR,2)==CAMERA_OK);
   assert(camera_producer_recover(&p,2)==CAMERA_OK);
  }else assert(camera_producer_detach(&p,first,2)==CAMERA_OK);
  struct camera_producer_work stop=next(&p,2,CAMERA_PRODUCER_STOP),active;
  uint64_t generation=p.generation;int result;
  if(recovery){
   assert(p.state==CAMERA_PRODUCER_RECOVERING);
   assert(camera_producer_detach(&p,first,3)==CAMERA_OK);
  }
  for(unsigned i=0;i<8;i++){
   uint64_t now=3+2*i;
   assert(camera_producer_attach(&p,CAMERA_PRODUCER_UI,now,&transient)==CAMERA_OK);
   assert(camera_producer_detach(&p,transient,now+1)==CAMERA_OK);
   assert(camera_producer_detach(&p,transient,now+1)==CAMERA_OK);
   assert(p.state==CAMERA_PRODUCER_DRAINING&&p.demand_count==0&&p.needs_stop);
   assert(camera_producer_active(&p,&active,&result)==CAMERA_OK);
   assert(result==CAMERA_PRODUCER_PENDING&&active.token.id==stop.token.id);
   assert(active.deadline_ns==stop.deadline_ns&&p.generation==generation);
   assert(notifications(&log,stop.token)==0);
   assert(camera_producer_next(&p,now+1,CAMERA_PRODUCER_NO_BOUNDARY,&active)==CAMERA_NOT_READY);
  }
  if(retained)assert(camera_producer_attach(&p,CAMERA_PRODUCER_RECORDER,20,&keeper)==CAMERA_OK);
  uint64_t now=21;
  if(outcome==2){
   now=stop.deadline_ns;
   assert(camera_producer_tick(&p,now)==CAMERA_OK);
   assert(p.state==CAMERA_PRODUCER_FAILED&&p.needs_stop&&!camera_producer_quiescent(&p));
   assert(camera_producer_active(&p,&active,&result)==CAMERA_OK&&result==CAMERA_PRODUCER_TIMEOUT);
   assert(camera_producer_next(&p,now,CAMERA_PRODUCER_NO_BOUNDARY,&active)==CAMERA_NOT_READY);
   assert(camera_producer_complete(&p,stop.token,CAMERA_OK,++now)==CAMERA_OK);
  }else assert(camera_producer_complete(&p,stop.token,outcome?CAMERA_IO_ERROR:CAMERA_OK,now)==CAMERA_OK);
  assert(notifications(&log,stop.token)==1);
  assert(log.event[log.count-1].result==(outcome==2?CAMERA_PRODUCER_TIMEOUT:outcome?CAMERA_IO_ERROR:CAMERA_OK));
  assert(camera_producer_complete(&p,stop.token,CAMERA_OK,now)==CAMERA_STALE_LEASE);
  if(!outcome){
   assert(!p.needs_stop);
   if(retained){
    assert(p.state==CAMERA_PRODUCER_STARTING&&p.generation==generation+1&&p.demand_count==1);
    active=next(&p,now,CAMERA_PRODUCER_START); /* not redundant STOP or RECOVER */
    assert(camera_producer_complete(&p,active.token,CAMERA_OK,++now)==CAMERA_OK);
    shutdown(&p,now+1);
   }else{
    assert(p.state==CAMERA_PRODUCER_STOPPED&&p.generation==generation);
    assert(camera_producer_next(&p,now,CAMERA_PRODUCER_NO_BOUNDARY,&active)==CAMERA_NOT_READY);
    assert(camera_producer_shutdown(&p,now)==CAMERA_OK&&camera_producer_quiescent(&p));
   }
  }else{
   assert(p.state==CAMERA_PRODUCER_FAILED&&p.needs_stop);
   assert(camera_producer_next(&p,now,CAMERA_PRODUCER_NO_BOUNDARY,&active)==CAMERA_NOT_READY);
   if(retained){
    assert(camera_producer_recover(&p,++now)==CAMERA_OK);
    active=next(&p,now,CAMERA_PRODUCER_STOP); /* explicit retry after actual failure */
    assert(camera_producer_complete(&p,active.token,CAMERA_OK,++now)==CAMERA_OK);
    active=next(&p,now,CAMERA_PRODUCER_RECOVER);
    assert(camera_producer_complete(&p,active.token,CAMERA_OK,++now)==CAMERA_OK);
   }
   shutdown(&p,now+1);
  }
 }
}
static void assert_unknown_history(struct camera_producer *p,uint32_t key,
                                   const struct camera_producer_control_state *before)
{
 struct camera_producer_control_state s=snapshot(p,key);
 assert(s.applied_evidence==CAMERA_PRODUCER_APPLIED_UNKNOWN);
 assert(!memcmp(s.applied.word,before->applied.word,sizeof(s.applied.word)));
 assert(!memcmp(s.desired.word,before->desired.word,sizeof(s.desired.word)));
 assert(s.applied_generation==before->applied_generation&&s.applied_producer_generation==before->applied_producer_generation);
 assert(s.desired_generation==before->desired_generation&&s.applied_ns==before->applied_ns);
 assert(s.desired_operation.owner==before->desired_operation.owner&&s.desired_operation.id==before->desired_operation.id);
 assert(s.desired_operation.generation==before->desired_operation.generation);
 assert(s.desired_result==before->desired_result&&s.desired_reason==before->desired_reason&&s.result_ns==before->result_ns);
}
/* R2: drain/shutdown invalidates all current evidence before async STOP,
 * preserving both historical applied state and unchanged desired results.
 */
static void lifecycle_stop_evidence(void)
{
 for(unsigned shutting_down=0;shutting_down<2;shutting_down++){
  struct camera_producer p;struct events log={0};init(&p,&log);
  struct camera_producer_demand d=start(&p,0);
  struct camera_producer_token t=submit(&p,0,42,2);
  struct camera_producer_work w=next(&p,2,CAMERA_PRODUCER_CONTROL);
  assert(camera_producer_complete(&p,t,CAMERA_OK,3)==CAMERA_OK);
  t=submit(&p,2,84,3);w=next(&p,3,CAMERA_PRODUCER_CONTROL);
  assert(camera_producer_complete(&p,t,CAMERA_OK,4)==CAMERA_OK);
  struct camera_producer_control_state a=snapshot(&p,0),b=snapshot(&p,2);
  assert(a.applied_evidence==CAMERA_PRODUCER_APPLIED_ACKNOWLEDGED&&b.applied_evidence==CAMERA_PRODUCER_APPLIED_ACKNOWLEDGED);
  if(shutting_down)assert(camera_producer_shutdown(&p,5)==CAMERA_OK);
  else assert(camera_producer_detach(&p,d,5)==CAMERA_OK);
  assert(p.state==CAMERA_PRODUCER_DRAINING&&!p.active_present);
  assert_unknown_history(&p,0,&a);assert_unknown_history(&p,2,&b); /* before dispatch */
  w=next(&p,5,CAMERA_PRODUCER_STOP);
  assert_unknown_history(&p,0,&a);assert_unknown_history(&p,2,&b); /* async interval */
  assert(camera_producer_tick(&p,w.deadline_ns-1)==CAMERA_OK);
  struct camera_producer_work active;int result;
  assert(camera_producer_active(&p,&active,&result)==CAMERA_OK&&result==CAMERA_PRODUCER_PENDING);
  assert_unknown_history(&p,0,&a);assert_unknown_history(&p,2,&b);
  assert(camera_producer_complete(&p,w.token,CAMERA_OK,w.deadline_ns-1)==CAMERA_OK);
  assert_unknown_history(&p,0,&a);assert_unknown_history(&p,2,&b); /* after retirement */
  assert(camera_producer_shutdown(&p,w.deadline_ns-1)==CAMERA_OK&&camera_producer_quiescent(&p));
 }
}

static void demand_and_restart(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);
 struct camera_producer_demand rtsp,ui,recorder;
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_RTSP,0,&rtsp)==CAMERA_OK);
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_UI,0,&ui)==CAMERA_OK);
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_RECORDER,0,&recorder)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_STARTING&&p.demand_count==3&&p.generation==1);
 struct camera_producer_work w=next(&p,1,CAMERA_PRODUCER_START),out;
 assert(camera_producer_next(&p,1,CAMERA_PRODUCER_PROVED_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,2)==CAMERA_OK);
 assert(camera_producer_detach(&p,rtsp,3)==CAMERA_OK);
 assert(camera_producer_detach(&p,rtsp,3)==CAMERA_OK);
 assert(camera_producer_detach(&p,ui,3)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_RUNNING&&p.demand_count==1); /* recorder holds */
 assert(camera_producer_detach(&p,recorder,4)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_DRAINING);
 w=next(&p,4,CAMERA_PRODUCER_STOP);
 assert(camera_producer_detach(&p,recorder,4)==CAMERA_OK&&!p.active_terminal);
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_UI,5,&ui)==CAMERA_OK);
 assert(camera_producer_detach(&p,rtsp,5)==CAMERA_STALE_LEASE); /* reused slot */
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,6)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_STARTING&&p.generation==2);
 struct camera_producer_token old=w.token;
 w=next(&p,6,CAMERA_PRODUCER_START);
 assert(camera_producer_complete(&p,old,CAMERA_OK,6)==CAMERA_STALE_LEASE);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,7)==CAMERA_OK);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,7)==CAMERA_STALE_LEASE);
 shutdown(&p,8);
 assert(camera_producer_detach(&p,ui,9)==CAMERA_OK);
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_RTSP,9,&rtsp)==CAMERA_NOT_READY);
}
static void cancelled_start_orders(void)
{
 for(unsigned dispatched=0;dispatched<2;dispatched++){
  struct camera_producer p;struct events log={0};init(&p,&log);
  struct camera_producer_demand d;struct camera_producer_work w={0},out;
  assert(camera_producer_attach(&p,CAMERA_PRODUCER_RTSP,0,&d)==CAMERA_OK);
  if(dispatched)w=next(&p,0,CAMERA_PRODUCER_START);
  assert(camera_producer_detach(&p,d,1)==CAMERA_OK);
  assert(camera_producer_detach(&p,d,1)==CAMERA_OK);
  if(dispatched){
   assert(p.state==CAMERA_PRODUCER_DRAINING);
   assert(camera_producer_next(&p,1,CAMERA_PRODUCER_PROVED_BOUNDARY,&out)==CAMERA_NOT_READY);
   int result;
   assert(camera_producer_active(&p,&out,&result)==CAMERA_OK&&result==CAMERA_PRODUCER_CANCELLED);
   assert(camera_producer_complete(&p,w.token,CAMERA_OK,2)==CAMERA_OK);
   assert(p.state==CAMERA_PRODUCER_DRAINING); /* late start cannot resurrect */
   assert(notifications(&log,w.token)==1);
   assert(camera_producer_complete(&p,w.token,CAMERA_OK,2)==CAMERA_STALE_LEASE);
   w=next(&p,2,CAMERA_PRODUCER_STOP);
   assert(camera_producer_complete(&p,w.token,CAMERA_OK,3)==CAMERA_OK);
  }else assert(p.state==CAMERA_PRODUCER_STOPPED&&!p.needs_stop);
  assert(camera_producer_shutdown(&p,4)==CAMERA_OK&&camera_producer_quiescent(&p));
 }
}
static void keyframe_dedup(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);
 struct camera_producer_demand d;
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_RTSP,0,&d)==CAMERA_OK);
 struct camera_producer_token a,b;
 assert(camera_producer_request_keyframe(&p,0,&a)==CAMERA_OK);
 assert(camera_producer_request_keyframe(&p,0,&b)==CAMERA_OK&&a.id==b.id);
 struct camera_producer_work w=next(&p,1,CAMERA_PRODUCER_START);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,2)==CAMERA_OK);
 w=next(&p,2,CAMERA_PRODUCER_KEYFRAME);assert(w.token.id==a.id);
 assert(camera_producer_request_keyframe(&p,2,&b)==CAMERA_OK&&a.id==b.id);
 assert(camera_producer_cancel(&p,a,3)==CAMERA_OK);
 assert(camera_producer_cancel(&p,a,3)==CAMERA_OK); /* active cancel idempotent */
 assert(camera_producer_request_keyframe(&p,3,&b)==CAMERA_OK&&b.id!=a.id);
 assert(camera_producer_complete(&p,a,CAMERA_OK,4)==CAMERA_OK);
 assert(notifications(&log,a)==1);
 w=next(&p,4,CAMERA_PRODUCER_KEYFRAME);assert(w.token.id==b.id);
 assert(camera_producer_complete(&p,b,CAMERA_UNSUPPORTED,5)==CAMERA_OK);
 assert(log.event[log.count-1].result==CAMERA_UNSUPPORTED&&p.state==CAMERA_PRODUCER_RUNNING);
 assert(camera_producer_request_keyframe(&p,5,&a)==CAMERA_OK);
 assert(camera_producer_cancel(&p,a,5)==CAMERA_OK);
 assert(camera_producer_cancel(&p,a,5)==CAMERA_STALE_LEASE);
 assert(camera_producer_request_keyframe(&p,6,&a)==CAMERA_OK);
 w=next(&p,6,CAMERA_PRODUCER_KEYFRAME);
 assert(camera_producer_tick(&p,106)==CAMERA_OK&&p.state==CAMERA_PRODUCER_RUNNING);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,107)==CAMERA_OK);
 assert(notifications(&log,a)==1&&log.event[log.count-1].result==CAMERA_PRODUCER_TIMEOUT);
 shutdown(&p,108);
}
static void desired_applied_and_cancellation(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);start(&p,0);
 struct camera_producer_token a=submit(&p,0,10,2),b=submit(&p,0,20,2);
 struct camera_producer_control_state s=snapshot(&p,0);
 assert(s.desired.word[0]==20&&s.desired_generation==2&&s.applied_generation==0);
 struct camera_producer_work w=next(&p,2,CAMERA_PRODUCER_CONTROL);assert(w.token.id==a.id);
 assert(camera_producer_complete(&p,a,CAMERA_OK,3)==CAMERA_OK);
 assert(camera_producer_cancel(&p,a,3)==CAMERA_STALE_LEASE); /* completion wins */
 s=snapshot(&p,0);
 assert(s.applied.word[0]==10&&s.applied_generation==1&&s.applied_producer_generation==1);
 assert(s.applied_evidence==CAMERA_PRODUCER_APPLIED_ACKNOWLEDGED);
 assert(s.desired_result==CAMERA_PRODUCER_PENDING&&s.result_ns==2&&s.applied_ns==3);
 w=next(&p,3,CAMERA_PRODUCER_CONTROL);assert(w.token.id==b.id);
 assert(snapshot(&p,0).applied_evidence==CAMERA_PRODUCER_APPLIED_UNKNOWN);
 assert(camera_producer_complete(&p,b,CAMERA_BUSY,4)==CAMERA_OK);
 s=snapshot(&p,0);assert(s.applied_generation==1&&s.desired_generation==2&&s.desired_result==CAMERA_BUSY);
 assert(s.desired_reason==CAMERA_PRODUCER_REASON_BACKEND);
 assert(s.applied_evidence==CAMERA_PRODUCER_APPLIED_UNKNOWN); /* partial backend effects possible */
 a=submit(&p,0,30,4);w=next(&p,4,CAMERA_PRODUCER_CONTROL);
 assert(camera_producer_cancel(&p,a,5)==CAMERA_OK);
 s=snapshot(&p,0);assert(s.desired_result==CAMERA_PRODUCER_CANCELLED&&s.applied.word[0]==10);
 assert(camera_producer_complete(&p,a,CAMERA_OK,6)==CAMERA_OK);
 assert(snapshot(&p,0).applied_generation==1&&notifications(&log,a)==1);
 assert(snapshot(&p,0).applied_evidence==CAMERA_PRODUCER_APPLIED_UNKNOWN);
 a=submit(&p,0,40,6);assert(camera_producer_cancel(&p,a,6)==CAMERA_OK);
 assert(camera_producer_complete(&p,a,CAMERA_OK,6)==CAMERA_STALE_LEASE);
 assert(snapshot(&p,0).desired_result==CAMERA_PRODUCER_CANCELLED);
 shutdown(&p,7);
}
static void stop_priority_and_limits(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);start(&p,0);
 struct camera_producer_token active=submit(&p,1,7,2),queued[CAMERA_PRODUCER_QUEUE];
 struct camera_producer_work w=next(&p,2,CAMERA_PRODUCER_CONTROL),out;
 for(unsigned i=0;i<CAMERA_PRODUCER_QUEUE;i++)queued[i]=submit(&p,i%3,100+i,3);
 struct camera_producer_command c=command(0,999);struct camera_producer_token t;
 uint64_t before=p.next_id,desired=p.controls[0].desired_generation;
 assert(camera_producer_submit(&p,&c,3,&t)==CAMERA_BUSY);
 assert(p.next_id==before&&p.controls[0].desired_generation==desired);
 c=command(1,0);c.kind=CAMERA_PRODUCER_COMMAND_STOP;
 assert(camera_producer_submit(&p,&c,4,&t)==CAMERA_OK); /* reserved slot when full */
 assert(camera_producer_submit(&p,&c,4,&t)==CAMERA_BUSY);
 assert(p.active_terminal&&p.active_result==CAMERA_PRODUCER_CANCELLED);
 assert(camera_producer_next(&p,4,CAMERA_PRODUCER_PROVED_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(camera_producer_complete(&p,active,CAMERA_OK,5)==CAMERA_OK);
 assert(snapshot(&p,1).applied_generation==0);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,5)==CAMERA_STALE_LEASE);
 assert(camera_producer_next(&p,5,CAMERA_PRODUCER_NO_BOUNDARY,&out)==CAMERA_NOT_READY);
 w=next(&p,5,CAMERA_PRODUCER_CONTROL);
 assert(w.command.kind==CAMERA_PRODUCER_COMMAND_STOP&&w.command.key==1);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,6)==CAMERA_OK);
 assert(snapshot(&p,1).applied.word[0]==0);
 for(unsigned i=0;i<CAMERA_PRODUCER_QUEUE;i++){
  if(i%3==1){
   assert(notifications(&log,queued[i])==1);
   assert(camera_producer_complete(&p,queued[i],CAMERA_OK,6)==CAMERA_STALE_LEASE);
  }else{
   w=next(&p,6,CAMERA_PRODUCER_CONTROL);assert(w.token.id==queued[i].id);
   assert(camera_producer_complete(&p,w.token,CAMERA_OK,6)==CAMERA_OK);
  }
 }
 /* Lifecycle stop cancels both priority and ordinary control work. */
 active=submit(&p,0,3,7);w=next(&p,7,CAMERA_PRODUCER_CONTROL);
 c=command(1,0);c.kind=CAMERA_PRODUCER_COMMAND_STOP;
 assert(camera_producer_submit(&p,&c,7,&t)==CAMERA_OK);
 submit(&p,2,9,7);
 assert(camera_producer_shutdown(&p,8)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_DRAINING&&!p.queue_count&&!p.stop_pending);
 assert(camera_producer_complete(&p,active,CAMERA_OK,9)==CAMERA_OK);
 w=next(&p,9,CAMERA_PRODUCER_STOP);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,10)==CAMERA_OK);
 assert(camera_producer_quiescent(&p));
}
static void source_failure_and_recovery(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);start(&p,0);
 struct camera_producer_token good=submit(&p,0,10,2);
 struct camera_producer_work w=next(&p,2,CAMERA_PRODUCER_CONTROL);
 assert(camera_producer_complete(&p,good,CAMERA_OK,3)==CAMERA_OK);
 struct camera_producer_token active=submit(&p,0,20,4),pending=submit(&p,0,30,4),idr;
 w=next(&p,4,CAMERA_PRODUCER_CONTROL);
 assert(camera_producer_request_keyframe(&p,4,&idr)==CAMERA_OK);
 assert(camera_producer_source_failed(&p,CAMERA_IO_ERROR,5)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_FAILED&&p.demand_count==1);
 assert(snapshot(&p,0).desired_result==CAMERA_IO_ERROR&&snapshot(&p,0).applied.word[0]==10);
 assert(notifications(&log,pending)==1&&notifications(&log,idr)==1&&notifications(&log,active)==1);
 assert(log.event[log.count-1].work.kind==CAMERA_PRODUCER_NO_WORK);
 assert(log.event[log.count-1].reason==CAMERA_PRODUCER_REASON_SOURCE);
 assert(camera_producer_recover(&p,5)==CAMERA_BUSY);
 assert(camera_producer_complete(&p,active,CAMERA_OK,6)==CAMERA_OK);
 assert(camera_producer_recover(&p,6)==CAMERA_OK&&p.generation==2);
 assert(camera_producer_recover(&p,6)==CAMERA_NOT_READY);
 w=next(&p,6,CAMERA_PRODUCER_STOP);
 assert(camera_producer_complete(&p,w.token,CAMERA_BUSY,7)==CAMERA_OK&&p.state==CAMERA_PRODUCER_FAILED);
 assert(camera_producer_recover(&p,7)==CAMERA_OK&&p.generation==3);
 w=next(&p,7,CAMERA_PRODUCER_STOP);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,8)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_RECOVERING);
 w=next(&p,8,CAMERA_PRODUCER_RECOVER);
 assert(camera_producer_complete(&p,active,CAMERA_OK,8)==CAMERA_STALE_LEASE);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,9)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_RUNNING&&p.demand_count==1);
 assert(snapshot(&p,0).applied_producer_generation!=p.generation); /* must reapply config */
 assert(snapshot(&p,0).applied_evidence==CAMERA_PRODUCER_APPLIED_UNKNOWN);
 active=submit(&p,0,40,9);w=next(&p,9,CAMERA_PRODUCER_CONTROL);
 assert(camera_producer_complete(&p,active,CAMERA_OK,10)==CAMERA_OK);
 assert(snapshot(&p,0).applied_producer_generation==3&&snapshot(&p,0).applied.word[0]==40);
 shutdown(&p,11);
}
static void startup_failure_and_recovery_failure(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);
 struct camera_producer_demand d;
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_RECORDER,0,&d)==CAMERA_OK);
 struct camera_producer_work w=next(&p,0,CAMERA_PRODUCER_START),out;
 assert(camera_producer_cancel(&p,w.token,0)==CAMERA_INVALID);
 assert(camera_producer_complete(&p,w.token,CAMERA_UNSUPPORTED,1)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_FAILED&&p.needs_stop);
 assert(camera_producer_next(&p,2,CAMERA_PRODUCER_NO_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(camera_producer_recover(&p,2)==CAMERA_OK);
 w=next(&p,2,CAMERA_PRODUCER_STOP);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,3)==CAMERA_OK);
 w=next(&p,3,CAMERA_PRODUCER_RECOVER);
 assert(camera_producer_complete(&p,w.token,CAMERA_IO_ERROR,4)==CAMERA_OK&&p.state==CAMERA_PRODUCER_FAILED);
 assert(camera_producer_detach(&p,d,5)==CAMERA_OK&&p.state==CAMERA_PRODUCER_DRAINING);
 w=next(&p,5,CAMERA_PRODUCER_STOP);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,6)==CAMERA_OK);
 assert(camera_producer_shutdown(&p,6)==CAMERA_OK&&camera_producer_quiescent(&p));
}
static void recovery_timeout_and_predispatch_failure(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);
 struct camera_producer_demand d;
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_UI,0,&d)==CAMERA_OK);
 assert(camera_producer_source_failed(&p,CAMERA_DISCONNECTED,1)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_FAILED&&!p.needs_stop);
 assert(camera_producer_recover(&p,2)==CAMERA_OK);
 struct camera_producer_work w=next(&p,2,CAMERA_PRODUCER_RECOVER),out;
 assert(camera_producer_tick(&p,102)==CAMERA_OK&&p.state==CAMERA_PRODUCER_FAILED);
 assert(camera_producer_recover(&p,102)==CAMERA_BUSY);
 assert(camera_producer_next(&p,103,CAMERA_PRODUCER_NO_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,104)==CAMERA_OK);
 assert(p.state==CAMERA_PRODUCER_FAILED); /* late recovery cannot resurrect */
 assert(camera_producer_recover(&p,105)==CAMERA_OK);
 w=next(&p,105,CAMERA_PRODUCER_STOP);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,106)==CAMERA_OK);
 w=next(&p,106,CAMERA_PRODUCER_RECOVER);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,107)==CAMERA_OK);
 shutdown(&p,108);
}
static void deadlines_and_shutdown_failure(void)
{
 for(unsigned kind=0;kind<3;kind++){
  struct camera_producer p;struct events log={0};init(&p,&log);
  struct camera_producer_work w,out;int result;
  if(kind==0){
   struct camera_producer_demand d;
   assert(camera_producer_attach(&p,CAMERA_PRODUCER_UI,0,&d)==CAMERA_OK);
   w=next(&p,0,CAMERA_PRODUCER_START);
  }else{
   start(&p,0);
   if(kind==1){submit(&p,0,10,2);w=next(&p,2,CAMERA_PRODUCER_CONTROL);}
   else{assert(camera_producer_shutdown(&p,2)==CAMERA_OK);w=next(&p,2,CAMERA_PRODUCER_STOP);}
  }
  assert(camera_producer_tick(&p,w.deadline_ns-1)==CAMERA_OK);
  assert(camera_producer_active(&p,&out,&result)==CAMERA_OK&&result==CAMERA_PRODUCER_PENDING);
  /* Completion itself checks deadline, even if caller did not tick first. */
  assert(camera_producer_complete(&p,w.token,CAMERA_OK,w.deadline_ns)==CAMERA_OK);
  assert(log.event[log.count-1].result==CAMERA_PRODUCER_TIMEOUT);
  assert(notifications(&log,w.token)==1);
  if(kind==1){assert(p.state==CAMERA_PRODUCER_RUNNING&&snapshot(&p,0).applied_generation==0);}
  else assert(p.state==CAMERA_PRODUCER_FAILED&&p.needs_stop);
  assert(camera_producer_shutdown(&p,w.deadline_ns)==CAMERA_OK);
  assert(!camera_producer_quiescent(&p));
  out=next(&p,w.deadline_ns,CAMERA_PRODUCER_STOP);
  assert(camera_producer_complete(&p,out.token,CAMERA_IO_ERROR,w.deadline_ns+1)==CAMERA_OK);
  assert(!camera_producer_quiescent(&p)&&p.state==CAMERA_PRODUCER_FAILED);
  assert(camera_producer_shutdown(&p,w.deadline_ns+2)==CAMERA_OK);
  out=next(&p,w.deadline_ns+2,CAMERA_PRODUCER_STOP);
  assert(camera_producer_complete(&p,out.token,CAMERA_OK,w.deadline_ns+3)==CAMERA_OK);
  assert(camera_producer_quiescent(&p));
 }
 /* Tick timeout preserves active storage and prevents unbounded automatic work. */
 struct camera_producer p;struct events log={0};init(&p,&log);start(&p,0);
 struct camera_producer_token t=submit(&p,0,10,2);
 struct camera_producer_work w=next(&p,2,CAMERA_PRODUCER_CONTROL),out;
 assert(camera_producer_tick(&p,102)==CAMERA_OK&&p.active_present);
 assert(camera_producer_tick(&p,103)==CAMERA_OK&&notifications(&log,t)==1);
 assert(camera_producer_next(&p,103,CAMERA_PRODUCER_PROVED_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(camera_producer_shutdown(&p,104)==CAMERA_OK&&!camera_producer_quiescent(&p));
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,105)==CAMERA_OK);
 w=next(&p,105,CAMERA_PRODUCER_STOP);
 assert(camera_producer_complete(&p,w.token,CAMERA_OK,106)==CAMERA_OK&&camera_producer_quiescent(&p));
}
static void logical_demand_is_not_a_lease(void)
{
 struct camera_producer p;struct events log={0};init(&p,&log);start(&p,0);
 struct camera_lease_pool leases;assert(camera_pool_init(&leases,2)==CAMERA_OK);
 assert(camera_pool_queue(&leases,0)==CAMERA_OK);uint64_t generation;
 assert(camera_pool_device(&leases,0,&generation)==CAMERA_OK);
 struct camera_producer_token t=submit(&p,0,7,2);struct camera_producer_work out;
 assert(camera_producer_next(&p,2,CAMERA_PRODUCER_NO_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(camera_pool_stop(&leases,0)==CAMERA_BUSY); /* no invented retirement */
 assert(camera_pool_done(&leases,0,generation)==CAMERA_OK);
 assert(camera_pool_lease(&leases,0,&generation)==CAMERA_OK);
 assert(camera_producer_next(&p,3,CAMERA_PRODUCER_NO_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(camera_pool_release(&leases,0,generation)==CAMERA_OK);
 /* Adapter now supplies a proven boundary. A connected RTSP demand is not busy. */
 out=next(&p,4,CAMERA_PRODUCER_CONTROL);
 assert(p.demand_count==1&&out.token.id==t.id);
 assert(camera_producer_complete(&p,t,CAMERA_OK,5)==CAMERA_OK);
 shutdown(&p,6);
}
struct mock_engine {const struct camera_producer *owner;unsigned admission_attempts;};
static int admit(struct mock_engine *e,const struct camera_producer *p)
{
 e->admission_attempts++;
 if(e->owner)return CAMERA_BUSY;
 e->owner=p;return CAMERA_OK;
}
static void retire(struct mock_engine *e,const struct camera_producer *p)
{
 /* Failed admission never acquired another camera's engine ownership. */
 if(e->owner==p)e->owner=NULL;
}
static void two_cameras_one_mock_engine(void)
{
 struct camera_producer a,b;struct events loga={0},logb={0};init(&a,&loga);init(&b,&logb);
 struct mock_engine engine={0};struct camera_producer_demand da,db;
 assert(camera_producer_attach(&a,CAMERA_PRODUCER_RECORDER,0,&da)==CAMERA_OK);
 assert(camera_producer_attach(&b,CAMERA_PRODUCER_UI,0,&db)==CAMERA_OK);
 struct camera_producer_work wa=next(&a,0,CAMERA_PRODUCER_START),wb=next(&b,0,CAMERA_PRODUCER_START);
 assert(wa.token.id==wb.token.id); /* coincident numbers are not shared identities */
 assert(camera_producer_complete(&b,wa.token,CAMERA_OK,0)==CAMERA_STALE_LEASE);
 assert(camera_producer_detach(&b,da,0)==CAMERA_STALE_LEASE);
 assert(admit(&engine,&a)==CAMERA_OK);
 assert(camera_producer_complete(&a,wa.token,CAMERA_OK,1)==CAMERA_OK);
 assert(admit(&engine,&b)==CAMERA_BUSY);
 assert(camera_producer_complete(&b,wb.token,CAMERA_BUSY,1)==CAMERA_OK);
 assert(a.state==CAMERA_PRODUCER_RUNNING&&b.state==CAMERA_PRODUCER_FAILED);
 assert(camera_producer_recover(&b,1)==CAMERA_OK);
 wb=next(&b,1,CAMERA_PRODUCER_STOP);retire(&engine,&b);
 assert(engine.owner==&a); /* B's cleanup cannot stop A's engine */
 assert(camera_producer_complete(&b,wb.token,CAMERA_OK,2)==CAMERA_OK);
 wb=next(&b,2,CAMERA_PRODUCER_RECOVER);
 assert(admit(&engine,&b)==CAMERA_BUSY);
 assert(camera_producer_complete(&b,wb.token,CAMERA_BUSY,3)==CAMERA_OK);
 assert(a.state==CAMERA_PRODUCER_RUNNING&&engine.owner==&a);
 struct camera_producer_token t=submit(&a,0,10,2);
 wa=next(&a,2,CAMERA_PRODUCER_CONTROL);
 assert(camera_producer_complete(&a,t,CAMERA_OK,3)==CAMERA_OK);
 assert(snapshot(&b,0).desired_generation==0&&snapshot(&b,0).applied_generation==0);
 assert(camera_producer_detach(&a,da,4)==CAMERA_OK);
 wa=next(&a,4,CAMERA_PRODUCER_STOP);
 assert(engine.owner==&a);retire(&engine,&a); /* mock's explicit retirement */
 assert(camera_producer_complete(&a,wa.token,CAMERA_OK,5)==CAMERA_OK);
 assert(camera_producer_recover(&b,5)==CAMERA_OK);
 wb=next(&b,5,CAMERA_PRODUCER_STOP); /* failed start conservatively cleans up */
 retire(&engine,&b);
 assert(engine.owner==NULL);
 assert(camera_producer_complete(&b,wb.token,CAMERA_OK,6)==CAMERA_OK);
 wb=next(&b,6,CAMERA_PRODUCER_RECOVER);
 assert(admit(&engine,&b)==CAMERA_OK);
 assert(camera_producer_complete(&b,wb.token,CAMERA_OK,7)==CAMERA_OK);
 assert(engine.owner==&b&&b.demand_count==1&&a.demand_count==0&&engine.admission_attempts==4);
 assert(camera_producer_shutdown(&a,7)==CAMERA_OK&&camera_producer_quiescent(&a));
 assert(camera_producer_shutdown(&b,7)==CAMERA_OK);
 wb=next(&b,7,CAMERA_PRODUCER_STOP);assert(engine.owner==&b);retire(&engine,&b);
 assert(camera_producer_complete(&b,wb.token,CAMERA_OK,8)==CAMERA_OK&&camera_producer_quiescent(&b));
}
static void unsupported_invalid_bounds_and_exhaustion(void)
{
 struct camera_producer p;struct events log={0};struct camera_producer_config c=config(&log);
 assert(camera_producer_init(NULL,&c)==CAMERA_INVALID);
 c.operation_timeout_ns=0;assert(camera_producer_init(&p,&c)==CAMERA_INVALID);
 c=config(&log);c.keyframe_supported=0;assert(camera_producer_init(&p,&c)==CAMERA_OK);
 struct camera_producer_token t;struct camera_producer_command cmd=command(7,10);
 struct camera_producer_control_state s;struct camera_producer_work out;
 assert(camera_producer_request_keyframe(&p,0,&t)==CAMERA_UNSUPPORTED);
 assert(camera_producer_submit(&p,&cmd,0,&t)==CAMERA_UNSUPPORTED);
 assert(camera_producer_control_snapshot(&p,7,&s)==CAMERA_UNSUPPORTED);
 assert(p.next_id==0&&p.controls[7].desired_generation==0);
 cmd=command(0,10);cmd.kind=CAMERA_PRODUCER_COMMAND_STOP;
 assert(camera_producer_submit(&p,&cmd,0,&t)==CAMERA_UNSUPPORTED);
 cmd.kind=CAMERA_PRODUCER_SET;
 assert(camera_producer_submit(&p,&cmd,0,&t)==CAMERA_NOT_READY);
 struct camera_producer_demand d[CAMERA_PRODUCER_CONSUMERS],extra;
 for(unsigned i=0;i<CAMERA_PRODUCER_CONSUMERS;i++)
  assert(camera_producer_attach(&p,CAMERA_PRODUCER_UI,1,&d[i])==CAMERA_OK);
 assert(camera_producer_attach(&p,CAMERA_PRODUCER_UI,1,&extra)==CAMERA_BUSY);
 out=next(&p,1,CAMERA_PRODUCER_START);
 assert(camera_producer_complete(&p,out.token,17,2)==CAMERA_INVALID&&p.active_present);
 assert(camera_producer_complete(&p,out.token,CAMERA_OK,2)==CAMERA_OK);
 assert(camera_producer_tick(&p,1)==CAMERA_INVALID);
 assert(camera_producer_next(&p,2,(enum camera_producer_boundary)42,&out)==CAMERA_INVALID);
 p.controls[0].desired_generation=UINT64_MAX;
 assert(camera_producer_submit(&p,&cmd,2,&t)==CAMERA_NOT_READY);
 p.next_id=UINT64_MAX;cmd.key=1;
 assert(camera_producer_submit(&p,&cmd,2,&t)==CAMERA_NOT_READY&&p.controls[1].desired_generation==0);
 assert(camera_producer_shutdown(&p,3)==CAMERA_OK);
 assert(camera_producer_next(&p,3,CAMERA_PRODUCER_PROVED_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(!camera_producer_quiescent(&p)); /* exhaustion never wraps to stale ID */

 /* Fresh isolated contexts for counter/clock edge fixtures. */
 struct camera_producer q;struct events logq={0};init(&q,&logq);
 q.generation=UINT64_MAX;
 assert(camera_producer_attach(&q,CAMERA_PRODUCER_UI,0,&extra)==CAMERA_NOT_READY);
 assert(q.demand_count==0&&q.state==CAMERA_PRODUCER_STOPPED);
 assert(camera_producer_shutdown(&q,0)==CAMERA_OK&&camera_producer_quiescent(&q));
 struct camera_producer r;struct events logr={0};init(&r,&logr);
 assert(camera_producer_attach(&r,CAMERA_PRODUCER_UI,UINT64_MAX-50,&extra)==CAMERA_OK);
 uint64_t before=r.next_id;
 assert(camera_producer_next(&r,UINT64_MAX-50,CAMERA_PRODUCER_NO_BOUNDARY,&out)==CAMERA_NOT_READY);
 assert(r.next_id==before&&!r.active_present);
 assert(camera_producer_shutdown(&r,UINT64_MAX-50)==CAMERA_OK&&camera_producer_quiescent(&r));
}
int main(int argc,char **argv)
{
 if(argc==2){
  if(!strcmp(argv[1],"stop-churn"))lifecycle_stop_churn();
  else if(!strcmp(argv[1],"stop-evidence"))lifecycle_stop_evidence();
  else return 2;
  puts("Focused lifecycle STOP regression PASS");return 0;
 }
 if(argc!=1)return 2;
 lifecycle_stop_churn();lifecycle_stop_evidence();
 demand_and_restart();cancelled_start_orders();keyframe_dedup();
 desired_applied_and_cancellation();stop_priority_and_limits();
 source_failure_and_recovery();startup_failure_and_recovery_failure();
 recovery_timeout_and_predispatch_failure();
 deadlines_and_shutdown_failure();logical_demand_is_not_a_lease();
 two_cameras_one_mock_engine();unsupported_invalid_bounds_and_exhaustion();
 puts("Producer lifecycle/control/cancellation/deadline/lease-boundary/two-camera mock fixtures PASS");
 return 0;
}
