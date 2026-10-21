/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_STAGE_TELEMETRY_UAPI_H
#define QC_STAGE_TELEMETRY_UAPI_H
#include <linux/types.h>
#include <linux/ioctl.h>
#define QC_STAGE_VERSION 1U
#define QC_STAGE_CLOCK_MONOTONIC_KTIME_NS 1U
#define QC_STAGE_KINDS 9U
#define QC_STAGE_PHASES 17U
enum qc_stage_kind {
 QC_STAGE_OTHER, QC_STAGE_BEGIN, QC_STAGE_NEXT, QC_STAGE_END,
 QC_STAGE_QUEUE, QC_STAGE_H264, QC_STAGE_JPEG, QC_STAGE_INFO, QC_STAGE_NEXT_IRQ
};
enum qc_stage_phase {
 QC_PHASE_RESET, QC_PHASE_PREPARE, QC_PHASE_PRE_IDLE, QC_PHASE_PROGRAM,
 QC_PHASE_FRAME_WAIT, QC_PHASE_PP_COMMIT, QC_PHASE_PRE_GATE_DRAIN,
 QC_PHASE_POST_GATE_DRAIN, QC_PHASE_GUARDS, QC_PHASE_CRC_PRE,
 QC_PHASE_PIXEL_COPY, QC_PHASE_CRC_POST, QC_PHASE_OUTPUT_COPY,
 QC_PHASE_ALLOC, QC_PHASE_FREE, QC_PHASE_METADATA_COPY, QC_PHASE_RESET_RETIRE
};
#define QC_STAGE_F_SATURATED 1U
#define QC_STAGE_F_CLOCK_ERROR 2U
#define QC_STAGE_F_LEDGER_ERROR 4U
#define QC_STAGE_F_RETIRE_FAILED 8U
/* Query one bounded aggregate + last operation, not a percentile/trace ring.
 * Input: version=1, struct_bytes=sizeof, kind/phase in range, record=0 latest
 * or1 latest failed operation, reserved=0.
 * All remaining fields output. Fixed u32/aligned-u64 ABI; no address input.
 */
struct qc_stage_query {
 __u32 version,struct_bytes,kind,phase,record,clock_source,flags,supported_mask;
 __aligned_u64 samples,failed_samples,total_ns,min_ns,max_ns,work_bytes;
 __aligned_u64 failed_total_ns;
 __aligned_u64 operation_count,operation_ok,operation_failed,delivered;
 __aligned_u64 last_operation,last_owner_generation,last_start_ns,last_end_ns;
 __aligned_u64 last_phase_mask,last_phase_ns,last_phase_bytes;
 __aligned_u64 last_phase_start_ns,last_phase_end_ns;
 __s32 last_result;
 __u32 last_kind,owner_active,last_retirement_failed;
 __u32 reserved;
 __aligned_u64 allocation_attempts,allocation_failures,allocation_successes,free_calls;
 __aligned_u64 live_requested_bytes,peak_requested_bytes,total_requested_bytes,total_freed_bytes;
 __aligned_u64 retire_failures;
 __aligned_u64 owner_generation;
};
#define QC_ISP_STAGE_QUERY _IOWR('I',0x7f,struct qc_stage_query)
#define QC_HWENC_STAGE_QUERY _IOWR('Q',0x65,struct qc_stage_query)
#endif
