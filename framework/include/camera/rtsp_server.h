/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_SERVER_H
#define CAMERA_RTSP_SERVER_H
#ifdef __cplusplus
extern "C" {
#endif
#include <camera/packet_hub.h>
#include <camera/producer.h>
#include <camera/rtsp_input.h>
#include <signal.h>
#include <string.h>
#define CAMERA_RTSP_FRAME_HANDLE_API 1U
/* A frame bracket is NOT a primitive command token. The extra namespace is the
 * stable lifecycle hook context (the bridge address for common hooks). Neither
 * copying primitive owner/id/generation nor using another bridge's handle can
 * acknowledge a frame. Identity checks are not authentication capabilities.
 */
struct camera_rtsp_frame_handle {
 const struct camera_producer *owner;
 uint64_t id,generation;
 const void *namespace_context;
};
enum camera_rtsp_failure_stage {
 CAMERA_RTSP_START_FAILURE,CAMERA_RTSP_SOURCE_FAILURE,CAMERA_RTSP_PACKET_FAILURE,
 CAMERA_RTSP_DETACH_FAILURE
};
struct camera_rtsp_failure {
 /* Frame ID for SOURCE/PACKET; zero for an admission failure before a frame.
  * Primitive lifecycle operation IDs are separately exposed by its event API.
  */
 struct camera_rtsp_frame_handle source;
 struct camera_producer_demand demand;
 enum camera_rtsp_failure_stage stage;
 int result;uint64_t monotonic_ns;
};
/* Optional, synchronous, caller-serialized owner hooks. Tokens/context remain
 * stable through serve and all notifications. Failure must not leave an admitted
 * demand on attach; detach is invoked exactly once for each admitted token.
 * begin/end bracket synchronous next + owned packet release, NOT parallel DMA.
 * Non-OK next owns no returned packet and must retire its partial work internally.
 * retire reports OK only after actual STOP/idle retirement (or no STOP needed).
 * Notifications copy metadata; no reentry, no asynchronous token completion here.
 */
struct camera_rtsp_lifecycle {
 void *context;const struct camera_producer *owner;
 int (*attach)(void *,uint64_t,struct camera_producer_demand *);
 int (*detach)(void *,struct camera_producer_demand,uint64_t);
 int (*begin)(void *,uint64_t,uint32_t *,struct camera_rtsp_frame_handle *);
 int (*end)(void *,struct camera_rtsp_frame_handle,int,uint32_t,uint64_t);
 int (*retire)(void *,uint64_t);
 void (*failure)(void *,const struct camera_rtsp_failure *);
};
struct camera_rtsp_producer {
 void *context;
 /* Synchronous owned single VCL NAL (IDR5/P1), no Annex-B prefix;
  * one capture/encode for all consumers. Parameters supplied separately.
  */
 int (*next)(void *context,uint32_t force_keyframe,struct camera_packet *packet);
 void (*release)(void *context,const struct camera_packet *packet);
 void (*idle)(void *context);
 const uint8_t *parameter_sets;size_t parameter_set_bytes;
 /* Additive opt-in: old zero/designated initializers retain legacy behavior.
  * This is a source API, not a size-versioned binary plugin ABI.
  */
 int (*idle_checked)(void *context);
 const struct camera_rtsp_lifecycle *lifecycle;
};

/* Small common-primitive bridge, instantiated ONLY by opting-in callers.
 * Inline callbacks keep legacy server/cursor builds free of producer.c linkage.
 * Primitive + callback context are caller-owned; never copy/move while serving.
 * START validates logical admission/readiness; next still proves actual capture.
 * No automatic recovery or control dispatch. Recorder demand remains the owner's
 * common-primitive API; this bridge neither manufactures nor services recording.
 */
struct camera_rtsp_common_producer {
 struct camera_producer *producer;void *context;
 int (*start)(void *);int (*idle)(void *);
 uint64_t (*clock_ns)(void *); /* same CLOCK_MONOTONIC domain; MAX = error ONLY */
 uint32_t clock_failed; /* sticky: retain demands/work, no START/STOP retry */
 /* Initialize fresh/zero; bridge frame IDs also never wrap. These are not
  * primitive command IDs. One bracket remains open until source release/end.
  */
 uint64_t frame_sequence;uint32_t frame_open;
 struct camera_rtsp_frame_handle frame;
 struct camera_producer_token keyframe;
};
static inline int camera_rtsp_common_result(int r)
{return r<=CAMERA_OK&&r>=CAMERA_DISCONNECTED?r:CAMERA_IO_ERROR;}
static inline int camera_rtsp_common_time(struct camera_rtsp_common_producer *a,uint64_t now)
{
 if(a->clock_failed||now==UINT64_MAX||now<a->producer->last_ns){a->clock_failed=1;return CAMERA_IO_ERROR;}
 return CAMERA_OK;
}
static inline uint64_t camera_rtsp_common_after(struct camera_rtsp_common_producer *a,uint64_t before)
{
 uint64_t now=a->clock_ns(a->context);
 if(now==UINT64_MAX||now<before||camera_rtsp_common_time(a,now)){a->clock_failed=1;return UINT64_MAX;}
 return now;
}
static inline int camera_rtsp_common_complete(struct camera_producer *p,struct camera_producer_token t,int result,uint64_t now,int *completed_result)
{
 if(now==UINT64_MAX)return CAMERA_IO_ERROR;
 int r=camera_producer_tick(p,now);if(r)return r;
 struct camera_producer_work active;int outcome;
 r=camera_producer_active(p,&active,&outcome);if(r)return r;
 r=camera_producer_complete(p,t,result,now);if(r)return r;
 *completed_result=outcome==CAMERA_PRODUCER_PENDING?result:outcome;return CAMERA_OK;
}
static inline int camera_rtsp_common_attach(void *context,uint64_t now,struct camera_producer_demand *d)
{
 struct camera_rtsp_common_producer *a=(struct camera_rtsp_common_producer *)context;
 if(!a||!a->producer||!a->start||!a->idle||!a->clock_ns)return CAMERA_INVALID;
 if(camera_rtsp_common_time(a,now))return CAMERA_IO_ERROR;
 int r=camera_producer_attach(a->producer,CAMERA_PRODUCER_RTSP,now,d);if(r)return r;
 if(a->producer->state!=CAMERA_PRODUCER_DRAINING){
  struct camera_producer_token keyframe;
  r=camera_producer_request_keyframe(a->producer,now,&keyframe);
  if(r)camera_producer_detach(a->producer,*d,now);
 }
 return r;
}
static inline int camera_rtsp_common_detach(void *context,struct camera_producer_demand d,uint64_t now)
{
 struct camera_rtsp_common_producer *a=(struct camera_rtsp_common_producer *)context;
 if(camera_rtsp_common_time(a,now))return CAMERA_IO_ERROR;
 return camera_producer_detach(a->producer,d,now);
}
static inline int camera_rtsp_common_begin(void *context,uint64_t now,uint32_t *force,struct camera_rtsp_frame_handle *t)
{
 struct camera_rtsp_common_producer *a=(struct camera_rtsp_common_producer *)context;
 struct camera_producer *p=a->producer;
 t->owner=p;t->id=0;t->generation=p->generation;t->namespace_context=a;
 if(camera_rtsp_common_time(a,now))return CAMERA_IO_ERROR;
 if(a->frame_open)return CAMERA_BUSY;
 if(a->frame_sequence==UINT64_MAX)return CAMERA_NOT_READY;
 if(p->state!=CAMERA_PRODUCER_STARTING&&p->state!=CAMERA_PRODUCER_RUNNING)return CAMERA_NOT_READY;
 if(*force){struct camera_producer_token requested;int r=camera_producer_request_keyframe(p,now,&requested);if(r)return r;}
 memset(&a->keyframe,0,sizeof(a->keyframe));
 for(unsigned i=0;i<2;i++){
  struct camera_producer_work w;int r=camera_producer_next(p,now,CAMERA_PRODUCER_NO_BOUNDARY,&w);
  if(r==CAMERA_NOT_READY&&p->state==CAMERA_PRODUCER_RUNNING){
   struct camera_producer_work active;int outcome;
   if(camera_producer_active(p,&active,&outcome)==CAMERA_OK)return CAMERA_BUSY;
   *force=0;break;
  }
  if(r)return r;
  if(w.kind==CAMERA_PRODUCER_KEYFRAME){a->keyframe=w.token;*force=1;break;}
  if(w.kind!=CAMERA_PRODUCER_START)return CAMERA_INVALID;
  r=camera_rtsp_common_result(a->start(a->context));
   now=camera_rtsp_common_after(a,now);if(now==UINT64_MAX)return CAMERA_IO_ERROR;
  int completed=camera_rtsp_common_complete(p,w.token,r,now,&r);if(completed)return completed;
  if(r)return r;
  if(p->state!=CAMERA_PRODUCER_RUNNING)return CAMERA_NOT_READY;
 }
 a->frame.owner=p;a->frame.id=++a->frame_sequence;a->frame.generation=p->generation;a->frame.namespace_context=a;
 a->frame_open=1;*t=a->frame;return CAMERA_OK;
}
static inline int camera_rtsp_common_end(void *context,struct camera_rtsp_frame_handle t,int result,uint32_t keyframe,uint64_t now)
{
 struct camera_rtsp_common_producer *a=(struct camera_rtsp_common_producer *)context;
 struct camera_producer *p=a->producer;
 if(camera_rtsp_common_time(a,now))return CAMERA_IO_ERROR;
 if(!a->frame_open||t.namespace_context!=a||t.owner!=p||t.generation!=p->generation||
     t.id!=a->frame.id||t.generation!=a->frame.generation)return CAMERA_STALE_LEASE;
 int ticked=camera_producer_tick(p,now);if(ticked)return ticked;
 result=camera_rtsp_common_result(result);
 if(a->keyframe.id){
  if(result==CAMERA_OK&&!keyframe)result=CAMERA_IO_ERROR;
  int completed=camera_rtsp_common_complete(p,a->keyframe,result,now,&result);
  if(completed)return completed; /* API rejection is not a driver error result. */
 }
 a->frame_open=0;
 if(result==CAMERA_OK&&p->state!=CAMERA_PRODUCER_RUNNING)result=CAMERA_NOT_READY;
 if(result!=CAMERA_OK)camera_producer_source_failed(p,camera_rtsp_common_result(result),now);
 return result;
}
static inline int camera_rtsp_common_retire(void *context,uint64_t now)
{
 struct camera_rtsp_common_producer *a=(struct camera_rtsp_common_producer *)context;
 struct camera_producer *p=a->producer;
 if(camera_rtsp_common_time(a,now))return CAMERA_IO_ERROR;
 if(a->frame_open)return CAMERA_BUSY;
 if(p->state==CAMERA_PRODUCER_FAILED)return CAMERA_NOT_READY;
 if(p->state!=CAMERA_PRODUCER_DRAINING)return CAMERA_OK;
 struct camera_producer_work w;int r=camera_producer_next(p,now,CAMERA_PRODUCER_NO_BOUNDARY,&w);if(r)return r;
 if(w.kind!=CAMERA_PRODUCER_STOP)return CAMERA_INVALID;
 r=camera_rtsp_common_result(a->idle(a->context));
 now=camera_rtsp_common_after(a,now);if(now==UINT64_MAX)return CAMERA_IO_ERROR;
 int completed=camera_rtsp_common_complete(p,w.token,r,now,&r);if(completed)return completed;
 if(r)return r;
 return p->state==CAMERA_PRODUCER_FAILED?CAMERA_NOT_READY:CAMERA_OK;
}
static inline struct camera_rtsp_lifecycle camera_rtsp_common_hooks(struct camera_rtsp_common_producer *a)
{
 struct camera_rtsp_lifecycle h={a,a->producer,camera_rtsp_common_attach,camera_rtsp_common_detach,
  camera_rtsp_common_begin,camera_rtsp_common_end,camera_rtsp_common_retire,NULL};
 return h;
}
struct camera_rtsp_options {
 const char *bind_address,*sdp;
 uint16_t port;uint32_t interval_ms;
 uint32_t send_deadline_ms; /* 0 = lab100ms default; explicit10..2000ms */
 uint8_t *hub_storage;size_t packet_capacity;
 uint32_t hub_slots; /*0=four; serialized sender may explicitly use one */
 uint32_t tcp_coalesce; /*0=ordinary sends;1=MSG_MORE until final AU packet */
 uint32_t trace_every; /*0/1=every AU;2..1000=sparse normal logs, errors always */
 volatile sig_atomic_t *stop;
 /* Required exact public resource URI registry. Frozen for serve lifetime.
  * Header/body/interleaved completion budget: 0=2000ms; explicit500..5000ms.
  * Partial bytes never renew this absolute budget. NOT an auth configuration. */
 const struct camera_rtsp_registry *resources;
 uint32_t request_deadline_ms;
};
/* Experimental lab service: TCP only, at most four clients, bounded sends,
 * no authentication/discovery. Do NOT deploy as the protected product API.
 * One producer remains serialized with AU delivery, but independent bounded
 * nonblocking packet cursors service writable peers fairly. Slow/error clients disconnect;
 * other consumers continue. No hardware or device paths in this service.
 */
int camera_rtsp_serve(const struct camera_rtsp_options *options,
  const struct camera_rtsp_producer *producer);
#ifdef __cplusplus
}
#endif
#endif
