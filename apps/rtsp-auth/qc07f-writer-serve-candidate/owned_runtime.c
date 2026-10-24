/* SPDX-License-Identifier: GPL-2.0-only
 * ONE actual singleton TU, exclusive SOURCE_API5 derivative. No main.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "owned_runtime_v4.h"
#include "../qc07f-owned-runtime/server/adapter_state.h"
_Static_assert(QC07F_OWNED_RUNTIME_SOURCE_API==5U&&CAMERA_SPAN_HUB_LAYOUT_VERSION==3U,
 "exclusive complete writer/sender/hub definitions");
_Static_assert(sizeof(struct qc07f_owned_runtime_v4)<131072U,"bounded private metadata");
static struct qc07f_owned_runtime_v4 *runtime;
static int frame_zero(struct camera_rtsp_frame_handle f)
{return !f.owner&&!f.id&&!f.generation&&!f.namespace_context;}
static int frame_same(struct camera_rtsp_frame_handle a,struct camera_rtsp_frame_handle b)
{return a.owner==b.owner&&a.id==b.id&&a.generation==b.generation&&a.namespace_context==b.namespace_context;}
static int key_zero(const struct nh_key *k)
{return !k->source&&!k->file&&!k->generation&&!k->sequence&&!k->nal_crc32_bzip2;}
static int loan_zero(const struct qc07f_writer_loan_v4 *p)
{return !p->token&&!p->generation&&!p->monotonic_ns&&key_zero(&p->key)&&
 !p->written_bytes&&!p->prefix_bytes&&!p->keyframe;}
static int request_zero(const struct qc_hwenc_h264_spans_job *q)
{
 if(q->version||q->struct_bytes||q->keyframe||q->span_count||q->width||q->height||
    q->input_bytes||q->input_ptr||q->output_bytes||q->status||q->frame_num)return 0;
 for(unsigned i=0;i<QC_HWENC_OUTPUT_SPANS_COUNT;i++)if(q->spans[i].user_address||q->spans[i].bytes)return 0;
 for(unsigned i=0;i<5;i++)if(q->reserved[i])return 0;
 return 1;
}
static int writer_zero(const struct camera_span_hub_writer_v3 *w)
{
 if(w->owner||w->token||w->generation||w->span_count)return 0;
 for(unsigned i=0;i<8;i++)if(w->spans[i].data||w->spans[i].capacity)return 0;
 return 1;
}
static int live_pipeline_empty(const struct qc07f_owned_runtime_v4 *r)
{return !r->reservation.owner&&!r->reservation.token&&!r->reservation.generation&&
 writer_zero(&r->writer)&&request_zero(&r->request)&&request_zero(&r->request_expected)&&
 !r->result.codec&&!r->result.keyframe&&!r->result.monotonic_ns&&
 !r->result.written_bytes&&!r->result.prefix_bytes&&loan_zero(&r->loan)&&frame_zero(r->frame)&&
 !r->operation_begin_ns&&!r->arm_attempt&&!r->ioctl_attempt&&!r->release_attempt&&
 !r->end_attempt&&!r->commit_attempt;}
static int span_ranges_empty(const struct nh_memory_hub_ranges *a)
{
 if(a->owner||a->count||a->credit_kib)return 0;
 for(unsigned i=0;i<8;i++)if(a->begin[i]||a->end[i])return 0;
  return 1;
}
static int added_history_empty(const struct qc07f_owned_runtime_v4 *r)
{return !r->completed_frame_id&&!r->completed_common_generation&&!r->completed_operation_begin_ns&&
 loan_zero(&r->completed_loan)&&!r->span_relinquished&&!r->span_owner_retired&&
 !r->span_terminal_offset&&!r->span_terminal_bytes&&!r->span_terminal_consumer_issuance;}
static int runtime_span_history_empty(void)
{return !runtime||(!runtime->span_hub&&!runtime->span_setup_attempt&&!runtime->span_native_retire&&
 !runtime->pipeline&&!runtime->attempts&&!runtime->last_token&&!runtime->last_generation&&
 !runtime->error&&!runtime->retired&&!runtime->post_first&&live_pipeline_empty(runtime)&&added_history_empty(runtime));}
static int writer_native_busy(void)
{return runtime&&(runtime->pipeline!=QCW_IDLE||runtime->error||!live_pipeline_empty(runtime));}
#include "../qc07f-writer-runtime-v2/native_adapter.inc"

static int valid(const struct qc07f_owned_runtime_v4 *r)
{return r&&r==runtime&&r->self==r&&r->config&&r->config->context==r&&r->config->graph==r->graph;}
static int span_history_present(void)
{return !runtime_span_history_empty()||!span_ranges_empty(&memory_state.ledger.hub)||
 !span_ranges_empty(&memory_state.span_retained)||memory_state.span_attempt!=NH_MEM_SPAN_FRESH||memory_state.span_failure;}
static int native_owns(void)
{
 if(!native_initialized)return span_history_present();
 return native.watchdog.fd>=3||native.continuous_sensor>=3||native.continuous_frame>=3||
  native.h264_stream_fd>=3||native.encoder_hold>=3||native.legacy_control>=3||
  native.providers.sensor_staged>=3||native.providers.frame_staged.fd>=3||native.providers.encoder_staged.fd>=3||
  native.produced_storage||native.resident_storage||parked_after_stop||native.receipt_armed||native.close_unknown||span_history_present();
}
static int native_fault(void)
{return native_initialized&&(stop||native.retirement_uncertain||source_clock.failed||native.watchdog.state==QW_HOLD||
 native.providers.phase==QP_HOLD||memory_state.span_attempt==NH_MEM_SPAN_HOLD||memory_state.span_failure);}
static int common_drained(const struct qc07f_owned_runtime_v4 *r)
{
 if(!r->producer.initialized||r->bridge.producer!=&r->producer||r->bridge.context!=r||r->bridge.frame_open||r->bridge.clock_failed||
    r->producer.state!=CAMERA_PRODUCER_STOPPED||r->producer.demand_count||r->producer.active_present||
    r->producer.needs_stop||r->producer.queue_count||r->producer.stop_pending||r->producer.keyframe_pending)return 0;
 for(unsigned i=0;i<CAMERA_PRODUCER_CONSUMERS;i++)if(r->producer.consumers[i].attached)return 0;
 return 1;
}
static int completed_native_exact(const struct qc07f_owned_runtime_v4 *r)
{
 const struct qc07f_writer_loan_v4 *p=&r->completed_loan;
 return r->attempts&&r->attempts==r->last_token&&r->last_token==p->token&&r->last_generation==p->generation&&
  r->completed_frame_id&&r->completed_frame_id<=r->bridge.frame_sequence&&
  r->completed_common_generation==r->producer.generation&&r->completed_common_generation==r->bridge.frame.generation&&
  r->bridge.frame.owner==&r->producer&&r->bridge.frame.namespace_context==&r->bridge&&
  r->completed_operation_begin_ns!=UINT64_MAX&&p->monotonic_ns!=UINT64_MAX&&
  p->written_bytes>p->prefix_bytes+1&&p->written_bytes<=1048576U&&
  (p->prefix_bytes==0||p->prefix_bytes==3||p->prefix_bytes==4)&&p->keyframe<=1&&
  p->key.source==native.receipt_key.source_instance&&p->key.file==native.receipt_key.file_instance&&
  p->key.generation==native.receipt_key.generation&&p->key.sequence==native.capture_sequence&&p->key.sequence&&
  p->key.nal_crc32_bzip2==native.produced_nal_crc32_bzip2&&
  p->written_bytes==native.produced_timing.bytes&&p->keyframe==(uint32_t)native.produced_timing.keyframe&&
  native.produced_timing.captured<=UINT64_MAX/1000000ULL&&p->monotonic_ns==native.produced_timing.captured*1000000ULL;
}
static int native_retired_actual(const struct qc07f_owned_runtime_v4 *r)
{
 return native_initialized&&!stop&&r->retired==1&&r->span_native_retire==2&&!r->error&&!native_fault()&&
  r->pipeline==QCW_IDLE&&live_pipeline_empty(r)&&!r->bridge.frame_open&&!r->bridge.clock_failed&&completed_native_exact(r)&&
  r->completed_frame_id==r->bridge.frame.id&&
  native.providers.phase==QP_CLOSED&&native.providers.sensor_staged==-1&&
  native.providers.frame_staged.fd==-1&&native.providers.encoder_staged.fd==-1&&
  native.continuous_sensor==-1&&native.continuous_frame==-1&&native.h264_stream_fd==-1&&
  native.encoder_hold==-1&&native.legacy_control==-1&&!native.receipt_armed&&
   !native.produced_storage&&!native.resident_storage&&!parked_after_stop&&!native.raw_valid&&!native.close_unknown&&
  native.watchdog.state==QW_DISABLED&&native.watchdog.fd>=3&&native.watchdog.phase==QW_RETIRE&&
  native.watchdog.device&&native.watchdog.session_deadline_ns&&
  qc_receipt_same(&native.receipt_key,&native.receipt_evidence)&&
  qc_receipt_retired(&native.receipt_evidence)&&native.receipt_evidence.reason==QC_STOP_END;
}
static int terminal_span_history(const struct qc07f_owned_runtime_v4 *r);
static int retention_needed(void)
{
 if(runtime&&runtime->span_relinquished)
  return !(native_retired_actual(runtime)&&common_drained(runtime)&&terminal_span_history(runtime));
 return span_history_present()||(native_initialized&&(native_owns()||native.raw_valid||native.h264_stream_frames||
  memory_state.ledger.initialized||native.watchdog.session_deadline_ns));
}
static void latch(struct qc07f_owned_runtime_v4 *r,int error)
{if(error&&!r->error)r->error=camera_rtsp_common_result(error);}
static _Noreturn void retain(struct qc07f_owned_runtime_v4 *r,int error)
{
 /* Before any native fence/unsafe unwind, hold the ACTUAL currently armed
  * canonical identity. Wrong caller metadata never substitutes for this one. */
 if(r->span_hub&&r->span_setup_attempt&&r->reservation.owner==r->span_hub&&
    r->reservation.token&&r->span_hub->writer_token==r->reservation.token&&
    r->span_hub->writer_generation==r->reservation.generation&&
    r->span_hub->writer_phase==CAMERA_SPAN_WRITER_V3_ARMED){
  const struct camera_span_hub_writer_v3 identity={.owner=r->span_hub,
   .token=r->reservation.token,.generation=r->reservation.generation};
  (void)camera_span_hub_producer_hold_v3(r->span_hub,&identity);
 }
 latch(r,error?error:CAMERA_IO_ERROR);r->pipeline=QCW_HOLD;
 nh_native_fence(NULL);nh_native_quarantine();
}
void qc07f_owned_runtime_cleanup_v4(void *context,int clock_failed,unsigned final)
{
  struct qc07f_owned_runtime_v4 *r=context;if(!valid(r))return;
  if(r->span_relinquished&&retention_needed())retain(r,CAMERA_IO_ERROR);
 if(((native_fault()||r->error||r->pipeline==QCW_HOLD||clock_failed||r->bridge.clock_failed)&&native_owns())||
    (final&&retention_needed()))retain(r,CAMERA_IO_ERROR);
}
static int overlap(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d)
{return a<d&&b>c;}
static uint64_t interior(uintptr_t a,uintptr_t b)
{uint64_t first=a/4096+(a%4096!=0),last=b/4096;return first<last?(last-first)*4:0;}
static int current_frame(const struct qc07f_owned_runtime_v4 *r,struct camera_rtsp_frame_handle f)
{return r->bridge.frame_open&&!r->bridge.clock_failed&&f.id&&f.owner==&r->producer&&
 f.namespace_context==&r->bridge&&f.generation==r->producer.generation&&frame_same(f,r->bridge.frame);}
