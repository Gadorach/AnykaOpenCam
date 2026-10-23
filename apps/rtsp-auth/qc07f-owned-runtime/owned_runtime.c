/* SPDX-License-Identifier: GPL-2.0-only
 * Fresh direct snapshot inclusion keeps ONE real private native owner. Never
 * compile native_adapter.c separately. See INPUTS.json for exact provenance.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "native/native_adapter.c"
#include "owned_runtime_v1.h"

static struct qc07f_owned_runtime_v1 *runtime;

static int valid(const struct qc07f_owned_runtime_v1 *r)
{return r&&r==runtime&&r->self==r&&r->config&&r->config->context==r&&r->config->graph==r->graph;}

static int native_owns(void)
{
 if(!native_initialized)return 0; /* static zero FD fields are NOT ownership */
 return native.watchdog.fd>=3||native.continuous_sensor>=3||native.continuous_frame>=3||
  native.h264_stream_fd>=3||native.encoder_hold>=3||native.legacy_control>=3||
  native.providers.sensor_staged>=3||native.providers.frame_staged.fd>=3||
  native.providers.encoder_staged.fd>=3||native.produced_storage||native.resident_storage||
  parked_after_stop||native.receipt_armed||native.close_unknown;
}
static int native_fault(void)
{return native_initialized&&(native.retirement_uncertain||source_clock.failed||
 native.watchdog.state==QW_HOLD||native.providers.phase==QP_HOLD);}

