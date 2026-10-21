/* SPDX-License-Identifier: GPL-2.0-only
 * Finite host-memory fixtures only. All credential markers are public DUMMY.
 * Output contains counts/line numbers only, never request or header contents. */
#include <camera/rtsp_request.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

static unsigned long checks,calls;
#define CHECK(x) do {checks++;if(!(x)) {fprintf(stderr,"check failed at line %d\n",__LINE__);exit(1);}} while(0)
struct wire { unsigned char data[CAMERA_RTSP_REQUEST_MAX_BYTES+1U];size_t length; };
static void append(struct wire *w,const void *data,size_t n)
{
 CHECK(n<=sizeof(w->data)-w->length);memcpy(w->data+w->length,data,n);w->length+=n;
}
static void text(struct wire *w,const char *s) {append(w,s,strlen(s));}
static void repeat(struct wire *w,unsigned char c,size_t n)
{
 CHECK(n<=sizeof(w->data)-w->length);memset(w->data+w->length,c,n);w->length+=n;
}
static void in_bounds(struct camera_rtsp_span s,const unsigned char *p,size_t n)
{
 uintptr_t base=(uintptr_t)p,address=(uintptr_t)s.data;
 if(!s.data) {CHECK(s.length==0);return;}
 CHECK(address>=base);CHECK(address-base<=n);CHECK(s.length<=n-(address-base));
}
static enum camera_rtsp_parse_result parse(const void *data,size_t n,
 struct camera_rtsp_request *r)
{
 unsigned char before[sizeof(*r)],copy[CAMERA_RTSP_REQUEST_MAX_BYTES+1U];
 enum camera_rtsp_parse_result result;size_t i;
 CHECK(n<=sizeof(copy));
 memset(r,0xa5,sizeof(*r));memcpy(before,r,sizeof(*r));
 if(n)memcpy(copy,data,n);
 result=camera_rtsp_request_parse(data,n,r);calls++;
 if(n)CHECK(memcmp(copy,data,n)==0);
 if(result!=CAMERA_RTSP_READY)CHECK(memcmp(before,r,sizeof(*r))==0);
 else {
  CHECK(r->consumed>0&&r->consumed<=n&&r->consumed<=CAMERA_RTSP_REQUEST_MAX_BYTES);
  CHECK(r->header_count<=CAMERA_RTSP_REQUEST_MAX_HEADERS);
  CHECK(r->body.length==r->content_length&&r->body.length<=CAMERA_RTSP_REQUEST_MAX_BODY);
  in_bounds(r->method,data,r->consumed);in_bounds(r->target,data,r->consumed);
  in_bounds(r->body,data,r->consumed);in_bounds(r->session,data,r->consumed);
  in_bounds(r->session_token,data,r->consumed);in_bounds(r->transport,data,r->consumed);
  in_bounds(r->authorization,data,r->consumed);
  for(i=0;i<r->header_count;i++) {
   in_bounds(r->headers[i].name,data,r->consumed);
   in_bounds(r->headers[i].value,data,r->consumed);
  }
 }
 return result;
}
static void expect(const char *s,enum camera_rtsp_parse_result result)
{
 struct camera_rtsp_request r;CHECK(parse(s,strlen(s),&r)==result);
}
static void split_valid(const void *data,size_t n)
{
 size_t i;struct camera_rtsp_request r;
 for(i=0;i<n;i++)CHECK(parse(data,i,&r)==CAMERA_RTSP_NEED_MORE);
 CHECK(parse(data,n,&r)==CAMERA_RTSP_READY);CHECK(r.consumed==n);
}
static void valid_and_pipeline(void)
{
 const char *simple="OPTIONS * RTSP/1.0\r\nCSeq: 0\r\n\r\n";
 const char *full="PLAY rtsp://public.invalid:8554/a%2Fb?x=A%40B RTSP/1.0\r\n"
  "cSeQ: 4294967295\r\nSeSsIoN: DUMMY-public.session+1;TIMEOUT=4294967295\r\n"
  "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
  "Authorization: DUMMY-PUBLIC opaque=\"a,b\", never-authenticated\r\n"
  "Content-Length: 0000000005\r\nX-Public: one\r\nx-public: two\r\n\r\n";
 struct wire w={{0},0};struct camera_rtsp_request r;size_t first;
 const unsigned char body[]={0,'$','\r','\n',0xff};
 split_valid(simple,strlen(simple));text(&w,full);append(&w,body,sizeof(body));
 split_valid(w.data,w.length);
 CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_READY);
 CHECK(r.cseq==UINT32_MAX&&r.has_session_timeout&&r.session_timeout==UINT32_MAX);
 CHECK(r.has_content_length&&r.content_length==5&&!r.target_is_star);
 CHECK(r.method.length==4&&memcmp(r.method.data,"PLAY",4)==0);
 CHECK(r.target.length==strlen("rtsp://public.invalid:8554/a%2Fb?x=A%40B"));
 CHECK(memcmp(r.target.data,"rtsp://public.invalid:8554/a%2Fb?x=A%40B",r.target.length)==0);
 CHECK(r.authorization.length==strlen("DUMMY-PUBLIC opaque=\"a,b\", never-authenticated"));
 CHECK(memcmp(r.authorization.data,"DUMMY-PUBLIC opaque=\"a,b\", never-authenticated",r.authorization.length)==0);
 CHECK(r.session_token.length==strlen("DUMMY-public.session+1"));
 CHECK(memcmp(r.body.data,body,sizeof(body))==0);first=w.length;
 text(&w,simple);CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_READY);CHECK(r.consumed==first);
 CHECK(parse(w.data+first,w.length-first,&r)==CAMERA_RTSP_READY);
 CHECK(r.consumed==strlen(simple)&&r.target_is_star);
 expect("EXTENSION rtsp://public.invalid/resource RTSP/1.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_READY);
 expect("options rtsp://public.invalid/ RTSP/1.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_READY);
 expect("OPTIONS RTSPS://[::1]:322/ RTSP/1.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_READY);
 /* Parser never searches the opaque body for critical headers. */
 w.length=0;text(&w,"SET_PARAMETER rtsp://public.invalid/ RTSP/1.0\r\nCSeq: 2\r\nContent-Length: 44\r\n\r\n");
 text(&w,"CSeq: 999\r\nAuthorization: DUMMY-PUBLIC\r\n\r\nxx");
 CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_READY);CHECK(r.cseq==2&&!r.authorization.data);
 CHECK(r.body.length==44);CHECK(r.consumed==w.length);
 /* An incomplete declared body cannot be completed by scanning CRLFCRLF. */
 CHECK(parse(w.data,w.length-1,&r)==CAMERA_RTSP_NEED_MORE);
 w.length=0;text(&w,simple);append(&w,"$\0\0\1x",5);
 CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_READY);CHECK(r.consumed==strlen(simple));
 CHECK(parse(w.data+r.consumed,5,&r)==CAMERA_RTSP_INVALID);
}
static void malformed(void)
{
 static const char *const bad[]={
  "OPTIONS * RTSP/1.0\r\n\r\n",
  "OPTIONS * RTSP/1.0\nCSeq: 1\n\n",
  "OPTIONS * RTSP/1.0\rX", "\r\n", "$", "$METHOD rtsp://public.invalid/ RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS * RTSP/2.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS * HTTP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS * rtsp/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS * RTSP/1.0 extra\r\nCSeq: 1\r\n\r\n",
  " OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS  * RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS\t* RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "DESCRIBE * RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "options * RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS /resource RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://DUMMY-PUBLIC@public.invalid/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://DUMMY%40public.invalid/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp:///x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://public.invalid:/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://public.invalid:65536/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://public.invalid:0/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://[::1/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://[]/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://[::1]evil/x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://public.invalid/x#fragment RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://public.invalid/% RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://public.invalid/%GG RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS rtsp://public.invalid/\\x RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\n CSeq: 1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq : 1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n folded\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nX: a\tb\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nX: \177\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nX: \200\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: \r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: -1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: +1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1 2\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 4294967296\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 00000000000\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: -1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: +1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 4097\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 4294967296\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 1,1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 1x\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nTransfer-Encoding: chunked\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 0\r\ntRaNsFeR-eNcOdInG: identity\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nTrailer: CSeq\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nAuthorization: \r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nTransport: \r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY/PUBLIC\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY;timeout=0\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY;timeout=-1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY;timeout=4294967296\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY;timeout=1;timeout=2\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY ;timeout=1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY;timeout=1,OTHER\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: DUMMY;other=1\r\n\r\n"
 };
 static const char *const names[]={"CSeq","Content-Length","Authorization","Session","Transport"};
 static const char *const lower_names[]={"cseq","content-length","authorization","session","transport"};
 static const char *const values[]={"1","0","DUMMY-PUBLIC","DUMMY-PUBLIC","DUMMY-PUBLIC"};
 size_t i,j;struct wire w={{0},0};struct camera_rtsp_request r;
 for(i=0;i<sizeof(bad)/sizeof(bad[0]);i++)expect(bad[i],CAMERA_RTSP_INVALID);
 for(i=0;i<5;i++)for(j=0;j<2;j++) {
  w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\n");
  if(i!=0)text(&w,"CSeq: 1\r\n");
  text(&w,names[i]);text(&w,": ");text(&w,values[i]);text(&w,"\r\n");
  text(&w,lower_names[i]);text(&w,": ");text(&w,j?"2":values[i]);text(&w,"\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_INVALID);
 }
 /* A header-looking substring is not a header-name match. */
 expect("OPTIONS * RTSP/1.0\r\nX-CSeq: 1\r\n\r\n",CAMERA_RTSP_INVALID);
 expect("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nX: Authorization: DUMMY-PUBLIC\r\n\r\n",CAMERA_RTSP_READY);
 w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nX: ");
 append(&w,"\0",1);text(&w,"\r\n\r\n");CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_INVALID);
}
static void bounds(void)
{
 struct wire w={{0},0};struct camera_rtsp_request r;size_t i,n;
 for(n=32;n<=33;n++) {
  w.length=0;repeat(&w,'M',n);text(&w," rtsp://public.invalid/ RTSP/1.0\r\nCSeq: 1\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==(n==32?CAMERA_RTSP_READY:CAMERA_RTSP_INVALID));
 }
 for(n=512;n<=513;n++) {
  w.length=0;text(&w,"OPTIONS rtsp://public.invalid/");repeat(&w,'a',n-strlen("rtsp://public.invalid/"));
  text(&w," RTSP/1.0\r\nCSeq: 1\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==(n==512?CAMERA_RTSP_READY:CAMERA_RTSP_INVALID));
 }
 for(n=64;n<=65;n++) {
  w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n");repeat(&w,'X',n);text(&w,": y\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==(n==64?CAMERA_RTSP_READY:CAMERA_RTSP_INVALID));
 }
 for(n=1024;n<=1025;n++) {
  w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nAuthorization:");repeat(&w,'D',n);text(&w,"\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==(n==1024?CAMERA_RTSP_READY:CAMERA_RTSP_INVALID));
 }
 /* Raw value budget includes SP even though exposed value is trimmed. */
 w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nX:");repeat(&w,' ',1025);text(&w,"\r\n\r\n");
 CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_INVALID);
 for(n=128;n<=129;n++) {
  w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession:");repeat(&w,'D',n);text(&w,"\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_INVALID);
 }
 for(n=512;n<=513;n++) {
  w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nTransport:");repeat(&w,'D',n);text(&w,"\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==(n==512?CAMERA_RTSP_READY:CAMERA_RTSP_INVALID));
 }
 for(n=64;n<=65;n++) {
  w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession:");repeat(&w,'D',n);text(&w,"\r\n\r\n");
  CHECK(parse(w.data,w.length,&r)==(n==64?CAMERA_RTSP_READY:CAMERA_RTSP_INVALID));
 }
 for(n=24;n<=25;n++) {
  w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n");
  for(i=1;i<n;i++)text(&w,"X: y\r\n");
  text(&w,"\r\n");CHECK(parse(w.data,w.length,&r)==(n==24?CAMERA_RTSP_READY:CAMERA_RTSP_INVALID));
 }
 /* Exactly 8192 bytes, including a binary body of exactly 4096. */
 w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 4096\r\n");
 for(i=0;i<3;i++) {text(&w,"X:");repeat(&w,'p',1024);text(&w,"\r\n");}
 text(&w,"X:");CHECK(w.length+4<4096);repeat(&w,'p',4096-w.length-4);text(&w,"\r\n\r\n");
 CHECK(w.length==4096);repeat(&w,0,4096);split_valid(w.data,w.length);
 CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_READY&&r.consumed==8192);
 append(&w,"x",1);CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_INVALID);
 /* Aggregate budget failure is known before waiting for the declared body. */
 w.length=0;text(&w,"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 4096\r\n");
 for(i=0;i<3;i++) {text(&w,"X:");repeat(&w,'p',1024);text(&w,"\r\n");}
 text(&w,"X:");repeat(&w,'p',4097-w.length-4);text(&w,"\r\n\r\n");
 CHECK(w.length==4097);CHECK(parse(w.data,w.length,&r)==CAMERA_RTSP_INVALID);
 expect("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 0\r\n\r\n",CAMERA_RTSP_READY);
 expect("OPTIONS * RTSP/1.0\r\nCSeq:  1  \r\nX:\r\n\r\n",CAMERA_RTSP_READY);
 CHECK(parse(NULL,0,&r)==CAMERA_RTSP_NEED_MORE);
 CHECK(camera_rtsp_request_parse(NULL,1,&r)==CAMERA_RTSP_INVALID);
 CHECK(camera_rtsp_request_parse("",0,NULL)==CAMERA_RTSP_INVALID);
}
static uint32_t random_public(uint32_t *s)
{
 /* Deterministic fixture generator, NEVER security entropy. */
 *s=*s*1664525U+1013904223U;return *s;
}
static void finite_properties(void)
{
 struct wire w={{0},0};struct camera_rtsp_request r;uint32_t seed=1;
 const char *valid="OPTIONS rtsp://public.invalid/%2f RTSP/1.0\r\nCSeq: 1\r\nAuthorization: DUMMY-PUBLIC\r\n\r\n";
 size_t i,j,n=strlen(valid);unsigned int c;
 /* Every single-byte mutation, no payload dumps. Results need not be invalid,
  * but output and span invariants must hold for every result. */
 for(i=0;i<n;i++)for(c=0;c<=255;c++) {
  w.length=0;text(&w,valid);w.data[i]=(unsigned char)c;
  (void)parse(w.data,w.length,&r);
 }
 for(i=0;i<10000;i++) {
  w.length=(size_t)(random_public(&seed)%8193U);
  for(j=0;j<w.length;j++)w.data[j]=(unsigned char)(random_public(&seed)>>24);
  (void)parse(w.data,w.length,&r);
 }
}
static void exact_prefix_guards(void)
{
#if defined(__SANITIZE_ADDRESS__)
 /* Poison the entire unread tail, including immediately after each prefix.
  * This complements span checks: staying within the actual object alone does
  * not prove that a parser honors the supplied length. No heap allocation. */
 unsigned char storage[CAMERA_RTSP_REQUEST_MAX_BYTES+8U];
 const char *s="PLAY rtsp://public.invalid/%2f RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 3\r\n\r\nabc";
 size_t i,n=strlen(s);struct camera_rtsp_request r;
 memset(storage,0,sizeof(storage));memcpy(storage,s,n);
 for(i=0;i<=n;i++) {
  enum camera_rtsp_parse_result result;
  __asan_poison_memory_region(storage+i,sizeof(storage)-i);
  result=parse(storage,i,&r);
  __asan_unpoison_memory_region(storage,sizeof(storage));
  CHECK(result==(i<n?CAMERA_RTSP_NEED_MORE:CAMERA_RTSP_READY));
 }
#endif
}
int main(void)
{
 valid_and_pipeline();malformed();bounds();finite_properties();exact_prefix_guards();
 printf("rtsp_request_test: PASS calls=%lu checks=%lu\n",calls,checks);return 0;
}
