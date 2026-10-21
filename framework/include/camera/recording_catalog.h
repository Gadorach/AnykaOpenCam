/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RECORDING_CATALOG_H
#define CAMERA_RECORDING_CATALOG_H
#include <camera/backend.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CAMERA_RECORDING_ABI 1U
#define CAMERA_RECORDING_MAX_ENTRIES 64U
#define CAMERA_RECORDING_MAX_SENSORS 4U
#define CAMERA_RECORDING_MAX_PINS 32U

enum camera_recording_origin {
 CAMERA_RECORDING_SERVICE_OWNED=1,CAMERA_RECORDING_USER,
 CAMERA_RECORDING_IMPORTED,CAMERA_RECORDING_STOCK,CAMERA_RECORDING_UNOWNED,
 CAMERA_RECORDING_FOREIGN
};
enum camera_recording_state {
 CAMERA_RECORDING_OPEN=1,CAMERA_RECORDING_INCOMPLETE,
 CAMERA_RECORDING_COMPLETE,CAMERA_RECORDING_CORRUPT
};
enum camera_recording_time_domain {
 CAMERA_RECORDING_TIME_UNKNOWN=0,CAMERA_RECORDING_TIME_MONOTONIC,
 CAMERA_RECORDING_TIME_QUALIFIED_UTC
};
enum camera_recording_pressure {
 CAMERA_RECORDING_PRESSURE_COUNT=1U<<0,CAMERA_RECORDING_PRESSURE_BYTES=1U<<1,
 CAMERA_RECORDING_PRESSURE_AGE=1U<<2,CAMERA_RECORDING_PRESSURE_TIME_UNKNOWN=1U<<3
};
/* Namespace generations must be nonzero and never reused for this storage ID.
 * storage_generation changes on replacement/remount; instance_generation on
 * owner restart. Caller supplies/persists them; this module cannot prove them. */
struct camera_recording_namespace {
 char storage_id[CAMERA_ID_BYTES];
 uint64_t storage_generation,instance_generation;
};
struct camera_recording_scope {
 struct camera_recording_namespace space;char sensor_id[CAMERA_ID_BYTES];
};
/* Opaque ID, NOT a filename. Canonical zero-padded CORE/access-policy IDs only.
 * Times are actual source facts, not duration/FPS/container guesses. UNKNOWN
 * requires all time fields zero. Qualified UTC requires an explicit trust epoch;
 * monotonic facts never satisfy UTC age retention. */
struct camera_recording_metadata {
 uint32_t abi,struct_bytes;
 char recording_id[CAMERA_ID_BYTES],sensor_id[CAMERA_ID_BYTES];
 uint64_t bytes,start_ns,end_ns,time_epoch;
 uint32_t origin,state,closed,active,time_domain,reserved;
};
struct camera_recording_ref {
 struct camera_recording_scope scope;char recording_id[CAMERA_ID_BYTES];
 uint64_t generation;
};
/* These are exact owner-held value snapshots, NOT authentication credentials. */
struct camera_recording_pin {struct camera_recording_ref ref;uint64_t serial;};
struct camera_recording_reservation {struct camera_recording_ref ref;uint64_t serial;};
struct camera_recording_budget {
 uint64_t max_bytes,max_age_ns,utc_now_ns,utc_epoch;
 uint32_t max_count,age_enabled;
};
struct camera_recording_plan {
 struct camera_recording_scope scope;
 uint64_t revision,serial,remaining_bytes;
 uint32_t count,remaining_count,pressure,reserved;
 struct camera_recording_ref entries[CAMERA_RECORDING_MAX_ENTRIES];
};
/* Public solely for caller-owned static allocation; no direct live mutation.
 * Caller serializes ALL calls and native effects. No allocation, locks, I/O,
 * callbacks, clock reads, files, unlink, format, media parsing or authorization.
 * Argument/output spans overlapping catalogue storage are rejected INVALID
 * before any mutation, including clock observation/failed-init clearing. Other
 * snapshot input/output aliases are safe (inputs consumed/copied before output),
 * except reserve's declared output span must be disjoint from its entire plan.
 * Snapshots are local values, not wire/persistent representations.
 * One pending plan at a time; every catalogue/pin/reservation mutation stales it.
 * Chronology is owner admission order (generation), never reordered by UTC. */
