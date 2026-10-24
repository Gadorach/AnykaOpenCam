/* SPDX-License-Identifier: GPL-2.0-only
 * Derived from span-hub-v2; see LICENSE_NOTICES.md.
 * EXCLUSIVE source replacement: version3 layout/API, NOT old binary ABI.
 * Never include/link both hubs, old combined RELs or old-layout consumers.
 */
#ifndef CAMERA_SPAN_HUB_V1_H
#define CAMERA_SPAN_HUB_V1_H
#include "../rtp-span-candidate/rtp_h264_spans.h"

#define CAMERA_SPAN_HUB_LAYOUT_VERSION 3U
#define CAMERA_SPAN_HUB_PRODUCER_API_VERSION 3U
#define CAMERA_SPAN_HUB_V1_CHUNKS 8U
#define CAMERA_SPAN_HUB_V1_CONSUMERS 4U
#define CAMERA_SPAN_HUB_V1_CHUNK_BYTES CAMERA_RTP_H264_SPAN_V1_BYTES

enum camera_span_hub_phase_v1 {
 CAMERA_SPAN_HUB_V1_FRESH=0, CAMERA_SPAN_HUB_V1_INITIALIZING,
 CAMERA_SPAN_HUB_V1_INIT_FAILED, CAMERA_SPAN_HUB_V1_ACTIVE,
 CAMERA_SPAN_HUB_V1_STOPPED, CAMERA_SPAN_HUB_V1_DISPOSING,
 CAMERA_SPAN_HUB_V1_DISPOSED
};
enum camera_span_hub_writer_phase_v3 {
 CAMERA_SPAN_WRITER_V3_IDLE=0, CAMERA_SPAN_WRITER_V3_RESERVED,
 CAMERA_SPAN_WRITER_V3_ARMED, CAMERA_SPAN_WRITER_V3_HOLD
};

