/* SPDX-License-Identifier: GPL-2.0-only */
#include "motion.h"
#include <string.h>
_Static_assert(sizeof(struct motion_engine)<=MM_STORAGE_CAP,"4KiB motion state cap");
static int zero(const void *v,size_t n){const uint8_t *p=v;size_t i;for(i=0;i<n;i++)if(p[i])return 0;return 1;}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!an||!bn)return 0;if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;return x<y+bn&&y<x+an;}
static int id_valid(const char *id,int empty)
{size_t i;if(!id)return 0;if(!id[0])return empty&&zero(id,CAMERA_ID_BYTES);for(i=0;i<CAMERA_ID_BYTES;i++){
 unsigned char c=(unsigned char)id[i];if(!c)return zero(id+i,CAMERA_ID_BYTES-i);
 if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;}return 0;}
static int same_id(const char *a,const char *b){return !memcmp(a,b,CAMERA_ID_BYTES);}
static int live(const struct motion_engine *e){return e&&e->self==e&&!e->busy;}
static int disjoint(const struct motion_engine *e,const void *p,size_t n)
{return p&&!overlap(e,sizeof(*e),p,n)&&!overlap(e->policy,sizeof(*e->policy),p,n)&&
 (!e->owned||!e->span.cells||!overlap(e->span.cells,MM_CELLS*sizeof(*e->span.cells),p,n));}
static uint64_t clock_now(struct motion_engine *e)
{uint64_t n;e->busy=1;n=e->provider.clock_ns(e->provider.context);e->busy=0;return n;}
static int permission(struct motion_engine *e,const struct camera_access_principal *p,uint32_t op,uint64_t now)
{
 struct camera_access_request q;memset(&q,0,sizeof(q));q.abi=CAMERA_ACCESS_ABI;q.struct_bytes=sizeof(q);
 q.domain=p->fact.domain;q.operation=op;memcpy(q.device_id,e->policy->device_id,CAMERA_ID_BYTES);
 if(op==CAMERA_ACCESS_LIVE){q.kind=CAMERA_ACCESS_PROFILE;memcpy(q.sensor_id,e->identity.sensor,CAMERA_ID_BYTES);memcpy(q.profile_id,e->identity.profile,CAMERA_ID_BYTES);}
 else{q.kind=CAMERA_ACCESS_SENSOR;memcpy(q.sensor_id,e->identity.sensor,CAMERA_ID_BYTES);}
 if(op==CAMERA_ACCESS_RECORDING_CONFIG&&q.domain!=CAMERA_ACCESS_ADMIN)return MM_DENIED;
 return camera_access_check(e->policy,p,&q,now)==CAMERA_ACCESS_OK?MM_OK:MM_DENIED;
}
static int source_shape(const struct motion_engine *e,uint64_t now)
{
 if(!e->owned||e->uncertain||!e->span.cells||e->span.cell_count!=MM_CELLS||e->span.valid!=1||e->span.native_qualified||
    memcmp(&e->span.identity,&e->identity,sizeof(e->identity))||
    !e->span.ticket.namespace_epoch||!e->span.ticket.sequence||e->span.ticket.source_generation!=e->identity.source_generation||
    e->span.sampled_ns>now||now>=e->span.expires_ns||!e->span.exposure_epoch)return 0;
 return 1;
}
static int source_valid(struct motion_engine *e,uint64_t now)
{
 int result;if(!source_shape(e,now))return 0;
 e->busy=1;result=e->provider.valid(e->provider.context,&e->span,now);e->busy=0;return result==1;
}
static int check(struct motion_engine *e,const struct camera_access_principal *principal,uint32_t operation,uint64_t minimum,int need_source)
{
 uint64_t start,middle,end,until=0;int result;
 start=clock_now(e);if(start<minimum||start<e->last_ns)return MM_STALE;
 if(permission(e,principal,operation,start))return MM_DENIED;
 if(need_source&&!source_valid(e,start))return MM_UNAVAILABLE;
 e->busy=1;result=e->provider.admit(e->provider.context,&e->identity,start,&until);e->busy=0;
 middle=clock_now(e);if(middle<start||middle-start>MM_CALLBACK_NS)return MM_STALE;
 if(result!=1||middle>=until||permission(e,principal,operation,middle))return MM_DENIED;
 if(need_source&&!source_valid(e,middle))return MM_UNAVAILABLE;
 end=clock_now(e);if(end<middle||end-start>MM_CALLBACK_NS||end>=until||end>=principal->fact.expires_ns)return MM_DENIED;
 /* LAST external callback has finished. No provider call may intervene before
  * the effect. Recheck real current authority, not only cached time/expiry.
  * EVENTS can remain granted after LIVE removal: raw/staged statistics and
  * background STILL require that exact profile LIVE grant. */
 if(permission(e,principal,operation,end)||(need_source&&permission(e,&e->observer,CAMERA_ACCESS_LIVE,end)))return MM_DENIED;
 if(need_source&&!source_shape(e,end))return MM_UNAVAILABLE;
 e->last_ns=end;return MM_OK;
}
static uint32_t crc_byte(uint32_t crc,uint8_t v)
{unsigned i;crc^=v;for(i=0;i<8;i++)crc=(crc>>1)^((crc&1)?0xedb88320U:0U);return crc;}
uint32_t mm_stats_digest(const struct mm_cell cells[MM_CELLS])
{uint32_t crc=~0U,i;if(!cells)return 0;for(i=0;i<MM_CELLS;i++){
 crc=crc_byte(crc,(uint8_t)cells[i].pixels);crc=crc_byte(crc,(uint8_t)(cells[i].pixels>>8));crc=crc_byte(crc,cells[i].mean);crc=crc_byte(crc,cells[i].noise);}return ~crc;}
