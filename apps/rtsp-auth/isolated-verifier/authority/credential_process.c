/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "credential_process.h"
#include "credential_ipc.h"
#include "credential_mbedtls.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

struct cp_record {char id[CAMERA_ID_BYTES];uint32_t domain,used,enabled;uint64_t generation,last_session;};
struct cp_issued {struct camera_access_principal principal;int record;unsigned active;};
struct cp_client {
 int fd,pidfd;pid_t pid;unsigned live,retired,failed,hardening,stopping;
 unsigned clock_failed,force_kill;
 uint64_t last_ns,next_sequence,last_request,next_attempt,stop_deadline;
 uint64_t policy_boot;char device_id[CAMERA_ID_BYTES];
 struct cp_options options;struct camera_access_policy *policy;
 struct cp_record records[CW_ACCOUNT_MAX];struct cp_issued issued[CAMERA_ACCESS_MAX_SESSIONS];
 struct cp_message pending;int pending_record;unsigned status,discarded;
};
static int domain_ok(uint32_t d){return d==CAMERA_ACCESS_UI||d==CAMERA_ACCESS_ADMIN;}
static int ticket_same(struct cp_client *c,const struct cp_ticket *t)
{
 return t&&c->pending.sequence&&t->child_generation==c->pending.epoch&&
  t->sequence==c->pending.sequence&&t->request_generation==c->pending.request_generation;
}
static int find(struct cp_client *c,uint32_t d,const char *id)
{
 unsigned i;for(i=0;i<CW_ACCOUNT_MAX;i++)if(c->records[i].used&&c->records[i].domain==d&&
  !memcmp(c->records[i].id,id,CAMERA_ID_BYTES))return (int)i;
 return -1;
}
static void revoke_issued(struct cp_client *c,int record)
{
 unsigned i;for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)if(c->issued[i].active&&(record<0||c->issued[i].record==record)){
  camera_access_revoke_session(c->policy,&c->issued[i].principal);c->issued[i].active=0;
 }
}
static void fail(struct cp_client *c,unsigned status)
{
 c->failed=1;revoke_issued(c,-1);
 if(c->status==CP_WAITING||c->status==CP_MATCHED)c->status=status;
 if(c->fd>=0){close(c->fd);c->fd=-1;}
 if(!c->stopping){c->stopping=1;
  if(c->clock_failed||c->last_ns>UINT64_MAX-c->options.retirement_grace_ns){
   c->clock_failed=1;c->force_kill=1;
  }else c->stop_deadline=c->last_ns+c->options.retirement_grace_ns;
 }
}
/* SOLE waitpid owner with default SIGCHLD for the fork -> pidfd_open acquisition
 * window: the unreaped owned PID cannot be recycled. Thereafter signals use only
 * the immutable kernel pidfd, never a numeric PID. No input supplies either. */
