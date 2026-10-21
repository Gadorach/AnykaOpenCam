/* SPDX-License-Identifier: GPL-2.0-only */
/* Original V1 API/source only. Actual shared policy, synthetic cells. */
#include <motion.h>
#include <stdio.h>
#include <string.h>
struct old_fixture {uint64_t now;unsigned valids,revoke_at;struct motion_engine engine;
 struct camera_access_policy policy;struct camera_access_principal observer,admin;
 struct mm_span source;struct mm_cell cells[MM_CELLS];};
static void id(char *p,const char *s){memset(p,0,CAMERA_ID_BYTES);memcpy(p,s,strlen(s));}
static uint64_t clock_ns(void *p){return ((struct old_fixture *)p)->now;}
static int admit(void *p,const struct mm_identity *i,uint64_t n,uint64_t *until)
{(void)p;(void)i;*until=n+1000000000ULL;return 1;}
static int acquire(void *p,const struct mm_identity *i,struct mm_span *out)
{(void)i;*out=((struct old_fixture *)p)->source;return 0;}
static int valid(void *p,const struct mm_span *s,uint64_t n)
{struct old_fixture *f=p;(void)s;(void)n;f->valids++;
 if(f->revoke_at==f->valids&&camera_access_revoke_session(&f->policy,&f->observer))return 0;
 return 1;}
static int release(void *p,const struct mm_ticket *t,uint64_t n){(void)p;(void)t;(void)n;return 0;}
static int principal(struct old_fixture *f,int admin,struct camera_access_principal *out)
{
 struct camera_access_account a={0};struct camera_access_verified v={0};
 a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=admin?CAMERA_ACCESS_ADMIN:CAMERA_ACCESS_UI;
 a.roles=admin?CAMERA_ACCESS_RECORDING_MANAGER:CAMERA_ACCESS_VIEWER;a.credential_generation=1;a.grant_count=admin?1:2;
 id(a.account_id,admin?"PUBLICDUMMY-admin":"PUBLICDUMMY-observer");
 a.grants[0].kind=admin?CAMERA_ACCESS_SENSOR:CAMERA_ACCESS_PROFILE;a.grants[0].operations=admin?CAMERA_ACCESS_RECORDING_CONFIG:CAMERA_ACCESS_LIVE;
 id(a.grants[0].resource_id,admin?"PUBLICDUMMY-camera0":"PUBLICDUMMY-profile0");
 if(!admin){a.grants[1].kind=CAMERA_ACCESS_SENSOR;a.grants[1].operations=CAMERA_ACCESS_EVENTS;id(a.grants[1].resource_id,"PUBLICDUMMY-camera0");}
 if(camera_access_put_account(&f->policy,&a))return 2;
 v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;v.domain=a.domain;v.boot_generation=77;v.credential_generation=1;v.session_generation=1;
 v.issued_ns=f->now;v.expires_ns=f->now+10000000000ULL;id(v.device_id,"PUBLICDUMMY-device");memcpy(v.account_id,a.account_id,CAMERA_ID_BYTES);
 return camera_access_issue(&f->policy,&v,f->now,out);
}
int main(void)
{
 struct old_fixture f={0};struct camera_device d={0};struct camera_profile profile={0};struct camera_graph graph={0};
 struct mm_identity identity={0};struct mm_config cfg={0};struct mm_provider provider={&f,clock_ns,admit,acquire,valid,release};
 struct camera_access_resource r[3]={{0}};char device[CAMERA_ID_BYTES];unsigned i;int result;
 f.now=1000000000ULL;d.kind=CAMERA_DEVICE_SENSOR;d.state=CAMERA_PRESENT_UNQUALIFIED;id(d.id,"PUBLICDUMMY-camera0");
 id(profile.id,"PUBLICDUMMY-profile0");id(profile.camera_id,"PUBLICDUMMY-camera0");profile.width=1280;profile.height=720;
 graph.devices=&d;graph.device_count=1;graph.profiles=&profile;graph.profile_count=1;
 id(identity.sensor,"PUBLICDUMMY-camera0");id(identity.profile,"PUBLICDUMMY-profile0");identity.graph_epoch=identity.source_generation=identity.track_generation=1;
 identity.geometry=(struct mm_geometry){1280,720,0,0,1280,720,1280,720,1,1};
 r[0].kind=CAMERA_ACCESS_DEVICE;id(r[0].id,"PUBLICDUMMY-device");r[1].kind=CAMERA_ACCESS_SENSOR;id(r[1].id,"PUBLICDUMMY-camera0");
 r[2].kind=CAMERA_ACCESS_PROFILE;id(r[2].id,"PUBLICDUMMY-profile0");id(r[2].sensor_id,"PUBLICDUMMY-camera0");id(device,"PUBLICDUMMY-device");
 if(camera_access_init(&f.policy,device,77,f.now,r,3)||principal(&f,0,&f.observer)||principal(&f,1,&f.admin))return 2;
 if(mm_init(&f.engine,&provider,&f.policy,&f.observer,&graph,&identity,1,f.now))return 2;
 cfg.schema=1;cfg.generation=1;cfg.delta_min=12;cfg.noise_multiplier=2;cfg.area_permille=30;cfg.global_delta=8;cfg.global_permille=900;
 cfg.debounce_ns=cfg.release_ns=200000000;cfg.min_event_ns=500000000;cfg.max_gap_ns=500000000;cfg.event_ttl_ns=2000000000;
 if(mm_stage_config(&f.engine,&f.admin,&cfg,f.now)||mm_apply_config(&f.engine,f.now))return 2;
 f.source.identity=identity;f.source.ticket=(struct mm_ticket){1,1,1};f.source.cells=f.cells;f.source.cell_count=MM_CELLS;f.source.valid=1;f.source.exposure_epoch=1;
 for(i=0;i<MM_CELLS;i++)f.cells[i]=(struct mm_cell){1600,100,0};
 f.source.frame_sequence=1;f.source.sampled_ns=f.now;f.source.expires_ns=f.now+1000000000ULL;f.source.digest_crc32=mm_stats_digest(f.cells);
 if(mm_poll(&f.engine,f.now))return 2;
 for(i=0;i<MM_CELLS;i++)f.cells[i].mean=108;
 f.now+=100000000;f.source.frame_sequence=2;f.source.ticket.sequence=2;f.source.sampled_ns=f.now;f.source.expires_ns=f.now+1000000000ULL;
 f.source.digest_crc32=mm_stats_digest(f.cells);f.valids=0;f.revoke_at=4; /* initial2 + cell0 second/final valid */
 result=mm_poll(&f.engine,f.now);
 printf("V1 final-valid regression result=%d revoked_at=4 staging0=%u committed_bg0=%u owned=%u\n",result,(unsigned)f.engine.staging[0],(unsigned)f.engine.background[0],f.engine.owned);
 return result==MM_DENIED&&f.engine.staging[0]==108&&f.engine.background[0]==100&&f.engine.owned?1:2;
}
