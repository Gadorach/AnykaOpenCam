/* SPDX-License-Identifier: GPL-2.0-only
 * Isolated 3.6.7 candidate config; NEVER reuse HTTPS/RTSP objects. */
#ifndef PP_MBEDTLS_CONFIG_H
#define PP_MBEDTLS_CONFIG_H
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_MD_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_HKDF_C
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#endif
