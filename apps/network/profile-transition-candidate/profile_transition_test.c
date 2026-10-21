/* SPDX-License-Identifier: GPL-2.0-only
 * PUBLICDUMMY trusted policy issuer + PRIVATE synthetic profile/effect fixtures.
 * Never hash/print any fixture content, profiles, ciphertext, nonces/tags/keys.
 * No wire authentication/native radio/DMA/store/unseal qualification. */
#define _GNU_SOURCE
#ifndef PT_PUBLIC_DUMMY_TEST
#error "Explicit PUBLICDUMMY test source only, never product constants"
#endif
#include "profile_transition.h"
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <linux/capability.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
static unsigned cases,checks,serial;static int root;
static unsigned scenario;
static int query_failure;
static const void *watched_region;
static unsigned region_unlocks;
int __real_camera_network_transaction_query(const struct camera_network_transaction_store *,struct camera_network_token,struct camera_ntx_report *);
int __wrap_camera_network_transaction_query(const struct camera_network_transaction_store *s,struct camera_network_token t,struct camera_ntx_report *out)
{if(query_failure){memset(out,0,sizeof(*out));return query_failure;}return __real_camera_network_transaction_query(s,t,out);}
long __real_syscall(long,...);
long __wrap_syscall(long number,...)
{
 va_list ap;long r;va_start(ap,number);
 /* Forward only exact Linux call arities used by these reviewed sources;
  * no fabricated syscall result/readiness or speculative six-arg va reads. */
 if(number==SYS_mlock||number==SYS_munlock){void *p=va_arg(ap,void *);size_t n=va_arg(ap,size_t);
  if(number==SYS_munlock&&p==watched_region)region_unlocks++;
  r=__real_syscall(number,p,n);
 }else if(number==SYS_capget||number==SYS_capset){void *a=va_arg(ap,void *),*b=va_arg(ap,void *);r=__real_syscall(number,a,b);
 }else if(number==SYS_getrandom){void *p=va_arg(ap,void *);size_t n=va_arg(ap,size_t);int flags=va_arg(ap,int);r=__real_syscall(number,p,n,flags);
 }else{errno=ENOSYS;r=-1;}
 va_end(ap);return r;
}
static void scalar_summary(unsigned retained)
{printf("synthetic=1 scenario=%u cases=%u checks=%u private_dirs=%u retained_HOLD=%u passed=1 native_acceptance=0\n",scenario,cases,checks,serial,retained);fflush(stdout);}
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"check=%u line=%d\n",checks,__LINE__);abort();}}while(0)
#define OK(x) CHECK((x)==0)
static const unsigned char dummy_key[32]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
static const char device[CAMERA_ID_BYTES]="PUBLICDUMMY-device";
struct fixture {
 struct profile_transition pt;struct pp_session pp;struct pt_config config;
 struct camera_access_policy policy;struct camera_access_principal admin,other,viewer;
 struct pp_profile original;
 uint64_t now,floor,commit;int profile_fd,meta_fd,key_fd,authority_fd;
 struct camera_network_work source_work,retired_work;struct camera_network_link latest;
 const struct pp_profile *borrowed;
 unsigned begin_count,retire_count,step_count,begin_authority_count,source_live,authority_calls,advance_at,revocation_at;
 unsigned fault_point,fault_nth,fault_seen,io_count,close_seen,close_fail_nth,clock_calls,clock_fail_nth;
 int mutate_authority,advance_time,revoke_on_fence,begin_advance;
};
static void wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
static int zero(const void *p,size_t n){const unsigned char *q=p;unsigned v=0;for(size_t i=0;i<n;i++)v|=q[i];return !v;}
static void p64(unsigned char *p,uint64_t v){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(v>>(8*i));}
static uint64_t g64(const unsigned char *p){uint64_t v=0;for(unsigned i=0;i<8;i++)v|=(uint64_t)p[i]<<(8*i);return v;}
static void checkpoint(struct fixture *f)
{unsigned char state[24]={0};p64(state,5);p64(state+8,f->floor);p64(state+16,f->commit);
 CHECK(pwrite(f->authority_fd,state,24,0)==24&&!fsync(f->authority_fd));wipe(state,24);}
static int public_id(void *p,uint64_t n){(void)p;return n==100||n==101?0:-1;}
static int reserve(void *p,uint64_t key,uint64_t old,uint64_t next)
{struct fixture *f=p;if(key!=5||old!=f->floor||next!=old+1)return -1;f->floor=next;checkpoint(f);return 0;}
static int committed(void *p,uint64_t scope,uint64_t owner,uint64_t key,uint64_t old,uint64_t next)
{struct fixture *f=p;if(scope!=42||owner!=7||key!=5||old!=f->commit||next!=f->floor)return -1;
 f->commit=next;checkpoint(f);return 0;}
