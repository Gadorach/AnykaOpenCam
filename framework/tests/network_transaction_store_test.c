/* SPDX-License-Identifier: GPL-2.0-only
 * PUBLIC SYNTHETIC ONLY. argv[1] is a NEW retained host fixture directory.
 * No real settings, private profile data, hardware, radio or media. */
#define _POSIX_C_SOURCE 200809L
#include <camera/network_transaction_store.h>
#include <camera/dma_integrity.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
static unsigned checks,cases,serial;
static int root;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"check=%u line=%d\n",checks,__LINE__);abort();}} while(0)
#define OK(x) CHECK((x)==0)
#define FACTS (CAMERA_NETWORK_AUTH_ASSOC|CAMERA_NETWORK_ADDRESS|CAMERA_NETWORK_LOCAL_REACHABLE)
struct fault {unsigned point,nth,seen,counts[6],events[128],event_count;int fired,crash;uint64_t *clock;uint64_t jump;struct camera_network_link *latest;};
struct fixture {
 struct camera_network_transaction_store s;
 struct camera_ntx_config config;
 struct fault fault;
 uint64_t now;
 int fd,deny_reconcile,deny_effect,private_missing,old_profile_missing,owner_live,clock_fail;
 unsigned reconcile_calls,effect_calls;
 unsigned effect_clock_nth,effect_fail_nth,reconcile_clock_nth,reconcile_fail_nth;
 uint64_t effect_clock_to,reconcile_clock_to;
 struct camera_network_link latest;
};
static int fault(void *ctx,uint32_t point)
{
 struct fault *f=ctx;f->counts[point]++;
 if(f->event_count<128)f->events[f->event_count++]=point;
 if(point==f->point&&++f->seen==f->nth){
  f->fired=1;
  if(f->clock)*f->clock=f->jump;
  if(f->latest){f->latest->observation_id++;f->latest->facts=0;}
  if(f->crash)_exit(77);
  return f->clock||f->latest?0:1;
 }
 return 0;
}
static int handle(void *ctx,uint64_t id)
{(void)ctx;return id>=100&&id<=10000?0:CAMERA_SETTINGS_SENSITIVE;}
static int clock_ns(void *ctx,uint64_t *now)
{struct fixture *f=ctx;if(f->clock_fail)return -1;*now=f->now;return 0;}
/* Trust supplied ONLY by this synthetic effect owner. Never serialize these
 * controls as auth/radio evidence or call a real profile decryptor/driver. */
