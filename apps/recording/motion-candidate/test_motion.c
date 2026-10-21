/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include "motion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static unsigned checks,suites;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}} while(0)
#define BASE 1000000000ULL
#define STEP 100000000ULL
struct fake {uint64_t now;struct mm_span span;unsigned acquires,valids,releases,admissions,deny_tile,revoke_admin_at,reentry;
 int acquire_result,release_result,reentered;struct motion_engine *engine;struct camera_access_policy *policy;
 unsigned revoke_valid_at,events_only_at,clock_reads,revoke_clock_at,revoke_acquire;
 uint64_t admit_delay,valid_delay,acquire_delay,release_delay;
 struct camera_access_principal *observer,*admin;};
struct fixture {struct motion_engine engine;struct fake fake;struct mm_cell cells[MM_CELLS];struct mm_identity identity;
 struct camera_access_policy policy;struct camera_access_principal observer,admin;struct mm_config cfg;
 struct camera_device devices[2];struct camera_profile profile;struct camera_association assoc;struct camera_graph graph;};
static void id(char *dst,const char *s){memset(dst,0,CAMERA_ID_BYTES);memcpy(dst,s,strlen(s));}
static uint64_t clock_ns(void *ctx)
{struct fake *f=ctx;f->clock_reads++;if(f->revoke_clock_at==f->clock_reads)CHECK(camera_access_revoke_session(f->policy,f->observer)==CAMERA_ACCESS_OK);return f->now;}
static int admit(void *ctx,const struct mm_identity *identity,uint64_t now,uint64_t *until)
{
 struct fake *f=ctx;f->admissions++;CHECK(identity->source_generation==1);CHECK(now<=f->now);*until=f->now+1000000000ULL;
 if(f->deny_tile&&f->valids>=f->deny_tile)CHECK(camera_access_revoke_session(f->policy,f->observer)==CAMERA_ACCESS_OK);
 if(f->revoke_admin_at&&f->admissions==f->revoke_admin_at)CHECK(camera_access_revoke_session(f->policy,f->admin)==CAMERA_ACCESS_OK);
 if(f->reentry){f->reentry=0;f->reentered=mm_poll(f->engine,f->now);}
 f->now+=f->admit_delay;
 return 1;
}
static int acquire(void *ctx,const struct mm_identity *identity,struct mm_span *out)
{struct fake *f=ctx;(void)identity;f->acquires++;*out=f->span;f->now+=f->acquire_delay;
 if(f->revoke_acquire)CHECK(camera_access_revoke_session(f->policy,f->observer)==CAMERA_ACCESS_OK);
 return f->acquire_result;}
static void event_only_principal(struct fake *f)
{
 struct camera_access_account a;struct camera_access_verified v;struct camera_access_principal fresh;
 memset(&a,0,sizeof(a));a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_UI;
 a.roles=CAMERA_ACCESS_VIEWER;a.credential_generation=2;a.grant_count=1;id(a.account_id,"PUBLICDUMMY-recorder-owner");
 a.grants[0].kind=CAMERA_ACCESS_SENSOR;a.grants[0].operations=CAMERA_ACCESS_EVENTS;id(a.grants[0].resource_id,"PUBLICDUMMY-camera0");
 CHECK(camera_access_put_account(f->policy,&a)==CAMERA_ACCESS_OK);
 memset(&v,0,sizeof(v));v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;v.domain=CAMERA_ACCESS_UI;v.boot_generation=77;
 v.credential_generation=2;v.session_generation=2;v.issued_ns=f->now;v.expires_ns=f->now+60000000000ULL;
 id(v.device_id,"PUBLICDUMMY-device");id(v.account_id,"PUBLICDUMMY-recorder-owner");
 CHECK(camera_access_issue(f->policy,&v,f->now,&fresh)==CAMERA_ACCESS_OK);
 f->engine->observer=fresh;*f->observer=fresh; /* real owner credential rotation, not a grant flag */
}
static int valid(void *ctx,const struct mm_span *span,uint64_t now)
{struct fake *f=ctx;int result;f->valids++;result=now<span->expires_ns&&!memcmp(&span->ticket,&f->span.ticket,sizeof(span->ticket));
 if(f->revoke_valid_at==f->valids)CHECK(camera_access_revoke_session(f->policy,f->observer)==CAMERA_ACCESS_OK);
 if(f->events_only_at==f->valids)event_only_principal(f);
 f->now+=f->valid_delay;return result;}
