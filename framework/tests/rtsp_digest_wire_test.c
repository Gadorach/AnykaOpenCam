/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <camera/rtsp_digest_wire.h>
#include <camera/rtsp_input.h>
#include <camera/access_policy.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha256.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/platform_util.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>

static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){printf("HOST_ONLY FAIL check=%u line=%u\n",checks,(unsigned)__LINE__);return 1;}} while(0)
static const char live[]="rtsp://publicdummy.invalid/live",track[]="rtsp://publicdummy.invalid/track0";
static const char device[CAMERA_ID_BYTES]="PUBLICDUMMY-device",account_id[CAMERA_ID_BYTES]="PUBLICDUMMY-rtsp";
static struct camera_digest_span S(const char *p)
{struct camera_digest_span s={(const unsigned char *)p,strlen(p)};return s;}
static size_t hash_bytes(unsigned alg){return alg==CAMERA_DIGEST_SHA256?32U:16U;}
static int rng(void *ctx,unsigned char *out,size_t n)
{if(ctx&&*(int *)ctx)return -1;return getrandom(out,n,0)==(ssize_t)n?0:-1;} /* HOST OS CSPRNG, not target entropy */
static void hex(const unsigned char *in,size_t n,char *out)
{size_t i;static const char digits[]="0123456789abcdef";for(i=0;i<n;i++){out[2*i]=digits[in[i]>>4];out[2*i+1]=digits[in[i]&15];}out[2*n]=0;}
static int same_secret(const void *a,const void *b,size_t n){return mbedtls_ct_memcmp(a,b,n)==0;}
struct fixture {
 struct camera_rtsp_digest core;struct camera_digest_wire_adapter wire;
 struct camera_digest_wire_limit global,account,peer;
 struct camera_access_policy policy;struct camera_access_principal principal;
 struct camera_digest_grant grant;struct camera_rtsp_registry registry;
 uint64_t session_generation;unsigned active,selected,playing,effects,media;int random_fail;
 unsigned char ha1[32],nonce[64];
};
static int setup(struct fixture *f,unsigned alg)
{
 static const struct camera_access_resource resources[]={
  {CAMERA_ACCESS_DEVICE,"PUBLICDUMMY-device",""},{CAMERA_ACCESS_SENSOR,"camera0",""},
  {CAMERA_ACCESS_PROFILE,"camera0-main","camera0"},{CAMERA_ACCESS_PROFILE,"camera0-sub","camera0"}};
 struct camera_access_account a={0};unsigned char reply[512];size_t n;
 memset(f,0,sizeof(*f));
 if(camera_rtsp_digest_ha1(alg,S("PUBLICDUMMY"),S("PUBLICDUMMY RTSP"),S("PUBLICDUMMY-not-product"),f->ha1))return -1;
 if(camera_rtsp_digest_init(&f->core,alg,S("PUBLICDUMMY RTSP"),1000,100,rng,&f->random_fail))return -1;
 if(camera_rtsp_digest_provision(&f->core,7,S("PUBLICDUMMY"),f->ha1,hash_bytes(alg),1))return -1;
 if(camera_digest_wire_limit_init(&f->global,100,1000,1024,1024)||camera_digest_wire_limit_init(&f->account,100,1000,1024,1024)||
    camera_digest_wire_limit_init(&f->peer,100,1000,1024,1024))return -1;
 if(camera_digest_wire_adapter_init(&f->wire,&f->core,&f->global,&f->account))return -1;
 if(camera_digest_wire_challenge(&f->wire,&f->peer,11,1,100,1,CAMERA_DIGEST_WIRE_INITIAL,reply,sizeof(reply),&n))return -1;
 memcpy(f->nonce,f->core.nonces[0].nonce,64);
 if(camera_access_init(&f->policy,device,42,100000000,resources,4))return -1;
 a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_RTSP;a.roles=CAMERA_ACCESS_VIEWER;
 memcpy(a.account_id,account_id,CAMERA_ID_BYTES);a.credential_generation=f->core.generation;a.grant_count=1;
 a.grants[0].kind=CAMERA_ACCESS_PROFILE;a.grants[0].operations=CAMERA_ACCESS_LIVE;memcpy(a.grants[0].resource_id,"camera0-main",12);
 if(camera_access_put_account(&f->policy,&a)||camera_rtsp_registry_init(&f->registry,live,track))return -1;
 return 0;
}
/* Independent one-shot Mbed TLS client formulation, never core response helper.
 * All inputs PUBLICDUMMY; no fixture credential/nonce/response value is printed. */
