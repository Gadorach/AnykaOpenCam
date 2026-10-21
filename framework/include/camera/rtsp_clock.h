/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_CLOCK_H
#define CAMERA_RTSP_CLOCK_H
#define CAMERA_RTSP_CLOCK_API 1U
#include <stdint.h>
#include <time.h>
/* Caller-serialized, sticky clock fault. UINT64_MAX is ERROR ONLY, never time
 * for a producer callback/acknowledgement. Fresh owners initialize to zero;
 * reconnects cannot reset an uncertain owner. No realtime-clock fallback. */
struct camera_rtsp_clock {uint64_t last_ns;unsigned int seen,failed;};
static inline uint64_t camera_rtsp_clock_ns(struct camera_rtsp_clock *clock)
{
 struct timespec t;
 if(clock->failed)return UINT64_MAX;
 if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L||
    (uint64_t)t.tv_sec>(UINT64_MAX-1000000000ULL)/1000000000ULL){clock->failed=1;return UINT64_MAX;}
 uint64_t now=(uint64_t)t.tv_sec*1000000000ULL+(uint64_t)t.tv_nsec;
 if(now==UINT64_MAX||(clock->seen&&now<clock->last_ns)){clock->failed=1;return UINT64_MAX;}
 clock->last_ns=now;clock->seen=1;return now;
}
static inline uint64_t camera_rtsp_clock_ms(struct camera_rtsp_clock *clock)
{uint64_t now=camera_rtsp_clock_ns(clock);return now==UINT64_MAX?UINT64_MAX:now/1000000ULL;}
#endif
