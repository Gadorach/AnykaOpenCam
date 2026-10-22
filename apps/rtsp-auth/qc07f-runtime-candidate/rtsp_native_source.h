/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_NATIVE_SOURCE_H
#define QC07F_RTSP_NATIVE_SOURCE_H
#include "candidate_auth.h"

/* SOURCE/COMPILE candidate only; not a provider FD adoption API or memory GO.
 * Wire the retained singleton v6 owner into the existing common RTSP bridge.
 * Caller supplies a freshly initialized, stopped, keyframe-capable producer and
 * fresh, disjoint output/bridge/lifecycle records; none may move after wiring.
 * Config remains caller-owned: only source_guard_bind is installed here. Real
 * source_ready, bootstrap FDs, entropy, retention sinks, graph and policy remain
 * mandatory. Nothing initializes/adopts hardware, allocates, reads provisioning,
 * launches a child or opens a listener in this function.
 *
 * Native preparation still requires the ORIGINAL exact139/storage/memory gates
 * and parent admission; they are not invoked or waived. The old v6 provider's
 * allocation/FD/memory contracts are NOT suitable for product adoption yet.
 * All calls/callbacks are serialized, bounded and nonreentrant, as upstream.
 * One wire per process lifetime; no teardown/reinitialize/retry API is added.
 */
int qc07f_rtsp_native_wire(struct camera_rtsp_producer *output,
                         struct camera_rtsp_lifecycle *lifecycle,
                         struct camera_rtsp_common_producer *bridge,
                         struct camera_producer *producer,
                         struct candidate_config *config);
#endif
