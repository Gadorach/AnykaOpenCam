/* SPDX-License-Identifier: GPL-2.0-only
 * Derivative of framework/include/camera/rtp_h264.h; see LICENSE_NOTICES.md.
 */
#ifndef CAMERA_RTP_H264_SPANS_V1_H
#define CAMERA_RTP_H264_SPANS_V1_H
#include <camera/rtp_h264.h>

#define CAMERA_RTP_H264_SPANS_V1_MAX 16U
#define CAMERA_RTP_H264_SPAN_V1_BYTES 131072U
#define CAMERA_RTP_H264_NAL_V1_BYTES 1048576U

struct camera_rtp_h264_span_v1 {const uint8_t *data;size_t size;};

/* One NAL without Annex-B prefix, borrowed anew on each call; nothing retained.
 * 1..16 nonempty spans, each <=128KiB; checked sum must equal total <=1MiB.
 * All pointers designate valid, stable, serialized, nonoverlapping memory:
 * descriptor array, source objects, session, packet and separate scalar outputs.
 * This does not validate addresses, mutable aliases, DMA or lease ownership.
 *
 * Same build-one-packet wire/offset rules as camera_rtp_h264_packet. Initial
 * offset=0; session and caller offset never changed. CAMERA_INVALID leaves ALL
 * outputs untouched. CAMERA_OK returns an owned packet <=1404 bytes and next
 * offset. Caller retains that packet through partial writes and commits offset
 * and sequence ONLY after the whole packet is transmitted. No ACK assumption.
 * The actual source lease must survive every future source read; this function
 * neither retains the array nor grants release/END/drain authority.
 */
int camera_rtp_h264_spans_packet_v1(const struct camera_rtp_session *session,
 uint32_t timestamp,const struct camera_rtp_h264_span_v1 *spans,size_t count,
 size_t total,uint32_t last,size_t offset,
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],size_t *packet_bytes,size_t *next_offset);
#endif
