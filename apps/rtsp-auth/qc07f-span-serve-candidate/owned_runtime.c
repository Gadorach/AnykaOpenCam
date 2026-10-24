/* SPDX-License-Identifier: GPL-2.0-only
 * Explicit source derivative. Direct inclusion keeps ONE real private native
 * owner. Never compile native_adapter.c separately or link the original unit.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "owned_runtime_v1.h"
#include "../qc07f-span-memory/memory_runtime.h"

static struct qc07f_owned_runtime_v1 *runtime;

static int span_ranges_empty(const struct nh_memory_hub_ranges *a)
{
 if(a->owner||a->count||a->credit_kib)return 0;
 for(unsigned i=0;i<NH_MEMORY_HUB_RANGES;i++)if(a->begin[i]||a->end[i])return 0;
 return 1;
}
static int span_runtime_fields_empty(const struct qc07f_owned_runtime_v1 *r)
{return !r->span_hub&&!r->span_setup_attempt&&!r->span_native_retire&&
 !r->span_relinquished&&!r->span_output_begin&&!r->span_output_end&&!r->span_owner_retired;}
static int runtime_span_history_empty(void)
{return !runtime||span_runtime_fields_empty(runtime);}

#include "../qc07f-span-runtime-candidate/native/native_adapter.c"

static int valid(const struct qc07f_owned_runtime_v1 *r)
{return r&&r==runtime&&r->self==r&&r->config&&r->config->context==r&&r->config->graph==r->graph;}

static int span_history_present(const struct qc07f_owned_runtime_v1 *r)
{
  return (r&&!span_runtime_fields_empty(r))||
  !span_ranges_empty(&memory_state.ledger.hub)||!span_ranges_empty(&memory_state.span_retained)||
  memory_state.span_attempt!=NH_MEM_SPAN_FRESH||memory_state.span_failure;
}
static int native_owns(void)
{
 if(!native_initialized)return 0; /* static zero FD fields are NOT ownership */
 return native.watchdog.fd>=3||native.continuous_sensor>=3||native.continuous_frame>=3||
  native.h264_stream_fd>=3||native.encoder_hold>=3||native.legacy_control>=3||
  native.providers.sensor_staged>=3||native.providers.frame_staged.fd>=3||
  native.providers.encoder_staged.fd>=3||native.produced_storage||native.resident_storage||
  parked_after_stop||native.receipt_armed||native.close_unknown||span_history_present(runtime);
}
static int native_fault(void)
{return native_initialized&&(native.retirement_uncertain||source_clock.failed||
 native.watchdog.state==QW_HOLD||native.providers.phase==QP_HOLD||
 memory_state.span_attempt==NH_MEM_SPAN_HOLD||memory_state.span_failure);}

