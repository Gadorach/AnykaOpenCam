/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_SPAN_SERVER_V1_H
#define CAMERA_RTSP_SPAN_SERVER_V1_H
#include "../qc07f-owned-runtime/server/prestarted_owned_listener_v1.h"
#include "../span-hub-v2/span_hub.h"

/* SOURCE ONLY, new exclusive composition entry/layout, not native admission.
 * Read docs/qualification-plans/qc07f-span-serve-20261024/CONTRACT.md.
 * V3 retention fixes preserved; never link old sender/runtime/hubs too.
 * Sticky/observed clock or ownership faults retain before cleanup; demand stays
 * attached through one detach ACK + valid post-clock, otherwise nonreturn HOLD.
 * Direct rollback and producer-release fences preserve live source ownership on
 * observed uncertainty; they do NOT prove whole-interval clock prevention.
 * Compile callers with this entry and ONLY the new server C + v2 hub C/header;
 * never old server/hubs/combined RELs. Original backend/start/finish/listener
 * types and implementation remain unchanged. No serve-time fork/init/destroy.
 *
 * Trusted serialized, stable, disjoint storage; no mutable aliases, reentry,
 * signals, concurrency, FD duplication/mutation or fork. Caller initializes the
 * actual persistent hub ONCE after LAST verifier fork and required ownership
 * ordering, and independently admits actual chunk backing before entry. ACTIVE
 * pristine/canonical shape checks do NOT prove that ordering, allocation origin,
 * prefault/residency/ledger or memory fit. This entry connects the actual private
 * paid chunk runtime, NOT the old contiguous native ledger. Native use remains
 * unadmitted; do not pass chunks to old void* hub.
 * Caller retains hub/chunks + source/prestarted/config/context/external owner
 * through ALL refusal/return/HOLD paths. No server init/dispose/free/reinit.
 * Legacy options.hub_storage MUST be NULL; other fixed options unchanged.
 * SPS/PPS combined Annex-B parameter storage is stable and bounded to 256 bytes.
 *
 * Sink/context are the persistent real native sink installed by runtime prepare.
 * Sink retains the entire source/media process (including current stack/cursors,
 * hub, demands, native runtime and external media owner), not merely pointers.
 * It must not return/unwind/exit/free/close/END/retry or mutate these objects.
 * No synthetic/noop sink is supplied. A local nonreturning fallback
 * invokes original runtime cleanup and repeatedly SIGSTOPs if sink returns.
 * This retains process-local storage, NOT survival, all-domain recovery or an
 * implementation of external ownership. Original retain_verifier stays mandatory.
 * demand is an actual live record for consumer<4; NULL/index4 denotes whole
 * server/source uncertainty, retaining ALL demands/cursors, not no ownership.
 *
 * Original one-use phase/listener contract applies: BUSY first, invalid required
 * pointers/owner/phase do not touch outcome; READY valid required pointers consume
 * attempt before options/pristine/backend refusals. Before move listener remains
 * caller-owned; after move clear-before-close once only on known source cleanup.
 * Any uncertain ownership is nonreturning before stack/FD cleanup. Clean return
 * requires actual native END/ACK/closes/disable and real producer/client/hub
 * STOP/drain before live pointer retirement; scalar history stays terminal.
 * No disposal; verifier finish and lender/watchdog lifetime stay separate.
 */
#define CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1 CAMERA_SPAN_HUB_V1_CONSUMERS
struct camera_rtsp_span_retention_v1 {
 void *context;
 void (*retain)(void *,const struct camera_span_hub_v1 *,
                const struct camera_producer_demand *,unsigned);
};
struct qc07f_owned_runtime_v1;
/* SAME private runtime owner/source/config/paid hub and sink, never caller-selected
 * alternative callbacks. Early serve refusal still performs actual final runtime
 * retention when custody exists. No serve-time start/finish/fork/init/destroy. */
int qc07f_owned_runtime_span_serve_v1(struct qc07f_owned_runtime_v1 *,
 const struct camera_rtsp_options *,int *listener_fd,
 struct camera_rtsp_owned_listener_outcome_v1 *outcome);
#endif
