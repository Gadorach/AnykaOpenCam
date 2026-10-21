/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_PRODUCER_H
#define CAMERA_PRODUCER_H
#include <camera/backend.h>

#define CAMERA_PRODUCER_CONSUMERS 8U
#define CAMERA_PRODUCER_CONTROLS 8U
#define CAMERA_PRODUCER_QUEUE 8U
#define CAMERA_PRODUCER_VALUE_WORDS 4U

enum camera_producer_state {
 CAMERA_PRODUCER_STOPPED, CAMERA_PRODUCER_STARTING, CAMERA_PRODUCER_RUNNING,
 CAMERA_PRODUCER_DRAINING, CAMERA_PRODUCER_RECOVERING, CAMERA_PRODUCER_FAILED
};
enum camera_producer_consumer_kind {
 CAMERA_PRODUCER_RTSP, CAMERA_PRODUCER_UI, CAMERA_PRODUCER_RECORDER
};
enum camera_producer_work_kind {
 CAMERA_PRODUCER_NO_WORK, CAMERA_PRODUCER_START, CAMERA_PRODUCER_STOP,
 CAMERA_PRODUCER_RECOVER, CAMERA_PRODUCER_KEYFRAME, CAMERA_PRODUCER_CONTROL
};
enum camera_producer_command_kind {CAMERA_PRODUCER_SET, CAMERA_PRODUCER_COMMAND_STOP};
enum camera_producer_boundary {
 CAMERA_PRODUCER_NO_BOUNDARY, CAMERA_PRODUCER_PROVED_BOUNDARY
};
/* Stable service outcomes, separate from backend.h driver errors. */
enum camera_producer_result {
 CAMERA_PRODUCER_PENDING=1, CAMERA_PRODUCER_CANCELLED=-100,
 CAMERA_PRODUCER_TIMEOUT=-101
};
enum camera_producer_reason {
 CAMERA_PRODUCER_REASON_NONE, CAMERA_PRODUCER_REASON_BACKEND,
 CAMERA_PRODUCER_REASON_CANCELLED, CAMERA_PRODUCER_REASON_TIMEOUT,
 CAMERA_PRODUCER_REASON_SOURCE, CAMERA_PRODUCER_REASON_SHUTDOWN
};
enum camera_producer_applied_evidence {
 CAMERA_PRODUCER_APPLIED_UNKNOWN, CAMERA_PRODUCER_APPLIED_ACKNOWLEDGED
};
struct camera_producer;
struct camera_producer_token {
 const struct camera_producer *owner;
 uint64_t id,generation;
};
struct camera_producer_demand {
 const struct camera_producer *owner;
 uint64_t id;
 uint32_t slot;
};
/* Service-defined key and scalar units/schema; no vendor structures or pointers.
 * The owning service validates authorization, ranges and field relationships.
 */
struct camera_producer_value {int64_t word[CAMERA_PRODUCER_VALUE_WORDS];};
struct camera_producer_command {
 uint32_t key;
 enum camera_producer_command_kind kind;
 struct camera_producer_value value;
};
struct camera_producer_work {
 struct camera_producer_token token;
 enum camera_producer_work_kind kind;
 uint64_t deadline_ns,desired_generation;
 struct camera_producer_command command;
};
struct camera_producer_control_state {
 struct camera_producer_value desired,applied;
 uint64_t desired_generation,applied_generation,applied_producer_generation;
 struct camera_producer_token desired_operation;
 int desired_result;
 enum camera_producer_reason desired_reason;
 uint64_t result_ns,applied_ns;
  /* Historical applied values are retained, but control dispatch, drain entry,
   * failure or a new generation invalidates their current evidence before
   * asynchronous lifecycle STOP can alter resources. Cancellation cannot undo work.
  * ACKNOWLEDGED is adapter success, NOT an independent hardware readback.
  */
 enum camera_producer_applied_evidence applied_evidence;
};
struct camera_producer_event {
 struct camera_producer_work work;
 enum camera_producer_state state;
 int result;
 enum camera_producer_reason reason;
 uint64_t monotonic_ns;
};
struct camera_producer_config {
 uint64_t operation_timeout_ns;
 uint32_t keyframe_supported;
 struct {uint32_t set_supported,stop_supported;} controls[CAMERA_PRODUCER_CONTROLS];
 /* Synchronous notification; copy event if retained. MUST NOT reenter producer.
  * Fan out SOURCE/lifecycle failure to all dependent clients in the owner.
  */
 void (*event)(void *context,const struct camera_producer_event *event);
 void *event_context;
};
struct camera_producer_consumer {
 uint64_t id;
 enum camera_producer_consumer_kind kind;
 uint32_t attached;
};
/* Public only to permit static allocation. Fields are private to producer.c;
 * readers may inspect state/generation/demand_count under caller serialization.
 */
