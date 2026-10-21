/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_NETWORK_TRANSITION_H
#define CAMERA_NETWORK_TRANSITION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CAMERA_NETWORK_RESULTS 8U
enum camera_network_result {
 CAMERA_NETWORK_OK=0, CAMERA_NETWORK_INVALID=-1, CAMERA_NETWORK_STALE=-2,
 CAMERA_NETWORK_CONFLICT=-3, CAMERA_NETWORK_BUSY=-4, CAMERA_NETWORK_CAPACITY=-5,
 CAMERA_NETWORK_EXHAUSTED=-6, CAMERA_NETWORK_ORDER=-7,
 CAMERA_NETWORK_CLOCK=-8, CAMERA_NETWORK_NOT_READY=-9
};
enum camera_network_kind {CAMERA_NETWORK_SWITCH=1, CAMERA_NETWORK_SCAN=2};
enum camera_network_state {
 CAMERA_NETWORK_PREPARED=1, CAMERA_NETWORK_APPLYING, CAMERA_NETWORK_WAIT_LINK,
 CAMERA_NETWORK_CONFIRM_READY, CAMERA_NETWORK_ROLLBACK_PENDING,
 CAMERA_NETWORK_ROLLING_BACK, CAMERA_NETWORK_CANCEL_WAIT,
 CAMERA_NETWORK_CONFIRMED, CAMERA_NETWORK_RESTORED, CAMERA_NETWORK_FAILED,
 CAMERA_NETWORK_CANCELLED
};
enum camera_network_reason {
 CAMERA_NETWORK_REASON_NONE, CAMERA_NETWORK_REASON_APPLY,
 CAMERA_NETWORK_REASON_TIMEOUT, CAMERA_NETWORK_REASON_CANCEL,
 CAMERA_NETWORK_REASON_CLOCK, CAMERA_NETWORK_REASON_SCAN,
 CAMERA_NETWORK_REASON_REVISION, CAMERA_NETWORK_REASON_ROLLBACK
};
enum camera_network_effect {
 CAMERA_NETWORK_APPLY=1, CAMERA_NETWORK_DISRUPTIVE_SCAN, CAMERA_NETWORK_ROLLBACK
};
enum camera_network_facts {
 CAMERA_NETWORK_AUTH_ASSOC=1U, CAMERA_NETWORK_ADDRESS=2U,
 CAMERA_NETWORK_LOCAL_REACHABLE=4U, CAMERA_NETWORK_ROUTE=8U
};
/* Namespace is a caller-supplied nonsecret instance identifier, NOT authority.
 * It must not be shared across contexts or reused on restart/reinitialization.
 * IDs strictly increase in
 * that namespace. Caller retains high-water marks outside this volatile model. */
struct camera_network_token {uint64_t namespace_id,id;};
struct camera_network_request {
 struct camera_network_token token;
 uint64_t expected_revision,config_id;
 uint32_t kind,required_facts;
};
struct camera_network_work {
 struct camera_network_token token;
 uint64_t config_id,radio_generation,deadline_ns;
 uint32_t effect;
};
/* Trusted adapter facts, not an untrusted HTTP client's assertion. Association
 * must be authenticated under the exact private profile instance. Address must
 * be usable under the requested IP/address-family policy (incl. conflict checks
 * where supported). LOCAL_REACHABLE does not require Internet/cloud or UTC.
 * No WPA/AP/captive/TLS/clock qualification is inferred from these bits. */
struct camera_network_link {
 struct camera_network_token token;
 /* Nonzero, nonwrapping sequence assigned when facts are OBSERVED, not when
  * delivered; strictly newer observations supersede earlier ones in an instance.
  * Caller preserves this ordering across all trusted link observers. */
 uint64_t config_id,radio_generation,observation_id;
 uint32_t facts;
};
struct camera_network_report {
 struct camera_network_request request;
 uint64_t old_config_id,old_revision,apply_generation,rollback_generation;
 uint64_t deadline_ns,committed_config_id,committed_revision,link_observation_id;
 /* link_facts is the latest raw scalar observation, including invalid unknown
  * bits; it is NOT readiness/restoration. Its instance is rollback_generation
  * while ROLLING_BACK/RESTORED, apply_generation while WAIT_LINK/CONFIRM_READY. */
 uint32_t state,reason,receipt_acknowledged,in_flight,link_facts;
 uint32_t old_link_known,scan_completed,scan_succeeded;
 uint32_t rollback_completed,rollback_succeeded,rollback_reason;
 /* CONFIRMED is scalar acceptance only, NEVER durable storage evidence. */
};
struct camera_network_config {
 uint64_t namespace_id,operation_high_water,radio_high_water;
 uint64_t committed_config_id,committed_revision;
 uint64_t transition_timeout_ns,rollback_timeout_ns;
};
/* Static allocation only; fields private. No allocation, I/O, driver calls,
 * secret buffers, timers, automatic retries or fallback AP. Caller serializes
 * every call and actual effect across all clients/processes. Public input/output
 * structs must not overlap machine storage or one another; this is a caller
 * precondition, NOT dynamically checked or promised safe rejection. In particular,
 * do not init from &s->config, prepare from &s->results[i].request, next into
 * &s->live, query into &s->results[i], or complete from &s->live: memset/copy/state
 * updates can overwrite aliased inputs/outputs before consumption. Fresh init only;
 * never copy/reinitialize while work/callbacks are live. Scalar public queries
 * carry no credentials, secret hashes, SSIDs, addresses or device pointers. */
