/* SPDX-License-Identifier: GPL-2.0-only
 * Adapted from pinned canonical credential_ipc.c; UI material packers removed. */
#define _GNU_SOURCE
#include "verifier.h"
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
int rv_clock(uint64_t *out)
{
 struct timespec t;uint64_t base;
 if(!out||clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L||
    (uint64_t)t.tv_sec>UINT64_MAX/1000000000ULL)return -1;
 base=(uint64_t)t.tv_sec*1000000000ULL;if((uint64_t)t.tv_nsec>=UINT64_MAX-base)return -1;
 *out=base+(uint64_t)t.tv_nsec;return 0;
}
void rv_wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
void rv_put32(unsigned char *p,uint32_t v){unsigned i;for(i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
void rv_put64(unsigned char *p,uint64_t v){unsigned i;for(i=0;i<8;i++)p[i]=(unsigned char)(v>>(8*i));}
uint32_t rv_get32(const unsigned char *p){uint32_t v=0;unsigned i;for(i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
uint64_t rv_get64(const unsigned char *p){uint64_t v=0;unsigned i;for(i=0;i<8;i++)v|=(uint64_t)p[i]<<(8*i);return v;}
int rv_send(int fd,const struct rv_message *m)
{
 unsigned char data[RV_HEADER+RV_PAYLOAD]={0};ssize_t n;int r=-1;
 if(!m||m->bytes>RV_PAYLOAD||m->kind<RV_READY||m->kind>RV_BYE)return -1;
 rv_put32(data,0x52564931U);rv_put32(data+4,RV_ABI);rv_put32(data+8,m->kind);rv_put32(data+12,m->bytes);
 rv_put64(data+16,m->epoch);rv_put64(data+24,m->sequence);rv_put64(data+32,m->connection);rv_put64(data+40,m->scope);
 rv_put64(data+48,m->generation);rv_put64(data+56,m->deadline);rv_put32(data+64,(uint32_t)m->status);rv_put32(data+68,m->flags);rv_put64(data+72,m->token);
 memcpy(data+RV_HEADER,m->payload,m->bytes);n=send(fd,data,RV_HEADER+m->bytes,MSG_DONTWAIT|MSG_NOSIGNAL);
 if(n==(ssize_t)(RV_HEADER+m->bytes))r=0;
 else if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))r=1;
 rv_wipe(data,sizeof(data));return r;
}
int rv_receive(int fd,struct rv_message *out)
{
 unsigned char data[RV_HEADER+RV_PAYLOAD]={0};union {struct cmsghdr align;unsigned char bytes[CMSG_SPACE(8*sizeof(int))];} control;
 struct iovec iov={data,sizeof(data)};struct msghdr msg={0};struct cmsghdr *c;int ancillary=0,r=-1;ssize_t n;
 if(!out)return -1;
 memset(out,0,sizeof(*out));memset(&control,0,sizeof(control));
 msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control.bytes;msg.msg_controllen=sizeof(control);
 n=recvmsg(fd,&msg,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
 if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)){r=1;goto done;}
 if(n<=0)goto done;
 for(c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c)){
  ancillary=1;
  if(c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_RIGHTS&&c->cmsg_len>=CMSG_LEN(0)){
   size_t i,count=(c->cmsg_len-CMSG_LEN(0))/sizeof(int);int *fds=(int *)CMSG_DATA(c);for(i=0;i<count;i++)close(fds[i]);
  }
 }
 if(ancillary||(msg.msg_flags&(MSG_TRUNC|MSG_CTRUNC))||n<(ssize_t)RV_HEADER||rv_get32(data)!=0x52564931U||rv_get32(data+4)!=RV_ABI||
    rv_get32(data+12)>RV_PAYLOAD||n!=(ssize_t)(RV_HEADER+rv_get32(data+12)))goto done;
 out->kind=rv_get32(data+8);if(out->kind<RV_READY||out->kind>RV_BYE)goto done;
 out->bytes=rv_get32(data+12);out->epoch=rv_get64(data+16);out->sequence=rv_get64(data+24);out->connection=rv_get64(data+32);out->scope=rv_get64(data+40);
 out->generation=rv_get64(data+48);out->deadline=rv_get64(data+56);out->status=(int32_t)rv_get32(data+64);out->flags=rv_get32(data+68);out->token=rv_get64(data+72);
 memcpy(out->payload,data+RV_HEADER,out->bytes);r=0;
 done:rv_wipe(data,sizeof(data));rv_wipe(&control,sizeof(control));if(r)rv_wipe(out,sizeof(*out));return r;
}
