/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_PRESTARTED_OWNED_LISTENER_V1_H
#define CAMERA_RTSP_PRESTARTED_OWNED_LISTENER_V1_H
#include "candidate_auth.h"
#define CAMERA_RTSP_PRESTARTED_OWNED_LISTENER_SOURCE_API 1U

/* Source API only. Read COMPOSITION_CONTRACT.md before use. One actual stable
 * candidate_prestarted backend/config/child/peers, device-free real start ONCE,
 * all fork/exec complete BEFORE native/provider/watchdog/backing/userdata setup.
 * Selected listener must also be created AFTER last fork, under independently
 * admitted bootstrap endpoint policy. No inherited-listener exception, no new
 * bootstrap/permission/readiness/entropy/source implementation is provided.
 * Config/graph/context ADDRESS/callbacks immutable; later actual userdata only
 * inside that same stable context, after last fork. No FD aliases/dup/SCM/fork,
 * concurrent FD-table mutation, signals/threads/reentry or moved/aliased objects.
 * Callbacks must not use/mutate listener slot/outcome or retain the gate after
 * unbind. This is trusted serialized public C, not thread/hostile-code safety.
 *
 * Active nested entry -> BUSY FIRST, no owner/FD/outcome touch. Outside reentry,
 * NULL required storage, invalid owner/self/config or non-READY phase -> refusal
 * without touching any records; stale outcome is NOT a new disposition. Admitting
 * actual READY owner + non-NULL slot/outcome consumes the sole serve attempt,
 * even if options/backend/listener preflight then fails (terminal owner HOLD).
 * All storage is required live/stable/writable/disjoint through call and HOLD.
 *
 * Only exact original loopback127.0.0.1:options.port>=1024, AF_INET TCP listening,
 * nonblocking/CLOEXEC. Real backend checks/poll/READY/profile/child observation,
 * all original auth/source checks and final actual socket requery precede move.
 * Caller slot is actual ownership, NOT a copied numeric FD: precommit errors
 * leave it untouched/consumed0; move -> private listener, caller=-1/consumed1.
 * Poll/accept only that FD; no socket/bind/listen/F_SETFL/listener setsockopt or
 * internal fallback. Every postcommit return consumed1, one clear-before-close
 * after source cleanup/guard unbind. Error never grants another close/retry or
 * claims the former numeric description is retained. Close failure -> non-OK +
 * owner HOLD, never clean SERVED. Final outcome is published before reentry unlock.
 *
 * Original v1 scalar outcome layout/name preserved for unchanged custody bodies.
 * It is NOT a source/DMA/verifier retirement receipt or external media owner.
 * Finish remains explicit original verifier-only one-shot; never finish HOLD.
 * Native failed-NEXT/uncertain source cleanup/backing/retention remain BLOCKED.
 */
struct camera_rtsp_owned_listener_outcome_v1 {
 unsigned consumed,close_attempted;
 int close_errno,result;
};
int camera_rtsp_prestarted_owned_listener_serve_v1(struct candidate_prestarted *,
 const struct camera_rtsp_options *,const struct camera_rtsp_producer *,
 int *listener_fd,struct camera_rtsp_owned_listener_outcome_v1 *outcome);
#endif
