/* SPDX-License-Identifier: GPL-2.0-only
 *HOST own-FD observation only: byte COUNT, not data/hash/FD identity receipts.
 *Delegates real pread; not a child/filter/privilege mock.*/
#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
static int material=-1;static size_t calls,bytes,largest;
void is_watch_material(int fd){material=fd;}
ssize_t __real_pread(int,void *,size_t,off_t);
ssize_t __wrap_pread(int fd,void *p,size_t n,off_t offset)
{if(fd==material){calls++;bytes+=n;if(n>largest)largest=n;}return __real_pread(fd,p,n,offset);}
size_t is_material_reads(void){return calls;}
size_t is_material_max_read(void){return largest;}
