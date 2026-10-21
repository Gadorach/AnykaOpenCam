/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/network_transition.h>
#include <string.h>

#define NETWORK_MAGIC 0x4e545231U
#define NO_SLOT CAMERA_NETWORK_RESULTS
#define BASE_FACTS (CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS)
#define ALL_FACTS (BASE_FACTS|CAMERA_NETWORK_LOCAL_REACHABLE|CAMERA_NETWORK_ROUTE)
static int ready(const struct camera_network_transition *s)
{return s&&s->initialized==NETWORK_MAGIC;}
static int token_equal(struct camera_network_token a,struct camera_network_token b)
{return a.namespace_id==b.namespace_id&&a.id==b.id;}
static int find(const struct camera_network_transition *s,struct camera_network_token t)
{
 if(!ready(s)||!t.id||t.namespace_id!=s->config.namespace_id)return -1;
 for(unsigned i=0;i<CAMERA_NETWORK_RESULTS;i++)
  if(s->used[i]&&token_equal(s->results[i].request.token,t))return (int)i;
 return -1;
}
static int terminal(uint32_t state)
{return state>=CAMERA_NETWORK_CONFIRMED&&state<=CAMERA_NETWORK_CANCELLED;}
static void finish(struct camera_network_transition *s,struct camera_network_report *r,
 uint32_t state)
{
 r->state=state;r->in_flight=0;r->committed_config_id=s->config.committed_config_id;
 r->committed_revision=s->config.committed_revision;r->old_link_known=s->link_known;
 s->active_slot=NO_SLOT;
}
static void abort_operation(struct camera_network_transition *s,
 struct camera_network_report *r,uint32_t reason)
{
 if(!r->reason)r->reason=reason;
 if(r->state==CAMERA_NETWORK_PREPARED){
  finish(s,r,reason==CAMERA_NETWORK_REASON_CANCEL?
   CAMERA_NETWORK_CANCELLED:CAMERA_NETWORK_FAILED);
 }else if(s->live_present){
  /* Rolling back is itself live work, never reissued on timeout/cancel. */
  if(s->live.effect!=CAMERA_NETWORK_ROLLBACK)r->state=CAMERA_NETWORK_CANCEL_WAIT;
  else if(reason==CAMERA_NETWORK_REASON_TIMEOUT||reason==CAMERA_NETWORK_REASON_CLOCK)
   r->rollback_reason=reason;
 }else if(r->state!=CAMERA_NETWORK_ROLLBACK_PENDING){
  r->state=CAMERA_NETWORK_ROLLBACK_PENDING;r->link_facts=0;
 }
}
int camera_network_tick(struct camera_network_transition *s,uint64_t now)
{
 struct camera_network_report *r;
 if(!ready(s))return CAMERA_NETWORK_INVALID;
 if(now<s->last_ns){
  s->clock_fault=1;
  if(s->active_slot!=NO_SLOT){
   r=&s->results[s->active_slot];
   abort_operation(s,r,CAMERA_NETWORK_REASON_CLOCK);
  }
  return CAMERA_NETWORK_CLOCK;
 }
 s->last_ns=now;
 if(s->active_slot==NO_SLOT)return CAMERA_NETWORK_OK;
 r=&s->results[s->active_slot];
 if(r->state!=CAMERA_NETWORK_ROLLBACK_PENDING&&now>=r->deadline_ns)
  abort_operation(s,r,CAMERA_NETWORK_REASON_TIMEOUT);
 return CAMERA_NETWORK_OK;
}
int camera_network_init(struct camera_network_transition *s,
 const struct camera_network_config *c,uint64_t now)
{
 if(!s||!c||!c->namespace_id||!c->committed_config_id||
    !c->transition_timeout_ns||!c->rollback_timeout_ns)return CAMERA_NETWORK_INVALID;
 memset(s,0,sizeof(*s));s->config=*c;s->last_ns=now;
 s->operation_high_water=c->operation_high_water;s->radio_high_water=c->radio_high_water;
 s->config_high_water=c->committed_config_id;
 s->active_slot=NO_SLOT;s->link_known=1;s->initialized=NETWORK_MAGIC;
 /* Caller asserts a reconciled initial known-good config/radio; init is NOT
  * independent observation or permission to initialize over live state. */
 return CAMERA_NETWORK_OK;
}
static int same_request(const struct camera_network_request *a,
 const struct camera_network_request *b)
{
 return token_equal(a->token,b->token)&&a->expected_revision==b->expected_revision&&
  a->config_id==b->config_id&&a->kind==b->kind&&a->required_facts==b->required_facts;
}
int camera_network_prepare(struct camera_network_transition *s,
 const struct camera_network_request *q,uint64_t now)
{
 int i,r;unsigned slot=NO_SLOT;struct camera_network_report *v;
 if(!ready(s)||!q||!q->token.id||q->token.namespace_id!=s->config.namespace_id||
    !q->config_id||(q->kind!=CAMERA_NETWORK_SWITCH&&q->kind!=CAMERA_NETWORK_SCAN)||
    (q->required_facts&BASE_FACTS)!=BASE_FACTS||(q->required_facts&~ALL_FACTS))
  return CAMERA_NETWORK_INVALID;
 i=find(s,q->token);
 if(i>=0){
  if(!same_request(q,&s->results[i].request))return CAMERA_NETWORK_CONFLICT;
  /* Terminal replay is read-only: its timestamp cannot alter newer work. */
  return terminal(s->results[i].state)?CAMERA_NETWORK_OK:camera_network_tick(s,now);
 }
 r=camera_network_tick(s,now);if(r)return r;
 if(s->clock_fault)return CAMERA_NETWORK_CLOCK;
 if(s->active_slot!=NO_SLOT||!s->link_known)return CAMERA_NETWORK_BUSY;
 if(s->operation_high_water==UINT64_MAX)return CAMERA_NETWORK_EXHAUSTED;
 if(q->token.id<=s->operation_high_water||q->expected_revision!=s->config.committed_revision)
  return CAMERA_NETWORK_CONFLICT;
 if((q->kind==CAMERA_NETWORK_SWITCH&&q->config_id<=s->config_high_water)||
    (q->kind==CAMERA_NETWORK_SCAN&&q->config_id!=s->config.committed_config_id))
  return CAMERA_NETWORK_CONFLICT;
 if(s->radio_high_water>UINT64_MAX-2||
    (q->kind==CAMERA_NETWORK_SWITCH&&s->config.committed_revision==UINT64_MAX)||
    now>UINT64_MAX-s->config.transition_timeout_ns)return CAMERA_NETWORK_EXHAUSTED;
 for(unsigned j=0;j<CAMERA_NETWORK_RESULTS;j++)if(!s->used[j]){slot=j;break;}
 if(slot==NO_SLOT)return CAMERA_NETWORK_CAPACITY;
 v=&s->results[slot];memset(v,0,sizeof(*v));v->request=*q;
 v->old_config_id=s->config.committed_config_id;v->old_revision=s->config.committed_revision;
 v->committed_config_id=v->old_config_id;v->committed_revision=v->old_revision;
 v->apply_generation=s->radio_high_water+1;v->rollback_generation=s->radio_high_water+2;
 v->deadline_ns=now+s->config.transition_timeout_ns;
 v->state=CAMERA_NETWORK_PREPARED;v->old_link_known=1;
 s->radio_high_water+=2;s->operation_high_water=q->token.id;
 if(q->kind==CAMERA_NETWORK_SWITCH)s->config_high_water=q->config_id;
 s->used[slot]=1;s->active_slot=slot;return CAMERA_NETWORK_OK;
}
static int active(struct camera_network_transition *s,struct camera_network_token t,
 uint64_t now,struct camera_network_report **out)
{
 int i,r;if(!ready(s))return CAMERA_NETWORK_INVALID;
 i=find(s,t);if(i<0)return CAMERA_NETWORK_STALE;
 if(s->active_slot!=(unsigned)i)return CAMERA_NETWORK_ORDER;
 r=camera_network_tick(s,now);if(r)return r;
 *out=&s->results[i];
 return s->active_slot==(unsigned)i?CAMERA_NETWORK_OK:CAMERA_NETWORK_ORDER;
}
int camera_network_receipt_ack(struct camera_network_transition *s,
 struct camera_network_token t,uint64_t now)
{
 struct camera_network_report *r;int result=active(s,t,now,&r);if(result)return result;
 if(r->state!=CAMERA_NETWORK_PREPARED)return CAMERA_NETWORK_ORDER;
 r->receipt_acknowledged=1;return CAMERA_NETWORK_OK;
}
int camera_network_next(struct camera_network_transition *s,uint64_t now,
 struct camera_network_work *out)
{
 struct camera_network_report *r;struct camera_network_work w={0};int result;
 if(!out)return CAMERA_NETWORK_INVALID;
 memset(out,0,sizeof(*out));result=camera_network_tick(s,now);if(result)return result;
 if(s->live_present||s->active_slot==NO_SLOT)return CAMERA_NETWORK_NOT_READY;
 r=&s->results[s->active_slot];w.token=r->request.token;
 if(r->state==CAMERA_NETWORK_PREPARED&&r->receipt_acknowledged){
  w.effect=r->request.kind==CAMERA_NETWORK_SWITCH?CAMERA_NETWORK_APPLY:
   CAMERA_NETWORK_DISRUPTIVE_SCAN;
  w.config_id=r->request.config_id;w.radio_generation=r->apply_generation;
  r->state=CAMERA_NETWORK_APPLYING;
 }else if(r->state==CAMERA_NETWORK_ROLLBACK_PENDING){
  if(now>UINT64_MAX-s->config.rollback_timeout_ns){
   r->rollback_reason=CAMERA_NETWORK_REASON_TIMEOUT;
   finish(s,r,CAMERA_NETWORK_FAILED);return CAMERA_NETWORK_EXHAUSTED;
  }
  w.effect=CAMERA_NETWORK_ROLLBACK;w.config_id=r->old_config_id;
  w.radio_generation=r->rollback_generation;
  r->link_facts=0;r->link_observation_id=0;
  r->deadline_ns=now+s->config.rollback_timeout_ns;r->state=CAMERA_NETWORK_ROLLING_BACK;
 }else return CAMERA_NETWORK_NOT_READY;
 w.deadline_ns=r->deadline_ns;s->live=w;s->live_present=1;
 s->link_known=0;r->old_link_known=0;r->in_flight=1;*out=w;return CAMERA_NETWORK_OK;
}
static int same_work(const struct camera_network_work *a,const struct camera_network_work *b)
{
 return token_equal(a->token,b->token)&&a->effect==b->effect&&a->config_id==b->config_id&&
  a->radio_generation==b->radio_generation&&a->deadline_ns==b->deadline_ns;
}
static int proof(const struct camera_network_report *r,const struct camera_network_link *p,
 uint64_t config,uint64_t generation)
{
 return p&&p->observation_id&&token_equal(p->token,r->request.token)&&p->config_id==config&&
  p->radio_generation==generation&&!(p->facts&~ALL_FACTS)&&
  (p->facts&r->request.required_facts)==r->request.required_facts;
}
static int current_observation(const struct camera_network_report *r,
 const struct camera_network_link *p)
{
 return p&&p->observation_id&&p->observation_id>=r->link_observation_id&&
  (p->observation_id!=r->link_observation_id||p->facts==r->link_facts);
}
int camera_network_complete(struct camera_network_transition *s,
 const struct camera_network_work *w,uint32_t success,
 const struct camera_network_link *p,uint64_t now)
{
 struct camera_network_report *r;int result;
 if(!ready(s)||!w||success>1)return CAMERA_NETWORK_INVALID;
 if(!s->live_present||!same_work(&s->live,w))return CAMERA_NETWORK_STALE;
 /* Bad proof is not retirement acknowledgment; keep work until caller sends
  * valid success or explicit failure after real retirement. */
 r=&s->results[s->active_slot];
 if(w->effect==CAMERA_NETWORK_ROLLBACK&&success){
  if(!p||!token_equal(p->token,r->request.token)||p->config_id!=r->old_config_id||
     p->radio_generation!=r->rollback_generation||!current_observation(r,p))
   return CAMERA_NETWORK_STALE;
  if(!proof(r,p,r->old_config_id,r->rollback_generation)){
   /* A newer exact-instance loss/unknown report is still an observation even
    * when wrongly presented as success proof. Retain it, but never retire. */
   result=camera_network_link_proof(s,p,now);
   return result==CAMERA_NETWORK_CLOCK?result:CAMERA_NETWORK_STALE;
  }
 }
 if(w->effect!=CAMERA_NETWORK_ROLLBACK&&p)return CAMERA_NETWORK_INVALID;
 result=camera_network_tick(s,now);if(result)return result;
 s->live_present=0;memset(&s->live,0,sizeof(s->live));r->in_flight=0;
 if(w->effect==CAMERA_NETWORK_ROLLBACK){
  r->rollback_completed=1;
  if(success&&now<r->deadline_ns&&!s->clock_fault){
   r->rollback_succeeded=1;
    s->link_known=1;r->link_facts=p->facts;r->link_observation_id=p->observation_id;
    finish(s,r,CAMERA_NETWORK_RESTORED);
  }else{
   r->rollback_reason=s->clock_fault?CAMERA_NETWORK_REASON_CLOCK:
    now>=r->deadline_ns?CAMERA_NETWORK_REASON_TIMEOUT:CAMERA_NETWORK_REASON_ROLLBACK;
   if(!r->reason)r->reason=CAMERA_NETWORK_REASON_ROLLBACK;
   finish(s,r,CAMERA_NETWORK_FAILED);
  }
 }else if(w->effect==CAMERA_NETWORK_DISRUPTIVE_SCAN){
  r->scan_completed=1;r->scan_succeeded=success&&
   r->state==CAMERA_NETWORK_APPLYING&&!r->reason;
  if(!r->reason)r->reason=success?CAMERA_NETWORK_REASON_SCAN:CAMERA_NETWORK_REASON_APPLY;
  r->state=CAMERA_NETWORK_ROLLBACK_PENDING;
 }else if(!success||r->reason){
  if(!r->reason)r->reason=CAMERA_NETWORK_REASON_APPLY;
  r->state=CAMERA_NETWORK_ROLLBACK_PENDING;
 }else r->state=CAMERA_NETWORK_WAIT_LINK;
 return CAMERA_NETWORK_OK;
}
int camera_network_link_proof(struct camera_network_transition *s,
 const struct camera_network_link *p,uint64_t now)
{
 struct camera_network_report *r;int result,slot,rollback;
 uint64_t config,generation;
 if(!ready(s)||!p)return CAMERA_NETWORK_INVALID;
 slot=find(s,p->token);if(slot<0)return CAMERA_NETWORK_STALE;
 if(s->active_slot!=(unsigned)slot)return CAMERA_NETWORK_ORDER;
 r=&s->results[slot];
 rollback=r->state==CAMERA_NETWORK_ROLLING_BACK&&s->live_present&&
  s->live.effect==CAMERA_NETWORK_ROLLBACK;
 if(!rollback&&r->state!=CAMERA_NETWORK_WAIT_LINK&&r->state!=CAMERA_NETWORK_CONFIRM_READY)
  return CAMERA_NETWORK_ORDER;
 config=rollback?r->old_config_id:r->request.config_id;
 generation=rollback?r->rollback_generation:r->apply_generation;
 if(!token_equal(p->token,r->request.token)||p->config_id!=config||
    p->radio_generation!=generation||!current_observation(r,p))return CAMERA_NETWORK_STALE;
 /* Denied callbacks cannot advance/regress the clock of the current instance. */
 result=camera_network_tick(s,now);if(result)return result;
 if(!rollback&&r->state!=CAMERA_NETWORK_WAIT_LINK&&r->state!=CAMERA_NETWORK_CONFIRM_READY)
  return CAMERA_NETWORK_ORDER;
 /* Retain raw scalar facts, including unknown bits, so equal-ID conflicting
  * completion proof cannot replace an already-observed loss/invalid report. */
 r->link_observation_id=p->observation_id;r->link_facts=p->facts;
 if(p->facts&~ALL_FACTS){
  if(!rollback)r->state=CAMERA_NETWORK_WAIT_LINK;
  return CAMERA_NETWORK_INVALID;
 }
 if(!proof(r,p,config,generation)){
  if(!rollback)r->state=CAMERA_NETWORK_WAIT_LINK;
  return CAMERA_NETWORK_NOT_READY;
 }
 if(rollback){
  /* Observation is never retirement/restoration, including after timeout. */
  return now>=r->deadline_ns||s->clock_fault?CAMERA_NETWORK_NOT_READY:CAMERA_NETWORK_OK;
 }
 r->state=CAMERA_NETWORK_CONFIRM_READY;return CAMERA_NETWORK_OK;
}
int camera_network_confirm(struct camera_network_transition *s,
 struct camera_network_token t,uint64_t observed_revision,uint64_t now)
{
 struct camera_network_report *r;int result=active(s,t,now,&r);if(result)return result;
 if(r->state!=CAMERA_NETWORK_CONFIRM_READY||r->request.kind!=CAMERA_NETWORK_SWITCH)
  return CAMERA_NETWORK_ORDER;
 if(observed_revision!=r->old_revision||s->config.committed_revision!=r->old_revision){
  abort_operation(s,r,CAMERA_NETWORK_REASON_REVISION);return CAMERA_NETWORK_CONFLICT;
 }
 s->config.committed_config_id=r->request.config_id;s->config.committed_revision++;
 s->link_known=1;finish(s,r,CAMERA_NETWORK_CONFIRMED);
 /* old_link_known means OLD profile only; confirmed link is the NEW profile. */
 r->old_link_known=0;return CAMERA_NETWORK_OK;
}
int camera_network_cancel(struct camera_network_transition *s,
 struct camera_network_token t,uint64_t now)
{
 struct camera_network_report *r;int result=active(s,t,now,&r);if(result)return result;
 abort_operation(s,r,CAMERA_NETWORK_REASON_CANCEL);return CAMERA_NETWORK_OK;
}
int camera_network_query(const struct camera_network_transition *s,
 struct camera_network_token t,struct camera_network_report *out)
{
 int i;if(!out)return CAMERA_NETWORK_INVALID;memset(out,0,sizeof(*out));
 if(!ready(s))return CAMERA_NETWORK_INVALID;
 i=find(s,t);if(i<0)return CAMERA_NETWORK_STALE;*out=s->results[i];return CAMERA_NETWORK_OK;
}
int camera_network_forget(struct camera_network_transition *s,struct camera_network_token t)
{
 int i;if(!ready(s))return CAMERA_NETWORK_INVALID;i=find(s,t);if(i<0)return CAMERA_NETWORK_STALE;
 if(!terminal(s->results[i].state)||s->results[i].in_flight)return CAMERA_NETWORK_BUSY;
 memset(&s->results[i],0,sizeof(s->results[i]));s->used[i]=0;return CAMERA_NETWORK_OK;
}
