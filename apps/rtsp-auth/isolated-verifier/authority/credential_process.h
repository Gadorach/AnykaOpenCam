/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CREDENTIAL_PROCESS_H
#define CREDENTIAL_PROCESS_H
#include "credential_worker.h"
enum cp_result {CP_OK=0,CP_INVALID=-1,CP_BUSY=-2,CP_THROTTLED=-3,CP_UNSUPPORTED=-4,
 CP_STALE=-5,CP_PENDING=-6,CP_LOST=-7,CP_HARDENING=-8,CP_UNCERTAIN=-9};
enum cp_status {CP_WAITING=1,CP_MATCHED,CP_REJECTED,CP_CANCELLED,CP_EXPIRED,CP_REVOKED,CP_CHILD_LOST,CP_DISCARDED};
struct cp_options {
 uint64_t child_generation,global_spacing_ns,account_spacing_ns,max_job_lifetime_ns;
 uint64_t control_timeout_ns,retirement_grace_ns;
#ifdef CP_TEST_ONLY
 uint32_t fixture_mode; /* NEVER product ABI. 1 real/unlocked, 2 delayed mock
  * match, 3 mock hang, 4 crash, 5 forged request result, 6 forced strict lock fail. */
#endif
};
struct cp_ticket {uint64_t child_generation,sequence,request_generation;};
struct cp_receipt {struct cp_ticket ticket;uint32_t status;};
struct cp_client;
/* Trusted owner only. Serialize all calls, access-policy checks/effects. Child
 * ELF path is trusted local launch configuration, never a network/path command.
 * Parent must be nonroot and sole waitpid owner, SIGCHLD default/no NOCLDWAIT;
 * fork/exec branch uses only async-signal-safe operations. Parent socket FD is
 * private: no duplication/export/inheritance. No automatic child restart.
 * Empty exec environment and fixed nonsecret argv; all unrelated child FDs
 * closed before exec, child IPC=3 only. Mandatory pidfd before any secret IPC;
 * retirement signals use only that kernel handle, never numeric PID reuse.
 * Strict ready before any secret IPC. On startup failure *out MAY retain an
 * owned client; caller must cp_destroy it, retaining it on UNCERTAIN retirement.
 * policy must outlive client. Restart requires new boot/child generations and
 * revocation of old effects; never use old tickets on new instances. */
int cp_start(struct cp_client **,const char *child_elf,const struct cp_options *,
 const struct cw_ui_material *dummy,struct camera_access_policy *policy);
/* Credential changes revoke this client's tracked principals/pending jobs first.
 * Owner MUST also revoke/update the policy account and all external effects BEFORE
 * calling; re-enable policy grants only after successful ACK. No persistence. */
int cp_put_ui(struct cp_client *,uint32_t,const char [CAMERA_ID_BYTES],uint64_t,const struct cw_ui_material *);
int cp_revoke(struct cp_client *,uint32_t,const char [CAMERA_ID_BYTES],uint64_t);
/* One outstanding expensive job, no parent secret queue. request_generation
 * strictly increases over accepted requests; pending binding copied internally.
 * Caller wipes all source passwords/material and HTTP/transport buffers. */
int cp_submit(struct cp_client *,uint32_t,const char [CAMERA_ID_BYTES],uint64_t request_generation,
 const unsigned char *,size_t,uint64_t deadline_ns,struct cp_ticket *);
int cp_cancel(struct cp_client *,const struct cp_ticket *);
/* Owner-local irrevocable grant discard, NOT CPU cancellation/retirement.
 * Exact outstanding ticket only; preserves child, other issued principals and
 * all throttle/generation history. Slot remains BUSY until a bound child RESULT
 * after the original real KDF's local wipe/forget. Original hard deadline still
 * retires child on overrun; no deadline extension/restart/parallel KDF. */
int cp_discard(struct cp_client *,const struct cp_ticket *);
/* Revoke the internally tracked exact policy snapshot BEFORE slot reclamation.
 * Only exact owner-held snapshots previously issued by this client match; a
 * mutation/foreign/stale/replayed value cannot select a victim or reset account
 * session-generation/serial history. No wire/HTTP principal-release operation. */
int cp_release_access(struct cp_client *,const struct camera_access_principal *);
/* Pump bounded IPC and observe owned child exit. Call on owner loop and before
 * effects. Deadline/cancel revoke grants immediately; CPU retirement is true
 * ONLY after waitpid observes this child's exit. On lost/expired/cancelled child
 * begin shutdown; after grace signal only this kernel-pidfd child, then observe
 * exit. No asynchronous pthread cancellation, no arbitrary PID API. */
int cp_step(struct cp_client *,struct cp_receipt *);
int cp_issue_access(struct cp_client *,const struct cp_ticket *,uint64_t session_lifetime_ns,
 struct camera_access_principal *);
/* Completed job consumes slot exactly once; cancel/timeout closes client and
 * needs retirement, not reusable verification storage before observed exit. */
int cp_forget(struct cp_client *,const struct cp_ticket *);
/* Bounded grace then pidfd SIGKILL of ONLY its own spawned child. Returns OK only on
 * actual waitpid receipt; otherwise UNCERTAIN and owner retains object. Never
 * frees client while child retirement is unobserved. Revokes tracked principals. */
int cp_retire(struct cp_client *);
int cp_destroy(struct cp_client **);
int cp_healthy(const struct cp_client *);
int cp_retirement_observed(const struct cp_client *);
#ifdef CP_TEST_ONLY
int cp_test_inject(struct cp_client *,const void *,size_t);
int cp_test_hardening_flags(const struct cp_client *);
#endif
#endif
