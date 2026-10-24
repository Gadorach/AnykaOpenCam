/* SPDX-License-Identifier: GPL-2.0-only */
#include "bootstrap_v1.h"
#include "../rtsp-os-random/os_random.h"

_Static_assert(__builtin_types_compatible_p(
 __typeof__(((struct candidate_config *)0)->random_ready),
 __typeof__(&qc07f_rtsp_os_random_ready_v1)),
 "owned-runtime random_ready callback type changed");

static struct qc07f_bootstrap_v1 *process_attempt;
static unsigned active;

static int identity(const struct qc07f_bootstrap_v1 *b)
{return b&&b==process_attempt&&b->self==b;}

static void latch(struct qc07f_bootstrap_v1 *b,enum qc07f_bootstrap_error error)
{if(b->error==QC07F_BS_E_NONE)b->error=error;}

static int hold(struct qc07f_bootstrap_v1 *b,enum qc07f_bootstrap_error error)
{latch(b,error);b->phase=QC07F_BS_HOLD;return QC07F_BS_REFUSED;}

static int leave(int result)
{active=0;return result;}

int qc07f_bootstrap_begin_v1(struct qc07f_bootstrap_v1 *b,int *ingress_slot,
 int directory_fd,struct candidate_config *config,
 const struct camera_rtsp_registry *routes,void *bootstrap,uint32_t budget_ms)
{
 if(active)return QC07F_BS_BUSY;
 if(!b||process_attempt||b->self||b->phase!=QC07F_BS_EMPTY||!config||!routes)
  return QC07F_BS_REFUSED;
 if(config->random_ready)return QC07F_BS_REFUSED;
 active=1;process_attempt=b;b->self=b;b->config=config;b->routes=routes;
 b->bootstrap=bootstrap;b->phase=QC07F_BS_PROVISIONING;
 /* Select ONCE before actual provisioner snapshot; retain selection on failure.
  * No other config change or prepare before actual child retirement + bind.
  * Existing module owns the canonical ingress move, preflight, fork and waits. */
 config->random_ready=qc07f_rtsp_os_random_ready_v1;
 int r=rtsp_provisioner_begin(&b->provisioner,ingress_slot,directory_fd,
  config,routes,&b->owner,budget_ms);
 if(r!=RTSP_PP_MORE||b->provisioner.self!=&b->provisioner||
    b->provisioner.phase!=RTSP_PP_PENDING)
  return leave(hold(b,QC07F_BS_E_PROVISIONER));
 return leave(QC07F_BS_MORE);
}

int qc07f_bootstrap_observe_v1(struct qc07f_bootstrap_v1 *b)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 struct rtsp_provisioner_process *p=&b->provisioner;
 if(b->phase==QC07F_BS_HOLD){
  /* Do not drop a still-owned child just because our outer failure is sticky.
   * Underlying HOLD observe never retries unknown wait/ECHILD or clears HOLD. */
  if(p->self==p&&(p->phase==RTSP_PP_HOLD||p->phase==RTSP_PP_PENDING))
   (void)rtsp_provisioner_observe(p);
  return leave(QC07F_BS_REFUSED);
 }
 if(b->phase==QC07F_BS_PROVISIONED)return leave(QC07F_BS_OK);
 if(b->phase!=QC07F_BS_PROVISIONING)
  return leave(hold(b,QC07F_BS_E_ORDER));
 int r=rtsp_provisioner_observe(p);
 if(r==RTSP_PP_OK&&p->phase==RTSP_PP_READY){
  b->phase=QC07F_BS_PROVISIONED;return leave(QC07F_BS_OK);
 }
 if(r==RTSP_PP_MORE&&p->phase==RTSP_PP_PENDING)return leave(QC07F_BS_MORE);
 return leave(hold(b,QC07F_BS_E_PROVISIONER));
}

int qc07f_bootstrap_start_v1(struct qc07f_bootstrap_v1 *b)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(b->phase!=QC07F_BS_PROVISIONED||b->provisioner.phase!=RTSP_PP_READY)
  return leave(hold(b,QC07F_BS_E_ORDER));
 b->phase=QC07F_BS_STARTING;
 b->bind_result=rtsp_provisioner_bind(&b->provisioner,b->config,&b->owner);
 if(b->bind_result!=RTSP_PP_OK||b->provisioner.phase!=RTSP_PP_BOUND)
  return leave(hold(b,QC07F_BS_E_BIND));
 /* Config NOW owns material_fd. Prepare's config changes are permitted only
  * after real bind has completed all unchanged-snapshot/known-wait checks. */
 b->prepare_result=qc07f_owned_runtime_prepare_v1(&b->runtime,b->config,
  &b->owner,b->bootstrap);
 if(b->prepare_result!=CAMERA_OK)return leave(hold(b,QC07F_BS_E_PREPARE));
 b->start_result=candidate_prestarted_start(&b->owner,b->config);
 if(b->start_result==CAMERA_OK&&b->owner.phase==CANDIDATE_PS_READY){
  b->phase=QC07F_BS_READY;return leave(QC07F_BS_OK);
 }
 latch(b,QC07F_BS_E_START);
 if(b->owner.self==&b->owner&&b->owner.config==b->config&&
    b->owner.phase==CANDIDATE_PS_START_FAILED){
  b->phase=QC07F_BS_START_FAILED;return leave(QC07F_BS_REFUSED);
 }
 /* Actual start already invokes retain_verifier on UNKNOWN. Never repeat stop,
  * fabricate retirement, close cfg's FD or discard stable owner on this path. */
 return leave(hold(b,QC07F_BS_E_START));
}

int qc07f_bootstrap_finish_device_free_v1(struct qc07f_bootstrap_v1 *b)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(b->finish_attempted||b->owner.self!=&b->owner||b->owner.config!=b->config||
    !((b->phase==QC07F_BS_READY&&b->owner.phase==CANDIDATE_PS_READY)||
      (b->phase==QC07F_BS_START_FAILED&&b->owner.phase==CANDIDATE_PS_START_FAILED)))
  return leave(hold(b,QC07F_BS_E_ORDER));
 /* This API exposes no activation or serve. Trusted no-earlier-native/media
  * anywhere + private handles are essential, NOT inferred from backend fields. */
 b->finish_attempted=1;b->phase=QC07F_BS_FINISHING;
 b->finish_result=candidate_prestarted_finish(&b->owner);
 if(b->finish_result!=CAMERA_OK||b->owner.phase!=CANDIDATE_PS_FINISHED)
  return leave(hold(b,QC07F_BS_E_FINISH));
 b->phase=QC07F_BS_FINISHED_DEVICE_FREE;
 return leave(QC07F_BS_OK);
}
