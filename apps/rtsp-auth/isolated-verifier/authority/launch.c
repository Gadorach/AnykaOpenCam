/* SPDX-License-Identifier: GPL-2.0-only */
/* Fixed reviewed-image launch, private-FD delegation, one finite owned service.
 * No camera/driver/recovery protocol, restart, client-selected PID or secrets argv. */
#define _GNU_SOURCE
#include "web_service.h"
#include "image_pins.h"
#include <mbedtls/sha256.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <elf.h>
#include <fcntl.h>
#include <grp.h>
#include <linux/capability.h>
#include <linux/magic.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define MIB (1024U*1024U)
#ifndef LAUNCH_MEMLOCK
#define LAUNCH_MEMLOCK (2U*MIB)
#endif
static volatile sig_atomic_t stop;
static volatile sig_atomic_t revoke_requested;
static void stop_signal(int n){if(n==SIGUSR1)revoke_requested=1;else stop=1;}
static void wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
static uid_t launch_uid;
static gid_t launch_gid;
static int parent_core(void)
{
 struct rlimit core={0,0};
 return setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||
        prctl(PR_SET_CHILD_SUBREAPER,1,0,0,0);
}
/* Walk absolute paths without symlinks or .. . Root-owned sticky /tmp is allowed
 * as an ancestor, never as the private final directory. All path names are
 * trusted bootstrap launch configuration, never HTTP parameters. */
