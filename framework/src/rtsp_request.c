/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/rtsp_request.h>
#include <string.h>

static int alpha(unsigned char c)
{
 return (c>='A'&&c<='Z')||(c>='a'&&c<='z');
}
static int digit(unsigned char c) { return c>='0'&&c<='9'; }
static int hex(unsigned char c)
{
 return digit(c)||(c>='A'&&c<='F')||(c>='a'&&c<='f');
}
static int token(unsigned char c)
{
 return alpha(c)||digit(c)||(c!=0&&strchr("!#$%&'*+-.^_`|~",c)!=NULL);
}
static unsigned char lower(unsigned char c)
{
 return c>='A'&&c<='Z'?(unsigned char)(c+('a'-'A')):c;
}
static int equal_ci(struct camera_rtsp_span s,const char *literal)
{
 size_t i,n=strlen(literal);
 if(s.length!=n)return 0;
 for(i=0;i<n;i++)if(lower(s.data[i])!=lower((unsigned char)literal[i]))return 0;
 return 1;
}
static struct camera_rtsp_span span(const unsigned char *p,size_t n)
{
 struct camera_rtsp_span s;s.data=p;s.length=n;return s;
}

/* Reject bare CR/LF and every control/non-ASCII byte in textual lines. A final
 * CR may be an incomplete CRLF. Limits exclude the two delimiter bytes. */
static enum camera_rtsp_parse_result line(const unsigned char *p,size_t n,
 size_t start,size_t limit,size_t *end)
{
 size_t i;
 for(i=start;i<n;i++) {
  unsigned char c=p[i];
  if(c=='\r') {
   if(i-start>limit)return CAMERA_RTSP_INVALID;
   if(i+1==n)return CAMERA_RTSP_NEED_MORE;
   if(p[i+1]!='\n')return CAMERA_RTSP_INVALID;
   *end=i;return CAMERA_RTSP_READY;
  }
  if(c<0x20||c>0x7e||i-start>=limit)return CAMERA_RTSP_INVALID;
 }
 return CAMERA_RTSP_NEED_MORE;
}
static int decimal(struct camera_rtsp_span s,uint32_t maximum,uint32_t *out)
{
 size_t i;uint32_t value=0;
 if(s.length==0||s.length>10)return 0;
 for(i=0;i<s.length;i++) {
  uint32_t d;
  if(!digit(s.data[i]))return 0;
  d=(uint32_t)(s.data[i]-'0');
  if(d>maximum||value>(maximum-d)/10U)return 0;
  value=value*10U+d;
 }
 *out=value;return 1;
}
static int unreserved(unsigned char c)
{
 return alpha(c)||digit(c)||(c!=0&&strchr("-._~",c)!=NULL);
}
static int subdelim(unsigned char c)
{
 return c!=0&&strchr("!$&'()*+,;=",c)!=NULL;
}

/* Deliberately narrow RTSP absolute-URI profile. URI authority cannot contain
 * userinfo, percent escapes, empty hosts or malformed ports/brackets. Literal
 * IPv6 address semantics/DNS resolution and resource matching are not done. */
