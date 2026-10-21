/* SPDX-License-Identifier: GPL-2.0-only */
/* Seven-second PUBLIC root guard qualification only. No file/device/radio/auth IO. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define LIFETIME_NS UINT64_C(7000000000)
static volatile sig_atomic_t cancelled;
static void on_signal(int signal_number) { cancelled=signal_number; }
static int now_ns(uint64_t *out) {
 struct timespec t;
 if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L||
    (uint64_t)t.tv_sec>(UINT64_MAX-(uint64_t)t.tv_nsec)/UINT64_C(1000000000))return -1;
 *out=(uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;return 0;
}
static int harden(void) {
 struct rlimit core={0,0};
 struct __user_cap_header_struct header={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct caps[2]={{0}};
 if(setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||
    syscall(SYS_capset,&header,caps)||syscall(SYS_capget,&header,caps)||
    caps[0].effective||caps[0].permitted||caps[0].inheritable||
    caps[1].effective||caps[1].permitted||caps[1].inheritable||
    prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0))return -1;
 return 0;
}
int main(int argc,char **argv) {
 uint64_t start=0,last=0,now=0;int lifetime_reported=0;struct sigaction action={0};
 (void)argv;
 /* No arbitrary seconds/path/command/PID/arguments accepted, including HOST builds. */
 if(argc!=1){puts("GUARDQUAL_HELPER_ERROR version=1 class=arguments");return 21;}
 if(fcntl(3,F_GETFD)!=-1||errno!=EBADF){puts("GUARDQUAL_HELPER_ERROR version=1 class=FD3_not_closed");return 22;}
 if(harden()){puts("GUARDQUAL_HELPER_ERROR version=1 class=hardening");return 23;}
 action.sa_handler=on_signal;sigemptyset(&action.sa_mask);
 if(sigaction(SIGTERM,&action,NULL)||sigaction(SIGINT,&action,NULL)||now_ns(&start)){
  puts("GUARDQUAL_HELPER_ERROR version=1 class=clock_or_signal");return 24;
 }
 last=start;
 printf("GUARDQUAL_HELPER_READY version=1 lifetime_ns=%llu start_ns=%llu\n",
  (unsigned long long)LIFETIME_NS,(unsigned long long)start);fflush(stdout);
 for(;;){
  struct timespec pause={0,100000000};
  if(now_ns(&now)||now<last){puts("GUARDQUAL_HELPER_ERROR version=1 class=clock");return 25;}
  last=now;
  if(cancelled){
   printf("GUARDQUAL_HELPER_CANCEL version=1 signal=%d elapsed_ns=%llu\n",(int)cancelled,(unsigned long long)(now-start));
   printf("GUARDQUAL_HELPER_EXIT version=1 status=20 elapsed_ns=%llu\n",(unsigned long long)(now-start));fflush(stdout);return 20;
  }
  if(!lifetime_reported&&now-start>=UINT64_C(4000000000)){
   printf("GUARDQUAL_HELPER_LIFETIME version=1 elapsed_ns=%llu\n",(unsigned long long)(now-start));fflush(stdout);lifetime_reported=1;
  }
  if(now-start>=LIFETIME_NS){
   printf("GUARDQUAL_HELPER_EXIT version=1 status=0 elapsed_ns=%llu\n",(unsigned long long)(now-start));fflush(stdout);return 0;
  }
  while(nanosleep(&pause,&pause)&&errno==EINTR&&!cancelled){}
 }
}
