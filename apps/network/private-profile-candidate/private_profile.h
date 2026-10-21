/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PRIVATE_PROFILE_CANDIDATE_H
#define PRIVATE_PROFILE_CANDIDATE_H
#include <stdint.h>
#include <stddef.h>
#include <mbedtls/gcm.h>
#ifdef __cplusplus
extern "C" {
#endif
#define PP_CLEAR_BYTES 256U
#define PP_ENVELOPE_BYTES 384U
#define PP_BUNDLE_BYTES 848U
#define PP_LEDGER_BYTES 80U
#define PP_PROFILE_SLOTS 2U
#define PP_HOST_LOCK_LIMIT (4U*1024U*1024U)
enum pp_mode {PP_NATIVE=0,PP_HOST_PUBLICDUMMY_4M=1};
enum pp_result {PP_OK=0,PP_HOLD=-1,PP_INVALID=-2,PP_AUTH=-3,PP_IO=-4,
 PP_UNCERTAIN=-5,PP_CONFLICT=-6,PP_CAPACITY=-7,PP_EMPTY=-8,PP_EXHAUSTED=-9};
enum pp_fault_point {PP_WRITE=1,PP_FILE_SYNC,PP_RENAME,PP_DIR_SYNC,PP_READBACK,PP_ENTROPY,PP_AEAD};
/* SOURCE_PRIVATE internal-only object. Never use for public reports, URLs,
 * cookies, metadata, logs or hashes. No defaults/extensions/open AP. */
struct pp_profile {
 uint32_t ssid_bytes,psk_bytes,hostname_bytes,ipv4_mode;
 uint32_t ipv4,prefix,gateway,dns1,dns2,ipv6_mode;
 unsigned char ssid[32],psk[64],hostname[64];
};
struct pp_handle {uint64_t scope_id,owner_epoch,key_epoch,profile_id,revision;};
/* Scalar SOURCE authority, not secret/hash/challenge or an authentication grant.
 * nonce_floor is GLOBAL for the actual master key across ALL scopes/owners;
 * committed_revision is scoped by (scope_id,owner_epoch). Getter returns current
 * source state, never echoes selectors/cached options/disk metadata. */
struct pp_authority {uint64_t scope_id,owner_epoch,key_epoch,nonce_floor,committed_revision;};
struct pp_options {
 enum pp_mode mode;
 uint64_t scope_id,owner_epoch,key_epoch,trusted_nonce_floor,trusted_commit_revision;
 int (*public_id)(void *,uint64_t);void *id_context;
 /* Mandatory source-owned GLOBAL per-master-key durable CAS ledger. Must
  * reserve new revision before returning success, across stores/restarts.
  * Internal authenticated nonce.ledger is an additional write-ahead fence,
  * NOT a substitute for rollback-resistant key/epoch/checkpoint ownership. */
 int (*reserve)(void *,uint64_t,uint64_t,uint64_t);void *reserve_context;
 /* Separate rollback-resistant committed-current CAS checkpoint. A burned
  * nonce revision is not a profile commit. Failure after file sync is HOLD. */
 int (*commit)(void *,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);void *commit_context;
 /* Required LIVE getter: selectors scope/owner, active key epoch and global
  * floor +scoped commit returned. Admission expectations never refreshed from
  * getter data to accept stale files. Unknown/change/regression => HOLD.
  * Source authority/ownership protections span publication/private use; scalar
  * sampling is not atomicity or full native physical checkpoint evidence. */
 int (*authority)(void *,uint64_t,uint64_t,struct pp_authority *);void *authority_context;
 int (*fault)(void *,uint32_t);void *fault_context;
};
/* PRIVATE lifetime storage, statically allocated/nonmovable. Contains secrets,
 * salt/nonce/tag/ciphertext and crypto allocator cells. Do not inspect/serialize,
 * log/hash/copy it. Only one active session per isolated crypto build/process;
 * serialize all calls/files/effects. Inputs/outputs cannot overlap context or
 * one another. Native mode ALWAYS HOLD before key read (unqualified unseal,
 * entropy/global ledger/channel/isolation/backing-store gates). Host mode is
 * available ONLY in PP_PUBLIC_DUMMY_BUILD; real syscall checks + owned-region
 * mlock are not full-process/native2MiB/filter/SO_PEERCRED qualification. */
union pp_cell {uint64_t align;unsigned char bytes[512];};
struct pp_private {
 unsigned char master[32],derived[32],ledger_key[32];
 unsigned char clear[256],envelope[384],aad[128],info[96];
 unsigned char bundle[848],readback[848],ledger[80];
 struct pp_profile profile;
 mbedtls_gcm_context gcm;
 union pp_cell cells[8];uint32_t used[8];
};
struct pp_session {
 struct pp_options options;struct pp_private secret;
 int dirfd;uint32_t initialized,locked,key_loaded,opened,uncertain,count;
 uint64_t nonce_floor,current_revision,store_nonce_revision;
};
/* No privilege/limit raises, no fabricated readiness flags. Caller establishes
 * nonroot equal real/effective UID/GID, queried bounded group set/caps0, core0/nodump/NNP and
 * an actual memlock limit <=HOST4M before this function; HOST groups are NOT
 * claimed empty (native closure/group drop is HOLD). It checks actual kernel
 * state and mlocks owned region before any key/private file read. */
int pp_session_init(struct pp_session *,const struct pp_options *);
/* Trusted PRE-OPENED readonly owned0600 regular/nlink1 EXACT32-byte FD.
 * Native HOLD precedes fstat/read. Caller root bootstrap/peer PID/SO_PEERCRED,
 * qualified device key unseal and key rotation are absent, not inferred. */
int pp_import_key_fd(struct pp_session *,int);
int pp_store_open(struct pp_session *,int);
int pp_seed_ledger(struct pp_session *);
/* CAS current_revision, nonsecret owner-minted profile ID; secret input remains
 * caller-owned SOURCE_PRIVATE (caller zeroizes its original). Two retained slots
 * then CAPACITY; no GC/reset/prune/overwrite foreign content or repair. */
int pp_store_put(struct pp_session *,uint64_t,uint64_t,const struct pp_profile *);
/* No plaintext from public query. Handle exported only after authenticated
 * envelope +strict private codec +trusted ID/revision expectation. */
int pp_store_handle(struct pp_session *,uint64_t,uint64_t,struct pp_handle *);
typedef int (*pp_private_use_fn)(void *,const struct pp_profile *);
/* INTERNAL trusted private effect owner only, not a network/public callback.
 * Borrow lasts only until callback returns; all owned working material wiped. */
int pp_store_use_private(struct pp_session *,uint64_t,uint64_t,pp_private_use_fn,void *);
void pp_session_close(struct pp_session *);
/* Strict private codec; no public output/hash. Caller-owned codec buffers are
 * SOURCE_PRIVATE and must be zeroized after use, including rejection. */
int pp_private_encode(const struct pp_profile *,unsigned char[PP_CLEAR_BYTES]);
int pp_private_decode(const unsigned char[PP_CLEAR_BYTES],struct pp_profile *);
#ifdef __cplusplus
}
#endif
#endif
