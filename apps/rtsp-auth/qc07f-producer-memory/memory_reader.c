/* SPDX-License-Identifier: GPL-2.0-only
 * Fixed public proc metadata only. Separate TU makes HOST synthetic replacement
 * complete: tests cannot accidentally inline through the reader into host proc.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "memory_runtime.h"
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/statfs.h>
#include <unistd.h>
int nh_memory_read_fixed(unsigned kind,char *buffer,size_t cap,size_t *bytes)
{
 if(kind>1||!buffer||!bytes||cap!=(kind?NH_ZONEINFO_CAP:NH_MEMINFO_CAP))return -1;
 const char *path=kind?"/proc/zoneinfo":"/proc/meminfo";
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return -1;
 struct statfs fs;size_t used=0;int result=-1;unsigned reads=0;
 if(fstatfs(fd,&fs)||fs.f_type!=PROC_SUPER_MAGIC)goto done;
 while(used<cap&&reads++<32){ssize_t n=read(fd,buffer+used,cap-used);if(n<0)goto done;if(!n){*bytes=used;result=0;goto done;}used+=(size_t)n;}
 if(used<cap)goto done;
 {char extra;ssize_t n=read(fd,&extra,1);if(n==0){*bytes=used;result=0;}}
done:
 if(close(fd))result=-1;return result;
}
