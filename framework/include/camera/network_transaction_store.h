/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_NETWORK_TRANSACTION_STORE_H
#define CAMERA_NETWORK_TRANSACTION_STORE_H
#include <camera/settings_store.h>
#include <camera/network_transition.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CAMERA_NTX_SCHEMA 0x4e545801U
#define CAMERA_NTX_WIRE_BYTES 896U
#define CAMERA_NTX_HEADER_BYTES 128U
#define CAMERA_NTX_RECEIPT_BYTES 96U
#define CAMERA_NTX_NONE CAMERA_NETWORK_RESULTS
enum camera_ntx_result {
 CAMERA_NTX_OK=0, CAMERA_NTX_INVALID=-200, CAMERA_NTX_EMPTY=-201,
 CAMERA_NTX_RECONCILE=-202, CAMERA_NTX_STORAGE=-203,
 CAMERA_NTX_CAPACITY=-204, CAMERA_NTX_EXHAUSTED=-205,
 CAMERA_NTX_PROFILE_BLOCKED=-206
};
enum camera_ntx_phase {
 CAMERA_NTX_PREPARED=1, CAMERA_NTX_ACKED, CAMERA_NTX_APPLY_FENCE,
 CAMERA_NTX_WAIT, CAMERA_NTX_ROLLBACK_FENCE, CAMERA_NTX_COMMIT_FENCE,
 CAMERA_NTX_FINAL, CAMERA_NTX_RECONCILED
};
enum camera_ntx_flags {CAMERA_NTX_RECEIPT_ACK=256U};
enum camera_ntx_choice {CAMERA_NTX_CURRENT=1, CAMERA_NTX_PREVIOUS=2};
#define CAMERA_NTX_REASON_RESTART 16U
/* Compact PUBLIC metadata only. No serialized C structs, pointers, auth grants,
 * credentials, payload hashes, observed association facts or wall/boot clocks. */
struct camera_ntx_receipt {
 struct camera_network_request request;
 uint64_t old_profile_id,apply_generation,rollback_generation;
 uint64_t result_profile_id,result_revision;
 uint32_t state,reason,rollback_reason,phase_flags;
};
struct camera_ntx_image {
 uint64_t scope_id,namespace_high_water,operation_high_water,profile_high_water;
 uint64_t radio_high_water,committed_profile_id,committed_revision;
 uint32_t count,active_index;
 struct camera_ntx_receipt receipts[CAMERA_NETWORK_RESULTS];
};
/* Chosen by trusted source owner, NOT generated from disk. Reconciliation must
 * establish all prior owner's work/callbacks really retired, a freshly ordered
 * exact radio instance and private profile durability/unseal. This primitive
 * cannot qualify those facts. No boolean in persisted metadata grants authority.
 * PREVIOUS is allowed only at COMMIT_FENCE; it writes a NEW logical revision. */
struct camera_ntx_reconciliation {
 uint64_t namespace_id,radio_generation,profile_id;
 struct camera_network_token prior_token;
 uint64_t prior_apply_generation,prior_rollback_generation;
 uint32_t choice;
};
struct camera_ntx_config {
 uint64_t scope_id,transition_timeout_ns,rollback_timeout_ns;
 /* Pure trusted allowlist: OK asserts ID is a nonsecret profile HANDLE, not
  * SSID/PSK/password/HA1/key/bootstrap bytes or their hashes packed as integers.
  * Invoked before settings_store CRC/write and while validating disk records. */
 int (*public_profile_id)(void *,uint64_t);
 void *profile_context;
 /* Trusted fresh monotonic clock, sampled before/after storage effects. */
 int (*clock_ns)(void *,uint64_t *);
 void *clock_context;
 int (*reconcile)(void *,const struct camera_ntx_image *,
                  const struct camera_ntx_reconciliation *);
 void *reconcile_context;
 /* Reconcile proof is checked on BOTH sides of epoch persistence, against the
  * original prior image. Owner/retirement/observer/private-profile protection
  * must remain valid through that interval; numeric IDs never establish facts. */
 /* Pure admission, not a driver call. Requires private profiles durable/unsealed,
  * authenticated admin/setup authority and actual adapter ownership/capability.
  * Called at both sides of dispatch persistence for the requested effect AND
  * the old-profile rollback plan before APPLY/SCAN. Missing readiness blocks effects.
  * Purity is NOT zero latency: clock is sampled after the final admission call,
  * exact tuple/deadline rechecked, and expiry cannot select unfenced new work.
  * ROLLBACK_PENDING has a bounded admission window of rollback_timeout_ns from
  * selection, separate from core's unchanged timeout starting at effect dispatch. */
 int (*effect_ready)(void *,uint32_t,uint64_t,uint64_t);
 void *effect_context;
 /* Pure source-owned latest OBSERVED link snapshot, sampled after commit-fence
  * fsync to cover notifications queued during blocking storage. Expected IDs
  * select the observer; NEVER echo them as invented facts. Missing/stale/loss
  * blocks confirmation and requires prior/future reconciliation. */
 int (*latest_link)(void *,struct camera_network_token,uint64_t,uint64_t,
                    struct camera_network_link *);
 void *latest_context;
};
struct camera_ntx_inspection {
 int settings_result;
 uint32_t current_valid,pending_valid,hardware_applied_known,durability;
 uint64_t settings_revision,settings_operation_id;
 struct camera_ntx_image current,pending;
};
struct camera_ntx_report {
 struct camera_ntx_receipt receipt;
 uint64_t settings_revision;
 /* in_flight is this process's command tracking only. After reboot zero is NOT
  * a retirement receipt; work_may_be_live conservatively marks unresolved records. */
 uint32_t runtime_bound,reconciliation_required,in_flight,volatile_state,work_may_be_live;
 /* receipt.state is stored intent/history, NOT publication. This is nonzero
  * only for a final receipt behind this process's confirmed sync/reconcile fence.
  * A CRC-valid FINAL reopened after a crash still has published_state==0. */
 uint32_t published_state;
 /* Query/replay ALWAYS return NOT_CONFIRMED and hardware unknown. Phase FINAL
  * is historical result metadata, not a fresh sync or live-link certificate. */
 uint32_t durability,hardware_applied_known;
 int last_settings_result;
};
/* Static, caller-owned, nonmovable, private fields. Serialize all calls/effects
 * across processes. Borrow only a dedicated trusted 0700 dirfd containing the
 * existing settings.current/settings.pending files (0600); no path selection,
 * mkdir/unlink/prune/mount or driver I/O here. Input/output objects MUST NOT
 * overlap each other/context; no safe overlap rejection is promised. Callbacks
 * may not reenter/mutate this context or files. Providers must have source-owned,
 * qualified bounded execution; this API does not impose a numeric latency cap or
 * preempt a callback that never returns. Fresh clock checks only elapsed time,
 * not physical freshness: caller holds proof/ownership protections through fences
 * and rechecks rights/handles/time before actual external effects. Do not open over a live context.
 * Eight receipts retained forever in this bounded subset; full means CAPACITY,
 * no implicit eviction. Reboot never restores core/live work from disk. */
