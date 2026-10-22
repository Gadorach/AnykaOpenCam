/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_LISTENER_CUSTODY_INTERNAL_H
#define CAMERA_RTSP_LISTENER_CUSTODY_INTERNAL_H
#include "owned_listener_v1.h"
#include <sys/stat.h>
/* Internal snapshot only; not an ownership/admission capability. */
struct owned_listener_observation {int fd;struct stat identity;};
int owned_listener_observe(int,uint16_t,struct owned_listener_observation *);
int owned_listener_move(int *,uint16_t,const struct owned_listener_observation *,
 int *,struct camera_rtsp_owned_listener_outcome_v1 *);
int owned_listener_close_once(int *,struct camera_rtsp_owned_listener_outcome_v1 *);
#endif
