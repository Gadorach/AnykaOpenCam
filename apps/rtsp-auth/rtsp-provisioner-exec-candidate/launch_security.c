/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "launch_security.h"
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <linux/securebits.h>
#include <mbedtls/sha256.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
_Static_assert(sizeof(dev_t)<=8&&sizeof(ino_t)<=8,"public inode widths");
_Static_assert(CAP_LAST_CAP<63,"reviewed capability v3 two-word policy");
#define PE_SECURE (SECBIT_NOROOT|SECBIT_NOROOT_LOCKED|SECBIT_NO_SETUID_FIXUP| \
 SECBIT_NO_SETUID_FIXUP_LOCKED|SECBIT_KEEP_CAPS_LOCKED| \
 SECBIT_NO_CAP_AMBIENT_RAISE|SECBIT_NO_CAP_AMBIENT_RAISE_LOCKED)
#define PE_SECURE_KNOWN (PE_SECURE|SECBIT_KEEP_CAPS)
int pe_root_ids(void)
{
 uid_t r,e,s;gid_t gr,ge,gs;
 return !getresuid(&r,&e,&s)&&!getresgid(&gr,&ge,&gs)&&!r&&!e&&!s&&!gr&&!ge&&!gs&&getgroups(0,NULL)==0;
}
int pe_clock(uint64_t *out)
{
 struct timespec t;
 if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L||
    (uint64_t)t.tv_sec>(UINT64_MAX-1000)/1000)return -1;
 *out=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;return 0;
}
int pe_tick(uint64_t *last,uint64_t deadline)
{uint64_t now;if(pe_clock(&now)||now<*last||now>=deadline)return -1;*last=now;return 0;}
int pe_same_inode(const struct stat *a,const struct stat *b)
{return a->st_dev==b->st_dev&&a->st_ino==b->st_ino;}
int pe_close_once(int *slot)
{int fd=*slot;*slot=-1;return close(fd);}
static int flags(int fd,int access,int nonblock)
{
 int f=fcntl(fd,F_GETFL),d=fcntl(fd,F_GETFD);
 return f>=0&&!(f&O_PATH)&&(f&O_ACCMODE)==access&&(!nonblock||(f&O_NONBLOCK))&&d>=0&&(d&FD_CLOEXEC);
}
int pe_directory(int fd,struct stat *st)
{return fd>=0&&flags(fd,O_RDONLY,0)&&!fstat(fd,st)&&S_ISDIR(st->st_mode)&&!st->st_uid&&
 (st->st_mode&07777)==0700&&st->st_nlink?0:-1;}
int pe_ingress(int fd,struct stat *st)
{return fd>=0&&flags(fd,O_RDONLY,1)&&!fstat(fd,st)&&S_ISFIFO(st->st_mode)&&!st->st_uid&&
 (st->st_mode&07777)==0600?0:-1;}
