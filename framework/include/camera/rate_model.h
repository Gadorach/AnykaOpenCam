/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RATE_MODEL_H
#define CAMERA_RATE_MODEL_H
#include <camera/backend.h>

#define CAMERA_RATE_MODEL_ABI 1U
#define CAMERA_RATE_NS_PER_SECOND UINT64_C(1000000000)
#define CAMERA_RATE_ASIC_MAX_BYTES 1048576U
#define CAMERA_RATE_HEADER_MAX_BYTES 256U
#define CAMERA_RATE_RAW_BYTES 1382400U
#define CAMERA_RATE_PROFILE_QP 26U

/* ONLY shadow exists. This is neither a codec control nor a CBR capability. */
enum camera_rate_mode {CAMERA_RATE_SHADOW_ONLY=1};
enum camera_rate_picture {CAMERA_RATE_IDR=1,CAMERA_RATE_P=2};
struct camera_rate_config {
 uint32_t abi_version,struct_bytes;
 enum camera_rate_mode mode;
 /* Adapter-assigned identity of one serialized encoder/source/profile owner. */
 uint64_t context_id,source_generation,stream_generation;
 uint64_t target_bps,burst_bits,max_capture_gap_ns,max_completion_lag_ns;
 uint32_t candidate_p_min,candidate_p_max,gop_pictures,max_whole_au_bytes;
};
struct camera_rate_feedback {
 uint64_t model_generation,context_id,source_generation,stream_generation;
 /* Accepted sequence is contiguous from1. Capture sequence strictly increases
  * but may have gaps for uncoded captures. Neither is H.264 frame_num.
  */
 uint64_t accepted_sequence,capture_sequence,capture_ns,encode_start_ns,complete_ns;
 uint32_t raw_input_bytes,asic_bytes,whole_au_bytes;
 enum camera_rate_picture picture;
 /* Adapter facts, not model assertions. All must be1; reported profile must26.
  * No output-full, guard/copy fault, partial AU or failed reference commit.
  */
 uint32_t encode_success,whole_au_complete,reference_committed,profile_qp;
};
struct camera_rate_recommendation {
 enum camera_rate_mode mode;
 uint64_t model_generation,context_id,source_generation,stream_generation;
 uint64_t accepted_sequence,last_capture_sequence,last_capture_ns;
 int64_t debt_bits;
 uint64_t fractional_credit_remainder; /* numerator remainder, <1e9; not bits */
 uint32_t candidate_next_p_qp,candidate_next_idr_qp;
 /* FIXED PROFILE METADATA, never applied-QP readback or a programming request. */
 uint32_t hardware_profile_qp,hardware_qualified_min,hardware_qualified_max;
 uint32_t gop_position,next_gop_idr_due,candidate_at_bound,raising_at_max;
 /* next_gop_idr_due includes first frame of a fresh generation; advisory only.
  * Position counts accepted pictures including IDR1, saturating at GOP bound.
  */
 uint32_t debt_clamped; /* latest update exceeded +/-burst; not HRD overflow */
};
/* Public for static allocation only. Fields private to rate_model.c. Do not
 * copy, mutate or reinitialize a live context. No locks, callbacks, allocations,
 * image processing, clocks, device calls or hardware addresses.
 */
struct camera_rate_model {
 struct camera_rate_config config;
 uint64_t generation,accepted_sequence,last_capture_sequence,last_capture_ns,last_complete_ns;
 uint64_t remainder;
 int64_t debt;
 uint32_t initialized,candidate_p_qp,gop_position,debt_clamped;
};

/* Caller serializes ALL calls. Configuration/input/output storage must not
 * overlap model storage (config may alias model.config for reconfigure).
 * Failure changes neither context nor output. Fresh storage for init only.
 * Nonzero initial generation; anchor is actual capture-time origin, not now,
 * nominal FPS or encode completion. First accepted frame MUST be IDR.
 */
int camera_rate_model_init(struct camera_rate_model *model,const struct camera_rate_config *config,
                          uint64_t generation,uint64_t anchor_capture_ns,uint64_t anchor_capture_sequence);
/* At a retired adapter boundary only. expected must match; generation advances
 * exactly1 without wrapping, resetting debt/remainder/sequence/QP to26. Old
 * feedback becomes stale even if source/stream IDs remain unchanged.
 */
int camera_rate_model_reconfigure(struct camera_rate_model *model,const struct camera_rate_config *config,
                                 uint64_t expected_generation,uint64_t anchor_capture_ns,
                                 uint64_t anchor_capture_sequence);
/* Exactly once COMPLETE ACCEPTED-frame feedback. A refusal does NOT retire a
 * job, repair refs, consume sequence or train time/debt. Adapter must decide
 * recovery/new generation for coded failures; never retry failed evidence asOK.
 * Uncoded/missing captures are not zero-byte frames: next valid timestamp spans
 * their gap. A gap above config bound requires explicit reconfigure/recovery.
 */
int camera_rate_model_complete(struct camera_rate_model *model,const struct camera_rate_feedback *feedback,
                               struct camera_rate_recommendation *out);
int camera_rate_model_snapshot(const struct camera_rate_model *model,struct camera_rate_recommendation *out);
#endif
