/* SPDX-License-Identifier: GPL-2.0-only
 * Minimal owned-PIDFD/timeout/drain adaptation from pinned credential_process.c.
 * No UI KDF/material, source/producer/listener, arbitrary PID/path/command API. */
#define _GNU_SOURCE
#include "verifier.h"
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <linux/capability.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <elf.h>
#include <unistd.h>
#include <mbedtls/sha256.h>
#include <mbedtls/constant_time.h>
struct rv_client {
 int fd,pidfd;pid_t child;unsigned retired,failed,wiped,pending,discarded,revoking;
 unsigned barrier_released,barrier_armed,engine_ack;
 int owned_wait_status;unsigned owned_wait_observed;
 uint32_t ready_flags,profile;uint64_t epoch,generation,next_sequence,last_ns;
 struct rv_message active;
};
static int clock_owner(struct rv_client *c,uint64_t *now)
{if(rv_clock(now)||*now<c->last_ns){c->failed=1;return RV_LOST;}c->last_ns=*now;return RV_OK;}
static int reap(struct rv_client *c)
{
 int status;pid_t p;if(c->retired)return RV_OK;
 p=waitpid(c->child,&status,WNOHANG);
  if(p==c->child){c->owned_wait_status=status;c->owned_wait_observed=1;c->retired=1;c->failed=1;c->child=0;if(c->pidfd>=0){close(c->pidfd);c->pidfd=-1;}return RV_OK;}
 if(!p||(p<0&&errno==EINTR))return RV_MORE;
 c->failed=1;return RV_UNCERTAIN;
}
int rv_owned_signal(struct rv_client *c,int sig)
{
 if(!c||c->retired||c->pidfd<0||(sig!=SIGTERM&&sig!=SIGKILL))return RV_BAD;
 return syscall(SYS_pidfd_send_signal,c->pidfd,sig,NULL,0)&&errno!=ESRCH?RV_UNCERTAIN:RV_OK;
}
static int sequence(struct rv_client *c,struct rv_message *m)
{
 uint64_t now;if(c->failed||c->retired||reap(c)!=RV_MORE||clock_owner(c,&now)||c->next_sequence==UINT64_MAX)return RV_LOST;
 m->epoch=c->epoch;m->generation=c->generation;m->scope=1;m->sequence=c->next_sequence++;
 m->status=0;m->flags=0;if(!m->deadline)m->deadline=now+1000000000ULL;
 if(m->deadline<=now||m->deadline-now>5000000000ULL)return RV_BAD;
 return RV_OK;
}
static int bound(const struct rv_message *a,const struct rv_message *b)
{return a->epoch==b->epoch&&a->sequence==b->sequence&&a->connection==b->connection&&a->scope==b->scope&&a->generation==b->generation;}
static int receive_until(struct rv_client *c,struct rv_message *m,uint64_t until)
{
 for(unsigned i=0;i<4096;i++){
  uint64_t now;if(clock_owner(c,&now)||now>=until){c->failed=1;return RV_LOST;}
  int r=rv_receive(c->fd,m);
  if(!r){if(m->kind==RV_BYE){c->wiped=m->status==1;return RV_LOST;}return RV_OK;}
  if(r<0){c->failed=1;return RV_LOST;}
  if(reap(c)!=RV_MORE)return RV_LOST;
  struct pollfd fd={c->fd,POLLIN,0};if(poll(&fd,1,2)<0&&errno!=EINTR){c->failed=1;return RV_LOST;}
 }
 c->failed=1;return RV_LOST;
}
/* ONE existing owned seqpacket also serves as a pre-exec release barrier.
 * Child may report PDEATH armed, but cannot exec/harden/read FD4 until parent
 * actually owns pidfd and sends the fixed one-byte release. No readiness claim. */
