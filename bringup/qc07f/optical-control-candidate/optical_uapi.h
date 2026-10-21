/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
#ifndef QC07F_OPTICAL_UAPI_H
#define QC07F_OPTICAL_UAPI_H
#include <linux/types.h>
#include <linux/ioctl.h>

#define QO_ABI 1
#define QO_STATUS _IOR('Q', 0x60, struct qo_status)
#define QO_SETUP  _IO('Q', 0x61)
#define QO_PULSE_A _IO('Q', 0x62)
#define QO_PULSE_B _IO('Q', 0x63)
#define QO_IR_ON _IO('Q', 0x64)
#define QO_IR_OFF _IO('Q', 0x65)
#define QO_CLOSE _IO('Q', 0x66)

#define QO_FAULT_CONFIG 1U
#define QO_FAULT_READBACK 2U
#define QO_FAULT_GPIO 4U

/* All fields fixed width, no pointers or implicit padding. Pair: bit0=GPIO6,
 * bit1=GPIO7. 'A'/'B' never assert optical position. Zero release_verified
 * means NOT verified. Snapshot is not an authorization token or boot identity.
 */
struct qo_status {
	__u32 abi, setup, fault, owned_mask;
	__u32 pair_latch, pair_pad, ir_latch, ir_pad;
	__u32 direction_mask, mux_ok, release_verified, ir_active;
	__u32 ir_remaining_ms, cooldown_remaining_ms, pulse_us, ir_limit_ms;
	__u64 sequence, monotonic_ns;
};
#endif
