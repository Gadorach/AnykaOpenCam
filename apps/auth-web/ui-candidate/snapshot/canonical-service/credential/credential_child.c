/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "credential_ipc.h"
#include "credential_mbedtls.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

/* Default-deny syscall filter, no open/socket/connect/exec/ioctl/dup/filesystem
 * operations after startup. Same existing UID/groups: NOT a dedicated service
 * account. clone only with VM+SIGHAND+THREAD, clone3 ENOSYS fallback for pthread.
 * No privilege/capability acquisition and dumpability cannot be re-enabled. */
#define ALLOW(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SYS_##n,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
static int filter_install(void)
{
 const uint32_t own_tgid=(uint32_t)getpid();
 const uint32_t thread_required=CLONE_VM|CLONE_SIGHAND|CLONE_THREAD;
 const uint32_t thread_allowed=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|
  CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|CLONE_DETACHED;
#if defined(__x86_64__) && !defined(__ILP32__)
 const unsigned arch=AUDIT_ARCH_X86_64;
#elif defined(__arm__) && !defined(__ARMEB__)
 const unsigned arch=AUDIT_ARCH_ARM;
#else
#error "Only reviewed little-endian x86_64 and ARM EABI seccomp profiles are supported"
#endif
 struct sock_filter rules[]={
  BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,arch)),
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,arch,1,0),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
  BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
  ALLOW(read),ALLOW(write),ALLOW(close),ALLOW(recvmsg),ALLOW(sendto),
  ALLOW(poll),ALLOW(ppoll),ALLOW(futex),ALLOW(exit),ALLOW(exit_group),
  ALLOW(rt_sigaction),ALLOW(rt_sigprocmask),ALLOW(rt_sigreturn),ALLOW(sigaltstack),
  ALLOW(sched_yield),ALLOW(getpid),ALLOW(gettid),
  /* tgkill(tgid,tid,sig): high word zero, low word immutable OWN TGID.
   * Kernel verifies tid membership, so this cannot target another process.
   * Supported profiles are little-endian x86_64 and ARM EABI only. */
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SYS_tgkill,0,7),
  BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])+4),
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,1,0),
  BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
  BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,own_tgid,0,1),
  BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
  BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
  ALLOW(brk),ALLOW(munmap),ALLOW(mremap),ALLOW(mprotect),ALLOW(madvise),
  ALLOW(set_robust_list),ALLOW(nanosleep),ALLOW(restart_syscall),
#ifdef SYS_clock_gettime
  ALLOW(clock_gettime),
#endif
#ifdef SYS_clock_nanosleep
  ALLOW(clock_nanosleep),
#endif
#ifdef SYS_mmap
  ALLOW(mmap),
#endif
#ifdef SYS_mmap2
  ALLOW(mmap2),
#endif
#ifdef SYS_rseq
  ALLOW(rseq),
#endif
#ifdef SYS_clock_gettime64
  ALLOW(clock_gettime64),
#endif
#ifdef SYS_futex_time64
  ALLOW(futex_time64),
#endif
#ifdef SYS_ppoll_time64
  ALLOW(ppoll_time64),
#endif
#ifdef SYS_clock_nanosleep_time64
  ALLOW(clock_nanosleep_time64),
#endif
#ifdef SYS_clone3
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SYS_clone3,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|ENOSYS),
#endif
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SYS_clone,0,11),
  BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])+4),
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,1,0),
  BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
  BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
  BPF_STMT(BPF_ALU|BPF_AND|BPF_K,thread_required),
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,thread_required,1,0),
  BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
  BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
  BPF_STMT(BPF_ALU|BPF_AND|BPF_K,~thread_allowed),
  BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,1),
  BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
  BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)
 };
 struct sock_fprog program={(unsigned short)(sizeof(rules)/sizeof(rules[0])),rules};
 return prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program);
}
#undef ALLOW
static int broker_clock(void *context,uint64_t *now)
{(void)context;return cp_clock_read(now);}

