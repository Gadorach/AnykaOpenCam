/* SPDX-License-Identifier: GPL-2.0-only
 * HOST fixture clock-error boundary injection only. Ordinary calls delegate to
 * actual CLOCK_MONOTONIC. No crypto/source/stop success is mocked. ARM link only. */
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <errno.h>
static unsigned fault;
void candidate_test_clock_fail(void){fault=1;}
int __real_clock_gettime(clockid_t,struct timespec *);
int __wrap_clock_gettime(clockid_t clock,struct timespec *value)
{if(fault){errno=EIO;return -1;}return __real_clock_gettime(clock,value);}