int pe_control(int fd,pid_t creator,struct stat *st)
{
 struct ucred peer;struct sockaddr_storage address;socklen_t n=sizeof(peer);int type;
 if(fd<0||!flags(fd,O_RDWR,1)||fstat(fd,st)||!S_ISSOCK(st->st_mode)||
    getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&peer,&n)||n!=sizeof(peer)||
    peer.pid!=creator||peer.uid||peer.gid)return -1;
 n=sizeof(type);if(getsockopt(fd,SOL_SOCKET,SO_TYPE,&type,&n)||n!=sizeof(type)||type!=SOCK_SEQPACKET)return -1;
 n=sizeof(address);return getsockname(fd,(struct sockaddr *)&address,&n)||address.ss_family!=AF_UNIX?-1:0;
}
static int metadata_same(const struct stat *a,const struct stat *b)
{
 return pe_same_inode(a,b)&&a->st_mode==b->st_mode&&a->st_uid==b->st_uid&&a->st_gid==b->st_gid&&
  a->st_size==b->st_size&&a->st_nlink==b->st_nlink&&
  a->st_mtim.tv_sec==b->st_mtim.tv_sec&&a->st_mtim.tv_nsec==b->st_mtim.tv_nsec&&
  a->st_ctim.tv_sec==b->st_ctim.tv_sec&&a->st_ctim.tv_nsec==b->st_ctim.tv_nsec;
}
static int image_stat(const struct pe_worker_image *im,struct stat *st)
{
 return im&&im->fd>=0&&im->bytes&&im->bytes<=PE_MEMORY&&flags(im->fd,O_RDONLY,0)&&
  !fstat(im->fd,st)&&S_ISREG(st->st_mode)&&!st->st_uid&&st->st_nlink==1&&
  st->st_size==(off_t)im->bytes&&!(st->st_mode&07222)&&(st->st_mode&0500)==0500?0:-1;
}
static int elf(const struct pe_worker_image *im)
{
 Elf32_Ehdr h;Elf32_Phdr loads[32];unsigned count=0,stack=0,entry=0;uint64_t memory=0;
 if(pread(im->fd,&h,sizeof(h),0)!=(ssize_t)sizeof(h)||memcmp(h.e_ident,ELFMAG,SELFMAG)||
    h.e_ident[EI_CLASS]!=ELFCLASS32||h.e_ident[EI_DATA]!=ELFDATA2LSB||h.e_ident[EI_VERSION]!=EV_CURRENT||
    h.e_machine!=EM_ARM||h.e_type!=ET_EXEC||h.e_version!=EV_CURRENT||h.e_ehsize!=sizeof(h)||
    (h.e_flags&EF_ARM_EABIMASK)!=EF_ARM_EABI_VER5||!(h.e_flags&EF_ARM_ABI_FLOAT_SOFT)||
    (h.e_flags&EF_ARM_ABI_FLOAT_HARD)||!h.e_phnum||h.e_phnum>32||h.e_phentsize!=sizeof(Elf32_Phdr)||
    (uint64_t)h.e_phoff+(uint64_t)h.e_phnum*sizeof(Elf32_Phdr)>im->bytes||(h.e_entry&3))return -1;
 for(unsigned i=0;i<h.e_phnum;i++){
  Elf32_Phdr p;
  if(pread(im->fd,&p,sizeof(p),(off_t)h.e_phoff+(off_t)i*sizeof(p))!=(ssize_t)sizeof(p)||
     p.p_type==PT_INTERP||p.p_type==PT_DYNAMIC||(uint64_t)p.p_offset+p.p_filesz>im->bytes)return -1;
  if(p.p_type==PT_GNU_STACK){if(p.p_flags&PF_X||++stack!=1)return -1;}
  if(p.p_type!=PT_LOAD)continue;
  if(!p.p_memsz||p.p_filesz>p.p_memsz||(uint64_t)p.p_vaddr+p.p_memsz>UINT32_MAX||
     (p.p_flags&~(PF_R|PF_W|PF_X))||((p.p_flags&(PF_W|PF_X))==(PF_W|PF_X))||
     (p.p_align>1&&((p.p_align&(p.p_align-1))||p.p_vaddr%p.p_align!=p.p_offset%p.p_align)))return -1;
  for(unsigned j=0;j<count;j++)if((uint64_t)p.p_vaddr<(uint64_t)loads[j].p_vaddr+loads[j].p_memsz&&
     (uint64_t)loads[j].p_vaddr<(uint64_t)p.p_vaddr+p.p_memsz)return -1;
  loads[count++]=p;memory+=p.p_memsz;if(memory>PE_MEMORY)return -1;
  if((p.p_flags&(PF_R|PF_X))==(PF_R|PF_X)&&h.e_entry>=p.p_vaddr&&
     (uint64_t)h.e_entry<(uint64_t)p.p_vaddr+p.p_filesz)entry++;
 }
 return count&&stack==1&&entry==1?0:-1;
}
int pe_image_unchanged(const struct pe_worker_image *im,const struct stat *before)
{struct stat now;return image_stat(im,&now)||!metadata_same(before,&now)?-1:0;}
int pe_image_validate(const struct pe_worker_image *im,struct stat *before)
{
 unsigned char bytes[4096],digest[32];mbedtls_sha256_context hash;size_t used=0;int result=-1;
 if(image_stat(im,before)||elf(im))return -1;
 mbedtls_sha256_init(&hash);
 if(mbedtls_sha256_starts(&hash,0))goto done;
 while(used<im->bytes){size_t n=im->bytes-used;if(n>sizeof(bytes))n=sizeof(bytes);
  if(pread(im->fd,bytes,n,(off_t)used)!=(ssize_t)n||mbedtls_sha256_update(&hash,bytes,n))goto done;
  used+=n;
 }
 if(!mbedtls_sha256_finish(&hash,digest)&&!memcmp(digest,im->sha256,32)&&!pe_image_unchanged(im,before))result=0;
done:
 /* All bytes/hash here are PUBLIC executable data, never material/credentials. */
 mbedtls_sha256_free(&hash);return result;
}
int pe_receive(int fd,unsigned char *out,size_t capacity,ssize_t *bytes,
 int owned[PE_RIGHTS_MAX],unsigned *count,unsigned *bad)
{
 union {struct cmsghdr align;unsigned char bytes[CMSG_SPACE(PE_RIGHTS_MAX*sizeof(int))];} control;
 memset(&control,0,sizeof(control));struct iovec iov={out,capacity};
 struct msghdr m={.msg_iov=&iov,.msg_iovlen=1,.msg_control=control.bytes,.msg_controllen=sizeof(control.bytes)};
 ssize_t n=recvmsg(fd,&m,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
 if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))return 1;
 if(n<0)return -1;
 *bytes=n;*bad=0;unsigned controls=0;
 /* Linux constructs ancillary headers. Total installed integer rights cannot
  * exceed this buffer's PE_RIGHTS_MAX. Truncated undelivered rights stay kernel
  * owned; every delivered number is represented before packet refusal. */
 for(struct cmsghdr *c=CMSG_FIRSTHDR(&m);c;c=CMSG_NXTHDR(&m,c)){
  controls++;
  if(c->cmsg_len<CMSG_LEN(0)||c->cmsg_level!=SOL_SOCKET||c->cmsg_type!=SCM_RIGHTS){*bad=1;continue;}
  size_t b=c->cmsg_len-CMSG_LEN(0);if(b%sizeof(int))*bad=1;
  for(size_t i=0;i<b/sizeof(int);i++){
   int received;memcpy(&received,CMSG_DATA(c)+i*sizeof(int),sizeof(received));
   owned[(*count)++]=received;
  }
 }
 if(m.msg_flags&(MSG_TRUNC|MSG_CTRUNC))*bad=1;
 /* 0 rights is valid request, exactly1 valid reply; callers select that rule. */
 if(controls!=(*count?1U:0U))*bad=1;
 return 0;
}
static int caps_supported(void)
{
 /* Compile-time reviewed kernel header bound, not dynamic probing/extension.
  * Kernel with newer capabilities is unsupported; never silently retain them. */
 errno=0;
 if(prctl(PR_CAPBSET_READ,CAP_LAST_CAP+1,0,0,0)!=-1||errno!=EINVAL)return -1;
 return 0;
}
static int limits_verify(void)
{
 struct rlimit core,lock,as;
 return getrlimit(RLIMIT_CORE,&core)||core.rlim_cur||core.rlim_max||
  getrlimit(RLIMIT_MEMLOCK,&lock)||lock.rlim_cur!=PE_MEMORY||lock.rlim_max!=PE_MEMORY||
  getrlimit(RLIMIT_AS,&as)||as.rlim_cur!=PE_MEMORY||as.rlim_max!=PE_MEMORY?-1:0;
}
int pe_security_verify(void)
{
 struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct c[2]={{0}};
 if(!pe_root_ids()||caps_supported()||limits_verify()||prctl(PR_GET_SECCOMP,0,0,0,0)!=0||
    prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)!=1||prctl(PR_GET_SECUREBITS,0,0,0,0)!=PE_SECURE||
    syscall(SYS_capget,&h,c)||h.version!=_LINUX_CAPABILITY_VERSION_3)return -1;
 for(unsigned i=0;i<2;i++)if(c[i].effective||c[i].permitted||c[i].inheritable)return -1;
 for(unsigned i=0;i<=CAP_LAST_CAP;i++)if(prctl(PR_CAPBSET_READ,i,0,0,0)!=0||
    prctl(PR_CAP_AMBIENT,PR_CAP_AMBIENT_IS_SET,i,0,0)!=0)return -1;
 return 0;
}
static int security_prepare(void)
{
 struct rlimit core={0,0},mem={PE_MEMORY,PE_MEMORY},old;
 struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct c[2]={{0}};
 int bits=prctl(PR_GET_SECUREBITS,0,0,0,0);
 if(!pe_root_ids()||prctl(PR_GET_SECCOMP,0,0,0,0)!=0||caps_supported()||bits<0||
    (bits&~PE_SECURE_KNOWN)||((bits&SECBIT_KEEP_CAPS_LOCKED)&&(bits&SECBIT_KEEP_CAPS))||
    getrlimit(RLIMIT_MEMLOCK,&old)||old.rlim_cur<PE_MEMORY||old.rlim_max<PE_MEMORY||
    setrlimit(RLIMIT_MEMLOCK,&mem)||getrlimit(RLIMIT_AS,&old)||
    old.rlim_cur<PE_MEMORY||old.rlim_max<PE_MEMORY||setrlimit(RLIMIT_AS,&mem)||
    setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||
    prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||
    prctl(PR_CAP_AMBIENT,PR_CAP_AMBIENT_CLEAR_ALL,0,0,0)||
    prctl(PR_SET_SECUREBITS,(bits|PE_SECURE)&~SECBIT_KEEP_CAPS,0,0,0))return -1;
 for(unsigned i=0;i<=CAP_LAST_CAP;i++)if(prctl(PR_CAPBSET_DROP,i,0,0,0))return -1;
 if(syscall(SYS_capset,&h,c)||prctl(PR_GET_DUMPABLE)!=0)return -1;
 return pe_security_verify();
}
_Noreturn void pe_child_exec(const int stage[4],pid_t parent)
{
 char *argv[]={PE_ARGV0,NULL};char *environment[]={NULL};
 /* Root class retained solely for actual helper/DAC semantics, never caps. */
 if(getppid()!=parent||security_prepare())_exit(70);
 for(unsigned i=0;i<4;i++)if(dup2(stage[i],(int)i+3)!=(int)i+3||
    fcntl((int)i+3,F_SETFD,i==3?FD_CLOEXEC:0))_exit(71);
 if(syscall(SYS_close_range,7U,~0U,0U))_exit(72);
 for(int i=0;i<3;i++){
  int f=fcntl(i,F_GETFD);
  if(f<0){if(errno!=EBADF)_exit(73);}else if(close(i))_exit(73);
 }
 if(getppid()!=parent||pe_security_verify())_exit(74);
 syscall(SYS_execveat,6,"",argv,environment,AT_EMPTY_PATH);
 _exit(75); /* ONE exec attempt, never parent logic or interpreter fallback. */
}