#ifdef CP_TEST_ONLY
static unsigned mode;
static int mock(void *p,const struct cw_ui_material *m,const unsigned char *password,size_t n)
{
 struct timespec delay={0,250000000};(void)p;(void)m;(void)password;(void)n;
 if(mode==3){for(;;)pause();}
 nanosleep(&delay,NULL);return 1;
}
#endif
static int harden(unsigned *flags)
{
 struct rlimit core={0,0};pid_t parent=getppid();struct ucred cred;socklen_t n=sizeof(cred);int type;
 *flags=0;
 if(getuid()!=geteuid()||getgid()!=getegid()||!geteuid()||parent<=1)return -1;
 if(getsockopt(CP_FD,SOL_SOCKET,SO_PEERCRED,&cred,&n)||n!=sizeof(cred)||
    cred.pid!=parent||cred.uid!=geteuid()||cred.gid!=getegid())return -1;
 n=sizeof(type);if(getsockopt(CP_FD,SOL_SOCKET,SO_TYPE,&type,&n)||type!=SOCK_SEQPACKET)return -1;
 /* Kernel close_range BEFORE exec is enforced by launcher; this redundantly
  * closes all >=4 and stdio. Unsupported close_range fails before secrets. */
 if(syscall(SYS_close_range,4U,~0U,0U))return -1;
 close(0);close(1);close(2);*flags|=CP_FDS_CLOSED;
 if(setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0))return -1;
 {struct __user_cap_header_struct header={_LINUX_CAPABILITY_VERSION_3,0};
  struct __user_cap_data_struct capabilities[2]={{0}};unsigned i;
  if(syscall(SYS_capset,&header,capabilities)||syscall(SYS_capget,&header,capabilities))return -1;
  for(i=0;i<2;i++)if(capabilities[i].effective||capabilities[i].permitted||capabilities[i].inheritable)return -1;
 }
 if(prctl(PR_SET_PDEATHSIG,SIGKILL)||getppid()!=parent)return -1;
 *flags|=CP_NODUMP;
#ifdef CP_TEST_ONLY
 if(mode==6){struct rlimit zero={0,0};if(setrlimit(RLIMIT_MEMLOCK,&zero))return -1;
  /* Deterministic TEST-ONLY unsupported memory-hardening fault. No READY,
   * dummy, password or init admission even on a host with unusual mlock policy. */
  return -1;
 }
 if(mode&&mode!=6){*flags|=CP_FIXTURE;return 0;}
