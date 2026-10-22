/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QP_OWNED_PROVIDER_TYPES_H
#define QP_OWNED_PROVIDER_TYPES_H
#include "frame_fd_custody.h"
#include "encoder_fd_custody.h"
#include "stage-telemetry-uapi.h"
#include <sys/stat.h>
enum qp_phase {QP_EMPTY,QP_MOVING,QP_COMMITTED,QP_ACTIVE,QP_CLOSED,QP_HOLD,QP_RETURNING,QP_RETURNED};
struct qp_roles {
 enum qp_phase phase; /* transfer/source history, NEVER authorization */
 int sensor_staged;
 struct qc_frame_fd_custody frame_staged;
 struct qc_encoder_fd_custody encoder_staged;
 dev_t sensor_device,sensor_filesystem,frame_filesystem,encoder_filesystem;
 ino_t sensor_inode,frame_inode,encoder_inode;
 struct qc_frame_fd_identity frame_identity;
 struct qc_encoder_fd_identity encoder_identity;
 struct qc_capture_receipt frame_last;
 struct qc_hwenc_open_query encoder_last;
 struct qc_hwenc_lifetime global_last;
 uint32_t codec_frame_num;
};
#define QP_ROLES_EMPTY { .sensor_staged=-1, .frame_staged=QC_FRAME_FD_CUSTODY_EMPTY, .encoder_staged=QC_ENCODER_FD_CUSTODY_EMPTY }
#endif
