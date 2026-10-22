/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include "watchdog_owner.h"
#include <fcntl.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#define QW_PHASE_NS UINT64_C(2000000000)

void qw_owner_fail(struct qw_owner *o)
{if(o)o->state=QW_HOLD;}
static int fail(struct qw_owner *o)
{qw_owner_fail(o);return -1;}
static int clock_read(struct qw_owner *o,uint64_t *now)
{
 struct timespec t;
 if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L||
    (uint64_t)t.tv_sec>(UINT64_MAX-(uint64_t)t.tv_nsec)/UINT64_C(1000000000))return fail(o);
 *now=(uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
 if(*now==UINT64_MAX||*now<o->last_ns)return fail(o);
 o->last_ns=*now;return 0;
}
static int descriptor(struct qw_owner *o)
{
 struct stat st;
 if(o->fd<3)return fail(o);
 int flags=fcntl(o->fd,F_GETFL),fdflags=fcntl(o->fd,F_GETFD);
 if(flags<0||fdflags<0||fstat(o->fd,&st)||(flags&O_ACCMODE)!=O_WRONLY||
    !(fdflags&FD_CLOEXEC)||!S_ISCHR(st.st_mode)||st.st_rdev!=o->device||
    st.st_dev!=o->node_filesystem||st.st_ino!=o->node_inode)return fail(o);
 return 0;
}
int qw_owner_take(struct qw_owner *o,int *slot,dev_t device)
{
 if(!o||!slot||!device||o->fd!=-1||o->state!=QW_EMPTY||*slot<3)return -1;
 struct stat st;
 int flags=fcntl(*slot,F_GETFL),fdflags=fcntl(*slot,F_GETFD);
 if(flags<0||fdflags<0||fstat(*slot,&st)||(flags&O_ACCMODE)!=O_WRONLY||
    !(fdflags&FD_CLOEXEC)||!S_ISCHR(st.st_mode)||st.st_rdev!=device)return -1;
 *o=(struct qw_owner){.fd=*slot,.state=QW_UNKNOWN,.device=device,
  .node_filesystem=st.st_dev,.node_inode=st.st_ino};
 *slot=-1;return 0;
}
int qw_owner_seal_session(struct qw_owner *o,uint64_t start)
{
 if(!o||o->state==QW_HOLD)return -1;
 if(o->state!=QW_UNKNOWN||o->session_deadline_ns||
    start>=UINT64_MAX-QW_SESSION_NS)return fail(o);
 uint64_t now;
 if(clock_read(o,&now)||now<start||now-start>=QW_PHASE_NS)return fail(o);
 if(descriptor(o))return -1;
 if(clock_read(o,&now)||now<start||now-start>=QW_PHASE_NS)return fail(o);
 o->session_start_ns=start;o->session_deadline_ns=start+QW_SESSION_NS;
 return 0;
}
int qw_owner_check(struct qw_owner *o)
{
 if(!o||o->state==QW_HOLD)return -1;
 if(o->state!=QW_ENABLING&&o->state!=QW_ACTIVE5)return fail(o);
 uint64_t now;
 if(clock_read(o,&now)||now<o->start_ns||now-o->start_ns>=QW_PHASE_NS||
    now>=o->session_deadline_ns)return fail(o);
 if(descriptor(o))return -1;
 /* Metadata syscalls consume the SAME phase budget; re-read immediately before
  * a caller may service the watchdog, not just before descriptor inspection. */
 if(clock_read(o,&now)||now<o->start_ns||now-o->start_ns>=QW_PHASE_NS||
    now>=o->session_deadline_ns)return fail(o);
 return 0;
}
static int control(struct qw_owner *o,unsigned long command,int *value)
{
 if(qw_owner_check(o))return -1;
 if(ioctl(o->fd,command,value))return fail(o);
 return qw_owner_check(o);
}
int qw_owner_begin(struct qw_owner *o,uint64_t source_start,enum qw_phase phase,uint64_t session_deadline)
{
 if(!o||o->state==QW_HOLD)return -1;
 if((o->state!=QW_UNKNOWN&&o->state!=QW_DISABLED)||
    !o->session_deadline_ns||session_deadline!=o->session_deadline_ns||
    source_start==UINT64_MAX||source_start<o->session_start_ns||session_deadline<=source_start||
    (phase!=QW_NEXT&&phase!=QW_RETIRE))return fail(o);
 o->start_ns=source_start; /* original sealed session is never overwritten */
 o->phase=phase;o->capture_progress=0;o->state=QW_ENABLING;
 int timeout=5,option=WDIOS_ENABLECARD;
 /* SETTIMEOUT may itself ping/start inherited active hardware. Mark transition
  * unknown before the FIRST effect; do not service if phase/clock already bad. */
 if(control(o,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)return fail(o);
 if(control(o,WDIOC_SETOPTIONS,&option))return -1;
 timeout=0;
 if(control(o,WDIOC_GETTIMEOUT,&timeout)||timeout!=5)return fail(o);
 o->state=QW_ACTIVE5;return 0;
}
int qw_owner_capture_progress(struct qw_owner *o)
{
 if(!o||o->state==QW_HOLD)return -1;
 if(o->state!=QW_ACTIVE5||o->phase!=QW_NEXT||o->capture_progress)return fail(o);
 int ignored=0;
 if(control(o,WDIOC_KEEPALIVE,&ignored))return -1;
 o->capture_progress=1;return 0;
}
int qw_owner_disable_boundary(struct qw_owner *o)
{
 if(!o||o->state==QW_HOLD)return -1;
 if(o->state!=QW_ACTIVE5||(o->phase==QW_NEXT&&!o->capture_progress))return fail(o);
 int option=WDIOS_DISABLECARD;
 /* Failure/time loss after the syscall leaves UNKNOWN hardware state; do NOT
  * re-enable, ping, V-close or claim the retained FD guarantees recovery. */
 if(control(o,WDIOC_SETOPTIONS,&option))return -1;
 o->state=QW_DISABLED;return 0;
}
int qw_owner_idle_observe(struct qw_owner *o)
{
 if(!o||o->state==QW_HOLD)return -1;
 if(o->state!=QW_DISABLED)return fail(o);
 uint64_t now;
 if(clock_read(o,&now))return -1;
 if(descriptor(o))return -1;
 return clock_read(o,&now);
}
int qw_owner_give_back(struct qw_owner *o,int *slot)
{
 if(!o||!slot||*slot!=-1)return -1;
 if(qw_owner_idle_observe(o))return -1;
 *slot=o->fd;*o=(struct qw_owner)QW_OWNER_EMPTY;return 0;
}
int qw_owner_unknown_bootstrap_observe(struct qw_owner *o,uint64_t start)
{
 if(!o||o->state==QW_HOLD)return -1;
 if(o->state!=QW_UNKNOWN||o->fd<3||o->session_start_ns||o->session_deadline_ns||
    o->start_ns||o->phase||o->capture_progress||start==UINT64_MAX)return fail(o);
 uint64_t now;
 if(clock_read(o,&now)||now<start||now-start>=QW_PHASE_NS)return fail(o);
 if(descriptor(o))return -1;
 if(clock_read(o,&now)||now<start||now-start>=QW_PHASE_NS)return fail(o);
 return 0; /* UNKNOWN still; no guard/session initialization or service */
}
int qw_owner_return_unknown_bootstrap(struct qw_owner *o,int *slot,uint64_t start)
{
 if(!o||!slot||*slot!=-1)return -1;
 if(qw_owner_unknown_bootstrap_observe(o,start))return -1;
 *slot=o->fd;o->fd=-1;o->state=QW_RETURNED_UNKNOWN;
 return 0; /* recipient FD may be RUNNING with unknown timeout; never DISABLED */
}
