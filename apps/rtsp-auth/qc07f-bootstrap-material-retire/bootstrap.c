/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "bootstrap_v1.h"
#include "../rtsp-os-random/os_random.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

_Static_assert(__builtin_types_compatible_p(
 __typeof__(((struct candidate_config *)0)->random_ready),
 __typeof__(&qc07f_rtsp_os_random_ready_v1)),
 "owned-runtime random_ready callback type changed");
_Static_assert(sizeof(struct rv_material)==1536,"actual material ABI");

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
 b->prepare_result=qc07f_owned_runtime_prepare_v1(&b->runtime,b->config,
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

static int normal_finished(const struct qc07f_bootstrap_v1 *b)
{
 const struct candidate_prestarted *o=&b->owner;
 const struct candidate_backend *backend=&o->backend;
 if(b->phase!=QC07F_BS_FINISHED_DEVICE_FREE||b->error!=QC07F_BS_E_NONE||
    !b->material_snapshot_valid||!b->start_attempted||!b->finish_attempted||
    b->bind_result!=RTSP_PP_OK||b->prepare_result!=CAMERA_OK||
    b->start_result!=CAMERA_OK||b->finish_result!=CAMERA_OK||!bound_known(b)||
    o->self!=o||o->config!=b->config||o->phase!=CANDIDATE_PS_FINISHED||
    o->start_result!=CAMERA_OK||o->finish_result!=CAMERA_OK||o->serve_result||o->source_hold||
    backend->child||backend->config||backend->initialized||backend->failed||
    backend->pending||backend->pending_peer||backend->revoked||backend->revoke_pending||
    b->runtime.self!=&b->runtime||b->runtime.config!=b->config||
    b->runtime.owner!=o||b->config->context!=&b->runtime||b->runtime.bound||
    b->runtime.error||b->runtime.loan_present)return 0;
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
