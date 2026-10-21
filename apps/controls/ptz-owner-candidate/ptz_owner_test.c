/* SPDX-License-Identifier: GPL-2.0-only */
/* PUBLICDUMMY synthetic provider bookkeeping, NEVER mechanical simulation. */
#include "ptz_owner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,suites;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}} while(0)
#define START() do {suites++;} while(0)
#define BASE UINT64_C(1000000000)
struct fake {
 uint64_t now,latency,clock_jump;
 struct ptz_request requests[32];uint32_t count;
 int phase_result,stop_result,reenter,reentered;
 struct ptz_owner *owner;
};
struct fixture {
 struct camera_access_policy policy;
 struct camera_access_resource resources[5];
 struct camera_device devices[3];struct camera_profile profile;
 struct camera_association associations[2];struct camera_graph graph;
 struct ptz_auth auth;struct ptz_registry registry;struct ptz_owner owner;
 struct fake fake;struct ptz_provider provider;struct ptz_config config;
 struct camera_access_principal ui,onvif,rtsp;
};
static void id(char *dest,const char *text){memset(dest,0,CAMERA_ID_BYTES);memcpy(dest,text,strlen(text));}
static uint64_t fake_clock(void *context)
{
 struct fake *f=context;
 if(f->clock_jump){f->now+=f->clock_jump;f->clock_jump=0;}
 return f->now;
}
static int fake_submit(void *context,const struct ptz_request *q)
{
 struct fake *f=context;
 CHECK(f->count<32);f->requests[f->count++]=*q;
 if(f->reenter){f->reenter=0;f->reentered=ptz_owner_quiesce(f->owner,PTZ_SLIP,f->now);}
 f->now+=f->latency;
 return q->kind==PTZ_PHASE?f->phase_result:f->stop_result;
}
static void account(struct fixture *f,uint32_t domain)
{
 struct camera_access_account a;
 memset(&a,0,sizeof(a));a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);
 a.domain=domain;a.credential_generation=1;id(a.account_id,"PUBLICDUMMY-operator");
 if(domain==CAMERA_ACCESS_UI){
  a.roles=CAMERA_ACCESS_OPERATOR;a.grant_count=1;
  a.grants[0].kind=CAMERA_ACCESS_PTZ;a.grants[0].operations=CAMERA_ACCESS_PTZ_CONTROL;
  memcpy(a.grants[0].resource_id,f->auth.head,CAMERA_ID_BYTES);
 }else{
  a.roles=CAMERA_ACCESS_VIEWER;a.grant_count=1;
  a.grants[0].kind=CAMERA_ACCESS_PROFILE;a.grants[0].operations=CAMERA_ACCESS_LIVE;
  id(a.grants[0].resource_id,"PUBLICDUMMY-profile0");
  if(domain==CAMERA_ACCESS_ONVIF){
   a.roles|=CAMERA_ACCESS_OPERATOR;a.grant_count=2;
   a.grants[1].kind=CAMERA_ACCESS_PTZ;a.grants[1].operations=CAMERA_ACCESS_PTZ_CONTROL;
   memcpy(a.grants[1].resource_id,f->auth.head,CAMERA_ID_BYTES);
  }
 }
 CHECK(camera_access_put_account(&f->policy,&a)==CAMERA_ACCESS_OK);
}
static int issue(struct fixture *f,uint32_t domain,uint64_t serial,uint64_t expires,struct camera_access_principal *out)
{
 /* Only a trusted fixture engine issues these PUBLICDUMMY principals. The
  * candidate adapter never accepts/constructs verified=1 protocol assertions. */
 struct camera_access_verified v;
 memset(&v,0,sizeof(v));v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;
 v.domain=domain;v.boot_generation=77;v.credential_generation=1;v.session_generation=serial;
 v.issued_ns=BASE;v.expires_ns=expires;id(v.device_id,"PUBLICDUMMY-device");id(v.account_id,"PUBLICDUMMY-operator");
 return camera_access_issue(&f->policy,&v,BASE,out);
}
static void setup_head(struct fixture *f,const char *head_name)
{
 unsigned i;char device[CAMERA_ID_BYTES],head[CAMERA_ID_BYTES],user[CAMERA_ID_BYTES];
 memset(f,0,sizeof(*f));id(device,"PUBLICDUMMY-device");id(head,head_name);id(user,"PUBLICDUMMY-operator");
 f->resources[0].kind=CAMERA_ACCESS_DEVICE;id(f->resources[0].id,"PUBLICDUMMY-device");
 f->resources[1].kind=CAMERA_ACCESS_SENSOR;id(f->resources[1].id,"PUBLICDUMMY-sensor0");
 f->resources[2].kind=CAMERA_ACCESS_SENSOR;id(f->resources[2].id,"PUBLICDUMMY-sensor1");
 f->resources[3].kind=CAMERA_ACCESS_PROFILE;id(f->resources[3].id,"PUBLICDUMMY-profile0");id(f->resources[3].sensor_id,"PUBLICDUMMY-sensor0");
 f->resources[4].kind=CAMERA_ACCESS_PTZ;id(f->resources[4].id,head_name);
 CHECK(camera_access_init(&f->policy,device,77,BASE,f->resources,5)==CAMERA_ACCESS_OK);
 for(i=0;i<2;i++){
  id(f->devices[i].id,i?"PUBLICDUMMY-sensor1":"PUBLICDUMMY-sensor0");
  f->devices[i].kind=CAMERA_DEVICE_SENSOR;f->devices[i].state=CAMERA_PRESENT_UNQUALIFIED;
  id(f->associations[i].camera_id,i?"PUBLICDUMMY-sensor1":"PUBLICDUMMY-sensor0");id(f->associations[i].device_id,head_name);
 }
 id(f->devices[2].id,head_name);f->devices[2].kind=CAMERA_DEVICE_PTZ;f->devices[2].state=CAMERA_PRESENT_UNQUALIFIED;
 id(f->profile.id,"PUBLICDUMMY-profile0");id(f->profile.camera_id,"PUBLICDUMMY-sensor0");
 f->profile.width=1280;f->profile.height=720;f->profile.codec=CAMERA_PIXELS;
 f->graph.devices=f->devices;f->graph.device_count=3;f->graph.associations=f->associations;
 f->graph.association_count=2;f->graph.profiles=&f->profile;f->graph.profile_count=1;
 CHECK(ptz_auth_init(&f->auth,&f->policy,&f->graph,head)==CAMERA_OK);
 account(f,CAMERA_ACCESS_UI);account(f,CAMERA_ACCESS_ONVIF);account(f,CAMERA_ACCESS_RTSP);
 CHECK(issue(f,CAMERA_ACCESS_ONVIF,1,BASE+UINT64_C(1000000000),&f->onvif)==CAMERA_ACCESS_DENIED);
 CHECK(camera_access_map_nvr(&f->policy,user,user)==CAMERA_ACCESS_OK);
 CHECK(issue(f,CAMERA_ACCESS_UI,1,BASE+UINT64_C(1000000000),&f->ui)==CAMERA_ACCESS_OK);
 CHECK(issue(f,CAMERA_ACCESS_ONVIF,1,BASE+UINT64_C(1000000000),&f->onvif)==CAMERA_ACCESS_OK);
 CHECK(issue(f,CAMERA_ACCESS_RTSP,1,BASE+UINT64_C(1000000000),&f->rtsp)==CAMERA_ACCESS_OK);
 CHECK(ptz_registry_init(&f->registry,UINT64_C(0x5055424c4943))==PTZ_OK);
 f->fake.now=BASE;f->fake.owner=&f->owner;
 f->provider.abi=PTZ_OWNER_ABI;f->provider.struct_bytes=sizeof(f->provider);
 f->provider.readiness=PTZ_REQUIRED_READY;f->provider.callback_bound_ns=PTZ_CALLBACK_MAX_NS;
 f->provider.stop_bound_ns=PTZ_STOP_MAX_NS;f->provider.context=&f->fake;
 f->provider.clock_ns=fake_clock;f->provider.submit=fake_submit;
 f->config.phase_limit=4;f->config.pan_sign=f->config.tilt_sign=1;
 CHECK(ptz_owner_init(&f->owner,&f->registry,&f->auth,&f->provider,&f->config,1,BASE)==PTZ_OK);
}
static void setup(struct fixture *f){setup_head(f,"PUBLICDUMMY-head0");}
static struct ptz_move move(const struct camera_access_principal *p,uint32_t count)
{
 struct ptz_move m;
 memset(&m,0,sizeof(m));m.principal=*p;id(m.sensor,"PUBLICDUMMY-sensor0");
 m.phases=count;m.direction=1;m.lease_ns=PTZ_LEASE_MAX_NS;return m;
}
static int poll_at(struct fixture *f,uint64_t now){f->fake.now=now;return ptz_owner_poll(&f->owner,now);}
static int dispose_at(struct fixture *f,uint64_t now){f->fake.now=now;return ptz_owner_dispose(&f->owner,now);}
static int quiesce_at(struct fixture *f,uint32_t reason,uint64_t now){f->fake.now=now;return ptz_owner_quiesce(&f->owner,reason,now);}
static int stop_at(struct fixture *f,const struct camera_access_principal *p,const char sensor[CAMERA_ID_BYTES],uint64_t now)
{f->fake.now=now;return ptz_owner_stop(&f->owner,p,sensor,now);}
static struct ptz_completion completion(const struct ptz_request *q,uint32_t result)
{
 struct ptz_completion c;
 memset(&c,0,sizeof(c));c.ticket=q->ticket;c.kind=q->kind;c.result=result;
 return c;
}
static int complete_at(struct fixture *f,struct ptz_completion *c,uint64_t now)
{
 f->fake.now=now;return ptz_owner_complete(&f->owner,c,now);
}
static void ack_last(struct fixture *f,uint64_t now)
{
 struct ptz_completion c=completion(&f->fake.requests[f->fake.count-1],PTZ_ACK);
 CHECK(complete_at(f,&c,now)==PTZ_OK);
}
static struct ptz_view view(struct fixture *f)
{
 struct ptz_view v;CHECK(ptz_owner_view(&f->owner,&v)==PTZ_OK);return v;
}
static void normal_sequence(void)
{
 struct fixture f;struct ptz_move m;struct ptz_view v;uint32_t axis,orient,i;
 START();
 for(axis=0;axis<2;axis++)for(orient=0;orient<4;orient++){
  const uint32_t forward[4]={3,6,12,9},reverse[4]={3,9,12,6};
  int sign=(orient&1)?-1:1;int direction=(orient&2)?-1:1;
  setup(&f);f.config.pan_sign=f.config.tilt_sign=sign;
  CHECK(ptz_owner_reconfigure(&f.owner,&f.config,2,BASE)==PTZ_OK);
  m=move(&f.ui,4);m.axis=axis;m.direction=direction;
  CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);CHECK(f.fake.count==0);
  for(i=0;i<4;i++){
   uint64_t t=BASE+(uint64_t)i*PTZ_PHASE_NS;
   CHECK(poll_at(&f,t)==PTZ_OK);CHECK(f.fake.count==i+1);
   CHECK(f.fake.requests[i].kind==PTZ_PHASE);CHECK(f.fake.requests[i].axis==axis);
   CHECK(f.fake.requests[i].phase_bits==(sign*direction>0?forward[i]:reverse[i]));
   CHECK(f.fake.requests[i].neutral_by_ns==t+PTZ_PHASE_NS);
   ack_last(&f,t+1);CHECK(poll_at(&f,t+PTZ_PHASE_NS-1)==PTZ_OK);CHECK(f.fake.count==i+1);
  }
  CHECK(poll_at(&f,BASE+4*PTZ_PHASE_NS)==PTZ_OK);CHECK(f.fake.count==5);
  CHECK(f.fake.requests[4].kind==PTZ_NEUTRAL);CHECK(f.fake.requests[4].phase_bits==0);
  v=view(&f);CHECK(v.retained==1&&v.applied_phases==4&&v.desired_kind==PTZ_NEUTRAL);
  CHECK(!v.electrical_valid&&!v.position_known&&!v.native_qualified);
  ack_last(&f,BASE+4*PTZ_PHASE_NS+1);v=view(&f);
  CHECK(v.state==PTZ_IDLE&&!v.retained&&v.applied_kind==PTZ_NEUTRAL&&!v.electrical_valid);
  CHECK(dispose_at(&f,BASE+4*PTZ_PHASE_NS+2)==PTZ_OK);CHECK(view(&f).state==PTZ_DISPOSED);
  /* Nothing in config touches either preview-axis transform or decoded pixels. */
 }
}
static void arbitration(void)
{
 struct fixture f;struct ptz_move a,b;struct ptz_completion c;struct ptz_owner second={0};struct ptz_view v;
 START();setup(&f);a=move(&f.ui,4);b=move(&f.onvif,2);
 CHECK(ptz_owner_init(&second,&f.registry,&f.auth,&f.provider,&f.config,2,BASE)==PTZ_BUSY);
 CHECK(ptz_owner_move(&f.owner,&a,BASE)==PTZ_OK);
 CHECK(ptz_owner_move(&f.owner,&b,BASE)==PTZ_BUSY);CHECK(f.fake.count==0);
 CHECK(poll_at(&f,BASE)==PTZ_OK);c=completion(&f.fake.requests[0],PTZ_ACK);
 b=move(&f.ui,2);b.axis=1;id(b.sensor,"PUBLICDUMMY-sensor1");
 CHECK(ptz_owner_move(&f.owner,&b,BASE)==PTZ_OK);CHECK(f.fake.count==2);
 CHECK(f.fake.requests[1].kind==PTZ_NEUTRAL);CHECK(view(&f).queued==1);
 CHECK(ptz_owner_move(&f.owner,&b,BASE)==PTZ_BUSY);
 CHECK(complete_at(&f,&c,BASE+1)==PTZ_STALE);CHECK(f.fake.count==2);
 CHECK(poll_at(&f,BASE+2)==PTZ_OK);CHECK(f.fake.count==2);
 ack_last(&f,BASE+3);v=view(&f);CHECK(v.state==PTZ_QUEUED&&!v.retained);
 CHECK(poll_at(&f,BASE+4)==PTZ_OK);CHECK(f.fake.count==3&&f.fake.requests[2].axis==1);
 CHECK(f.fake.requests[2].ticket.sequence>f.fake.requests[1].ticket.sequence);
 CHECK(stop_at(&f,&f.onvif,b.sensor,BASE+4)==PTZ_RETAINED);
 CHECK(f.fake.count==4&&f.fake.requests[3].kind==PTZ_NEUTRAL);
 ack_last(&f,BASE+5);CHECK(view(&f).state==PTZ_IDLE);
 b=move(&f.onvif,1);CHECK(ptz_owner_move(&f.owner,&b,BASE+5)==PTZ_OK);
 CHECK(quiesce_at(&f,PTZ_SLIP,BASE+6)==PTZ_OK);CHECK(f.fake.count==4);
 CHECK(view(&f).position_reason==PTZ_SLIP);
}
static void policy_refusal(void)
{
 struct fixture f;struct ptz_move m;char unknown[CAMERA_ID_BYTES],user[CAMERA_ID_BYTES];
 START();setup(&f);id(unknown,"PUBLICDUMMY-not-associated");id(user,"PUBLICDUMMY-operator");
 m=move(&f.rtsp,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_DENIED);
 m=move(&f.ui,1);m.principal.snapshot_serial++;CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_DENIED);
 m=move(&f.ui,1);m.principal.fact.verified=0;CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_DENIED);
 m=move(&f.ui,1);memcpy(m.sensor,unknown,CAMERA_ID_BYTES);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_DENIED);
 CHECK(stop_at(&f,&f.rtsp,unknown,BASE)==PTZ_DENIED);CHECK(f.fake.count==0);
 CHECK(camera_access_revoke_account(&f.policy,CAMERA_ACCESS_RTSP,user)==CAMERA_ACCESS_OK);
 m=move(&f.onvif,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_DENIED);
 m=move(&f.ui,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);CHECK(f.fake.count==0);
}
static void revoke_and_expire(void)
{
 struct fixture f;struct ptz_move m;uint32_t stage,i;struct ptz_completion c;
 START();
 for(stage=0;stage<3;stage++)for(i=0;i<20;i++){
  setup(&f);m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
  if(stage){CHECK(poll_at(&f,BASE)==PTZ_OK);if(stage==2)ack_last(&f,BASE+1);}
  CHECK(camera_access_revoke_session(&f.policy,&f.ui)==CAMERA_ACCESS_OK);
  CHECK(poll_at(&f,BASE+2+i)==PTZ_DENIED);
  if(stage){
   CHECK(f.fake.count==2&&f.fake.requests[1].kind==PTZ_NEUTRAL);
   CHECK(stop_at(&f,&f.ui,m.sensor,BASE+2+i)==PTZ_DENIED);
   CHECK(f.fake.count==2);ack_last(&f,BASE+23);CHECK(!view(&f).retained);
  }else{CHECK(f.fake.count==0&&!view(&f).retained);}
 }
 setup(&f);m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
 CHECK(poll_at(&f,BASE)==PTZ_OK);c=completion(&f.fake.requests[0],PTZ_ACK);
 CHECK(complete_at(&f,&c,BASE+PTZ_LEASE_MAX_NS)==PTZ_RETAINED);
 CHECK(f.fake.count==2&&f.fake.requests[1].kind==PTZ_NEUTRAL);CHECK(view(&f).fault==1);
 ack_last(&f,BASE+PTZ_LEASE_MAX_NS+1);
 setup(&f);CHECK(camera_access_revoke_session(&f.policy,&f.ui)==CAMERA_ACCESS_OK);
 CHECK(issue(&f,CAMERA_ACCESS_UI,2,BASE+10,&f.ui)==CAMERA_ACCESS_OK);
 m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
 CHECK(poll_at(&f,BASE)==PTZ_OK);CHECK(poll_at(&f,BASE+10)==PTZ_DENIED);
 CHECK(f.fake.count==2);ack_last(&f,BASE+11);CHECK(!view(&f).retained);
}
static void stale_and_generation(void)
{
 struct fixture f;struct ptz_move m;struct ptz_completion c;struct ptz_view before,after;uint32_t i;
 START();setup(&f);m=move(&f.ui,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
 CHECK(poll_at(&f,BASE)==PTZ_OK);c=completion(&f.fake.requests[0],PTZ_ACK);before=view(&f);
 for(i=0;i<4;i++){
  struct ptz_completion bad=c;
  if(i==0)bad.ticket.space++;
  if(i==1)bad.ticket.generation++;
  if(i==2)bad.ticket.sequence++;
  if(i==3)bad.ticket.head[0]='Z';
  bad.electrical_valid=17;
  CHECK(complete_at(&f,&bad,BASE)==PTZ_STALE);after=view(&f);
  CHECK(!memcmp(&before,&after,sizeof(before)));CHECK(f.fake.count==1);
 }
 CHECK(complete_at(&f,&c,BASE+1)==PTZ_OK);CHECK(complete_at(&f,&c,BASE+1)==PTZ_STALE);
 CHECK(poll_at(&f,BASE+PTZ_PHASE_NS)==PTZ_OK);ack_last(&f,BASE+PTZ_PHASE_NS+1);
 CHECK(ptz_owner_reconfigure(&f.owner,&f.config,1,BASE+PTZ_PHASE_NS+1)==PTZ_STALE);
 CHECK(ptz_owner_reconfigure(&f.owner,&f.config,2,BASE+PTZ_PHASE_NS+1)==PTZ_OK);
 CHECK(view(&f).position_reason==PTZ_REBOOT&&!view(&f).position_known);
 CHECK(complete_at(&f,&c,BASE+PTZ_PHASE_NS+2)==PTZ_STALE);
}
static void uncertain_retention(void)
{
 struct fixture f;struct ptz_move m;struct ptz_completion c;struct ptz_owner other={0};uint32_t i;
 START();
 for(i=0;i<4;i++){
  setup(&f);f.fake.phase_result=i==0?PTZ_UNCERTAIN:PTZ_ACCEPTED;
  f.fake.stop_result=i==1?PTZ_UNCERTAIN:PTZ_ACCEPTED;
  m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
  CHECK(poll_at(&f,BASE)==(i==0?PTZ_RETAINED:PTZ_OK));
  if(i==2){c=completion(&f.fake.requests[0],PTZ_UNKNOWN);CHECK(complete_at(&f,&c,BASE+1)==PTZ_RETAINED);}
  if(!f.owner.stop_sent)CHECK(dispose_at(&f,BASE+2)==PTZ_RETAINED);
  CHECK(f.fake.count==2);CHECK(view(&f).retained==1);
  CHECK(ptz_owner_init(&other,&f.registry,&f.auth,&f.provider,&f.config,2,BASE+2)==PTZ_BUSY);
  CHECK(ptz_owner_init(&f.owner,&f.registry,&f.auth,&f.provider,&f.config,2,BASE+2)==PTZ_INVALID);
  {
   int ret=ptz_owner_reconfigure(&f.owner,&f.config,2,BASE+2);
   if(ret!=PTZ_BUSY)fprintf(stderr,"uncertain case=%u reconfigure=%d last=%llu clock_failed=%u owned=%u\n",
    i,ret,(unsigned long long)f.owner.view.last_ns,f.owner.clock_failed,f.owner.owned);
   CHECK(ret==PTZ_BUSY);
  }
  CHECK(poll_at(&f,BASE+PTZ_STOP_MAX_NS+4)==PTZ_RETAINED);
  CHECK(dispose_at(&f,BASE+PTZ_STOP_MAX_NS+5)==PTZ_RETAINED);
  CHECK(f.fake.count==2);c=completion(&f.fake.requests[1],PTZ_UNKNOWN);
  CHECK(complete_at(&f,&c,BASE+PTZ_STOP_MAX_NS+6)==PTZ_RETAINED);CHECK(f.fake.count==2);
  c=completion(&f.fake.requests[1],PTZ_ACK);CHECK(complete_at(&f,&c,BASE+PTZ_STOP_MAX_NS+7)==PTZ_OK);
  CHECK(!view(&f).retained&&view(&f).state==PTZ_DISPOSED);
  CHECK(complete_at(&f,&c,BASE+PTZ_STOP_MAX_NS+8)==PTZ_INVALID);
 }
 setup(&f);m=move(&f.ui,2);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);CHECK(poll_at(&f,BASE)==PTZ_OK);
 CHECK(dispose_at(&f,BASE+1)==PTZ_RETAINED);c=completion(&f.fake.requests[1],PTZ_FAILED);
 CHECK(complete_at(&f,&c,BASE+2)==PTZ_RETAINED);c.result=PTZ_ACK;
 CHECK(complete_at(&f,&c,BASE+3)==PTZ_STALE);CHECK(view(&f).retained&&f.fake.count==2);
}
static void unsafe_and_invalid(void)
{
 struct fixture f;struct ptz_move m;uint32_t bits,i;
 START();
 for(bits=0;bits<15;bits++){
  setup(&f);f.owner.provider.readiness=bits;m=move(&f.ui,1);
  CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_UNSAFE);
  CHECK(quiesce_at(&f,PTZ_SLIP,BASE)==PTZ_OK);CHECK(f.fake.count==0);
 }
 for(i=0;i<8;i++){
  setup(&f);m=move(&f.ui,1);
  if(i==0)m.phases=0;
  if(i==1)m.phases=5;
  if(i==2)m.axis=2;
  if(i==3)m.direction=0;
  if(i==4)m.direction=2;
  if(i==5)m.lease_ns=UINT64_MAX;
  if(i==6)m.lease_ns=PTZ_PHASE_NS-1;
  if(i==7)m.sensor[63]='Z';
  CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_INVALID);CHECK(f.fake.count==0);
 }
 setup(&f);f.owner.provider.callback_bound_ns=0;m=move(&f.ui,1);
 CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_UNSAFE);
 setup(&f);f.owner.provider.stop_bound_ns=PTZ_STOP_MAX_NS+1;m=move(&f.ui,1);
 CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_UNSAFE);
}
static void clock_and_exhaustion(void)
{
 struct fixture f;struct ptz_move m;uint64_t n;struct ptz_view v;
 START();setup(&f);m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
 CHECK(poll_at(&f,BASE)==PTZ_OK);CHECK(poll_at(&f,BASE-1)==PTZ_FAULT);
 CHECK(f.fake.count==2&&view(&f).retained);ack_last(&f,BASE+1);CHECK(view(&f).fault);
 CHECK(ptz_owner_move(&f.owner,&m,BASE+2)==PTZ_FAULT);
 setup(&f);m=move(&f.ui,4);n=f.registry.heads[0].next_sequence;
 f.registry.heads[0].next_sequence=UINT64_MAX-5;
 CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_EXHAUSTED);CHECK(f.fake.count==0);
 f.registry.heads[0].next_sequence=UINT64_MAX-6;
 CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);CHECK(f.registry.heads[0].next_sequence==UINT64_MAX);
 CHECK(poll_at(&f,BASE)==PTZ_OK);CHECK(quiesce_at(&f,PTZ_SLIP,BASE+1)==PTZ_RETAINED);
 CHECK(f.fake.requests[1].ticket.sequence==UINT64_MAX-1);ack_last(&f,BASE+2);
 CHECK(ptz_owner_move(&f.owner,&m,BASE+3)==PTZ_EXHAUSTED);CHECK(f.fake.count==2);
 CHECK(n==1);
 setup(&f);CHECK(camera_access_revoke_session(&f.policy,&f.ui)==CAMERA_ACCESS_OK);
 CHECK(issue(&f,CAMERA_ACCESS_UI,2,UINT64_MAX,&f.ui)==CAMERA_ACCESS_OK);
 m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,UINT64_MAX-3)==PTZ_EXHAUSTED);
 CHECK(f.fake.count==0);CHECK(poll_at(&f,0)==PTZ_FAULT);
 setup(&f);CHECK(ptz_owner_reconfigure(&f.owner,&f.config,UINT64_MAX,BASE)==PTZ_OK);
 CHECK(ptz_owner_reconfigure(&f.owner,&f.config,0,BASE)==PTZ_INVALID);
 CHECK(ptz_owner_reconfigure(&f.owner,&f.config,UINT64_MAX,BASE)==PTZ_STALE);
 setup(&f);m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
 f.fake.clock_jump=PTZ_LEASE_MAX_NS;CHECK(poll_at(&f,BASE)==PTZ_FAULT);CHECK(f.fake.count==0);
 v=view(&f);CHECK(v.fault&&!v.retained);
}
static void pointer_aliases(void)
{
 struct fixture f;struct ptz_move m;struct ptz_owner copied;struct ptz_registry copy;
 START();setup(&f);m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);CHECK(poll_at(&f,BASE)==PTZ_OK);
 CHECK(ptz_owner_move(&f.owner,&f.owner.job.move,BASE)==PTZ_INVALID);
 CHECK(f.fake.count==1&&view(&f).retained&&!view(&f).fault);
 CHECK(ptz_owner_view(&f.owner,&f.owner.view)==PTZ_INVALID);
 CHECK(ptz_owner_complete(&f.owner,(const struct ptz_completion *)&f.owner.job,BASE)==PTZ_INVALID);
 CHECK(f.fake.count==2);copied=f.owner;CHECK(ptz_owner_poll(&copied,BASE)==PTZ_INVALID);
 copy=f.registry;CHECK(ptz_registry_init(&copy,2)==PTZ_INVALID);
 CHECK(ptz_registry_init(&f.registry,2)==PTZ_INVALID);
 setup(&f);CHECK(ptz_auth_check(&f.auth,&f.policy.sessions[0].principal,f.associations[0].camera_id,BASE)==CAMERA_ACCESS_DENIED);
 CHECK(stop_at(&f,&f.policy.sessions[0].principal,f.associations[0].camera_id,BASE)==PTZ_INVALID);
 CHECK(f.fake.count==0);
 CHECK(ptz_overlap((const void *)(uintptr_t)(UINTPTR_MAX-2),8,&f,1));
}
static void callback_counterexample(void)
{
 struct fixture f;struct ptz_move m;struct ptz_view v;
 START();setup(&f);f.fake.latency=UINT64_C(80000000);f.fake.reenter=1;
 m=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
 CHECK(poll_at(&f,BASE)==PTZ_RETAINED);CHECK(f.fake.reentered==PTZ_INVALID);
 CHECK(f.fake.count==2&&f.fake.requests[1].kind==PTZ_NEUTRAL);
 v=view(&f);CHECK(v.fault&&v.retained&&v.state==PTZ_QUARANTINED);
 CHECK(v.stop_requested_ns>=BASE+UINT64_C(80000000));CHECK(!v.native_qualified&&!v.position_known);
 printf("counterexample=blocking80ms-provider delays STOP until callback returns; logical ticket is NOT physical STOP proof\n");
 setup(&f);f.fake.reenter=1;m=move(&f.ui,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
 CHECK(poll_at(&f,BASE)==PTZ_OK);CHECK(f.fake.reentered==PTZ_INVALID);CHECK(f.fake.count==1);
}
static void observations_and_lifecycle(void)
{
 struct fixture f;struct ptz_move m;struct ptz_completion c;struct ptz_owner next={0};uint32_t i;
 START();setup(&f);m=move(&f.ui,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);CHECK(poll_at(&f,BASE)==PTZ_OK);
 c=completion(&f.fake.requests[0],PTZ_ACK);c.electrical_valid=1;c.observed_phase_bits=7;
 CHECK(complete_at(&f,&c,BASE+1)==PTZ_RETAINED);CHECK(f.fake.count==2);
 c=completion(&f.fake.requests[1],PTZ_ACK);c.electrical_valid=1;c.observed_neutral=0;
 CHECK(complete_at(&f,&c,BASE+2)==PTZ_RETAINED);CHECK(view(&f).retained);
 setup(&f);m=move(&f.ui,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);CHECK(poll_at(&f,BASE)==PTZ_OK);
 ack_last(&f,BASE+1);CHECK(poll_at(&f,BASE+PTZ_PHASE_NS)==PTZ_OK);
 c=completion(&f.fake.requests[1],PTZ_ACK);c.electrical_valid=1;c.observed_neutral=1;
 CHECK(complete_at(&f,&c,BASE+PTZ_PHASE_NS+1)==PTZ_OK);
 CHECK(view(&f).electrical_valid&&view(&f).observed_neutral&&!view(&f).native_qualified);
 CHECK(dispose_at(&f,BASE+PTZ_PHASE_NS+2)==PTZ_OK);
 CHECK(ptz_owner_init(&next,&f.registry,&f.auth,&f.provider,&f.config,1,BASE+PTZ_PHASE_NS+2)==PTZ_STALE);
 CHECK(ptz_owner_init(&next,&f.registry,&f.auth,&f.provider,&f.config,2,BASE+PTZ_PHASE_NS+2)==PTZ_OK);
 CHECK(next.view.generation==2&&f.registry.heads[0].next_sequence>1);
 CHECK(ptz_owner_dispose(&next,BASE+PTZ_PHASE_NS+3)==PTZ_OK);
 for(i=0;i<100;i++){
  setup(&f);m=move(&f.ui,1);CHECK(ptz_owner_move(&f.owner,&m,BASE)==PTZ_OK);
  CHECK(poll_at(&f,BASE)==PTZ_OK);CHECK(dispose_at(&f,BASE+1)==PTZ_RETAINED);
  CHECK(view(&f).state==PTZ_STOP_PENDING);ack_last(&f,BASE+2);
  CHECK(view(&f).state==PTZ_DISPOSED&&f.fake.count==2);
 }
}
static void graph_and_independent_heads(void)
{
 struct fixture a,b;struct ptz_auth binding={0};struct ptz_owner second={0};
 struct ptz_move ma,mb;struct ptz_completion ca,cb;uint32_t i;
 char head[CAMERA_ID_BYTES];
 START();setup(&a);id(head,"PUBLICDUMMY-head0");
 a.graph.association_count=0;
 CHECK(ptz_auth_init(&binding,&a.policy,&a.graph,head)==CAMERA_INVALID);
 a.graph.association_count=2;a.devices[0].state=CAMERA_UNAVAILABLE;
 CHECK(ptz_auth_init(&binding,&a.policy,&a.graph,head)==CAMERA_INVALID);
 a.devices[0].state=CAMERA_PRESENT_UNQUALIFIED;
 a.associations[1]=a.associations[0];
 CHECK(ptz_auth_init(&binding,&a.policy,&a.graph,head)==CAMERA_INVALID);
 id(a.associations[1].camera_id,"PUBLICDUMMY-sensor1");a.associations[1].camera_id[63]='Z';
 CHECK(ptz_auth_init(&binding,&a.policy,&a.graph,head)==CAMERA_INVALID);
 id(a.associations[1].camera_id,"PUBLICDUMMY-sensor1");
 CHECK(ptz_auth_init(&binding,&a.policy,&a.graph,head)==CAMERA_OK);
 CHECK(ptz_auth_init(&binding,&a.policy,&a.graph,head)==CAMERA_INVALID);
 setup_head(&b,"PUBLICDUMMY-head1");
 CHECK(dispose_at(&b,BASE)==PTZ_OK);
 CHECK(ptz_owner_init(&second,&a.registry,&b.auth,&b.provider,&b.config,1,BASE)==PTZ_OK);
 ma=move(&a.ui,2);mb=move(&b.ui,2);
 CHECK(ptz_owner_move(&a.owner,&ma,BASE)==PTZ_OK);
 CHECK(ptz_owner_move(&second,&mb,BASE)==PTZ_OK);
 CHECK(poll_at(&a,BASE)==PTZ_OK);CHECK(ptz_owner_poll(&second,BASE)==PTZ_OK);
 CHECK(a.fake.count==1&&b.fake.count==1);
 ca=completion(&a.fake.requests[0],PTZ_ACK);cb=completion(&b.fake.requests[0],PTZ_ACK);
 CHECK(ca.ticket.space==cb.ticket.space&&ca.ticket.sequence==cb.ticket.sequence);
 CHECK(!ptz_ticket_equal(&ca.ticket,&cb.ticket));
 CHECK(complete_at(&a,&cb,BASE)==PTZ_STALE);CHECK(ptz_owner_complete(&second,&ca,BASE)==PTZ_STALE);
 CHECK(quiesce_at(&a,PTZ_SLIP,BASE+1)==PTZ_RETAINED);
 CHECK(a.fake.count==2&&b.fake.count==1);ack_last(&a,BASE+2);
 b.fake.now=BASE+2;CHECK(ptz_owner_complete(&second,&cb,BASE+2)==PTZ_OK);
 CHECK(ptz_owner_dispose(&second,BASE+2)==PTZ_RETAINED);CHECK(b.fake.count==2);
 cb=completion(&b.fake.requests[1],PTZ_ACK);b.fake.now=BASE+3;
 CHECK(ptz_owner_complete(&second,&cb,BASE+3)==PTZ_OK);
 CHECK(second.view.state==PTZ_DISPOSED&&!second.view.native_qualified);
 /* Intentional white-box bound injection: tombstones cannot be evicted to
  * create more untracked head identities. No physical topology inference. */
 for(i=0;i<PTZ_HEAD_MAX;i++){
  a.registry.heads[i].used=1;a.registry.heads[i].owner=NULL;
  id(a.registry.heads[i].head,"PUBLICDUMMY-tombstone");a.registry.heads[i].head[20]=(char)('0'+i);
 }
 memset(&second,0,sizeof(second));
 CHECK(ptz_owner_init(&second,&a.registry,&b.auth,&b.provider,&b.config,2,BASE+3)==PTZ_EXHAUSTED);
}
static void priority_near_cadence_and_rotation(void)
{
 struct fixture f;struct ptz_move a,b;struct ptz_completion c;uint32_t i;uint64_t t;
 START();
 for(i=0;i<20;i++){
  setup(&f);a=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&a,BASE)==PTZ_OK);
  CHECK(poll_at(&f,BASE)==PTZ_OK);ack_last(&f,BASE+1);
  t=BASE+PTZ_PHASE_NS-1;f.fake.now=t;b=move(&f.ui,4);
  CHECK(ptz_owner_move(&f.owner,&b,t)==PTZ_OK);CHECK(f.fake.count==2);
  CHECK(f.fake.requests[1].kind==PTZ_NEUTRAL);
  CHECK(camera_access_revoke_session(&f.policy,&f.ui)==CAMERA_ACCESS_OK);
  CHECK(poll_at(&f,BASE+PTZ_PHASE_NS)==PTZ_OK);CHECK(f.fake.count==2);
  c=completion(&f.fake.requests[1],PTZ_ACK);
  CHECK(complete_at(&f,&c,BASE+PTZ_PHASE_NS+1)==PTZ_OK);
  CHECK(!view(&f).queued&&!view(&f).retained&&view(&f).state==PTZ_IDLE);
  CHECK(complete_at(&f,&c,BASE+PTZ_PHASE_NS+1)==PTZ_STALE);
 }
 setup(&f);a=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&a,BASE)==PTZ_OK);
 CHECK(poll_at(&f,BASE)==PTZ_OK);CHECK(camera_access_revoke_session(&f.policy,&f.ui)==CAMERA_ACCESS_OK);
 CHECK(issue(&f,CAMERA_ACCESS_UI,2,BASE+UINT64_C(1000000000),&f.ui)==CAMERA_ACCESS_OK);
 b=move(&f.ui,1);CHECK(ptz_owner_move(&f.owner,&b,BASE)==PTZ_BUSY);
 CHECK(poll_at(&f,BASE+1)==PTZ_DENIED);CHECK(f.fake.count==2);ack_last(&f,BASE+2);
 CHECK(ptz_owner_move(&f.owner,&b,BASE+2)==PTZ_OK);CHECK(poll_at(&f,BASE+2)==PTZ_OK);
 CHECK(f.fake.count==3&&f.fake.requests[2].kind==PTZ_PHASE);
 CHECK(quiesce_at(&f,PTZ_REBOOT,BASE+3)==PTZ_RETAINED);CHECK(f.fake.count==4);
 CHECK(quiesce_at(&f,PTZ_REBOOT,BASE+4)==PTZ_RETAINED);CHECK(f.fake.count==4);
 ack_last(&f,BASE+5);CHECK(view(&f).position_reason==PTZ_REBOOT);
 setup(&f);f.fake.phase_result=PTZ_NO_EFFECT;a=move(&f.ui,1);
 CHECK(ptz_owner_move(&f.owner,&a,BASE)==PTZ_OK);CHECK(poll_at(&f,BASE)==PTZ_FAULT);
 CHECK(f.fake.count==1&&!view(&f).retained);CHECK(dispose_at(&f,BASE+1)==PTZ_OK);
}
static struct camera_access_principal client_principal(struct fixture *f,const char *name,
 int head_grant,uint64_t expires,int revoke)
{
 struct camera_access_account a;struct camera_access_verified v;struct camera_access_principal p;
 memset(&a,0,sizeof(a));a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);
 a.domain=CAMERA_ACCESS_UI;a.credential_generation=1;a.grant_count=1;id(a.account_id,name);
 a.roles=head_grant?CAMERA_ACCESS_OPERATOR:CAMERA_ACCESS_VIEWER;
 a.grants[0].kind=head_grant?CAMERA_ACCESS_PTZ:CAMERA_ACCESS_PROFILE;
 a.grants[0].operations=head_grant?CAMERA_ACCESS_PTZ_CONTROL:CAMERA_ACCESS_LIVE;
 id(a.grants[0].resource_id,head_grant?"PUBLICDUMMY-head0":"PUBLICDUMMY-profile0");
 CHECK(camera_access_put_account(&f->policy,&a)==CAMERA_ACCESS_OK);
 memset(&v,0,sizeof(v));v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;
 v.domain=CAMERA_ACCESS_UI;v.boot_generation=77;v.credential_generation=1;v.session_generation=1;
 v.issued_ns=BASE;v.expires_ns=expires;id(v.device_id,"PUBLICDUMMY-device");id(v.account_id,name);
 CHECK(camera_access_issue(&f->policy,&v,BASE,&p)==CAMERA_ACCESS_OK);
 if(revoke)CHECK(camera_access_revoke_session(&f->policy,&p)==CAMERA_ACCESS_OK);
 return p;
}
static void client_inert(struct fixture *f,const unsigned char *owner,const unsigned char *registry,
 uint32_t count,uint64_t serial,uint32_t stage,uint32_t actor,uint32_t mutation,const char *operation)
{
 if(f->fake.count!=count||memcmp(owner,&f->owner,sizeof(f->owner))||
    memcmp(registry,&f->registry,sizeof(f->registry)))
  fprintf(stderr,"client-authority regression stage=%u actor=%u %s mutation=%u submits_before=%u after=%u fault=%u\n",
   stage,actor,operation,mutation,count,f->fake.count,f->owner.view.fault);
 CHECK(f->fake.count==count);
 CHECK(!memcmp(owner,&f->owner,sizeof(f->owner)));
 CHECK(!memcmp(registry,&f->registry,sizeof(f->registry)));
 CHECK(f->policy.next_serial==serial); /* Neither owner ticket nor policy issuance reservation. */
 CHECK(!f->owner.view.fault&&f->owner.active);
}
static void client_authority_boundary(void)
{
 struct fixture f;struct camera_access_principal actors[8];struct ptz_move a,m;
 unsigned char owner_before[sizeof(struct ptz_owner)],registry_before[sizeof(struct ptz_registry)];
 uint32_t stage_index,stage,actor,mutation,count;uint64_t serial;
 char sensor[CAMERA_ID_BYTES];const struct ptz_move *input;
 START();
 /* Begin with an outstanding applied request: the V1 null-client regression
  * must reproduce its unauthorized NEUTRAL, not just an idle fault latch. */
 for(stage_index=1;stage_index<=4;stage_index++){
  stage=stage_index%4;setup(&f);
  actors[0]=f.ui;
  actors[1]=client_principal(&f,"PUBLICDUMMY-revoked-B",1,BASE+UINT64_C(1000000000),1);
  actors[2]=f.rtsp;
  actors[3]=client_principal(&f,"PUBLICDUMMY-viewer-B",0,BASE+UINT64_C(1000000000),0);
  actors[4]=client_principal(&f,"PUBLICDUMMY-operator-B",1,BASE+UINT64_C(1000000000),0);
  actors[5]=f.onvif;
  actors[6]=f.ui;actors[6].snapshot_serial++;
  actors[7]=client_principal(&f,"PUBLICDUMMY-expired-B",1,BASE+1,0);
  a=move(&f.ui,4);CHECK(ptz_owner_move(&f.owner,&a,BASE)==PTZ_OK);
  if(stage){CHECK(poll_at(&f,BASE)==PTZ_OK);}
  if(stage==2)ack_last(&f,BASE+1);
  if(stage==3){CHECK(ptz_owner_move(&f.owner,&a,BASE)==PTZ_OK);CHECK(view(&f).queued==1);}
  for(actor=0;actor<8;actor++){
   for(mutation=0;mutation<18;mutation++){
    m=move(&actors[actor],1);input=&m;
    switch(mutation){
    case 0:input=NULL;break;
    case 1:input=&f.owner.job.move;break; /* accessible typed object, prohibited alias */
    case 2:m.axis=UINT32_MAX;break;
    case 3:m.axis=2;break;
    case 4:m.direction=0;break;
    case 5:m.direction=INT32_MIN;break;
    case 6:m.phases=0;break;
    case 7:m.phases=5;break;
    case 8:m.phases=UINT32_MAX;break;
    case 9:m.lease_ns=0;break;
    case 10:m.lease_ns=PTZ_LEASE_MAX_NS+1;break;
    case 11:m.lease_ns=PTZ_PHASE_NS-1;break;
    case 12:memset(m.sensor,'Z',CAMERA_ID_BYTES);break;
    case 13:id(m.sensor,"PUBLICDUMMY-stale-sensor");break;
    case 14:memset(m.sensor,0,CAMERA_ID_BYTES);break;
    case 15:m.sensor[0]='/';break;
    case 16:m.sensor[63]='Z';break;
    case 17:m.direction=INT32_MAX;break;
    default:CHECK(0);break;
    }
    f.fake.now=BASE+2;count=f.fake.count;serial=f.policy.next_serial;
    memcpy(owner_before,&f.owner,sizeof(f.owner));memcpy(registry_before,&f.registry,sizeof(f.registry));
    CHECK(ptz_owner_move(&f.owner,input,BASE+2)==(mutation==13?PTZ_DENIED:PTZ_INVALID));
    client_inert(&f,owner_before,registry_before,count,serial,stage,actor,mutation,"MOVE");
   }
   for(mutation=0;mutation<10;mutation++){
    const struct camera_access_principal *p=&actors[actor];const char *s=sensor;
    id(sensor,"PUBLICDUMMY-sensor0");
    switch(mutation){
    case 0:p=NULL;break;
    case 1:s=NULL;break;
    case 2:p=&f.owner.job.move.principal;break;
    case 3:p=&f.policy.sessions[0].principal;break;
    case 4:s=f.owner.job.move.sensor;break;
    case 5:memset(sensor,'Z',CAMERA_ID_BYTES);break;
    case 6:sensor[0]='/';break;
    case 7:memset(sensor,0,CAMERA_ID_BYTES);break;
    case 8:sensor[63]='Z';break;
    case 9:id(sensor,"PUBLICDUMMY-stale-sensor");break;
    default:CHECK(0);break;
    }
    f.fake.now=BASE+2;count=f.fake.count;serial=f.policy.next_serial;
    memcpy(owner_before,&f.owner,sizeof(f.owner));memcpy(registry_before,&f.registry,sizeof(f.registry));
    CHECK(ptz_owner_stop(&f.owner,p,s,BASE+2)==(mutation==9?PTZ_DENIED:PTZ_INVALID));
    client_inert(&f,owner_before,registry_before,count,serial,stage,actor,mutation,"STOP");
   }
   if(actor!=0){
    int expected=(actor==4||actor==5)?PTZ_BUSY:PTZ_DENIED;
    m=move(&actors[actor],1);count=f.fake.count;serial=f.policy.next_serial;
    memcpy(owner_before,&f.owner,sizeof(f.owner));memcpy(registry_before,&f.registry,sizeof(f.registry));
    CHECK(ptz_owner_move(&f.owner,&m,BASE+2)==expected);
    client_inert(&f,owner_before,registry_before,count,serial,stage,actor,18,"valid MOVE refusal");
   }
   if(actor!=0&&actor!=4&&actor!=5){
    id(sensor,"PUBLICDUMMY-sensor0");count=f.fake.count;serial=f.policy.next_serial;
    memcpy(owner_before,&f.owner,sizeof(f.owner));memcpy(registry_before,&f.registry,sizeof(f.registry));
    CHECK(ptz_owner_stop(&f.owner,&actors[actor],sensor,BASE+2)==PTZ_DENIED);
    client_inert(&f,owner_before,registry_before,count,serial,stage,actor,10,"valid STOP refusal");
   }
  }
  CHECK(f.owner.owned==(stage!=0));CHECK(!f.owner.view.fault);
  /* Legitimate A still progresses: queued -> phase, pending -> ACK/next phase,
   * or retained replacement -> exact STOP ACK/new phase. */
  if(stage==0){CHECK(poll_at(&f,BASE+3)==PTZ_OK);}
  if(stage==1){ack_last(&f,BASE+3);CHECK(poll_at(&f,BASE+PTZ_PHASE_NS)==PTZ_OK);}
  if(stage==2){CHECK(poll_at(&f,BASE+PTZ_PHASE_NS)==PTZ_OK);}
  if(stage==3){ack_last(&f,BASE+3);CHECK(poll_at(&f,BASE+4)==PTZ_OK);}
  CHECK(f.owner.owned&&!f.owner.view.fault);
  count=f.fake.count;id(sensor,"PUBLICDUMMY-sensor0");
  CHECK(stop_at(&f,&actors[4],sensor,BASE+PTZ_PHASE_NS+1)==PTZ_RETAINED);
  CHECK(f.fake.count==count+1&&f.fake.requests[count].kind==PTZ_NEUTRAL);
  CHECK(!f.owner.view.fault);ack_last(&f,BASE+PTZ_PHASE_NS+2);
  CHECK(!f.owner.owned&&!f.owner.view.fault&&!f.owner.active);
 }
 printf("client-authority=PASS inert malformed/denied input across active-A/queued/replacement states; real policy and explicit granted STOP\n");
}
int main(int argc,char **argv)
{
 if(argc==2&&!strcmp(argv[1],"--client-boundary-only")){
  client_authority_boundary();
  printf("HOST PUBLICDUMMY client-boundary PASS suites=%u checks=%u\n",suites,checks);return 0;
 }
 CHECK(argc==1);
 normal_sequence();arbitration();policy_refusal();revoke_and_expire();stale_and_generation();
 uncertain_retention();unsafe_and_invalid();clock_and_exhaustion();pointer_aliases();
 callback_counterexample();observations_and_lifecycle();graph_and_independent_heads();priority_near_cadence_and_rotation();
 client_authority_boundary();
 printf("HOST PUBLICDUMMY PASS suites=%u checks=%u owner_bytes=%zu registry_bytes=%zu auth_bytes=%zu\n",
  suites,checks,sizeof(struct ptz_owner),sizeof(struct ptz_registry),sizeof(struct ptz_auth));
 return 0;
}
