/* SPDX-License-Identifier: GPL-2.0-only
 *Server/adapter IMAGE pin + zeroization only. RTSP HA1/MD5 objects live ONLY in
 *separate child/bootstrap fixture builds. NEVER included by UI/HTTPS objects.*/
#ifndef IS_SERVER_CRYPTO_CONFIG_H
#define IS_SERVER_CRYPTO_CONFIG_H
#define MBEDTLS_SHA256_C
#endif
