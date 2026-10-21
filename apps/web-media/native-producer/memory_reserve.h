/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NH_MEMORY_RESERVE_H
#define NH_MEMORY_RESERVE_H
#include <stddef.h>
#include <stdint.h>
#define NH_MEMINFO_CAP 4096U
#define NH_ZONEINFO_CAP 16384U
#define NH_MEMORY_MAX_SAMPLE_NS 100000000ULL
#define NH_MEMORY_OVERHEAD_KIB 512ULL
#define NH_MEMORY_RESERVE_KIB 1024ULL
#define NH_MEMORY_CODEC_KIB 8256ULL
#define NH_MEMORY_MIN_CPU_KIB 3832ULL
#define NH_MEMORY_RAW_REUSE_MIN_CPU_KIB 2804ULL
enum nh_memory_stage {NH_MEM_INITIAL,NH_MEM_FIXED_PAID,NH_MEM_PRE_NEXT,NH_MEM_POST_FIRST};
enum nh_memory_reason {NH_MEM_OK,NH_MEM_FORMAT,NH_MEM_LAYOUT,NH_MEM_STALE,NH_MEM_CLOCK,NH_MEM_LOW_AVAILABLE,NH_MEM_LOW_NONCMA,NH_MEM_CREDIT,NH_MEM_IO,NH_MEM_CANCEL};
struct nh_memory_values {uint64_t total,free,available,cma_total,cma_free,swap_total;};
struct nh_memory_zone {
 uint64_t free,boost,min,low,high,promo,spanned,present,managed,cma,protection[2],free_cma;
};
struct nh_memory_zones {struct nh_memory_zone normal,movable;uint64_t watermark_kib;};
struct nh_memory_ledger {
 uint64_t cpu_capacity_kib,cpu_paid_kib,nonmovable_cpu_debt_kib;
 uintptr_t begin[3],end[3];unsigned paid[3],codec_paid,initialized,raw_reuse;
};
struct nh_memory_observation {
 uint64_t begin_ns,end_ns,available,noncma_free,watermark_kib,cpu_debt,dma_debt,required_available,required_noncma;
 uint64_t migration_future_kib;
 enum nh_memory_stage stage;enum nh_memory_reason reason;
};
int nh_memory_parse_meminfo(const char *,size_t,struct nh_memory_values *);
int nh_memory_parse_zoneinfo(const char *,size_t,struct nh_memory_zones *);
int nh_memory_init(struct nh_memory_ledger *,uint64_t,uint64_t);
int nh_memory_init_mode(struct nh_memory_ledger *,uint64_t,uint64_t,unsigned);
int nh_memory_credit_owned(struct nh_memory_ledger *,unsigned,const void *,size_t);
int nh_memory_credit_codec(struct nh_memory_ledger *,uint64_t,unsigned);
int nh_memory_evaluate(const struct nh_memory_ledger *,const struct nh_memory_values *,
 const struct nh_memory_values *,const struct nh_memory_zones *,uint64_t,uint64_t,enum nh_memory_stage,struct nh_memory_observation *);
int nh_memory_own_layout(uint64_t *,uint64_t *,unsigned); /* explicit raw-reuse vs separate mapping */
#endif
