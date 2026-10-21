/* SPDX-License-Identifier: GPL-2.0-only; HOST syscall mocks, no network. */
#include "tcp_sink.h"
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
enum fault {GOOD,SOCKET_FAIL,SET_FAIL,GET_FAIL,BIG_BUF,CONNECT_FAIL,CONNECT_INTR,CONNECT_PENDING,
 POLL_INTR,POLL_TIMEOUT,POLL_HUP,SO_ERROR_FAIL,SEND_INTR,SEND_AGAIN,SEND_ZERO,SEND_CLOSE,
 CANCEL,CLOCK_FAIL,CLOCK_BACK,UNIT_EXPIRE,SESSION_EXPIRE,CAP_FAIL,CLOSE_INTR};
static struct {enum fault f;unsigned sockets,connects,sends,polls,closes;uint64_t time;int cancel;} m;
static uint64_t now(void *v){(void)v;return m.time;}
static int cancelled(void *v){(void)v;return m.cancel;}
int __wrap_socket(int domain,int type,int proto)
{assert(domain==AF_INET&&(type&SOCK_NONBLOCK)&&(type&SOCK_CLOEXEC)&&!proto);m.sockets++;return m.f==SOCKET_FAIL?-1:9;}
int __wrap_setsockopt(int fd,int level,int key,const void *p,socklen_t n)
{assert(fd==9&&level==SOL_SOCKET&&n==sizeof(int));assert(*(const int *)p==(key==SO_SNDBUF?4096:1024));return m.f==SET_FAIL?-1:0;}
int __wrap_getsockopt(int fd,int level,int key,void *p,socklen_t *n)
{
 assert(fd==9&&level==SOL_SOCKET&&*n==sizeof(int));if(m.f==GET_FAIL)return -1;
 *(int *)p=key==SO_SNDBUF?(m.f==BIG_BUF?16384:8192):key==SO_RCVBUF?2304:m.f==SO_ERROR_FAIL?ECONNREFUSED:0;return 0;
}
int __wrap_connect(int fd,const struct sockaddr *p,socklen_t n)
{
 assert(fd==9&&n==sizeof(struct sockaddr_in)&&p->sa_family==AF_INET);m.connects++;
 if(m.f==CONNECT_FAIL){errno=ECONNREFUSED;return -1;}
 if(m.f==CONNECT_INTR){errno=EINTR;return -1;}
 if(m.f>=CONNECT_PENDING&&m.f<=SO_ERROR_FAIL){errno=EINPROGRESS;return -1;}
 return 0;
}
int __wrap_poll(struct pollfd *p,nfds_t n,int timeout)
{
 assert(n==1&&p->fd==9&&timeout>0&&timeout<=2000);m.polls++;
 if(m.f==POLL_INTR&&m.polls==1){m.time+=1000000;errno=EINTR;return -1;}
 if(m.f==POLL_TIMEOUT){m.time+=NH_OUTPUT_NS;return 0;}
 p->revents=m.f==POLL_HUP?POLLHUP:POLLOUT;return 1;
}
int __wrap___poll_chk(struct pollfd *p,nfds_t n,int timeout,size_t bytes)
{assert(bytes>=n*sizeof(*p));return __wrap_poll(p,n,timeout);}
ssize_t __wrap_send(int fd,const void *p,size_t n,int flags)
{
 assert(fd==9&&p&&n&&n<=8192&&(flags&MSG_NOSIGNAL));m.sends++;
 if(m.f==SEND_INTR&&m.sends==1){errno=EINTR;return -1;}
 if(m.f==SEND_AGAIN&&m.sends==1){errno=EAGAIN;return -1;}
 if(m.f==SEND_ZERO)return 0;
 if(m.f==SEND_CLOSE){errno=EPIPE;return -1;}
 return n>3?3:(ssize_t)n; /* partial accepted bytes */
}
int __wrap_close(int fd){assert(fd==9);m.closes++;if(m.f==CLOSE_INTR){errno=EINTR;return -1;}return 0;}
int __wrap_getpeername(int fd,struct sockaddr *p,socklen_t *n)
{assert(fd==9&&*n==sizeof(struct sockaddr_in));struct sockaddr_in a;assert(!nh_tcp_endpoint("127.0.0.1","17891",&a));memcpy(p,&a,sizeof(a));return 0;}
int main(void)
{
 unsigned cases=0;struct sockaddr_in peer;
 assert(!nh_tcp_endpoint("127.0.0.1","17891",&peer));
 assert(nh_tcp_endpoint("192.168.1.15","17891",&peer)&&nh_tcp_endpoint("user@127.0.0.1","17891",&peer)&&nh_tcp_endpoint("127.0.0.1","0",&peer));
 assert(!nh_tcp_endpoint("127.0.0.1","17891",&peer));
 for(enum fault f=GOOD;f<=CLOSE_INTR;f++){
  memset(&m,0,sizeof(m));m.f=f;m.time=1000000000;struct nh_tcp_sink s;
  int r=nh_tcp_connect(&s,&peer,m.time,now,cancelled,NULL);
  if(f==SOCKET_FAIL||f==SET_FAIL||f==GET_FAIL||f==BIG_BUF||f==CONNECT_FAIL||f==POLL_TIMEOUT||f==POLL_HUP||f==SO_ERROR_FAIL){
   assert(r&&m.connects<=1&&m.closes<2);cases++;continue;
  }
  if(r||m.connects!=1||s.actual_send!=8192||s.actual_receive!=2304)fprintf(stderr,"TCP MOCK unexpected f=%u result=%d sockets=%u connects=%u polls=%u sendbuf=%d rcvbuf=%d closes=%u errno=%d\n",(unsigned)f,r,m.sockets,m.connects,m.polls,s.actual_send,s.actual_receive,m.closes,errno);
  if(r)fprintf(stderr,"TCP MOCK clock now=%llu start=%llu last=%llu cancel=%d\n",(unsigned long long)m.time,(unsigned long long)s.session_start,(unsigned long long)s.last_clock,m.cancel);
  assert(!r&&m.connects==1&&s.actual_send==8192&&s.actual_receive==2304);
  assert(!nh_tcp_begin(&s,m.time,12));size_t accepted=0;uint8_t data[12]={0};
  if(f==CANCEL)m.cancel=1;if(f==CLOCK_FAIL)m.time=UINT64_MAX;if(f==CLOCK_BACK)m.time--;
  if(f==UNIT_EXPIRE)m.time+=NH_OUTPUT_NS;if(f==SESSION_EXPIRE)m.time=s.session_start+NH_SESSION_NS;
  if(f==CAP_FAIL)s.accepted=NH_OUTPUT_LIMIT;
  r=nh_tcp_write(&s,data,12,&accepted);
  if(f==CANCEL||f==CLOCK_FAIL||f==CLOCK_BACK||f==UNIT_EXPIRE||f==SESSION_EXPIRE||f==CAP_FAIL||f==SEND_ZERO||f==SEND_CLOSE){assert(r&&accepted==0&&m.closes==1);}
  else {
   if(r==CAMERA_BUSY){assert(!accepted);r=nh_tcp_write(&s,data,12,&accepted);}
   assert(!r&&accepted==3&&s.accepted==3&&s.unit_remaining==9);
   /* Multiple continuations do NOT restart whole-unit deadline. */
   m.time=s.unit_start+NH_OUTPUT_NS;assert(nh_tcp_write(&s,data,9,&accepted)&&!accepted);
  }
  (void)nh_tcp_discard(&s,0);(void)nh_tcp_close(&s);assert(m.closes==1&&m.connects==1);cases++;
 }
 printf("HOST TCP sink %u mocked cases PASS: partial/EINTR/EAGAIN/connect/zero/peer-close/cancel/clock/2s+10s/cap/requested-actual-buffers/close-once; NO_NETWORK\n",cases+1);
 return 0;
}