static int target_valid(struct camera_rtsp_span s)
{
 size_t i,start,end,host_end,port_start=0;
 if(s.length>=7&&equal_ci(span(s.data,7),"rtsp://"))start=7;
 else if(s.length>=8&&equal_ci(span(s.data,8),"rtsps://"))start=8;
 else return 0;
 end=start;
 while(end<s.length&&s.data[end]!='/'&&s.data[end]!='?')end++;
 if(end==start)return 0;
 if(s.data[start]=='[') {
  host_end=start+1;
  while(host_end<end&&s.data[host_end]!=']') {
   if(!hex(s.data[host_end])&&s.data[host_end]!=':'&&s.data[host_end]!='.')return 0;
   host_end++;
  }
  if(host_end==start+1||host_end==end)return 0;
  host_end++;
  if(host_end<end) {
   if(s.data[host_end]!=':')return 0;
   port_start=host_end+1;
  }
 } else {
  host_end=start;
  while(host_end<end&&s.data[host_end]!=':') {
   unsigned char c=s.data[host_end];
   if(!alpha(c)&&!digit(c)&&c!='-'&&c!='.')return 0;
   host_end++;
  }
  if(host_end==start)return 0;
  if(host_end<end)port_start=host_end+1;
 }
 if(port_start) {
  uint32_t port;
  if(!decimal(span(s.data+port_start,end-port_start),65535U,&port)||port==0)return 0;
 }
 for(i=end;i<s.length;i++) {
  unsigned char c=s.data[i];
  if(c=='%') {
   if(s.length-i<3||!hex(s.data[i+1])||!hex(s.data[i+2]))return 0;
   i+=2;
  } else if(!unreserved(c)&&!subdelim(c)&&c!=':'&&c!='@'&&c!='/'&&c!='?')return 0;
 }
 return 1;
}
static int request_line(const unsigned char *p,size_t end,
 struct camera_rtsp_request *r)
{
 size_t i=0,start;
 while(i<end&&token(p[i]))i++;
 if(i==0||i>CAMERA_RTSP_REQUEST_MAX_METHOD||i==end||p[i]!=' ')return 0;
 r->method=span(p,i);start=++i;
 while(i<end&&p[i]!=' ')i++;
 if(i==start||i-start>CAMERA_RTSP_REQUEST_MAX_TARGET||i==end)return 0;
 r->target=span(p+start,i-start);
 if(end-i!=9||memcmp(p+i," RTSP/1.0",9)!=0)return 0;
 if(r->target.length==1&&r->target.data[0]=='*') {
  if(r->method.length!=7||memcmp(r->method.data,"OPTIONS",7)!=0)return 0;
  r->target_is_star=1;return 1;
 }
 return target_valid(r->target);
}
static int session_valid(struct camera_rtsp_request *r)
{
 size_t i=0;struct camera_rtsp_span s=r->session;
 while(i<s.length) {
  unsigned char c=s.data[i];
  if(!alpha(c)&&!digit(c)&&c!='$'&&c!='-'&&c!='_'&&c!='.'&&c!='+')break;
  i++;
 }
 if(i==0||i>CAMERA_RTSP_REQUEST_MAX_SESSION_TOKEN)return 0;
 r->session_token=span(s.data,i);
 if(i==s.length)return 1;
 if(s.length-i<=9||!equal_ci(span(s.data+i,9),";timeout="))return 0;
 if(!decimal(span(s.data+i+9,s.length-i-9),UINT32_MAX,&r->session_timeout)
    ||r->session_timeout==0)return 0;
 r->has_session_timeout=1;return 1;
}
static int header(const unsigned char *p,size_t start,size_t end,
 struct camera_rtsp_request *r,unsigned int *seen)
{
 size_t i=start,v,last;unsigned int bit=0;
 struct camera_rtsp_header h;
 while(i<end&&token(p[i]))i++;
 if(i==start||i-start>CAMERA_RTSP_REQUEST_MAX_HEADER_NAME||i==end||p[i]!=':')return 0;
 h.name=span(p+start,i-start);v=i+1;
 if(end-v>CAMERA_RTSP_REQUEST_MAX_HEADER_VALUE)return 0;
 last=end;
 while(v<last&&p[v]==' ')v++;
 while(last>v&&p[last-1]==' ')last--;
 h.value=span(p+v,last-v);
 if(equal_ci(h.name,"transfer-encoding")||equal_ci(h.name,"trailer"))return 0;
 if(equal_ci(h.name,"cseq"))bit=1U;
 else if(equal_ci(h.name,"content-length"))bit=2U;
 else if(equal_ci(h.name,"authorization"))bit=4U;
 else if(equal_ci(h.name,"session"))bit=8U;
 else if(equal_ci(h.name,"transport"))bit=16U;
 if(bit&&((*seen&bit)||h.value.length==0))return 0;
 *seen|=bit;
 switch(bit) {
 case 1U:
  if(!decimal(h.value,UINT32_MAX,&r->cseq))return 0;
  break;
 case 2U:
  if(!decimal(h.value,CAMERA_RTSP_REQUEST_MAX_BODY,&r->content_length))return 0;
  r->has_content_length=1;break;
 case 4U:
  if(h.value.length>CAMERA_RTSP_REQUEST_MAX_AUTHORIZATION)return 0;
  r->authorization=h.value;break;
 case 8U:
  if(h.value.length>CAMERA_RTSP_REQUEST_MAX_SESSION)return 0;
  r->session=h.value;
  if(!session_valid(r))return 0;
  break;
 case 16U:
  if(h.value.length>CAMERA_RTSP_REQUEST_MAX_TRANSPORT)return 0;
  r->transport=h.value;break;
 default:break;
 }
 r->headers[r->header_count++]=h;return 1;
}

enum camera_rtsp_parse_result camera_rtsp_request_parse(
 const void *buffer,size_t length,struct camera_rtsp_request *output)
{
 const unsigned char *p=(const unsigned char *)buffer;
 struct camera_rtsp_request r;
 enum camera_rtsp_parse_result result;size_t pos=0,end;unsigned int seen=0;
 if(!output||(!p&&length)||length>CAMERA_RTSP_REQUEST_MAX_BYTES)return CAMERA_RTSP_INVALID;
 if(length==0)return CAMERA_RTSP_NEED_MORE;
 /* '$' is also a legal token character; explicitly keep frames out of RTSP. */
 if(p[0]=='$')return CAMERA_RTSP_INVALID;
 memset(&r,0,sizeof(r));
 result=line(p,length,0,CAMERA_RTSP_REQUEST_MAX_METHOD+1U+
             CAMERA_RTSP_REQUEST_MAX_TARGET+1U+8U,&end);
 if(result!=CAMERA_RTSP_READY)goto incomplete;
 if(!request_line(p,end,&r))return CAMERA_RTSP_INVALID;
 pos=end+2;
 for(;;) {
  result=line(p,length,pos,CAMERA_RTSP_REQUEST_MAX_HEADER_NAME+1U+
              CAMERA_RTSP_REQUEST_MAX_HEADER_VALUE,&end);
  if(result!=CAMERA_RTSP_READY)goto incomplete;
  if(end==pos) {pos=end+2;break;}
  if(r.header_count==CAMERA_RTSP_REQUEST_MAX_HEADERS||!header(p,pos,end,&r,&seen))
   return CAMERA_RTSP_INVALID;
  pos=end+2;
 }
 if(!(seen&1U)||r.content_length>CAMERA_RTSP_REQUEST_MAX_BYTES-pos)return CAMERA_RTSP_INVALID;
 if(length-pos<r.content_length)return CAMERA_RTSP_NEED_MORE;
 r.body=span(p+pos,r.content_length);r.consumed=pos+r.content_length;
 *output=r;return CAMERA_RTSP_READY;
incomplete:
 if(result==CAMERA_RTSP_NEED_MORE&&length==CAMERA_RTSP_REQUEST_MAX_BYTES)
  return CAMERA_RTSP_INVALID;
 return result;
}