static int retired_actual(const struct qc07f_owned_runtime_v1 *r)
{
 /* r->retired is set ONLY from successful actual nh_native_retire. Corroborate
  * its remaining owner shape; no public permission bool creates this fact. */
 return r->retired&&!native_fault()&&native.providers.phase==QP_CLOSED&&
  native.continuous_sensor==-1&&native.continuous_frame==-1&&native.h264_stream_fd==-1&&
  native.encoder_hold==-1&&native.legacy_control==-1&&!native.receipt_armed&&
  !native.produced_storage&&!native.resident_storage&&!parked_after_stop&&
  !native.close_unknown&&!r->loan_present&&native.watchdog.state==QW_DISABLED;
}
static int retention_needed(const struct qc07f_owned_runtime_v1 *r)
{
 if(!native_initialized||retired_actual(r))return 0;
 return (native_fault()&&native_owns())||native.receipt_armed||native.last_capture_generation||
  native.providers.phase==QP_ACTIVE||native.produced_storage||native.resident_storage||
  parked_after_stop||native.raw_valid||native.h264_stream_frames||r->loan_present||
  memory_state.ledger.initialized||native.watchdog.session_deadline_ns||native.close_unknown;
}
static void latch(struct qc07f_owned_runtime_v1 *r,int result)
{if(result&&!r->error)r->error=camera_rtsp_common_result(result);}
static _Noreturn void retain(struct qc07f_owned_runtime_v1 *r,int result)
{
 latch(r,result?result:CAMERA_IO_ERROR);
 nh_native_fence(NULL);
 nh_native_quarantine(); /* actual loop; NEVER an outward result/cleanup */
}
void qc07f_owned_runtime_cleanup_v1(void *context,int clock_failed,unsigned final)
{
 struct qc07f_owned_runtime_v1 *r=context;
 if(!valid(r))return; /* trusted identity prerequisite, not arbitrary-pointer safety */
 if((native_fault()&&native_owns())||
    ((clock_failed||r->bridge.clock_failed||final)&&retention_needed(r)))retain(r,CAMERA_IO_ERROR);
}
static int ready(void *context,uint64_t connection,uint64_t now)
{
 struct qc07f_owned_runtime_v1 *r=context;(void)connection;
 if(!valid(r))return CAMERA_INVALID;
 qc07f_owned_runtime_cleanup_v1(r,now==UINT64_MAX,0);
 if(!r->bound||!native_initialized)return CAMERA_NOT_READY;
 if(r->error)return r->error;
 if(stop||native_fault()||provider_terminal(&native)||!provider_shape(&native)||
    !sensor_baseline_requested||!storage_selected||!memory_state.ledger.initialized||
    !memory_state.ledger.paid[0]||!memory_state.ledger.paid[1]||
    native.watchdog.fd<3||!native.watchdog.session_deadline_ns||
    now<native.watchdog.session_start_ns||now>=native.watchdog.session_deadline_ns||
    (native.watchdog.state!=QW_UNKNOWN&&native.watchdog.state!=QW_DISABLED))return CAMERA_NOT_READY;
 return CAMERA_OK; /* local refusal checks only; actual guarded NEXT still required */
}
static void bind_gate(void *context,int (*gate)(void *),void *gate_context)
{
 struct qc07f_owned_runtime_v1 *r=context;
 if(!valid(r))return;
 if((gate&&r->gate)||(!gate&&gate_context)){
  latch(r,CAMERA_INVALID);
  if(retention_needed(r))retain(r,CAMERA_INVALID);
  return;
 }
 if(!gate)qc07f_owned_runtime_cleanup_v1(r,0,1);
 r->gate=gate;r->gate_context=gate?gate_context:NULL;
}
static void retain_demand(void *context,const struct camera_packet_hub *hub,
 const struct camera_producer_demand *demand,unsigned index)
{
 struct qc07f_owned_runtime_v1 *r=context;(void)hub;(void)demand;(void)index;
 if(!valid(r))return;
 latch(r,CAMERA_IO_ERROR);
 if(retention_needed(r))retain(r,CAMERA_IO_ERROR);
}
static void retain_frame(void *context,struct camera_rtsp_frame_handle frame,int result)
{
 struct qc07f_owned_runtime_v1 *r=context;(void)frame;
 if(!valid(r))return;
 latch(r,result?result:CAMERA_IO_ERROR);
 if(retention_needed(r))retain(r,result);
}
static uint64_t source_now(void *context)
{
 struct qc07f_owned_runtime_v1 *r=context;
 if(!valid(r)||!r->bound||!native_initialized)return UINT64_MAX;
 uint64_t now=nh_native_now(NULL);
 qc07f_owned_runtime_cleanup_v1(r,now==UINT64_MAX,0);
 return now;
}
static int logical_start(void *context)
{
 struct qc07f_owned_runtime_v1 *r=context;
 if(!valid(r))return CAMERA_INVALID;
 int result=ready(r,0,source_now(r));
 if(result)return result;
 return r->gate?r->gate(r->gate_context):CAMERA_NOT_READY;
}
static int next_packet(void *context,uint32_t force,struct camera_packet *packet)
{
 struct qc07f_owned_runtime_v1 *r=context;
 if(!valid(r)||!packet||force>1)return CAMERA_INVALID;
 memset(packet,0,sizeof(*packet));
 if(!r->bound||!native_initialized)return CAMERA_NOT_READY;
 qc07f_owned_runtime_cleanup_v1(r,0,0);
 if(r->error){if(retention_needed(r))retain(r,r->error);return r->error;}
 if(r->loan_present){if(retention_needed(r))retain(r,CAMERA_BUSY);return CAMERA_BUSY;}
 int admitted=r->gate?r->gate(r->gate_context):CAMERA_NOT_READY;
 if(admitted){
  /* Only a genuinely no-work refusal can return normally. Once native session/
   * backing/activation exists, RTSP's failure path cannot retire it by unwind. */
  if(retention_needed(r))retain(r,admitted);
  return admitted;
 }
 int result=nh_native_memory_gate(NULL,NH_MEM_PRE_NEXT);
 if(result){latch(r,result);if(retention_needed(r))retain(r,result);return result;}
 struct camera_packet acquired={0};struct nh_key key={0};
 result=nh_native_next_keyframe(NULL,force,&acquired,&key);
 if(result){
  latch(r,result);
  /* Guard failure may zero outward fields while retaining actual DMA/backing.
   * Never return such a failure into RTSP's partial-work-retired contract. */
  if(retention_needed(r))retain(r,result);
  return result;
 }
 r->retired=0;r->packet=acquired;r->key=key;r->loan_present=1;
 if(!r->post_first){
  result=nh_native_memory_gate(NULL,NH_MEM_POST_FIRST);
  if(result)retain(r,result); /* actual acquired packet/backing still owned */
  r->post_first=1;
 }
 *packet=acquired;
 return CAMERA_OK;
}
static void release_packet(void *context,const struct camera_packet *packet)
{
 struct qc07f_owned_runtime_v1 *r=context;
 if(!valid(r))return;
 if(!packet||!r->loan_present||packet->data!=r->packet.data||packet->bytes!=r->packet.bytes||
    packet->monotonic_ns!=r->packet.monotonic_ns||packet->codec!=r->packet.codec||
    packet->keyframe!=r->packet.keyframe||packet->lease_token!=r->packet.lease_token||
    packet->generation!=r->packet.generation){
  latch(r,CAMERA_STALE_LEASE);
  if(retention_needed(r))retain(r,CAMERA_STALE_LEASE);
  return;
 }
 int result=nh_native_release(NULL,packet);
 if(result){latch(r,result);if(retention_needed(r))retain(r,result);return;}
 r->loan_present=0;memset(&r->packet,0,sizeof(r->packet));
 /* r->key remains the real last released scalar identity; no payload survives. */
}
static int idle_source(void *context)
{
 struct qc07f_owned_runtime_v1 *r=context;
 if(!valid(r))return CAMERA_INVALID;
 if(!r->bound||!native_initialized)return CAMERA_NOT_READY;
 int result=nh_native_retire(NULL);
 if(result){latch(r,result);if(retention_needed(r))retain(r,result);return result;}
 r->retired=1;
 return r->error?r->error:CAMERA_OK;
}
static int frame_end(void *context,struct camera_rtsp_frame_handle frame,
 int result,uint32_t keyframe,uint64_t now)
{
 struct camera_rtsp_common_producer *bridge=context;
 struct qc07f_owned_runtime_v1 *r=bridge->context;
 qc07f_owned_runtime_cleanup_v1(r,now==UINT64_MAX,0);
 if(r->error)result=r->error;
 int ended=camera_rtsp_common_end(context,frame,result,keyframe,now);
 if(ended){latch(r,ended);if(retention_needed(r))retain(r,ended);}
 return ended;
}
static int frame_retire(void *context,uint64_t now)
{
 struct camera_rtsp_common_producer *bridge=context;
 struct qc07f_owned_runtime_v1 *r=bridge->context;
 qc07f_owned_runtime_cleanup_v1(r,now==UINT64_MAX,0);
 int result=camera_rtsp_common_retire(context,now);
 if(r->error&&!result)result=r->error;
 if(result){latch(r,result);if(retention_needed(r))retain(r,result);}
 return result;
}

