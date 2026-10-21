/* SPDX-License-Identifier: GPL-2.0-only */
/* Capture constraints adapted from Anyka ak_pcm.c (GPL v2 or later) and
 * plat-anyka/l2.h (GPL v2); original complete notices/copies accompany evidence. */
#ifndef QC07F_AUDIO_CAPTURE_CORE_H
#define QC07F_AUDIO_CAPTURE_CORE_H
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/stddef.h>
typedef u8 ac_u8;typedef u32 ac_u32;typedef u64 ac_u64;typedef unsigned long ac_uptr;
#else
#include <stdint.h>
#include <stddef.h>
typedef uint8_t ac_u8;typedef uint32_t ac_u32;typedef uint64_t ac_u64;typedef uintptr_t ac_uptr;
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define AC_ABI 1U
#define AC_MAX64 (~(ac_u64)0)
#define AC_MAX32 (~(ac_u32)0)
#define AC_PERIOD_BYTES 512U
#define AC_PERIOD_FRAMES 256U
#define AC_SLOTS 4U
#define AC_RING_BYTES (AC_PERIOD_BYTES*AC_SLOTS) /* 2048B, not family ceiling64KiB */
#define AC_STAGE_BOUND_NS 100000000ULL
enum ac_result {AC_OK=0,AC_EMPTY=1,AC_RETAINED=2,AC_INVALID=-1,AC_STALE=-2,
 AC_BUSY=-3,AC_UNAVAILABLE=-4,AC_FAULT=-5,AC_EXHAUSTED=-6};
enum ac_operation {AC_CLAIM=1,AC_CONFIGURE,AC_CACHE_DEVICE,AC_DMA_PERIOD,AC_CACHE_CPU,AC_STOP,AC_RELEASE};
enum ac_submit {AC_ACCEPTED=0,AC_NO_EFFECT=1,AC_UNCERTAIN=2};
enum ac_completion_status {AC_ACK=0,AC_FAILED=1,AC_UNKNOWN=2};
enum ac_state {AC_CLOSED=0,AC_CLAIMING,AC_CONFIGURING,AC_PREPARED,AC_CAPTURING,
 AC_STOPPING,AC_WAIT_BORROW,AC_RELEASING,AC_QUARANTINED};
enum ac_slot_state {AC_SLOT_FREE=0,AC_SLOT_DEVICE,AC_SLOT_DMA,AC_SLOT_CPU,AC_SLOT_READY,AC_SLOT_BORROWED};
enum ac_evidence {AC_PRESENT_UNQUALIFIED=1};
enum ac_resource {AC_MIC_ROUTE=1U,AC_ANALOG_CLOCK_OWNER=2U,AC_SHARED_L2_OWNER=4U,
 AC_DMA_CACHE_OWNER=8U,AC_IRQ_RETIRE_OWNER=16U,AC_RESOURCE_REQUIRED=31U};
struct ac_ticket {ac_u64 namespace_epoch,generation,sequence,resource_epoch;};
struct ac_params {ac_u32 rate,channels,sample_bits,period_bytes,periods;};
struct ac_clock {ac_u64 parent_hz,epoch;ac_u32 ready_mask,mic_gain_code;};
struct ac_rate {ac_u64 numerator;ac_u32 denominator,osr,divider,requested_hz;};
struct ac_request {
 struct ac_ticket ticket,work;
 ac_u32 operation,slot,offset,bytes,resource_mask;
 ac_u64 deadline_ns;struct ac_rate rate;
};
struct ac_completion {
 struct ac_ticket ticket;ac_u32 operation,status,bytes,timestamp_valid;
 ac_u64 sample_index,first_sample_ns,irq_ns;
};
/* Nonblocking serialized trusted provider; copy requests by value. No private
 * L2 reset/fraction/DAC/GPIO operations exist here. STOP ACK fences ALL prior
 * claim/config/cache/DMA/IRQ/callback effects, even uncertain partial init;
 * RELEASE ACK ends owned references. Provider, ring and code remain alive until
 * both known fences and every borrowed span release. Typed masks are NOT proof
 * of retail wiring/clock/current/cache/IRQ qualification. Native probe refuses. */
struct ac_provider {ac_u32 abi,struct_bytes;void *context;int (*submit)(void *,const struct ac_request *);};
struct ac_span {
 struct ac_ticket ticket;const ac_u8 *data;ac_u32 bytes,frames,slot,evidence,native_qualified;
 ac_u64 sample_index,monotonic_ns,track_generation;struct ac_rate rate;
};
struct ac_slot {struct ac_ticket ticket;ac_u32 state;ac_u64 sample_index,pts_ns;};
struct ac_core {
 struct ac_core *self;struct ac_provider provider;struct ac_params params;struct ac_clock clock;struct ac_rate rate;
 struct ac_ticket work,pending,configure,stop,release,device,dma,cpu;
 struct ac_slot slots[AC_SLOTS];ac_u8 *ring;
 ac_u64 namespace_epoch,generation,next_sequence,track_generation,last_ns,deadline_ns,
  next_sample_index,epoch_ns,last_irq_ns;
 ac_u32 state,operation,slot_index,borrowed,retained,closing,stop_sent,stop_acked,
  release_sent,terminal,fault,xrun,muted,have_epoch,busy,native_qualified;
};
int ac_rate_plan(const struct ac_params *,const struct ac_clock *,struct ac_rate *);
int ac_init(struct ac_core *,const struct ac_provider *,ac_u64 namespace_epoch,ac_u64 now_ns);
int ac_prepare(struct ac_core *,const struct ac_params *,const struct ac_clock *,
 ac_u8 *ring,size_t ring_bytes,ac_u64 newer_generation,ac_u64 now_ns);
int ac_start(struct ac_core *,ac_u64 now_ns);
/* Trusted monotonic owner timer tick in EVERY waiting state (CLAIM/CONFIG,
 * both caches/DMA, STOP/RELEASE), not just data completion callbacks. Equality
 * expires. Init/data timeout requests one reserved exact owned STOP; cleanup
 * timeout latches fault/quarantine while retaining the pending fence/backing.
 * No retry/free/ACK is synthesized. WAIT_BORROW has no pending hardware deadline
 * after normal STOP ACK: exact held returns are still mandatory before RELEASE.
 * Future native broker MUST independently drive this timer when completions are
 * missing. This function creates no timer and cannot preempt a blocked provider
 * callback or establish bounded physical STOP/native retirement by itself. */
int ac_pump(struct ac_core *,ac_u64 now_ns);
int ac_complete(struct ac_core *,const struct ac_completion *,ac_u64 now_ns);
int ac_close(struct ac_core *,ac_u64 now_ns);
int ac_source_fault(struct ac_core *,ac_u64 now_ns); /* no reset/bypass operation */
int ac_mute(struct ac_core *,ac_u32 enabled,ac_u64 now_ns);
int ac_borrow(struct ac_core *,struct ac_span *);
int ac_span_valid(const struct ac_core *,const struct ac_span *);
int ac_return(struct ac_core *,const struct ac_ticket *,ac_u64 now_ns);
int ac_can_free(const struct ac_core *);
int ac_pts(const struct ac_core *,ac_u64 sample_index,ac_u64 *);
int ac_ticket_equal(const struct ac_ticket *,const struct ac_ticket *);
/* Complete accessible typed storage only. No wire input, credential/domain
 * assertion, physical address, sample file output or allocation in this core.
 * Caller owns single-resource arbitration and lifetime-unique namespace. */
#ifdef __cplusplus
}
#endif
#endif
