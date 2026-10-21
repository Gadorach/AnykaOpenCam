/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/rtsp_digest.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/md5.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/sha256.h>
#include <string.h>

static size_t digest_bytes(uint32_t algorithm)
{return algorithm==CAMERA_DIGEST_SHA256?32U:algorithm==CAMERA_DIGEST_MD5_LEGACY?16U:0U;}
static struct camera_digest_span span(const void *p,size_t n)
{struct camera_digest_span s={(const unsigned char *)p,n};return s;}
static int text(struct camera_digest_span s,size_t max,int spaces,int name)
{
 size_t i;if(!s.data||!s.length||s.length>max)return 0;
 for(i=0;i<s.length;i++) {
  unsigned char c=s.data[i];
  if(c<(spaces?32U:33U)||c>126U||c=='"'||c=='\\'||(name&&c==':'))return 0;
 }
 return 1;
}
static int method_ok(struct camera_digest_span s)
{
 size_t i;if(!text(s,24,0,0))return 0;
 for(i=0;i<s.length;i++)if((s.data[i]<'A'||s.data[i]>'Z')&&s.data[i]!='_')return 0;
 return 1;
}
static int equal(struct camera_digest_span a,struct camera_digest_span b)
{return a.data&&b.data&&a.length==b.length&&!memcmp(a.data,b.data,a.length);}
static int literal(struct camera_digest_span s,const char *p)
{return equal(s,span(p,strlen(p)));}
static int digit(unsigned char c)
{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static int count(struct camera_digest_span s,uint32_t *nc)
{
 size_t i;uint32_t v=0;if(!s.data||s.length!=8)return 0;
 for(i=0;i<8;i++){int d=digit(s.data[i]);if(d<0)return 0;v=(v<<4)|(uint32_t)d;}
 *nc=v;return v!=0;
}
static int decode(struct camera_digest_span s,unsigned char *out,size_t n)
{
 size_t i;if(!s.data||s.length!=2*n)return 0;
 for(i=0;i<n;i++) {
  int a=digit(s.data[2*i]),b=digit(s.data[2*i+1]);if(a<0||b<0)return 0;
  out[i]=(unsigned char)((a<<4)|b);
 }
 return 1;
}
static void hex(const unsigned char *p,size_t n,unsigned char *out)
{
 static const unsigned char digits[]="0123456789abcdef";size_t i;
 for(i=0;i<n;i++){out[2*i]=digits[p[i]>>4];out[2*i+1]=digits[p[i]&15];}
}
/* Hash bounded field spans separated by ':'; no full-request or heap copy. */
static int hash(uint32_t algorithm,const struct camera_digest_span *parts,size_t n,unsigned char out[32])
{
 mbedtls_sha256_context sha;mbedtls_md5_context md5;size_t i;int r;
 memset(out,0,32);mbedtls_sha256_init(&sha);mbedtls_md5_init(&md5);
 if(algorithm==CAMERA_DIGEST_SHA256)r=mbedtls_sha256_starts(&sha,0);
 else if(algorithm==CAMERA_DIGEST_MD5_LEGACY)r=mbedtls_md5_starts(&md5);
 else r=-1;
 for(i=0;i<n&&!r;i++) {
  if(i)r=algorithm==CAMERA_DIGEST_SHA256?mbedtls_sha256_update(&sha,(const unsigned char *)":",1):mbedtls_md5_update(&md5,(const unsigned char *)":",1);
  if(!r)r=algorithm==CAMERA_DIGEST_SHA256?mbedtls_sha256_update(&sha,parts[i].data,parts[i].length):mbedtls_md5_update(&md5,parts[i].data,parts[i].length);
 }
 if(!r)r=algorithm==CAMERA_DIGEST_SHA256?mbedtls_sha256_finish(&sha,out):mbedtls_md5_finish(&md5,out);
 mbedtls_sha256_free(&sha);mbedtls_md5_free(&md5);
 if(r)mbedtls_platform_zeroize(out,32);
 return r?CAMERA_DIGEST_CRYPTO_FAILED:CAMERA_DIGEST_OK;
}
int camera_rtsp_digest_ha1(uint32_t alg,struct camera_digest_span username,
 struct camera_digest_span realm,struct camera_digest_span password,unsigned char out[32])
{
 struct camera_digest_span p[3]={username,realm,password};
 if(!out)return CAMERA_DIGEST_INVALID;
 memset(out,0,32);
 if(!digest_bytes(alg)||!text(username,CAMERA_DIGEST_NAME_MAX,1,1)||
    !text(realm,CAMERA_DIGEST_NAME_MAX,1,1)||!password.data||!password.length||password.length>CAMERA_DIGEST_PASSWORD_MAX)return CAMERA_DIGEST_INVALID;
 return hash(alg,p,3,out);
}
int camera_rtsp_digest_response(uint32_t alg,const unsigned char ha1[32],
 struct camera_digest_span method,struct camera_digest_span uri,
 struct camera_digest_span nonce,struct camera_digest_span nc,
 struct camera_digest_span cnonce,unsigned char out[32])
{
 unsigned char a2[32],h1[64],h2[64];size_t n=digest_bytes(alg);uint32_t value;int r;
 struct camera_digest_span p[6],a[2]={method,uri};
 if(!out)return CAMERA_DIGEST_INVALID;
 memset(out,0,32);
 if(!n||!ha1||!method_ok(method)||!text(uri,CAMERA_DIGEST_URI_MAX,0,0)||
    !text(nonce,CAMERA_DIGEST_NONCE_BYTES,0,0)||!count(nc,&value)||!text(cnonce,CAMERA_DIGEST_CNONCE_MAX,0,0))return CAMERA_DIGEST_INVALID;
 r=hash(alg,a,2,a2);
 if(!r) {
  hex(ha1,n,h1);hex(a2,n,h2);
  p[0]=span(h1,2*n);p[1]=nonce;p[2]=nc;p[3]=cnonce;p[4]=span("auth",4);p[5]=span(h2,2*n);
  r=hash(alg,p,6,out);
 }
 mbedtls_platform_zeroize(a2,sizeof(a2));mbedtls_platform_zeroize(h1,sizeof(h1));mbedtls_platform_zeroize(h2,sizeof(h2));return r;
}
static void clear_nonces(struct camera_rtsp_digest *s)
{mbedtls_platform_zeroize(s->nonces,sizeof(s->nonces));}
static int live(struct camera_rtsp_digest *s)
{
 if(!s||!s->initialized)return CAMERA_DIGEST_INVALID;
 if(s->clock_failed)return CAMERA_DIGEST_CLOCK_FAILED;
 if(s->exhausted)return CAMERA_DIGEST_EXHAUSTED;
 return CAMERA_DIGEST_OK;
}
static int tick(struct camera_rtsp_digest *s,uint64_t now)
{
 size_t i;int r=live(s);if(r)return r;
 if(now<s->last_ms){s->clock_failed=1;clear_nonces(s);return CAMERA_DIGEST_CLOCK_FAILED;}
 s->last_ms=now;
 for(i=0;i<CAMERA_DIGEST_NONCES;i++)if(s->nonces[i].used&&now>=s->nonces[i].expires_ms)mbedtls_platform_zeroize(&s->nonces[i],sizeof(s->nonces[i]));
 return CAMERA_DIGEST_OK;
}
static int advance(struct camera_rtsp_digest *s)
{
 clear_nonces(s);
 if(s->generation==UINT64_MAX) {
  s->exhausted=1;s->enabled=0;mbedtls_platform_zeroize(s->ha1,sizeof(s->ha1));return CAMERA_DIGEST_EXHAUSTED;
 }
 ++s->generation;return CAMERA_DIGEST_OK;
}
static int one_scope(uint64_t scope)
{return scope&&!(scope&(scope-1));}
int camera_rtsp_digest_init(struct camera_rtsp_digest *s,uint32_t alg,
 struct camera_digest_span realm,uint32_t ttl,uint64_t now,camera_digest_random_fn random,void *context)
{
 if(!s)return CAMERA_DIGEST_INVALID;
 memset(s,0,sizeof(*s));
 if(!digest_bytes(alg)||!text(realm,CAMERA_DIGEST_NAME_MAX,1,1)||!ttl||ttl>CAMERA_DIGEST_TTL_MAX_MS||!random)return CAMERA_DIGEST_INVALID;
 memcpy(s->realm,realm.data,realm.length);s->realm_length=realm.length;s->algorithm=alg;s->ttl_ms=ttl;
 s->last_ms=now;s->random=random;s->random_context=context;s->initialized=1;return CAMERA_DIGEST_OK;
}
int camera_rtsp_digest_provision(struct camera_rtsp_digest *s,uint64_t account,
 struct camera_digest_span username,const unsigned char *ha1,size_t n,uint64_t scopes)
{
 int r=live(s);if(r)return r;
 if(!account||!scopes||!text(username,CAMERA_DIGEST_NAME_MAX,1,1)||!ha1||n!=digest_bytes(s->algorithm))return CAMERA_DIGEST_INVALID;
 r=advance(s);if(r)return r;
 mbedtls_platform_zeroize(s->ha1,sizeof(s->ha1));memcpy(s->ha1,ha1,n);
 memset(s->username,0,sizeof(s->username));memcpy(s->username,username.data,username.length);s->username_length=username.length;
 s->account=account;s->scope_mask=scopes;s->enabled=1;return CAMERA_DIGEST_OK;
}
int camera_rtsp_digest_invalidate(struct camera_rtsp_digest *s)
{
 int r=live(s);if(r)return r;
 s->enabled=0;mbedtls_platform_zeroize(s->ha1,sizeof(s->ha1));return advance(s);
}
int camera_rtsp_digest_issue(struct camera_rtsp_digest *s,uint64_t connection,
 uint64_t scope,uint64_t now,unsigned char out[CAMERA_DIGEST_NONCE_BYTES])
{
 size_t i;unsigned char random[24],counter[8];struct camera_digest_nonce *n=NULL;int r;
 if(!out)return CAMERA_DIGEST_INVALID;
 memset(out,0,CAMERA_DIGEST_NONCE_BYTES);r=tick(s,now);if(r)return r;
 if(!s->enabled||!connection||!one_scope(scope)||!(scope&s->scope_mask))return CAMERA_DIGEST_DENIED;
 if(now>UINT64_MAX-s->ttl_ms||s->serial==UINT64_MAX)return CAMERA_DIGEST_EXHAUSTED;
 for(i=0;i<CAMERA_DIGEST_NONCES;i++)if(!s->nonces[i].used){n=&s->nonces[i];break;}
 if(!n)return CAMERA_DIGEST_FULL;
 memset(random,0,sizeof(random));r=s->random(s->random_context,random,sizeof(random));
 if(r){mbedtls_platform_zeroize(random,sizeof(random));return CAMERA_DIGEST_RANDOM_FAILED;}
 ++s->serial;
 for(i=0;i<8;i++)counter[7-i]=(unsigned char)(s->serial>>(8*i));
 /* Opaque nonce: 192 ready random bits + nonwrapping public issue serial. Not
  * a custom signed nonce; exact server registry membership is always required. */
 hex(random,sizeof(random),n->nonce);hex(counter,sizeof(counter),n->nonce+48);
 mbedtls_platform_zeroize(random,sizeof(random));
 n->serial=s->serial;n->connection=connection;n->scope=scope;n->generation=s->generation;n->expires_ms=now+s->ttl_ms;n->used=1;
 memcpy(out,n->nonce,CAMERA_DIGEST_NONCE_BYTES);return CAMERA_DIGEST_OK;
}
int camera_rtsp_digest_verify(struct camera_rtsp_digest *s,uint64_t connection,
 uint64_t scope,uint64_t now,struct camera_digest_span method,
 struct camera_digest_span uri,const struct camera_digest_fields *f,struct camera_digest_grant *grant)
{
 size_t i;uint32_t nc;unsigned char expected[32],response[32];struct camera_digest_nonce *n=NULL;int r;
 if(!grant)return CAMERA_DIGEST_INVALID;
 memset(grant,0,sizeof(*grant));r=tick(s,now);if(r)return r;
 if(!s->enabled||!connection||!one_scope(scope)||!(scope&s->scope_mask))return CAMERA_DIGEST_DENIED;
 if(!f||!method_ok(method)||!text(uri,CAMERA_DIGEST_URI_MAX,0,0)||
    !text(f->username,CAMERA_DIGEST_NAME_MAX,1,1)||!text(f->realm,CAMERA_DIGEST_NAME_MAX,1,1)||
    !text(f->uri,CAMERA_DIGEST_URI_MAX,0,0)||!text(f->algorithm,16,0,0)||
    !literal(f->qop,"auth")||!text(f->nonce,CAMERA_DIGEST_NONCE_BYTES,0,0)||f->nonce.length!=CAMERA_DIGEST_NONCE_BYTES||
    !text(f->cnonce,CAMERA_DIGEST_CNONCE_MAX,0,0)||!count(f->nc,&nc))return CAMERA_DIGEST_INVALID;
 if(!decode(f->response,response,digest_bytes(s->algorithm))){r=CAMERA_DIGEST_INVALID;goto done;}
 if(!equal(f->username,span(s->username,s->username_length))||!equal(f->realm,span(s->realm,s->realm_length))||!equal(f->uri,uri)||
    !literal(f->algorithm,s->algorithm==CAMERA_DIGEST_SHA256?"SHA-256":"MD5")) {r=CAMERA_DIGEST_DENIED;goto done;}
 for(i=0;i<CAMERA_DIGEST_NONCES;i++)if(s->nonces[i].used&&equal(f->nonce,span(s->nonces[i].nonce,CAMERA_DIGEST_NONCE_BYTES))){n=&s->nonces[i];break;}
 if(!n){r=CAMERA_DIGEST_STALE;goto done;}
 if(n->connection!=connection||n->scope!=scope||n->generation!=s->generation){r=CAMERA_DIGEST_DENIED;goto done;}
  /* RFC7616 non-sess qop=auth may supply a fresh cnonce on each request. Global
   * nc per server nonce, not per-cnonce state, prevents replay-counter reset.
   * Authenticate this request's cnonce in the response before advancing state. */
 if(nc<=n->last_nc){r=CAMERA_DIGEST_REPLAY;goto done;}
 r=camera_rtsp_digest_response(s->algorithm,s->ha1,method,uri,f->nonce,f->nc,f->cnonce,expected);
 if(!r&&mbedtls_ct_memcmp(expected,response,digest_bytes(s->algorithm)))r=CAMERA_DIGEST_DENIED;
 if(!r) {
  n->last_nc=nc;memcpy(n->cnonce,f->cnonce.data,f->cnonce.length);n->cnonce_length=f->cnonce.length;
  grant->account=s->account;grant->generation=s->generation;grant->serial=n->serial;
  grant->connection=connection;grant->scope=scope;grant->expires_ms=n->expires_ms;
 }
done:
 mbedtls_platform_zeroize(expected,sizeof(expected));mbedtls_platform_zeroize(response,sizeof(response));return r;
}
int camera_rtsp_digest_check(struct camera_rtsp_digest *s,uint64_t connection,
 uint64_t scope,uint64_t now,const struct camera_digest_grant *g)
{
 size_t i;int r=tick(s,now);if(r)return r;
 if(!g||!s->enabled||!connection||!one_scope(scope)||!(scope&s->scope_mask)||g->account!=s->account||
    g->generation!=s->generation||g->connection!=connection||g->scope!=scope)return CAMERA_DIGEST_DENIED;
 for(i=0;i<CAMERA_DIGEST_NONCES;i++) {
  const struct camera_digest_nonce *n=&s->nonces[i];
  if(n->used&&n->last_nc&&g->serial==n->serial&&g->expires_ms==n->expires_ms&&
     n->generation==s->generation&&n->scope==scope&&n->connection==connection)return CAMERA_DIGEST_OK;
 }
 return CAMERA_DIGEST_STALE;
}
void camera_rtsp_digest_close(struct camera_rtsp_digest *s,uint64_t connection)
{
 size_t i;if(!s||!s->initialized||!connection)return;
 for(i=0;i<CAMERA_DIGEST_NONCES;i++)if(s->nonces[i].used&&s->nonces[i].connection==connection)mbedtls_platform_zeroize(&s->nonces[i],sizeof(s->nonces[i]));
}
void camera_rtsp_digest_destroy(struct camera_rtsp_digest *s)
{if(s)mbedtls_platform_zeroize(s,sizeof(*s));}
