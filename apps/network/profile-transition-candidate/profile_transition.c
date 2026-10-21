/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "profile_transition.h"
#include <mbedtls/platform_util.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#define PT_MAGIC 0x50544331U
static int ready(const struct profile_transition *s){return s&&s->initialized==PT_MAGIC;}
static int token_eq(struct camera_network_token a,struct camera_network_token b)
{return a.namespace_id==b.namespace_id&&a.id==b.id;}
static int handle_eq(const struct pp_handle *a,const struct pp_handle *b)
{return a->scope_id==b->scope_id&&a->owner_epoch==b->owner_epoch&&a->key_epoch==b->key_epoch&&a->profile_id==b->profile_id&&a->revision==b->revision;}
static int work_eq(const struct camera_network_work *a,const struct camera_network_work *b)
{return token_eq(a->token,b->token)&&a->config_id==b->config_id&&a->radio_generation==b->radio_generation&&a->deadline_ns==b->deadline_ns&&a->effect==b->effect;}
static int lease_eq(const struct profile_transition *s,struct pt_lease l)
{return s->active&&l.instance==s->lease.instance&&l.id==s->lease.id&&token_eq(l.operation,s->lease.operation);}
static int clock_now(struct profile_transition *s,uint64_t *now)
{if(s->config.clock_ns(s->config.clock_context,now)||*now<s->last_ns)return PT_HOLD;s->last_ns=*now;return 0;}
static int policy_check(struct profile_transition *s,const struct camera_access_principal *p)
{
 struct camera_access_request q={0};uint64_t now;
 if(!p||p->fact.domain!=CAMERA_ACCESS_ADMIN||clock_now(s,&now))return PT_DENIED;
 q.abi=CAMERA_ACCESS_ABI;q.struct_bytes=sizeof(q);q.domain=CAMERA_ACCESS_ADMIN;
 q.kind=CAMERA_ACCESS_DEVICE;q.operation=CAMERA_ACCESS_DEVICE_CONFIG;
 memcpy(q.device_id,s->config.device_id,CAMERA_ID_BYTES);
 if(camera_access_check(s->policy,p,&q,now))return PT_DENIED;
 if(clock_now(s,&now))return PT_DENIED;
 return camera_access_check(s->policy,p,&q,now)?PT_DENIED:0;
}
static int owner_check(struct profile_transition *s,const struct camera_access_principal *p)
{
 if(!p||p->snapshot_serial!=s->owner.snapshot_serial||p->policy_revision!=s->owner.policy_revision)return PT_DENIED;
 return policy_check(s,p);
}
static int ref_index(const struct profile_transition *s,uint64_t id)
{for(unsigned i=0;i<2;i++)if(s->config.profiles[i].profile_id==id)return (int)i;return -1;}
static int live_profile(struct profile_transition *s,uint64_t id)
{
 struct pp_handle h={0};int i=ref_index(s,id),r;
 if(i<0)return PT_INVALID;
 r=pp_store_handle(s->profiles,id,s->config.profiles[i].revision,&h);
 if(r||!handle_eq(&h,&s->config.profiles[i]))return PT_HOLD;
 return 0;
}
static int both_profiles(struct profile_transition *s)
{return live_profile(s,s->config.profiles[0].profile_id)||live_profile(s,s->config.profiles[1].profile_id)?PT_HOLD:0;}
static int meta_public_id(void *ctx,uint64_t id)
{struct profile_transition *s=ctx;return ref_index(s,id)>=0?0:CAMERA_SETTINGS_SENSITIVE;}
static int meta_clock(void *ctx,uint64_t *now)
{struct profile_transition *s=ctx;return clock_now(s,now);}
static int meta_reconcile(void *ctx,const struct camera_ntx_image *image,const struct camera_ntx_reconciliation *p)
{
 struct profile_transition *s=ctx;int r;
 if(policy_check(s,&s->owner)||live_profile(s,p->profile_id)||policy_check(s,&s->owner))return -1;
 r=s->config.reconcile(s->config.source_context,image,p);
 return r||policy_check(s,&s->owner)?-1:0;
}
static int meta_effect_ready(void *ctx,uint32_t effect,uint64_t id,uint64_t generation)
{
 struct profile_transition *s=ctx;int recovery=s->safety&&effect==CAMERA_NETWORK_ROLLBACK;
 (void)generation;
 if(owner_check(s,&s->owner)&&!recovery)return -1;
 if(ref_index(s,id)<0||both_profiles(s))return -1;
 return owner_check(s,&s->owner)&&!recovery?-1:0;
}
static int meta_latest(void *ctx,struct camera_network_token token,uint64_t id,uint64_t generation,struct camera_network_link *out)
{
 struct profile_transition *s=ctx;struct camera_network_link p={0};
 if(owner_check(s,&s->owner)||both_profiles(s)||owner_check(s,&s->owner)||
    s->config.latest(s->config.source_context,&p)||!token_eq(p.token,token)||p.config_id!=id||p.radio_generation!=generation)return -1;
 if(owner_check(s,&s->owner)||both_profiles(s)||owner_check(s,&s->owner))return -1;
 *out=p;return 0;
}
static int gate(struct profile_transition *s,const struct camera_access_principal *p,struct pt_lease l,int profiles)
{
 if(!ready(s)||!lease_eq(s,l))return PT_STALE;
 if(owner_check(s,p))return PT_DENIED;
 if(profiles&&both_profiles(s))return PT_HOLD;
 return owner_check(s,p);
}
static int private_work_gate(struct profile_transition *s)
{
 struct camera_network_work live;uint64_t now;
 if(s->abort_requested&&!(s->safety&&s->work.effect==CAMERA_NETWORK_ROLLBACK))return PT_DENIED;
 if(!s->safety&&owner_check(s,&s->owner))return PT_DENIED;
 if(s->safety&&s->work.effect!=CAMERA_NETWORK_ROLLBACK)return PT_DENIED;
 if(clock_now(s,&now)||now>=s->work.deadline_ns||
    camera_network_transaction_live(&s->metadata,&live)||!work_eq(&live,&s->work))return PT_HOLD;
 return 0;
}
static int copy_and_begin(void *ctx,const struct pp_profile *private_profile)
{
 struct profile_transition *s=ctx;s->extraction_result=private_work_gate(s);
 if(s->extraction_result)return 0; /* do not destroy key solely on admin denial */
 memcpy(&s->held,private_profile,sizeof(s->held));s->held_present=1;
 s->extraction_result=private_work_gate(s);
 if(s->extraction_result)return 0;
 s->attempted=1;
 s->extraction_result=s->config.begin(s->config.source_context,&s->work,&s->held);
 /* The private pointer can be retained even on source failure. Never wipe
  * based on begin/cancel/discard/revoke outcome; require actual retirement. */
 if(!s->extraction_result)s->extraction_result=private_work_gate(s);
 return 0;
}
int pt_open(struct profile_transition *s,const struct pt_config *c,struct camera_access_policy *policy,
 const struct camera_access_principal *principal,struct pp_session *profiles,int dirfd)
{
 struct camera_ntx_config meta={0};int r;
 if(!s||!c||!policy||!profiles||!principal||!c->clock_ns||!c->begin||!c->step||!c->retired||!c->latest||!c->reconcile||
    !c->instance_id||!c->source_id||!c->scope_id||c->profiles[0].profile_id==c->profiles[1].profile_id)return PT_INVALID;
 if(ready(s))return PT_BUSY; /* retained close/HOLD cannot be erased by open */
 memset(s,0,sizeof(*s));
#ifndef PT_PUBLIC_DUMMY_BUILD
 (void)dirfd;return PT_HOLD;
#else
 if(c->source_class!=PT_SOURCE_SYNTHETIC)return PT_HOLD;
 s->config=*c;s->policy=policy;s->profiles=profiles;s->owner=*principal;
 if(policy_check(s,principal)||both_profiles(s)||policy_check(s,principal))return PT_HOLD;
 if(syscall(SYS_mlock,&s->held,sizeof(s->held)))return PT_HOLD;
 s->locked=1;s->initialized=PT_MAGIC;
 meta.scope_id=c->scope_id;meta.transition_timeout_ns=c->transition_timeout_ns;meta.rollback_timeout_ns=c->rollback_timeout_ns;
 meta.public_profile_id=meta_public_id;meta.profile_context=s;meta.clock_ns=meta_clock;meta.clock_context=s;
 meta.reconcile=meta_reconcile;meta.reconcile_context=s;meta.effect_ready=meta_effect_ready;meta.effect_context=s;
 meta.latest_link=meta_latest;meta.latest_context=s;
 r=camera_network_transaction_open(&s->metadata,dirfd,&meta,c->storage_fault,c->storage_context);
 s->metadata_open_result=r;
 return r;
#endif
}
int pt_seed_and_bind(struct profile_transition *s,const struct camera_access_principal *p,uint64_t id,
 uint64_t revision,const struct camera_ntx_reconciliation *proof)
{
 int r;if(!ready(s)||!proof||owner_check(s,p)||both_profiles(s)||owner_check(s,p))return PT_HOLD;
 s->metadata_started=1; /* even a failed attempt can own a pending disk fence */
 r=camera_network_transaction_seed(&s->metadata,id,revision);if(r)return r;
 r=camera_network_transaction_reconcile(&s->metadata,proof);if(r)return r;
 return owner_check(s,p);
}
int pt_prepare(struct profile_transition *s,const struct camera_access_principal *p,const struct camera_network_request *q,
 const struct pp_handle *proposed,struct pt_lease *out)
{
 int i,r;struct camera_ntx_report old;
 if(!out)return PT_INVALID;
 memset(out,0,sizeof(*out));
 if(!ready(s)||!p||!q||!proposed)return PT_INVALID;
 if(policy_check(s,p))return PT_DENIED;
 if(!q->token.namespace_id||!q->token.id||
    (q->kind!=CAMERA_NETWORK_SWITCH&&q->kind!=CAMERA_NETWORK_SCAN)||
    (q->required_facts&(CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS))!=(CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS)||
    q->required_facts&~(CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS|CAMERA_NETWORK_LOCAL_REACHABLE|CAMERA_NETWORK_ROUTE))return PT_INVALID;
 i=ref_index(s,q->config_id);
 if(i<0||!handle_eq(proposed,&s->config.profiles[i]))return PT_INVALID;
 if(s->active){
  if(token_eq(q->token,s->lease.operation)&&p->snapshot_serial==s->owner.snapshot_serial&&
     !camera_network_transaction_query(&s->metadata,q->token,&old)&&
     old.receipt.request.config_id==q->config_id&&old.receipt.request.kind==q->kind&&
     old.receipt.request.expected_revision==q->expected_revision&&old.receipt.request.required_facts==q->required_facts){
   *out=s->lease;return 0; /* readonly idempotent result, no crypto/fsync/effect */
  }
  return PT_BUSY;
 }
 if(s->next_lease==UINT64_MAX)return PT_HOLD;
 if(both_profiles(s)||policy_check(s,p))return PT_HOLD;
 s->owner=*p;
 s->metadata_started=1;
 r=camera_network_transaction_prepare(&s->metadata,q);if(r)return r;
 s->lease.instance=s->config.instance_id;s->lease.id=++s->next_lease;s->lease.operation=q->token;s->active=1;
 *out=s->lease;return owner_check(s,p);
}
int pt_receipt_ack(struct profile_transition *s,const struct camera_access_principal *p,struct pt_lease l)
{int r=gate(s,p,l,1);if(r)return r;r=camera_network_transaction_receipt_ack(&s->metadata,l.operation);return r?r:owner_check(s,p);}
static int dispatch(struct profile_transition *s,struct camera_network_work *out)
{
 int i,r;memset(out,0,sizeof(*out));
 if(s->logical_work||s->held_present)return PT_BUSY;
 r=camera_network_transaction_next(&s->metadata,&s->work);if(r)return r;
 s->logical_work=1;i=ref_index(s,s->work.config_id);if(i<0)return PT_HOLD;
 s->extraction_result=PT_HOLD;
 r=pp_store_use_private(s->profiles,s->work.config_id,s->config.profiles[i].revision,copy_and_begin,s);
 if(r||s->extraction_result){
  s->abort_requested=1;(void)camera_network_transaction_cancel(&s->metadata,s->lease.operation);
  return r?PT_HOLD:s->extraction_result;
 }
 *out=s->work;return 0;
}
int pt_dispatch(struct profile_transition *s,const struct camera_access_principal *p,struct pt_lease l,struct camera_network_work *out)
{
 int r;if(!out)return PT_INVALID;
 memset(out,0,sizeof(*out));r=gate(s,p,l,1);if(r)return r;
 if(s->abort_requested)return PT_DENIED;
 s->safety=0;return dispatch(s,out);
}
static int continuation(struct profile_transition *s,const struct camera_network_work *w,uint32_t step)
{
 int r;if(!w||!s->logical_work||!s->held_present||!s->attempted||!work_eq(w,&s->work)||
          (step!=PT_SYNTH_ASSOCIATE&&step!=PT_SYNTH_ADDRESS))return PT_STALE;
 if(both_profiles(s))return PT_HOLD;
 r=private_work_gate(s);if(r)return r; /* fresh AFTER crypto/policy before effect */
 r=s->config.step(s->config.source_context,w,&s->held,step);
 if(!r&&both_profiles(s))r=PT_HOLD;
 if(!r)r=private_work_gate(s);
 if(r){s->abort_requested=1;(void)camera_network_transaction_cancel(&s->metadata,s->lease.operation);}
 return r;
}
int pt_continue(struct profile_transition *s,const struct camera_access_principal *p,struct pt_lease l,
 const struct camera_network_work *w,uint32_t step)
{
 int r;if(!ready(s)||!lease_eq(s,l)||!w||!work_eq(w,&s->work))return PT_STALE;
 r=gate(s,p,l,0);if(r)return r;s->safety=0;return continuation(s,w,step);
}
int pt_cancel(struct profile_transition *s,const struct camera_access_principal *p,struct pt_lease l)
{int r=gate(s,p,l,0);if(r)return r;s->abort_requested=1;return camera_network_transaction_cancel(&s->metadata,l.operation);}
int pt_frontend_discard(struct profile_transition *s,const struct camera_access_principal *p,struct pt_lease l)
{return pt_cancel(s,p,l);}
int pt_confirm(struct profile_transition *s,const struct camera_access_principal *p,struct pt_lease l,uint64_t rev)
{int r=gate(s,p,l,1);if(r)return r;r=camera_network_transaction_confirm(&s->metadata,l.operation,rev);return r?r:owner_check(s,p);}
int pt_query(struct profile_transition *s,const struct camera_access_principal *p,struct camera_network_token token,struct pt_report *out)
{
 int r;if(!out)return PT_INVALID;
 memset(out,0,sizeof(*out));if(!ready(s)||policy_check(s,p))return PT_DENIED;
 r=camera_network_transaction_query(&s->metadata,token,&out->metadata);if(r)return r;
 out->synthetic_class=PT_SOURCE_SYNTHETIC;
 if(token_eq(token,s->lease.operation)){out->held_private_copy=s->held_present;out->source_attempted=s->attempted;
  out->logical_work=s->logical_work;out->abort_requested=s->abort_requested;}
 return 0;
}
static int source_gate(struct profile_transition *s,uint64_t source,struct pt_lease l)
{return ready(s)&&source==s->config.source_id&&lease_eq(s,l)?0:PT_STALE;}
int pt_source_observe(struct profile_transition *s,uint64_t source,struct pt_lease l,const struct camera_network_link *p)
{
 struct camera_network_report q;uint64_t config,generation;
 int r=source_gate(s,source,l);if(r)return r;if(!p||!token_eq(p->token,l.operation))return PT_STALE;
 if(camera_network_query(&s->metadata.core,l.operation,&q))return PT_STALE;
 config=q.state==CAMERA_NETWORK_ROLLING_BACK?q.old_config_id:q.request.config_id;
 generation=q.state==CAMERA_NETWORK_ROLLING_BACK?q.rollback_generation:q.apply_generation;
 if(p->config_id!=config||p->radio_generation!=generation||!p->observation_id||
    p->observation_id<q.link_observation_id||(p->observation_id==q.link_observation_id&&p->facts!=q.link_facts))return PT_STALE;
 if(owner_check(s,&s->owner)){s->abort_requested=1;(void)camera_network_transaction_cancel(&s->metadata,l.operation);}
 /* Loss/unknown observation is safety evidence, not permission to confirm. */
 return camera_network_transaction_link(&s->metadata,p);
}
int pt_source_abort(struct profile_transition *s,uint64_t source,struct pt_lease l)
{
 int r=source_gate(s,source,l);if(r)return r;s->abort_requested=1;
 return camera_network_transaction_cancel(&s->metadata,l.operation);
}
int pt_source_retire(struct profile_transition *s,uint64_t source,struct pt_lease l,const struct camera_network_work *w,
 uint32_t success,const struct camera_network_link *proof)
{
 struct camera_network_work remaining;struct camera_network_link latest={0};int r=source_gate(s,source,l);
 if(r)return r;
 if(!w||!s->logical_work||!work_eq(w,&s->work)||success>1)return PT_STALE;
 if(s->config.retired(s->config.source_context,w))return PT_BUSY;
 /* Revoked admin cannot continue/confirm; trusted source may abort only this
  * exact owned work before consuming its real retirement receipt. */
 if(owner_check(s,&s->owner)){s->abort_requested=1;(void)camera_network_transaction_cancel(&s->metadata,l.operation);}
 if(both_profiles(s)){
  s->abort_requested=1;(void)camera_network_transaction_cancel(&s->metadata,l.operation);
  if(w->effect==CAMERA_NETWORK_ROLLBACK){success=0;proof=NULL;} /* no stale-profile restoration claim */
 }
 if(owner_check(s,&s->owner)){s->abort_requested=1;(void)camera_network_transaction_cancel(&s->metadata,l.operation);}
 if(w->effect==CAMERA_NETWORK_ROLLBACK&&success){
  if(s->config.latest(s->config.source_context,&latest)||!token_eq(latest.token,w->token)||
     latest.config_id!=w->config_id||latest.radio_generation!=w->radio_generation||!latest.observation_id)return PT_HOLD;
  if(both_profiles(s)){success=0;proof=NULL;s->abort_requested=1;}
  else {
   r=camera_network_transaction_link(&s->metadata,&latest);
   if(r==CAMERA_NETWORK_STALE||r==CAMERA_NETWORK_ORDER||r==CAMERA_NETWORK_CLOCK)return PT_HOLD;
  }
 }
 if(owner_check(s,&s->owner)){s->abort_requested=1;(void)camera_network_transaction_cancel(&s->metadata,l.operation);}
 r=camera_network_transaction_complete(&s->metadata,w,success,proof);
 if(camera_network_transaction_live(&s->metadata,&remaining)==CAMERA_NETWORK_NOT_READY){
  mbedtls_platform_zeroize(&s->held,sizeof(s->held));s->held_present=0;s->logical_work=0;s->attempted=0;
 }
 return r;
}
int pt_source_rollback(struct profile_transition *s,uint64_t source,struct pt_lease l,struct camera_network_work *out)
{
 struct camera_ntx_report r;int e;if(!out)return PT_INVALID;
 memset(out,0,sizeof(*out));
 e=source_gate(s,source,l);if(e)return e;
 e=camera_network_transaction_query(&s->metadata,l.operation,&r);if(e)return e;
 if(r.volatile_state!=CAMERA_NETWORK_ROLLBACK_PENDING||s->logical_work||s->held_present)return PT_BUSY;
 s->safety=1;e=dispatch(s,out);s->safety=0;return e;
}
int pt_source_continue_rollback(struct profile_transition *s,uint64_t source,struct pt_lease l,
 const struct camera_network_work *w,uint32_t step)
{
 int r=source_gate(s,source,l);if(r)return r;
 if(!w||w->effect!=CAMERA_NETWORK_ROLLBACK||!work_eq(w,&s->work))return PT_STALE;
 /* Actual policy checked even though exact trusted cleanup can continue. */
 (void)owner_check(s,&s->owner);s->safety=1;r=continuation(s,w,step);s->safety=0;return r;
}
int pt_source_pending(const struct profile_transition *s,uint64_t source,struct pt_lease l,struct camera_network_work *out)
{
 if(!out)return PT_INVALID;
 memset(out,0,sizeof(*out));
 if(!ready(s)||source!=s->config.source_id||!lease_eq(s,l)||!s->logical_work)return PT_STALE;
 *out=s->work;return 0;
}
static int terminal(uint32_t state)
{return state==CAMERA_NETWORK_CONFIRMED||state==CAMERA_NETWORK_RESTORED||
 state==CAMERA_NETWORK_FAILED||state==CAMERA_NETWORK_CANCELLED;}
