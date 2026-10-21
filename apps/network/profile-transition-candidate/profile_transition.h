/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PROFILE_TRANSITION_CANDIDATE_H
#define PROFILE_TRANSITION_CANDIDATE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <camera/access_policy.h>
#include <camera/network_transaction_store.h>
#include "private_profile.h"
#define PT_SOURCE_SYNTHETIC 1U
enum pt_result {PT_OK=0,PT_HOLD=-300,PT_DENIED=-301,PT_STALE=-302,PT_INVALID=-303,PT_BUSY=-304};
enum pt_synthetic_step {PT_SYNTH_ASSOCIATE=1,PT_SYNTH_ADDRESS=2};
/* Local source identities/lease are NOT wire authentication/bearer grants. */
struct pt_lease {uint64_t instance,id;struct camera_network_token operation;};
struct pt_config {
 uint32_t source_class;
 uint64_t instance_id,source_id,scope_id,transition_timeout_ns,rollback_timeout_ns;
 char device_id[CAMERA_ID_BYTES];
 struct pp_handle profiles[2];
 int (*clock_ns)(void *,uint64_t *);void *clock_context;
 /* INTERNAL trusted SYNTHETIC radio owner only; no real fd/driver/network I/O.
  * begin may retain the immutable PRIVATE pointer until exact retired receipt.
  * error is uncertainty, NOT a guarantee no pointer/command was retained. */
 int (*begin)(void *,const struct camera_network_work *,const struct pp_profile *);
 int (*step)(void *,const struct camera_network_work *,const struct pp_profile *,uint32_t);
 int (*retired)(void *,const struct camera_network_work *);
 int (*reconcile)(void *,const struct camera_ntx_image *,const struct camera_ntx_reconciliation *);
 int (*latest)(void *,struct camera_network_link *);
 void *source_context;
 camera_settings_fault_fn storage_fault;void *storage_context;
};
struct pt_report {
 struct camera_ntx_report metadata;
 uint32_t synthetic_class,held_private_copy,source_attempted,logical_work,abort_requested;
};
/* Private/static/nonmovable. Caller serializes policy, source authority, profiles,
 * metadata, callbacks/effects across processes. Zero-initialize fresh storage.
 * Never copy/live-reinitialize or
 * alias args/outputs/context. Borrowed policy/private backend stay allocated and
 * source-owned until command +private-consumer retirement/reconciliation.
 * PRINCIPALS are owner snapshots issued by real policy's trusted auth adapter;
 * tests use PUBLICDUMMY issuer, never actual wire authentication.
 * Only metadata IDs persist; principal/credentials/lease are never serialized.
 * HOST-owned lock below is NOT full native2MiB/process isolation. */
struct profile_transition {
 struct pt_config config;
 struct camera_access_policy *policy;struct pp_session *profiles;
 struct camera_network_transaction_store metadata;
 struct camera_access_principal owner;
 struct pt_lease lease;
 struct camera_network_work work;
 struct pp_profile held; /* SOURCE_PRIVATE: NEVER report/hash/log/copy context */
 uint64_t next_lease,last_ns;
 uint32_t initialized,locked,active,logical_work,held_present,attempted,abort_requested,safety;
 uint32_t metadata_started;
 int extraction_result,metadata_open_result;
};
int pt_open(struct profile_transition *,const struct pt_config *,struct camera_access_policy *,
 const struct camera_access_principal *,struct pp_session *,int metadata_dirfd);
int pt_seed_and_bind(struct profile_transition *,const struct camera_access_principal *,uint64_t initial_profile,
 uint64_t logical_revision,const struct camera_ntx_reconciliation *);
int pt_prepare(struct profile_transition *,const struct camera_access_principal *,const struct camera_network_request *,
 const struct pp_handle *proposed,struct pt_lease *);
int pt_receipt_ack(struct profile_transition *,const struct camera_access_principal *,struct pt_lease);
int pt_dispatch(struct profile_transition *,const struct camera_access_principal *,struct pt_lease,struct camera_network_work *);
int pt_continue(struct profile_transition *,const struct camera_access_principal *,struct pt_lease,const struct camera_network_work *,uint32_t);
int pt_cancel(struct profile_transition *,const struct camera_access_principal *,struct pt_lease);
int pt_frontend_discard(struct profile_transition *,const struct camera_access_principal *,struct pt_lease);
int pt_confirm(struct profile_transition *,const struct camera_access_principal *,struct pt_lease,uint64_t logical_revision);
int pt_query(struct profile_transition *,const struct camera_access_principal *,struct camera_network_token,struct pt_report *);
/* SOURCE-ONLY entry points, not callable from public/admin RPC based on an ID.
 * Identity argument only detects wrong local binding; it does not authenticate.
 * No auth revoke/discard/timeout can free held copy. Source proves actual exact
 * retirement; stale positive rollback cannot retire or publish restoration. */
int pt_source_observe(struct profile_transition *,uint64_t source,struct pt_lease,const struct camera_network_link *);
int pt_source_retire(struct profile_transition *,uint64_t source,struct pt_lease,const struct camera_network_work *,uint32_t success,const struct camera_network_link *);
int pt_source_abort(struct profile_transition *,uint64_t source,struct pt_lease);
int pt_source_rollback(struct profile_transition *,uint64_t source,struct pt_lease,struct camera_network_work *);
int pt_source_continue_rollback(struct profile_transition *,uint64_t source,struct pt_lease,const struct camera_network_work *,uint32_t);
int pt_source_pending(const struct profile_transition *,uint64_t source,struct pt_lease,struct camera_network_work *);
/* Close never interprets query/inspection failure as retirement. Active close
 * requires fresh matching disk revision +published source-known terminal result,
 * no pending/reconciliation/uncertainty. Failure retains initialized/lock/refs.
 * A partial constructor/EMPTY-before-episode is abandoned ONLY via the explicit
 * below API after actual inspection proves no adopted metadata/episode. */
int pt_close(struct profile_transition *);
int pt_abandon_empty(struct profile_transition *);
#ifdef __cplusplus
}
#endif
#endif
