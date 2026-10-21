/* SPDX-License-Identifier: GPL-2.0-only */
/* HOST TEST link wrappers ONLY. NEVER production or ARM objects. */
#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
ssize_t __real_getrandom(void *,size_t,unsigned int);
int __real_clock_gettime(clockid_t,struct timespec *);
ssize_t __wrap_getrandom(void *p,size_t n,unsigned int flags)
{
#if WEB_FAULT_KIND == 1
 (void)p;(void)n;(void)flags;errno=EAGAIN;return -1;
#elif WEB_FAULT_KIND == 2
 static unsigned call;(void)flags;
 if(call++==0&&n){((unsigned char *)p)[0]=0;return 1;}
 errno=EAGAIN;return -1;
#else
 return __real_getrandom(p,n,flags);
#endif
}
int __wrap_clock_gettime(clockid_t clock,struct timespec *t)
{
#if WEB_FAULT_KIND == 3
 (void)clock;(void)t;errno=EINVAL;return -1;
#elif WEB_FAULT_KIND == 4
 (void)clock;t->tv_sec=(time_t)(UINT64_MAX/1000000000ULL-10);t->tv_nsec=0;return 0;
#else
 return __real_clock_gettime(clock,t);
#endif
}
