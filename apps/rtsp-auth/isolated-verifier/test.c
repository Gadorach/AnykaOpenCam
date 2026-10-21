/* SPDX-License-Identifier: GPL-2.0-only
 * PUBLICDUMMY IPC/real Mbed tests, no listener/media/private/device inputs. */
#define _GNU_SOURCE
#include "verifier.h"
#include <camera/rtsp_digest.h>
#include <camera/access_policy.h>
#include <mbedtls/sha256.h>
#include <mbedtls/md5.h>
#include <linux/capability.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){printf("HOST_ONLY FAIL check=%u line=%u\n",checks,(unsigned)__LINE__);return 1;}}while(0)
static uint64_t now(void){uint64_t n=0;if(rv_clock(&n))return 0;return n;}
static struct camera_digest_span S(const char *p){return (struct camera_digest_span){(const unsigned char *)p,strlen(p)};}
static void hex(const unsigned char *p,size_t n,char *out){static const char d[]="0123456789abcdef";for(size_t i=0;i<n;i++){out[2*i]=d[p[i]>>4];out[2*i+1]=d[p[i]&15];}out[2*n]=0;}
static void material(struct rv_material *m,unsigned alg)
{
 memset(m,0,sizeof(*m));m->abi=RV_ABI;m->bytes=sizeof(*m);m->domain=CAMERA_ACCESS_RTSP;m->algorithm=alg;
 m->epoch=888;m->boot_generation=777;m->credential_generation=7;m->account=9;
 memcpy(m->device,"PUBLICDUMMY-device",19);memcpy(m->account_id,"PUBLICDUMMY-rtsp",16);memcpy(m->sensor,"camera0",8);memcpy(m->profile,"camera0-main",13);
 memcpy(m->username,"PUBLICDUMMY",11);m->username_bytes=11;memcpy(m->realm,"PUBLICDUMMY RTSP",16);m->realm_bytes=16;m->ha1_bytes=alg==1?32:16;
 m->nonce_ttl_ms=1000;m->window_ms=1000;m->global_verifies=m->account_verifies=m->peer_verifies=16;m->global_challenges=m->account_challenges=m->peer_challenges=8;
 strcpy(m->live,"rtsp://publicdummy.invalid/live");strcpy(m->track,"rtsp://publicdummy.invalid/live/track0");
 camera_rtsp_digest_ha1(alg,S("PUBLICDUMMY"),S("PUBLICDUMMY RTSP"),S("PUBLICDUMMY-not-product"),m->ha1);
}
static int material_fd(const struct rv_material *m)
{
 int fd=(int)syscall(SYS_memfd_create,"PUBLICDUMMY-RTSP-startup",MFD_CLOEXEC|MFD_ALLOW_SEALING);
 if(fd<0||fchmod(fd,0600)||write(fd,m,sizeof(*m))!=(ssize_t)sizeof(*m)||lseek(fd,0,SEEK_SET)!=0||
    fcntl(fd,F_ADD_SEALS,F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL))return -1;
 return fd;
}
static int image(const char *path,unsigned profile,struct rv_image *out)
{
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);unsigned char data[4096];mbedtls_sha256_context hash;ssize_t n;
 if(fd<0)return -1;
 memset(out,0,sizeof(*out));out->abi=RV_ABI;out->profile=profile;out->fd=fd;mbedtls_sha256_init(&hash);
 if(mbedtls_sha256_starts(&hash,0))return -1;
 while((n=read(fd,data,sizeof(data)))>0){out->bytes+=(size_t)n;if(mbedtls_sha256_update(&hash,data,(size_t)n))return -1;}
 int r=n<0?-1:mbedtls_sha256_finish(&hash,out->sha256);mbedtls_sha256_free(&hash);rv_wipe(data,sizeof(data));lseek(fd,0,SEEK_SET);return r;
}
static int finish(struct rv_client *c,uint64_t *token)
{uint64_t until=now()+2000000000ULL;int r;do{r=rv_complete(c,token);if(r!=RV_MORE)return r;poll(NULL,0,2);}while(now()<until);return RV_LOST;}
static int request(unsigned alg,const struct rv_material *m,const char *nonce,const char *nc,const char *cnonce,int wrong,char *out,size_t capacity)
{
 char a1[65],a2[65],response[65],joined[1024];unsigned char digest[32];int n;
 hex(m->ha1,alg==1?32:16,a1);n=snprintf(joined,sizeof(joined),"DESCRIBE:%s",m->live);
 if(alg==1)mbedtls_sha256((unsigned char *)joined,(size_t)n,digest,0);else mbedtls_md5((unsigned char *)joined,(size_t)n,digest);
 hex(digest,alg==1?32:16,a2);n=snprintf(joined,sizeof(joined),"%s:%s:%s:%s:auth:%s",a1,nonce,nc,cnonce,a2);
 if(alg==1)mbedtls_sha256((unsigned char *)joined,(size_t)n,digest,0);else mbedtls_md5((unsigned char *)joined,(size_t)n,digest);
 hex(digest,alg==1?32:16,response);if(wrong)response[0]=response[0]=='a'?'b':'a';
 n=snprintf(out,capacity,"DESCRIBE %s RTSP/1.0\r\nCSeq: 1\r\nAuthorization: Digest username=\"PUBLICDUMMY\", realm=\"PUBLICDUMMY RTSP\", uri=\"%s\", algorithm=%s, qop=auth, nonce=\"%s\", nc=%s, cnonce=\"%s\", response=\"%s\"\r\n\r\n",
 m->live,m->live,alg==1?"SHA-256":"MD5",nonce,nc,cnonce,response);
 rv_wipe(a1,sizeof(a1));rv_wipe(a2,sizeof(a2));rv_wipe(response,sizeof(response));rv_wipe(joined,sizeof(joined));rv_wipe(digest,sizeof(digest));return n<0||(size_t)n>=capacity?-1:n;
}
static int run_profile(const char *path,unsigned alg,int delayed)
{
 struct rv_material m;struct rv_image im;struct rv_client *c=NULL;struct rv_message msg={0};uint64_t token,conn,other;char nonce[65],wire[2048];int n,fd;
 material(&m,alg);CHECK(image(path,RV_HOST_PUBLIC_4M,&im)==0);fd=material_fd(&m);CHECK(fd>=0);
 int started=rv_start(&c,&im,fd);CHECK(started==RV_OK);CHECK(rv_ready_flags(c)==15&&rv_ready_profile(c)==RV_HOST_PUBLIC_4M);
 msg.kind=RV_OPEN;CHECK(rv_rpc(c,&msg)==0);conn=msg.connection;CHECK(conn!=0);
 memset(&msg,0,sizeof(msg));msg.kind=RV_CHALLENGE;msg.connection=conn;msg.bytes=8;rv_put32(msg.payload,1);rv_put32(msg.payload+4,0);
 CHECK(rv_rpc(c,&msg)==0&&msg.bytes<512);msg.payload[msg.bytes]=0;
 {char *p=strstr((char *)msg.payload,"nonce=\"");CHECK(p!=NULL);memcpy(nonce,p+7,64);nonce[64]=0;}
 n=request(alg,&m,nonce,"00000001","PUBLICDUMMY-client-A",1,wire,sizeof(wire));CHECK(n>0);
 CHECK(rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);CHECK(finish(c,&token)!=0&&token==0);
 n=request(alg,&m,nonce,"00000001","PUBLICDUMMY-client-A",0,wire,sizeof(wire));CHECK(n>0);
 CHECK(rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);CHECK(finish(c,&token)==0&&token!=0);
 CHECK(rv_query(c,conn,token)==0);CHECK(rv_query(c,conn,token+1)!=0);
 memset(&msg,0,sizeof(msg));msg.kind=RV_OPEN;CHECK(rv_rpc(c,&msg)==0);other=msg.connection;CHECK(other!=conn);
 CHECK(rv_query(c,other,token)!=0);
 n=request(alg,&m,nonce,"00000002","PUBLICDUMMY-fresh-B",0,wire,sizeof(wire));CHECK(n>0);
 CHECK(rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);CHECK(finish(c,&token)==0);CHECK(rv_query(c,conn,token)==0);
 n=request(alg,&m,nonce,"00000002","PUBLICDUMMY-fresh-C",0,wire,sizeof(wire));CHECK(n>0);
 CHECK(rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);CHECK(finish(c,&token)!=0&&token==0);
 n=request(alg,&m,nonce,"00000003","PUBLICDUMMY-fresh-D",0,wire,sizeof(wire));CHECK(n>0);
 CHECK(rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);CHECK(rv_discard(c)==0);
 CHECK(rv_submit(c,other,wire,(size_t)n,now()+1000000000ULL)==RV_BUSY);CHECK(finish(c,&token)==RV_DENIED&&token==0);
 /* Global/account histories stay in child; peer close/open never resets them. */
 memset(&msg,0,sizeof(msg));msg.kind=RV_CLOSE;msg.connection=other;CHECK(rv_rpc(c,&msg)==0);
 if(delayed){
  n=request(alg,&m,nonce,"00000004","PUBLICDUMMY-pending-revoke",0,wire,sizeof(wire));CHECK(n>0);
  CHECK(rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);
  CHECK(rv_owner_revoke(c)==RV_BUSY);CHECK(rv_query(c,conn,1)!=0);
  CHECK(rv_submit(c,other,wire,(size_t)n,now()+1000000000ULL)==RV_BUSY);CHECK(finish(c,&token)==RV_DENIED&&token==0);
 }
 CHECK(rv_owner_revoke(c)==0);
 CHECK(rv_query(c,conn,1)!=0);
 CHECK(rv_stop(c)==0&&rv_retired(c)&&rv_wipe_observed(c));CHECK(rv_destroy(&c)==0&&c==NULL);
 close(im.fd);close(fd);rv_wipe(&m,sizeof(m));rv_wipe(wire,sizeof(wire));rv_wipe(nonce,sizeof(nonce));(void)delayed;return 0;
}
static int hardfault_expiry(const char *path)
{
 struct rv_material m;struct rv_image im;struct rv_client *c=NULL;struct rv_message msg={0};char nonce[65],wire[2048];uint64_t token,conn;int fd,n;
 material(&m,1);m.nonce_ttl_ms=200;CHECK(image(path,RV_HOST_PUBLIC_4M,&im)==0);fd=material_fd(&m);CHECK(fd>=0&&rv_start(&c,&im,fd)==0);
 msg.kind=RV_OPEN;CHECK(rv_rpc(c,&msg)==0);conn=msg.connection;
 memset(&msg,0,sizeof(msg));msg.kind=RV_CHALLENGE;msg.connection=conn;msg.bytes=8;rv_put32(msg.payload,1);
 CHECK(rv_rpc(c,&msg)==0);msg.payload[msg.bytes]=0;char *p=strstr((char *)msg.payload,"nonce=\"");CHECK(p!=NULL);memcpy(nonce,p+7,64);nonce[64]=0;
 n=request(1,&m,nonce,"00000001","PUBLICDUMMY-expiry",0,wire,sizeof(wire));CHECK(n>0&&rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);
 CHECK(finish(c,&token)==0&&rv_query(c,conn,token)==0);poll(NULL,0,250);
 CHECK(rv_query(c,conn,token)!=0); /*fresh child check, not cached auth TTL*/
 CHECK(rv_owned_signal(c,SIGKILL)==0);poll(NULL,0,10);CHECK(rv_query(c,conn,token)!=0);
 CHECK(rv_stop(c)==0&&rv_retired(c));CHECK(!rv_wipe_observed(c)); /*hardfault retires address space, NOT user-space wipe proof*/
 CHECK(rv_destroy(&c)==0);close(fd);close(im.fd);rv_wipe(&m,sizeof(m));rv_wipe(wire,sizeof(wire));return 0;
}
static int replace(char *buffer,size_t cap,const char *from,const char *to)
{
 char *p=strstr(buffer,from);if(!p)return -1;
 size_t n=strlen(to),tail=strlen(p+strlen(from));if((size_t)(p-buffer)+n+tail>=cap)return -1;
 memmove(p+n,p+strlen(from),tail+1);memcpy(p,to,n);return 0;
}
static int malformed_wire(const char *path,unsigned alg)
{
 struct rv_material m;struct rv_image im;struct rv_client *c=NULL;struct rv_message msg={0};char nonce[65],base[2048],wire[2048];uint64_t token,conn;int fd,n;
 material(&m,alg);m.nonce_ttl_ms=5000;m.window_ms=5000;m.global_verifies=m.account_verifies=m.peer_verifies=64;
 CHECK(image(path,RV_HOST_PUBLIC_4M,&im)==0);fd=material_fd(&m);CHECK(fd>=0&&rv_start(&c,&im,fd)==0);
 msg.kind=RV_OPEN;CHECK(rv_rpc(c,&msg)==0);conn=msg.connection;memset(&msg,0,sizeof(msg));msg.kind=RV_CHALLENGE;msg.connection=conn;msg.bytes=8;rv_put32(msg.payload,1);
 CHECK(rv_rpc(c,&msg)==0);msg.payload[msg.bytes]=0;char *p=strstr((char *)msg.payload,"nonce=\"");CHECK(p!=NULL);memcpy(nonce,p+7,64);nonce[64]=0;
 CHECK(request(alg,&m,nonce,"00000001","PUBLICDUMMY-negative",0,base,sizeof(base))>0);
 static const struct{const char *from,*to;} faults[]={
  {"Digest username=","Basic username="},{"qop=auth","qop=auth-int"},
  {"qop=auth","qop="},{"nc=00000001","nc=00000000"},
  {"username=\"PUBLICDUMMY\"","username*=\"PUBLICDUMMY\""},
  {"qop=auth","qop=auth, QOP=auth"},
  {"rtsp://publicdummy.invalid/live","rtsp://PUBLICDUMMY@publicdummy.invalid/live"}};
 for(size_t i=0;i<sizeof(faults)/sizeof(faults[0]);i++){
  strcpy(wire,base);CHECK(replace(wire,sizeof(wire),faults[i].from,faults[i].to)==0);n=(int)strlen(wire);
  CHECK(rv_submit(c,conn,wire,(size_t)n,now()+1000000000ULL)==0);CHECK(finish(c,&token)!=0&&token==0);
 }
 strcpy(wire,base);CHECK(replace(wire,sizeof(wire),alg==1?"algorithm=SHA-256":"algorithm=MD5",alg==1?"algorithm=MD5":"algorithm=SHA-256")==0);
 CHECK(rv_submit(c,conn,wire,strlen(wire),now()+1000000000ULL)==0);CHECK(finish(c,&token)!=0&&token==0);
 strcpy(wire,base);CHECK(replace(wire,sizeof(wire),alg==1?"algorithm=SHA-256":"algorithm=MD5",alg==1?"algorithm=SHA-256-sess":"algorithm=MD5-sess")==0);
 CHECK(rv_submit(c,conn,wire,strlen(wire),now()+1000000000ULL)==0);CHECK(finish(c,&token)!=0&&token==0);
 CHECK(rv_submit(c,conn,base,strlen(base),now()+1000000000ULL)==0);CHECK(finish(c,&token)==0&&rv_query(c,conn,token)==0); /*invalid work cannot advance global nc*/
 CHECK(rv_stop(c)==0&&rv_retired(c)&&rv_wipe_observed(c));CHECK(rv_destroy(&c)==0);close(fd);close(im.fd);rv_wipe(&m,sizeof(m));rv_wipe(wire,sizeof(wire));rv_wipe(base,sizeof(base));return 0;
}
static int pdeath(const char *path)
{
 struct rv_material m;struct rv_image im;material(&m,1);int fd=material_fd(&m),pair[2];
 CHECK(fd>=0&&image(path,RV_HOST_PUBLIC_4M,&im)==0&&socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0,pair)==0);
 CHECK(prctl(PR_SET_CHILD_SUBREAPER,1,0,0,0)==0);
 pid_t creator=fork();CHECK(creator>=0);
 if(!creator){
  close(pair[0]);struct rv_client *child=NULL;if(rv_start(&child,&im,fd))_exit(31);
  int handle=rv_test_owned_pidfd(child);if(handle<0)_exit(32);
  char byte=1;struct iovec iov={&byte,1};union{struct cmsghdr align;unsigned char bytes[CMSG_SPACE(sizeof(int))];} ancillary;
  memset(&ancillary,0,sizeof(ancillary));struct msghdr msg={.msg_iov=&iov,.msg_iovlen=1,.msg_control=ancillary.bytes,.msg_controllen=sizeof(ancillary)};
  struct cmsghdr *cm=CMSG_FIRSTHDR(&msg);cm->cmsg_level=SOL_SOCKET;cm->cmsg_type=SCM_RIGHTS;cm->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(cm),&handle,sizeof(handle));
  if(sendmsg(pair[1],&msg,MSG_NOSIGNAL)!=1)_exit(33);
  _exit(0); /*OWN creator death, no verifier shutdown signal; kernel PDEATHSIG required*/
 }
 close(pair[1]);char byte=0;struct iovec iov={&byte,1};union{struct cmsghdr align;unsigned char bytes[CMSG_SPACE(sizeof(int))];} ancillary;
 memset(&ancillary,0,sizeof(ancillary));struct msghdr msg={.msg_iov=&iov,.msg_iovlen=1,.msg_control=ancillary.bytes,.msg_controllen=sizeof(ancillary)};
 CHECK(recvmsg(pair[0],&msg,MSG_CMSG_CLOEXEC)==1&&byte==1);struct cmsghdr *cm=CMSG_FIRSTHDR(&msg);CHECK(cm&&cm->cmsg_type==SCM_RIGHTS);int handle;memcpy(&handle,CMSG_DATA(cm),sizeof(handle));
 int status;CHECK(waitpid(creator,&status,0)==creator&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 struct pollfd event={handle,POLLIN,0};CHECK(poll(&event,1,2000)>0&&(event.revents&POLLIN));
 pid_t adopted=waitpid(-1,&status,0);CHECK(adopted>0&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGKILL); /*own lineage only, no PID selectors*/
 close(handle);close(pair[0]);close(fd);close(im.fd);rv_wipe(&m,sizeof(m));return 0;
}
int main(int argc,char **argv)
{
 struct rlimit core={0,0};struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};struct __user_cap_data_struct caps[2]={{0}};
 CHECK(argc==4);CHECK(getuid()!=0);CHECK(setrlimit(RLIMIT_CORE,&core)==0&&prctl(PR_SET_DUMPABLE,0)==0&&syscall(SYS_capset,&h,caps)==0);
 struct rv_material m;struct rv_image im;struct rv_client *c=NULL;material(&m,1);int fd=material_fd(&m);CHECK(fd>=0);
 CHECK(image(argv[1],RV_NATIVE,&im)==0);CHECK(rv_start(&c,&im,fd)==RV_BAD);CHECK(lseek(fd,0,SEEK_CUR)==0);CHECK(c==NULL);close(im.fd);close(fd);rv_wipe(&m,sizeof(m));
 printf("HOST_ONLY native_ROOT_2M=HOLD root_public_ARM_image_origin_unavailable=1 strict_material_read=0 native_fork=0\n");
 if(run_profile(argv[2],1,0)||run_profile(argv[2],2,0)||run_profile(argv[3],1,1)||malformed_wire(argv[2],1)||malformed_wire(argv[2],2)||hardfault_expiry(argv[2])||pdeath(argv[2]))return 1;
 printf("HOST_ONLY PASS checks=%u real_child_flags15=1 HOST_memlock4M=1 crypto_mocked=0 native_ROOT_2M=HOLD\n",checks);return 0;
}
