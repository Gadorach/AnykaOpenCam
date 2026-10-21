/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual owned host filter, no secret/target input or foreign process signals. */
#define _GNU_SOURCE
#include "web_service.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
static volatile sig_atomic_t stopped;
static void stop_signal(int sig){(void)sig;stopped=1;}
int main(void)
{
 int output[2];assert(pipe(output)==0);
 pid_t pid=fork();int status;assert(pid>=0);
 if(!pid){struct timespec t;unsigned char byte;
  close(output[0]);if(dup2(output[1],1)<0)_exit(18);close(output[1]);
  struct sigaction action={0};action.sa_handler=stop_signal;
  sigemptyset(&action.sa_mask);if(sigaction(SIGTERM,&action,NULL))_exit(20);
  sigset_t blocked,empty;sigemptyset(&blocked);sigaddset(&blocked,SIGTERM);
  sigemptyset(&empty);if(sigprocmask(SIG_BLOCK,&blocked,NULL))_exit(23);
  pid_t own=getpid();
  if(web_parent_sandbox())_exit(10);
  if(clock_gettime(CLOCK_MONOTONIC,&t)||getrandom(&byte,1,GRND_NONBLOCK)!=1)_exit(11);
  errno=0;if(open("/WEB_FILTER_MUST_NOT_OPEN",O_RDONLY)!=-1||errno!=EPERM)_exit(12);
  errno=0;if(socket(AF_UNIX,SOCK_STREAM,0)!=-1||errno!=EPERM)_exit(13);
  errno=0;if(dup(1)!=-1||errno!=EPERM)_exit(14);
  errno=0;if(fcntl(1,F_DUPFD_CLOEXEC,50)!=-1||errno!=EPERM)_exit(15);
  if(fcntl(1,F_GETFD)<0)_exit(16);
   errno=0;if(ioctl(-1,0)!=-1||errno!=EPERM)_exit(17);
   /* musl stdio uses writev; glibc-only puts coverage missed this native ABI.
    * Only a preopened owned stdout pipe is used; no key/request material. */
   struct iovec vectors[2]={{(void *)"owned-",6},{(void *)"stdio\n",6}};
    if(syscall(SYS_writev,1,vectors,2)!=12)_exit(19);
    errno=0;if(syscall(SYS_kill,own,0)!=-1||errno!=EPERM)_exit(21);
    /* Return from an actual ordinary handler, then retire normally. The parent
     * owns this child; no filtered process gets signal-sending authority. */
    /* ppoll atomically unmasks SIGTERM: no lost-wakeup test race. */
    while(!stopped){if(ppoll(NULL,0,NULL,&empty)!=-1||errno!=EINTR)_exit(22);}
   _exit(0);
  }
 close(output[1]);char actual[12];size_t count=0;
 while(count<sizeof(actual)){ssize_t n=read(output[0],actual+count,sizeof(actual)-count);assert(n>0);count+=(size_t)n;}
 close(output[0]);assert(kill(pid,SIGTERM)==0);
   assert(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 assert(count==12&&!memcmp(actual,"owned-stdio\n",12));
  puts("parent_actual_seccomp=PASS open_socket_dup_fcntldup_ioctl_signal_send_denied=1 clock_rng_fdflags_allowed=1 ordinary_signal_return=1");return 0;
}
