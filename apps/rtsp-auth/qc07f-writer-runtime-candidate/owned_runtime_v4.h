/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_WRITER_RUNTIME_V4_H
#define QC07F_WRITER_RUNTIME_V4_H
/* Complete replacement definitions BEFORE all original native/server imports. */
#include "../qc07f-producer-memory/handoff.h"
#include "../qc07f-producer-memory/memory_runtime.h"
#include "../qc07f-owned-runtime/server/prestarted_owned_listener_v1.h"
#include "../qc07f-owned-runtime/native/native_watchdog.h"
#include "../qc07f-owned-runtime/native/owned-provider.h"
#include "../qc07f-owned-runtime/native/provider-return.h"
#include "../../qualification/qc07f-encoder-output-spans/encoder-output-spans.h"
#define QC07F_OWNED_RUNTIME_SOURCE_API 4U

/* REAL scalar output, no payload pointers and no camera_packet surrogate. */
struct qc07f_writer_loan_v4 {
 uint64_t token,generation,monotonic_ns;
 struct nh_key key;
 uint32_t written_bytes,prefix_bytes,keyframe;
};
struct qc07f_writer_source_v4 {
 void *context;
 int (*next)(void *,struct camera_rtsp_frame_handle,uint32_t,struct qc07f_writer_loan_v4 *);
 int (*release)(void *,struct camera_rtsp_frame_handle,const struct qc07f_writer_loan_v4 *);
 const uint8_t *parameter_sets;size_t parameter_set_bytes;
 const struct camera_rtsp_lifecycle *lifecycle;
};
enum qc07f_writer_phase_v4 {QCW_IDLE,QCW_ATTEMPT,QCW_RESERVED,QCW_ARMED,
 QCW_LOAN,QCW_RELEASING,QCW_RELEASED,QCW_ENDED,QCW_HOLD};
/* Allocation layout ONLY, private after prepare. Static zero storage at final
 * address. Serialized trusted C; no edits/copies/aliases/reentry/signals/threads/
 * later fork/FD-table mutation. See CONTRACT.md. This is NOT an old frontend ABI.
 * Source, config, actual child/native owner and ALL backing survive HOLD forever
 * until independently admitted recovery; no destructor or reset API. */
struct qc07f_owned_runtime_v4 {
 const struct qc07f_owned_runtime_v4 *self;
 const struct candidate_config *config;
 const struct camera_graph *graph;
 struct candidate_prestarted *owner;void *bootstrap;
 unsigned bound,retired,post_first,span_setup_attempt,span_native_retire;
 int error;int (*gate)(void *);void *gate_context;
 struct camera_producer producer;
 struct camera_rtsp_common_producer bridge;
 struct camera_rtsp_lifecycle lifecycle;
 struct qc07f_writer_source_v4 source;
 struct camera_span_hub_v1 *span_hub;
 enum qc07f_writer_phase_v4 pipeline;
 uint64_t attempts,last_token,last_generation;
 unsigned arm_attempt,ioctl_attempt,release_attempt,end_attempt,commit_attempt;
 struct camera_span_hub_reservation_v3 reservation;
 struct camera_span_hub_writer_v3 writer;
 struct qc_hwenc_h264_spans_job request,request_expected;
 struct camera_span_hub_result_v3 result;
 struct qc07f_writer_loan_v4 loan;
 struct camera_rtsp_frame_handle frame;
 uint64_t operation_begin_ns;
};
int qc07f_owned_runtime_prepare_v4(struct qc07f_owned_runtime_v4 *,
 struct candidate_config *,struct candidate_prestarted *,void *);
int qc07f_owned_runtime_bind_postfork_v4(struct qc07f_owned_runtime_v4 *);
int qc07f_owned_runtime_memory_span_hub_v4(struct qc07f_owned_runtime_v4 *,struct camera_span_hub_v1 *);
void qc07f_owned_runtime_cleanup_v4(void *,int,unsigned);
/* Setup remains the actual one-owner API, AFTER last fork/bind. */
void nh_native_init(void);
int nh_native_optin_exact139(void);
int nh_native_storage_select(const char *);
int nh_native_memory_start(uint64_t);
int nh_native_memory_fixed_buffers(void *); /* permanent no-effect refusal */
int nh_native_memory_gate(void *,enum nh_memory_stage);
void nh_native_memory_report(void);
void nh_native_sensor_report(void);
uint64_t nh_native_now(void *);
int nh_native_cancelled(void *);
int nh_native_config(struct camera_fmp4_config *);
void nh_native_cancel(int);
void nh_native_fence(void *);
_Noreturn void nh_native_quarantine(void);
/* Removed old producer route: retained names return NOT_READY before effects. */
int nh_native_next(void *,struct camera_packet *,struct nh_key *);
int nh_native_next_keyframe(void *,uint32_t,struct camera_packet *,struct nh_key *);
int nh_native_release(void *,const struct camera_packet *);
int nh_native_retire_held(void *,const struct camera_packet *);
int nh_native_retire(void *);
/* NO serve/main or borrow-end export. Future typed frontend must supply its REAL
 * source/common/client/hub drain tail before the persistent CPU borrow may end. */
#endif
