/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include "credential_worker.h"
#include "credential_mbedtls.h"
#include <mbedtls/platform_util.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SECOND 1000000000ULL
struct cw_account {
 char id[CAMERA_ID_BYTES];uint32_t domain,used,enabled;
 uint64_t generation,last_session,next_attempt;
 struct cw_ui_material material;
};
struct cw_secret {
 struct cw_ui_material material;unsigned char password[CW_PASSWORD_MAX];size_t bytes;
};
struct cw_job {
 uint64_t id,generation,deadline;uint32_t domain,status,used,active;
 int account;struct cw_secret secret;
};
struct cw_broker {
 pthread_mutex_t lock;pthread_cond_t wake;pthread_t worker;
 struct cw_options options;uint64_t last_ns,next_job,next_attempt;
 uint32_t fatal,stopping;
 struct cw_ui_material dummy;
 struct cw_account accounts[CW_ACCOUNT_MAX];struct cw_job jobs[CW_JOB_MAX];
#ifdef CW_TEST_ONLY
 cw_test_verify_fn verify;void *verify_context;
#endif
};
static int default_clock(void *context,uint64_t *out)
{
 struct timespec ts;(void)context;
 if(clock_gettime(CLOCK_MONOTONIC,&ts)||ts.tv_sec<0||ts.tv_nsec<0||
    (uint64_t)ts.tv_sec>UINT64_MAX/SECOND)return -1;
 *out=(uint64_t)ts.tv_sec*SECOND;
 if(*out>UINT64_MAX-(uint64_t)ts.tv_nsec)return -1;
 *out+=(uint64_t)ts.tv_nsec;return 0;
}
static int supported(uint32_t d){return d==CAMERA_ACCESS_UI||d==CAMERA_ACCESS_ADMIN;}
static int id_valid(const char *id)
{
 size_t i,j;if(!id||!id[0])return 0;
 for(i=0;i<CAMERA_ID_BYTES;i++){
  unsigned char c=(unsigned char)id[i];
  if(!c){for(j=i;j<CAMERA_ID_BYTES;j++)if(id[j])return 0;return 1;}
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))return 0;
 }return 0;
}
static void wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
static int observe(struct cw_broker *b,uint64_t *now)
{
 if(b->fatal||b->stopping)return CW_FAILED;
 if(b->options.clock(b->options.clock_context,now)||*now<b->last_ns){
  unsigned i;b->fatal=1;
  for(i=0;i<CW_JOB_MAX;i++)if(b->jobs[i].used){
   b->jobs[i].status=CW_STOPPED;wipe(&b->jobs[i].secret,sizeof(b->jobs[i].secret));
  }
  pthread_cond_signal(&b->wake);return CW_FAILED;
 }
 b->last_ns=*now;return CW_OK;
}
static int account_find(struct cw_broker *b,uint32_t d,const char *id)
{
 unsigned i;for(i=0;i<CW_ACCOUNT_MAX;i++)if(b->accounts[i].used&&
  b->accounts[i].domain==d&&!memcmp(b->accounts[i].id,id,CAMERA_ID_BYTES))return (int)i;
 return -1;
}
static struct cw_job *job_find(struct cw_broker *b,uint64_t id)
{
 unsigned i;for(i=0;i<CW_JOB_MAX;i++)if(b->jobs[i].used&&b->jobs[i].id==id)return &b->jobs[i];
 return NULL;
}
static int job_current(struct cw_broker *b,struct cw_job *j)
{
 return j->account>=0&&b->accounts[j->account].enabled&&
  b->accounts[j->account].generation==j->generation;
}
static void refresh(struct cw_broker *b,struct cw_job *j,uint64_t now)
{
 if(j->status==CW_QUEUED||j->status==CW_RUNNING||j->status==CW_MATCH){
  if(j->account>=0&&!job_current(b,j))j->status=CW_REVOKED;
  else if(now>=j->deadline)j->status=CW_EXPIRED;
  if(j->status==CW_REVOKED||j->status==CW_EXPIRED)wipe(&j->secret,sizeof(j->secret));
 }
}
static void invalidate_jobs(struct cw_broker *b,int account)
{
 unsigned i;for(i=0;i<CW_JOB_MAX;i++)if(b->jobs[i].used&&b->jobs[i].account==account){
  b->jobs[i].status=CW_REVOKED;wipe(&b->jobs[i].secret,sizeof(b->jobs[i].secret));
 }
}
static void *worker(void *context)
{
 struct cw_broker *b=context;
 pthread_mutex_lock(&b->lock);
 for(;;){
  struct cw_job *j=NULL;struct cw_secret local;uint64_t now;unsigned i;int result;
  if(b->stopping)break;
  if(!observe(b,&now))for(i=0;i<CW_JOB_MAX;i++)if(b->jobs[i].used){
   refresh(b,&b->jobs[i],now);
   if(b->jobs[i].status==CW_QUEUED&&(!j||b->jobs[i].id<j->id))j=&b->jobs[i];
  }
  if(!j){pthread_cond_wait(&b->wake,&b->lock);continue;}
  j->active=1;j->status=CW_RUNNING;local=j->secret;wipe(&j->secret,sizeof(j->secret));
  pthread_mutex_unlock(&b->lock);
#ifdef CW_TEST_ONLY
  result=b->verify?b->verify(b->verify_context,&local.material,local.password,local.bytes):
   cw_mbedtls_verify(&local.material,local.password,local.bytes);
#else
  result=cw_mbedtls_verify(&local.material,local.password,local.bytes);
#endif
  wipe(&local,sizeof(local));
  pthread_mutex_lock(&b->lock);j->active=0;
  if(b->stopping)j->status=CW_STOPPED;
  else if(!observe(b,&now)){
   refresh(b,j,now);
   if(j->status==CW_RUNNING)j->status=result<0?CW_LIBRARY_ERROR:
    ((result==1&&job_current(b,j))?CW_MATCH:CW_NO_MATCH);
  }
 }
 pthread_mutex_unlock(&b->lock);return NULL;
}
int cw_create(struct cw_broker **out,const struct cw_options *o,const struct cw_ui_material *dummy)
{
 struct cw_broker *b;uint64_t now;pthread_attr_t attr;int failed;
 if(!out||!o||!cw_material_valid(dummy)||o->global_spacing_ns<SECOND||
    o->account_spacing_ns<SECOND||o->global_spacing_ns<o->account_spacing_ns||
    !o->max_job_lifetime_ns)return CW_INVALID;
 b=calloc(1,sizeof(*b));if(!b)return CW_FULL;
 b->options=*o;if(!b->options.clock)b->options.clock=default_clock;
 b->next_job=1;b->dummy=*dummy;
 if(pthread_mutex_init(&b->lock,NULL)){wipe(b,sizeof(*b));free(b);return CW_FAILED;}
 if(pthread_cond_init(&b->wake,NULL)){
  pthread_mutex_destroy(&b->lock);wipe(b,sizeof(*b));free(b);return CW_FAILED;
 }
 failed=observe(b,&now);
 if(!failed){
  failed=pthread_attr_init(&attr);
  if(!failed){
   failed=pthread_attr_setstacksize(&attr,32768U);
   if(!failed)failed=pthread_create(&b->worker,&attr,worker,b);
   pthread_attr_destroy(&attr);
  }
 }
 if(failed){
  pthread_cond_destroy(&b->wake);pthread_mutex_destroy(&b->lock);
  wipe(b,sizeof(*b));free(b);return CW_FAILED;
 }
 *out=b;return CW_OK;
}
void cw_destroy(struct cw_broker *b)
{
 unsigned i;if(!b)return;
 pthread_mutex_lock(&b->lock);b->stopping=1;
 for(i=0;i<CW_JOB_MAX;i++){b->jobs[i].status=CW_STOPPED;wipe(&b->jobs[i].secret,sizeof(b->jobs[i].secret));}
 pthread_cond_signal(&b->wake);pthread_mutex_unlock(&b->lock);
 pthread_join(b->worker,NULL);pthread_cond_destroy(&b->wake);pthread_mutex_destroy(&b->lock);
 wipe(b,sizeof(*b));free(b);
}
int cw_put_ui(struct cw_broker *b,uint32_t domain,const char id[CAMERA_ID_BYTES],uint64_t gen,const struct cw_ui_material *m)
{
 int index,ret=CW_OK;unsigned i;uint64_t now;struct cw_account *a;
 if(!supported(domain))return CW_UNSUPPORTED;
 if(!b||!id_valid(id)||!gen||!cw_material_valid(m))return CW_INVALID;
 pthread_mutex_lock(&b->lock);
 if(observe(b,&now)){ret=CW_FAILED;goto out;}
 index=account_find(b,domain,id);
 if(index<0){
  for(i=0;i<CW_ACCOUNT_MAX;i++)if(!b->accounts[i].used)break;
  if(i==CW_ACCOUNT_MAX){ret=CW_FULL;goto out;}index=(int)i;
 }
 a=&b->accounts[index];
 if(a->used&&gen<=a->generation){ret=CW_STALE;goto out;}
 invalidate_jobs(b,index);wipe(&a->material,sizeof(a->material));
 memcpy(a->id,id,CAMERA_ID_BYTES);a->domain=domain;a->generation=gen;
 a->material=*m;a->used=1;a->enabled=1;
out:pthread_mutex_unlock(&b->lock);return ret;
}
int cw_revoke(struct cw_broker *b,uint32_t domain,const char id[CAMERA_ID_BYTES],uint64_t gen)
{
 int index,ret=CW_OK;uint64_t now;
 if(!supported(domain))return CW_UNSUPPORTED;
 if(!b||!id_valid(id)||!gen)return CW_INVALID;
 pthread_mutex_lock(&b->lock);
 if(observe(b,&now)){ret=CW_FAILED;goto out;}
 index=account_find(b,domain,id);
 if(index<0||gen<=b->accounts[index].generation){ret=CW_STALE;goto out;}
 b->accounts[index].generation=gen;b->accounts[index].enabled=0;
 wipe(&b->accounts[index].material,sizeof(b->accounts[index].material));invalidate_jobs(b,index);
out:pthread_mutex_unlock(&b->lock);return ret;
}
int cw_submit(struct cw_broker *b,uint32_t domain,const char id[CAMERA_ID_BYTES],const unsigned char *password,
 size_t bytes,uint64_t deadline,uint64_t *job_id)
{
 uint64_t now;unsigned i,queued=0;int index,ret=CW_OK;struct cw_job *j;struct cw_account *a=NULL;
 if(!supported(domain))return CW_UNSUPPORTED;
 if(!b||!id_valid(id)||!password||!bytes||bytes>CW_PASSWORD_MAX||!job_id)return CW_INVALID;
 pthread_mutex_lock(&b->lock);
 if(observe(b,&now)){ret=CW_FAILED;goto out;}
 if(deadline<=now||deadline-now>b->options.max_job_lifetime_ns){ret=CW_INVALID;goto out;}
 index=account_find(b,domain,id);if(index>=0)a=&b->accounts[index];
 if(now<b->next_attempt||(a&&now<a->next_attempt)){ret=CW_THROTTLED;goto out;}
 if(now>UINT64_MAX-b->options.global_spacing_ns||
    (a&&now>UINT64_MAX-b->options.account_spacing_ns)||b->next_job==UINT64_MAX){ret=CW_EXHAUSTED;goto out;}
 for(i=0;i<CW_JOB_MAX;i++)if(b->jobs[i].used){
  refresh(b,&b->jobs[i],now);if(b->jobs[i].status==CW_QUEUED)queued++;
 }
 if(queued>=2){ret=CW_FULL;goto out;}
 for(i=0;i<CW_JOB_MAX;i++)if(!b->jobs[i].used)break;
 if(i==CW_JOB_MAX){ret=CW_FULL;goto out;}
 j=&b->jobs[i];memset(j,0,sizeof(*j));j->used=1;j->id=b->next_job++;
 j->domain=domain;j->account=(a&&a->enabled)?index:-1;j->generation=(a&&a->enabled)?a->generation:0;
 j->deadline=deadline;j->status=CW_QUEUED;
 j->secret.material=(a&&a->enabled)?a->material:b->dummy;
 memcpy(j->secret.password,password,bytes);j->secret.bytes=bytes;
 b->next_attempt=now+b->options.global_spacing_ns;if(a)a->next_attempt=now+b->options.account_spacing_ns;
 *job_id=j->id;pthread_cond_signal(&b->wake);
out:pthread_mutex_unlock(&b->lock);return ret;
}
int cw_cancel(struct cw_broker *b,uint64_t id)
{
 struct cw_job *j;int ret=CW_OK;if(!b||!id)return CW_INVALID;
 pthread_mutex_lock(&b->lock);j=job_find(b,id);
 if(!j)ret=CW_STALE;
 else {j->status=CW_CANCELLED;wipe(&j->secret,sizeof(j->secret));}
 pthread_mutex_unlock(&b->lock);return ret;
}
int cw_poll(struct cw_broker *b,uint64_t id,struct cw_completion *out)
{
 struct cw_job *j;uint64_t now;int ret=CW_OK;
 if(!b||!id||!out)return CW_INVALID;
 pthread_mutex_lock(&b->lock);j=job_find(b,id);
 if(!j){ret=CW_STALE;goto done;}
 if(!observe(b,&now))refresh(b,j,now);
 out->job_id=j->id;out->domain=j->domain;out->credential_generation=j->generation;out->status=j->status;
done:pthread_mutex_unlock(&b->lock);return ret;
}
int cw_forget(struct cw_broker *b,uint64_t id)
{
 struct cw_job *j;int ret=CW_OK;if(!b||!id)return CW_INVALID;
 pthread_mutex_lock(&b->lock);j=job_find(b,id);
 if(!j)ret=CW_STALE;
 else if(j->active||j->status==CW_QUEUED||j->status==CW_RUNNING)ret=CW_NOT_READY;
 else wipe(j,sizeof(*j));
 pthread_mutex_unlock(&b->lock);return ret;
}
int cw_issue_access(struct cw_broker *b,uint64_t id,struct camera_access_policy *policy,
 uint64_t lifetime,struct camera_access_principal *out)
{
 struct cw_job *j;struct cw_account *a;struct camera_access_verified f;
 uint64_t now;int ret=CW_FAILED;
 if(!b||!id||!policy||!lifetime||!out)return CW_INVALID;
 pthread_mutex_lock(&b->lock);
 if(observe(b,&now))goto done;
 j=job_find(b,id);if(!j){ret=CW_STALE;goto done;}refresh(b,j,now);
 if(j->active||j->status==CW_QUEUED||j->status==CW_RUNNING){ret=CW_NOT_READY;goto done;}
 if(j->status!=CW_MATCH||!job_current(b,j)){ret=CW_STALE;goto consume;}
 a=&b->accounts[j->account];
 if(now>UINT64_MAX-lifetime||a->last_session==UINT64_MAX){ret=CW_EXHAUSTED;goto consume;}
 memset(&f,0,sizeof(f));f.abi=CAMERA_ACCESS_ABI;f.struct_bytes=sizeof(f);
 f.verified=1;f.domain=j->domain;memcpy(f.account_id,a->id,CAMERA_ID_BYTES);
 memcpy(f.device_id,policy->device_id,CAMERA_ID_BYTES);f.boot_generation=policy->boot_generation;
 f.credential_generation=j->generation;f.session_generation=++a->last_session;
 f.issued_ns=now;f.expires_ns=now+lifetime;
 ret=camera_access_issue(policy,&f,now,out)==CAMERA_ACCESS_OK?CW_OK:CW_FAILED;
consume:wipe(j,sizeof(*j));
done:pthread_mutex_unlock(&b->lock);return ret;
}
#ifdef CW_TEST_ONLY
int cw_test_set_verify(struct cw_broker *b,cw_test_verify_fn fn,void *ctx)
{
 unsigned i;int ret=CW_OK;if(!b)return CW_INVALID;
 pthread_mutex_lock(&b->lock);
 for(i=0;i<CW_JOB_MAX;i++)if(b->jobs[i].used){ret=CW_NOT_READY;goto done;}
 b->verify=fn;b->verify_context=ctx;
done:pthread_mutex_unlock(&b->lock);return ret;
}
int cw_test_set_next_job(struct cw_broker *b,uint64_t id)
{
 unsigned i;int ret=CW_OK;if(!b||!id)return CW_INVALID;
 pthread_mutex_lock(&b->lock);
 for(i=0;i<CW_JOB_MAX;i++)if(b->jobs[i].used){ret=CW_NOT_READY;goto done;}
 b->next_job=id;
done:pthread_mutex_unlock(&b->lock);return ret;
}
size_t cw_test_storage_bytes(void){return sizeof(struct cw_broker);}
#endif