static int enqueue(struct motion_engine *e,uint32_t kind,uint32_t reason,uint64_t at,uint64_t started)
{
 struct mm_event event;
 if(e->event_count==2)return MM_BUSY;
 if(!e->next_event||e->next_event==UINT64_MAX||UINT64_MAX-at<e->config.event_ttl_ns)return MM_EXHAUSTED;
 memset(&event,0,sizeof(event));event.namespace_epoch=e->namespace_epoch;event.sequence=e->next_event++;
 event.source_generation=e->identity.source_generation;event.config_generation=e->config.generation;
 event.started_ns=started;event.observed_ns=at;event.expires_ns=at+e->config.event_ttl_ns;event.kind=kind;event.reason=reason;
 e->events[e->event_count++]=event;return MM_OK;
}
static void unavailable(struct motion_engine *e,uint32_t reason)
{
 e->status=MM_UNKNOWN;e->reason=reason;e->have_background=0;e->candidate=e->quiet=0;
 if(e->active){
  /* Explicit terminal/gap, NOT quiet/continuous recording acceptance. If privacy
   * denies this event, status/reason retain the termination without publishing. */
  if(!check(e,&e->observer,CAMERA_ACCESS_EVENTS,e->last_ns,0)){
   if(enqueue(e,MM_TERMINATED_GAP,reason,e->last_ns,e->active_ns))e->reason=MM_QUEUE_FULL;
  }
  e->active=0;
 }
}
static int return_loan(struct motion_engine *e,uint64_t now)
{
 int result;uint64_t start,end;
 if(!e->owned)return MM_EMPTY;
 if(e->return_attempted)return MM_RETAINED;
 if(!e->span.ticket.namespace_epoch||!e->span.ticket.sequence)return MM_RETAINED;
 start=clock_now(e);if(start<now||start<e->last_ns)return MM_STALE;
 e->return_attempted=1;e->busy=1;result=e->provider.release(e->provider.context,&e->span.ticket,start);e->busy=0;
 /* Preserve actual normal return BEFORE any further callback/time check. */
 if(result==0)e->owned=e->uncertain=0;
 end=clock_now(e);
 if(result!=0){e->status=MM_QUARANTINE;e->reason=MM_RETURN_UNKNOWN;e->uncertain=1;return MM_RETAINED;}
 if(end<start||end-start>MM_CALLBACK_NS){unavailable(e,MM_CLOCK);e->fault=1;return MM_STALE;}
 e->last_ns=end;return MM_OK;
}
int mm_init(struct motion_engine *e,const struct mm_provider *provider,struct camera_access_policy *policy,
 const struct camera_access_principal *observer,const struct camera_graph *graph,const struct mm_identity *id,uint64_t space,uint64_t now)
{
 size_t i;int sensor=0,profile=0,head=0,association=0;struct mm_rational_point dummy;struct mm_point origin={0,0};
 if(!e||!provider||!policy||!observer||!graph||!id||!space||!zero(e,sizeof(*e))||
    overlap(e,sizeof(*e),provider,sizeof(*provider))||overlap(e,sizeof(*e),policy,sizeof(*policy))||
    overlap(e,sizeof(*e),observer,sizeof(*observer))||overlap(e,sizeof(*e),id,sizeof(*id))||
    !provider->context||overlap(e,sizeof(*e),provider->context,1)||!provider->clock_ns||!provider->admit||!provider->acquire||!provider->valid||!provider->release||
    !id_valid(id->sensor,0)||!id_valid(id->profile,0)||!id_valid(id->head,1)||!id->graph_epoch||!id->source_generation||!id->track_generation||
    (id->head[0]&&!id->head_epoch)||mm_display_to_sensor(&id->geometry,origin,&dummy))return MM_INVALID;
 if(graph->device_count>16||graph->profile_count>16||graph->association_count>32||graph->group_count>8||
    (graph->device_count&&!graph->devices)||(graph->profile_count&&!graph->profiles)||(graph->association_count&&!graph->associations)||
    (graph->group_count&&!graph->groups))return MM_INVALID;
 for(i=0;i<graph->device_count;i++){
  const struct camera_device *d=&graph->devices[i];if(!id_valid(d->id,0))return MM_INVALID;
  if(same_id(d->id,id->sensor)&&d->kind==CAMERA_DEVICE_SENSOR&&d->state!=CAMERA_UNAVAILABLE)sensor=1;
  if(same_id(d->id,id->head)&&d->kind==CAMERA_DEVICE_PTZ&&d->state!=CAMERA_UNAVAILABLE)head=1;
 }
 for(i=0;i<graph->profile_count;i++){
  const struct camera_profile *p=&graph->profiles[i];if(!id_valid(p->id,0)||!id_valid(p->camera_id,0))return MM_INVALID;
  if(same_id(p->id,id->profile)&&same_id(p->camera_id,id->sensor)&&p->width==id->geometry.display_w&&p->height==id->geometry.display_h)profile=1;
 }
 for(i=0;i<graph->association_count;i++){
  const struct camera_association *a=&graph->associations[i];if(!id_valid(a->camera_id,0)||!id_valid(a->device_id,0))return MM_INVALID;
  if(same_id(a->camera_id,id->sensor)&&same_id(a->device_id,id->head))association=1;
 }
 if(!sensor||!profile||(id->head[0]&&(!head||!association))||camera_graph_validate(graph))return MM_INVALID;
 e->self=e;e->provider=*provider;e->policy=policy;e->observer=*observer;e->identity=*id;
 e->namespace_epoch=space;e->next_event=1;e->last_ns=now;e->status=MM_NO_CONFIG;return MM_OK;
}
int mm_stage_config(struct motion_engine *e,const struct camera_access_principal *admin,const struct mm_config *cfg,uint64_t now)
{
 uint8_t mask[MM_MASK_BYTES];int ret;
 if(!live(e)||!disjoint(e,admin,sizeof(*admin))||!disjoint(e,cfg,sizeof(*cfg))||mm_config_validate(cfg,&e->identity.geometry))return MM_INVALID;
 if(e->pending)return MM_BUSY;
 if(cfg->generation<=e->highest_config_generation)return MM_STALE;
 ret=check(e,admin,CAMERA_ACCESS_RECORDING_CONFIG,now,0);if(ret)return ret;
 if(mm_mask_rasterize(cfg,&e->identity.geometry,mask))return MM_INVALID;
 ret=check(e,admin,CAMERA_ACCESS_RECORDING_CONFIG,e->last_ns,0);if(ret)return ret;
 e->pending_config=*cfg;e->pending_admin=*admin;memcpy(e->pending_mask,mask,sizeof(mask));
 e->pending=1;e->highest_config_generation=cfg->generation;return MM_OK;
}
int mm_apply_config(struct motion_engine *e,uint64_t now)
{
 int ret;
 if(!live(e))return MM_INVALID;
 if(e->owned)return MM_BUSY;
 if(!e->pending)return MM_EMPTY;
 ret=check(e,&e->pending_admin,CAMERA_ACCESS_RECORDING_CONFIG,now,0);
 if(ret){e->pending=0;return ret;} /* revoked uncommitted config is cancelled */
 ret=check(e,&e->observer,CAMERA_ACCESS_LIVE,e->last_ns,0);if(ret)return ret;
 ret=check(e,&e->pending_admin,CAMERA_ACCESS_RECORDING_CONFIG,e->last_ns,0);if(ret){e->pending=0;return ret;}
 if(e->active){ret=check(e,&e->observer,CAMERA_ACCESS_EVENTS,e->last_ns,0);if(ret)return ret;}
 /* All resource callbacks are now finished: final direct shared-policy proof
  * for BOTH authorities immediately before the atomic config/event effects. */
 if(permission(e,&e->pending_admin,CAMERA_ACCESS_RECORDING_CONFIG,e->last_ns)){e->pending=0;return MM_DENIED;}
 if(permission(e,&e->observer,CAMERA_ACCESS_LIVE,e->last_ns)||
    (e->active&&permission(e,&e->observer,CAMERA_ACCESS_EVENTS,e->last_ns)))return MM_DENIED;
 if(e->active&&enqueue(e,MM_TERMINATED_GAP,MM_CONFIG_CHANGE,e->last_ns,e->active_ns))return MM_BUSY;
 e->config=e->pending_config;memcpy(e->mask,e->pending_mask,sizeof(e->mask));e->pending=0;e->configured=1;
 e->have_background=0;e->active=e->candidate=e->quiet=0;e->status=MM_BASELINE;e->reason=MM_CONFIG_CHANGE;return MM_OK;
}
int mm_cancel(struct motion_engine *e,uint64_t now)
{if(!live(e))return MM_INVALID;return return_loan(e,now);}
int mm_poll(struct motion_engine *e,uint64_t now)
{
 uint64_t total=0,changed=0,positive=0,negative=0,sum_delta=0;uint32_t crc=~0U,i,flags,reason=MM_NONE;int ret,baseline,high;
 uint64_t time,acquire_start,acquire_end;uint32_t next_active,next_candidate,next_quiet,kind=0;uint64_t candidate_ns,active_ns,quiet_ns;
 if(!live(e))return MM_INVALID;
 if(e->owned)return MM_BUSY;
 if(e->fault)return MM_UNAVAILABLE;
 if(!e->configured)return MM_UNAVAILABLE;
 ret=check(e,&e->observer,CAMERA_ACCESS_LIVE,now,0);
 if(ret){unavailable(e,ret==MM_STALE?MM_CLOCK:MM_PRIVACY);if(ret==MM_STALE)e->fault=1;return ret;} /* BEFORE acquire */
 if(e->event_count==2)return MM_BUSY;
 acquire_start=clock_now(e);
 if(acquire_start<e->last_ns||acquire_start<now){unavailable(e,MM_CLOCK);e->fault=1;return MM_STALE;}
 if(permission(e,&e->observer,CAMERA_ACCESS_LIVE,acquire_start)){unavailable(e,MM_PRIVACY);return MM_DENIED;}
 e->last_ns=acquire_start;e->tiles_read=0;
 memset(&e->span,0,sizeof(e->span));e->busy=1;ret=e->provider.acquire(e->provider.context,&e->identity,&e->span);e->busy=0;
 /* Record the ACTUAL acquisition outcome before the post-callback clock. */
 if(ret!=1){e->owned=1;e->return_attempted=0;e->uncertain=ret!=0;}
 acquire_end=clock_now(e);
 if(acquire_end<acquire_start||acquire_end-acquire_start>MM_CALLBACK_NS){
  unavailable(e,MM_CLOCK);e->fault=1;return e->uncertain?MM_RETAINED:MM_STALE;
 }
 e->last_ns=acquire_end;
 if(ret==1){unavailable(e,MM_SOURCE);return MM_UNAVAILABLE;}
 if(e->uncertain){unavailable(e,MM_SOURCE);return MM_RETAINED;}
 ret=check(e,&e->observer,CAMERA_ACCESS_LIVE,e->last_ns,1);if(ret){unavailable(e,ret==MM_DENIED?MM_PRIVACY:MM_SOURCE);return ret;}
 if((e->span.inhibit_flags&~7U)||((e->span.inhibit_flags&MM_INHIBIT_PTZ)&&
    (!e->identity.head[0]||e->span.inhibit_epoch!=e->identity.head_epoch))){unavailable(e,MM_SOURCE);return MM_UNAVAILABLE;}
 time=e->span.sampled_ns;
 if(e->have_frame&&(time<=e->last_frame_ns||e->last_frame_sequence==UINT64_MAX)){unavailable(e,MM_CLOCK);e->fault=1;return MM_STALE;}
 baseline=!e->have_background;
 if(e->have_frame&&(e->span.frame_sequence!=e->last_frame_sequence+1||time-e->last_frame_ns>e->config.max_gap_ns))reason=MM_GAP;
 if(e->have_frame&&e->span.exposure_epoch!=e->exposure_epoch)reason=MM_EXPOSURE;
 flags=e->span.inhibit_flags;
 if(flags&MM_INHIBIT_PTZ)reason=MM_PTZ;
 else if(flags&MM_INHIBIT_DAYNIGHT)reason=MM_DAYNIGHT;
 else if(flags&MM_INHIBIT_EXPOSURE)reason=MM_EXPOSURE;
 for(i=0;i<MM_CELLS;i++){
  struct mm_cell cell;uint32_t col=i%MM_COLS,row=i/MM_COLS,expected,delta,threshold;int difference;
  ret=check(e,&e->observer,CAMERA_ACCESS_LIVE,e->last_ns,1);if(ret){unavailable(e,ret==MM_DENIED?MM_PRIVACY:MM_SOURCE);return ret;}
   cell=e->span.cells[i];e->tiles_read++; /* actual protected loads, not inferred callback count */
  expected=(((col+1)*e->identity.geometry.crop_w)/MM_COLS-(col*e->identity.geometry.crop_w)/MM_COLS)*
           (((row+1)*e->identity.geometry.crop_h)/MM_ROWS-(row*e->identity.geometry.crop_h)/MM_ROWS);
  if(cell.pixels!=expected){unavailable(e,MM_COVERAGE);return MM_UNAVAILABLE;}
  crc=crc_byte(crc,(uint8_t)cell.pixels);crc=crc_byte(crc,(uint8_t)(cell.pixels>>8));crc=crc_byte(crc,cell.mean);crc=crc_byte(crc,cell.noise);
  e->staging[i]=cell.mean;
  if(e->mask[i/8]&(1U<<(i%8)))continue;
  total+=cell.pixels;difference=(int)cell.mean-e->background[i];delta=(uint32_t)(difference<0?-difference:difference);
  threshold=e->config.delta_min+(uint32_t)cell.noise*e->config.noise_multiplier;if(threshold>255)threshold=255;
  if(delta>=threshold)changed+=cell.pixels;
  if(delta>=e->config.global_delta){if(difference>0)positive+=cell.pixels;else negative+=cell.pixels;}
  sum_delta+=(uint64_t)delta*cell.pixels;
 }
 if(~crc!=e->span.digest_crc32){unavailable(e,MM_DIGEST);return MM_UNAVAILABLE;}
 if(!baseline&&!reason&&total&&(positive*1000>=total*e->config.global_permille||negative*1000>=total*e->config.global_permille)&&
    sum_delta>=total*e->config.global_delta)reason=MM_GLOBAL_LUMA;
 high=total&&changed*1000>=total*e->config.area_permille;
 next_active=e->active;next_candidate=e->candidate;next_quiet=e->quiet;
 candidate_ns=e->candidate_ns;active_ns=e->active_ns;quiet_ns=e->quiet_ns;
 if(reason||baseline){
  if(e->active)kind=MM_TERMINATED_GAP;
  next_active=next_candidate=next_quiet=0;
 }else if(high){
  next_quiet=0;
  if(!next_active){if(!next_candidate){next_candidate=1;candidate_ns=time;}
   if(time-candidate_ns>=e->config.debounce_ns){next_active=1;active_ns=candidate_ns;kind=MM_BEGIN;}}
 }else{
  next_candidate=0;
  if(next_active){if(!next_quiet){next_quiet=1;quiet_ns=time;}
   if(time-quiet_ns>=e->config.release_ns&&time-active_ns>=e->config.min_event_ns){next_active=next_quiet=0;kind=MM_END;}}
 }
 /* Stage next background in already-owned bounded array; no partial bg update
  * if a tile/check fails. Changed cells freeze until valid quiet recovery. */
 for(i=0;i<MM_CELLS;i++)if(!baseline&&!reason){
  int d=(int)e->staging[i]-e->background[i];unsigned absdiff=(unsigned)(d<0?-d:d);
  if(absdiff<e->config.delta_min)e->staging[i]=(uint8_t)(((unsigned)e->background[i]*7+e->staging[i])/8);
  else e->staging[i]=e->background[i];
 }
 ret=check(e,&e->observer,CAMERA_ACCESS_LIVE,e->last_ns,1);
 if(!ret&&kind)ret=check(e,&e->observer,CAMERA_ACCESS_EVENTS,e->last_ns,1);
 if(ret){unavailable(e,MM_PRIVACY);return ret;}
 /* Atomic combined background/event commit: both actual grants after ALL
  * resource/source/clock callbacks, not a former LIVE check before EVENTS. */
 if(permission(e,&e->observer,CAMERA_ACCESS_LIVE,e->last_ns)||
    (kind&&permission(e,&e->observer,CAMERA_ACCESS_EVENTS,e->last_ns))){unavailable(e,MM_PRIVACY);return MM_DENIED;}
 if(kind){ret=enqueue(e,kind,reason,time,kind==MM_BEGIN?active_ns:e->active_ns);if(ret){
  unavailable(e,ret==MM_EXHAUSTED?MM_COUNTER:MM_QUEUE_FULL);if(ret==MM_EXHAUSTED)e->fault=1;return ret;}}
 memcpy(e->background,e->staging,sizeof(e->background));e->have_background=1;e->have_frame=1;
 e->last_frame_sequence=e->span.frame_sequence;e->last_frame_ns=time;e->exposure_epoch=e->span.exposure_epoch;
 e->active=next_active;e->candidate=next_candidate;e->quiet=next_quiet;e->candidate_ns=candidate_ns;e->active_ns=active_ns;e->quiet_ns=quiet_ns;
 e->changed_permille=total?(uint32_t)(changed*1000/total):0;
 e->status=reason?MM_INHIBITED:(baseline?MM_BASELINE:(e->active?MM_MOVING:(e->candidate?MM_CANDIDATE:MM_QUIET)));e->reason=reason;
 ret=return_loan(e,e->last_ns);return ret;
}
static int event_equal(const struct mm_event *a,const struct mm_event *b)
{return !memcmp(a,b,sizeof(*a));} /* fully zero-initialized fixed metadata, no wire struct */
int mm_event_peek(struct motion_engine *e,uint64_t now,struct mm_event *out)
{
 int ret;if(!live(e)||!disjoint(e,out,sizeof(*out)))return MM_INVALID;
 ret=check(e,&e->observer,CAMERA_ACCESS_EVENTS,now,0);if(ret)return ret;
 if(!e->event_count)return MM_EMPTY;
 if(e->last_ns>=e->events[0].expires_ns){e->event_count=0;unavailable(e,MM_GAP);return MM_STALE;}
 *out=e->events[0];return MM_OK;
}
int mm_event_take(struct motion_engine *e,const struct mm_event *event,uint64_t now)
{
 int ret;if(!live(e)||!disjoint(e,event,sizeof(*event)))return MM_INVALID;
 if(!e->event_count||!event_equal(event,&e->events[0]))return MM_STALE;
 ret=check(e,&e->observer,CAMERA_ACCESS_EVENTS,now,0);if(ret)return ret;
 if(e->last_ns>=event->expires_ns){e->event_count=0;unavailable(e,MM_GAP);return MM_STALE;}
 e->event_count--;if(e->event_count)e->events[0]=e->events[1];return MM_OK;
}
