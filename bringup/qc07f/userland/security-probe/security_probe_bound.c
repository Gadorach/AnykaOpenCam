/* SPDX-License-Identifier: GPL-2.0-only
 * Parent-owned bounded qualification wrapper, not a public command service.
 * Only two reviewed public probes, no credentials/network/driver operation.
 * Kill only an owned, unreaped child. One-second/100-poll retirement grace;
 * no universal wall-time/liveness guarantee for uninterruptible kernel sleep.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static volatile sig_atomic_t cancelled;
static void cancel_probe(int signal_number){cancelled=signal_number;}
static int now(uint64_t *ns)
{
 struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||
  t.tv_nsec<0||t.tv_nsec>=1000000000L||
  (uint64_t)t.tv_sec>(UINT64_MAX-(uint64_t)t.tv_nsec)/1000000000ULL)return -1;
 *ns=(uint64_t)t.tv_sec*1000000000ULL+(uint64_t)t.tv_nsec;return 0;
}
struct child_state {
 int owned,reaped,status,kill_error,wait_error,retirement_error;
 unsigned kill_interrupts,wait_interrupts,retire_polls;
};
static void wait_result(struct child_state *s,pid_t child,pid_t r)
{
 if(r==child){
  if(WIFEXITED(s->status)||WIFSIGNALED(s->status)){s->owned=0;s->reaped=1;}
  else{s->wait_error=EPROTO;s->retirement_error=EPROTO;}
 }
 else if(r<0){
  if(errno==EINTR)s->wait_interrupts++;
  else{
   s->wait_error=errno;s->retirement_error=errno;
   if(errno==ECHILD)s->owned=0; /* Numeric PID no longer belongs to us. */
  }
 }
}
static void retire(pid_t child,struct child_state *s,int *clock_error,int *poll_error)
{
 if(!s->owned)return;
 for(unsigned attempt=0;attempt<8;attempt++){
  if(!kill(child,SIGKILL))break;
  if(errno==EINTR){s->kill_interrupts++;if(attempt<7)continue;}
  s->kill_error=errno;s->retirement_error=errno;break;
 }
 uint64_t start=0,current=0,last=0;int timed=!now(&start);
 if(!timed)*clock_error=1;else last=start;
 /* Never use blocking wait, including when SIGKILL fails. Count ceiling also
  * bounds retries if the clock fails/freezes or signals interrupt every poll. */
 for(unsigned i=0;i<100&&s->owned;i++){
  pid_t r=waitpid(child,&s->status,WNOHANG);wait_result(s,child,r);
  if(!s->owned)break;
  if(r<0&&errno!=EINTR)break;
  if(now(&current)|| (timed&&current<last)){*clock_error=1;timed=0;}
  else if(timed){last=current;if(current-start>=1000000000ULL)break;}
  s->retire_polls++;
  if(poll(NULL,0,10)<0&&errno!=EINTR){
   *poll_error=errno;s->retirement_error=errno;break;
  }
 }
 if(s->owned&&!s->retirement_error)s->retirement_error=ETIMEDOUT;
}
int main(int argc,char **argv)
{
 if(argc!=2)return 2;
 const char *path;unsigned seconds;
 if(!strcmp(argv[1],"entropy")){path="/run/plan-security-entropy01";seconds=5;}
 else if(!strcmp(argv[1],"kdf")){path="/run/plan-security-kdf01";seconds=35;}
 else return 2;
 uint64_t start=0,current=0,last=0;
 int result=1,expired=0,clock_error=0,final_clock_failure=0,poll_error=0;
 int setup_error=0,fork_error=0,started=0,elapsed_valid=0;
 struct child_state state={0};pid_t child=-1;
 if(now(&start)){clock_error=1;goto report;}
 started=1;last=start;
 struct sigaction action={0};action.sa_handler=cancel_probe;sigemptyset(&action.sa_mask);
 struct sigaction normal={0};normal.sa_handler=SIG_DFL;sigemptyset(&normal.sa_mask);
 if(sigaction(SIGCHLD,&normal,NULL)||sigaction(SIGINT,&action,NULL)||
  sigaction(SIGTERM,&action,NULL)||sigaction(SIGQUIT,&action,NULL)){
  setup_error=errno;goto report;
 }
 if(cancelled)goto report;
 child=fork();if(child<0){fork_error=errno;goto report;}
 if(!child){
  if(sigaction(SIGINT,&normal,NULL)||sigaction(SIGTERM,&normal,NULL)||
   sigaction(SIGQUIT,&normal,NULL))_exit(126);
  execl(path,path,(char *)NULL);_exit(127);
 }
 state.owned=1;
 for(;;){
   if(cancelled)break;
   pid_t r=waitpid(child,&state.status,WNOHANG);wait_result(&state,child,r);
   if(!state.owned||state.retirement_error)break;
   /* EINTR must not bypass the deadline/cancellation checks. */
   if(now(&current)||current<last){clock_error=1;break;}
   last=current;
   if(current-start>=(uint64_t)seconds*1000000000ULL){expired=1;break;}
   if(poll(NULL,0,10)<0&&errno!=EINTR){poll_error=errno;break;}
  }
 /* Reaping/ECHILD invalidated ownership, even if cancellation arrived while
  * waitpid was returning. Never kill/wait again using that unowned PID. */
 if(state.owned)retire(child,&state,&clock_error,&poll_error);
report:
 if(started){
  if(now(&current)||current<last){clock_error=1;final_clock_failure=1;}
  else elapsed_valid=1;
 }
 /* Conservatively reject completion observed past the deadline, even when
  * the child exited between polls and was already reaped before the sample. */
 if(elapsed_valid&&current-start>=(uint64_t)seconds*1000000000ULL)expired=1;
 /* Snapshot cancellation once for consistent result/report decision. Later
  * delivery cannot trigger retirement or alter an already-committed result. */
 int cancellation=(int)cancelled;
 int exited=state.reaped&&WIFEXITED(state.status);
 int exit_code=exited?WEXITSTATUS(state.status):-1;
 int child_signal=state.reaped&&WIFSIGNALED(state.status)?WTERMSIG(state.status):0;
 if(!setup_error&&!fork_error&&!clock_error&&!poll_error&&
  !state.retirement_error&&state.reaped){
  result=cancellation?128+cancellation:expired?124:exited?exit_code:1;
 }
 printf("SECURITY PROBE BOUND mode=%s deadline_s=%u expired=%d clock_error=%d "
  "final_clock_failure=%d elapsed_valid=%d cancelled_signal=%d elapsed_ns=%llu "
  "child_status=%d child_exited=%d child_exit=%d child_signal=%d child_reaped=%d "
  "child_owned=%d kill_error=%d wait_error=%d retirement_error=%d "
  "kill_interrupts=%u wait_interrupts=%u retire_polls=%u poll_error=%d "
  "setup_error=%d fork_error=%d wrapper_return=%d observation_complete=%d\n",
  argv[1],seconds,expired,clock_error,final_clock_failure,elapsed_valid,cancellation,
  (unsigned long long)(elapsed_valid?current-start:0),state.reaped?state.status:-1,
  exited,exit_code,child_signal,state.reaped,state.owned,state.kill_error,state.wait_error,
  state.retirement_error,state.kill_interrupts,state.wait_interrupts,state.retire_polls,
  poll_error,setup_error,fork_error,result,state.reaped&&!state.owned);
 return result;
}
