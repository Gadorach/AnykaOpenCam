/* SPDX-License-Identifier: GPL-2.0-only */
#include "candidate_auth.h"
#include <mbedtls/platform_util.h>
#include <string.h>

static void inc(uint64_t *v){if(*v!=UINT64_MAX)(*v)++;}
static int id(const char *s)
{
 size_t i,j;if(!s||!s[0])return 0;
 for(i=0;i<CAMERA_ID_BYTES;i++) {
  unsigned char c=(unsigned char)s[i];
  if(!c){for(j=i;j<CAMERA_ID_BYTES;j++)if(s[j])return 0;return 1;}
  if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))return 0;
 }
 return 0;
}
static int ready(const struct candidate_backend *b)
{return b&&b->initialized&&!b->failed&&b->core.enabled&&!b->core.clock_failed&&!b->core.exhausted;}
static int account_put(struct candidate_backend *b)
{
 struct camera_access_account a={0};a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);
 a.domain=CAMERA_ACCESS_RTSP;a.roles=CAMERA_ACCESS_VIEWER;a.credential_generation=b->core.generation;
 memcpy(a.account_id,b->config->account_id,CAMERA_ID_BYTES);a.grant_count=1;
 a.grants[0].kind=CAMERA_ACCESS_PROFILE;a.grants[0].operations=CAMERA_ACCESS_LIVE;
 memcpy(a.grants[0].resource_id,b->config->profile_id,CAMERA_ID_BYTES);
 return camera_access_put_account(&b->policy,&a);
}
static int provision(struct candidate_backend *b,const struct candidate_credential *c,int first)
{
 struct camera_digest_span user,realm;int r;
 if(!c||c->domain!=CAMERA_ACCESS_RTSP||c->algorithm!=b->config->algorithm||!c->account||
    !c->username_bytes||c->username_bytes>64||!c->realm_bytes||c->realm_bytes>64||
    c->ha1_bytes!=(c->algorithm==CAMERA_DIGEST_SHA256?32U:16U))return CAMERA_INVALID;
 if(!first&&(c->account!=b->account_number||c->username_bytes!=b->core.username_length||
    c->realm_bytes!=b->core.realm_length||memcmp(c->username,b->core.username,c->username_bytes)||memcmp(c->realm,b->core.realm,c->realm_bytes)))return CAMERA_INVALID;
 user=(struct camera_digest_span){c->username,c->username_bytes};realm=(struct camera_digest_span){c->realm,c->realm_bytes};
 if(first) {
  r=camera_rtsp_digest_init(&b->core,c->algorithm,realm,b->config->nonce_ttl_ms,b->policy.last_ns/1000000U,b->config->random_ready,b->config->context);
  if(r)return r;
 }
 r=camera_rtsp_digest_provision(&b->core,c->account,user,c->ha1,c->ha1_bytes,1);if(r)return r;
 b->account_number=c->account;
 r=account_put(b);if(r)b->failed=1;
 return r;
}
int candidate_backend_init(struct candidate_backend *b,const struct candidate_config *cfg,uint64_t now)
{
 struct camera_access_resource resources[3];struct candidate_credential credential;unsigned char probe[32];
 const struct camera_profile *profile=NULL;size_t i;int r=CAMERA_INVALID;
 if(!b)return CAMERA_INVALID;
 mbedtls_platform_zeroize(b,sizeof(*b));memset(resources,0,sizeof(resources));memset(&credential,0,sizeof(credential));memset(probe,0,sizeof(probe));
 if(!cfg||cfg->abi!=CANDIDATE_AUTH_ABI||cfg->struct_bytes!=sizeof(*cfg)||cfg->host_fixture_only!=1||now==UINT64_MAX||
    !cfg->boot_generation||!id(cfg->device_id)||!id(cfg->account_id)||!id(cfg->profile_id)||
    (cfg->algorithm!=CAMERA_DIGEST_SHA256&&cfg->algorithm!=CAMERA_DIGEST_MD5_LEGACY)||!cfg->bootstrap_rtsp||!cfg->random_ready||!cfg->source_ready||!cfg->retain_uncertain||
    cfg->begin_contract!=CANDIDATE_BEGIN_LOGICAL_ONLY||!cfg->source_guard_bind||!cfg->retain_frame_uncertain||
    !cfg->nonce_ttl_ms||cfg->nonce_ttl_ms>CAMERA_DIGEST_TTL_MAX_MS||cfg->host_send_chunk>1404||!cfg->graph||
    cfg->graph->device_count>16||cfg->graph->profile_count>8||cfg->graph->association_count>16||cfg->graph->group_count>8||camera_graph_validate(cfg->graph))goto done;
 for(i=0;i<cfg->graph->profile_count;i++)if(!memcmp(cfg->graph->profiles[i].id,cfg->profile_id,CAMERA_ID_BYTES))profile=&cfg->graph->profiles[i];
 if(!profile||profile->codec!=CAMERA_H264||!id(profile->camera_id))goto done;
 b->config=cfg;b->next_connection=b->next_session=1;
 resources[0].kind=CAMERA_ACCESS_DEVICE;memcpy(resources[0].id,cfg->device_id,CAMERA_ID_BYTES);
 resources[1].kind=CAMERA_ACCESS_SENSOR;memcpy(resources[1].id,profile->camera_id,CAMERA_ID_BYTES);
 resources[2].kind=CAMERA_ACCESS_PROFILE;memcpy(resources[2].id,cfg->profile_id,CAMERA_ID_BYTES);memcpy(resources[2].sensor_id,profile->camera_id,CAMERA_ID_BYTES);
 if(camera_access_init(&b->policy,cfg->device_id,cfg->boot_generation,now,resources,3))goto done;
 if(camera_digest_wire_limit_init(&b->global,now/1000000U,cfg->rate_window_ms,cfg->global_verifies,cfg->global_challenges)||
    camera_digest_wire_limit_init(&b->account,now/1000000U,cfg->rate_window_ms,cfg->account_verifies,cfg->account_challenges)||
    !cfg->peer_verifies||cfg->peer_verifies>1024||!cfg->peer_challenges||cfg->peer_challenges>1024)goto done;
 /* Bootstrap callback is the only credential ingress; no product defaults. */
 if(cfg->bootstrap_rtsp(cfg->context,&credential)||cfg->random_ready(cfg->context,probe,sizeof(probe)))goto done;
 r=provision(b,&credential,1);if(r)goto done;
 r=camera_digest_wire_adapter_init(&b->wire,&b->core,&b->global,&b->account);if(r)goto done;
 b->query.abi=CAMERA_ACCESS_ABI;b->query.struct_bytes=sizeof(b->query);b->query.domain=CAMERA_ACCESS_RTSP;
 b->query.kind=CAMERA_ACCESS_PROFILE;b->query.operation=CAMERA_ACCESS_LIVE;
 memcpy(b->query.device_id,cfg->device_id,CAMERA_ID_BYTES);memcpy(b->query.sensor_id,profile->camera_id,CAMERA_ID_BYTES);memcpy(b->query.profile_id,cfg->profile_id,CAMERA_ID_BYTES);
 b->initialized=1;r=CAMERA_OK;
 done:
 mbedtls_platform_zeroize(&credential,sizeof(credential));mbedtls_platform_zeroize(probe,sizeof(probe));
 if(r!=CAMERA_OK){b->failed=1;camera_rtsp_digest_destroy(&b->core);}return r;
}
void candidate_backend_destroy(struct candidate_backend *b)
{if(b){camera_rtsp_digest_destroy(&b->core);camera_digest_wire_adapter_clear(&b->wire);mbedtls_platform_zeroize(b,sizeof(*b));}}
int candidate_backend_poll(struct candidate_backend *b,uint64_t now)
{
 if(!b||!b->initialized||b->failed||now==UINT64_MAX)return CAMERA_NOT_READY;
 if(b->config->owner_poll&&b->config->owner_poll(b->config->context,b,now)){b->failed=1;return CAMERA_IO_ERROR;}
 return b->failed?CAMERA_IO_ERROR:CAMERA_OK;
}
int candidate_peer_open(struct candidate_backend *b,struct candidate_peer *p,unsigned slot,uint64_t now)
{
 unsigned char random[32];static const char hex[]="0123456789abcdef";size_t i;int r;
 if(!ready(b)||!p||slot>=CAMERA_HUB_CONSUMERS||b->peers[slot]||now==UINT64_MAX||b->next_connection==UINT64_MAX)return CAMERA_NOT_READY;
 memset(p,0,sizeof(*p));
 r=camera_digest_wire_limit_init(&p->rate,now/1000000U,b->config->rate_window_ms,b->config->peer_verifies,b->config->peer_challenges);if(r)return r;
 /* Connection admission also bounds reconnect / malformed outer-parser work.
  * Global/account buckets never reset when a client slot is reused. */
 r=camera_digest_wire_admit(&b->global,&b->account,&p->rate,CAMERA_DIGEST_WIRE_VERIFY,now/1000000U);if(r)return r;
 memset(random,0,sizeof(random));r=b->config->random_ready(b->config->context,random,sizeof(random));
 if(r){mbedtls_platform_zeroize(random,sizeof(random));return CAMERA_NOT_READY;}
 for(i=0;i<32;i++){p->session[2*i]=hex[random[i]>>4];p->session[2*i+1]=hex[random[i]&15];}
 mbedtls_platform_zeroize(random,sizeof(random));p->connection=b->next_connection++;p->live=1;b->peers[slot]=p;inc(&b->stats.opened);return CAMERA_OK;
}
void candidate_peer_close(struct candidate_backend *b,struct candidate_peer *p)
{
 unsigned i;if(!b||!p||!p->live)return;
 camera_digest_wire_close(&b->wire,p->connection);
 if(p->principal_valid)camera_access_revoke_session(&b->policy,&p->principal);
 for(i=0;i<CAMERA_HUB_CONSUMERS;i++)if(b->peers[i]==p)b->peers[i]=NULL;
 inc(&b->stats.closed);mbedtls_platform_zeroize(p,sizeof(*p));
}
int candidate_effect_final(struct candidate_backend *b,struct candidate_peer *p,uint64_t now)
{
 int r=CAMERA_ACCESS_DENIED;
 if(ready(b)&&p&&p->live&&!p->canceled&&p->principal_valid&&now!=UINT64_MAX&&
    camera_rtsp_digest_check(&b->core,p->connection,1,now/1000000U,&p->grant)==0&&
    camera_access_check(&b->policy,&p->principal,&b->query,now)==0)r=CAMERA_OK;
 if(r&&b)inc(&b->stats.effect_denied);
 return r;
}
int candidate_effect(struct candidate_backend *b,struct candidate_peer *p,uint64_t now)
{
 int r=candidate_effect_final(b,p,now);
 if(!r&&b->config->source_ready(b->config->context,p->connection,now))r=CAMERA_NOT_READY;
 return r;
}
int candidate_request_verify(struct candidate_backend *b,struct candidate_peer *p,const struct camera_rtsp_request *request,uint64_t now,enum camera_digest_wire_reason *reason)
{
 struct camera_digest_grant grant;struct camera_access_verified fact={0};int r;
 if(!reason)return CAMERA_INVALID;
 *reason=CAMERA_DIGEST_WIRE_NO_CHALLENGE;
 if(!ready(b)||!p||!p->live||p->canceled||now==UINT64_MAX)return CAMERA_NOT_READY;
 r=camera_digest_wire_verify(&b->wire,&p->rate,p->connection,1,now/1000000U,request,&grant,reason);
 if(r){inc(&b->stats.auth_denied);return r;}
 /* No typed verified flag, URI/user-provided identity or nonce note authorizes.
  * Canonical identity, boot, resource and roles come only from trusted backend. */
 if(grant.account!=b->account_number||grant.connection!=p->connection||grant.scope!=1||grant.generation!=b->core.generation||
    grant.expires_ms>UINT64_MAX/1000000U||camera_rtsp_digest_check(&b->core,p->connection,1,now/1000000U,&grant))return CAMERA_ACCESS_DENIED;
 if(!p->principal_valid||p->grant.serial!=grant.serial||p->grant.generation!=grant.generation) {
  if(b->next_session==UINT64_MAX)return CAMERA_DIGEST_EXHAUSTED;
  if(p->principal_valid)camera_access_revoke_session(&b->policy,&p->principal);
  p->principal_valid=0;
  fact.abi=CAMERA_ACCESS_ABI;fact.struct_bytes=sizeof(fact);fact.verified=1;fact.domain=CAMERA_ACCESS_RTSP;
  memcpy(fact.device_id,b->config->device_id,CAMERA_ID_BYTES);memcpy(fact.account_id,b->config->account_id,CAMERA_ID_BYTES);
  fact.boot_generation=b->config->boot_generation;fact.credential_generation=grant.generation;fact.session_generation=b->next_session++;
  fact.issued_ns=now;fact.expires_ns=grant.expires_ms*1000000U;
  r=camera_access_issue(&b->policy,&fact,now,&p->principal);
  if(r){inc(&b->stats.policy_denied);return r;}p->principal_valid=1;
 }
 p->grant=grant;r=candidate_effect(b,p,now);if(!r)inc(&b->stats.verified);return r;
}
int candidate_challenge(struct candidate_backend *b,struct candidate_peer *p,uint32_t cseq,uint64_t now,enum camera_digest_wire_reason reason,unsigned char out[512],size_t *bytes)
{
 int r;if(!ready(b)||!p||!p->live||p->canceled||now==UINT64_MAX)return CAMERA_NOT_READY;
 r=camera_digest_wire_challenge(&b->wire,&p->rate,p->connection,1,now/1000000U,cseq,reason,out,512,bytes);
 if(!r)inc(&b->stats.challenges);
 return r;
}
int candidate_backend_rotate(struct candidate_backend *b,const struct candidate_credential *c)
{if(!b||!b->initialized||b->failed)return CAMERA_NOT_READY;return provision(b,c,0);}
int candidate_backend_revoke(struct candidate_backend *b)
{
 int r,s;if(!b||!b->initialized||b->failed)return CAMERA_NOT_READY;
 r=camera_rtsp_digest_invalidate(&b->core);s=camera_access_revoke_account(&b->policy,CAMERA_ACCESS_RTSP,b->config->account_id);
 if(r||s)b->failed=1;
 return r?r:s;
}
int candidate_backend_cancel(struct candidate_backend *b,uint64_t connection)
{
 unsigned i,found=0;if(!b||!b->initialized)return CAMERA_NOT_READY;
 for(i=0;i<CAMERA_HUB_CONSUMERS;i++)if(b->peers[i]&&(!connection||b->peers[i]->connection==connection)){b->peers[i]->canceled=1;found=1;}
 return found?CAMERA_OK:CAMERA_NOT_READY;
}