static int client(struct fixture *f,const char *method,const char *uri,const char *cnonce,const char *nc,char response[65])
{
 unsigned char a2[32],digest[32];char h1[65],h2[65],joined[1024];int n,r;size_t bytes=hash_bytes(f->core.algorithm);
 n=snprintf(joined,sizeof(joined),"%s:%s",method,uri);if(n<0||(size_t)n>=sizeof(joined))return -1;
 r=f->core.algorithm==CAMERA_DIGEST_SHA256?mbedtls_sha256((const unsigned char *)joined,(size_t)n,a2,0):mbedtls_md5((const unsigned char *)joined,(size_t)n,a2);
 if(r)return r;
 hex(f->ha1,bytes,h1);hex(a2,bytes,h2);
 n=snprintf(joined,sizeof(joined),"%s:%.*s:%s:%s:auth:%s",h1,64,f->nonce,nc,cnonce,h2);
 if(n<0||(size_t)n>=sizeof(joined))return -1;
 r=f->core.algorithm==CAMERA_DIGEST_SHA256?mbedtls_sha256((const unsigned char *)joined,(size_t)n,digest,0):mbedtls_md5((const unsigned char *)joined,(size_t)n,digest);
 if(!r)hex(digest,bytes,response);
 mbedtls_platform_zeroize(a2,sizeof(a2));mbedtls_platform_zeroize(digest,sizeof(digest));mbedtls_platform_zeroize(h1,sizeof(h1));
 mbedtls_platform_zeroize(h2,sizeof(h2));mbedtls_platform_zeroize(joined,sizeof(joined));return r;
}
static int auth(struct fixture *f,const char *method,const char *uri,const char *cnonce,const char *nc,
 int skip,int duplicate,char *out,size_t capacity)
{
 static const char *const names[9]={"username","realm","uri","algorithm","qop","nonce","cnonce","nc","response"};
 const char *v[9];char nonce[65],response[65];size_t used=0;int i,n,first=1;
 if(client(f,method,uri,cnonce,nc,response))return -1;
 memcpy(nonce,f->nonce,64);nonce[64]=0;
 v[0]="PUBLICDUMMY";v[1]="PUBLICDUMMY RTSP";v[2]=uri;v[3]=f->core.algorithm==CAMERA_DIGEST_SHA256?"SHA-256":"MD5";
 v[4]="auth";v[5]=nonce;v[6]=cnonce;v[7]=nc;v[8]=response;
 n=snprintf(out,capacity,"Digest ");if(n<0||(size_t)n>=capacity)return -1;used=(size_t)n;
 for(i=0;i<10;i++) {
  int id=i==9?duplicate:i;if(id<0||id==skip)continue;
  n=snprintf(out+used,capacity-used,"%s%s=\"%s\"",first?"":", ",names[id],v[id]);
  if(n<0||(size_t)n>=capacity-used)return -1;
  used+=(size_t)n;first=0;
 }
 mbedtls_platform_zeroize(response,sizeof(response));return (int)used;
}
static int request(char *out,size_t capacity,const char *method,const char *uri,const char *authorization,const char *extra)
{
 int n=snprintf(out,capacity,"%s %s RTSP/1.0\r\nCSeq: 1\r\n%s%s%s%s\r\n",method,uri,
  authorization?"aUtHoRiZaTiOn: ":"",authorization?authorization:"",authorization?"\r\n":"",extra?extra:"");
 return n<0||(size_t)n>=capacity?-1:n;
}
static int replace(char *buffer,size_t capacity,const char *from,const char *to)
{
 char *p=strstr(buffer,from);size_t head,tail,n=strlen(to);if(!p)return -1;
 head=(size_t)(p-buffer);tail=strlen(p+strlen(from));if(head+n+tail>=capacity)return -1;
 memmove(p+n,p+strlen(from),tail+1);memcpy(p,to,n);return 0;
}
static struct camera_access_request query(void)
{
 struct camera_access_request q={0};q.abi=CAMERA_ACCESS_ABI;q.struct_bytes=sizeof(q);q.domain=CAMERA_ACCESS_RTSP;
 q.kind=CAMERA_ACCESS_PROFILE;q.operation=CAMERA_ACCESS_LIVE;memcpy(q.device_id,device,CAMERA_ID_BYTES);
 memcpy(q.sensor_id,"camera0",8);memcpy(q.profile_id,"camera0-main",13);return q;
}
/* HOST integration proposal fixture, not native server code. It creates the
 * policy fact ONLY after real wire/core verification and explicit graph/role
 * mapping. No incoming verified flag or typed claim can call this effects gate.
 * Reuse one owner principal per connection/nonce; session issuance is strictly
 * monotonic across credential changes, never the request's nc/nonce serial. */
