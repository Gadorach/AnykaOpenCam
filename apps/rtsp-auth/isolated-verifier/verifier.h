/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTSP_ISOLATED_VERIFIER_H
#define RTSP_ISOLATED_VERIFIER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RV_ABI 1U
#define RV_FD 3
#define RV_MATERIAL_FD 4
#define RV_HEADER 80U
#define RV_PAYLOAD 8192U
#define RV_FLAGS15 15U
#define RV_NATIVE 1U
#define RV_HOST_PUBLIC_4M 2U
#define RV_PEERS 4U
enum rv_kind {RV_READY=1,RV_OPEN,RV_CHALLENGE,RV_VERIFY,RV_RESULT,RV_RETIRE_RESULT,
 RV_CHECK,RV_CLOSE,RV_REVOKE,RV_ACK,RV_SHUTDOWN,RV_BYE};
enum rv_rc {RV_OK=0,RV_MORE=1,RV_BUSY=-100,RV_LOST=-101,RV_BAD=-102,RV_DENIED=-103,RV_UNCERTAIN=-104,RV_HARDENING=-105};
/* One fixed RTSP-only trusted startup FD record. No plaintext password or UI
 * PBKDF2/salt/verifier fields. Little-endian owned-process source ABI, sized,
 * zero-initialized trusted bootstrap only; never a network configuration object.
 * Descriptor must be nlink0 completely sealed memfd, or nlink1 private readonly
 * regular FD, exact size. Readonly descriptor/mode are NOT proof that no other
 * writer exists: trusted bootstrap must quiesce writers before delegation.
 * Caller closes/wipes its own copies; no value or payload hash may be logged. */
struct rv_material {
 uint32_t abi,bytes,domain,algorithm;
 uint64_t epoch,boot_generation,credential_generation,account;
 uint32_t username_bytes,realm_bytes,ha1_bytes,nonce_ttl_ms;
 uint32_t window_ms,global_verifies,global_challenges,account_verifies,account_challenges,peer_verifies,peer_challenges;
 char device[64],account_id[64],sensor[64],profile[64];
 unsigned char username[64],realm[64],ha1[32];
 char live[513],track[513];
};
/* Explicit endian wire header, bounded seqpacket, ancillary/truncation rejected.
 * Only owned private library FD. Epoch/sequence/connection/scope/gen/deadline
 * bound on every operation/result. Token is an INTERNAL child handle, not a
 * caller-verified flag, public bearer token or a per-user allocation key. */
struct rv_message {
 uint32_t kind,bytes;int32_t status;uint32_t flags;
 uint64_t epoch,sequence,connection,scope,generation,deadline,token;
 unsigned char payload[RV_PAYLOAD];
};
int rv_clock(uint64_t *);
void rv_wipe(void *,size_t);
void rv_put32(unsigned char *,uint32_t);void rv_put64(unsigned char *,uint64_t);
uint32_t rv_get32(const unsigned char *);uint64_t rv_get64(const unsigned char *);
int rv_send(int,const struct rv_message *);int rv_receive(int,struct rv_message *);
int rv_harden(unsigned *,unsigned *stage);
struct rv_engine;
/* Owned single-account state: four peers, eight core nonces, one result slot.
 * All identities/material/generations/rates/nonce replay and grant state live
 * exclusively here in child. No parent-provided verified flag/grant accepted.
 * No producer/media/DMA/codec/device/discovery interface or whole-AU buffers. */
size_t rv_engine_bytes(void);
int rv_engine_init(void *,size_t,const struct rv_material *);
int rv_engine_command(void *,const struct rv_message *,struct rv_message *);
void rv_engine_destroy(void *,size_t);
/* Adapter serializes owner calls. Startup image/material FDs are preopened,
 * owned trusted descriptors, never network paths/selectors. Child fixed argv /
 * empty environment; same owned IPC release barrier blocks exec/material until
 * immutable pidfd is actually acquired. Failure closes barrier and waits known
 * unreaped child, never numeric PID signals. Unknown retirement retains context.
 * PID and pidfd from its OWN fork only. Nonroot child and
 * native2MiB hard ceiling (HOST4MiB separate public profile). SO_PEERCRED root
 * creator/groups0 enforced native; HOST class explicitly lacks those proofs.
 * One pending submit stays BUSY after discard until bound result RETIRE ACK.
 * No cached grant TTL fallback: every effect MUST rv_query, which rechecks child
 * nonce/generation/policy/time and owned child liveness after the bound response.
 * All objects/buffers/FD lifetimes disjoint; no adversarial pointer alias support. */
struct rv_client;
/*Native: root-owned nonwritable public r-x0555/0505-style ELF32 ARM EABI5 softfloat
 * static/no interpreter/no executable stack, exactly reviewed pin. Root0500 is
 * not executable by UID1000 and is rejected BEFORE fork. Caller pre-stages code
 * inode/sibling; adapter NEVER chmods/chowns source or substitutes privileged exec.
 * HOST4M: separate correctly pinned x86_64 ELF + actual child execute permissions.
 * Source bytes hash below is an IMAGE pin, never credential/material payload hash.*/
struct rv_image {uint32_t abi,profile;int fd;size_t bytes;unsigned char sha256[32];};
int rv_start(struct rv_client **,const struct rv_image *,int material_fd);
int rv_rpc(struct rv_client *,struct rv_message *); /*only exact child-owned conn/token*/
int rv_submit(struct rv_client *,uint64_t connection,const void *actual_request,size_t,uint64_t deadline_ns);
int rv_discard(struct rv_client *);
int rv_owner_revoke(struct rv_client *); /*fixed account/profile; pending invalidated immediately, drain then retry exact control*/
int rv_complete(struct rv_client *,uint64_t *grant_token); /*drain+retire; discarded never grants*/
int rv_query(struct rv_client *,uint64_t connection,uint64_t grant_token);
int rv_stop(struct rv_client *); /*SIGTERM courtesy then owned pidfd fallback; waitpid receipt required*/
int rv_destroy(struct rv_client **); /*only after observed retirement*/
int rv_retired(const struct rv_client *);
uint32_t rv_ready_flags(const struct rv_client *);
uint32_t rv_ready_profile(const struct rv_client *);
int rv_wipe_observed(const struct rv_client *);
int rv_owned_signal(struct rv_client *,int signal); /*fixed own child handle, no numeric PID selector*/
#ifdef RV_TEST_PUBLIC_ONLY
enum rv_launch_fault {RV_TEST_PIDFD_FAIL=1,RV_TEST_RELEASE_FAIL,RV_TEST_CHILD_EXIT,
 RV_TEST_RELEASE_RECV_EINTR,RV_TEST_RELEASE_EINTR,RV_TEST_RELEASE_ZERO};
void rv_test_launch_fault(unsigned);
void rv_test_before_release(void (*owned_event)(int pidfd));
unsigned rv_test_engine_ack(const struct rv_client *);
unsigned rv_test_barrier_released(const struct rv_client *);
int rv_test_owned_pidfd(struct rv_client *); /*test duplicate of OWN handle only, never product secret IPC export*/
#endif
#ifdef __cplusplus
}
#endif
#endif