static int reap(struct cp_client *c)
{
 int status;pid_t p;if(c->retired)return CP_OK;
 p=waitpid(c->pid,&status,WNOHANG);
 if(p==c->pid){c->retired=1;c->live=0;c->pid=0;
  if(c->pidfd>=0){close(c->pidfd);c->pidfd=-1;}
  fail(c,CP_CHILD_LOST);return CP_OK;}
 if(p==0)return CP_PENDING;
 if(p<0&&errno==EINTR)return CP_PENDING;
 fail(c,CP_CHILD_LOST);return CP_UNCERTAIN;
}
static int signal_owned(struct cp_client *c)
{
 if(c->pidfd<0||c->retired)return CP_UNCERTAIN;
 if(syscall(SYS_pidfd_send_signal,c->pidfd,SIGKILL,NULL,0)&&errno!=ESRCH)return CP_UNCERTAIN;
 return CP_OK;
}
static int client_clock(struct cp_client *c,uint64_t *out)
{
 uint64_t now;
 if(c->clock_failed)return CP_LOST;
 if(cp_clock_read(&now)||now<c->last_ns){
  c->clock_failed=1;c->force_kill=1;fail(c,CP_CHILD_LOST);
  if(c->live&&!c->retired)signal_owned(c); /* permission revoked, exit not presumed */
  return CP_LOST;
 }
 c->last_ns=now;*out=now;return CP_OK;
}
static int observe(struct cp_client *c,uint64_t *now)
{
 int clock_ok=client_clock(c,now)==CP_OK;
 if(c->policy->boot_generation!=c->policy_boot||memcmp(c->policy->device_id,c->device_id,CAMERA_ID_BYTES)){
  fail(c,CP_CHILD_LOST);return CP_LOST;
 }
 if(reap(c)!=CP_PENDING||c->failed||!clock_ok)return CP_LOST;
 return CP_OK;
}
static int sequence(struct cp_client *c,struct cp_message *m)
{
 if(c->next_sequence==UINT64_MAX){fail(c,CP_CHILD_LOST);return CP_LOST;}
 m->sequence=c->next_sequence++;m->epoch=c->options.child_generation;return CP_OK;
}
static int binding(const struct cp_message *a,const struct cp_message *b)
{
 return a->epoch==b->epoch&&a->sequence==b->sequence&&a->request_generation==b->request_generation&&
  a->credential_generation==b->credential_generation&&a->deadline==b->deadline&&a->domain==b->domain;
}
static int result(struct cp_client *c,const struct cp_message *m)
{
 if(m->kind!=CP_RESULT||m->bytes||!binding(m,&c->pending)||
     (m->status!=CW_MATCH&&m->status!=CW_NO_MATCH)||c->status!=CP_WAITING){fail(c,CP_CHILD_LOST);return CP_LOST;}
 if(c->discarded){c->status=CP_DISCARDED;return CP_OK;}
 if(m->status==CW_MATCH&&c->pending_record>=0&&c->records[c->pending_record].enabled&&
    c->records[c->pending_record].generation==m->credential_generation)c->status=CP_MATCHED;
 else c->status=CP_REJECTED;
 return CP_OK;
}
static int kernel_timer(uint64_t span)
{
 struct itimerspec timer={0};int fd;
 if(!span||span>5000000000ULL)return -1;
 fd=timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);if(fd<0)return -1;
 timer.it_value.tv_sec=(time_t)(span/1000000000ULL);timer.it_value.tv_nsec=(long)(span%1000000000ULL);
 if(timerfd_settime(fd,0,&timer,NULL)){close(fd);return -1;}return fd;
}
static int timer_expired(int fd)
{
 struct pollfd timer={fd,POLLIN,0};int ret=poll(&timer,1,0);
 return ret<0||(ret>0&&timer.revents); /* failure expires, never unbounded retry */
}
static int rpc(struct cp_client *c,struct cp_message *request)
{
 uint64_t now,until;struct cp_message response={0};int ret=CP_LOST,timer=-1;unsigned attempts;
 if(observe(c,&now))goto done;
 timer=kernel_timer(c->options.control_timeout_ns);if(timer<0)goto broken;
 if(sequence(c,request)||cp_send(c->fd,request))goto broken;
 if(now>UINT64_MAX-c->options.control_timeout_ns){c->clock_failed=1;c->force_kill=1;goto broken;}
 until=now+c->options.control_timeout_ns;
 for(attempts=0;attempts<4096;attempts++){
  struct pollfd fd={c->fd,POLLIN,0};int read;
  if(observe(c,&now)||now>=until||timer_expired(timer))goto broken;
  read=cp_receive(c->fd,&response);
  if(read<0)goto broken;
  if(read==1){poll(&fd,1,2);continue;}
  if(response.kind==CP_RESULT){if(result(c,&response))goto broken;continue;}
  if(response.kind!=CP_ACK||response.bytes||response.status||!binding(request,&response))goto broken;
  ret=CP_OK;goto done;
 }
broken:fail(c,CP_CHILD_LOST);
done:if(timer>=0)close(timer);
 cp_wipe(request,sizeof(*request));cp_wipe(&response,sizeof(response));return ret;
}
static void launch(int socket_fd,int image_fd,char *const argv[])
{
 char *empty[]={NULL};
 /* dup image first: selected image FD may be 3, socket dup would replace it. */
 if(dup2(image_fd,4)<0||fcntl(4,F_SETFD,FD_CLOEXEC)<0||dup2(socket_fd,CP_FD)<0||
    fcntl(CP_FD,F_SETFD,0)<0||syscall(SYS_close_range,5U,~0U,0U))_exit(74);
 close(0);close(1);close(2);
 syscall(SYS_execveat,4,"",argv,empty,AT_EMPTY_PATH);_exit(75);
}
int cp_start(struct cp_client **out,const char *path,const struct cp_options *o,
 const struct cw_ui_material *dummy,struct camera_access_policy *policy)
{
 struct cp_client *c;struct stat st;struct sigaction action;struct rlimit core;
 int pair[2],image,copy,buffer=4096,ret=CP_LOST,timer=-1;unsigned attempts;
 struct cp_message m={0};struct ucred cred;socklen_t bytes=sizeof(cred);uint64_t now,until;
 char *argv[]={"credential-child",NULL,NULL};
 if(!out||!path||!o||!policy||!policy->boot_generation||!o->child_generation||!cw_material_valid(dummy)||
    o->global_spacing_ns<1000000000ULL||o->account_spacing_ns<1000000000ULL||
    o->global_spacing_ns<o->account_spacing_ns||!o->max_job_lifetime_ns||
    !o->control_timeout_ns||o->control_timeout_ns>5000000000ULL||
    o->retirement_grace_ns>1000000000ULL||geteuid()==0||getuid()!=geteuid()||getgid()!=getegid())return CP_INVALID;
 if(sigaction(SIGCHLD,NULL,&action)||action.sa_handler!=SIG_DFL||(action.sa_flags&SA_NOCLDWAIT))return CP_UNSUPPORTED;
 /* Do not silently alter the media/web parent's process policy. Owner must
  * establish these prerequisites; fixture changes only its own process. */
 if(getrlimit(RLIMIT_CORE,&core)||core.rlim_cur||prctl(PR_GET_DUMPABLE)!=0)return CP_HARDENING;
 if(cp_clock_read(&now)||now>UINT64_MAX-o->control_timeout_ns)return CP_LOST;
 if(camera_access_sweep(policy,now)!=CAMERA_ACCESS_OK)return CP_INVALID;
#ifdef CP_TEST_ONLY
 {static char *modes[]={NULL,"--test-real-unlocked","--test-delay-match","--test-hang","--test-crash","--test-forge","--test-strict-zero-lock"};
  if(o->fixture_mode>6)return CP_INVALID;argv[1]=modes[o->fixture_mode];}
#endif
 image=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
 if(image<0)return CP_INVALID;
 if(fstat(image,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||
    (st.st_uid!=geteuid()&&st.st_uid!=0)||(st.st_mode&0022)){close(image);return CP_INVALID;}
 if(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,0,pair)){close(image);return CP_LOST;}
 /* Stage both child sources >=5 BEFORE fork, even if caller stdio is closed.
  * Thus fixed destination FDs 3/4 cannot overwrite the other source. */
 copy=fcntl(image,F_DUPFD_CLOEXEC,5);
 if(copy<0){close(image);close(pair[0]);close(pair[1]);return CP_LOST;}
 close(image);image=copy;copy=fcntl(pair[1],F_DUPFD_CLOEXEC,5);
 if(copy<0){close(image);close(pair[0]);close(pair[1]);return CP_LOST;}
 close(pair[1]);pair[1]=copy;
 if(setsockopt(pair[0],SOL_SOCKET,SO_SNDBUF,&buffer,sizeof(buffer))||
    setsockopt(pair[1],SOL_SOCKET,SO_SNDBUF,&buffer,sizeof(buffer))||
    getsockopt(pair[0],SOL_SOCKET,SO_PEERCRED,&cred,&bytes)||cred.pid!=getpid()||cred.uid!=geteuid()||cred.gid!=getegid()){
  close(image);close(pair[0]);close(pair[1]);return CP_UNSUPPORTED;
 }
 c=calloc(1,sizeof(*c));if(!c){close(image);close(pair[0]);close(pair[1]);return CP_LOST;}
 c->options=*o;c->policy=policy;c->fd=pair[0];c->pidfd=-1;c->next_sequence=1;c->pending_record=-1;
 c->last_ns=now;
 c->policy_boot=policy->boot_generation;memcpy(c->device_id,policy->device_id,CAMERA_ID_BYTES);
 c->pid=fork();if(c->pid==0)launch(pair[1],image,argv);
 close(image);close(pair[1]);
 if(c->pid<0){close(pair[0]);free(c);return CP_LOST;}
 c->live=1;*out=c;
 c->pidfd=(int)syscall(SYS_pidfd_open,c->pid,0);
 if(c->pidfd<0){ret=CP_UNSUPPORTED;goto failure;}
 if(client_clock(c,&now))goto failure;
 if(now>UINT64_MAX-o->control_timeout_ns){c->clock_failed=1;c->force_kill=1;goto failure;}
 until=now+o->control_timeout_ns;
 timer=kernel_timer(o->control_timeout_ns);if(timer<0)goto failure;
 for(attempts=0;attempts<4096;attempts++){
  struct pollfd fd={c->fd,POLLIN,0};int received;
  if(observe(c,&now)||now>=until||timer_expired(timer))goto failure;
  received=cp_receive(c->fd,&m);if(received<0)goto failure;
  if(received==1){poll(&fd,1,2);continue;}
  if(m.kind!=CP_READY||m.bytes||m.epoch||m.sequence||m.domain||m.deadline||m.request_generation||m.credential_generation)goto failure;
  c->hardening=m.status;
#ifdef CP_TEST_ONLY
  if(o->fixture_mode&&o->fixture_mode!=6){if(m.status!=(CP_NODUMP|CP_FDS_CLOSED|CP_FIXTURE))goto failure;}
  else
#endif
  if(m.status!=(CP_NODUMP|CP_FDS_CLOSED|CP_LOCKED|CP_FILTERED)){ret=CP_HARDENING;goto failure;}
  break;
 }
 if(attempts==4096)goto failure;
 close(timer);timer=-1;
 cp_wipe(&m,sizeof(m));m.kind=CP_INIT;m.bytes=80;
 cp_u64(m.payload,o->global_spacing_ns);cp_u64(m.payload+8,o->account_spacing_ns);
 cp_u64(m.payload+16,o->max_job_lifetime_ns);cp_pack_material(m.payload+24,dummy);
 if(rpc(c,&m))goto failure;
 return CP_OK;
failure:if(timer>=0)close(timer);
 cp_wipe(&m,sizeof(m));fail(c,CP_CHILD_LOST);cp_retire(c);return ret;
}
static int change(struct cp_client *c,uint32_t domain,const char *id,uint64_t generation,const struct cw_ui_material *material)
{
 int index;unsigned i;uint64_t now;struct cp_message m={0};struct cp_record *r;
 if(!domain_ok(domain))return CP_UNSUPPORTED;
 if(!c||!cp_id_valid(id)||!generation||(material&&!cw_material_valid(material)))return CP_INVALID;
 if(observe(c,&now))return CP_LOST;
 index=find(c,domain,id);
 if(index<0){
  if(!material)return CP_STALE;
  for(i=0;i<CW_ACCOUNT_MAX;i++)if(!c->records[i].used)break;
  if(i==CW_ACCOUNT_MAX)return CP_BUSY;
  index=(int)i;
 }
 r=&c->records[index];if(r->used&&generation<=r->generation)return CP_STALE;
 revoke_issued(c,index);
 /* Invalidate before sending/waiting for control ACK. Never grant stale work. */
 if(c->pending.sequence&&c->pending_record==index&&!c->discarded){c->status=CP_REVOKED;
  fail(c,CP_REVOKED);return CP_STALE; /* active change retires child, no reactivation */}
 r->used=1;r->enabled=0;r->domain=domain;r->generation=generation;memcpy(r->id,id,CAMERA_ID_BYTES);
 m.kind=material?CP_PUT:CP_REVOKE;m.domain=domain;m.credential_generation=generation;
 m.bytes=material?120:64;memcpy(m.payload,id,CAMERA_ID_BYTES);
 if(material)cp_pack_material(m.payload+64,material);
 if(rpc(c,&m))return CP_LOST;
 r->enabled=material!=NULL;return CP_OK;
}
int cp_put_ui(struct cp_client *c,uint32_t domain,const char id[CAMERA_ID_BYTES],uint64_t gen,const struct cw_ui_material *m)
{
 if(!m)return CP_INVALID;
 return change(c,domain,id,gen,m);
}
int cp_revoke(struct cp_client *c,uint32_t domain,const char id[CAMERA_ID_BYTES],uint64_t gen)
{return change(c,domain,id,gen,NULL);}
int cp_submit(struct cp_client *c,uint32_t domain,const char id[CAMERA_ID_BYTES],uint64_t req,
 const unsigned char *password,size_t size,uint64_t deadline,struct cp_ticket *out)
{
 struct cp_message m={0};uint64_t now;int index;
 if(!domain_ok(domain))return CP_UNSUPPORTED;
 if(!c||!cp_id_valid(id)||!req||!password||!size||size>CW_PASSWORD_MAX||!out)return CP_INVALID;
 if(observe(c,&now))return CP_LOST;
 if(c->pending.sequence)return CP_BUSY;
 if(req<=c->last_request)return CP_STALE;
 if(now>UINT64_MAX-c->options.global_spacing_ns){c->clock_failed=1;c->force_kill=1;fail(c,CP_CHILD_LOST);return CP_LOST;}
 if(deadline<=now||deadline-now>c->options.max_job_lifetime_ns)return CP_INVALID;
 if(now<c->next_attempt)return CP_THROTTLED;
 m.kind=CP_VERIFY;m.domain=domain;m.request_generation=req;m.deadline=deadline;
 index=find(c,domain,id);m.credential_generation=index>=0&&c->records[index].enabled?c->records[index].generation:0;
 if(sequence(c,&m))return CP_LOST;
 memcpy(m.payload,id,CAMERA_ID_BYTES);cp_u32(m.payload+64,(uint32_t)size);memcpy(m.payload+68,password,size);m.bytes=68+(uint32_t)size;
 if(cp_send(c->fd,&m)){cp_wipe(&m,sizeof(m));fail(c,CP_CHILD_LOST);return CP_LOST;}
 c->pending=m;cp_wipe(c->pending.payload,sizeof(c->pending.payload));c->pending.bytes=0;
 c->pending_record=(index>=0&&c->records[index].enabled)?index:-1;c->status=CP_WAITING;c->discarded=0;
 c->last_request=req;c->next_attempt=now+c->options.global_spacing_ns;
 out->child_generation=m.epoch;out->sequence=m.sequence;out->request_generation=req;
 cp_wipe(&m,sizeof(m));return CP_OK;
}
int cp_cancel(struct cp_client *c,const struct cp_ticket *t)
{
 if(!c||!ticket_same(c,t))return CP_STALE;
 c->status=CP_CANCELLED;fail(c,CP_CANCELLED);return CP_OK;
}
int cp_discard(struct cp_client *c,const struct cp_ticket *t)
{
 uint64_t now;
 if(!c||!ticket_same(c,t))return CP_STALE;
 if(observe(c,&now))return CP_LOST;
 /* Preserve binding while CPU work is outstanding. A received success is also
  * irrevocably demoted; no supplied match fact is used. */
 c->discarded=1;
 if(c->status==CP_MATCHED||c->status==CP_REJECTED)c->status=CP_DISCARDED;
 return CP_OK;
}
static int principal_same(const struct camera_access_principal *a,
 const struct camera_access_principal *b)
{
 const struct camera_access_verified *x=&a->fact,*y=&b->fact;
 return a->policy_revision==b->policy_revision&&a->snapshot_serial==b->snapshot_serial&&
  x->abi==y->abi&&x->struct_bytes==y->struct_bytes&&x->verified==y->verified&&x->domain==y->domain&&
  !memcmp(x->device_id,y->device_id,CAMERA_ID_BYTES)&&!memcmp(x->account_id,y->account_id,CAMERA_ID_BYTES)&&
  x->boot_generation==y->boot_generation&&x->credential_generation==y->credential_generation&&
  x->session_generation==y->session_generation&&x->issued_ns==y->issued_ns&&
  x->expires_ns==y->expires_ns&&x->reserved==y->reserved;
}
int cp_release_access(struct cp_client *c,const struct camera_access_principal *principal)
{
 unsigned i;uint64_t now;int ret;
 if(!c||!principal)return CP_INVALID;
 if(observe(c,&now))return CP_LOST;
 for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)if(c->issued[i].active&&
    principal_same(principal,&c->issued[i].principal))break;
 if(i==CAMERA_ACCESS_MAX_SESSIONS)return CP_STALE;
 /* Pass the INTERNAL snapshot, never caller storage, to the authoritative
  * policy revoker. Its exact documented DENIED means no matching active snapshot
  * (e.g. already swept expiry), not ignored INVALID/clock failure/permission. */
 ret=camera_access_revoke_session(c->policy,&c->issued[i].principal);
 if(ret!=CAMERA_ACCESS_OK&&ret!=CAMERA_ACCESS_DENIED){fail(c,CP_CHILD_LOST);return CP_LOST;}
 cp_wipe(&c->issued[i],sizeof(c->issued[i]));return CP_OK;
}
int cp_step(struct cp_client *c,struct cp_receipt *out)
{
 uint64_t now;unsigned i;struct cp_message m={0};int ret=CP_OK;
 if(!c)return CP_INVALID;
 if(observe(c,&now))ret=CP_LOST;
 if(!c->failed&&c->pending.sequence&&now>=c->pending.deadline){c->status=CP_EXPIRED;fail(c,CP_EXPIRED);ret=CP_LOST;}
 if(!c->failed)for(i=0;i<4;i++){
  int received=cp_receive(c->fd,&m);
  if(received==1)break;
  if(received<0||result(c,&m)){fail(c,CP_CHILD_LOST);ret=CP_LOST;break;}
  cp_wipe(&m,sizeof(m));
 }
 cp_wipe(&m,sizeof(m));
 if(c->stopping&&!c->retired&&(c->force_kill||(!c->clock_failed&&c->last_ns>=c->stop_deadline))){
  int r=reap(c);
  if(r==CP_PENDING){if(signal_owned(c)!=CP_OK)ret=CP_UNCERTAIN;}
  else if(r==CP_UNCERTAIN)ret=r;
  reap(c);
 }
 if(out){out->ticket.child_generation=c->pending.epoch;out->ticket.sequence=c->pending.sequence;
  out->ticket.request_generation=c->pending.request_generation;out->status=c->status;}
 return ret;
}
int cp_issue_access(struct cp_client *c,const struct cp_ticket *t,uint64_t lifetime,struct camera_access_principal *out)
{
 struct camera_access_verified f={0};struct cp_record *r;uint64_t now;unsigned i;int ret=CP_STALE;
 if(!c||!out||!lifetime||!ticket_same(c,t))return CP_STALE;
 cp_step(c,NULL);if(c->failed)return CP_LOST;
 if(c->discarded)return CP_STALE; /* retain busy slot until bound RESULT drain */
 if(c->status==CP_WAITING)return CP_PENDING;
 if(c->status!=CP_MATCHED||c->pending_record<0)goto consume;
  r=&c->records[c->pending_record];if(client_clock(c,&now))return CP_LOST;
 if(!r->enabled||r->generation!=c->pending.credential_generation||now>=c->pending.deadline||
    now>UINT64_MAX-lifetime||r->last_session==UINT64_MAX)goto consume;
 for(i=0;i<CAMERA_ACCESS_MAX_SESSIONS;i++)if(!c->issued[i].active)break;
 if(i==CAMERA_ACCESS_MAX_SESSIONS){ret=CP_BUSY;goto consume;}
 f.abi=CAMERA_ACCESS_ABI;f.struct_bytes=sizeof(f);f.verified=1;f.domain=r->domain;
 memcpy(f.account_id,r->id,CAMERA_ID_BYTES);memcpy(f.device_id,c->device_id,CAMERA_ID_BYTES);
 f.boot_generation=c->policy_boot;f.credential_generation=r->generation;
 f.session_generation=++r->last_session;f.issued_ns=now;f.expires_ns=now+lifetime;
 if(camera_access_issue(c->policy,&f,now,out)==CAMERA_ACCESS_OK){
  c->issued[i].principal=*out;c->issued[i].record=c->pending_record;c->issued[i].active=1;ret=CP_OK;
 }
consume:cp_wipe(&c->pending,sizeof(c->pending));c->status=0;c->pending_record=-1;c->discarded=0;return ret;
}
int cp_forget(struct cp_client *c,const struct cp_ticket *t)
{
 if(!c||!ticket_same(c,t))return CP_STALE;
 if(c->status==CP_WAITING)return CP_PENDING;
 if(c->failed&&!c->retired)return CP_PENDING;
 cp_wipe(&c->pending,sizeof(c->pending));c->status=0;c->pending_record=-1;c->discarded=0;return CP_OK;
}
/* Relative kernel timerfd plus a finite poll-attempt budget, independent of
 * injected/failing/regressing primary clock. No absolute post-kill arithmetic.
 * EINTR consumes attempts, never restarts an unbounded timeout. A kernel/scheduler
 * failure still requires outer supervision; unknown exit returns UNCERTAIN. */