int qc07f_owned_runtime_prepare_v1(struct qc07f_owned_runtime_v1 *r,
 struct candidate_config *config,struct candidate_prestarted *owner,void *bootstrap)
{
 if(!r||!config||!owner||!config->graph||!config->random_ready||!config->retain_verifier)
  return CAMERA_INVALID;
 if(runtime||r->self||native_initialized||owner->phase!=CANDIDATE_PS_EMPTY||
    owner->self||owner->config||owner->backend.child)return CAMERA_NOT_READY;
 if(camera_graph_validate(config->graph))return CAMERA_INVALID;
 const struct camera_profile *profile=NULL;
 for(size_t i=0;i<config->graph->profile_count;i++)
  if(!memcmp(config->graph->profiles[i].id,config->profile_id,CAMERA_ID_BYTES))
   profile=&config->graph->profiles[i];
 if(!profile||profile->codec!=CAMERA_H264||profile->width!=1280||profile->height!=720)
  return CAMERA_INVALID; /* fixed real native profile, not a graph-only promise */
 r->self=r;r->config=config;r->graph=config->graph;r->owner=owner;r->bootstrap=bootstrap;
 r->bridge=(struct camera_rtsp_common_producer){.producer=&r->producer,.context=r,
  .start=logical_start,.idle=idle_source,.clock_ns=source_now};
 r->lifecycle=camera_rtsp_common_hooks(&r->bridge);
 r->lifecycle.end=frame_end;r->lifecycle.retire=frame_retire;
 r->source=(struct camera_rtsp_producer){.context=r,.next=next_packet,.release=release_packet,
  .parameter_sets=parameters,.lifecycle=&r->lifecycle};
 config->context=r;config->source_ready=ready;config->source_guard_bind=bind_gate;
 config->retain_uncertain=retain_demand;config->retain_frame_uncertain=retain_frame;
 runtime=r;
 return CAMERA_OK;
}
int qc07f_owned_runtime_bind_postfork_v1(struct qc07f_owned_runtime_v1 *r)
{
 if(!valid(r))return CAMERA_INVALID;
 struct candidate_prestarted *o=r->owner;
 if(r->bound||native_initialized||o->self!=o||o->config!=r->config||
    o->phase!=CANDIDATE_PS_READY||o->backend.config!=r->config||
    !o->backend.initialized||o->backend.failed||o->backend.revoked||
    o->backend.pending||o->backend.pending_peer||!o->backend.child)return CAMERA_NOT_READY;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(o->backend.peers[i])return CAMERA_NOT_READY;
 /* Actual init, not a caller Boolean or memset surrogate. No device operation. */
 nh_native_init();
 if(!native_initialized||native_fault())return CAMERA_IO_ERROR;
 r->bound=1;
 const struct camera_producer_config config={.operation_timeout_ns=NH_OUTPUT_NS,.keyframe_supported=1};
 int result=camera_producer_init(&r->producer,&config);
 struct camera_fmp4_config parameters_config;
 if(!result)result=nh_native_config(&parameters_config); /* original real SPS/PPS generator */
 if(!result){
  size_t bytes=parameters_config.sps_bytes+parameters_config.pps_bytes+8;
  if(bytes>sizeof(parameters))result=CAMERA_INVALID;
  else r->source.parameter_set_bytes=bytes;
 }
 latch(r,result);
 return result;
}
