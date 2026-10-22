/* SPDX-License-Identifier: GPL-2.0-only
 * Opaque child grant custody; no local Digest state, HA1, secret bootstrap or
 * manufactured camera_access_verified/principal. Child owns actual RTSP policy.*/
#define _GNU_SOURCE
#include "candidate_auth.h"
#include "adapter_state.h"
#include <camera/access_policy.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef IS_HOST_PUBLIC_ONLY
#include "host_pin.h"
#include "delay_pin.h"
/* Fixed HOST-only scalar stage: 1 domain denial, 2 generation denial,
 * 3 fully accepted prefix. No callback or caller-provided permission. */
void is_host_prefix_stage(unsigned,unsigned,unsigned);
#else
#include "native_pin.h"
#endif
struct routing_prefix {
 uint32_t abi,bytes,domain,algorithm;uint64_t epoch,boot_generation,credential_generation,account;
 uint32_t lengths_and_limits[11];char device[64],account_id[64],sensor[64],profile[64];
};
typedef char prefix_layout[offsetof(struct routing_prefix,profile)+64==offsetof(struct rv_material,username)?1:-1];
typedef char source_size[sizeof(struct rv_material)==1536?1:-1];
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Reviewed SOURCE ABI requires little endian"
#endif
static void inc(uint64_t *n){if(*n!=UINT64_MAX)(*n)++;}
static int id(const char *s)
{size_t i,j;if(!s||!s[0])return 0;for(i=0;i<64;i++){unsigned char c=s[i];if(!c){for(j=i;j<64;j++)if(s[j])return 0;return 1;}if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;}return 0;}
static int tick(struct candidate_backend *b,uint64_t now)
{if(!b||now==UINT64_MAX||now<b->last_ns){if(b)b->failed=1;return CAMERA_NOT_READY;}b->last_ns=now;return CAMERA_OK;}
static int ready(struct candidate_backend *b,uint64_t now)
{return b&&b->initialized&&!b->failed&&!b->revoked&&b->child&&!tick(b,now);}
static void latency(struct candidate_backend *b,uint64_t start)
{uint64_t end;if(rv_clock(&end)||tick(b,end)||end<start){b->failed=1;return;}uint64_t n=end-start;if(n>b->stats.IPC_max_ns)b->stats.IPC_max_ns=n;if(UINT64_MAX-b->stats.IPC_total_ns>=n)b->stats.IPC_total_ns+=n;}
static int candidate_backend_init(struct candidate_backend *b,const struct candidate_config *cfg,uint64_t now)
{
 struct routing_prefix prefix={0};unsigned char probe[32]={0};struct stat st;const struct camera_profile *profile=NULL;int result=CAMERA_INVALID;
 if(!b)return result;
  memset(b,0,sizeof(*b));
  b->startup_result=RV_BAD;
 if(!cfg||cfg->abi!=CANDIDATE_AUTH_ABI||cfg->struct_bytes!=sizeof(*cfg)||now==UINT64_MAX||!cfg->epoch||!cfg->boot_generation||!cfg->credential_generation||
    !id(cfg->device_id)||!id(cfg->account_id)||!id(cfg->profile_id)||!cfg->random_ready||!cfg->source_ready||!cfg->retain_uncertain||!cfg->retain_frame_uncertain||!cfg->retain_verifier||
    !cfg->source_guard_bind||cfg->begin_contract!=CANDIDATE_BEGIN_LOGICAL_ONLY||cfg->host_send_chunk>1404||!cfg->graph||
    cfg->graph->profile_count>8||cfg->graph->device_count>16||cfg->graph->association_count>16||cfg->graph->group_count>8||camera_graph_validate(cfg->graph)||
    !cfg->nonce_ttl_ms||cfg->nonce_ttl_ms>CAMERA_DIGEST_TTL_MAX_MS||!cfg->peer_verifies||cfg->peer_verifies>1024||!cfg->peer_challenges||cfg->peer_challenges>1024||
    (cfg->algorithm!=CAMERA_DIGEST_SHA256&&cfg->algorithm!=CAMERA_DIGEST_MD5_LEGACY))goto done;
 /*Native refusal BEFORE even nonsecret startup prefix. Never HOST fallback.*/
#ifdef IS_HOST_PUBLIC_ONLY
 if(cfg->host_fixture_only!=1||!getuid()||getuid()!=geteuid()||getgid()!=getegid()||cfg->image.profile!=RV_HOST_PUBLIC_4M||
    !((cfg->image.bytes==IS_HOST_BYTES&&!memcmp(cfg->image.sha256,is_host_sha256,32))||
      (cfg->image.bytes==IS_DELAY_BYTES&&!memcmp(cfg->image.sha256,is_delay_sha256,32))))goto done;
#else
 if(cfg->host_fixture_only||getuid()||geteuid()||getgid()||getegid()||getgroups(0,NULL)!=0||cfg->image.profile!=RV_NATIVE||
    cfg->image.bytes!=IS_NATIVE_BYTES||memcmp(cfg->image.sha256,is_native_sha256,32))goto done;
#endif
 if(cfg->image.abi!=RV_ABI||cfg->image.fd<0||cfg->image.fd==cfg->material_fd||fstat(cfg->image.fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||
    st.st_uid!=getuid()||(st.st_mode&07777)!=0555||st.st_size!=(off_t)cfg->image.bytes||(fcntl(cfg->image.fd,F_GETFL)&O_ACCMODE)!=O_RDONLY)goto done;
 for(size_t i=0;i<cfg->graph->profile_count;i++)if(!memcmp(cfg->graph->profiles[i].id,cfg->profile_id,64))profile=&cfg->graph->profiles[i];
 if(!profile||profile->codec!=CAMERA_H264||!id(profile->camera_id))goto done;
 if(cfg->material_fd<0||fstat(cfg->material_fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_uid!=getuid()||(st.st_mode&07777)!=0600||st.st_size!=1536||
    (fcntl(cfg->material_fd,F_GETFL)&O_ACCMODE)!=O_RDONLY||lseek(cfg->material_fd,0,SEEK_CUR)!=0||
    pread(cfg->material_fd,&prefix,offsetof(struct rv_material,username),0)!=(ssize_t)offsetof(struct rv_material,username)||
     prefix.abi!=RV_ABI||prefix.bytes!=1536||prefix.algorithm!=cfg->algorithm||prefix.epoch!=cfg->epoch||
     prefix.boot_generation!=cfg->boot_generation||
    prefix.lengths_and_limits[3]!=cfg->nonce_ttl_ms||prefix.lengths_and_limits[4]!=cfg->rate_window_ms||
    prefix.lengths_and_limits[5]!=cfg->global_verifies||prefix.lengths_and_limits[6]!=cfg->global_challenges||
    prefix.lengths_and_limits[7]!=cfg->account_verifies||prefix.lengths_and_limits[8]!=cfg->account_challenges||
    prefix.lengths_and_limits[9]!=cfg->peer_verifies||prefix.lengths_and_limits[10]!=cfg->peer_challenges||
     memcmp(prefix.device,cfg->device_id,64)||memcmp(prefix.account_id,cfg->account_id,64)||memcmp(prefix.sensor,profile->camera_id,64)||memcmp(prefix.profile,cfg->profile_id,64))goto done;
#ifdef IS_HOST_PUBLIC_ONLY
 if(prefix.account!=UINT64_C(0x505542444d59))goto done;
#else
 if(prefix.account!=1)goto done;
#endif
 /* Same accepted predicate set. Split only to observe the actual rejection,
  * after other nonsecret binding prerequisites, with no payload probing. */
 if(prefix.domain!=CAMERA_ACCESS_RTSP){
#ifdef IS_HOST_PUBLIC_ONLY
  is_host_prefix_stage(1,prefix.credential_generation==cfg->credential_generation,0);
#endif
  goto done;
 }
 if(prefix.credential_generation!=cfg->credential_generation){
#ifdef IS_HOST_PUBLIC_ONLY
  is_host_prefix_stage(2,0,1);
#endif
  goto done;
 }
#ifdef IS_HOST_PUBLIC_ONLY
 is_host_prefix_stage(3,1,1);
#endif
 b->config=cfg;b->last_ns=now;
 if(cfg->random_ready(cfg->context,probe,sizeof(probe)))goto done;
 if(camera_digest_wire_limit_init(&b->global,now/1000000U,cfg->rate_window_ms,cfg->global_verifies,cfg->global_challenges)||
    camera_digest_wire_limit_init(&b->account,now/1000000U,cfg->rate_window_ms,cfg->account_verifies,cfg->account_challenges))goto done;
  b->startup_result=rv_start(&b->child,&cfg->image,cfg->material_fd);
  if(b->startup_result!=RV_OK||rv_ready_flags(b->child)!=RV_FLAGS15||rv_ready_profile(b->child)!=cfg->image.profile)goto done;
 b->initialized=1;result=CAMERA_OK;
 done:rv_wipe(&prefix,sizeof(prefix));rv_wipe(probe,sizeof(probe));if(result)b->failed=1;return result;
}
static int candidate_backend_destroy(struct candidate_backend *b)
{
 if(!b)return CAMERA_INVALID;
 if(b->child){
  if(rv_stop(b->child)!=RV_OK||!rv_retired(b->child)){if(b->config)b->config->retain_verifier(b->config->context,b->child);return RV_UNCERTAIN;}
  int wiped=rv_wipe_observed(b->child),wait0=is_rv_wait0(b->child),was_ready=b->initialized;
  if(rv_destroy(&b->child)!=RV_OK||b->child){if(b->config&&b->child)b->config->retain_verifier(b->config->context,b->child);return RV_UNCERTAIN;}
  rv_wipe(b,sizeof(*b));return was_ready&&(!wiped||!wait0)?CAMERA_IO_ERROR:CAMERA_OK;
 }
 rv_wipe(b,sizeof(*b));return CAMERA_OK;
}
int candidate_backend_poll(struct candidate_backend *b,uint64_t now)
{if(!b||!b->initialized||b->failed||tick(b,now))return CAMERA_NOT_READY;if(b->config->owner_poll&&b->config->owner_poll(b->config->context,b,now)){b->failed=1;return CAMERA_IO_ERROR;}return b->failed?CAMERA_IO_ERROR:CAMERA_OK;}
int candidate_peer_open(struct candidate_backend *b,struct candidate_peer *p,unsigned slot,uint64_t now)
{
 unsigned char random[32]={0};struct rv_message m={0};static const char hex[]="0123456789abcdef";int result=CAMERA_NOT_READY;
 if(!ready(b,now)||!p||slot>=4||b->peers[slot]||b->pending)return result;
 memset(p,0,sizeof(*p));
 if(camera_digest_wire_limit_init(&p->rate,now/1000000U,b->config->rate_window_ms,b->config->peer_verifies,b->config->peer_challenges)||
    camera_digest_wire_admit(&b->global,&b->account,&p->rate,CAMERA_DIGEST_WIRE_VERIFY,now/1000000U))goto done;
 m.kind=RV_OPEN;if(rv_rpc(b->child,&m)!=RV_OK||!m.connection)goto done;
 p->connection=m.connection;p->live=1;b->peers[slot]=p;
 if(b->config->random_ready(b->config->context,random,sizeof(random))){candidate_peer_close(b,p);goto done;}
 for(size_t i=0;i<32;i++){p->session[2*i]=hex[random[i]>>4];p->session[2*i+1]=hex[random[i]&15];}inc(&b->stats.opened);result=CAMERA_OK;
 done:rv_wipe(&m,sizeof(m));rv_wipe(random,sizeof(random));return result;
}
void candidate_peer_close(struct candidate_backend *b,struct candidate_peer *p)
{
 struct rv_message m={0};if(!b||!p||!p->live)return;
 if(b->pending&&b->pending_peer==p){p->canceled=1;if(!b->discarded&&rv_discard(b->child)==RV_OK){b->discarded=1;inc(&b->stats.discarded);}return;} /*cannot erase pending connection/slot*/
 m.kind=RV_CLOSE;m.connection=p->connection;if(!b->failed&&rv_rpc(b->child,&m)!=RV_OK)b->failed=1;
 for(unsigned i=0;i<4;i++)if(b->peers[i]==p)b->peers[i]=NULL;
 inc(&b->stats.closed);rv_wipe(&m,sizeof(m));rv_wipe(p,sizeof(*p));
}
int candidate_effect_final(struct candidate_backend *b,struct candidate_peer *p,uint64_t now)
{
 if(!ready(b,now)||!p||!p->live||p->canceled||!p->grant_present||!p->token||b->pending){if(b)inc(&b->stats.effect_denied);return CAMERA_NOT_READY;}
 /*First exact CHECK exposes only the grant's expiry for a DENY-only post-cost
  *comparison; the second real rv_query is still mandatory on EVERY effect and
  *also checks owned liveness. No cached expiry/Boolean ever grants permission.*/
 struct rv_message m={0};m.kind=RV_CHECK;m.connection=p->connection;m.token=p->token;
 inc(&b->stats.checks);int result=rv_rpc(b->child,&m);uint64_t expires=m.deadline;rv_wipe(&m,sizeof(m));
 if(result==RV_OK){inc(&b->stats.checks);result=rv_query(b->child,p->connection,p->token);}
 latency(b,now);
 if(result==RV_LOST||result==RV_UNCERTAIN)b->failed=1;
 if(result!=RV_OK||b->failed||b->last_ns>=expires){inc(&b->stats.effect_denied);return CAMERA_NOT_READY;}return CAMERA_OK;
}
int candidate_effect(struct candidate_backend *b,struct candidate_peer *p,uint64_t now)
{if(!ready(b,now)||!p||!p->live||p->canceled)return CAMERA_NOT_READY;int r=b->config->source_ready(b->config->context,p->connection,now);uint64_t fresh;if(r||rv_clock(&fresh))return CAMERA_NOT_READY;return candidate_effect_final(b,p,fresh);}
int candidate_admission_check(struct candidate_backend *b,struct candidate_peer *p,uint64_t now)
{
 /*ONLY source-free 401 nonce emission. This negative CHECK is NOT a grant,
  * principal/known-peer oracle or policy permission. CHALLENGE itself validates
  * known connection/rates/epoch/gen; same serialized owner cannot close/rekey
  * between its bound ACK and header writes. No protected effect uses this path.*/
 struct rv_message m={0};if(!ready(b,now)||!p||!p->live||p->canceled||b->pending)return CAMERA_NOT_READY;
 m.kind=RV_CHECK;m.connection=p->connection;m.token=UINT64_MAX;inc(&b->stats.admission_checks);
 int result=rv_rpc(b->child,&m);latency(b,now);rv_wipe(&m,sizeof(m));return result==RV_BAD&&!b->failed?CAMERA_OK:CAMERA_NOT_READY;
}
int candidate_challenge(struct candidate_backend *b,struct candidate_peer *p,uint32_t seq,uint64_t now,enum camera_digest_wire_reason reason,unsigned char out[512],size_t *bytes)
{
 (void)reason;struct rv_message m={0};int result=CAMERA_NOT_READY;uint64_t fresh;
 if(!bytes||!out||candidate_admission_check(b,p,now)||rv_clock(&fresh))return result;
 m.kind=RV_CHALLENGE;m.connection=p->connection;m.bytes=8;rv_put32(m.payload,seq);rv_put32(m.payload+4,CAMERA_DIGEST_WIRE_INITIAL);
 if(rv_rpc(b->child,&m)==RV_OK&&m.bytes&&m.bytes<=512){memcpy(out,m.payload,m.bytes);*bytes=m.bytes;inc(&b->stats.challenges);result=CAMERA_OK;}
 latency(b,fresh);rv_wipe(&m,sizeof(m));return b->failed?CAMERA_NOT_READY:result;
}
int candidate_request_verify(struct candidate_backend *b,struct candidate_peer *p,const void *raw,size_t bytes,int socket_fd,uint64_t now,enum camera_digest_wire_reason *reason)
{
 uint64_t deadline,fresh,token=0;int result=RV_LOST;
 if(!reason)return CAMERA_INVALID;
 *reason=CAMERA_DIGEST_WIRE_NO_CHALLENGE;
 if(!ready(b,now)||!p||!p->live||p->canceled||!raw||!bytes||bytes>RV_PAYLOAD||now>UINT64_MAX-2000000000ULL)return CAMERA_NOT_READY;
 if(b->pending){inc(&b->stats.busy);return CAMERA_BUSY;}deadline=now+2000000000ULL;
 p->grant_present=0;p->token=0;
 if(rv_submit(b->child,p->connection,raw,bytes,deadline)!=RV_OK){b->failed=1;return CAMERA_NOT_READY;}
 b->pending=1;b->pending_peer=p;b->discarded=0;inc(&b->stats.submitted);
 for(unsigned i=0;i<4096;i++){
  if(rv_clock(&fresh)||tick(b,fresh)){b->failed=1;break;}
  if(b->config->owner_poll&&b->config->owner_poll(b->config->context,b,fresh)){b->failed=1;result=RV_LOST;break;}
  struct pollfd fd={socket_fd,POLLRDHUP|POLLHUP,0};
  if(p->canceled||(socket_fd>=0&&poll(&fd,1,0)>0&&(fd.revents&(POLLRDHUP|POLLHUP|POLLERR|POLLNVAL)))){
   p->canceled=1;if(!b->discarded&&rv_discard(b->child)==RV_OK){b->discarded=1;inc(&b->stats.discarded);}
  }
  result=rv_complete(b->child,&token);
  if(is_rv_pending_result(b->child)==0||result!=RV_MORE)break;
  if(rv_clock(&fresh)||tick(b,fresh)||fresh>=deadline){b->failed=1;break;}poll(NULL,0,1);
 }
 /*rv_complete success/denial includes exact RETIRE_RESULT ACK. Lost/timeout
  *keeps pending/context unknown and never admits a second crypto request.*/
 if(is_rv_pending_result(b->child)!=0){b->failed=1;return CAMERA_NOT_READY;}
 b->pending=0;b->pending_peer=NULL;inc(&b->stats.retired_results);
 if(b->revoke_pending){b->revoke_pending=0;if(rv_owner_revoke(b->child)!=RV_OK)b->failed=1;}
 if(result!=RV_OK||!token||p->canceled){inc(&b->stats.auth_denied);if(!p->canceled)*reason=CAMERA_DIGEST_WIRE_INITIAL;return CAMERA_NOT_READY;}
 p->token=token;p->grant_present=1;
 if(rv_clock(&fresh)||candidate_effect(b,p,fresh)){p->grant_present=0;return CAMERA_NOT_READY;}
 inc(&b->stats.verified);return CAMERA_OK;
}
int candidate_backend_revoke(struct candidate_backend *b)
{
 if(!b||!b->initialized||b->failed)return CAMERA_NOT_READY;
 for(unsigned i=0;i<4;i++)if(b->peers[i]){b->peers[i]->grant_present=0;b->peers[i]->canceled=1;}
 b->revoked=1;
 int r=rv_owner_revoke(b->child);if(r==RV_BUSY){b->revoke_pending=1;b->discarded=1;inc(&b->stats.discarded);return CAMERA_BUSY;}if(r!=RV_OK)b->failed=1;return r==RV_OK?CAMERA_OK:CAMERA_NOT_READY;
}
int candidate_backend_cancel(struct candidate_backend *b,uint64_t connection)
{unsigned found=0;if(!b||!b->initialized)return CAMERA_NOT_READY;for(unsigned i=0;i<4;i++)if(b->peers[i]&&(!connection||b->peers[i]->connection==connection)){b->peers[i]->canceled=1;found=1;}return found?CAMERA_OK:CAMERA_NOT_READY;}

/* Single process-local source owner, not an installed media supervisor. Never
 * reset after failure/finish: a second object must not start a second child. */
static struct candidate_prestarted *process_owner;
int candidate_prestarted_start(struct candidate_prestarted *o,const struct candidate_config *cfg)
{
 if(!o)return CAMERA_INVALID;
 if(process_owner||o->phase!=CANDIDATE_PS_EMPTY||o->self||o->config||o->backend.child)
  return CAMERA_NOT_READY;
 process_owner=o;o->self=o;o->config=cfg;o->phase=CANDIDATE_PS_STARTING;
 uint64_t now;
 int r=rv_clock(&now)?CAMERA_NOT_READY:candidate_backend_init(&o->backend,cfg,now);
 o->start_result=r;
 if(o->backend.startup_result==RV_UNCERTAIN){
  /* rv_start already attempted stop and could not establish retirement. Do not
   * route that partial failure through another destructor/stop attempt. */
  o->phase=CANDIDATE_PS_HOLD;o->start_result=RV_UNCERTAIN;
  if(o->backend.child&&o->backend.config)
   o->backend.config->retain_verifier(o->backend.config->context,o->backend.child);
  return RV_UNCERTAIN;
 }
 if(!r&&(!is_rv_running(o->backend.child)||rv_ready_flags(o->backend.child)!=RV_FLAGS15||
         rv_ready_profile(o->backend.child)!=cfg->image.profile)){
  o->backend.failed=1;r=CAMERA_NOT_READY;o->start_result=r;
 }
 o->phase=r?CANDIDATE_PS_START_FAILED:CANDIDATE_PS_READY;
 return r;
}
int candidate_prestarted_finish(struct candidate_prestarted *o)
{
 if(!o||o!=process_owner||o->self!=o)return CAMERA_INVALID;
 if(o->phase!=CANDIDATE_PS_READY&&o->phase!=CANDIDATE_PS_START_FAILED&&
    o->phase!=CANDIDATE_PS_SERVED)return CAMERA_NOT_READY;
 /* No stop/destroy while any actual serve/peer callback can still use this
  * backend. A pending/failed serve is HOLD, not normal finishing admission. */
 if(o->source_hold||o->backend.pending||o->backend.pending_peer)return CAMERA_NOT_READY;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)
  if(o->backend.peers[i])return CAMERA_NOT_READY;
 o->phase=CANDIDATE_PS_FINISHING;
 int r=candidate_backend_destroy(&o->backend); /* exactly one wrapper attempt */
 o->finish_result=r;
 if(o->backend.child){o->phase=CANDIDATE_PS_HOLD;return r?r:RV_UNCERTAIN;}
 o->phase=CANDIDATE_PS_FINISHED;
 return r?r:o->start_result?o->start_result:o->serve_result;
}
