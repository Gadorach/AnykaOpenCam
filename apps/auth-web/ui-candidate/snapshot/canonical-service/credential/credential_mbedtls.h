/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CREDENTIAL_MBEDTLS_H
#define CREDENTIAL_MBEDTLS_H
#include "credential_worker.h"
int cw_material_valid(const struct cw_ui_material *);
/* 1 match, 0 mismatch, -1 error; wipes derived output on every return. */
int cw_mbedtls_verify(const struct cw_ui_material *,const unsigned char *,size_t);
#endif
