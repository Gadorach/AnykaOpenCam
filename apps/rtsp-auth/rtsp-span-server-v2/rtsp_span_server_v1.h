/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_SPAN_SERVER_V1_H
#define CAMERA_RTSP_SPAN_SERVER_V1_H
#include "../qc07f-owned-runtime/server/prestarted_owned_listener_v1.h"
#include "../span-hub-v2/span_hub.h"

/* SOURCE ONLY, not drop-in binary ABI/native admission. Read CONTRACT.md in
 * docs/qualification-plans/rtsp-span-server-v2-20261024 before any use.
 * V2 R1/R2 source replacement: SAME exports/types/guard, never link v1 too.
 * Sticky/observed clock or ownership faults retain before cleanup; demand stays
 * attached through one detach ACK + valid post-clock, otherwise nonreturn HOLD.
 * Compile callers with this entry and ONLY the new server C + v2 hub C/header;
 * never old server/hubs/combined RELs. Original backend/start/finish/listener
 * types and implementation remain unchanged. No serve-time fork/init/destroy.
 *
 * Trusted serialized, stable, disjoint storage; no mutable aliases, reentry,
 * signals, concurrency, FD duplication/mutation or fork. Caller initializes the
 * actual persistent hub ONCE after LAST verifier fork and required ownership
 * ordering, and independently admits actual chunk backing before entry. ACTIVE
 * pristine/canonical shape checks do NOT prove that ordering, allocation origin,
 * prefault/residency/ledger or memory fit. The old contiguous native ledger is
 * incompatible: native use remains BLOCKED. Do not pass chunks to old void* hub.
 * Caller retains hub/chunks + source/prestarted/config/context/external owner
 * through ALL refusal/return/HOLD paths. No server init/dispose/free/reinit.
 * Legacy options.hub_storage MUST be NULL; other fixed options unchanged.
 * SPS/PPS combined Annex-B parameter storage is stable and bounded to 256 bytes.
 *
 * Sink/context must be actually configured BEFORE entry and remain unchanged.
 * Sink retains the entire source/media process (including current stack/cursors,
 * hub, demands, native runtime and external media owner), not merely pointers.
 * It must not return/unwind/exit/free/close/END/retry or mutate these objects.
 * No sink implementation/default/noop is supplied. A local nonreturning fallback
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
 * requires actual STOP + drained + zero canonical tokens, never disposal. Caller
 * still owns backing after clean return; verifier finish is explicit and separate.
 */
#define CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1 CAMERA_SPAN_HUB_V1_CONSUMERS
struct camera_rtsp_span_retention_v1 {
 void *context;
 void (*retain)(void *,const struct camera_span_hub_v1 *,
                const struct camera_producer_demand *,unsigned);
};
int camera_rtsp_prestarted_span_listener_serve_v1(struct candidate_prestarted *,
 const struct camera_rtsp_options *,const struct camera_rtsp_producer *,
 struct camera_span_hub_v1 *borrowed_hub,
 const struct camera_rtsp_span_retention_v1 *,int *listener_fd,
 struct camera_rtsp_owned_listener_outcome_v1 *outcome);
#endif
