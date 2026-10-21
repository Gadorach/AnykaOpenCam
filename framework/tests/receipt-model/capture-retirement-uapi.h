/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_CAPTURE_RETIREMENT_UAPI_H
#define QC_CAPTURE_RETIREMENT_UAPI_H
#include <linux/types.h>
#include <linux/ioctl.h>
#define QC_CAPTURE_RECEIPT_VERSION 1U
enum qc_capture_receipt_state {
 QC_RECEIPT_NONE, QC_RECEIPT_ARMED, QC_RECEIPT_STARTING, QC_RECEIPT_ACTIVE,
 QC_RECEIPT_DRAINING, QC_RECEIPT_RETIRED, QC_RECEIPT_UNCERTAIN
};
enum qc_capture_stop_reason {QC_STOP_NONE, QC_STOP_END, QC_STOP_EXPIRY, QC_STOP_ERROR, QC_STOP_CLOSE};
/* Proposed additive, fixed-size UAPI. Boot/source/file/generation identities are
 * counters backed by retained objects, NOT pointers, FD numbers or auth tokens.
 * Input: version1, exact key (zero key for BIND), all output/reserved words zero.
 * ARM requires bound source/file, generation0; output is a RESERVED generation,
 * never a claim of DMA retirement. Only QUERY state RETIRED / stop_result0 after
 * a successful syscall is proof of the separately executed stop/reset boundary.
 * Do not inspect a partially copied reply after any nonzero syscall result.
 */
struct qc_capture_receipt {
 __u32 version,reserved;
 __aligned_u64 source_instance,file_instance,generation;
 __u32 state,reason;
 __s32 stop_result;
 __u32 reserved2;
 __aligned_u64 stopped_ns;
};
/* Proposed numbers, not installed in the live kernel72 header. */
#define QC_ISP_RECEIPT_BIND  _IOWR('I',0x81,struct qc_capture_receipt)
#define QC_ISP_RECEIPT_ARM   _IOWR('I',0x82,struct qc_capture_receipt)
#define QC_ISP_RECEIPT_QUERY _IOWR('I',0x83,struct qc_capture_receipt)
#define QC_ISP_RECEIPT_ACK   _IOW('I',0x84,struct qc_capture_receipt)
#endif
