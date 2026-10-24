/* SPDX-License-Identifier: GPL-2.0-only
 * SOURCE replacement: retained names/guards, NEW layout, never old+new units.
 */
#ifndef NH_MEMORY_RESERVE_H
#define NH_MEMORY_RESERVE_H
#include <stddef.h>
#include <stdint.h>
#include "../span-hub-producer-candidate/span_hub.h"
/* TRUSTED source API: valid stable disjoint ledger/owner/output/private chunks,
 * true-fresh storage and actual version3 allocation AND zero writer history,
 * no mutable aliases,
 * reentry/signals/fork/concurrency or callbacks mutating any of these objects.
 * Never reset/copy/edit ledgers or owner/range association to regain credit.
 * Retain owner/backing until separately proved real retirement. Nonnull/self/
 * history checks are NOT hostile-pointer, malloc or residency validation. */
#define NH_MEMINFO_CAP 4096U
#define NH_ZONEINFO_CAP 16384U
#define NH_MEMORY_MAX_SAMPLE_NS 100000000ULL
#define NH_MEMORY_OVERHEAD_KIB 512ULL
#define NH_MEMORY_RESERVE_KIB 1024ULL
#define NH_MEMORY_CODEC_KIB 8256ULL
#define NH_MEMORY_MIN_CPU_KIB 3860ULL
#define NH_MEMORY_RAW_REUSE_MIN_CPU_KIB 2832ULL
#define NH_MEMORY_HUB_RANGES 8U
#define NH_MEMORY_HUB_BYTES 131072U
enum nh_memory_stage {NH_MEM_INITIAL,NH_MEM_FIXED_PAID,NH_MEM_PRE_NEXT,NH_MEM_POST_FIRST};
enum nh_memory_reason {NH_MEM_OK,NH_MEM_FORMAT,NH_MEM_LAYOUT,NH_MEM_STALE,NH_MEM_CLOCK,NH_MEM_LOW_AVAILABLE,NH_MEM_LOW_NONCMA,NH_MEM_CREDIT,NH_MEM_IO,NH_MEM_CANCEL};
struct nh_memory_values {uint64_t total,free,available,cma_total,cma_free,swap_total;};
struct nh_memory_zone {
 uint64_t free,boost,min,low,high,promo,spanned,present,managed,cma,protection[2],free_cma;
};
struct nh_memory_zones {struct nh_memory_zone normal,movable;uint64_t watermark_kib;};
struct nh_memory_hub_ranges {
 const struct camera_span_hub_v1 *owner;
 uintptr_t begin[NH_MEMORY_HUB_RANGES],end[NH_MEMORY_HUB_RANGES];
 unsigned count;uint64_t credit_kib;
};
struct nh_memory_ledger {
 uint64_t cpu_capacity_kib,cpu_paid_kib,nonmovable_cpu_debt_kib;
 uintptr_t begin[3],end[3];unsigned paid[3],codec_paid,initialized,raw_reuse;
 /* begin/end[1] stay zero: there is NO flat hub extent. */
 struct nh_memory_hub_ranges hub;
};
struct nh_memory_observation {
 uint64_t begin_ns,end_ns,available,noncma_free,watermark_kib,cpu_debt,dma_debt,required_available,required_noncma;
 uint64_t migration_future_kib;
 enum nh_memory_stage stage;enum nh_memory_reason reason;
};
int nh_memory_parse_meminfo(const char *,size_t,struct nh_memory_values *);
int nh_memory_parse_zoneinfo(const char *,size_t,struct nh_memory_zones *);
/* Trusted true-fresh storage only, once; no reset/reinit of retained histories. */
int nh_memory_init(struct nh_memory_ledger *,uint64_t,uint64_t);
int nh_memory_init_mode(struct nh_memory_ledger *,uint64_t,uint64_t,unsigned);
/* Scalar raw(0)/producer(2) metadata half. Hub(1) ALWAYS refuses before touch. */
int nh_memory_credit_owned(struct nh_memory_ledger *,unsigned,const void *,size_t);
/* Metadata preflight only, no chunk reads/writes. Output unchanged on refusal.
 * Exhaustive v3 pristine history: IDLE alone cannot admit cancelled reservations;
 * All writer issuance/current/next-generation/exposure/error and slice offset
 * must be zero, alongside all existing consumer/publication/allocation checks.
 * Actual typed prefault checks this before attempt/callback/touch AND precredit.
 * This is UNPAID admission only: legitimate later writer activity is not fresh;
 * paid readiness/return/borrow-end need separate actual typed integration. */
int nh_memory_span_hub_forecast(const struct nh_memory_ledger *,
 const struct camera_span_hub_v1 *,struct nh_memory_hub_ranges *);
/* Metadata half ONLY: trusted prior actual touch of ALL eight whole chunks is
 * mandatory. Not native admission/residency proof or an alternative ready flag.
 * Refusal leaves the entire ledger unchanged; caller retains owner/backing. */
int nh_memory_credit_span_hub(struct nh_memory_ledger *,const struct camera_span_hub_v1 *);
int nh_memory_credit_codec(struct nh_memory_ledger *,uint64_t,unsigned);
int nh_memory_evaluate(const struct nh_memory_ledger *,const struct nh_memory_values *,
 const struct nh_memory_values *,const struct nh_memory_zones *,uint64_t,uint64_t,enum nh_memory_stage,struct nh_memory_observation *);
int nh_memory_own_layout(uint64_t *,uint64_t *,unsigned);
#endif
