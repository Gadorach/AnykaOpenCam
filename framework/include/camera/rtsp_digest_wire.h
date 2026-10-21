/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_DIGEST_WIRE_H
#define CAMERA_RTSP_DIGEST_WIRE_H
#include <camera/rtsp_digest.h>
#include <camera/rtsp_request.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CAMERA_DIGEST_WIRE_DIRECTIVES 9U
#define CAMERA_DIGEST_WIRE_NAME_MAX 16U
#define CAMERA_DIGEST_WIRE_RESPONSE_MAX 512U
#define CAMERA_DIGEST_WIRE_RATE_MAX 1024U
enum camera_digest_wire_result {
 CAMERA_DIGEST_WIRE_OK=0,CAMERA_DIGEST_WIRE_INVALID=-100,
 CAMERA_DIGEST_WIRE_MISSING=-101,CAMERA_DIGEST_WIRE_THROTTLED=-102,
 CAMERA_DIGEST_WIRE_CLOCK=-103,CAMERA_DIGEST_WIRE_SPACE=-104
};
enum camera_digest_wire_reason {
 CAMERA_DIGEST_WIRE_INITIAL=0,CAMERA_DIGEST_WIRE_BAD,
 CAMERA_DIGEST_WIRE_UNKNOWN_NONCE,CAMERA_DIGEST_WIRE_EXPIRED_NONCE,
 CAMERA_DIGEST_WIRE_NO_CHALLENGE
};
enum camera_digest_wire_attempt {CAMERA_DIGEST_WIRE_VERIFY=1,CAMERA_DIGEST_WIRE_CHALLENGE=2};
/* Owned secret-sensitive scratch, not a receipt or bearer credential. Fields
 * point into this object's arrays; do not copy/move/reuse before finishing use.
 * No Authorization/URI/username/response/nonce contents may enter logs/hashes. */
struct camera_digest_wire_decoded {
 unsigned char username[64],realm[64],uri[512],algorithm[16],qop[8];
 unsigned char nonce[64],cnonce[64],nc[8],response[64];
 size_t lengths[CAMERA_DIGEST_WIRE_DIRECTIVES];
 struct camera_digest_fields fields;
};
/* Fixed-window admission. Caller owns a bounded global set, a persistent bucket
 * per admitted RTSP account (including unknown-username attempts), and a bucket
 * per admitted connection. No allocation keyed by incoming names/IPs. Account
 * and global buckets survive peer churn/password changes. Equal timestamps do
 * NOT replenish credit. No configured defaults or target capacity claim. */
struct camera_digest_wire_limit {
 uint64_t window_start_ms,last_ms;
 uint32_t window_ms,verify_limit,challenge_limit,verifies,challenges;
 uint32_t initialized,clock_failed;
};
struct camera_digest_wire_nonce_note {
 unsigned char nonce[64];
 uint64_t generation,connection,scope,expires_ms;uint32_t used;
};
/* One immutable core/account binding per adapter. Shared global bucket must be
 * the SAME object across every adapter. Nonce notes are bounded expiry hints,
 * never authentication authority. Slot reuse may forget old expired hints;
 * forgotten/unknown/revoked nonces receive stale=false, never stale=true. */
struct camera_digest_wire_adapter {
 struct camera_rtsp_digest *core;
 struct camera_digest_wire_limit *global,*account;
 struct camera_digest_wire_nonce_note notes[CAMERA_DIGEST_NONCES];
 uint32_t initialized;
};
/* Trusted C object/alias contract: all writable outputs, owner structs and
 * callback context are disjoint from all input objects/bytes AND each other.
 * Read-only input spans may alias each other. Request MUST be a fresh READY
 * result of camera_rtsp_request_parse/input_next, with live borrowed views of
 * the actual Request-Line (not caller-made Authorization-derived views).
 * The parser's 8192-byte request / 1024-byte Authorization limits remain in
 * force, including fragmented/pipelined input. No pointer-overlap detection.
 * Outputs are cleared before validation; violating nonoverlap may corrupt
 * input/core/credentials even on failure. Serialize all calls and effects.
 * READY, decoded fields, nonce notes or a typed flag alone NEVER authenticate.
 * No internal allocation, clock, I/O, logging, transport or media effect. */
int camera_digest_wire_parse(const struct camera_rtsp_request *,uint32_t configured_algorithm,
 struct camera_digest_wire_decoded *);
void camera_digest_wire_clear(struct camera_digest_wire_decoded *);
int camera_digest_wire_limit_init(struct camera_digest_wire_limit *,uint64_t now_ms,
 uint32_t window_ms,uint32_t verify_limit,uint32_t challenge_limit);
/* Three distinct owner buckets, transactional credit consumption; rejected
 * admission updates clock/window observations but consumes NO attempt counts.
 * Once admitted, failed parsing/crypto/issuance still consumes the attempt.
 * Clock regression latches ALL three buckets closed; no counter wrap. */
int camera_digest_wire_admit(struct camera_digest_wire_limit *global,
 struct camera_digest_wire_limit *account,struct camera_digest_wire_limit *peer,
 enum camera_digest_wire_attempt,uint64_t now_ms);
int camera_digest_wire_adapter_init(struct camera_digest_wire_adapter *,
 struct camera_rtsp_digest *,struct camera_digest_wire_limit *global,
 struct camera_digest_wire_limit *account);
/* Admission -> strict wire parsing -> REAL Digest verification against actual
 * method/target. Returns WIRE errors or original Digest core error codes. Grant
 * only on actual core OK; caller must then map exact URI/resource/role into the
 * existing access policy, and recheck BOTH core grant and policy per effect.
 * Expiry hint is never a credential verdict and only controls a retry challenge.
 * No shared-policy principal or verified flag is manufactured by this API. */
int camera_digest_wire_verify(struct camera_digest_wire_adapter *,struct camera_digest_wire_limit *peer,
 uint64_t connection,uint64_t scope,uint64_t now_ms,const struct camera_rtsp_request *,
 struct camera_digest_grant *,enum camera_digest_wire_reason *);
/* Config-only bounded RTSP401 formatter. nonce must be a trusted freshly issued
 * 64-byte lowercase hex registry key. Only EXPIRED_NONCE emits stale=true;
 * INITIAL/BAD/UNKNOWN emit false; NO_CHALLENGE is rejected. out capacity1..512,
 * byte count excludes terminating NUL. No echo of any Authorization/request URI.
 * Requires live enabled core and its immutable safe realm/algorithm. */
int camera_digest_wire_format_challenge(const struct camera_rtsp_digest *,
 const unsigned char nonce[64],uint32_t cseq,enum camera_digest_wire_reason,
 unsigned char *out,size_t capacity,size_t *bytes);
/* Preflight formatter capacity BEFORE consuming admission/nonce. Then charge
 * global/account/peer challenge credit and issue through existing CSPRNG core.
 * Use reason returned by verify, not an untrusted wire boolean. */
int camera_digest_wire_challenge(struct camera_digest_wire_adapter *,struct camera_digest_wire_limit *peer,
 uint64_t connection,uint64_t scope,uint64_t now_ms,uint32_t cseq,
 enum camera_digest_wire_reason,unsigned char *out,size_t capacity,size_t *bytes);
/* Retires that connection's core nonces AND expiry notes, not policy/media
 * subscriptions. Parent must revoke its policy principal and release leases.
 * Never reset persistent global/account limits on disconnect. */
void camera_digest_wire_close(struct camera_digest_wire_adapter *,uint64_t connection);
void camera_digest_wire_adapter_clear(struct camera_digest_wire_adapter *);
#ifdef __cplusplus
}
#endif
#endif
