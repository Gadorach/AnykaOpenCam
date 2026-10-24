/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_WRITER_SERVER_V5_H
#define QC07F_WRITER_SERVER_V5_H
#include "../qc07f-producer-memory/handoff.h"
#include "../qc07f-owned-runtime/server/prestarted_owned_listener_v1.h"
#include "../span-hub-producer-candidate/span_hub.h"

/* EXCLUSIVE SOURCE_API5 replacement, not a binary ABI or execution admission.
 * Rebuild ALL consumers; never old runtime/server/memory/hub/combined RELs.
 * Only actual prepared/bound runtime/owner/config/source/lifecycle/paid hub3 and
 * real persistent nonreturning sink. No camera_packet compatibility or alternate
 * caller-selected source. Actual release/exact runtime END0/commit precede all
 * consumer acquisitions/packetization/sends; no copy/recommit/fake error END.
 * Read docs/qualification-plans/qc07f-writer-serve-20261024/CONTRACT.md.
 *
 * True-fresh final-address private disjoint persistent objects/backing; serialized
 * calls, immutable callbacks/config, no aliases/edits/copies/reentry/signals/
 * threads/concurrency/later fork or FD mutation. One real child start completes
 * BEFORE native/provider/watchdog/backing/listener setup. This entry never forks,
 * initializes/destroys a backend or supplies provisioning/backing/recovery/main.
 * Real crypto/auth/random policy is unchanged. <=256B stable SPS/PPS, original
 * options/loopback/listener custody and fixed2s/10s/128-frame limits remain.
 * Shape/tokens/metadata are not origin/residency/physical-safety/strict-C1 proof.
 *
 * BUSY first/no-touch. Invalid required pointers/owner/phase do not touch outcome;
 * valid READY required records consume one-use owner attempt before options or
 * backend refusal. Before move caller owns listener; after move one actual
 * clear-before-close on known cleanup. Errors never clean SERVED. After custody,
 * ordinary pre-entry/options/backend refusal still requires actual final runtime
 * retention, not destructor/unwind. Invalid identity is not destruction authority.
 *
 * Uncertainty retains CURRENT actual native/writer/hub/demands/cursors/stack and
 * entire media process BEFORE cleanup. Sink is installed by prepare, never a noop;
 * it must not return/unwind/exit/free/close/retry/END. Fallback actual cleanup plus
 * repeated SIGSTOP remains if it returns. demand=NULL/index4 means whole process,
 * not no owner. No old-hub cast/NULL surrogate enters retain_uncertain.
 *
 * V3 retain-before-cleanup and detach0 plus valid post-clock fences remain. Safe
 * post-publication auth/socket/deadline failures release exact consumer tokens
 * and close partial connections. Known normal return additionally requires actual
 * native END/query/ACK/closes/disable, all clients/common/hub STOP/drain and exact
 * still-bound gate. Only that tail retires live hub pointers, preserving terminal
 * scalar history. No hub free/watchdog giveback/verifier finish/material retirement
 * or survival/recovery claim. Caller storage survives ALL uncertain outcomes.
 */
#define CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1 CAMERA_SPAN_HUB_V1_CONSUMERS
struct camera_rtsp_span_retention_v1 {
 void *context;
 void (*retain)(void *,const struct camera_span_hub_v1 *,
                const struct camera_producer_demand *,unsigned);
};
struct qc07f_owned_runtime_v4;
int qc07f_owned_runtime_writer_serve_v5(struct qc07f_owned_runtime_v4 *,
 const struct camera_rtsp_options *,int *,
 struct camera_rtsp_owned_listener_outcome_v1 *);
#endif
