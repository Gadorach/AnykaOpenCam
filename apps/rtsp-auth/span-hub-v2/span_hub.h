/* SPDX-License-Identifier: GPL-2.0-only
 * Derived from camera/packet_hub.h; see LICENSE_NOTICES.md.
 * V2 explicit source replacement ONLY: owner/view layout is NOT V1 binary ABI.
 * Never include/link both hub versions or reuse a V1 combined REL.
 */
#ifndef CAMERA_SPAN_HUB_V1_H
#define CAMERA_SPAN_HUB_V1_H
#include "../rtp-span-candidate/rtp_h264_spans.h"

#define CAMERA_SPAN_HUB_V1_CHUNKS 8U
#define CAMERA_SPAN_HUB_V1_CONSUMERS 4U
#define CAMERA_SPAN_HUB_V1_CHUNK_BYTES CAMERA_RTP_H264_SPAN_V1_BYTES

enum camera_span_hub_phase_v1 {
 CAMERA_SPAN_HUB_V1_FRESH=0, CAMERA_SPAN_HUB_V1_INITIALIZING,
 CAMERA_SPAN_HUB_V1_INIT_FAILED, CAMERA_SPAN_HUB_V1_ACTIVE,
 CAMERA_SPAN_HUB_V1_STOPPED, CAMERA_SPAN_HUB_V1_DISPOSING,
 CAMERA_SPAN_HUB_V1_DISPOSED
};

struct camera_span_hub_v1 {
 const struct camera_span_hub_v1 *self;
 enum camera_span_hub_phase_v1 phase;
 uint8_t *chunks[CAMERA_SPAN_HUB_V1_CHUNKS];
 uint64_t generation,monotonic_ns;
 /* One owner lifetime: issued never resets; active_token[c] iff acquired bit. */
 uint64_t issued_token,active_token[CAMERA_SPAN_HUB_V1_CONSUMERS];
 enum camera_codec codec;
 uint32_t keyframe,subscribers,need_keyframe,pending,acquired;
 size_t bytes;
 /* Actual allocation-call / successful-request history, not allocator overhead,
  * prefault residency, locked memory or native/process budget evidence. */
 uint32_t allocation_calls;
 size_t allocated_bytes,maximum_allocation;
};

struct camera_span_hub_input_v1 {
 enum camera_codec codec;
 uint32_t keyframe;
 uint64_t monotonic_ns;
 size_t bytes,span_count;
 const struct camera_rtp_h264_span_v1 *spans;
};

struct camera_span_hub_view_v1 {
 const struct camera_span_hub_v1 *owner;
 uint64_t lease_token,generation,monotonic_ns;
 enum camera_codec codec;
 uint32_t keyframe,consumer;
 size_t bytes,span_count;
 struct camera_rtp_h264_span_v1 spans[CAMERA_SPAN_HUB_V1_CHUNKS];
};

/* TRUSTED C API: initialize only a persistent true-fresh owner = {0}, once at
 * its final address; never copy/move/edit it. Caller serializes all operations,
 * with no signals/reentry/callbacks/fork. All caller objects and outputs are
 * valid, stable and disjoint from each other and from owner/chunks; input source
 * objects are mutually disjoint. No mutable aliases into chunks. Nonnull/self
 * checks are not hostile-pointer/plugin validation. No post-dispose use/calls.
 * Tokens identify acquisitions within this owner only, not authorization across
 * owners/processes/cameras. Retained V1 names/guard require exclusive replacement.
 *
 * Init stores each malloc result directly in its canonical slot. Failed init
 * retains partial known CPU chunks until explicit dispose, never auto-cleans or
 * retries. No native pointer/FD/DMA/source ownership is adopted by this hub.
 */
int camera_span_hub_init_v1(struct camera_span_hub_v1 *hub);
int camera_span_hub_subscribe_v1(struct camera_span_hub_v1 *hub,uint32_t consumer);
int camera_span_hub_resync_v1(struct camera_span_hub_v1 *hub,uint32_t consumer);
int camera_span_hub_unsubscribe_v1(struct camera_span_hub_v1 *hub,uint32_t consumer);
/* One H264 VCL NAL, type1/non-IDR or type5/IDR, keyframe exactly agrees.
 * Checked input spans: 1..16, each <=128KiB, exact total <=existing1MiB envelope.
 * OK always secures ONE actual owned copy. No eligible client -> NOT_READY.
 * BUSY may drop only unacquired backlog and set resync bits, never held payload.
 * Invalid input leaves all state untouched. Producer release/END stays governed
 * by its original actual receipt/lease contract, NOT by this module's CPU copy.
 */
int camera_span_hub_publish_v1(struct camera_span_hub_v1 *hub,
 const struct camera_span_hub_input_v1 *input);
/* Success issues a distinct nonzero token and canonical active token[consumer].
 * Checked UINT64_MAX refuses before output/state changes; no wrap/reuse/reset.
 * Publication generation is separate. Only exact release clears an active token.
 */
int camera_span_hub_acquire_v1(struct camera_span_hub_v1 *hub,uint32_t consumer,
 struct camera_span_hub_view_v1 *view);
int camera_span_hub_release_v1(struct camera_span_hub_v1 *hub,uint32_t consumer,
 uint64_t token,uint64_t generation);
/* Small view metadata borrows const chunks until release. Validate this exact
 * consumer-tagged view/canonical active token before EVERY deferred packet
 * build/send/effect under caller serialization.
 * STOP invalidates effects immediately but old exact releases still drain it.
 */
int camera_span_hub_lease_valid_v1(const struct camera_span_hub_v1 *hub,
 uint32_t consumer,const struct camera_span_hub_view_v1 *view);
/* Actual production connection: rechecks lease then calls the accepted unchanged
 * span packetizer. No send, sequence/offset commit, callbacks or cached cursor.
 * Caller retains the <=1404B packet through partial writes; checks lease before
 * each send and commits sequence/offset only after complete packet transmission.
 */
int camera_span_hub_packet_v1(const struct camera_span_hub_v1 *hub,uint32_t consumer,
 const struct camera_span_hub_view_v1 *view,const struct camera_rtp_session *session,
 uint32_t timestamp,uint32_t last,size_t offset,
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],size_t *packet_bytes,size_t *next_offset);
int camera_span_hub_stop_v1(struct camera_span_hub_v1 *hub);
int camera_span_hub_drained_v1(const struct camera_span_hub_v1 *hub);
/* Only INIT_FAILED/no published view or STOPPED/drained (active tokens zero by
 * invariant). Clear each canonical chunk slot BEFORE its known-CPU void free;
 * preserve phase/generation/issued-token/history.
 * Never native END/ACK, external retirement, automatic cleanup or retry authority.
 */
int camera_span_hub_dispose_v1(struct camera_span_hub_v1 *hub);
#endif
