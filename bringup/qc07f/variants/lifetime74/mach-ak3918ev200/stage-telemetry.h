/* SPDX-License-Identifier: GPL-2.0-only
 * Caller serialized, per-device bounded observational counters. No allocation,
 * clocks, MMIO, DMA decisions, printk, lease/fence or ownership mutation here.
 */
#ifndef QC_STAGE_TELEMETRY_H
#define QC_STAGE_TELEMETRY_H
#include "stage-telemetry-uapi.h"
#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif
#define QC_STAGE_U64_MAX (~(__u64)0)
struct qc_stage_stat {
 __u64 samples,failed,total_ns,min_ns,max_ns,bytes,failed_ns;
};
struct qc_stage_last {
 __u64 operation,owner_generation,start_ns,end_ns,mask;
 __u64 phase_ns[QC_STAGE_PHASES],phase_bytes[QC_STAGE_PHASES];
 __u64 phase_start_ns[QC_STAGE_PHASES],phase_end_ns[QC_STAGE_PHASES];
 __s32 result;
 __u32 kind,retirement_failed;
};
struct qc_stage_context {
 struct qc_stage_stat stat[QC_STAGE_KINDS][QC_STAGE_PHASES];
 struct qc_stage_last active_record,last,last_failure;
 __u64 operations,ok,failed,delivered,owner_generation;
 __u64 alloc_attempts,alloc_failures,alloc_successes,free_calls;
 __u64 live_bytes,peak_bytes,requested_bytes,freed_bytes,retire_failures;
 __u32 active,owner_active,flags,supported_mask[QC_STAGE_KINDS];
};
/* Instrumentation reachability, not zero-cost measurements. Initialize before
 * publishing a fresh device; do not reset counters or an active context here.
 */
