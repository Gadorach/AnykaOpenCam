/* SPDX-License-Identifier: GPL-2.0-only */
#include "credential_mbedtls.h"
#include <mbedtls/version.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/platform_util.h>
#if MBEDTLS_VERSION_NUMBER != 0x03060700
#error "Review/requalify adapter before changing the selected Mbed TLS 3.6.7 pin"
#endif
int cw_material_valid(const struct cw_ui_material *m)
{
 return m&&m->schema==CW_SCHEMA&&m->iterations==CW_ITERATIONS;
}
int cw_mbedtls_verify(const struct cw_ui_material *m,const unsigned char *p,size_t n)
{
 unsigned char derived[32]={0};int result=-1;
 if(cw_material_valid(m)&&p&&n&&n<=CW_PASSWORD_MAX){
  if(!mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,p,n,m->salt,sizeof(m->salt),
      m->iterations,sizeof(derived),derived))
   result=mbedtls_ct_memcmp(derived,m->verifier,sizeof(derived))==0;
 }
 mbedtls_platform_zeroize(derived,sizeof(derived));return result;
}