struct camera_span_hub_v1 {
 const struct camera_span_hub_v1 *self;
 enum camera_span_hub_phase_v1 phase;
 uint8_t *chunks[CAMERA_SPAN_HUB_V1_CHUNKS];
 uint64_t generation,monotonic_ns;
 /* Consumer R1 identities: one lifetime, no wrap/reset/reuse. */
 uint64_t issued_token,active_token[CAMERA_SPAN_HUB_V1_CONSUMERS];
 enum camera_codec codec;
 uint32_t keyframe,subscribers,need_keyframe,pending,acquired;
 size_t bytes;
 uint32_t allocation_calls;
 size_t allocated_bytes,maximum_allocation;
 /* New layout; old memory/native/runtime/sender code must not reuse it. */
 uint32_t layout_version;
 enum camera_span_hub_writer_phase_v3 writer_phase;
 /* Last canonical identities remain even in IDLE/disposed; never reset. */
 uint64_t issued_writer_token,writer_token,writer_generation,armed_writer_token;
 int writer_error; /* sticky terminal HOLD, not a native receipt */
 size_t data_offset; /* canonical logical NAL slice in the eight owned chunks */
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
/* Reservation intentionally contains NO writable pointers. */
struct camera_span_hub_reservation_v3 {
 const struct camera_span_hub_v1 *owner;
 uint64_t token,generation;
};
struct camera_span_hub_write_span_v3 {uint8_t *data;size_t capacity;};
struct camera_span_hub_writer_v3 {
 const struct camera_span_hub_v1 *owner;
 uint64_t token,generation;
 size_t span_count;
 struct camera_span_hub_write_span_v3 spans[CAMERA_SPAN_HUB_V1_CHUNKS];
};
struct camera_span_hub_result_v3 {
 enum camera_codec codec;
 uint32_t keyframe;
 uint64_t monotonic_ns;
 size_t written_bytes,prefix_bytes; /* exact successful result, prefix0/3/4 */
};

/* TRUSTED SOURCE API: persistent true-fresh owner={0}, once at final address.
 * Valid stable disjoint private objects, serialized calls; never edit/copy/move
 * owner. No signals, reentry, callbacks, later fork or concurrency. Output and
 * input metadata objects are mutually disjoint and disjoint from owner/chunks.
 * ONLY arm's intended writable chunk aliases are allowed during admitted writer
 * custody; NEVER use them after commit. All other chunk aliases are const leases.
 * Self/token/range checks are not hostile-pointer, DMA, authorization or physical
 * provenance proof. CPU tokens cannot establish actual ioctl/release/END/ACK.
 * No post-dispose calls. Retained names/guard mean exclusive source replacement.
 * Failed init retains partial CPU allocations until explicit dispose, no retry.
 */
int camera_span_hub_init_v1(struct camera_span_hub_v1 *hub);
int camera_span_hub_subscribe_v1(struct camera_span_hub_v1 *hub,uint32_t consumer);
int camera_span_hub_resync_v1(struct camera_span_hub_v1 *hub,uint32_t consumer);
int camera_span_hub_unsubscribe_v1(struct camera_span_hub_v1 *hub,uint32_t consumer);
/* Legacy normalized one-VCL input. Actual one-copy publish at offset0; ordinary
 * no-writer backlog/keyframe semantics preserved. Refuses any retained writer.
 * Source release/END remains governed by actual external contracts, not copy.
 */
int camera_span_hub_publish_v1(struct camera_span_hub_v1 *hub,
 const struct camera_span_hub_input_v1 *input);

/* ACTIVE/IDLE, subscribers, NO pending/acquired/active tokens. Guard generation
 * and writer issuance exhaustion before mutation; refusal leaves output/state.
 * Only exact unexposed RESERVED may cancel, including after STOP. */
int camera_span_hub_producer_reserve_v3(struct camera_span_hub_v1 *hub,
 struct camera_span_hub_reservation_v3 *reservation);
int camera_span_hub_producer_cancel_v3(struct camera_span_hub_v1 *hub,
 const struct camera_span_hub_reservation_v3 *reservation);
/* Latches ARMED before exposing all eight canonical131072-byte writable ranges.
 * No abort/free/reset after arm. STOP after arm becomes terminal HOLD. */
int camera_span_hub_producer_arm_v3(struct camera_span_hub_v1 *hub,
 const struct camera_span_hub_reservation_v3 *reservation,
 struct camera_span_hub_writer_v3 *writer);
/* Actual future caller MUST already have syscall0/complete actual result,
 * successful real producer release/exact frame END and ALL native/clock/auth
 * guards. No bool/callback/receipt argument fabricates those facts here.
 * Checks exact writer shape and owned prefix/VCL bytes; publishes borrowed slice
 * with no copy/allocation. Recognized ARMED identity + any refusal -> sticky HOLD.
 * Stale identity leaves actual writer retained; no repair/retry. Invalid metadata
 * is never partial publication or automatic source/consumer retirement. */
int camera_span_hub_producer_commit_v3(struct camera_span_hub_v1 *hub,
 const struct camera_span_hub_writer_v3 *writer,
 const struct camera_span_hub_result_v3 *result);
/* Exact identity only; shape need not survive a partial fault. Sticky terminal
 * HOLD retains backing/history, no external retirement/effect/recovery. */
int camera_span_hub_producer_hold_v3(struct camera_span_hub_v1 *hub,
 const struct camera_span_hub_writer_v3 *writer);

/* Distinct consumer-tagged acquisition tokens; exact scalar release preserves
 * v2 R1 meaning. No consumer operation can coexist with retained writer custody.
 * STOP denies effects but exact old consumer releases still drain it. */
int camera_span_hub_acquire_v1(struct camera_span_hub_v1 *hub,uint32_t consumer,
 struct camera_span_hub_view_v1 *view);
int camera_span_hub_release_v1(struct camera_span_hub_v1 *hub,uint32_t consumer,
 uint64_t token,uint64_t generation);
int camera_span_hub_lease_valid_v1(const struct camera_span_hub_v1 *hub,
 uint32_t consumer,const struct camera_span_hub_view_v1 *view);
/* Rechecks exact lease/slice BEFORE EVERY actual accepted packetizer call.
 * Caller checks again before each send, retains <=1404B packet during partial
 * writes, commits sequence/offset only after whole packet. No send/callback here.
 */
int camera_span_hub_packet_v1(const struct camera_span_hub_v1 *hub,uint32_t consumer,
 const struct camera_span_hub_view_v1 *view,const struct camera_rtp_session *session,
 uint32_t timestamp,uint32_t last,size_t offset,
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],size_t *packet_bytes,size_t *next_offset);
int camera_span_hub_stop_v1(struct camera_span_hub_v1 *hub);
int camera_span_hub_drained_v1(const struct camera_span_hub_v1 *hub);
/* INIT_FAILED/pristine or STOPPED/drained only. Never free retained RESERVED,
 * ARMED/HOLD/error backing. Preserve generation, BOTH issuance histories, last
 * writer identities/exposure history, data_offset and allocation history. */
int camera_span_hub_dispose_v1(struct camera_span_hub_v1 *hub);
#endif