static inline void qc_stage_isp_coverage(struct qc_stage_context *c)
{
 __u32 common=(1U<<QC_PHASE_RESET)|(1U<<QC_PHASE_PREPARE)|(1U<<QC_PHASE_RESET_RETIRE);
 __u32 pool=(1U<<QC_PHASE_ALLOC)|(1U<<QC_PHASE_FREE);
 __u32 next=(1U<<QC_PHASE_PRE_IDLE)|(1U<<QC_PHASE_PROGRAM)|(1U<<QC_PHASE_FRAME_WAIT)|
  (1U<<QC_PHASE_PP_COMMIT)|(1U<<QC_PHASE_PRE_GATE_DRAIN)|(1U<<QC_PHASE_POST_GATE_DRAIN)|
  (1U<<QC_PHASE_GUARDS)|(1U<<QC_PHASE_CRC_PRE)|(1U<<QC_PHASE_PIXEL_COPY)|
  (1U<<QC_PHASE_CRC_POST)|(1U<<QC_PHASE_RESET_RETIRE);
 memset(c->supported_mask,0,sizeof(c->supported_mask));
 c->supported_mask[QC_STAGE_OTHER]=common|pool;
 c->supported_mask[QC_STAGE_BEGIN]=common;
 c->supported_mask[QC_STAGE_END]=(1U<<QC_PHASE_RESET_RETIRE)|(1U<<QC_PHASE_FREE);
 c->supported_mask[QC_STAGE_QUEUE]=common|pool;
 c->supported_mask[QC_STAGE_NEXT]=next;c->supported_mask[QC_STAGE_NEXT_IRQ]=next;
}
static inline void qc_stage_codec_coverage(struct qc_stage_context *c)
{
 memset(c->supported_mask,0,sizeof(c->supported_mask));
 c->supported_mask[QC_STAGE_INFO]=(1U<<QC_PHASE_RESET);
 c->supported_mask[QC_STAGE_JPEG]=(1U<<QC_PHASE_RESET)|(1U<<QC_PHASE_ALLOC)|
  (1U<<QC_PHASE_FREE)|(1U<<QC_PHASE_RESET_RETIRE);
 c->supported_mask[QC_STAGE_H264]=(1U<<QC_PHASE_RESET)|(1U<<QC_PHASE_PREPARE)|
  (1U<<QC_PHASE_PROGRAM)|(1U<<QC_PHASE_FRAME_WAIT)|(1U<<QC_PHASE_GUARDS)|
  (1U<<QC_PHASE_PIXEL_COPY)|(1U<<QC_PHASE_OUTPUT_COPY)|(1U<<QC_PHASE_ALLOC)|
  (1U<<QC_PHASE_FREE)|(1U<<QC_PHASE_RESET_RETIRE);
 c->supported_mask[QC_STAGE_END]=(1U<<QC_PHASE_FREE)|(1U<<QC_PHASE_RESET_RETIRE);
}
static inline __u64 qc_stage_add(struct qc_stage_context *c,__u64 a,__u64 b)
{
 if(b>QC_STAGE_U64_MAX-a){c->flags|=QC_STAGE_F_SATURATED;return QC_STAGE_U64_MAX;}
 return a+b;
}
static inline void qc_stage_begin(struct qc_stage_context *c,__u32 kind,__u64 now)
{
 if(c->active||kind>=QC_STAGE_KINDS){c->flags|=QC_STAGE_F_LEDGER_ERROR;return;}
 memset(&c->active_record,0,sizeof(c->active_record));
 c->operations=qc_stage_add(c,c->operations,1);
 c->active_record.operation=c->operations;c->active_record.kind=kind;
 c->active_record.owner_generation=c->owner_generation;c->active_record.start_ns=now;c->active=1;
}
static inline void qc_stage_sample(struct qc_stage_context *c,__u32 phase,
 __u64 start,__u64 end,__u64 bytes,int result)
{
 struct qc_stage_stat *s;__u64 elapsed;
 if(!c->active)return; /* probe/free ledger has no operation stage until begun */
 if(phase>=QC_STAGE_PHASES){c->flags|=QC_STAGE_F_LEDGER_ERROR;return;}
 if(end<start){c->flags|=QC_STAGE_F_CLOCK_ERROR;return;}
 elapsed=end-start;s=&c->stat[c->active_record.kind][phase];
 if(!s->samples||elapsed<s->min_ns)s->min_ns=elapsed;
 if(elapsed>s->max_ns)s->max_ns=elapsed;
 s->samples=qc_stage_add(c,s->samples,1);
 if(result){s->failed=qc_stage_add(c,s->failed,1);s->failed_ns=qc_stage_add(c,s->failed_ns,elapsed);}
 s->total_ns=qc_stage_add(c,s->total_ns,elapsed);s->bytes=qc_stage_add(c,s->bytes,bytes);
 if(!(c->active_record.mask&((__u64)1<<phase)))c->active_record.phase_start_ns[phase]=start;
 c->active_record.phase_end_ns[phase]=end;c->active_record.mask|=(__u64)1<<phase;
 c->active_record.phase_ns[phase]=qc_stage_add(c,c->active_record.phase_ns[phase],elapsed);
 c->active_record.phase_bytes[phase]=qc_stage_add(c,c->active_record.phase_bytes[phase],bytes);
}
static inline void qc_stage_end(struct qc_stage_context *c,int result,__u32 delivered,__u64 now)
{
 if(!c->active){c->flags|=QC_STAGE_F_LEDGER_ERROR;return;}
 c->active_record.result=result;c->active_record.end_ns=now;
 if(now<c->active_record.start_ns)c->flags|=QC_STAGE_F_CLOCK_ERROR;
 if(result){c->failed=qc_stage_add(c,c->failed,1);c->last_failure=c->active_record;}
 else c->ok=qc_stage_add(c,c->ok,1);
 if(!result&&delivered)c->delivered=qc_stage_add(c,c->delivered,1);
 c->last=c->active_record;c->active=0;
}
static inline void qc_stage_owner(struct qc_stage_context *c,__u32 active,__u32 new_session)
{
 if(c->owner_active!=!!active||new_session)c->owner_generation=qc_stage_add(c,c->owner_generation,1);
 c->owner_active=!!active; /* observational only, never a lease generation */
}
static inline void qc_stage_alloc(struct qc_stage_context *c,__u64 bytes,__u32 success)
{
 c->alloc_attempts=qc_stage_add(c,c->alloc_attempts,1);
 c->requested_bytes=qc_stage_add(c,c->requested_bytes,bytes); /* includes failed requests */
 if(!success){c->alloc_failures=qc_stage_add(c,c->alloc_failures,1);return;}
 c->alloc_successes=qc_stage_add(c,c->alloc_successes,1);
 c->live_bytes=qc_stage_add(c,c->live_bytes,bytes);
 if(c->live_bytes>c->peak_bytes)c->peak_bytes=c->live_bytes;
}
static inline void qc_stage_free(struct qc_stage_context *c,__u64 bytes)
{
 c->free_calls=qc_stage_add(c,c->free_calls,1);
 c->freed_bytes=qc_stage_add(c,c->freed_bytes,bytes);
 if(bytes>c->live_bytes){c->flags|=QC_STAGE_F_LEDGER_ERROR;return;}
 c->live_bytes-=bytes;
}
static inline void qc_stage_retirement(struct qc_stage_context *c,int result)
{
 if(!result)return;
 c->retire_failures=qc_stage_add(c,c->retire_failures,1);c->flags|=QC_STAGE_F_RETIRE_FAILED;
 if(c->active)c->active_record.retirement_failed=1;
}
static inline int qc_stage_query(const struct qc_stage_context *c,struct qc_stage_query *q)
{
 const struct qc_stage_stat *s;const struct qc_stage_last *last;
 __u32 kind=q->kind,phase=q->phase,record=q->record;
 if(q->version!=QC_STAGE_VERSION||q->struct_bytes!=sizeof(*q)||q->reserved||
    kind>=QC_STAGE_KINDS||phase>=QC_STAGE_PHASES||record>1)return -1;
 memset(q,0,sizeof(*q));q->version=QC_STAGE_VERSION;q->struct_bytes=sizeof(*q);
 q->kind=kind;q->phase=phase;q->record=record;q->clock_source=QC_STAGE_CLOCK_MONOTONIC_KTIME_NS;
 last=record?&c->last_failure:&c->last;
 q->flags=c->flags;q->supported_mask=c->supported_mask[kind];s=&c->stat[kind][phase];
 q->samples=s->samples;q->failed_samples=s->failed;q->total_ns=s->total_ns;
 q->min_ns=s->min_ns;q->max_ns=s->max_ns;q->work_bytes=s->bytes;
 q->failed_total_ns=s->failed_ns;
 q->operation_count=c->operations;q->operation_ok=c->ok;q->operation_failed=c->failed;q->delivered=c->delivered;
 q->last_operation=last->operation;q->last_owner_generation=last->owner_generation;
 q->last_start_ns=last->start_ns;q->last_end_ns=last->end_ns;q->last_phase_mask=last->mask;
 q->last_phase_ns=last->phase_ns[phase];q->last_phase_bytes=last->phase_bytes[phase];
 q->last_phase_start_ns=last->phase_start_ns[phase];q->last_phase_end_ns=last->phase_end_ns[phase];
 q->last_result=last->result;q->last_kind=last->kind;q->owner_active=c->owner_active;
 q->last_retirement_failed=last->retirement_failed;
 q->allocation_attempts=c->alloc_attempts;q->allocation_failures=c->alloc_failures;
 q->allocation_successes=c->alloc_successes;q->free_calls=c->free_calls;
 q->live_requested_bytes=c->live_bytes;q->peak_requested_bytes=c->peak_bytes;
 q->total_requested_bytes=c->requested_bytes;q->total_freed_bytes=c->freed_bytes;q->retire_failures=c->retire_failures;
 q->owner_generation=c->owner_generation;
 return 0;
}
#endif
