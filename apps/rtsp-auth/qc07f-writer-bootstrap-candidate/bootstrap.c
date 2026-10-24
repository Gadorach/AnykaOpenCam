/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "writer_bootstrap_v1.h"
#include "../rtsp-os-random/os_random.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

_Static_assert(__builtin_types_compatible_p(
 __typeof__(((struct candidate_config *)0)->random_ready),
 __typeof__(&qc07f_rtsp_os_random_ready_v1)),
 "owned-runtime random_ready callback type changed");
_Static_assert(sizeof(struct rv_material)==1536,"actual material ABI");
_Static_assert(sizeof(struct qc07f_bootstrap_v1)<131072U,"bounded bootstrap metadata");

static struct qc07f_bootstrap_v1 *process_attempt;
static unsigned active;

static int identity(const struct qc07f_bootstrap_v1 *b)
{return b&&b==process_attempt&&b->self==b;}

static void latch(struct qc07f_bootstrap_v1 *b,enum qc07f_bootstrap_error error)
{if(b->error==QC07F_BS_E_NONE)b->error=error;}

static int hold(struct qc07f_bootstrap_v1 *b,enum qc07f_bootstrap_error error)
{
 latch(b,error);b->phase=QC07F_BS_HOLD;
 /* Actual runtime knows private moved/partial/native/paid ownership. Never infer
  * no ownership from a failed take. Final cleanup retains BEFORE unsafe unwind;
  * a normal return still requires every external object/remaining slot retained. */
 if(b->postfork_attempted)qc07f_owned_runtime_cleanup_v4(&b->runtime,0,1);
 return QC07F_BS_REFUSED;
}

static int leave(int result)
{active=0;return result;}

static int same_inode(const struct stat *a,const struct stat *b)
{return a->st_dev==b->st_dev&&a->st_ino==b->st_ino;}

/* Reads can change atime and the shared open-file offset, NOT these fields under
 * the mandatory no-writer/no-namespace-change contract. Never compare padding. */
static int same_metadata(const struct stat *a,const struct stat *b)
{
 return same_inode(a,b)&&a->st_mode==b->st_mode&&a->st_uid==b->st_uid&&
  a->st_gid==b->st_gid&&a->st_nlink==b->st_nlink&&a->st_size==b->st_size&&
  a->st_mtim.tv_sec==b->st_mtim.tv_sec&&a->st_mtim.tv_nsec==b->st_mtim.tv_nsec&&
  a->st_ctim.tv_sec==b->st_ctim.tv_sec&&a->st_ctim.tv_nsec==b->st_ctim.tv_nsec;
}

static int directory_stat(const struct stat *s)
{return S_ISDIR(s->st_mode)&&s->st_uid==0&&(s->st_mode&07777)==0700&&s->st_nlink;}

static int material_stat(const struct stat *s)
{return S_ISREG(s->st_mode)&&!s->st_uid&&!s->st_gid&&
 (s->st_mode&07777)==0600&&s->st_nlink==1&&s->st_size==1536;}

static int bound_known(const struct qc07f_bootstrap_v1 *b)
{
 const struct rtsp_provisioner_process *p=&b->provisioner;
 if(!b->config||p->self!=p||p->phase!=RTSP_PP_BOUND||p->error!=RTSP_PP_E_NONE||
    p->config!=b->config||p->routes!=b->routes||p->unused_owner!=&b->owner||
    p->directory_fd!=b->material_directory_fd||p->child_pid<=0||
    !p->message_received||p->received_count!=1||!p->wait_observed||!p->wait0||
    p->wait_status!=0||p->child_unreaped||p->wait_unknown||p->close_unknown||
    p->ingress_fd!=-1||p->sockets[0]!=-1||p->sockets[1]!=-1)return 0;
 for(unsigned i=0;i<RTSP_PP_RIGHTS_MAX;i++)if(p->received[i]!=-1)return 0;
 return 1;
}