static int open_absolute(const char *path,int flags)
{
 char part[256];const char *p;int fd,next;size_t n;struct stat st;
 if(!path||path[0]!='/'||strlen(path)>1800)return -1;
 fd=open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(fd<0)return -1;
 p=path+1;
 while(*p){
  n=strcspn(p,"/");if(!n||n>=sizeof(part))goto bad;
  memcpy(part,p,n);part[n]=0;
  if(!strcmp(part,".")||!strcmp(part,".."))goto bad;
  next=openat(fd,part,(* (p+n)?O_RDONLY|O_DIRECTORY:flags)|O_NOFOLLOW|O_CLOEXEC);
  if(next<0)goto bad;
  close(fd);fd=next;
  if(*(p+n)){
   if(fstat(fd,&st)||!S_ISDIR(st.st_mode)||
      (st.st_uid!=geteuid()&&st.st_uid!=0)||
      ((st.st_mode&0022)&&!((st.st_mode&S_ISVTX)&&st.st_uid==0)))goto bad;
   p+=n+1;if(!*p)goto bad;
  }else break;
 }
 return fd;
bad:close(fd);return -1;
}
static int read_at(int fd,void *p,size_t bytes,off_t offset)
{
 size_t done=0;unsigned attempts=0;
 while(done<bytes&&attempts++<64){
  ssize_t n=pread(fd,(char *)p+done,bytes-done,offset+(off_t)done);
  if(n<0&&errno==EINTR)continue;
  if(n<=0)return -1;
  done+=(size_t)n;
 }
 return done==bytes?0:-1;
}
static int elf_valid(int fd,size_t size)
{
 unsigned char ident[EI_NIDENT];
 if(read_at(fd,ident,sizeof(ident),0)||memcmp(ident,ELFMAG,SELFMAG)||
    ident[EI_DATA]!=ELFDATA2LSB||ident[EI_VERSION]!=EV_CURRENT)return -1;
#ifdef LAUNCH_HOST_PUBLIC_ONLY
 {Elf64_Ehdr h;
  if(ident[EI_CLASS]!=ELFCLASS64||read_at(fd,&h,sizeof(h),0)||h.e_machine!=EM_X86_64||
     (h.e_type!=ET_EXEC&&h.e_type!=ET_DYN)||h.e_version!=EV_CURRENT||h.e_ehsize!=sizeof(h))return -1;
 }
#else
 {Elf32_Ehdr h;unsigned i;
  if(ident[EI_CLASS]!=ELFCLASS32||read_at(fd,&h,sizeof(h),0)||h.e_machine!=EM_ARM||
     h.e_type!=ET_EXEC||h.e_version!=EV_CURRENT||h.e_ehsize!=sizeof(h)||
     (h.e_flags&EF_ARM_EABIMASK)!=EF_ARM_EABI_VER5||(h.e_flags&EF_ARM_ABI_FLOAT_HARD)||
     h.e_phnum>32||h.e_phentsize!=sizeof(Elf32_Phdr)||
     (uint64_t)h.e_phoff+(uint64_t)h.e_phnum*sizeof(Elf32_Phdr)>size)return -1;
  for(i=0;i<h.e_phnum;i++){
   Elf32_Phdr p;
   if(read_at(fd,&p,sizeof(p),h.e_phoff+(off_t)i*sizeof(p))||p.p_type==PT_INTERP||
      p.p_type==PT_DYNAMIC||(p.p_type==PT_GNU_STACK&&(p.p_flags&PF_X)))return -1;
  }
 }
#endif
 (void)size;return 0;
}
static int hash_fd(int fd,size_t expected,const unsigned char pin[32])
{
 mbedtls_sha256_context hash;unsigned char bytes[4096],digest[32];size_t done=0;int result=-1;
 mbedtls_sha256_init(&hash);
 if(mbedtls_sha256_starts(&hash,0))goto out;
 while(done<expected){
  size_t n=expected-done;if(n>sizeof(bytes))n=sizeof(bytes);
  if(read_at(fd,bytes,n,(off_t)done)||mbedtls_sha256_update(&hash,bytes,n))goto out;
  done+=n;
 }
 if(!mbedtls_sha256_finish(&hash,digest)&&!mbedtls_ct_memcmp(digest,pin,32))result=0;
out:wipe(bytes,sizeof(bytes));wipe(digest,sizeof(digest));mbedtls_sha256_free(&hash);return result;
}
static int image_source(const char *path,size_t bytes,const unsigned char pin[32])
{
 struct stat st;int fd=open_absolute(path,O_RDONLY);
 if(fd<0)return -1;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_size!=(off_t)bytes||
    (st.st_uid!=geteuid()&&st.st_uid!=0)||(st.st_mode&0022)||
    elf_valid(fd,bytes)||hash_fd(fd,bytes,pin)){close(fd);return -1;}
 return fd;
}
static int copy_image(int source,int dir,const char *name,size_t bytes,const unsigned char pin[32])
{
 int fd=-1,result=-1;size_t done=0;unsigned char buffer[4096];
 fd=openat(dir,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return -1;
 while(done<bytes){
  size_t n=bytes-done,put=0;unsigned attempts=0;if(n>sizeof(buffer))n=sizeof(buffer);
  if(read_at(source,buffer,n,(off_t)done))goto out;
  while(put<n&&attempts++<64){
   ssize_t written=write(fd,buffer+put,n-put);
   if(written<0&&errno==EINTR)continue;
   if(written<=0)goto out;
   put+=(size_t)written;
  }
  if(put!=n)goto out;
  done+=n;
 }
 /* Scope-local root-owned public read/execute copy solves root0500 source
  * executability WITHOUT modifying source/global permissions or creating users. */
 if(fsync(fd)||fchmod(fd,0555))goto out;
 close(fd);fd=openat(dir,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
 if(fd<0||hash_fd(fd,bytes,pin)||elf_valid(fd,bytes))goto out;
 result=fd;fd=-1;
out:if(fd>=0)close(fd);wipe(buffer,sizeof(buffer));return result;
}
static int input_fd(int dir,const char *name,size_t max,size_t exact)
{
 struct stat st;int fd=openat(dir,name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
 if(fd<0)return -1;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_uid!=geteuid()||
    (st.st_mode&07777)!=0600||st.st_size<=0||(uint64_t)st.st_size>max||
    (exact&&st.st_size!=(off_t)exact)||lseek(fd,0,SEEK_CUR)!=0||
    (fcntl(fd,F_GETFL)&O_ACCMODE)!=O_RDONLY){close(fd);return -1;}
 return fd;
}
static int timer(unsigned seconds)
{
 int fd;struct itimerspec value={0};value.it_value.tv_sec=seconds;
 fd=timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);
 if(fd<0)return -1;
 if(timerfd_settime(fd,0,&value,NULL)){close(fd);return -1;}return fd;
}
static int zero_caps(int retain_kill)
{
 struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct d[2]={{0}};
 if(retain_kill){d[0].effective=d[0].permitted=1U<<CAP_KILL;}
 if(syscall(SYS_capset,&h,d)||syscall(SYS_capget,&h,d))return -1;
 return d[0].effective!=(retain_kill?1U<<CAP_KILL:0)||
        d[0].permitted!=(retain_kill?1U<<CAP_KILL:0)||d[0].inheritable||
        d[1].effective||d[1].permitted||d[1].inheritable;
}
static int setup_caps(void)
{
 struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct d[2]={{0}};
 if(geteuid()!=0)return zero_caps(0);
 d[0].effective=d[0].permitted=(1U<<CAP_SETUID)|(1U<<CAP_SETGID)|(1U<<CAP_KILL);
 return syscall(SYS_capset,&h,d);
}
/* All child source FDs are staged >=32 so fixed destinations cannot alias. */
static void exec_service(int images,int input[4],int owner,int log,int gate,
 const char *child_path,pid_t parent)
{
 struct rlimit core={0,0},mem={LAUNCH_MEMLOCK,LAUNCH_MEMLOCK};
 unsigned char go;char *argv[]={"auth-web-service",(char *)child_path,NULL};char *env[]={NULL};
 if(read(gate,&go,1)!=1||go!=1)_exit(70);
#ifndef LAUNCH_HOST_PUBLIC_ONLY
 if(setgroups(0,NULL)||setresgid(launch_gid,launch_gid,launch_gid)||
    setresuid(launch_uid,launch_uid,launch_uid))_exit(71);
#endif
 if(getuid()!=launch_uid||geteuid()!=launch_uid||getgid()!=launch_gid||getegid()!=launch_gid||
    !launch_uid||setrlimit(RLIMIT_CORE,&core)||setrlimit(RLIMIT_MEMLOCK,&mem)||
    zero_caps(0)||prctl(PR_SET_DUMPABLE,0)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||
    prctl(PR_SET_PDEATHSIG,SIGKILL)||getppid()!=parent)_exit(72);
 if(dup2(log,1)<0||dup2(log,2)<0)_exit(73);
 for(unsigned i=0;i<4;i++)if(dup2(input[i],(int)i+3)<0)_exit(73);
 if(dup2(owner,7)<0||dup2(images,8)<0||fcntl(8,F_SETFD,FD_CLOEXEC)||
    syscall(SYS_close_range,9U,~0U,0))_exit(73);
 close(0);
 /* Execution checks permissions after UID drop and uses verified opened inode. */
 syscall(SYS_execveat,8,"",argv,env,AT_EMPTY_PATH);_exit(74);
}
static int high_fd(int fd)
{int high=fcntl(fd,F_DUPFD_CLOEXEC,32);close(fd);return high;}
static int send_signal(int fd,int sig)
{return syscall(SYS_pidfd_send_signal,fd,sig,NULL,0)&&errno!=ESRCH?-1:0;}
static int collect_line(const char *line,int *ready,int *child_retired)
{
 if(!strcmp(line,"web_ready=1 https_only=1 strict_child=1 product_accepted=0")){
  *ready=1;puts("service_ready=1 source=pinned_service strict_child_ready_reported=1");fflush(stdout);
 }else if(!strcmp(line,"child_retirement=observed"))*child_retired=1;
 else if(!strcmp(line,"child_retirement=uncertain"))return -1;
 /* No forwarding of passwords, hashes, cookies, CSRF, account/match values,
  * TLS errors or arbitrary child output. Unrecognized lines are discarded. */
 return 0;
}
int main(int argc,char **argv)
{
 int scope=-1,code=-1,sources[2]={-1,-1},image=-1,input[4]={-1,-1,-1,-1};
 int pair[2]={-1,-1},logs[2]={-1,-1},gate[2]={-1,-1},pidfd=-1,limit=-1,grace=-1;
 int ready=0,child_retired=0,status=0,exit_seen=0,result=20,term_sent=0,revoke_sent=0,revoke_ack=0;
 pid_t child=0;struct stat st;struct statfs fs;struct web_config cfg={0};
 struct rusage usage={0};struct sigaction action={0};char child_path[2000],line[384];size_t used=0;
 unsigned lines=0;pid_t own=getpid();
 if(argc!=4||getuid()!=geteuid()||getgid()!=getegid())return 20;
#ifdef LAUNCH_HOST_PUBLIC_ONLY
 launch_uid=getuid();launch_gid=getgid();if(!launch_uid)return 20;
 puts("bootstrap_class=PUBLICDUMMY_HOST_ONLY root_UID_groups_proof=0");
#else
 if(getuid()!=0)return 20;
 launch_uid=1000;launch_gid=1000;
 puts("bootstrap_class=ROOT_RAM_QUALIFICATION nonroot_UID=1000 nonroot_GID=1000");
#endif
 action.sa_handler=stop_signal;sigemptyset(&action.sa_mask);
 if(sigaction(SIGTERM,&action,NULL)||sigaction(SIGINT,&action,NULL)||sigaction(SIGUSR1,&action,NULL)||parent_core())return 20;
 {struct sigaction normal={0};normal.sa_handler=SIG_DFL;sigemptyset(&normal.sa_mask);
  if(sigaction(SIGCHLD,&normal,NULL))return 20;}
 signal(SIGPIPE,SIG_IGN);umask(077);
 scope=open_absolute(argv[1],O_RDONLY|O_DIRECTORY);
 if(scope<0||fstat(scope,&st)||!S_ISDIR(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&07777)!=0700)goto end;
#ifndef LAUNCH_HOST_PUBLIC_ONLY
 if(fstatfs(scope,&fs)||fs.f_type!=TMPFS_MAGIC)goto end;
#else
 (void)fs;
#endif
 sources[0]=image_source(argv[2],SERVICE_BYTES,service_pin);
 sources[1]=image_source(argv[3],CHILD_BYTES,child_pin);
 if(sources[0]<0||sources[1]<0)goto end;
 input[0]=input_fd(scope,"cert.pem",8192,0);input[1]=input_fd(scope,"key.pem",4096,0);
 input[2]=input_fd(scope,"credentials.bin",112,112);input[3]=input_fd(scope,"config.bin",768,768);
 for(unsigned i=0;i<4;i++)if(input[i]<0)goto end;
 if(read_at(input[3],&cfg,sizeof(cfg),0)||cfg.magic!=WEB_CONFIG_MAGIC||cfg.version!=2||
    cfg.bytes!=768||cfg.trust_mode!=1||cfg.runtime_ns<120*WEB_NS||cfg.runtime_ns>180*WEB_NS||
    cfg.response_ns!=35*WEB_NS||!cfg.boot_generation||!cfg.child_generation||!cfg.credential_generation)goto end;
 /* Both reviewed immutable ELFs inherit this single limit; no secret-child
  * wrapper or post-READY prlimit race. Native uses conservative2MiB BOTH.
  * Host4MiB fixture is a separately compiled public-only profile, never auto-raise. */
 {struct rlimit mem={LAUNCH_MEMLOCK,LAUNCH_MEMLOCK};
  if(setrlimit(RLIMIT_MEMLOCK,&mem)||setup_caps()||mlockall(MCL_CURRENT|MCL_FUTURE))goto end;}
 if(mkdirat(scope,"code",0700))goto end;
 code=openat(scope,"code",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);if(code<0)goto end;
 image=copy_image(sources[0],code,"service",SERVICE_BYTES,service_pin);
 {int fd=copy_image(sources[1],code,"child",CHILD_BYTES,child_pin);if(fd<0)goto end;close(fd);}
 if(image<0||snprintf(child_path,sizeof(child_path),"%s/code/child",argv[1])<0)goto end;
 /* Private scope ancestor needs traversal by the service for its CHILD path,
  * but must NOT expose secret files. Move code into separate PUBLIC sibling
  * directory, leaving private scope0700. Rename within same trusted RAM parent. */
 {char public_path[1900];int parent=-1;char *slash;
  if(snprintf(public_path,sizeof(public_path),"%s.public-code",argv[1])<0)goto end;
  slash=strrchr(public_path,'/');if(!slash||slash==public_path)goto end;
  *slash=0;parent=open_absolute(public_path,O_RDONLY|O_DIRECTORY);*slash='/';
  if(parent<0)goto end;
  if(mkdirat(parent,slash+1,0700)){close(parent);goto end;}
  {int public_dir=openat(parent,slash+1,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
   close(parent);if(public_dir<0)goto end;
   if(renameat(code,"service",public_dir,"service")||renameat(code,"child",public_dir,"child")||
      fchmod(public_dir,0555)||fchmod(code,0555)){close(public_dir);goto end;}
   close(public_dir);
  }
  if(snprintf(child_path,sizeof(child_path),"%s/child",public_path)<0)goto end;
 }
 for(unsigned i=0;i<4;i++){input[i]=high_fd(input[i]);if(input[i]<0)goto end;}
 image=high_fd(image);if(image<0)goto end;
 if(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,0,pair)||
    pipe2(logs,O_CLOEXEC|O_NONBLOCK)||pipe2(gate,O_CLOEXEC))goto end;
 pair[1]=high_fd(pair[1]);logs[1]=high_fd(logs[1]);gate[0]=high_fd(gate[0]);
 if(pair[1]<0||logs[1]<0||gate[0]<0)goto end;
 limit=timer((unsigned)(cfg.runtime_ns/WEB_NS)+10);if(limit<0)goto end;
 child=fork();if(child<0)goto end;
 if(!child)exec_service(image,input,pair[1],logs[1],gate[0],child_path,own);
 close(pair[1]);pair[1]=-1;close(logs[1]);logs[1]=-1;close(gate[0]);gate[0]=-1;
 pidfd=(int)syscall(SYS_pidfd_open,child,0);
 if(pidfd<0){close(gate[1]);gate[1]=-1;goto supervise;}
 /* Own unreaped child cannot be recycled before pidfd acquisition. Drop all
  * bootstrap privilege except CAP_KILL needed for its differing-UID child;
  * no CAP_IPC_LOCK/SYS_RESOURCE or global debug policy retained/changed. */
 if(zero_caps(geteuid()==0)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)){
  close(gate[1]);gate[1]=-1;stop=1;goto supervise;
 }
 {unsigned char go=1;if(write(gate[1],&go,1)!=1)goto supervise;close(gate[1]);gate[1]=-1;}
 printf("bootstrap_launched=1 memlock_service_bytes=%u memlock_child_bytes=%u fd_contract=3_7 readonly_files=1\n",LAUNCH_MEMLOCK,LAUNCH_MEMLOCK);fflush(stdout);
supervise:
 for(unsigned attempts=0;attempts<30000&&!exit_seen;attempts++){
  struct pollfd events[4]={{logs[0],POLLIN,0},{pidfd,POLLIN,0},{term_sent?grace:limit,POLLIN,0},{pair[0],POLLIN,0}};
  char buffer[512];ssize_t n;pid_t got=wait4(child,&status,WNOHANG,&usage);
  if(got==child){exit_seen=1;break;}
  if(got<0&&errno!=EINTR)break;
  if(poll(events,4,10)<0&&errno!=EINTR)break;
  n=read(logs[0],buffer,sizeof(buffer));
  if(n>0)for(ssize_t i=0;i<n;i++){
   if(buffer[i]=='\n'){
    line[used]=0;if(lines++>256||collect_line(line,&ready,&child_retired))stop=1;used=0;
   }else if(used<sizeof(line)-1)line[used++]=buffer[i];else stop=1;
  }
  wipe(buffer,sizeof(buffer));
  /* One trusted-bootstrap signal, not a network/HTTP operation or PID API.
   * Sends only the existing nonsecret fixed-account owner revocation command. */
  if(revoke_requested&&ready&&!revoke_sent&&!term_sent){
   unsigned char command[16]={1,0,0,0};uint64_t generation=cfg.credential_generation+1;
   if(!generation){stop=1;}else{
    for(unsigned i=0;i<8;i++)command[8+i]=(unsigned char)(generation>>(8*i));
    if(send(pair[0],command,sizeof(command),MSG_DONTWAIT|MSG_NOSIGNAL)!=sizeof(command))stop=1;
    else{revoke_sent=1;puts("owner_revoke_sent=1");fflush(stdout);}
   }
  }
  if(revoke_sent&&!revoke_ack&&(events[3].revents&POLLIN)){
   unsigned char ack[9]={0};ssize_t size=recv(pair[0],ack,sizeof(ack),MSG_DONTWAIT|MSG_TRUNC);
   if(size!=8||ack[0]!=1||ack[1]||ack[2]||ack[3]||ack[4]||ack[5]||ack[6]||ack[7])stop=1;
   else{revoke_ack=1;puts("owner_revoke_ack=1");fflush(stdout);}
  }
  if(!term_sent&&(stop||events[2].revents||pidfd<0)){
   if(pidfd>=0)send_signal(pidfd,SIGTERM);
   term_sent=1;grace=timer(3);if(grace<0)break;
  }else if(term_sent&&events[2].revents){if(pidfd>=0)send_signal(pidfd,SIGKILL);break;}
 }
 if(!exit_seen){
  if(pidfd>=0)send_signal(pidfd,SIGKILL);
  for(unsigned i=0;i<200;i++){
   pid_t got=wait4(child,&status,WNOHANG,&usage);if(got==child){exit_seen=1;break;}
   struct pollfd event={pidfd,POLLIN,0};poll(&event,1,10);
  }
 }
 /* Drain finite remaining scalar output after exit; never forward raw bytes. */
 if(exit_seen)for(unsigned count=0;count<32;count++){
  char buffer[512];ssize_t n=read(logs[0],buffer,sizeof(buffer));if(n<=0)break;
  for(ssize_t i=0;i<n;i++)if(buffer[i]=='\n'){
   line[used]=0;collect_line(line,&ready,&child_retired);used=0;
  }else if(used<sizeof(line)-1)line[used++]=buffer[i];
  wipe(buffer,sizeof(buffer));
 }
 result=exit_seen&&ready&&child_retired&&WIFEXITED(status)&&WEXITSTATUS(status)==0?0:20;
 /* Subreaper is scoped to this bootstrap. Only its own service lineage can be
  * present; do not claim orphan CPU retirement from a parent death signal. */
 {int no_children=0,adopted=0;unsigned attempts;
  for(attempts=0;attempts<200;attempts++){
   int orphan_status;pid_t got=waitpid(-1,&orphan_status,WNOHANG);
   if(got>0){adopted++;continue;}
   if(got<0&&errno==ECHILD){no_children=1;break;}
   if(got<0&&errno!=EINTR)break;
   poll(NULL,0,10);
  }
  if(!no_children)result=22;
  printf("owned_lineage_no_remaining_children=%d adopted_exit_receipts=%d\n",no_children,adopted);
 }
 printf("service_wait_exit_observed=%d strict_child_retirement_reported=%d service_status=%d rss_peak_kb=%ld bootstrap_result=%d\n",
  exit_seen,child_retired,exit_seen&&WIFEXITED(status)?WEXITSTATUS(status):-1,exit_seen?usage.ru_maxrss:-1L,result);
end:
 for(unsigned i=0;i<4;i++)if(input[i]>=0)close(input[i]);
 for(unsigned i=0;i<2;i++){if(sources[i]>=0)close(sources[i]);if(pair[i]>=0)close(pair[i]);if(logs[i]>=0)close(logs[i]);if(gate[i]>=0)close(gate[i]);}
 if(image>=0)close(image);
 if(code>=0)close(code);
 if(scope>=0)close(scope);
 if(pidfd>=0)close(pidfd);
 if(limit>=0)close(limit);
 if(grace>=0)close(grace);
 wipe(line,sizeof(line));wipe(&cfg,sizeof(cfg));
 if(!child)puts("bootstrap_startup=denied no_service_exec=1");
 return result;
}