static int result_complete(const struct qc07f_owned_runtime_v4 *,uint32_t);
static int result_empty(const struct qc07f_owned_runtime_v4 *r)
{return !r->result.codec&&!r->result.keyframe&&!r->result.monotonic_ns&&!r->result.written_bytes&&
 !r->result.prefix_bytes&&loan_zero(&r->loan);}
static int request_input_exact(const struct qc07f_owned_runtime_v4 *r)
{
 const struct qc_hwenc_h264_spans_job *e=&r->request_expected,*q=&r->request;
 if((uintptr_t)raw>UINT32_MAX||e->version!=QC_HWENC_OUTPUT_SPANS_VERSION||e->struct_bytes!=sizeof(*e)||
    e->span_count!=8||e->width!=1280||e->height!=720||e->input_bytes!=1382400||
    e->input_ptr!=(uint32_t)(uintptr_t)raw||e->keyframe>1||e->output_bytes||e->status||e->frame_num||
    q->version!=e->version||q->struct_bytes!=e->struct_bytes||q->keyframe!=e->keyframe||q->span_count!=e->span_count||
    q->width!=e->width||q->height!=e->height||q->input_bytes!=e->input_bytes||q->input_ptr!=e->input_ptr)return 0;
 for(unsigned i=0;i<8;i++)if((uintptr_t)r->span_hub->chunks[i]>UINT32_MAX||
    e->spans[i].user_address!=(uint32_t)(uintptr_t)r->span_hub->chunks[i]||e->spans[i].bytes!=131072||
    q->spans[i].user_address!=e->spans[i].user_address||q->spans[i].bytes!=e->spans[i].bytes)return 0;
 for(unsigned i=0;i<5;i++)if(e->reserved[i]||q->reserved[i])return 0;
 return 1;
}
static int pipeline_exact(const struct qc07f_owned_runtime_v4 *r)
{
 if(r->error||r->retired||r->span_native_retire||r->operation_begin_ns==UINT64_MAX||
    r->arm_attempt>1||r->ioctl_attempt>1||r->release_attempt>1||r->end_attempt>1||r->commit_attempt)return 0;
 if(r->pipeline==QCW_ENDED){if(r->bridge.frame_open||!frame_same(r->frame,r->bridge.frame))return 0;}
 else if(!current_frame(r,r->frame))return 0;
 if(r->pipeline==QCW_RESERVED){
  if(r->arm_attempt||r->ioctl_attempt||r->release_attempt||r->end_attempt||!result_empty(r))return 0;
  return (request_zero(&r->request)&&request_zero(&r->request_expected))||
   (request_input_exact(r)&&!r->request.output_bytes&&!r->request.status&&!r->request.frame_num);
 }
 if(!r->arm_attempt||!request_input_exact(r))return 0;
 if(r->pipeline==QCW_ARMED&&!r->ioctl_attempt)return !r->release_attempt&&!r->end_attempt&&
  result_empty(r)&&!r->request.output_bytes&&!r->request.status&&!r->request.frame_num;
 if(!r->ioctl_attempt||!result_complete(r,native.providers.codec_frame_num)||
    r->result.codec!=CAMERA_H264||r->result.keyframe!=r->request.keyframe||
    r->result.written_bytes!=r->request.output_bytes||
    (r->result.prefix_bytes!=0&&r->result.prefix_bytes!=3&&r->result.prefix_bytes!=4)||
    r->result.prefix_bytes>=r->result.written_bytes||r->result.monotonic_ns==UINT64_MAX||
    r->loan.token!=r->reservation.token||r->loan.generation!=r->reservation.generation||
    r->loan.monotonic_ns!=r->result.monotonic_ns||r->loan.written_bytes!=r->result.written_bytes||
    r->loan.prefix_bytes!=r->result.prefix_bytes||r->loan.keyframe!=r->result.keyframe||
    r->loan.key.source!=native.receipt_key.source_instance||r->loan.key.file!=native.receipt_key.file_instance||
    r->loan.key.generation!=native.receipt_key.generation||r->loan.key.sequence!=native.capture_sequence||
    r->loan.key.nal_crc32_bzip2!=native.produced_nal_crc32_bzip2)return 0;
 return r->release_attempt==(unsigned)(r->pipeline>=QCW_RELEASING)&&
  r->end_attempt==(unsigned)(r->pipeline==QCW_ENDED);
}
static int writer_association(const struct qc07f_owned_runtime_v4 *r)
{
 const struct camera_span_hub_v1 *h=r->span_hub;
 if(h->layout_version!=3||h->writer_error||h->issued_writer_token!=h->writer_token||
    h->generation!=r->last_generation||h->armed_writer_token>h->writer_token)return 0;
  if(r->pipeline==QCW_IDLE){
  if(!live_pipeline_empty(r)||r->attempts!=r->last_token||h->writer_phase!=CAMERA_SPAN_WRITER_V3_IDLE||
     h->writer_token!=r->last_token||h->writer_generation!=r->last_generation||
     h->armed_writer_token!=r->last_token)return 0;
   if(!r->last_token)return added_history_empty(r)&&!h->generation&&!h->data_offset&&!h->bytes&&!h->codec&&!h->keyframe&&!h->monotonic_ns;
   return completed_native_exact(r)&&h->data_offset==r->completed_loan.prefix_bytes&&
    h->bytes==r->completed_loan.written_bytes-r->completed_loan.prefix_bytes&&
    h->codec==CAMERA_H264&&h->keyframe==r->completed_loan.keyframe&&h->monotonic_ns==r->completed_loan.monotonic_ns;
 }
 if(r->pipeline<QCW_RESERVED||r->pipeline>QCW_ENDED||r->last_token==UINT64_MAX||r->last_generation==UINT64_MAX||
    r->attempts!=r->last_token+1||r->reservation.owner!=h||r->reservation.token!=r->last_token+1||
    r->reservation.generation!=r->last_generation+1||h->writer_token!=r->reservation.token||
    h->writer_generation!=r->reservation.generation||h->pending||h->acquired||!pipeline_exact(r))return 0;
 for(unsigned i=0;i<4;i++)if(h->active_token[i])return 0;
 if(r->pipeline==QCW_RESERVED)return h->writer_phase==CAMERA_SPAN_WRITER_V3_RESERVED&&
  h->armed_writer_token==r->last_token&&!r->arm_attempt&&!r->ioctl_attempt&&writer_zero(&r->writer);
 if(h->writer_phase!=CAMERA_SPAN_WRITER_V3_ARMED||h->armed_writer_token!=h->writer_token||r->arm_attempt!=1||
    r->writer.owner!=h||r->writer.token!=h->writer_token||r->writer.generation!=h->writer_generation||r->writer.span_count!=8)return 0;
 for(unsigned i=0;i<8;i++)if(r->writer.spans[i].data!=h->chunks[i]||r->writer.spans[i].capacity!=131072)return 0;
 return 1;
}
enum paid_mode {PAID_ACTIVE,PAID_RETIRING,PAID_FINAL};
static int paid_association(const struct qc07f_owned_runtime_v4 *r,enum paid_mode mode)
{
  if(r->span_relinquished)return 0; /* BEFORE any former live owner/hub access */
 const struct nh_memory_ledger *l=&memory_state.ledger;
 const struct nh_memory_hub_ranges *a=&l->hub,*b=&memory_state.span_retained;
 const struct camera_span_hub_v1 *h=r->span_hub;
 if(r->span_setup_attempt!=1||!h||h->self!=h||
     (mode==PAID_FINAL?h->phase!=CAMERA_SPAN_HUB_V1_STOPPED:
      (h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE&&(mode!=PAID_RETIRING||h->phase!=CAMERA_SPAN_HUB_V1_STOPPED)))||
     (mode==PAID_FINAL?r->span_native_retire!=2:r->span_native_retire!=0)||
    h->allocation_calls!=8||h->allocated_bytes!=1048576||h->maximum_allocation!=131072||
    memory_state.span_attempt!=NH_MEM_SPAN_PAID||memory_state.span_failure||!l->initialized||
    l->paid[0]!=1||l->paid[1]!=1||l->paid[2]||l->raw_reuse||raw_reuse_requested||
    l->begin[1]||l->end[1]||l->begin[2]||l->end[2]||native.resident_storage||native.produced_storage||parked_after_stop||
    a->owner!=h||b->owner!=h||a->count!=8||b->count!=8||a->credit_kib!=b->credit_kib||
    l->cpu_paid_kib>l->cpu_capacity_kib||!writer_association(r))return 0;
 uintptr_t raw_begin=(uintptr_t)raw;
 if(sizeof(raw)>UINTPTR_MAX-raw_begin||l->begin[0]!=raw_begin||l->end[0]!=raw_begin+sizeof(raw))return 0;
 uint64_t scalar=interior(l->begin[0],l->end[0]),credit=0;if(!scalar)return 0;
 for(unsigned i=0;i<8;i++){
  uintptr_t begin=(uintptr_t)h->chunks[i];if(!begin||131072U>UINTPTR_MAX-begin)return 0;
  uintptr_t end=begin+131072U;
  if(a->begin[i]!=begin||a->end[i]!=end||b->begin[i]!=begin||b->end[i]!=end||overlap(begin,end,l->begin[0],l->end[0]))return 0;
  for(unsigned j=0;j<i;j++)if(overlap(begin,end,a->begin[j],a->end[j]))return 0;
  uint64_t part=interior(begin,end);if(!part||part>UINT64_MAX-credit)return 0;credit+=part;
 }
  return credit==a->credit_kib&&credit<=UINT64_MAX-scalar&&l->cpu_paid_kib==scalar+credit;
}
/* Only numeric terminal ranges, never read/compare/reconstruct an old pointer. */
static int terminal_span_history(const struct qc07f_owned_runtime_v4 *r)
{
 const struct nh_memory_ledger *l=&memory_state.ledger;
 const struct nh_memory_hub_ranges *a=&l->hub,*b=&memory_state.span_retained;
 if(r->span_relinquished!=1||r->span_setup_attempt!=1||r->span_native_retire!=2||!r->span_owner_retired||
    r->span_hub||a->owner||b->owner||!live_pipeline_empty(r)||!completed_native_exact(r)||
    r->span_terminal_offset!=r->completed_loan.prefix_bytes||
    r->span_terminal_bytes!=r->completed_loan.written_bytes-r->completed_loan.prefix_bytes||
    memory_state.span_attempt!=NH_MEM_SPAN_PAID||memory_state.span_failure||
    !l->initialized||l->paid[0]!=1||l->paid[1]!=1||l->paid[2]||l->raw_reuse||raw_reuse_requested||
    l->begin[1]||l->end[1]||l->begin[2]||l->end[2]||a->count!=8||b->count!=8||
    a->credit_kib!=b->credit_kib||l->cpu_paid_kib>l->cpu_capacity_kib||
    l->begin[0]!=(uintptr_t)raw||sizeof(raw)>UINTPTR_MAX-l->begin[0]||l->end[0]!=l->begin[0]+sizeof(raw))return 0;
 uint64_t scalar=interior(l->begin[0],l->end[0]),credit=0;if(!scalar)return 0;
 for(unsigned i=0;i<8;i++){
  uintptr_t begin=a->begin[i],end=a->end[i];
  if(!begin||131072U>UINTPTR_MAX-begin||end!=begin+131072U||b->begin[i]!=begin||b->end[i]!=end||
     overlap(begin,end,l->begin[0],l->end[0]))return 0;
  for(unsigned j=0;j<i;j++)if(overlap(begin,end,a->begin[j],a->end[j]))return 0;
  uint64_t part=interior(begin,end);if(!part||part>UINT64_MAX-credit)return 0;credit+=part;
 }
 return credit==a->credit_kib&&credit<=UINT64_MAX-scalar&&l->cpu_paid_kib==scalar+credit;
}
static int ready(void *context,uint64_t connection,uint64_t now)
{
 struct qc07f_owned_runtime_v4 *r=context;(void)connection;if(!valid(r))return CAMERA_INVALID;
 qc07f_owned_runtime_cleanup_v4(r,now==UINT64_MAX,0);
  if(r->span_relinquished||r->retired||!r->bound||!native_initialized)return CAMERA_NOT_READY;
 if(r->error)return r->error;
 if(!paid_association(r,PAID_ACTIVE)){if(span_history_present())retain(r,CAMERA_IO_ERROR);return CAMERA_NOT_READY;}
 if(stop||native_fault()||provider_terminal(&native)||!provider_shape(&native)||!sensor_baseline_requested||!storage_selected||
    native.watchdog.fd<3||!native.watchdog.session_deadline_ns||now<native.watchdog.session_start_ns||now>=native.watchdog.session_deadline_ns)return CAMERA_NOT_READY;
 if(native.watchdog.state==QW_ACTIVE5)return (r->pipeline==QCW_RESERVED||r->pipeline==QCW_ARMED)&&native.watchdog.phase==QW_NEXT?CAMERA_OK:CAMERA_NOT_READY;
 return native.watchdog.state==QW_UNKNOWN||native.watchdog.state==QW_DISABLED?CAMERA_OK:CAMERA_NOT_READY;
}
static void bind_gate(void *context,int (*gate)(void *),void *gate_context)
{
 struct qc07f_owned_runtime_v4 *r=context;if(!valid(r))return;
 if((gate&&r->gate)||(!gate&&gate_context)){latch(r,CAMERA_INVALID);if(retention_needed())retain(r,CAMERA_INVALID);return;}
 if(!gate)qc07f_owned_runtime_cleanup_v4(r,0,1);
 r->gate=gate;r->gate_context=gate?gate_context:NULL;
}
static void retain_demand(void *context,const struct camera_packet_hub *h,const struct camera_producer_demand *d,unsigned i)
{struct qc07f_owned_runtime_v4 *r=context;(void)h;(void)d;(void)i;if(valid(r))retain(r,CAMERA_IO_ERROR);}
static void retain_frame(void *context,struct camera_rtsp_frame_handle f,int result)
{struct qc07f_owned_runtime_v4 *r=context;(void)f;if(valid(r))retain(r,result);}
static _Noreturn void retain_native_span(void *context,const struct camera_span_hub_v1 *hub,
 const struct camera_producer_demand *demand,unsigned index)
{
 struct qc07f_owned_runtime_v4 *r=context;(void)hub;(void)demand;(void)index;
 if(!valid(r))nh_native_quarantine();
 retain(r,CAMERA_IO_ERROR); /* actual native/writer/process unconditionally */
}
static uint64_t source_now(void *context)
{
 struct qc07f_owned_runtime_v4 *r=context;if(!valid(r)||!r->bound||!native_initialized)return UINT64_MAX;
 uint64_t now=nh_native_now(NULL);qc07f_owned_runtime_cleanup_v4(r,now==UINT64_MAX,0);return now;
}
static int logical_start(void *context)
{
 struct qc07f_owned_runtime_v4 *r=context;if(!valid(r))return CAMERA_INVALID;
 int result=ready(r,0,source_now(r));if(result)return result;
 return r->gate?r->gate(r->gate_context):CAMERA_NOT_READY;
}
/* Actual auth, complete private source identity/receipt/lifetime, same operation
 * and sealed session. All blocking observations charged; never renew budgets. */
