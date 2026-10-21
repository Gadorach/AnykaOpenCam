/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_SETTINGS_STORE_H
#define CAMERA_SETTINGS_STORE_H
#include <stddef.h>
#include <stdint.h>

#define CAMERA_SETTINGS_MAX_PAYLOAD 1024U
#define CAMERA_SETTINGS_CURRENT "settings.current"
#define CAMERA_SETTINGS_PENDING "settings.pending"

enum camera_settings_result {
 CAMERA_SETTINGS_OK=0, CAMERA_SETTINGS_INVALID=-1,
 CAMERA_SETTINGS_UNSUPPORTED=-2, CAMERA_SETTINGS_SENSITIVE=-3,
 CAMERA_SETTINGS_CONFLICT=-4, CAMERA_SETTINGS_EXHAUSTED=-5,
 CAMERA_SETTINGS_UNSAFE=-6, CAMERA_SETTINGS_CORRUPT=-7,
 CAMERA_SETTINGS_IO=-8, CAMERA_SETTINGS_RECOVERY_REQUIRED=-9,
 CAMERA_SETTINGS_UNCERTAIN=-10
};
enum camera_settings_kind {
 CAMERA_SETTINGS_VALUES=1, CAMERA_SETTINGS_RESET_REQUEST=2
};
enum camera_settings_file_state {
 CAMERA_SETTINGS_ABSENT=0, CAMERA_SETTINGS_VALID=1,
 CAMERA_SETTINGS_BAD=2
};
enum camera_settings_desired_state {
 CAMERA_SETTINGS_NONE=0, CAMERA_SETTINGS_COMMITTED=1,
 CAMERA_SETTINGS_PREPARED=2, CAMERA_SETTINGS_BLOCKED=3
};
enum camera_settings_durability {
 CAMERA_SETTINGS_NOT_CONFIRMED=0, CAMERA_SETTINGS_SYNC_CONFIRMED=1
};
/* Hooks inject host faults only; NULL is normal POSIX I/O. CLOSE always closes
 * the descriptor, even if a hook asks it to report failure. Never retry close.
 * No hook may reenter/mutate the store or directory. Return nonzero to fail. */
enum camera_settings_io_point {
 CAMERA_SETTINGS_WRITE=1, CAMERA_SETTINGS_FILE_SYNC,
 CAMERA_SETTINGS_DIR_SYNC, CAMERA_SETTINGS_RENAME, CAMERA_SETTINGS_CLOSE
};
typedef int (*camera_settings_fault_fn)(void *,uint32_t);
/* Mandatory trusted allowlist/schema/capability validator. OK asserts the
 * ENTIRE payload is public/nonsecret and understood. Reject credentials,
 * verifiers/digest equivalents, private keys, unknown/unsupported fields and
 * values; opaque credential IDs only. Must be pure, nonreentrant and must not
 * mutate input/store/directory. Never log/hash input. No path/command
 * interpretation here. Called before CRC or persistence, also when loading.
 * The store cannot infer secrecy from arbitrary opaque bytes. */
typedef int (*camera_settings_validate_fn)(void *,uint32_t,uint32_t,
 const unsigned char *,uint32_t);
struct camera_settings_snapshot {
 uint32_t schema,kind,payload_bytes;
 uint64_t revision,operation_id;
 unsigned char payload[CAMERA_SETTINGS_MAX_PAYLOAD];
};
struct camera_settings_report {
 uint32_t current_file,pending_file,desired_state,durability;
 /* Hardware applied is ALWAYS unknown here; no application receipt is made. */
 uint32_t hardware_applied_known;
 int current_error,pending_error;
 struct camera_settings_snapshot current,previous,pending,pending_previous;
};
/* Caller owns allocation and serializes ALL calls/effects, across processes
 * too. Exclusive camera/SEC ownership of the directory is required; these
 * checks are not a sandbox against another process with the same UID.
 * Borrowed dirfd: setup/SEC opens a trusted, dedicated, owned 0700 directory;
 * store does not select paths, mkdir, chmod, format, unlink or provision media.
 * Reserved files must be owned regular 0600 single-link files; no other entries.
 * Do not initialize over a live store; keep dirfd/callback context alive. */
struct camera_settings_store {
 int dirfd;
 uint32_t initialized;
 camera_settings_validate_fn validate;
 void *validate_context;
 camera_settings_fault_fn fault;
 void *fault_context;
};
int camera_settings_init(struct camera_settings_store *,int,
 camera_settings_validate_fn,void *,camera_settings_fault_fn,void *);
/* Read-only recovery receipt. Populates valid snapshots even when the other
 * file is bad. No automatic promotion, deletion, migration or defaults. */
int camera_settings_inspect(struct camera_settings_store *,
 struct camera_settings_report *);
/* Latest-operation replay only: identical request returns observed receipt,
 * not freshly confirmed durability. IDs strictly increase; older IDs conflict.
 * Failures after creating pending return UNCERTAIN and preserve evidence.
 * expected_revision=0 is allowed only for an empty dedicated store. */
int camera_settings_commit(struct camera_settings_store *,uint64_t,uint64_t,
 uint32_t,uint32_t,const void *,uint32_t,struct camera_settings_report *);
/* Explicit promotion of a fully validated pending record whose embedded old
 * snapshot EXACTLY matches current (or both are absent). Re-sync/readback;
 * corrupt/conflicting state is never replaced. Repeated call after promotion
 * returns latest replay receipt with unconfirmed durability. */
int camera_settings_resume(struct camera_settings_store *,uint64_t,
 struct camera_settings_report *);
/* Restore previous desired snapshot as a NEW revision, not hardware rollback.
 * Uses the same transaction rules and retains the displaced current snapshot. */
int camera_settings_rollback(struct camera_settings_store *,uint64_t,uint64_t,
 struct camera_settings_report *);
#endif
