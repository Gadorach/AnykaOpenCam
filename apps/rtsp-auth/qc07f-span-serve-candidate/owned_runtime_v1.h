/* SPDX-License-Identifier: GPL-2.0-only
 * Explicit source replacement, NEW layout, not the original binary ABI.
 */
#ifndef QC07F_OWNED_RUNTIME_V1_H
#define QC07F_OWNED_RUNTIME_V1_H
#include "../qc07f-span-runtime-candidate/native/native_adapter.h"
#include "rtsp_span_server_v1.h"
#include "../qc07f-owned-runtime/server/prestarted_owned_listener_v1.h"
#include "../qc07f-owned-runtime/native/native_watchdog.h"
#include "../qc07f-owned-runtime/native/owned-provider.h"
#include "../qc07f-owned-runtime/native/provider-return.h"
#define QC07F_OWNED_RUNTIME_SOURCE_API 3U

/* Static fresh zero storage only. Public for allocation, private after prepare.
 * One singleton/native provider + common producer, NOT one per RTSP peer.
 * All calls, outputs, FD slots and callbacks serialized, stable, disjoint,
 * nonreentrant; no signals/threads/concurrent FD mutation, fork/dup/SCM aliases.
 * This is trusted C, not a plugin ABI/concurrency/hostile-memory guarantee.
 * Caller callbacks configured BEFORE prepare accept this context; bootstrap
 * points to caller-owned stable device-free context (no native userdata yet).
 */
struct qc07f_owned_runtime_v1 {
 const struct qc07f_owned_runtime_v1 *self;
 const struct candidate_config *config;
 const struct camera_graph *graph;
 struct candidate_prestarted *owner;
 void *bootstrap;
 unsigned bound,loan_present,retired,post_first;
 int error;
 int (*gate)(void *);void *gate_context;
 struct camera_producer producer;
 struct camera_rtsp_common_producer bridge;
 struct camera_rtsp_lifecycle lifecycle;
 struct camera_rtsp_producer source;
 struct camera_packet packet;
 struct nh_key key; /* real native capture identity, not a common/hub token */
 /* Set ONLY by the actual typed call, never cleared on failure or source END.
  * Borrowed caller CPU owner, NOT native.resident_storage or a free authority.
  * Only the integrated normal server tail retires these LIVE pointers. */
 struct camera_span_hub_v1 *span_hub;
 unsigned span_setup_attempt;
 /* Scalar one-use history survives normal pointer retirement. Stage1 precedes
  * actual native STOP; stage2 follows its ACK, never a caller success flag.
  * Output integers are captured before the unchanged native output free. */
 unsigned span_native_retire,span_relinquished;
 uintptr_t span_output_begin,span_output_end,span_owner_retired;
 struct camera_rtsp_span_retention_v1 span_sink; /* actual persistent typed sink */
};

/* BEFORE candidate_prestarted_start: device-free addresses/references only.
 * cfg's real random_ready/retain_verifier, routing/graph/image/material policy
 * must already be supplied by trusted bootstrap; no dummies installed here.
 * Replaces only context, source_ready/guard/uncertain callbacks. Other caller
 * callbacks must accept this runtime address and use ->bootstrap if needed.
 * No native init/open/adopt/memory/sensor/clock/guard operation or readiness.
 */
int qc07f_owned_runtime_prepare_v1(struct qc07f_owned_runtime_v1 *,
 struct candidate_config *,struct candidate_prestarted *,void *bootstrap);

/* AFTER successful start and the LAST fork/exec: explicit actual one-time native
 * init + common producer/parameter-set binding, no device effects. Config,
 * backend, peers, graph and callback/context ADDRESSES must remain immutable.
 * Actual sole-provider/watchdog/backing setup follows via original nh_* APIs,
 * in original caller order; source-ready never performs it. No retry/reinit.
 * Independent admitted bootstrap, external media-process owner, stable backing
 * and all-domain recovery are mandatory TRUSTED prerequisites, NOT supplied by
 * this function or proved by READY metadata. No later fork, including in serve.
 * All setup failures after custody/work require caller retention; no destructor.
 */
int qc07f_owned_runtime_bind_postfork_v1(struct qc07f_owned_runtime_v1 *);

/* AFTER that bind, watchdog take, storage/exact139, provider take and original
 * one-shot memory_start. True pristine accepted v2 hub at its final address,
 * all8 canonical chunks private writable/stable/disjoint, caller initialized
 * AFTER last fork. No publish/subscribe/mutation/callback aliases during setup.
 * Exactly one admitted borrow/setup; actual raw prefault then WHOLE hub helper.
 * Pre-borrow no-effect refusal may return; every later failure quarantines the
 * actual process without unwind. All owner/backing/config/runtime addresses
 * survive every outcome. Success is source history, not native fit/readiness.
 * No ownership move/init/allocation/disposal/retry is provided here. ONLY the
 * composed serve's actual known-normal tail may relinquish the CPU borrow.
 */
int qc07f_owned_runtime_memory_span_hub_v1(struct qc07f_owned_runtime_v1 *,
 struct camera_span_hub_v1 *);

/* Internal fresh-server cleanup boundary. Not an owner/retirement API. On native
 * uncertainty/clock uncertainty with retained work, or final unretired work,
 * enters ACTUAL nonreturning native quarantine BEFORE stack/FD cleanup. Does not
 * stop DMA, guarantee survival/recovery, feed/rearm/END/reset/kill/resume/close.
 * Known pre-effect refusals can return. No public RTSP contract is relaxed.
 */
void qc07f_owned_runtime_cleanup_v1(void *,int clock_failed,unsigned final);

/* Internal typed composition checks, NOT general owner/retirement permissions.
 * End is called ONLY from the real server tail after its actual client drain,
 * same-hub STOP/drain/token checks and successful common retirement. It verifies
 * private native/ledger facts and exact still-bound gate. No IO/callback/retry.
 * Failure after borrow quarantines. Success retires pointers, NEVER history,
 * lender/native/watchdog lifetime or caller hub storage. */
int qc07f_owned_runtime_span_entry_v1(struct qc07f_owned_runtime_v1 *);
void qc07f_owned_runtime_span_end_borrow_v1(struct qc07f_owned_runtime_v1 *,
 struct camera_span_hub_v1 *,int (*gate)(void *),void *gate_context);
#endif
