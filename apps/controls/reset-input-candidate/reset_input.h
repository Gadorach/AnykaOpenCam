/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RESET_INPUT_CANDIDATE_H
#define RESET_INPUT_CANDIDATE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RESET_INPUT_ABI 1U
#define RI_NS_PER_MS UINT64_C(1000000)
#define RI_DEBOUNCE_MAX_NS UINT64_C(500000000)
#define RI_LONG_MAX_NS UINT64_C(30000000000)
#define RI_STUCK_MAX_NS UINT64_C(60000000000)
#define RI_GAP_MAX_NS UINT64_C(1000000000)
#define RI_EVENT_TTL_MAX_NS UINT64_C(5000000000)
enum ri_result {RI_OK=0,RI_EMPTY=1,RI_UNAVAILABLE=2,RI_INVALID=-1,
 RI_STALE=-2,RI_BUSY=-3,RI_FAULT=-4,RI_EXHAUSTED=-5};
enum ri_provider_result {RI_READ_OK=0,RI_READ_UNAVAILABLE=1,RI_READ_EOF=2,RI_READ_ERROR=3};
enum ri_source_class {RI_INPUT_ONLY_TRUSTED=1,RI_PUBLICDUMMY_INPUT_ONLY=2};
enum ri_presence {RI_INPUT_ABSENT=0,RI_INPUT_PRESENT_UNQUALIFIED=1};
enum ri_logical {RI_RELEASED=0,RI_PRESSED=1,RI_LOGICAL_UNKNOWN=2};
enum ri_validity {RI_OBS_UNKNOWN=0,RI_OBS_VALID_UNQUALIFIED,RI_OBS_STALE,
 RI_OBS_UNAVAILABLE,RI_OBS_ABSENT,RI_OBS_FAULT};
enum ri_state {RI_WAIT_RELEASE=0,RI_ARMED,RI_PRESS_DEBOUNCE,RI_HELD,
 RI_RELEASE_DEBOUNCE,RI_EVENT_PENDING,RI_FAILED_CLOSED,RI_DISPOSED};
enum ri_reason {RI_REASON_NONE=0,RI_REASON_STARTUP,RI_REASON_METADATA,RI_REASON_EOF,
 RI_REASON_SOURCE_CHANGE,RI_REASON_SOURCE_REUSE,RI_REASON_CLOCK,
 RI_REASON_GAP,RI_REASON_CONFLICT,RI_REASON_STUCK,RI_REASON_EXPIRED,
 RI_REASON_CONSUMED,RI_REASON_EXHAUSTED,RI_REASON_PROVIDER};
enum ri_unit {RI_UNIT_LOGICAL_PRESSED=1}; /* NOT a GPIO latch/voltage/polarity. */
enum ri_event_kind {RI_DELIBERATE_LONG_PRESS_RELEASE=1};
enum ri_evidence {RI_PROVIDER_DECLARATION_ONLY=1};
struct ri_config {
 uint64_t debounce_ns,long_press_ns,stuck_cap_ns,fresh_gap_ns,event_ttl_ns;
};
/* Provider output ONLY. Invalid/unknown data is never normalized into RELEASE.
 * Monotonic timestamps share the trusted caller clock; source epochs increase
 * across provider restarts. GPIO polarity normalization belongs to an audited
 * input-only provider, not this unit; do not reuse the stock GPIO0 SET/GET wrapper.
 */
struct ri_observation {
 uint64_t source_epoch,sampled_ns;
 uint32_t logical_pressed,valid,source_class;
};
struct ri_provider {
 uint32_t abi,struct_bytes,presence,source_class;
 void *context;
 int (*read)(void *context,struct ri_observation *observation);
};
struct ri_ticket {uint64_t namespace_epoch,engine_generation,source_epoch,sequence;};
struct ri_event {
 struct ri_ticket ticket;
 uint64_t issued_ns,expires_ns,stable_press_ns,stable_release_ns,held_ns;
 uint32_t kind,evidence,native_qualified;
};
struct ri_view {
 uint64_t source_epoch,sampled_ns,last_received_ns,press_since_ns;
 uint32_t desired_state,logical_observed,observation_validity,logical_unit;
 uint32_t presence,source_class,reason,event_queued,long_threshold_observed;
 uint32_t native_qualified,fault;
};
/* Complete accessible typed storage, zero initialized ONCE. Do not mutate/copy
 * initialized objects or reinitialize their lifetime namespace/generation.
 * Namespace/generation are trusted lifetime-unique nonsecret correlation keys,
 * NOT a PID, identity disclosure, credential or physical-reset permission.
 */
struct reset_input {
 struct reset_input *self;
 struct ri_provider provider;struct ri_config config;struct ri_view view;
 struct ri_event event;struct ri_ticket last_consumed;
 uint64_t namespace_epoch,engine_generation,next_sequence,last_now_ns,
  highest_source_epoch,last_sample_ns,anchor_ns,press_since_ns,last_pressed_ns,raw_held_since_ns;
 uint32_t busy,disposed,have_sample,release_tracking,raw_held_tracking;
};
int reset_input_init(struct reset_input *engine,const struct ri_provider *provider,
 const struct ri_config *config,uint64_t namespace_epoch,uint64_t generation,uint64_t now_ns);
/* OWNER ONLY: invokes exactly one trusted input-only read, no action callback.
 * Wire/admin clients MUST NOT call this, install providers or inject samples.
 * No public feed-observation or pressed=1/reset endpoint exists.
 * Read returns an ALREADY-CAPTURED observation with sampled_ns <= now_ns.
 * The trusted input owner captures/timestamps first, then calls poll with its
 * current monotonic time; this read ABI retrieves that fixed snapshot, not a
 * future timestamp/conversion or blocking device operation. Caller drives polls;
 * provider read must return promptly and never retain output
 * pointers/reenter. A blocked read is not bounded by portable C, nor qualified.
 */
int reset_input_poll(struct reset_input *engine,uint64_t now_ns);
/* Event is only a deliberate logical gesture, not a SETUP transaction/grant.
 * One static queue; peek is non-destructive, consume is exact-once and NEVER
 * executes reset/format/erase/file/network/provisioning. Lost receipt expires,
 * never retries; SETUP later owns protected idempotent settings-only recovery.
 * Null/known-storage alias/wrong receipts are inert. now is trusted monotonic
 * caller time, not UTC or a wire timestamp. Output unchanged on failure.
 */
int reset_input_peek(struct reset_input *engine,uint64_t now_ns,struct ri_event *event);
int reset_input_consume(struct reset_input *engine,const struct ri_event *receipt,uint64_t now_ns);
int reset_input_view(const struct reset_input *engine,struct ri_view *view);
int reset_input_dispose(struct reset_input *engine);
#ifdef __cplusplus
}
#endif
#endif