struct camera_network_transaction_store {
 struct camera_ntx_config config;
 struct camera_settings_store settings;
 struct camera_settings_report scratch;
 struct camera_network_transition core;
 struct camera_ntx_image image;
 struct camera_network_token runtime_token;
 struct camera_network_work live_work;
 uint64_t settings_revision,settings_operation_id,last_ns;
 uint32_t initialized,loaded,blocked,bound,live_present,uncertain;
 int last_settings_result;
};
/* Strict codec/validator: fixed LE version1/896 bytes; rejects any extra fields,
 * unknown schema/version, secret-bearing payloads and unsupported handles BEFORE
 * hashing/writing. The only encoder accepts typed public metadata. */
int camera_ntx_encode(const struct camera_ntx_config *,const struct camera_ntx_image *,
 unsigned char out[CAMERA_NTX_WIRE_BYTES]);
int camera_ntx_decode(const struct camera_ntx_config *,const unsigned char *,uint32_t,
 struct camera_ntx_image *);
int camera_ntx_validate(void *,uint32_t,uint32_t,const unsigned char *,uint32_t);
int camera_network_transaction_open(struct camera_network_transaction_store *,int,
 const struct camera_ntx_config *,camera_settings_fault_fn,void *);
/* Explicit empty-store seed from qualified owner-provided public handle/revision;
 * no defaults. Still RECONCILE-only until trusted owner/radio proof binds an epoch. */
int camera_network_transaction_seed(struct camera_network_transaction_store *,uint64_t,uint64_t);
int camera_network_transaction_inspect(struct camera_network_transaction_store *,struct camera_ntx_inspection *);
/* Read-only inspect never promotes/resumes/repairs pending data. Reconcile is
 * allowed only with a valid current record and NO pending/uncertain state; no
 * automatic repair. Never rebind this process while its core work is live. */
int camera_network_transaction_reconcile(struct camera_network_transaction_store *,
 const struct camera_ntx_reconciliation *);
int camera_network_transaction_prepare(struct camera_network_transaction_store *,const struct camera_network_request *);
int camera_network_transaction_receipt_ack(struct camera_network_transaction_store *,struct camera_network_token);
int camera_network_transaction_next(struct camera_network_transaction_store *,struct camera_network_work *);
int camera_network_transaction_link(struct camera_network_transaction_store *,const struct camera_network_link *);
int camera_network_transaction_complete(struct camera_network_transaction_store *,const struct camera_network_work *,uint32_t,const struct camera_network_link *);
int camera_network_transaction_cancel(struct camera_network_transaction_store *,struct camera_network_token);
int camera_network_transaction_tick(struct camera_network_transaction_store *);
/* COMMIT_FENCE durable desired snapshot -> fresh clock/revision/core confirm ->
 * durable FINAL. Until both barriers succeed no confirmed result is published.
 * Failure/expiry between barriers requires explicit prior/future reconciliation,
 * retains old/proposed handles and never retries writes/effects automatically. */
int camera_network_transaction_confirm(struct camera_network_transaction_store *,struct camera_network_token,uint64_t);
int camera_network_transaction_query(const struct camera_network_transaction_store *,struct camera_network_token,struct camera_ntx_report *);
int camera_network_transaction_live(const struct camera_network_transaction_store *,struct camera_network_work *);
/* Settings reset intentionally absent: preserve firmware/recovery/identity/
 * calibration/ALL footage. No fallback/open AP, private-profile reconstruction,
 * authorization from a token, native SD/WLAN or durable secret qualification. */
#ifdef __cplusplus
}
#endif
#endif
