/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_CANDIDATE_AUTH_H
#define CAMERA_RTSP_CANDIDATE_AUTH_H
#include <camera/rtsp_server.h>
#include <camera/rtsp_digest_wire.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <camera/access_policy.h>
#define CANDIDATE_AUTH_ABI 2U
#define CANDIDATE_BEGIN_LOGICAL_ONLY 1U
#define CANDIDATE_HUB_BYTES 1048576U
struct candidate_backend;
/* Trusted independent RTSP-only bootstrap data. HA1 is a SECRET, not UI PBKDF2.
 * No password/defaults, URL credentials, wire flags or TLS configuration here.
 * Callback fills bounded lengths/bytes; candidate wipes its temporary copy. */
struct candidate_credential {
 uint32_t domain,algorithm;
 uint64_t account;
 unsigned char username[64],realm[64],ha1[32];
 size_t username_bytes,realm_bytes,ha1_bytes;
};
struct candidate_config {
 uint32_t abi,struct_bytes,host_fixture_only,algorithm;
 uint64_t boot_generation;
 char device_id[CAMERA_ID_BYTES],account_id[CAMERA_ID_BYTES],profile_id[CAMERA_ID_BYTES];
 const struct camera_graph *graph;
 uint32_t nonce_ttl_ms,rate_window_ms;
 uint32_t global_verifies,global_challenges,account_verifies,account_challenges;
 uint32_t peer_verifies,peer_challenges;
 uint32_t host_send_chunk; /*0=ordinary;1..1404 HOST forced-real-partial-write fixture only*/
 /* Mandatory SOURCE contract: BEGIN/START coordinate logical work only; no
  * capture/DMA/reset/P-reference acquisition may occur there in this candidate.
  * Native integration must move acquisition to next(), or separately review an
  * effect-gated native BEGIN. This flag is not authentication/qualification. */
 uint32_t begin_contract;
 void *context;
 int (*bootstrap_rtsp)(void *,struct candidate_credential *);
 camera_digest_random_fn random_ready; /* 0 ONLY for complete ready CSPRNG output */
 /* Mandatory bounded pure readiness check for the exact configured synthetic
  * source/profile; not authentication and not a hardware qualified flag. */
 int (*source_ready)(void *,uint64_t connection,uint64_t now_ns);
 /* Optional SERIALIZED trusted owner pump, no arbitrary signal handler mutation.
  * May use rotate/revoke/cancel below; no producer/lease/device effects or reentry.
  * Must return promptly; no timeout or CPU-budget relaxation is implemented. */
 int (*owner_poll)(void *,struct candidate_backend *,uint64_t now_ns);
 void (*listener_ready)(void *,uint16_t); /* scalar owned-loopback endpoint receipt */
 /* Mandatory copy-only retirement-uncertainty sink. Preserve fixed hub/CPU lease
  * and demand metadata before serve returns failure; never free/unload from this
  * notification. Referenced backing/lifecycle remain caller-owned and retained. */
 void (*retain_uncertain)(void *,const struct camera_packet_hub *,const struct camera_producer_demand *,unsigned slot);
 /* Pure bind/unbind. START/BEGIN and next adapters must invoke this live guard
  * immediately BEFORE source effects; no saved boolean/admission is authority.
  * It runs auth/policy/readiness only, never producer callbacks. No reentry from
  * source_ready/owner_poll. Binding is removed before the stack owner is wiped. */
 void (*source_guard_bind)(void *,int (*current_guard)(void *),void *guard_context);
 /* Exact original bracket if END fails/clock is uncertain. Copy-only, no
  * reset/P-reference release/STOP retry or false retirement from this receipt. */
 void (*retain_frame_uncertain)(void *,struct camera_rtsp_frame_handle,int end_result);
};
struct candidate_peer {
 uint64_t connection;
 unsigned live,canceled,principal_valid;
 char session[65];
 struct camera_digest_wire_limit rate;
 struct camera_digest_grant grant;
 struct camera_access_principal principal;
};
struct candidate_stats {
 uint64_t opened,closed,challenges,verified,auth_denied,policy_denied,effect_denied;
 uint64_t sdp,setup,play,rtcp,rtp_calls,rtp_bytes,lease_releases,partial_discard;
 uint64_t begin_calls,next_calls,frame_ends,hub_publish_calls,boundary_denied;
};
/* One backend/account and immutable known source graph/profile per serve.
 * Separate boot/account session sequence never resets on reconnect/rotation.
 * Fixed4 peer references; no unbounded tables. All calls/read/callbacks/effects
 * serialized; writable owner/output/context and input objects must be disjoint.
 * No adversarial pointer/alias handling or native secret-isolation claim. */
struct candidate_backend {
 const struct candidate_config *config;
 struct camera_rtsp_digest core;
 struct camera_digest_wire_adapter wire;
 struct camera_digest_wire_limit global,account;
 struct camera_access_policy policy;
 struct camera_access_request query;
 struct candidate_peer *peers[CAMERA_HUB_CONSUMERS];
 uint64_t next_connection,next_session,account_number;
 uint32_t initialized,failed;
 struct candidate_stats stats;
};
int candidate_backend_init(struct candidate_backend *,const struct candidate_config *,uint64_t now_ns);
void candidate_backend_destroy(struct candidate_backend *);
int candidate_backend_poll(struct candidate_backend *,uint64_t now_ns);
int candidate_peer_open(struct candidate_backend *,struct candidate_peer *,unsigned slot,uint64_t now_ns);
void candidate_peer_close(struct candidate_backend *,struct candidate_peer *);
int candidate_request_verify(struct candidate_backend *,struct candidate_peer *,const struct camera_rtsp_request *,uint64_t now_ns,enum camera_digest_wire_reason *);
int candidate_effect(struct candidate_backend *,struct candidate_peer *,uint64_t now_ns);
int candidate_effect_final(struct candidate_backend *,struct candidate_peer *,uint64_t now_ns); /*no external callbacks*/
int candidate_challenge(struct candidate_backend *,struct candidate_peer *,uint32_t cseq,uint64_t now_ns,enum camera_digest_wire_reason,unsigned char out[512],size_t *);
int candidate_backend_rotate(struct candidate_backend *,const struct candidate_credential *);
int candidate_backend_revoke(struct candidate_backend *);
int candidate_backend_cancel(struct candidate_backend *,uint64_t connection); /*0 = all owned peers*/
/* SOURCE/HOST CANDIDATE ONLY: distinct symbol, existing server ABI untouched.
 * Requires exact loopback bind, explicit2000ms whole-AU bound, one1MiB hub,
 * valid trusted bootstrap/graph/callbacks before creating a listener. Mandatory
 * lifecycle remains caller-owned; unsafe retirement is never called success. */
int camera_rtsp_candidate_serve(const struct camera_rtsp_options *,const struct camera_rtsp_producer *,const struct candidate_config *);
#ifdef __cplusplus
}
#endif
#endif
