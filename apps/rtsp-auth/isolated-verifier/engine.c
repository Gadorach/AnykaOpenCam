/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "verifier.h"
#include <camera/rtsp_digest_wire.h>
#include <camera/rtsp_input.h>
#include <camera/access_policy.h>
#include <string.h>
#include <sys/random.h>
struct peer {
 uint64_t connection,token;unsigned live,principal_valid;
 struct camera_digest_wire_limit rate;struct camera_digest_grant grant;
 struct camera_access_principal principal;
};
struct rv_engine {
 struct rv_material material;
 struct camera_rtsp_digest core;struct camera_digest_wire_adapter wire;
 struct camera_digest_wire_limit global,account;
 struct camera_access_policy policy;struct camera_access_request query;struct camera_rtsp_registry registry;
 struct peer peers[RV_PEERS];uint64_t last_ns,last_sequence,next_connection,next_token,next_session;
 uint64_t pending_sequence,pending_connection,pending_token;unsigned pending,failed;
};
size_t rv_engine_bytes(void){return sizeof(struct rv_engine);}
static int id_ok(const char *p)
{
 size_t i,j;if(!p||!p[0])return 0;
 for(i=0;i<64;i++){
  unsigned char c=(unsigned char)p[i];if(!c){for(j=i;j<64;j++)if(p[j])return 0;return 1;}
  if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;
 }return 0;
}
static int rng(void *ctx,unsigned char *out,size_t n)
{(void)ctx;return getrandom(out,n,0)==(ssize_t)n?0:-1;}
static int tick(struct rv_engine *e,uint64_t *now)
{if(e->failed||rv_clock(now)||*now<e->last_ns){e->failed=1;return RV_LOST;}e->last_ns=*now;return RV_OK;}
static struct peer *find(struct rv_engine *e,uint64_t conn)
{unsigned i;for(i=0;i<RV_PEERS;i++)if(e->peers[i].live&&e->peers[i].connection==conn)return &e->peers[i];return NULL;}
static void close_peer(struct rv_engine *e,struct peer *p)
{
 if(!p)return;
 camera_digest_wire_close(&e->wire,p->connection);
 if(p->principal_valid)camera_access_revoke_session(&e->policy,&p->principal);
 rv_wipe(p,sizeof(*p));
}
int rv_engine_init(void *storage,size_t capacity,const struct rv_material *m)
{
 struct rv_engine *e=storage;struct camera_access_resource resources[3]={{0}};struct camera_access_account a={0};uint64_t now;unsigned char entropy[32];
 if(!e||capacity<sizeof(*e))return RV_BAD;
 memset(e,0,sizeof(*e));
 if(!m||m->abi!=RV_ABI||m->bytes!=sizeof(*m)||m->domain!=CAMERA_ACCESS_RTSP||!m->epoch||!m->boot_generation||!m->credential_generation||!m->account||
    (m->algorithm!=CAMERA_DIGEST_SHA256&&m->algorithm!=CAMERA_DIGEST_MD5_LEGACY)||!m->username_bytes||m->username_bytes>64||!m->realm_bytes||m->realm_bytes>64||
    m->ha1_bytes!=(m->algorithm==CAMERA_DIGEST_SHA256?32U:16U)||!id_ok(m->device)||!id_ok(m->account_id)||!id_ok(m->sensor)||!id_ok(m->profile)||
    !memchr(m->live,0,sizeof(m->live))||!memchr(m->track,0,sizeof(m->track))||rv_clock(&now)||rng(NULL,entropy,sizeof(entropy)))return RV_BAD;
 rv_wipe(entropy,sizeof(entropy));e->material=*m;e->last_ns=now;e->next_connection=e->next_token=e->next_session=1;
 if(camera_rtsp_registry_init(&e->registry,m->live,m->track)||
    camera_rtsp_digest_init(&e->core,m->algorithm,(struct camera_digest_span){m->realm,m->realm_bytes},m->nonce_ttl_ms,now/1000000U,rng,NULL)||
    camera_rtsp_digest_provision(&e->core,m->account,(struct camera_digest_span){m->username,m->username_bytes},m->ha1,m->ha1_bytes,1)||
    camera_digest_wire_limit_init(&e->global,now/1000000U,m->window_ms,m->global_verifies,m->global_challenges)||
    camera_digest_wire_limit_init(&e->account,now/1000000U,m->window_ms,m->account_verifies,m->account_challenges)||
    !m->peer_verifies||m->peer_verifies>1024||!m->peer_challenges||m->peer_challenges>1024||
    camera_digest_wire_adapter_init(&e->wire,&e->core,&e->global,&e->account))goto bad;
 resources[0].kind=CAMERA_ACCESS_DEVICE;memcpy(resources[0].id,m->device,64);
 resources[1].kind=CAMERA_ACCESS_SENSOR;memcpy(resources[1].id,m->sensor,64);
 resources[2].kind=CAMERA_ACCESS_PROFILE;memcpy(resources[2].id,m->profile,64);memcpy(resources[2].sensor_id,m->sensor,64);
 if(camera_access_init(&e->policy,m->device,m->boot_generation,now,resources,3))goto bad;
 a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_RTSP;a.roles=CAMERA_ACCESS_VIEWER;a.credential_generation=m->credential_generation;
 memcpy(a.account_id,m->account_id,64);a.grant_count=1;a.grants[0].kind=CAMERA_ACCESS_PROFILE;a.grants[0].operations=CAMERA_ACCESS_LIVE;memcpy(a.grants[0].resource_id,m->profile,64);
 if(camera_access_put_account(&e->policy,&a))goto bad;
 e->query.abi=CAMERA_ACCESS_ABI;e->query.struct_bytes=sizeof(e->query);e->query.domain=CAMERA_ACCESS_RTSP;e->query.kind=CAMERA_ACCESS_PROFILE;e->query.operation=CAMERA_ACCESS_LIVE;
 memcpy(e->query.device_id,m->device,64);memcpy(e->query.sensor_id,m->sensor,64);memcpy(e->query.profile_id,m->profile,64);
 /* Keep only core HA1 copy; no duplicate startup material secret remains. */
 rv_wipe(e->material.ha1,sizeof(e->material.ha1));return RV_OK;
 bad:rv_engine_destroy(e,sizeof(*e));return RV_BAD;
}
static int current(struct rv_engine *e,struct peer *p,uint64_t now)
{
 return p&&p->live&&p->token&&p->principal_valid&&camera_rtsp_digest_check(&e->core,p->connection,1,now/1000000U,&p->grant)==0&&
  camera_access_check(&e->policy,&p->principal,&e->query,now)==0?RV_OK:RV_DENIED;
}
int rv_engine_command(void *storage,const struct rv_message *in,struct rv_message *out)
{
 struct rv_engine *e=storage;struct peer *p;uint64_t now;int r=RV_BAD;
 if(!e||!in||!out)return RV_BAD;
 memset(out,0,sizeof(*out));
 if(tick(e,&now)||in->epoch!=e->material.epoch||!in->sequence||e->last_sequence==UINT64_MAX||in->sequence!=e->last_sequence+1||in->scope!=1||in->status||in->flags||
    in->generation!=e->material.credential_generation||in->deadline<=now||in->deadline-now>5000000000ULL||in->bytes>RV_PAYLOAD)return RV_BAD;
 e->last_sequence=in->sequence;*out=*in;rv_wipe(out->payload,sizeof(out->payload));out->bytes=0;out->kind=RV_ACK;out->status=RV_BAD;
 p=find(e,in->connection);
 if(in->kind==RV_RETIRE_RESULT){
  if(!e->pending||in->bytes!=8||rv_get64(in->payload)!=e->pending_sequence||in->connection!=e->pending_connection||in->token>1)return RV_BAD;
  p=find(e,e->pending_connection);
  if(in->token&&p){if(p->principal_valid)camera_access_revoke_session(&e->policy,&p->principal);p->principal_valid=0;p->token=0;}
  e->pending=0;e->pending_sequence=e->pending_connection=e->pending_token=0;out->status=0;return RV_OK;
 }
 if(e->pending)return RV_BUSY;
 if(in->kind==RV_OPEN){
  if(in->connection||in->token||in->bytes||e->next_connection==UINT64_MAX)return RV_BAD;
  for(unsigned i=0;i<RV_PEERS;i++)if(!e->peers[i].live){p=&e->peers[i];break;}
  if(!p)return RV_BUSY;
  if(camera_digest_wire_limit_init(&p->rate,now/1000000U,e->material.window_ms,e->material.peer_verifies,e->material.peer_challenges))return RV_BAD;
  r=camera_digest_wire_admit(&e->global,&e->account,&p->rate,CAMERA_DIGEST_WIRE_VERIFY,now/1000000U);
  if(!r){p->connection=e->next_connection++;p->live=1;out->connection=p->connection;}else rv_wipe(p,sizeof(*p));
 }else if(in->kind==RV_REVOKE){
  if(in->connection||in->token||in->bytes||e->material.credential_generation==UINT64_MAX)return RV_BAD;
  r=camera_rtsp_digest_invalidate(&e->core);
  if(!r)r=camera_access_revoke_account(&e->policy,CAMERA_ACCESS_RTSP,e->material.account_id);
  for(unsigned i=0;i<RV_PEERS;i++){e->peers[i].token=0;e->peers[i].principal_valid=0;}
  if(!r){e->material.credential_generation++;out->generation=e->material.credential_generation;}
 }else if(in->kind==RV_SHUTDOWN){if(in->connection||in->token||in->bytes)return RV_BAD;r=0;
 }else if(!p)return RV_BAD;
 else if(in->kind==RV_CLOSE){if(in->token||in->bytes)return RV_BAD;close_peer(e,p);r=0;
 }else if(in->kind==RV_CHALLENGE){
  enum camera_digest_wire_reason reason;size_t bytes;
  if(in->bytes!=8||in->token)return RV_BAD;
  reason=(enum camera_digest_wire_reason)rv_get32(in->payload+4);
  /* Parent may request only fresh challenge here; expiry hint belongs child. */
  if(reason!=CAMERA_DIGEST_WIRE_INITIAL)return RV_BAD;
  r=camera_digest_wire_challenge(&e->wire,&p->rate,p->connection,1,now/1000000U,rv_get32(in->payload),reason,out->payload,512,&bytes);
  if(!r)out->bytes=(uint32_t)bytes;
 }else if(in->kind==RV_VERIFY){
  struct camera_rtsp_request request;struct camera_digest_grant grant;enum camera_digest_wire_reason reason;struct camera_access_verified fact={0};
  /* Bound even malformed outer Request-Line work BEFORE parsing. Wire/core
   * additionally charge the crypto attempt; both histories remain child-owned. */
  r=camera_digest_wire_admit(&e->global,&e->account,&p->rate,CAMERA_DIGEST_WIRE_VERIFY,now/1000000U);
  if(r)goto verify_done;
  if(!in->bytes||in->token||camera_rtsp_request_parse(in->payload,in->bytes,&request)!=CAMERA_RTSP_READY||request.consumed!=in->bytes){r=RV_BAD;goto verify_done;}
  if(!request.target_is_star&&
    !((request.target.length==strlen(e->registry.live_uri)&&!memcmp(request.target.data,e->registry.live_uri,request.target.length))||
      (request.target.length==strlen(e->registry.track_uri)&&!memcmp(request.target.data,e->registry.track_uri,request.target.length)))){r=RV_BAD;goto verify_done;}
  r=camera_digest_wire_verify(&e->wire,&p->rate,p->connection,1,now/1000000U,&request,&grant,&reason);
  if(!r){
   if(grant.generation!=e->core.generation||grant.expires_ms>UINT64_MAX/1000000U||e->next_session==UINT64_MAX||e->next_token==UINT64_MAX)r=RV_DENIED;
   else {
    if(p->principal_valid)camera_access_revoke_session(&e->policy,&p->principal);
    p->principal_valid=0;
    fact.abi=CAMERA_ACCESS_ABI;fact.struct_bytes=sizeof(fact);fact.verified=1;fact.domain=CAMERA_ACCESS_RTSP;
    memcpy(fact.device_id,e->material.device,64);memcpy(fact.account_id,e->material.account_id,64);fact.boot_generation=e->material.boot_generation;
    fact.credential_generation=e->material.credential_generation;fact.session_generation=e->next_session++;fact.issued_ns=now;fact.expires_ns=grant.expires_ms*1000000U;
    r=camera_access_issue(&e->policy,&fact,now,&p->principal);
    if(!r){p->principal_valid=1;p->grant=grant;p->token=e->next_token++;out->token=p->token;}
   }
  }
  verify_done:
  if(tick(e,&now)||now>=in->deadline||(!r&&current(e,p,now)))r=RV_DENIED;
  if(r&&p){if(p->principal_valid)camera_access_revoke_session(&e->policy,&p->principal);p->principal_valid=0;p->token=0;out->token=0;}
  out->kind=RV_RESULT;e->pending=1;e->pending_sequence=in->sequence;e->pending_connection=in->connection;e->pending_token=out->token;
 }else if(in->kind==RV_CHECK){
  if(in->bytes||!in->token||p->token!=in->token)return RV_BAD;
  r=current(e,p,now);if(!r)out->deadline=p->grant.expires_ms*1000000U;
 }
 out->status=r;return RV_OK;
}
void rv_engine_destroy(void *storage,size_t bytes)
{if(storage){struct rv_engine *e=storage;camera_rtsp_digest_destroy(&e->core);camera_digest_wire_adapter_clear(&e->wire);rv_wipe(storage,bytes);}}
