/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTP_H264_H
#define CAMERA_RTP_H264_H
#include <camera/backend.h>
struct camera_rtp_session {
 uint32_t ssrc;
 uint16_t sequence;
 uint8_t channel,payload_type;
};
typedef int (*camera_rtp_send)(void *context,const uint8_t *data,size_t bytes);
#define CAMERA_RTP_PACKET_BYTES 1404U
/* Build one packet without changing session/offset. A nonblocking sender keeps
 * this owned packet through partial writes, then commits next_offset and
 * advances sequence once. Initial offset=0 for both single-NAL and FU-A.
 */
int camera_rtp_h264_packet(const struct camera_rtp_session *session,uint32_t timestamp,
 const uint8_t *nal,size_t bytes,uint32_t last,size_t offset,
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],size_t *packet_bytes,size_t *next_offset);
/* One NAL without Annex-B prefix, TCP-interleaved RFC6184 mode1. Caller
 * supplies SPS/PPS before IDR and marks only the access unit's last NAL.
 * Session state is per consumer, not per sensor or shared codec engine.
 * Send callback consumes data synchronously; stack storage cannot be retained.
 */
int camera_rtp_h264_nal(struct camera_rtp_session *session,uint32_t timestamp,
 const uint8_t *nal,size_t bytes,uint32_t last,camera_rtp_send send,void *context);
#endif
