/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_STARTUP_MATERIAL_H
#define QC07F_RTSP_STARTUP_MATERIAL_H
#include "../qc07f-owned-runtime/server/candidate_auth.h"

/* Provisioning process ONLY; do not link into media or UI/HTTPS processes.
 * Builds the ACTUAL current native verifier startup record, not a new on-disk
 * format. cfg/graph/routes must be the same immutable public values later used
 * by owned-runtime bootstrap. No credentials/defaults/epochs are invented.
 *
 * Trusted valid typed storage and borrowed spans, serialized/nonreentrant.
 * out must be fresh writable storage DISJOINT from every input object/span,
 * including cfg/graph/routes. Read-only inputs may alias each other. No overlap
 * detection: out is cleared BEFORE validation, including on failure.
 * username/realm/password and out are PRIVATE. Do not log or evidence-hash them.
 * Password remains borrowed, is not copied to out and is NOT wiped by this call.
 * Caller owns authenticated secret ingress, protected process memory, wiping its
 * password copies and private descriptor handoff. Success constructs material
 * ONLY: no persistence, publication, entropy/boot/credential-generation proof,
 * verifier launch, device readiness or safe live credential rotation follows.
 * cfg must NOT be changed once a backend starts; active replacement is absent.
 */
int qc07f_rtsp_startup_material_build_v1(const struct candidate_config *cfg,
 const struct camera_rtsp_registry *routes,struct camera_digest_span username,
 struct camera_digest_span realm,struct camera_digest_span password,
 struct rv_material *out);

/* Destroys this output only, not caller inputs, emitted FDs or other copies. */
void qc07f_rtsp_startup_material_clear_v1(struct rv_material *material);
#endif
