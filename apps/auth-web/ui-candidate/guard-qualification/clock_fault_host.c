/* SPDX-License-Identifier: GPL-2.0-only */
/* HOST fault injector only, never linked into ARM qualification helper. */
#include <errno.h>
#include <time.h>
#ifndef HOST_CLOCK_FAULT
#error HOST fault kind required
#endif
int __wrap_clock_gettime(clockid_t clock,struct timespec *out) {
 static unsigned calls;
 (void)clock;++calls;
 if(HOST_CLOCK_FAULT==1){errno=EIO;return -1;}
 out->tv_sec=calls==1?100:99;out->tv_nsec=0;return 0;
}
