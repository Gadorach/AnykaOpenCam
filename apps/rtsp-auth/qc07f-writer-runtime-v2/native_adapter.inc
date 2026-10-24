/* SPDX-License-Identifier: GPL-2.0-only
 * Single-TU derivative of qc07f-span-runtime-candidate/native/native_adapter.c.
 * Original low-level import/provider/receipt/sensor/clock bodies are unchanged.
 */
#include "../qc07f-owned-runtime/native/sensor_baseline.h"
/* Frozen static contiguous bodies are reference-only. No selected caller and
 * prepare_output below refuses before ANY effect; no output malloc exists here. */
static int produce_h264_frame(void *,uint32_t,struct camera_packet *) __attribute__((unused));
static void release_h264_frame(void *,const struct camera_packet *) __attribute__((unused));
#include "../qc07f-owned-runtime/native/rtspd-native-import.c"
#include <sys/resource.h>
static struct qc_rtsp_context native;
static uint8_t parameters[256];
static struct camera_packet expected; /* frozen strict return audit, always zero */
static uint8_t *parked_after_stop; /* never populated by selected typed route */
static struct nh_memory_runtime memory_state;
static uint64_t codec_live_bytes;
static unsigned storage_selected,raw_reuse_requested;
static int native_guard_present(void)
{return !stop&&!native.retirement_uncertain&&!source_clock.failed&&native.watchdog.fd>=3&&
 (native.watchdog.state==QW_UNKNOWN||native.watchdog.state==QW_DISABLED||native.watchdog.state==QW_ACTIVE5)&&provider_shape(&native);}
