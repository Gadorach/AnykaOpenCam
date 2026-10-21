/* SPDX-License-Identifier: GPL-2.0-only; synthetic metadata, not storage/media proof. */
#include <camera/recording_catalog.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks,suites;
#define CHECK(x) do {checks++;assert(x);} while(0)
static const char sensors[2][CAMERA_ID_BYTES]={"sensor-a","sensor-b"};
static const struct camera_recording_namespace space={"storage-a",1,1};
static void init(struct camera_recording_catalog *c)
{CHECK(camera_recording_catalog_init(c,&space,sensors,2,10)==CAMERA_OK);}
static struct camera_recording_scope scope(unsigned sensor)
{
 struct camera_recording_scope s;memset(&s,0,sizeof(s));s.space=space;
 memcpy(s.sensor_id,sensors[sensor],CAMERA_ID_BYTES);return s;
}
static struct camera_recording_metadata metadata(const char *id,unsigned sensor,uint64_t bytes)
{
 struct camera_recording_metadata m;memset(&m,0,sizeof(m));
 m.abi=CAMERA_RECORDING_ABI;m.struct_bytes=sizeof(m);
 CHECK(strlen(id)<CAMERA_ID_BYTES);memcpy(m.recording_id,id,strlen(id));
 memcpy(m.sensor_id,sensors[sensor],CAMERA_ID_BYTES);m.bytes=bytes;
 m.origin=CAMERA_RECORDING_SERVICE_OWNED;m.state=CAMERA_RECORDING_COMPLETE;m.closed=1;return m;
}
static struct camera_recording_ref add(struct camera_recording_catalog *c,const char *id,unsigned sensor,uint64_t bytes)
{
 struct camera_recording_ref r;struct camera_recording_metadata m=metadata(id,sensor,bytes);
 CHECK(camera_recording_catalog_add(c,&m,&r)==CAMERA_OK);return r;
}
static struct camera_recording_budget budget(uint32_t count,uint64_t bytes)
{struct camera_recording_budget b;memset(&b,0,sizeof(b));b.max_count=count;b.max_bytes=bytes;return b;}
static void basic(void)
{
 struct camera_recording_catalog c;struct camera_recording_ref a,b,d;
 struct camera_recording_scope s=scope(0);struct camera_recording_budget limit=budget(1,10);
 struct camera_recording_plan p;struct camera_recording_reservation r[CAMERA_RECORDING_MAX_ENTRIES];
 struct camera_recording_metadata m;
 init(&c);a=add(&c,"older",0,10);b=add(&c,"newer",0,10);d=add(&c,"other-sensor",1,100);
 CHECK(camera_recording_retention_plan(&c,&s,&limit,10,&p)==CAMERA_OK);
 CHECK(p.count==1&&p.remaining_count==1&&p.remaining_bytes==10&&!p.pressure);
 CHECK(!memcmp(p.entries[0].recording_id,a.recording_id,CAMERA_ID_BYTES));
 CHECK(camera_recording_retention_reserve(&c,&p,11,r,0)==CAMERA_INVALID);
 CHECK(camera_recording_retention_reserve(&c,&p,11,r,CAMERA_RECORDING_MAX_ENTRIES)==CAMERA_OK);
 CHECK(camera_recording_retention_check(&c,&r[0],12)==CAMERA_OK);
 CHECK(camera_recording_catalog_get(&c,&a,&m)==CAMERA_OK);
 CHECK(camera_recording_retention_ack(&c,&r[0],1)==CAMERA_OK);
 CHECK(camera_recording_catalog_get(&c,&a,&m)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_catalog_get(&c,&b,&m)==CAMERA_OK&&m.bytes==10);
 CHECK(camera_recording_catalog_get(&c,&d,&m)==CAMERA_OK&&m.bytes==100);
 CHECK(c.total_bytes==110);
 CHECK(camera_recording_retention_ack(&c,&r[0],1)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_reserve(&c,&p,12,r,CAMERA_RECORDING_MAX_ENTRIES)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_plan(&c,&s,&limit,12,&p)==CAMERA_OK&&p.count==0);
 CHECK(camera_recording_retention_reserve(&c,&p,12,NULL,0)==CAMERA_OK);
 CHECK(camera_recording_retention_reserve(&c,&p,12,NULL,0)==CAMERA_STALE_LEASE);suites++;
}
static void protected_origins_states(void)
{
 struct camera_recording_catalog c;struct camera_recording_metadata m;
 struct camera_recording_ref r;struct camera_recording_pin pin;
 struct camera_recording_scope s=scope(0);struct camera_recording_budget b=budget(0,0);
 struct camera_recording_plan p;struct camera_recording_reservation reservations[CAMERA_RECORDING_MAX_ENTRIES];
 char id[32];unsigned i;
 init(&c);
 for(i=CAMERA_RECORDING_USER;i<=CAMERA_RECORDING_FOREIGN;i++){
  snprintf(id,sizeof(id),"origin-%u",i);m=metadata(id,0,10);m.origin=i;
  CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
  CHECK(camera_recording_pin_acquire(&c,&r,&pin)==CAMERA_OK);
  CHECK(camera_recording_pin_release(&c,&pin)==CAMERA_OK);
 }
 for(i=CAMERA_RECORDING_OPEN;i<=CAMERA_RECORDING_CORRUPT;i++){
  if(i==CAMERA_RECORDING_COMPLETE)continue;
  snprintf(id,sizeof(id),"state-%u",i);m=metadata(id,0,10);m.state=i;
  m.closed=i!=CAMERA_RECORDING_OPEN;m.active=i==CAMERA_RECORDING_OPEN;
  CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
  CHECK(camera_recording_pin_acquire(&c,&r,&pin)==CAMERA_BUSY);
 }
 m=metadata("active-incomplete",0,10);m.state=CAMERA_RECORDING_INCOMPLETE;m.active=1;
 CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 r=add(&c,"safe",0,10);
 CHECK(camera_recording_pin_acquire(&c,&r,&pin)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_BUSY&&p.count==0&&
  p.remaining_count==10&&p.remaining_bytes==100&&
  p.pressure==(CAMERA_RECORDING_PRESSURE_COUNT|CAMERA_RECORDING_PRESSURE_BYTES));
 CHECK(camera_recording_pin_release(&c,&pin)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_BUSY&&p.count==1&&p.remaining_count==9);
 CHECK(camera_recording_retention_reserve(&c,&p,10,reservations,CAMERA_RECORDING_MAX_ENTRIES)==CAMERA_OK);
 CHECK(camera_recording_retention_ack(&c,&reservations[0],0)==CAMERA_OK);
 CHECK(camera_recording_catalog_get(&c,&r,&m)==CAMERA_OK);suites++;
}
static void pins_races(void)
{
 struct camera_recording_catalog c;struct camera_recording_ref r,replacement;
 struct camera_recording_pin p,q,bad;struct camera_recording_metadata m;
 struct camera_recording_scope s=scope(0);struct camera_recording_budget b=budget(0,0);
 struct camera_recording_plan plan,old;struct camera_recording_reservation reservation;
 init(&c);r=add(&c,"held",0,100);m=metadata("held",0,101);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&plan)==CAMERA_OK);
 CHECK(camera_recording_pin_acquire(&c,&r,&p)==CAMERA_OK);
 CHECK(camera_recording_retention_reserve(&c,&plan,10,&reservation,1)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&replacement)==CAMERA_BUSY);
 CHECK(camera_recording_pin_acquire(&c,&r,&q)==CAMERA_OK&&q.serial!=p.serial);
 bad=p;bad.serial=q.serial; /* An exact q copy would legitimately release q. */
 bad.ref.generation++;
 CHECK(camera_recording_pin_release(&c,&bad)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_pin_release(&c,&p)==CAMERA_OK);
 CHECK(camera_recording_pin_release(&c,&p)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_pin_check(&c,&q)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&plan)==CAMERA_BUSY&&plan.count==0);
 CHECK(camera_recording_pin_release(&c,&q)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&plan)==CAMERA_OK);old=plan;
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&replacement)==CAMERA_OK);
 CHECK(replacement.generation>r.generation);
 CHECK(camera_recording_pin_acquire(&c,&r,&p)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_reserve(&c,&old,10,&reservation,1)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&plan)==CAMERA_OK);
 CHECK(camera_recording_retention_reserve(&c,&plan,10,&reservation,1)==CAMERA_OK);
 CHECK(camera_recording_pin_acquire(&c,&replacement,&p)==CAMERA_BUSY);
 CHECK(camera_recording_catalog_replace(&c,&replacement,&m,&r)==CAMERA_BUSY);
 CHECK(camera_recording_retention_ack(&c,&reservation,0)==CAMERA_OK);
 CHECK(camera_recording_retention_check(&c,&reservation,10)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_pin_acquire(&c,&replacement,&p)==CAMERA_OK);
 CHECK(camera_recording_retention_ack(&c,&reservation,1)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_pin_release(&c,&p)==CAMERA_OK);suites++;
}
static void scopes(void)
{
 struct camera_recording_catalog c,d;struct camera_recording_namespace other=space;
 struct camera_recording_ref r,bad,foreign;struct camera_recording_metadata m;
 struct camera_recording_pin p,bad_pin;
 struct camera_recording_scope s=scope(0),bad_scope;
 struct camera_recording_budget b=budget(0,0);struct camera_recording_plan plan;
 struct camera_recording_reservation reservation,bad_reservation;
 init(&c);r=add(&c,"scoped",0,1);m=metadata("scoped",1,1);
 CHECK(camera_recording_catalog_add(&c,&m,&foreign)==CAMERA_INVALID); /* Duplicate across sensors. */
 CHECK(camera_recording_pin_acquire(&c,&r,&p)==CAMERA_OK);
 for(unsigned i=0;i<5;i++){
  bad=r;
  if(i==0)memcpy(bad.scope.sensor_id,sensors[1],CAMERA_ID_BYTES);
  if(i==1)bad.scope.space.storage_generation++;
  if(i==2)bad.scope.space.instance_generation++;
  if(i==3)memcpy(bad.scope.space.storage_id,"other\0",6);
  if(i==4)bad.generation++;
  bad_pin=p;bad_pin.ref=bad;
  CHECK(camera_recording_catalog_get(&c,&bad,&m)==CAMERA_STALE_LEASE);
  CHECK(camera_recording_pin_check(&c,&bad_pin)==CAMERA_STALE_LEASE);
  CHECK(camera_recording_pin_release(&c,&bad_pin)==CAMERA_STALE_LEASE);
  CHECK(camera_recording_pin_acquire(&c,&bad,&bad_pin)==CAMERA_STALE_LEASE);
 }
 CHECK(camera_recording_pin_check(&c,&p)==CAMERA_OK);
 CHECK(camera_recording_pin_release(&c,&p)==CAMERA_OK);
 other.instance_generation++;
 CHECK(camera_recording_catalog_init(&d,&other,sensors,2,10)==CAMERA_OK);
 foreign=add(&d,"scoped",0,1);
 CHECK(foreign.generation==r.generation);
 CHECK(camera_recording_pin_acquire(&d,&r,&p)==CAMERA_STALE_LEASE);
 bad_scope=s;memcpy(bad_scope.sensor_id,"unknown",8);
 CHECK(camera_recording_retention_plan(&c,&bad_scope,&b,10,&plan)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&plan)==CAMERA_OK);
 CHECK(camera_recording_retention_reserve(&c,&plan,10,&reservation,1)==CAMERA_OK);
 bad_reservation=reservation;memcpy(bad_reservation.ref.scope.sensor_id,sensors[1],CAMERA_ID_BYTES);
 CHECK(camera_recording_retention_ack(&c,&bad_reservation,1)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_check(&d,&reservation,10)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_ack(&c,&reservation,0)==CAMERA_OK);suites++;
}
static void age_and_order(void)
{
 struct camera_recording_catalog c;struct camera_recording_metadata m;
 struct camera_recording_ref r;struct camera_recording_plan p;
 struct camera_recording_scope s=scope(0);struct camera_recording_budget b=budget(64,UINT64_MAX);
 init(&c);b.age_enabled=1;b.utc_epoch=7;b.utc_now_ns=100;b.max_age_ns=10;
 m=metadata("admitted-first-new-utc",0,1);m.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;
 m.time_epoch=7;m.start_ns=90;m.end_ns=95;CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 m=metadata("admitted-second-old-utc",0,1);m.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;
 m.time_epoch=7;m.start_ns=0;m.end_ns=1;CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 m=metadata("age-boundary",0,1);m.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;
 m.time_epoch=7;m.start_ns=90;m.end_ns=90;CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==1&&
  !memcmp(p.entries[0].recording_id,"admitted-second-old-utc",22));
 add(&c,"utc-unknown",0,1);
 m=metadata("monotonic",0,1);m.time_domain=CAMERA_RECORDING_TIME_MONOTONIC;
 m.time_epoch=7;m.end_ns=1;CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 m=metadata("wrong-epoch",0,1);m.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;
 m.time_epoch=6;m.end_ns=1;CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 m=metadata("future",0,1);m.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;
 m.time_epoch=7;m.end_ns=101;CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 m=metadata("protected-aged",0,1);m.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;
 m.time_epoch=7;m.end_ns=1;m.origin=CAMERA_RECORDING_STOCK;
 CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,11,&p)==CAMERA_BUSY&&p.count==1&&
  p.pressure==(CAMERA_RECORDING_PRESSURE_AGE|CAMERA_RECORDING_PRESSURE_TIME_UNKNOWN));
 b.max_count=0;b.max_bytes=0;
 CHECK(camera_recording_retention_plan(&c,&s,&b,12,&p)==CAMERA_BUSY&&p.count==7&&p.remaining_count==1);
 CHECK(!memcmp(p.entries[0].recording_id,"admitted-first-new-utc",21));
 CHECK(p.entries[0].generation<p.entries[1].generation);suites++;
}
static void clocks(void)
{
 struct camera_recording_catalog c;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(0,0);struct camera_recording_plan p;
 struct camera_recording_reservation r;struct camera_recording_pin pin;struct camera_recording_ref ref;
 init(&c);ref=add(&c,"clock",0,1);
 CHECK(camera_recording_pin_acquire(&c,&ref,&pin)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,9,&p)==CAMERA_NOT_READY&&c.clock_failed);
 CHECK(camera_recording_retention_plan(&c,&s,&b,100,&p)==CAMERA_NOT_READY);
 CHECK(camera_recording_pin_release(&c,&pin)==CAMERA_OK); /* Drain possible after clock failure. */
 init(&c);add(&c,"clock",0,1);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK);
 CHECK(camera_recording_retention_reserve(&c,&p,9,&r,1)==CAMERA_NOT_READY);
 init(&c);add(&c,"clock",0,1);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK);
 CHECK(camera_recording_retention_reserve(&c,&p,11,&r,1)==CAMERA_OK);
 CHECK(camera_recording_retention_check(&c,&r,10)==CAMERA_NOT_READY);
 CHECK(camera_recording_retention_ack(&c,&r,0)==CAMERA_OK);
 init(&c);b.age_enabled=1;b.utc_epoch=1;b.utc_now_ns=100;
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK);
 b.utc_now_ns=99;
 CHECK(camera_recording_retention_plan(&c,&s,&b,11,&p)==CAMERA_NOT_READY);
 init(&c);b.utc_epoch=2;b.utc_now_ns=100;
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK);
 b.utc_epoch=3;b.utc_now_ns=1;
 CHECK(camera_recording_retention_plan(&c,&s,&b,11,&p)==CAMERA_OK); /* Explicit new trust epoch. */
 b.utc_epoch=2;
 CHECK(camera_recording_retention_plan(&c,&s,&b,12,&p)==CAMERA_NOT_READY);suites++;
}
static void stale_and_forged_plans(void)
{
 struct camera_recording_catalog c;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(0,0);struct camera_recording_plan p,q,bad;
 struct camera_recording_reservation r[CAMERA_RECORDING_MAX_ENTRIES],wrong;
 struct camera_recording_ref old,reused;struct camera_recording_metadata m;
 init(&c);old=add(&c,"reuse",0,1);add(&c,"second",0,1);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==2);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&q)==CAMERA_OK&&q.serial!=p.serial);
 CHECK(camera_recording_retention_reserve(&c,&p,10,r,64)==CAMERA_STALE_LEASE);
 for(unsigned i=0;i<8;i++){
  bad=q;
  if(i==0)bad.entries[0].generation++;
  if(i==1)bad.count--;
  if(i==2)bad.remaining_count++;
  if(i==3)bad.pressure++;
  if(i==4)bad.remaining_bytes++;
  if(i==5)bad.entries[2].generation++;
  if(i==6)memcpy(bad.entries[0].scope.sensor_id,sensors[1],CAMERA_ID_BYTES);
  if(i==7)bad.reserved=1;
  CHECK(camera_recording_retention_reserve(&c,&bad,10,r,64)==CAMERA_STALE_LEASE);
  CHECK(!c.slots[0].reservation&&!c.slots[1].reservation);
 }
 CHECK(camera_recording_retention_reserve(&c,&q,10,r,64)==CAMERA_OK);
 wrong=r[0];wrong.serial++;
 CHECK(camera_recording_retention_ack(&c,&wrong,1)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_ack(&c,&r[0],2)==CAMERA_INVALID);
 CHECK(camera_recording_retention_ack(&c,&r[0],1)==CAMERA_OK);
 reused=add(&c,"reuse",0,2);CHECK(reused.generation>old.generation);
 CHECK(camera_recording_catalog_get(&c,&old,&m)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_ack(&c,&r[0],1)==CAMERA_STALE_LEASE);
 CHECK(camera_recording_retention_check(&c,&r[1],11)==CAMERA_OK); /* Other ack/add doesn't lose reservation. */
 CHECK(camera_recording_retention_ack(&c,&r[1],0)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,11,&p)==CAMERA_OK&&p.count==2);
 CHECK(!memcmp(p.entries[0].recording_id,"second",6));suites++;
}
static void capacity(void)
{
 struct camera_recording_catalog c;struct camera_recording_ref ref,unused;
 struct camera_recording_pin pins[CAMERA_RECORDING_MAX_PINS],extra;
 struct camera_recording_metadata m;char id[32];
 init(&c);ref=add(&c,"pins",0,0);
 for(unsigned i=0;i<CAMERA_RECORDING_MAX_PINS;i++)
  CHECK(camera_recording_pin_acquire(&c,&ref,&pins[i])==CAMERA_OK);
 CHECK(camera_recording_pin_acquire(&c,&ref,&extra)==CAMERA_BUSY);
 for(unsigned i=0;i<CAMERA_RECORDING_MAX_PINS;i++){
  CHECK(camera_recording_pin_release(&c,&pins[i])==CAMERA_OK);
  CHECK(camera_recording_pin_release(&c,&pins[i])==CAMERA_STALE_LEASE);
 }
 CHECK(camera_recording_pin_acquire(&c,&ref,&extra)==CAMERA_OK&&extra.serial>pins[31].serial);
 CHECK(camera_recording_pin_check(&c,&pins[0])==CAMERA_STALE_LEASE);
 CHECK(camera_recording_pin_release(&c,&extra)==CAMERA_OK);
 for(unsigned i=1;i<CAMERA_RECORDING_MAX_ENTRIES;i++){
  snprintf(id,sizeof(id),"entry-%u",i);add(&c,id,i%2,0);
 }
 m=metadata("overflow-slot",0,0);
 CHECK(camera_recording_catalog_add(&c,&m,&unused)==CAMERA_BUSY);
 m=metadata("pins",0,1);
 CHECK(camera_recording_catalog_replace(&c,&ref,&m,&unused)==CAMERA_OK);suites++;
}
static void overflow(void)
{
 struct camera_recording_catalog c;struct camera_recording_ref r,q;
 struct camera_recording_metadata m;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(64,UINT64_MAX);struct camera_recording_plan p;
 struct camera_recording_pin pin;struct camera_recording_reservation reservation;
 init(&c);r=add(&c,"max",0,UINT64_MAX);m=metadata("overflow",1,1);
 CHECK(camera_recording_catalog_add(&c,&m,&q)==CAMERA_INVALID&&c.total_bytes==UINT64_MAX);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==0&&p.remaining_bytes==UINT64_MAX);
 m=metadata("max",0,UINT64_MAX-1);
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&q)==CAMERA_OK);add(&c,"one",1,1);
 m=metadata("max",0,UINT64_MAX);
 CHECK(camera_recording_catalog_replace(&c,&q,&m,&r)==CAMERA_INVALID);
 init(&c);c.next_generation=UINT64_MAX;m=metadata("generation",0,0);
 CHECK(camera_recording_catalog_add(&c,&m,&r)==CAMERA_NOT_READY&&!c.slots[0].used);
 init(&c);c.next_generation=UINT64_MAX-1;r=add(&c,"last-generation",0,0);
 CHECK(r.generation==UINT64_MAX);m=metadata("last-generation",0,0);
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&q)==CAMERA_NOT_READY);
 init(&c);r=add(&c,"serial",0,0);c.next_serial=UINT64_MAX;
 CHECK(camera_recording_pin_acquire(&c,&r,&pin)==CAMERA_NOT_READY);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_NOT_READY);
 init(&c);add(&c,"reservation-serial",0,1);b=budget(0,0);
 c.next_serial=UINT64_MAX-1;
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.serial==UINT64_MAX);
 CHECK(camera_recording_retention_reserve(&c,&p,10,&reservation,1)==CAMERA_NOT_READY);
 init(&c);r=add(&c,"revision",0,0);c.revision=UINT64_MAX;
 CHECK(camera_recording_pin_acquire(&c,&r,&pin)==CAMERA_NOT_READY);
 m=metadata("revision",0,1);
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&q)==CAMERA_NOT_READY);
 CHECK(c.revision==UINT64_MAX&&c.next_generation==r.generation);
 init(&c);add(&c,"last-revision",0,1);c.revision=UINT64_MAX-1;
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK);
 CHECK(camera_recording_retention_reserve(&c,&p,10,&reservation,1)==CAMERA_OK&&c.revision==UINT64_MAX);
 CHECK(camera_recording_retention_check(&c,&reservation,10)==CAMERA_NOT_READY);
 CHECK(camera_recording_retention_ack(&c,&reservation,1)==CAMERA_NOT_READY&&c.total_bytes==1);suites++;
}
static void invalid_inputs(void)
{
 struct camera_recording_catalog c;struct camera_recording_ref r,q;
 struct camera_recording_metadata m,bad;struct camera_recording_namespace invalid=space;
 struct camera_recording_scope s=scope(0);struct camera_recording_budget b;
 struct camera_recording_plan p;char duplicate[2][CAMERA_ID_BYTES]={"sensor-a","sensor-a"};
 CHECK(camera_recording_catalog_init(NULL,&space,sensors,2,10)==CAMERA_INVALID);
 invalid.storage_generation=0;
 CHECK(camera_recording_catalog_init(&c,&invalid,sensors,2,10)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_add(&c,NULL,&r)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_init(&c,&space,(const char (*)[CAMERA_ID_BYTES])duplicate,2,10)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_init(&c,&space,sensors,0,10)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_init(&c,&space,sensors,5,10)==CAMERA_INVALID);
 init(&c);m=metadata("valid",0,1);
 for(unsigned i=0;i<18;i++){
  bad=m;
  switch(i){
  case 0:bad.abi++;break;case 1:bad.struct_bytes--;break;case 2:bad.reserved=1;break;
  case 3:bad.origin=0;break;case 4:bad.origin=7;break;case 5:bad.state=0;break;
  case 6:bad.state=5;break;case 7:bad.closed=2;break;case 8:bad.active=2;break;
  case 9:bad.time_domain=3;break;case 10:bad.end_ns=1;break;
  case 11:bad.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;break;
  case 12:bad.time_domain=CAMERA_RECORDING_TIME_MONOTONIC;bad.time_epoch=1;bad.start_ns=2;bad.end_ns=1;break;
  case 13:bad.recording_id[6]='x';break; /* Nonzero padding after terminator. */
  case 14:memset(bad.recording_id,'x',CAMERA_ID_BYTES);break;
  case 15:bad.recording_id[0]='/';break;
  case 16:bad.state=CAMERA_RECORDING_OPEN;break;
  case 17:bad.active=1;break;
  }
  CHECK(camera_recording_catalog_add(&c,&bad,&r)==CAMERA_INVALID&&!c.slots[0].used);
 }
 r=add(&c,"valid",0,1);
 CHECK(camera_recording_catalog_add(&c,&m,&q)==CAMERA_INVALID);
 m.origin=CAMERA_RECORDING_IMPORTED;
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&q)==CAMERA_INVALID);
 for(unsigned i=0;i<5;i++){
  b=budget(64,UINT64_MAX);
  if(i==0)b.max_count=65;
  if(i==1)b.age_enabled=2;
  if(i==2)b.utc_now_ns=1;
  if(i==3)b.max_age_ns=1;
  if(i==4)b.age_enabled=1;
  CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_INVALID);
 }
 CHECK(camera_recording_pin_acquire(&c,NULL,NULL)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_get(&c,&r,NULL)==CAMERA_INVALID);
 CHECK(camera_recording_retention_reserve(&c,NULL,10,NULL,0)==CAMERA_INVALID);
 CHECK(camera_recording_retention_ack(&c,NULL,0)==CAMERA_STALE_LEASE);suites++;
}
static void bounded_churn(void)
{
 struct camera_recording_catalog c;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(0,0);struct camera_recording_ref r,old;
 struct camera_recording_pin pin,old_pin;struct camera_recording_reservation reservation,old_reservation;
 struct camera_recording_plan plan;struct camera_recording_metadata m;
 memset(&old,0,sizeof(old));memset(&old_pin,0,sizeof(old_pin));memset(&old_reservation,0,sizeof(old_reservation));
 init(&c);
 for(unsigned i=0;i<256;i++){
  r=add(&c,"same-id-reused",0,i);
  CHECK(r.generation>old.generation);
  CHECK(camera_recording_catalog_get(&c,&old,&m)==CAMERA_STALE_LEASE);
  CHECK(camera_recording_pin_acquire(&c,&r,&pin)==CAMERA_OK);
  CHECK(camera_recording_pin_release(&c,&old_pin)==CAMERA_STALE_LEASE);
  CHECK(camera_recording_retention_plan(&c,&s,&b,10+i,&plan)==CAMERA_BUSY&&plan.count==0);
  CHECK(camera_recording_pin_release(&c,&pin)==CAMERA_OK);
  CHECK(camera_recording_retention_plan(&c,&s,&b,10+i,&plan)==CAMERA_OK&&plan.count==1);
  CHECK(camera_recording_retention_reserve(&c,&plan,10+i,&reservation,1)==CAMERA_OK);
  CHECK(camera_recording_retention_ack(&c,&old_reservation,1)==CAMERA_STALE_LEASE);
  CHECK(camera_recording_retention_check(&c,&reservation,10+i)==CAMERA_OK);
  CHECK(camera_recording_retention_ack(&c,&reservation,1)==CAMERA_OK&&c.total_bytes==0);
  old=r;old_pin=pin;old_reservation=reservation;
 }suites++;
}
static void full_batch_and_boundaries(void)
{
 struct camera_recording_catalog c;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(64,64);struct camera_recording_plan p;
 struct camera_recording_reservation r[CAMERA_RECORDING_MAX_ENTRIES];char id[32];
 init(&c);
 for(unsigned i=0;i<64;i++){snprintf(id,sizeof(id),"batch-%u",i);add(&c,id,0,1);}
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==0);
 b.max_bytes=63;
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==1&&p.remaining_count==63);
 b.max_bytes=64;b.max_count=63;
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==1&&p.remaining_bytes==63);
 b=budget(0,0);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==64&&p.remaining_bytes==0);
 for(unsigned i=1;i<64;i++)CHECK(p.entries[i].generation>p.entries[i-1].generation);
 CHECK(camera_recording_retention_reserve(&c,&p,10,r,63)==CAMERA_INVALID);
 CHECK(camera_recording_retention_reserve(&c,&p,10,r,64)==CAMERA_OK);
 for(unsigned i=0;i<64;i++){
  CHECK(camera_recording_retention_check(&c,&r[i],10)==CAMERA_OK);
  CHECK(camera_recording_retention_ack(&c,&r[i],1)==CAMERA_OK);
 }
 CHECK(c.total_bytes==0);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==0);suites++;
}
static void reserve_overlap(void)
{
 struct camera_recording_catalog c,before;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(0,0);struct camera_recording_plan p;
 struct {
  union {
   struct camera_recording_plan plan;
   struct camera_recording_reservation output[CAMERA_RECORDING_MAX_ENTRIES];
  } storage;
  uint64_t canary[4];
 } exact,saved_exact;
 struct {
  union {
   struct camera_recording_plan plan;
   struct {
    unsigned char prefix[offsetof(struct camera_recording_plan,entries)];
    struct camera_recording_reservation output[CAMERA_RECORDING_MAX_ENTRIES];
   } shifted;
  } storage;
  uint64_t canary[4];
 } tail,saved_tail;
 struct {
  union {
   struct {struct camera_recording_reservation prefix[2];struct camera_recording_plan plan;} input;
   struct camera_recording_reservation output[CAMERA_RECORDING_MAX_ENTRIES];
  } storage;
  uint64_t canary[4];
 } capacity_overlap,saved_capacity;
 struct {struct camera_recording_plan plan;struct camera_recording_reservation output[2];uint64_t canary;} adjacent;
 struct {struct camera_recording_reservation output[2];struct camera_recording_plan plan;uint64_t canary;} reverse;
 uintptr_t high=UINTPTR_MAX-UINTPTR_MAX%_Alignof(struct camera_recording_reservation);
 init(&c);add(&c,"alias-first",0,1);add(&c,"alias-second",0,1);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_OK&&p.count==2);
 memcpy(&before,&c,sizeof(c));
 memset(&exact,0xa5,sizeof(exact));exact.storage.plan=p;memcpy(&saved_exact,&exact,sizeof(exact));
 for(unsigned i=0;i<2;i++){
  CHECK(camera_recording_retention_reserve(&c,&exact.storage.plan,i?9:11,
   exact.storage.output,CAMERA_RECORDING_MAX_ENTRIES)==CAMERA_INVALID);
  CHECK(!memcmp(&c,&before,sizeof(c))); /* Includes revision/serial/clock/pending/ALL slots. */
  CHECK(!memcmp(&exact,&saved_exact,sizeof(exact)));
  CHECK(!memcmp(exact.canary,saved_exact.canary,sizeof(exact.canary)));
 }
 memset(&tail,0xa5,sizeof(tail));tail.storage.plan=p;memcpy(&saved_tail,&tail,sizeof(tail));
 CHECK(camera_recording_retention_reserve(&c,&tail.storage.plan,11,
  tail.storage.shifted.output,CAMERA_RECORDING_MAX_ENTRIES)==CAMERA_INVALID);
 CHECK(!memcmp(&c,&before,sizeof(c))&&!memcmp(&tail,&saved_tail,sizeof(tail)));
 CHECK(!memcmp(tail.canary,saved_tail.canary,sizeof(tail.canary)));
 /* Actual two-element write prefix is disjoint, but declared capacity overlaps. */
 memset(&capacity_overlap,0xa5,sizeof(capacity_overlap));capacity_overlap.storage.input.plan=p;
 memcpy(&saved_capacity,&capacity_overlap,sizeof(capacity_overlap));
 CHECK(camera_recording_retention_reserve(&c,&capacity_overlap.storage.input.plan,11,
  capacity_overlap.storage.output,CAMERA_RECORDING_MAX_ENTRIES)==CAMERA_INVALID);
 CHECK(!memcmp(&c,&before,sizeof(c))&&!memcmp(&capacity_overlap,&saved_capacity,sizeof(capacity_overlap)));
 CHECK(!memcmp(capacity_overlap.canary,saved_capacity.canary,sizeof(capacity_overlap.canary)));
 /* Synthetic aligned address span overflow: validation must never dereference it.
  * On 32-bit SIZE_MAX the huge-capacity multiplication is also rejected. */
 CHECK(camera_recording_retention_reserve(&c,&p,11,
  (struct camera_recording_reservation *)high,2)==CAMERA_INVALID);
 CHECK(camera_recording_retention_reserve(&c,&p,11,
  (struct camera_recording_reservation *)high,UINT32_MAX)==CAMERA_INVALID);
 CHECK(!memcmp(&c,&before,sizeof(c))&&!c.slots[0].reservation&&!c.slots[1].reservation);
 /* Adjacent spans in both address orders must not be mistaken for overlap. */
 memset(&adjacent,0xa5,sizeof(adjacent));adjacent.plan=p;
 CHECK(camera_recording_retention_reserve(&c,&adjacent.plan,11,adjacent.output,2)==CAMERA_OK);
 CHECK(!memcmp(&adjacent.plan,&p,sizeof(p))&&adjacent.canary==UINT64_C(0xa5a5a5a5a5a5a5a5));
 CHECK(c.slots[0].reservation&&c.slots[1].reservation);
 for(unsigned i=0;i<2;i++)CHECK(camera_recording_retention_ack(&c,&adjacent.output[i],0)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,11,&p)==CAMERA_OK);
 memset(&reverse,0xa5,sizeof(reverse));reverse.plan=p;
 CHECK(camera_recording_retention_reserve(&c,&reverse.plan,11,reverse.output,2)==CAMERA_OK);
 CHECK(!memcmp(&reverse.plan,&p,sizeof(p))&&reverse.canary==UINT64_C(0xa5a5a5a5a5a5a5a5));
 for(unsigned i=0;i<2;i++)CHECK(camera_recording_retention_ack(&c,&reverse.output[i],0)==CAMERA_OK);
 b=budget(64,UINT64_MAX);
 CHECK(camera_recording_retention_plan(&c,&s,&b,11,&p)==CAMERA_OK&&p.count==0);
 exact.storage.plan=p;memcpy(&saved_exact,&exact,sizeof(exact));
 CHECK(camera_recording_retention_reserve(&c,&exact.storage.plan,11,exact.storage.output,0)==CAMERA_OK);
 CHECK(!memcmp(&exact,&saved_exact,sizeof(exact))); /* Empty span does not write. */
 suites++;
}
static void catalogue_aliases(void)
{
 struct camera_recording_catalog c,before;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(0,0);struct camera_recording_plan p;
 struct camera_recording_ref r,out;struct camera_recording_pin pin;
 struct camera_recording_metadata m,output;
 init(&c);r=add(&c,"alias-owned",0,1);m=metadata("alias-owned",0,1);
 CHECK(camera_recording_pin_acquire(&c,&r,&pin)==CAMERA_OK);
 CHECK(camera_recording_retention_plan(&c,&s,&b,10,&p)==CAMERA_BUSY);
 memcpy(&before,&c,sizeof(c));memset(&out,0xa5,sizeof(out));memset(&output,0xa5,sizeof(output));
 CHECK(camera_recording_catalog_init(&c,&c.space,sensors,2,11)==CAMERA_INVALID);
 CHECK(!memcmp(&c,&before,sizeof(c)));
 CHECK(camera_recording_catalog_init(&c,&space,(const char (*)[CAMERA_ID_BYTES])c.sensors,2,11)==CAMERA_INVALID);
 CHECK(!memcmp(&c,&before,sizeof(c)));
 CHECK(camera_recording_catalog_add(&c,&m,&c.pins[1].pin.ref)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_add(&c,&c.slots[0].metadata,&out)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&c.pins[1].pin.ref)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_replace(&c,&c.pins[0].pin.ref,&m,&out)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_replace(&c,&r,&c.slots[0].metadata,&out)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_get(&c,&r,&c.slots[0].metadata)==CAMERA_INVALID);
 CHECK(camera_recording_catalog_get(&c,&c.pins[0].pin.ref,&output)==CAMERA_INVALID);
 CHECK(camera_recording_pin_acquire(&c,&r,&c.pins[1].pin)==CAMERA_INVALID);
 CHECK(camera_recording_pin_acquire(&c,&c.pins[0].pin.ref,&pin)==CAMERA_INVALID);
 CHECK(camera_recording_pin_check(&c,&c.pins[0].pin)==CAMERA_INVALID);
 CHECK(camera_recording_pin_release(&c,&c.pins[0].pin)==CAMERA_INVALID);
 CHECK(camera_recording_retention_plan(&c,&s,&b,11,&c.pending)==CAMERA_INVALID);
 CHECK(camera_recording_retention_plan(&c,&c.pending.scope,&b,11,&p)==CAMERA_INVALID);
 CHECK(camera_recording_retention_plan(&c,&s,
  (const struct camera_recording_budget *)&c.slots[1],11,&p)==CAMERA_INVALID);
 CHECK(camera_recording_retention_reserve(&c,&c.pending,11,NULL,0)==CAMERA_INVALID);
 CHECK(camera_recording_retention_reserve(&c,&p,11,
  (struct camera_recording_reservation *)&c.pins[1].pin,1)==CAMERA_INVALID);
 CHECK(camera_recording_retention_check(&c,
  (const struct camera_recording_reservation *)&c.pins[0].pin,11)==CAMERA_INVALID);
 CHECK(camera_recording_retention_ack(&c,
  (const struct camera_recording_reservation *)&c.pins[0].pin,1)==CAMERA_INVALID);
 CHECK(!memcmp(&c,&before,sizeof(c))&&c.revision==before.revision&&c.next_serial==before.next_serial);
 for(size_t i=0;i<sizeof(out);i++)CHECK(((const unsigned char *)&out)[i]==0xa5);
 for(size_t i=0;i<sizeof(output);i++)CHECK(((const unsigned char *)&output)[i]==0xa5);
 CHECK(camera_recording_pin_check(&c,&pin)==CAMERA_OK);
 CHECK(camera_recording_pin_release(&c,&pin)==CAMERA_OK);suites++;
}
static void safe_external_aliases(void)
{
 struct camera_recording_catalog c;struct camera_recording_metadata m;
 struct camera_recording_pin pin;struct camera_recording_scope s=scope(0);
 struct camera_recording_budget b=budget(0,0);struct camera_recording_ref r;
 struct camera_recording_plan p;struct camera_recording_reservation reservations[2];
 union {struct camera_recording_metadata metadata;struct camera_recording_ref ref;} shared;
 union {struct camera_recording_budget budget;struct camera_recording_plan plan;} shared_plan;
 init(&c);shared.metadata=metadata("safe-alias",0,1);
 CHECK(camera_recording_catalog_add(&c,&shared.metadata,&shared.ref)==CAMERA_OK);r=shared.ref;
 CHECK(camera_recording_catalog_get(&c,&shared.ref,&shared.metadata)==CAMERA_OK&&shared.metadata.bytes==1);
 m=metadata("safe-alias",0,2);
 CHECK(camera_recording_catalog_replace(&c,&r,&m,&r)==CAMERA_OK);
 pin.ref=r;
 CHECK(camera_recording_pin_acquire(&c,&pin.ref,&pin)==CAMERA_OK);
 CHECK(camera_recording_pin_check(&c,&pin)==CAMERA_OK);
 CHECK(camera_recording_pin_release(&c,&pin)==CAMERA_OK);
 p.scope=s;
 CHECK(camera_recording_retention_plan(&c,&p.scope,&b,10,&p)==CAMERA_OK&&p.count==1);
 shared_plan.budget=b;
 CHECK(camera_recording_retention_plan(&c,&s,&shared_plan.budget,10,&shared_plan.plan)==CAMERA_OK&&
  shared_plan.plan.count==1);
 CHECK(camera_recording_retention_reserve(&c,&shared_plan.plan,10,reservations,2)==CAMERA_OK);
 CHECK(camera_recording_retention_ack(&c,&reservations[0],0)==CAMERA_OK);suites++;
}
int main(void)
{
 basic();protected_origins_states();pins_races();scopes();age_and_order();clocks();
 stale_and_forged_plans();capacity();overflow();invalid_inputs();bounded_churn();full_batch_and_boundaries();
 reserve_overlap();catalogue_aliases();safe_external_aliases();
 printf("recording_catalog suites=%u checks=%u PASS (synthetic host metadata only)\n",suites,checks);
 return 0;
}
