/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_ENCODER_OUTPUT_SPANS_H
#define QC_ENCODER_OUTPUT_SPANS_H
#include <linux/types.h>
#include <linux/ioctl.h>

#define QC_HWENC_OUTPUT_SPANS_VERSION 1U
#define QC_HWENC_OUTPUT_SPANS_COUNT 8U
#define QC_HWENC_OUTPUT_SPAN_BYTES 131072U

struct qc_hwenc_output_span {
 __u32 user_address,bytes;
};
/* CPU-copy destinations, NOT DMA/physical addresses or a user-MMIO interface.
 * Caller supplies stable ordinary CPU memory and serializes mapping/alias use
 * through the whole ioctl. access_ok does not reserve pages or prove aliasing.
 * Exactly eight full128KiB capacities, mutually virtual-disjoint and disjoint
 * from the complete raw input and this complete control request. No tail copy.
 * All output fields and reserved words MUST be zero on input. On syscall0 only,
 * output_bytes/status/frame_num describe delivery. Any error invalidates ALL
 * output, even complete-looking partial data/metadata. No retry/repair; a failed
 * actual job loses its logical P reference and requires a newly admitted IDR.
 * Preflight-only rejection preserves the old owner. No errno proves retirement.
 * Old kernels: ENOTTY, no fallback to contiguous or another command.
 */
struct qc_hwenc_h264_spans_job {
 __u32 version,struct_bytes,keyframe,span_count;
 __u32 width,height,input_bytes,input_ptr;
 struct qc_hwenc_output_span spans[QC_HWENC_OUTPUT_SPANS_COUNT];
 __u32 output_bytes,status,frame_num;
 __u32 reserved[5];
};
#define QC_HWENC_H264_STREAM_SPANS _IOWR('Q',0x67,struct qc_hwenc_h264_spans_job)
#endif
