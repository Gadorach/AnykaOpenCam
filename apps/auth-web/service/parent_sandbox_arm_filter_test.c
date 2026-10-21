/* SPDX-License-Identifier: GPL-2.0-only */
/* Interpret the real parent filter under compiler-derived ARM syscall numbers.
 * Never install an ARM filter on the host or execute an ARM firmware binary. */
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
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct sock_filter captured[256];
static unsigned count, nnp;
static int capture_prctl(int request,...)
{
 va_list args;va_start(args,request);
 if(request==PR_SET_NO_NEW_PRIVS){assert(va_arg(args,int)==1);nnp=1;}
 else {
  assert(request==PR_SET_SECCOMP&&nnp);
  assert(va_arg(args,int)==SECCOMP_MODE_FILTER);
  const struct sock_fprog *program=va_arg(args,const struct sock_fprog *);
  assert(program->len<=sizeof(captured)/sizeof(captured[0]));count=program->len;
  memcpy(captured,program->filter,count*sizeof(captured[0]));
 }
 va_end(args);return 0;
}

/* All host headers have been included before the compiler-derived overlay.
 * The architecture switch affects only the source's explicit filter selector. */
#include "arm_syscall_overlay.h"
#undef __x86_64__
#define __arm__ 1
#define prctl capture_prctl
#include "parent_sandbox.c"
#undef prctl
#undef __arm__
#define __x86_64__ 1

static uint32_t evaluate(uint32_t arch,int syscall_number)
{
 struct seccomp_data data={.nr=syscall_number,.arch=arch};uint32_t value=0;
 for(unsigned pc=0;pc<count;pc++){
  const struct sock_filter *f=&captured[pc];
  switch(f->code){
  case BPF_LD|BPF_W|BPF_ABS:
   assert(f->k<=sizeof(data)-sizeof(value));
   memcpy(&value,(const unsigned char *)&data+f->k,sizeof(value));break;
  case BPF_JMP|BPF_JEQ|BPF_K:pc+=value==f->k?f->jt:f->jf;break;
  case BPF_RET|BPF_K:return f->k;
  default:assert(!"unreviewed filter instruction");
  }
 }
 assert(!"filter fell through");return 0;
}
int main(void)
{
 assert(SYS_sigreturn==119&&SYS_rt_sigreturn==173&&SYS_writev==146);
 assert(web_parent_sandbox()==0&&nnp&&count);
 const int allowed[]={SYS_read,SYS_write,SYS_writev,SYS_rt_sigreturn,
                      SYS_rt_sigprocmask,SYS_exit,SYS_exit_group};
 const int denied[]={SYS_open,SYS_socket,SYS_dup,SYS_ioctl,SYS_execve,
                     SYS_clone,SYS_ptrace,SYS_kill,SYS_tgkill,SYS_rt_sigaction};
 for(unsigned i=0;i<sizeof(allowed)/sizeof(allowed[0]);i++)
  assert(evaluate(AUDIT_ARCH_ARM,allowed[i])==SECCOMP_RET_ALLOW);
 for(unsigned i=0;i<sizeof(denied)/sizeof(denied[0]);i++)
  assert(evaluate(AUDIT_ARCH_ARM,denied[i])==(SECCOMP_RET_ERRNO|EPERM));
 assert(evaluate(AUDIT_ARCH_X86_64,SYS_sigreturn)==SECCOMP_RET_KILL_PROCESS);
 /* This desired ABI assertion fails on the pre-fix production filter. */
 assert(evaluate(AUDIT_ARCH_ARM,SYS_sigreturn)==SECCOMP_RET_ALLOW);
 puts("parent_ARM_filter=PASS legacy_and_RT_signalreturn_allowed=1 signal_sending_denied=1 wrong_arch_killed=1");
 return 0;
}