static int receipt_eq(const struct camera_ntx_receipt *a,const struct camera_ntx_receipt *b)
{
 return token_eq(a->request.token,b->request.token)&&a->request.expected_revision==b->request.expected_revision&&
  a->request.config_id==b->request.config_id&&a->request.kind==b->request.kind&&a->request.required_facts==b->request.required_facts&&
  a->old_profile_id==b->old_profile_id&&a->apply_generation==b->apply_generation&&a->rollback_generation==b->rollback_generation&&
  a->result_profile_id==b->result_profile_id&&a->result_revision==b->result_revision&&a->state==b->state&&
  a->reason==b->reason&&a->rollback_reason==b->rollback_reason&&a->phase_flags==b->phase_flags;
}
static int release_empty_region(struct profile_transition *s)
{
 /* Only called after actual scope-lifetime checks. Never close borrowed policy,
  * private provider, metadata dirfd, files or owning source here. */
 if(s->locked&&syscall(SYS_munlock,&s->held,sizeof(s->held)))return PT_HOLD;
 mbedtls_platform_zeroize(&s->held,sizeof(s->held));s->initialized=0;s->locked=0;return 0;
}
int pt_close(struct profile_transition *s)
{
 struct camera_ntx_report r;struct camera_ntx_inspection disk;struct camera_network_token none={0};int e,found=0;
 if(!ready(s))return PT_INVALID;
 if(s->logical_work||s->held_present)return PT_BUSY;
 /* Inspect ONLY public metadata; no policy/private crypto callbacks/unseal.
  * Cached FINAL cannot authorize close after disk failure or another revision. */
 e=camera_network_transaction_inspect(&s->metadata,&disk);
 if(e!=CAMERA_NTX_OK||disk.settings_result!=CAMERA_SETTINGS_OK||!disk.current_valid||disk.pending_valid)return PT_HOLD;
 if(s->metadata.blocked||s->metadata.uncertain)return PT_HOLD;
 if(disk.settings_revision!=s->metadata.settings_revision||disk.settings_operation_id!=s->metadata.settings_operation_id||
    disk.current.scope_id!=s->config.scope_id||disk.current.active_index!=CAMERA_NTX_NONE)return PT_HOLD;
 if(s->active){
  e=camera_network_transaction_query(&s->metadata,s->lease.operation,&r);
  if(e!=CAMERA_NTX_OK)return PT_HOLD;
  if(r.reconciliation_required||!r.runtime_bound||r.in_flight||r.work_may_be_live||
     !terminal(r.receipt.state)||r.published_state!=r.receipt.state||
     r.volatile_state!=r.receipt.state||r.settings_revision!=disk.settings_revision)return PT_BUSY;
  for(unsigned i=0;i<disk.current.count;i++)if(receipt_eq(&disk.current.receipts[i],&r.receipt)){found=1;break;}
  if(!found)return PT_HOLD;
 }else{
  /* A source-bound empty history is closable, not a reopened/blocked history.
   * Actual unknown-token readiness query exposes RECONCILE vs ready STALE. */
  e=camera_network_transaction_query(&s->metadata,none,&r);
  if(disk.current.count||e!=CAMERA_NETWORK_STALE||r.reconciliation_required)return PT_HOLD;
 }
 return release_empty_region(s);
}
int pt_abandon_empty(struct profile_transition *s)
{
 struct camera_ntx_inspection disk;int e;
 if(!ready(s))return PT_INVALID;
 if(s->active||s->metadata_started||s->logical_work||s->held_present||s->attempted||s->next_lease||s->lease.id)return PT_BUSY;
 if(s->metadata.loaded||
    (s->metadata_open_result!=CAMERA_NTX_EMPTY&&s->metadata_open_result!=CAMERA_NTX_INVALID&&
     s->metadata_open_result!=CAMERA_NETWORK_CLOCK))return PT_HOLD;
 e=camera_network_transaction_inspect(&s->metadata,&disk);
 /* INVALID paired with the constructor's INVALID means backend validation
  * failed BEFORE adopting/initializing a metadata store, not query uncertainty.
  * Otherwise require real EMPTY inspection with no current/pending/history. */
 if(e==CAMERA_NTX_INVALID&&s->metadata_open_result==CAMERA_NTX_INVALID)return release_empty_region(s);
 if(e!=CAMERA_NTX_EMPTY||disk.settings_result!=CAMERA_SETTINGS_OK||disk.current_valid||disk.pending_valid||
    disk.settings_revision||disk.settings_operation_id)return PT_HOLD;
 return release_empty_region(s);
}
