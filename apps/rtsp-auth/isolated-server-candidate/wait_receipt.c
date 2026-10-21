/* SPDX-License-Identifier: GPL-2.0-only
 * Observe actual reviewed adapter waits for its OWN fork; no manufactured exit.*/
#include <sys/wait.h>
static unsigned exits,zero,signaled;
pid_t __real_waitpid(pid_t,int *,int);
pid_t __wrap_waitpid(pid_t pid,int *status,int options)
{pid_t p=__real_waitpid(pid,status,options);if(pid>0&&p==pid&&status){exits++;if(WIFEXITED(*status)&&WEXITSTATUS(*status)==0)zero++;if(WIFSIGNALED(*status))signaled++;}return p;}
unsigned is_owned_exits(void){return exits;}
unsigned is_owned_wait0(void){return zero;}
unsigned is_owned_signaled(void){return signaled;}
