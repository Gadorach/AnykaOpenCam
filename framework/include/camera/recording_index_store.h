/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RECORDING_INDEX_STORE_H
#define CAMERA_RECORDING_INDEX_STORE_H
#include <camera/recording_catalog.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CAMERA_RECORDING_INDEX_VERSION 1U
#define CAMERA_RECORDING_INDEX_HEADER_BYTES 512U
#define CAMERA_RECORDING_INDEX_ENTRY_BYTES 200U
#define CAMERA_RECORDING_INDEX_MAX_BYTES (512U+200U*CAMERA_RECORDING_MAX_ENTRIES)
#define CAMERA_RECORDING_INDEX_CURRENT "recording.index"
#define CAMERA_RECORDING_INDEX_PENDING "recording.index.pending"
enum camera_recording_index_result {
 CAMERA_INDEX_OK=0,CAMERA_INDEX_INVALID=-1,CAMERA_INDEX_CORRUPT=-2,
 CAMERA_INDEX_SCOPE=-3,CAMERA_INDEX_UNSAFE=-4,CAMERA_INDEX_IO=-5,
 CAMERA_INDEX_CONFLICT=-6,CAMERA_INDEX_EXHAUSTED=-7,
 CAMERA_INDEX_RECONCILE=-8,CAMERA_INDEX_RECOVERY_REQUIRED=-9,CAMERA_INDEX_UNCERTAIN=-10
};
enum camera_recording_index_file_state {
 CAMERA_INDEX_ABSENT=0,CAMERA_INDEX_VALID,CAMERA_INDEX_OLD_NAMESPACE,CAMERA_INDEX_BAD
};
enum camera_recording_index_intent {CAMERA_INDEX_NORMAL=1,CAMERA_INDEX_RECONCILED=2};
enum camera_recording_index_outcome {
 CAMERA_INDEX_BLOCKED=0,CAMERA_INDEX_READY,CAMERA_INDEX_PENDING,
 CAMERA_INDEX_RENAMED_UNCONFIRMED,CAMERA_INDEX_COMMITTED
};
enum camera_recording_index_fault_point {
 CAMERA_INDEX_STAGE_OPEN=1,CAMERA_INDEX_WRITE,CAMERA_INDEX_FILE_SYNC,
 CAMERA_INDEX_STAGE_DIR_SYNC,CAMERA_INDEX_RENAME,CAMERA_INDEX_COMMIT_DIR_SYNC,
 CAMERA_INDEX_CLOSE
};
/* Failure injection only: no reentry/store/directory mutation. NULL = POSIX I/O.
 * CLOSE always closes once even when injected; no retry of close. */
typedef int (*camera_recording_index_fault_fn)(void *,uint32_t);
struct camera_recording_index_scope {
 struct camera_recording_namespace space;uint32_t sensor_count;
 char sensors[CAMERA_RECORDING_MAX_SENSORS][CAMERA_ID_BYTES];
};
struct camera_recording_index_entry {
 struct camera_recording_metadata metadata;
 uint64_t generation,instance_generation; /* zero instance means unknown/RECONCILE */
};
/* VALUE objects only: never fwrite these structs. No pins, reservations, tokens,
 * credentials, pointers, footage, container timestamps derived here or tickets.
 * Entries in strictly increasing nonzero generation/admission order. Unknown or
 * older entry instances are quarantine facts, NOT usable catalogue handles. */
struct camera_recording_index_snapshot {
 struct camera_recording_index_scope scope;
 uint64_t revision,catalog_revision,generation_highwater,serial_highwater;
 uint32_t entry_count;
 struct camera_recording_index_entry entries[CAMERA_RECORDING_MAX_ENTRIES];
};
/* Public metadata fingerprint, NOT cryptographic authentication. Caller owns
 * directory exclusively across processes; same-UID hostile races are outside
 * this serialization contract. CRC detects accidental corruption, not forgery. */
