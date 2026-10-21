/* SPDX-License-Identifier: GPL-2.0-only */
#include "ptz_owner.h"
#include <string.h>

/* Licensed local diagnostic ptz.c:19,66-82 corroborates these logical phase
 * bits. This candidate does NOT inherit its GPIO0/off-level assumption. */
static const uint32_t phases[4]={3,6,12,9};
static int zero(const void *v,size_t n)
{
 const unsigned char *p=v;size_t i;
 for(i=0;i<n;i++)if(p[i])return 0;
 return 1;
}
int ptz_ticket_equal(const struct ptz_ticket *a,const struct ptz_ticket *b)
{
 return a&&b&&a->space==b->space&&a->generation==b->generation&&
  a->sequence==b->sequence&&!memcmp(a->head,b->head,CAMERA_ID_BYTES);
}
static int usable(const struct ptz_owner *o)
{
 return o&&o->self==o&&o->registry&&o->registry->self==o->registry&&
  o->head_slot<PTZ_HEAD_MAX&&o->registry->heads[o->head_slot].owner==o&&
  o->auth&&o->auth->self==o->auth&&!o->disposed&&!o->callback_active;
}
static int disjoint(const struct ptz_owner *o,const void *v,size_t n)
{
 return v&&!ptz_overlap(o,sizeof(*o),v,n)&&
  !ptz_overlap(o->registry,sizeof(*o->registry),v,n)&&
  !ptz_overlap(o->auth,sizeof(*o->auth),v,n)&&
  !ptz_overlap(o->auth->policy,sizeof(*o->auth->policy),v,n);
}
static int config_valid(const struct ptz_config *c)
{
 return c&&c->phase_limit&&c->phase_limit<=PTZ_PHASE_MAX&&
  (c->pan_sign==1||c->pan_sign==-1)&&(c->tilt_sign==1||c->tilt_sign==-1);
}
static int safe(const struct ptz_owner *o)
{
 const struct ptz_provider *p=&o->provider;
 return p->readiness==PTZ_REQUIRED_READY&&p->callback_bound_ns&&
  p->callback_bound_ns<=PTZ_CALLBACK_MAX_NS&&p->stop_bound_ns&&p->stop_bound_ns<=PTZ_STOP_MAX_NS;
}
static void state(struct ptz_owner *o,uint32_t s)
{
 o->view.state=s;o->view.retained=o->owned;o->view.queued=o->has_replacement;
}
static void dispose_idle(struct ptz_owner *o)
{
 if(o->disposing&&!o->owned&&!o->active){
  o->registry->heads[o->head_slot].owner=NULL;o->disposed=1;state(o,PTZ_DISPOSED);
 }
}
static void idle(struct ptz_owner *o)
{
 o->active=0;o->phase_waiting=0;o->stop_sent=0;o->stop_terminal=0;
 o->view.desired_kind=PTZ_NEUTRAL;
 memset(&o->view.pending,0,sizeof(o->view.pending));
 memset(&o->view.active_work,0,sizeof(o->view.active_work));
 state(o,PTZ_IDLE);dispose_idle(o);
}
static int observe(struct ptz_owner *o,uint64_t now)
{
 if(now<o->view.last_ns){o->clock_failed=1;o->view.fault=1;return PTZ_FAULT;}
 o->view.last_ns=now;return o->clock_failed?PTZ_FAULT:PTZ_OK;
}
static int submit(struct ptz_owner *o,const struct ptz_request *q)
{
 uint64_t start,end;int result;struct ptz_request effective=*q;
 o->callback_active=1;start=o->provider.clock_ns(o->provider.context);
 o->callback_active=0;
 if(start<o->view.last_ns){o->clock_failed=1;o->view.fault=1;}
 else o->view.last_ns=start;
 /* Recheck at the last measured boundary before every energized effect. */
 if(q->kind==PTZ_PHASE&&(o->clock_failed||start>=o->job.expires_ns||UINT64_MAX-start<PTZ_PHASE_NS||
    ptz_auth_check(o->auth,&o->job.move.principal,o->job.move.sensor,start)!=CAMERA_ACCESS_OK))return PTZ_NO_EFFECT;
 if(q->kind==PTZ_PHASE){
  effective.issued_ns=start;effective.neutral_by_ns=start+PTZ_PHASE_NS;
  if(effective.neutral_by_ns>o->job.expires_ns)effective.neutral_by_ns=o->job.expires_ns;
  o->phase_sent_ns=start;o->phase_ack_deadline_ns=effective.neutral_by_ns;
 }
 o->callback_active=1;result=o->provider.submit(o->provider.context,&effective);
 end=o->provider.clock_ns(o->provider.context);o->callback_active=0;
 if(end<start||end<o->view.last_ns){o->clock_failed=1;o->view.fault=1;}
 else o->view.last_ns=end;
 if(end<start||end-start>o->provider.callback_bound_ns||o->clock_failed){
  o->view.fault=1;return PTZ_UNCERTAIN;
 }
 return result==PTZ_ACCEPTED||result==PTZ_NO_EFFECT?result:PTZ_UNCERTAIN;
}
static void stop_owned(struct ptz_owner *o,int fault,int discard)
{
 struct ptz_request q;int result;
 if(fault)o->view.fault=1;
 if(discard){o->has_replacement=0;o->view.queued=0;}
 o->view.desired_kind=PTZ_NEUTRAL;
 if(!o->owned){idle(o);return;}
 if(o->stop_sent){
  state(o,o->view.fault?PTZ_QUARANTINED:PTZ_STOP_PENDING);return;
 }
 /* Reserved before any phase: even sequence exhaustion cannot block cleanup.
  * Exact owned-work fence only; no caller GPIO/phase-neutral mask is supplied. */
 memset(&q,0,sizeof(q));q.ticket=o->job.stop;q.work=o->job.work;q.kind=PTZ_NEUTRAL;
 q.issued_ns=o->view.last_ns;
 q.neutral_by_ns=UINT64_MAX-q.issued_ns<o->provider.stop_bound_ns?
  UINT64_MAX:q.issued_ns+o->provider.stop_bound_ns;
 o->stop_deadline_ns=q.neutral_by_ns;o->view.stop_requested_ns=q.issued_ns;
 o->stop_sent=1;o->phase_waiting=0;o->view.pending=q.ticket;
 o->view.electrical_valid=0;state(o,PTZ_STOP_PENDING);
 result=submit(o,&q);
 if(result!=PTZ_ACCEPTED){
  o->view.fault=1;o->has_replacement=0;
  /* Definitive rejection is terminal, UNKNOWN may receive late exact ACK. */
  if(result==PTZ_NO_EFFECT)o->stop_terminal=1;
 }
 state(o,o->view.fault?PTZ_QUARANTINED:PTZ_STOP_PENDING);
}
/* Only trusted internal/provider input faults may initiate safety cleanup.
 * Client schema/alias failures below NEVER use this path. */
