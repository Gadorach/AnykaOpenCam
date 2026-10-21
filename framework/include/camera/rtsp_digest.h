/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_DIGEST_H
#define CAMERA_RTSP_DIGEST_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CAMERA_DIGEST_NONCES 8U
#define CAMERA_DIGEST_NAME_MAX 64U
#define CAMERA_DIGEST_URI_MAX 512U
#define CAMERA_DIGEST_CNONCE_MAX 64U
#define CAMERA_DIGEST_NONCE_BYTES 64U
#define CAMERA_DIGEST_PASSWORD_MAX 128U
#define CAMERA_DIGEST_TTL_MAX_MS 300000U
enum camera_digest_algorithm { CAMERA_DIGEST_SHA256=1, CAMERA_DIGEST_MD5_LEGACY=2 };
enum camera_digest_result {
 CAMERA_DIGEST_OK=0, CAMERA_DIGEST_INVALID=-1, CAMERA_DIGEST_DENIED=-2,
 CAMERA_DIGEST_STALE=-3, CAMERA_DIGEST_REPLAY=-4, CAMERA_DIGEST_FULL=-5,
 CAMERA_DIGEST_RANDOM_FAILED=-6, CAMERA_DIGEST_CLOCK_FAILED=-7,
 CAMERA_DIGEST_EXHAUSTED=-8, CAMERA_DIGEST_CRYPTO_FAILED=-9
};
struct camera_digest_span { const unsigned char *data; size_t length; };
/* Bounded, already unquoted fields, NOT an Authorization wire parser. Caller
 * rejects duplicate/missing/unknown directives, Basic, userhash/username*, sess
 * variants and auth-int; no implied MD5 or no-qop fallback. Method/target come
 * ONLY from the actual parsed request, never from an Authorization directive. */
struct camera_digest_fields {
 struct camera_digest_span username,realm,uri,algorithm,qop,nonce,cnonce,nc,response;
};
typedef int (*camera_digest_random_fn)(void *,unsigned char *,size_t);
/* Static owner storage, not editable application state. One RTSP account per
 * core and one explicitly fixed algorithm; caller bounds any outer account set.
 * No UI/admin/PBKDF2 inputs. HA1 is a digest-equivalent SECRET. Serialize all
 * calls and media effects. No allocation, I/O, logging, PRNG or internal clock.
 * Random callback returns 0 ONLY after filling ALL requested bytes with ready
 * CSPRNG output; target entropy readiness is the integrating owner's duty. */
struct camera_digest_nonce {
 /* Last accepted cnonce is bounded bookkeeping only, never a replay key or pin.
  * nc is strictly increasing globally for this server nonce across cnonces. */
 unsigned char nonce[CAMERA_DIGEST_NONCE_BYTES],cnonce[CAMERA_DIGEST_CNONCE_MAX];
 size_t cnonce_length;
 uint64_t serial,connection,scope,generation,expires_ms;
 uint32_t last_nc,used;
};
struct camera_rtsp_digest {
 unsigned char username[CAMERA_DIGEST_NAME_MAX],realm[CAMERA_DIGEST_NAME_MAX],ha1[32];
 size_t username_length,realm_length;
 uint64_t account,generation,serial,last_ms,scope_mask;
 uint32_t algorithm,ttl_ms,initialized,enabled,clock_failed,exhausted;
 camera_digest_random_fn random; void *random_context;
 struct camera_digest_nonce nonces[CAMERA_DIGEST_NONCES];
};
/* Trusted owner fact, NOT a bearer token or a shared-access-policy principal.
 * Per-effect check is required for ongoing media; request verification is still
 * required for every subsequent RTSP command. No authorization of another URI,
 * profile, client or operation follows from this value. */
