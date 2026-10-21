/* SPDX-License-Identifier: GPL-2.0-only
 * Memory-only SEC-01/03 authorization fixtures. No real authentication/target. */
#include <camera/access_policy.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do {checks++;assert(x);} while(0)
#define OK(x) CHECK((x)==CAMERA_ACCESS_OK)
#define DENY(x) CHECK((x)==CAMERA_ACCESS_DENIED)
#define INVALID(x) CHECK((x)==CAMERA_ACCESS_INVALID)
static const char device[CAMERA_ID_BYTES]="fixture-device";
static const char alice[CAMERA_ID_BYTES]="alice";
/* Same physical IDs/profiles/shared head as backend_contract_test.c. Synthetic
 * device ID, not a measured camera identifier. Profile != physical sensor. */
static const struct camera_access_resource resources[]={
 {CAMERA_ACCESS_DEVICE,"fixture-device",""},
 {CAMERA_ACCESS_SENSOR,"camera0",""},{CAMERA_ACCESS_SENSOR,"camera1",""},
 {CAMERA_ACCESS_PROFILE,"camera0-main","camera0"},
 {CAMERA_ACCESS_PROFILE,"camera0-sub","camera0"},
 {CAMERA_ACCESS_PROFILE,"camera1-main","camera1"},
 {CAMERA_ACCESS_PTZ,"head0",""}
};
static void id(char dst[CAMERA_ID_BYTES],const char *s)
{
 CHECK(strlen(s)<CAMERA_ID_BYTES);memset(dst,0,CAMERA_ID_BYTES);strcpy(dst,s);
}
static struct camera_access_grant grant(uint32_t kind,const char *name,uint32_t ops)
{
 struct camera_access_grant g={0};g.kind=kind;id(g.resource_id,name);g.operations=ops;return g;
}
static struct camera_access_account account(uint32_t domain)
{
 struct camera_access_account a={0};
 a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=domain;
 id(a.account_id,"alice");a.credential_generation=1;
 switch(domain){
 case CAMERA_ACCESS_UI:
  a.roles=CAMERA_ACCESS_VIEWER|CAMERA_ACCESS_OPERATOR|CAMERA_ACCESS_RECORDING_READER;
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PROFILE,"camera0-main",CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_SENSOR,"camera0",CAMERA_ACCESS_EVENTS|CAMERA_ACCESS_IMAGING_CONFIG);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_SENSOR,"camera1",CAMERA_ACCESS_PLAYBACK|CAMERA_ACCESS_DOWNLOAD);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PTZ,"head0",CAMERA_ACCESS_PTZ_CONTROL);break;
 case CAMERA_ACCESS_ADMIN:
  a.roles=CAMERA_ACCESS_VIEWER|CAMERA_ACCESS_OPERATOR|CAMERA_ACCESS_CONFIGURATOR|
   CAMERA_ACCESS_RECORDING_READER|CAMERA_ACCESS_RECORDING_MANAGER|
   CAMERA_ACCESS_RESET_REQUESTER|CAMERA_ACCESS_CREDENTIAL_MANAGER;
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_DEVICE,"fixture-device",CAMERA_ACCESS_DEVICE_CONFIG|
   CAMERA_ACCESS_SETTINGS_RESET_REQUEST|CAMERA_ACCESS_CREDENTIAL_CONFIG);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_SENSOR,"camera0",CAMERA_ACCESS_EVENTS|
   CAMERA_ACCESS_IMAGING_CONFIG|CAMERA_ACCESS_RECORDING_CONFIG|CAMERA_ACCESS_PLAYBACK|CAMERA_ACCESS_DOWNLOAD);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_SENSOR,"camera1",CAMERA_ACCESS_EVENTS|
   CAMERA_ACCESS_IMAGING_CONFIG|CAMERA_ACCESS_RECORDING_CONFIG|CAMERA_ACCESS_PLAYBACK|CAMERA_ACCESS_DOWNLOAD);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PROFILE,"camera0-main",CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PROFILE,"camera0-sub",CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PROFILE,"camera1-main",CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PTZ,"head0",CAMERA_ACCESS_PTZ_CONTROL);break;
 case CAMERA_ACCESS_RTSP:
  a.roles=CAMERA_ACCESS_VIEWER;
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PROFILE,"camera0-main",CAMERA_ACCESS_LIVE);break;
 case CAMERA_ACCESS_ONVIF:
  a.roles=CAMERA_ACCESS_VIEWER|CAMERA_ACCESS_OPERATOR|CAMERA_ACCESS_DISCOVERER;
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PROFILE,"camera0-main",CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PTZ,"head0",CAMERA_ACCESS_PTZ_CONTROL);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_SENSOR,"camera0",CAMERA_ACCESS_EVENTS);
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_DEVICE,"fixture-device",CAMERA_ACCESS_DISCOVERY);break;
 case CAMERA_ACCESS_SETUP:
  a.roles=CAMERA_ACCESS_ONBOARDER;
  a.grants[a.grant_count++]=grant(CAMERA_ACCESS_DEVICE,"fixture-device",CAMERA_ACCESS_ONBOARD);break;
 default:CHECK(0);
 }
 return a;
}
static void init(struct camera_access_policy *p)
{
 OK(camera_access_init(p,device,42,100,resources,sizeof(resources)/sizeof(resources[0])));
}
static void fixture(struct camera_access_policy *p)
{
 uint32_t d;init(p);
 for(d=CAMERA_ACCESS_UI;d<=CAMERA_ACCESS_SETUP;d++){
  struct camera_access_account a=account(d);OK(camera_access_put_account(p,&a));
 }
}
static struct camera_access_verified fact(uint32_t domain,uint64_t session)
{
 struct camera_access_verified f={0};f.abi=CAMERA_ACCESS_ABI;f.struct_bytes=sizeof(f);
 f.verified=1;f.domain=domain;id(f.device_id,"fixture-device");id(f.account_id,"alice");
 f.boot_generation=42;f.credential_generation=1;f.session_generation=session;
 f.issued_ns=100;f.expires_ns=1000;return f;
}
static struct camera_access_principal issue(struct camera_access_policy *p,uint32_t domain,uint64_t generation)
{
 struct camera_access_verified f=fact(domain,generation);struct camera_access_principal s={0};
 OK(camera_access_issue(p,&f,100,&s));return s;
}
static uint32_t kind_for(uint32_t op)
{
 switch(op){
 case CAMERA_ACCESS_LIVE:case CAMERA_ACCESS_SNAPSHOT:return CAMERA_ACCESS_PROFILE;
 case CAMERA_ACCESS_PTZ_CONTROL:return CAMERA_ACCESS_PTZ;
 case CAMERA_ACCESS_EVENTS:case CAMERA_ACCESS_IMAGING_CONFIG:case CAMERA_ACCESS_RECORDING_CONFIG:
 case CAMERA_ACCESS_PLAYBACK:case CAMERA_ACCESS_DOWNLOAD:return CAMERA_ACCESS_SENSOR;
 default:return CAMERA_ACCESS_DEVICE;
 }
}
static struct camera_access_request request(uint32_t domain,uint32_t op)
{
 struct camera_access_request q={0};q.abi=CAMERA_ACCESS_ABI;q.struct_bytes=sizeof(q);
 q.domain=domain;q.kind=kind_for(op);q.operation=op;id(q.device_id,"fixture-device");
 if(q.kind==CAMERA_ACCESS_PROFILE){id(q.sensor_id,"camera0");id(q.profile_id,"camera0-main");}
 if(q.kind==CAMERA_ACCESS_SENSOR)id(q.sensor_id,"camera0");
 if(q.kind==CAMERA_ACCESS_PTZ)id(q.resource_id,"head0");
 if(domain==CAMERA_ACCESS_UI&&(op==CAMERA_ACCESS_PLAYBACK||op==CAMERA_ACCESS_DOWNLOAD))id(q.sensor_id,"camera1");
 return q;
}
static int explicitly_granted(uint32_t domain,uint32_t op)
{
 struct camera_access_account a=account(domain);uint32_t i;
 for(i=0;i<a.grant_count;i++)if(a.grants[i].operations&op)return 1;
 return 0;
}
static void matrix(void)
{
 struct camera_access_policy p;struct camera_access_principal s[5];uint32_t d,target,bit;
 fixture(&p);OK(camera_access_map_nvr(&p,alice,alice));
 for(d=1;d<=5;d++)s[d-1]=issue(&p,d,1);
 /* Every domain, every operation, every requested domain: 325 checks. */
 for(d=1;d<=5;d++)for(target=1;target<=5;target++)for(bit=1;bit<=CAMERA_ACCESS_DISCOVERY;bit<<=1){
  struct camera_access_request q=request(target,bit);
  CHECK(camera_access_check(&p,&s[d-1],&q,100)==
   ((d==target&&explicitly_granted(target,bit))?CAMERA_ACCESS_OK:CAMERA_ACCESS_DENIED));
 }
 /* No operation can be moved to a different scope to acquire parent/alias rights. */
 for(d=1;d<=5;d++)for(bit=1;bit<=CAMERA_ACCESS_DISCOVERY;bit<<=1){
  uint32_t kind;
  for(kind=1;kind<=4;kind++)if(kind!=kind_for(bit)){
   struct camera_access_request q=request(d,bit);
   q.kind=kind;memset(q.sensor_id,0,CAMERA_ID_BYTES);
   memset(q.profile_id,0,CAMERA_ID_BYTES);memset(q.resource_id,0,CAMERA_ID_BYTES);
   if(kind==CAMERA_ACCESS_SENSOR)id(q.sensor_id,"camera0");
   if(kind==CAMERA_ACCESS_PROFILE){id(q.sensor_id,"camera0");id(q.profile_id,"camera0-main");}
   if(kind==CAMERA_ACCESS_PTZ)id(q.resource_id,"head0");
   DENY(camera_access_check(&p,&s[d-1],&q,100));
  }
 }
 /* Shared head authorization does not depend on selecting a profile/sensor. */
 {
  struct camera_access_request q=request(CAMERA_ACCESS_UI,CAMERA_ACCESS_PTZ_CONTROL);
  OK(camera_access_check(&p,&s[0],&q,100));
  id(q.sensor_id,"camera0");DENY(camera_access_check(&p,&s[0],&q,100));
  id(q.sensor_id,"camera1");DENY(camera_access_check(&p,&s[0],&q,100));
 }
 puts("PASS domain/operation matrix; explicit shared PTZ; setup/admin separation");
}
static void invalid_accounts(void)
{
 struct camera_access_policy p;struct camera_access_account a,base=account(CAMERA_ACCESS_UI);unsigned bit;
 fixture(&p);
#define BAD_ACCOUNT(field,value) do {a=base;a.field=(value);INVALID(camera_access_put_account(&p,&a));} while(0)
 BAD_ACCOUNT(abi,0);BAD_ACCOUNT(abi,2);BAD_ACCOUNT(struct_bytes,sizeof(a)-1);
 BAD_ACCOUNT(struct_bytes,sizeof(a)+1);BAD_ACCOUNT(domain,0);BAD_ACCOUNT(domain,6);
 BAD_ACCOUNT(roles,0);BAD_ACCOUNT(credential_generation,0);BAD_ACCOUNT(reserved,1);
 BAD_ACCOUNT(grant_count,0);BAD_ACCOUNT(grant_count,CAMERA_ACCESS_MAX_GRANTS+1);
 BAD_ACCOUNT(roles,CAMERA_ACCESS_VIEWER); /* grants still require operator/reader */
 for(bit=9;bit<32;bit++)BAD_ACCOUNT(roles,base.roles|(1U<<bit));
 for(bit=13;bit<32;bit++)BAD_ACCOUNT(grants[0].operations,base.grants[0].operations|(1U<<bit));
 BAD_ACCOUNT(grants[0].operations,0);BAD_ACCOUNT(grants[0].kind,0);BAD_ACCOUNT(grants[0].kind,5);
 BAD_ACCOUNT(grants[0].operations,CAMERA_ACCESS_SETTINGS_RESET_REQUEST);
 BAD_ACCOUNT(grants[0].operations,CAMERA_ACCESS_PTZ_CONTROL);
 a=base;id(a.grants[0].resource_id,"missing");INVALID(camera_access_put_account(&p,&a));
 a=base;a.grants[1]=a.grants[0];INVALID(camera_access_put_account(&p,&a));
 a=base;a.grants[7].operations=1;INVALID(camera_access_put_account(&p,&a));
 a=base;a.grants[7].kind=1;INVALID(camera_access_put_account(&p,&a));
 a=base;a.grants[7].resource_id[0]='x';INVALID(camera_access_put_account(&p,&a));
 a=base;a.account_id[6]='x';INVALID(camera_access_put_account(&p,&a)); /* nonzero tail */
 a=base;memset(a.account_id,'x',sizeof(a.account_id));INVALID(camera_access_put_account(&p,&a));
 a=base;id(a.account_id,"../private/key");INVALID(camera_access_put_account(&p,&a));
 a=base;id(a.account_id,"");INVALID(camera_access_put_account(&p,&a));
 a=base;a.account_id[0]=(char)0x80;INVALID(camera_access_put_account(&p,&a));
 a=account(CAMERA_ACCESS_RTSP);a.roles|=CAMERA_ACCESS_CONFIGURATOR;
 INVALID(camera_access_put_account(&p,&a));
 a=account(CAMERA_ACCESS_ONVIF);a.roles|=CAMERA_ACCESS_CREDENTIAL_MANAGER;
 INVALID(camera_access_put_account(&p,&a));
 a=account(CAMERA_ACCESS_SETUP);a.roles|=CAMERA_ACCESS_VIEWER;
 INVALID(camera_access_put_account(&p,&a));
 INVALID(camera_access_put_account(&p,NULL));INVALID(camera_access_put_account(NULL,&base));
 /* Rejected edits leave the prior grants/principal valid. */
 {
  struct camera_access_principal s=issue(&p,CAMERA_ACCESS_UI,1);
  struct camera_access_request q=request(CAMERA_ACCESS_UI,CAMERA_ACCESS_LIVE);
  OK(camera_access_check(&p,&s,&q,100));
 }
 puts("PASS account validation; roles; unknown bits/tail fields; atomic rejection");
#undef BAD_ACCOUNT
}
static void bad_facts_and_snapshots(void)
{
 struct camera_access_policy p;struct camera_access_verified f,base=fact(CAMERA_ACCESS_RTSP,1);
 struct camera_access_principal out={0},before={0},s,t;
 struct camera_access_request q=request(CAMERA_ACCESS_RTSP,CAMERA_ACCESS_LIVE);
 fixture(&p);memset(&out,0xa5,sizeof(out));before=out;
#define BAD_FACT(field,value) do {f=base;f.field=(value);DENY(camera_access_issue(&p,&f,100,&out));CHECK(!memcmp(&out,&before,sizeof(out)));} while(0)
 BAD_FACT(abi,0);BAD_FACT(abi,2);BAD_FACT(struct_bytes,sizeof(f)-1);BAD_FACT(struct_bytes,sizeof(f)+1);
 BAD_FACT(verified,0);BAD_FACT(verified,2);BAD_FACT(domain,0);BAD_FACT(domain,6);
 BAD_FACT(boot_generation,0);BAD_FACT(boot_generation,41);BAD_FACT(boot_generation,43);
 BAD_FACT(credential_generation,0);BAD_FACT(credential_generation,2);
 BAD_FACT(session_generation,0);BAD_FACT(issued_ns,101);BAD_FACT(issued_ns,1000);
 BAD_FACT(expires_ns,100);BAD_FACT(expires_ns,99);BAD_FACT(reserved,1);
 f=base;id(f.device_id,"other-device");DENY(camera_access_issue(&p,&f,100,&out));
 f=base;id(f.account_id,"other-account");DENY(camera_access_issue(&p,&f,100,&out));
 f=base;f.device_id[20]='x';DENY(camera_access_issue(&p,&f,100,&out));
 f=base;memset(f.account_id,'x',sizeof(f.account_id));DENY(camera_access_issue(&p,&f,100,&out));
 DENY(camera_access_issue(&p,NULL,100,&out));DENY(camera_access_issue(NULL,&base,100,&out));
 INVALID(camera_access_issue(&p,&base,100,NULL));
 s=issue(&p,CAMERA_ACCESS_RTSP,1);OK(camera_access_check(&p,&s,&q,100));
 DENY(camera_access_issue(&p,&base,100,&out)); /* same session generation replay */
#define BAD_SNAPSHOT(field,value) do {t=s;t.field=(value);DENY(camera_access_check(&p,&t,&q,100));} while(0)
 BAD_SNAPSHOT(fact.abi,2);BAD_SNAPSHOT(fact.struct_bytes,sizeof(f)+1);
 BAD_SNAPSHOT(fact.verified,0);BAD_SNAPSHOT(fact.verified,2);BAD_SNAPSHOT(fact.domain,CAMERA_ACCESS_UI);
 BAD_SNAPSHOT(fact.boot_generation,43);BAD_SNAPSHOT(fact.credential_generation,2);
 BAD_SNAPSHOT(fact.session_generation,2);BAD_SNAPSHOT(fact.issued_ns,99);
 BAD_SNAPSHOT(fact.expires_ns,2000);BAD_SNAPSHOT(fact.reserved,1);
 BAD_SNAPSHOT(policy_revision,s.policy_revision+1);BAD_SNAPSHOT(snapshot_serial,s.snapshot_serial+1);
 t=s;id(t.fact.device_id,"other");DENY(camera_access_check(&p,&t,&q,100));
 t=s;id(t.fact.account_id,"other");DENY(camera_access_check(&p,&t,&q,100));
 DENY(camera_access_check(&p,NULL,&q,100));DENY(camera_access_check(&p,&s,NULL,100));
 DENY(camera_access_check(NULL,&s,&q,100));
 OK(camera_access_revoke_session(&p,&s));DENY(camera_access_check(&p,&s,&q,100));
 DENY(camera_access_revoke_session(&p,&s));DENY(camera_access_issue(&p,&base,100,&out));
 t=issue(&p,CAMERA_ACCESS_RTSP,2);OK(camera_access_check(&p,&t,&q,100));
 DENY(camera_access_check(&p,&s,&q,100));
 /* Test missing role at use-time by a deliberate fixture-only storage fault. */
 p.records[2].account.roles=0;DENY(camera_access_check(&p,&t,&q,100));
 puts("PASS unverified/malformed facts; immutable snapshots; session revocation/replay");
#undef BAD_FACT
#undef BAD_SNAPSHOT
}
static void request_scoping(void)
{
 struct camera_access_policy p;struct camera_access_principal s;
 struct camera_access_request q,base=request(CAMERA_ACCESS_RTSP,CAMERA_ACCESS_LIVE);unsigned bit;
 fixture(&p);s=issue(&p,CAMERA_ACCESS_RTSP,1);
#define BAD_REQUEST(field,value) do {q=base;q.field=(value);DENY(camera_access_check(&p,&s,&q,100));} while(0)
 BAD_REQUEST(abi,0);BAD_REQUEST(abi,2);BAD_REQUEST(struct_bytes,sizeof(q)-1);
 BAD_REQUEST(struct_bytes,sizeof(q)+1);BAD_REQUEST(reserved,1);BAD_REQUEST(domain,0);
 BAD_REQUEST(domain,6);BAD_REQUEST(kind,0);BAD_REQUEST(kind,5);BAD_REQUEST(operation,0);
 for(bit=0;bit<32;bit++)if(bit!=0)BAD_REQUEST(operation,CAMERA_ACCESS_LIVE|(1U<<bit));
 for(bit=13;bit<32;bit++)BAD_REQUEST(operation,1U<<bit);
 q=base;id(q.device_id,"other-device");DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.sensor_id,"camera1");DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.sensor_id,"missing");DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.profile_id,"camera0-sub");DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.profile_id,"camera1-main");id(q.sensor_id,"camera1");DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.profile_id,"camera0");DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.profile_id,"../../footage");DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.profile_id,"");DENY(camera_access_check(&p,&s,&q,100));
 q=base;q.profile_id[20]='x';DENY(camera_access_check(&p,&s,&q,100));
 q=base;memset(q.profile_id,'x',CAMERA_ID_BYTES);DENY(camera_access_check(&p,&s,&q,100));
 q=base;id(q.resource_id,"head0");DENY(camera_access_check(&p,&s,&q,100));
 /* Extra fields on all other scopes are rejected, not silently discarded. */
 s=issue(&p,CAMERA_ACCESS_ADMIN,1);
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_DEVICE_CONFIG);id(q.sensor_id,"camera0");DENY(camera_access_check(&p,&s,&q,100));
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_DEVICE_CONFIG);id(q.profile_id,"camera0-main");DENY(camera_access_check(&p,&s,&q,100));
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_DEVICE_CONFIG);id(q.resource_id,"head0");DENY(camera_access_check(&p,&s,&q,100));
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_PLAYBACK);id(q.profile_id,"camera0-main");DENY(camera_access_check(&p,&s,&q,100));
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_PLAYBACK);id(q.resource_id,"head0");DENY(camera_access_check(&p,&s,&q,100));
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_PLAYBACK);id(q.sensor_id,"camera0-sub");DENY(camera_access_check(&p,&s,&q,100));
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_PTZ_CONTROL);id(q.profile_id,"camera0-main");DENY(camera_access_check(&p,&s,&q,100));
 q=request(CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_PTZ_CONTROL);id(q.resource_id,"missing");DENY(camera_access_check(&p,&s,&q,100));
 /* Viewer grant doesn't imply snapshots, playback, PTZ or device config. */
 puts("PASS device/sensor/profile/resource exact scope; unknown/unsafe operations");