static int directory_check(const struct qc07f_bootstrap_v1 *b,struct stat *s)
{
 int flags=fcntl(b->material_directory_fd,F_GETFD);
 return flags>=0&&(flags&FD_CLOEXEC)&&!fstat(b->material_directory_fd,s)&&
  directory_stat(s)&&same_inode(s,&b->provisioner.directory_inode);
}

static int named_check(const struct qc07f_bootstrap_v1 *b)
{
 struct stat directory,named;
 return directory_check(b,&directory)&&
  same_metadata(&directory,&b->material_directory_inode)&&
  !fstatat(b->material_directory_fd,RTSP_PRIVATE_HANDOFF_FILE,&named,AT_SYMLINK_NOFOLLOW)&&
  material_stat(&named)&&same_metadata(&named,&b->material_inode)&&
  directory_check(b,&directory)&&same_metadata(&directory,&b->material_directory_inode);
}

static int material_check(const struct qc07f_bootstrap_v1 *b)
{
 struct stat s;int fd=b->config->material_fd;
 int access=fcntl(fd,F_GETFL),flags=fcntl(fd,F_GETFD);
 return fd>=0&&fd==b->material_bound_fd&&fd!=b->config->image.fd&&
  access>=0&&(access&O_ACCMODE)==O_RDONLY&&flags>=0&&(flags&FD_CLOEXEC)&&
  !fstat(fd,&s)&&material_stat(&s)&&same_metadata(&s,&b->material_inode)&&named_check(b);
}

static int capture_material(struct qc07f_bootstrap_v1 *b)
{
 /* Actual bind moved this original FD; numbers here are observations, not aliases
  * or ownership fabrication. No secret/prefix read and no child h reconstruction. */
 b->material_bound_fd=b->config->material_fd;
 b->material_directory_fd=b->provisioner.directory_fd;
 if(!bound_known(b)||!directory_check(b,&b->material_directory_inode)||
    fstat(b->config->material_fd,&b->material_inode)||
    (uint64_t)b->material_inode.st_dev!=b->provisioner.received_envelope.file_device||
    (uint64_t)b->material_inode.st_ino!=b->provisioner.received_envelope.file_inode||
    !material_check(b))return 0;
 b->material_snapshot_valid=1;
 return 1;
}

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
 if(!capture_material(b))return leave(hold(b,QC07F_BS_E_MATERIAL_SNAPSHOT));
 /* Config NOW owns material_fd. Prepare's config changes are permitted only
  * after real bind has completed all unchanged-snapshot/known-wait checks. */
 b->prepare_result=qc07f_owned_runtime_prepare_v4(&b->runtime,b->config,
  &b->owner,b->bootstrap);
 if(b->prepare_result!=CAMERA_OK)return leave(hold(b,QC07F_BS_E_PREPARE));
 b->start_attempted=1;
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

static int runtime_identity(const struct qc07f_bootstrap_v1 *b)
{
 const struct qc07f_owned_runtime_v4 *r=&b->runtime;
 return r->self==r&&r->config==b->config&&r->owner==&b->owner&&
  r->graph==b->config->graph&&b->config->context==r&&r->bootstrap==b->bootstrap&&
  b->config->random_ready==qc07f_rtsp_os_random_ready_v1;
}

static int device_free(const struct qc07f_bootstrap_v1 *b)
{
 const struct qc07f_owned_runtime_v4 *r=&b->runtime;
 /* No-earlier-native ANYWHERE is still an essential trusted prerequisite. */
 return !b->postfork_attempted&&!b->setup_attempted&&!b->watchdog_attempted&&
  !b->storage_attempted&&!b->exact139_attempted&&!b->provider_attempted&&
  !b->memory_attempted&&!b->hub_attempted&&!b->serve_attempted&&
  !b->served_finish_attempted&&runtime_identity(b)&&!r->bound&&!r->error&&
  !r->retired&&!r->span_hub&&!r->span_setup_attempt&&!r->span_native_retire&&
  r->pipeline==QCW_IDLE&&!r->attempts&&!r->last_token&&!r->last_generation&&
  !r->span_relinquished&&!r->span_owner_retired&&!r->completed_frame_id;
}

