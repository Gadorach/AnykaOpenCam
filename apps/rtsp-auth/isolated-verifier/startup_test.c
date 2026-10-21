/* SPDX-License-Identifier: GPL-2.0-only
 * PUBLICDUMMY launch-barrier/image/material FD boundaries. Actual kernel owned
 * fork/pidfd/wait/EOF/PDEATH; injected entry faults only, no crypto/READY mocks. */
#define _GNU_SOURCE
#include "verifier.h"
#include <camera/rtsp_digest.h>
#include <camera/access_policy.h>
#include <mbedtls/sha256.h>
#include <linux/capability.h>
#include <elf.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){printf("HOST_ONLY FAIL check=%u line=%u\n",checks,(unsigned)__LINE__);return 1;}}while(0)
static void material(struct rv_material *m)
{
 memset(m,0,sizeof(*m));m->abi=RV_ABI;m->bytes=sizeof(*m);m->domain=CAMERA_ACCESS_RTSP;m->algorithm=1;
 m->epoch=888;m->boot_generation=777;m->credential_generation=7;m->account=9;
 strcpy(m->device,"PUBLICDUMMY-device");strcpy(m->account_id,"PUBLICDUMMY-rtsp");strcpy(m->sensor,"camera0");strcpy(m->profile,"camera0-main");
 memcpy(m->username,"PUBLICDUMMY",11);m->username_bytes=11;memcpy(m->realm,"PUBLICDUMMY RTSP",16);m->realm_bytes=16;m->ha1_bytes=32;
 m->nonce_ttl_ms=1000;m->window_ms=1000;m->global_verifies=m->account_verifies=m->peer_verifies=16;m->global_challenges=m->account_challenges=m->peer_challenges=8;
 strcpy(m->live,"rtsp://publicdummy.invalid/live");strcpy(m->track,"rtsp://publicdummy.invalid/live/track0");
 camera_rtsp_digest_ha1(1,(struct camera_digest_span){m->username,11},(struct camera_digest_span){m->realm,16},
  (struct camera_digest_span){(unsigned char *)"PUBLICDUMMY-not-product",sizeof("PUBLICDUMMY-not-product")-1},m->ha1);
}
static int matfd(const struct rv_material *m,int complete)
{
 int fd=(int)syscall(SYS_memfd_create,"PUBLICDUMMY-entry",MFD_CLOEXEC|MFD_ALLOW_SEALING);
 if(fd<0||fchmod(fd,0600)||write(fd,m,sizeof(*m))!=(ssize_t)sizeof(*m)||lseek(fd,0,SEEK_SET)!=0)return -1;
 if(complete&&fcntl(fd,F_ADD_SEALS,F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL))return -1;
 return fd;
}
static int image_pin(int fd,struct rv_image *im)
{
 struct stat st;unsigned char buf[4096];mbedtls_sha256_context h;size_t offset=0;
 if(fstat(fd,&st))return -1;
 memset(im,0,sizeof(*im));im->abi=RV_ABI;im->profile=RV_HOST_PUBLIC_4M;im->fd=fd;im->bytes=(size_t)st.st_size;
 mbedtls_sha256_init(&h);if(mbedtls_sha256_starts(&h,0))return -1;
 while(offset<im->bytes){size_t n=im->bytes-offset;if(n>sizeof(buf))n=sizeof(buf);if(pread(fd,buf,n,(off_t)offset)!=(ssize_t)n||mbedtls_sha256_update(&h,buf,n))return -1;offset+=n;}
 int r=mbedtls_sha256_finish(&h,im->sha256);mbedtls_sha256_free(&h);rv_wipe(buf,sizeof(buf));return r;
}
static int no_children(void)
{int status;errno=0;return waitpid(-1,&status,WNOHANG)==-1&&errno==ECHILD;}
static int injected_entry(const struct rv_image *im,const struct rv_material *m,unsigned fault,int success)
{
 struct rv_client *c=NULL;int fd=matfd(m,1);CHECK(fd>=0);rv_test_launch_fault(fault);
 int r=rv_start(&c,im,fd);rv_test_launch_fault(0);
 if(success){CHECK(r==0&&rv_test_barrier_released(c)==1&&rv_test_engine_ack(c)==1);CHECK(rv_stop(c)==0&&rv_retired(c)&&rv_wipe_observed(c));}
 else {
  CHECK(r==RV_HARDENING&&c&&rv_retired(c));CHECK(rv_test_barrier_released(c)==0&&rv_test_engine_ack(c)==0&&rv_ready_flags(c)==0);
  CHECK(lseek(fd,0,SEEK_CUR)==0); /*shared actual startup OFD: no child material read*/
 }
 CHECK(rv_destroy(&c)==0&&c==NULL);CHECK(no_children());close(fd);return 0;
}
static int early_pair=-1;
static void die_before_release(int pidfd)
{
 char byte=1;struct iovec iov={&byte,1};union{struct cmsghdr a;unsigned char b[CMSG_SPACE(sizeof(int))];} ancillary;
 memset(&ancillary,0,sizeof(ancillary));struct msghdr msg={.msg_iov=&iov,.msg_iovlen=1,.msg_control=ancillary.b,.msg_controllen=sizeof(ancillary)};
 struct cmsghdr *cm=CMSG_FIRSTHDR(&msg);cm->cmsg_level=SOL_SOCKET;cm->cmsg_type=SCM_RIGHTS;cm->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(cm),&pidfd,sizeof(pidfd));
 if(sendmsg(early_pair,&msg,MSG_NOSIGNAL)!=1)_exit(61);
 _exit(0); /*actual OWN parent death, after child PDEATH armed but BEFORE gate release/exec*/
}
static int early_death(const struct rv_image *im,const struct rv_material *m)
{
 int fd=matfd(m,1),pair[2],status;CHECK(fd>=0&&socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0,pair)==0);CHECK(prctl(PR_SET_CHILD_SUBREAPER,1,0,0,0)==0);
 pid_t parent=fork();CHECK(parent>=0);
 if(!parent){close(pair[0]);early_pair=pair[1];rv_test_before_release(die_before_release);struct rv_client *c=NULL;rv_start(&c,im,fd);_exit(62);}
 close(pair[1]);char byte=0;struct iovec iov={&byte,1};union{struct cmsghdr a;unsigned char b[CMSG_SPACE(sizeof(int))];} ancillary;
 memset(&ancillary,0,sizeof(ancillary));struct msghdr msg={.msg_iov=&iov,.msg_iovlen=1,.msg_control=ancillary.b,.msg_controllen=sizeof(ancillary)};
 CHECK(recvmsg(pair[0],&msg,MSG_CMSG_CLOEXEC)==1&&byte==1);struct cmsghdr *cm=CMSG_FIRSTHDR(&msg);CHECK(cm&&cm->cmsg_type==SCM_RIGHTS);int handle;memcpy(&handle,CMSG_DATA(cm),sizeof(handle));
 CHECK(waitpid(parent,&status,0)==parent&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 struct pollfd event={handle,POLLIN,0};CHECK(poll(&event,1,2000)>0&&(event.revents&POLLIN));
 pid_t child=waitpid(-1,&status,0);CHECK(child>0&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGKILL);
 CHECK(lseek(fd,0,SEEK_CUR)==0&&no_children());close(handle);close(pair[0]);close(fd);return 0;
}
static int copy_image(const char *directory,int input,const char *name,mode_t mode,int mutate)
{
 char path[2048];snprintf(path,sizeof(path),"%s/%s",directory,name);int fd=open(path,O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
 if(fd<0)return -1;
 unsigned char buf[4096];off_t offset=0;ssize_t n;
 while((n=pread(input,buf,sizeof(buf),offset))>0){if(write(fd,buf,(size_t)n)!=n)return -1;offset+=n;}
 if(n<0)return -1;
 if(mutate){Elf64_Ehdr h;if(pread(fd,&h,sizeof(h),0)!=(ssize_t)sizeof(h))return -1;
  if(mutate==1)h.e_machine=EM_AARCH64;
  else if(mutate==2)h.e_phnum=65535;
  if(pwrite(fd,&h,sizeof(h),0)!=(ssize_t)sizeof(h))return -1;
  if(mutate==3){for(unsigned i=0;i<h.e_phnum;i++){Elf64_Phdr p;if(pread(fd,&p,sizeof(p),(off_t)h.e_phoff+(off_t)i*sizeof(p))!=(ssize_t)sizeof(p))return -1;
    if(p.p_type==PT_GNU_STACK){p.p_flags|=PF_X;if(pwrite(fd,&p,sizeof(p),(off_t)h.e_phoff+(off_t)i*sizeof(p))!=(ssize_t)sizeof(p))return -1;break;}}}
 }
 rv_wipe(buf,sizeof(buf));if(fchmod(fd,mode))return -1;close(fd);return open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
}
int main(int argc,char **argv)
{
 CHECK(argc==4&&getuid()!=0);struct rlimit core={0,0};CHECK(setrlimit(RLIMIT_CORE,&core)==0&&prctl(PR_SET_DUMPABLE,0)==0);signal(SIGPIPE,SIG_IGN);
 struct rv_material m;material(&m);struct rv_image im;int image=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);CHECK(image>=0&&image_pin(image,&im)==0);
 for(unsigned f=1;f<=6;f++)if(injected_entry(&im,&m,f,f==RV_TEST_RELEASE_EINTR||f==RV_TEST_RELEASE_RECV_EINTR))return 1;
 if(early_death(&im,&m))return 1;
 /*Class mismatch is after owned release/hardening, BEFORE any material read or engine ACK.*/
 struct rv_image wrong;int strict=open(argv[2],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);CHECK(strict>=0&&image_pin(strict,&wrong)==0);struct rv_client *c=NULL;int fd=matfd(&m,1);
 CHECK(fd>=0&&rv_start(&c,&wrong,fd)==RV_HARDENING&&c&&rv_retired(c));CHECK(lseek(fd,0,SEEK_CUR)==0&&rv_test_engine_ack(c)==0);CHECK(rv_destroy(&c)==0&&no_children());close(fd);close(strict);
 /*Unsealed anonymous FD may not masquerade as trusted readonly/private material.*/
 fd=matfd(&m,0);CHECK(fd>=0&&rv_start(&c,&im,fd)==RV_HARDENING&&rv_retired(c));CHECK(lseek(fd,0,SEEK_CUR)==0&&rv_test_engine_ack(c)==0);CHECK(rv_destroy(&c)==0);close(fd);
 {char first[2048],second[2048];snprintf(first,sizeof(first),"%s/PUBLICDUMMY-blank-material",argv[3]);snprintf(second,sizeof(second),"%s/PUBLICDUMMY-blank-material-link",argv[3]);
  int blank=open(first,O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);CHECK(blank>=0&&ftruncate(blank,sizeof(m))==0);close(blank);
  CHECK(link(first,second)==0);fd=open(first,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);CHECK(fd>=0);
  CHECK(rv_start(&c,&im,fd)==RV_HARDENING&&rv_retired(c));CHECK(lseek(fd,0,SEEK_CUR)==0&&rv_test_engine_ack(c)==0);CHECK(rv_destroy(&c)==0&&no_children());close(fd);
 } /*zero test bytes only, NOT a persisted credential/payload hash*/
 for(unsigned i=0;i<7;i++){
  char name[32];snprintf(name,sizeof(name),"PUBLICDUMMY-image-%u",i);mode_t mode=i==0?0700:i==1?0755:i==2?0444:0555;
  int copy=copy_image(argv[3],image,name,mode,i>=3?(int)i-2:0);CHECK(copy>=0&&image_pin(copy,&wrong)==0);
  if(i==6){wrong.profile=RV_NATIVE;} /*root public/ARM gate cannot be satisfied by owned x86 bytes*/
  fd=matfd(&m,1);CHECK(fd>=0&&rv_start(&c,&wrong,fd)==RV_BAD&&c==NULL);CHECK(lseek(fd,0,SEEK_CUR)==0&&no_children());close(fd);close(copy);
 }
 /*High staging must survive actual caller destinations image3/material4.*/
 pid_t aliases=fork();CHECK(aliases>=0);
 if(!aliases){int a=fcntl(image,F_DUPFD_CLOEXEC,20),b=matfd(&m,1);if(a<0||b<0||dup2(a,3)<0||dup2(b,4)<0)_exit(71);
  struct rv_image pinned=im;pinned.fd=3;struct rv_client *child=NULL;int r=rv_start(&child,&pinned,4);
  if(r||rv_stop(child)||!rv_retired(child)||rv_destroy(&child))_exit(72);
  _exit(0);}
 int status;CHECK(waitpid(aliases,&status,0)==aliases&&WIFEXITED(status)&&WEXITSTATUS(status)==0&&no_children());
 close(image);rv_wipe(&m,sizeof(m));printf("HOST_ONLY PASS startup_checks=%u material_read_on_barrier_failure=0 engine_ACK_on_failure=0 owned_exit_observed=1 no_extra_child=1\n",checks);return 0;
}
