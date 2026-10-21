/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include "security_rng.h"
#include "security_memory.h"
#include <stdio.h>
#include <sys/resource.h>
#include <time.h>

static uint64_t monotonic_ns(void)
{
 struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;
 return (uint64_t)t.tv_sec*1000000000ULL+(uint64_t)t.tv_nsec;
}
/* No network, files, keys or payload output. One bounded nonblocking request.
 * This probes OS readiness ONLY, not PSA/library startup or entropy quality. */
int main(void)
{
 struct security_rng rng;unsigned char output[32];struct rusage usage={0};
 if(security_rng_init(&rng,NULL,NULL))return 3;
 uint64_t start=monotonic_ns();int status=security_rng_fill(&rng,output,sizeof(output));
 uint64_t end=monotonic_ns();security_rng_zero(output,sizeof(output));
 int resource_ok=getrusage(RUSAGE_SELF,&usage)==0;
 printf("SECURITY ENTROPY ready=%u status=%d bytes=%zu calls=%u errno=%d "
  "clock_ok=%u elapsed_ns=%llu resource_ok=%u maxrss_kib=%ld "
  "usable_mib=40 cma_mib=16 noncma_mib=24 psa_ready=0\n",
  status==SECURITY_RNG_OK,status,rng.last_bytes,rng.last_calls,rng.os_error,
  start!=0&&end>=start,(unsigned long long)(start&&end>=start?end-start:0),
  resource_ok,usage.ru_maxrss);
 if(!start||end<start||!resource_ok)return 3;
 return status==SECURITY_RNG_OK?0:status==SECURITY_RNG_NOT_READY?2:3;
}
