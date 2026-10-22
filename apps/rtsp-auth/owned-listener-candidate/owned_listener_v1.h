/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_OWNED_LISTENER_V1_H
#define CAMERA_RTSP_OWNED_LISTENER_V1_H
#include "frozen/candidate_auth.h"
#define CAMERA_RTSP_OWNED_LISTENER_SOURCE_API 1U

/* Source API only, not a binary plugin ABI or network admission receipt.
 * Read CONTRACT.md before use: trusted same-process bootstrap, serialized FD
 * table, no aliases/forks/SCM except inspected rv_start transient inheritance,
 * disjoint stable live storage, synchronous bounded nonreentrant callbacks.
 * Bootstrap alone creates/binds/listens per separately admitted policy. This
 * entry supports ONLY exact original options: 127.0.0.1:port, port>=1024,
 * nonblocking/CLOEXEC listening AF_INET TCP, actual strict backend readiness.
 * No fallback open/bind/listen, authority flags or provisioning is supplied.
 *
 * listener_fd is the REAL caller-owned writable int slot, not a copied number.
 * Before commit all errors leave it untouched. On move it becomes -1 exactly
 * when outcome.consumed becomes 1; poll/accept then use the private owned slot.
 * Every postcommit return reports consumed=1; caller must never close/retry that
 * original numeric descriptor. Server clears its own slot before one close,
 * including on close error, which may already have released the number. Outcome
 * does not certify FD/source/verifier/DMA retirement. Original uncertainty sinks
 * and provider/backing/code/owner retention remain mandatory and independent.
 *
 * outcome is required, writable and disjoint from all inputs/backing/contexts;
 * neither callbacks nor caller may mutate it or listener_fd during the call.
 * Original serve symbol declared in frozen/candidate_auth.h is NOT implemented
 * in this derivative. Do not link with another server/backend implementation.
 */
struct camera_rtsp_owned_listener_outcome_v1 {
 unsigned consumed,close_attempted;
 int close_errno,result;
};
int camera_rtsp_owned_listener_serve_v1(const struct camera_rtsp_options *,
 const struct camera_rtsp_producer *,const struct candidate_config *,
 int *listener_fd,struct camera_rtsp_owned_listener_outcome_v1 *outcome);
#endif
