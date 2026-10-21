/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_ISP_STREAM_H
#define QC07F_ISP_STREAM_H
#include <linux/types.h>
#include <linux/ioctl.h>
/* Exclusive gated continuousRGB input; not zero-copy/V4L2 or a free-running
 * ring. All addresses are userspace destinations, never physical DMA values.
 */
struct qc_isp_next {
 __u32 bytes,user_ptr,sequence,status,guard_errors,crc32_bzip2;
};
#define QC_ISP_BEGIN _IO('I',0x71)
#define QC_ISP_NEXT _IOWR('I',0x72,struct qc_isp_next)
#define QC_ISP_END _IO('I',0x73)
/* Idle-only measuredretailYUV-effect UVgain presets, no illumination/GPIO. */
struct qc_isp_colour {__u32 monochrome,uv_gain_readback;};
#define QC_ISP_COLOUR _IOWR('I',0x74,struct qc_isp_colour)
#define QC_ISP_BEGIN_IRQ _IO('I',0x77)
struct qc_isp_ring_test {
 __u32 frames,slot,user_ptr,bytes,completed,irq_status[16],irq_control[16],irq_elapsed_ms[16],slot_crc[4],slot_guards[4];
 __u32 stat_guards[4],main_changed[4],sub_changed[4];
 /* Explicit bounded held-slot diagnostic, never a DMA physical-address API. */
 __u32 hold_selected,held_pre_crc,held_post_crc,held_final_crc,held_shadow_crc;
 /* IRQ progress spans the pre-CRC/copy/post-CRC bracket, not bare memcpy. */
 __u32 held_bracket_start_events,held_bracket_end_events,held_active_bracket;
 /* Negative diagnostic changes only CPU shadow, never sensor/DMA pixels. */
 __u32 corrupt_shadow;
};
#define QC_ISP_RING_TEST _IOWR('I',0x78,struct qc_isp_ring_test)
/* Experimental copied-frame queue. No exported mappings or physical-address
 * input. At most one source lease; callers validate delivered CRC and RETURN
 * its exact token before NEXT. STOP invalidates tokens and retires DMA.
 */
struct qc_isp_queue_frame {
 __u64 token;
 __u32 bytes,user_ptr,slot,status,guard_errors,crc32_bzip2,sequence,irq_events,dropped;
};
#define QC_ISP_QUEUE_BEGIN _IO('I',0x79)
#define QC_ISP_QUEUE_NEXT _IOWR('I',0x7a,struct qc_isp_queue_frame)
#define QC_ISP_QUEUE_RETURN _IOW('I',0x7b,struct qc_isp_queue_frame)
#define QC_ISP_QUEUE_END _IO('I',0x7c)
/* Idle-only WB comparison, not AWB or arbitrary registers. GET reports desired
 * next-table fields, never claims live MMIO readback. set=1 changes RAM state;
 * preset=0 restores snapshot,1=retained first SC1346 pack's manual fields.
 * GET may report CUSTOM (2); legacy SET still accepts only 0/1.
 */
struct qc_isp_whitebalance {__u32 set,preset,gain[3],offset_raw[3];};
#define QC_ISP_WHITEBALANCE _IOWR('I',0x7d,struct qc_isp_whitebalance)
#define QC_ISP_WB_SNAPSHOT 0U
#define QC_ISP_WB_RETAINED_MANUAL 1U
#define QC_ISP_WB_CUSTOM 2U
#define QC_ISP_WB_MANUAL_VERSION 1U
/* Idle-only RAM desired configuration. GET remains available with any owner.
 * SET gains=1..4095, offset_raw=0..1023; signed semantics NOT qualified.
 * Inputs: version=1, set=0/1, reserved=0; gains/offsets used only by SET.
 * All other fields are output. last_prepared_* are valid only when
 * table_prepared=1: RAM table metadata, NOT applied/live MMIO confirmation.
 * Timestamps are CLOCK_MONOTONIC nanoseconds since this kernel boot.
 * camera_id=2 is the stable QC07F inventory ID. No register/address API.
 */
struct qc_isp_whitebalance_manual {
 __u32 version,set,gain[3],offset_raw[3],preset;
 __u32 table_prepared,last_prepared_preset;
 __u32 last_prepared_gain[3],last_prepared_offset_raw[3];
 __u32 camera_id,owner_active,reserved;
 __aligned_u64 desired_changed_ns,last_prepared_ns;
};
#define QC_ISP_WHITEBALANCE_MANUAL _IOWR('I',0x7e,struct qc_isp_whitebalance_manual)
#endif