static int authority(void *p,uint64_t scope,uint64_t owner,struct pp_authority *out)
{
 struct fixture *f=p;unsigned char state[24]={0};f->authority_calls++;
 if(f->authority_calls==f->advance_at)f->now=1100;
 if(f->authority_calls==f->revocation_at)OK(camera_access_revoke_session(&f->policy,&f->admin));
 if(scope!=42||owner!=7||pread(f->authority_fd,state,24,0)!=24){wipe(state,24);return -1;}
 out->scope_id=42;out->owner_epoch=7;out->key_epoch=g64(state);out->nonce_floor=g64(state+8);out->committed_revision=g64(state+16);
 wipe(state,24);return 0;
}
static int clock_ns(void *p,uint64_t *now)
{struct fixture *f=p;if(++f->clock_calls==f->clock_fail_nth)return -1;*now=f->now;return 0;}
static int workeq(const struct camera_network_work *a,const struct camera_network_work *b)
{return a->token.namespace_id==b->token.namespace_id&&a->token.id==b->token.id&&a->effect==b->effect&&
 a->config_id==b->config_id&&a->radio_generation==b->radio_generation&&a->deadline_ns==b->deadline_ns;}
static int begin(void *p,const struct camera_network_work *w,const struct pp_profile *private_profile)
{
 struct fixture *f=p;CHECK(!f->source_live);CHECK(!memcmp(private_profile,&f->original,sizeof(*private_profile)));
 f->source_work=*w;f->borrowed=private_profile;f->source_live=1;f->begin_count++;
 f->begin_authority_count=f->authority_calls;
 if(f->begin_advance)f->now=w->deadline_ns;
 return 0;
}
static int step(void *p,const struct camera_network_work *w,const struct pp_profile *private_profile,uint32_t kind)
{
 struct fixture *f=p;CHECK(f->source_live&&workeq(w,&f->source_work));
 CHECK(private_profile==f->borrowed&&!memcmp(private_profile,&f->original,sizeof(*private_profile)));
 CHECK(kind==PT_SYNTH_ASSOCIATE||kind==PT_SYNTH_ADDRESS);f->step_count++;return 0;
}
static int retired(void *p,const struct camera_network_work *w)
{struct fixture *f=p;return f->source_live||!workeq(w,&f->retired_work)?-1:0;}
static int reconcile(void *p,const struct camera_ntx_image *m,const struct camera_ntx_reconciliation *r)
{struct fixture *f=p;CHECK(m->scope_id==42&&r->profile_id==100);return f->source_live?-1:0;}
static int latest(void *p,struct camera_network_link *out){*out=((struct fixture *)p)->latest;return 0;}
static int storage_fault(void *p,uint32_t point)
{
 struct fixture *f=p;f->io_count++;
 if(point==CAMERA_SETTINGS_CLOSE&&++f->close_seen==f->close_fail_nth)return 1;
 if(point==CAMERA_SETTINGS_DIR_SYNC&&++f->fault_seen==f->fault_nth){
  if(f->mutate_authority){f->floor++;checkpoint(f);}
  if(f->advance_time)f->now=1100;
  if(f->revoke_on_fence)OK(camera_access_revoke_session(&f->policy,&f->admin));
 }
 return point==f->fault_point;
}
static void idcopy(char dst[CAMERA_ID_BYTES],const char *src){memset(dst,0,CAMERA_ID_BYTES);CHECK(strlen(src)<CAMERA_ID_BYTES);strcpy(dst,src);}
static struct camera_access_principal issue(struct fixture *f,const char *name,uint32_t roles)
{
 struct camera_access_account a={0};struct camera_access_verified fact={0};struct camera_access_principal p;
 a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_ADMIN;a.roles=roles;a.credential_generation=1;
 idcopy(a.account_id,name);a.grant_count=1;a.grants[0].kind=CAMERA_ACCESS_DEVICE;
 memcpy(a.grants[0].resource_id,device,CAMERA_ID_BYTES);a.grants[0].operations=CAMERA_ACCESS_DEVICE_CONFIG;
 /* Real policy requires a valid nonempty grant; viewer has EVENTS on a
  * synthetic sensor, never DEVICE_CONFIG. This is not hardware capability. */
 if(!(roles&CAMERA_ACCESS_CONFIGURATOR)){a.grants[0].kind=CAMERA_ACCESS_SENSOR;
  idcopy(a.grants[0].resource_id,"PUBLICDUMMY-sensor");a.grants[0].operations=CAMERA_ACCESS_EVENTS;}
 OK(camera_access_put_account(&f->policy,&a));
 fact.abi=CAMERA_ACCESS_ABI;fact.struct_bytes=sizeof(fact);fact.verified=1; /* trusted PUBLICDUMMY issuer ONLY */
 fact.domain=CAMERA_ACCESS_ADMIN;memcpy(fact.device_id,device,CAMERA_ID_BYTES);idcopy(fact.account_id,name);
 fact.boot_generation=99;fact.credential_generation=1;fact.session_generation=1;fact.issued_ns=1000;fact.expires_ns=10000;
 OK(camera_access_issue(&f->policy,&fact,1000,&p));return p;
}
static int newdir(const char *name)
{int fd;CHECK(!mkdirat(root,name,0700));fd=openat(root,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(fd>=0);return fd;}
static void setup(struct fixture *f)
{
 char name[48];int base,k;struct pp_options o={0};struct camera_access_resource resource[2]={{0}};
 struct camera_ntx_reconciliation rec={0};memset(f,0,sizeof(*f));f->now=1000;cases++;
 CHECK(snprintf(name,sizeof(name),"PRIVATE-integrated-%04u",++serial)>0);base=newdir(name);
 CHECK(!mkdirat(base,"profiles",0700)&&!mkdirat(base,"metadata",0700));
 f->profile_fd=openat(base,"profiles",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 f->meta_fd=openat(base,"metadata",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(f->profile_fd>=0&&f->meta_fd>=0);
 f->authority_fd=openat(base,"PRIVATE-owner",O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);CHECK(f->authority_fd>=0);checkpoint(f);
 k=openat(base,"PRIVATE-publicdummy-key",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);CHECK(k>=0&&write(k,dummy_key,32)==32&&!close(k));
 f->key_fd=openat(base,"PRIVATE-publicdummy-key",O_RDONLY|O_NOFOLLOW|O_CLOEXEC);CHECK(f->key_fd>=0&&!close(base));
 o.mode=PP_HOST_PUBLICDUMMY_4M;o.scope_id=42;o.owner_epoch=7;o.key_epoch=5;o.public_id=public_id;
 o.reserve=reserve;o.reserve_context=f;o.commit=committed;o.commit_context=f;o.authority=authority;o.authority_context=f;
 OK(pp_session_init(&f->pp,&o));OK(pp_import_key_fd(&f->pp,f->key_fd));CHECK(pp_store_open(&f->pp,f->profile_fd)==PP_EMPTY);OK(pp_seed_ledger(&f->pp));
 f->original.ssid_bytes=16;f->original.psk_bytes=16;f->original.hostname_bytes=11;f->original.ipv4_mode=1;
 memcpy(f->original.ssid,"PUBLICDUMMY-SSID!",16);memcpy(f->original.psk,"PUBLICDUMMY-PSK!!",16);memcpy(f->original.hostname,"dummycamera",11);
 OK(pp_store_put(&f->pp,0,100,&f->original));OK(pp_store_put(&f->pp,1,101,&f->original));
 resource[0].kind=CAMERA_ACCESS_DEVICE;memcpy(resource[0].id,device,CAMERA_ID_BYTES);
 resource[1].kind=CAMERA_ACCESS_SENSOR;idcopy(resource[1].id,"PUBLICDUMMY-sensor");
 OK(camera_access_init(&f->policy,device,99,1000,resource,2));f->admin=issue(f,"PUBLICDUMMY-admin",CAMERA_ACCESS_CONFIGURATOR);
 f->other=issue(f,"PUBLICDUMMY-other",CAMERA_ACCESS_CONFIGURATOR);f->viewer=issue(f,"PUBLICDUMMY-viewer",CAMERA_ACCESS_VIEWER);
 f->config.source_class=PT_SOURCE_SYNTHETIC;f->config.instance_id=201;f->config.source_id=701;f->config.scope_id=42;
 f->config.transition_timeout_ns=100;f->config.rollback_timeout_ns=50;memcpy(f->config.device_id,device,CAMERA_ID_BYTES);
 OK(pp_store_handle(&f->pp,100,1,&f->config.profiles[0]));OK(pp_store_handle(&f->pp,101,2,&f->config.profiles[1]));
 f->config.clock_ns=clock_ns;f->config.clock_context=f;f->config.begin=begin;f->config.step=step;f->config.retired=retired;
 f->config.reconcile=reconcile;f->config.latest=latest;f->config.source_context=f;f->config.storage_fault=storage_fault;f->config.storage_context=f;
 CHECK(pt_open(&f->pt,&f->config,&f->policy,&f->admin,&f->pp,f->meta_fd)==CAMERA_NTX_EMPTY);
 rec.namespace_id=101;rec.radio_generation=30;rec.profile_id=100;rec.choice=CAMERA_NTX_CURRENT;
 OK(pt_seed_and_bind(&f->pt,&f->admin,100,7,&rec));f->io_count=0;f->fault_seen=0;
}
static struct camera_network_request request(void)
{struct camera_network_request r={0};r.token.namespace_id=101;r.token.id=1;r.config_id=101;r.expected_revision=7;
 r.kind=CAMERA_NETWORK_SWITCH;r.required_facts=CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS;return r;}
static struct pt_lease prepare(struct fixture *f)
{struct pt_lease l;struct camera_network_request r=request();OK(pt_prepare(&f->pt,&f->admin,&r,&f->config.profiles[1],&l));return l;}
static struct camera_network_work start(struct fixture *f,struct pt_lease l)
{struct camera_network_work w;OK(pt_receipt_ack(&f->pt,&f->admin,l));OK(pt_dispatch(&f->pt,&f->admin,l,&w));return w;}
static struct camera_network_link observation(const struct camera_network_work *w,uint64_t seq,uint32_t facts)
{struct camera_network_link p={0};p.token=w->token;p.config_id=w->config_id;p.radio_generation=w->radio_generation;p.observation_id=seq;p.facts=facts;return p;}
static void physical_retire(struct fixture *f,const struct camera_network_work *w)
{if(f->source_live)CHECK(workeq(w,&f->source_work));f->source_live=0;f->borrowed=NULL;f->retired_work=*w;f->retire_count++;}
static void finish(struct fixture *f)
{
 int closed;
 CHECK(!f->source_live);CHECK(!f->pt.logical_work&&!f->pt.held_present);
 watched_region=&f->pt.held;region_unlocks=0;closed=pt_close(&f->pt);
 if(closed==PT_BUSY||closed==PT_HOLD){
  CHECK(region_unlocks==0&&f->pt.initialized&&f->pt.locked);
  /* Whole PUBLICDUMMY process stops with retained context/provider/pin.
   * No manual munlock, fake close, reinit, borrowed-provider key wipe or repair. */
  scalar_summary(1);_exit(0);
 }
 CHECK(closed==0&&region_unlocks==1&&!f->pt.initialized&&!f->pt.locked);
 pp_session_close(&f->pp);CHECK(zero(&f->pp.secret,sizeof(f->pp.secret)));wipe(&f->original,sizeof(f->original));
 CHECK(!close(f->profile_fd)&&!close(f->meta_fd)&&!close(f->key_fd)&&!close(f->authority_fd));
}
static void positive(void)
{
 struct fixture f;struct pt_lease l,replay;struct camera_network_work w;struct pt_report r;struct camera_network_request q=request();
 setup(&f);l=prepare(&f);f.io_count=0;OK(pt_prepare(&f.pt,&f.admin,&q,&f.config.profiles[1],&replay));
 CHECK(replay.id==l.id&&f.io_count==0);CHECK(pt_dispatch(&f.pt,&f.admin,l,&w)==CAMERA_NETWORK_NOT_READY&&f.begin_count==0);
 w=start(&f,l);CHECK(f.source_live&&f.pt.held_present&&f.begin_count==1);CHECK(pt_close(&f.pt)==PT_BUSY);
 OK(pt_continue(&f.pt,&f.admin,l,&w,PT_SYNTH_ASSOCIATE));OK(pt_continue(&f.pt,&f.admin,l,&w,PT_SYNTH_ADDRESS));CHECK(f.step_count==2);
 CHECK(pt_source_retire(&f.pt,701,l,&w,1,NULL)==PT_BUSY&&f.pt.held_present);
 physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,1,NULL));CHECK(zero(&f.pt.held,sizeof(f.pt.held)));
 f.latest=observation(&w,1,q.required_facts);OK(pt_source_observe(&f.pt,701,l,&f.latest));
 OK(pt_confirm(&f.pt,&f.admin,l,7));OK(pt_query(&f.pt,&f.admin,l.operation,&r));
 CHECK(r.metadata.published_state==CAMERA_NETWORK_CONFIRMED&&f.begin_count==1);
 f.io_count=0;OK(pt_query(&f.pt,&f.admin,l.operation,&r));CHECK(f.io_count==0&&!r.metadata.durability&&!r.metadata.hardware_applied_known);
 finish(&f);
}
static void denials_and_stale(void)
{
 struct fixture f;struct pt_lease l,bad,out;struct camera_network_request q=request();struct camera_network_work w,wrong;
 struct pp_handle wrong_handle;struct pt_report r;setup(&f);f.io_count=0;
 CHECK(pt_prepare(&f.pt,&f.viewer,&q,&f.config.profiles[1],&out)==PT_DENIED&&f.io_count==0);
 wrong_handle=f.config.profiles[1];wrong_handle.key_epoch++;
 CHECK(pt_prepare(&f.pt,&f.admin,&q,&wrong_handle,&out)==PT_INVALID&&f.io_count==0);
 q.config_id=999;CHECK(pt_prepare(&f.pt,&f.admin,&q,&wrong_handle,&out)==PT_INVALID&&f.io_count==0);
 q=request();q.token.namespace_id=102;CHECK(pt_prepare(&f.pt,&f.admin,&q,&f.config.profiles[1],&out)!=0&&f.begin_count==0);
 q=request();l=prepare(&f);bad=l;bad.id++;
 CHECK(pt_receipt_ack(&f.pt,&f.admin,bad)==PT_STALE);CHECK(pt_frontend_discard(&f.pt,&f.other,l)==PT_DENIED);
 w=start(&f,l);wrong=w;wrong.radio_generation++;
 CHECK(pt_source_retire(&f.pt,701,l,&wrong,0,NULL)==PT_STALE&&f.pt.held_present&&f.source_live);
 CHECK(pt_source_abort(&f.pt,702,l)==PT_STALE&&f.source_live);
 CHECK(pt_cancel(&f.pt,&f.admin,bad)==PT_STALE&&f.pt.held_present);
 OK(pt_cancel(&f.pt,&f.admin,l));CHECK(f.pt.held_present&&f.source_live);
 physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,0,NULL));
 OK(pt_source_rollback(&f.pt,701,l,&w));CHECK(w.effect==CAMERA_NETWORK_ROLLBACK&&f.pt.held_present);
 f.latest=observation(&w,1,q.required_facts);OK(pt_source_observe(&f.pt,701,l,&f.latest));
 {struct camera_network_link loss=observation(&w,2,0);f.latest=loss;CHECK(pt_source_observe(&f.pt,701,l,&loss)==CAMERA_NETWORK_NOT_READY);}
 physical_retire(&f,&w);CHECK(pt_source_retire(&f.pt,701,l,&w,1,&f.latest)==CAMERA_NETWORK_STALE&&f.pt.held_present);
 f.latest=observation(&w,3,q.required_facts);OK(pt_source_retire(&f.pt,701,l,&w,1,&f.latest));
 OK(pt_query(&f.pt,&f.admin,l.operation,&r));CHECK(r.metadata.published_state==CAMERA_NETWORK_RESTORED);finish(&f);
}
static void revoke_pending(void)
{
 struct fixture f;struct pt_lease l;struct camera_network_work w;setup(&f);l=prepare(&f);w=start(&f,l);
 OK(camera_access_revoke_session(&f.policy,&f.admin));
 CHECK(pt_continue(&f.pt,&f.admin,l,&w,PT_SYNTH_ASSOCIATE)==PT_DENIED&&f.step_count==0&&f.pt.held_present);
 CHECK(pt_cancel(&f.pt,&f.admin,l)==PT_DENIED&&f.pt.held_present&&f.source_live);
 CHECK(pt_frontend_discard(&f.pt,&f.other,l)==PT_DENIED&&f.pt.held_present);
 OK(pt_source_abort(&f.pt,701,l));CHECK(f.pt.held_present&&pt_close(&f.pt)==PT_BUSY);
 physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,1,NULL));
 OK(pt_source_rollback(&f.pt,701,l,&w));CHECK(w.effect==CAMERA_NETWORK_ROLLBACK);
 OK(pt_source_continue_rollback(&f.pt,701,l,&w,PT_SYNTH_ASSOCIATE));CHECK(f.step_count==1);
 f.latest=observation(&w,1,request().required_facts);physical_retire(&f,&w);
 OK(pt_source_retire(&f.pt,701,l,&w,1,&f.latest));CHECK(!f.pt.held_present);finish(&f);
}
static void faults_deadlines_and_authority(void)
{
  for(unsigned kind=0;kind<5;kind++){
   if(scenario!=3+kind)continue;
  struct fixture f;struct pt_lease l;struct camera_network_work w;struct camera_ntx_report r;
  setup(&f);l=prepare(&f);OK(pt_receipt_ack(&f.pt,&f.admin,l));f.fault_seen=0;f.fault_nth=2;
  if(kind==0)f.advance_time=1;
  if(kind==1)f.mutate_authority=1;
  if(kind==2)f.revoke_on_fence=1;
  if(kind==3){f.authority_calls=0;f.advance_at=50;} /* crypto callback time consumption */
  if(kind==4)f.fault_point=CAMERA_SETTINGS_FILE_SYNC;
  CHECK(pt_dispatch(&f.pt,&f.admin,l,&w)!=0&&w.effect==0&&f.begin_count==0);
  CHECK(!f.pt.held_present);OK(camera_network_transaction_query(&f.pt.metadata,l.operation,&r));CHECK(!r.published_state);
  if(f.pt.logical_work){OK(pt_source_pending(&f.pt,701,l,&w));physical_retire(&f,&w);
   CHECK(pt_source_retire(&f.pt,701,l,&w,0,NULL)!=CAMERA_NETWORK_STALE);}
  /* Unknown metadata/current crypto authority remains HOLD; no repair/unseal. */
  finish(&f); /* isolated PUBLICDUMMY fixture ends with no source consumer; no successful adapter-close claim */
 }
 if(scenario==8){struct fixture f;struct pt_lease l;struct camera_network_work w;
  setup(&f);l=prepare(&f);f.begin_advance=1;CHECK(pt_receipt_ack(&f.pt,&f.admin,l)==0);
  CHECK(pt_dispatch(&f.pt,&f.admin,l,&w)!=0&&f.source_live&&f.pt.held_present&&f.begin_count==1);
  OK(pt_source_pending(&f.pt,701,l,&w));CHECK(pt_close(&f.pt)==PT_BUSY);physical_retire(&f,&w);
  CHECK(pt_source_retire(&f.pt,701,l,&w,0,NULL)==0);finish(&f);}
}
static void queued_loss_and_private_lifetime(void)
{
 struct fixture f;struct pt_lease l;struct camera_network_work w;struct camera_network_link positive;
 setup(&f);l=prepare(&f);w=start(&f,l);OK(pt_frontend_discard(&f.pt,&f.admin,l));
 CHECK(f.pt.held_present&&f.source_live&&f.borrowed==&f.pt.held);
 CHECK(pt_continue(&f.pt,&f.admin,l,&w,PT_SYNTH_ASSOCIATE)==PT_DENIED&&f.step_count==0);
 physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,0,NULL));
 OK(pt_source_rollback(&f.pt,701,l,&w));positive=observation(&w,1,request().required_facts);
 f.latest=positive;OK(pt_source_observe(&f.pt,701,l,&positive));
 f.latest=observation(&w,2,0); /* queued source loss, NOT delivered separately */
 physical_retire(&f,&w);CHECK(pt_source_retire(&f.pt,701,l,&w,1,&positive)==CAMERA_NETWORK_STALE);
 CHECK(f.pt.held_present&&!f.source_live&&pt_close(&f.pt)==PT_BUSY);
 f.latest=observation(&w,3,request().required_facts);OK(pt_source_retire(&f.pt,701,l,&w,1,&f.latest));
 CHECK(zero(&f.pt.held,sizeof(f.pt.held)));finish(&f);
}
static void stale_crypto_and_expired_owner(void)
{
  for(unsigned mode=0;mode<3;mode++){
   if(scenario!=10+mode)continue;
  struct fixture f;struct pt_lease l;struct camera_network_work w,wrong;
  setup(&f);l=prepare(&f);w=start(&f,l);wrong=w;wrong.config_id++;
  CHECK(pt_continue(&f.pt,&f.admin,l,&wrong,PT_SYNTH_ASSOCIATE)==PT_STALE&&f.source_live);
  if(mode==0){f.floor++;checkpoint(&f);}
  if(mode==1)f.now=10000; /* actual policy session expiry */
  if(mode==2)OK(camera_access_revoke_account(&f.policy,CAMERA_ACCESS_ADMIN,f.admin.fact.account_id));
  CHECK(pt_continue(&f.pt,&f.admin,l,&w,PT_SYNTH_ASSOCIATE)!=0&&f.step_count==0&&f.pt.held_present);
  CHECK(pt_close(&f.pt)==PT_BUSY);physical_retire(&f,&w);
  CHECK(pt_source_retire(&f.pt,701,l,&w,0,NULL)==0);
  CHECK(!f.pt.held_present&&zero(&f.pt.held,sizeof(f.pt.held)));finish(&f);
 }
}
static void before_begin_boundary_and_final_fault(void)
{
 unsigned count;
 {struct fixture f;struct pt_lease l;struct camera_network_work w;
  setup(&f);l=prepare(&f);w=start(&f,l);count=f.begin_authority_count;
  physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,1,NULL));
  f.latest=observation(&w,1,request().required_facts);OK(pt_source_observe(&f.pt,701,l,&f.latest));
  OK(pt_confirm(&f.pt,&f.admin,l,7));finish(&f);}
 if(scenario==14){struct fixture f;struct pt_lease l;struct camera_network_work w;
  setup(&f);l=prepare(&f);OK(pt_receipt_ack(&f.pt,&f.admin,l));f.advance_at=count;
  CHECK(pt_dispatch(&f.pt,&f.admin,l,&w)!=0&&w.effect==0&&f.begin_count==0&&f.pt.logical_work);
  OK(pt_source_pending(&f.pt,701,l,&w));physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,0,NULL));finish(&f);}
 if(scenario==15){struct fixture f;struct pt_lease l;struct camera_network_work w;struct pt_report out;
  setup(&f);l=prepare(&f);w=start(&f,l);physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,1,NULL));
  f.latest=observation(&w,1,request().required_facts);OK(pt_source_observe(&f.pt,701,l,&f.latest));
  f.fault_point=CAMERA_SETTINGS_DIR_SYNC;
  CHECK(pt_confirm(&f.pt,&f.admin,l,7)!=0);OK(pt_query(&f.pt,&f.admin,l.operation,&out));CHECK(!out.metadata.published_state);
  finish(&f);}
}
static void missing_profiles_and_public_validation(void)
{
 struct fixture f;struct pt_lease l;struct camera_network_request q=request();struct pp_handle wrong;
 setup(&f);f.io_count=0;
 for(unsigned field=0;field<5;field++){
  q=request();if(field==0)q.kind=0;if(field==1)q.required_facts=16;
  if(field==2)q.token.id=0;
  wrong=f.config.profiles[1];if(field==3)wrong.owner_epoch++;if(field==4)wrong.revision++;
  CHECK(pt_prepare(&f.pt,&f.admin,&q,&wrong,&l)==PT_INVALID&&f.io_count==0);
 }
 q=request();q.expected_revision=6;
 CHECK(pt_prepare(&f.pt,&f.admin,&q,&f.config.profiles[1],&l)==CAMERA_NETWORK_CONFLICT&&f.io_count==0);
 CHECK(pt_close(&f.pt)==0);pp_session_close(&f.pp);wipe(&f.original,sizeof(f.original));
 CHECK(!close(f.profile_fd)&&!close(f.meta_fd)&&!close(f.key_fd)&&!close(f.authority_fd));
  for(unsigned mode=0;mode<2;mode++){
   if(scenario!=17+mode)continue;
  struct fixture broken;struct camera_network_work w;int fd;
  setup(&broken);l=prepare(&broken);OK(pt_receipt_ack(&broken.pt,&broken.admin,l));
  fd=openat(broken.profile_fd,mode==0?"profile.current":"nonce.ledger",O_WRONLY|O_NOFOLLOW);
  CHECK(fd>=0&&!ftruncate(fd,1)&&!close(fd));broken.io_count=0;
  CHECK(pt_dispatch(&broken.pt,&broken.admin,l,&w)==PT_HOLD&&w.effect==0&&broken.begin_count==0&&broken.io_count==0);
  CHECK(!broken.pt.held_present);finish(&broken);
 }
}
static void retirement_sync_failure(void)
{
 struct fixture f;struct pt_lease l;struct camera_network_work w;
 setup(&f);l=prepare(&f);w=start(&f,l);physical_retire(&f,&w);f.fault_point=CAMERA_SETTINGS_FILE_SYNC;
 CHECK(pt_source_retire(&f.pt,701,l,&w,1,NULL)==CAMERA_NTX_STORAGE);
 CHECK(!f.pt.logical_work&&!f.pt.held_present&&zero(&f.pt.held,sizeof(f.pt.held)));
 finish(&f);
}
static struct pt_lease confirmed(struct fixture *f)
{
 struct pt_lease l;struct camera_network_work w;setup(f);l=prepare(f);w=start(f,l);
 physical_retire(f,&w);OK(pt_source_retire(&f->pt,701,l,&w,1,NULL));
 f->latest=observation(&w,1,request().required_facts);OK(pt_source_observe(&f->pt,701,l,&f->latest));
 OK(pt_confirm(&f->pt,&f->admin,l,7));return l;
}
static void close_retained(struct fixture *f)
{
 unsigned secret_reads=f->authority_calls;struct pp_session *provider=f->pt.profiles;int closed;
 watched_region=&f->pt.held;region_unlocks=0;
 closed=pt_close(&f->pt);CHECK(closed==PT_HOLD||closed==PT_BUSY);
 CHECK(region_unlocks==0&&f->pt.initialized&&f->pt.locked&&f->pt.profiles==provider);
 CHECK(f->authority_calls==secret_reads); /* no close-time secret callback/read */
 CHECK(pt_open(&f->pt,&f->config,&f->policy,&f->admin,&f->pp,f->meta_fd)==PT_BUSY);
 CHECK(region_unlocks==0&&f->pt.initialized&&f->pt.locked);
 CHECK(pt_abandon_empty(&f->pt)==PT_BUSY); /* not a forced forget/recovery route */
}
static void close_query_faults(void)
{
 static const int errors[]={CAMERA_NTX_RECONCILE,CAMERA_NTX_STORAGE,CAMERA_SETTINGS_CORRUPT,PT_HOLD,CAMERA_NETWORK_STALE};
 struct fixture f;struct pt_lease l=confirmed(&f);struct pt_report report;
 for(unsigned i=0;i<sizeof(errors)/sizeof(errors[0]);i++){
  query_failure=errors[i];close_retained(&f);CHECK(f.pp.key_loaded);
  query_failure=0;OK(pt_query(&f.pt,&f.admin,l.operation,&report));
  CHECK(report.metadata.published_state==CAMERA_NETWORK_CONFIRMED);
 }
 finish(&f); /* normal successful query+fresh terminal permits one actual unlock */
}
static void close_backend_uncertainty(void)
{
 struct fixture f;struct pt_lease l;struct camera_ntx_inspection disk;
 unsigned char metadata[2128];int fd;ssize_t bytes;
 l=confirmed(&f);
 if(scenario==21){f.now=999;CHECK(camera_network_transaction_tick(&f.pt.metadata)==CAMERA_NETWORK_CLOCK);}
 if(scenario==22){f.fault_point=CAMERA_SETTINGS_CLOSE;}
 if(scenario==23){
  fd=openat(f.meta_fd,CAMERA_SETTINGS_CURRENT,O_RDONLY|O_NOFOLLOW);CHECK(fd>=0);
  bytes=read(fd,metadata,sizeof(metadata));CHECK(bytes>80&&!close(fd));metadata[12]^=1;
  fd=openat(f.meta_fd,CAMERA_SETTINGS_CURRENT,O_WRONLY|O_TRUNC|O_NOFOLLOW);CHECK(fd>=0);
  CHECK(write(fd,metadata,(size_t)bytes)==bytes&&!close(fd));wipe(metadata,sizeof(metadata));
 }
 if(scenario==24){
  unsigned char wire[CAMERA_NTX_WIRE_BYTES];struct camera_settings_report wrote;
  OK(camera_network_transaction_inspect(&f.pt.metadata,&disk));
  OK(camera_ntx_encode(&f.pt.metadata.config,&disk.current,wire));
  /* Real separate serialized metadata writer advances store revision. Adapter
   * cannot silently refresh its old terminal publication to close this scope. */
  OK(camera_settings_commit(&f.pt.metadata.settings,disk.settings_revision,disk.settings_operation_id+1,
   CAMERA_NTX_SCHEMA,CAMERA_SETTINGS_VALUES,wire,sizeof(wire),&wrote));wipe(wire,sizeof(wire));
 }
 (void)l;close_retained(&f);CHECK(f.pp.key_loaded);scalar_summary(1);_exit(0);
}
static void close_unresolved_restore(void)
{
 struct fixture f;struct pt_lease l;struct camera_network_work w;setup(&f);l=prepare(&f);w=start(&f,l);
 OK(pt_cancel(&f.pt,&f.admin,l));physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,0,NULL));
 close_retained(&f);CHECK(f.pp.key_loaded); /* actual apply retired is not rollback retirement */
 OK(pt_source_rollback(&f.pt,701,l,&w));f.latest=observation(&w,1,request().required_facts);
 physical_retire(&f,&w);OK(pt_source_retire(&f.pt,701,l,&w,1,&f.latest));finish(&f);
}
static void close_cancelled_and_readback(void)
{
 struct fixture f;struct pt_lease l;struct camera_ntx_report r;setup(&f);l=prepare(&f);
 if(scenario==26){OK(pt_cancel(&f.pt,&f.admin,l));finish(&f);return;}
 if(scenario==27)f.fault_point=CAMERA_SETTINGS_FILE_SYNC;
 if(scenario==28){f.close_seen=0;f.close_fail_nth=6;} /* actual final commit readback close */
 CHECK(pt_cancel(&f.pt,&f.admin,l)==CAMERA_NTX_STORAGE);
 f.fault_point=0;f.close_fail_nth=0;
 OK(camera_network_transaction_query(&f.pt.metadata,l.operation,&r));CHECK(r.reconciliation_required&&!r.published_state);
 close_retained(&f);CHECK(f.pp.key_loaded);scalar_summary(1);_exit(0);
}
static void partial_open_lifetime(void)
{
 struct fixture f;struct profile_transition partial={0},empty={0},clocked={0},history={0};
 unsigned reads;int dir,err;setup(&f);reads=f.authority_calls;
 watched_region=&partial.held;region_unlocks=0;
 CHECK(pt_open(&partial,&f.config,&f.policy,&f.admin,&f.pp,-1)==CAMERA_NTX_INVALID);
 CHECK(partial.initialized&&partial.locked&&region_unlocks==0);
 CHECK(pt_close(&partial)==PT_HOLD&&partial.initialized&&region_unlocks==0);
 OK(pt_abandon_empty(&partial));CHECK(region_unlocks==1&&!partial.initialized&&f.pp.key_loaded);
 CHECK(f.authority_calls>reads); /* only constructor authenticated handles, not close */
 dir=newdir("PRIVATE-empty-before-episode");watched_region=&empty.held;region_unlocks=0;
 CHECK(pt_open(&empty,&f.config,&f.policy,&f.admin,&f.pp,dir)==CAMERA_NTX_EMPTY);
 CHECK(pt_close(&empty)==PT_HOLD&&region_unlocks==0);OK(pt_abandon_empty(&empty));CHECK(region_unlocks==1);
 CHECK(!close(dir));
 dir=newdir("PRIVATE-clock-partial");f.clock_calls=0;f.clock_fail_nth=5;
 err=pt_open(&clocked,&f.config,&f.policy,&f.admin,&f.pp,dir);CHECK(err==CAMERA_NETWORK_CLOCK);
 f.clock_fail_nth=0;watched_region=&clocked.held;region_unlocks=0;
 CHECK(pt_close(&clocked)==PT_HOLD&&clocked.initialized&&region_unlocks==0);
 OK(pt_abandon_empty(&clocked));CHECK(region_unlocks==1&&!clocked.initialized&&!close(dir));
 /* Actual nonempty existing scope, even zero receipts, is not abandon-empty. */
 CHECK(pt_open(&history,&f.config,&f.policy,&f.admin,&f.pp,f.meta_fd)==CAMERA_NTX_RECONCILE);
 watched_region=&history.held;region_unlocks=0;
 CHECK(pt_close(&history)==PT_HOLD&&history.initialized&&history.locked&&region_unlocks==0);
 CHECK(pt_abandon_empty(&history)==PT_HOLD&&region_unlocks==0&&f.pp.key_loaded);
 /* Retain original/history contexts/providers until PUBLICDUMMY process exit,
  * not an API closure/namespace reconciliation/native retirement claim. */
 scalar_summary(1);_exit(0);
}
static int harden(void)
{
 struct rlimit core={0,0},lock;struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct caps[2]={{0}};
 if(!geteuid()){if(fchown(root,65534,65534)||setgroups(0,NULL)||setgid(65534)||setuid(65534))return -1;}
 if(setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||syscall(SYS_capset,&h,caps)||getrlimit(RLIMIT_MEMLOCK,&lock))return -1;
 if(lock.rlim_cur>PP_HOST_LOCK_LIMIT)lock.rlim_cur=PP_HOST_LOCK_LIMIT;
 lock.rlim_max=lock.rlim_cur;
 return setrlimit(RLIMIT_MEMLOCK,&lock);
}
int main(int argc,char **argv)
{
 CHECK(argc==3);scenario=(unsigned)strtoul(argv[2],NULL,10);CHECK(!mkdir(argv[1],0700));
 root=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(root>=0);CHECK(!harden());
 if(scenario==0)positive();else if(scenario==1)denials_and_stale();else if(scenario==2)revoke_pending();
 else if(scenario<=8)faults_deadlines_and_authority();else if(scenario==9)queued_loss_and_private_lifetime();
 else if(scenario<=12)stale_crypto_and_expired_owner();else if(scenario<=15)before_begin_boundary_and_final_fault();
 else if(scenario<=18)missing_profiles_and_public_validation();else if(scenario==19)retirement_sync_failure();
 else if(scenario==20)close_query_faults();else if(scenario<=24)close_backend_uncertainty();
 else if(scenario==25)close_unresolved_restore();else if(scenario<=28)close_cancelled_and_readback();
 else if(scenario==29)partial_open_lifetime();
 else return 3;
 CHECK(!close(root));scalar_summary(0);return 0;
}
