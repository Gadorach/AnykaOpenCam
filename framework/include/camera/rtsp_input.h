/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_INPUT_H
#define CAMERA_RTSP_INPUT_H
#include <camera/rtsp_request.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CAMERA_RTSP_RTCP_MAX_BYTES 1024U
#define CAMERA_RTSP_REQUEST_DEADLINE_DEFAULT_MS 2000U
enum camera_rtsp_input_result {
 CAMERA_RTSP_INPUT_INVALID=-1,CAMERA_RTSP_INPUT_NEED_MORE=0,
 CAMERA_RTSP_INPUT_REQUEST=1,CAMERA_RTSP_INPUT_RTCP=2
};
enum camera_rtsp_input_failure {
 CAMERA_RTSP_INPUT_NONE=0,CAMERA_RTSP_INPUT_FRAMING,CAMERA_RTSP_INPUT_CLOCK,
 CAMERA_RTSP_INPUT_DEADLINE,CAMERA_RTSP_INPUT_CHANNEL,CAMERA_RTSP_INPUT_RTCP_FORMAT,
 CAMERA_RTSP_INPUT_RESOURCE,CAMERA_RTSP_INPUT_SESSION,CAMERA_RTSP_INPUT_TRANSPORT,
 CAMERA_RTSP_INPUT_STATE,CAMERA_RTSP_INPUT_BODY,CAMERA_RTSP_INPUT_IO
};
enum camera_rtsp_method {
 CAMERA_RTSP_METHOD_UNSUPPORTED=0,CAMERA_RTSP_METHOD_OPTIONS,
 CAMERA_RTSP_METHOD_DESCRIBE,CAMERA_RTSP_METHOD_SETUP,CAMERA_RTSP_METHOD_PLAY,
 CAMERA_RTSP_METHOD_PAUSE,CAMERA_RTSP_METHOD_GET_PARAMETER,CAMERA_RTSP_METHOD_TEARDOWN
};
/* Two exact configured URI entries. NOT paths into a filesystem. Freeze while
 * serving. No decoding/canonicalization; Digest must use request.target itself. */
struct camera_rtsp_registry {
 char live_uri[CAMERA_RTSP_REQUEST_MAX_TARGET+1U];
 char track_uri[CAMERA_RTSP_REQUEST_MAX_TARGET+1U];
};
struct camera_rtsp_input {
 unsigned char data[CAMERA_RTSP_REQUEST_MAX_BYTES];size_t used;
 uint64_t started_ms,last_ms;uint32_t deadline_ms;
 unsigned int deadline_active;
 enum camera_rtsp_input_failure failure;
};
int camera_rtsp_registry_init(struct camera_rtsp_registry *,const char *,const char *);
int camera_rtsp_input_init(struct camera_rtsp_input *,uint64_t,uint32_t);
int camera_rtsp_input_tick(struct camera_rtsp_input *,uint64_t);
int camera_rtsp_input_append(struct camera_rtsp_input *,const void *,size_t,uint64_t);
enum camera_rtsp_input_result camera_rtsp_input_next(struct camera_rtsp_input *,
 unsigned int lab_transport_selected,uint64_t,struct camera_rtsp_request *,size_t *);
/* All borrowed request views expire at retire/clear. Uses libc explicit_bzero,
 * not ordinary memset, for wire storage. Secret contents never enter metadata. */
int camera_rtsp_input_retire(struct camera_rtsp_input *,size_t,uint64_t);
void camera_rtsp_input_clear(struct camera_rtsp_input *);
enum camera_rtsp_input_failure camera_rtsp_request_policy(
 const struct camera_rtsp_request *,const struct camera_rtsp_registry *,
 unsigned int lab_transport_selected,unsigned int playing,const char *session,
 enum camera_rtsp_method *);
#ifdef __cplusplus
}
#endif
#endif
