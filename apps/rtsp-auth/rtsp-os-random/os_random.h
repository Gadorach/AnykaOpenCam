/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_OS_RANDOM_H
#define QC07F_RTSP_OS_RANDOM_H

#include <camera/backend.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public RTSP startup probe/session bytes ONLY; not verifier seeds/nonces.
 * Stateless: context ignored. Caller supplies a legitimate disjoint writable
 * 32-byte span with serialized lifetime. Null output/unsupported length is refused
 * untouched (no arbitrary-pointer safety guarantee for a nonnull pointer).
 * Exactly one Linux getrandom(...,32,GRND_NONBLOCK) per valid request; no retry,
 * fallback or cached readiness. Valid output is cleared before the OS call and
 * wiped on any error/short result. OK means exactly32 bytes, not global entropy
 * qualification. Select before provisioning snapshots; no config mutation here.
 */
int qc07f_rtsp_os_random_ready_v1(void *context, unsigned char *out, size_t bytes);

#ifdef __cplusplus
}
#endif
#endif