static int ready_owner(const struct qc07f_bootstrap_v1 *b)
{
 const struct candidate_prestarted *o=&b->owner;
 const struct candidate_backend *p=&o->backend;
 if(b->error||!b->start_attempted||b->start_result||b->prepare_result||
    !b->material_snapshot_valid||b->bind_result||!bound_known(b)||!runtime_identity(b)||
    o->self!=o||o->config!=b->config||o->phase!=CANDIDATE_PS_READY||
    o->start_result||o->source_hold||p->config!=b->config||!p->initialized||
    !p->child||p->failed||p->pending||p->pending_peer||p->revoked||p->revoke_pending)
  return 0;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(p->peers[i]||o->peers[i].live)return 0;
 return 1;
}

int qc07f_bootstrap_bind_postfork_v1(struct qc07f_bootstrap_v1 *b)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(b->phase!=QC07F_BS_READY||!ready_owner(b)||!device_free(b)||b->finish_attempted)
  return leave(hold(b,QC07F_BS_E_ORDER));
 b->postfork_attempted=1;b->phase=QC07F_BS_POSTFORK_BINDING;
 b->postfork_result=qc07f_owned_runtime_bind_postfork_v4(&b->runtime);
 if(b->postfork_result||b->runtime.bound!=1||b->runtime.error)
  return leave(hold(b,QC07F_BS_E_POSTFORK));
 b->phase=QC07F_BS_POSTFORK_BOUND;return leave(QC07F_BS_OK);
}

int qc07f_bootstrap_setup_writer_v1(struct qc07f_bootstrap_v1 *b,
 int *watchdog_slot,dev_t watchdog_device,int *sensor_slot,dev_t sensor_device,
 int *frame_slot,const struct qc_frame_fd_identity *frame,
 int *encoder_slot,const struct qc_encoder_fd_identity *encoder,
 struct camera_span_hub_v1 *hub,uint64_t real_start_ns)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(b->phase!=QC07F_BS_POSTFORK_BOUND||!ready_owner(b)||b->setup_attempted||
    b->postfork_attempted!=1||b->postfork_result||b->runtime.bound!=1)
  return leave(hold(b,QC07F_BS_E_ORDER));
 if(!watchdog_slot||!sensor_slot||!frame_slot||!encoder_slot||!frame||!encoder||!hub||
    !watchdog_device||!sensor_device||!frame->device||!frame->source_instance||
    !frame->file_instance||!encoder->device||!encoder->session_identity||real_start_ns==UINT64_MAX||
    watchdog_slot==sensor_slot||watchdog_slot==frame_slot||watchdog_slot==encoder_slot||
    sensor_slot==frame_slot||sensor_slot==encoder_slot||frame_slot==encoder_slot)
  return leave(hold(b,QC07F_BS_E_SETUP_ARGUMENT));
 int *slots[4]={watchdog_slot,sensor_slot,frame_slot,encoder_slot};
 for(unsigned i=0;i<4;i++){
  if(*slots[i]<3||*slots[i]==b->config->image.fd||*slots[i]==b->config->material_fd||
     *slots[i]==b->material_directory_fd)return leave(hold(b,QC07F_BS_E_SETUP_ARGUMENT));
  for(unsigned j=0;j<i;j++)if(*slots[i]==*slots[j])return leave(hold(b,QC07F_BS_E_SETUP_ARGUMENT));
 }
 b->setup_attempted=1;b->phase=QC07F_BS_WRITER_SETUP;
 b->watchdog_attempted=1;
 b->watchdog_result=nh_native_watchdog_take(watchdog_slot,watchdog_device);
 if(b->watchdog_result)return leave(hold(b,QC07F_BS_E_WATCHDOG));
 b->storage_attempted=1;b->storage_result=nh_native_storage_select("separate");
 if(b->storage_result)return leave(hold(b,QC07F_BS_E_STORAGE));
 b->exact139_attempted=1;b->exact139_result=nh_native_optin_exact139();
 if(b->exact139_result)return leave(hold(b,QC07F_BS_E_EXACT139));
 b->provider_attempted=1;
 b->provider_result=nh_native_provider_take(sensor_slot,sensor_device,frame_slot,frame,encoder_slot,encoder);
 if(b->provider_result)return leave(hold(b,QC07F_BS_E_PROVIDER));
 b->memory_attempted=1;b->memory_result=nh_native_memory_start(real_start_ns);
 if(b->memory_result)return leave(hold(b,QC07F_BS_E_MEMORY));
 b->hub_attempted=1;
 b->hub_result=qc07f_owned_runtime_memory_span_hub_v4(&b->runtime,hub);
 if(b->hub_result)return leave(hold(b,QC07F_BS_E_HUB));
 b->phase=QC07F_BS_WRITER_READY;return leave(QC07F_BS_OK);
}