static void checked_source(struct qc07f_owned_runtime_v4 *r,unsigned active)
{
 uint64_t now=source_now(r);
 if(now==UINT64_MAX||now<r->operation_begin_ns||now-r->operation_begin_ns>=NH_OUTPUT_NS||
    now<native.watchdog.session_start_ns||now>=native.watchdog.session_deadline_ns||
    ready(r,0,now)||!r->gate||r->gate(r->gate_context)||lifetime()||nh_native_cancelled(NULL))retain(r,CAMERA_IO_ERROR);
 if(active?qw_owner_check(&native.watchdog):qw_owner_idle_observe(&native.watchdog))retain(r,CAMERA_IO_ERROR);
 now=source_now(r);
 if(now==UINT64_MAX||now<r->operation_begin_ns||now-r->operation_begin_ns>=NH_OUTPUT_NS||
    now>=native.watchdog.session_deadline_ns||ready(r,0,now)||r->gate(r->gate_context))retain(r,CAMERA_IO_ERROR);
 /* Charge the final blocking gate AND actual watchdog descriptor observation.
  * Finish with a source-clock fence, never another blocking auth callback. */
 if(active?qw_owner_check(&native.watchdog):qw_owner_idle_observe(&native.watchdog))retain(r,CAMERA_IO_ERROR);
 now=source_now(r);
 if(now==UINT64_MAX||now<r->operation_begin_ns||now-r->operation_begin_ns>=NH_OUTPUT_NS||
    now<native.watchdog.session_start_ns||now>=native.watchdog.session_deadline_ns||
    now<native.watchdog.last_ns||
    (active&&(native.watchdog.state!=QW_ACTIVE5||native.watchdog.phase!=QW_NEXT||
     now<native.watchdog.start_ns||now-native.watchdog.start_ns>=NH_OUTPUT_NS))||
    (!active&&native.watchdog.state!=QW_DISABLED)||ready(r,0,now))retain(r,CAMERA_IO_ERROR);
}
static int range32(const void *p,size_t bytes)
{uintptr_t a=(uintptr_t)p;return a&&a<=UINT32_MAX&&bytes&&bytes<=UINT32_MAX-a;}
static void build_request(struct qc07f_owned_runtime_v4 *r,uint32_t keyframe)
{
 struct qc_hwenc_h264_spans_job *q=&r->request;
 if(!request_zero(q)||!request_zero(&r->request_expected)||!range32(raw,sizeof(raw))||!range32(r,sizeof(*r))||
    !range32(r->span_hub,sizeof(*r->span_hub))||
    !range32(q,sizeof(*q))||!range32(&r->request_expected,sizeof(r->request_expected)))retain(r,CAMERA_INVALID);
 *q=(struct qc_hwenc_h264_spans_job){.version=QC_HWENC_OUTPUT_SPANS_VERSION,.struct_bytes=sizeof(*q),
  .keyframe=keyframe,.span_count=QC_HWENC_OUTPUT_SPANS_COUNT,.width=width,.height=height,
  .input_bytes=MAIN_PIXELS*3/2,.input_ptr=(uint32_t)(uintptr_t)raw};
 if(q->width!=1280||q->height!=720||q->input_bytes!=1382400||q->keyframe>1||
    overlap((uintptr_t)raw,(uintptr_t)raw+sizeof(raw),(uintptr_t)q,(uintptr_t)q+sizeof(*q)))retain(r,CAMERA_INVALID);
 for(unsigned i=0;i<8;i++){
  uint8_t *p=r->span_hub->chunks[i];if(!range32(p,QC_HWENC_OUTPUT_SPAN_BYTES))retain(r,CAMERA_INVALID);
  uintptr_t a=(uintptr_t)p,b=a+QC_HWENC_OUTPUT_SPAN_BYTES;
  if(overlap(a,b,(uintptr_t)raw,(uintptr_t)raw+sizeof(raw))||
     overlap(a,b,(uintptr_t)r,(uintptr_t)r+sizeof(*r))||
     overlap(a,b,(uintptr_t)r->span_hub,(uintptr_t)r->span_hub+sizeof(*r->span_hub)))retain(r,CAMERA_INVALID);
  for(unsigned j=0;j<i;j++)if(overlap(a,b,(uintptr_t)r->span_hub->chunks[j],(uintptr_t)r->span_hub->chunks[j]+131072U))retain(r,CAMERA_INVALID);
  q->spans[i]=(struct qc_hwenc_output_span){(uint32_t)a,QC_HWENC_OUTPUT_SPAN_BYTES};
 }
 r->request_expected=*q;
}
static int result_complete(const struct qc07f_owned_runtime_v4 *r,uint32_t frame_num)
{
 const struct qc_hwenc_h264_spans_job *q=&r->request,*e=&r->request_expected;
 if(q->version!=e->version||q->struct_bytes!=e->struct_bytes||q->keyframe!=e->keyframe||q->span_count!=e->span_count||
    q->width!=e->width||q->height!=e->height||q->input_bytes!=e->input_bytes||q->input_ptr!=e->input_ptr||
    !q->output_bytes||q->output_bytes>1048576U||!(q->status&4U)||(q->status&0x38U)||q->frame_num!=frame_num)return 0;
 for(unsigned i=0;i<8;i++)if(q->spans[i].user_address!=e->spans[i].user_address||q->spans[i].bytes!=e->spans[i].bytes)return 0;
 for(unsigned i=0;i<5;i++)if(q->reserved[i]!=e->reserved[i])return 0;
 return 1;
}
static uint8_t written_byte(const struct qc07f_owned_runtime_v4 *r,size_t offset)
{return r->writer.spans[offset/131072U].data[offset%131072U];}
static void parse_result(struct qc07f_owned_runtime_v4 *r,uint64_t captured)
{
 size_t bytes=r->request.output_bytes,prefix=0;if(bytes<3)retain(r,CAMERA_IO_ERROR);
 if(!written_byte(r,0)&&!written_byte(r,1)){
  if(written_byte(r,2)==1)prefix=3;
  else if(bytes>4&&!written_byte(r,2)&&written_byte(r,3)==1)prefix=4;
 }
 if(bytes<=prefix+1||captured>UINT64_MAX/1000000ULL)retain(r,CAMERA_IO_ERROR);
 uint8_t header=written_byte(r,prefix);unsigned type=header&31;
 if((header&0x80)||(type!=1&&type!=5)||(type==5)!=r->request.keyframe)retain(r,CAMERA_IO_ERROR);
 /* Same polynomial/table/initial/final and full logical NAL as original CRC.
  * Carry ONE scalar accumulator across physical chunks, no per-chunk final xor. */
 static uint32_t table[256];static unsigned initialized;
 if(!initialized){camera_crc_bzip2_table(table);initialized=1;}
 uint64_t crc_begin=camera_rtsp_clock_ns(&source_clock);uint32_t crc=0xffffffffU;
 size_t at=prefix;
 while(at<bytes){
  size_t index=at/131072U,offset=at%131072U,take=131072U-offset;
  if(take>bytes-at)take=bytes-at;
  const uint8_t *p=r->writer.spans[index].data+offset;
  for(size_t i=0;i<take;i++)crc=(crc<<8)^table[((crc>>24)^p[i])&255];
  at+=take;
 }
 crc^=0xffffffffU;uint64_t crc_end=camera_rtsp_clock_ns(&source_clock);
 if(crc_begin==UINT64_MAX||crc_end==UINT64_MAX||crc_end<crc_begin)retain(r,CAMERA_IO_ERROR);
 native.produced_nal_crc32_bzip2=crc;
 r->result=(struct camera_span_hub_result_v3){CAMERA_H264,r->request.keyframe,captured*1000000ULL,bytes,prefix};
 r->loan=(struct qc07f_writer_loan_v4){.token=r->writer.token,.generation=r->writer.generation,
  .monotonic_ns=r->result.monotonic_ns,.key={native.receipt_key.source_instance,native.receipt_key.file_instance,
  native.receipt_key.generation,native.capture_sequence,crc},.written_bytes=(uint32_t)bytes,
  .prefix_bytes=(uint32_t)prefix,.keyframe=r->request.keyframe};
 fprintf(stderr,"NH_NAL_CRC sequence=%u bytes=%zu begin_ns=%llu end_ns=%llu crc_bzip2=%08x\n",
 native.capture_sequence,bytes-prefix,(unsigned long long)crc_begin,(unsigned long long)crc_end,crc);
 if(!(frame_count%trace_every))fprintf(stderr,"RTSP SOURCE timestamp=%u nal_bytes=%zu crc_bzip2=%08x keyframe=%u\n",
 (uint32_t)(captured*90),bytes-prefix,crc,r->request.keyframe);
}
static int typed_next(void *context,struct camera_rtsp_frame_handle frame,uint32_t force,struct qc07f_writer_loan_v4 *out)
{
 struct qc07f_owned_runtime_v4 *r=context;
 if(!valid(r)||!out||force>1)return CAMERA_INVALID;
  if(r->span_relinquished||r->retired||!r->bound||!native_initialized)return CAMERA_NOT_READY;
 qc07f_owned_runtime_cleanup_v4(r,0,0);
 if(writer_native_busy())retain(r,CAMERA_BUSY);
 if(!current_frame(r,frame)||(r->bridge.keyframe.id&&!force))retain(r,CAMERA_STALE_LEASE);
 uint64_t now=source_now(r);
 if(ready(r,0,now)||!r->gate||r->gate(r->gate_context))retain(r,CAMERA_NOT_READY);
 if(nh_native_memory_gate(NULL,NH_MEM_PRE_NEXT)||r->attempts==UINT64_MAX)retain(r,CAMERA_NOT_READY);
 r->attempts++;r->pipeline=QCW_ATTEMPT;r->frame=frame;r->operation_begin_ns=now;
 int result=camera_span_hub_producer_reserve_v3(r->span_hub,&r->reservation);
 if(result)retain(r,result);
 r->pipeline=QCW_RESERVED;
 if(!paid_association(r,PAID_ACTIVE))retain(r,CAMERA_IO_ERROR);
 if(native_guard_begin(QW_NEXT))retain(r,CAMERA_IO_ERROR);
 native.producer_begin_ns=native.watchdog.start_ns;
 /* The OUTER admission cost is included too; never move our earlier start. */
 checked_source(r,1);
 uint64_t begin=milliseconds();if(begin==UINT64_MAX)retain(r,CAMERA_IO_ERROR);
 native.raw_valid=0;
 if(continuous_next(&native))retain(r,CAMERA_IO_ERROR);
 uint64_t captured=milliseconds();if(captured==UINT64_MAX)retain(r,CAMERA_IO_ERROR);
 checked_source(r,1);
 if(before_encode_gate(native.producer_begin_ns)||qw_owner_check(&native.watchdog)||
    !provider_shape(&native)||provider_encoder_observe(&native)||!native.raw_valid)retain(r,CAMERA_IO_ERROR);
 unsigned ownership_keyframe=native.ownership_idr;
 uint32_t keyframe=force||ownership_keyframe||!h264_p_frames||!(native.h264_stream_frames%30);
 uint32_t frame_num=keyframe?0:(native.providers.codec_frame_num+1)&0xffffU;
 build_request(r,keyframe);
 checked_source(r,1);
 r->arm_attempt=1;
 if(camera_span_hub_producer_arm_v3(r->span_hub,&r->reservation,&r->writer))retain(r,CAMERA_IO_ERROR);
 r->pipeline=QCW_ARMED;
 if(!paid_association(r,PAID_ACTIVE))retain(r,CAMERA_IO_ERROR);
 checked_source(r,1); /* actual post-arm/presyscall auth/clock/source checks */
 r->ioctl_attempt=1;native.raw_valid=0;
 if(ioctl(native.h264_stream_fd,QC_HWENC_H264_STREAM_SPANS,&r->request))retain(r,CAMERA_IO_ERROR);
 /* NOTHING in returned metadata is valid before the exact syscall0 above. */
 if(!result_complete(r,frame_num))retain(r,CAMERA_IO_ERROR);
 uint64_t encoded=milliseconds();if(encoded==UINT64_MAX||encoded<begin||encoded-begin>=2000)retain(r,CAMERA_IO_ERROR);
 native.providers.codec_frame_num=r->request.frame_num;
 /* Update real reference history BEFORE exact per-open observer checks result. */
 if(keyframe)native.h264_stream_frames=0;native.h264_stream_frames++;
 if(ownership_keyframe&&keyframe){native.ownership_idr=0;native.ownership_idrs++;}
 parse_result(r,captured);
 native.produced_timing.begin=begin;native.produced_timing.captured=captured;native.produced_timing.encoded=encoded;
 native.produced_timing.bytes=r->request.output_bytes;native.produced_timing.keyframe=keyframe;
 if(keyframe&&native.receipt_armed)fprintf(stderr,"RTSP CAPTURE KEY source=%llu file=%llu generation=%llu IDR=%s LAB-NOT-AUTH\n",
 (unsigned long long)native.receipt_key.source_instance,(unsigned long long)native.receipt_key.file_instance,
 (unsigned long long)native.receipt_key.generation,ownership_keyframe?"ownership":force?"demand":"max-GOP30");
 frame_count++;
 if(!((frame_count-1)%trace_every))fprintf(stderr,"RTSP PRODUCED n=%lu width=%u height=%u h264_bytes=%u capture_ms=%llu encode_ms=%llu NATIVE-CANDIDATE encoder=HARDWARE-H264-IP nal=%s\n",
 frame_count,width,height,r->request.output_bytes,(unsigned long long)(captured-begin),(unsigned long long)(encoded-captured),keyframe?"IDR":"P");
 checked_source(r,1);
 if(!r->post_first){if(nh_native_memory_gate(NULL,NH_MEM_POST_FIRST))retain(r,CAMERA_IO_ERROR);r->post_first=1;}
 checked_source(r,1);
 if(qw_owner_disable_boundary(&native.watchdog))retain(r,CAMERA_IO_ERROR);
 r->pipeline=QCW_LOAN;
 checked_source(r,0);
 *out=r->loan;return CAMERA_OK;
}
static int loan_same(const struct qc07f_writer_loan_v4 *a,const struct qc07f_writer_loan_v4 *b)
{return a->token==b->token&&a->generation==b->generation&&a->monotonic_ns==b->monotonic_ns&&
 a->key.source==b->key.source&&a->key.file==b->key.file&&a->key.generation==b->key.generation&&
 a->key.sequence==b->key.sequence&&a->key.nal_crc32_bzip2==b->key.nal_crc32_bzip2&&
 a->written_bytes==b->written_bytes&&a->prefix_bytes==b->prefix_bytes&&a->keyframe==b->keyframe;}