static int wait_owned(struct cp_client *c,uint64_t span,int grace)
{
 unsigned i;int fd,ret=CP_PENDING;
 if(!span)return reap(c);
 fd=kernel_timer(span);
 if(fd<0)return CP_UNCERTAIN;
 for(i=0;i<512;i++){
  struct pollfd fds[2]={{c->pidfd,POLLIN,0},{fd,POLLIN,0}};uint64_t now;int p;
  ret=reap(c);if(ret!=CP_PENDING)break;
  if(client_clock(c,&now)!=CP_OK&&grace)break;
  p=poll(fds,2,2);
  if(p<0&&errno!=EINTR){ret=CP_UNCERTAIN;break;}
  if((fds[0].revents|fds[1].revents)&(POLLERR|POLLNVAL)){ret=CP_UNCERTAIN;break;}
  if(fds[1].revents&POLLIN)break;
 }
 close(fd);return ret;
}
int cp_retire(struct cp_client *c)
{
 uint64_t now,remaining=0;int result;
 if(!c)return CP_INVALID;
 /* Shutdown is courtesy, not retirement. Closing IPC forces child join, which
  * supervisor bounds; SIGKILL then owned wait receipt establishes CPU exit. */
 if(!c->failed&&c->fd>=0){struct cp_message m={0};m.kind=CP_SHUTDOWN;
  if(!sequence(c,&m))cp_send(c->fd,&m);
  cp_wipe(&m,sizeof(m));}
 fail(c,CP_CHILD_LOST);
 if(client_clock(c,&now)==CP_OK&&!c->force_kill&&now<c->stop_deadline)remaining=c->stop_deadline-now;
 result=wait_owned(c,remaining,1);
 if(result==CP_OK)return CP_OK;
 if(signal_owned(c)!=CP_OK)return CP_UNCERTAIN;
 result=wait_owned(c,1000000000ULL,0);
 return result==CP_OK?CP_OK:CP_UNCERTAIN;
}
int cp_destroy(struct cp_client **p)
{
 struct cp_client *c;int result;if(!p||!*p)return CP_INVALID;c=*p;
 result=cp_retire(c);if(result!=CP_OK)return result;
 cp_wipe(c,sizeof(*c));free(c);*p=NULL;return CP_OK;
}
int cp_healthy(const struct cp_client *c){return c&&c->live&&!c->failed&&!c->retired;}
int cp_retirement_observed(const struct cp_client *c){return c&&c->retired;}
#ifdef CP_TEST_ONLY
int cp_test_inject(struct cp_client *c,const void *p,size_t n)
{return c&&c->fd>=0&&send(c->fd,p,n,MSG_DONTWAIT|MSG_NOSIGNAL)==(ssize_t)n?CP_OK:CP_LOST;}
int cp_test_hardening_flags(const struct cp_client *c){return c?(int)c->hardening:0;}
#endif
