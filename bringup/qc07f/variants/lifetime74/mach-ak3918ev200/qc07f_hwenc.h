/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_HWENC_UAPI_H
#define QC07F_HWENC_UAPI_H
#include <linux/types.h>
#include <linux/ioctl.h>
struct qc_hwenc_info {
 __u32 id,config,status,control,input_dma,output_dma,guard_errors,last_bytes;
};
struct qc_hwenc_job {
 __u32 width,height,input_bytes,input_ptr,output_capacity,output_ptr,output_bytes,status;
 __u8 quant[128]; /* canonical natural8x8 luma/chroma values, never addresses */
};
#define QC_HWENC_INFO _IOR('Q',0x61,struct qc_hwenc_info)
#define QC_HWENC_JPEG _IOWR('Q',0x62,struct qc_hwenc_job)
/* First bounded profile:1280x720 baseline/CAVLC, QP26, independent IDR.
 * No register values, physical addresses, user-specified headers or refs.
 */
struct qc_hwenc_h264_job {
 __u32 width,height,input_bytes,input_ptr,output_capacity,output_ptr,output_bytes,status;
};
#define QC_HWENC_H264_IDR _IOWR('Q',0x63,struct qc_hwenc_h264_job)
struct qc_hwenc_h264_stream_job {
 struct qc_hwenc_h264_job frame;
 __u32 keyframe,frame_num; /* keyframe0/1 input; frame_num is driveroutput */
};
#define QC_HWENC_H264_STREAM _IOWR('Q',0x64,struct qc_hwenc_h264_stream_job)
/* Additive, observational boot-lifetime status. No addresses or drain/reset. */
struct qc_hwenc_lifetime {
 __u32 version,struct_bytes,quarantined,dma_uncertain;
 __u32 guard_poisoned,failed_reset_step;
 __s32 reset_error;
 __u32 reserved;
 __u64 fault_session,session_sequence,live_bytes;
};
#define QC_HWENC_LIFETIME_QUERY _IOR('Q',0x65,struct qc_hwenc_lifetime)
#endif
