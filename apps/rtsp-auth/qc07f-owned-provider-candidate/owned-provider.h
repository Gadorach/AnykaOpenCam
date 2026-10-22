/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QP_OWNED_PROVIDER_H
#define QP_OWNED_PROVIDER_H
#include "native_watchdog.h"
#include "frame_fd_custody.h"
#include "encoder_fd_custody.h"
/* Source API only; stable/disjoint slots/expected values/output objects, no
 * aliases, concurrent FD/I/O/reentry/fork or competing provider dispatch. Trusted
 * bootstrap independently establishes actual boot/image/source/device/session/
 * file associations, permitted effects and backing/recovery. Metadata is NOT
 * proof of these, permission, readiness or DMA retirement. Old76 lacks query88.
 *
 * init -> watchdog take UNKNOWN -> storage select + exact139 opt-in -> take roles
 * -> original one-shot memory setup -> NEXT/release/normal retirement. H264 IP
 * stream only; no IDR-only temporary role, fallback opens or retry/restart.
 *
 * This is NOT atomic: sensor GET metadata, encoder take, frame take, sensor move,
 * then observed helper give-back moves to ACTUAL native slots. A failed initial
 * encoder take leaves all slots/bootstrap ownership unchanged. Once any moves,
 * failure latches partial HOLD retaining actual moved FDs in stable native/staging
 * records. Bootstrap still owns its nonnegative slots; it MUST retain them and
 * this real process. Never assume return!=0 means bootstrap owns all descriptions.
 * No rollback/close/destructor/retry or automatic process retention is provided.
 * Expected identities and latest valid known snapshots survive HOLD; failed/
 * malformed/mismatched replies do not overwrite them. No borrowed caller pointer
 * persists in the native owner; snapshots are not duplicate FD ownership.
 */
int nh_native_provider_take(int *sensor_slot,dev_t sensor_device,
 int *frame_slot,const struct qc_frame_fd_identity *,
 int *encoder_slot,const struct qc_encoder_fd_identity *);
#endif