static int typed_release(void *context,struct camera_rtsp_frame_handle frame,const struct qc07f_writer_loan_v4 *loan)
{
  struct qc07f_owned_runtime_v4 *r=context;if(!valid(r))return CAMERA_INVALID;
  if(r->span_relinquished||r->retired)return CAMERA_NOT_READY;
  qc07f_owned_runtime_cleanup_v4(r,0,0);
 if(!loan||r->pipeline!=QCW_LOAN||r->release_attempt||!current_frame(r,frame)||
    !frame_same(frame,r->frame)||!loan_same(loan,&r->loan))retain(r,CAMERA_STALE_LEASE);
 /* Unlike old void release, this is the ACTUAL typed native CPU-loan boundary:
  * private expected metadata, real source receipt/clock and idle-watchdog proof. */
 r->release_attempt=1;r->pipeline=QCW_RELEASING;checked_source(r,0);
 if(r->loan.key.source!=native.receipt_key.source_instance||r->loan.key.file!=native.receipt_key.file_instance||
    r->loan.key.generation!=native.receipt_key.generation||r->loan.key.sequence!=native.capture_sequence||
    r->loan.key.nal_crc32_bzip2!=native.produced_nal_crc32_bzip2||!paid_association(r,PAID_ACTIVE))retain(r,CAMERA_IO_ERROR);
 r->pipeline=QCW_RELEASED; /* CPU loan retired ONLY here; writer still ARMED */
 return CAMERA_OK;
}
static void retire_live_aliases(struct qc07f_owned_runtime_v4 *r)
{
  /* New typed frontend observes ONLY these private completed scalar facts.
   * This is after actual commit0; never expose/keep a writable alias as receipt. */
  r->completed_frame_id=r->frame.id;r->completed_common_generation=r->frame.generation;
  r->completed_operation_begin_ns=r->operation_begin_ns;r->completed_loan=r->loan;
 /* No callback/fallible action after actual commit. Keep monotonic numeric
  * histories but retire EVERY live source/writer/reservation alias immediately. */
 r->last_token=r->reservation.token;r->last_generation=r->reservation.generation;
 r->reservation=(struct camera_span_hub_reservation_v3){0};
 r->writer=(struct camera_span_hub_writer_v3){0};
 r->request=(struct qc_hwenc_h264_spans_job){0};r->request_expected=(struct qc_hwenc_h264_spans_job){0};
 r->result=(struct camera_span_hub_result_v3){0};r->loan=(struct qc07f_writer_loan_v4){0};
 r->frame=(struct camera_rtsp_frame_handle){0};r->operation_begin_ns=0;
 r->arm_attempt=r->ioctl_attempt=r->release_attempt=r->end_attempt=r->commit_attempt=0;r->pipeline=QCW_IDLE;
}
static int frame_end(void *context,struct camera_rtsp_frame_handle frame,int result,uint32_t keyframe,uint64_t now)
{
 struct camera_rtsp_common_producer *bridge=context;struct qc07f_owned_runtime_v4 *r=bridge->context;
 if(!valid(r))nh_native_quarantine();
 qc07f_owned_runtime_cleanup_v4(r,now==UINT64_MAX,0);
 if(result||r->error||r->pipeline!=QCW_RELEASED||r->release_attempt!=1||r->end_attempt||
    !current_frame(r,frame)||!frame_same(frame,r->frame)||keyframe!=r->result.keyframe||
    keyframe!=r->loan.keyframe||!r->ioctl_attempt||!paid_association(r,PAID_ACTIVE))retain(r,CAMERA_IO_ERROR);
 checked_source(r,0);
 if(now<r->operation_begin_ns||now-r->operation_begin_ns>=NH_OUTPUT_NS||camera_rtsp_common_time(bridge,now))retain(r,CAMERA_IO_ERROR);
 uint64_t actual=source_now(r);
 if(actual==UINT64_MAX||actual<now||actual<r->operation_begin_ns||actual-r->operation_begin_ns>=NH_OUTPUT_NS)retain(r,CAMERA_IO_ERROR);
 now=actual; /* charge actual release/guard cost to the real common END */
 r->end_attempt=1;
 int ended=camera_rtsp_common_end(context,frame,CAMERA_OK,keyframe,now);
 if(ended||bridge->frame_open||bridge->clock_failed||!frame_same(frame,bridge->frame))retain(r,ended?ended:CAMERA_IO_ERROR);
 r->pipeline=QCW_ENDED;
 checked_source(r,0); /* REAL post-END auth/clock/native; no Boolean receipt */
 if(!paid_association(r,PAID_ACTIVE))retain(r,CAMERA_IO_ERROR);
 r->commit_attempt=1;
 if(camera_span_hub_producer_commit_v3(r->span_hub,&r->writer,&r->result))retain(r,CAMERA_IO_ERROR);
 retire_live_aliases(r);return CAMERA_OK;
}
static int idle_source(void *context)
{
 struct qc07f_owned_runtime_v4 *r=context;if(!valid(r))return CAMERA_INVALID;
 if(!r->bound||!native_initialized)return CAMERA_NOT_READY;
 if(r->span_native_retire||writer_native_busy()||r->bridge.frame_open||
    r->producer.state!=CAMERA_PRODUCER_DRAINING||!r->producer.active_present||
    r->producer.active.kind!=CAMERA_PRODUCER_STOP||!paid_association(r,PAID_RETIRING))retain(r,CAMERA_IO_ERROR);
 r->span_native_retire=1;int result=nh_native_retire(NULL);
 if(result)retain(r,result);
 r->retired=1;r->span_native_retire=2;return CAMERA_OK;
}
static int frame_retire(void *context,uint64_t now)
{
 struct camera_rtsp_common_producer *bridge=context;struct qc07f_owned_runtime_v4 *r=bridge->context;
 if(!valid(r))nh_native_quarantine();qc07f_owned_runtime_cleanup_v4(r,now==UINT64_MAX,0);
 if(writer_native_busy())retain(r,CAMERA_BUSY);
 int result=camera_rtsp_common_retire(context,now);
 if(result||r->error)retain(r,result?result:r->error);return CAMERA_OK;
}
int qc07f_owned_runtime_prepare_v4(struct qc07f_owned_runtime_v4 *r,struct candidate_config *config,
 struct candidate_prestarted *owner,void *bootstrap)
{
 if(!r||!config||!owner||!config->graph||!config->random_ready||!config->retain_verifier)return CAMERA_INVALID;
  if(runtime||r->self||r->span_hub||r->span_setup_attempt||r->span_native_retire||r->pipeline||r->attempts||
     !added_history_empty(r)||r->span_sink.context||r->span_sink.retain||
    r->last_token||r->last_generation||r->error||r->retired||r->post_first||!live_pipeline_empty(r)||
    native_initialized||owner->phase!=CANDIDATE_PS_EMPTY||owner->self||owner->config||owner->backend.child)return CAMERA_NOT_READY;
 if(camera_graph_validate(config->graph))return CAMERA_INVALID;
 const struct camera_profile *profile=NULL;
 for(size_t i=0;i<config->graph->profile_count;i++)if(!memcmp(config->graph->profiles[i].id,config->profile_id,CAMERA_ID_BYTES))profile=&config->graph->profiles[i];
 if(!profile||profile->codec!=CAMERA_H264||profile->width!=1280||profile->height!=720)return CAMERA_INVALID;
 r->self=r;r->config=config;r->graph=config->graph;r->owner=owner;r->bootstrap=bootstrap;
 r->bridge=(struct camera_rtsp_common_producer){.producer=&r->producer,.context=r,.start=logical_start,.idle=idle_source,.clock_ns=source_now};
 r->lifecycle=camera_rtsp_common_hooks(&r->bridge);r->lifecycle.end=frame_end;r->lifecycle.retire=frame_retire;
 r->source=(struct qc07f_writer_source_v4){r,typed_next,typed_release,parameters,0,&r->lifecycle};
 config->context=r;config->source_ready=ready;config->source_guard_bind=bind_gate;
  config->retain_uncertain=retain_demand;config->retain_frame_uncertain=retain_frame;
  r->span_sink=(struct camera_rtsp_span_retention_v1){r,retain_native_span};runtime=r;return CAMERA_OK;
}
int qc07f_owned_runtime_bind_postfork_v4(struct qc07f_owned_runtime_v4 *r)
{
 if(!valid(r))return CAMERA_INVALID;struct candidate_prestarted *o=r->owner;
 if(r->bound||native_initialized||o->self!=o||o->config!=r->config||o->phase!=CANDIDATE_PS_READY||
    o->backend.config!=r->config||!o->backend.initialized||o->backend.failed||o->backend.revoked||
    o->backend.pending||o->backend.pending_peer||!o->backend.child)return CAMERA_NOT_READY;
 for(unsigned i=0;i<4;i++)if(o->backend.peers[i])return CAMERA_NOT_READY;
 nh_native_init();if(!native_initialized||native_fault())return CAMERA_IO_ERROR;r->bound=1;
 const struct camera_producer_config config={.operation_timeout_ns=NH_OUTPUT_NS,.keyframe_supported=1};
 int result=camera_producer_init(&r->producer,&config);struct camera_fmp4_config pc;
 if(!result)result=nh_native_config(&pc);
 if(!result){size_t bytes=pc.sps_bytes+pc.pps_bytes+8;if(bytes>sizeof(parameters))result=CAMERA_INVALID;else r->source.parameter_set_bytes=bytes;}
 latch(r,result);return result;
}
int qc07f_owned_runtime_memory_span_hub_v4(struct qc07f_owned_runtime_v4 *r,struct camera_span_hub_v1 *hub)
{
 if(!valid(r))return CAMERA_INVALID;qc07f_owned_runtime_cleanup_v4(r,0,0);
  if(r->span_relinquished||r->retired||!r->bound||!native_initialized)return CAMERA_NOT_READY;
 if(r->error||stop||native_fault()){if(retention_needed())retain(r,CAMERA_IO_ERROR);return CAMERA_NOT_READY;}
 if(span_history_present()){if(!paid_association(r,PAID_ACTIVE))retain(r,CAMERA_IO_ERROR);return CAMERA_NOT_READY;}
 struct candidate_prestarted *o=r->owner;const struct nh_memory_ledger *l=&memory_state.ledger;
 if(!o||o->self!=o||o->config!=r->config||o->phase!=CANDIDATE_PS_READY||o->backend.config!=r->config||
    !o->backend.initialized||o->backend.failed||o->backend.revoked||o->backend.revoke_pending||o->backend.pending||
    o->backend.pending_peer||!o->backend.child||o->source_hold||r->gate||r->gate_context||
    !runtime_span_history_empty()||r->config->source_ready!=ready||r->config->source_guard_bind!=bind_gate||
    r->config->retain_uncertain!=retain_demand||r->config->retain_frame_uncertain!=retain_frame||
    native.providers.phase!=QP_COMMITTED||!native_guard_present()||!sensor_baseline_requested||!storage_selected||
    native.receipt_armed||native.last_capture_generation||native.produced_storage||native.resident_storage||parked_after_stop||native.close_unknown||
    !l->initialized||l->paid[0]||l->paid[1]||l->paid[2]||l->codec_paid||l->cpu_paid_kib||
    l->begin[0]||l->begin[1]||l->begin[2]||l->end[0]||l->end[1]||l->end[2]||l->raw_reuse||raw_reuse_requested||memory_state.count>60||
    memory_state.now!=nh_native_now||memory_state.cancelled!=nh_native_cancelled||memory_state.context||
    native.watchdog.state!=QW_UNKNOWN||native.watchdog.fd<3||!native.watchdog.session_deadline_ns||
    memory_state.session_start!=native.watchdog.session_start_ns)return CAMERA_NOT_READY;
 for(unsigned i=0;i<4;i++)if(o->backend.peers[i])return CAMERA_NOT_READY;
 struct nh_memory_hub_ranges forecast;if(nh_memory_span_hub_forecast(l,hub,&forecast))return CAMERA_NOT_READY;
 r->span_hub=hub;r->span_setup_attempt=1;
  int result=native_memory_fixed_span_buffers(hub);if(result)retain(r,result);return CAMERA_OK;
}

