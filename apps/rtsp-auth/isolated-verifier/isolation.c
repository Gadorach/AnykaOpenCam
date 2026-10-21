/* SPDX-License-Identifier: GPL-2.0-only
 * Default-deny BPF copied from pinned canonical credential_child.c, with RTSP
 * getrandom and canonical ARM119 return addition; native budget remains2MiB. */
#define _GNU_SOURCE
#include "verifier.h"
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#define ALLOW(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SYS_##n,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
static int filter_install(void)
{
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
  ALLOW(read),ALLOW(write),ALLOW(close),ALLOW(recvmsg),ALLOW(sendto),ALLOW(getrandom),
  ALLOW(poll),ALLOW(ppoll),ALLOW(futex),ALLOW(exit),ALLOW(exit_group),
  ALLOW(rt_sigaction),ALLOW(rt_sigprocmask),ALLOW(rt_sigreturn),ALLOW(sigaltstack),
#if defined(__arm__) && defined(SYS_sigreturn)
   ALLOW(sigreturn), /*canonical musl ARM119 ordinary handler return repair*/
#endif
  ALLOW(sched_yield),ALLOW(getpid),ALLOW(gettid),
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
   /*No RTSP worker threads/process creation: narrower than UI broker subset.*/
   BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)
 };
 struct sock_fprog program={(unsigned short)(sizeof(rules)/sizeof(rules[0])),rules};
 return prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program);
}
#undef ALLOW
int rv_harden(unsigned *flags,unsigned *stage)
{
 struct rlimit core={0,0},mem;struct ucred peer;struct stat st;pid_t parent=getppid();
 socklen_t n;int type,seals,mode;struct __user_cap_header_struct header={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct caps[2]={{0}};
 *flags=0;*stage=1;
#ifdef RV_HOST_PUBLIC_ONLY
 mem=(struct rlimit){4U*1024U*1024U,4U*1024U*1024U};
#else
 mem=(struct rlimit){2U*1024U*1024U,2U*1024U*1024U};
#endif
 if(!getuid()||getuid()!=geteuid()||getgid()!=getegid()||parent<=1)return -1;
 *stage=2;n=sizeof(peer);
 if(getsockopt(RV_FD,SOL_SOCKET,SO_PEERCRED,&peer,&n)||n!=sizeof(peer)||peer.pid!=parent)return -1;
#ifdef RV_HOST_PUBLIC_ONLY
 if(peer.uid!=geteuid()||peer.gid!=getegid())return -1; /*NOT native root-creator proof*/
#else
 if(peer.uid!=0||getgroups(0,NULL)!=0)return -1;
#endif
 n=sizeof(type);if(getsockopt(RV_FD,SOL_SOCKET,SO_TYPE,&type,&n)||type!=SOCK_SEQPACKET)return -1;
 /* Trusted startup FD identity/length before hardening, but NEVER read material
  * until all flags15. Sealed memfd has immutable effective write protection;
  * native private regular FD may instead be readonly mode0600 root-owned. */
 *stage=3;
 if(fstat(RV_MATERIAL_FD,&st)||!S_ISREG(st.st_mode)||st.st_size!=(off_t)sizeof(struct rv_material)||
    st.st_uid!=peer.uid||(st.st_mode&0777)!=0600)return -1;
 mode=fcntl(RV_MATERIAL_FD,F_GETFL)&O_ACCMODE;seals=fcntl(RV_MATERIAL_FD,F_GET_SEALS);
 if(st.st_nlink==0){
  if(seals<0||(seals&(F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL))!=(F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL))return -1;
 }else if(st.st_nlink!=1||mode!=O_RDONLY)return -1;
 /*Readonly regular FD metadata cannot prove absence of other writers. Trusted
  * owner must freeze them; no credential hash/globally-immutable-file claim.*/
 if(lseek(RV_MATERIAL_FD,0,SEEK_CUR)!=0)return -1;
 *stage=4;if(syscall(SYS_close_range,5U,~0U,0U))return -1;
 close(0);close(1);close(2);*flags|=8U;
 *stage=5;
 if(setrlimit(RLIMIT_CORE,&core)||setrlimit(RLIMIT_MEMLOCK,&mem)||prctl(PR_SET_DUMPABLE,0)||
    prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||syscall(SYS_capset,&header,caps)||syscall(SYS_capget,&header,caps))return -1;
 for(unsigned i=0;i<2;i++)if(caps[i].effective||caps[i].permitted||caps[i].inheritable)return -1;
 if(prctl(PR_SET_PDEATHSIG,SIGKILL)||getppid()!=parent)return -1;
 *flags|=1U;
 *stage=6;if(mlockall(MCL_CURRENT|MCL_FUTURE))return -1;*flags|=2U;
 *stage=7;if(filter_install())return -1;
 errno=0;if(syscall(SYS_openat,AT_FDCWD,"",O_RDONLY,0)!=-1||errno!=EPERM)return -1;
 errno=0;if(syscall(SYS_dup,RV_FD)!=-1||errno!=EPERM)return -1;
 errno=0;if(syscall(SYS_ioctl,RV_FD,0,0)!=-1||errno!=EPERM)return -1;
 errno=0;if(prctl(PR_SET_DUMPABLE,1)!=-1||errno!=EPERM)return -1;
 *flags|=4U;*stage=8;return 0;
}
