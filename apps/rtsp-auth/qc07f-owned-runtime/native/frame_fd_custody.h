/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_FRAME_FD_CUSTODY_H
#define QC07F_FRAME_FD_CUSTODY_H
#include <stdint.h>
#include <sys/types.h>
#include "capture-retirement-uapi.h"

/* Source API, not a binary plugin, provider-readiness flag or DMA lease.
 * TRUSTED PRECONDITION: the bootstrap supplied the actual qc07f-frame open
 * description for the exact pinned kernel, with the independently observed
 * device/source/file tuple below. It exclusively transfers that description:
 * no surviving dup/fork/SCM_RIGHTS alias, no concurrent fd-table changes, I/O,
 * signal-handler reentry or external close. fstat/BIND cannot prove exclusivity.
 * No task binding exists in this kernel UAPI; do not invent one here.
 * This precondition is NOT native admission or permission to acquire/open an FD.
 */
struct qc_frame_fd_identity {
 dev_t device;
 uint64_t source_instance,file_instance;
};
enum qc_frame_fd_state {QC_FRAME_FD_EMPTY,QC_FRAME_FD_HELD,QC_FRAME_FD_HOLD};
enum qc_frame_fd_result {
 QC_FRAME_FD_OK=0,QC_FRAME_FD_INVALID=-1,QC_FRAME_FD_SYSTEM=-2,
 QC_FRAME_FD_IDENTITY=-3,QC_FRAME_FD_NOT_FRESH=-4,QC_FRAME_FD_UNCERTAIN=-5
};
/* Public for static allocation ONLY. Do not copy, move, reinitialize or modify
 * while held. This record owns the actual numeric FD, not just a receipt copy.
 * Explicit -1 initializer is mandatory: zero/FD0 is never an empty owner.
 */
struct qc_frame_fd_custody {
 int fd;
 enum qc_frame_fd_state state;
 struct qc_frame_fd_identity identity;
 dev_t node_filesystem;
 ino_t node_inode;
 struct qc_capture_receipt last_observed;
};
#define QC_FRAME_FD_CUSTODY_EMPTY { .fd=-1 }

/* Observe an already preopened O_RDONLY|FD_CLOEXEC char FD>=3 using ONLY
 * fcntl GET/fstat and the existing observational BIND (not ARM/QUERY/ACK).
 * Require exact expected tuple, NONE/gen0/clean result before committing.
 * On success move *bootstrap_slot into owner, set slot=-1. On failure the
 * bootstrap retains its FD and owner is unchanged. No dup/close/set/open/alloc.
 * Initial observation is not source health, global availability, prior-use,
 * backing admission or proof that an unknown earlier DMA operation retired.
 */
int qc_frame_fd_take(struct qc_frame_fd_custody *owner,int *bootstrap_slot,
                     const struct qc_frame_fd_identity *expected);

/* Transfer the SAME FD back into an explicit -1 recipient slot, never close it.
 * A failed/mismatching observation latches HOLD and retains FD/identity/record.
 * No automatic destructor, reset, ACK, END or recovery API exists. An actual
 * unexpected generation is retained in last_observed when its identity is valid.
 * This return is custody transfer ONLY, NOT DMA/normal-close proof or permission
 * for the recipient to free backing/close a provider. It cannot clear HOLD.
 */
int qc_frame_fd_give_back(struct qc_frame_fd_custody *owner,int *recipient_slot);
#endif