#undef BAD_REQUEST
}
static void single_bit_mutations(void)
{
 struct camera_access_policy p;struct camera_access_principal s,t;
 struct camera_access_request q,base=request(CAMERA_ACCESS_RTSP,CAMERA_ACCESS_LIVE);
 size_t byte;unsigned bit;
 fixture(&p);s=issue(&p,CAMERA_ACCESS_RTSP,1);
 /* Exhaust every bit of every named field, excluding C padding. These are
  * authorization objects, NOT a wire parser fuzz test or crypto integrity test. */
#define MUTATE_REQUEST(field) do { \
 for(byte=0;byte<sizeof(base.field);byte++)for(bit=0;bit<8;bit++){ \
  q=base;((unsigned char *)&q.field)[byte]^=(unsigned char)(1U<<bit); \
  DENY(camera_access_check(&p,&s,&q,100)); \
 } \
} while(0)
 MUTATE_REQUEST(abi);MUTATE_REQUEST(struct_bytes);MUTATE_REQUEST(domain);
 MUTATE_REQUEST(kind);MUTATE_REQUEST(operation);MUTATE_REQUEST(reserved);
 MUTATE_REQUEST(device_id);MUTATE_REQUEST(sensor_id);MUTATE_REQUEST(profile_id);MUTATE_REQUEST(resource_id);
