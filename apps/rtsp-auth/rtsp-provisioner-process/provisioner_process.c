/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "provisioner_process.h"
#include <camera/access_policy.h>
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

_Static_assert(sizeof(struct rv_material)==1536,"actual material ABI");
_Static_assert(sizeof(struct rtsp_pp_envelope)==1392,"same-fork envelope ABI");
_Static_assert(sizeof(dev_t)<=8&&sizeof(ino_t)<=8,"public inode representation");
static struct rtsp_provisioner_process *process_attempt;

static int hold(struct rtsp_provisioner_process *o,enum rtsp_pp_error error)
{
 if(o->error==RTSP_PP_E_NONE)o->error=error;
 o->phase=RTSP_PP_HOLD;
 return RTSP_PP_REFUSED;
}

static int clock_ms(uint64_t *out)
{
 struct timespec ts;
 if(clock_gettime(CLOCK_MONOTONIC,&ts)||ts.tv_sec<0||ts.tv_nsec<0||
    ts.tv_nsec>=1000000000L||(uint64_t)ts.tv_sec>(UINT64_MAX-1000)/1000)return -1;
 *out=(uint64_t)ts.tv_sec*1000+(uint64_t)ts.tv_nsec/1000000;
 return 0;
}

static int identity(void)
{
#ifdef RTSP_HANDOFF_HOST_PUBLICDUMMY
 return getuid()!=0&&getuid()==geteuid()&&getgid()==getegid();
#else
 uid_t r,e,s;gid_t gr,ge,gs;
 return !getresuid(&r,&e,&s)&&!getresgid(&gr,&ge,&gs)&&
        !r&&!e&&!s&&!gr&&!ge&&!gs&&getgroups(0,NULL)==0;
#endif
}

static int fresh(const struct candidate_prestarted *p)
{
 return p&&p->phase==CANDIDATE_PS_EMPTY&&!p->self&&!p->config&&
  !p->backend.child&&!p->backend.initialized&&!p->backend.config&&!p->source_hold;
}

static int config_class(const struct candidate_config *cfg)
{
 if(!cfg||cfg->abi!=CANDIDATE_AUTH_ABI||cfg->struct_bytes!=sizeof(*cfg)||
    cfg->material_fd!=-1||cfg->image.abi!=RV_ABI)return 0;
#ifdef RTSP_HANDOFF_HOST_PUBLICDUMMY
 return cfg->host_fixture_only==1&&cfg->image.profile==RV_HOST_PUBLIC_4M;
#else
 return !cfg->host_fixture_only&&cfg->image.profile==RV_NATIVE;
#endif
}

static int inode_same(const struct stat *a,const struct stat *b)
{return a->st_dev==b->st_dev&&a->st_ino==b->st_ino;}

static int directory_check(const struct rtsp_provisioner_process *o)
{
 struct stat st;int f=fcntl(o->directory_fd,F_GETFD);
 return f>=0&&(f&FD_CLOEXEC)&&!fstat(o->directory_fd,&st)&&S_ISDIR(st.st_mode)&&
  st.st_uid==getuid()&&(st.st_mode&07777)==0700&&st.st_nlink&&
  inode_same(&st,&o->directory_inode);
}