#endif
 if(mlockall(MCL_CURRENT|MCL_FUTURE))return -1;
 *flags|=CP_LOCKED;
 if(filter_install())return -1;
 /* Kernel checks of the default-deny boundary, before any secret IPC. These
  * cannot open a file/device, acquire a socket or change dumpability: seccomp
  * must reject before dispatch. Empty path is not an arbitrary file reference. */
 errno=0;if(syscall(SYS_openat,AT_FDCWD,"",O_RDONLY,0)!=-1||errno!=EPERM)return -1;
 errno=0;if(syscall(SYS_dup,CP_FD)!=-1||errno!=EPERM)return -1;
 errno=0;if(syscall(SYS_ioctl,CP_FD,0,0)!=-1||errno!=EPERM)return -1;
 errno=0;if(prctl(PR_SET_DUMPABLE,1)!=-1||errno!=EPERM)return -1;
 *flags|=CP_FILTERED;return 0;
}
static int send_response(const struct cp_message *request,unsigned kind,unsigned status)
{
 struct cp_message m=*request;int ret;m.kind=kind;m.status=status;m.bytes=0;
 cp_wipe(m.payload,sizeof(m.payload));ret=cp_send(CP_FD,&m);cp_wipe(&m,sizeof(m));return ret;
}
int main(int argc,char **argv)
{
 struct cw_broker *broker=NULL;struct cp_message message={0},active={0};
 struct cw_options options={0};struct cw_ui_material material={0};
 uint64_t epoch=0,sequence=0,job=0;unsigned flags=0;int ret,quit=0;
#ifdef CP_TEST_ONLY
 if(argc==2){
  if(!strcmp(argv[1],"--test-real-unlocked"))mode=1;
  else if(!strcmp(argv[1],"--test-delay-match"))mode=2;
  else if(!strcmp(argv[1],"--test-hang"))mode=3;
  else if(!strcmp(argv[1],"--test-crash"))mode=4;
  else if(!strcmp(argv[1],"--test-forge"))mode=5;
  else if(!strcmp(argv[1],"--test-strict-zero-lock"))mode=6;
  else return 70;
 }else if(argc!=1)return 70;
#else
 (void)argv;if(argc!=1)return 70;
#endif
 ret=harden(&flags);message.kind=CP_READY;message.status=ret?0:flags;
 if(cp_send(CP_FD,&message)||ret)return 71;
 for(;;){
  struct pollfd fd={CP_FD,POLLIN,0};
  if(poll(&fd,1,5)<0&&errno!=EINTR)break;
  ret=cp_receive(CP_FD,&message);if(ret<0)break;
  if(!ret){
   if(message.status||message.sequence!=sequence+1||!message.sequence||
      !message.epoch||(epoch&&message.epoch!=epoch))break;
   sequence=message.sequence;
   if(!broker){
    if(message.kind!=CP_INIT||message.bytes!=80||message.domain||message.request_generation||message.credential_generation)break;
    epoch=message.epoch;options.global_spacing_ns=cp_get64(message.payload);
    options.account_spacing_ns=cp_get64(message.payload+8);options.max_job_lifetime_ns=cp_get64(message.payload+16);
    options.clock=broker_clock; /* same checked timespec conversion, broker latch */
    cp_unpack_material(&material,message.payload+24);
    ret=cw_create(&broker,&options,&material);cp_wipe(&material,sizeof(material));
#ifdef CP_TEST_ONLY
    if(!ret&&(mode==2||mode==3||mode==5))ret=cw_test_set_verify(broker,mock,NULL);
#endif
   }else if(message.kind==CP_PUT||message.kind==CP_REVOKE){
    if(message.request_generation||message.bytes!=(message.kind==CP_PUT?120U:64U)||
       !cp_id_valid((const char *)message.payload))break;
    if(message.kind==CP_PUT){cp_unpack_material(&material,message.payload+64);
     ret=cw_put_ui(broker,message.domain,(const char *)message.payload,message.credential_generation,&material);
     cp_wipe(&material,sizeof(material));
    }else ret=cw_revoke(broker,message.domain,(const char *)message.payload,message.credential_generation);
   }else if(message.kind==CP_VERIFY){
    uint32_t bytes;
    if(job||message.bytes<69||!message.request_generation||!cp_id_valid((const char *)message.payload))break;
    bytes=cp_get32(message.payload+64);
    if(!bytes||bytes>CW_PASSWORD_MAX||message.bytes!=68+bytes)break;
#ifdef CP_TEST_ONLY
    if(mode==4)_exit(72);
#endif
    ret=cw_submit(broker,message.domain,(const char *)message.payload,message.payload+68,bytes,message.deadline,&job);
    active=message;cp_wipe(active.payload,sizeof(active.payload));active.bytes=0;
    cp_wipe(&message,sizeof(message));
    if(ret){if(send_response(&active,CP_RESULT,CW_NO_MATCH))break;job=0;}
    continue;
   }else if(message.kind==CP_SHUTDOWN){
    if(message.bytes||message.domain||message.request_generation||message.credential_generation)break;
    quit=1;ret=0;
   }else break;
   if(send_response(&message,CP_ACK,ret?1U:0U))break;
   cp_wipe(&message,sizeof(message));if(ret||quit)break;
  }
  if(job){
   struct cw_completion completion={0};
   if(cw_poll(broker,job,&completion))break;
   if(completion.status!=CW_QUEUED&&completion.status!=CW_RUNNING&&cw_forget(broker,job)==CW_OK){
    unsigned result=(completion.status==CW_MATCH&&completion.credential_generation==active.credential_generation&&
      completion.domain==active.domain)?CW_MATCH:CW_NO_MATCH;
#ifdef CP_TEST_ONLY
    if(mode==5)active.request_generation++;
#endif
    if(send_response(&active,CP_RESULT,result))break;
    cp_wipe(&active,sizeof(active));job=0;
   }
  }
 }
 cp_wipe(&message,sizeof(message));cp_wipe(&active,sizeof(active));cp_wipe(&material,sizeof(material));
 if(broker)cw_destroy(broker);
 close(CP_FD);return quit?0:73;
}
