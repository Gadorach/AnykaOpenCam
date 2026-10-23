/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "os_random.h"
#include "../qc07f-owned-runtime/server/candidate_auth.h"
#include <mbedtls/platform_util.h>
#include <sys/random.h>

/* Type-only binding to the actual field. No config instance/registration. */
_Static_assert(__builtin_types_compatible_p(
    __typeof__(((struct candidate_config *)0)->random_ready),
    __typeof__(&qc07f_rtsp_os_random_ready_v1)),
    "owned-runtime random_ready callback type changed");

int qc07f_rtsp_os_random_ready_v1(void *context, unsigned char *out, size_t bytes)
{
    (void)context;
    if (!out || bytes != 32)
        return CAMERA_INVALID;

    mbedtls_platform_zeroize(out, 32);
    if (getrandom(out, 32, GRND_NONBLOCK) != 32) {
        mbedtls_platform_zeroize(out, 32);
        return CAMERA_NOT_READY;
    }
    return CAMERA_OK;
}