static int gate(struct fixture *f,const struct camera_rtsp_request *r,uint64_t now)
{
 struct camera_digest_grant g;enum camera_digest_wire_reason reason;enum camera_rtsp_method method;
 struct camera_access_request q=query();int result;
 result=camera_digest_wire_verify(&f->wire,&f->peer,11,1,now,r,&g,&reason);if(result)return result;
 if(camera_rtsp_request_policy(r,&f->registry,f->selected,f->playing,"PUBLICDUMMY-session",&method)||
    method==CAMERA_RTSP_METHOD_UNSUPPORTED||g.account!=7||g.scope!=1||g.connection!=11)return CAMERA_DIGEST_WIRE_INVALID;
 if(now>UINT64_MAX/1000000U||g.expires_ms>UINT64_MAX/1000000U)return CAMERA_DIGEST_WIRE_INVALID;
 if(!f->active) {
  struct camera_access_verified fact={0};
  if(f->session_generation==UINT64_MAX)return CAMERA_DIGEST_EXHAUSTED;
  fact.abi=CAMERA_ACCESS_ABI;fact.struct_bytes=sizeof(fact);fact.domain=CAMERA_ACCESS_RTSP;
  memcpy(fact.device_id,device,CAMERA_ID_BYTES);memcpy(fact.account_id,account_id,CAMERA_ID_BYTES);
  fact.boot_generation=42;fact.credential_generation=g.generation;fact.session_generation=++f->session_generation;
  fact.issued_ns=now*1000000U;fact.expires_ns=g.expires_ms*1000000U;fact.verified=1; /* ONLY after crypto above */
  if(camera_access_issue(&f->policy,&fact,now*1000000U,&f->principal))return CAMERA_ACCESS_DENIED;
  f->active=1;
 }
 if(camera_rtsp_digest_check(&f->core,11,1,now,&g)||camera_access_check(&f->policy,&f->principal,&q,now*1000000U))return CAMERA_ACCESS_DENIED;
 f->grant=g;f->effects++; /* Pure HOST counters, no SDP/driver/subscription I/O. */
 if(method==CAMERA_RTSP_METHOD_SETUP)f->selected=1;
 if(method==CAMERA_RTSP_METHOD_PLAY)f->playing=1;
 return 0;
}
static int media_effect(struct fixture *f,uint64_t now,const struct camera_access_request *q)
{
 if(!f->active||!f->playing||now>UINT64_MAX/1000000U||camera_rtsp_digest_check(&f->core,11,1,now,&f->grant)||
    camera_access_check(&f->policy,&f->principal,q,now*1000000U))return CAMERA_ACCESS_DENIED;
 f->media++;return 0; /* Not actual video bytes or native fan-out. */
}
static int parser_profile(unsigned alg)
{
 struct fixture f;struct camera_rtsp_request r;struct camera_digest_wire_decoded d,zero={0};
 char base[1100],bad[1600],wire[8193],big[1100];int n,i;unsigned char reply[512];size_t bytes;
 CHECK(setup(&f,alg)==0);CHECK(auth(&f,"DESCRIBE",live,"PUBLICDUMMY-A","00000001",-1,-1,base,sizeof(base))>0);
 n=request(wire,sizeof(wire),"DESCRIBE",live,base,NULL);CHECK(n>0);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(camera_digest_wire_parse(&r,alg,&d)==0);CHECK(d.fields.uri.data==d.uri&&d.fields.username.data==d.username);
 camera_digest_wire_clear(&d);CHECK(same_secret(&d,&zero,sizeof(d)));
 {
  struct camera_rtsp_request before;size_t prefix;
  memset(&r,0,sizeof(r));memcpy(&before,&r,sizeof(r));
  for(prefix=0;prefix<(size_t)n;prefix++) {
   CHECK(camera_rtsp_request_parse(wire,prefix,&r)==CAMERA_RTSP_NEED_MORE);
   CHECK(same_secret(&before,&r,sizeof(r))); /* no usable view on incomplete data */
  }
 }
 for(i=0;i<9;i++) {
  CHECK(auth(&f,"DESCRIBE",live,"PUBLICDUMMY-A","00000001",i,-1,bad,sizeof(bad))>0);
  n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);
  CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);CHECK(same_secret(&d,&zero,sizeof(d)));
  CHECK(auth(&f,"DESCRIBE",live,"PUBLICDUMMY-A","00000001",-1,i,bad,sizeof(bad))>0);
  n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);
  CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 }
 {
  static const struct {const char *from,*to;} cases[]={
   {"Digest ","Basic "},{"username=","username*= "},{"username=","ABCDEFGHIJKLMNOPQ= "},
   {"qop=\"auth\"","qop=\"auth-int\""},{"qop=\"auth\"","qop=\"AUTH\""},
   {"nc=\"00000001\"","nc=\"00000000\""},{"nc=\"00000001\"","nc=\"0000000g\""},
   {"cnonce=\"PUBLICDUMMY-A\"","cnonce=\"\""},{"cnonce=\"PUBLICDUMMY-A\"","cnonce=\"bad value\""},
   {"username=\"PUBLICDUMMY\"","username=\"PUBLIC:DUMMY\""},
   {"realm=\"PUBLICDUMMY RTSP\"","realm=\"PUBLICDUMMY RTSP"},
   {"cnonce=\"PUBLICDUMMY-A\"","cnonce=\"PUBLICDUMMY\\\"-A\""},
   {"cnonce=\"PUBLICDUMMY-A\"","cnonce=\"PUBLICDUMMY\\\\-A\""},
   {"cnonce=\"PUBLICDUMMY-A\"","cnonce=PUBLICDUMMY/A"},
   {"username=\"PUBLICDUMMY\"","username=\"PUBLICDUMMY\" junk"},
   {"username=\"PUBLICDUMMY\"","username=\"PUBLICDUMMY\",, "},
  };size_t j;
  for(j=0;j<sizeof(cases)/sizeof(cases[0]);j++) {
   strcpy(bad,base);CHECK(replace(bad,sizeof(bad),cases[j].from,cases[j].to)==0);
   n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);
   CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);CHECK(same_secret(&d,&zero,sizeof(d)));
  }
 }
 strcpy(bad,base);CHECK(replace(bad,sizeof(bad),alg==CAMERA_DIGEST_SHA256?"SHA-256":"MD5",alg==CAMERA_DIGEST_SHA256?"MD5":"SHA-256")==0);
 n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 strcpy(bad,base);CHECK(replace(bad,sizeof(bad),alg==CAMERA_DIGEST_SHA256?"SHA-256":"MD5",alg==CAMERA_DIGEST_SHA256?"SHA-256-sess":"MD5-sess")==0);
 n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 strcpy(bad,base);strcat(bad,", userhash=false");n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 strcpy(bad,base);strcat(bad,", opaque=PUBLICDUMMY");n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 strcpy(bad,base);strcat(bad,", verified=1");n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 strcpy(bad,base);strcat(bad,", method=PLAY");n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 strcpy(bad,base);strcat(bad,",");n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 strcpy(bad,base);strcat(bad,"\\");n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 {
  static const char *const names[]={"username","realm","uri","algorithm","qop","nonce","cnonce","nc","response"};
  static const size_t caps[]={64,64,512,16,8,64,64,8,64};size_t j;
  char from[620],to[620],value[514],key[32];
  for(j=0;j<9;j++) {
   char *start,*end;size_t length,cap=j==8&&alg==CAMERA_DIGEST_MD5_LEGACY?32:caps[j];
   strcpy(bad,base);snprintf(key,sizeof(key),"%s=\"",names[j]);start=strstr(bad,key);CHECK(start!=NULL);
   end=strchr(start+strlen(key),'"');CHECK(end!=NULL);length=(size_t)(end+1-start);CHECK(length<sizeof(from));
   memcpy(from,start,length);from[length]=0;memset(value,'X',cap+1);value[cap+1]=0;
   snprintf(to,sizeof(to),"%s=\"%s\"",names[j],value);CHECK(replace(bad,sizeof(bad),from,to)==0);
   n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);
   CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID&&same_secret(&d,&zero,sizeof(d)));
  }
 }
 memset(big,'X',1025);big[1025]=0;n=request(wire,sizeof(wire),"DESCRIBE",live,big,NULL);
 CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_INVALID);
 n=request(wire,sizeof(wire),"DESCRIBE",live,base,"Authorization: Basic PUBLICDUMMY\r\n");CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_INVALID);
 {
  char headers[2048],name[67];size_t used=0;int j;
  memset(name,'X',65);name[65]=0;snprintf(headers,sizeof(headers),"%s: PUBLICDUMMY\r\n",name);
  n=request(wire,sizeof(wire),"DESCRIBE",live,base,headers);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_INVALID);
  for(j=0;j<23;j++){int written=snprintf(headers+used,sizeof(headers)-used,"X-%d: PUBLICDUMMY\r\n",j);CHECK(written>0);used+=(size_t)written;}
  n=request(wire,sizeof(wire),"DESCRIBE",live,base,headers);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_INVALID); /* 25 headers */
 }
 n=request(wire,sizeof(wire),"DESCRIBE","rtsp://PUBLICDUMMY@publicdummy.invalid/live",base,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_INVALID);
 n=request(wire,sizeof(wire),"DESCRIBE","rtsp://publicdummy%40invalid/live",base,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_INVALID);
 n=request(wire,sizeof(wire),"DESCRIBE",track,base,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 n=request(wire,sizeof(wire),"DESCRIBE",live,base,"Content-Length: 1\r\n");wire[n++]='\0';CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);CHECK(camera_digest_wire_parse(&r,alg,&d)==CAMERA_DIGEST_WIRE_INVALID);
 memset(wire,'X',sizeof(wire));CHECK(camera_rtsp_request_parse(wire,sizeof(wire),&r)==CAMERA_RTSP_INVALID);
 for(i=0;i<33;i++) {
  n=request(wire,sizeof(wire),"DESCRIBE",live,base,NULL);wire[100]=(char)(i==32?127:i);
  CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_INVALID);
 }
 /* Bounded legitimate quoted-pair and token/BWS values, name/scheme/header CI.
  * No case rewriting of hashed qop/cnonce/nc/URI. */
 strcpy(bad,base);CHECK(replace(bad,sizeof(bad),"Digest ","dIgEsT ")==0);
 CHECK(replace(bad,sizeof(bad),"username=\"PUBLICDUMMY\"","UsErNaMe = \"PUBLIC\\DUMMY\"")==0);
 CHECK(replace(bad,sizeof(bad),"qop=\"auth\"","QoP = auth")==0);
 CHECK(replace(bad,sizeof(bad),"nc=\"00000001\"","NC = 00000001")==0);
 CHECK(replace(bad,sizeof(bad),"cnonce=\"PUBLICDUMMY-A\"","CNoNcE=PUBLICDUMMY-A")==0);
 {
  char from[100],to[100],response[65];CHECK(client(&f,"DESCRIBE",live,"PUBLICDUMMY-A","00000001",response)==0);
  snprintf(from,sizeof(from),"response=\"%s\"",response);snprintf(to,sizeof(to),"ReSpOnSe=%s",response);
  CHECK(replace(bad,sizeof(bad),from,to)==0);mbedtls_platform_zeroize(response,sizeof(response));
 }
 CHECK(replace(bad,sizeof(bad),alg==CAMERA_DIGEST_SHA256?"algorithm=\"SHA-256\"":"algorithm=\"MD5\"",alg==CAMERA_DIGEST_SHA256?"AlGoRiThM=SHA-256":"AlGoRiThM=MD5")==0);
 CHECK(replace(bad,sizeof(bad),alg==CAMERA_DIGEST_SHA256?"SHA-256":"MD5",alg==CAMERA_DIGEST_SHA256?"sHa-256":"mD5")==0);
 n=request(wire,sizeof(wire),"DESCRIBE",live,bad,NULL);CHECK(camera_rtsp_request_parse(wire,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(camera_digest_wire_parse(&r,alg,&d)==0);camera_digest_wire_clear(&d);
 CHECK(gate(&f,&r,100)==0&&f.effects==1);
 CHECK(camera_digest_wire_format_challenge(&f.core,f.nonce,UINT32_MAX,CAMERA_DIGEST_WIRE_UNKNOWN_NONCE,reply,sizeof(reply),&bytes)==0);
 CHECK(bytes<512&&strstr((char *)reply,"stale=false")&&strstr((char *)reply,"CSeq: 4294967295")&&strstr((char *)reply,"Content-Length: 0\r\n\r\n"));
 CHECK(!strstr((char *)reply,"Authorization:")&&!strstr((char *)reply,live)&&!strstr((char *)reply,"response="));
 CHECK(camera_digest_wire_format_challenge(&f.core,f.nonce,1,CAMERA_DIGEST_WIRE_NO_CHALLENGE,reply,sizeof(reply),&bytes)==CAMERA_DIGEST_WIRE_INVALID&&bytes==0);
 camera_rtsp_digest_destroy(&f.core);camera_digest_wire_adapter_clear(&f.wire);mbedtls_platform_zeroize(f.ha1,sizeof(f.ha1));return 0;
}
static int integration_profile(unsigned alg)
{
 struct fixture f;struct camera_rtsp_request r;struct camera_rtsp_input input;
 struct camera_access_request q=query();char authorization[1025],buffer[8192],pipeline[8192];
 const char *methods[]={"DESCRIBE","SETUP","PLAY"};const char *uris[]={live,track,live};
 const char *extra[]={NULL,"Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n","Session: PUBLICDUMMY-session\r\n"};
 const char *counts[]={"00000001","00000002","00000003"};const char *cnonces[]={"PUBLICDUMMY-A","PUBLICDUMMY/B","PUBLICDUMMY-C"};
 size_t total=0,consumed,i;int n;enum camera_digest_wire_reason reason;struct camera_digest_grant g;
 CHECK(setup(&f,alg)==0);
 for(i=0;i<3;i++) {
  unsigned char original[32];
  n=request(buffer,sizeof(buffer),methods[i],uris[i],NULL,extra[i]);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
  CHECK(gate(&f,&r,100)==CAMERA_DIGEST_WIRE_MISSING&&!f.active&&!f.effects&&!f.media&&!f.selected&&!f.playing);
  memcpy(original,f.ha1,32);
  CHECK(camera_rtsp_digest_ha1(alg,S("PUBLICDUMMY"),S("PUBLICDUMMY RTSP"),S("PUBLICDUMMY-wrong-password"),f.ha1)==0);
  CHECK(auth(&f,methods[i],uris[i],cnonces[i],counts[0],-1,-1,authorization,sizeof(authorization))>0);
  memcpy(f.ha1,original,32);mbedtls_platform_zeroize(original,sizeof(original)); /* core credential never changed */
  n=request(buffer,sizeof(buffer),methods[i],uris[i],authorization,extra[i]);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
  CHECK(gate(&f,&r,100)==CAMERA_DIGEST_DENIED&&!f.active&&!f.effects&&!f.media&&!f.selected&&!f.playing&&!f.core.nonces[0].last_nc);
 }
 CHECK(auth(&f,"DESCRIBE",live,"PUBLICDUMMY-A","00000001",-1,-1,authorization,sizeof(authorization))>0);
 {char *p=strstr(authorization,"response=\"");CHECK(p!=NULL);p+=10;*p=*p=='a'?'b':'a';}
 n=request(buffer,sizeof(buffer),"DESCRIBE",live,authorization,NULL);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(gate(&f,&r,100)==CAMERA_DIGEST_DENIED&&!f.active&&!f.effects&&!f.core.nonces[0].last_nc);
 for(i=0;i<3;i++) {
  CHECK(auth(&f,methods[i],uris[i],cnonces[i],counts[i],-1,-1,authorization,sizeof(authorization))>0);
  n=request(buffer,sizeof(buffer),methods[i],uris[i],authorization,extra[i]);CHECK(n>0&&total+(size_t)n<sizeof(pipeline));
  memcpy(pipeline+total,buffer,(size_t)n);total+=(size_t)n;
 }
 CHECK(camera_rtsp_input_init(&input,100,2000)==0);
 for(i=0;i<20;i++) {
  CHECK(camera_rtsp_input_append(&input,pipeline+i,1,100)==0);
  CHECK(camera_rtsp_input_next(&input,0,100,&r,&consumed)==CAMERA_RTSP_INPUT_NEED_MORE);
  CHECK(!f.effects&&!f.active);
 }
 CHECK(camera_rtsp_input_append(&input,pipeline+20,total-20,100)==0);
 for(i=0;i<3;i++) {
  CHECK(camera_rtsp_input_next(&input,f.selected,100,&r,&consumed)==CAMERA_RTSP_INPUT_REQUEST);
  CHECK(consumed==r.consumed&&consumed<=input.used&&r.authorization.length<=1024);
  CHECK(gate(&f,&r,100)==0&&f.effects==i+1);
  CHECK(camera_rtsp_input_retire(&input,consumed,100)==0);
 }
 CHECK(input.used==0&&f.active&&f.selected&&f.playing&&f.core.nonces[0].last_nc==3);
 CHECK(media_effect(&f,100,&q)==0&&f.media==1);
 q.domain=CAMERA_ACCESS_UI;CHECK(media_effect(&f,100,&q)==CAMERA_ACCESS_DENIED&&f.media==1);q=query();
 memcpy(q.sensor_id,"camera1",8);CHECK(media_effect(&f,100,&q)==CAMERA_ACCESS_DENIED&&f.media==1);q=query();
 memcpy(q.profile_id,"camera0-sub",12);CHECK(media_effect(&f,100,&q)==CAMERA_ACCESS_DENIED&&f.media==1);q=query();
 q.operation=CAMERA_ACCESS_SNAPSHOT;CHECK(media_effect(&f,100,&q)==CAMERA_ACCESS_DENIED&&f.media==1);q=query();
 /* Correctly hashed equal/lower nc with another fresh cnonce is still replay. */
 CHECK(auth(&f,"PLAY",live,"PUBLICDUMMY-D","00000003",-1,-1,authorization,sizeof(authorization))>0);
 n=request(buffer,sizeof(buffer),"PLAY",live,authorization,extra[2]);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(gate(&f,&r,100)==CAMERA_DIGEST_REPLAY&&f.effects==3&&f.core.nonces[0].last_nc==3);
 CHECK(auth(&f,"PLAY",live,"PUBLICDUMMY-D","00000004",-1,-1,authorization,sizeof(authorization))>0);
 n=request(buffer,sizeof(buffer),"PAUSE",live,authorization,extra[2]);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(gate(&f,&r,100)==CAMERA_DIGEST_DENIED&&f.effects==3&&f.core.nonces[0].last_nc==3);
 n=request(buffer,sizeof(buffer),"PLAY",live,authorization,extra[2]);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,12,1,100,&r,&g,&reason)==CAMERA_DIGEST_DENIED);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,2,100,&r,&g,&reason)==CAMERA_DIGEST_DENIED);
 CHECK(gate(&f,&r,100)==0&&f.effects==4&&f.core.nonces[0].last_nc==4);
 /* Per-effect policy revoke independently blocks media, without fake auth. */
 CHECK(camera_access_revoke_account(&f.policy,CAMERA_ACCESS_RTSP,account_id)==0);
 CHECK(media_effect(&f,100,&q)==CAMERA_ACCESS_DENIED&&f.media==1);
 CHECK(camera_rtsp_digest_invalidate(&f.core)==0);CHECK(media_effect(&f,100,&q)==CAMERA_ACCESS_DENIED&&f.media==1);
 camera_rtsp_input_clear(&input);camera_rtsp_digest_destroy(&f.core);camera_digest_wire_adapter_clear(&f.wire);
 CHECK(setup(&f,alg)==0);CHECK(auth(&f,"DESCRIBE",live,"PUBLICDUMMY-A","00000001",-1,-1,authorization,sizeof(authorization))>0);
 n=request(buffer,sizeof(buffer),"DESCRIBE",live,authorization,NULL);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(gate(&f,&r,100)==0);f.playing=1;CHECK(media_effect(&f,1099,&q)==0);
 CHECK(media_effect(&f,1100,&q)==CAMERA_ACCESS_DENIED); /* core tick erases expired slot */
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,1100,&r,&g,&reason)==CAMERA_DIGEST_STALE&&reason==CAMERA_DIGEST_WIRE_EXPIRED_NONCE);
 {unsigned char reply[512];size_t bytes;CHECK(camera_digest_wire_challenge(&f.wire,&f.peer,11,1,1100,1,reason,reply,sizeof(reply),&bytes)==0);CHECK(strstr((char *)reply,"stale=true")!=NULL);}
 camera_rtsp_digest_destroy(&f.core);camera_digest_wire_adapter_clear(&f.wire);mbedtls_platform_zeroize(f.ha1,sizeof(f.ha1));return 0;
}
static int admission(void)
{
 struct camera_digest_wire_limit g,a,p,other,before;unsigned i;
 CHECK(camera_digest_wire_limit_init(&g,100,1000,3,3)==0);CHECK(camera_digest_wire_limit_init(&a,100,1000,2,2)==0);
 CHECK(camera_digest_wire_limit_init(&p,100,1000,1,1)==0);CHECK(camera_digest_wire_limit_init(&other,100,1000,4,4)==0);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,100)==0);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,100)==CAMERA_DIGEST_WIRE_THROTTLED&&g.verifies==1&&a.verifies==1&&p.verifies==1);
 CHECK(camera_digest_wire_admit(&g,&a,&other,CAMERA_DIGEST_WIRE_VERIFY,100)==0);
 CHECK(camera_digest_wire_admit(&g,&a,&other,CAMERA_DIGEST_WIRE_VERIFY,100)==CAMERA_DIGEST_WIRE_THROTTLED&&g.verifies==2&&a.verifies==2&&other.verifies==1);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_CHALLENGE,100)==0);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_CHALLENGE,1099)==CAMERA_DIGEST_WIRE_THROTTLED);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_CHALLENGE,1100)==0&&g.challenges==1&&a.challenges==1&&p.challenges==1);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,1100)==0);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,1099)==CAMERA_DIGEST_WIRE_CLOCK&&g.clock_failed&&a.clock_failed&&p.clock_failed);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,2100)==CAMERA_DIGEST_WIRE_CLOCK);
 CHECK(camera_digest_wire_limit_init(&g,UINT64_MAX-1,1,1024,1024)==0);CHECK(camera_digest_wire_limit_init(&a,UINT64_MAX-1,1,1024,1024)==0);
 CHECK(camera_digest_wire_limit_init(&p,UINT64_MAX-1,1,1024,1024)==0);
 for(i=0;i<1024;i++)CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,UINT64_MAX-1)==0);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,UINT64_MAX-1)==CAMERA_DIGEST_WIRE_THROTTLED&&g.verifies==1024);
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,UINT64_MAX)==0&&g.verifies==1);
 before=g;CHECK(camera_digest_wire_admit(&g,&g,&p,CAMERA_DIGEST_WIRE_VERIFY,UINT64_MAX)==CAMERA_DIGEST_WIRE_INVALID&&!memcmp(&before,&g,sizeof(g)));
 g.verifies=UINT32_MAX; /* owner-counter edge fixture, not editable public input */
 CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,UINT64_MAX)==CAMERA_DIGEST_WIRE_THROTTLED&&g.verifies==UINT32_MAX);
 CHECK(camera_digest_wire_limit_init(&g,100,0,1,1)==CAMERA_DIGEST_WIRE_INVALID);
 CHECK(camera_digest_wire_limit_init(&g,100,60001,1,1)==CAMERA_DIGEST_WIRE_INVALID);
 CHECK(camera_digest_wire_limit_init(&g,100,1000,0,1)==CAMERA_DIGEST_WIRE_INVALID);
 CHECK(camera_digest_wire_limit_init(&g,100,1000,1,1025)==CAMERA_DIGEST_WIRE_INVALID);
 CHECK(camera_rtsp_input_retire(NULL,1,100)==-1); /* unchanged dependency guard */
 {
  struct camera_digest_wire_limit a2,p2;
  CHECK(camera_digest_wire_limit_init(&g,100,1000,2,2)==0);CHECK(camera_digest_wire_limit_init(&a,100,1000,4,4)==0);
  CHECK(camera_digest_wire_limit_init(&p,100,1000,4,4)==0);CHECK(camera_digest_wire_limit_init(&a2,100,1000,4,4)==0);
  CHECK(camera_digest_wire_limit_init(&p2,100,1000,4,4)==0);
  CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_VERIFY,100)==0);
  CHECK(camera_digest_wire_admit(&g,&a2,&p2,CAMERA_DIGEST_WIRE_VERIFY,100)==0);
  CHECK(camera_digest_wire_admit(&g,&a2,&p2,CAMERA_DIGEST_WIRE_VERIFY,100)==CAMERA_DIGEST_WIRE_THROTTLED&&g.verifies==2&&a2.verifies==1&&p2.verifies==1);
  CHECK(camera_digest_wire_admit(&g,&a,&p,CAMERA_DIGEST_WIRE_CHALLENGE,100)==0);
  CHECK(camera_digest_wire_admit(&g,&a2,&p2,CAMERA_DIGEST_WIRE_CHALLENGE,100)==0);
  CHECK(camera_digest_wire_admit(&g,&a2,&p2,CAMERA_DIGEST_WIRE_CHALLENGE,100)==CAMERA_DIGEST_WIRE_THROTTLED&&g.challenges==2&&a2.challenges==1&&p2.challenges==1);
 }
 return 0;
}
static int failures_and_challenges(unsigned alg)
{
 struct fixture f;struct camera_rtsp_request r;struct camera_digest_grant grant,zero={0};
 enum camera_digest_wire_reason reason;char authorization[1025],buffer[8192],saved[1025];
 unsigned char reply[512],none[512]={0};size_t bytes;int n;uint64_t serial;unsigned count,i;
 CHECK(setup(&f,alg)==0);serial=f.core.serial;count=f.global.challenges;
 CHECK(camera_digest_wire_challenge(&f.wire,&f.peer,11,1,100,1,CAMERA_DIGEST_WIRE_INITIAL,reply,16,&bytes)==CAMERA_DIGEST_WIRE_SPACE);
 CHECK(bytes==0&&!memcmp(reply,none,16)&&f.core.serial==serial&&f.global.challenges==count);
 CHECK(camera_digest_wire_challenge(&f.wire,&f.peer,11,1,100,1,CAMERA_DIGEST_WIRE_NO_CHALLENGE,reply,sizeof(reply),&bytes)==CAMERA_DIGEST_WIRE_INVALID);
 CHECK(f.core.serial==serial&&f.global.challenges==count);
 f.random_fail=1;
 CHECK(camera_digest_wire_challenge(&f.wire,&f.peer,11,1,100,1,CAMERA_DIGEST_WIRE_INITIAL,reply,sizeof(reply),&bytes)==CAMERA_DIGEST_RANDOM_FAILED);
 CHECK(bytes==0&&!memcmp(reply,none,sizeof(reply))&&f.core.serial==serial&&f.global.challenges==count+1);f.random_fail=0;
 for(i=1;i<CAMERA_DIGEST_NONCES;i++)CHECK(camera_digest_wire_challenge(&f.wire,&f.peer,11+i,1,100,1,CAMERA_DIGEST_WIRE_INITIAL,reply,sizeof(reply),&bytes)==0);
 CHECK(camera_digest_wire_challenge(&f.wire,&f.peer,100,1,100,1,CAMERA_DIGEST_WIRE_INITIAL,reply,sizeof(reply),&bytes)==CAMERA_DIGEST_FULL);
 CHECK(bytes==0&&!memcmp(reply,none,sizeof(reply)));
 CHECK(auth(&f,"DESCRIBE",live,"PUBLICDUMMY-A","00000001",-1,-1,authorization,sizeof(authorization))>0);strcpy(saved,authorization);
 {
  char *p=strstr(authorization,"nonce=\"");CHECK(p!=NULL);p+=7;*p=*p=='a'?'b':'a';
 }
 n=request(buffer,sizeof(buffer),"DESCRIBE",live,authorization,NULL);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,100,&r,&grant,&reason)==CAMERA_DIGEST_STALE&&reason==CAMERA_DIGEST_WIRE_UNKNOWN_NONCE);
 CHECK(!memcmp(&grant,&zero,sizeof(grant))&&!f.core.nonces[0].last_nc);
 CHECK(camera_digest_wire_format_challenge(&f.core,f.nonce,1,reason,reply,sizeof(reply),&bytes)==0&&strstr((char *)reply,"stale=false"));
 strcpy(authorization,saved);CHECK(replace(authorization,sizeof(authorization),"username=\"PUBLICDUMMY\"","username=\"PUBLICDUMMY-other\"")==0);
 n=request(buffer,sizeof(buffer),"DESCRIBE",live,authorization,NULL);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,1100,&r,&grant,&reason)==CAMERA_DIGEST_DENIED&&reason==CAMERA_DIGEST_WIRE_BAD);
 /* Expired known nonce, but wrong identity/scope never gives stale=true. */
 n=request(buffer,sizeof(buffer),"DESCRIBE",live,saved,NULL);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,12,1,1100,&r,&grant,&reason)==CAMERA_DIGEST_STALE&&reason==CAMERA_DIGEST_WIRE_UNKNOWN_NONCE);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,1100,&r,&grant,&reason)==CAMERA_DIGEST_STALE&&reason==CAMERA_DIGEST_WIRE_EXPIRED_NONCE);
 camera_digest_wire_close(&f.wire,11);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,1100,&r,&grant,&reason)==CAMERA_DIGEST_STALE&&reason==CAMERA_DIGEST_WIRE_UNKNOWN_NONCE);
 CHECK(camera_rtsp_digest_provision(&f.core,7,S("PUBLICDUMMY"),f.ha1,hash_bytes(alg),1)==0);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,12,1,1100,&r,&grant,&reason)==CAMERA_DIGEST_STALE&&reason==CAMERA_DIGEST_WIRE_UNKNOWN_NONCE);
 /* Malformed requests consume verify attempts, but never consume core nc. */
 CHECK(camera_digest_wire_limit_init(&f.peer,1100,1000,1,1)==0);
 n=request(buffer,sizeof(buffer),"DESCRIBE",live,"Basic PUBLICDUMMY",NULL);CHECK(camera_rtsp_request_parse(buffer,(size_t)n,&r)==CAMERA_RTSP_READY);
 count=f.global.verifies;
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,1100,&r,&grant,&reason)==CAMERA_DIGEST_WIRE_INVALID&&f.global.verifies==count+1);
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,1100,&r,&grant,&reason)==CAMERA_DIGEST_WIRE_THROTTLED&&reason==CAMERA_DIGEST_WIRE_NO_CHALLENGE);
 CHECK(f.global.verifies==count+1&&!memcmp(&grant,&zero,sizeof(grant)));
 CHECK(camera_digest_wire_verify(&f.wire,&f.peer,11,1,1099,&r,&grant,&reason)==CAMERA_DIGEST_WIRE_CLOCK&&reason==CAMERA_DIGEST_WIRE_NO_CHALLENGE);
 camera_rtsp_digest_destroy(&f.core);camera_digest_wire_adapter_clear(&f.wire);mbedtls_platform_zeroize(f.ha1,sizeof(f.ha1));return 0;
}
int main(void)
{
 if(parser_profile(CAMERA_DIGEST_SHA256)||parser_profile(CAMERA_DIGEST_MD5_LEGACY)||
    integration_profile(CAMERA_DIGEST_SHA256)||integration_profile(CAMERA_DIGEST_MD5_LEGACY)||admission()||
    failures_and_challenges(CAMERA_DIGEST_SHA256)||failures_and_challenges(CAMERA_DIGEST_MD5_LEGACY))return 1;
 printf("HOST_ONLY PASS checks=%u profiles=2 wire_directives=9 nonce_notes=8 decoded_bytes=%zu adapter_bytes=%zu\n",checks,sizeof(struct camera_digest_wire_decoded),sizeof(struct camera_digest_wire_adapter));return 0;
}
