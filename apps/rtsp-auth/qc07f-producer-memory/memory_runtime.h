/* SPDX-License-Identifier: GPL-2.0-only
 * SOURCE replacement only; rebuild consumers, never old+new memory units.
 */
#ifndef NH_MEMORY_RUNTIME_H
#define NH_MEMORY_RUNTIME_H
#include "memory_reserve.h"
enum nh_memory_span_attempt {NH_MEM_SPAN_FRESH,NH_MEM_SPAN_TOUCHING,NH_MEM_SPAN_HOLD,NH_MEM_SPAN_PAID};
struct nh_memory_runtime {
 struct nh_memory_ledger ledger;
 struct nh_memory_observation observations[64];unsigned count; /* finite forecast points for13 AUs, not a frame/deadline ceiling change */
 char meminfo[NH_MEMINFO_CAP],zoneinfo[NH_ZONEINFO_CAP];
 uint64_t session_start;
 uint64_t (*now)(void *);int (*cancelled)(void *);void *context;
 /* Exact borrowed association survives EVERY admitted attempt outcome. */
 struct nh_memory_hub_ranges span_retained;
 enum nh_memory_span_attempt span_attempt;
 enum nh_memory_reason span_failure;
};
int nh_memory_read_fixed(unsigned,char *,size_t,size_t *); /* ONLY meminfo/zoneinfo */
/* True-fresh zeroed persistent runtime, once. No reset/reinitialization. */
int nh_memory_runtime_init(struct nh_memory_runtime *,uint64_t,
 uint64_t (*)(void *),int (*)(void *),void *,unsigned);
int nh_memory_runtime_gate(struct nh_memory_runtime *,enum nh_memory_stage,uint64_t);
int nh_memory_prefault_owned(struct nh_memory_runtime *,unsigned,void *,size_t,uint64_t);
/* Actual typed bridge, not called by this source-only delivery. Valid stable
 * disjoint runtime/owner/chunks, private writable backing; no aliases/copies,
 * concurrency/reentry/signals/fork or callbacks changing ledger/hub/config.
 * Borrow exact owner/ranges through HOLD/success until real retirement elsewhere.
 * Caller must not dispose/publish/subscribe/mutate during this whole operation.
 * Failures after admission retain unpaid backing permanently; no retry/free.
 * No hostile-pointer probing or fault catching; callbacks use REAL clock/cancel.
 */
int nh_memory_prefault_span_hub(struct nh_memory_runtime *,
 struct camera_span_hub_v1 *,uint64_t);
void nh_memory_log(const struct nh_memory_runtime *);
#endif
