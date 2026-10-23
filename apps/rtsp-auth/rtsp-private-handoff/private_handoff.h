/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_PRIVATE_HANDOFF_H
#define QC07F_RTSP_PRIVATE_HANDOFF_H
#include "../rtsp-startup-material/startup_material.h"
#include <sys/stat.h>

#define RTSP_PRIVATE_HANDOFF_FILE "rtsp-startup.material"
enum rtsp_private_handoff_phase {
 RTSP_PH_EMPTY,RTSP_PH_ATTEMPTING,RTSP_PH_REJECTED,
 RTSP_PH_CREATED_PARTIAL,RTSP_PH_WRITER_CLOSE_UNKNOWN,
 RTSP_PH_CLOSED_FILE_HOLD,RTSP_PH_READONLY_FILLED,
 RTSP_PH_READONLY_HOLD,RTSP_PH_READY,RTSP_PH_MOVED
};
/* Stable trusted caller storage; fields are private after first call. No copy,
 * reset, duplicate ownership, concurrent access, signal/reentry or retry. The
 * directory FD is BORROWED and must remain exclusive/live until move completes.
 * All inputs/scratch/owner/config storage are valid, disjoint and serialized.
 * Preserve this object and all partial output on failure; there is no destructor.
 * WRITER_CLOSE_UNKNOWN retains uncertainty, NOT a retryable descriptor number.
 */
struct rtsp_private_handoff {
 const struct rtsp_private_handoff *self;
 const struct candidate_config *config;
 const struct camera_rtsp_registry *routes;
 enum rtsp_private_handoff_phase phase;
 int directory_fd,writer_fd,readonly_fd;
 unsigned created,identity_valid,writer_close_known,file_synced;
 size_t bytes_written;
 struct stat directory_inode,file_inode; /* metadata only, never payload */
};
#define RTSP_PRIVATE_HANDOFF_INIT { .phase=RTSP_PH_EMPTY, .directory_fd=-1, \
 .writer_fd=-1, .readonly_fd=-1 }

/* Separate compile-time symbols: never a HOST fallback in native code. */
#ifdef RTSP_HANDOFF_HOST_PUBLICDUMMY
#define rtsp_private_handoff_create qc07f_rtsp_publicdummy_handoff_create_v1
#define rtsp_private_handoff_move qc07f_rtsp_publicdummy_handoff_move_v1
#else
#define rtsp_private_handoff_create qc07f_rtsp_private_handoff_create_v1
#define rtsp_private_handoff_move qc07f_rtsp_private_handoff_move_v1
/* ONLY in the separate provisioner, BEFORE authenticated secret ingress.
 * Actual root/no-groups, core=0, nondumpable, NNP and current/future memory lock.
 * Process-wide irreversible/partial effects; no rollback or permission token.
 * Caller still implements ingress/authorization, caps/environment/FD policy,
 * real process separation and password wiping. Never call in media/UI/HTTPS.
 */
int qc07f_rtsp_private_provisioner_harden_v1(void);
#endif

/* One attempt: actual constructor -> exclusive fixed-file write -> single close
 * -> actual readonly CLOEXEC FD. No payload readback/hash/log or implicit cleanup.
 * Scratch is crypto-cleared on ALL returns; password remains borrowed/unmodified.
 * Native repeats actual hardening; earlier pre-ingress hardening remains required.
 * See CONTRACT.md for namespace/no-alias trust prerequisites and failure custody.
 */
int rtsp_private_handoff_create(struct rtsp_private_handoff *,int directory_fd,
 const struct candidate_config *,const struct camera_rtsp_registry *,
 struct camera_digest_span username,struct camera_digest_span realm,
 struct camera_digest_span password,struct rv_material *scratch);

/* EXPLICIT final step immediately BEFORE owned-runtime prepare/start. Same config
 * address/immutable contents; material_fd MUST be -1. Actual prestarted owner must
 * be fresh, and caller guarantees prepare/start never ran anywhere for this config.
 * Rechecks inode/metadata/access/offset, then MOVES FD into config, no alias.
 * Any failed attempted move preserves readonly FD and becomes READONLY_HOLD.
 * No backend call; bootstrap owns moved FD even if subsequent prepare/start fails.
 */
int rtsp_private_handoff_move(struct rtsp_private_handoff *,
 struct candidate_config *,const struct candidate_prestarted *unused_owner);
#endif
