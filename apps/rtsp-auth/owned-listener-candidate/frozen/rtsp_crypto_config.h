/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_RTSP_CRYPTO_CONFIG_H
#define CAMERA_RTSP_CRYPTO_CONFIG_H
/* RTSP-only standalone object set: NEVER use this config in auth-web HTTPS.
 * MD5 is solely an explicit legacy Digest interoperability profile. No TLS,
 * PSA, entropy implementation or home-made crypto. */
#define MBEDTLS_SHA256_C
#define MBEDTLS_MD5_C
#endif