static int envelope(struct rtsp_pp_envelope *e,const struct candidate_config *cfg,
 const struct camera_rtsp_registry *r)
{
 struct camera_rtsp_registry checked;
 const struct camera_profile *p=NULL;
 memset(e,0,sizeof(*e));
 if(!config_class(cfg)||!cfg->graph||cfg->graph->profile_count>8||
    cfg->graph->device_count>16||cfg->graph->association_count>16||
    cfg->graph->group_count>8||camera_graph_validate(cfg->graph)||!r||
    !memchr(r->live_uri,0,sizeof(r->live_uri))||
    !memchr(r->track_uri,0,sizeof(r->track_uri))||
    camera_rtsp_registry_init(&checked,r->live_uri,r->track_uri))return -1;
 for(size_t i=0;i<cfg->graph->profile_count;i++)
  if(!memcmp(cfg->graph->profiles[i].id,cfg->profile_id,64))p=&cfg->graph->profiles[i];
 if(!p)return -1;
 e->version=RTSP_PP_VERSION;e->build_class=cfg->image.profile;
 e->material_abi=RV_ABI;e->material_bytes=1536;e->domain=CAMERA_ACCESS_RTSP;
 e->algorithm=cfg->algorithm;e->boot_generation=cfg->boot_generation;
 e->epoch=cfg->epoch;e->credential_generation=cfg->credential_generation;
#ifdef RTSP_HANDOFF_HOST_PUBLICDUMMY
 e->account=UINT64_C(0x505542444d59);
#else
 e->account=1;
#endif
 e->ttl=cfg->nonce_ttl_ms;e->window=cfg->rate_window_ms;
 e->global_verifies=cfg->global_verifies;e->global_challenges=cfg->global_challenges;
 e->account_verifies=cfg->account_verifies;e->account_challenges=cfg->account_challenges;
 e->peer_verifies=cfg->peer_verifies;e->peer_challenges=cfg->peer_challenges;
 memcpy(e->device,cfg->device_id,64);memcpy(e->account_id,cfg->account_id,64);
 memcpy(e->sensor,p->camera_id,64);memcpy(e->profile,cfg->profile_id,64);
 memcpy(e->live,checked.live_uri,sizeof(e->live));
 memcpy(e->track,checked.track_uri,sizeof(e->track));
 return 0;
}

static int unchanged(const struct rtsp_provisioner_process *o)
{
 struct rtsp_pp_envelope now;
 return o->config&&o->routes&&config_class(o->config)&&fresh(o->unused_owner)&&
  !memcmp(o->config,&o->config_snapshot,sizeof(*o->config))&&
  !memcmp(o->routes,&o->routes_snapshot,sizeof(*o->routes))&&
  !envelope(&now,o->config,o->routes)&&!memcmp(&now,&o->expected,sizeof(now));
}

static int material_check(const struct rtsp_provisioner_process *o)
{
 struct stat st,named;int fd=o->received[0];
 int access=fcntl(fd,F_GETFL),flags=fcntl(fd,F_GETFD);
 return identity()&&o->received_count==1&&fd>=0&&fd!=o->config->image.fd&&
  directory_check(o)&&access>=0&&(access&O_ACCMODE)==O_RDONLY&&
  flags>=0&&(flags&FD_CLOEXEC)&&!fstat(fd,&st)&&S_ISREG(st.st_mode)&&
  st.st_uid==getuid()&&(st.st_mode&07777)==0600&&st.st_nlink==1&&st.st_size==1536&&
  (uint64_t)st.st_dev==o->received_envelope.file_device&&
  (uint64_t)st.st_ino==o->received_envelope.file_inode&&lseek(fd,0,SEEK_CUR)==0&&
  !fstatat(o->directory_fd,RTSP_PRIVATE_HANDOFF_FILE,&named,AT_SYMLINK_NOFOLLOW)&&
  S_ISREG(named.st_mode)&&named.st_uid==getuid()&&(named.st_mode&07777)==0600&&
  named.st_nlink==1&&named.st_size==1536&&inode_same(&st,&named)&&directory_check(o);
}

static int close_once(int *slot)
{
 int fd=*slot;*slot=-1;
 return close(fd); /* EINTR/error is UNKNOWN, never a numerical retry. */
}

/* Only child-owned secret storage, never passed out or observed by parent. */
struct child_secret {
 unsigned char header[RTSP_PP_INGRESS_HEADER];
 unsigned char payload[2*CAMERA_DIGEST_NAME_MAX+CAMERA_DIGEST_PASSWORD_MAX];
 unsigned char extra;
 struct rv_material material;
};
_Static_assert(sizeof(struct child_secret)<4096,"bounded secret scratch");

static uint32_t le32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}

/* Nonblocking reads and finite shared syscall/poll budget. Nominal clock bound
 * is NOT a scheduling/syscall/independent recovery guarantee. */
static int read_part(int fd,unsigned char *p,size_t n,int eof,
 uint64_t deadline,uint64_t *last,unsigned *calls)
{
 size_t done=0;
 while((*calls)++<2048){
  uint64_t now;
  if(clock_ms(&now)||now<*last||now>=deadline)return -1;
  *last=now;
  ssize_t got=read(fd,p+done,n-done);
  if(eof&&got==0)return 0;
  if(eof&&got>0)return -1;
  if(got>0){done+=(size_t)got;if(done==n)return 0;continue;}
  if(got<0&&errno==EINTR)continue;
  if(got<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
   struct pollfd wait={fd,POLLIN,0};
   (void)poll(&wait,1,1);continue;
  }
  return -1;
 }
 return -1;
}

