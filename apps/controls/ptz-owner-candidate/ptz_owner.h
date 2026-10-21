/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PTZ_CANDIDATE_OWNER_H
#define PTZ_CANDIDATE_OWNER_H
#include "ptz_auth.h"
#ifdef __cplusplus
extern "C" {
#endif
#define PTZ_OWNER_ABI 1U
#define PTZ_HEAD_MAX 4U
#define PTZ_PHASE_MAX 4U
#define PTZ_PHASE_NS UINT64_C(20000000)
#define PTZ_LEASE_MAX_NS UINT64_C(100000000)
#define PTZ_STOP_MAX_NS UINT64_C(250000000)
#define PTZ_CALLBACK_MAX_NS UINT64_C(1000000)
enum ptz_result {PTZ_OK=0,PTZ_DENIED=-1,PTZ_INVALID=-2,PTZ_BUSY=-3,
 PTZ_UNSAFE=-4,PTZ_STALE=-5,PTZ_EXHAUSTED=-6,PTZ_FAULT=-7,PTZ_RETAINED=-8};
enum ptz_kind {PTZ_PHASE=1,PTZ_NEUTRAL=2};
enum ptz_submit_result {PTZ_ACCEPTED=0,PTZ_NO_EFFECT=1,PTZ_UNCERTAIN=2};
enum ptz_completion_result {PTZ_ACK=0,PTZ_FAILED=1,PTZ_UNKNOWN=2};
enum ptz_state {PTZ_IDLE=0,PTZ_QUEUED,PTZ_PHASE_PENDING,PTZ_CADENCE,
 PTZ_STOP_PENDING,PTZ_QUARANTINED,PTZ_DISPOSED};
enum ptz_position_reason {PTZ_UNCALIBRATED=0,PTZ_SLIP,PTZ_REBOOT};
enum ptz_ready_bits {PTZ_ROUTE_READY=1U,PTZ_STOP_READY=2U,PTZ_BOUNDED_PHASE=4U,
 PTZ_ASYNC_STOP=8U,PTZ_REQUIRED_READY=15U};
/* Namespace is a trusted lifetime-unique registry epoch, not PID/auth token.
 * Head/generation/sequence are exact non-wrapping provider correlation keys. */
struct ptz_ticket {uint64_t space,generation,sequence;char head[CAMERA_ID_BYTES];};
struct ptz_request {
 struct ptz_ticket ticket,work;
 uint32_t kind,axis,phase_bits;
 uint64_t issued_ns,neutral_by_ns;
};
struct ptz_completion {
 struct ptz_ticket ticket;
 uint32_t kind,result,electrical_valid,observed_neutral,observed_phase_bits;
};
/* No GPIOs/levels cross this boundary; neutral is a provider operation, NOT 0000.
 * Copy requests by value before returning; never retain request pointer. submit
 * cannot block/reenter; clock is trusted monotonic and also must not block.
 * A NEUTRAL ACK is a retirement fence for ALL older requests of exact work,
 * including uncertain submissions: no later phase/IRQ/output from that work.
 * Provider independently enforces neutral_by_ns even if caller dies. submit's
 * ACCEPTED/NO_EFFECT/UNCERTAIN is not electrical/mechanical proof. */
struct ptz_provider {
 uint32_t abi,struct_bytes,readiness;
 uint64_t callback_bound_ns,stop_bound_ns;
 void *context;
 uint64_t (*clock_ns)(void *context);
 int (*submit)(void *context,const struct ptz_request *request);
};
struct ptz_config {uint32_t phase_limit;int32_t pan_sign,tilt_sign;};
struct ptz_move {
 struct camera_access_principal principal;
 char sensor[CAMERA_ID_BYTES];
 uint32_t axis,phases;
 int32_t direction;
 uint64_t lease_ns;
};
/* Storage is caller-owned zero-initialized once. No mutation/copy/reinit while
 * retained work exists. No allocation, threads, signals or supervisor here. */
struct ptz_head_record {char head[CAMERA_ID_BYTES];uint64_t generation,next_sequence;
 struct ptz_owner *owner;uint32_t used;};
struct ptz_registry {struct ptz_registry *self;uint64_t space;
 struct ptz_head_record heads[PTZ_HEAD_MAX];};
struct ptz_job {
 struct ptz_move move;struct ptz_ticket work,phase[PTZ_PHASE_MAX],stop;
 uint64_t expires_ns;uint32_t next_phase,phase_index;
};
struct ptz_view {
 uint32_t state,desired_kind,applied_kind,applied_phase_bits,applied_phases;
 uint32_t electrical_valid,observed_neutral,observed_phase_bits;
 uint32_t position_known,position_reason,native_qualified,fault,retained,queued;
 uint64_t generation,last_ns,stop_requested_ns,stop_ack_ns;
 struct ptz_ticket active_work,pending;
};
struct ptz_owner {
 struct ptz_owner *self;struct ptz_registry *registry;struct ptz_auth *auth;
 struct ptz_provider provider;struct ptz_config config;
 struct ptz_job job,replacement;
 struct ptz_view view;
 uint64_t phase_sent_ns,phase_ack_deadline_ns,stop_deadline_ns;
 uint32_t axis_phase[2];
 uint32_t head_slot,active,has_replacement,owned,stop_sent,stop_terminal,
  phase_waiting,disposing,disposed,callback_active,clock_failed;
};
int ptz_registry_init(struct ptz_registry *registry,uint64_t namespace_epoch);
int ptz_owner_init(struct ptz_owner *owner,struct ptz_registry *registry,
 struct ptz_auth *auth,const struct ptz_provider *provider,const struct ptz_config *config,
 uint64_t generation,uint64_t now_ns);
/* Client boundary: null/known-storage alias/schema failures are INVALID and
 * inert, including for an authorized principal. Policy denial is DENIED/inert.
 * Neither path submits STOP, latches fault, observes owner time, drops intent
 * or reserves tickets. Inputs must designate complete accessible typed objects;
 * arbitrary invalid pointer dereference protection is NOT a C API guarantee.
 * Valid MOVE requires PTZ_CONTROL then same-principal active-lease arbitration.
 * Explicit valid granted STOP may stop this head even from another operator;
 * no malformed command implicitly becomes STOP. now_ns is trusted caller time,
 * never a wire timestamp. Shared policy may observe that time on policy checks. */
int ptz_owner_move(struct ptz_owner *owner,const struct ptz_move *command,uint64_t now_ns);
int ptz_owner_stop(struct ptz_owner *owner,const struct camera_access_principal *principal,
 const char sensor[CAMERA_ID_BYTES],uint64_t now_ns);
/* The following lifecycle/provider functions are trusted-owner entry points,
 * NOT wire client control. Actual internal clock/provider/completion failures
 * retain exact-owned cleanup/quarantine, independent of a revoked session. */
int ptz_owner_poll(struct ptz_owner *owner,uint64_t now_ns);
int ptz_owner_complete(struct ptz_owner *owner,const struct ptz_completion *completion,uint64_t now_ns);
/* Safety cancellation is trusted-owner only, NOT an unauthenticated wire STOP.
 * It can neutralize only exact owned work, not arbitrary outputs/other heads. */
int ptz_owner_quiesce(struct ptz_owner *owner,uint32_t position_reason,uint64_t now_ns);
int ptz_owner_dispose(struct ptz_owner *owner,uint64_t now_ns);
int ptz_owner_reconfigure(struct ptz_owner *owner,const struct ptz_config *config,
 uint64_t newer_generation,uint64_t now_ns);
int ptz_owner_view(const struct ptz_owner *owner,struct ptz_view *view);
int ptz_ticket_equal(const struct ptz_ticket *a,const struct ptz_ticket *b);
#ifdef __cplusplus
}
#endif
#endif