static int release(void *ctx,const struct mm_ticket *ticket,uint64_t now)
{struct fake *f=ctx;CHECK(now>=f->span.sampled_ns);CHECK(!memcmp(ticket,&f->span.ticket,sizeof(*ticket)));f->releases++;f->now+=f->release_delay;return f->release_result;}
static struct mm_config config(void)
{
 struct mm_config c;memset(&c,0,sizeof(c));c.schema=1;c.generation=1;c.delta_min=12;c.noise_multiplier=2;c.area_permille=30;
 c.global_delta=8;c.global_permille=900;c.debounce_ns=200000000;c.release_ns=200000000;c.min_event_ns=500000000;
 c.max_gap_ns=500000000;c.event_ttl_ns=2000000000;return c;
}
static void issue(struct fixture *f,uint32_t domain,struct camera_access_principal *out)
{
 struct camera_access_account a;struct camera_access_verified v;
 memset(&a,0,sizeof(a));a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=domain;a.credential_generation=1;
 id(a.account_id,domain==CAMERA_ACCESS_ADMIN?"PUBLICDUMMY-admin":"PUBLICDUMMY-recorder-owner");
 if(domain==CAMERA_ACCESS_ADMIN){a.roles=CAMERA_ACCESS_RECORDING_MANAGER;a.grant_count=1;a.grants[0].kind=CAMERA_ACCESS_SENSOR;
  a.grants[0].operations=CAMERA_ACCESS_RECORDING_CONFIG;id(a.grants[0].resource_id,"PUBLICDUMMY-camera0");}
 else{a.roles=CAMERA_ACCESS_VIEWER;a.grant_count=2;a.grants[0].kind=CAMERA_ACCESS_PROFILE;a.grants[0].operations=CAMERA_ACCESS_LIVE;
  id(a.grants[0].resource_id,"PUBLICDUMMY-profile0");a.grants[1].kind=CAMERA_ACCESS_SENSOR;a.grants[1].operations=CAMERA_ACCESS_EVENTS;id(a.grants[1].resource_id,"PUBLICDUMMY-camera0");}
 CHECK(camera_access_put_account(&f->policy,&a)==CAMERA_ACCESS_OK);
 memset(&v,0,sizeof(v));v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;v.domain=domain;v.boot_generation=77;
 v.credential_generation=1;v.session_generation=1;v.issued_ns=f->fake.now;v.expires_ns=f->fake.now+60000000000ULL;
 id(v.device_id,"PUBLICDUMMY-device");memcpy(v.account_id,a.account_id,CAMERA_ID_BYTES);
 CHECK(camera_access_issue(&f->policy,&v,f->fake.now,out)==CAMERA_ACCESS_OK);
}
static void setup(struct fixture *f)
{
 struct camera_access_resource r[4];struct mm_provider provider;char device[CAMERA_ID_BYTES];
 memset(f,0,sizeof(*f));f->fake.now=BASE;f->fake.engine=&f->engine;f->fake.policy=&f->policy;f->fake.observer=&f->observer;f->fake.admin=&f->admin;
 id(f->identity.sensor,"PUBLICDUMMY-camera0");id(f->identity.profile,"PUBLICDUMMY-profile0");id(f->identity.head,"PUBLICDUMMY-shared-head0");
 f->identity.graph_epoch=f->identity.source_generation=f->identity.track_generation=f->identity.head_epoch=1;
 f->identity.geometry=(struct mm_geometry){1280,720,0,0,1280,720,1280,720,1,1};
 f->devices[0].kind=CAMERA_DEVICE_SENSOR;f->devices[0].state=CAMERA_PRESENT_UNQUALIFIED;id(f->devices[0].id,"PUBLICDUMMY-camera0");
 f->devices[1].kind=CAMERA_DEVICE_PTZ;f->devices[1].state=CAMERA_PRESENT_UNQUALIFIED;id(f->devices[1].id,"PUBLICDUMMY-shared-head0");
 id(f->profile.id,"PUBLICDUMMY-profile0");id(f->profile.camera_id,"PUBLICDUMMY-camera0");f->profile.width=1280;f->profile.height=720;f->profile.codec=CAMERA_PIXELS;
 id(f->assoc.camera_id,"PUBLICDUMMY-camera0");id(f->assoc.device_id,"PUBLICDUMMY-shared-head0");
 f->graph.devices=f->devices;f->graph.device_count=2;f->graph.profiles=&f->profile;f->graph.profile_count=1;f->graph.associations=&f->assoc;f->graph.association_count=1;
 memset(r,0,sizeof(r));r[0].kind=CAMERA_ACCESS_DEVICE;id(r[0].id,"PUBLICDUMMY-device");r[1].kind=CAMERA_ACCESS_SENSOR;id(r[1].id,"PUBLICDUMMY-camera0");
 r[2].kind=CAMERA_ACCESS_PROFILE;id(r[2].id,"PUBLICDUMMY-profile0");id(r[2].sensor_id,"PUBLICDUMMY-camera0");r[3].kind=CAMERA_ACCESS_PTZ;id(r[3].id,"PUBLICDUMMY-shared-head0");
 id(device,"PUBLICDUMMY-device");CHECK(camera_access_init(&f->policy,device,77,BASE,r,4)==CAMERA_ACCESS_OK);
 issue(f,CAMERA_ACCESS_UI,&f->observer);issue(f,CAMERA_ACCESS_ADMIN,&f->admin);
 provider=(struct mm_provider){&f->fake,clock_ns,admit,acquire,valid,release};f->cfg=config();
 CHECK(mm_init(&f->engine,&provider,&f->policy,&f->observer,&f->graph,&f->identity,0x5055424c4943ULL,BASE)==MM_OK);
 CHECK(mm_stage_config(&f->engine,&f->admin,&f->cfg,BASE)==MM_OK);CHECK(mm_apply_config(&f->engine,BASE)==MM_OK);
 f->fake.span.identity=f->identity;f->fake.span.ticket=(struct mm_ticket){0x5354415453ULL,1,1};
 f->fake.span.cells=f->cells;f->fake.span.cell_count=MM_CELLS;f->fake.span.valid=1;f->fake.span.exposure_epoch=1;f->fake.span.inhibit_epoch=1;
}
static void uniform(struct fixture *f,uint8_t mean,uint8_t noise)
{unsigned i;for(i=0;i<MM_CELLS;i++)f->cells[i]=(struct mm_cell){1600,mean,noise};}
static int frame(struct fixture *f)
{
 int result;f->fake.now+=STEP;f->fake.span.frame_sequence++;f->fake.span.ticket.sequence++;
 f->fake.span.sampled_ns=f->fake.now;f->fake.span.expires_ns=f->fake.now+1000000000ULL;f->fake.span.digest_crc32=mm_stats_digest(f->cells);
 result=mm_poll(&f->engine,f->fake.now);CHECK(!f->engine.native_qualified);return result;
}
static void take(struct fixture *f,unsigned kind,unsigned reason)
{
 struct mm_event event;CHECK(mm_event_peek(&f->engine,f->fake.now,&event)==MM_OK);
 CHECK(event.kind==kind&&event.reason==reason&&!event.native_qualified);CHECK(mm_event_take(&f->engine,&event,f->fake.now)==MM_OK);
 CHECK(mm_event_take(&f->engine,&event,f->fake.now)==MM_STALE);
}
static void rectangle(struct mm_config *c,unsigned p,unsigned x,unsigned y,unsigned w,unsigned h)
{
 c->polygon_count=p+1;c->polygons[p].vertices=4;
 c->polygons[p].point[0]=(struct mm_point){(uint16_t)x,(uint16_t)y};c->polygons[p].point[1]=(struct mm_point){(uint16_t)(x+w),(uint16_t)y};
 c->polygons[p].point[2]=(struct mm_point){(uint16_t)(x+w),(uint16_t)(y+h)};c->polygons[p].point[3]=(struct mm_point){(uint16_t)x,(uint16_t)(y+h)};
}
static void geometry_rectangle_oracle_and_validation(void)
{
 struct mm_geometry g={1301,739,5,7,1280,720,640,360,0,0};struct mm_config c=config();uint8_t mask[MM_MASK_BYTES],before[MM_MASK_BYTES];
 struct mm_rational_point a,b;unsigned fx,fy,x,y,cell,edge,case_number;
 suites++;
 for(fx=0;fx<2;fx++)for(fy=0;fy<2;fy++){
  g.flip_x=fx;g.flip_y=fy;
  for(edge=0;edge<640;edge++){
   c=config();rectangle(&c,0,edge,30,1,100);CHECK(mm_mask_rasterize(&c,&g,mask)==MM_OK);
   for(cell=0;cell<MM_CELLS;cell++){
    unsigned col=cell%MM_COLS,row=cell/MM_COLS;
    /* Independent rectangle interval oracle in DISPLAY rational units.
     * Grid native40x40, display scaled1/2. Contact is deliberately ignored. */
    unsigned left=fx?640-(col+1)*20:col*20,right=left+20;
    unsigned top=fy?360-(row+1)*20:row*20,bottom=top+20;
    unsigned expected=left<=edge+1&&right>=edge&&top<=130&&bottom>=30;
    CHECK((((unsigned)mask[cell/8]>>(cell%8))&1U)==expected);
   }
  }
  for(x=0;x<=640;x+=40)for(y=0;y<=360;y+=40){
   struct mm_point p={(uint16_t)x,(uint16_t)y},sensor;
   CHECK(mm_display_to_sensor(&g,p,&a)==MM_OK);CHECK(a.x_num%a.x_den==0&&a.y_num%a.y_den==0);
   sensor=(struct mm_point){(uint16_t)(a.x_num/a.x_den),(uint16_t)(a.y_num/a.y_den)};
   CHECK(mm_sensor_to_display(&g,sensor,&b)==MM_OK);CHECK(b.x_num==(int64_t)x*b.x_den&&b.y_num==(int64_t)y*b.y_den);
  }
 }
 for(case_number=0;case_number<9;case_number++){
  c=config();rectangle(&c,0,10,10,100,100);memset(mask,0xa7,sizeof(mask));memcpy(before,mask,sizeof(mask));
  if(case_number==0)c.schema=0;
  if(case_number==1)c.polygons[0].point[0]=c.polygons[0].point[1];
  if(case_number==2)c.polygons[0].point[1].x=UINT16_MAX;
  if(case_number==3){struct mm_point p=c.polygons[0].point[1];c.polygons[0].point[1]=c.polygons[0].point[2];c.polygons[0].point[2]=p;}
  if(case_number==4)c.polygons[0].point[4].x=1;
  if(case_number==5)c.polygon_count=9;
  if(case_number==6)c.polygons[0].vertices=17;
  if(case_number==7)c.polygons[0].point[2]=(struct mm_point){210,10};
  if(case_number==8){struct mm_point p=c.polygons[0].point[1];c.polygons[0].point[1]=c.polygons[0].point[3];c.polygons[0].point[3]=p;}
  CHECK(mm_mask_rasterize(&c,&g,mask)==MM_INVALID);CHECK(!memcmp(mask,before,sizeof(mask)));
 }
 c=config();rectangle(&c,0,20,20,100,100);rectangle(&c,1,20,20,100,100);CHECK(mm_mask_rasterize(&c,&g,mask)==MM_OK);
 c.polygon_count=1;memset(&c.polygons[1],0,sizeof(c.polygons[1]));CHECK(mm_mask_rasterize(&c,&g,before)==MM_OK);CHECK(!memcmp(mask,before,sizeof(mask)));
}
static void local_noise_flicker_event_duration(void)
{
 struct fixture f;unsigned i,step;
 suites++;setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK&&f.engine.status==MM_BASELINE);
 for(step=0;step<3;step++){
  uniform(&f,100,0);for(i=0;i<30;i++)f.cells[100+i].mean=160;
  CHECK(frame(&f)==MM_OK);
 }
 CHECK(f.engine.active&&f.engine.status==MM_MOVING);take(&f,MM_BEGIN,MM_NONE);
 uniform(&f,100,0);CHECK(frame(&f)==MM_OK);CHECK(f.engine.active);CHECK(frame(&f)==MM_OK);CHECK(f.engine.active);
 CHECK(frame(&f)==MM_OK);CHECK(!f.engine.active);take(&f,MM_END,MM_NONE);
 for(step=0;step<4;step++){uniform(&f,100,12);for(i=0;i<MM_CELLS;i++)if((i+step)%17==0)f.cells[i].mean=130;CHECK(frame(&f)==MM_OK);}
 CHECK(!f.engine.active&&f.engine.event_count==0);
 uniform(&f,125,0);CHECK(frame(&f)==MM_OK);CHECK(f.engine.status==MM_INHIBITED&&f.engine.reason==MM_GLOBAL_LUMA&&!f.engine.active);
 CHECK(frame(&f)==MM_OK&&f.engine.status==MM_QUIET);
}
static void masked_patch_control_changes_and_inhibit(void)
{
 struct fixture f;unsigned i,step;struct mm_config next;
 suites++;setup(&f);next=f.cfg;next.generation=2;rectangle(&next,0,0,0,200,200);
 CHECK(mm_stage_config(&f.engine,&f.admin,&next,f.fake.now)==MM_OK);CHECK(mm_apply_config(&f.engine,f.fake.now)==MM_OK);
 uniform(&f,100,0);CHECK(frame(&f)==MM_OK);
 for(step=0;step<4;step++){uniform(&f,100,0);for(i=0;i<MM_CELLS;i++)if(f.engine.mask[i/8]&(1U<<(i%8)))f.cells[i].mean=200;CHECK(frame(&f)==MM_OK);}
 CHECK(!f.engine.active&&f.engine.event_count==0);
 for(step=0;step<3;step++){uniform(&f,100,0);for(i=0;i<50;i++)f.cells[i].mean=200;CHECK(frame(&f)==MM_OK);}
 CHECK(f.engine.active);take(&f,MM_BEGIN,MM_NONE);
 f.fake.span.inhibit_flags=MM_INHIBIT_PTZ;CHECK(frame(&f)==MM_OK);CHECK(f.engine.status==MM_INHIBITED&&f.engine.reason==MM_PTZ);
 take(&f,MM_TERMINATED_GAP,MM_PTZ);f.fake.span.inhibit_flags=MM_INHIBIT_DAYNIGHT;CHECK(frame(&f)==MM_OK);CHECK(f.engine.reason==MM_DAYNIGHT);
 f.fake.span.inhibit_flags=0;f.fake.span.exposure_epoch++;CHECK(frame(&f)==MM_OK);CHECK(f.engine.reason==MM_EXPOSURE);
 f.fake.span.identity.head[0]='X';f.fake.now+=STEP;f.fake.span.sampled_ns=f.fake.now;f.fake.span.expires_ns=f.fake.now+STEP;
 CHECK(mm_poll(&f.engine,f.fake.now)==MM_UNAVAILABLE);CHECK(f.engine.owned&&f.engine.reason==MM_SOURCE);
 CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);
}
static void policy_schema_staging_and_lifetime(void)
{
 struct fixture f;struct motion_engine before;struct mm_config next;struct camera_access_principal bad={0};unsigned count;
 suites++;setup(&f);next=f.cfg;next.generation=2;
 memcpy(&before,&f.engine,sizeof(before));CHECK(mm_stage_config(&f.engine,&bad,&next,f.fake.now)==MM_DENIED);
 CHECK(!memcmp(&before,&f.engine,sizeof(before)));next.polygons[0].vertices=2;
 CHECK(mm_stage_config(&f.engine,&f.admin,&next,f.fake.now)==MM_INVALID);CHECK(!memcmp(&before,&f.engine,sizeof(before)));
 next=f.cfg;next.generation=2;CHECK(mm_stage_config(&f.engine,&f.admin,&next,f.fake.now)==MM_OK);
 CHECK(camera_access_revoke_session(&f.policy,&f.admin)==CAMERA_ACCESS_OK);count=f.fake.acquires;
 CHECK(mm_apply_config(&f.engine,f.fake.now)==MM_DENIED);CHECK(f.engine.config.generation==1&&!f.engine.pending&&f.fake.acquires==count);
 setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);count=f.fake.acquires;
 CHECK(camera_access_revoke_session(&f.policy,&f.observer)==CAMERA_ACCESS_OK);
 CHECK(frame(&f)==MM_DENIED);CHECK(f.fake.acquires==count&&!f.engine.owned);
 setup(&f);uniform(&f,100,0);f.fake.deny_tile=20;
 CHECK(frame(&f)==MM_DENIED);CHECK(f.engine.owned&&!f.engine.have_background);CHECK(f.fake.releases==0);
 CHECK(mm_poll(&f.engine,f.fake.now)==MM_BUSY);CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK&&f.fake.releases==1);
 CHECK(mm_cancel(&f.engine,f.fake.now)==MM_EMPTY&&f.fake.releases==1);
 setup(&f);uniform(&f,100,0);f.fake.release_result=-1;CHECK(frame(&f)==MM_RETAINED);
 CHECK(f.engine.status==MM_QUARANTINE&&f.engine.owned);count=f.fake.releases;
 CHECK(mm_cancel(&f.engine,f.fake.now)==MM_RETAINED&&f.fake.releases==count);
 setup(&f);uniform(&f,100,0);f.fake.acquire_result=-1;CHECK(frame(&f)==MM_RETAINED);
 CHECK(f.engine.owned&&f.engine.uncertain);CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);
 setup(&f);uniform(&f,100,0);f.fake.reentry=1;CHECK(frame(&f)==MM_OK);CHECK(f.fake.reentered==MM_INVALID);
}
static void gaps_integrity_unknown_queue_and_counters(void)
{
 struct fixture f;struct mm_event event,bad;unsigned i,step,count;
 suites++;setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);
 f.fake.now+=f.cfg.max_gap_ns-STEP;CHECK(frame(&f)==MM_OK);CHECK(f.engine.status==MM_QUIET); /* equality allowed */
 f.fake.now+=f.cfg.max_gap_ns;CHECK(frame(&f)==MM_OK);CHECK(f.engine.status==MM_INHIBITED&&f.engine.reason==MM_GAP);
 f.cells[0].pixels--;CHECK(frame(&f)==MM_UNAVAILABLE&&f.engine.reason==MM_COVERAGE);CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);
 setup(&f);uniform(&f,100,0);f.fake.now+=STEP;f.fake.span.sampled_ns=f.fake.now;f.fake.span.expires_ns=f.fake.now;
 CHECK(mm_poll(&f.engine,f.fake.now)==MM_UNAVAILABLE&&f.engine.owned);CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);
 setup(&f);uniform(&f,100,0);f.fake.acquire_result=1;CHECK(frame(&f)==MM_UNAVAILABLE);CHECK(!f.engine.owned&&f.engine.status==MM_UNKNOWN);
 setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);f.engine.next_event=UINT64_MAX;
 for(step=0;step<2;step++){for(i=0;i<40;i++)f.cells[i].mean=200;CHECK(frame(&f)==MM_OK);}
 CHECK(frame(&f)==MM_EXHAUSTED);CHECK(f.engine.owned&&f.engine.event_count==0);CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);
 setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);
 for(step=0;step<3;step++){for(i=0;i<40;i++)f.cells[i].mean=200;CHECK(frame(&f)==MM_OK);}
 CHECK(mm_event_peek(&f.engine,f.fake.now,&event)==MM_OK);bad=event;bad.sequence++;
 CHECK(mm_event_take(&f.engine,&bad,f.fake.now)==MM_STALE);CHECK(f.engine.event_count==1);
 CHECK(mm_event_peek(&f.engine,f.fake.now,&f.engine.events[0])==MM_INVALID);
 uniform(&f,100,0);for(step=0;step<3;step++)CHECK(frame(&f)==MM_OK);
 CHECK(f.engine.event_count==2);count=f.fake.acquires;CHECK(frame(&f)==MM_BUSY&&f.fake.acquires==count);
 CHECK(mm_event_take(&f.engine,&event,f.fake.now)==MM_OK);CHECK(mm_event_take(&f.engine,&event,f.fake.now)==MM_STALE);
 CHECK(mm_event_peek(&f.engine,f.fake.now,&event)==MM_OK);
 f.fake.now=event.expires_ns;CHECK(mm_event_take(&f.engine,&event,f.fake.now)==MM_STALE);
 CHECK(f.engine.status==MM_UNKNOWN&&f.engine.reason==MM_GAP);
}
static void fuzz_digest_config_boundary_clock_and_lost_receipt(void)
{
 struct fixture f;struct mm_config c;struct mm_geometry g={4096,2160,3,5,4093,2155,1301,739,1,1};
 uint8_t out[MM_MASK_BYTES],unchanged[MM_MASK_BYTES];uint64_t random=0x5055424c4943ULL;unsigned n,i,step,count;struct mm_event event;
 suites++;
 for(n=0;n<256;n++){
  c=config();rectangle(&c,0,10,10,100,100);random=random*6364136223846793005ULL+1;
  c.polygons[0].vertices=3+(uint32_t)((random>>7)%14);
  for(i=0;i<c.polygons[0].vertices;i++){random=random*6364136223846793005ULL+1;c.polygons[0].point[i]=(struct mm_point){(uint16_t)(random>>32),(uint16_t)random};}
  memset(out,0xa7,sizeof(out));memcpy(unchanged,out,sizeof(out));
  if(mm_mask_rasterize(&c,&g,out)==MM_INVALID)CHECK(!memcmp(out,unchanged,sizeof(out)));
 }
 setup(&f);uniform(&f,100,0);f.fake.now+=STEP;f.fake.span.frame_sequence=1;f.fake.span.sampled_ns=f.fake.now;
 f.fake.span.expires_ns=f.fake.now+STEP;f.fake.span.digest_crc32=mm_stats_digest(f.cells)^1U;
 CHECK(mm_poll(&f.engine,f.fake.now)==MM_UNAVAILABLE&&f.engine.reason==MM_DIGEST);CHECK(!f.engine.have_background);
 CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);
 setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);
 f.fake.now--;count=f.fake.acquires;CHECK(mm_poll(&f.engine,f.fake.now)==MM_STALE);
 CHECK(f.engine.status==MM_UNKNOWN&&f.engine.reason==MM_CLOCK&&f.engine.fault&&f.fake.acquires==count);
 f.fake.now+=2*STEP;CHECK(mm_poll(&f.engine,f.fake.now)==MM_UNAVAILABLE&&f.fake.acquires==count);
 setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);c=f.cfg;c.generation=2;rectangle(&c,0,0,0,120,120);
 CHECK(mm_stage_config(&f.engine,&f.admin,&c,f.fake.now)==MM_OK);
 f.fake.revoke_admin_at=f.fake.admissions+2; /* revoke during observer/resource check AFTER first admin proof */
 CHECK(mm_apply_config(&f.engine,f.fake.now)==MM_DENIED);
 CHECK(!f.engine.pending&&f.engine.config.generation==1);
 setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);
 for(step=0;step<3;step++){for(i=0;i<40;i++)f.cells[i].mean=200;CHECK(frame(&f)==MM_OK);}
 CHECK(mm_event_peek(&f.engine,f.fake.now,&event)==MM_OK);count=(unsigned)f.engine.next_event;
 CHECK(mm_event_peek(&f.engine,f.fake.now,&event)==MM_OK&&f.engine.next_event==count);
 f.fake.now=event.expires_ns;CHECK(mm_event_peek(&f.engine,f.fake.now,&event)==MM_STALE);
 CHECK(f.engine.status==MM_UNKNOWN&&f.engine.reason==MM_GAP); /* lost receipt is NOT repeat/quiet */
}
static void host_stats_cost(void)
{
 struct fixture f;struct timespec before,after;uint64_t ns;unsigned i;
 setup(&f);uniform(&f,100,0);
 CHECK(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&before)==0);
 for(i=0;i<256;i++)CHECK(frame(&f)==MM_OK);
 CHECK(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&after)==0);
 ns=(uint64_t)(after.tv_sec-before.tv_sec)*1000000000ULL;
 if(after.tv_nsec>=before.tv_nsec)ns+=(uint64_t)(after.tv_nsec-before.tv_nsec);else ns-=(uint64_t)(before.tv_nsec-after.tv_nsec);
 printf("HOST process_cpu_ns=%llu synthetic_frames=256 tiles=147456 NOT_ARM_15fps_HEADROOM\n",(unsigned long long)ns);
}
static void final_callback_revocation_no_raw_effect(void)
{
 struct fixture f;uint8_t committed[MM_CELLS];unsigned phase,count,i;const unsigned at[]={2,4,6,2+2*MM_CELLS+2};
 struct mm_event event,sentinel;
 suites++;
 for(phase=0;phase<4;phase++){
  setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);memcpy(committed,f.engine.background,sizeof(committed));
  uniform(&f,108,0);f.fake.valids=0;f.fake.revoke_valid_at=at[phase];count=f.fake.releases;
  CHECK(frame(&f)==MM_DENIED);
  CHECK(f.engine.tiles_read==(phase==2?1U:(phase==3?MM_CELLS:0U))); /* actual loads, not guessed from callbacks */
  CHECK(!memcmp(committed,f.engine.background,sizeof(committed))&&f.engine.event_count==0);
  CHECK(f.engine.owned&&!f.engine.return_attempted&&f.fake.releases==count);
  CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK&&f.fake.releases==count+1);
  CHECK(mm_cancel(&f.engine,f.fake.now)==MM_EMPTY&&f.fake.releases==count+1);
 }
 setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);
 for(phase=0;phase<2;phase++){for(i=0;i<40;i++)f.cells[i].mean=200;CHECK(frame(&f)==MM_OK);}
 memcpy(committed,f.engine.background,sizeof(committed));f.fake.valids=0;
 f.fake.events_only_at=2+2*MM_CELLS+4; /* final EVENTS-valid callback rotates into actual EVENTS-only principal */
 CHECK(frame(&f)==MM_DENIED);CHECK(f.engine.tiles_read==MM_CELLS);
 CHECK(!memcmp(committed,f.engine.background,sizeof(committed))&&!f.engine.event_count);
 CHECK(f.engine.owned);CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);
 /* Event queries/takes have no raw-source valid callback: test LAST clock
  * callback and direct post-callback policy, not a fictional provider hook. */
 for(phase=0;phase<2;phase++){
  setup(&f);uniform(&f,100,0);CHECK(frame(&f)==MM_OK);
  for(i=0;i<3;i++){unsigned cell;for(cell=0;cell<40;cell++)f.cells[cell].mean=200;CHECK(frame(&f)==MM_OK);}
  CHECK(mm_event_peek(&f.engine,f.fake.now,&event)==MM_OK);count=f.engine.event_count;
  f.fake.revoke_clock_at=f.fake.clock_reads+3;memset(&sentinel,0xa7,sizeof(sentinel));
  if(phase==0){event=sentinel;CHECK(mm_event_peek(&f.engine,f.fake.now,&event)==MM_DENIED);CHECK(!memcmp(&event,&sentinel,sizeof(event)));}
  else CHECK(mm_event_take(&f.engine,&event,f.fake.now)==MM_DENIED);
  CHECK(f.engine.event_count==count);
 }
 printf("final-policy=PASS revoke at second valid/cell0/cell1/final-bg/events-only rotation/query/take; no unauthorized load or bg/event commit\n");
}
static void callback_timing_actual_lifetime(void)
{
 struct fixture f;unsigned mode,count;uint64_t delay;
 suites++;CHECK(MM_CALLBACK_NS==1000000ULL);
 for(mode=0;mode<3;mode++){
  setup(&f);uniform(&f,100,0);f.fake.acquire_delay=MM_CALLBACK_NS+1;
  if(mode==1)f.fake.acquire_result=1;
  if(mode==2)f.fake.acquire_result=-1;
  CHECK(frame(&f)==(mode==2?MM_RETAINED:MM_STALE));
  CHECK(f.engine.fault&&f.engine.status==MM_UNKNOWN&&f.engine.reason==MM_CLOCK&&f.engine.tiles_read==0);
  CHECK(f.engine.owned==(mode!=1)&&f.engine.uncertain==(mode==2));count=f.fake.acquires;
  if(mode!=1){CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);CHECK(f.fake.releases==1);}
  else CHECK(mm_cancel(&f.engine,f.fake.now)==MM_EMPTY&&f.fake.releases==0);
  CHECK(mm_poll(&f.engine,f.fake.now)==MM_UNAVAILABLE&&f.fake.acquires==count);
 }
 setup(&f);uniform(&f,100,0);f.fake.acquire_delay=MM_CALLBACK_NS;
 CHECK(frame(&f)==MM_OK&&f.engine.tiles_read==MM_CELLS); /* equality<=1ms admitted */
 setup(&f);uniform(&f,100,0);f.fake.revoke_acquire=1;
 CHECK(frame(&f)==MM_DENIED&&f.engine.tiles_read==0&&f.engine.owned);
 CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK&&f.fake.releases==1);
 for(mode=0;mode<2;mode++){
  setup(&f);uniform(&f,100,0);f.fake.release_delay=MM_CALLBACK_NS+1;f.fake.release_result=mode?-1:0;
  CHECK(frame(&f)==(mode?MM_RETAINED:MM_STALE));count=f.fake.releases;
  if(!mode){CHECK(!f.engine.owned&&!f.engine.uncertain&&f.engine.fault&&f.engine.reason==MM_CLOCK);
   CHECK(mm_cancel(&f.engine,f.fake.now)==MM_EMPTY);CHECK(mm_poll(&f.engine,f.fake.now)==MM_UNAVAILABLE);}
  else{CHECK(f.engine.owned&&f.engine.uncertain&&f.engine.status==MM_QUARANTINE);CHECK(mm_cancel(&f.engine,f.fake.now)==MM_RETAINED);}
  CHECK(f.fake.releases==count); /* normal ACK or uncertainty, never duplicate actual return */
 }
 for(mode=0;mode<2;mode++)for(delay=MM_CALLBACK_NS;delay<=MM_CALLBACK_NS+1;delay++){
  setup(&f);uniform(&f,100,0);
  if(mode==0)f.fake.valid_delay=delay;else f.fake.admit_delay=delay;
  if(mode==1&&delay==MM_CALLBACK_NS){CHECK(frame(&f)==MM_OK);CHECK(f.engine.tiles_read==MM_CELLS);}
  else{CHECK(frame(&f)!=MM_OK);CHECK(f.engine.tiles_read==0&&!f.engine.have_background);
   if(f.engine.owned)CHECK(mm_cancel(&f.engine,f.fake.now)==MM_OK);}
 }
}
int main(int argc,char **argv)
{
 geometry_rectangle_oracle_and_validation();local_noise_flicker_event_duration();masked_patch_control_changes_and_inhibit();
 policy_schema_staging_and_lifetime();gaps_integrity_unknown_queue_and_counters();
 fuzz_digest_config_boundary_clock_and_lost_receipt();
 final_callback_revocation_no_raw_effect();callback_timing_actual_lifetime();
 if(argc==2&&!strcmp(argv[1],"--host-cost"))host_stats_cost();
 printf("HOST PUBLICDUMMY MOTION PASS suites=%u checks=%u state_bytes=%zu stats_borrowed_bytes=%zu no_frames_or_footage_IO native_qualified=0\n",
  suites,checks,sizeof(struct motion_engine),sizeof(struct mm_cell)*MM_CELLS);return 0;
}
