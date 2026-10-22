/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_ENCODER_FD_CUSTODY_H
#define QC07F_ENCODER_FD_CUSTODY_H
#include <stdint.h>
#include <sys/types.h>
#include "encoder-open-query.h"

/* ONE persistent encoder open description, not the temporary-job role.
 * TRUSTED PRECONDITIONS: exact admitted boot/image/source/query association;
 * expected identity independently observed from that actual description;
 * sole serialized custody, no surviving dup/fork/SCM_RIGHTS aliases, concurrent
 * I/O/fd-table changes/close or signal reentry. Objects/slots are disjoint.
 * fstat/query cannot prove these facts, authority, backing or native admission.
 * This is an in-process source API, not a binary plugin/descriptor wire format.
 */
struct qc_encoder_fd_identity {dev_t device;uint64_t session_identity;};
enum qc_encoder_fd_state {QC_ENCODER_FD_EMPTY,QC_ENCODER_FD_HELD,QC_ENCODER_FD_HOLD};
enum qc_encoder_fd_result {
 QC_ENCODER_FD_OK=0,QC_ENCODER_FD_INVALID=-1,QC_ENCODER_FD_SYSTEM=-2,
 QC_ENCODER_FD_IDENTITY=-3,QC_ENCODER_FD_SNAPSHOT=-4,
 QC_ENCODER_FD_NOT_CLEAR=-5,QC_ENCODER_FD_UNCERTAIN=-6
};
/* Static allocation only. Explicit -1 initializer mandatory; zero/FD0 refuses.
 * Do not copy/reinitialize/modify while held. Owns the ACTUAL numeric FD plus
 * last fully valid snapshot, not just metadata pretending to own a process/DMA.
 */
struct qc_encoder_fd_custody {
 int fd;
 enum qc_encoder_fd_state state;
 struct qc_encoder_fd_identity identity;
 dev_t node_filesystem;
 ino_t node_inode;
 struct qc_hwenc_open_query last_observed;
};
#define QC_ENCODER_FD_CUSTODY_EMPTY { .fd=-1 }

/* Observe preopened FD>=3 with GET-only fcntl/fstat and ONLY OPEN_QUERY; require
 * O_RDWR/CLOEXEC/matching char node and exact version1/88/reserved0/session tuple.
 * Commit move ONLY for relationNONE/owner0/frame0, no observed fault/active ledger
 * or telemetry error/unknown-flag contradiction. Saturation alone is retained,
 * not converted to a unique generation or discarded history. No open/dup/close,
 * F_SET, malloc, jobs, resets or fallback to global session_sequence.
 * Failure before commit leaves bootstrap slot/owner unchanged. Failed ioctl
 * returns SYSTEM with its errno (including ENOTTY) unchanged; partial replies
 * are never read. Old frozen76 does not support this candidate query.
 */
int qc_encoder_fd_take(struct qc_encoder_fd_custody *,int *bootstrap_slot,
                       const struct qc_encoder_fd_identity *expected);

/* Recipient MUST be -1. Reobserve then move same FD back; never close/disarm it.
 * Observation failure latches sticky HOLD retaining FD/identity/last valid full
 * snapshot (including a valid THIS/OTHER/fault snapshot); never clears HOLD or
 * attempts cleanup/retry. Invalid caller arguments do not change ownership.
 * Caller must retain owner/process on HOLD. No destructor/recovery/free API.
 * Success means custody transfer only: NOT DMA health/retirement, never-used,
 * readiness, authorization, backing admission, alias exclusivity or kernel pairing.
 */
int qc_encoder_fd_give_back(struct qc_encoder_fd_custody *,int *recipient_slot);
#endif
