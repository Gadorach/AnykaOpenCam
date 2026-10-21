/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_CREDENTIAL_WORKER_H
#define CAMERA_CREDENTIAL_WORKER_H
#include <camera/access_policy.h>

#define CW_PASSWORD_MAX 128U
#define CW_ACCOUNT_MAX 8U
#define CW_JOB_MAX 3U /* one active + at most two queued; unread results occupy slots */
#define CW_ITERATIONS 600000U
#define CW_SCHEMA 1U

enum cw_result {
 CW_OK=0,CW_INVALID=-1,CW_FULL=-2,CW_THROTTLED=-3,CW_UNSUPPORTED=-4,
 CW_STALE=-5,CW_NOT_READY=-6,CW_EXHAUSTED=-7,CW_FAILED=-8
};
enum cw_status {
 CW_QUEUED=1,CW_RUNNING,CW_MATCH,CW_NO_MATCH,CW_CANCELLED,CW_EXPIRED,
 CW_REVOKED,CW_LIBRARY_ERROR,CW_STOPPED
};
/* SECRET memory object, never a configuration/settings/wire/log object.
 * Exactly PBKDF2-HMAC-SHA256/600000, 16-byte salt, 32-byte verifier.
 * Provisioning/entropy/persistence are NOT implemented. No implicit defaults. */
struct cw_ui_material {
 uint32_t schema,iterations;
 unsigned char salt[16],verifier[32];
};
/* Nonsecret, owner-local receipt. Job IDs are NOT bearer/session credentials. */
struct cw_completion {
 uint64_t job_id,credential_generation;
 uint32_t domain,status;
};
struct cw_broker;
/* Clock callback must be thread-safe, bounded, nonreentrant; same monotonic
 * clock as access_policy. Return zero on success. NULL uses CLOCK_MONOTONIC. */
typedef int (*cw_clock_fn)(void *,uint64_t *);
struct cw_options {
 uint64_t global_spacing_ns,account_spacing_ns,max_job_lifetime_ns;
 cw_clock_fn clock;void *clock_context;
};
/* Starts exactly one pthread. Creation/destruction at owner startup/shutdown
 * only; never reinitialize a live broker. A restarted broker requires owner
 * revocation of every old request/session and a new policy boot generation;
 * job IDs are local to this instance, not restart-safe external transaction IDs.
 * Mandatory public/unknown-user dummy
 * material has the SAME cost. Caller must wipe all its material/password copies.
 * Both admission spacings must be >=1 second, and global >= account spacing.
 * This deliberately globally dominant policy removes registration-dependent
 * throttle eligibility for identical accepted-attempt histories. Product timing
 * is NOT selected. No per-source limiter or network enumeration proof follows. */
int cw_create(struct cw_broker **,const struct cw_options *,const struct cw_ui_material *dummy);
/* Join waits for any in-flight library KDF; no asynchronous pthread cancellation.
 * Caller quiesces all API users before destroy; secrets wiped before free. */
void cw_destroy(struct cw_broker *);
/* UI and ADMIN only. RTSP/ONVIF/SETUP material explicitly unsupported.
 * Strictly increasing generations, including after revoke; tombstones retained.
 * Last session generation and per-account throttle survive credential changes.
 * These functions invalidate broker jobs, NOT already issued policy principals.
 * Owner MUST revoke/update policy and tear down effects in its serialized loop
 * BEFORE credential replacement/revoke, fail closed if either step fails. */
int cw_put_ui(struct cw_broker *,uint32_t,const char account[CAMERA_ID_BYTES],
 uint64_t,const struct cw_ui_material *);
int cw_revoke(struct cw_broker *,uint32_t,const char account[CAMERA_ID_BYTES],uint64_t);
/* Owner copies bounded password bytes, including embedded NUL, without trim.
 * Unknown accounts use dummy material and always complete NO_MATCH.
 * All time values are boot-local ns; deadline exclusive and within max lifetime.
 * Failed submission leaves job_id unchanged. Input must not alias broker memory. */
int cw_submit(struct cw_broker *,uint32_t,const char account[CAMERA_ID_BYTES],
 const unsigned char *,size_t,uint64_t deadline_ns,uint64_t *job_id);
int cw_cancel(struct cw_broker *,uint64_t);
int cw_poll(struct cw_broker *,uint64_t,struct cw_completion *);
/* Explicitly discard a completed result; a running canceled job stays occupied
 * until its local KDF copy is wiped. Never consumes unfinished jobs. */
int cw_forget(struct cw_broker *,uint64_t);
/* The ONLY bridge from a successful job to access_policy. Owner must serialize
 * ALL policy mutations/checks/effects and calls to this function on its event
 * loop. Checks live generation/deadline again, consumes result exactly once,
 * and issues fact directly while holding broker lock; no caller-made receipt
 * is accepted. Policy mismatch/failure also consumes success, fail closed.
 * No bearer randomness, cookies, CSRF, expiry transport or HTTP server here. */
int cw_issue_access(struct cw_broker *,uint64_t,struct camera_access_policy *,
 uint64_t session_lifetime_ns,struct camera_access_principal *);

#ifdef CW_TEST_ONLY
/* Test builds only; not part of production ABI. Fast/mock hooks must never be
 * linked into a product service. Positive match is trusted only in host tests. */
typedef int (*cw_test_verify_fn)(void *,const struct cw_ui_material *,const unsigned char *,size_t);
int cw_test_set_verify(struct cw_broker *,cw_test_verify_fn,void *);
int cw_test_set_next_job(struct cw_broker *,uint64_t);
size_t cw_test_storage_bytes(void);
#endif
#endif
