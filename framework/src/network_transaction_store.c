/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/network_transaction_store.h>
#include <string.h>
#define NTX_MAGIC 0x3158544eU
#define CTX_MAGIC 0x4e545843U
#define PUBLIC_IDS 1U
#define BASE_FACTS (CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS)
#define ALL_FACTS (BASE_FACTS|CAMERA_NETWORK_LOCAL_REACHABLE|CAMERA_NETWORK_ROUTE)
_Static_assert(CAMERA_NTX_HEADER_BYTES+CAMERA_NETWORK_RESULTS*CAMERA_NTX_RECEIPT_BYTES==CAMERA_NTX_WIRE_BYTES,"wire bound");
_Static_assert(CAMERA_NTX_WIRE_BYTES<=CAMERA_SETTINGS_MAX_PAYLOAD,"existing ceiling");
static uint32_t get32(const unsigned char *p)
{return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t get64(const unsigned char *p){return get32(p)|((uint64_t)get32(p+4)<<32);}
static void put32(unsigned char *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void put64(unsigned char *p,uint64_t v){put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}
static int equal(struct camera_network_token a,struct camera_network_token b)
{return a.namespace_id==b.namespace_id&&a.id==b.id;}
static int same_request(const struct camera_network_request *a,const struct camera_network_request *b)
{return equal(a->token,b->token)&&a->expected_revision==b->expected_revision&&a->config_id==b->config_id&&
 a->kind==b->kind&&a->required_facts==b->required_facts;}
static int same_image(const struct camera_ntx_image *a,const struct camera_ntx_image *b)
{
 if(a->scope_id!=b->scope_id||a->namespace_high_water!=b->namespace_high_water||
    a->operation_high_water!=b->operation_high_water||a->profile_high_water!=b->profile_high_water||
    a->radio_high_water!=b->radio_high_water||a->committed_profile_id!=b->committed_profile_id||
    a->committed_revision!=b->committed_revision||a->count!=b->count||a->active_index!=b->active_index)return 0;
 for(unsigned i=0;i<a->count;i++){
  const struct camera_ntx_receipt *x=&a->receipts[i],*y=&b->receipts[i];
  if(!same_request(&x->request,&y->request)||x->old_profile_id!=y->old_profile_id||
     x->apply_generation!=y->apply_generation||x->rollback_generation!=y->rollback_generation||
     x->result_profile_id!=y->result_profile_id||x->result_revision!=y->result_revision||
     x->state!=y->state||x->reason!=y->reason||x->rollback_reason!=y->rollback_reason||
     x->phase_flags!=y->phase_flags)return 0;
 }
 return 1;
}
static int terminal(uint32_t state){return state>=CAMERA_NETWORK_CONFIRMED&&state<=CAMERA_NETWORK_CANCELLED;}
static int public_id(const struct camera_ntx_config *c,uint64_t id)
{return id&&c->public_profile_id(c->profile_context,id)==CAMERA_NTX_OK;}
static int valid_image(const struct camera_ntx_config *c,const struct camera_ntx_image *m)
{
 if(!c||!c->scope_id||!c->public_profile_id||!m||m->scope_id!=c->scope_id||
    m->count>CAMERA_NETWORK_RESULTS||m->active_index>CAMERA_NTX_NONE||
    !public_id(c,m->committed_profile_id)||m->committed_profile_id>m->profile_high_water)
  return CAMERA_SETTINGS_INVALID;
 if(m->active_index!=CAMERA_NTX_NONE&&m->active_index>=m->count)return CAMERA_SETTINGS_INVALID;
 for(unsigned i=0;i<m->count;i++){
  const struct camera_ntx_receipt *r=&m->receipts[i];uint32_t phase=r->phase_flags&255U;
  if(!r->request.token.namespace_id||r->request.token.namespace_id>m->namespace_high_water||
     !r->request.token.id||r->request.token.id>m->operation_high_water||
     (i&&r->request.token.id<=m->receipts[i-1].request.token.id)||
     r->request.expected_revision>m->committed_revision||
     !public_id(c,r->request.config_id)||!public_id(c,r->old_profile_id)||
     !public_id(c,r->result_profile_id)||r->request.config_id>m->profile_high_water||
     r->old_profile_id>m->profile_high_water||r->result_profile_id>m->profile_high_water||
     r->result_revision>m->committed_revision||!r->apply_generation||
     r->apply_generation==UINT64_MAX||r->rollback_generation!=r->apply_generation+1||
     r->rollback_generation>m->radio_high_water||
     (r->request.kind!=CAMERA_NETWORK_SWITCH&&r->request.kind!=CAMERA_NETWORK_SCAN)||
     (r->request.kind==CAMERA_NETWORK_SCAN&&r->request.config_id!=r->old_profile_id)||
     (r->request.required_facts&BASE_FACTS)!=BASE_FACTS||r->request.required_facts&~ALL_FACTS||
     r->state<CAMERA_NETWORK_PREPARED||r->state>CAMERA_NETWORK_CANCELLED||
     (r->reason>CAMERA_NETWORK_REASON_ROLLBACK&&r->reason!=CAMERA_NTX_REASON_RESTART)||
     r->rollback_reason>CAMERA_NETWORK_REASON_ROLLBACK||
     phase<CAMERA_NTX_PREPARED||phase>CAMERA_NTX_RECONCILED||
     r->phase_flags&~(255U|CAMERA_NTX_RECEIPT_ACK))return CAMERA_SETTINGS_INVALID;
  if((phase==CAMERA_NTX_FINAL||phase==CAMERA_NTX_RECONCILED)!=terminal(r->state))
   return CAMERA_SETTINGS_INVALID;
  if(i==m->active_index&&terminal(r->state))return CAMERA_SETTINGS_INVALID;
  if(i!=m->active_index&&!terminal(r->state))return CAMERA_SETTINGS_INVALID;
  if(phase==CAMERA_NTX_RECONCILED&&
     (r->state!=CAMERA_NETWORK_FAILED||r->reason!=CAMERA_NTX_REASON_RESTART))return CAMERA_SETTINGS_INVALID;
  if(phase==CAMERA_NTX_PREPARED&&
     (r->state!=CAMERA_NETWORK_PREPARED||r->phase_flags&CAMERA_NTX_RECEIPT_ACK))return CAMERA_SETTINGS_INVALID;
  if((phase==CAMERA_NTX_ACKED||phase==CAMERA_NTX_APPLY_FENCE)&&
     (r->state!=CAMERA_NETWORK_PREPARED||!(r->phase_flags&CAMERA_NTX_RECEIPT_ACK)))return CAMERA_SETTINGS_INVALID;
  if(phase==CAMERA_NTX_ROLLBACK_FENCE&&
     (r->state!=CAMERA_NETWORK_ROLLBACK_PENDING||!(r->phase_flags&CAMERA_NTX_RECEIPT_ACK)))return CAMERA_SETTINGS_INVALID;
  if(phase==CAMERA_NTX_COMMIT_FENCE&&
     (r->request.kind!=CAMERA_NETWORK_SWITCH||r->state!=CAMERA_NETWORK_CONFIRM_READY||
      r->request.expected_revision==UINT64_MAX||m->committed_profile_id!=r->request.config_id||
      m->committed_revision!=r->request.expected_revision+1||
      !(r->phase_flags&CAMERA_NTX_RECEIPT_ACK)))return CAMERA_SETTINGS_INVALID;
  if(phase==CAMERA_NTX_FINAL){
   if(r->state==CAMERA_NETWORK_CONFIRMED){
    if(r->request.kind!=CAMERA_NETWORK_SWITCH||r->request.expected_revision==UINT64_MAX||
       r->result_profile_id!=r->request.config_id||r->result_revision!=r->request.expected_revision+1||
       !(r->phase_flags&CAMERA_NTX_RECEIPT_ACK))return CAMERA_SETTINGS_INVALID;
   }else if(r->result_profile_id!=r->old_profile_id||r->result_revision!=r->request.expected_revision)
    return CAMERA_SETTINGS_INVALID;
  }
 }
 return CAMERA_SETTINGS_OK;
}
int camera_ntx_encode(const struct camera_ntx_config *c,const struct camera_ntx_image *m,
 unsigned char out[CAMERA_NTX_WIRE_BYTES])
{
 int result;if(!out)return CAMERA_SETTINGS_INVALID;result=valid_image(c,m);if(result)return result;
 memset(out,0,CAMERA_NTX_WIRE_BYTES);put32(out,NTX_MAGIC);put32(out+4,1);
 put32(out+8,CAMERA_NTX_WIRE_BYTES);put32(out+12,PUBLIC_IDS);
 put64(out+16,m->scope_id);put64(out+24,m->namespace_high_water);
 put64(out+32,m->operation_high_water);put64(out+40,m->profile_high_water);
 put64(out+48,m->radio_high_water);put64(out+56,m->committed_profile_id);
 put64(out+64,m->committed_revision);put32(out+72,m->count);put32(out+76,m->active_index);
 for(unsigned i=0;i<m->count;i++){
  const struct camera_ntx_receipt *r=&m->receipts[i];unsigned char *p=out+128+i*96;
  put64(p,r->request.token.namespace_id);put64(p+8,r->request.token.id);
  put64(p+16,r->request.expected_revision);put64(p+24,r->request.config_id);
  put64(p+32,r->old_profile_id);put64(p+40,r->apply_generation);put64(p+48,r->rollback_generation);
  put64(p+56,r->result_profile_id);put64(p+64,r->result_revision);
  put32(p+72,r->state);put32(p+76,r->reason);put32(p+80,r->rollback_reason);
  put32(p+84,r->request.kind);put32(p+88,r->request.required_facts);put32(p+92,r->phase_flags);
 }
 return CAMERA_SETTINGS_OK;
}
int camera_ntx_decode(const struct camera_ntx_config *c,const unsigned char *p,uint32_t n,
 struct camera_ntx_image *out)
{
 struct camera_ntx_image decoded,*m=&decoded;int r;
 if(!c||!out||!p)return CAMERA_SETTINGS_INVALID;
 memset(out,0,sizeof(*out));memset(m,0,sizeof(*m));
 /* Reject non-allowlisted payloads before any CRC/hash/file writes. No generic
  * settings, extension strings, credentials or opaque payload envelopes. */
 if(n!=CAMERA_NTX_WIRE_BYTES||get32(p)!=NTX_MAGIC)return CAMERA_SETTINGS_SENSITIVE;
 if(get32(p+4)!=1)return CAMERA_SETTINGS_UNSUPPORTED;
 if(get32(p+8)!=n||get32(p+12)!=PUBLIC_IDS)return CAMERA_SETTINGS_INVALID;
 for(unsigned i=80;i<128;i++)if(p[i])return CAMERA_SETTINGS_INVALID;
 m->scope_id=get64(p+16);m->namespace_high_water=get64(p+24);
 m->operation_high_water=get64(p+32);m->profile_high_water=get64(p+40);
 m->radio_high_water=get64(p+48);m->committed_profile_id=get64(p+56);
 m->committed_revision=get64(p+64);m->count=get32(p+72);m->active_index=get32(p+76);
 if(m->count>CAMERA_NETWORK_RESULTS)return CAMERA_SETTINGS_INVALID;
 for(unsigned i=0;i<CAMERA_NETWORK_RESULTS;i++){
  const unsigned char *q=p+128+i*96;struct camera_ntx_receipt *r=&m->receipts[i];
  if(i>=m->count){for(unsigned j=0;j<96;j++)if(q[j])return CAMERA_SETTINGS_INVALID;continue;}
  r->request.token.namespace_id=get64(q);r->request.token.id=get64(q+8);
  r->request.expected_revision=get64(q+16);r->request.config_id=get64(q+24);
  r->old_profile_id=get64(q+32);r->apply_generation=get64(q+40);r->rollback_generation=get64(q+48);
  r->result_profile_id=get64(q+56);r->result_revision=get64(q+64);
  r->state=get32(q+72);r->reason=get32(q+76);r->rollback_reason=get32(q+80);
  r->request.kind=get32(q+84);r->request.required_facts=get32(q+88);r->phase_flags=get32(q+92);
 }
 r=valid_image(c,m);if(!r)*out=*m;return r;
}
int camera_ntx_validate(void *ctx,uint32_t schema,uint32_t kind,const unsigned char *p,uint32_t n)
{
 struct camera_ntx_image m;
 if(schema!=CAMERA_NTX_SCHEMA)return CAMERA_SETTINGS_UNSUPPORTED;
 if(kind!=CAMERA_SETTINGS_VALUES)return CAMERA_SETTINGS_SENSITIVE;
 return camera_ntx_decode(ctx,p,n,&m);
}
static int ready(const struct camera_network_transaction_store *s)
{return s&&s->initialized==CTX_MAGIC;}
static int clock_now(struct camera_network_transaction_store *s,uint64_t *now)
{
 if(s->config.clock_ns(s->config.clock_context,now)||*now<s->last_ns){
  s->blocked=1;
  if(s->bound)(void)camera_network_tick(&s->core,0);
  return CAMERA_NETWORK_CLOCK;
 }
 s->last_ns=*now;return CAMERA_NTX_OK;
}
static int find(const struct camera_ntx_image *m,struct camera_network_token t)
{for(unsigned i=0;i<m->count;i++)if(equal(t,m->receipts[i].request.token))return (int)i;return -1;}
static int snapshot(struct camera_network_transaction_store *s,const struct camera_settings_snapshot *v,
 struct camera_ntx_image *m)
{
 if(v->schema!=CAMERA_NTX_SCHEMA||v->kind!=CAMERA_SETTINGS_VALUES)return CAMERA_SETTINGS_UNSUPPORTED;
 return camera_ntx_decode(&s->config,v->payload,v->payload_bytes,m);
}
int camera_network_transaction_inspect(struct camera_network_transaction_store *s,struct camera_ntx_inspection *out)
{
 int r;if(!ready(s)||!out)return CAMERA_NTX_INVALID;memset(out,0,sizeof(*out));
 r=camera_settings_inspect(&s->settings,&s->scratch);s->last_settings_result=r;out->settings_result=r;
 if(s->scratch.current_file==CAMERA_SETTINGS_VALID&&
    !snapshot(s,&s->scratch.current,&out->current)){
  out->current_valid=1;out->settings_revision=s->scratch.current.revision;
  out->settings_operation_id=s->scratch.current.operation_id;
 }
 if(s->scratch.pending_file==CAMERA_SETTINGS_VALID&&
    !snapshot(s,&s->scratch.pending,&out->pending))out->pending_valid=1;
 if(r){s->blocked=1;s->uncertain=1;return CAMERA_NTX_RECONCILE;}
 return out->current_valid?CAMERA_NTX_OK:CAMERA_NTX_EMPTY;
}
int camera_network_transaction_open(struct camera_network_transaction_store *s,int fd,
 const struct camera_ntx_config *c,camera_settings_fault_fn hook,void *hc)
{
 struct camera_ntx_inspection out;uint64_t now;int r;
 if(!s||!c||fd<0||!c->scope_id||!c->public_profile_id||!c->clock_ns||!c->reconcile||
    !c->effect_ready||!c->latest_link||!c->transition_timeout_ns||!c->rollback_timeout_ns)return CAMERA_NTX_INVALID;
 memset(s,0,sizeof(*s));s->config=*c;s->blocked=1;s->initialized=CTX_MAGIC;
 r=camera_settings_init(&s->settings,fd,camera_ntx_validate,&s->config,hook,hc);
 s->last_settings_result=r;if(r)return CAMERA_NTX_RECONCILE;
 r=clock_now(s,&now);if(r)return r;
 r=camera_network_transaction_inspect(s,&out);
 if(out.current_valid){s->image=out.current;s->loaded=1;s->settings_revision=out.settings_revision;
  s->settings_operation_id=out.settings_operation_id;}
 return r==CAMERA_NTX_EMPTY?CAMERA_NTX_EMPTY:CAMERA_NTX_RECONCILE;
}
static int append(struct camera_network_transaction_store *s,const struct camera_ntx_image *m)
{
 unsigned char wire[CAMERA_NTX_WIRE_BYTES];uint64_t now;int r;
 if(s->uncertain)return CAMERA_NTX_RECONCILE;
 r=camera_ntx_encode(&s->config,m,wire);if(r){s->last_settings_result=r;return CAMERA_NTX_INVALID;}
 /* Idempotent scalar replay is not a new durability receipt. Do not consume
  * disk operation/revision counters or fsync again for an unchanged image. */
 if(s->loaded&&same_image(m,&s->image))return CAMERA_NTX_OK;
 if(s->settings_revision==UINT64_MAX||s->settings_operation_id==UINT64_MAX)return CAMERA_NTX_EXHAUSTED;
 /* All candidate bytes are public/validated before the existing store hashes. */
 r=camera_settings_commit(&s->settings,s->settings_revision,s->settings_operation_id+1,
  CAMERA_NTX_SCHEMA,CAMERA_SETTINGS_VALUES,wire,sizeof(wire),&s->scratch);
 s->last_settings_result=r;
 if(r||s->scratch.durability!=CAMERA_SETTINGS_SYNC_CONFIRMED){s->blocked=1;s->uncertain=1;return CAMERA_NTX_STORAGE;}
 s->image=*m;s->loaded=1;s->settings_revision=s->scratch.current.revision;
 s->settings_operation_id=s->scratch.current.operation_id;
 r=clock_now(s,&now);if(r){s->uncertain=1;return r;}
 return CAMERA_NTX_OK;
}
int camera_network_transaction_seed(struct camera_network_transaction_store *s,uint64_t profile,uint64_t revision)
{
 struct camera_ntx_image m={0};struct camera_ntx_inspection out;int r;
 if(!ready(s)||s->bound)return CAMERA_NTX_INVALID;
 if(s->uncertain)return CAMERA_NTX_RECONCILE;
 r=camera_network_transaction_inspect(s,&out);if(r!=CAMERA_NTX_EMPTY)return CAMERA_NTX_RECONCILE;
 m.scope_id=s->config.scope_id;m.committed_profile_id=profile;m.committed_revision=revision;
 m.profile_high_water=profile;m.active_index=CAMERA_NTX_NONE;
 return append(s,&m);
}
int camera_network_transaction_reconcile(struct camera_network_transaction_store *s,
 const struct camera_ntx_reconciliation *p)
{
 struct camera_ntx_inspection out;struct camera_ntx_image m;struct camera_network_config c={0};
 uint64_t now,profile;int r;
 if(!ready(s)||!p||s->bound||s->live_present)return CAMERA_NTX_INVALID;
 if(s->uncertain)return CAMERA_NTX_RECONCILE;
 r=camera_network_transaction_inspect(s,&out);if(r!=CAMERA_NTX_OK)return CAMERA_NTX_RECONCILE;
 m=out.current;profile=m.committed_profile_id;
 if(p->choice!=CAMERA_NTX_CURRENT&&p->choice!=CAMERA_NTX_PREVIOUS)return CAMERA_NTX_INVALID;
 if(m.active_index!=CAMERA_NTX_NONE){
  struct camera_ntx_receipt *old=&m.receipts[m.active_index];
  if(!equal(p->prior_token,old->request.token)||p->prior_apply_generation!=old->apply_generation||
     p->prior_rollback_generation!=old->rollback_generation)return CAMERA_NETWORK_STALE;
  if(p->choice==CAMERA_NTX_PREVIOUS){
   if((old->phase_flags&255U)!=CAMERA_NTX_COMMIT_FENCE)return CAMERA_NTX_INVALID;
   profile=old->old_profile_id;
  }
 }else if(p->prior_token.id||p->prior_token.namespace_id||p->prior_apply_generation||
          p->prior_rollback_generation||p->choice!=CAMERA_NTX_CURRENT)return CAMERA_NETWORK_STALE;
 if(m.namespace_high_water==UINT64_MAX||m.radio_high_water==UINT64_MAX)return CAMERA_NTX_EXHAUSTED;
 if(p->namespace_id<=m.namespace_high_water||p->radio_generation<=m.radio_high_water||
    p->profile_id!=profile)return CAMERA_NETWORK_CONFLICT;
 if(!public_id(&s->config,p->profile_id))return CAMERA_NTX_PROFILE_BLOCKED;
 r=s->config.reconcile(s->config.reconcile_context,&out.current,p);
 if(r)return CAMERA_NTX_RECONCILE;
 if(profile!=m.committed_profile_id){
  if(m.committed_revision==UINT64_MAX)return CAMERA_NTX_EXHAUSTED;
  m.committed_revision++;m.committed_profile_id=profile;
 }
 if(m.active_index!=CAMERA_NTX_NONE){
  struct camera_ntx_receipt *old=&m.receipts[m.active_index];
  old->state=CAMERA_NETWORK_FAILED;old->reason=CAMERA_NTX_REASON_RESTART;
  old->phase_flags=(old->phase_flags&CAMERA_NTX_RECEIPT_ACK)|CAMERA_NTX_RECONCILED;
  old->result_profile_id=m.committed_profile_id;old->result_revision=m.committed_revision;
  m.active_index=CAMERA_NTX_NONE;
 }
 m.namespace_high_water=p->namespace_id;m.radio_high_water=p->radio_generation;
 s->settings_revision=out.settings_revision;s->settings_operation_id=out.settings_operation_id;
 r=append(s,&m);if(r)return r;
 /* Pure proof checks can take time, and the first observation predates fsync.
  * Revalidate the same owner-held proof after the fence; typed IDs are not facts. */
 r=s->config.reconcile(s->config.reconcile_context,&out.current,p);
 if(r){s->blocked=1;return CAMERA_NTX_RECONCILE;}
 r=clock_now(s,&now);if(r)return r;
 c.namespace_id=p->namespace_id;c.operation_high_water=m.operation_high_water;
 c.radio_high_water=m.radio_high_water;c.committed_config_id=m.committed_profile_id;
 c.committed_revision=m.committed_revision;c.transition_timeout_ns=s->config.transition_timeout_ns;
 c.rollback_timeout_ns=s->config.rollback_timeout_ns;
 r=camera_network_init(&s->core,&c,now);if(r){s->blocked=1;return r;}
 s->bound=1;s->blocked=0;return CAMERA_NTX_OK;
}
static int permitted(const struct camera_network_transaction_store *s)
{return ready(s)&&s->bound&&!s->blocked;}
static int core_error(struct camera_network_transaction_store *s,int r)
{if(r==CAMERA_NETWORK_CLOCK)s->blocked=1;return r;}
static void record_core(struct camera_ntx_receipt *r,const struct camera_network_report *q,uint32_t phase)
{
 r->request=q->request;r->old_profile_id=q->old_config_id;r->apply_generation=q->apply_generation;
 r->rollback_generation=q->rollback_generation;r->result_profile_id=q->committed_config_id;
 r->result_revision=q->committed_revision;r->state=q->state;r->reason=q->reason;
 r->rollback_reason=q->rollback_reason;r->phase_flags=phase|(q->receipt_acknowledged?CAMERA_NTX_RECEIPT_ACK:0U);
}
static int save_core(struct camera_network_transaction_store *s,uint32_t phase)
{
 struct camera_network_report q;struct camera_ntx_image m=s->image;int i,r;
 if(s->blocked)return CAMERA_NTX_RECONCILE;
 i=find(&m,s->runtime_token);if(i<0)return CAMERA_NTX_INVALID;
 r=camera_network_query(&s->core,s->runtime_token,&q);if(r)return r;
 if(terminal(q.state)){phase=CAMERA_NTX_FINAL;m.active_index=CAMERA_NTX_NONE;}
 else if(q.state==CAMERA_NETWORK_PREPARED)phase=q.receipt_acknowledged?CAMERA_NTX_ACKED:CAMERA_NTX_PREPARED;
 record_core(&m.receipts[i],&q,phase);return append(s,&m);
}
int camera_network_transaction_prepare(struct camera_network_transaction_store *s,const struct camera_network_request *q)
{
 struct camera_ntx_image m;struct camera_network_report r;uint64_t now;int i,result;
 if(!ready(s)||!q)return CAMERA_NTX_INVALID;
 i=find(&s->image,q->token);
 if(i>=0)return same_request(q,&s->image.receipts[i].request)?CAMERA_NTX_OK:CAMERA_NETWORK_CONFLICT;
 if(!permitted(s))return CAMERA_NTX_RECONCILE;
 if(s->image.active_index!=CAMERA_NTX_NONE||s->live_present)return CAMERA_NETWORK_BUSY;
 if(s->image.count==CAMERA_NETWORK_RESULTS)return CAMERA_NTX_CAPACITY;
 /* Leave room for the ordinary bounded prepare/ACK/dispatch/retire/fence/final
  * sequence. Uncertainty never consumes remaining IDs via automatic retries. */
 if(s->settings_revision>UINT64_MAX-8||s->settings_operation_id>UINT64_MAX-8)return CAMERA_NTX_EXHAUSTED;
 if(!public_id(&s->config,q->config_id))return CAMERA_NTX_PROFILE_BLOCKED;
 if(q->kind==CAMERA_NETWORK_SWITCH&&q->config_id<=s->image.profile_high_water)return CAMERA_NETWORK_CONFLICT;
 if(q->token.id<=s->image.operation_high_water)return CAMERA_NETWORK_CONFLICT;
 result=clock_now(s,&now);if(result)return result;
 result=camera_network_prepare(&s->core,q,now);if(result)return core_error(s,result);
 result=camera_network_query(&s->core,q->token,&r);if(result)return result;
 m=s->image;m.active_index=m.count++;m.operation_high_water=q->token.id;
 if(q->config_id>m.profile_high_water)m.profile_high_water=q->config_id;
 m.radio_high_water=r.rollback_generation;record_core(&m.receipts[m.active_index],&r,CAMERA_NTX_PREPARED);
 s->runtime_token=q->token;return append(s,&m);
}
static int operation(struct camera_network_transaction_store *s,struct camera_network_token t,uint64_t *now)
{
 struct camera_network_report q;int r;
 if(!ready(s)||!s->bound)return CAMERA_NTX_RECONCILE;
 if(!equal(t,s->runtime_token))return CAMERA_NETWORK_STALE;
 r=camera_network_query(&s->core,t,&q);if(r)return r;
 if(terminal(q.state))return CAMERA_NETWORK_ORDER;
 return clock_now(s,now);
}
int camera_network_transaction_receipt_ack(struct camera_network_transaction_store *s,struct camera_network_token t)
{
 struct camera_ntx_image m;struct camera_network_report q;uint64_t now;int r,i;
 if(!permitted(s))return CAMERA_NTX_RECONCILE;
 r=operation(s,t,&now);if(r)return r;
 r=camera_network_tick(&s->core,now);if(r)return core_error(s,r);
 r=camera_network_query(&s->core,t,&q);if(r)return r;
 if(q.state!=CAMERA_NETWORK_PREPARED)return CAMERA_NETWORK_ORDER;
 i=find(&s->image,t);if(i<0)return CAMERA_NETWORK_STALE;
 m=s->image;m.receipts[i].phase_flags=CAMERA_NTX_ACKED|CAMERA_NTX_RECEIPT_ACK;
 r=append(s,&m);if(r)return r;
 r=clock_now(s,&now);if(r)return r;
 r=camera_network_receipt_ack(&s->core,t,now);
 if(r){s->blocked=1;return CAMERA_NTX_RECONCILE;}
 return CAMERA_NTX_OK;
}
static int admit(struct camera_network_transaction_store *s,const struct camera_network_report *q,
 uint32_t effect,uint64_t profile,uint64_t generation)
{
 if(s->config.effect_ready(s->config.effect_context,effect,profile,generation))return 0;
 /* Applying/scanning is not admitted merely because the new handle is usable:
  * the retained old profile must still be durable/unsealed for rollback too. */
 return effect==CAMERA_NETWORK_ROLLBACK||
  !s->config.effect_ready(s->config.effect_context,CAMERA_NETWORK_ROLLBACK,
                         q->old_config_id,q->rollback_generation);
}
static int dispatch_recheck(struct camera_network_transaction_store *s,
 const struct camera_network_report *expected,uint64_t deadline,uint64_t now)
{
 struct camera_network_report current;int r;
 r=camera_network_tick(&s->core,now);if(r)return core_error(s,r);
 r=camera_network_query(&s->core,expected->request.token,&current);
 if(r||now>=deadline||current.state!=expected->state||current.in_flight||
    !same_request(&current.request,&expected->request)||
    current.old_config_id!=expected->old_config_id||current.old_revision!=expected->old_revision||
    current.apply_generation!=expected->apply_generation||current.rollback_generation!=expected->rollback_generation||
    current.deadline_ns!=expected->deadline_ns||current.receipt_acknowledged!=expected->receipt_acknowledged){
  /* Never select a new/rollback tuple under a fence written for older work. */
  s->blocked=1;return CAMERA_NTX_RECONCILE;
 }
 return CAMERA_NTX_OK;
}
int camera_network_transaction_next(struct camera_network_transaction_store *s,struct camera_network_work *out)
{
 struct camera_network_report q;struct camera_ntx_image m;uint64_t now,profile,generation,deadline;
 uint32_t effect,phase;int r,i;
 if(!out)return CAMERA_NTX_INVALID;
 memset(out,0,sizeof(*out));
 if(!permitted(s))return CAMERA_NTX_RECONCILE;
 if(s->live_present||s->image.active_index==CAMERA_NTX_NONE)return CAMERA_NETWORK_NOT_READY;
 r=clock_now(s,&now);if(r)return r;
 r=camera_network_tick(&s->core,now);if(r)return core_error(s,r);
 r=camera_network_query(&s->core,s->runtime_token,&q);if(r)return r;
 if(q.state==CAMERA_NETWORK_PREPARED&&q.receipt_acknowledged){
  effect=q.request.kind==CAMERA_NETWORK_SWITCH?CAMERA_NETWORK_APPLY:CAMERA_NETWORK_DISRUPTIVE_SCAN;
  profile=q.request.config_id;generation=q.apply_generation;phase=CAMERA_NTX_APPLY_FENCE;deadline=q.deadline_ns;
 }else if(q.state==CAMERA_NETWORK_ROLLBACK_PENDING){
  effect=CAMERA_NETWORK_ROLLBACK;profile=q.old_config_id;generation=q.rollback_generation;phase=CAMERA_NTX_ROLLBACK_FENCE;
  /* Separate bounded dispatch-admission window. Core's rollback EFFECT timeout
   * still starts at actual dispatch; an expired apply deadline is not reused. */
  if(now>UINT64_MAX-s->config.rollback_timeout_ns)return CAMERA_NTX_EXHAUSTED;
  deadline=now+s->config.rollback_timeout_ns;
 }else return CAMERA_NETWORK_NOT_READY;
 if(!admit(s,&q,effect,profile,generation))return CAMERA_NTX_PROFILE_BLOCKED;
 r=clock_now(s,&now);if(r)return r;
 r=dispatch_recheck(s,&q,deadline,now);if(r)return r;
 m=s->image;i=find(&m,s->runtime_token);if(i<0)return CAMERA_NTX_INVALID;
 record_core(&m.receipts[i],&q,phase);r=append(s,&m);if(r)return r;
 r=clock_now(s,&now);if(r)return r;
 if(!admit(s,&q,effect,profile,generation)){
  s->blocked=1;return CAMERA_NTX_PROFILE_BLOCKED;
 }
 /* Admission is pure, not instantaneous. Sample AFTER its last callback, then
  * verify the exact fenced tuple/state/deadline before core next can consume it. */
 r=clock_now(s,&now);if(r)return r;
 r=dispatch_recheck(s,&q,deadline,now);if(r)return r;
 r=camera_network_next(&s->core,now,out);
 if(r){s->blocked=1;return CAMERA_NTX_RECONCILE;}
 s->live_work=*out;s->live_present=1;return CAMERA_NTX_OK;
}
int camera_network_transaction_link(struct camera_network_transaction_store *s,const struct camera_network_link *p)
{
 uint64_t now;int r;
 if(!ready(s)||!s->bound||!p)return CAMERA_NTX_INVALID;
 /* Core validates stale identities before ticking; use a fresh source timestamp
  * without advancing wrapper time on denied callbacks. */
 if(s->config.clock_ns(s->config.clock_context,&now)){s->blocked=1;return CAMERA_NETWORK_CLOCK;}
 r=camera_network_link_proof(&s->core,p,now);
 if(r==CAMERA_NETWORK_CLOCK)s->blocked=1;
 if(r==CAMERA_NETWORK_OK||r==CAMERA_NETWORK_NOT_READY||r==CAMERA_NETWORK_INVALID)s->last_ns=now;
 return r;
}
int camera_network_transaction_complete(struct camera_network_transaction_store *s,
 const struct camera_network_work *w,uint32_t success,const struct camera_network_link *p)
{
 uint64_t now;int r;
 if(!ready(s)||!s->bound||!w)return CAMERA_NTX_INVALID;
 if(s->config.clock_ns(s->config.clock_context,&now)){s->blocked=1;return CAMERA_NETWORK_CLOCK;}
 r=camera_network_complete(&s->core,w,success,p,now);
 if(r==CAMERA_NETWORK_CLOCK)s->blocked=1;
 if(r)return r;
 s->last_ns=now;s->live_present=0;memset(&s->live_work,0,sizeof(s->live_work));
 return save_core(s,CAMERA_NTX_WAIT);
}
int camera_network_transaction_cancel(struct camera_network_transaction_store *s,struct camera_network_token t)
{
 uint64_t now;int r=operation(s,t,&now);if(r)return r;
 r=camera_network_cancel(&s->core,t,now);if(r)return core_error(s,r);return save_core(s,CAMERA_NTX_WAIT);
}
int camera_network_transaction_tick(struct camera_network_transaction_store *s)
{
 uint64_t now;int r;
 if(!ready(s)||!s->bound)return CAMERA_NTX_RECONCILE;
 r=clock_now(s,&now);if(r)return r;r=camera_network_tick(&s->core,now);if(r)return core_error(s,r);
 if(!s->runtime_token.id)return CAMERA_NTX_OK;
 return save_core(s,CAMERA_NTX_WAIT);
}
int camera_network_transaction_confirm(struct camera_network_transaction_store *s,
 struct camera_network_token t,uint64_t expected)
{
 struct camera_network_report q;struct camera_network_link latest={0};
 struct camera_ntx_image m;uint64_t now;int r,i;
 if(!permitted(s))return CAMERA_NTX_RECONCILE;
 r=operation(s,t,&now);if(r)return r;r=camera_network_tick(&s->core,now);if(r)return core_error(s,r);
 r=camera_network_query(&s->core,t,&q);if(r)return r;
 if(q.state!=CAMERA_NETWORK_CONFIRM_READY||q.request.kind!=CAMERA_NETWORK_SWITCH)return CAMERA_NETWORK_ORDER;
 if(expected!=q.old_revision||expected!=s->image.committed_revision)return CAMERA_NETWORK_CONFLICT;
 if(expected==UINT64_MAX||s->settings_revision>UINT64_MAX-2||s->settings_operation_id>UINT64_MAX-2)
  return CAMERA_NTX_EXHAUSTED;
 i=find(&s->image,t);if(i<0)return CAMERA_NETWORK_STALE;
 m=s->image;m.committed_profile_id=q.request.config_id;m.committed_revision=expected+1;
 record_core(&m.receipts[i],&q,CAMERA_NTX_COMMIT_FENCE);
 r=append(s,&m);if(r)return r;
 if(s->config.latest_link(s->config.latest_context,t,q.request.config_id,q.apply_generation,&latest)){
  s->blocked=1;return CAMERA_NTX_RECONCILE;
 }
 r=clock_now(s,&now);if(r)return r;
 r=camera_network_link_proof(&s->core,&latest,now);
 if(r){s->blocked=1;return CAMERA_NTX_RECONCILE;}
 r=camera_network_confirm(&s->core,t,expected,now);
 if(r){s->blocked=1;return CAMERA_NTX_RECONCILE;}
 return save_core(s,CAMERA_NTX_FINAL);
}
int camera_network_transaction_query(const struct camera_network_transaction_store *s,
 struct camera_network_token t,struct camera_ntx_report *out)
{
 int i;struct camera_network_report q;
 if(!ready(s)||!out)return CAMERA_NTX_INVALID;
 memset(out,0,sizeof(*out));
 i=find(&s->image,t);
 if(i<0){out->reconciliation_required=s->blocked||!s->bound;
  return out->reconciliation_required?CAMERA_NTX_RECONCILE:CAMERA_NETWORK_STALE;}
 out->receipt=s->image.receipts[i];out->settings_revision=s->settings_revision;
 out->runtime_bound=s->bound;out->reconciliation_required=s->blocked||!s->bound;
 out->last_settings_result=s->last_settings_result;
 if(s->bound&&equal(t,s->runtime_token)&&!camera_network_query(&s->core,t,&q)){
  out->in_flight=q.in_flight;out->volatile_state=q.state;
  if(terminal(q.state)&&(s->blocked||(out->receipt.phase_flags&255U)!=CAMERA_NTX_FINAL))
   out->volatile_state=out->receipt.state;
 }
 out->work_may_be_live=out->in_flight||
  ((!s->bound||s->uncertain)&&!terminal(out->receipt.state));
 if(s->bound&&!s->blocked&&terminal(out->receipt.state))out->published_state=out->receipt.state;
 return CAMERA_NTX_OK;
}
int camera_network_transaction_live(const struct camera_network_transaction_store *s,struct camera_network_work *out)
{
 if(!ready(s)||!out)return CAMERA_NTX_INVALID;
 memset(out,0,sizeof(*out));
 if(!s->live_present)return CAMERA_NETWORK_NOT_READY;
 *out=s->live_work;return CAMERA_NTX_OK;
}