static int send_material(int socket_fd,int material,
 const struct rtsp_pp_envelope *e)
{
 union {struct cmsghdr align;unsigned char bytes[CMSG_SPACE(sizeof(int))];} control;
 memset(&control,0,sizeof(control));
 struct iovec iov={(void *)e,sizeof(*e)};
 struct msghdr m={.msg_iov=&iov,.msg_iovlen=1,
  .msg_control=control.bytes,.msg_controllen=sizeof(control.bytes)};
 struct cmsghdr *c=CMSG_FIRSTHDR(&m);
 c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));
 memcpy(CMSG_DATA(c),&material,sizeof(material));
 /* Exactly ONE send. Even EINTR/unknown delivery is failure, never resend. */
 return sendmsg(socket_fd,&m,MSG_DONTWAIT|MSG_NOSIGNAL)==(ssize_t)sizeof(*e)?0:-1;
}

static _Noreturn void provisioner_child(struct rtsp_provisioner_process *o)
{
 struct child_secret s={0};
 struct rtsp_private_handoff h=RTSP_PRIVATE_HANDOFF_INIT;
 struct rtsp_pp_envelope e;
 unsigned calls=0;uint64_t last=o->last_ms;
 int result=1;
 if(close_once(&o->sockets[0]))goto done;
#ifndef RTSP_HANDOFF_HOST_PUBLICDUMMY
 /* Actual root/saved IDs/groups/core0/nondump/NNP/mlock BEFORE any secret read. */
 if(qc07f_rtsp_private_provisioner_harden_v1()!=CAMERA_OK)goto done;
#endif
 if(read_part(o->ingress_fd,s.header,sizeof(s.header),0,o->deadline_ms,&last,&calls))goto done;
 uint32_t version=le32(s.header),u=le32(s.header+4),r=le32(s.header+8),p=le32(s.header+12);
 if(version!=RTSP_PP_VERSION||!u||u>CAMERA_DIGEST_NAME_MAX||!r||
    r>CAMERA_DIGEST_NAME_MAX||!p||p>CAMERA_DIGEST_PASSWORD_MAX)goto done;
 if(read_part(o->ingress_fd,s.payload,(size_t)u+r+p,0,o->deadline_ms,&last,&calls)||
    read_part(o->ingress_fd,&s.extra,1,1,o->deadline_ms,&last,&calls)||
    close_once(&o->ingress_fd))goto done;
 if(rtsp_private_handoff_create(&h,o->directory_fd,o->config,o->routes,
    (struct camera_digest_span){s.payload,u},
    (struct camera_digest_span){s.payload+u,r},
    (struct camera_digest_span){s.payload+u+r,p},&s.material)!=CAMERA_OK)goto done;
 if(rtsp_private_handoff_move(&h,o->config,o->unused_owner)!=CAMERA_OK)goto done;
 /* Move used the ACTUAL h and original-address child COW config; receiver does
  * not forge/reuse h. This slot now owns child sender's readonly reference. */
 e=o->expected;e.file_device=(uint64_t)h.file_inode.st_dev;
 e.file_inode=(uint64_t)h.file_inode.st_ino;
 if(send_material(o->sockets[1],o->config->material_fd,&e))goto done;
 if(close_once(&o->config->material_fd)||close_once(&o->sockets[1])||
    close_once(&o->directory_fd))goto done;
 result=0;
done:
 /* All owned input/material buffers wiped on EVERY path. Any unclosed alias
  * retires at _exit; nonzero exit NEVER establishes successful handoff cleanup.
  * No return into parent code, no secret logging/readback/hash or failure unlink. */
 mbedtls_platform_zeroize(&s,sizeof(s));
 _exit(result);
}