#ifdef RV_TEST_PUBLIC_ONLY
static unsigned launch_fault;
static void (*launch_event)(int);
#endif
static int barrier_receive(int fd,unsigned char expected)
{
 uint64_t start,now;if(rv_clock(&start))return -1;
 for(unsigned attempt=0;attempt<4096;attempt++){
  unsigned char byte=0;union{struct cmsghdr align;unsigned char bytes[CMSG_SPACE(8*sizeof(int))];} control;
  struct iovec iov={&byte,1};struct msghdr msg={0};memset(&control,0,sizeof(control));
  msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control.bytes;msg.msg_controllen=sizeof(control);
  ssize_t n;
#ifdef RV_TEST_PUBLIC_ONLY
  if(launch_fault==RV_TEST_RELEASE_RECV_EINTR&&expected==1&&attempt==0){errno=EINTR;n=-1;}
  else
#endif
  n=recvmsg(fd,&msg,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
  if(n<0&&(errno==EINTR||errno==EAGAIN||errno==EWOULDBLOCK)){
   if(rv_clock(&now)||now<start||now-start>=2000000000ULL)return -1;
   struct pollfd p={fd,POLLIN,0};poll(&p,1,2);continue;
  }
  int rights=0;for(struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c)){
   rights=1;if(c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_RIGHTS&&c->cmsg_len>=CMSG_LEN(0)){
    size_t count=(c->cmsg_len-CMSG_LEN(0))/sizeof(int);int *fds=(int *)CMSG_DATA(c);for(size_t i=0;i<count;i++)close(fds[i]);
   }
  }
  return n==1&&!rights&&!(msg.msg_flags&(MSG_TRUNC|MSG_CTRUNC))&&byte==expected?0:-1;
 }
 return -1;
}
#ifdef RV_TEST_PUBLIC_ONLY
void rv_test_launch_fault(unsigned fault){launch_fault=fault;}
void rv_test_before_release(void (*event)(int)){launch_event=event;}
#endif
static void launch(int image,int socket_fd,int material,pid_t parent)
{
 struct rlimit core={0,0};struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};struct __user_cap_data_struct caps[2]={{0}};
 char *argv[]={"rtsp-isolated-verifier",NULL};char *environment[]={NULL};
 if(prctl(PR_SET_PDEATHSIG,SIGKILL)||getppid()!=parent)_exit(76);
#ifdef RV_TEST_PUBLIC_ONLY
 if(launch_fault==RV_TEST_CHILD_EXIT)_exit(77);