struct camera_recording_slot {
 struct camera_recording_metadata metadata;uint64_t generation,reservation;
 uint32_t used,pins;
};
struct camera_recording_pin_slot {struct camera_recording_pin pin;uint32_t used;};
struct camera_recording_catalog {
 uint32_t initialized,sensor_count,clock_failed,plan_valid;
 struct camera_recording_namespace space;
 char sensors[CAMERA_RECORDING_MAX_SENSORS][CAMERA_ID_BYTES];
 uint64_t revision,next_generation,next_serial,total_bytes,last_ns,last_utc_ns,last_utc_epoch;
 struct camera_recording_slot slots[CAMERA_RECORDING_MAX_ENTRIES];
 struct camera_recording_pin_slot pins[CAMERA_RECORDING_MAX_PINS];
 struct camera_recording_plan pending;
};

/* Startup only, never reinitialize a live namespace to reclaim pins/counters.
 * Errors: INVALID malformed; STALE_LEASE exact scope/ref/snapshot mismatch;
 * BUSY protected/capacity or plan pressure; NOT_READY clock/counter exhaustion.
 * Failed init is unusable except alias/span rejection leaves storage unchanged.
 * Successful outputs only, except plan on BUSY: its
 * nonempty safe subset may be reserved but pressure MUST remain visible. */
int camera_recording_catalog_init(struct camera_recording_catalog *catalog,
 const struct camera_recording_namespace *space,
 const char sensors[][CAMERA_ID_BYTES],uint32_t sensor_count,uint64_t now_ns);
int camera_recording_catalog_add(struct camera_recording_catalog *catalog,
 const struct camera_recording_metadata *metadata,struct camera_recording_ref *ref);
/* Exact compare/replace; IDs/sensor/origin cannot change. Pinned/reserved entries
 * cannot change. Replacement gets a fresh generation, including same metadata. */
int camera_recording_catalog_replace(struct camera_recording_catalog *catalog,
 const struct camera_recording_ref *expected,
 const struct camera_recording_metadata *metadata,struct camera_recording_ref *ref);
int camera_recording_catalog_get(const struct camera_recording_catalog *catalog,
 const struct camera_recording_ref *ref,struct camera_recording_metadata *metadata);
/* Caller must camera_access_check PLAYBACK/DOWNLOAD on this exact physical sensor
 * before acquisition and EVERY read/range effect; pin is retention-only. It does
 * not certify codec/file playability. Complete closed nonactive metadata only;
 * imported/stock/user footage may be pinned but is never retention-eligible.
 * No automatic expiry: close/timeout/revoke must stop reads before exact release. */
int camera_recording_pin_acquire(struct camera_recording_catalog *catalog,
 const struct camera_recording_ref *ref,struct camera_recording_pin *pin);
int camera_recording_pin_check(const struct camera_recording_catalog *catalog,
 const struct camera_recording_pin *pin);
int camera_recording_pin_release(struct camera_recording_catalog *catalog,
 const struct camera_recording_pin *pin);
/* Budgets count ALL entries/bytes in exactly one physical sensor scope. Age
 * selects only matching qualified UTC epoch with end<=now; unknown/future/other
 * epochs report uncertainty/pressure, not age deletion. Counter/byte pruning
 * may select unknown-time SERVICE_OWNED entries by admission chronology. */
int camera_recording_retention_plan(struct camera_recording_catalog *catalog,
 const struct camera_recording_scope *scope,const struct camera_recording_budget *budget,
 uint64_t now_ns,struct camera_recording_plan *plan);
/* All-or-nothing reservation of exact pending plan. Blocks replacement and new
 * pins until ack/cancel. Output array needs capacity>=plan.count. Its declared
 * capacity*sizeof(reservation) span must not overlap the entire input plan or
 * catalogue, even if only a prefix would be written. Checked size/address spans
 * reject overflow/overlap INVALID before ANY mutation; zero capacity is empty.
 * NULL output remains allowed only for an empty plan. No filesystem
 * effect; caller must recheck reservation and anchored native inode/storage
 * identity atomically with deletion under its owner/directory capability. */
int camera_recording_retention_reserve(struct camera_recording_catalog *catalog,
 const struct camera_recording_plan *plan,uint64_t now_ns,
 struct camera_recording_reservation *reservations,uint32_t capacity);
int camera_recording_retention_check(struct camera_recording_catalog *catalog,
 const struct camera_recording_reservation *reservation,uint64_t now_ns);
/* removed=1 is caller's confirmed native outcome, NOT proof generated here;
 * removes metadata only. removed=0 cancels exactly this reservation. */
int camera_recording_retention_ack(struct camera_recording_catalog *catalog,
 const struct camera_recording_reservation *reservation,uint32_t removed);
#ifdef __cplusplus
}
#endif
#endif
