/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ISOLATED_SERVER_CANDIDATE_H
#define ISOLATED_SERVER_CANDIDATE_H
#include <camera/rtsp_server.h>
#include <camera/rtsp_digest_wire.h>
#include "verifier.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CANDIDATE_AUTH_ABI 3U
#define CANDIDATE_BEGIN_LOGICAL_ONLY 1U
#define CANDIDATE_HUB_BYTES 1048576U
struct candidate_backend;
/*PUBLIC routing/limits only. No password/HA1/username/realm/principal ingress.
 *Trusted bootstrap owns preopened descriptors; no path/PID/credential selector.
 *Same-boot CLOCK_MONOTONIC: rv_start forks without time-namespace changes.*/
struct candidate_config {
 uint32_t abi,struct_bytes,host_fixture_only,algorithm;
 uint64_t boot_generation,epoch,credential_generation;
 char device_id[CAMERA_ID_BYTES],account_id[CAMERA_ID_BYTES],profile_id[CAMERA_ID_BYTES];
 const struct camera_graph *graph;
 uint32_t nonce_ttl_ms,rate_window_ms,global_verifies,global_challenges,account_verifies,account_challenges,peer_verifies,peer_challenges;
 uint32_t host_send_chunk,begin_contract;
 struct rv_image image;int material_fd;
 void *context;
 int (*random_ready)(void *,unsigned char *,size_t); /*public RTSP session randomness only*/
 int (*source_ready)(void *,uint64_t,uint64_t);
 int (*owner_poll)(void *,struct candidate_backend *,uint64_t);
 void (*listener_ready)(void *,uint16_t);
 void (*retain_uncertain)(void *,const struct camera_packet_hub *,const struct camera_producer_demand *,unsigned);
 void (*source_guard_bind)(void *,int (*)(void *),void *);
 void (*retain_frame_uncertain)(void *,struct camera_rtsp_frame_handle,int);
 /*Mandatory copy-only ownership sink on UNKNOWN retirement. Caller retains
  *the allocated client/process namespace AND provider code/backing, not free.*/
 void (*retain_verifier)(void *,struct rv_client *);
};
struct candidate_peer {
 uint64_t connection,token;
 unsigned live,canceled,grant_present;
 char session[65];struct camera_digest_wire_limit rate;
};
struct candidate_stats {
 uint64_t opened,closed,challenges,verified,auth_denied,policy_denied,effect_denied;
 uint64_t sdp,setup,play,rtcp,rtp_calls,rtp_bytes,lease_releases,partial_discard;
 uint64_t begin_calls,next_calls,frame_ends,hub_publish_calls,boundary_denied;
 uint64_t checks,admission_checks,submitted,retired_results,discarded,busy;
 uint64_t IPC_total_ns,IPC_max_ns;
};
struct candidate_backend {
 const struct candidate_config *config;struct rv_client *child;
 struct camera_digest_wire_limit global,account;
 struct candidate_peer *peers[CAMERA_HUB_CONSUMERS],*pending_peer;
 uint64_t last_ns;uint32_t initialized,failed,pending,discarded,revoked,revoke_pending;
 struct candidate_stats stats;
};
int candidate_backend_init(struct candidate_backend *,const struct candidate_config *,uint64_t);
int candidate_backend_destroy(struct candidate_backend *);
int candidate_backend_finish(struct candidate_backend *,int);
int candidate_backend_poll(struct candidate_backend *,uint64_t);
int candidate_peer_open(struct candidate_backend *,struct candidate_peer *,unsigned,uint64_t);
void candidate_peer_close(struct candidate_backend *,struct candidate_peer *);
int candidate_request_verify(struct candidate_backend *,struct candidate_peer *,const void *,size_t,int,uint64_t,enum camera_digest_wire_reason *);
int candidate_effect(struct candidate_backend *,struct candidate_peer *,uint64_t);
int candidate_effect_final(struct candidate_backend *,struct candidate_peer *,uint64_t);
int candidate_admission_check(struct candidate_backend *,struct candidate_peer *,uint64_t);
int candidate_challenge(struct candidate_backend *,struct candidate_peer *,uint32_t,uint64_t,enum camera_digest_wire_reason,unsigned char [512],size_t *);
int candidate_backend_revoke(struct candidate_backend *);
int candidate_backend_cancel(struct candidate_backend *,uint64_t); /*0=all OWN peers*/
int camera_rtsp_isolated_candidate_serve(const struct camera_rtsp_options *,const struct camera_rtsp_producer *,const struct candidate_config *);
#ifdef __cplusplus
}
#endif
#endif
