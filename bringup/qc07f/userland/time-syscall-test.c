/* SPDX-License-Identifier: GPL-2.0-only
 * Native ARM userland delay compatibility fixture; no camera/device access.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static uint64_t ms(void)
{struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return UINT64_MAX;return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
int main(int argc,char **argv)
{
 int allow_enosys=argc==2&&!strcmp(argv[1],"--allow-enosys");if(argc!=1&&!allow_enosys)return 2;
 struct timespec delay={0,150000000};uint64_t start=ms();if(start==UINT64_MAX)return 1;
 errno=0;int result=nanosleep(&delay,NULL),error=errno;uint64_t end=ms();if(end==UINT64_MAX)return 1;
 printf("TIME NATIVE nanosleep_ret=%d errno=%d elapsed_ms=%llu\n",result,error,(unsigned long long)(end-start));
 int native_ok=!result&&end-start>=150;
 if(!native_ok&&!(allow_enosys&&result==-1&&error==ENOSYS))return 1;
 start=ms();if(start==UINT64_MAX)return 1;errno=0;result=poll(NULL,0,150);error=errno;end=ms();if(end==UINT64_MAX)return 1;
 printf("TIME POLL ret=%d errno=%d elapsed_ms=%llu\n",result,error,(unsigned long long)(end-start));
 if(result||end-start<150)return 1;
 puts(native_ok?"TIME native+poll delay PASS":"TIME legacyENOSYS observed/poll PASS; nativeNOTqualified");return 0;
}