int rtsp_provisioner_begin(struct rtsp_provisioner_process *o,int *ingress_slot,
 int directory_fd,struct candidate_config *cfg,const struct camera_rtsp_registry *r,
 const struct candidate_prestarted *unused,uint32_t budget_ms)
{
 if(!o||process_attempt||o->self||o->phase!=RTSP_PP_EMPTY||o->ingress_fd!=-1||
    o->directory_fd!=-1||o->sockets[0]!=-1||o->sockets[1]!=-1||o->received_count)
  return RTSP_PP_REFUSED;
 for(unsigned i=0;i<RTSP_PP_RIGHTS_MAX;i++)if(o->received[i]!=-1)return RTSP_PP_REFUSED;
 process_attempt=o;o->self=o;o->phase=RTSP_PP_PENDING;
 o->config=cfg;o->routes=r;o->unused_owner=unused;o->directory_fd=directory_fd;
 /* Ownership is moved before any fallible preflight/fork; no caller numeric
  * copy is left as another canonical slot, even if fork is refused. */
 if(!ingress_slot||*ingress_slot<0)return hold(o,RTSP_PP_E_PREFLIGHT);
 o->ingress_fd=*ingress_slot;*ingress_slot=-1;
 struct stat st;struct sigaction action;
 int flags=fcntl(o->ingress_fd,F_GETFL),descriptor=fcntl(o->ingress_fd,F_GETFD);
 if(!identity()||!config_class(cfg)||!fresh(unused)||!budget_ms||budget_ms>10000||
    cfg->image.fd==o->ingress_fd||directory_fd==o->ingress_fd||
    fstat(o->ingress_fd,&st)||!S_ISFIFO(st.st_mode)||st.st_uid!=getuid()||
    flags<0||(flags&O_ACCMODE)!=O_RDONLY||!(flags&O_NONBLOCK)||
    descriptor<0||!(descriptor&FD_CLOEXEC)||
    sigaction(SIGCHLD,NULL,&action)||action.sa_handler!=SIG_DFL||
    (action.sa_flags&SA_NOCLDWAIT)||fstat(directory_fd,&o->directory_inode)||
    !directory_check(o)||envelope(&o->expected,cfg,r))return hold(o,RTSP_PP_E_PREFLIGHT);
 memcpy(&o->config_snapshot,cfg,sizeof(*cfg));
 memcpy(&o->routes_snapshot,r,sizeof(*r));
 if(clock_ms(&o->started_ms)||o->started_ms>UINT64_MAX-budget_ms)return hold(o,RTSP_PP_E_CLOCK);
 o->last_ms=o->started_ms;o->deadline_ms=o->started_ms+budget_ms;
 int pair[2];
 if(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,0,pair))
  return hold(o,RTSP_PP_E_SOCKET);
 o->sockets[0]=pair[0];o->sockets[1]=pair[1];
 o->child_pid=fork();
 if(o->child_pid<0)return hold(o,RTSP_PP_E_FORK);
 if(!o->child_pid)provisioner_child(o);
 o->child_unreaped=1;o->possible_file=1; /* even before any child result */
 if(close_once(&o->ingress_fd)){o->close_unknown|=1;hold(o,RTSP_PP_E_CLOSE_UNKNOWN);}
 if(close_once(&o->sockets[1])){o->close_unknown|=2;hold(o,RTSP_PP_E_CLOSE_UNKNOWN);}
 return o->phase==RTSP_PP_HOLD?RTSP_PP_REFUSED:RTSP_PP_MORE;
}

static int receive_material(struct rtsp_provisioner_process *o)
{
 union {struct cmsghdr align;
  unsigned char bytes[CMSG_SPACE(RTSP_PP_RIGHTS_MAX*sizeof(int))];} control;
 memset(&control,0,sizeof(control));
 struct iovec iov={&o->received_envelope,sizeof(o->received_envelope)};
 struct msghdr m={.msg_iov=&iov,.msg_iovlen=1,
  .msg_control=control.bytes,.msg_controllen=sizeof(control.bytes)};
 ssize_t n=recvmsg(o->sockets[0],&m,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
 if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))return RTSP_PP_MORE;
 if(n<0)return hold(o,RTSP_PP_E_RECEIVE);
 /* Store EVERY actually installed received right BEFORE any fallible check.
  * Bounded control can install at most RIGHTS_MAX ints. Kernel closes excess
  * undelivered/truncated rights; MSG_CTRUNC still forces permanent HOLD. */
 unsigned controls=0,bad=0;
 for(struct cmsghdr *c=CMSG_FIRSTHDR(&m);c;c=CMSG_NXTHDR(&m,c)){
  controls++;
  if(c->cmsg_len<CMSG_LEN(0)){bad=1;continue;}
  if(c->cmsg_level!=SOL_SOCKET||c->cmsg_type!=SCM_RIGHTS){bad=1;continue;}
  size_t bytes=c->cmsg_len-CMSG_LEN(0);
  if(bytes%sizeof(int))bad=1;
  for(size_t i=0;i<bytes/sizeof(int);i++){
   /* Kernel-generated ancillary layout fits the supplied control buffer. */
   int fd;memcpy(&fd,CMSG_DATA(c)+i*sizeof(int),sizeof(fd));
   o->received[o->received_count++]=fd;
  }
 }
 o->message_received=1;
 if(n!=(ssize_t)sizeof(o->received_envelope)||m.msg_flags&(MSG_TRUNC|MSG_CTRUNC)||
    bad||controls!=1||o->received_count!=1)return hold(o,RTSP_PP_E_RECEIVE);
 struct rtsp_pp_envelope wanted=o->expected;
 wanted.file_device=o->received_envelope.file_device;
 wanted.file_inode=o->received_envelope.file_inode;
 if(memcmp(&wanted,&o->received_envelope,sizeof(wanted)))return hold(o,RTSP_PP_E_ENVELOPE);
 if(!unchanged(o))return hold(o,RTSP_PP_E_CHANGED);
 if(!material_check(o))return hold(o,RTSP_PP_E_METADATA);
 return RTSP_PP_OK;
}

