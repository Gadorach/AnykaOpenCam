/* SPDX-License-Identifier: GPL-2.0-only
 * One outbound TCP qualification sink; no listener/DNS/reconnect/AU copy/ACK
 * protocol. Core rechecks CPU lease before EVERY send attempt/continuation.
 */
#define _POSIX_C_SOURCE 200809L
#include "tcp_sink.h"
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
static const char *allowed_peer(void)
{
#ifdef NH_TCP_PUBLIC_DUMMY_HOST
 return "127.0.0.1";
#else
 return "192.168.1.15";
#endif
}
int nh_tcp_endpoint(const char *peer,const char *port,struct sockaddr_in *out)
{
 if(!peer||!port||!out||strcmp(peer,allowed_peer())||!*port||strlen(port)>5)return CAMERA_INVALID;
 unsigned n=0;for(const char *p=port;*p;p++){if(*p<'0'||*p>'9')return CAMERA_INVALID;n=n*10U+(unsigned)(*p-'0');}
 if(n<1024||n>65535)return CAMERA_INVALID;
#ifndef NH_TCP_PUBLIC_DUMMY_HOST
 if(n!=NH_TCP_PARENT_PORT)return CAMERA_INVALID;
#endif
 memset(out,0,sizeof(*out));out->sin_family=AF_INET;out->sin_port=htons((uint16_t)n);
 return inet_pton(AF_INET,peer,&out->sin_addr)==1?CAMERA_OK:CAMERA_INVALID;
}
static int checked(struct nh_tcp_sink *s,uint64_t start,uint64_t *now)
{
 *now=s->now(s->clock_context);
 if(*now==UINT64_MAX||*now<start||*now<s->session_start||*now<s->last_clock||s->cancelled(s->clock_context)||
    *now-start>=NH_OUTPUT_NS||*now-s->session_start>=NH_SESSION_NS)return CAMERA_IO_ERROR;
 s->last_clock=*now;return CAMERA_OK;
}
int nh_tcp_close(struct nh_tcp_sink *s)
{
 if(s->close_attempted)return s->close_result;
 s->close_attempted=1;int fd=s->fd;s->fd=-1;
 s->close_result=fd>=0&&close(fd)?CAMERA_IO_ERROR:CAMERA_OK;
 return s->close_result; /* no close retry after ambiguous result */
}
static int failed(struct nh_tcp_sink *s)
{s->failed=1;(void)nh_tcp_close(s);return CAMERA_IO_ERROR;}
static int wait_write(struct nh_tcp_sink *s,uint64_t start)
{
 for(;;){
  uint64_t now;if(checked(s,start,&now))return failed(s);
  uint64_t left=NH_OUTPUT_NS-(now-start),session_left=NH_SESSION_NS-(now-s->session_start);
  if(left>session_left)left=session_left;
  int ms=(int)((left+999999ULL)/1000000ULL);if(ms>2000)ms=2000;
  struct pollfd p={.fd=s->fd,.events=POLLOUT};int result=poll(&p,1,ms);
  int poll_error=result<0?errno:0; /* health/clock callbacks may change errno */
  if(checked(s,start,&now))return failed(s);
  if(result<0&&poll_error==EINTR)continue; /* same deadline; no source/connect retry */
  if(result<=0||p.revents&(POLLERR|POLLHUP|POLLNVAL)||!(p.revents&POLLOUT))return failed(s);
  return CAMERA_OK;
 }
}
int nh_tcp_connect(struct nh_tcp_sink *s,const struct sockaddr_in *peer,uint64_t start,
 uint64_t (*now)(void *),int (*cancelled)(void *),void *context)
{
 if(!s||!peer||!now||!cancelled||start==UINT64_MAX)return CAMERA_INVALID;
 struct sockaddr_in allowed;unsigned n=ntohs(peer->sin_port);
 /* Revalidate direct API callers too; public CLI cannot select another host. */
 if(peer->sin_family!=AF_INET||inet_pton(AF_INET,allowed_peer(),&allowed.sin_addr)!=1||
    peer->sin_addr.s_addr!=allowed.sin_addr.s_addr)return CAMERA_INVALID;
#ifndef NH_TCP_PUBLIC_DUMMY_HOST
 if(n!=NH_TCP_PARENT_PORT)return CAMERA_INVALID;
#endif
 if(n<1024)return CAMERA_INVALID;
 memset(s,0,sizeof(*s));s->fd=-1;s->now=now;s->cancelled=cancelled;s->clock_context=context;s->session_start=start;
 s->requested_send=NH_TCP_SNDBUF_REQUEST;s->requested_receive=NH_TCP_RCVBUF_REQUEST;
 uint64_t clock;if(checked(s,start,&clock))return failed(s);
 s->fd=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);if(s->fd<0)return failed(s);
 if(setsockopt(s->fd,SOL_SOCKET,SO_SNDBUF,&s->requested_send,sizeof(int))||
    setsockopt(s->fd,SOL_SOCKET,SO_RCVBUF,&s->requested_receive,sizeof(int)))return failed(s);
 socklen_t len=sizeof(int);
 if(getsockopt(s->fd,SOL_SOCKET,SO_SNDBUF,&s->actual_send,&len)||len!=sizeof(int)||s->actual_send<=0||s->actual_send>8192)return failed(s);
 len=sizeof(int);if(getsockopt(s->fd,SOL_SOCKET,SO_RCVBUF,&s->actual_receive,&len)||len!=sizeof(int)||s->actual_receive<=0||s->actual_receive>4096)return failed(s);
 if(checked(s,start,&clock))return failed(s);
  int result=connect(s->fd,(const struct sockaddr *)peer,sizeof(*peer));
  int connect_error=result?errno:0;
  if(result&&connect_error!=EINPROGRESS&&connect_error!=EINTR)return failed(s);
 if(result&&wait_write(s,start))return CAMERA_IO_ERROR;
 int error=0;len=sizeof(error);
 if(getsockopt(s->fd,SOL_SOCKET,SO_ERROR,&error,&len)||len!=sizeof(error)||error||checked(s,start,&clock))return failed(s);
 struct sockaddr_in connected;len=sizeof(connected);memset(&connected,0,sizeof(connected));
 if(getpeername(s->fd,(struct sockaddr *)&connected,&len)||len!=sizeof(connected)||
    connected.sin_family!=AF_INET||connected.sin_addr.s_addr!=peer->sin_addr.s_addr||connected.sin_port!=peer->sin_port||checked(s,start,&clock))return failed(s);
 return CAMERA_OK;
}
int nh_tcp_begin(void *context,uint64_t start,size_t total)
{
 struct nh_tcp_sink *s=context;uint64_t now;
 if(s->failed||s->fd<0||!total||total>NH_OUTPUT_LIMIT-s->accepted||checked(s,start,&now))return failed(s);
 s->unit_start=start;s->unit_remaining=total;return CAMERA_OK;
}
int nh_tcp_write(void *context,const uint8_t *data,size_t bytes,size_t *accepted)
{
 struct nh_tcp_sink *s=context;uint64_t now;*accepted=0;
 if(s->failed||s->fd<0||!data||!bytes||bytes>8192||bytes>s->unit_remaining||
    bytes>NH_OUTPUT_LIMIT-s->accepted||checked(s,s->unit_start,&now))return failed(s);
 ssize_t result=send(s->fd,data,bytes,MSG_NOSIGNAL);
 if(result<0){
  int error=errno;
  if(error==EINTR){if(checked(s,s->unit_start,&now))return failed(s);return CAMERA_BUSY;}
  if(error==EAGAIN||error==EWOULDBLOCK){if(wait_write(s,s->unit_start))return CAMERA_IO_ERROR;return CAMERA_BUSY;}
  return failed(s);
 }
 if(result<=0||(size_t)result>bytes)return failed(s);
 *accepted=(size_t)result;s->accepted+=*accepted;s->unit_remaining-=*accepted;
 if(checked(s,s->unit_start,&now))return failed(s); /* accepted prefix still counted */
 return CAMERA_OK;
}
int nh_tcp_discard(void *context,size_t complete)
{
 struct nh_tcp_sink *s=context;s->complete_prefix=complete;
 /* TCP accepted bytes cannot be undone or called remote ACK. Receiver retains
  * all wire bytes and independently stages only its validated complete prefix. */
 return nh_tcp_close(s);
}
