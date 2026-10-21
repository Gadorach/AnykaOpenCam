/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_CAPTURE_RETIREMENT_H
#define QC_CAPTURE_RETIREMENT_H
#include "capture-retirement-uapi.h"
struct qc_retirement_source;
/* One boot-lifetime registry; must outlive device rebind/reprobe. A module-local
 * counter reset on unload is NOT an adequate source-instance namespace.
 */
struct qc_retirement_registry {__u64 next_source;};
struct qc_retirement_file {
 struct qc_retirement_source *source;
 __u64 id,generation,stopped_ns;
 enum qc_capture_receipt_state state;
 enum qc_capture_stop_reason reason;
 int stop_result;
 unsigned open;
};
struct qc_retirement_source {
 __u64 id,next_file,next_generation;
 unsigned open_files;
 /* Independent of physical stream_owner: retained through successful selfstop.
  * On failed last close, hold references this separately allocated context,
  * NEVER the released struct file. Its device/backing must remain pinned too.
  */
 struct qc_retirement_file *hold;
};
struct qc_retirement_ops {
 /* Whole existing gated begin, or mask/grant removal + IRQ synchronization +
  * actual successful reset sequence. Not a diagnostic flag or errno heuristic.
  * Existing caller lock surrounds callbacks; no reentry or IRQ work here.
  */
 int (*begin)(void *);
 int (*stop_reset)(void *);
 __u64 (*clock_ns)(void *);
 void *context;
};
struct qc_retirement_uaccess {
 int (*from_user)(void *,struct qc_capture_receipt *);
 int (*to_user)(void *,const struct qc_capture_receipt *);
 void *context;
};
/* Private fence identity captured before the actual stop. A late completion
 * cannot mint a receipt for a reused file object or successor generation.
 */
struct qc_retirement_fence {__u64 source_instance,file_instance,generation;};
/* SOURCE-ONLY contract/helper model. Caller serializes with the actual ISP lock.
 * Objects are caller-owned and zero/fresh initialized. No allocation/free, IRQ,
 * timer, DMA, reset or ioctl syscall is performed except the supplied callbacks.
 * Model can refuse destruction; baseline devres CANNOT be pinned by this model.
 * Read the adoption blocker docs before considering a kernel integration.
 */
int qc_retirement_source_init(struct qc_retirement_registry *,struct qc_retirement_source *);
int qc_retirement_open(struct qc_retirement_source *,struct qc_retirement_file *);
int qc_retirement_ioctl(struct qc_retirement_file *,unsigned long,const struct qc_retirement_uaccess *);
int qc_retirement_begin(struct qc_retirement_file *,const struct qc_retirement_ops *);
int qc_retirement_stop_enter(struct qc_retirement_file *,enum qc_capture_stop_reason,struct qc_retirement_fence *);
int qc_retirement_stop_finish(struct qc_retirement_file *,const struct qc_retirement_fence *,int,__u64);
int qc_retirement_stop(struct qc_retirement_file *,enum qc_capture_stop_reason,const struct qc_retirement_ops *);
int qc_retirement_close(struct qc_retirement_file *,const struct qc_retirement_ops *);
int qc_retirement_file_releasable(const struct qc_retirement_file *);
int qc_retirement_source_releasable(const struct qc_retirement_source *);
/* Every successor capture path, including legacy/sysfs/ring/queue, must refuse
 * an unresolved receipt hold. This is an exclusive ownership barrier, not a
 * viewer/demand refcount and not a replacement for actual kernel DMA leases.
 */
int qc_retirement_admission(const struct qc_retirement_source *);
#endif
