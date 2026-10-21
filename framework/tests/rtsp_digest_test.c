/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <camera/rtsp_digest.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/md5.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/sha256.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>

static unsigned checks;
/* Scalar failures only: never print expression, fields or credential material. */
#define CHECK(x) do { ++checks; if(!(x)){printf("HOST_ONLY FAIL check=%u line=%u\n",checks,(unsigned)__LINE__);return 1;} } while(0)
static struct camera_digest_span S(const char *p)
{struct camera_digest_span s={(const unsigned char *)p,strlen(p)};return s;}
static struct camera_digest_span B(const void *p,size_t n)
{struct camera_digest_span s={(const unsigned char *)p,n};return s;}
static size_t bytes(unsigned alg){return alg==CAMERA_DIGEST_SHA256?32:16;}
static void hex(const unsigned char *in,size_t n,unsigned char *out)
{
 static const char digits[]="0123456789abcdef";size_t i;
 for(i=0;i<n;i++){out[2*i]=(unsigned char)digits[in[i]>>4];out[2*i+1]=(unsigned char)digits[in[i]&15];}
}
/* Independent one-shot library formulation for PUBLICDUMMY requests. It does
 * not invoke the core response helper, so verifies field-joining independently.
 * No hash/PRNG/comparison implementation is supplied by these fixtures. */
static int client(unsigned alg,const unsigned char *ha1,struct camera_digest_span method,
 struct camera_digest_span uri,const struct camera_digest_fields *f,unsigned char response[64])
{
 unsigned char buf[1024],a2[32],h1[64],h2[64],digest[32];size_t len=0,n=bytes(alg),i;
 struct camera_digest_span p[6];int r;
 memcpy(buf,method.data,method.length);len=method.length;buf[len++]=':';
 memcpy(buf+len,uri.data,uri.length);len+=uri.length;
 r=alg==CAMERA_DIGEST_SHA256?mbedtls_sha256(buf,len,a2,0):mbedtls_md5(buf,len,a2);
 if(r)return r;
 hex(ha1,n,h1);hex(a2,n,h2);p[0]=B(h1,2*n);p[1]=f->nonce;p[2]=f->nc;p[3]=f->cnonce;p[4]=S("auth");p[5]=B(h2,2*n);len=0;
 for(i=0;i<6;i++){if(i)buf[len++]=':';memcpy(buf+len,p[i].data,p[i].length);len+=p[i].length;}
 r=alg==CAMERA_DIGEST_SHA256?mbedtls_sha256(buf,len,digest,0):mbedtls_md5(buf,len,digest);
 if(!r)hex(digest,n,response);
 mbedtls_platform_zeroize(buf,sizeof(buf));mbedtls_platform_zeroize(a2,sizeof(a2));
 mbedtls_platform_zeroize(h1,sizeof(h1));mbedtls_platform_zeroize(h2,sizeof(h2));mbedtls_platform_zeroize(digest,sizeof(digest));return r;
}
struct rng { unsigned calls; int fail; };
/* HOST fixture OS CSPRNG, not target cold entropy/readiness evidence. */
static int random_fixture(void *ctx,unsigned char *out,size_t n)
{
 struct rng *r=ctx;r->calls++;
 if(r->fail){if(n)out[0]=0;return -1;}
 return getrandom(out,n,0)==(ssize_t)n?0:-1;
}
/* Callback contract violation model: serial still prevents nonce reuse within
 * the owner lifetime. This is NOT a PRNG and NOT an accepted entropy source. */
