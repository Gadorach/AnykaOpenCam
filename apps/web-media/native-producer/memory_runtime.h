/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NH_MEMORY_RUNTIME_H
#define NH_MEMORY_RUNTIME_H
#include "memory_reserve.h"
struct nh_memory_runtime {
 struct nh_memory_ledger ledger;
 struct nh_memory_observation observations[64];unsigned count; /* finite forecast points for13 AUs, not a frame/deadline ceiling change */
 char meminfo[NH_MEMINFO_CAP],zoneinfo[NH_ZONEINFO_CAP];
 uint64_t session_start;
 uint64_t (*now)(void *);int (*cancelled)(void *);void *context;
};
int nh_memory_read_fixed(unsigned,char *,size_t,size_t *); /* ONLY meminfo/zoneinfo */
int nh_memory_runtime_init(struct nh_memory_runtime *,uint64_t,
 uint64_t (*)(void *),int (*)(void *),void *,unsigned);
int nh_memory_runtime_gate(struct nh_memory_runtime *,enum nh_memory_stage,uint64_t);
int nh_memory_prefault_owned(struct nh_memory_runtime *,unsigned,void *,size_t,uint64_t);
void nh_memory_log(const struct nh_memory_runtime *);
#endif