#define MUTATE_SNAPSHOT(field) do { \
 for(byte=0;byte<sizeof(s.field);byte++)for(bit=0;bit<8;bit++){ \
  t=s;((unsigned char *)&t.field)[byte]^=(unsigned char)(1U<<bit); \
  DENY(camera_access_check(&p,&t,&base,100)); \
 } \
} while(0)
 MUTATE_SNAPSHOT(fact.abi);MUTATE_SNAPSHOT(fact.struct_bytes);MUTATE_SNAPSHOT(fact.verified);
 MUTATE_SNAPSHOT(fact.domain);MUTATE_SNAPSHOT(fact.device_id);MUTATE_SNAPSHOT(fact.account_id);
 MUTATE_SNAPSHOT(fact.boot_generation);MUTATE_SNAPSHOT(fact.credential_generation);
 MUTATE_SNAPSHOT(fact.session_generation);MUTATE_SNAPSHOT(fact.issued_ns);
 MUTATE_SNAPSHOT(fact.expires_ns);MUTATE_SNAPSHOT(fact.reserved);
 MUTATE_SNAPSHOT(policy_revision);MUTATE_SNAPSHOT(snapshot_serial);
 OK(camera_access_check(&p,&s,&base,100));
 puts("PASS exhaustive single-bit mutation of every request/principal named field");
