/* SPDX-License-Identifier: GPL-2.0-only
 * Observe actual existing rv_adapter wait calls; no fabricated exit/READY.
 * Only sole owned child waits count, never arbitrary PID signal/selectors. */
#include <sys/types.h>
#include <sys/wait.h>
static unsigned exits,zero,signaled;
pid_t __real_waitpid(pid_t,int *,int);
pid_t __wrap_waitpid(pid_t pid,int *status,int options)
{
 pid_t observed=__real_waitpid(pid,status,options);
 if(pid>0&&observed==pid&&status){exits++;if(WIFEXITED(*status)&&WEXITSTATUS(*status)==0)zero++;if(WIFSIGNALED(*status))signaled++;}
 return observed;
}
unsigned nq_owned_exits(void){return exits;}
unsigned nq_owned_exit_zero(void){return zero;}
unsigned nq_owned_signaled(void){return signaled;}
