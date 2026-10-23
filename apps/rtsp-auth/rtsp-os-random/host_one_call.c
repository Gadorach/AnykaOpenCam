/* SPDX-License-Identifier: GPL-2.0-only */
#if !defined(RTSP_OS_RANDOM_HOST_ONE_CALL) || !defined(__linux__) || \
    !defined(__x86_64__) || defined(__ILP32__) || defined(__arm__) || \
    defined(IS_HOST_PUBLIC_ONLY) || defined(RTSP_HANDOFF_HOST_PUBLICDUMMY)
#error "Distinct Linux x86_64 HOST-only one-call harness, never native/app"
#endif
#include "os_random.h"
#include <mbedtls/platform_util.h>
#include <stdio.h>

int main(void)
{
    unsigned char out[32];
    int result = qc07f_rtsp_os_random_ready_v1(NULL, out, sizeof(out));
    mbedtls_platform_zeroize(out, sizeof(out));
    if (result != CAMERA_OK) {
        (void)printf("HOST_OS_RANDOM_ONE_CALL_REFUSED rc=%d NOT_NATIVE\n", result);
        return 1;
    }
    (void)puts("HOST_OS_RANDOM_ONE_CALL_PASS NOT_NATIVE");
    return 0;
}
