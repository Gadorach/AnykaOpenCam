/* SPDX-License-Identifier: GPL-2.0-only
 * Complete source replacement of the original adapter header. Selecting the
 * accepted memory/handoff definitions changes layouts: recompile consumers.
 */
#ifndef QC_NATIVE_ADAPTER_H
#define QC_NATIVE_ADAPTER_H
#include "../../qc07f-span-memory/handoff.h"
void nh_native_init(void);
int nh_native_optin_exact139(void);
int nh_native_storage_select(const char *);
int nh_native_memory_start(uint64_t);
/* Legacy flat hub admission is permanently refused, without any effect. */
int nh_native_memory_fixed_buffers(void *);
int nh_native_memory_gate(void *,enum nh_memory_stage);
void nh_native_memory_report(void);
void nh_native_sensor_report(void);
int nh_native_next(void *,struct camera_packet *,struct nh_key *);
/* Same actual owned guard/check/disable path; force must be 0 or 1. */
int nh_native_next_keyframe(void *,uint32_t,struct camera_packet *,struct nh_key *);
int nh_native_release(void *,const struct camera_packet *);
int nh_native_retire(void *);
int nh_native_retire_held(void *,const struct camera_packet *);
uint64_t nh_native_now(void *);
int nh_native_cancelled(void *);
int nh_native_config(struct camera_fmp4_config *);
void nh_native_cancel(int);
void nh_native_fence(void *); /* logical sticky quarantine, no hardware operation */
_Noreturn void nh_native_quarantine(void); /* process survival only; NOT DMA stop */
#endif