#endif
 unsigned char armed=2;
 if(send(socket_fd,&armed,1,MSG_DONTWAIT|MSG_NOSIGNAL)!=1)_exit(78);
 if(barrier_receive(socket_fd,1)||getppid()!=parent)_exit(79); /*EOF on parent barrier-close is actual exit, no secret read*/
 if(geteuid()==0){if(setgroups(0,NULL)||setresgid(1000,1000,1000)||setresuid(1000,1000,1000))_exit(70);}
 if(!getuid()||getuid()!=geteuid()||getgid()!=getegid()||setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||
    syscall(SYS_capset,&h,caps)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||prctl(PR_SET_PDEATHSIG,SIGKILL)||getppid()!=parent)_exit(71);
 if(dup2(image,5)<0||fcntl(5,F_SETFD,FD_CLOEXEC)||dup2(socket_fd,RV_FD)<0||fcntl(RV_FD,F_SETFD,0)||
    dup2(material,RV_MATERIAL_FD)<0||fcntl(RV_MATERIAL_FD,F_SETFD,0)||syscall(SYS_close_range,6U,~0U,0U))_exit(72);
 close(0);close(1);close(2);syscall(SYS_execveat,5,"",argv,environment,AT_EMPTY_PATH);_exit(73);
}
static int elf_valid(const struct rv_image *im)
{
 unsigned char ident[EI_NIDENT];
 if(pread(im->fd,ident,sizeof(ident),0)!=(ssize_t)sizeof(ident)||memcmp(ident,ELFMAG,SELFMAG)||ident[EI_DATA]!=ELFDATA2LSB||ident[EI_VERSION]!=EV_CURRENT)return -1;
 if(im->profile==RV_NATIVE){
  Elf32_Ehdr h;if(ident[EI_CLASS]!=ELFCLASS32||pread(im->fd,&h,sizeof(h),0)!=(ssize_t)sizeof(h)||h.e_machine!=EM_ARM||h.e_type!=ET_EXEC||
   h.e_version!=EV_CURRENT||h.e_ehsize!=sizeof(h)||(h.e_flags&EF_ARM_EABIMASK)!=EF_ARM_EABI_VER5||!(h.e_flags&EF_ARM_ABI_FLOAT_SOFT)||
   (h.e_flags&EF_ARM_ABI_FLOAT_HARD)||!h.e_phnum||h.e_phnum>32||h.e_phentsize!=sizeof(Elf32_Phdr)||
   (uint64_t)h.e_phoff+(uint64_t)h.e_phnum*sizeof(Elf32_Phdr)>im->bytes)return -1;
  for(unsigned i=0;i<h.e_phnum;i++){Elf32_Phdr p;
   if(pread(im->fd,&p,sizeof(p),(off_t)h.e_phoff+(off_t)i*sizeof(p))!=(ssize_t)sizeof(p)||p.p_type==PT_INTERP||p.p_type==PT_DYNAMIC||
      (p.p_type==PT_GNU_STACK&&(p.p_flags&PF_X))||(uint64_t)p.p_offset+p.p_filesz>im->bytes)return -1;
  }
 }else if(im->profile==RV_HOST_PUBLIC_4M){
  Elf64_Ehdr h;if(ident[EI_CLASS]!=ELFCLASS64||pread(im->fd,&h,sizeof(h),0)!=(ssize_t)sizeof(h)||h.e_machine!=EM_X86_64||
   (h.e_type!=ET_EXEC&&h.e_type!=ET_DYN)||h.e_version!=EV_CURRENT||h.e_ehsize!=sizeof(h)||!h.e_phnum||h.e_phnum>32||
   h.e_phentsize!=sizeof(Elf64_Phdr)||h.e_phoff+(uint64_t)h.e_phnum*sizeof(Elf64_Phdr)>im->bytes)return -1;
  for(unsigned i=0;i<h.e_phnum;i++){Elf64_Phdr p;
   if(pread(im->fd,&p,sizeof(p),(off_t)h.e_phoff+(off_t)i*sizeof(p))!=(ssize_t)sizeof(p)||
      (p.p_type==PT_GNU_STACK&&(p.p_flags&PF_X))||p.p_offset>im->bytes||p.p_filesz>im->bytes-p.p_offset)return -1;
  }
 }else return -1;
 return 0;
}
static int image_valid(const struct rv_image *im)
{
 struct stat st;unsigned char bytes[4096],digest[32];mbedtls_sha256_context hash;size_t used=0;int r=-1;
 if(!im||im->abi!=RV_ABI||im->fd<0||!im->bytes||im->bytes>2097152||fstat(im->fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||
     st.st_size!=(off_t)im->bytes||(st.st_mode&0222)||(st.st_mode&07000)||(fcntl(im->fd,F_GETFL)&O_ACCMODE)!=O_RDONLY)return -1;
 /* Native UID1000 cannot execute root0500: require root-owned public r-x subset
  * (0555/0505 are valid), not runtime chmod/chown or privileged exec workaround. */
 if(im->profile==RV_NATIVE){if(st.st_uid!=0||(st.st_mode&0005)!=0005)return -1;}
 else if(im->profile==RV_HOST_PUBLIC_4M){
  if((st.st_uid!=getuid()&&st.st_uid!=0)||((st.st_uid==getuid()?st.st_mode&0500:st.st_mode&0005)!=(st.st_uid==getuid()?0500:0005)))return -1;
 }else return -1;
 if(elf_valid(im))return -1;
 mbedtls_sha256_init(&hash);if(mbedtls_sha256_starts(&hash,0))goto done;
 while(used<im->bytes){size_t n=im->bytes-used;if(n>sizeof(bytes))n=sizeof(bytes);if(pread(im->fd,bytes,n,(off_t)used)!=(ssize_t)n||mbedtls_sha256_update(&hash,bytes,n))goto done;used+=n;}
 if(!mbedtls_sha256_finish(&hash,digest)&&!mbedtls_ct_memcmp(digest,im->sha256,32))r=0;
 done:mbedtls_sha256_free(&hash);rv_wipe(bytes,sizeof(bytes));rv_wipe(digest,sizeof(digest));return r;
}
int rv_start(struct rv_client **out,const struct rv_image *im,int material_fd)
{
 int pair[2],image=-1,material=-1,child_socket=-1;struct stat st;struct sigaction action;struct rlimit core;struct rv_message ready={0};uint64_t now;
 uint32_t expected_profile=im?im->profile:0;
 if(!out||image_valid(im)||material_fd<0||(expected_profile!=RV_NATIVE&&expected_profile!=RV_HOST_PUBLIC_4M)||
    getuid()!=geteuid()||getgid()!=getegid()||sigaction(SIGCHLD,NULL,&action)||action.sa_handler!=SIG_DFL||(action.sa_flags&SA_NOCLDWAIT)||
    getrlimit(RLIMIT_CORE,&core)||core.rlim_cur||prctl(PR_GET_DUMPABLE)!=0)return RV_BAD;
 if(fstat(material_fd,&st)||rv_clock(&now))return RV_BAD;
 image=fcntl(im->fd,F_DUPFD_CLOEXEC,16);material=fcntl(material_fd,F_DUPFD_CLOEXEC,16);
 if(image<0||material<0)goto bad;
 if(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,0,pair))goto bad;
 child_socket=fcntl(pair[1],F_DUPFD_CLOEXEC,16);close(pair[1]);if(child_socket<0){close(pair[0]);goto bad;}
 struct rv_client *c=calloc(1,sizeof(*c));if(!c){close(pair[0]);goto bad;}
 c->fd=pair[0];c->pidfd=-1;c->next_sequence=1;c->last_ns=now;
 pid_t owner=getpid();
 c->child=fork();if(!c->child){close(c->fd);launch(image,child_socket,material,owner);}
 close(image);close(material);close(child_socket);image=material=child_socket=-1;
 if(c->child<0){close(c->fd);free(c);return RV_LOST;}
 *out=c;
#ifdef RV_TEST_PUBLIC_ONLY
 if(launch_fault==RV_TEST_PIDFD_FAIL){errno=EPERM;c->pidfd=-1;}
 else
#endif
 c->pidfd=(int)syscall(SYS_pidfd_open,c->child,0);
 if(c->pidfd<0)goto failure;
 if(barrier_receive(c->fd,2)||reap(c)!=RV_MORE)goto failure;
 c->barrier_armed=1;
#ifdef RV_TEST_PUBLIC_ONLY
 if(launch_event)launch_event(c->pidfd); /*owned test can die here; still no exec/material/engine ACK*/
#endif
 {unsigned char go=1;int released=0;
  for(unsigned attempt=0;attempt<8;attempt++){
   ssize_t n;
#ifdef RV_TEST_PUBLIC_ONLY
   if(launch_fault==RV_TEST_RELEASE_FAIL){errno=EPIPE;n=-1;}
   else if(launch_fault==RV_TEST_RELEASE_ZERO)n=0;
   else if(launch_fault==RV_TEST_RELEASE_EINTR&&attempt==0){errno=EINTR;n=-1;}
   else
#endif
   n=send(c->fd,&go,1,MSG_DONTWAIT|MSG_NOSIGNAL);
   if(n==1){released=1;break;}
   if(n<0&&errno==EINTR)continue;
   break;
  }
  if(!released)goto failure;
  c->barrier_released=1;
 }
 if(receive_until(c,&ready,now+2000000000ULL)||ready.kind!=RV_READY||ready.bytes!=16||ready.status!=(int32_t)RV_FLAGS15||
    ready.epoch||ready.sequence||ready.connection||ready.scope||ready.generation||ready.deadline||ready.token||
    rv_get32(ready.payload)!=expected_profile||rv_get32(ready.payload+4)!=(expected_profile==RV_NATIVE?2U:4U)*1024U*1024U)goto failure;
 c->ready_flags=(uint32_t)ready.status;c->profile=expected_profile;
 /* Epoch/gen are trusted startup FD inputs; parent copies only these NONSECRET
  * fixed binding fields after child flags15. Never retains HA1/startup material. */
 struct rv_material prefix;
 if(pread(material_fd,&prefix,offsetof(struct rv_material,username),0)!=(ssize_t)offsetof(struct rv_material,username)||!prefix.epoch||!prefix.credential_generation)goto failure;
 c->epoch=prefix.epoch;c->generation=prefix.credential_generation;rv_wipe(&prefix,sizeof(prefix));rv_wipe(&ready,sizeof(ready));
 if(receive_until(c,&ready,now+2000000000ULL)||ready.kind!=RV_ACK||ready.status||ready.flags!=RV_FLAGS15||ready.bytes||ready.epoch!=c->epoch||
    ready.generation!=c->generation||ready.scope!=1||ready.sequence||ready.connection||ready.deadline||ready.token)goto failure;
 c->engine_ack=1;rv_wipe(&ready,sizeof(ready));return RV_OK;
 failure:rv_wipe(&ready,sizeof(ready));c->failed=1;
 if(!c->barrier_released&&c->fd>=0){close(c->fd);c->fd=-1;} /*release-barrier EOF BEFORE any exec/material*/
 return rv_stop(c)==RV_OK?RV_HARDENING:RV_UNCERTAIN;
 bad:if(image>=0)close(image);if(material>=0)close(material);if(child_socket>=0)close(child_socket);return RV_LOST;
}
int rv_rpc(struct rv_client *c,struct rv_message *m)
{
 struct rv_message reply={0};uint64_t now;int r;
 if(!c||!m||c->pending)return RV_BUSY;
 if(sequence(c,m)||clock_owner(c,&now)||rv_send(c->fd,m)){c->failed=1;return RV_LOST;}
 r=receive_until(c,&reply,m->deadline);
 if(r||reply.kind!=RV_ACK||reply.epoch!=m->epoch||reply.sequence!=m->sequence||reply.scope!=1||
    (m->kind!=RV_OPEN&&reply.connection!=m->connection)||
    reply.flags||(m->kind!=RV_CHALLENGE&&reply.bytes)||
    (m->kind!=RV_OPEN&&reply.token!=m->token)||
    (m->kind!=RV_CHECK&&reply.deadline!=m->deadline)||
    (m->kind!=RV_REVOKE&&reply.generation!=c->generation)||
    (m->kind==RV_REVOKE&&(!reply.status&&(c->generation==UINT64_MAX||reply.generation!=c->generation+1)))){c->failed=1;rv_wipe(&reply,sizeof(reply));return RV_LOST;}
 r=reply.status;if(!r&&m->kind==RV_REVOKE)c->generation=reply.generation;
 *m=reply;rv_wipe(&reply,sizeof(reply));return r;
}
int rv_submit(struct rv_client *c,uint64_t connection,const void *request,size_t bytes,uint64_t deadline)
{
 struct rv_message m={0};int r;if(!c||!request||!bytes||bytes>RV_PAYLOAD||!connection)return RV_BAD;if(c->pending)return RV_BUSY;
 m.kind=RV_VERIFY;m.connection=connection;m.deadline=deadline;m.bytes=(uint32_t)bytes;memcpy(m.payload,request,bytes);
 r=sequence(c,&m);if(!r)r=rv_send(c->fd,&m);
 if(!r){c->active=m;rv_wipe(c->active.payload,sizeof(c->active.payload));c->active.bytes=0;c->pending=1;c->discarded=0;}
 rv_wipe(&m,sizeof(m));return r;
}
int rv_discard(struct rv_client *c){if(!c||!c->pending)return RV_BAD;c->discarded=1;return RV_OK;}
int rv_owner_revoke(struct rv_client *c)
{
 struct rv_message m={0};if(!c||c->failed||c->retired)return RV_LOST;
 c->revoking=1; /*all adapter effect queries fail BEFORE any control wait*/
 if(c->pending){c->discarded=1;return RV_BUSY;}
 m.kind=RV_REVOKE;int r=rv_rpc(c,&m);rv_wipe(&m,sizeof(m));return r;
}
int rv_complete(struct rv_client *c,uint64_t *token)
{
 struct rv_message result={0},ack={0};uint64_t now;int ret=RV_LOST;
 if(!c||!token||!c->pending)return RV_BAD;
 *token=0;
 if(clock_owner(c,&now)||now>=c->active.deadline){c->failed=1;return RV_LOST;}
 int r=rv_receive(c->fd,&result);if(r==1)return RV_MORE;
 if(r||result.kind!=RV_RESULT||result.bytes||!bound(&result,&c->active)||result.deadline!=c->active.deadline){c->failed=1;goto done;}
 /* Actual result has arrived after child scratch wipe. Slot still not reusable
  * until RETIRE_RESULT ACK; discarded success is never interpreted as authority. */
 ack.kind=RV_RETIRE_RESULT;ack.connection=c->active.connection;ack.bytes=8;rv_put64(ack.payload,c->active.sequence);ack.token=c->discarded?1:0;
 if(sequence(c,&ack)){c->failed=1;goto done;}
 struct rv_message sent=ack;
 if(rv_send(c->fd,&ack)||receive_until(c,&ack,sent.deadline)||ack.kind!=RV_ACK||ack.status||ack.bytes||ack.flags||!bound(&ack,&sent)||ack.deadline!=sent.deadline||ack.token!=sent.token){c->failed=1;rv_wipe(&sent,sizeof(sent));goto done;}
 rv_wipe(&sent,sizeof(sent));
 ret=c->discarded?RV_DENIED:result.status;if(ret==0)*token=result.token;
 c->pending=0;c->discarded=0;rv_wipe(&c->active,sizeof(c->active));
 done:rv_wipe(&result,sizeof(result));rv_wipe(&ack,sizeof(ack));return ret;
}
int rv_query(struct rv_client *c,uint64_t connection,uint64_t token)
{
 struct rv_message m={0};uint64_t now;int r;if(!c||!connection||!token)return RV_BAD;
 if(c->failed||c->retired||c->revoking)return RV_DENIED;
 m.kind=RV_CHECK;m.connection=connection;m.token=token;r=rv_rpc(c,&m);
 if(!r&&(clock_owner(c,&now)||now>=m.deadline||reap(c)!=RV_MORE))r=RV_LOST;
 rv_wipe(&m,sizeof(m));return r;
}
int rv_stop(struct rv_client *c)
{
 struct rv_message m={0};uint64_t start;int killed=0;if(!c)return RV_BAD;if(c->retired)return RV_OK;
 if(rv_clock(&start))start=0;
 if(c->pidfd>=0)rv_owned_signal(c,SIGTERM); /*request alone is NOT retirement*/
 for(unsigned i=0;i<1500;i++){
  int r=c->fd>=0?rv_receive(c->fd,&m):-1;if(!r&&m.kind==RV_BYE)c->wiped=m.status==1;rv_wipe(&m,sizeof(m));
  r=reap(c);if(r==RV_OK){if(c->fd>=0){close(c->fd);c->fd=-1;}return RV_OK;}if(r==RV_UNCERTAIN)return r;
  uint64_t now=0;
  if(!killed&&(i>=250||rv_clock(&now)||!start||now<start||now-start>=500000000ULL)){
   if(c->pidfd<0)return RV_UNCERTAIN; /*barrier-close + owned wait only; NEVER numeric PID kill*/
   if(rv_owned_signal(c,SIGKILL)!=RV_OK)return RV_UNCERTAIN;
   killed=1;
  }
  struct pollfd fd={c->pidfd,POLLIN,0};poll(&fd,1,2);
 }
 return RV_UNCERTAIN;
}
int rv_destroy(struct rv_client **p)
{if(!p||!*p)return RV_BAD;int r=rv_stop(*p);if(r)return r;rv_wipe(*p,sizeof(**p));free(*p);*p=NULL;return RV_OK;}
int rv_retired(const struct rv_client *c){return c&&c->retired;}
uint32_t rv_ready_flags(const struct rv_client *c){return c?c->ready_flags:0;}
uint32_t rv_ready_profile(const struct rv_client *c){return c?c->profile:0;}
int rv_wipe_observed(const struct rv_client *c){return c&&c->wiped;}
/*Scoped server integration observer ONLY; original wire/child/adapter authority
 *unchanged. Zero cannot be inferred from a status code, timeout or signal.*/
int is_rv_pending_result(const struct rv_client *c){return c?(int)c->pending:-1;}
size_t is_rv_context_bytes(void){return sizeof(struct rv_client);}
int is_rv_wait0(const struct rv_client *c){return c&&c->owned_wait_observed&&WIFEXITED(c->owned_wait_status)&&WEXITSTATUS(c->owned_wait_status)==0;}
#ifdef RV_TEST_PUBLIC_ONLY
int rv_test_owned_pidfd(struct rv_client *c)
{return c&&!c->retired&&c->pidfd>=0?fcntl(c->pidfd,F_DUPFD_CLOEXEC,16):-1;}
unsigned rv_test_engine_ack(const struct rv_client *c){return c?c->engine_ack:0;}
unsigned rv_test_barrier_released(const struct rv_client *c){return c?c->barrier_released:0;}
#endif
