/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PTZ_CANDIDATE_AUTH_H
#define PTZ_CANDIDATE_AUTH_H
#include <camera/access_policy.h>
#ifdef __cplusplus
extern "C" {
#endif
#define PTZ_ASSOC_MAX 4U
/* Trusted graph/policy binding, NOT a wire object or authentication engine.
 * Zero-initialize, initialize once, serialize policy changes and all effects.
 * Graph is an actual owner-supplied graph; PUBLICDUMMY fixtures are not retail.
 * Callers must not mutate/copy initialized bindings. Policy outlives all work. */
struct ptz_auth {
 const struct ptz_auth *self;
 struct camera_access_policy *policy;
 char head[CAMERA_ID_BYTES];
 char sensors[PTZ_ASSOC_MAX][CAMERA_ID_BYTES];
 uint32_t sensor_count;
};
int ptz_id_valid(const char id[CAMERA_ID_BYTES]);
int ptz_overlap(const void *a,size_t an,const void *b,size_t bn);
int ptz_auth_init(struct ptz_auth *binding,struct camera_access_policy *policy,
 const struct camera_graph *graph,const char head[CAMERA_ID_BYTES]);
/* Both MOVE and client STOP require the real shared PTZ_CONTROL grant.
 * Sensor association is verified separately: shared policy PTZ requests MUST
 * have zero sensor/profile fields. ONVIF mapping is enforced by shared policy.
 * This API does not consume camera_access_verified or issue principals. */
int ptz_auth_check(struct ptz_auth *binding,const struct camera_access_principal *principal,
 const char sensor[CAMERA_ID_BYTES],uint64_t now_ns);
int ptz_same_principal(const struct camera_access_principal *a,
 const struct camera_access_principal *b);
#ifdef __cplusplus
}
#endif
#endif
