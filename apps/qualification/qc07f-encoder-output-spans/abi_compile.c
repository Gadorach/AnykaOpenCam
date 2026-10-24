/* SPDX-License-Identifier: GPL-2.0-only
 * Declarations/static assertions only: no main, fixture or runtime call.
 */
#include <stddef.h>
#include "qc07f_hwenc.h"
#include "stage-telemetry-uapi.h"
#define OFFSET(field,value) _Static_assert(offsetof(struct qc_hwenc_h264_spans_job,field)==value,#field)
_Static_assert(sizeof(struct qc_hwenc_output_span)==8,"fixed descriptor size");
_Static_assert(_Alignof(struct qc_hwenc_output_span)==4,"descriptor alignment");
_Static_assert(offsetof(struct qc_hwenc_output_span,user_address)==0,"address offset");
_Static_assert(offsetof(struct qc_hwenc_output_span,bytes)==4,"capacity offset");
_Static_assert(sizeof(struct qc_hwenc_h264_spans_job)==128,"fixed request size");
_Static_assert(_Alignof(struct qc_hwenc_h264_spans_job)==4,"no u64/native-pointer ambiguity");
OFFSET(version,0);OFFSET(struct_bytes,4);OFFSET(keyframe,8);OFFSET(span_count,12);
OFFSET(width,16);OFFSET(height,20);OFFSET(input_bytes,24);OFFSET(input_ptr,28);
OFFSET(spans,32);OFFSET(output_bytes,96);OFFSET(status,100);OFFSET(frame_num,104);OFFSET(reserved,108);
_Static_assert(sizeof(((struct qc_hwenc_h264_spans_job *)0)->spans)==64,"eight descriptors");
_Static_assert(QC_HWENC_OUTPUT_SPANS_COUNT==8,"fixed count");
_Static_assert(QC_HWENC_OUTPUT_SPAN_BYTES==131072,"per-range cap");
_Static_assert(QC_HWENC_OUTPUT_SPANS_COUNT*QC_HWENC_OUTPUT_SPAN_BYTES==1048576,"exact capacity");
_Static_assert(_IOC_SIZE(QC_HWENC_H264_STREAM_SPANS)==128,"full encoded size");
_Static_assert(_IOC_TYPE(QC_HWENC_H264_STREAM_SPANS)=='Q',"encoder type");
_Static_assert(_IOC_NR(QC_HWENC_H264_STREAM_SPANS)==0x67,"new local number");
_Static_assert(_IOC_DIR(QC_HWENC_H264_STREAM_SPANS)==(_IOC_READ|_IOC_WRITE),"bidirectional");
_Static_assert(QC_HWENC_H264_STREAM_SPANS==0xc0805167U,"exact new full command");
_Static_assert(sizeof(struct qc_hwenc_info)==32,"legacy INFO");
_Static_assert(sizeof(struct qc_hwenc_job)==160,"legacy JPEG");
_Static_assert(sizeof(struct qc_hwenc_h264_job)==32,"legacy IDR");
_Static_assert(sizeof(struct qc_hwenc_h264_stream_job)==40,"legacy STREAM");
_Static_assert(sizeof(struct qc_hwenc_lifetime)==56,"legacy lifetime");
_Static_assert(sizeof(struct qc_stage_query)==296,"legacy stage");
_Static_assert(sizeof(struct qc_hwenc_open_query)==88,"preserved open query");
_Static_assert(QC_HWENC_INFO==0x80205161U,"exact legacy INFO");
_Static_assert(QC_HWENC_JPEG==0xc0a05162U,"exact legacy JPEG");
_Static_assert(QC_HWENC_H264_IDR==0xc0205163U,"exact legacy IDR");
_Static_assert(QC_HWENC_H264_STREAM==0xc0285164U,"exact legacy STREAM");
_Static_assert(QC_HWENC_LIFETIME_QUERY==0x80385165U,"exact legacy lifetime");
_Static_assert(QC_HWENC_STAGE_QUERY==0xc1285165U,"same number, different full command");
_Static_assert(QC_HWENC_OPEN_QUERY==0x80585166U,"preserved exact open query");
const unsigned long qc_encoder_spans_compile_commands[]={
 QC_HWENC_INFO,QC_HWENC_JPEG,QC_HWENC_H264_IDR,QC_HWENC_H264_STREAM,
 QC_HWENC_LIFETIME_QUERY,QC_HWENC_STAGE_QUERY,QC_HWENC_OPEN_QUERY,QC_HWENC_H264_STREAM_SPANS
};
