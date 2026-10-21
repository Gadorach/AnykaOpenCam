/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/access_policy.h>
#include <string.h>

#define POLICY_MAGIC 0x41504331U
#define ALL_ROLES ((1U<<9)-1U)
#define ALL_OPS ((1U<<13)-1U)

static int zero(const void *ptr,size_t bytes)
{
 const unsigned char *p=ptr;size_t i;
 for(i=0;i<bytes;i++)if(p[i])return 0;
 return 1;
}
static int id_valid(const char id[CAMERA_ID_BYTES])
{
 size_t i;
 if(!id||!id[0])return 0;
 for(i=0;i<CAMERA_ID_BYTES;i++){
  unsigned char c=(unsigned char)id[i];
  if(!c)return zero(id+i,CAMERA_ID_BYTES-i);
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||
       c=='-'||c=='_'||c=='.'))return 0;
 }
 return 0;
}
static int same_id(const char *a,const char *b){return !memcmp(a,b,CAMERA_ID_BYTES);}
static int domain_valid(uint32_t d){return d>=CAMERA_ACCESS_UI&&d<=CAMERA_ACCESS_SETUP;}
static int ready(const struct camera_access_policy *p)
{
 return p&&p->initialized==POLICY_MAGIC&&!p->clock_failed;
}
static int observe(struct camera_access_policy *p,uint64_t now)
{
 if(!ready(p))return CAMERA_ACCESS_DENIED;
 if(now<p->last_ns){p->clock_failed=1;return CAMERA_ACCESS_DENIED;}
 p->last_ns=now;return CAMERA_ACCESS_OK;
}
static int resource_find(const struct camera_access_policy *p,uint32_t kind,const char *id)
{
 uint32_t i;
 for(i=0;i<p->resource_count;i++)
  if(p->resources[i].kind==kind&&same_id(p->resources[i].id,id))return (int)i;
 return -1;
}
static int record_find(const struct camera_access_policy *p,uint32_t domain,const char *id)
{
 uint32_t i;
 for(i=0;i<CAMERA_ACCESS_MAX_ACCOUNTS;i++)
  if(p->records[i].used&&p->records[i].account.domain==domain&&
     same_id(p->records[i].account.account_id,id))return (int)i;
 return -1;
}
static uint32_t role_for(uint32_t op)
{
 switch(op){
 case CAMERA_ACCESS_LIVE:case CAMERA_ACCESS_SNAPSHOT:case CAMERA_ACCESS_EVENTS:
  return CAMERA_ACCESS_VIEWER;
 case CAMERA_ACCESS_PTZ_CONTROL:case CAMERA_ACCESS_IMAGING_CONFIG:return CAMERA_ACCESS_OPERATOR;
 case CAMERA_ACCESS_DEVICE_CONFIG:return CAMERA_ACCESS_CONFIGURATOR;
 case CAMERA_ACCESS_RECORDING_CONFIG:return CAMERA_ACCESS_RECORDING_MANAGER;
 case CAMERA_ACCESS_PLAYBACK:case CAMERA_ACCESS_DOWNLOAD:return CAMERA_ACCESS_RECORDING_READER;
 case CAMERA_ACCESS_ONBOARD:return CAMERA_ACCESS_ONBOARDER;
 case CAMERA_ACCESS_SETTINGS_RESET_REQUEST:return CAMERA_ACCESS_RESET_REQUESTER;
 case CAMERA_ACCESS_CREDENTIAL_CONFIG:return CAMERA_ACCESS_CREDENTIAL_MANAGER;
 case CAMERA_ACCESS_DISCOVERY:return CAMERA_ACCESS_DISCOVERER;
 default:return 0;
 }
}
static uint32_t domain_ops(uint32_t domain)
{
 const uint32_t viewing=CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT|CAMERA_ACCESS_EVENTS;
 const uint32_t control=CAMERA_ACCESS_PTZ_CONTROL|CAMERA_ACCESS_IMAGING_CONFIG;
 switch(domain){
 case CAMERA_ACCESS_UI:return viewing|control|CAMERA_ACCESS_PLAYBACK|CAMERA_ACCESS_DOWNLOAD;
 case CAMERA_ACCESS_ADMIN:return viewing|control|CAMERA_ACCESS_DEVICE_CONFIG|
  CAMERA_ACCESS_RECORDING_CONFIG|CAMERA_ACCESS_PLAYBACK|CAMERA_ACCESS_DOWNLOAD|
  CAMERA_ACCESS_SETTINGS_RESET_REQUEST|CAMERA_ACCESS_CREDENTIAL_CONFIG;
 case CAMERA_ACCESS_RTSP:return CAMERA_ACCESS_LIVE;
 case CAMERA_ACCESS_ONVIF:return viewing|CAMERA_ACCESS_PTZ_CONTROL|CAMERA_ACCESS_DISCOVERY;
 case CAMERA_ACCESS_SETUP:return CAMERA_ACCESS_ONBOARD;
 default:return 0;
 }
}
static uint32_t scope_ops(uint32_t kind)
{
 switch(kind){
 case CAMERA_ACCESS_DEVICE:return CAMERA_ACCESS_DEVICE_CONFIG|CAMERA_ACCESS_ONBOARD|
  CAMERA_ACCESS_SETTINGS_RESET_REQUEST|CAMERA_ACCESS_CREDENTIAL_CONFIG|CAMERA_ACCESS_DISCOVERY;
 case CAMERA_ACCESS_SENSOR:return CAMERA_ACCESS_EVENTS|CAMERA_ACCESS_IMAGING_CONFIG|
  CAMERA_ACCESS_RECORDING_CONFIG|CAMERA_ACCESS_PLAYBACK|CAMERA_ACCESS_DOWNLOAD;
 case CAMERA_ACCESS_PROFILE:return CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT;
 case CAMERA_ACCESS_PTZ:return CAMERA_ACCESS_PTZ_CONTROL;
 default:return 0;
 }
}
static uint32_t roles_for_ops(uint32_t ops)
{
 uint32_t bit,roles=0;
 for(bit=1;bit<=CAMERA_ACCESS_DISCOVERY;bit<<=1)if(ops&bit)roles|=role_for(bit);
 return roles;
}
static int account_valid(const struct camera_access_policy *p,const struct camera_access_account *a)
{
 uint32_t i,j,allowed_roles;
 if(!a||a->abi!=CAMERA_ACCESS_ABI||a->struct_bytes!=sizeof(*a)||a->reserved||
    !domain_valid(a->domain)||!id_valid(a->account_id)||!a->credential_generation||
    !a->roles||(a->roles&~ALL_ROLES)||!a->grant_count||a->grant_count>CAMERA_ACCESS_MAX_GRANTS)
  return 0;
 allowed_roles=roles_for_ops(domain_ops(a->domain));
 if(a->roles&~allowed_roles)return 0;
 for(i=0;i<a->grant_count;i++){
  const struct camera_access_grant *g=&a->grants[i];
  if(!id_valid(g->resource_id)||resource_find(p,g->kind,g->resource_id)<0||
     !g->operations||(g->operations&~ALL_OPS)||
     (g->operations&~domain_ops(a->domain))||(g->operations&~scope_ops(g->kind))||
     (roles_for_ops(g->operations)&~a->roles))return 0;
  for(j=0;j<i;j++)if(g->kind==a->grants[j].kind&&
     same_id(g->resource_id,a->grants[j].resource_id))return 0;
 }
 /* Unknown tail fields in unused grant slots are not tolerated. */
 for(i=a->grant_count;i<CAMERA_ACCESS_MAX_GRANTS;i++)
  if(a->grants[i].kind||a->grants[i].operations||
     !zero(a->grants[i].resource_id,CAMERA_ID_BYTES))return 0;
 return 1;
}
int camera_access_init(struct camera_access_policy *p,const char device_id[CAMERA_ID_BYTES],
 uint64_t boot,uint64_t now,const struct camera_access_resource *r,uint32_t count)
{
 uint32_t i,j,devices=0;
 if(!p)return CAMERA_ACCESS_INVALID;
 memset(p,0,sizeof(*p));
 if(!id_valid(device_id)||!boot||!r||!count||count>CAMERA_ACCESS_MAX_RESOURCES)
  return CAMERA_ACCESS_INVALID;
 for(i=0;i<count;i++){
  if(!scope_ops(r[i].kind)||!id_valid(r[i].id))return CAMERA_ACCESS_INVALID;
  if(r[i].kind==CAMERA_ACCESS_PROFILE){
   if(!id_valid(r[i].sensor_id))return CAMERA_ACCESS_INVALID;
   for(j=0;j<count;j++)if(r[j].kind==CAMERA_ACCESS_SENSOR&&same_id(r[j].id,r[i].sensor_id))break;
   if(j==count)return CAMERA_ACCESS_INVALID;
  }else if(!zero(r[i].sensor_id,CAMERA_ID_BYTES))return CAMERA_ACCESS_INVALID;
  if(r[i].kind==CAMERA_ACCESS_DEVICE){
   if(!same_id(r[i].id,device_id))return CAMERA_ACCESS_INVALID;
   devices++;
  }
  /* No identifier aliases, including a physical sensor reused as a profile. */
  for(j=0;j<i;j++)if(same_id(r[j].id,r[i].id))return CAMERA_ACCESS_INVALID;
 }
 if(devices!=1)return CAMERA_ACCESS_INVALID;
 memcpy(p->device_id,device_id,CAMERA_ID_BYTES);p->boot_generation=boot;
 p->last_ns=now;p->next_serial=1;p->resource_count=count;
 memcpy(p->resources,r,count*sizeof(*r));p->initialized=POLICY_MAGIC;
 return CAMERA_ACCESS_OK;
}
/* All mutation preflights complete before this no-fail unlink/invalidate step. */
static int revision_available(const struct camera_access_policy *p,uint32_t i)
{
 uint32_t peer=p->records[i].nvr_peer;
 return p->records[i].revision<UINT64_MAX&&
  (!peer||p->records[peer-1].revision<UINT64_MAX);
}
static void invalidate(struct camera_access_policy *p,uint32_t i)
{
 uint32_t peer=p->records[i].nvr_peer;
 if(peer){
  p->records[peer-1].nvr_peer=0;p->records[peer-1].revision++;
 }
 p->records[i].nvr_peer=0;p->records[i].revision++;
}
int camera_access_put_account(struct camera_access_policy *p,const struct camera_access_account *a)
{
 int found;uint32_t i;
 if(!ready(p)||!account_valid(p,a))return CAMERA_ACCESS_INVALID;
 found=record_find(p,a->domain,a->account_id);
 if(found>=0){
  struct camera_access_record *r=&p->records[found];
  if(a->credential_generation<r->account.credential_generation||
     (!r->enabled&&a->credential_generation==r->account.credential_generation))
   return CAMERA_ACCESS_INVALID;
  i=(uint32_t)found;
 }else{
  for(i=0;i<CAMERA_ACCESS_MAX_ACCOUNTS;i++)if(!p->records[i].used)break;
  if(i==CAMERA_ACCESS_MAX_ACCOUNTS)return CAMERA_ACCESS_FULL;
 }
 if(!revision_available(p,i))return CAMERA_ACCESS_EXHAUSTED;
 invalidate(p,i);p->records[i].account=*a;
 p->records[i].used=1;p->records[i].enabled=1;return CAMERA_ACCESS_OK;
}
int camera_access_revoke_account(struct camera_access_policy *p,uint32_t domain,const char id[CAMERA_ID_BYTES])
{
 int i;
 if(!ready(p)||!domain_valid(domain)||!id_valid(id))return CAMERA_ACCESS_INVALID;
 i=record_find(p,domain,id);
 if(i<0)return CAMERA_ACCESS_DENIED;
 if(!revision_available(p,(uint32_t)i))return CAMERA_ACCESS_EXHAUSTED;
 invalidate(p,(uint32_t)i);p->records[i].enabled=0;return CAMERA_ACCESS_OK;
}
int camera_access_map_nvr(struct camera_access_policy *p,const char onvif[CAMERA_ID_BYTES],
 const char rtsp[CAMERA_ID_BYTES])
{
 int oi,ri;uint32_t i,j,has_live=0;struct camera_access_record *o,*r;
 if(!ready(p)||!id_valid(onvif)||!id_valid(rtsp))return CAMERA_ACCESS_INVALID;
 oi=record_find(p,CAMERA_ACCESS_ONVIF,onvif);ri=record_find(p,CAMERA_ACCESS_RTSP,rtsp);
 if(oi<0||ri<0)return CAMERA_ACCESS_DENIED;
 o=&p->records[oi];r=&p->records[ri];
 if(!o->enabled||!r->enabled||o->nvr_peer||r->nvr_peer||
    o->account.credential_generation!=r->account.credential_generation)return CAMERA_ACCESS_DENIED;
 for(i=0;i<o->account.grant_count;i++){
  const struct camera_access_grant *og=&o->account.grants[i];
  if(og->kind!=CAMERA_ACCESS_PROFILE)continue;
  if(og->operations&CAMERA_ACCESS_LIVE)has_live=1;
  for(j=0;j<r->account.grant_count;j++){
   const struct camera_access_grant *rg=&r->account.grants[j];
   if(rg->kind==CAMERA_ACCESS_PROFILE&&(rg->operations&CAMERA_ACCESS_LIVE)&&
      same_id(og->resource_id,rg->resource_id))break;
  }
  if(j==r->account.grant_count)return CAMERA_ACCESS_DENIED;
 }
 if(!has_live)return CAMERA_ACCESS_DENIED;
 if(!revision_available(p,(uint32_t)oi)||!revision_available(p,(uint32_t)ri))
  return CAMERA_ACCESS_EXHAUSTED;
 invalidate(p,(uint32_t)oi);invalidate(p,(uint32_t)ri);
 o->nvr_peer=(uint32_t)ri+1;r->nvr_peer=(uint32_t)oi+1;return CAMERA_ACCESS_OK;
}
static int fact_valid(const struct camera_access_policy *p,const struct camera_access_verified *f,uint64_t now)
{
 return f&&f->abi==CAMERA_ACCESS_ABI&&f->struct_bytes==sizeof(*f)&&f->verified==1&&
  !f->reserved&&domain_valid(f->domain)&&id_valid(f->device_id)&&id_valid(f->account_id)&&
  same_id(f->device_id,p->device_id)&&f->boot_generation==p->boot_generation&&
  f->credential_generation&&f->session_generation&&f->issued_ns<=now&&
  f->issued_ns<f->expires_ns&&now<f->expires_ns;
}
static int current(const struct camera_access_policy *p,const struct camera_access_principal *s,uint64_t now)
{
 int i;
 if(!fact_valid(p,&s->fact,now)||!s->snapshot_serial)return 0;
 i=record_find(p,s->fact.domain,s->fact.account_id);
 if(i<0||!p->records[i].enabled||p->records[i].revision!=s->policy_revision||
    p->records[i].account.credential_generation!=s->fact.credential_generation)return 0;
 if(s->fact.domain==CAMERA_ACCESS_ONVIF&&!p->records[i].nvr_peer)return 0;
 return 1;
}
static int snapshot_equal(const struct camera_access_principal *a,const struct camera_access_principal *b)
{
 const struct camera_access_verified *x=&a->fact,*y=&b->fact;
 return a->policy_revision==b->policy_revision&&a->snapshot_serial==b->snapshot_serial&&
  x->abi==y->abi&&x->struct_bytes==y->struct_bytes&&x->verified==y->verified&&x->domain==y->domain&&
  same_id(x->device_id,y->device_id)&&same_id(x->account_id,y->account_id)&&
  x->boot_generation==y->boot_generation&&x->credential_generation==y->credential_generation&&
  x->session_generation==y->session_generation&&x->issued_ns==y->issued_ns&&
  x->expires_ns==y->expires_ns&&x->reserved==y->reserved;
}
static int session_find(const struct camera_access_policy *p,const struct camera_access_principal *s)
{
 uint32_t i;
 for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)
  if(p->sessions[i].active&&snapshot_equal(s,&p->sessions[i].principal))return (int)i;
 return -1;
}
int camera_access_sweep(struct camera_access_policy *p,uint64_t now)
{
 uint32_t i;
 if(observe(p,now))return CAMERA_ACCESS_DENIED;
 for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)if(p->sessions[i].active&&
    !current(p,&p->sessions[i].principal,now))p->sessions[i].active=0;
 return CAMERA_ACCESS_OK;
}
int camera_access_issue(struct camera_access_policy *p,const struct camera_access_verified *f,
 uint64_t now,struct camera_access_principal *out)
{
 uint32_t i;int ri;struct camera_access_principal candidate;
 if(!out)return CAMERA_ACCESS_INVALID;
 /* Failure never modifies out: callers must branch on return, not reuse it. */
 if(observe(p,now)||!fact_valid(p,f,now))return CAMERA_ACCESS_DENIED;
 ri=record_find(p,f->domain,f->account_id);
 if(ri<0||!p->records[ri].enabled||
    p->records[ri].account.credential_generation!=f->credential_generation||
    f->session_generation<=p->records[ri].last_session_generation||
    (f->domain==CAMERA_ACCESS_ONVIF&&!p->records[ri].nvr_peer))return CAMERA_ACCESS_DENIED;
 camera_access_sweep(p,now);
 for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)if(p->sessions[i].active){
  const struct camera_access_verified *old=&p->sessions[i].principal.fact;
  if(old->domain==f->domain&&same_id(old->account_id,f->account_id)&&
     old->session_generation==f->session_generation)return CAMERA_ACCESS_DENIED;
 }
 for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)if(!p->sessions[i].active)break;
 if(i==CAMERA_ACCESS_MAX_SESSIONS)return CAMERA_ACCESS_FULL;
 /* Never wrap: serial disambiguates session-slot reuse within this boot. */
 if(p->next_serial==UINT64_MAX)return CAMERA_ACCESS_EXHAUSTED;
 memset(&candidate,0,sizeof(candidate));candidate.fact=*f;
 candidate.policy_revision=p->records[ri].revision;candidate.snapshot_serial=p->next_serial++;
 p->sessions[i].principal=candidate;p->sessions[i].active=1;*out=candidate;
 p->records[ri].last_session_generation=f->session_generation;
 return CAMERA_ACCESS_OK;
}
int camera_access_revoke_session(struct camera_access_policy *p,const struct camera_access_principal *s)
{
 int i;
 if(!ready(p)||!s)return CAMERA_ACCESS_INVALID;
 i=session_find(p,s);if(i<0)return CAMERA_ACCESS_DENIED;
 p->sessions[i].active=0;return CAMERA_ACCESS_OK;
}
static int request_resource(const struct camera_access_policy *p,const struct camera_access_request *q)
{
 int i;
 if(!q||q->abi!=CAMERA_ACCESS_ABI||q->struct_bytes!=sizeof(*q)||q->reserved||
    !domain_valid(q->domain)||!id_valid(q->device_id)||!same_id(q->device_id,p->device_id)||
    !q->operation||(q->operation&~ALL_OPS)||
    (q->operation&(q->operation-1))||!(q->operation&scope_ops(q->kind)))return -1;
 switch(q->kind){
 case CAMERA_ACCESS_DEVICE:
  if(!zero(q->sensor_id,CAMERA_ID_BYTES)||!zero(q->profile_id,CAMERA_ID_BYTES)||
     !zero(q->resource_id,CAMERA_ID_BYTES))return -1;
  return resource_find(p,q->kind,q->device_id);
 case CAMERA_ACCESS_SENSOR:
  if(!id_valid(q->sensor_id)||!zero(q->profile_id,CAMERA_ID_BYTES)||
     !zero(q->resource_id,CAMERA_ID_BYTES))return -1;
  return resource_find(p,q->kind,q->sensor_id);
 case CAMERA_ACCESS_PROFILE:
  if(!id_valid(q->sensor_id)||!id_valid(q->profile_id)||!zero(q->resource_id,CAMERA_ID_BYTES))return -1;
  i=resource_find(p,q->kind,q->profile_id);
  return i>=0&&same_id(p->resources[i].sensor_id,q->sensor_id)?i:-1;
 case CAMERA_ACCESS_PTZ:
  if(!id_valid(q->resource_id)||!zero(q->sensor_id,CAMERA_ID_BYTES)||
     !zero(q->profile_id,CAMERA_ID_BYTES))return -1;
  return resource_find(p,q->kind,q->resource_id);
 default:return -1;
 }
}
int camera_access_check(struct camera_access_policy *p,const struct camera_access_principal *s,
 const struct camera_access_request *q,uint64_t now)
{
 int resource,ri;uint32_t i;const struct camera_access_account *a;
 if(observe(p,now)||!s||!current(p,s,now)||session_find(p,s)<0)return CAMERA_ACCESS_DENIED;
 resource=request_resource(p,q);
 if(resource<0||q->domain!=s->fact.domain||!(q->operation&domain_ops(q->domain)))
  return CAMERA_ACCESS_DENIED;
 ri=record_find(p,s->fact.domain,s->fact.account_id);a=&p->records[ri].account;
 if(!(a->roles&role_for(q->operation)))return CAMERA_ACCESS_DENIED;
 for(i=0;i<a->grant_count;i++)if(a->grants[i].kind==q->kind&&
    same_id(a->grants[i].resource_id,p->resources[resource].id)&&
    (a->grants[i].operations&q->operation))return CAMERA_ACCESS_OK;
 return CAMERA_ACCESS_DENIED;
}