static int setup_known(const struct qc07f_bootstrap_v1 *b)
{
 return b->postfork_attempted==1&&!b->postfork_result&&b->setup_attempted==1&&
  b->watchdog_attempted==1&&!b->watchdog_result&&b->storage_attempted==1&&!b->storage_result&&
  b->exact139_attempted==1&&!b->exact139_result&&b->provider_attempted==1&&!b->provider_result&&
  b->memory_attempted==1&&!b->memory_result&&b->hub_attempted==1&&!b->hub_result;
}

static int writer_terminal(const struct qc07f_bootstrap_v1 *b)
{
 const struct qc07f_owned_runtime_v4 *r=&b->runtime;
 const struct camera_producer *p=&r->producer;
 if(!runtime_identity(b)||!setup_known(b)||!b->serve_attempted||b->serve_result||
    !b->serve_options||!b->listener_slot||*b->listener_slot!=-1||
    b->listener_outcome.consumed!=1||b->listener_outcome.close_attempted!=1||
    b->listener_outcome.close_errno||b->listener_outcome.result||
    r->bound!=1||r->retired!=1||r->error||r->span_native_retire!=2||
    r->span_setup_attempt!=1||r->span_relinquished!=1||r->span_hub||
    !r->span_owner_retired||r->pipeline!=QCW_IDLE||!r->attempts||
    r->attempts!=r->last_token||!r->last_generation||!r->completed_frame_id||
    r->completed_frame_id!=r->bridge.frame.id||
    r->completed_common_generation!=p->generation||r->gate||r->gate_context||
    r->bridge.producer!=p||r->bridge.context!=r||r->bridge.frame_open||r->bridge.clock_failed||
    !p->initialized||p->state!=CAMERA_PRODUCER_STOPPED||p->demand_count||p->active_present||
    p->needs_stop||p->queue_count||p->stop_pending||p->keyframe_pending)return 0;
 for(unsigned i=0;i<CAMERA_PRODUCER_CONSUMERS;i++)if(p->consumers[i].attached)return 0;
 /* Corroborates THIS actual serve ACK chain, not a native receipt. Actual runtime
  * final cleanup separately checks PRIVATE native/paid/pointer-free history. */
 return 1;
}

static int served_owner(const struct qc07f_bootstrap_v1 *b)
{
 const struct candidate_prestarted *o=&b->owner;const struct candidate_backend *p=&o->backend;
 if(o->self!=o||o->config!=b->config||o->phase!=CANDIDATE_PS_SERVED||
    o->start_result||o->serve_result||o->source_hold||p->config!=b->config||
    !p->initialized||!p->child||p->failed||p->pending||p->pending_peer||
    p->revoked||p->revoke_pending)return 0;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(p->peers[i]||o->peers[i].live)return 0;
 return 1;
}

