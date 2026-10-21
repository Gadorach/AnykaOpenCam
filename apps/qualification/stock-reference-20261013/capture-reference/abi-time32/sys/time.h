/* Compile-only replacement for libc's modern time64 view while comparing the
 * unchanged original3.4 videodev2.h. Not used by the candidate at runtime. */
#ifndef CR_ABI_TIME32_H
#define CR_ABI_TIME32_H
#include <stdint.h>
struct timeval { int32_t tv_sec,tv_usec; };
struct timespec { int32_t tv_sec,tv_nsec; };
#endif