static void observe_child(struct rtsp_provisioner_process *o)
{
 if(!o->child_unreaped||o->wait_unknown)return;
 int status;pid_t result=waitpid(o->child_pid,&status,WNOHANG);
 if(result==0||(result<0&&errno==EINTR))return;
 if(result!=o->child_pid){o->wait_unknown=1;hold(o,RTSP_PP_E_WAIT_UNKNOWN);return;}
 o->child_unreaped=0;o->wait_observed=1;o->wait_status=status;
 o->wait0=WIFEXITED(status)&&WEXITSTATUS(status)==0;
 if(!o->wait0)hold(o,RTSP_PP_E_CHILD_FAILED);
}

int rtsp_provisioner_observe(struct rtsp_provisioner_process *o)
{
 if(!o||o!=process_attempt||o->self!=o)return RTSP_PP_REFUSED;
 if(o->phase==RTSP_PP_HOLD){observe_child(o);return RTSP_PP_REFUSED;}
 if(o->phase!=RTSP_PP_PENDING)return o->phase==RTSP_PP_READY?RTSP_PP_OK:RTSP_PP_REFUSED;
 uint64_t now;
 if(clock_ms(&now)||now<o->last_ms)hold(o,RTSP_PP_E_CLOCK);
 else {
  o->last_ms=now;
  if(now>=o->deadline_ms)hold(o,RTSP_PP_E_TIMEOUT);
 }
 if(o->phase!=RTSP_PP_HOLD&&!o->message_received)(void)receive_material(o);
 observe_child(o); /* even protocol/clock failure retains and observes own child */
 if(o->phase==RTSP_PP_HOLD)return RTSP_PP_REFUSED;
 if(!o->message_received||!o->wait0)return RTSP_PP_MORE;
 if(!unchanged(o))return hold(o,RTSP_PP_E_CHANGED);
 if(!material_check(o))return hold(o,RTSP_PP_E_METADATA);
 if(close_once(&o->sockets[0])){
  o->close_unknown|=4;return hold(o,RTSP_PP_E_CLOSE_UNKNOWN);
 }
 o->phase=RTSP_PP_READY;
 return RTSP_PP_OK;
}

int rtsp_provisioner_bind(struct rtsp_provisioner_process *o,
 struct candidate_config *cfg,const struct candidate_prestarted *unused)
{
 if(!o||o!=process_attempt||o->self!=o||o->phase!=RTSP_PP_READY)return RTSP_PP_REFUSED;
 if(cfg!=o->config||unused!=o->unused_owner||!unchanged(o)||!o->wait_observed||
    !o->wait0||o->child_unreaped||o->wait_unknown||o->close_unknown||
    o->ingress_fd!=-1||o->sockets[0]!=-1||o->sockets[1]!=-1)
  return hold(o,RTSP_PP_E_CHANGED);
 if(!material_check(o))return hold(o,RTSP_PP_E_METADATA);
 int fd=o->received[0];o->received[0]=-1;cfg->material_fd=fd;
 o->phase=RTSP_PP_BOUND;
 return RTSP_PP_OK;
}
