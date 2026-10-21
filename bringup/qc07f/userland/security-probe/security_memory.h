/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_SECURITY_MEMORY_H
#define QC07F_SECURITY_MEMORY_H
#include "security_rng.h"
#include <stdint.h>

/* Parent correction2026-10-11. Not MemAvailable; no CMA allocation by this code. */
#define SECURITY_MIB (1024U*1024U)
#define SECURITY_USABLE_RAM_BYTES (40U*SECURITY_MIB)
#define SECURITY_CMA_BYTES (16U*SECURITY_MIB)
#define SECURITY_NONCMA_BYTES (24U*SECURITY_MIB)
#define SECURITY_HEAP_LIMIT_BYTES SECURITY_MIB
#define SECURITY_ALLOCATION_LIMIT_BYTES (128U*1024U)
#define SECURITY_MEMORY_SLOTS 256U
#define SECURITY_HTTPS_PEERS 2U
#define SECURITY_HTTPS_HANDSHAKES 1U
#define SECURITY_SEND_PEER_BYTES (16U*1024U)
#define SECURITY_SEND_TOTAL_BYTES (32U*1024U)
#define SECURITY_PRETLS_TIMEOUT_MS 5000U
enum security_resource_status {
 SECURITY_RESOURCE_OK=0,SECURITY_RESOURCE_INVALID=-10,
 SECURITY_RESOURCE_LIMIT=-11,SECURITY_RESOURCE_ALLOC_FAILED=-12,
 SECURITY_RESOURCE_OWNERSHIP=-13,SECURITY_RESOURCE_NOT_READY=-14,
 SECURITY_RESOURCE_STALE=-15,SECURITY_RESOURCE_TIMEOUT=-16
};
typedef void *(*security_allocate_fn)(void *,size_t);
typedef void (*security_deallocate_fn)(void *,void *);
struct security_allocation {void *pointer;size_t bytes;};
/* Stable address, caller-serialized; no reentrant allocator seam or reinit live.
 * Tracking slots are outside allocations: rejecting foreign/double free NEVER
 * dereferences unowned or already-freed memory. No reclaim-on-pressure. */
struct security_memory {
 security_allocate_fn allocate;security_deallocate_fn deallocate;void *context;
 size_t limit,single_limit,used,high_water;unsigned active,high_active;
 unsigned initialized,fatal;int last_status;uint64_t allocations,failures;
 struct security_allocation owned[SECURITY_MEMORY_SLOTS];
};
int security_memory_init(struct security_memory *,size_t limit,size_t single_limit,
 security_allocate_fn,security_deallocate_fn,void *);
void *security_memory_calloc(struct security_memory *,size_t count,size_t bytes);
int security_memory_free(struct security_memory *,void *pointer);
#ifdef SECURITY_WITH_MBEDTLS
/* Install before ANY library allocation/PSA/Mongoose startup. No rebinding.
 * Requires MBEDTLS_PLATFORM_C + MBEDTLS_PLATFORM_MEMORY in the library build.
 * Also provides mg_calloc/mg_free with MG_ENABLE_CUSTOM_CALLOC=1. */
int security_memory_bind_library(struct security_memory *);
#endif
struct security_admission;
struct security_peer_token {
 const struct security_admission *owner;uint64_t id;unsigned slot;
};
struct security_peer {
 uint64_t id,deadline_ms;size_t send_bytes;unsigned handshake,tls_ready;
};
/* Admission/accounting only, not a network server or authentication engine.
 * The owner must reserve BEFORE mg_send/allocation and close actual sockets
 * when rejected/expired/halted. No plaintext serving while TLS init is pending. */
struct security_admission {
 const struct security_rng *rng;const struct security_memory *memory;
 struct security_peer peers[SECURITY_HTTPS_PEERS];
 uint64_t next_id,last_ms;size_t send_bytes,high_send;
 unsigned initialized,stopped,connections,handshakes,high_connections,high_handshakes;
};
int security_admission_init(struct security_admission *,const struct security_rng *,
 const struct security_memory *);
void security_admission_halt(struct security_admission *);
int security_admission_open(struct security_admission *,uint64_t now_ms,
 struct security_peer_token *);
int security_admission_handshake_begin(struct security_admission *,
 struct security_peer_token,uint64_t now_ms);
int security_admission_handshake_finish(struct security_admission *,
 struct security_peer_token,uint64_t now_ms,int tls_success);
int security_admission_send_reserve(struct security_admission *,
 struct security_peer_token,size_t bytes);
int security_admission_send_release(struct security_admission *,
 struct security_peer_token,size_t bytes);
int security_admission_close(struct security_admission *,struct security_peer_token);
/* Retires pre-TLS/handshake tokens; owner sweeps corresponding actual sockets.
 * Returns count>=0 or negative status. Clock regression halts all admission. */
int security_admission_expire(struct security_admission *,uint64_t now_ms);
#endif