static int native_retired_actual(const struct qc07f_owned_runtime_v1 *r)
{
  /* Original retired predicates, strengthened by staged-FD/clock/custody checks;
   * excludes ONLY the CPU hub borrow, not source/watchdog/FD ownership. */
  return native_initialized&&!stop&&r->retired&&!r->error&&!native_fault()&&
   !r->bridge.clock_failed&&native.providers.phase==QP_CLOSED&&
   native.providers.sensor_staged==-1&&native.providers.frame_staged.fd==-1&&
   native.providers.encoder_staged.fd==-1&&
   native.continuous_sensor==-1&&native.continuous_frame==-1&&native.h264_stream_fd==-1&&
  native.encoder_hold==-1&&native.legacy_control==-1&&!native.receipt_armed&&
  !native.produced_storage&&!native.resident_storage&&!parked_after_stop&&
   !native.close_unknown&&!r->loan_present&&native.watchdog.state==QW_DISABLED&&
   native.watchdog.fd>=3&&native.watchdog.phase==QW_RETIRE&&
   native.watchdog.session_deadline_ns&&native.watchdog.device&&
   qc_receipt_same(&native.receipt_key,&native.receipt_evidence)&&
   qc_receipt_retired(&native.receipt_evidence)&&native.receipt_evidence.reason==QC_STOP_END;
}
static int terminal_span_history(const struct qc07f_owned_runtime_v1 *r);
static int retired_actual(const struct qc07f_owned_runtime_v1 *r)
{return native_retired_actual(r)&&(!span_history_present(r)||terminal_span_history(r));}
static int retention_needed(const struct qc07f_owned_runtime_v1 *r)
{
  if(r->span_relinquished)return !retired_actual(r);
 if(span_history_present(r))return 1; /* includes pre-touch borrow/failure */
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
  if(r->span_relinquished&&!retired_actual(r))retain(r,CAMERA_IO_ERROR);
  if((native_fault()&&native_owns())||
     ((clock_failed||r->bridge.clock_failed)&&native_owns())||
    ((clock_failed||r->bridge.clock_failed||final)&&retention_needed(r)))retain(r,CAMERA_IO_ERROR);
}
static int span_overlap(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d)
{return a<d&&b>c;}
static uint64_t span_interior_kib(uintptr_t a,uintptr_t b)
{
 uint64_t first=a/4096+(a%4096!=0),last=b/4096;
 return first<last?(last-first)*4:0;
}
enum span_association_mode {SPAN_READY_ACTIVE,SPAN_BEFORE_NATIVE_RETIRE,SPAN_FINAL_STOPPED};
static int span_paid_association(const struct qc07f_owned_runtime_v1 *r,enum span_association_mode mode)
{
  if(r->span_relinquished)return 0; /* BEFORE any retired owner/hub pointer read */
 const struct nh_memory_ledger *l=&memory_state.ledger;
 const struct nh_memory_hub_ranges *a=&l->hub,*b=&memory_state.span_retained;
 const struct camera_span_hub_v1 *h=r->span_hub;
  if(r->span_setup_attempt!=1||!h||h->self!=h||
     (mode==SPAN_FINAL_STOPPED?h->phase!=CAMERA_SPAN_HUB_V1_STOPPED:
      mode==SPAN_READY_ACTIVE?h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE:
      (h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE&&h->phase!=CAMERA_SPAN_HUB_V1_STOPPED))||
     (mode==SPAN_FINAL_STOPPED?r->span_native_retire!=2:r->span_native_retire!=0)||
    h->allocation_calls!=8||h->allocated_bytes!=1048576||h->maximum_allocation!=131072||
    memory_state.span_attempt!=NH_MEM_SPAN_PAID||memory_state.span_failure||
    !l->initialized||l->paid[0]!=1||l->paid[1]!=1||l->paid[2]>1||l->raw_reuse>1||
    l->begin[1]||l->end[1]||a->owner!=h||b->owner!=h||
    a->count!=NH_MEMORY_HUB_RANGES||b->count!=NH_MEMORY_HUB_RANGES||
    a->credit_kib!=b->credit_kib||l->cpu_paid_kib>l->cpu_capacity_kib)return 0;
 /* Original actual raw ownership/payment must still agree, not just paid[0]. */
 uintptr_t raw_begin=(uintptr_t)raw;
 if(sizeof(raw)>UINTPTR_MAX-raw_begin||l->begin[0]!=raw_begin||
    l->end[0]!=raw_begin+sizeof(raw))return 0;
 uint64_t scalar=span_interior_kib(l->begin[0],l->end[0]),credit=0;
 if(!scalar)return 0;
 if(l->paid[2]){
   if(l->raw_reuse||
      (mode==SPAN_FINAL_STOPPED?
       (l->begin[2]!=r->span_output_begin||l->end[2]!=r->span_output_end):
       (!native.resident_storage||l->begin[2]!=(uintptr_t)native.resident_storage))||
     1048576U>UINTPTR_MAX-l->begin[2]||l->end[2]!=l->begin[2]+1048576U||
     span_overlap(l->begin[0],l->end[0],l->begin[2],l->end[2]))return 0;
  uint64_t extra=span_interior_kib(l->begin[2],l->end[2]);
  if(!extra||extra>UINT64_MAX-scalar)return 0;scalar+=extra;
  }else if(l->begin[2]||l->end[2]||r->span_output_begin||r->span_output_end)return 0;
 for(unsigned i=0;i<NH_MEMORY_HUB_RANGES;i++){
  uintptr_t begin=(uintptr_t)h->chunks[i];
  if(!begin||NH_MEMORY_HUB_BYTES>UINTPTR_MAX-begin)return 0;
  uintptr_t end=begin+NH_MEMORY_HUB_BYTES;
  if(a->begin[i]!=begin||a->end[i]!=end||b->begin[i]!=begin||b->end[i]!=end)return 0;
  for(unsigned j=0;j<i;j++)if(span_overlap(begin,end,a->begin[j],a->end[j]))return 0;
  for(unsigned j=0;j<3;j++)if(j!=1&&l->paid[j]&&span_overlap(begin,end,l->begin[j],l->end[j]))return 0;
  uint64_t part=span_interior_kib(begin,end);
  if(!part||part>UINT64_MAX-credit)return 0;credit+=part;
 }
 /* No generation/token/client/pristine test: legitimate publications are OK.
  * Never call unpaid-only forecast after payment. Exact source history only. */
  return credit==a->credit_kib&&credit<=UINT64_MAX-scalar&&
   l->cpu_paid_kib==scalar+credit;
}
/* Pointer-free terminal audit. Never reconstruct/dereference the old hub or
 * freed output from retained integers. Payments/ranges remain immutable history. */