static int reconcile(void *ctx,const struct camera_ntx_image *m,const struct camera_ntx_reconciliation *p)
{
 struct fixture *f=ctx;f->reconcile_calls++;
 CHECK(m->scope_id==42&&p->profile_id>=100);
 if(f->reconcile_clock_nth==f->reconcile_calls)f->now=f->reconcile_clock_to;
 return f->deny_reconcile||f->private_missing||f->owner_live||f->reconcile_fail_nth==f->reconcile_calls;
}
static int effect(void *ctx,uint32_t kind,uint64_t id,uint64_t generation)
{
 struct fixture *f=ctx;f->effect_calls++;
 CHECK(kind>=CAMERA_NETWORK_APPLY&&kind<=CAMERA_NETWORK_ROLLBACK&&id>=100&&generation>30);
 /* Synthetic callback latency only: no body I/O, driver calls or reentry. */
 if(f->effect_clock_nth==f->effect_calls)f->now=f->effect_clock_to;
 return f->deny_effect||f->private_missing||(f->old_profile_missing&&id==100)||
  f->effect_fail_nth==f->effect_calls;
}
static int latest_link(void *ctx,struct camera_network_token t,uint64_t profile,uint64_t generation,
 struct camera_network_link *out)
{
 struct fixture *f=ctx;(void)t;(void)profile;(void)generation;
 if(f->private_missing||!f->latest.observation_id)return -1;
 *out=f->latest;return 0; /* real fixture owner observation, not expected-ID echo */
}
static void configure(struct fixture *f,int fd)
{
 memset(f,0,sizeof(*f));f->fd=fd;f->now=1000;f->config.scope_id=42;
 f->config.transition_timeout_ns=100;f->config.rollback_timeout_ns=50;
 f->config.public_profile_id=handle;f->config.clock_ns=clock_ns;f->config.clock_context=f;
 f->config.reconcile=reconcile;f->config.reconcile_context=f;
 f->config.effect_ready=effect;f->config.effect_context=f;
 f->config.latest_link=latest_link;f->config.latest_context=f;
}
static int directory(void)
{
 char name[32];int fd;
 CHECK(snprintf(name,sizeof(name),"synthetic-%04u",++serial)>0);
 CHECK(!mkdirat(root,name,0700));fd=openat(root,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 CHECK(fd>=0);return fd;
}
static void open_empty(struct fixture *f)
{
 configure(f,directory());cases++;
 CHECK(camera_network_transaction_open(&f->s,f->fd,&f->config,fault,&f->fault)==CAMERA_NTX_EMPTY);
}
static struct camera_ntx_reconciliation proof(struct fixture *f,uint64_t ns,uint64_t radio,uint32_t choice)
{
 struct camera_ntx_inspection out;struct camera_ntx_reconciliation p={0};
 OK(camera_network_transaction_inspect(&f->s,&out));
 p.namespace_id=ns;p.radio_generation=radio;p.profile_id=out.current.committed_profile_id;p.choice=choice;
 if(out.current.active_index!=CAMERA_NTX_NONE){
  struct camera_ntx_receipt *r=&out.current.receipts[out.current.active_index];
  p.prior_token=r->request.token;p.prior_apply_generation=r->apply_generation;
  p.prior_rollback_generation=r->rollback_generation;
  if(choice==CAMERA_NTX_PREVIOUS)p.profile_id=r->old_profile_id;
 }
 return p;
}
static void setup(struct fixture *f)
{
 struct camera_ntx_reconciliation p;
 open_empty(f);OK(camera_network_transaction_seed(&f->s,100,7));
 p=proof(f,101,30,CAMERA_NTX_CURRENT);OK(camera_network_transaction_reconcile(&f->s,&p));
 CHECK(f->s.bound&&!f->s.blocked);
}
static struct camera_network_request request(uint64_t id)
{
 struct camera_network_request q={0};q.token.namespace_id=101;q.token.id=id;
 q.expected_revision=7;q.config_id=100+id;q.kind=CAMERA_NETWORK_SWITCH;q.required_facts=FACTS;return q;
}
static struct camera_ntx_report query(struct fixture *f,struct camera_network_token t)
{
 struct camera_ntx_report r;OK(camera_network_transaction_query(&f->s,t,&r));
 CHECK(!r.durability&&!r.hardware_applied_known);return r;
}
static struct camera_network_link link_facts(const struct camera_network_work *w,uint64_t observation,uint32_t facts)
{
 struct camera_network_link p={0};p.token=w->token;p.config_id=w->config_id;
 p.radio_generation=w->radio_generation;p.observation_id=observation;p.facts=facts;return p;
}
static struct camera_network_work start(struct fixture *f,struct camera_network_request q)
{
 struct camera_network_work w;
 OK(camera_network_transaction_prepare(&f->s,&q));
 OK(camera_network_transaction_receipt_ack(&f->s,q.token));
 OK(camera_network_transaction_next(&f->s,&w));return w;
}
static void waiting(struct fixture *f)
{
 struct camera_network_request q=request(1);struct camera_network_work w;struct camera_network_link p;
 setup(f);w=start(f,q);OK(camera_network_transaction_complete(&f->s,&w,1,NULL));
 p=link_facts(&w,1,FACTS);f->latest=p;OK(camera_network_transaction_link(&f->s,&p));
 CHECK(query(f,q.token).volatile_state==CAMERA_NETWORK_CONFIRM_READY);
}
static void close_fixture(struct fixture *f){CHECK(!close(f->fd));}
static size_t read_current(struct fixture *f,unsigned char *wire)
{
 int fd=openat(f->fd,CAMERA_SETTINGS_CURRENT,O_RDONLY|O_NOFOLLOW);ssize_t n;
 CHECK(fd>=0);n=read(fd,wire,2128);CHECK(n>0);CHECK(!close(fd));return (size_t)n;
}
static void p32(unsigned char *p,uint32_t v){for(unsigned j=0;j<4;j++)p[j]=(unsigned char)(v>>(8*j));}
static void p64(unsigned char *p,uint64_t v){p32(p,(uint32_t)v);p32(p+4,(uint32_t)(v>>32));}
static void crc(unsigned char *wire,size_t n)
{uint32_t t[256];p32(wire+12,0);camera_crc_bzip2_table(t);p32(wire+12,camera_crc_bzip2_ram(wire,n,t));}
static void replace_fixture(struct fixture *f,const unsigned char *wire,size_t n)
{
 int fd=openat(f->fd,CAMERA_SETTINGS_CURRENT,O_WRONLY|O_TRUNC|O_NOFOLLOW);
 CHECK(fd>=0);CHECK(write(fd,wire,n)==(ssize_t)n);CHECK(!close(fd));
}
static void codec(void)
{
 struct fixture f;struct camera_ntx_image m={0},got;unsigned char wire[CAMERA_NTX_WIRE_BYTES],bad[CAMERA_NTX_WIRE_BYTES];
 open_empty(&f);m.scope_id=42;m.committed_profile_id=100;m.committed_revision=7;
 m.profile_high_water=100;m.active_index=CAMERA_NTX_NONE;
 OK(camera_ntx_encode(&f.config,&m,wire));OK(camera_ntx_decode(&f.config,wire,sizeof(wire),&got));
 CHECK(!memcmp(wire,"NTX1",4)&&wire[4]==1&&wire[16]==42&&wire[56]==100&&wire[72]==0&&wire[76]==8);
 CHECK(got.scope_id==42&&got.committed_profile_id==100&&got.count==0);
 for(unsigned i=0;i<CAMERA_NTX_WIRE_BYTES;i++)if(i>=80)CHECK(wire[i]==0);
 for(unsigned change=0;change<9;change++){
  memcpy(bad,wire,sizeof(bad));
  if(change==0)p32(bad+4,2);
  if(change==1)bad[80]=1;
  if(change==2)bad[128]=1;
  if(change==3)p32(bad+72,9);
  if(change==4)p64(bad+16,43);
  if(change==5)p64(bad+56,1); /* synthetic forbidden handle, NOT a credential */
  if(change==6)p32(bad+12,2);
  if(change==7)p32(bad+8,1024);
  if(change==8)p32(bad,0);
  CHECK(camera_ntx_decode(&f.config,bad,sizeof(bad),&got)!=0);
  CHECK(got.scope_id==0&&got.count==0); /* rejected metadata never echoed */
 }
 {
  struct camera_settings_report r;unsigned before=f.fault.event_count;
  static const unsigned char forbidden[]="PUBLIC-SYNTHETIC-FORBIDDEN-PRIVATE-FIELD";
  CHECK(camera_settings_commit(&f.s.settings,0,1,CAMERA_NTX_SCHEMA,1,forbidden,sizeof(forbidden),&r)==CAMERA_SETTINGS_SENSITIVE);
  CHECK(f.fault.event_count==before);
  memcpy(bad,wire,sizeof(bad));p64(bad+56,1);
  CHECK(camera_settings_commit(&f.s.settings,0,1,CAMERA_NTX_SCHEMA,1,bad,sizeof(bad),&r)!=0);
  CHECK(f.fault.event_count==before);
  CHECK(camera_settings_commit(&f.s.settings,0,1,CAMERA_NTX_SCHEMA,CAMERA_SETTINGS_RESET_REQUEST,wire,sizeof(wire),&r)==CAMERA_SETTINGS_SENSITIVE);
  CHECK(f.fault.event_count==before);
 }
 close_fixture(&f);
}
static void positive(void)
{
 struct fixture f,restart;struct camera_network_request q=request(1),bad=q;
 struct camera_network_work w;struct camera_ntx_report r;struct camera_network_link p;
 struct camera_ntx_reconciliation rp;unsigned char before[2128],after[2128];size_t n,m;
 setup(&f);memset(&f.fault,0,sizeof(f.fault));OK(camera_network_transaction_prepare(&f.s,&q));
 {
  const unsigned order[]={5,1,1,1,1,1,1,1,1,2,5,5,5,2,5,3,4,3,5};
  CHECK(f.fault.event_count==sizeof(order)/sizeof(order[0]));CHECK(!memcmp(order,f.fault.events,sizeof(order)));
 }
 CHECK(camera_network_transaction_next(&f.s,&w)==CAMERA_NETWORK_NOT_READY);
 CHECK(f.effect_calls==0&&!f.s.live_present);
 n=read_current(&f,before);memset(&f.fault,0,sizeof(f.fault));
 OK(camera_network_transaction_prepare(&f.s,&q));r=query(&f,q.token);
 CHECK((r.receipt.phase_flags&255U)==CAMERA_NTX_PREPARED&&r.receipt.old_profile_id==100);
 CHECK(f.fault.event_count==0);
 bad.config_id++;CHECK(camera_network_transaction_prepare(&f.s,&bad)==CAMERA_NETWORK_CONFLICT);
 m=read_current(&f,after);CHECK(n==m&&!memcmp(before,after,n));
 OK(camera_network_transaction_receipt_ack(&f.s,q.token));
 OK(camera_network_transaction_next(&f.s,&w));r=query(&f,q.token);
 CHECK(r.in_flight&&r.volatile_state==CAMERA_NETWORK_APPLYING&&w.radio_generation==31);
 CHECK((r.receipt.phase_flags&255U)==CAMERA_NTX_APPLY_FENCE&&r.receipt.phase_flags&CAMERA_NTX_RECEIPT_ACK);
 CHECK(camera_network_transaction_next(&f.s,&w)==CAMERA_NETWORK_NOT_READY);
 OK(camera_network_transaction_live(&f.s,&w));
 OK(camera_network_transaction_complete(&f.s,&w,1,NULL));p=link_facts(&w,1,FACTS);
 f.latest=p;OK(camera_network_transaction_link(&f.s,&p));
 CHECK(camera_network_transaction_confirm(&f.s,q.token,8)==CAMERA_NETWORK_CONFLICT);
 CHECK(f.s.image.committed_profile_id==100&&f.s.image.committed_revision==7);
 OK(camera_network_transaction_confirm(&f.s,q.token,7));r=query(&f,q.token);
 CHECK(r.receipt.state==CAMERA_NETWORK_CONFIRMED&&(r.receipt.phase_flags&255U)==CAMERA_NTX_FINAL);
 CHECK(r.published_state==CAMERA_NETWORK_CONFIRMED);
 CHECK(r.receipt.result_profile_id==101&&r.receipt.result_revision==8&&!r.in_flight);
 memset(&f.fault,0,sizeof(f.fault));OK(camera_network_transaction_prepare(&f.s,&q));r=query(&f,q.token);
 CHECK(f.fault.event_count==0&&r.receipt.state==CAMERA_NETWORK_CONFIRMED);
 configure(&restart,f.fd);cases++;
 CHECK(camera_network_transaction_open(&restart.s,restart.fd,&restart.config,fault,&restart.fault)==CAMERA_NTX_RECONCILE);
 r=query(&restart,q.token);CHECK(!r.runtime_bound&&r.reconciliation_required&&!r.in_flight&&!r.published_state);
 CHECK(camera_network_transaction_next(&restart.s,&w)==CAMERA_NTX_RECONCILE);
 CHECK(restart.fault.counts[CAMERA_SETTINGS_WRITE]==0);
 rp=proof(&restart,101,33,CAMERA_NTX_CURRENT);
 CHECK(camera_network_transaction_reconcile(&restart.s,&rp)==CAMERA_NETWORK_CONFLICT);
 rp.namespace_id=102;restart.private_missing=1;
 CHECK(camera_network_transaction_reconcile(&restart.s,&rp)==CAMERA_NTX_RECONCILE);
 restart.private_missing=0;OK(camera_network_transaction_reconcile(&restart.s,&rp));
 CHECK(restart.s.image.operation_high_water==1&&restart.s.image.profile_high_water==101);
 close_fixture(&restart);
}
static void rollback_and_cancel(void)
{
 struct fixture f;struct camera_network_request q=request(1);struct camera_network_work w,rb;
 struct camera_network_link p,loss;struct camera_ntx_report r;
 setup(&f);w=start(&f,q);OK(camera_network_transaction_cancel(&f.s,q.token));
 r=query(&f,q.token);CHECK(r.in_flight&&r.volatile_state==CAMERA_NETWORK_CANCEL_WAIT);
 CHECK(camera_network_transaction_next(&f.s,&rb)==CAMERA_NETWORK_NOT_READY);
 OK(camera_network_transaction_complete(&f.s,&w,1,NULL));OK(camera_network_transaction_next(&f.s,&rb));
 p=link_facts(&rb,1,FACTS);OK(camera_network_transaction_link(&f.s,&p));
 loss=link_facts(&rb,2,0);CHECK(camera_network_transaction_link(&f.s,&loss)==CAMERA_NETWORK_NOT_READY);
 CHECK(camera_network_transaction_complete(&f.s,&rb,1,&p)==CAMERA_NETWORK_STALE);
 r=query(&f,q.token);CHECK(r.in_flight&&r.volatile_state==CAMERA_NETWORK_ROLLING_BACK);
 p.observation_id=2;CHECK(camera_network_transaction_complete(&f.s,&rb,1,&p)==CAMERA_NETWORK_STALE);
 p.observation_id=3;OK(camera_network_transaction_complete(&f.s,&rb,1,&p));
 r=query(&f,q.token);CHECK(r.receipt.state==CAMERA_NETWORK_RESTORED&&!r.in_flight);
 CHECK(r.receipt.result_profile_id==100&&f.s.image.committed_revision==7);
 close_fixture(&f);
 setup(&f);w=start(&f,q);OK(camera_network_transaction_complete(&f.s,&w,0,NULL));
 OK(camera_network_transaction_next(&f.s,&rb));p=link_facts(&rb,1,FACTS);loss=link_facts(&rb,2,FACTS|16U);
 CHECK(camera_network_transaction_link(&f.s,&loss)==CAMERA_NETWORK_INVALID);
 CHECK(camera_network_transaction_complete(&f.s,&rb,1,&p)==CAMERA_NETWORK_STALE);
 OK(camera_network_transaction_complete(&f.s,&rb,0,NULL));
 CHECK(query(&f,q.token).receipt.state==CAMERA_NETWORK_FAILED);
 close_fixture(&f);
 setup(&f);OK(camera_network_transaction_prepare(&f.s,&q));OK(camera_network_transaction_cancel(&f.s,q.token));
 CHECK(query(&f,q.token).receipt.state==CAMERA_NETWORK_CANCELLED&&f.effect_calls==0);
 close_fixture(&f);
 setup(&f);q.kind=CAMERA_NETWORK_SCAN;q.config_id=100;w=start(&f,q);
 CHECK(w.effect==CAMERA_NETWORK_DISRUPTIVE_SCAN);OK(camera_network_transaction_complete(&f.s,&w,1,NULL));
 OK(camera_network_transaction_next(&f.s,&rb));p=link_facts(&rb,1,FACTS);
 OK(camera_network_transaction_complete(&f.s,&rb,1,&p));
 CHECK(query(&f,q.token).receipt.state==CAMERA_NETWORK_RESTORED&&f.s.image.committed_revision==7);
 close_fixture(&f);
}
static void restart_pending(void)
{
 for(unsigned stage=0;stage<4;stage++){
  struct fixture f,b;struct camera_network_request q=request(1);struct camera_network_work w;
  struct camera_ntx_reconciliation p;struct camera_ntx_report r;
  setup(&f);OK(camera_network_transaction_prepare(&f.s,&q));
  if(stage>=1)OK(camera_network_transaction_receipt_ack(&f.s,q.token));
  if(stage>=2)OK(camera_network_transaction_next(&f.s,&w));
  if(stage==3)OK(camera_network_transaction_cancel(&f.s,q.token));
  configure(&b,f.fd);cases++;
  CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
  r=query(&b,q.token);CHECK(r.reconciliation_required&&!r.runtime_bound&&!r.in_flight&&r.work_may_be_live);
  CHECK(camera_network_transaction_next(&b.s,&w)==CAMERA_NTX_RECONCILE);
  p=proof(&b,102,40,CAMERA_NTX_CURRENT);p.prior_apply_generation++;
  CHECK(camera_network_transaction_reconcile(&b.s,&p)==CAMERA_NETWORK_STALE);
  p.prior_apply_generation--;b.owner_live=1;
  CHECK(camera_network_transaction_reconcile(&b.s,&p)==CAMERA_NTX_RECONCILE);
  CHECK(b.fault.counts[CAMERA_SETTINGS_WRITE]==0);
  b.owner_live=0;OK(camera_network_transaction_reconcile(&b.s,&p));r=query(&b,q.token);
  CHECK(r.receipt.state==CAMERA_NETWORK_FAILED&&(r.receipt.phase_flags&255U)==CAMERA_NTX_RECONCILED);
  CHECK(r.receipt.reason==CAMERA_NTX_REASON_RESTART&&b.s.image.committed_profile_id==100);
  q=request(2);q.token.namespace_id=102;OK(camera_network_transaction_prepare(&b.s,&q));
  CHECK(b.s.image.profile_high_water==102&&b.s.image.operation_high_water==2);
  close_fixture(&b);
 }
}
/* Site fixtures exercise actual settings_store syscalls, not a fake backend. */
static void at_site(struct fixture *f,unsigned site)
{
 struct camera_network_request q=request(1);struct camera_network_work w;struct camera_network_link p;
 setup(f);
 if(site==0)return; /* prepare */
 OK(camera_network_transaction_prepare(&f->s,&q));if(site==1)return; /* ACK */
 OK(camera_network_transaction_receipt_ack(&f->s,q.token));if(site==2)return; /* apply fence */
 OK(camera_network_transaction_next(&f->s,&w));if(site==3)return; /* apply completion */
 OK(camera_network_transaction_complete(&f->s,&w,site>=5?0:1,NULL));
 if(site==4){p=link_facts(&w,1,FACTS);f->latest=p;OK(camera_network_transaction_link(&f->s,&p));return;} /* both commit barriers */
 if(site==5)return; /* rollback fence */
 OK(camera_network_transaction_next(&f->s,&w)); /* rollback completion */
}
static int run_site(struct fixture *f,unsigned site)
{
 struct camera_network_request q=request(1);struct camera_network_work w;struct camera_network_link p;
 if(site==0)return camera_network_transaction_prepare(&f->s,&q);
 if(site==1)return camera_network_transaction_receipt_ack(&f->s,q.token);
 if(site==2||site==5)return camera_network_transaction_next(&f->s,&w);
 if(site==4)return camera_network_transaction_confirm(&f->s,q.token,7);
 OK(camera_network_transaction_live(&f->s,&w));p=link_facts(&w,1,FACTS);
 return camera_network_transaction_complete(&f->s,&w,1,site==6?&p:NULL);
}
static void fault_campaign(void)
{
 for(unsigned site=0;site<7;site++){
  struct fixture reference;unsigned counts[6];at_site(&reference,site);
  memset(&reference.fault,0,sizeof(reference.fault));OK(run_site(&reference,site));
  memcpy(counts,reference.fault.counts,sizeof(counts));close_fixture(&reference);
  for(unsigned point=1;point<=5;point++)for(unsigned nth=1;nth<=counts[point];nth++){
   struct fixture f,restart;struct camera_network_request q=request(1);struct camera_ntx_report r;
   struct camera_ntx_inspection inspected;struct camera_ntx_reconciliation p;int result;
   at_site(&f,site);memset(&f.fault,0,sizeof(f.fault));f.fault.point=point;f.fault.nth=nth;
   result=run_site(&f,site);CHECK(f.fault.fired);CHECK(result!=0);CHECK(f.s.blocked);
   if(!camera_network_transaction_query(&f.s,q.token,&r)){
    CHECK(r.reconciliation_required);
    CHECK(!r.published_state);
    if(site==4)CHECK(r.volatile_state!=CAMERA_NETWORK_CONFIRMED&&r.receipt.state!=CAMERA_NETWORK_CONFIRMED);
    if(site==2||site==5)CHECK(!r.in_flight&&!f.s.live_present);
   }
   {
    struct camera_network_work w;
    CHECK(camera_network_transaction_next(&f.s,&w)==CAMERA_NTX_RECONCILE);
    if(f.s.live_present){ /* safe actual host-command retirement still allowed */
     OK(camera_network_transaction_live(&f.s,&w));
     CHECK(camera_network_transaction_complete(&f.s,&w,0,NULL)==CAMERA_NTX_RECONCILE);
     CHECK(!f.s.live_present);
    }
   }
   memset(&f.fault,0,sizeof(f.fault));configure(&restart,f.fd);cases++;
   CHECK(camera_network_transaction_open(&restart.s,restart.fd,&restart.config,fault,&restart.fault)==CAMERA_NTX_RECONCILE);
   result=camera_network_transaction_inspect(&restart.s,&inspected);
   CHECK(inspected.hardware_applied_known==0&&inspected.durability==0);
   CHECK(restart.fault.counts[CAMERA_SETTINGS_WRITE]==0);
   if(result==CAMERA_NTX_OK){
    p=proof(&restart,102,100,CAMERA_NTX_CURRENT);
    restart.owner_live=1;CHECK(camera_network_transaction_reconcile(&restart.s,&p)==CAMERA_NTX_RECONCILE);
    CHECK(restart.fault.counts[CAMERA_SETTINGS_WRITE]==0);
   }else CHECK(result==CAMERA_NTX_RECONCILE);
   close_fixture(&restart);
  }
 }
}
static void clocks_and_fences(void)
{
 struct fixture f,b;struct camera_network_request q=request(1);struct camera_ntx_reconciliation p;
 struct camera_network_work w;struct camera_network_link loss;struct camera_ntx_report r;
 waiting(&f);memset(&f.fault,0,sizeof(f.fault));f.fault.point=CAMERA_SETTINGS_DIR_SYNC;
 f.fault.nth=2;f.fault.clock=&f.now;f.fault.jump=1100;
 CHECK(camera_network_transaction_confirm(&f.s,q.token,7)==CAMERA_NTX_RECONCILE);
 r=query(&f,q.token);CHECK(r.receipt.state!=CAMERA_NETWORK_CONFIRMED&&r.reconciliation_required);
 CHECK((r.receipt.phase_flags&255U)==CAMERA_NTX_COMMIT_FENCE&&f.s.image.committed_profile_id==101);
 configure(&b,f.fd);cases++;
 CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
 p=proof(&b,102,40,CAMERA_NTX_PREVIOUS);OK(camera_network_transaction_reconcile(&b.s,&p));
 CHECK(b.s.image.committed_profile_id==100&&b.s.image.committed_revision==9);
 CHECK(query(&b,q.token).receipt.state==CAMERA_NETWORK_FAILED);close_fixture(&b);
 waiting(&f);memset(&f.fault,0,sizeof(f.fault));f.fault.point=CAMERA_SETTINGS_DIR_SYNC;
 f.fault.nth=2;f.fault.clock=&f.now;f.fault.jump=1100;
 CHECK(camera_network_transaction_confirm(&f.s,q.token,7)==CAMERA_NTX_RECONCILE);
 configure(&b,f.fd);cases++;
 CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
 p=proof(&b,102,40,CAMERA_NTX_CURRENT);OK(camera_network_transaction_reconcile(&b.s,&p));
 CHECK(b.s.image.committed_profile_id==101&&b.s.image.committed_revision==8);
 CHECK(query(&b,q.token).receipt.state==CAMERA_NETWORK_FAILED);close_fixture(&b);
 setup(&f);OK(camera_network_transaction_prepare(&f.s,&q));OK(camera_network_transaction_receipt_ack(&f.s,q.token));
 memset(&f.fault,0,sizeof(f.fault));f.fault.point=CAMERA_SETTINGS_DIR_SYNC;f.fault.nth=2;
 f.fault.clock=&f.now;f.fault.jump=1100;
 CHECK(camera_network_transaction_next(&f.s,&w)==CAMERA_NTX_RECONCILE);CHECK(!f.s.live_present);
 close_fixture(&f);
 setup(&f);w=start(&f,q);f.now=1100;OK(camera_network_transaction_tick(&f.s));
 r=query(&f,q.token);CHECK(r.in_flight&&r.volatile_state==CAMERA_NETWORK_CANCEL_WAIT);
 OK(camera_network_transaction_complete(&f.s,&w,1,NULL));OK(camera_network_transaction_next(&f.s,&w));
 loss=link_facts(&w,2,0);CHECK(camera_network_transaction_link(&f.s,&loss)==CAMERA_NETWORK_NOT_READY);
 f.now=1150;OK(camera_network_transaction_tick(&f.s));CHECK(query(&f,q.token).in_flight);
 OK(camera_network_transaction_complete(&f.s,&w,0,NULL));
 CHECK(query(&f,q.token).receipt.state==CAMERA_NETWORK_FAILED);close_fixture(&f);
 setup(&f);w=start(&f,q);f.now=999;
 CHECK(camera_network_transaction_tick(&f.s)==CAMERA_NETWORK_CLOCK);CHECK(query(&f,q.token).in_flight);
 f.now=1000;CHECK(camera_network_transaction_complete(&f.s,&w,0,NULL)==CAMERA_NTX_RECONCILE);
 CHECK(!f.s.live_present&&f.s.blocked);close_fixture(&f);
}
static void bad_files_and_versions(void)
{
 for(unsigned kind=0;kind<6;kind++){
  struct fixture f,b;unsigned char before[2128],after[2128];size_t n,m;int fd;
  setup(&f);n=read_current(&f,before);
  if(kind==0){p32(before+84,2);crc(before,n);replace_fixture(&f,before,n);} /* codec version */
  if(kind==1){before[80+80]=1;crc(before,n);replace_fixture(&f,before,n);} /* unknown reserved field */
  if(kind==2)replace_fixture(&f,before,n-1); /* torn/truncated */
  if(kind==3){fd=openat(f.fd,"PUBLIC-SYNTHETIC-foreign",O_WRONLY|O_CREAT|O_EXCL,0600);CHECK(fd>=0);CHECK(!close(fd));}
  if(kind==4)CHECK(!fchmodat(f.fd,CAMERA_SETTINGS_CURRENT,0644,0));
  if(kind==5){before[12]^=1;replace_fixture(&f,before,n);} /* bad outer CRC */
  n=read_current(&f,before);configure(&b,f.fd);cases++;
  CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
  CHECK(camera_network_transaction_seed(&b.s,100,7)==CAMERA_NTX_RECONCILE);
  m=read_current(&b,after);CHECK(n==m&&!memcmp(before,after,n));
  CHECK(b.fault.counts[CAMERA_SETTINGS_WRITE]==0);close_fixture(&b);
 }
 {
  struct fixture f,b;struct camera_network_request q=request(1);struct camera_ntx_inspection out;
  struct camera_ntx_reconciliation p={0};
  setup(&f);memset(&f.fault,0,sizeof(f.fault));f.fault.point=CAMERA_SETTINGS_RENAME;f.fault.nth=1;
  CHECK(camera_network_transaction_prepare(&f.s,&q)==CAMERA_NTX_STORAGE);
  memset(&f.fault,0,sizeof(f.fault));configure(&b,f.fd);cases++;
  CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
  CHECK(camera_network_transaction_inspect(&b.s,&out)==CAMERA_NTX_RECONCILE);
  CHECK(out.current_valid&&out.pending_valid&&out.pending.count==1&&out.current.count==0);
  p.namespace_id=102;p.radio_generation=40;p.profile_id=100;p.choice=CAMERA_NTX_CURRENT;
  CHECK(camera_network_transaction_reconcile(&b.s,&p)==CAMERA_NTX_RECONCILE);
  CHECK(b.reconcile_calls==0&&b.fault.counts[CAMERA_SETTINGS_WRITE]==0);close_fixture(&b);
 }
}
static void capacity_and_privacy(void)
{
 struct fixture f;struct camera_network_request q;struct camera_network_work w;struct camera_ntx_inspection out;
 setup(&f);
 for(unsigned i=1;i<=8;i++){
  q=request(i);OK(camera_network_transaction_prepare(&f.s,&q));OK(camera_network_transaction_cancel(&f.s,q.token));
 }
 q=request(9);CHECK(camera_network_transaction_prepare(&f.s,&q)==CAMERA_NTX_CAPACITY);
 OK(camera_network_transaction_inspect(&f.s,&out));CHECK(out.current.count==8);
 CHECK(out.current.operation_high_water==8&&out.current.profile_high_water==108&&out.current.radio_high_water==46);
 CHECK(query(&f,request(1).token).receipt.state==CAMERA_NETWORK_CANCELLED);close_fixture(&f);
 setup(&f);q=request(1);q.config_id=1;CHECK(camera_network_transaction_prepare(&f.s,&q)==CAMERA_NTX_PROFILE_BLOCKED);
 q=request(1);OK(camera_network_transaction_prepare(&f.s,&q));OK(camera_network_transaction_receipt_ack(&f.s,q.token));
 f.private_missing=1;CHECK(camera_network_transaction_next(&f.s,&w)==CAMERA_NTX_PROFILE_BLOCKED);
 CHECK(!f.s.live_present);close_fixture(&f);
 setup(&f);q=request(1);OK(camera_network_transaction_prepare(&f.s,&q));
 OK(camera_network_transaction_receipt_ack(&f.s,q.token));f.old_profile_missing=1;
 CHECK(camera_network_transaction_next(&f.s,&w)==CAMERA_NTX_PROFILE_BLOCKED);
 CHECK(!f.s.live_present&&(query(&f,q.token).receipt.phase_flags&255U)==CAMERA_NTX_ACKED);
 close_fixture(&f);
 setup(&f);q=request(1);f.s.settings_revision=UINT64_MAX-7;
 CHECK(camera_network_transaction_prepare(&f.s,&q)==CAMERA_NTX_EXHAUSTED);
 CHECK(f.s.image.count==0);close_fixture(&f);
}
static void replay_stale_and_conflict(void)
{
 struct fixture f,b;struct camera_network_request q=request(1),q2=request(2);
 struct camera_network_work w,wrong;struct camera_network_link p,stale;
 struct camera_ntx_reconciliation rp;struct camera_ntx_inspection out;
 uint64_t revision;unsigned char wire[CAMERA_NTX_WIRE_BYTES];
 setup(&f);w=start(&f,q);OK(camera_network_transaction_complete(&f.s,&w,1,NULL));
 p=link_facts(&w,1,FACTS);f.latest=p;OK(camera_network_transaction_link(&f.s,&p));
 OK(camera_network_transaction_confirm(&f.s,q.token,7));revision=f.s.settings_revision;
 memset(&f.fault,0,sizeof(f.fault));f.now=0;
 CHECK(camera_network_transaction_cancel(&f.s,q.token)==CAMERA_NETWORK_ORDER);
 CHECK(f.s.last_ns==1000&&!f.s.blocked);
 f.now=1000;OK(camera_network_transaction_tick(&f.s));
 CHECK(f.s.settings_revision==revision&&f.fault.counts[CAMERA_SETTINGS_WRITE]==0);
 q2.expected_revision=8;OK(camera_network_transaction_prepare(&f.s,&q2));
 OK(camera_network_transaction_receipt_ack(&f.s,q2.token));revision=f.s.settings_revision;
 memset(&f.fault,0,sizeof(f.fault));OK(camera_network_transaction_receipt_ack(&f.s,q2.token));
 CHECK(f.s.settings_revision==revision&&f.fault.counts[CAMERA_SETTINGS_WRITE]==0);
 OK(camera_network_transaction_next(&f.s,&w));wrong=w;wrong.radio_generation++;
 CHECK(camera_network_transaction_complete(&f.s,&wrong,0,NULL)==CAMERA_NETWORK_STALE);
 CHECK(query(&f,q2.token).in_flight);
 CHECK(camera_network_transaction_complete(&f.s,&w,0,NULL)==0);
 OK(camera_network_transaction_next(&f.s,&w));p=link_facts(&w,1,FACTS);
 OK(camera_network_transaction_link(&f.s,&p));stale=p;stale.config_id=100;
 f.now=UINT64_MAX;CHECK(camera_network_transaction_link(&f.s,&stale)==CAMERA_NETWORK_STALE);
 CHECK(f.s.last_ns==1000&&!f.s.blocked);f.now=1000;
 OK(camera_network_transaction_complete(&f.s,&w,1,&p));
 /* Two serialized contexts cannot commit from an out-of-date store revision.
  * This external write is a PUBLIC fixture using the real allowlisted store. */
 close_fixture(&f);setup(&f);OK(camera_network_transaction_prepare(&f.s,&q));
 configure(&b,f.fd);cases++;
 CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
 rp=proof(&b,102,40,CAMERA_NTX_CURRENT);OK(camera_network_transaction_reconcile(&b.s,&rp));
 CHECK(camera_network_transaction_receipt_ack(&f.s,q.token)==CAMERA_NTX_STORAGE);
 CHECK(f.s.last_settings_result==CAMERA_SETTINGS_CONFLICT&&f.s.blocked&&!f.s.live_present);
 close_fixture(&b);
 /* Stored high-water exhaustion cannot produce a fresh epoch or wrap IDs. */
 for(unsigned field=0;field<2;field++){
  struct camera_settings_report settings_out;struct camera_ntx_image image;
  setup(&f);OK(camera_network_transaction_inspect(&f.s,&out));image=out.current;
  if(field==0)image.namespace_high_water=UINT64_MAX;
  else image.radio_high_water=UINT64_MAX;
  OK(camera_ntx_encode(&f.config,&image,wire));
  OK(camera_settings_commit(&f.s.settings,out.settings_revision,out.settings_operation_id+1,
   CAMERA_NTX_SCHEMA,CAMERA_SETTINGS_VALUES,wire,sizeof(wire),&settings_out));
  configure(&b,f.fd);cases++;
  CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
  rp=proof(&b,102,40,CAMERA_NTX_CURRENT);
  CHECK(camera_network_transaction_reconcile(&b.s,&rp)==CAMERA_NTX_EXHAUSTED);
  CHECK(b.fault.counts[CAMERA_SETTINGS_WRITE]==0&&!b.s.bound);close_fixture(&b);
 }
}
static void codec_receipt_fields(void)
{
 struct fixture f;struct camera_network_request q=request(1);struct camera_ntx_image got;
 unsigned char wire[CAMERA_NTX_WIRE_BYTES],bad[CAMERA_NTX_WIRE_BYTES];
 setup(&f);OK(camera_network_transaction_prepare(&f.s,&q));
 OK(camera_ntx_encode(&f.config,&f.s.image,wire));
 for(unsigned field=0;field<12;field++){
  memcpy(bad,wire,sizeof(bad));
  if(field==0)p64(bad+128,0); /* operation namespace */
  if(field==1)p64(bad+136,0); /* operation ID */
  if(field==2)p64(bad+152,1); /* private/unknown synthetic handle */
  if(field==3)p64(bad+160,1); /* rollback handle */
  if(field==4)p64(bad+168,UINT64_MAX); /* apply generation overflow */
  if(field==5)p64(bad+176,1); /* generation mismatch */
  if(field==6)p64(bad+192,UINT64_MAX); /* future logical result revision */
  if(field==7)p32(bad+200,CAMERA_NETWORK_CONFIRMED); /* terminal without FINAL */
  if(field==8)p32(bad+212,3); /* reset/unsupported operation kind */
  if(field==9)p32(bad+216,16); /* unknown facts */
  if(field==10)p32(bad+220,CAMERA_NTX_ACKED); /* missing receipt ACK */
  if(field==11)p32(bad+220,CAMERA_NTX_PREPARED|1024U); /* unknown/auth grant flag */
  CHECK(camera_ntx_decode(&f.config,bad,sizeof(bad),&got)!=0);
  CHECK(got.count==0&&got.scope_id==0);
 }
 close_fixture(&f);
}
static void precommit_observers(void)
{
 struct camera_network_request q=request(1);
 for(unsigned scenario=0;scenario<7;scenario++){
  struct fixture f;struct camera_ntx_report r;waiting(&f);
  if(scenario==0){ /* loss OBSERVED during fsync, not yet delivered to core */
   memset(&f.fault,0,sizeof(f.fault));f.fault.point=CAMERA_SETTINGS_DIR_SYNC;
   f.fault.nth=2;f.fault.latest=&f.latest;
  }
  if(scenario==1){f.latest.observation_id=2;f.latest.facts=FACTS|16U;}
  if(scenario==2)f.latest.radio_generation++;
  if(scenario==3)f.latest.observation_id=0;
  if(scenario==4)f.latest.facts=0; /* equal-ID conflicting report */
  if(scenario==5)f.private_missing=1;
  if(scenario==6)f.latest.observation_id=2; /* fresh positive source observation */
  if(scenario==6){
   OK(camera_network_transaction_confirm(&f.s,q.token,7));
   CHECK(query(&f,q.token).published_state==CAMERA_NETWORK_CONFIRMED);
  }else{
   CHECK(camera_network_transaction_confirm(&f.s,q.token,7)==CAMERA_NTX_RECONCILE);
   r=query(&f,q.token);
   CHECK(!r.published_state&&r.reconciliation_required&&r.receipt.state!=CAMERA_NETWORK_CONFIRMED);
   CHECK((r.receipt.phase_flags&255U)==CAMERA_NTX_COMMIT_FENCE);
   CHECK(f.s.image.committed_profile_id==101&&f.s.image.committed_revision==8);
   CHECK(f.s.image.receipts[0].old_profile_id==100&&!f.s.live_present);
  }
  close_fixture(&f);
 }
}
static void no_dispatched_work(struct fixture *f,struct camera_network_token token,
 const struct camera_network_work *out,uint32_t phase)
{
 const struct camera_network_work zero={0};struct camera_network_work live;
 struct camera_network_report core;struct camera_ntx_report r;
 CHECK(!memcmp(out,&zero,sizeof(zero))&&!f->s.live_present);
 CHECK(!memcmp(&f->s.live_work,&zero,sizeof(zero)));
 OK(camera_network_query(&f->s.core,token,&core));
 CHECK(!core.in_flight&&core.state!=CAMERA_NETWORK_APPLYING&&core.state!=CAMERA_NETWORK_ROLLING_BACK);
 CHECK(camera_network_transaction_live(&f->s,&live)==CAMERA_NETWORK_NOT_READY);
 CHECK(!memcmp(&live,&zero,sizeof(zero)));
 r=query(f,token);CHECK(!r.in_flight&&!r.published_state&&r.reconciliation_required);
 CHECK((r.receipt.phase_flags&255U)==phase);
 CHECK(r.receipt.old_profile_id==100&&r.receipt.request.config_id>=100);
 CHECK(f->s.image.committed_profile_id==100&&f->s.image.committed_revision==7);
}
static void dispatch_callback_clocks(void)
{
 /* APPLY/SCAN post admission has operation callback3 then old-profile callback4;
  * ROLLBACK has only callback2. Test consumption by EACH possible last callback. */
 for(unsigned kind=0;kind<3;kind++)for(unsigned last=0;last<(kind==2?1U:2U);last++){
  for(unsigned scenario=0;scenario<4;scenario++){
   struct fixture f;struct camera_network_request q=request(1);struct camera_network_work out,w;
   uint64_t deadline;unsigned nth;int result;uint32_t phase;
   setup(&f);
   if(kind==1){q.kind=CAMERA_NETWORK_SCAN;q.config_id=100;}
   OK(camera_network_transaction_prepare(&f.s,&q));OK(camera_network_transaction_receipt_ack(&f.s,q.token));
   if(kind==2){
    OK(camera_network_transaction_next(&f.s,&w));
    OK(camera_network_transaction_complete(&f.s,&w,0,NULL));
   }
   f.effect_calls=0;memset(&f.fault,0,sizeof(f.fault));
   deadline=kind==2?1050:1100;phase=kind==2?CAMERA_NTX_ROLLBACK_FENCE:CAMERA_NTX_APPLY_FENCE;
   nth=kind==2?2:3+last;f.effect_clock_nth=nth;
   f.effect_clock_to=scenario==2?999:deadline+(scenario==1?1:0);
   if(scenario==3)f.effect_fail_nth=nth; /* late denial before dispatch */
   memset(&out,0xa5,sizeof(out));result=camera_network_transaction_next(&f.s,&out);
   CHECK(result==(scenario==2?CAMERA_NETWORK_CLOCK:scenario==3?CAMERA_NTX_PROFILE_BLOCKED:CAMERA_NTX_RECONCILE));
   CHECK(f.effect_calls==(scenario==3||kind==2?nth:4));
   CHECK(f.fault.counts[CAMERA_SETTINGS_RENAME]==1&&f.fault.counts[CAMERA_SETTINGS_DIR_SYNC]==2);
   no_dispatched_work(&f,q.token,&out,phase);
   CHECK(camera_network_transaction_next(&f.s,&out)==CAMERA_NTX_RECONCILE);
   no_dispatched_work(&f,q.token,&out,phase);close_fixture(&f);
  }
 }
 /* Pre-fence admission delay now fails BEFORE storage, without body I/O. */
 for(unsigned nth=1;nth<=2;nth++){
  struct fixture f;struct camera_network_request q=request(1);struct camera_network_work out;
  setup(&f);OK(camera_network_transaction_prepare(&f.s,&q));OK(camera_network_transaction_receipt_ack(&f.s,q.token));
  f.effect_calls=0;memset(&f.fault,0,sizeof(f.fault));f.effect_clock_nth=nth;f.effect_clock_to=1100;
  CHECK(camera_network_transaction_next(&f.s,&out)==CAMERA_NTX_RECONCILE);
  CHECK(f.fault.counts[CAMERA_SETTINGS_WRITE]==0&&f.fault.counts[CAMERA_SETTINGS_RENAME]==0);
  no_dispatched_work(&f,q.token,&out,CAMERA_NTX_ACKED);close_fixture(&f);
 }
 /* Time-consuming SUCCESS still emits exactly ONE matching effect with the
  * correct freshly based rollback effect deadline, never a clock-free dispatch. */
 for(unsigned kind=0;kind<3;kind++){
  struct fixture f;struct camera_network_request q=request(1);struct camera_network_work out,again,w;
  setup(&f);if(kind==1){q.kind=CAMERA_NETWORK_SCAN;q.config_id=100;}
  OK(camera_network_transaction_prepare(&f.s,&q));OK(camera_network_transaction_receipt_ack(&f.s,q.token));
  if(kind==2){OK(camera_network_transaction_next(&f.s,&w));OK(camera_network_transaction_complete(&f.s,&w,0,NULL));}
  f.effect_calls=0;f.effect_clock_nth=kind==2?2:4;f.effect_clock_to=kind==2?1049:1099;
  OK(camera_network_transaction_next(&f.s,&out));
  CHECK(out.effect==(kind==2?CAMERA_NETWORK_ROLLBACK:kind==1?CAMERA_NETWORK_DISRUPTIVE_SCAN:CAMERA_NETWORK_APPLY));
  CHECK(out.deadline_ns==(kind==2?1099:1100)&&out.deadline_ns>f.now);
  CHECK(query(&f,q.token).in_flight&&f.s.live_present);
  CHECK(camera_network_transaction_next(&f.s,&again)==CAMERA_NETWORK_NOT_READY);
  CHECK(again.effect==0);close_fixture(&f);
 }
}
static void reconciliation_callback_clocks(void)
{
 for(unsigned scenario=0;scenario<3;scenario++){
  struct fixture f;struct camera_ntx_reconciliation p;struct camera_network_request q=request(1);
  struct camera_network_report core;
  open_empty(&f);OK(camera_network_transaction_seed(&f.s,100,7));p=proof(&f,101,30,CAMERA_NTX_CURRENT);
  f.reconcile_clock_nth=2;f.reconcile_clock_to=scenario==1?999:1400;
  if(scenario==2)f.reconcile_fail_nth=2;
  if(scenario==0){
   memset(&f.fault,0,sizeof(f.fault));f.fault.point=CAMERA_SETTINGS_DIR_SYNC;
   f.fault.nth=2;f.fault.clock=&f.now;f.fault.jump=1300;
   OK(camera_network_transaction_reconcile(&f.s,&p));
   CHECK(f.reconcile_calls==2&&f.s.last_ns==1400);
   OK(camera_network_transaction_prepare(&f.s,&q));
   OK(camera_network_query(&f.s.core,q.token,&core));CHECK(core.deadline_ns==1500);
  }else{
   CHECK(camera_network_transaction_reconcile(&f.s,&p)==
    (scenario==1?CAMERA_NETWORK_CLOCK:CAMERA_NTX_RECONCILE));
   CHECK(!f.s.bound&&!f.s.live_present&&f.s.blocked);
   CHECK(f.s.image.namespace_high_water==101&&f.s.image.radio_high_water==30);
  }
  close_fixture(&f);
 }
}
static void process_crashes(void)
{
 for(unsigned site=0;site<3;site++){
  struct fixture f,b;struct camera_network_request q=request(1);int status;pid_t pid;
  struct camera_ntx_inspection out;
  if(site==0)setup(&f);else waiting(&f);
  memset(&f.fault,0,sizeof(f.fault));f.fault.point=CAMERA_SETTINGS_DIR_SYNC;
  f.fault.nth=site==2?4:2;f.fault.crash=1;
  pid=fork();CHECK(pid>=0);
  if(!pid){
   if(site==0)(void)camera_network_transaction_prepare(&f.s,&q);
   else (void)camera_network_transaction_confirm(&f.s,q.token,7);
   _exit(99);
  }
  CHECK(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==77);
  configure(&b,f.fd);cases++;
  CHECK(camera_network_transaction_open(&b.s,b.fd,&b.config,fault,&b.fault)==CAMERA_NTX_RECONCILE);
  OK(camera_network_transaction_inspect(&b.s,&out));
  CHECK(out.current_valid&&!out.durability&&!out.hardware_applied_known);
  CHECK(!b.s.bound&&!b.s.live_present&&b.fault.counts[CAMERA_SETTINGS_WRITE]==0);
  if(site==1)CHECK((out.current.receipts[0].phase_flags&255U)==CAMERA_NTX_COMMIT_FENCE);
  if(site==2)CHECK(query(&b,q.token).published_state==0);
  close_fixture(&b);
 }
}
int main(int argc,char **argv)
{
 CHECK(argc==2);CHECK(!mkdir(argv[1],0700));root=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 CHECK(root>=0);codec();positive();rollback_and_cancel();restart_pending();fault_campaign();
 clocks_and_fences();bad_files_and_versions();capacity_and_privacy();replay_stale_and_conflict();
 codec_receipt_fields();precommit_observers();dispatch_callback_clocks();reconciliation_callback_clocks();process_crashes();
 CHECK(!close(root));printf("synthetic=1 cases=%u checks=%u retained_dirs=%u passed=1\n",cases,checks,serial);
 return 0;
}
