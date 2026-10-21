/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_SECURITY_RNG_H
#define QC07F_SECURITY_RNG_H
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define SECURITY_RNG_MAX_BYTES 4096U
#define SECURITY_RNG_CHUNK_BYTES 256U
#define SECURITY_RNG_MAX_CALLS 32U
enum security_rng_status {
 SECURITY_RNG_OK=0, SECURITY_RNG_NOT_READY=-1, SECURITY_RNG_IO=-2,
 SECURITY_RNG_RETRY_LIMIT=-3, SECURITY_RNG_INVALID=-4,
 SECURITY_RNG_FATAL=-5, SECURITY_RNG_PSA_INIT_FAILED=-6
};
/* Returns syscall length or -1 and writes errno into error. No reentry.
 * The seam must never write outside the supplied span. */
typedef ssize_t (*security_getrandom_fn)(void *,void *,size_t,unsigned,int *);
typedef int (*security_psa_init_fn)(void *);
/* Caller-owned, serialized, stable-address context. No live reinitialization.
 * Public for static allocation; fields are read-only outside this module. */
struct security_rng {
 security_getrandom_fn syscall;void *syscall_context;
 unsigned initialized,failed,start_called,ready,psa_initialized,last_calls;
 size_t last_bytes;int status,os_error,psa_status;
};
void security_rng_zero(void *output,size_t bytes);
int security_rng_init(struct security_rng *,security_getrandom_fn,void *);
/* Zero length, NULL, or >MAX_BYTES are caller-contract errors: no syscall.
 * For a valid writable span, EVERY failure wipes the ENTIRE requested output.
 * Any failure latches failed; retry requires fresh service startup, not fallback. */
int security_rng_fill(struct security_rng *,void *output,size_t bytes);
/* Exactly once before listeners/keys/sessions; qualified OS readiness first,
 * then explicit init callback success (zero). Failed init latches failure.
 * Callback is a test seam; production must use real psa_crypto_init(). */
int security_rng_start(struct security_rng *,security_psa_init_fn,void *);
bool security_rng_can_serve(const struct security_rng *);
#ifdef SECURITY_WITH_MBEDTLS
/* Single-process library callback binding; bind once before PSA or Mongoose.
 * Context outlives ALL library calls; no threads without external serialization. */
int security_rng_bind_library(struct security_rng *);
int security_rng_library_start(struct security_rng *);
#endif
#endif
