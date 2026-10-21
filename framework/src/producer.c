// SPDX-License-Identifier: GPL-2.0-only
#include <camera/producer.h>
#include <string.h>

static int valid(const struct camera_producer *p)
{return p&&p->initialized;}
static int clock_check(struct camera_producer *p,uint64_t now)
{
 if(!valid(p)||now<p->last_ns)return CAMERA_INVALID;
 p->last_ns=now;return CAMERA_OK;
}
static int driver_result(int r)
{return r<=CAMERA_OK&&r>=CAMERA_DISCONNECTED;}
static int same(struct camera_producer_token a,struct camera_producer_token b)
{return a.owner==b.owner&&a.id==b.id&&a.generation==b.generation;}
static int token(struct camera_producer *p,struct camera_producer_token *t)
{
 if(p->next_id==UINT64_MAX)return CAMERA_NOT_READY;
 t->owner=p;t->id=++p->next_id;t->generation=p->generation;return CAMERA_OK;
}
static void notify(struct camera_producer *p,const struct camera_producer_work *w,
                   int result,enum camera_producer_reason reason,uint64_t now)
{
 if(w->kind==CAMERA_PRODUCER_CONTROL){
  struct camera_producer_control_state *s=&p->controls[w->command.key];
  if(same(s->desired_operation,w->token)){
   s->desired_result=result;s->desired_reason=reason;s->result_ns=now;
  }
 }
 if(p->config.event){
  struct camera_producer_event e;
  e.work=*w;e.state=p->state;e.result=result;e.reason=reason;e.monotonic_ns=now;
  p->config.event(p->config.event_context,&e);
 }
}
static void remove_queue(struct camera_producer *p,uint32_t i)
{
 for(uint32_t j=i+1;j<p->queue_count;j++)p->queue[j-1]=p->queue[j];
 p->queue_count--;
}
static void cancel_active(struct camera_producer *p,int result,
                          enum camera_producer_reason reason,uint64_t now)
{
 if(p->active_present&&!p->active_terminal){
  p->active_terminal=1;p->active_result=result;p->active_reason=reason;
  notify(p,&p->active,result,reason,now);
 }
}
static void cancel_pending(struct camera_producer *p,int result,
                           enum camera_producer_reason reason,uint64_t now)
{
 while(p->queue_count){
  struct camera_producer_work w=p->queue[0];remove_queue(p,0);
  notify(p,&w,result,reason,now);
 }
 if(p->stop_pending){p->stop_pending=0;notify(p,&p->stop,result,reason,now);}
 if(p->keyframe_pending){p->keyframe_pending=0;notify(p,&p->keyframe,result,reason,now);}
}
static void invalidate_applied(struct camera_producer *p)
{
 for(uint32_t i=0;i<CAMERA_PRODUCER_CONTROLS;i++)
  p->controls[i].applied_evidence=CAMERA_PRODUCER_APPLIED_UNKNOWN;
}
static void failed(struct camera_producer *p,int result,
                   enum camera_producer_reason reason,uint64_t now)
{
 p->state=CAMERA_PRODUCER_FAILED;invalidate_applied(p);
 cancel_pending(p,result,reason,now);cancel_active(p,result,reason,now);
}
static void drain(struct camera_producer *p,enum camera_producer_reason reason,uint64_t now)
{
 p->state=(p->needs_stop||p->active_present)?CAMERA_PRODUCER_DRAINING:CAMERA_PRODUCER_STOPPED;
 invalidate_applied(p);
 cancel_pending(p,CAMERA_PRODUCER_CANCELLED,reason,now);
 /* Cleanup already in progress must retire normally despite demand churn.
  * Last detach during recovery still changes state to DRAINING, so successful
  * STOP will not dispatch RECOVER without demand. Timeout/failure is unchanged.
  */
 if(p->active_present&&p->active.kind!=CAMERA_PRODUCER_STOP)
  cancel_active(p,CAMERA_PRODUCER_CANCELLED,reason,now);
}
int camera_producer_init(struct camera_producer *p,const struct camera_producer_config *c)
{
 if(!p||!c||!c->operation_timeout_ns||c->keyframe_supported>1)return CAMERA_INVALID;
 for(uint32_t i=0;i<CAMERA_PRODUCER_CONTROLS;i++)
  if(c->controls[i].set_supported>1||c->controls[i].stop_supported>1)return CAMERA_INVALID;
 /* Copy before clearing: configuration may be stored in fresh context memory. */
 struct camera_producer_config copy=*c;
 memset(p,0,sizeof(*p));p->config=copy;p->initialized=1;
 for(uint32_t i=0;i<CAMERA_PRODUCER_CONTROLS;i++)p->controls[i].desired_result=CAMERA_NOT_READY;
 return CAMERA_OK;
}
int camera_producer_attach(struct camera_producer *p,enum camera_producer_consumer_kind kind,
                          uint64_t now,struct camera_producer_demand *d)
{
 if(!d||kind<CAMERA_PRODUCER_RTSP||kind>CAMERA_PRODUCER_RECORDER||clock_check(p,now))return CAMERA_INVALID;
 if(p->shutdown||p->state==CAMERA_PRODUCER_FAILED)return CAMERA_NOT_READY;
 uint32_t i;
 for(i=0;i<CAMERA_PRODUCER_CONSUMERS;i++)if(!p->consumers[i].attached)break;
 if(i==CAMERA_PRODUCER_CONSUMERS)return CAMERA_BUSY;
 if(p->next_id==UINT64_MAX||(p->state==CAMERA_PRODUCER_STOPPED&&p->generation==UINT64_MAX))return CAMERA_NOT_READY;
 if(p->state==CAMERA_PRODUCER_STOPPED){
  p->generation++;p->state=CAMERA_PRODUCER_STARTING;invalidate_applied(p);
 }
 p->consumers[i].id=++p->next_id;p->consumers[i].kind=kind;p->consumers[i].attached=1;
 p->demand_count++;d->owner=p;d->id=p->consumers[i].id;d->slot=i;return CAMERA_OK;
}
int camera_producer_detach(struct camera_producer *p,struct camera_producer_demand d,uint64_t now)
{
 if(clock_check(p,now))return CAMERA_INVALID;
 if(d.owner!=p||d.slot>=CAMERA_PRODUCER_CONSUMERS||!d.id||p->consumers[d.slot].id!=d.id)return CAMERA_STALE_LEASE;
 if(!p->consumers[d.slot].attached)return CAMERA_OK;
 p->consumers[d.slot].attached=0;p->demand_count--;
 if(!p->demand_count)drain(p,CAMERA_PRODUCER_REASON_CANCELLED,now);
 return CAMERA_OK;
}
int camera_producer_request_keyframe(struct camera_producer *p,uint64_t now,struct camera_producer_token *out)
{
 if(!out||clock_check(p,now))return CAMERA_INVALID;
 if(!p->config.keyframe_supported)return CAMERA_UNSUPPORTED;
 if(p->shutdown||(p->state!=CAMERA_PRODUCER_RUNNING&&p->state!=CAMERA_PRODUCER_STARTING))return CAMERA_NOT_READY;
 if(p->keyframe_pending){*out=p->keyframe.token;return CAMERA_OK;}
 if(p->active_present&&p->active.kind==CAMERA_PRODUCER_KEYFRAME&&!p->active_terminal){
  *out=p->active.token;return CAMERA_OK;
 }
 struct camera_producer_work w={0};
 int r=token(p,&w.token);if(r)return r;
 w.kind=CAMERA_PRODUCER_KEYFRAME;p->keyframe=w;p->keyframe_pending=1;*out=w.token;return CAMERA_OK;
}
int camera_producer_submit(struct camera_producer *p,const struct camera_producer_command *c,
                           uint64_t now,struct camera_producer_token *out)
{
 if(!c||!out||c->key>=CAMERA_PRODUCER_CONTROLS||
    (c->kind!=CAMERA_PRODUCER_SET&&c->kind!=CAMERA_PRODUCER_COMMAND_STOP)||clock_check(p,now))return CAMERA_INVALID;
 if((c->kind==CAMERA_PRODUCER_SET&&!p->config.controls[c->key].set_supported)||
    (c->kind==CAMERA_PRODUCER_COMMAND_STOP&&!p->config.controls[c->key].stop_supported))return CAMERA_UNSUPPORTED;
 if(p->shutdown||p->state!=CAMERA_PRODUCER_RUNNING)return CAMERA_NOT_READY;
 if(c->kind==CAMERA_PRODUCER_COMMAND_STOP?p->stop_pending:p->queue_count==CAMERA_PRODUCER_QUEUE)return CAMERA_BUSY;
 struct camera_producer_control_state *s=&p->controls[c->key];
 if(s->desired_generation==UINT64_MAX)return CAMERA_NOT_READY;
 struct camera_producer_work w={0};int r=token(p,&w.token);if(r)return r;
 w.kind=CAMERA_PRODUCER_CONTROL;w.command=*c;w.desired_generation=++s->desired_generation;
 s->desired=c->value;s->desired_operation=w.token;s->desired_result=CAMERA_PRODUCER_PENDING;
 s->desired_reason=CAMERA_PRODUCER_REASON_NONE;s->result_ns=now;
 if(c->kind==CAMERA_PRODUCER_COMMAND_STOP){
  for(uint32_t i=0;i<p->queue_count;){
   if(p->queue[i].command.key==c->key){
    struct camera_producer_work old=p->queue[i];remove_queue(p,i);
    notify(p,&old,CAMERA_PRODUCER_CANCELLED,CAMERA_PRODUCER_REASON_CANCELLED,now);
   }else i++;
  }
  if(p->active_present&&p->active.kind==CAMERA_PRODUCER_CONTROL&&
     p->active.command.key==c->key&&p->active.command.kind==CAMERA_PRODUCER_SET)
   cancel_active(p,CAMERA_PRODUCER_CANCELLED,CAMERA_PRODUCER_REASON_CANCELLED,now);
  p->stop=w;p->stop_pending=1;
 }else p->queue[p->queue_count++]=w;
 *out=w.token;return CAMERA_OK;
}
int camera_producer_cancel(struct camera_producer *p,struct camera_producer_token t,uint64_t now)
{
 if(clock_check(p,now))return CAMERA_INVALID;
 if(t.owner!=p||!t.id)return CAMERA_STALE_LEASE;
 if(p->active_present&&same(p->active.token,t)){
  if(p->active.kind!=CAMERA_PRODUCER_CONTROL&&p->active.kind!=CAMERA_PRODUCER_KEYFRAME)return CAMERA_INVALID;
  cancel_active(p,CAMERA_PRODUCER_CANCELLED,CAMERA_PRODUCER_REASON_CANCELLED,now);return CAMERA_OK;
 }
 for(uint32_t i=0;i<p->queue_count;i++)if(same(p->queue[i].token,t)){
  struct camera_producer_work w=p->queue[i];remove_queue(p,i);
  notify(p,&w,CAMERA_PRODUCER_CANCELLED,CAMERA_PRODUCER_REASON_CANCELLED,now);return CAMERA_OK;
 }
 if(p->stop_pending&&same(p->stop.token,t)){
  p->stop_pending=0;notify(p,&p->stop,CAMERA_PRODUCER_CANCELLED,CAMERA_PRODUCER_REASON_CANCELLED,now);return CAMERA_OK;
 }
 if(p->keyframe_pending&&same(p->keyframe.token,t)){
  p->keyframe_pending=0;notify(p,&p->keyframe,CAMERA_PRODUCER_CANCELLED,CAMERA_PRODUCER_REASON_CANCELLED,now);return CAMERA_OK;
 }
 return CAMERA_STALE_LEASE;
}
int camera_producer_tick(struct camera_producer *p,uint64_t now)
{
 if(clock_check(p,now))return CAMERA_INVALID;
 if(p->active_present&&!p->active_terminal&&now>=p->active.deadline_ns){
  if(p->active.kind==CAMERA_PRODUCER_START||p->active.kind==CAMERA_PRODUCER_STOP||p->active.kind==CAMERA_PRODUCER_RECOVER)
   failed(p,CAMERA_PRODUCER_TIMEOUT,CAMERA_PRODUCER_REASON_TIMEOUT,now);
  else cancel_active(p,CAMERA_PRODUCER_TIMEOUT,CAMERA_PRODUCER_REASON_TIMEOUT,now);
 }
 return CAMERA_OK;
}
int camera_producer_next(struct camera_producer *p,uint64_t now,
                         enum camera_producer_boundary boundary,struct camera_producer_work *out)
{
 if(!out||(boundary!=CAMERA_PRODUCER_NO_BOUNDARY&&boundary!=CAMERA_PRODUCER_PROVED_BOUNDARY))return CAMERA_INVALID;
 int r=camera_producer_tick(p,now);if(r)return r;
 if(p->active_present||p->state==CAMERA_PRODUCER_FAILED||p->state==CAMERA_PRODUCER_STOPPED)return CAMERA_NOT_READY;
 struct camera_producer_work w={0};
 if(p->state==CAMERA_PRODUCER_DRAINING||(p->state==CAMERA_PRODUCER_RECOVERING&&p->needs_stop))w.kind=CAMERA_PRODUCER_STOP;
 else if(p->state==CAMERA_PRODUCER_STARTING)w.kind=CAMERA_PRODUCER_START;
 else if(p->state==CAMERA_PRODUCER_RECOVERING)w.kind=CAMERA_PRODUCER_RECOVER;
 else if(p->stop_pending){if(boundary!=CAMERA_PRODUCER_PROVED_BOUNDARY)return CAMERA_NOT_READY;w=p->stop;}
 else if(p->queue_count){if(boundary!=CAMERA_PRODUCER_PROVED_BOUNDARY)return CAMERA_NOT_READY;w=p->queue[0];}
 else if(p->keyframe_pending)w=p->keyframe;
 else return CAMERA_NOT_READY;
 if(now>UINT64_MAX-p->config.operation_timeout_ns)return CAMERA_NOT_READY;
 if(!w.token.id){r=token(p,&w.token);if(r)return r;}
 w.deadline_ns=now+p->config.operation_timeout_ns;
 if(w.kind==CAMERA_PRODUCER_CONTROL){
  p->controls[w.command.key].applied_evidence=CAMERA_PRODUCER_APPLIED_UNKNOWN;
  if(w.command.kind==CAMERA_PRODUCER_COMMAND_STOP)p->stop_pending=0;
  else remove_queue(p,0);
 }else if(w.kind==CAMERA_PRODUCER_KEYFRAME)p->keyframe_pending=0;
 else if(w.kind==CAMERA_PRODUCER_START||w.kind==CAMERA_PRODUCER_RECOVER)p->needs_stop=1;
 p->active=w;p->active_present=1;p->active_terminal=0;p->active_result=CAMERA_PRODUCER_PENDING;
 p->active_reason=CAMERA_PRODUCER_REASON_NONE;*out=w;return CAMERA_OK;
}
int camera_producer_active(const struct camera_producer *p,struct camera_producer_work *w,int *result)
{
 if(!valid(p)||!w||!result)return CAMERA_INVALID;
 if(!p->active_present)return CAMERA_NOT_READY;
 *w=p->active;*result=p->active_result;return CAMERA_OK;
}
int camera_producer_complete(struct camera_producer *p,struct camera_producer_token t,int result,uint64_t now)
{
 if(!driver_result(result)||clock_check(p,now))return CAMERA_INVALID;
 if(!p->active_present||!same(p->active.token,t))return CAMERA_STALE_LEASE;
 camera_producer_tick(p,now);
 struct camera_producer_work w=p->active;
 uint32_t terminal=p->active_terminal;
 p->active_present=0;p->active_terminal=0;
 /* A logically cancelled operation still needed this retirement acknowledgement. */
 if(terminal)return CAMERA_OK;
 if(w.kind==CAMERA_PRODUCER_START||w.kind==CAMERA_PRODUCER_RECOVER){
  if(result==CAMERA_OK)p->state=CAMERA_PRODUCER_RUNNING;
  else failed(p,result,CAMERA_PRODUCER_REASON_BACKEND,now);
 }else if(w.kind==CAMERA_PRODUCER_STOP){
  if(result!=CAMERA_OK)failed(p,result,CAMERA_PRODUCER_REASON_BACKEND,now);
  else{
   p->needs_stop=0;invalidate_applied(p);
   if(p->state!=CAMERA_PRODUCER_RECOVERING){
    p->state=CAMERA_PRODUCER_STOPPED;
    if(p->demand_count&&!p->shutdown){
     if(p->generation==UINT64_MAX)failed(p,CAMERA_NOT_READY,CAMERA_PRODUCER_REASON_BACKEND,now);
     else{p->generation++;p->state=CAMERA_PRODUCER_STARTING;}
    }
   }
  }
 }else if(w.kind==CAMERA_PRODUCER_CONTROL&&result==CAMERA_OK){
  struct camera_producer_control_state *s=&p->controls[w.command.key];
  s->applied=w.command.value;s->applied_generation=w.desired_generation;
  s->applied_producer_generation=w.token.generation;s->applied_ns=now;
  s->applied_evidence=CAMERA_PRODUCER_APPLIED_ACKNOWLEDGED;
 }
 notify(p,&w,result,result==CAMERA_OK?CAMERA_PRODUCER_REASON_NONE:CAMERA_PRODUCER_REASON_BACKEND,now);
 return CAMERA_OK;
}
int camera_producer_source_failed(struct camera_producer *p,int result,uint64_t now)
{
 if(!driver_result(result)||result==CAMERA_OK||clock_check(p,now))return CAMERA_INVALID;
 if(p->state==CAMERA_PRODUCER_STOPPED)return CAMERA_NOT_READY;
 failed(p,result,CAMERA_PRODUCER_REASON_SOURCE,now);
 struct camera_producer_work w={0};w.token.owner=p;w.token.generation=p->generation;
 notify(p,&w,result,CAMERA_PRODUCER_REASON_SOURCE,now);return CAMERA_OK;
}
int camera_producer_recover(struct camera_producer *p,uint64_t now)
{
 if(clock_check(p,now))return CAMERA_INVALID;
 if(p->shutdown||!p->demand_count||p->state!=CAMERA_PRODUCER_FAILED)return CAMERA_NOT_READY;
 if(p->active_present)return CAMERA_BUSY;
 if(p->generation==UINT64_MAX)return CAMERA_NOT_READY;
 p->generation++;p->state=CAMERA_PRODUCER_RECOVERING;invalidate_applied(p);return CAMERA_OK;
}
int camera_producer_shutdown(struct camera_producer *p,uint64_t now)
{
 if(clock_check(p,now))return CAMERA_INVALID;
 p->shutdown=1;
 for(uint32_t i=0;i<CAMERA_PRODUCER_CONSUMERS;i++)p->consumers[i].attached=0;
 p->demand_count=0;
 /* Repeated shutdown must not cancel its own retirement STOP. A failed STOP
  * may be explicitly retried by another shutdown call, never an automatic loop.
  */
 if(p->state!=CAMERA_PRODUCER_DRAINING)drain(p,CAMERA_PRODUCER_REASON_SHUTDOWN,now);
 return CAMERA_OK;
}
int camera_producer_quiescent(const struct camera_producer *p)
{
 return valid(p)&&p->shutdown&&p->state==CAMERA_PRODUCER_STOPPED&&!p->needs_stop&&
        !p->active_present&&!p->queue_count&&!p->stop_pending&&!p->keyframe_pending&&!p->demand_count;
}
int camera_producer_control_snapshot(const struct camera_producer *p,uint32_t key,
                                     struct camera_producer_control_state *s)
{
 if(!valid(p)||!s||key>=CAMERA_PRODUCER_CONTROLS)return CAMERA_INVALID;
 if(!p->config.controls[key].set_supported&&!p->config.controls[key].stop_supported)return CAMERA_UNSUPPORTED;
 *s=p->controls[key];return CAMERA_OK;
}
