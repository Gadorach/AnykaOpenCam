/* SPDX-License-Identifier: GPL-2.0-only
 * Included after real provider take/wrappers. No helper or stack-only FD owner.
 */
#include "provider-return.h"
static int return_uncertain(void)
{
 return stop||native.retirement_uncertain||source_clock.failed||native.watchdog.state==QW_HOLD||
  native.providers.phase==QP_HOLD||native.providers.frame_staged.state==QC_FRAME_FD_HOLD||
  native.providers.encoder_staged.state==QC_ENCODER_FD_HOLD;
}
static int return_boundary(uint64_t begin)
{
 if(return_uncertain()||qw_owner_unknown_bootstrap_observe(&native.watchdog,begin)||return_uncertain())
  return native_guard_failure();
 return CAMERA_OK;
}
static int return_receipt_empty(const struct qc_capture_receipt *q)
{
 return !q->version&&!q->reserved&&!q->source_instance&&!q->file_instance&&!q->generation&&
  !q->state&&!q->reason&&!q->stop_result&&!q->reserved2&&!q->stopped_ns;
}
static int return_local_history_empty(void)
{
 const struct nh_memory_ledger *l=&memory_state.ledger;
 return native.providers.phase==QP_COMMITTED&&provider_shape(&native)&&
  native.providers.frame_staged.state==QC_FRAME_FD_EMPTY&&
  native.providers.encoder_staged.state==QC_ENCODER_FD_EMPTY&&
  !native.receipt_armed&&!native.last_source_instance&&!native.last_file_instance&&
  !native.last_capture_generation&&!native.capture_sequence&&!native.receipt_renewals&&
  !native.ownership_idr&&!native.ownership_idrs&&!native.h264_stream_frames&&!frame_count&&
  !native.providers.codec_frame_num&&!native.raw_valid&&!native.produced_nal_crc32_bzip2&&
  !native.producer_begin_ns&&!native.produced_timing.begin&&!native.produced_timing.captured&&
  !native.produced_timing.encoded&&!native.produced_timing.bytes&&!native.produced_timing.keyframe&&
  !native.produced_storage&&!native.resident_storage&&!parked_after_stop&&
  native.encoder_hold==-1&&native.legacy_control==-1&&!native.close_unknown&&native.last_close_attempt==-1&&
  return_receipt_empty(&native.receipt_key)&&return_receipt_empty(&native.receipt_evidence)&&
  !expected.lease_token&&!expected.generation&&!expected.monotonic_ns&&!expected.codec&&
  !expected.keyframe&&!expected.data&&!expected.bytes&&
  !l->initialized&&!l->cpu_capacity_kib&&!l->cpu_paid_kib&&!l->nonmovable_cpu_debt_kib&&
  !l->paid[0]&&!l->paid[1]&&!l->paid[2]&&!l->codec_paid&&!l->raw_reuse&&
  !l->begin[0]&&!l->begin[1]&&!l->begin[2]&&!l->end[0]&&!l->end[1]&&!l->end[2]&&
  !memory_state.count&&!memory_state.session_start&&!memory_state.now&&!memory_state.cancelled&&!memory_state.context&&
  native.watchdog.state==QW_UNKNOWN&&native.watchdog.fd>=3&&!native.watchdog.session_start_ns&&
  !native.watchdog.session_deadline_ns&&!native.watchdog.start_ns&&!native.watchdog.phase&&!native.watchdog.capture_progress;
}
static int return_frame_observe(void)
{
 struct qp_roles *p=&native.providers;
 if(!provider_node(native.continuous_frame,O_RDONLY,p->frame_identity.device,
       p->frame_filesystem,p->frame_inode))return native_guard_failure();
 struct qc_capture_receipt bound=qc_receipt_request(NULL);
 if(ioctl(native.continuous_frame,QC_ISP_RECEIPT_BIND,&bound)||provider_store_frame(&native,&bound))
  return native_guard_failure();
 if(bound.generation||bound.state!=QC_RECEIPT_NONE||bound.reason||bound.stop_result||bound.stopped_ns)
  return native_guard_failure();
 return CAMERA_OK;
}
int nh_native_return_pre_memory_unknown_bootstrap(int *sensor_slot,int *frame_slot,
 int *encoder_slot,int *unknown_watchdog_slot)
{
 /* Do not classify already uncertain ownership as a healthy ineligible return. */
 if(return_uncertain())return native_guard_failure();
 if(!native_initialized||!return_local_history_empty())return CAMERA_NOT_READY;
 if(!sensor_slot||!frame_slot||!encoder_slot||!unknown_watchdog_slot||
    sensor_slot==frame_slot||sensor_slot==encoder_slot||sensor_slot==unknown_watchdog_slot||
    frame_slot==encoder_slot||frame_slot==unknown_watchdog_slot||encoder_slot==unknown_watchdog_slot||
    *sensor_slot!=-1||*frame_slot!=-1||*encoder_slot!=-1||*unknown_watchdog_slot!=-1)return CAMERA_INVALID;
 uint64_t begin=nh_native_now(NULL);
 if(begin==UINT64_MAX||return_boundary(begin)||lifetime())
  return native_guard_failure();
 if(return_boundary(begin))return native_guard_failure();
 native.providers.phase=QP_RETURNING;
 struct qp_roles *p=&native.providers;
 if(!provider_node(native.continuous_sensor,O_RDWR,p->sensor_device,p->sensor_filesystem,p->sensor_inode)||
    return_boundary(begin))return native_guard_failure();
 *sensor_slot=native.continuous_sensor;native.continuous_sensor=-1;
 if(return_uncertain()||return_frame_observe()||return_boundary(begin))
  return native_guard_failure();
 *frame_slot=native.continuous_frame;native.continuous_frame=-1;
 /* All-role observer cannot run after earlier roles moved. Encoder-only observer
  * is the real existing schema/reference check; no dispatch/recipient IO in between. */
 if(return_uncertain()||provider_encoder_observe(&native)||return_boundary(begin))
  return native_guard_failure();
 *encoder_slot=native.h264_stream_fd;native.h264_stream_fd=-1;
 if(return_uncertain()||qw_owner_return_unknown_bootstrap(&native.watchdog,unknown_watchdog_slot,begin))
  return native_guard_failure();
 native.providers.phase=QP_RETURNED; /* terminal; NOT CLOSED or a retirement proof */
 if(return_uncertain())return native_guard_failure(); /* all actual FDs already returned */
 return CAMERA_OK;
}
