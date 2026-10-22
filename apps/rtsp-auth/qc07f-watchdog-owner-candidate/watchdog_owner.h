/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_WATCHDOG_OWNER_H
#define QC_WATCHDOG_OWNER_H
#include <stdint.h>
#include <sys/types.h>
enum qw_state {QW_EMPTY,QW_UNKNOWN,QW_ENABLING,QW_ACTIVE5,QW_DISABLED,QW_HOLD};
enum qw_phase {QW_NEXT=1,QW_RETIRE=2};
struct qw_owner {
 int fd;
 enum qw_state state;
 enum qw_phase phase;
 dev_t device,node_filesystem;
 ino_t node_inode;
 uint64_t last_ns,start_ns,session_deadline_ns;
 unsigned capture_progress;
};
#define QW_OWNER_EMPTY { .fd=-1 }
/* Trusted exact driver/core/image and sole open-description custody, no aliases,
 * concurrent I/O/fd-table changes/reentry. Static fresh/disjoint objects only.
 * Preopen already STARTS hardware. Move metadata-only take leaves UNKNOWN, never
 * armed proof. No actor opens/duplicates/closes here. Source associations and
 * original provider/backing admission are NOT proved by descriptor metadata. */
int qw_owner_take(struct qw_owner *,int *,dev_t);
int qw_owner_begin(struct qw_owner *,uint64_t,enum qw_phase,uint64_t);
int qw_owner_check(struct qw_owner *); /* active phase, real clock, <2s */
/* ONLY exact producer callsite after full raw CRC/extent/status and ACTIVE keyed
 * receipt/sequence checks. At most once per NEXT; not a generic feeder API. */
int qw_owner_capture_progress(struct qw_owner *);
/* Trusted integration calls ONLY after real successful producer/postchecks or
 * normal END/QUERY/ACK and cleanup. No caller-supplied 'qualified' Boolean.
 * DISABLED is ioctl ACK/logical state only, not electrical/DMA stop proof. */
int qw_owner_disable_boundary(struct qw_owner *);
int qw_owner_idle_observe(struct qw_owner *); /* DISABLED; no servicing */
int qw_owner_give_back(struct qw_owner *,int *); /* DISABLED custody move, no close */
void qw_owner_fail(struct qw_owner *); /* sticky HOLD, no IO, no armed guarantee */
/* No reset/close/destructor/HOLD-clear/retry/periodic thread or timer. On failure
 * actual FD stays owned, unless caller violates trusted custody. Failed disable
 * may already have stopped it; HOLD NEVER guarantees armed recovery. Final close
 * is outside this API; Linux close error cannot preserve an actual description
 * merely by retaining its former numeric FD or retrying it. Nominal5s unchanged. */
#endif
