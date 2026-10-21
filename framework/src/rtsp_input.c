/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <camera/rtsp_input.h>
#include <string.h>

static int same(struct camera_rtsp_span s,const char *literal)
{size_t n=strlen(literal);return s.length==n&&!memcmp(s.data,literal,n);}
static int ci(struct camera_rtsp_span s,const char *literal)
{
 size_t i,n=strlen(literal);if(s.length!=n)return 0;
 for(i=0;i<n;i++) {
  unsigned char c=s.data[i],d=(unsigned char)literal[i];
  if(c>='A'&&c<='Z')c=(unsigned char)(c+32);
  if(d>='A'&&d<='Z')d=(unsigned char)(d+32);
  if(c!=d)return 0;
 }
 return 1;
}
static int uri_config(const char *uri,size_t *length)
{
 unsigned char wire[CAMERA_RTSP_REQUEST_MAX_TARGET+64U];
 static const char suffix[]=" RTSP/1.0\r\nCSeq: 0\r\n\r\n";
 struct camera_rtsp_request r;size_t n=0,total;
 if(!uri)return 0;
 while(n<=CAMERA_RTSP_REQUEST_MAX_TARGET&&uri[n])n++;
 if(!n||n>CAMERA_RTSP_REQUEST_MAX_TARGET)return 0;
 /* This copied listener is ordinary RTSP/TCP, never a plaintext RTSPS service. */
 struct camera_rtsp_span scheme={(const unsigned char *)uri,7};
 if(n<7||!ci(scheme,"rtsp://"))return 0;
 memcpy(wire,"OPTIONS ",8);memcpy(wire+8,uri,n);
 memcpy(wire+8+n,suffix,sizeof(suffix)-1);total=n+8+sizeof(suffix)-1;
 /* This is trusted public configuration only, never an incoming URL log. */
 if(camera_rtsp_request_parse(wire,total,&r)!=CAMERA_RTSP_READY||r.target_is_star||r.consumed!=total||r.target.length!=n)return 0;
 *length=n;return 1;
}
int camera_rtsp_registry_init(struct camera_rtsp_registry *r,const char *live,const char *track)
{
 size_t a,b;if(!r||!uri_config(live,&a)||!uri_config(track,&b)||!strcmp(live,track))return -1;
 memset(r,0,sizeof(*r));memcpy(r->live_uri,live,a);memcpy(r->track_uri,track,b);return 0;
}
static int fail(struct camera_rtsp_input *r,enum camera_rtsp_input_failure code)
{if(r&&r->failure==CAMERA_RTSP_INPUT_NONE)r->failure=code;return -1;}
int camera_rtsp_input_init(struct camera_rtsp_input *r,uint64_t now,uint32_t budget)
{
 if(!r||now==UINT64_MAX||(budget&&(budget<500U||budget>5000U)))return -1;
 memset(r,0,sizeof(*r));r->last_ms=r->started_ms=now;r->deadline_active=1;
 r->deadline_ms=budget?budget:CAMERA_RTSP_REQUEST_DEADLINE_DEFAULT_MS;return 0;
}
int camera_rtsp_input_tick(struct camera_rtsp_input *r,uint64_t now)
{
 if(!r||r->failure)return -1;
 if(now==UINT64_MAX||now<r->last_ms)return fail(r,CAMERA_RTSP_INPUT_CLOCK);
 r->last_ms=now;
 if(r->deadline_active&&now-r->started_ms>=r->deadline_ms)return fail(r,CAMERA_RTSP_INPUT_DEADLINE);
 return 0;
}
int camera_rtsp_input_append(struct camera_rtsp_input *r,const void *p,size_t n,uint64_t now)
{
 if(camera_rtsp_input_tick(r,now))return -1;
 if((!p&&n)||n>sizeof(r->data)-r->used)return fail(r,CAMERA_RTSP_INPUT_FRAMING);
 if(n&&!r->deadline_active){r->started_ms=now;r->deadline_active=1;}
 if(n)memcpy(r->data+r->used,p,n);
 r->used+=n;return 0;
}
static size_t be16(const unsigned char *p){return ((size_t)p[0]<<8)|p[1];}
static int rtcp(const unsigned char *p,size_t n)
{
 size_t pos=0,packets=0;
 while(pos<n) {
  size_t bytes,count,q,chunk;unsigned int type;
  if(n-pos<4||p[pos]>>6!=2||(p[pos]&0x20)||++packets>16)return 0;
  count=p[pos]&31;type=p[pos+1];bytes=(be16(p+pos+2)+1U)*4U;
  if(bytes>n-pos)return 0;
  if(packets==1&&type!=200&&type!=201)return 0;
  if(type==200) {if(bytes!=28U+24U*count)return 0;}
  else if(type==201) {if(bytes!=8U+24U*count)return 0;}
  else if(type==202) {
   if(!count)return 0;q=4;
   for(chunk=0;chunk<count;chunk++) {
    if(bytes-q<4)return 0;q+=4;
    for(;;) {
     size_t value;if(q==bytes)return 0;
     if(p[pos+q]==0) {q++;break;}
     if(p[pos+q]>8||bytes-q<2)return 0;
     value=p[pos+q+1];q+=2;
     if(!value||value>bytes-q)return 0;q+=value;
    }
    while(q%4) {if(q==bytes||p[pos+q]!=0)return 0;q++;}
   }
   if(q!=bytes)return 0;
  } else if(type==203) {
   if(!count||bytes<4U+4U*count)return 0;q=4U+4U*count;
   if(q<bytes) {
    size_t reason=p[pos+q++];if(reason>bytes-q)return 0;q+=reason;
    while(q%4){if(q==bytes||p[pos+q]!=0)return 0;q++;}
    if(q!=bytes)return 0;
   }
  } else if(type==204) {if(bytes<12)return 0;}
  else return 0;
  pos+=bytes;
 }
 return packets!=0;
}
enum camera_rtsp_input_result camera_rtsp_input_next(struct camera_rtsp_input *r,
 unsigned int selected,uint64_t now,struct camera_rtsp_request *request,size_t *consumed)
{
 size_t bytes;enum camera_rtsp_parse_result parsed;
 if(!request||!consumed||camera_rtsp_input_tick(r,now))return CAMERA_RTSP_INPUT_INVALID;
 if(!r->used)return CAMERA_RTSP_INPUT_NEED_MORE;
 if(r->data[0]=='$') {
  if(!selected){fail(r,CAMERA_RTSP_INPUT_STATE);return CAMERA_RTSP_INPUT_INVALID;}
  if(r->used>=2&&r->data[1]!=1){fail(r,CAMERA_RTSP_INPUT_CHANNEL);return CAMERA_RTSP_INPUT_INVALID;}
  if(r->used<4)return CAMERA_RTSP_INPUT_NEED_MORE;
  bytes=be16(r->data+2);
  if(bytes<8||bytes>CAMERA_RTSP_RTCP_MAX_BYTES||bytes%4){fail(r,CAMERA_RTSP_INPUT_RTCP_FORMAT);return CAMERA_RTSP_INPUT_INVALID;}
  if(r->used<bytes+4)return CAMERA_RTSP_INPUT_NEED_MORE;
  if(!rtcp(r->data+4,bytes)){fail(r,CAMERA_RTSP_INPUT_RTCP_FORMAT);return CAMERA_RTSP_INPUT_INVALID;}
  *consumed=bytes+4;return CAMERA_RTSP_INPUT_RTCP;
 }
 parsed=camera_rtsp_request_parse(r->data,r->used,request);
 if(parsed==CAMERA_RTSP_NEED_MORE)return CAMERA_RTSP_INPUT_NEED_MORE;
 if(parsed==CAMERA_RTSP_INVALID){fail(r,CAMERA_RTSP_INPUT_FRAMING);return CAMERA_RTSP_INPUT_INVALID;}
 *consumed=request->consumed;return CAMERA_RTSP_INPUT_REQUEST;
}
int camera_rtsp_input_retire(struct camera_rtsp_input *r,size_t n,uint64_t now)
{
 size_t remaining;
 if(camera_rtsp_input_tick(r,now)||!n||n>r->used)return -1;
 remaining=r->used-n;
 explicit_bzero(r->data,n);
 memmove(r->data,r->data+n,remaining);
 explicit_bzero(r->data+remaining,r->used-remaining);
 r->used=remaining;
 /* Preserve oldest receive time for an incomplete coalesced tail, rather than
  * granting another budget on every complete preceding unit or socket touch. */
 if(!remaining)r->deadline_active=0;
 return 0;
}
void camera_rtsp_input_clear(struct camera_rtsp_input *r)
{if(r){explicit_bzero(r->data,sizeof(r->data));r->used=0;r->deadline_active=0;}}
static int transport(struct camera_rtsp_span s)
{
 size_t pos=0,start,end;unsigned int seen=0,part=0;
 while(pos<s.length) {
  struct camera_rtsp_span v;
  start=pos;while(pos<s.length&&s.data[pos]!=';')pos++;end=pos;
  while(start<end&&s.data[start]==' ')start++;
  while(end>start&&s.data[end-1]==' ')end--;
  v.data=s.data+start;v.length=end-start;
  if(part++==0){if(!ci(v,"RTP/AVP/TCP"))return 0;}
  else if(ci(v,"unicast")){if(seen&1U)return 0;seen|=1U;}
  else if(ci(v,"interleaved=0-1")){if(seen&2U)return 0;seen|=2U;}
  else return 0;
  if(pos<s.length){pos++;if(pos==s.length)return 0;}
 }
 return part==3&&seen==3U;
}
enum camera_rtsp_input_failure camera_rtsp_request_policy(
 const struct camera_rtsp_request *r,const struct camera_rtsp_registry *registry,
 unsigned int selected,unsigned int playing,const char *session,enum camera_rtsp_method *out)
{
 enum camera_rtsp_method method=CAMERA_RTSP_METHOD_UNSUPPORTED;int live,track;
 if(!r||!registry||!session||!out)return CAMERA_RTSP_INPUT_STATE;
 if(same(r->method,"OPTIONS"))method=CAMERA_RTSP_METHOD_OPTIONS;
 else if(same(r->method,"DESCRIBE"))method=CAMERA_RTSP_METHOD_DESCRIBE;
 else if(same(r->method,"SETUP"))method=CAMERA_RTSP_METHOD_SETUP;
 else if(same(r->method,"PLAY"))method=CAMERA_RTSP_METHOD_PLAY;
 else if(same(r->method,"PAUSE"))method=CAMERA_RTSP_METHOD_PAUSE;
 else if(same(r->method,"GET_PARAMETER"))method=CAMERA_RTSP_METHOD_GET_PARAMETER;
 else if(same(r->method,"TEARDOWN"))method=CAMERA_RTSP_METHOD_TEARDOWN;
 /* Unknown method is unsupported, never silently interpreted as another verb. */
 if(method==CAMERA_RTSP_METHOD_UNSUPPORTED){*out=method;return CAMERA_RTSP_INPUT_NONE;}
 live=same(r->target,registry->live_uri);track=same(r->target,registry->track_uri);
 if(r->target_is_star) {
  if(method!=CAMERA_RTSP_METHOD_OPTIONS)return CAMERA_RTSP_INPUT_RESOURCE;
 } else if(!live&&!(track&&(method==CAMERA_RTSP_METHOD_SETUP||method==CAMERA_RTSP_METHOD_OPTIONS)))
  return CAMERA_RTSP_INPUT_RESOURCE;
 if(r->body.length)return CAMERA_RTSP_INPUT_BODY; /* no parameter/body effects implemented */
 if(r->session.data&&(!selected||!same(r->session_token,session)))return CAMERA_RTSP_INPUT_SESSION;
 if(method==CAMERA_RTSP_METHOD_SETUP) {
  if(selected||r->session.data)return CAMERA_RTSP_INPUT_STATE;
  if(!transport(r->transport))return CAMERA_RTSP_INPUT_TRANSPORT;
 } else if(r->transport.data)return CAMERA_RTSP_INPUT_TRANSPORT;
 if(method==CAMERA_RTSP_METHOD_PLAY||method==CAMERA_RTSP_METHOD_PAUSE||method==CAMERA_RTSP_METHOD_TEARDOWN||
    (method==CAMERA_RTSP_METHOD_GET_PARAMETER&&selected)) {
  if(!selected)return CAMERA_RTSP_INPUT_STATE;
  if(!r->session.data||!same(r->session_token,session))return CAMERA_RTSP_INPUT_SESSION;
 }
 if(method==CAMERA_RTSP_METHOD_PAUSE&&!playing)return CAMERA_RTSP_INPUT_STATE;
 *out=method;return CAMERA_RTSP_INPUT_NONE;
}
