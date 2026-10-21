/* SPDX-License-Identifier: GPL-2.0-only
 * Minimal absolute-path/no-symlink walk adapted from canonical launch.c.
 * Only trusted qualification scope dirs/fixed names; no network/path selectors. */
#define _GNU_SOURCE
#include <stddef.h>
#include "paths.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
int nq_open_dir(const char *path,uid_t owner,mode_t final_mode)
{
 char part[256];const char *p;int fd,next;size_t n;struct stat st;unsigned count=0;
 if(!path||path[0]!='/'||strlen(path)>1800)return -1;
 fd=open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(fd<0)return -1;p=path+1;
 while(*p){
  if(++count>32)goto bad;
  n=strcspn(p,"/");if(!n||n>=sizeof(part))goto bad;
  memcpy(part,p,n);part[n]=0;if(!strcmp(part,".")||!strcmp(part,".."))goto bad;
  next=openat(fd,part,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);if(next<0)goto bad;close(fd);fd=next;
  if(fstat(fd,&st)||!S_ISDIR(st.st_mode)||(st.st_uid!=owner&&st.st_uid!=0))goto bad;
  if(*(p+n)){
   if((st.st_mode&0022)&&!((st.st_mode&S_ISVTX)&&st.st_uid==0))goto bad;
   p+=n+1;if(!*p)goto bad;
  }else {if(st.st_uid!=owner||(st.st_mode&07777)!=final_mode)goto bad;return fd;}
 }
 bad:close(fd);return -1;
}
int nq_open_file(int directory,const char *name,uid_t owner,mode_t mode,size_t bytes)
{
 struct stat prior,st;int fd;if(!name||!name[0]||strchr(name,'/')||!strcmp(name,".")||!strcmp(name,".."))return -1;
 /*Reject special inodes without opening them. O_NONBLOCK also closes the FIFO
  * replacement window before post-open inode/type validation. Scope directories
  * and writers must remain trusted/quiesced, as for rv_start's readonly FD.*/
 if(fstatat(directory,name,&prior,AT_SYMLINK_NOFOLLOW)||!S_ISREG(prior.st_mode)||prior.st_nlink!=1||prior.st_uid!=owner||
    (prior.st_mode&07777)!=mode||prior.st_size!=(off_t)bytes)return -1;
 fd=openat(directory,name,O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return -1;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_uid!=owner||(st.st_mode&07777)!=mode||
    st.st_dev!=prior.st_dev||st.st_ino!=prior.st_ino||st.st_size!=(off_t)bytes||lseek(fd,0,SEEK_CUR)!=0||
    (fcntl(fd,F_GETFL)&O_ACCMODE)!=O_RDONLY){close(fd);return -1;}
 return fd;
}
