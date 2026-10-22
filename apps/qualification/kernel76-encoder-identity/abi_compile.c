/* SPDX-License-Identifier: GPL-2.0-only
 * Translation-unit ABI declarations only, no main, fixtures or runtime tests.
 */
#include <stddef.h>
#include "qc07f_hwenc.h"
#include "stage-telemetry-uapi.h"
_Static_assert(sizeof(struct qc_hwenc_open_query)==88,"new fixed ABI");
_Static_assert(_Alignof(struct qc_hwenc_open_query)==8,"aligned numeric ABI");
_Static_assert(offsetof(struct qc_hwenc_open_query,session_identity)==32,"identity offset");
_Static_assert(offsetof(struct qc_hwenc_open_query,operation_sequence)==64,"operation offset");
_Static_assert(offsetof(struct qc_hwenc_open_query,reserved)==84,"reserved offset");
_Static_assert(_IOC_SIZE(QC_HWENC_OPEN_QUERY)==88,"query command size");
_Static_assert(_IOC_DIR(QC_HWENC_OPEN_QUERY)==_IOC_READ,"output only");
_Static_assert(_IOC_NR(QC_HWENC_OPEN_QUERY)==0x66,"encoder local number");
_Static_assert(sizeof(struct qc_hwenc_info)==32,"legacy INFO");
_Static_assert(sizeof(struct qc_hwenc_job)==160,"legacy JPEG");
_Static_assert(sizeof(struct qc_hwenc_h264_job)==32,"legacy IDR");
_Static_assert(sizeof(struct qc_hwenc_h264_stream_job)==40,"legacy STREAM");
_Static_assert(sizeof(struct qc_hwenc_lifetime)==56,"legacy lifetime");
_Static_assert(QC_HWENC_OPEN_QUERY!=QC_HWENC_STAGE_QUERY,"stage command distinct");
_Static_assert(QC_HWENC_OPEN_QUERY!=QC_HWENC_LIFETIME_QUERY,"lifetime command distinct");
const unsigned long qc_encoder_identity_compile_commands[]={
 QC_HWENC_INFO,QC_HWENC_JPEG,QC_HWENC_H264_IDR,QC_HWENC_H264_STREAM,
 QC_HWENC_LIFETIME_QUERY,QC_HWENC_STAGE_QUERY,QC_HWENC_OPEN_QUERY
};
