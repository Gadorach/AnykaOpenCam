/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "launch_security.h"
#include <errno.h>
#include <fcntl.h>
#include <mbedtls/platform_util.h>
#include <poll.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <unistd.h>
/* Static allocation is worker-private after real exec, never parent-owned/COW
 * credential storage. No new heap/allocation requests or runtime mode selector. */
static struct pe_public public;
static unsigned char packet[PE_PACKET],reply[PE_REPLY];
static struct {
 unsigned char header[16],payload[256],extra;
 struct rv_material material;
} secret;
static int unexpected[PE_RIGHTS_MAX]={-1,-1,-1,-1,-1,-1,-1,-1};
static unsigned rights,calls;
static int charge(void)
{if(calls>=2048)return -1;calls++;return 0;}
static int pause_input(int fd)
{
 struct pollfd p={fd,POLLIN,0};
 if(charge())return -1;
 int r=poll(&p,1,1);return r<0&&errno!=EINTR?-1:0;
}
static int request(uint64_t entry)
{
 for(;;){
  ssize_t n;unsigned bad;
  if(charge())return -1;
  int r=pe_receive(3,packet,sizeof(packet),&n,unexpected,&rights,&bad);
  if(r==1){if(pause_input(3))return -1;continue;}
  if(r<0||bad||rights||n<0||pe_decode_request(&public,packet,(size_t)n))return -1;
  /* First receipt supplies the original deadline, never a newly granted budget.
   * Bounded prepacket waits cannot know it; no new relative timeout is invented. */
  if(entry<public.binding.started||entry>=public.binding.deadline)return -1;
  return 0;
 }
}
static int read_part(int fd,unsigned char *out,size_t n,int eof,uint64_t *last)
{
 size_t done=0;
 for(;;){
  if(pe_tick(last,public.binding.deadline)||charge())return -1;
  ssize_t got=read(fd,out+done,n-done);
  if(eof&&got==0)return 0;
  if(eof&&got>0)return -1;
  if(got>0){done+=(size_t)got;if(done==n)return 0;continue;}
  if(got<0&&errno==EINTR)continue;
  if(got<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){if(pause_input(fd))return -1;continue;}
  return -1;
 }
}
static int roles(pid_t parent,int bindings)
{
 struct stat st[3];
 if(getppid()!=parent||pe_control(3,parent,&st[2])||pe_ingress(4,&st[0])||pe_directory(5,&st[1]))return -1;
 if(bindings)for(unsigned i=0;i<3;i++)if((uint64_t)st[i].st_dev!=public.binding.device[i]||
    (uint64_t)st[i].st_ino!=public.binding.inode[i])return -1;
 return 0;
}
static int send_material(int fd)
{
 union {struct cmsghdr align;unsigned char bytes[CMSG_SPACE(sizeof(int))];} control;
 memset(&control,0,sizeof(control));struct iovec iov={reply,sizeof(reply)};
 struct msghdr m={.msg_iov=&iov,.msg_iovlen=1,.msg_control=control.bytes,.msg_controllen=sizeof(control.bytes)};
 struct cmsghdr *c=CMSG_FIRSTHDR(&m);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));
 memcpy(CMSG_DATA(c),&fd,sizeof(fd));
 return sendmsg(3,&m,MSG_DONTWAIT|MSG_NOSIGNAL)==PE_REPLY?0:-1;
}
int main(int argc,char **argv,char **envp)
{
 struct rtsp_private_handoff handoff=RTSP_PRIVATE_HANDOFF_INIT;
 struct candidate_prestarted local_unused=CANDIDATE_PRESTARTED_INIT;
 int ingress=4,control=3,directory=5,result=1;uint64_t last;pid_t parent=getppid();
 if(argc!=1||!argv||!argv[0]||strcmp(argv[0],PE_ARGV0)||argv[1]||!envp||envp[0]||
    parent<=1||pe_security_verify()||pe_clock(&last))goto done;
 /* Descriptor image is established by trusted pre-exec close_range + CLOEXEC6;
  * check stdio/image really absent here, before ANY file creation may reuse0..2. */
 for(int fd=0;fd<=6;fd++){
  if(fd>=3&&fd<=5){int f=fcntl(fd,F_GETFD);if(f<0||fcntl(fd,F_SETFD,f|FD_CLOEXEC))goto done;}
  else {errno=0;if(fcntl(fd,F_GETFD)!=-1||errno!=EBADF)goto done;}
 }
 if(roles(parent,0)||request(last)||pe_tick(&last,public.binding.deadline)||
    roles(parent,1)||pe_security_verify()||
    qc07f_rtsp_private_provisioner_harden_v1()!=CAMERA_OK||
    prctl(PR_GET_DUMPABLE)!=0||pe_security_verify()||getppid()!=parent||
    pe_tick(&last,public.binding.deadline))goto done;
 /* FIRST credential byte is below all real post-exec/public/role/hardener checks. */
 if(read_part(ingress,secret.header,sizeof(secret.header),0,&last))goto done;
 uint32_t version=pe_get32(secret.header),u=pe_get32(secret.header+4),r=pe_get32(secret.header+8),p=pe_get32(secret.header+12);
 if(version!=1||!u||u>CAMERA_DIGEST_NAME_MAX||!r||r>CAMERA_DIGEST_NAME_MAX||!p||p>CAMERA_DIGEST_PASSWORD_MAX)goto done;
 if(read_part(ingress,secret.payload,(size_t)u+r+p,0,&last)||
    read_part(ingress,&secret.extra,1,1,&last)||pe_close_once(&ingress)||
    pe_tick(&last,public.binding.deadline))goto done;
 int created=rtsp_private_handoff_create(&handoff,directory,&public.config,&public.routes,
  (struct camera_digest_span){secret.payload,u},(struct camera_digest_span){secret.payload+u,r},
  (struct camera_digest_span){secret.payload+u+r,p},&secret.material);
 /* Constructor password is borrowed: clear the ACTUAL owned input on return,
  * even though helper already clears material scratch on every return. */
 mbedtls_platform_zeroize(secret.payload,sizeof(secret.payload));
 mbedtls_platform_zeroize(secret.header,sizeof(secret.header));
 if(created!=CAMERA_OK||pe_tick(&last,public.binding.deadline)||
    pe_encode_reply(reply,&public.config,&public.routes,(uint64_t)handoff.file_inode.st_dev,(uint64_t)handoff.file_inode.st_ino)||
    rtsp_private_handoff_move(&handoff,&public.config,&local_unused)!=CAMERA_OK||
    pe_tick(&last,public.binding.deadline)||getppid()!=parent||send_material(public.config.material_fd))goto done;
 if(pe_close_once(&public.config.material_fd)||pe_close_once(&control)||pe_close_once(&directory)||
    pe_tick(&last,public.binding.deadline))goto done;
 result=0;
done:
 /* Controlled failure: owned scratch wipe, nonzero exit. No unlink/repair/close
  * retries; remaining refs (including all unexpected rights) retire at _exit.
  * No guarantee of wipe on SIGKILL/crash or while fsync is blocked. */
 mbedtls_platform_zeroize(&secret,sizeof(secret));
 _exit(result);
}