struct camera_producer {
 struct camera_producer_config config;
 enum camera_producer_state state;
 uint64_t generation,next_id,last_ns;
 uint32_t initialized,shutdown,needs_stop,demand_count;
 struct camera_producer_consumer consumers[CAMERA_PRODUCER_CONSUMERS];
 struct camera_producer_control_state controls[CAMERA_PRODUCER_CONTROLS];
 struct camera_producer_work queue[CAMERA_PRODUCER_QUEUE],stop,keyframe,active;
 uint32_t queue_count,stop_pending,keyframe_pending,active_present,active_terminal;
 int active_result;
 enum camera_producer_reason active_reason;
};

/* CALLER SERIALIZES EVERY call, read and callback delivery. No locks, threads,
 * allocation, timers or driver calls. Initialize fresh storage only: do not move,
 * copy or reinitialize a context while tokens/adapter callbacks reference it.
 * Tokens bind owner address + nonwrapping ID + producer generation. Demand refs
 * survive recovery and are NOT kernel/DMA leases or evidence of hardware busy.
 * All timed calls use one nondecreasing monotonic clock; regression is INVALID.
 */
int camera_producer_init(struct camera_producer *producer,const struct camera_producer_config *config);
int camera_producer_attach(struct camera_producer *producer,enum camera_producer_consumer_kind kind,
                          uint64_t now_ns,struct camera_producer_demand *demand);
/* Duplicate detach is OK until that slot is reused; after reuse the old token
 * is STALE_LEASE and cannot detach the new consumer. Last demand drains/stops.
 * Drain cancels ordinary/START/RECOVER work but preserves an active lifecycle
 * STOP, including recovery cleanup; timeout/backend failure still fails normally.
 */
int camera_producer_detach(struct camera_producer *producer,struct camera_producer_demand demand,uint64_t now_ns);
int camera_producer_request_keyframe(struct camera_producer *producer,uint64_t now_ns,
                                    struct camera_producer_token *operation);
int camera_producer_submit(struct camera_producer *producer,const struct camera_producer_command *command,
                           uint64_t now_ns,struct camera_producer_token *operation);
int camera_producer_cancel(struct camera_producer *producer,struct camera_producer_token operation,uint64_t now_ns);
/* Poll at most one work item. Adapter must establish/prove the control boundary
 * (drained capture leases or a qualified safe point); client presence proves
 * nothing. Lifecycle STOP > reserved command STOP > FIFO SET > keyframe.
 * Command STOP cancels older SETs on the same key, preventing delayed movement.
 * Priority never preempts unretired work. NOT_READY means no dispatchable work.
 */
int camera_producer_next(struct camera_producer *producer,uint64_t now_ns,
                         enum camera_producer_boundary boundary,struct camera_producer_work *work);
/* Copy active work and its logical outcome for adapter cancellation/progress.
 * PENDING means continue; a terminal result means abort/retire safely. Even a
 * timed-out/cancelled work remains active until complete acknowledges retirement.
 */
int camera_producer_active(const struct camera_producer *producer,struct camera_producer_work *work,int *result);
/* One completion AFTER adapter work/callbacks have retired; result is a
 * camera_result from backend.h. Never report OK for merely requested DMA stop.
 * A cancelled/timed-out success retires but NEVER commits applied state.
 * Duplicate/foreign/old completions are STALE_LEASE. Deadlines win at equality.
 */
int camera_producer_complete(struct camera_producer *producer,struct camera_producer_token operation,
                             int backend_result,uint64_t now_ns);
/* Caller must poll tick/active and enforce deadlines in its event loop. This
 * cannot forcibly interrupt a backend or authorize freeing live storage.
 */
int camera_producer_tick(struct camera_producer *producer,uint64_t now_ns);
int camera_producer_source_failed(struct camera_producer *producer,int backend_result,uint64_t now_ns);
/* Explicit recovery only: STOP for actual retirement, then RECOVER (recreate
 * producer), new generation. Existing consumers are retained, never auto-loop.
 */
int camera_producer_recover(struct camera_producer *producer,uint64_t now_ns);
int camera_producer_shutdown(struct camera_producer *producer,uint64_t now_ns);
/* True only after shutdown + successful STOP + callback retirement. Separately
 * retire packet/frame exports and external observer references before freeing.
 */
int camera_producer_quiescent(const struct camera_producer *producer);
int camera_producer_control_snapshot(const struct camera_producer *producer,uint32_t key,
                                     struct camera_producer_control_state *state);
#endif
