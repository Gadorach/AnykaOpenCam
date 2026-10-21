/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "credential_ipc.h"
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
int cp_clock_read(uint64_t *out)
{
 struct timespec t;uint64_t base;
 if(!out||clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L||
    (uint64_t)t.tv_sec>UINT64_MAX/1000000000ULL)return -1;
 base=(uint64_t)t.tv_sec*1000000000ULL;
 if((uint64_t)t.tv_nsec>=UINT64_MAX-base)return -1; /* reserved MAX excluded */
 *out=base+(uint64_t)t.tv_nsec;return 0;
}
#ifdef CP_TEST_ONLY
uint64_t cp_now(void){uint64_t now;return cp_clock_read(&now)?UINT64_MAX:now;}
#endif
void cp_wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
void cp_u32(unsigned char *p,uint32_t x){unsigned i;for(i=0;i<4;i++)p[i]=(unsigned char)(x>>(8*i));}
void cp_u64(unsigned char *p,uint64_t x){unsigned i;for(i=0;i<8;i++)p[i]=(unsigned char)(x>>(8*i));}
uint32_t cp_get32(const unsigned char *p){uint32_t x=0;unsigned i;for(i=0;i<4;i++)x|=(uint32_t)p[i]<<(8*i);return x;}
uint64_t cp_get64(const unsigned char *p){uint64_t x=0;unsigned i;for(i=0;i<8;i++)x|=(uint64_t)p[i]<<(8*i);return x;}
int cp_id_valid(const char id[CAMERA_ID_BYTES])
{
 unsigned i,j;if(!id||!id[0])return 0;
 for(i=0;i<CAMERA_ID_BYTES;i++){
  unsigned char c=(unsigned char)id[i];
  if(!c){for(j=i;j<CAMERA_ID_BYTES;j++)if(id[j])return 0;return 1;}
  if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))return 0;
 }return 0;
}
void cp_pack_material(unsigned char *p,const struct cw_ui_material *m)
{
 cp_u32(p,m->schema);cp_u32(p+4,m->iterations);memcpy(p+8,m->salt,16);memcpy(p+24,m->verifier,32);
}
void cp_unpack_material(struct cw_ui_material *m,const unsigned char *p)
{
 m->schema=cp_get32(p);m->iterations=cp_get32(p+4);memcpy(m->salt,p+8,16);memcpy(m->verifier,p+24,32);
}
int cp_send(int fd,const struct cp_message *m)
{
 unsigned char data[CP_FRAME_MAX]={0};ssize_t n;int ret=-1;
 if(!m||m->bytes>CP_PAYLOAD_MAX||m->kind<CP_READY||m->kind>CP_SHUTDOWN)return -1;
 cp_u32(data,0x43504931U);cp_u32(data+4,1);cp_u32(data+8,m->kind);cp_u32(data+12,m->bytes);
 cp_u64(data+16,m->epoch);cp_u64(data+24,m->sequence);cp_u64(data+32,m->request_generation);
 cp_u64(data+40,m->credential_generation);cp_u64(data+48,m->deadline);cp_u32(data+56,m->domain);cp_u32(data+60,m->status);
 memcpy(data+CP_HEADER,m->payload,m->bytes);
 n=send(fd,data,CP_HEADER+m->bytes,MSG_NOSIGNAL|MSG_DONTWAIT);
 if(n==(ssize_t)(CP_HEADER+m->bytes))ret=0;
 else if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))ret=1;
 cp_wipe(data,sizeof(data));return ret;
}
int cp_receive(int fd,struct cp_message *out)
{
 unsigned char data[CP_FRAME_MAX]={0};union {struct cmsghdr align;unsigned char bytes[CMSG_SPACE(8*sizeof(int))];} control;
 struct iovec iov={data,sizeof(data)};struct msghdr msg={0};struct cmsghdr *c;
 ssize_t n;int ret=-1,ancillary=0;
 memset(&control,0,sizeof(control));memset(out,0,sizeof(*out));
 msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control.bytes;msg.msg_controllen=sizeof(control);
 n=recvmsg(fd,&msg,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
 if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)){ret=1;goto done;}
 if(n<=0)goto done;
 for(c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c)){
  ancillary=1;
  if(c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_RIGHTS&&c->cmsg_len>=CMSG_LEN(0)){
   size_t i,count=(c->cmsg_len-CMSG_LEN(0))/sizeof(int);int *fds=(int *)CMSG_DATA(c);
   for(i=0;i<count;i++)close(fds[i]);
  }
 }
 if(ancillary||(msg.msg_flags&(MSG_TRUNC|MSG_CTRUNC))||n<(ssize_t)CP_HEADER)goto done;
 if(cp_get32(data)!=0x43504931U||cp_get32(data+4)!=1||cp_get32(data+12)>CP_PAYLOAD_MAX||
    n!=(ssize_t)(CP_HEADER+cp_get32(data+12)))goto done;
 out->kind=cp_get32(data+8);if(out->kind<CP_READY||out->kind>CP_SHUTDOWN)goto done;
 out->bytes=cp_get32(data+12);out->epoch=cp_get64(data+16);out->sequence=cp_get64(data+24);
 out->request_generation=cp_get64(data+32);out->credential_generation=cp_get64(data+40);
 out->deadline=cp_get64(data+48);out->domain=cp_get32(data+56);out->status=cp_get32(data+60);
 memcpy(out->payload,data+CP_HEADER,out->bytes);ret=0;
done:cp_wipe(data,sizeof(data));cp_wipe(&control,sizeof(control));
 if(ret)cp_wipe(out,sizeof(*out));
 return ret;
}