static int actual_owner(const struct qc07f_owned_runtime_v4 *r,enum candidate_prestarted_phase phase)
{
 const struct candidate_prestarted *o=r->owner;
 if(!o||o->self!=o||o->config!=r->config||o->phase!=phase||o->source_hold)return 0;
 const struct candidate_backend *b=&o->backend;
 return b->config==r->config&&b->initialized&&!b->failed&&!b->revoked&&!b->revoke_pending&&
  !b->pending&&!b->pending_peer&&b->child&&rv_ready_flags(b->child)==RV_FLAGS15&&
  rv_ready_profile(b->child)==r->config->image.profile&&is_rv_running(b->child);
}
static int exact_source(const struct qc07f_owned_runtime_v4 *r)
{
 return r->source.context==r&&r->source.next==typed_next&&r->source.release==typed_release&&
  r->source.parameter_sets==parameters&&r->source.parameter_set_bytes&&r->source.parameter_set_bytes<=sizeof(parameters)&&
  r->source.lifecycle==&r->lifecycle&&r->lifecycle.context==&r->bridge&&r->lifecycle.owner==&r->producer&&
  r->lifecycle.attach==camera_rtsp_common_attach&&r->lifecycle.detach==camera_rtsp_common_detach&&
  r->lifecycle.begin==camera_rtsp_common_begin&&r->lifecycle.end==frame_end&&r->lifecycle.retire==frame_retire&&
  !r->lifecycle.failure&&r->bridge.producer==&r->producer&&r->bridge.context==r&&
  r->bridge.start==logical_start&&r->bridge.idle==idle_source&&r->bridge.clock_ns==source_now&&
  r->config->source_ready==ready&&r->config->source_guard_bind==bind_gate&&
  r->config->retain_uncertain==retain_demand&&r->config->retain_frame_uncertain==retain_frame&&
  r->span_sink.context==r&&r->span_sink.retain==retain_native_span;
}
int qc07f_owned_runtime_writer_entry_v5(struct qc07f_owned_runtime_v4 *r)
{
 if(!valid(r))return CAMERA_INVALID;
 qc07f_owned_runtime_cleanup_v4(r,0,0);
 if(r->span_relinquished||r->retired)return CAMERA_NOT_READY;
 if(!r->bound||!native_initialized||stop||r->error||r->bridge.clock_failed||writer_native_busy()||
    !exact_source(r)||r->gate||r->gate_context||!actual_owner(r,CANDIDATE_PS_READY)||
    !paid_association(r,PAID_ACTIVE)){
  if(retention_needed())retain(r,CAMERA_NOT_READY);return CAMERA_NOT_READY;
 }
 for(unsigned i=0;i<4;i++)if(r->owner->backend.peers[i]||r->owner->peers[i].live)retain(r,CAMERA_NOT_READY);
 return CAMERA_OK;
}
int qc07f_owned_runtime_writer_publication_v5(struct qc07f_owned_runtime_v4 *r,
 struct camera_rtsp_frame_handle frame,const struct qc07f_writer_loan_v4 *loan)
{
 if(!valid(r))return CAMERA_INVALID;
 if(r->span_relinquished||r->retired)return CAMERA_NOT_READY; /* before old hub */
 qc07f_owned_runtime_cleanup_v4(r,0,0);
 if(!loan||!exact_source(r)||!actual_owner(r,CANDIDATE_PS_SERVING)||!r->gate||!r->gate_context||
    r->pipeline!=QCW_IDLE||!live_pipeline_empty(r)||r->bridge.frame_open||
    frame.owner!=&r->producer||frame.namespace_context!=&r->bridge||
    frame.id!=r->completed_frame_id||frame.generation!=r->completed_common_generation||
    !loan_same(loan,&r->completed_loan)||!completed_native_exact(r)||!paid_association(r,PAID_ACTIVE))
  retain(r,CAMERA_IO_ERROR);
 /* This observes actual source time/session, not reset operation_begin_ns=0.
  * Caller separately performs real auth before acquisition and on every packet;
  * ready() also rechecks this exact canonical publication in every guarded call. */
 uint64_t now=source_now(r);
 return ready(r,0,now);
}
static void retirement_time_checked(struct qc07f_owned_runtime_v4 *r)
{
 /* No auth callback: all peers are already closed. Charge actual idle watchdog
  * observation to the SAME real native RETIRE start and original sealed session. */
 if(qw_owner_idle_observe(&native.watchdog))retain(r,CAMERA_IO_ERROR);
 uint64_t now=source_now(r);
 if(now==UINT64_MAX||now<native.watchdog.last_ns||now<native.watchdog.start_ns||
    now-native.watchdog.start_ns>=NH_OUTPUT_NS||now<native.watchdog.session_start_ns||
    now>=native.watchdog.session_deadline_ns||native.watchdog.phase!=QW_RETIRE||
    native.watchdog.state!=QW_DISABLED)retain(r,CAMERA_IO_ERROR);
}
void qc07f_owned_runtime_writer_end_borrow_v5(struct qc07f_owned_runtime_v4 *r,
 struct camera_span_hub_v1 *hub,int (*gate)(void *),void *gate_context)
{
 if(!valid(r))nh_native_quarantine();
 if(r->span_relinquished)retain(r,CAMERA_NOT_READY); /* BEFORE retired pointer */
 if(!actual_owner(r,CANDIDATE_PS_SERVING)||!exact_source(r)||!gate||!gate_context||
    r->gate!=gate||r->gate_context!=gate_context||!native_retired_actual(r)||!common_drained(r)||
    !hub||r->span_hub!=hub||!paid_association(r,PAID_FINAL)||!camera_span_hub_drained_v1(hub)||
    hub->subscribers||hub->need_keyframe||hub->pending||hub->acquired||
    hub->writer_phase!=CAMERA_SPAN_WRITER_V3_IDLE||hub->writer_error)
  retain(r,CAMERA_IO_ERROR);
 for(unsigned i=0;i<4;i++)if(r->owner->backend.peers[i]||r->owner->peers[i].live||hub->active_token[i])retain(r,CAMERA_IO_ERROR);
 retirement_time_checked(r);
 /* Last fallible action was above. No callback or old hub access after these
  * scalar snapshots and live-pointer retirement. CPU lender is NOT freed. */
 r->span_owner_retired=(uintptr_t)hub;r->span_terminal_offset=hub->data_offset;
 r->span_terminal_bytes=hub->bytes;r->span_terminal_consumer_issuance=hub->issued_token;
 r->span_hub=NULL;memory_state.ledger.hub.owner=NULL;memory_state.span_retained.owner=NULL;
 r->span_relinquished=1;
}