struct camera_digest_grant {
 uint64_t account,generation,serial,connection,scope,expires_ms;
};
/* TRUSTED C BUFFER CONTRACT (not a wire/adversarial-pointer API):
 * - Caller supplies valid typed writable owner/output objects and readable
 *   input objects/span bytes for the full declared lengths throughout the call.
 * - For APIs taking owner s, external inputs MUST be disjoint from the ENTIRE
 *   mutable owner struct, including nonce/credential storage (init/provision/
 *   verify/check). Owner s itself is naturally the intended write destination.
 * - Each out[32], issue nonce[64] or verify grant MUST be disjoint from all
 *   input objects and bytes AND the entire owner and random callback context.
 *   Read-only inputs MAY alias each other (e.g. actual_uri and fields.uri).
 * - ha1/response helpers clear out[32] BEFORE validation; in-place hashing or
 *   response output overlapping HA1/password/any input is UNSUPPORTED.
 * - init clears the entire owner before reading realm; provision clears nonce
 *   state/HA1/username before copying credentials. NEVER pass s->ha1, s->realm,
 *   s->username or nonce-backed inputs to these mutating calls; copy to separate
 *   trusted storage first. init is never a live credential replacement call.
 * - issue clears nonce[64], verify clears grant before tick/field validation;
 *   passing owner/credential/input-backed output would corrupt it, even on a
 *   failed call. check also ticks and may erase expired owner nonce storage.
 * - Callback context lives independently until destroy; callback must not
 *   reenter/mutate owner/input/output objects or retain its scratch buffer.
 * There is NO runtime overlap detection or rejection guarantee. These caller
 * invariants are mandatory; malformed-field tests do NOT prove alias safety. */
/* Init once at owner startup, never reset/reinitialize a live core. Restart must
 * discard all old connections/grants and obtain fresh qualified random output.
 * Realm and username are restricted printable ASCII, no colon/quote/backslash;
 * no charset/userhash feature is advertised. Config algorithm is immutable. */
int camera_rtsp_digest_init(struct camera_rtsp_digest *,uint32_t algorithm,
 struct camera_digest_span realm,uint32_t ttl_ms,uint64_t now_ms,
 camera_digest_random_fn,void *);
/* Separate RTSP-only provisioning helper, no password persistence or defaults.
 * Output is SECRET; caller zeroizes password/HA1 copies via its crypto library.
 * Password is bounded opaque octets; API does not implement Unicode policy. */
int camera_rtsp_digest_ha1(uint32_t algorithm,struct camera_digest_span username,
 struct camera_digest_span realm,struct camera_digest_span password,unsigned char out[32]);
/* Stateless Digest calculation, also for public RFC vector qualification.
 * All spans bounded; out SECRET; output tail beyond algorithm size is zero.
 * Hash/constant-time/zeroization implementations are real vendored Mbed TLS. */
int camera_rtsp_digest_response(uint32_t algorithm,const unsigned char ha1[32],
 struct camera_digest_span method,struct camera_digest_span uri,
 struct camera_digest_span nonce,struct camera_digest_span nc,
 struct camera_digest_span cnonce,unsigned char out[32]);
/* Increments boot-local RTSP credential generation and clears all nonces/grants.
 * Scope mask uses one bit per caller-defined authorized resource/operation pair.
 * Every issue/verify/check requires exactly ONE bit. No wildcard scope. */
int camera_rtsp_digest_provision(struct camera_rtsp_digest *,uint64_t account,
 struct camera_digest_span username,const unsigned char *ha1,size_t ha1_bytes,uint64_t scope_mask);
int camera_rtsp_digest_invalidate(struct camera_rtsp_digest *); /* revoke + wipe secret */
int camera_rtsp_digest_issue(struct camera_rtsp_digest *,uint64_t connection,
 uint64_t scope,uint64_t now_ms,unsigned char nonce[CAMERA_DIGEST_NONCE_BYTES]);
int camera_rtsp_digest_verify(struct camera_rtsp_digest *,uint64_t connection,
 uint64_t scope,uint64_t now_ms,struct camera_digest_span actual_method,
 struct camera_digest_span actual_uri,const struct camera_digest_fields *,struct camera_digest_grant *);
int camera_rtsp_digest_check(struct camera_rtsp_digest *,uint64_t connection,
 uint64_t scope,uint64_t now_ms,const struct camera_digest_grant *);
void camera_rtsp_digest_close(struct camera_rtsp_digest *,uint64_t connection);
void camera_rtsp_digest_destroy(struct camera_rtsp_digest *);
#ifdef __cplusplus
}
#endif
#endif
