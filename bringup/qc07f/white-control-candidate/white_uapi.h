/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
#ifndef QC07F_WHITE_UAPI_H
#define QC07F_WHITE_UAPI_H
#include <linux/types.h>
#include <linux/ioctl.h>
#define QW_ABI 1U
#define QW_STATUS _IOR('Q', 0x70, struct qw_status)
#define QW_OFF _IO('Q', 0x71)
#define QW_ON _IO('Q', 0x72)
#define QW_LIMIT_MS 4900U
#define QW_LIMIT_NS 4900000000ULL
#define QW_FAULT_CONFIG 1U
#define QW_FAULT_READBACK 2U
#define QW_FAULT_GPIO 4U
#define QW_SETUP 1U
#define QW_ACTIVE 2U
#define QW_PRIOR_VALID 4U
#define QW_OFF_VERIFIED 8U
/* 64 bytes, no pointers/padding. Flags above. Values describe GPIO5 ONLY.
 * Prior pad/direction are sampled immediately before the latest admitted OFF.
 * Meaningful only with PRIOR_VALID. Snapshot is not boot admission or a lease.
 */
struct qw_status {
	__u32 abi, flags, fault, owned;
	__u32 white_latch, white_pad, direction, mux_ok;
	__u32 remaining_ms, limit_ms, prior_pad, prior_direction;
	__u64 sequence, monotonic_ns;
};
#endif