#undef MUTATE_REQUEST
#undef MUTATE_SNAPSHOT
}
static void lifecycle(void)
{
 struct camera_access_policy p;struct camera_access_principal ui,rtsp,fresh;
 struct camera_access_account a;struct camera_access_verified f;
 struct camera_access_request uq=request(CAMERA_ACCESS_UI,CAMERA_ACCESS_LIVE),rq=request(CAMERA_ACCESS_RTSP,CAMERA_ACCESS_LIVE);
 fixture(&p);ui=issue(&p,CAMERA_ACCESS_UI,1);rtsp=issue(&p,CAMERA_ACCESS_RTSP,1);
 a=account(CAMERA_ACCESS_RTSP);a.credential_generation=2;OK(camera_access_put_account(&p,&a));
 DENY(camera_access_check(&p,&rtsp,&rq,100));OK(camera_access_check(&p,&ui,&uq,100));
 f=fact(CAMERA_ACCESS_RTSP,2);DENY(camera_access_issue(&p,&f,100,&fresh));
 f.credential_generation=2;OK(camera_access_issue(&p,&f,100,&fresh));OK(camera_access_check(&p,&fresh,&rq,100));
 a.credential_generation=1;INVALID(camera_access_put_account(&p,&a));
 a=account(CAMERA_ACCESS_UI);a.credential_generation=2;OK(camera_access_put_account(&p,&a));
 DENY(camera_access_check(&p,&ui,&uq,100));OK(camera_access_check(&p,&fresh,&rq,100));
 /* Grant removal with unchanged credential generation still revokes snapshots. */
 a=account(CAMERA_ACCESS_RTSP);a.credential_generation=2;id(a.grants[0].resource_id,"camera0-sub");
 OK(camera_access_put_account(&p,&a));DENY(camera_access_check(&p,&fresh,&rq,100));
 f.session_generation=3;OK(camera_access_issue(&p,&f,100,&fresh));
 DENY(camera_access_check(&p,&fresh,&rq,100));id(rq.profile_id,"camera0-sub");OK(camera_access_check(&p,&fresh,&rq,100));
 OK(camera_access_revoke_account(&p,CAMERA_ACCESS_RTSP,alice));DENY(camera_access_check(&p,&fresh,&rq,100));
 INVALID(camera_access_put_account(&p,&a)); /* re-enable cannot reuse credential generation */
 a.credential_generation=3;OK(camera_access_put_account(&p,&a));
 DENY(camera_access_check(&p,&fresh,&rq,100));
 f.credential_generation=3;DENY(camera_access_issue(&p,&f,100,&fresh)); /* old session gen */
 f.session_generation=4;OK(camera_access_issue(&p,&f,100,&fresh));
 INVALID(camera_access_revoke_account(&p,0,alice));INVALID(camera_access_revoke_account(&p,1,NULL));
 {
  char missing[CAMERA_ID_BYTES]="missing";DENY(camera_access_revoke_account(&p,1,missing));
 }
 /* Inclusive issue boundary, exclusive expiry, stale boot, regression latch. */
 OK(camera_access_check(&p,&fresh,&rq,999));DENY(camera_access_check(&p,&fresh,&rq,1000));
 OK(camera_access_sweep(&p,1000));CHECK(!p.sessions[0].active&&!p.sessions[1].active);
 fixture(&p);ui=issue(&p,CAMERA_ACCESS_UI,1);OK(camera_access_check(&p,&ui,&uq,100));
 DENY(camera_access_check(&p,&ui,&uq,99));DENY(camera_access_check(&p,&ui,&uq,100));
 DENY(camera_access_sweep(&p,100));CHECK(p.clock_failed==1);
 INVALID(camera_access_put_account(&p,&a));
 fixture(&p);ui=issue(&p,CAMERA_ACCESS_UI,1);
 OK(camera_access_init(&p,device,43,100,resources,7));a=account(CAMERA_ACCESS_UI);OK(camera_access_put_account(&p,&a));
 DENY(camera_access_check(&p,&ui,&uq,100));f=fact(CAMERA_ACCESS_UI,1);DENY(camera_access_issue(&p,&f,100,&fresh));
 puts("PASS independent credential/grant/account revocation; monotonic expiry/boot/regression");
}
static void nvr_mapping(void)
{
 struct camera_access_policy p;struct camera_access_verified f=fact(CAMERA_ACCESS_ONVIF,1);
 struct camera_access_principal onvif,rtsp,ui;struct camera_access_account a;
 struct camera_access_request oq=request(CAMERA_ACCESS_ONVIF,CAMERA_ACCESS_LIVE),rq=request(CAMERA_ACCESS_RTSP,CAMERA_ACCESS_LIVE),uq=request(CAMERA_ACCESS_UI,CAMERA_ACCESS_LIVE);
 fixture(&p);DENY(camera_access_issue(&p,&f,100,&onvif));
 a=account(CAMERA_ACCESS_ONVIF);a.credential_generation=2;OK(camera_access_put_account(&p,&a));
 DENY(camera_access_map_nvr(&p,alice,alice));
 a.credential_generation=1;INVALID(camera_access_put_account(&p,&a));
 fixture(&p);a=account(CAMERA_ACCESS_ONVIF);id(a.grants[0].resource_id,"camera1-main");
 OK(camera_access_put_account(&p,&a));DENY(camera_access_map_nvr(&p,alice,alice));
 /* One overlapping profile cannot smuggle a second, non-mapped media profile. */
 a=account(CAMERA_ACCESS_ONVIF);
 a.grants[a.grant_count++]=grant(CAMERA_ACCESS_PROFILE,"camera1-main",CAMERA_ACCESS_SNAPSHOT);
 OK(camera_access_put_account(&p,&a));DENY(camera_access_map_nvr(&p,alice,alice));
 a=account(CAMERA_ACCESS_ONVIF);a.grants[0].operations=CAMERA_ACCESS_SNAPSHOT;
 OK(camera_access_put_account(&p,&a));DENY(camera_access_map_nvr(&p,alice,alice));
 a=account(CAMERA_ACCESS_ONVIF);OK(camera_access_put_account(&p,&a));
 rtsp=issue(&p,CAMERA_ACCESS_RTSP,1);ui=issue(&p,CAMERA_ACCESS_UI,1);
 OK(camera_access_map_nvr(&p,alice,alice));DENY(camera_access_check(&p,&rtsp,&rq,100));
 OK(camera_access_check(&p,&ui,&uq,100));DENY(camera_access_map_nvr(&p,alice,alice));
 onvif=issue(&p,CAMERA_ACCESS_ONVIF,1);rtsp=issue(&p,CAMERA_ACCESS_RTSP,2);
 OK(camera_access_check(&p,&onvif,&oq,100));OK(camera_access_check(&p,&rtsp,&rq,100));
 DENY(camera_access_check(&p,&onvif,&uq,100));DENY(camera_access_check(&p,&onvif,&rq,100));
 id(oq.sensor_id,"camera1");id(oq.profile_id,"camera1-main");DENY(camera_access_check(&p,&onvif,&oq,100));
 oq=request(CAMERA_ACCESS_ONVIF,CAMERA_ACCESS_SETTINGS_RESET_REQUEST);DENY(camera_access_check(&p,&onvif,&oq,100));
 oq=request(CAMERA_ACCESS_ONVIF,CAMERA_ACCESS_CREDENTIAL_CONFIG);DENY(camera_access_check(&p,&onvif,&oq,100));
 oq=request(CAMERA_ACCESS_ONVIF,CAMERA_ACCESS_PLAYBACK);DENY(camera_access_check(&p,&onvif,&oq,100));
 /* Changing either mapped account breaks mapping and BOTH sessions, not UI. */
 a=account(CAMERA_ACCESS_RTSP);a.credential_generation=2;OK(camera_access_put_account(&p,&a));
 oq=request(CAMERA_ACCESS_ONVIF,CAMERA_ACCESS_LIVE);DENY(camera_access_check(&p,&onvif,&oq,100));
 DENY(camera_access_check(&p,&rtsp,&rq,100));OK(camera_access_check(&p,&ui,&uq,100));
 f.session_generation=2;DENY(camera_access_issue(&p,&f,100,&onvif));
 a=account(CAMERA_ACCESS_ONVIF);a.credential_generation=2;OK(camera_access_put_account(&p,&a));
 OK(camera_access_map_nvr(&p,alice,alice));f.credential_generation=2;OK(camera_access_issue(&p,&f,100,&onvif));
 OK(camera_access_check(&p,&onvif,&oq,100));
 OK(camera_access_revoke_account(&p,CAMERA_ACCESS_ONVIF,alice));DENY(camera_access_check(&p,&onvif,&oq,100));
 CHECK(!p.records[2].nvr_peer&&!p.records[3].nvr_peer);
 /* No automatically created ONVIF record, even with same-named UI/admin. */
 init(&p);a=account(CAMERA_ACCESS_UI);OK(camera_access_put_account(&p,&a));
 a=account(CAMERA_ACCESS_ADMIN);OK(camera_access_put_account(&p,&a));
 a=account(CAMERA_ACCESS_RTSP);OK(camera_access_put_account(&p,&a));
 DENY(camera_access_map_nvr(&p,alice,alice));DENY(camera_access_issue(&p,&f,100,&onvif));
 INVALID(camera_access_map_nvr(&p,NULL,alice));INVALID(camera_access_map_nvr(NULL,alice,alice));
 puts("PASS explicit ONVIF/RTSP mapping only; synchronized revocation; no UI linking");
}
static void registry_and_bounds(void)
{
 struct camera_access_policy p;struct camera_access_resource r[7];
 struct camera_access_account a;struct camera_access_principal s,old;
 struct camera_access_verified f;struct camera_access_request q=request(CAMERA_ACCESS_RTSP,CAMERA_ACCESS_LIVE);
 uint32_t i;char name[CAMERA_ID_BYTES];
#define BAD_REGISTRY(action) do {memcpy(r,resources,sizeof(r));action;INVALID(camera_access_init(&p,device,42,100,r,7));CHECK(!p.initialized);} while(0)
 BAD_REGISTRY(r[0].kind=0);BAD_REGISTRY(r[0].kind=5);
 BAD_REGISTRY(id(r[0].id,"wrong-device"));BAD_REGISTRY(r[0].kind=CAMERA_ACCESS_SENSOR);
 BAD_REGISTRY(id(r[3].sensor_id,"camera0-main"));BAD_REGISTRY(id(r[3].sensor_id,"missing"));
 BAD_REGISTRY(id(r[1].sensor_id,"camera0"));BAD_REGISTRY(id(r[6].sensor_id,"camera0"));
 BAD_REGISTRY(id(r[3].id,"camera0"));BAD_REGISTRY(r[4]=r[3]);
 BAD_REGISTRY(memset(r[1].id,'x',CAMERA_ID_BYTES));BAD_REGISTRY(r[1].id[20]='x');
 INVALID(camera_access_init(&p,device,0,100,resources,7));
 INVALID(camera_access_init(&p,device,42,100,resources,0));
 INVALID(camera_access_init(&p,device,42,100,resources,17));
 INVALID(camera_access_init(&p,device,42,100,NULL,7));
 INVALID(camera_access_init(NULL,device,42,100,resources,7));
 INVALID(camera_access_init(&p,NULL,42,100,resources,7));
 init(&p);a=account(CAMERA_ACCESS_RTSP);
 for(i=0;i<CAMERA_ACCESS_MAX_ACCOUNTS;i++){
  snprintf(name,sizeof(name),"account%u",i);id(a.account_id,name);OK(camera_access_put_account(&p,&a));
 }
 id(a.account_id,"overflow");CHECK(camera_access_put_account(&p,&a)==CAMERA_ACCESS_FULL);
 id(a.account_id,"account0");OK(camera_access_revoke_account(&p,a.domain,a.account_id));
 id(a.account_id,"overflow");CHECK(camera_access_put_account(&p,&a)==CAMERA_ACCESS_FULL); /* tombstones */
 fixture(&p);
 for(i=1;i<=CAMERA_ACCESS_MAX_SESSIONS;i++){s=issue(&p,CAMERA_ACCESS_RTSP,i);if(i==1)old=s;}
 f=fact(CAMERA_ACCESS_RTSP,17);CHECK(camera_access_issue(&p,&f,100,&s)==CAMERA_ACCESS_FULL);
 OK(camera_access_revoke_session(&p,&old));OK(camera_access_issue(&p,&f,100,&s));
 DENY(camera_access_check(&p,&old,&q,100));OK(camera_access_check(&p,&s,&q,100));
 OK(camera_access_sweep(&p,1000));
 f.session_generation=18;f.issued_ns=1000;f.expires_ns=2000;OK(camera_access_issue(&p,&f,1000,&s));
 OK(camera_access_check(&p,&s,&q,1000));
 /* Deliberate fixture-only exhaustion injection; never wrap security epochs. */
 p.next_serial=UINT64_MAX;f.session_generation=19;
 CHECK(camera_access_issue(&p,&f,1000,&s)==CAMERA_ACCESS_EXHAUSTED);
 p.records[2].revision=UINT64_MAX;a=account(CAMERA_ACCESS_RTSP);
 CHECK(camera_access_put_account(&p,&a)==CAMERA_ACCESS_EXHAUSTED);
 CHECK(camera_access_revoke_account(&p,CAMERA_ACCESS_RTSP,alice)==CAMERA_ACCESS_EXHAUSTED);
 fixture(&p);p.records[3].revision=UINT64_MAX;
 CHECK(camera_access_map_nvr(&p,alice,alice)==CAMERA_ACCESS_EXHAUSTED);
 CHECK(!p.records[2].nvr_peer&&!p.records[3].nvr_peer);
 INVALID(camera_access_revoke_session(NULL,&s));INVALID(camera_access_revoke_session(&p,NULL));
 puts("PASS topology/no aliases; bounded accounts/sessions; slot reuse; epoch exhaustion");
#undef BAD_REGISTRY
}
int main(void)
{
 matrix();invalid_accounts();bad_facts_and_snapshots();request_scoping();single_bit_mutations();
 lifecycle();nvr_mapping();registry_and_bounds();
 printf("access_policy: PASS %u assertions; policy=%zu principal=%zu account=%zu bytes (host)\n",
  checks,sizeof(struct camera_access_policy),sizeof(struct camera_access_principal),sizeof(struct camera_access_account));
 return 0;
}
