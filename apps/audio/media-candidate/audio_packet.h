/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef AUDIO_MEDIA_CANDIDATE_PACKET_H
#define AUDIO_MEDIA_CANDIDATE_PACKET_H
#include "g711.h"
#include <capture_core.h>
#ifdef __cplusplus
extern "C" {
#endif
#define AM_HEADER_BYTES 12U
#define AM_CODE_BYTES 256U
#define AM_WIRE_BYTES (AM_HEADER_BYTES+AM_CODE_BYTES)
#define AM_EFFECT_MAX 64U
#define AM_STORAGE_CAP 1024U
#define AM_CALLBACK_BOUND_NS 1000000ULL /* software callback bound, not physical preemption */
enum am_result {AM_OK=0,AM_COMPLETE=1,AM_INVALID=-1,AM_BUSY=-2,AM_DENIED=-3,
 AM_STALE=-4,AM_FORMAT=-5,AM_CLOCK=-6,AM_EXHAUSTED=-7,AM_RETAINED=-8};
enum am_effect {AM_ENCODE=1,AM_HEADER,AM_PAYLOAD,AM_QUEUED_SEND};
struct am_format {uint32_t law,payload_type,clock_hz,channels;};
struct am_key {uint64_t stream_namespace,stream_generation,packet_serial;
 struct ac_ticket capture_ticket;uint64_t track_generation;};
struct am_chunk {struct am_key key;uint64_t chunk_serial;uint32_t offset,bytes,effect;};
/* Known pre-subscription context ONLY: no capture ticket, slot, data pointer,
 * sample index or claimed owned read span. Profile/principal binding is held
 * by the trusted callback context; these fields bind its actual source owner. */
struct am_borrow_identity {
 uint64_t stream_namespace,stream_generation,capture_namespace,capture_generation,
  resource_epoch,track_generation,parent_hz;
 uint32_t rate_hz,channels,sample_bits;
};
/* TRUSTED adapter callback: invoke ACTUAL current protocol policy + admission,
 * not a peer flag/role/token assertion. Immutable principal/realm/profile context
 * belongs to caller. Must be bounded, nonblocking, serialized with effect and
 * capture state; no callback pointer/context is deserialized from wire input.
 * valid_until_ns MUST be the actual minimum current principal/admission expiry,
 * not a fabricated lease/verified flag. Helper samples trusted clock around
 * callbacks and repeats the real check at fresh post-callback time; expiry at
 * equality, source mutation, regression or callback overrun denies effects.
 * Final check must be read-only/current policy proof with no authority mutation
 * after checking. A lying adapter cannot be authenticated by portable C. */
typedef int (*am_admit_fn)(void *context,const struct am_key *key,uint64_t now_ns,
 uint32_t effect,uint32_t offset,uint32_t bytes,uint64_t *valid_until_ns);
typedef int (*am_preborrow_fn)(void *context,const struct am_borrow_identity *,uint64_t now_ns,uint64_t *valid_until_ns);
typedef uint64_t (*am_clock_fn)(void *context); /* actual trusted monotonic, not peer/UTC time */
struct am_seed {uint64_t stream_namespace,stream_generation;uint32_t ssrc,rtp_timestamp;
 uint16_t sequence;}; /* caller supplies qualified entropy/unique SSRC; no verified flag */
struct am_packet {
 struct am_packet *self;struct ac_core *capture;am_admit_fn admit;am_preborrow_fn preborrow;am_clock_fn clock_now;void *admission_context;
 struct ac_span span;struct am_key key;struct am_chunk offered;
 uint8_t header[AM_HEADER_BYTES],codes[AM_CODE_BYTES];
 uint64_t sequence_ext,rtp_ext,packet_serial,next_chunk_serial,last_ns,
  anchor_index,anchor_pts,expected_index,resource_epoch,track_generation,capture_generation;
 uint32_t ssrc,rtp_base,law,cursor,offered_valid,owned,invalid,clock_failed,
  busy,have_anchor,return_attempted,disposed,native_qualified;
};
int am_format_select(uint32_t law,struct am_format *format); /* no SDP strings/parser */
int am_init(struct am_packet *,struct ac_core *,am_preborrow_fn,am_admit_fn,am_clock_fn,void *,const struct am_seed *,uint32_t law,uint64_t now_ns);
/* Inert schema/source/pre-policy/admission rejection BEFORE ac_borrow or any
 * helper counter/key mutation. After successful actual grant/source recheck,
 * acquire one loan; post-borrow policy races retain it until explicit cancel. */
int am_begin(struct am_packet *,struct am_key *key,uint64_t now_ns);
/* Copies one bounded HEADER or PAYLOAD offer; it is NOT socket/TLS I/O. */
int am_offer(struct am_packet *,const struct am_key *,uint64_t now_ns,uint8_t *output,size_t capacity,struct am_chunk *);
/* MANDATORY immediately before each actual async/queued transport effect,
 * including a partial-write continuation. No successful prior check is a lease.
 * Caller atomically serializes recheck->effect->advance with policy/capture state. */
int am_recheck(struct am_packet *,const struct am_chunk *,uint64_t now_ns);
int am_advance(struct am_packet *,const struct am_chunk *,uint32_t actual_bytes,uint64_t now_ns);
int am_done(struct am_packet *,const struct am_key *,uint64_t now_ns);
int am_cancel(struct am_packet *,uint64_t now_ns); /* trusted consumer discards all outstanding copies */
int am_dispose(struct am_packet *,uint64_t now_ns);
/* Return is exactly once for this consumer loan only. No ac_close/source_fault,
 * no DMA retirement/supervision/socket-EOF inference. Original STOP+RELEASE+
 * held-return class owns actual backing/code retirement. Unknown return keeps
 * helper quarantined; no duplicate implicit release/reinit.
 * Complete accessible typed objects, caller serialized, no pointer aliasing or
 * direct mutation/copy of initialized objects. No heap/thread/network/UTC/IO. */
#ifdef __cplusplus
}
#endif
#endif