struct camera_network_transition {
 struct camera_network_config config;
 uint64_t last_ns,operation_high_water,radio_high_water,config_high_water;
 uint32_t initialized,clock_fault,link_known,used[CAMERA_NETWORK_RESULTS];
 struct camera_network_report results[CAMERA_NETWORK_RESULTS];
 struct camera_network_work live;
 uint32_t active_slot,live_present;
};
int camera_network_init(struct camera_network_transition *,
 const struct camera_network_config *,uint64_t now_ns);
/* Caller has privately validated configuration/capabilities/authorization and
 * protected old configuration plus pending/rollback metadata BEFORE prepare.
 * SWITCH uses a strictly increasing opaque config instance (even failed IDs
 * are consumed); SCAN uses the committed instance. IDs are NOT payload hashes.
 * One active operation; BUSY never cancels/replaces it. Any retained exact
 * request replay retrieves state without action; mismatched replay conflicts.
 * Terminal replay is read-only and ignores now; pending replay advances timeout.
 * Mandatory AUTH_ASSOC|ADDRESS; optional route/local reachability only. */
int camera_network_prepare(struct camera_network_transition *,
 const struct camera_network_request *,uint64_t now_ns);
/* This is delivery ACK, not queued/written HTTP response. Caller establishes
 * that the initiating client received reconnect/result instructions. No effect
 * can be dispatched before this exact-token ACK. Lost receipt expires safely. */
int camera_network_receipt_ack(struct camera_network_transition *,
 struct camera_network_token,uint64_t now_ns);
/* At most one effect, exactly once. Adapter owns service quiesce, private
 * profile, radio, callbacks and retirement. Inspect live logical abort reason
 * via query; cancellation/timeout cannot retire or release live resources. */
int camera_network_next(struct camera_network_transition *,uint64_t now_ns,
 struct camera_network_work *);
/* Completion asserts this exact asynchronous command/effect and its completion
 * callbacks are retired; the resulting radio instance remains owned by the
 * adapter and observed by separate link notifications until rollback/reconcile.
 * success is 0/1; rollback success additionally requires exact link proof.
 * That proof must be the latest observed rollback-instance sequence with identical
 * raw facts, or a strictly newer positive observation. Older/conflicting same-ID
 * proof is STALE before retirement; live work remains. Explicit success=0 after
 * actual command retirement ignores rollback_proof and permits FAILED retirement.
 * A newer exact-instance loss/unknown report supplied as success proof is retained
 * as an observation but returns STALE (CLOCK on regression), without retirement.
 * Late completion can retire work but NEVER confirm a late/stale link. */
int camera_network_complete(struct camera_network_transition *,
 const struct camera_network_work *,uint32_t success,
 const struct camera_network_link *rollback_proof,uint64_t now_ns);
/* An exact-instance callback missing required facts revokes earlier readiness
 * and returns NOT_READY; unknown fact bits revoke readiness and return INVALID.
 * Also tracks the old-config/rollback-generation instance during live ROLLING_BACK
 * without changing lifecycle/in_flight/old_link_known. Even positive observation
 * is NOT restoration; timeout/clock fault denies readiness without freeing work.
 * Stale instances/sequences cannot revoke newer evidence or alter the clock. */
int camera_network_link_proof(struct camera_network_transition *,
 const struct camera_network_link *,uint64_t now_ns);
/* Explicit scalar commit decision after link proof, rechecking revision.
 * Does not call settings_store or write anything. Parent must design durable
 * prepare/commit/reboot reconciliation before integrating actual persistence. */
int camera_network_confirm(struct camera_network_transition *,
 struct camera_network_token,uint64_t observed_revision,uint64_t now_ns);
int camera_network_cancel(struct camera_network_transition *,
 struct camera_network_token,uint64_t now_ns);
/* now==deadline expires; equal successive timestamps allowed. Regression
 * latches a fault, denies new operations, requests rollback and retains live
 * work. Caller can retire/recover with times >= last accepted time; no fresh
 * operation until supervised reconciliation/new namespace outside this API. */
int camera_network_tick(struct camera_network_transition *,uint64_t now_ns);
int camera_network_query(const struct camera_network_transition *,
 struct camera_network_token,struct camera_network_report *);
/* Explicit terminal-result retirement; never evicts live/pending work. Old IDs
 * remain forbidden after forgetting. Reboot result retention is unimplemented. */
int camera_network_forget(struct camera_network_transition *,struct camera_network_token);
/* Settings reset is deliberately absent. Integration MUST preserve firmware,
 * recovery, identity, physical calibration and ALL footage; no erase/format or
 * shared/default admin credentials are authorized by this primitive. */
#ifdef __cplusplus
}
#endif
#endif
