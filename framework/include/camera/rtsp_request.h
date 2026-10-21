/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_REQUEST_H
#define CAMERA_RTSP_REQUEST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Frozen parser/admission profile; not measured target service capacity. */
#define CAMERA_RTSP_REQUEST_MAX_BYTES 8192U
#define CAMERA_RTSP_REQUEST_MAX_METHOD 32U
#define CAMERA_RTSP_REQUEST_MAX_TARGET 512U
#define CAMERA_RTSP_REQUEST_MAX_HEADERS 24U
#define CAMERA_RTSP_REQUEST_MAX_HEADER_NAME 64U
#define CAMERA_RTSP_REQUEST_MAX_HEADER_VALUE 1024U
#define CAMERA_RTSP_REQUEST_MAX_BODY 4096U
#define CAMERA_RTSP_REQUEST_MAX_SESSION 128U
#define CAMERA_RTSP_REQUEST_MAX_SESSION_TOKEN 64U
#define CAMERA_RTSP_REQUEST_MAX_TRANSPORT 512U
#define CAMERA_RTSP_REQUEST_MAX_AUTHORIZATION 1024U

enum camera_rtsp_parse_result {
 CAMERA_RTSP_INVALID=-1, CAMERA_RTSP_NEED_MORE=0, CAMERA_RTSP_READY=1
};

/* Borrowed binary spans, NOT NUL-terminated strings. No normalization occurs. */
struct camera_rtsp_span { const unsigned char *data; size_t length; };
struct camera_rtsp_header {
 struct camera_rtsp_span name,value; /* value excludes surrounding SP only */
};
struct camera_rtsp_request {
 size_t consumed; /* exact first request bytes, including body, on READY only */
 struct camera_rtsp_span method,target,body;
 size_t header_count;
 struct camera_rtsp_header headers[CAMERA_RTSP_REQUEST_MAX_HEADERS];
 uint32_t cseq,content_length,session_timeout;
 unsigned int has_content_length,has_session_timeout,target_is_star;
 struct camera_rtsp_span session,session_token,transport;
 /* Opaque SECRET-SENSITIVE span: never log, echo, CSV-split or treat as auth. */
 struct camera_rtsp_span authorization;
};

/* Incremental caller-buffer API: retain an incomplete prefix and append bytes,
 * then call again. No internal state, heap allocation, I/O, callbacks or effects.
 * length must be <= MAX_BYTES; buffer NULL is allowed only for length=0.
 * output is REQUIRED, disjoint from buffer, and unchanged on NEED_MORE/INVALID.
 * Every READY span borrows buffer; finish all use BEFORE append, move, reuse,
 * erase or free. Caller owns restricted receive storage and secure zeroization.
 * Parse only one request. Caller retires consumed bytes after using the views.
 * '$' at a request boundary is INVALID: separately demultiplex bounded binary
 * interleaved frames only under an established/authorized transport contract.
 * Body bytes are opaque (including '$'/CRLF/NUL), exactly Content-Length bytes.
 * READY does NOT authenticate, authorize, dispatch or validate Transport syntax.
 */
enum camera_rtsp_parse_result camera_rtsp_request_parse(
 const void *buffer,size_t length,struct camera_rtsp_request *output);

#ifdef __cplusplus
}
#endif
#endif