static int prepare_output(struct qc_rtsp_context *c,uint64_t begin)
{(void)c;(void)begin;return CAMERA_NOT_READY; /* frozen contiguous body cannot allocate/capture */}
static int before_encode_gate(uint64_t begin)
{
 int result=nh_memory_runtime_gate(&memory_state,NH_MEM_PRE_NEXT,begin);
 if(result==NH_MEM_CLOCK)native.retirement_uncertain=1;
 return result?CAMERA_NOT_READY:CAMERA_OK;
}
_Static_assert(sizeof(struct qc_capture_receipt)==56,"receipt ABI");
_Static_assert(sizeof(struct qc_hwenc_lifetime)==56,"lifetime ABI");
_Static_assert(sizeof(struct qc_hwenc_h264_spans_job)==128,"shared span ABI");
_Static_assert(QC_HWENC_H264_STREAM_SPANS==0xc0805167UL,"shared actual ioctl");
static int lifetime(void)
{
 struct qc_hwenc_lifetime q={0};
 if(provider_observe(&native))return CAMERA_IO_ERROR;
 if(ioctl(native.h264_stream_fd,QC_HWENC_LIFETIME_QUERY,&q)||q.version!=1||
    q.struct_bytes!=sizeof(q)||q.reserved)return capture_failed(&native);
 native.providers.global_last=q;
 if(q.quarantined||q.dma_uncertain||q.guard_poisoned||q.failed_reset_step||
    q.reset_error||q.fault_session||q.session_sequence<native.providers.encoder_identity.session_identity)
  return capture_failed(&native);
 fprintf(stderr,"NH_CODEC_LIFETIME session_sequence=%llu live_bytes=%llu healthy=1\n",
 (unsigned long long)q.session_sequence,(unsigned long long)q.live_bytes);
 codec_live_bytes=q.live_bytes;return CAMERA_OK;
}
static void native_init_legacy(void)
{source_context_init(&native);memset(&source_clock,0,sizeof(source_clock));sensor_baseline_requested=0;memset(&sensor_baseline_report,0,sizeof(sensor_baseline_report));memset(&memory_state,0,sizeof(memory_state));codec_live_bytes=0;storage_selected=0;raw_reuse_requested=0;}
int nh_native_storage_select(const char *mode)
{
 if(provider_terminal(&native))return CAMERA_NOT_READY;
 if(!mode||storage_selected||memory_state.ledger.initialized||native.continuous_frame>=0||native.h264_stream_fd>=0)return CAMERA_NOT_READY;
 /* No raw-reuse reinterpretation/output credit under this source profile. */
 if(strcmp(mode,"separate"))return CAMERA_INVALID;
 storage_selected=1;return CAMERA_OK;
}
int nh_native_optin_exact139(void)
{
 if(provider_terminal(&native))return CAMERA_NOT_READY;
 if(native.retirement_uncertain||native.continuous_frame>=0||native.continuous_sensor>=0||native.h264_stream_fd>=0||sensor_baseline_requested)return CAMERA_NOT_READY;
 sensor_baseline_requested=1;return CAMERA_OK;
}
void nh_native_sensor_report(void){nh_sensor_log(&sensor_baseline_report);}
static int native_memory_start_legacy(uint64_t start)
{
 if(!native_guard_present()||!storage_selected)return CAMERA_NOT_READY;
 int r=nh_memory_runtime_init(&memory_state,start,nh_native_now,nh_native_cancelled,NULL,raw_reuse_requested);
 if(!r)r=nh_memory_runtime_gate(&memory_state,NH_MEM_INITIAL,start);
 if(r==NH_MEM_CLOCK)native.retirement_uncertain=1;
 return r?CAMERA_NOT_READY:CAMERA_OK;
}
int nh_native_memory_fixed_buffers(void *hub)
{(void)hub;return CAMERA_NOT_READY;}
int nh_native_memory_gate(void *unused,enum nh_memory_stage stage)
{
 if(!native_guard_present())return CAMERA_NOT_READY;
 (void)unused;if(!memory_state.ledger.initialized)return CAMERA_NOT_READY;
 if(stage==NH_MEM_POST_FIRST&&nh_memory_credit_codec(&memory_state.ledger,codec_live_bytes,!native.retirement_uncertain&&!source_clock.failed))return CAMERA_NOT_READY;
 uint64_t begin=nh_native_now(NULL);int r=nh_memory_runtime_gate(&memory_state,stage,begin);
 if(r==NH_MEM_CLOCK)native.retirement_uncertain=1;
 return r?CAMERA_NOT_READY:CAMERA_OK;
}
void nh_native_memory_report(void){nh_memory_log(&memory_state);}
void nh_native_cancel(int sig){on_signal(sig);}
uint64_t nh_native_now(void *unused)
{
 (void)unused;uint64_t now=producer_clock_ns(NULL);
 if(now==UINT64_MAX){native.retirement_uncertain=1;qw_owner_fail(&native.watchdog);}
 return now;
}
int nh_native_cancelled(void *unused)
{
 (void)unused;
 if(stop||native.retirement_uncertain)return 1;
 if(native.receipt_armed){
  struct qc_capture_receipt proof;
  if(provider_receipt_query(&native,&native.receipt_key,&proof)||
     proof.state!=QC_RECEIPT_ACTIVE||proof.reason||proof.stop_result||proof.stopped_ns){
   native.retirement_uncertain=1;return 1;
  }
  native.receipt_evidence=proof;
 }
 return 0;
}
static int native_retire_body(void *unused)
{
 (void)unused;if(lifetime())return CAMERA_IO_ERROR;int r=idle_h264_checked(&native);
 fprintf(stderr,"NH_NATIVE_RETIRE result=%d source=%llu file=%llu generation=%llu state=%u reason=%u stop_result=%d stopped_ns=%llu armed=%u uncertain=%d\n",
 r,(unsigned long long)native.receipt_key.source_instance,(unsigned long long)native.receipt_key.file_instance,
 (unsigned long long)native.receipt_key.generation,native.receipt_evidence.state,native.receipt_evidence.reason,
 native.receipt_evidence.stop_result,(unsigned long long)native.receipt_evidence.stopped_ns,
 native.receipt_armed,native.retirement_uncertain);
 if(!r&&qw_owner_check(&native.watchdog)){nh_native_fence(NULL);return CAMERA_IO_ERROR;}
 return r;
}
int nh_native_config(struct camera_fmp4_config *c)
{
 if(provider_terminal(&native))return CAMERA_NOT_READY;
 size_t n=qc_h264_parameter_sets(parameters),split=4;
 while(split+4<n&&memcmp(parameters+split,"\0\0\0\1",4))split++;
 if(split+4>=n)return CAMERA_INVALID;
 *c=(struct camera_fmp4_config){.sps=parameters+4,.sps_bytes=split-4,
 .pps=parameters+split+4,.pps_bytes=n-split-4,.generation=0};return CAMERA_OK;
}
void nh_native_fence(void *unused){(void)unused;native.retirement_uncertain=1;qw_owner_fail(&native.watchdog);provider_hold(&native);}
_Noreturn void nh_native_quarantine(void)
{
 nh_native_fence(NULL);
 fprintf(stderr,"NH_QUARANTINE writer_profile retained_source_fds=%d,%d encoder=%d watchdog_actual_fd=%d watchdog_state=%u close_unknown=%u parent_recovery_REQUIRED\n",
 native.continuous_frame,native.continuous_sensor,native.h264_stream_fd,native.watchdog.fd,(unsigned)native.watchdog.state,native.close_unknown);
 fflush(stderr);for(;;)raise(SIGSTOP);
}
#include "native-watchdog.inc"
#include "../qc07f-owned-runtime/native/provider-take.inc"
static int native_memory_fixed_span_buffers(struct camera_span_hub_v1 *hub)
{
 if(!native_guard_present())return CAMERA_NOT_READY;
 uint64_t begin=nh_native_now(NULL);
 if(begin==UINT64_MAX)return native_guard_failure();
 if(lifetime())return CAMERA_NOT_READY;
 int r=nh_memory_runtime_gate(&memory_state,NH_MEM_FIXED_PAID,begin);
 if(!r)r=nh_memory_prefault_owned(&memory_state,0,raw,sizeof(raw),begin);
 if(!r)r=nh_memory_runtime_gate(&memory_state,NH_MEM_FIXED_PAID,begin);
 if(!r)r=nh_memory_prefault_span_hub(&memory_state,hub,begin);
 if(r==NH_MEM_CLOCK)native.retirement_uncertain=1;
 return r?CAMERA_NOT_READY:CAMERA_OK;
}
/* Complete old return body consumes our exhaustive runtime history predicate. */
#include "../qc07f-span-runtime-candidate/native/provider-return.inc"
