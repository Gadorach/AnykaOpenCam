/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/rtsp_digest_wire.h>
#include <mbedtls/platform_util.h>
#include <stdio.h>
#include <string.h>

static unsigned char lower(unsigned char c)
{return c>='A'&&c<='Z'?(unsigned char)(c+32):c;}
static int ci(const unsigned char *p,size_t n,const char *s)
{size_t i;if(!p||n!=strlen(s))return 0;for(i=0;i<n;i++)if(lower(p[i])!=lower((unsigned char)s[i]))return 0;return 1;}
static int token(unsigned char c)
{return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||(c&&strchr("!#$%&'*+-.^_`|~",c));}
static int printable(const unsigned char *p,size_t n,int spaces,int name)
{
 size_t i;if(!p||!n)return 0;
 for(i=0;i<n;i++)if(p[i]<(spaces?32:33)||p[i]>126||p[i]=='"'||p[i]=='\\'||(name&&p[i]==':'))return 0;
 return 1;
}
static int hex_digit(unsigned char c)
{return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');}
static struct camera_digest_span ds(const void *p,size_t n)
{struct camera_digest_span s={(const unsigned char *)p,n};return s;}
static unsigned char *field(struct camera_digest_wire_decoded *d,unsigned i,size_t *capacity)
{
 switch(i) {
 case 0:*capacity=sizeof(d->username);return d->username;
 case 1:*capacity=sizeof(d->realm);return d->realm;
 case 2:*capacity=sizeof(d->uri);return d->uri;
 case 3:*capacity=sizeof(d->algorithm);return d->algorithm;
 case 4:*capacity=sizeof(d->qop);return d->qop;
 case 5:*capacity=sizeof(d->nonce);return d->nonce;
 case 6:*capacity=sizeof(d->cnonce);return d->cnonce;
 case 7:*capacity=sizeof(d->nc);return d->nc;
 default:*capacity=sizeof(d->response);return d->response;
 }
}
void camera_digest_wire_clear(struct camera_digest_wire_decoded *d)
{if(d)mbedtls_platform_zeroize(d,sizeof(*d));}
static int bad(struct camera_digest_wire_decoded *d)
{camera_digest_wire_clear(d);return CAMERA_DIGEST_WIRE_INVALID;}
int camera_digest_wire_parse(const struct camera_rtsp_request *r,uint32_t alg,
 struct camera_digest_wire_decoded *d)
{
 static const char *const names[9]={"username","realm","uri","algorithm","qop","nonce","cnonce","nc","response"};
 const unsigned char *p;size_t n,pos,start,cap,j;unsigned seen=0,count=0,i;unsigned char *dst;
 if(!d)return CAMERA_DIGEST_WIRE_INVALID;
 camera_digest_wire_clear(d);
 if(!r||!r->consumed||r->consumed>CAMERA_RTSP_REQUEST_MAX_BYTES||r->header_count>CAMERA_RTSP_REQUEST_MAX_HEADERS||r->body.length||
    !r->method.length||r->method.length>24||!printable(r->method.data,r->method.length,0,0)||
    !r->target.length||r->target.length>CAMERA_DIGEST_URI_MAX||!printable(r->target.data,r->target.length,0,0)||
    (alg!=CAMERA_DIGEST_SHA256&&alg!=CAMERA_DIGEST_MD5_LEGACY))return bad(d);
 for(j=0;j<r->method.length;j++)if((r->method.data[j]<'A'||r->method.data[j]>'Z')&&r->method.data[j]!='_')return bad(d);
 p=r->authorization.data;n=r->authorization.length;
 if(!n)return CAMERA_DIGEST_WIRE_MISSING;
 if(!p||n>CAMERA_RTSP_REQUEST_MAX_AUTHORIZATION)return bad(d);
 /* Frozen request parser rejects every CTL including HTAB; match that profile. */
 for(j=0;j<n;j++)if(p[j]<32||p[j]>126)return bad(d);
 pos=0;while(pos<n&&token(p[pos]))pos++;
 if(!ci(p,pos,"Digest")||pos==n||p[pos]!=' ')return bad(d);
 while(pos<n&&p[pos]==' ')pos++;
 while(pos<n) {
  if(++count>CAMERA_DIGEST_WIRE_DIRECTIVES)return bad(d);
  start=pos;while(pos<n&&token(p[pos]))pos++;
  if(pos==start||pos-start>CAMERA_DIGEST_WIRE_NAME_MAX)return bad(d);
  for(i=0;i<9;i++)if(ci(p+start,pos-start,names[i]))break;
  if(i==9||(seen&(1U<<i)))return bad(d);
  seen|=1U<<i;
  while(pos<n&&p[pos]==' ')pos++;
  if(pos==n||p[pos++]!='=')return bad(d);
  while(pos<n&&p[pos]==' ')pos++;
  if(pos==n)return bad(d);
  dst=field(d,i,&cap);j=0;
  if(p[pos]=='"') {
   int closed=0;pos++;
   while(pos<n) {
    unsigned char c=p[pos++];
    if(c=='"'){closed=1;break;}
    if(c=='\\'){if(pos==n)return bad(d);c=p[pos++];}
    /* RFC quoted-pair accepts printable ASCII/SP, but decoded values must
     * also meet the existing core's restricted ASCII profile below. */
    if(j==cap)return bad(d);
    dst[j++]=c;
   }
   if(!closed)return bad(d);
  } else {while(pos<n&&token(p[pos])){if(j==cap)return bad(d);dst[j++]=p[pos++];}}
  if(!j)return bad(d);
  d->lengths[i]=j;
  while(pos<n&&p[pos]==' ')pos++;
  if(pos==n)break;
  if(p[pos++]!=',')return bad(d);
  while(pos<n&&p[pos]==' ')pos++;
  if(pos==n)return bad(d);
 }
 if(seen!=511U)return bad(d);
 if(!printable(d->username,d->lengths[0],1,1)||!printable(d->realm,d->lengths[1],1,1)||
    !printable(d->uri,d->lengths[2],0,0)||!printable(d->cnonce,d->lengths[6],0,0)||
    d->lengths[2]!=r->target.length||memcmp(d->uri,r->target.data,r->target.length)||
    d->lengths[4]!=4||memcmp(d->qop,"auth",4)||d->lengths[5]!=64||d->lengths[7]!=8||
    d->lengths[8]!=(alg==CAMERA_DIGEST_SHA256?64U:32U))return bad(d);
 if(!ci(d->algorithm,d->lengths[3],alg==CAMERA_DIGEST_SHA256?"SHA-256":"MD5"))return bad(d);
 for(j=0;j<64;j++)if(!hex_digit(d->nonce[j]))return bad(d);
 for(j=0;j<8;j++)if(!hex_digit(d->nc[j]))return bad(d);
 if(!memcmp(d->nc,"00000000",8))return bad(d);
 for(j=0;j<d->lengths[8];j++)if(!hex_digit(d->response[j]))return bad(d);
 /* Algorithm token is case-insensitive and not hashed; canonicalize only it.
  * qop/nc/cnonce/URI are hashed text, never case-normalized or rewritten. */
 d->lengths[3]=alg==CAMERA_DIGEST_SHA256?7U:3U;memcpy(d->algorithm,alg==CAMERA_DIGEST_SHA256?"SHA-256":"MD5",d->lengths[3]);
 d->fields.username=ds(d->username,d->lengths[0]);d->fields.realm=ds(d->realm,d->lengths[1]);d->fields.uri=ds(d->uri,d->lengths[2]);
 d->fields.algorithm=ds(d->algorithm,d->lengths[3]);d->fields.qop=ds(d->qop,d->lengths[4]);d->fields.nonce=ds(d->nonce,d->lengths[5]);
 d->fields.cnonce=ds(d->cnonce,d->lengths[6]);d->fields.nc=ds(d->nc,d->lengths[7]);d->fields.response=ds(d->response,d->lengths[8]);return CAMERA_DIGEST_WIRE_OK;
}
int camera_digest_wire_limit_init(struct camera_digest_wire_limit *b,uint64_t now,
 uint32_t window,uint32_t verifies,uint32_t challenges)
{
 if(!b)return CAMERA_DIGEST_WIRE_INVALID;
 memset(b,0,sizeof(*b));
 if(!window||window>60000||!verifies||verifies>CAMERA_DIGEST_WIRE_RATE_MAX||!challenges||challenges>CAMERA_DIGEST_WIRE_RATE_MAX)return CAMERA_DIGEST_WIRE_INVALID;
 b->window_start_ms=b->last_ms=now;b->window_ms=window;b->verify_limit=verifies;b->challenge_limit=challenges;b->initialized=1;return CAMERA_DIGEST_WIRE_OK;
}
int camera_digest_wire_admit(struct camera_digest_wire_limit *global,
 struct camera_digest_wire_limit *account,struct camera_digest_wire_limit *peer,
 enum camera_digest_wire_attempt kind,uint64_t now)
{
 struct camera_digest_wire_limit *b[3]={global,account,peer},candidate[3];size_t i;int full=0;
 if(!global||!account||!peer||global==account||global==peer||account==peer||
    (kind!=CAMERA_DIGEST_WIRE_VERIFY&&kind!=CAMERA_DIGEST_WIRE_CHALLENGE))return CAMERA_DIGEST_WIRE_INVALID;
 for(i=0;i<3;i++)if(!b[i]->initialized||!b[i]->window_ms||b[i]->window_ms>60000||
    !b[i]->verify_limit||b[i]->verify_limit>1024||!b[i]->challenge_limit||b[i]->challenge_limit>1024)return CAMERA_DIGEST_WIRE_INVALID;
 for(i=0;i<3;i++)if(b[i]->clock_failed||now<b[i]->last_ms){size_t j;for(j=0;j<3;j++)b[j]->clock_failed=1;return CAMERA_DIGEST_WIRE_CLOCK;}
 for(i=0;i<3;i++) {
  candidate[i]=*b[i];candidate[i].last_ms=now;
  if(now-candidate[i].window_start_ms>=candidate[i].window_ms) {
   candidate[i].window_start_ms=now;candidate[i].verifies=candidate[i].challenges=0;
  }
  if(kind==CAMERA_DIGEST_WIRE_VERIFY) {if(candidate[i].verifies>=candidate[i].verify_limit)full=1;}
  else if(candidate[i].challenges>=candidate[i].challenge_limit)full=1;
 }
 for(i=0;i<3;i++) {
  if(!full){if(kind==CAMERA_DIGEST_WIRE_VERIFY)candidate[i].verifies++;else candidate[i].challenges++;}
  *b[i]=candidate[i];
 }
 return full?CAMERA_DIGEST_WIRE_THROTTLED:CAMERA_DIGEST_WIRE_OK;
}
int camera_digest_wire_adapter_init(struct camera_digest_wire_adapter *a,
 struct camera_rtsp_digest *core,struct camera_digest_wire_limit *global,struct camera_digest_wire_limit *account)
{
 if(!a)return CAMERA_DIGEST_WIRE_INVALID;
 mbedtls_platform_zeroize(a,sizeof(*a));
 if(!core||!core->initialized||!global||!account||global==account||!global->initialized||!account->initialized)return CAMERA_DIGEST_WIRE_INVALID;
 a->core=core;a->global=global;a->account=account;a->initialized=1;return CAMERA_DIGEST_WIRE_OK;
}
int camera_digest_wire_verify(struct camera_digest_wire_adapter *a,struct camera_digest_wire_limit *peer,
 uint64_t connection,uint64_t scope,uint64_t now,const struct camera_rtsp_request *r,
 struct camera_digest_grant *g,enum camera_digest_wire_reason *reason)
{
 struct camera_digest_wire_decoded d;size_t i;int result,expired=0;
 if(!g||!reason)return CAMERA_DIGEST_WIRE_INVALID;
 memset(g,0,sizeof(*g));*reason=CAMERA_DIGEST_WIRE_NO_CHALLENGE;
 if(!a||!a->initialized||!a->core)return CAMERA_DIGEST_WIRE_INVALID;
 result=camera_digest_wire_admit(a->global,a->account,peer,CAMERA_DIGEST_WIRE_VERIFY,now);if(result)return result;
 result=camera_digest_wire_parse(r,a->core->algorithm,&d);
 if(result){*reason=result==CAMERA_DIGEST_WIRE_MISSING?CAMERA_DIGEST_WIRE_INITIAL:CAMERA_DIGEST_WIRE_BAD;return result;}
 if(now>=a->core->last_ms&&!a->core->clock_failed&&a->core->enabled&&
    d.fields.username.length==a->core->username_length&&!memcmp(d.username,a->core->username,a->core->username_length)&&
    d.fields.realm.length==a->core->realm_length&&!memcmp(d.realm,a->core->realm,a->core->realm_length)) {
  for(i=0;i<CAMERA_DIGEST_NONCES;i++) {
   const struct camera_digest_wire_nonce_note *n=&a->notes[i];
   if(n->used&&n->generation==a->core->generation&&n->connection==connection&&n->scope==scope&&
      !memcmp(n->nonce,d.nonce,64)&&now>=n->expires_ms){expired=1;break;}
  }
 }
 result=camera_rtsp_digest_verify(a->core,connection,scope,now,ds(r->method.data,r->method.length),ds(r->target.data,r->target.length),&d.fields,g);
 camera_digest_wire_clear(&d);
 if(!result)*reason=CAMERA_DIGEST_WIRE_NO_CHALLENGE;
 else if(result==CAMERA_DIGEST_STALE)*reason=expired?CAMERA_DIGEST_WIRE_EXPIRED_NONCE:CAMERA_DIGEST_WIRE_UNKNOWN_NONCE;
 else if(result==CAMERA_DIGEST_CLOCK_FAILED||result==CAMERA_DIGEST_EXHAUSTED||result==CAMERA_DIGEST_CRYPTO_FAILED)*reason=CAMERA_DIGEST_WIRE_NO_CHALLENGE;
 else *reason=CAMERA_DIGEST_WIRE_BAD;
 return result;
}
int camera_digest_wire_format_challenge(const struct camera_rtsp_digest *core,
 const unsigned char nonce[64],uint32_t cseq,enum camera_digest_wire_reason reason,
 unsigned char *out,size_t capacity,size_t *bytes)
{
 unsigned char tmp[CAMERA_DIGEST_WIRE_RESPONSE_MAX];size_t i;int n;
 if(!bytes)return CAMERA_DIGEST_WIRE_INVALID;
 *bytes=0;
 if(!out||!capacity||capacity>CAMERA_DIGEST_WIRE_RESPONSE_MAX)return CAMERA_DIGEST_WIRE_INVALID;
 memset(out,0,capacity);
 if(!core||!core->initialized||!core->enabled||core->clock_failed||core->exhausted||!nonce||
    core->realm_length>64||!printable(core->realm,core->realm_length,1,1)||
    (core->algorithm!=CAMERA_DIGEST_SHA256&&core->algorithm!=CAMERA_DIGEST_MD5_LEGACY)||
    reason<CAMERA_DIGEST_WIRE_INITIAL||reason>CAMERA_DIGEST_WIRE_EXPIRED_NONCE)return CAMERA_DIGEST_WIRE_INVALID;
 for(i=0;i<64;i++)if(!((nonce[i]>='0'&&nonce[i]<='9')||(nonce[i]>='a'&&nonce[i]<='f')))return CAMERA_DIGEST_WIRE_INVALID;
 n=snprintf((char *)tmp,sizeof(tmp),"RTSP/1.0 401 Unauthorized\r\nCSeq: %u\r\nWWW-Authenticate: Digest realm=\"%.*s\", algorithm=%s, nonce=\"%.*s\", qop=\"auth\", stale=%s\r\nContent-Length: 0\r\n\r\n",
  cseq,(int)core->realm_length,core->realm,core->algorithm==CAMERA_DIGEST_SHA256?"SHA-256":"MD5",64,nonce,reason==CAMERA_DIGEST_WIRE_EXPIRED_NONCE?"true":"false");
 if(n<0||(size_t)n>=sizeof(tmp)){mbedtls_platform_zeroize(tmp,sizeof(tmp));return CAMERA_DIGEST_WIRE_INVALID;}
 if((size_t)n>=capacity){mbedtls_platform_zeroize(tmp,sizeof(tmp));return CAMERA_DIGEST_WIRE_SPACE;}
 memcpy(out,tmp,(size_t)n+1);*bytes=(size_t)n;mbedtls_platform_zeroize(tmp,sizeof(tmp));return CAMERA_DIGEST_WIRE_OK;
}
int camera_digest_wire_challenge(struct camera_digest_wire_adapter *a,struct camera_digest_wire_limit *peer,
 uint64_t connection,uint64_t scope,uint64_t now,uint32_t cseq,enum camera_digest_wire_reason reason,
 unsigned char *out,size_t capacity,size_t *bytes)
{
 unsigned char tmp[512],nonce[64];size_t n,i;int result;
 if(!bytes)return CAMERA_DIGEST_WIRE_INVALID;
 *bytes=0;
 if(!out||!capacity||capacity>512)return CAMERA_DIGEST_WIRE_INVALID;
 memset(out,0,capacity);
 if(!a||!a->initialized||!a->core)return CAMERA_DIGEST_WIRE_INVALID;
 memset(nonce,'0',64);result=camera_digest_wire_format_challenge(a->core,nonce,cseq,reason,tmp,sizeof(tmp),&n);
 if(result)return result;
 if(n>=capacity)return CAMERA_DIGEST_WIRE_SPACE;
 result=camera_digest_wire_admit(a->global,a->account,peer,CAMERA_DIGEST_WIRE_CHALLENGE,now);if(result)return result;
 result=camera_rtsp_digest_issue(a->core,connection,scope,now,nonce);if(result)return result;
 for(i=0;i<CAMERA_DIGEST_NONCES;i++)if(a->core->nonces[i].used&&!memcmp(a->core->nonces[i].nonce,nonce,64)) {
  struct camera_digest_wire_nonce_note *note=&a->notes[i];
  memcpy(note->nonce,nonce,64);note->generation=a->core->generation;note->connection=connection;
  note->scope=scope;note->expires_ms=a->core->nonces[i].expires_ms;note->used=1;break;
 }
 result=camera_digest_wire_format_challenge(a->core,nonce,cseq,reason,out,capacity,bytes);
 mbedtls_platform_zeroize(tmp,sizeof(tmp));mbedtls_platform_zeroize(nonce,sizeof(nonce));return result;
}
void camera_digest_wire_adapter_clear(struct camera_digest_wire_adapter *a)
{if(a)mbedtls_platform_zeroize(a,sizeof(*a));}
void camera_digest_wire_close(struct camera_digest_wire_adapter *a,uint64_t connection)
{
 size_t i;if(!a||!a->initialized||!a->core||!connection)return;
 camera_rtsp_digest_close(a->core,connection);
 for(i=0;i<CAMERA_DIGEST_NONCES;i++)if(a->notes[i].used&&a->notes[i].connection==connection)mbedtls_platform_zeroize(&a->notes[i],sizeof(a->notes[i]));
}
