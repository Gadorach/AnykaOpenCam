/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_BOOTSTRAP_V1_H
#define QC07F_BOOTSTRAP_V1_H
#include "../qc07f-owned-runtime/owned_runtime_v1.h"
#include "../rtsp-provisioner-process/provisioner_process.h"
#if defined(RTSP_HANDOFF_HOST_PUBLICDUMMY) || defined(IS_HOST_PUBLIC_ONLY) || defined(RV_TEST_PUBLIC_ONLY)
#error "Bootstrap has no fixture/native fallback class"
#endif

/* Source progress ONLY, never native permission/readiness or retirement. */
enum qc07f_bootstrap_phase {
 QC07F_BS_EMPTY,QC07F_BS_PROVISIONING,QC07F_BS_PROVISIONED,
 QC07F_BS_STARTING,QC07F_BS_READY,QC07F_BS_START_FAILED,
 QC07F_BS_FINISHING,QC07F_BS_FINISHED_DEVICE_FREE,QC07F_BS_HOLD
};
enum qc07f_bootstrap_error {
 QC07F_BS_E_NONE,QC07F_BS_E_ORDER,QC07F_BS_E_PROVISIONER,
 QC07F_BS_E_BIND,QC07F_BS_E_PREPARE,QC07F_BS_E_START,QC07F_BS_E_FINISH
};
enum qc07f_bootstrap_result {
 QC07F_BS_OK=0,QC07F_BS_MORE=1,QC07F_BS_REFUSED=-1,QC07F_BS_BUSY=-2
};

/* Static persistent storage initialized ONCE. ALL fields private after begin,
 * including cfg, runtime/owner and callback context; no handles may be borrowed
 * for activation/serve. Trusted C, not hostile-memory encapsulation. No copy,
 * reset, destructor, scope exit, reinit, aliases, extra fork, signals, external
 * reaper, threads or callback reentry. Read CONTRACT.md before any future use.
 * Hold whole object/config/graph/routes/context/borrowed directory/linked file
 * and actual process namespace through failure and verifier retirement. No
 * automatic FD close/unlink, cleanup, retry, native init or main is supplied.
 */
struct qc07f_bootstrap_v1 {
 const struct qc07f_bootstrap_v1 *self;
 struct candidate_config *config;
 const struct camera_rtsp_registry *routes;
 void *bootstrap;
 struct qc07f_owned_runtime_v1 runtime;
 struct candidate_prestarted owner;
 struct rtsp_provisioner_process provisioner;
 enum qc07f_bootstrap_phase phase;
 enum qc07f_bootstrap_error error;
 int bind_result,prepare_result,start_result,finish_result;
 unsigned finish_attempted;
};
#define QC07F_BOOTSTRAP_INIT { .owner=CANDIDATE_PRESTARTED_INIT, \
 .provisioner=RTSP_PROVISIONER_PROCESS_INIT, .phase=QC07F_BS_EMPTY }

/* ONE process-local attempt. All public config/callback policy supplied before
 * begin, material_fd=-1, cfg.context=&owner_storage.runtime (future address).
 * Runtime remains zero until prepare; callbacks then use runtime.bootstrap.
 * Real random_ready/retain_verifier and secret-free/device-free parent, authorized
 * external ingress/writer, native image/hardening, FD/environment/cap hygiene and
 * no earlier native/media/prepare/start ANYWHERE are TRUSTED prerequisites, not
 * metadata/Boolean proof. Callbacks themselves stay device-free in this slice.
 * Actual provisioner begin consumes canonical *ingress_slot; even preflight/fork
 * failure retains it in provisioner. Invalid/null/bootstrap identity or reentry
 * before admission leaves caller slot untouched. No parent secret read.
 */
int qc07f_bootstrap_begin_v1(struct qc07f_bootstrap_v1 *,int *ingress_slot,
 int directory_fd,struct candidate_config *,const struct camera_rtsp_registry *,
 void *stable_public_bootstrap,uint32_t budget_ms);
/* Observe never auto starts. Still delegates actual provisioner observation on
 * HOLD when that module retains a pending/held known own child. Never clears HOLD.
 */
int qc07f_bootstrap_observe_v1(struct qc07f_bootstrap_v1 *);
/* Explicit actual READY provisioner -> bind -> prepare -> same owner's start.
 * No postfork bind, native init/provider/backing/listener or serve export.
 * Start failure retains whole owner; START_FAILED may need explicit finish.
 */
int qc07f_bootstrap_start_v1(struct qc07f_bootstrap_v1 *);
/* ONE explicit verifier-only finish, only this slice's actual READY/START_FAILED.
 * No native activation is available here; private handles MUST NOT be used to
 * activate elsewhere. No fresh/backend field is global native-ownership proof.
 * Actual finish0+PS_FINISHED -> FINISHED_DEVICE_FREE; any error -> sticky HOLD.
 * UNKNOWN invokes actual caller retain_verifier as defined by backend, retains
 * object + child namespace, never retry. Config still owns original material_fd;
 * linked nlink1 file/directory lifetime and later retirement remain caller policy.
 */
int qc07f_bootstrap_finish_device_free_v1(struct qc07f_bootstrap_v1 *);
#endif