int qc07f_bootstrap_serve_writer_v1(struct qc07f_bootstrap_v1 *b,
 const struct camera_rtsp_options *options,int *listener_slot)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(b->phase!=QC07F_BS_WRITER_READY||!ready_owner(b)||!setup_known(b)||
    b->serve_attempted||b->finish_attempted||!options||!listener_slot)
  return leave(hold(b,QC07F_BS_E_ORDER));
 b->serve_options=options;b->listener_slot=listener_slot;
 b->serve_attempted=1;b->phase=QC07F_BS_SERVING;
 b->serve_result=qc07f_owned_runtime_writer_serve_v5(&b->runtime,b->serve_options,
  b->listener_slot,&b->listener_outcome);
 if(b->serve_result||!served_owner(b)||!writer_terminal(b))
  return leave(hold(b,QC07F_BS_E_SERVE));
 qc07f_owned_runtime_cleanup_v4(&b->runtime,0,1);
 b->phase=QC07F_BS_SERVED;return leave(QC07F_BS_OK);
}

int qc07f_bootstrap_finish_device_free_v1(struct qc07f_bootstrap_v1 *b)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(!device_free(b)||b->finish_attempted||b->owner.self!=&b->owner||b->owner.config!=b->config||
    !((b->phase==QC07F_BS_READY&&b->owner.phase==CANDIDATE_PS_READY)||
      (b->phase==QC07F_BS_START_FAILED&&b->owner.phase==CANDIDATE_PS_START_FAILED)))
  return leave(hold(b,QC07F_BS_E_ORDER));
 /* No earlier native/media ANYWHERE remains a trusted condition. Selecting ANY
  * postfork bind/setup permanently excludes this STRICT device-free route. */
 b->finish_attempted=1;b->phase=QC07F_BS_FINISHING;
 b->finish_result=candidate_prestarted_finish(&b->owner);
 if(b->finish_result!=CAMERA_OK||b->owner.phase!=CANDIDATE_PS_FINISHED)
  return leave(hold(b,QC07F_BS_E_FINISH));
 b->phase=QC07F_BS_FINISHED_DEVICE_FREE;
 return leave(QC07F_BS_OK);
}

static int backend_finished(const struct candidate_prestarted *o)
{
 const struct candidate_backend *p=&o->backend;
 if(o->phase!=CANDIDATE_PS_FINISHED||o->finish_result||o->source_hold||
    p->child||p->config||p->initialized||p->failed||p->pending||p->pending_peer||
    p->revoked||p->revoke_pending)return 0;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(p->peers[i]||o->peers[i].live)return 0;
 return 1;
}

int qc07f_bootstrap_finish_served_v1(struct qc07f_bootstrap_v1 *b)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(b->phase!=QC07F_BS_SERVED||b->error||b->finish_attempted||
    !served_owner(b)||!writer_terminal(b))return leave(hold(b,QC07F_BS_E_ORDER));
 /* Actual private native ACK/common drain/paid terminal audit, not public flags.
  * No former live hub pointer is accessed. Verifier finish remains separate. */
 qc07f_owned_runtime_cleanup_v4(&b->runtime,0,1);
 b->finish_attempted=1;b->served_finish_attempted=1;b->phase=QC07F_BS_FINISHING;
 b->finish_result=candidate_prestarted_finish(&b->owner);
 if(b->finish_result!=CAMERA_OK||!backend_finished(&b->owner))
  return leave(hold(b,QC07F_BS_E_FINISH));
 b->phase=QC07F_BS_FINISHED_SERVED;return leave(QC07F_BS_OK);
}