static int internal_input_fault(struct ptz_owner *o)
{
 stop_owned(o,1,1);return PTZ_INVALID;
}
int ptz_registry_init(struct ptz_registry *r,uint64_t space)
{
 if(!r||!space||!zero(r,sizeof(*r)))return PTZ_INVALID;
 r->self=r;r->space=space;return PTZ_OK;
}
int ptz_owner_init(struct ptz_owner *o,struct ptz_registry *r,struct ptz_auth *a,
 const struct ptz_provider *p,const struct ptz_config *c,uint64_t generation,uint64_t now)
{
 uint32_t i,free_slot=PTZ_HEAD_MAX;struct ptz_owner candidate;
 if(!o||!r||r->self!=r||!a||a->self!=a||!p||!c||!generation||
    !ptz_id_valid(a->head)||!zero(o,sizeof(*o)))return PTZ_INVALID;
 if(ptz_overlap(o,sizeof(*o),r,sizeof(*r))||ptz_overlap(o,sizeof(*o),a,sizeof(*a))||
    ptz_overlap(o,sizeof(*o),a->policy,sizeof(*a->policy))||ptz_overlap(o,sizeof(*o),p,sizeof(*p))||
    ptz_overlap(o,sizeof(*o),c,sizeof(*c))||ptz_overlap(r,sizeof(*r),a,sizeof(*a))||
    ptz_overlap(r,sizeof(*r),a->policy,sizeof(*a->policy))||ptz_overlap(r,sizeof(*r),p,sizeof(*p))||
    ptz_overlap(r,sizeof(*r),c,sizeof(*c))||!config_valid(c)||p->abi!=PTZ_OWNER_ABI||
    p->struct_bytes!=sizeof(*p)||!p->clock_ns||!p->submit||!p->context||
    ptz_overlap(o,sizeof(*o),p->context,1)||ptz_overlap(r,sizeof(*r),p->context,1)||
    ptz_overlap(a,sizeof(*a),p->context,1)||ptz_overlap(a->policy,sizeof(*a->policy),p->context,1))return PTZ_INVALID;
 for(i=0;i<PTZ_HEAD_MAX;i++){
  if(!r->heads[i].used){if(free_slot==PTZ_HEAD_MAX)free_slot=i;continue;}
  if(!memcmp(r->heads[i].head,a->head,CAMERA_ID_BYTES))break;
 }
 if(i<PTZ_HEAD_MAX){
  if(r->heads[i].owner)return PTZ_BUSY;
  if(generation<=r->heads[i].generation)return PTZ_STALE;
 }else{
  if(free_slot==PTZ_HEAD_MAX)return PTZ_EXHAUSTED;
  i=free_slot;
 }
 memset(&candidate,0,sizeof(candidate));candidate.self=o;candidate.registry=r;candidate.auth=a;
 candidate.provider=*p;candidate.config=*c;candidate.head_slot=i;candidate.view.generation=generation;
 candidate.view.last_ns=now;candidate.view.position_reason=PTZ_UNCALIBRATED;
 candidate.view.desired_kind=PTZ_NEUTRAL;
 *o=candidate;
 if(!r->heads[i].used){r->heads[i].used=1;r->heads[i].next_sequence=1;memcpy(r->heads[i].head,a->head,CAMERA_ID_BYTES);}
 r->heads[i].generation=generation;r->heads[i].owner=o;
 return PTZ_OK;
}
static int reserve(struct ptz_owner *o,const struct ptz_move *m,uint64_t now,struct ptz_job *j)
{
 struct ptz_head_record *h=&o->registry->heads[o->head_slot];uint32_t i;uint64_t n;
 uint64_t needed=(uint64_t)m->phases+2;
 if(!h->next_sequence||h->next_sequence>UINT64_MAX-needed||UINT64_MAX-now<m->lease_ns)return PTZ_EXHAUSTED;
 memset(j,0,sizeof(*j));j->move=*m;j->expires_ns=now+m->lease_ns;
 j->phase_index=o->axis_phase[m->axis];n=h->next_sequence;
 j->work.space=o->registry->space;j->work.generation=o->view.generation;j->work.sequence=n++;
 memcpy(j->work.head,o->auth->head,CAMERA_ID_BYTES);
 for(i=0;i<m->phases;i++){j->phase[i]=j->work;j->phase[i].sequence=n++;}
 j->stop=j->work;j->stop.sequence=n++;h->next_sequence=n;return PTZ_OK;
}
int ptz_owner_move(struct ptz_owner *o,const struct ptz_move *m,uint64_t now)
{
 struct ptz_job j;int ret;
 if(!usable(o))return PTZ_INVALID;
 if(!disjoint(o,m,sizeof(*m)))return PTZ_INVALID;
 if(!ptz_id_valid(m->sensor)||m->axis>1||!m->phases||m->phases>o->config.phase_limit||
    (m->direction!=1&&m->direction!=-1)||!m->lease_ns||m->lease_ns>PTZ_LEASE_MAX_NS||
    m->lease_ns<(uint64_t)m->phases*PTZ_PHASE_NS)return PTZ_INVALID;
 /* A rejected client request cannot stop/fault another principal, discard
  * queued intent, observe the owner's clock or reserve a provider ticket.
  * Even authorized malformed commands are inert; quiesce is internal-only. */
 if(ptz_auth_check(o->auth,&m->principal,m->sensor,now)!=CAMERA_ACCESS_OK)return PTZ_DENIED;
 if(o->view.fault||o->disposing)return PTZ_FAULT;
 if(!safe(o))return PTZ_UNSAFE;
 if(o->active&&(!ptz_same_principal(&o->job.move.principal,&m->principal)||o->has_replacement))return PTZ_BUSY;
 if(observe(o,now)){stop_owned(o,1,1);return PTZ_FAULT;}
 ret=reserve(o,m,now,&j);if(ret)return ret;
 if(o->active){
  o->replacement=j;o->has_replacement=1;stop_owned(o,0,0);
  /* An admitted but not submitted command owns no outputs: no fake STOP ACK. */
  if(!o->owned&&!o->disposing&&!o->view.fault){
   o->job=o->replacement;o->has_replacement=0;o->active=1;
   o->view.active_work=o->job.work;o->view.desired_kind=PTZ_PHASE;state(o,PTZ_QUEUED);
  }
 }else{
  o->job=j;o->active=1;o->view.active_work=j.work;o->view.desired_kind=PTZ_PHASE;state(o,PTZ_QUEUED);
 }
 return o->view.fault?PTZ_RETAINED:PTZ_OK;
}
int ptz_owner_stop(struct ptz_owner *o,const struct camera_access_principal *p,
 const char sensor[CAMERA_ID_BYTES],uint64_t now)
{
 if(!usable(o))return PTZ_INVALID;
 if(!disjoint(o,p,sizeof(*p))||!disjoint(o,sensor,CAMERA_ID_BYTES)||!ptz_id_valid(sensor))return PTZ_INVALID;
 if(ptz_auth_check(o->auth,p,sensor,now)!=CAMERA_ACCESS_OK)return PTZ_DENIED;
 if(observe(o,now)){stop_owned(o,1,1);return PTZ_FAULT;}
 stop_owned(o,0,1);return o->owned?PTZ_RETAINED:PTZ_OK;
}
int ptz_owner_quiesce(struct ptz_owner *o,uint32_t reason,uint64_t now)
{
 if(!usable(o)||reason>PTZ_REBOOT)return PTZ_INVALID;
 o->view.position_reason=reason;
 if(observe(o,now))o->view.fault=1;
 stop_owned(o,0,1);return o->owned?PTZ_RETAINED:PTZ_OK;
}
int ptz_owner_dispose(struct ptz_owner *o,uint64_t now)
{
 if(!usable(o))return PTZ_INVALID;
 o->disposing=1;
 if(observe(o,now))o->view.fault=1;
 stop_owned(o,0,1);return o->owned?PTZ_RETAINED:PTZ_OK;
}
int ptz_owner_poll(struct ptz_owner *o,uint64_t now)
{
 struct ptz_request q;int result;uint32_t had_owned;
 if(!usable(o))return PTZ_INVALID;
 if(observe(o,now)){stop_owned(o,1,1);return PTZ_FAULT;}
 if(o->stop_sent){
  if(now>=o->stop_deadline_ns){stop_owned(o,1,1);return PTZ_RETAINED;}
  return o->view.fault?PTZ_RETAINED:PTZ_OK;
 }
 if(!o->active)return o->view.fault?PTZ_FAULT:PTZ_OK;
 if(o->view.fault||o->disposing){stop_owned(o,0,1);return PTZ_FAULT;}
 if(now>=o->job.expires_ns||ptz_auth_check(o->auth,&o->job.move.principal,o->job.move.sensor,now)!=CAMERA_ACCESS_OK){
  stop_owned(o,0,1);return PTZ_DENIED;
 }
 if(o->phase_waiting){
  if(now>=o->phase_ack_deadline_ns){stop_owned(o,1,1);return PTZ_RETAINED;}
  return PTZ_OK;
 }
 if(o->job.next_phase&&now-o->phase_sent_ns<PTZ_PHASE_NS)return PTZ_OK;
 if(o->job.next_phase==o->job.move.phases){stop_owned(o,0,0);return PTZ_OK;}
 if(UINT64_MAX-now<PTZ_PHASE_NS){stop_owned(o,1,1);return PTZ_EXHAUSTED;}
 memset(&q,0,sizeof(q));q.kind=PTZ_PHASE;q.ticket=o->job.phase[o->job.next_phase];
 q.work=o->job.work;q.axis=o->job.move.axis;q.phase_bits=phases[o->job.phase_index];
 q.issued_ns=now;q.neutral_by_ns=now+PTZ_PHASE_NS;
 if(q.neutral_by_ns>o->job.expires_ns)q.neutral_by_ns=o->job.expires_ns;
 had_owned=o->owned;o->owned=1;o->phase_waiting=1;o->phase_sent_ns=now;
 o->phase_ack_deadline_ns=q.neutral_by_ns;o->view.pending=q.ticket;
 o->view.electrical_valid=0;state(o,PTZ_PHASE_PENDING);
 result=submit(o,&q);
 if(result!=PTZ_ACCEPTED){
  if(result==PTZ_NO_EFFECT)o->owned=had_owned;
  stop_owned(o,1,1);return o->owned?PTZ_RETAINED:PTZ_FAULT;
 }
 return PTZ_OK;
}
int ptz_owner_complete(struct ptz_owner *o,const struct ptz_completion *c,uint64_t now)
{
 int time_bad;int32_t sign;
 if(!usable(o))return PTZ_INVALID;
 if(!disjoint(o,c,sizeof(*c)))return internal_input_fault(o);
 time_bad=observe(o,now);
 if(time_bad)stop_owned(o,1,1);
 if(c->kind!=PTZ_PHASE&&c->kind!=PTZ_NEUTRAL)return PTZ_STALE;
 if(!ptz_ticket_equal(&c->ticket,&o->view.pending))return PTZ_STALE;
 if(c->result>PTZ_UNKNOWN||c->electrical_valid>1||c->observed_neutral>1||
    c->observed_phase_bits>15)return internal_input_fault(o);
 if(o->stop_sent){
  if(c->kind!=PTZ_NEUTRAL||!ptz_ticket_equal(&c->ticket,&o->job.stop)||o->stop_terminal)return PTZ_STALE;
  if(c->result!=PTZ_ACK){
   if(c->result==PTZ_FAILED)o->stop_terminal=1;
   stop_owned(o,1,1);return PTZ_RETAINED;
  }
  if(c->electrical_valid&&!c->observed_neutral){o->stop_terminal=1;stop_owned(o,1,1);return PTZ_RETAINED;}
  if(now>=o->stop_deadline_ns||time_bad)o->view.fault=1;
  o->view.stop_ack_ns=o->view.last_ns;o->view.applied_kind=PTZ_NEUTRAL;
  o->view.electrical_valid=c->electrical_valid;o->view.observed_neutral=c->observed_neutral;
  o->view.observed_phase_bits=c->observed_phase_bits;o->owned=0;
  idle(o);
  if(o->has_replacement&&!o->view.fault&&!o->disposing&&now<o->replacement.expires_ns&&
     ptz_auth_check(o->auth,&o->replacement.move.principal,o->replacement.move.sensor,now)==CAMERA_ACCESS_OK){
   o->job=o->replacement;o->active=1;o->has_replacement=0;
   o->view.active_work=o->job.work;o->view.desired_kind=PTZ_PHASE;state(o,PTZ_QUEUED);
  }else{o->has_replacement=0;o->view.queued=0;}
  return PTZ_OK;
 }
 if(!o->active||!o->phase_waiting||c->kind!=PTZ_PHASE||
    !ptz_ticket_equal(&c->ticket,&o->job.phase[o->job.next_phase]))return PTZ_STALE;
 if(now>=o->job.expires_ns||now>o->phase_ack_deadline_ns||
    ptz_auth_check(o->auth,&o->job.move.principal,o->job.move.sensor,now)!=CAMERA_ACCESS_OK){
  stop_owned(o,now>o->phase_ack_deadline_ns,1);return PTZ_RETAINED;
 }
 if(c->result!=PTZ_ACK){stop_owned(o,1,1);return PTZ_RETAINED;}
 if(c->electrical_valid&&!c->observed_neutral&&c->observed_phase_bits!=phases[o->job.phase_index]){
  stop_owned(o,1,1);return PTZ_RETAINED;
 }
 o->view.applied_kind=PTZ_PHASE;o->view.applied_phase_bits=phases[o->job.phase_index];
 if(o->view.applied_phases==UINT32_MAX){stop_owned(o,1,1);return PTZ_EXHAUSTED;}
 o->view.applied_phases++;o->view.electrical_valid=c->electrical_valid;
 o->view.observed_neutral=c->observed_neutral;o->view.observed_phase_bits=c->observed_phase_bits;
 sign=o->job.move.direction*(o->job.move.axis?o->config.tilt_sign:o->config.pan_sign);
 o->job.phase_index=(o->job.phase_index+(sign>0?1U:3U))&3U;
 o->axis_phase[o->job.move.axis]=o->job.phase_index;
 o->job.next_phase++;o->phase_waiting=0;memset(&o->view.pending,0,sizeof(o->view.pending));
 state(o,PTZ_CADENCE);
 return PTZ_OK;
}
int ptz_owner_reconfigure(struct ptz_owner *o,const struct ptz_config *c,uint64_t generation,uint64_t now)
{
 if(!usable(o))return PTZ_INVALID;
 if(!disjoint(o,c,sizeof(*c)))return internal_input_fault(o);
 if(observe(o,now)){stop_owned(o,1,1);return PTZ_FAULT;}
 if(o->owned||o->active||o->has_replacement)return PTZ_BUSY;
 if(o->view.fault||o->disposing)return PTZ_FAULT;
 if(!config_valid(c)||!generation)return PTZ_INVALID;
 if(generation<=o->view.generation)return PTZ_STALE;
 o->config=*c;o->view.generation=generation;o->registry->heads[o->head_slot].generation=generation;
 o->view.position_reason=PTZ_REBOOT;o->axis_phase[0]=o->axis_phase[1]=0;return PTZ_OK;
}
int ptz_owner_view(const struct ptz_owner *o,struct ptz_view *v)
{
 if(!o||o->self!=o||!o->registry||!o->auth||!v||!disjoint(o,v,sizeof(*v)))return PTZ_INVALID;
 *v=o->view;return PTZ_OK;
}
