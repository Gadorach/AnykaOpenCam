/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_ENCODER_OPEN_QUERY_H
#define QC_ENCODER_OPEN_QUERY_H
#include <linux/types.h>
#include <linux/ioctl.h>

#define QC_HWENC_OPEN_QUERY_VERSION 1U
enum qc_hwenc_reference_relation {
 QC_HWENC_REFERENCE_NONE=0,QC_HWENC_REFERENCE_THIS_OPEN=1,
 QC_HWENC_REFERENCE_OTHER_OPEN=2
};
/* Additive output-only snapshot for the exact encoder open description.
 * No input flag/grant, address, device label, payload or hardware access.
 * Callers MUST require syscall0/version1/struct_bytes88; ENOTTY means unsupported,
 * never fall back to global session_sequence as this open's identity.
 *
 * Numeric session identities are boot-lifetime/nonreused, assigned on successful
 * open. dup/fork share an open identity. This does NOT prove exclusive custody,
 * task binding, present authorization, DMA retirement or safe close/free.
 *
 * Reference generation and operation sequence are EXISTING observational stage
 * counters: saturating, not leases/nonces. telemetry_flags has the existing
 * QC_STAGE_F_* bits; SATURATED invalidates uniqueness/increment assumptions.
 * reference_frame_num is the current owner's codec frame number (wraps16 bits),
 * NOT an operation identity. Relation NONE makes owner identity/frame number0.
 *
 * Query takes enc.lock, so it waits behind synchronous jobs. ledger_active is
 * the actual stage ledger flag, normally0 here, NOT hardware idle/availability.
 * last_dispatch_identity is the last nonquery command's file identity (including
 * INFO/denied BUSY); it is historical and can refer to a closed description.
 * last operation kind/result are global history, not this caller's completion.
 * operation_sequence0 means no recorded command. Queries do not update counters.
 */
struct qc_hwenc_open_query {
 __u32 version,struct_bytes,reference_relation,reference_frame_num;
 __u32 quarantined,dma_uncertain,guard_poisoned,telemetry_flags;
 __aligned_u64 session_identity,reference_owner_identity,reference_generation;
 __aligned_u64 last_dispatch_identity,operation_sequence;
 __s32 last_operation_result;
 __u32 last_operation_kind,ledger_active,reserved;
};
/* Candidate encoder-local command: existing exact0x61–0x65 commands unchanged.
 * Not installed or supported by the frozen kernel76 image. Full command (size,
 * direction/type/number) comparison makes old kernels return ENOTTY. */
#define QC_HWENC_OPEN_QUERY _IOR('Q',0x66,struct qc_hwenc_open_query)
#endif