static int normal_finished(const struct qc07f_bootstrap_v1 *b)
{
 const struct candidate_prestarted *o=&b->owner;
 const struct candidate_backend *backend=&o->backend;
 int route=(b->phase==QC07F_BS_FINISHED_DEVICE_FREE&&device_free(b))||
  (b->phase==QC07F_BS_FINISHED_SERVED&&b->served_finish_attempted==1&&writer_terminal(b));
 if(!route||b->error!=QC07F_BS_E_NONE||
    !b->material_snapshot_valid||!b->start_attempted||!b->finish_attempted||
    b->bind_result!=RTSP_PP_OK||b->prepare_result!=CAMERA_OK||
    b->start_result!=CAMERA_OK||b->finish_result!=CAMERA_OK||!bound_known(b)||
    o->self!=o||o->config!=b->config||o->phase!=CANDIDATE_PS_FINISHED||
    o->start_result!=CAMERA_OK||o->finish_result!=CAMERA_OK||o->serve_result||o->source_hold||
    backend->child||backend->config||backend->initialized||backend->failed||
    backend->pending||backend->pending_peer||backend->revoked||backend->revoke_pending||
    !runtime_identity(b)||!backend_finished(o))return 0;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(backend->peers[i])return 0;
 return 1;
}

int qc07f_bootstrap_retire_material_v1(struct qc07f_bootstrap_v1 *b)
{
 if(active)return QC07F_BS_BUSY;
 if(!identity(b))return QC07F_BS_REFUSED;
 active=1;
 if(b->material_retire_attempted)return leave(QC07F_BS_REFUSED);
 b->material_retire_attempted=1; /* explicit request consumed, even if refused */
 if(b->phase==QC07F_BS_HOLD)return leave(QC07F_BS_REFUSED);
 if(b->postfork_attempted)qc07f_owned_runtime_cleanup_v4(&b->runtime,0,1);
 if(!normal_finished(b))return leave(hold(b,QC07F_BS_E_ORDER));
 if(!material_check(b))return leave(hold(b,QC07F_BS_E_MATERIAL_PREFLIGHT));
 b->phase=QC07F_BS_MATERIAL_RETIRING;
 int fd=b->config->material_fd;
 b->config->material_fd=-1; /* canonical owned slot relinquished BEFORE close */
 b->material_close_attempted=1;b->material_close_last_not_owned=fd;
 b->material_close_result=close(fd); /* ONE attempt, including EINTR */
 if(b->material_close_result){
  b->material_close_errno=errno>0?errno:EIO;b->material_close_unknown=1;
  return leave(hold(b,QC07F_BS_E_MATERIAL_CLOSE));
 }
 /* No FD remains to compare: use captured actual inode + same fixed named entry.
  * Exclusive quiet caller namespace is essential, NOT hostile-root TOCTOU proof. */
 if(!named_check(b))return leave(hold(b,QC07F_BS_E_MATERIAL_NAMED));
 b->material_unlink_attempted=1;
 b->material_unlink_result=unlinkat(b->material_directory_fd,RTSP_PRIVATE_HANDOFF_FILE,0);
 if(b->material_unlink_result){
  b->material_unlink_errno=errno>0?errno:EIO;b->material_unlink_unknown=1;
  return leave(hold(b,QC07F_BS_E_MATERIAL_UNLINK));
 }
 /* Unlink legitimately changes directory metadata; no pre-unlink time/size
  * comparison now. Borrowed FD stays caller-owned/live, NEVER closed here. */
 b->material_directory_ack_attempted=1;
 b->material_directory_ack_result=fsync(b->material_directory_fd);
 if(b->material_directory_ack_result){
  b->material_directory_ack_errno=errno>0?errno:EIO;b->material_directory_ack_unknown=1;
  return leave(hold(b,QC07F_BS_E_MATERIAL_DIRECTORY_ACK));
 }
 b->phase=QC07F_BS_MATERIAL_RETIRED;
 return leave(QC07F_BS_OK);
}