struct camera_recording_index_fingerprint {
 uint64_t device,inode,bytes,revision;uint32_t present,crc;
};
struct camera_recording_index_file {
 uint32_t state,intent;int error;
 struct camera_recording_index_fingerprint fingerprint,base;
 struct camera_recording_index_snapshot snapshot;
};
struct camera_recording_index_report {
 /* Effects in this call only: inspect clears sync/effects; resume may rename
  * existing pending but does not claim stage creation. Not native SD proof. */
 uint32_t outcome,sync_confirmed,stage_created,renamed;
 struct camera_recording_index_file current,pending;
};
/* Caller supplies an independently qualified storage/sensor scope and fresh
 * never-reused owner instance. Never derive owner authority from arbitrary file
 * contents/last clock. Borrowed dedicated owned 0700 metadata-only dirfd; reserved
 * files must be own-UID regular 0600 single-link files. No other entries allowed.
 * No mkdir/chmod/unlink/format/media operations. Keep fd/context alive, serialize
 * ALL calls/effects, do not reinit live. Anchor dev/inode/UID checked every call.
 * Argument/output spans must be disjoint from store storage; codec output spans
 * must be disjoint from all inputs and each other. Invalid overlap is rejected
 * before mutation/I/O. Public allocation only: never edit live fields directly. */
struct camera_recording_index_store {
 uint32_t initialized;int dirfd;uint64_t directory_device,directory_inode,owner_uid;
 struct camera_recording_index_scope scope;
 camera_recording_index_fault_fn fault;void *fault_context;
};
int camera_recording_index_encode(const struct camera_recording_index_snapshot *,
 const struct camera_recording_index_fingerprint *,uint32_t intent,
 unsigned char *,size_t capacity,size_t *used);
/* Fully validates before output. RECONCILE returns validated but quarantined
 * old/unknown-instance metadata; wrong storage ID/sensor graph or future owner
 * is SCOPE. Changed storage generation is also RECONCILE, never write readiness.
 * No defaults or automatic activation. Other errors leave outputs unchanged. */
int camera_recording_index_decode(const unsigned char *,size_t,
 const struct camera_recording_index_scope *,struct camera_recording_index_snapshot *,
 struct camera_recording_index_fingerprint *base,uint32_t *intent);
int camera_recording_index_store_init(struct camera_recording_index_store *,int dirfd,
 const struct camera_recording_index_scope *,camera_recording_index_fault_fn,void *);
/* Read-only; preserves both file observations even if one is bad. Never cleans,
 * promotes, truncates or repairs pending/corrupt/foreign files. */
int camera_recording_index_inspect(struct camera_recording_index_store *,
 struct camera_recording_index_report *);
/* Exact observed current fingerprint required, including absence. No overwrite
 * of any preexisting pending file. next.revision = expected.revision+1; all
 * highwaters nondecreasing. NORMAL requires current fresh qualified scope.
 * Old generations may only retain byte-equivalent defined metadata facts/ref;
 * changes/new IDs require generations above the old watermark. Existing IDs
 * cannot change origin or physical sensor (no silent adoption/cross-sensor move).
 * RECONCILED is an explicit trusted-owner assertion of actual native inventory/
 * source reconciliation: old owner MUST be older; storage generation unchanged;
 * all refs reissued above previous generation highwater into fresh owner space.
 * This assertion is NOT verified here, NOT an auth grant or media repair. */
int camera_recording_index_commit(struct camera_recording_index_store *,
 const struct camera_recording_index_fingerprint *expected,
 const struct camera_recording_index_snapshot *next,uint32_t intent,
 struct camera_recording_index_report *);
/* Explicit recovery only: exact current AND pending fingerprints, embedded base,
 * owner scope and highwater transition rechecked. Torn/mismatched state requires
 * caller quarantine/reconciliation; no automatic rollback/deletion. */
int camera_recording_index_resume(struct camera_recording_index_store *,
 const struct camera_recording_index_fingerprint *expected_current,
 const struct camera_recording_index_fingerprint *expected_pending,
 struct camera_recording_index_report *);
#ifdef __cplusplus
}
#endif
#endif
