/* SPDX-License-Identifier: GPL-2.0-only
 * Pure PUBLICDUMMY schema/crypto/owner boundary fixtures, no listener/devices. */
#include "candidate_auth.h"
#include <mbedtls/constant_time.h>
#include <mbedtls/platform_util.h>
#include <stdio.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){printf("HOST_ONLY FAIL check=%u line=%u\n",checks,(unsigned)__LINE__);return 1;}} while(0)
static struct camera_digest_span S(const char *p){return (struct camera_digest_span){(const unsigned char *)p,strlen(p)};}
static void hex(const unsigned char *p,size_t n,unsigned char *out)
{size_t i;static const unsigned char d[]="0123456789abcdef";for(i=0;i<n;i++){out[2*i]=d[p[i]>>4];out[2*i+1]=d[p[i]&15];}}
static int vectors(void)
{
 unsigned char ha1[32],response[32],encoded[64];
 CHECK(camera_rtsp_digest_ha1(CAMERA_DIGEST_MD5_LEGACY,S("Mufasa"),S("testrealm@host.com"),S("Circle Of Life"),ha1)==0);
 CHECK(camera_rtsp_digest_response(CAMERA_DIGEST_MD5_LEGACY,ha1,S("GET"),S("/dir/index.html"),S("dcd98b7102dd2f0e8b11d0f600bfb0c093"),S("00000001"),S("0a4f113b"),response)==0);
 hex(response,16,encoded);CHECK(mbedtls_ct_memcmp(encoded,"6629fae49393a05397450978507c4ef1",32)==0);
 CHECK(camera_rtsp_digest_ha1(CAMERA_DIGEST_SHA256,S("Mufasa"),S("http-auth@example.org"),S("Circle of Life"),ha1)==0);
 CHECK(camera_rtsp_digest_response(CAMERA_DIGEST_SHA256,ha1,S("GET"),S("/dir/index.html"),S("7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v"),S("00000001"),S("f2/wE4q74E6zIJEtWaHKaf5wv/H5QzzpXusqGemxURZJ"),response)==0);
 hex(response,32,encoded);CHECK(mbedtls_ct_memcmp(encoded,"753927fa0e85d155564e2e272a28d1802ca10daf4496794697cf8db5856cb6c1",64)==0);
 mbedtls_platform_zeroize(ha1,sizeof(ha1));mbedtls_platform_zeroize(response,sizeof(response));mbedtls_platform_zeroize(encoded,sizeof(encoded));return 0;
}
int main(void)
{
 struct candidate_backend b;struct candidate_config c={0};struct candidate_peer p={0};
 CHECK(vectors()==0);
 CHECK(candidate_backend_init(&b,NULL,100)==CAMERA_INVALID&&!b.initialized);
 CHECK(candidate_backend_init(&b,&c,100)==CAMERA_INVALID&&!b.initialized);
 c.abi=CANDIDATE_AUTH_ABI;c.struct_bytes=sizeof(c);c.host_fixture_only=1;c.algorithm=CAMERA_DIGEST_SHA256;
 CHECK(candidate_backend_init(&b,&c,100)==CAMERA_INVALID&&!b.initialized); /* missing bootstrap/CSPRNG/graph/roles */
 CHECK(candidate_peer_open(&b,&p,0,100)!=CAMERA_OK&&!p.live);
 CHECK(candidate_effect(&b,&p,100)!=CAMERA_OK);
 CHECK(candidate_backend_rotate(&b,NULL)!=CAMERA_OK);
 CHECK(candidate_backend_cancel(&b,0)!=CAMERA_OK);
 candidate_backend_destroy(&b);CHECK(!b.initialized&&!b.core.initialized);
 printf("HOST_ONLY PASS checks=%u RFC_vectors=2 no_listener=1 no_crypto_mock=1\n",checks);return 0;
}
