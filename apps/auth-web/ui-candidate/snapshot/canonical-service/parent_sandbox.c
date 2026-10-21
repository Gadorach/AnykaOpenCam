/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "web_service.h"
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <fcntl.h>
/* Installed only AFTER owned child READY/provisioning and listener creation.
 * Unlike the secret child, parent needs accept/TLS/socket I/O. No open, socket,
 * connect, exec, dup, ioctl, ptrace, clone or arbitrary signal syscall allowed.
 * pidfd_send_signal is needed by reviewed cp retirement; only owned FDs exist.
 * No claimed protection from same-UID malicious deployment/bootstrap. */
#define ALLOW(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,(n),0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
#define FCNTL(n) \
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,(n),0,13), \
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])+4), \
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,1,0), \
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM), \
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])), \
 ALLOW(F_GETFL),ALLOW(F_SETFL),ALLOW(F_GETFD),ALLOW(F_SETFD), \
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)
int web_parent_sandbox(void)
{
#if defined(__x86_64__)
#define WEB_ARCH AUDIT_ARCH_X86_64
#elif defined(__arm__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
#define WEB_ARCH AUDIT_ARCH_ARM
#else
 return -1;
#endif
#ifdef WEB_ARCH
 struct sock_filter f[]={
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,arch)),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,WEB_ARCH,1,0),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
  /* musl's __stdio_write issues writev (ARM EABI146), unlike the glibc host
   * path used by earlier fixtures. Same existing owned-FD write authority;
   * no new descriptor, device, file, process or signal operation is exposed. */
  ALLOW(SYS_read),ALLOW(SYS_write),ALLOW(SYS_writev),ALLOW(SYS_close),
 ALLOW(SYS_recvfrom),ALLOW(SYS_sendto),ALLOW(SYS_recvmsg),ALLOW(SYS_sendmsg),
 ALLOW(SYS_accept),ALLOW(SYS_accept4),ALLOW(SYS_getsockname),
 ALLOW(SYS_getsockopt),ALLOW(SYS_setsockopt),ALLOW(SYS_shutdown),
 FCNTL(SYS_fcntl),
#ifdef SYS_fcntl64
 FCNTL(SYS_fcntl64),
#endif
 ALLOW(SYS_poll),ALLOW(SYS_ppoll),
#ifdef SYS_select
 ALLOW(SYS_select),
#endif
#ifdef SYS_pselect6
 ALLOW(SYS_pselect6),
#endif
#ifdef SYS_pselect6_time64
 ALLOW(SYS_pselect6_time64),
#endif
#ifdef SYS_ppoll_time64
 ALLOW(SYS_ppoll_time64),
#endif
#ifdef SYS_epoll_wait
 ALLOW(SYS_epoll_wait),
#endif
 ALLOW(SYS_epoll_ctl),ALLOW(SYS_epoll_pwait),
 ALLOW(SYS_getrandom),
#ifdef SYS_clock_gettime
 ALLOW(SYS_clock_gettime),
#endif
#ifdef SYS_clock_gettime32
 ALLOW(SYS_clock_gettime32),
#endif
#ifdef SYS_clock_gettime64
 ALLOW(SYS_clock_gettime64),
#endif
 ALLOW(SYS_timerfd_create),
#ifdef SYS_timerfd_settime
 ALLOW(SYS_timerfd_settime),
#endif
#ifdef SYS_timerfd_settime32
 ALLOW(SYS_timerfd_settime32),
#endif
#ifdef SYS_timerfd_settime64
 ALLOW(SYS_timerfd_settime64),
#endif
 ALLOW(SYS_wait4),ALLOW(SYS_pidfd_send_signal),
 ALLOW(SYS_getrusage),
 ALLOW(SYS_brk),ALLOW(SYS_munmap),ALLOW(SYS_mprotect),
#ifdef SYS_mmap
 ALLOW(SYS_mmap),
#endif
#ifdef SYS_mmap2
 ALLOW(SYS_mmap2),
#endif
 ALLOW(SYS_madvise),ALLOW(SYS_munlockall),ALLOW(SYS_futex),
#ifdef SYS_futex_time64
 ALLOW(SYS_futex_time64),
#endif
  ALLOW(SYS_rt_sigreturn),ALLOW(SYS_rt_sigprocmask),
#if defined(__arm__) && defined(SYS_sigreturn)
  /* Ordinary musl ARM handlers use __restore/EABI119 even though handler
   * installation uses rt_sigaction. Permit returning from that frame, not
   * installing handlers or sending signals to another process. */
  ALLOW(SYS_sigreturn),
#endif
 ALLOW(SYS_exit),ALLOW(SYS_exit_group),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)};
 struct sock_fprog program={(unsigned short)(sizeof(f)/sizeof(f[0])),f};
 return prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||
        prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program);
#endif
}
