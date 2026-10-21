/* SPDX-License-Identifier: GPL-2.0-only */
/* HOST ONLY PUBLIC DUMMY. Actual strict child/full600k, not a match hook. */
#define _GNU_SOURCE
#include "credential/credential_process.h"
#include "credential/credential_ipc.h"
#include "web_service.h"
#include <mbedtls/pkcs5.h>
#include <assert.h>
#include <linux/capability.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#define S 1000000000ULL
static unsigned checks;
#define CHECK(x) do{checks++;assert(x);}while(0)
static const char id[CAMERA_ID_BYTES]="public-fixture-user";
static const unsigned char password[]="PUBLIC-DUMMY-NOT-A-PRODUCT-PASSWORD";
static uint64_t now(void){uint64_t n=0;CHECK(cp_clock_read(&n)==0);return n;}
static void tick(void){CHECK(poll(NULL,0,2)==0);}
static struct cp_ticket submit(struct cp_client *c,uint64_t request,uint64_t ttl)
{
 struct cp_ticket t={0};uint64_t until=now()+2*S;int ret;
 do{ret=cp_submit(c,CAMERA_ACCESS_ADMIN,id,request,password,sizeof(password)-1,now()+ttl,&t);
  if(ret!=CP_THROTTLED)break;
  tick();}while(now()<until);
 CHECK(ret==CP_OK);return t;
}
static struct cp_receipt finish(struct cp_client *c,unsigned status)
{
 struct cp_receipt r={0};uint64_t until=now()+4*S;
 do{cp_step(c,&r);if(r.status==status)return r;tick();}while(now()<until);
 CHECK(r.status==status);return r;
}
static void request(struct camera_access_request *q,const struct camera_access_policy *p)
{
 memset(q,0,sizeof(*q));q->abi=CAMERA_ACCESS_ABI;q->struct_bytes=sizeof(*q);
 q->domain=CAMERA_ACCESS_ADMIN;q->kind=CAMERA_ACCESS_DEVICE;q->operation=CAMERA_ACCESS_DEVICE_CONFIG;
 memcpy(q->device_id,p->device_id,CAMERA_ID_BYTES);
}
int main(int argc,char **argv)
{
 struct rlimit core={0,0};struct __user_cap_header_struct caph={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct caps[2]={{0}};
 struct camera_access_policy policy;struct camera_access_resource resource={CAMERA_ACCESS_DEVICE,"public-device",{0}};
 struct camera_access_account account={0};struct cw_ui_material material={CW_SCHEMA,CW_ITERATIONS,{0},{0}};
 struct cp_options options={0};struct cp_client *c=NULL;struct cp_ticket t,other;
 struct camera_access_principal p,changed,victim,alias_copy;struct camera_access_request q;uint64_t gen;
 CHECK(argc==2&&getuid()!=0);
 CHECK(!setrlimit(RLIMIT_CORE,&core)&&!prctl(PR_SET_DUMPABLE,0));
 CHECK(!syscall(SYS_capset,&caph,caps));CHECK(!mlockall(MCL_CURRENT|MCL_FUTURE));
 memcpy(material.salt,"PUBLIC-DUMMY-001",16);
 CHECK(!mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,password,sizeof(password)-1,
  material.salt,16,CW_ITERATIONS,32,material.verifier));
 CHECK(!camera_access_init(&policy,resource.id,777,now(),&resource,1));
 account.abi=CAMERA_ACCESS_ABI;account.struct_bytes=sizeof(account);account.domain=CAMERA_ACCESS_ADMIN;
 account.roles=CAMERA_ACCESS_CONFIGURATOR;memcpy(account.account_id,id,CAMERA_ID_BYTES);
 account.credential_generation=1;account.grant_count=1;account.grants[0].kind=CAMERA_ACCESS_DEVICE;
 account.grants[0].operations=CAMERA_ACCESS_DEVICE_CONFIG;memcpy(account.grants[0].resource_id,resource.id,CAMERA_ID_BYTES);
 CHECK(!camera_access_put_account(&policy,&account));
 options.child_generation=778;options.global_spacing_ns=2*S;options.account_spacing_ns=2*S;
 options.max_job_lifetime_ns=4*S;options.control_timeout_ns=2*S;options.retirement_grace_ns=100000000ULL;
 CHECK(cp_start(&c,argv[1],&options,&material,&policy)==CP_OK);
 CHECK(cp_put_ui(c,CAMERA_ACCESS_ADMIN,id,1,&material)==CP_OK);
 cp_wipe(&material,sizeof(material));CHECK(!web_parent_sandbox());
 request(&q,&policy);
 t=submit(c,1,4*S);finish(c,CP_MATCHED);CHECK(cp_issue_access(c,&t,10*S,&p)==CP_OK);
 gen=p.fact.session_generation;
 CHECK(camera_access_check(&policy,&p,&q,now())==CAMERA_ACCESS_OK);
 #define BAD(field,value) do{changed=p;changed.field=(value);CHECK(cp_release_access(c,&changed)==CP_STALE);\
  CHECK(camera_access_check(&policy,&p,&q,now())==CAMERA_ACCESS_OK);}while(0)
 BAD(snapshot_serial,p.snapshot_serial+1);BAD(policy_revision,p.policy_revision+1);
 BAD(fact.abi,p.fact.abi+1);BAD(fact.struct_bytes,p.fact.struct_bytes+1);
 BAD(fact.verified,0);BAD(fact.domain,CAMERA_ACCESS_RTSP);BAD(fact.reserved,1);
 BAD(fact.boot_generation,p.fact.boot_generation+1);BAD(fact.credential_generation,p.fact.credential_generation+1);
 BAD(fact.session_generation,p.fact.session_generation+1);BAD(fact.issued_ns,p.fact.issued_ns+1);
 BAD(fact.expires_ns,p.fact.expires_ns+1);
 #undef BAD
 changed=p;changed.fact.device_id[0]='X';CHECK(cp_release_access(c,&changed)==CP_STALE);
 changed=p;changed.fact.account_id[0]='X';CHECK(cp_release_access(c,&changed)==CP_STALE);
 CHECK(cp_release_access(c,&p)==CP_OK);CHECK(cp_release_access(c,&p)==CP_STALE);
 CHECK(camera_access_check(&policy,&p,&q,now())==CAMERA_ACCESS_DENIED);
 /* Field-wise owner release also tolerates a legitimate source alias to the
  * policy's immutable value: revoke stored snapshot first, don't zero caller. */
 t=submit(c,2,4*S);finish(c,CP_MATCHED);CHECK(cp_issue_access(c,&t,10*S,&victim)==CP_OK);
 CHECK(victim.fact.session_generation>gen);gen=victim.fact.session_generation;
 CHECK(cp_release_access(c,&p)==CP_STALE);
 CHECK(camera_access_check(&policy,&victim,&q,now())==CAMERA_ACCESS_OK);
 {unsigned i;for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)if(policy.sessions[i].active&&
    policy.sessions[i].principal.snapshot_serial==victim.snapshot_serial)break;
  CHECK(i<CAMERA_ACCESS_MAX_SESSIONS);alias_copy=policy.sessions[i].principal;
  CHECK(cp_release_access(c,&policy.sessions[i].principal)==CP_OK);
  CHECK(policy.sessions[i].principal.snapshot_serial==alias_copy.snapshot_serial);
 }
 /* Running discard cannot free slot or issue a fact. Unknown/known work retains
  * same global admission; test uses the real known600k then denies late MATCH. */
 t=submit(c,3,4*S);other=t;other.request_generation++;
 CHECK(cp_discard(c,&other)==CP_STALE);CHECK(cp_issue_access(c,&t,S,&changed)==CP_PENDING);
 CHECK(cp_discard(c,&t)==CP_OK);
 CHECK(cp_forget(c,&t)==CP_PENDING);CHECK(cp_issue_access(c,&t,S,&changed)==CP_STALE);
 CHECK(cp_submit(c,CAMERA_ACCESS_ADMIN,id,4,password,sizeof(password)-1,now()+4*S,&other)==CP_BUSY);
 finish(c,CP_DISCARDED);CHECK(cp_issue_access(c,&t,S,&changed)==CP_STALE);
 CHECK(cp_healthy(c)&&!cp_retirement_observed(c));CHECK(cp_forget(c,&t)==CP_OK);
 CHECK(cp_discard(c,&t)==CP_STALE);
 CHECK(cp_submit(c,CAMERA_ACCESS_ADMIN,id,4,password,sizeof(password)-1,now()+4*S,&other)==CP_THROTTLED);
 /* Cancel AFTER received match, without consuming result early or permitting
  * the result to be treated as authority after discard. */
 t=submit(c,4,4*S);finish(c,CP_MATCHED);CHECK(cp_discard(c,&t)==CP_OK);
 CHECK(cp_issue_access(c,&t,S,&changed)==CP_STALE);CHECK(cp_forget(c,&t)==CP_OK);
 t=submit(c,5,4*S);finish(c,CP_MATCHED);CHECK(cp_issue_access(c,&t,1000000ULL,&p)==CP_OK);
 CHECK(p.fact.session_generation>gen);tick();CHECK(!camera_access_sweep(&policy,now()));
 CHECK(cp_release_access(c,&p)==CP_OK); /* exact policy absence after expiry */
 CHECK(cp_release_access(c,&p)==CP_STALE);
 /* Original hard CPU timeout still fails/retire; logical discard is NOT an
  * exemption. No new spawn syscall/filter permission has been introduced. */
 t=submit(c,6,20000000ULL);CHECK(cp_discard(c,&t)==CP_OK);finish(c,CP_EXPIRED);
 CHECK(!cp_healthy(c));CHECK(cp_issue_access(c,&t,S,&changed)==CP_LOST);
 CHECK(cp_retire(c)==CP_OK&&cp_retirement_observed(c));CHECK(cp_destroy(&c)==CP_OK&&!c);
 printf("strict_child_real600k=1 release_mutation_alias_replay_expiry=PASS discard_lateMATCH_busy=PASS hard_timeout_owned_retirement=PASS checks=%u\n",checks);
 return 0;
}