static int terminal_span_history(const struct qc07f_owned_runtime_v1 *r)
{
 const struct nh_memory_ledger *l=&memory_state.ledger;
 const struct nh_memory_hub_ranges *a=&l->hub,*b=&memory_state.span_retained;
 if(r->span_relinquished!=1||r->span_setup_attempt!=1||r->span_native_retire!=2||
    !r->span_owner_retired||r->span_hub||a->owner||b->owner||
    memory_state.span_attempt!=NH_MEM_SPAN_PAID||memory_state.span_failure||
    !l->initialized||l->paid[0]!=1||l->paid[1]!=1||l->paid[2]>1||l->raw_reuse>1||
    l->begin[1]||l->end[1]||a->count!=8||b->count!=8||
    a->credit_kib!=b->credit_kib||l->cpu_paid_kib>l->cpu_capacity_kib||
    l->begin[0]!=(uintptr_t)raw||sizeof(raw)>UINTPTR_MAX-l->begin[0]||
    l->end[0]!=l->begin[0]+sizeof(raw))return 0;
 uint64_t scalar=span_interior_kib(l->begin[0],l->end[0]),credit=0;
 if(!scalar)return 0;
 if(l->paid[2]){
  if(l->raw_reuse||!r->span_output_begin||l->begin[2]!=r->span_output_begin||
     l->end[2]!=r->span_output_end||1048576U>UINTPTR_MAX-l->begin[2]||
     l->end[2]!=l->begin[2]+1048576U||
     span_overlap(l->begin[0],l->end[0],l->begin[2],l->end[2]))return 0;
  uint64_t extra=span_interior_kib(l->begin[2],l->end[2]);
  if(!extra||extra>UINT64_MAX-scalar)return 0;scalar+=extra;
 }else if(l->begin[2]||l->end[2]||r->span_output_begin||r->span_output_end)return 0;
 for(unsigned i=0;i<8;i++){
  uintptr_t begin=a->begin[i],end=a->end[i];
  if(!begin||131072U>UINTPTR_MAX-begin||end!=begin+131072U||
     b->begin[i]!=begin||b->end[i]!=end)return 0;
  for(unsigned j=0;j<i;j++)if(span_overlap(begin,end,a->begin[j],a->end[j]))return 0;
  for(unsigned j=0;j<3;j++)if(j!=1&&l->paid[j]&&span_overlap(begin,end,l->begin[j],l->end[j]))return 0;
  uint64_t part=span_interior_kib(begin,end);
  if(!part||part>UINT64_MAX-credit)return 0;credit+=part;
 }
 return credit==a->credit_kib&&credit<=UINT64_MAX-scalar&&l->cpu_paid_kib==scalar+credit;
}
static int ready(void *context,uint64_t connection,uint64_t now)
{
 struct qc07f_owned_runtime_v1 *r=context;(void)connection;
 if(!valid(r))return CAMERA_INVALID;
  qc07f_owned_runtime_cleanup_v1(r,now==UINT64_MAX,0);
  if(r->span_relinquished||r->retired)return CAMERA_NOT_READY;
 if(!r->bound||!native_initialized)return CAMERA_NOT_READY;
 if(r->error)return r->error;
  if(!span_paid_association(r,SPAN_READY_ACTIVE)){
  if(span_history_present(r))retain(r,CAMERA_IO_ERROR);
  return CAMERA_NOT_READY;
 }
 if(stop||native_fault()||provider_terminal(&native)||!provider_shape(&native)||
    !sensor_baseline_requested||!storage_selected||!memory_state.ledger.initialized||
    !memory_state.ledger.paid[0]||
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
static _Noreturn void retain_native_span(void *context,const struct camera_span_hub_v1 *hub,
 const struct camera_producer_demand *demand,unsigned index)
{
 struct qc07f_owned_runtime_v1 *r=context;(void)hub;(void)demand;(void)index;
 /* Trusted valid stable sink identity. Even an identity breach never unwinds. */
 if(!valid(r))nh_native_quarantine();
 retain(r,CAMERA_IO_ERROR);
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
  if(r->span_relinquished||r->retired)return CAMERA_NOT_READY;
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
  if(r->span_relinquished)return; /* no packet or retired-hub access */
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
  /* Actual STOP dispatch only, not freshly initialized STOPPED/no-work. Preserve
   * output integers BEFORE native_retired_cpu_release frees that actual owner. */
  if(r->span_native_retire||r->span_relinquished||r->loan_present||r->bridge.frame_open||
     r->producer.state!=CAMERA_PRODUCER_DRAINING||!r->producer.active_present||
     r->producer.active.kind!=CAMERA_PRODUCER_STOP||
     !span_paid_association(r,SPAN_BEFORE_NATIVE_RETIRE))retain(r,CAMERA_IO_ERROR);
  r->span_output_begin=memory_state.ledger.begin[2];
  r->span_output_end=memory_state.ledger.end[2];r->span_native_retire=1;
  int result=nh_native_retire(NULL);
 if(result){latch(r,result);if(retention_needed(r))retain(r,result);return result;}
  r->retired=1;r->span_native_retire=2;
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
  if(runtime||r->self||!span_runtime_fields_empty(r)||r->span_sink.context||r->span_sink.retain||
     native_initialized||owner->phase!=CANDIDATE_PS_EMPTY||
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
  r->span_sink=(struct camera_rtsp_span_retention_v1){r,retain_native_span};
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

int qc07f_owned_runtime_memory_span_hub_v1(struct qc07f_owned_runtime_v1 *r,
 struct camera_span_hub_v1 *hub)
{
  if(!valid(r))return CAMERA_INVALID;
  qc07f_owned_runtime_cleanup_v1(r,0,0);
  if(r->span_relinquished||r->retired)return CAMERA_NOT_READY;
 if(!r->bound||!native_initialized)return CAMERA_NOT_READY;
 if(r->error){if(retention_needed(r))retain(r,r->error);return r->error;}
 if(stop||native_fault()){if(retention_needed(r))retain(r,CAMERA_IO_ERROR);return CAMERA_NOT_READY;}
 /* A prior borrow is NEVER replaced. A paid repeat is a no-effect refusal;
  * an incomplete previous call is uncertain and cannot unwind/retry. */
 if(span_history_present(r)){
   if(!span_paid_association(r,SPAN_READY_ACTIVE))retain(r,CAMERA_IO_ERROR);
  return CAMERA_NOT_READY;
 }
 struct candidate_prestarted *o=r->owner;
 const struct nh_memory_ledger *l=&memory_state.ledger;
 if(!o||o->self!=o||o->config!=r->config||o->phase!=CANDIDATE_PS_READY||
    o->backend.config!=r->config||!o->backend.initialized||o->backend.failed||
    o->backend.revoked||o->backend.revoke_pending||o->backend.pending||o->backend.pending_peer||
    !o->backend.child||o->source_hold||r->gate||r->gate_context||r->loan_present||r->retired||r->post_first||
    r->config->source_ready!=ready||r->config->source_guard_bind!=bind_gate||
    r->config->retain_uncertain!=retain_demand||r->config->retain_frame_uncertain!=retain_frame||
    native.providers.phase!=QP_COMMITTED||!native_guard_present()||
    !sensor_baseline_requested||!storage_selected||native.receipt_armed||native.last_capture_generation||
    native.produced_storage||native.resident_storage||parked_after_stop||native.close_unknown||
    !l->initialized||l->paid[0]||l->paid[1]||l->paid[2]||l->codec_paid||l->cpu_paid_kib||
    l->begin[0]||l->begin[1]||l->begin[2]||l->end[0]||l->end[1]||l->end[2]||
    l->raw_reuse!=raw_reuse_requested||memory_state.count>60||
    memory_state.now!=nh_native_now||memory_state.cancelled!=nh_native_cancelled||memory_state.context||
    native.watchdog.state!=QW_UNKNOWN||native.watchdog.fd<3||!native.watchdog.session_deadline_ns||
    memory_state.session_start!=native.watchdog.session_start_ns)return CAMERA_NOT_READY;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(o->backend.peers[i])return CAMERA_NOT_READY;
 struct nh_memory_hub_ranges forecast;
 if(nh_memory_span_hub_forecast(l,hub,&forecast))return CAMERA_NOT_READY;
 /* Acquisition point precedes even the first native clock/query/raw touch.
  * No call-scope owner, setter, callback success flag, reset or cleanup. */
 r->span_hub=hub;r->span_setup_attempt=1;
 int result=native_memory_fixed_span_buffers(hub);
 if(result)retain(r,result); /* first error, actual owner and true credit survive */
  return CAMERA_OK; /* no post-credit callback/check or renewed unit start */
}

int qc07f_owned_runtime_span_entry_v1(struct qc07f_owned_runtime_v1 *r)
{
 if(!valid(r))return CAMERA_INVALID;
 qc07f_owned_runtime_cleanup_v1(r,0,0);
 if(r->span_relinquished||r->retired)return CAMERA_NOT_READY;
 if(stop||r->error||r->bridge.clock_failed||r->loan_present||
    !r->bound||!native_initialized||r->span_sink.context!=r||
    r->span_sink.retain!=retain_native_span||r->source.context!=r||
    r->source.next!=next_packet||r->source.release!=release_packet||
    r->source.parameter_sets!=parameters||!r->source.parameter_set_bytes||
    r->source.parameter_set_bytes>sizeof(parameters)||
    r->source.lifecycle!=&r->lifecycle||r->lifecycle.context!=&r->bridge||
    r->lifecycle.owner!=&r->producer||r->lifecycle.end!=frame_end||
    r->lifecycle.attach!=camera_rtsp_common_attach||r->lifecycle.detach!=camera_rtsp_common_detach||
    r->lifecycle.begin!=camera_rtsp_common_begin||
    r->lifecycle.retire!=frame_retire||r->bridge.producer!=&r->producer||
    r->bridge.context!=r||r->bridge.start!=logical_start||r->bridge.idle!=idle_source||
    r->bridge.clock_ns!=source_now||r->config->source_ready!=ready||
    r->config->source_guard_bind!=bind_gate||r->config->retain_uncertain!=retain_demand||
    r->config->retain_frame_uncertain!=retain_frame||r->gate||r->gate_context||
    !r->owner||r->owner->config!=r->config||!span_paid_association(r,SPAN_READY_ACTIVE)){
  if(retention_needed(r))retain(r,CAMERA_NOT_READY);
  return CAMERA_NOT_READY;
 }
 return CAMERA_OK;
}

void qc07f_owned_runtime_span_end_borrow_v1(struct qc07f_owned_runtime_v1 *r,
 struct camera_span_hub_v1 *hub,int (*gate)(void *),void *gate_context)
{
 if(!valid(r))nh_native_quarantine();
 /* Terminal rejection precedes any potentially retired caller pointer access. */
 if(r->span_relinquished)retain(r,CAMERA_NOT_READY);
 struct candidate_prestarted *o=r->owner;
 if(!o||o->self!=o||o->config!=r->config||o->phase!=CANDIDATE_PS_SERVING||
    o->source_hold||o->backend.config!=r->config||!o->backend.initialized||
    !o->backend.child||o->backend.failed||o->backend.revoked||o->backend.revoke_pending||
    o->backend.pending||o->backend.pending_peer||!gate||!gate_context||
    r->gate!=gate||r->gate_context!=gate_context||!native_retired_actual(r)||
    r->bridge.producer!=&r->producer||r->bridge.context!=r||r->bridge.frame_open||
    r->bridge.clock_failed||r->producer.state!=CAMERA_PRODUCER_STOPPED||
    r->producer.demand_count||r->producer.active_present||r->producer.needs_stop||
    r->producer.queue_count||r->producer.stop_pending||r->producer.keyframe_pending||
    !hub||r->span_hub!=hub||!span_paid_association(r,SPAN_FINAL_STOPPED)||
    !camera_span_hub_drained_v1(hub)||hub->subscribers||hub->pending||hub->acquired)
  retain(r,CAMERA_IO_ERROR);
 for(unsigned i=0;i<CAMERA_PRODUCER_CONSUMERS;i++)
  if(r->producer.consumers[i].attached)retain(r,CAMERA_IO_ERROR);
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)
  if(o->backend.peers[i]||o->peers[i].live||hub->active_token[i])retain(r,CAMERA_IO_ERROR);
 /* ACK: last live hub access is above. No callback/refusal/IO follows this
  * serialized transition. Integers/payments/attempts remain terminal audit only. */
 r->span_owner_retired=(uintptr_t)hub;
 r->span_hub=NULL;memory_state.ledger.hub.owner=NULL;memory_state.span_retained.owner=NULL;
 r->span_relinquished=1;
}