static int repeated_fixture(void *ctx,unsigned char *out,size_t n)
{(void)ctx;memset(out,0,n);return 0;}
static int setup(struct camera_rtsp_digest *s,unsigned alg,struct rng *rng,unsigned char h[32])
{
 if(camera_rtsp_digest_ha1(alg,S("PUBLICDUMMY"),S("PUBLICDUMMY RTSP"),S("PUBLICDUMMY-not-product"),h))return -1;
 if(camera_rtsp_digest_init(s,alg,S("PUBLICDUMMY RTSP"),1000,100,random_fixture,rng))return -1;
 return camera_rtsp_digest_provision(s,1,S("PUBLICDUMMY"),h,bytes(alg),3);
}
static struct camera_digest_fields fields(unsigned alg,unsigned char *nonce,unsigned char *response)
{
 struct camera_digest_fields f;
 f.username=S("PUBLICDUMMY");f.realm=S("PUBLICDUMMY RTSP");f.uri=S("rtsp://publicdummy.invalid/live");
 f.algorithm=S(alg==CAMERA_DIGEST_SHA256?"SHA-256":"MD5");f.qop=S("auth");f.nonce=B(nonce,64);
 f.cnonce=S("PUBLICDUMMY-client");f.nc=S("00000001");f.response=B(response,2*bytes(alg));return f;
}
static int vectors(void)
{
 unsigned char h[32],r[32],wire[64];
 /* Public RFC2617 section 3.5 MD5 and RFC7616 section 3.9.1 SHA-256 data.
  * No RFC implementation code copied. Values are qualification-only, not
  * product credentials, never printed or separately payload-hashed. */
 CHECK(camera_rtsp_digest_ha1(CAMERA_DIGEST_MD5_LEGACY,S("Mufasa"),S("testrealm@host.com"),S("Circle Of Life"),h)==0);
 CHECK(camera_rtsp_digest_response(CAMERA_DIGEST_MD5_LEGACY,h,S("GET"),S("/dir/index.html"),S("dcd98b7102dd2f0e8b11d0f600bfb0c093"),S("00000001"),S("0a4f113b"),r)==0);
 hex(r,16,wire);CHECK(mbedtls_ct_memcmp(wire,"6629fae49393a05397450978507c4ef1",32)==0);
 CHECK(camera_rtsp_digest_ha1(CAMERA_DIGEST_SHA256,S("Mufasa"),S("http-auth@example.org"),S("Circle of Life"),h)==0);
 CHECK(camera_rtsp_digest_response(CAMERA_DIGEST_SHA256,h,S("GET"),S("/dir/index.html"),S("7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v"),S("00000001"),S("f2/wE4q74E6zIJEtWaHKaf5wv/H5QzzpXusqGemxURZJ"),r)==0);
 hex(r,32,wire);CHECK(mbedtls_ct_memcmp(wire,"753927fa0e85d155564e2e272a28d1802ca10daf4496794697cf8db5856cb6c1",64)==0);
 mbedtls_platform_zeroize(h,sizeof(h));mbedtls_platform_zeroize(r,sizeof(r));mbedtls_platform_zeroize(wire,sizeof(wire));return 0;
}
static int profile(unsigned alg)
{
 struct camera_rtsp_digest s;struct rng rng={0,0};struct camera_digest_fields f,bad;
 struct camera_digest_grant g,old,other,zero={0};
 unsigned char h[32],wrong[32],nonce[64],response[64],other_nonce[64],other_response[64],oversize[513];size_t i;
 CHECK(setup(&s,alg,&rng,h)==0);
 CHECK(s.generation==1&&s.enabled&&s.serial==0);
 CHECK(camera_rtsp_digest_issue(&s,11,0,100,nonce)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_issue(&s,11,3,100,nonce)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_issue(&s,11,4,100,nonce)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_issue(&s,0,1,100,nonce)==CAMERA_DIGEST_DENIED);
 rng.fail=1;CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==CAMERA_DIGEST_RANDOM_FAILED);
 CHECK(s.serial==0&&!s.nonces[0].used);rng.fail=0;
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==0);
 f=fields(alg,nonce,response);CHECK(client(alg,h,S("DESCRIBE"),f.uri,&f,response)==0);
 /* No failed request may update nc/cnonce or return a usable grant. */
 bad=f;bad.username=S("PUBLICDUMMY-wrong");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 CHECK(!memcmp(&g,&zero,sizeof(g)));
 bad=f;bad.realm=S("PUBLICDUMMY UI");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 bad=f;bad.algorithm=S(alg==CAMERA_DIGEST_SHA256?"MD5":"SHA-256");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 bad=f;bad.algorithm=S("SHA-256-sess");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 bad=f;bad.algorithm=B(NULL,0);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.qop=S("auth-int");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.qop=B(NULL,0);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.uri=S("rtsp://publicdummy.invalid/other");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,12,1,100,S("DESCRIBE"),f.uri,&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,11,2,100,S("DESCRIBE"),f.uri,&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("describe"),f.uri,&f,&g)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),S("rtsp://publicdummy.invalid/other"),&f,&g)==CAMERA_DIGEST_DENIED);
 bad=f;bad.nc=S("00000000");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.nc=S("0000000g");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.nc=S("100000001");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.nonce=B(nonce,63);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 memcpy(other_nonce,nonce,64);other_nonce[0]=other_nonce[0]=='a'?'b':'a';bad=f;bad.nonce=B(other_nonce,64);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_STALE);
 bad=f;bad.cnonce=S("PUBLICDUMMY-other");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 memset(oversize,'X',sizeof(oversize));
 bad=f;bad.uri=B(oversize,513);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.username=B(oversize,65);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.realm=B(oversize,65);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.cnonce=B(oversize,65);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.cnonce=S("bad\r\nvalue");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.username=S("bad:name");CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 /* Every response digit mutated, plus malformed length/null/hex. */
 for(i=0;i<2*bytes(alg);i++) {
  memcpy(other_response,response,2*bytes(alg));other_response[i]=other_response[i]=='a'?'b':'a';bad=f;bad.response=B(other_response,2*bytes(alg));
  CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 }
 bad=f;bad.response=B(response,2*bytes(alg)-1);CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 bad=f;bad.response=B(NULL,2*bytes(alg));CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 memcpy(other_response,response,2*bytes(alg));other_response[0]='z';bad=f;bad.response=B(other_response,2*bytes(alg));
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 CHECK(!s.nonces[0].last_nc&&!s.nonces[0].cnonce_length);
 CHECK(camera_rtsp_digest_ha1(alg,S("PUBLICDUMMY"),S("PUBLICDUMMY RTSP"),S("PUBLICDUMMY-wrong-password"),wrong)==0);
 CHECK(client(alg,wrong,S("DESCRIBE"),f.uri,&f,other_response)==0);bad=f;bad.response=B(other_response,2*bytes(alg));
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&f,&g)==0);old=g;
 CHECK(camera_rtsp_digest_check(&s,11,1,100,&g)==0);
 CHECK(camera_rtsp_digest_check(&s,12,1,100,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_check(&s,11,2,100,&g)==CAMERA_DIGEST_DENIED);
 other=g;other.expires_ms++;CHECK(camera_rtsp_digest_check(&s,11,1,100,&other)==CAMERA_DIGEST_STALE);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&f,&g)==CAMERA_DIGEST_REPLAY);
 f.nc=S("00000002");f.cnonce=S("PUBLICDUMMY-new-client");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==0);
 f.cnonce=S("PUBLICDUMMY-client");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_REPLAY);
 f.nc=S("00000003");CHECK(client(alg,h,S("GET_PARAMETER"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("GET_PARAMETER"),f.uri,&f,&g)==0);
 f.nc=S("00000004");CHECK(client(alg,h,S("SET_PARAMETER"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("SET_PARAMETER"),f.uri,&f,&g)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("GET PARAMETER"),f.uri,&f,&g)==CAMERA_DIGEST_INVALID);
 f.nc=S("00000001");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_REPLAY);
 f.nc=S("ffffffff");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==0);
 f.nc=S("00000001");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_REPLAY);
 /* Separate connection gets no inherited authentication. */
 CHECK(camera_rtsp_digest_issue(&s,12,1,100,other_nonce)==0);
 bad=fields(alg,other_nonce,other_response);CHECK(client(alg,h,S("PLAY"),bad.uri,&bad,other_response)==0);
 CHECK(camera_rtsp_digest_verify(&s,12,1,100,S("PLAY"),bad.uri,&bad,&other)==0);
 camera_rtsp_digest_close(&s,11);
 CHECK(camera_rtsp_digest_check(&s,11,1,100,&old)==CAMERA_DIGEST_STALE);
 CHECK(camera_rtsp_digest_check(&s,12,1,100,&other)==0);
 CHECK(camera_rtsp_digest_provision(&s,1,S("PUBLICDUMMY"),h,bytes(alg),3)==0);
 CHECK(s.generation==2);
 CHECK(camera_rtsp_digest_check(&s,12,1,100,&other)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,12,1,100,S("PLAY"),bad.uri,&bad,&g)==CAMERA_DIGEST_STALE);
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==0);
 f=fields(alg,nonce,response);CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,1099,S("PLAY"),f.uri,&f,&g)==0);old=g;
 CHECK(camera_rtsp_digest_check(&s,11,1,1100,&old)==CAMERA_DIGEST_STALE);
 CHECK(camera_rtsp_digest_verify(&s,11,1,1100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_STALE);
 for(i=0;i<CAMERA_DIGEST_NONCES;i++)CHECK(camera_rtsp_digest_issue(&s,11+i,1,1100,nonce)==0);
 {unsigned calls=rng.calls;CHECK(camera_rtsp_digest_issue(&s,99,1,1100,nonce)==CAMERA_DIGEST_FULL);CHECK(rng.calls==calls);}
 CHECK(camera_rtsp_digest_issue(&s,99,1,2100,nonce)==0);
 CHECK(camera_rtsp_digest_invalidate(&s)==0&&s.generation==3&&!s.enabled);
 CHECK(camera_rtsp_digest_issue(&s,99,1,2100,nonce)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_provision(&s,1,S("PUBLICDUMMY"),h,bytes(alg),3)==0&&s.generation==4);
 CHECK(camera_rtsp_digest_issue(&s,99,1,2100,nonce)==0);
 CHECK(camera_rtsp_digest_issue(&s,99,1,2099,nonce)==CAMERA_DIGEST_CLOCK_FAILED);
 CHECK(camera_rtsp_digest_issue(&s,99,1,2101,nonce)==CAMERA_DIGEST_CLOCK_FAILED);
 CHECK(camera_rtsp_digest_provision(&s,1,S("PUBLICDUMMY"),h,bytes(alg),3)==CAMERA_DIGEST_CLOCK_FAILED);
 camera_rtsp_digest_destroy(&s);
 CHECK(!s.initialized&&!s.enabled&&!s.generation);
 CHECK(setup(&s,alg,&rng,h)==0);
 /* Owner-storage mutation ONLY to reach otherwise infeasible counter edges. */
 s.generation=UINT64_MAX;
 CHECK(camera_rtsp_digest_provision(&s,1,S("PUBLICDUMMY"),h,bytes(alg),3)==CAMERA_DIGEST_EXHAUSTED);
 CHECK(s.exhausted&&!s.enabled&&s.generation==UINT64_MAX);
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==CAMERA_DIGEST_EXHAUSTED);
 camera_rtsp_digest_destroy(&s);CHECK(setup(&s,alg,&rng,h)==0);s.serial=UINT64_MAX;
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==CAMERA_DIGEST_EXHAUSTED&&s.serial==UINT64_MAX);
 camera_rtsp_digest_destroy(&s);CHECK(setup(&s,alg,&rng,h)==0);
 CHECK(camera_rtsp_digest_issue(&s,11,1,UINT64_MAX-999,nonce)==CAMERA_DIGEST_EXHAUSTED);
 camera_rtsp_digest_destroy(&s);CHECK(setup(&s,alg,&rng,h)==0);s.random=repeated_fixture;
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==0);camera_rtsp_digest_close(&s,11);
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,other_nonce)==0);
 CHECK(memcmp(nonce,other_nonce,64)!=0);
 camera_rtsp_digest_destroy(&s);mbedtls_platform_zeroize(h,sizeof(h));mbedtls_platform_zeroize(wrong,sizeof(wrong));
 mbedtls_platform_zeroize(response,sizeof(response));mbedtls_platform_zeroize(other_response,sizeof(other_response));return 0;
}
static int invalid_inputs(void)
{
 struct camera_rtsp_digest s;struct rng rng={0,0};unsigned char h[32],buf[513];
 memset(buf,'X',sizeof(buf));
 CHECK(camera_rtsp_digest_init(&s,0,S("PUBLICDUMMY"),1000,0,random_fixture,&rng)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_init(&s,1,S("PUBLICDUMMY"),0,0,random_fixture,&rng)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_init(&s,1,S("PUBLICDUMMY"),300001,0,random_fixture,&rng)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_init(&s,1,S("PUBLICDUMMY"),1000,0,NULL,&rng)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_init(&s,1,B(buf,65),1000,0,random_fixture,&rng)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_ha1(1,S("PUBLICDUMMY"),S("PUBLICDUMMY"),B(buf,129),h)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_ha1(1,S("PUBLIC:DUMMY"),S("PUBLICDUMMY"),S("PUBLICDUMMY"),h)==CAMERA_DIGEST_INVALID);
 CHECK(setup(&s,1,&rng,h)==0);
 CHECK(camera_rtsp_digest_provision(&s,0,S("PUBLICDUMMY"),h,32,1)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_provision(&s,1,S("PUBLICDUMMY"),h,16,1)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_provision(&s,1,S("PUBLICDUMMY"),h,32,0)==CAMERA_DIGEST_INVALID);
 CHECK(s.generation==1);
 camera_rtsp_digest_destroy(&s);mbedtls_platform_zeroize(h,sizeof(h));return 0;
}
static int malformed_fields(unsigned alg)
{
 struct camera_rtsp_digest s;struct rng rng={0,0};struct camera_digest_fields f,bad;
 struct camera_digest_grant g;struct camera_digest_span *p;
 unsigned char h[32],nonce[64],response[64],buffer[513];size_t i,j;
 CHECK(setup(&s,alg,&rng,h)==0);
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==0);
 f=fields(alg,nonce,response);CHECK(client(alg,h,S("DESCRIBE"),f.uri,&f,response)==0);
 /* Explicit switch: do not assume C struct members form an array. */
 for(i=0;i<9;i++)for(j=0;j<4;j++) {
  bad=f;
  switch(i) {
  case 0:p=&bad.username;break;case 1:p=&bad.realm;break;case 2:p=&bad.uri;break;
  case 3:p=&bad.algorithm;break;case 4:p=&bad.qop;break;case 5:p=&bad.nonce;break;
  case 6:p=&bad.cnonce;break;case 7:p=&bad.nc;break;default:p=&bad.response;break;
  }
  if(j==0)*p=B(NULL,p->length);
  else if(j==1)*p=B(buffer,0);
  else if(j==2)*p=B(buffer,513); /* all bounds tested before reads */
  else {memset(buffer,0,sizeof(buffer));*p=B(buffer,p->length);}
  CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
  CHECK(!g.serial&&!s.nonces[0].last_nc&&!s.nonces[0].cnonce_length);
 }
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,NULL,&g)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,B(NULL,8),f.uri,&f,&g)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),B(buffer,513),&f,&g)==CAMERA_DIGEST_INVALID);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("DESCRIBE"),f.uri,&f,&g)==0);
 camera_rtsp_digest_destroy(&s);mbedtls_platform_zeroize(h,sizeof(h));mbedtls_platform_zeroize(response,sizeof(response));return 0;
}
static int cnonce_changes(unsigned alg)
{
 struct camera_rtsp_digest s;struct rng rng={0,0};struct camera_digest_fields f,bad;
 struct camera_digest_grant g,old,zero={0};struct camera_digest_nonce before;
 unsigned char h[32],nonce[64],response[64],other_response[64];
 CHECK(setup(&s,alg,&rng,h)==0);
 CHECK(camera_rtsp_digest_issue(&s,11,1,100,nonce)==0);
 f=fields(alg,nonce,response);CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==0);old=g;before=s.nonces[0];
 /* Fresh cnonce/higher nc but response still binds the old cnonce: denied. */
 f.nc=S("00000002");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 f.cnonce=S("PUBLICDUMMY-fresh-B");
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(!memcmp(&s.nonces[0],&before,sizeof(before))&&!memcmp(&g,&zero,sizeof(g)));
 /* Fresh cnonce/higher nc plus one bad response digit: no nc/metadata advance. */
 CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 memcpy(other_response,response,2*bytes(alg));other_response[0]=other_response[0]=='a'?'b':'a';
 bad=f;bad.response=B(other_response,2*bytes(alg));
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&bad,&g)==CAMERA_DIGEST_DENIED);
 CHECK(!memcmp(&s.nonces[0],&before,sizeof(before))&&!memcmp(&g,&zero,sizeof(g)));
 CHECK(camera_rtsp_digest_check(&s,11,1,100,&old)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==0);
 CHECK(s.nonces[0].last_nc==2&&s.nonces[0].cnonce_length==f.cnonce.length&&
       !memcmp(s.nonces[0].cnonce,f.cnonce.data,f.cnonce.length));before=s.nonces[0];
 /* Correct MAC with another fresh cnonce cannot reset the global counter. */
 f.cnonce=S("PUBLICDUMMY-fresh-C");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_REPLAY);
 f.nc=S("00000001");f.cnonce=S("PUBLICDUMMY-fresh-D");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_REPLAY);
 CHECK(!memcmp(&s.nonces[0],&before,sizeof(before))&&!memcmp(&g,&zero,sizeof(g)));
 f.nc=S("00000003");f.cnonce=S("PUBLICDUMMY-fresh-C");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,12,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,11,2,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PAUSE"),f.uri,&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),S("rtsp://publicdummy.invalid/other"),&f,&g)==CAMERA_DIGEST_DENIED);
 CHECK(!memcmp(&s.nonces[0],&before,sizeof(before))&&!memcmp(&g,&zero,sizeof(g)));
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==0);
 CHECK(s.nonces[0].last_nc==3&&s.nonces[0].cnonce_length==f.cnonce.length&&
       !memcmp(s.nonces[0].cnonce,f.cnonce.data,f.cnonce.length));
 /* Supported read-only input alias: nonce and cnonce share external bytes;
  * actual_uri already aliases fields.uri. Output grant remains fully disjoint. */
 f.nc=S("00000004");f.cnonce=f.nonce;CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==0);
 CHECK(s.nonces[0].last_nc==4&&s.nonces[0].cnonce_length==64);
 CHECK(camera_rtsp_digest_check(&s,11,1,100,&old)==0);
 CHECK(mbedtls_ct_memcmp(s.ha1,h,bytes(alg))==0);before=s.nonces[0];
 bad=f;bad.nc=S("00000005");bad.cnonce=B(NULL,0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&bad,&g)==CAMERA_DIGEST_INVALID);
 CHECK(!memcmp(&s.nonces[0],&before,sizeof(before))&&!memcmp(&g,&zero,sizeof(g)));
 f.nc=S("ffffffff");f.cnonce=S("PUBLICDUMMY-final");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==0);before=s.nonces[0];
 f.nc=S("00000001");f.cnonce=S("PUBLICDUMMY-after-final");CHECK(client(alg,h,S("PLAY"),f.uri,&f,response)==0);
 CHECK(camera_rtsp_digest_verify(&s,11,1,100,S("PLAY"),f.uri,&f,&g)==CAMERA_DIGEST_REPLAY);
 CHECK(!memcmp(&s.nonces[0],&before,sizeof(before))&&!memcmp(&g,&zero,sizeof(g)));
 camera_rtsp_digest_destroy(&s);mbedtls_platform_zeroize(h,sizeof(h));mbedtls_platform_zeroize(response,sizeof(response));
 mbedtls_platform_zeroize(other_response,sizeof(other_response));return 0;
}
int main(void)
{
 if(vectors()||profile(CAMERA_DIGEST_SHA256)||profile(CAMERA_DIGEST_MD5_LEGACY)||
    invalid_inputs()||malformed_fields(CAMERA_DIGEST_SHA256)||malformed_fields(CAMERA_DIGEST_MD5_LEGACY)||
    cnonce_changes(CAMERA_DIGEST_SHA256)||cnonce_changes(CAMERA_DIGEST_MD5_LEGACY))return 1;
 printf("HOST_ONLY PASS checks=%u profiles=2 rfc_vectors=2 nonce_capacity=%u core_bytes=%zu\n",checks,CAMERA_DIGEST_NONCES,sizeof(struct camera_rtsp_digest));return 0;
}
