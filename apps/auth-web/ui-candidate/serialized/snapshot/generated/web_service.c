/* SPDX-License-Identifier: GPL-2.0-only */
/* Complete copied service candidate; no native/product acceptance asserted. */
#define _GNU_SOURCE
#include "web_service.h"
#include "ui_assets.h"
#include "credential/credential_process.h"
#include "credential/credential_ipc.h"
#include "mongoose.h"
#include <mbedtls/constant_time.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/pk.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define SESSION_COUNT 8U
#define PEER_COUNT 2U
#define HEADER_MAX 8192U
#define BODY_MAX 4096U
#define TOKEN_BYTES 32U
#define TOKEN_TEXT 64U
extern size_t web_heap_peak(void);
extern int web_heap_failed(void);
static volatile sig_atomic_t stopping;
static int fatal;
static unsigned startup_stage;
static uint64_t last_clock;
static struct {
 struct web_config cfg;
 struct camera_access_policy policy;
 struct cp_client *child;
 struct mg_mgr mgr;
 char cert[8193],key[4097];size_t cert_len,key_len;
 struct {
  unsigned state; /* 0 absent, 1 anonymous, 2 pending, 3 authenticated */
  unsigned char cookie[32],csrf[32];
  uint64_t idle_until,absolute_until,serial;
  struct camera_access_principal principal;
 } sessions[SESSION_COUNT];
 struct {
  unsigned used,handshake,done,pending;
  unsigned auth_session;uint64_t auth_serial;
  unsigned long connection;uint64_t deadline,drain_deadline;
 } peers[PEER_COUNT];
 struct {unsigned active,session,abandoned;unsigned long connection;
  uint64_t session_serial,request,credential_generation,deadline;
  struct cp_ticket ticket;
 } pending;
 unsigned peers_peak,handshake_peak,requests,login_success,login_failure;
 uint64_t next_request,next_session,start_ns,accept_window,request_window;
 unsigned accept_count,request_count;
} app;

static void wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
static int now_ns(uint64_t *out)
{
 struct timespec t;uint64_t n;
 if(fatal||clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||
    t.tv_nsec>=1000000000L||(uint64_t)t.tv_sec>(UINT64_MAX-1)/WEB_NS)goto bad;
 n=(uint64_t)t.tv_sec*WEB_NS;
 if((uint64_t)t.tv_nsec>=UINT64_MAX-n)goto bad;
 n+=(uint64_t)t.tv_nsec;
 /* Reserve more than maximum runtime + maximum session lifetime + startup/
  * request margins on EVERY observation, not only at startup. This makes all
  * bounded owner deadline additions safe even after a large forward jump. */
 if(n<last_clock||n>UINT64_MAX-7300*WEB_NS)goto bad;
 last_clock=n;*out=n;return 0;
bad:fatal=1;return -1;
}
/* Mongoose's unsigned time API has no error channel. Cached internal timestamp
 * is used ONLY to unwind a failed poll; fatal is sticky and all handlers/effects
 * check it. It is never returned as successful authentication/time evidence. */
uint64_t mg_millis(void){uint64_t n;return now_ns(&n)?last_clock/1000000:n/1000000;}
bool mg_random(void *buffer,size_t bytes)
{
 unsigned char *p=buffer;size_t done=0;unsigned calls=0;
 if(fatal)goto bad;
 while(done<bytes&&calls++<32){
  ssize_t n=getrandom(p+done,bytes-done,GRND_NONBLOCK);
  if(n<0&&errno==EINTR)continue;
  if(n<=0||(size_t)n>bytes-done)goto bad;
  done+=(size_t)n;
 }
 if(done==bytes)return true;
bad:if(buffer)wipe(buffer,bytes);fatal=1;return false;
}
psa_status_t mbedtls_psa_external_get_random(
 mbedtls_psa_external_random_context_t *context,unsigned char *out,
 size_t size,size_t *length)
{
 (void)context;*length=0;
 if(!mg_random(out,size))return PSA_ERROR_INSUFFICIENT_ENTROPY;
 *length=size;return PSA_SUCCESS;
}
static int rng(void *context,unsigned char *p,size_t n)
{(void)context;return mg_random(p,n)?0:-1;}
static void stop_signal(int sig){(void)sig;stopping=1;}
static int zero_tail(const char *p,size_t n)
{
 size_t i=0;while(i<n&&p[i])i++;if(i==n)return 0;
 for(;i<n;i++)if(p[i])return 0;
 return 1;
}
static int id_valid(const char *p,size_t n)
{
 size_t i;if(!p[0]||!zero_tail(p,n))return 0;
 for(i=0;p[i];i++)if(!((p[i]>='a'&&p[i]<='z')||(p[i]>='A'&&p[i]<='Z')||
  (p[i]>='0'&&p[i]<='9')||p[i]=='-'||p[i]=='_'||p[i]=='.'))return 0;
 return 1;
}
static int read_private(int fd,void *buffer,size_t capacity,size_t *length)
{
 struct stat st;size_t done=0;unsigned loops=0;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||
    (st.st_uid!=getuid()&&st.st_uid!=0)||(st.st_mode&0777)!=0600||
    st.st_size<=0||(uint64_t)st.st_size>capacity||lseek(fd,0,SEEK_CUR)!=0)return -1;
 while(done<(size_t)st.st_size&&loops++<64){
  ssize_t n=read(fd,(unsigned char *)buffer+done,(size_t)st.st_size-done);
  if(n<0&&errno==EINTR)continue;
  if(n<=0)return -1;
  done+=(size_t)n;
 }
 if(done!=(size_t)st.st_size)return -1;
 *length=done;return 0;
}
static int config_valid(void)
{
 struct web_config *c=&app.cfg;size_t i;struct mg_str host;
 if(c->magic!=WEB_CONFIG_MAGIC||c->version!=2||c->bytes!=sizeof(*c)||c->trust_mode!=1||
    !zero_tail(c->listen,sizeof(c->listen))||!zero_tail(c->host,sizeof(c->host))||
    !zero_tail(c->origin,sizeof(c->origin))||strncmp(c->listen,"https://",8)||
    strncmp(c->origin,"https://",8)||!c->host[0]||
    !id_valid(c->device,64)||!id_valid(c->sensor,64)||!id_valid(c->profile,64)||
    !id_valid(c->account,64)||!c->boot_generation||!c->child_generation||!c->credential_generation||
    c->absolute_ns<WEB_NS||c->absolute_ns>3600*WEB_NS||c->idle_ns<WEB_NS||
    c->idle_ns>600*WEB_NS||c->idle_ns>c->absolute_ns||
    (c->runtime_ns&&c->runtime_ns<WEB_NS)||c->runtime_ns>3600*WEB_NS||
    c->response_ns<1000000ULL||c->response_ns>35*WEB_NS)return 0;
 for(i=0;i<sizeof(c->reserved);i++)if(c->reserved[i])return 0;
 for(i=0;c->host[i];i++)if(!((c->host[i]>='a'&&c->host[i]<='z')||
  (c->host[i]>='0'&&c->host[i]<='9')||strchr(".:-[]",c->host[i])))return 0;
 if(strcmp(c->origin+8,c->host))return 0; /* exact scheme + authority, no path */
 host=mg_url_host(c->listen);
 if(host.len==0||mg_url_port(c->listen)<1024||strcmp(mg_url_uri(c->listen),"/")||
    mg_url_user(c->listen).len||mg_url_pass(c->listen).len)return 0;
 /* Numeric bind only: no resolver/outbound DNS activity. */
 for(i=0;i<host.len;i++)if(!strchr("0123456789abcdefABCDEF:.",host.buf[i]))return 0;
#ifdef WEB_PUBLIC_HOST_TEST
 if(strcmp(c->listen+8,c->host)||host.len!=9||memcmp(host.buf,"127.0.0.1",9))return 0;
#endif
 return 1;
}
static int parse_material(const unsigned char *wire,struct cw_ui_material *m)
{
 m->schema=cp_get32(wire);m->iterations=cp_get32(wire+4);
 memcpy(m->salt,wire+8,16);memcpy(m->verifier,wire+24,32);
 return m->schema==CW_SCHEMA&&m->iterations==CW_ITERATIONS?0:-1;
}
static void account_template(struct camera_access_account *a)
{
 memset(a,0,sizeof(*a));a->abi=CAMERA_ACCESS_ABI;a->struct_bytes=sizeof(*a);
 a->domain=CAMERA_ACCESS_ADMIN;a->roles=CAMERA_ACCESS_VIEWER|CAMERA_ACCESS_CONFIGURATOR;
 memcpy(a->account_id,app.cfg.account,64);a->credential_generation=app.cfg.credential_generation;
 a->grant_count=2;a->grants[0].kind=CAMERA_ACCESS_DEVICE;
 memcpy(a->grants[0].resource_id,app.cfg.device,64);
 a->grants[0].operations=CAMERA_ACCESS_DEVICE_CONFIG;
 a->grants[1].kind=CAMERA_ACCESS_PROFILE;memcpy(a->grants[1].resource_id,app.cfg.profile,64);
 a->grants[1].operations=CAMERA_ACCESS_LIVE|CAMERA_ACCESS_SNAPSHOT;
}
static void clear_session(unsigned i)
{
 unsigned j;struct mg_connection *c;
 for(j=0;j<PEER_COUNT;j++)if(app.peers[j].used&&app.peers[j].auth_session==i+1&&
    app.peers[j].auth_serial==app.sessions[i].serial){
  for(c=app.mgr.conns;c;c=c->next)if(c->id==app.peers[j].connection)c->is_closing=1;
 }
 if(app.sessions[i].principal.snapshot_serial){
  int result=cp_release_access(app.child,&app.sessions[i].principal);
  if(result!=CP_OK){
   (void)camera_access_revoke_session(&app.policy,&app.sessions[i].principal);
   /* A failed child has already revoked tracked grants; cleanup is still safe.
    * An unexpected stale snapshot in a healthy client is NOT ignored. */
   if(cp_healthy(app.child))fatal=1;
  }
 }
 wipe(&app.sessions[i],sizeof(app.sessions[i]));
}
static void clear_all(void){unsigned i;for(i=0;i<SESSION_COUNT;i++)clear_session(i);}
static void request_template(struct camera_access_request *q,int media,int snapshot)
{
 memset(q,0,sizeof(*q));q->abi=CAMERA_ACCESS_ABI;q->struct_bytes=sizeof(*q);
 q->domain=CAMERA_ACCESS_ADMIN;q->kind=media?CAMERA_ACCESS_PROFILE:CAMERA_ACCESS_DEVICE;
 q->operation=media?(snapshot?CAMERA_ACCESS_SNAPSHOT:CAMERA_ACCESS_LIVE):CAMERA_ACCESS_DEVICE_CONFIG;
 memcpy(q->device_id,app.cfg.device,64);
 if(media){memcpy(q->sensor_id,app.cfg.sensor,64);memcpy(q->profile_id,app.cfg.profile,64);}
}
static int authorized(unsigned i,uint64_t now)
{
 struct camera_access_request q;
 if(i>=SESSION_COUNT||app.sessions[i].state!=3||now>=app.sessions[i].absolute_until||
    now>=app.sessions[i].idle_until||!cp_healthy(app.child)||fatal)return 0;
 request_template(&q,0,0);
 return camera_access_check(&app.policy,&app.sessions[i].principal,&q,now)==CAMERA_ACCESS_OK;
}
static void hex_encode(const unsigned char p[32],char out[65])
{
 static const char h[]="0123456789abcdef";unsigned i;
 for(i=0;i<32;i++){out[2*i]=h[p[i]>>4];out[2*i+1]=h[p[i]&15];}out[64]=0;
}
static int hex_decode(struct mg_str s,unsigned char out[32])
{
 unsigned i;if(s.len!=64)return -1;
 for(i=0;i<32;i++){unsigned j,v=0;for(j=0;j<2;j++){
  unsigned char c=(unsigned char)s.buf[2*i+j];unsigned d;
  if(c>='0'&&c<='9')d=c-'0';else if(c>='a'&&c<='f')d=c-'a'+10;else return -1;
  v=16*v+d;}out[i]=(unsigned char)v;}
 return 0;
}
static int equal(struct mg_str s,const char *p){return s.len==strlen(p)&&!memcmp(s.buf,p,s.len);}
static struct mg_str header(struct mg_http_message *h,const char *name)
{struct mg_str *s=mg_http_get_header(h,name);return s?*s:mg_str("");}
static int session_find(struct mg_http_message *h,uint64_t now)
{
 unsigned i;int found=-1;unsigned char token[32]={0};struct mg_str s=header(h,"Cookie");
 /* One dedicated cookie only; no ambiguous duplicates, quoted values or extras. */
 if(s.len!=78||memcmp(s.buf,"__Host-camera=",14)||
    hex_decode(mg_str_n(s.buf+14,64),token))goto out;
 for(i=0;i<SESSION_COUNT;i++){
  int same=mbedtls_ct_memcmp(app.sessions[i].cookie,token,32)==0;
  if(same&&app.sessions[i].state&&now<app.sessions[i].absolute_until&&
     now<app.sessions[i].idle_until)found=(int)i;
 }
out:wipe(token,sizeof(token));return found;
}
static int csrf_valid(struct mg_http_message *h,unsigned i)
{
 unsigned char token[32]={0};int result=0;
 if(i<SESSION_COUNT&&!hex_decode(header(h,"X-CSRF-Token"),token))
  result=mbedtls_ct_memcmp(app.sessions[i].csrf,token,32)==0;
 wipe(token,sizeof(token));return result;
}
static int peer_find(unsigned long id)
{unsigned i;for(i=0;i<PEER_COUNT;i++)if(app.peers[i].used&&app.peers[i].connection==id)return (int)i;return -1;}
static struct mg_connection *connection_find(unsigned long id)
{struct mg_connection *c;for(c=app.mgr.conns;c;c=c->next)if(c->id==id&&!c->is_closing)return c;return NULL;}
static void response(struct mg_connection *c,int status,const char *body,const char *extra)
{
 char headers[1024];int n;int peer=peer_find(c->id);uint64_t now;
 if(fatal||now_ns(&now)){c->is_closing=1;return;}
 n=snprintf(headers,sizeof(headers),
  "Content-Type: application/json\r\nCache-Control: no-store\r\nConnection: close\r\n"
  "X-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n"
  "Content-Security-Policy: default-src 'none'; frame-ancestors 'none'\r\n%s",extra?extra:"");
 if(n<0||(size_t)n>=sizeof(headers)||strlen(body)>4096||c->send.len){fatal=1;c->is_closing=1;return;}
 mg_http_reply(c,status,headers,"%s",body);c->is_draining=1;c->is_full=1;
 if(c->send.len>8192||web_heap_failed())fatal=1;
 if(peer>=0){app.peers[peer].done=1;app.peers[peer].drain_deadline=now+2*WEB_NS;}
 wipe(headers,sizeof(headers));
}
static void error_response(struct mg_connection *c,int status)
{
 const char *body=status==401?"{\"error\":\"authentication_failed\"}\n":
  status==403?"{\"error\":\"request_denied\"}\n":
  status==429?"{\"error\":\"admission_limited\"}\n":
  status==501?"{\"error\":\"unsupported_unqualified\"}\n":
  "{\"error\":\"invalid_request\"}\n";
 response(c,status,body,NULL);
}
static void session_response(struct mg_connection *c,unsigned i)
{
 char cookie[65],csrf[65],extra[256],body[320];
 int peer=peer_find(c->id);
 if(peer>=0&&app.sessions[i].state==3){
  app.peers[peer].auth_session=i+1;app.peers[peer].auth_serial=app.sessions[i].serial;
 }
 hex_encode(app.sessions[i].cookie,cookie);hex_encode(app.sessions[i].csrf,csrf);
 snprintf(extra,sizeof(extra),"Set-Cookie: __Host-camera=%s; Path=/; Secure; HttpOnly; SameSite=Strict\r\n",cookie);
 snprintf(body,sizeof(body),"{\"schema\":1,\"authenticated\":%s,\"csrf\":\"%s\",\"session_generation\":%llu,\"credential_generation\":%llu}\n",
  app.sessions[i].state==3?"true":"false",csrf,
  (unsigned long long)(app.sessions[i].state==3?app.sessions[i].principal.fact.session_generation:0),
  (unsigned long long)(app.sessions[i].state==3?app.sessions[i].principal.fact.credential_generation:0));
 response(c,200,body,extra);wipe(cookie,sizeof(cookie));wipe(csrf,sizeof(csrf));
 wipe(extra,sizeof(extra));wipe(body,sizeof(body));
}
static int session_new(uint64_t now)
{
 unsigned i;for(i=0;i<SESSION_COUNT;i++)if(!app.sessions[i].state)break;
 if(i==SESSION_COUNT||app.next_session==UINT64_MAX)return -1;
 if(!mg_random(app.sessions[i].cookie,32)||!mg_random(app.sessions[i].csrf,32))return -1;
 app.sessions[i].serial=++app.next_session;app.sessions[i].state=1;
 app.sessions[i].absolute_until=now+60*WEB_NS;app.sessions[i].idle_until=now+60*WEB_NS;
 return (int)i;
}
/* Strict wire framing BEFORE asking Mongoose to parse or admitting any effect.
 * Reject duplicate headers (including cookies/auth), LF-only/obs-fold, CTLs,
 * TE/Expect/Authorization/Upgrade, ambiguous framing, queries and pipelining.
 * One request per TLS connection simplifies cancellation and request binding. */
static int strict_headers(const unsigned char *p,size_t bytes,size_t *header_bytes)
{
 size_t i,start=0,end=0,count=0,names[24],lengths[24];
 *header_bytes=0;
 for(i=0;i<bytes;i++){
  if(!p[i]||(p[i]<32&&p[i]!='\r'&&p[i]!='\n'&&p[i]!='\t')||p[i]==127)return -1;
  if(p[i]=='\n'&&(i==0||p[i-1]!='\r'))return -1;
  if(p[i]=='\r'&&i+1<bytes&&p[i+1]!='\n')return -1;
  if(i>=3&&!memcmp(p+i-3,"\r\n\r\n",4)){end=i+1;break;}
 }
 if(!end)return bytes>HEADER_MAX?-1:0;
 if(end>HEADER_MAX)return -1;
 for(i=0;i+1<end;i++)if(p[i]=='\r'&&p[i+1]=='\n'){start=i+2;break;}
 if(start==0)return -1;
 while(start<end-2){
  size_t colon=start,j,line=start;
  while(line+1<end&&!(p[line]=='\r'&&p[line+1]=='\n'))line++;
  while(colon<line&&p[colon]!=':')colon++;
  if(colon==start||colon==line||count>=24)return -1;
  for(j=start;j<colon;j++)if(!((p[j]>='a'&&p[j]<='z')||(p[j]>='A'&&p[j]<='Z')||
    (p[j]>='0'&&p[j]<='9')||strchr("!#$%&'*+-.^_`|~",p[j])))return -1;
  for(j=0;j<count;j++)if(lengths[j]==colon-start&&
    mg_strcasecmp(mg_str_n((const char *)p+names[j],lengths[j]),
                  mg_str_n((const char *)p+start,colon-start))==0)return -1;
  names[count]=start;lengths[count++]=colon-start;start=line+2;
 }
 *header_bytes=end;return 1;
}
static int validate_http(struct mg_http_message *h,size_t headers,size_t *body)
{
 struct mg_str cl=header(h,"Content-Length");size_t i;uint64_t length=0;
 if(!equal(h->proto,"HTTP/1.1")||h->uri.len>512||h->query.len||
    h->uri.len==0||h->uri.buf[0]!='/'||!equal(header(h,"Host"),app.cfg.host)||
    header(h,"Transfer-Encoding").len||header(h,"Expect").len||
    header(h,"Authorization").len||header(h,"Upgrade").len)return -1;
 if(h->method.buf!=h->message.buf||h->uri.buf!=h->method.buf+h->method.len+1||
    h->proto.buf!=h->uri.buf+h->uri.len+1||h->method.buf[h->method.len]!=' '||
    h->uri.buf[h->uri.len]!=' '||memcmp(h->proto.buf+8,"\r\n",2))return -1;
 for(i=0;i<h->uri.len;i++)if(h->uri.buf[i]=='%'||h->uri.buf[i]=='?'||
    h->uri.buf[i]=='\\'||(unsigned char)h->uri.buf[i]<=32)return -1;
 if(cl.len){
  if(cl.len>5)return -1;
  for(i=0;i<cl.len;i++){if(cl.buf[i]<'0'||cl.buf[i]>'9')return -1;length=length*10+(unsigned)(cl.buf[i]-'0');}
 }
 if(length>BODY_MAX||headers+length>16384)return -1;
 if(equal(h->method,"GET")){if(length)return -1;}
 else if(equal(h->method,"POST")){
  if(!cl.len||!equal(header(h,"Origin"),app.cfg.origin))return -1;
 }else return -1;
 *body=(size_t)length;return 0;
}
static void dispatch(struct mg_connection *c,struct mg_http_message *h,uint64_t now)
{
 int s=session_find(h,now);unsigned i;struct camera_access_request q;
 /* Fixed public UI only. The API/authentication dispatch below is unchanged. */
 if(equal(h->method,"GET")){
  const struct ui_asset *asset=NULL;size_t j;char headers[512];int n,peer;
  for(j=0;j<sizeof(ui_assets)/sizeof(ui_assets[0]);j++)
   if(equal(h->uri,ui_assets[j].route)){asset=&ui_assets[j];break;}
  if(asset){
   n=snprintf(headers,sizeof(headers),
    "Content-Type: %s\r\nCache-Control: no-store\r\nConnection: close\r\n"
    "X-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n"
    "Content-Security-Policy: default-src 'none'; script-src 'self'; style-src 'self'; "
    "connect-src 'self'; media-src blob:; form-action 'self'; base-uri 'none'; frame-ancestors 'none'\r\n",
    asset->type);
   /* Includes Mongoose's exact fixed status/Content-Length framing, before any send. */
   if(n<0||(size_t)n>=sizeof(headers)||asset->bytes>7168U||c->send.len||
      asset->bytes+(size_t)n+sizeof("HTTP/1.1 200 OK\r\nContent-Length:            \r\n\r\n")-1>8192U){
    fatal=1;c->is_closing=1;return;
   }
   peer=peer_find(c->id);
   mg_http_reply(c,200,headers,"%s",(const char *)asset->data);
   c->is_draining=1;c->is_full=1;
   if(peer>=0){app.peers[peer].done=1;app.peers[peer].drain_deadline=now+2*WEB_NS;}
   if(c->send.len>8192||web_heap_failed()){fatal=1;c->is_closing=1;}
   return;
  }
 }
 if(equal(h->method,"GET")&&equal(h->uri,"/api/session")){
  if(s>=0&&app.sessions[s].state==3&&!authorized((unsigned)s,now)){clear_session((unsigned)s);s=-1;}
  if(s>=0&&app.sessions[s].state==2){error_response(c,429);return;}
  if(s<0)s=session_new(now);
  if(s<0){error_response(c,429);return;}
  if(app.sessions[s].state==3)app.sessions[s].idle_until=now+app.cfg.idle_ns;
  session_response(c,(unsigned)s);return;
 }
 if(equal(h->method,"POST")&&equal(h->uri,"/api/login")){
  char account[64]={0};struct mg_str name=header(h,"X-Account");int ret;
  uint64_t response_ns=app.cfg.response_ns;struct mg_str wait=header(h,"X-Login-Wait-Ms");
  if(s<0||app.sessions[s].state!=1||!csrf_valid(h,(unsigned)s)){
   error_response(c,403);return;
  }
  if(name.len<1||name.len>=64||h->body.len<12||h->body.len>128||
     !equal(header(h,"Content-Type"),"application/octet-stream")){error_response(c,401);return;}
  /* Optional client wait budget can ONLY shorten HTTP lifetime. It cannot
   * shorten/extend the CPU bound or reclaim a running KDF slot. */
  if(wait.len){
   uint64_t ms=0;size_t j;
   if(wait.len>5){error_response(c,400);return;}
   for(j=0;j<wait.len;j++){
    if(wait.buf[j]<'0'||wait.buf[j]>'9'){error_response(c,400);return;}
    ms=ms*10+(unsigned)(wait.buf[j]-'0');
   }
   if(!ms||ms*1000000ULL>response_ns){error_response(c,400);return;}
   response_ns=ms*1000000ULL;
  }
  memcpy(account,name.buf,name.len);
  if(!id_valid(account,64)){wipe(account,sizeof(account));error_response(c,401);return;}
  if(app.pending.active||app.next_request==UINT64_MAX){wipe(account,sizeof(account));error_response(c,429);return;}
  ret=cp_submit(app.child,CAMERA_ACCESS_ADMIN,account,++app.next_request,
    (const unsigned char *)h->body.buf,h->body.len,now+40*WEB_NS,&app.pending.ticket);
  wipe(account,sizeof(account));
  if(ret!=CP_OK){error_response(c,ret==CP_THROTTLED||ret==CP_BUSY?429:401);return;}
  app.pending.active=1;app.pending.session=(unsigned)s;app.pending.connection=c->id;
  app.pending.session_serial=app.sessions[s].serial;app.pending.request=app.next_request;
  app.pending.credential_generation=app.cfg.credential_generation;app.pending.deadline=now+response_ns;
  app.sessions[s].state=2;wipe(app.sessions[s].csrf,32); /* one-use login challenge */
  i=(unsigned)peer_find(c->id);app.peers[i].pending=1;app.peers[i].deadline=app.pending.deadline;
  /* Uniform nonsecret owner receipt, no account/request/token/material fields. */
  puts("credential_job_admitted=1");fflush(stdout);
  /* Reads remain enabled so disconnect/extra bytes immediately cancel. */
  return;
 }
 if(s<0||!authorized((unsigned)s,now)){error_response(c,401);return;}
 if(equal(h->method,"POST")&&!csrf_valid(h,(unsigned)s)){error_response(c,403);return;}
 app.sessions[s].idle_until=now+app.cfg.idle_ns;
 {
  int peer=peer_find(c->id);
  if(peer>=0){app.peers[peer].auth_session=(unsigned)s+1;
   app.peers[peer].auth_serial=app.sessions[s].serial;}
 }
 if(equal(h->method,"POST")&&equal(h->uri,"/api/logout")){
  /* Public logout receipt contains no protected state. Terminate all OTHER
   * session-bound queued responses before revocation, retain this receipt only. */
  int peer=peer_find(c->id);if(peer>=0)app.peers[peer].auth_session=0;
  clear_session((unsigned)s);response(c,200,"{\"schema\":1,\"logged_out\":true}\n",
   "Set-Cookie: __Host-camera=; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=0\r\n");return;
 }
 if(equal(h->method,"GET")&&equal(h->uri,"/api/status")){
  char body[768];snprintf(body,sizeof(body),
   "{\"schema\":1,\"service\":\"authenticated_https\",\"device_id\":\"%s\","
   "\"credential_generation\":%llu,\"browser_media\":\"unsupported_unqualified\","
   "\"rtsp\":\"independent_lab_noauth_not_wrapped\",\"hardware_applied_known\":false,"
   "\"settings_desired_state\":\"unbound\",\"settings_revision\":null,"
   "\"security_heap_peak_bytes\":%llu,\"product_accepted\":false}\n",app.cfg.device,
   (unsigned long long)app.cfg.credential_generation,(unsigned long long)web_heap_peak());
  response(c,200,body,NULL);return;
 }
 if(equal(h->method,"GET")&&equal(h->uri,"/api/capabilities")){
  response(c,200,"{\"schema\":1,\"https_auth_api\":true,\"browser_video\":false,"
   "\"snapshot\":false,\"websocket\":false,\"control\":false,\"settings_commit\":false,"
   "\"rtsp_auth\":false,\"producer_binding\":\"unqualified\",\"all_product_flags\":false}\n",NULL);return;
 }
 if(equal(h->method,"GET")&&(equal(h->uri,"/api/media/live")||equal(h->uri,"/api/media/snapshot"))){
  request_template(&q,1,equal(h->uri,"/api/media/snapshot"));
  error_response(c,web_media_admit(&app.policy,&app.sessions[s].principal,&q,now,
   cp_healthy(app.child)&&!fatal,0,NULL,NULL));return;
 }
 /* Authenticated but never silently activate alternate WS/snapshot/media/control,
  * filesystem, diagnostics, settings or credential endpoints. */
 error_response(c,501);
}
static void cancel_pending(void)
{
 struct mg_connection *c;
 if(!app.pending.active||app.pending.abandoned)return;
 if(cp_discard(app.child,&app.pending.ticket)!=CP_OK){fatal=1;return;}
 app.pending.abandoned=1;
 c=connection_find(app.pending.connection);if(c)c->is_closing=1;
 clear_session(app.pending.session);
 /* Keep the only job binding/slot until actual bound RESULT drain. No respawn,
  * no queue, no CPU-retired claim and no alteration of throttle/generations. */
 puts("credential_job_discarded=1");fflush(stdout);
}
static void handler(struct mg_connection *c,int event,void *data)
{
 uint64_t now;int index=peer_find(c->id);unsigned i;
 (void)data;
 /* Accepted socket OPEN precedes ACCEPT; admission/TLS begins at ACCEPT only. */
 if(event==MG_EV_OPEN)return;
 if(event==MG_EV_ERROR){
  unsigned code=0;
  /* Recognize only upstream's numeric TLS failure format; never echo message,
   * peer URL, headers, certificate, key or request material. */
  if(data)(void)sscanf((const char *)data,"TLS handshake: -%x",&code);
  printf("peer_error=1 handshake=%u library_code=%u os_errno=%d\n",c->is_tls_hs,code,errno);
  return;
 }
 if(event==MG_EV_CLOSE){
  if(app.pending.active&&app.pending.connection==c->id)cancel_pending();
  if(index>=0)wipe(&app.peers[index],sizeof(app.peers[index]));
  return;
 }
 if(c->is_listening)return;
 if(fatal||now_ns(&now)||web_heap_failed()){fatal=1;c->is_closing=1;return;}
 if(event==MG_EV_ACCEPT){
  unsigned active=0,handshakes=0;struct mg_tls_opts tls={0};int size=8192;
  if(now-app.accept_window>=WEB_NS){app.accept_window=now;app.accept_count=0;}
  for(i=0;i<PEER_COUNT;i++)if(app.peers[i].used){active++;handshakes+=app.peers[i].handshake;}
  if(active>=PEER_COUNT||handshakes||app.accept_count++>=16){c->is_closing=1;return;}
  for(i=0;i<PEER_COUNT&&app.peers[i].used;i++){}
  app.peers[i].used=app.peers[i].handshake=1;app.peers[i].connection=c->id;
  app.peers[i].deadline=now+5*WEB_NS;
  if(active+1>app.peers_peak)app.peers_peak=active+1;
  app.handshake_peak=1;
  if(setsockopt((int)(uintptr_t)c->fd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size))||
     setsockopt((int)(uintptr_t)c->fd,SOL_SOCKET,SO_SNDBUF,&size,sizeof(size))){c->is_closing=1;return;}
  tls.cert=mg_str_n(app.cert,app.cert_len);tls.key=mg_str_n(app.key,app.key_len);
  mg_tls_init(c,&tls);
  /* Void init must never be treated as plaintext permission. Public flags only. */
  if(!c->is_tls||!c->is_tls_hs||c->is_closing||web_heap_failed())c->is_closing=1;
  return;
 }
 if(index<0){c->is_closing=1;return;}
 if(event==MG_EV_TLS_HS){app.peers[index].handshake=0;app.peers[index].deadline=now+5*WEB_NS;return;}
 if(event==MG_EV_POLL){
  if(app.peers[index].auth_session){
   unsigned session=app.peers[index].auth_session-1;
   if(session>=SESSION_COUNT||app.peers[index].auth_serial!=app.sessions[session].serial||
      !authorized(session,now)){c->is_closing=1;return;}
  }
  if(now>=app.peers[index].deadline||(app.peers[index].done&&
     now>=app.peers[index].drain_deadline)){
   if(app.peers[index].pending&&app.pending.active&&app.pending.connection==c->id)cancel_pending();
   c->is_closing=1;
  }
  return;
 }
 if(event==MG_EV_READ){
  struct mg_http_message h;size_t headers=0,body=0;int strict,parsed;
  if(!c->is_tls||c->is_tls_hs||app.peers[index].done||app.peers[index].pending){
    if(app.peers[index].pending&&app.pending.active&&app.pending.connection==c->id)cancel_pending();
   c->is_closing=1;return;
  }
  strict=strict_headers(c->recv.buf,c->recv.len,&headers);
  if(strict==0)return;
  if(strict<0){error_response(c,400);goto erase;}
  parsed=mg_http_parse((const char *)c->recv.buf,c->recv.len,&h);
  if(parsed<0||(size_t)parsed!=headers||validate_http(&h,headers,&body)){
   error_response(c,400);goto erase;
  }
  if(c->recv.len<headers+body)return;
  if(c->recv.len!=headers+body){error_response(c,400);goto erase;}
  if(now-app.request_window>=WEB_NS){app.request_window=now;app.request_count=0;}
  if(app.request_count++>=32){error_response(c,429);goto erase;}
  h.body=mg_str_n((char *)c->recv.buf+headers,body);app.requests++;
  /* Pump again immediately BEFORE any auth/effect dispatch. */
  cp_step(app.child,NULL);
  if(!cp_healthy(app.child)||now_ns(&now)){fatal=1;c->is_closing=1;goto erase;}
  dispatch(c,&h,now);
erase:wipe(c->recv.buf,c->recv.size);c->recv.len=0;
 }
}
static void complete_login(uint64_t now,struct cp_receipt *receipt)
{
 struct mg_connection *c;unsigned i=app.pending.session;int ret;unsigned char byte;
 ssize_t visible;
 if(!app.pending.active||receipt->status==CP_WAITING)return;
 if(app.pending.abandoned){
  if(receipt->status!=CP_DISCARDED||cp_forget(app.child,&app.pending.ticket)!=CP_OK){fatal=1;return;}
  wipe(&app.pending,sizeof(app.pending));
  puts("credential_job_drained=1");fflush(stdout);return;
 }
 c=connection_find(app.pending.connection);
 if(!c||c->is_closing||c->is_draining||!c->is_tls||c->is_tls_hs||
    i>=SESSION_COUNT||app.sessions[i].state!=2||
    app.sessions[i].serial!=app.pending.session_serial||
    app.pending.request!=app.pending.ticket.request_generation||
    now>=app.pending.deadline||now>=app.sessions[i].absolute_until||
    app.pending.credential_generation!=app.cfg.credential_generation||
    !cp_healthy(app.child)||fatal){cancel_pending();return;}
 /* Observe queued EOF/extra TLS bytes before granting. The owned socket is not
  * duplicated; positive bytes (including close_notify) cancel this one request.
  * A remote close racing AFTER this check still receives no known rotated token
  * and has no media effect: a later protected request must reauthenticate. */
 visible=recv((int)(uintptr_t)c->fd,&byte,1,MSG_PEEK|MSG_DONTWAIT);
 if(visible>=0||(errno!=EAGAIN&&errno!=EWOULDBLOCK)){cancel_pending();return;}
 ret=cp_issue_access(app.child,&app.pending.ticket,app.cfg.absolute_ns,&app.sessions[i].principal);
 if(ret==CP_OK){
  if(!mg_random(app.sessions[i].cookie,32)||!mg_random(app.sessions[i].csrf,32)||now_ns(&now)){
   clear_session(i);fatal=1;return;
  }
  app.sessions[i].state=3;app.sessions[i].absolute_until=app.sessions[i].principal.fact.expires_ns;
  app.sessions[i].idle_until=now+app.cfg.idle_ns;app.login_success++;session_response(c,i);
  }else{
  /* cp_issue consumes failure exactly once, no result supplied by HTTP. */
   clear_session(i);app.login_failure++;error_response(c,401);
  }
  {int peer=peer_find(app.pending.connection);if(peer>=0)app.peers[peer].pending=0;}
  wipe(&app.pending,sizeof(app.pending));
}
/* Private owner FD7: SOCK_SEQPACKET, no network endpoint. 16-byte LE command:
 * op(1 revoke,2 replace), payload_bytes(0 or56), increasing credential_generation.
 * Replacement carries schema/count/salt/verifier. No paths/accounts selectable.
 * Revocation/teardown precedes child change. Pending change retires service;
 * successful replacement enables policy ONLY after child's bounded ACK.
 * Responses are 8 public bytes: op/result (0 success,1 fail). No secret echo. */
static void owner_control(void)
{
 unsigned char packet[73]={0};struct iovec io={packet,sizeof(packet)};
 union {struct cmsghdr align;unsigned char bytes[CMSG_SPACE(sizeof(int)*4)];} ancillary;
 struct msghdr m={0};ssize_t n;uint32_t op,length;uint64_t generation;
 struct cw_ui_material material={0};struct camera_access_account account;int result=-1;
 unsigned char reply[8]={0};
 memset(&ancillary,0,sizeof(ancillary));m.msg_iov=&io;m.msg_iovlen=1;
 m.msg_control=ancillary.bytes;m.msg_controllen=sizeof(ancillary);
 n=recvmsg(7,&m,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
 if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){wipe(packet,sizeof(packet));return;}
 if(n<16||n>72||m.msg_flags&(MSG_TRUNC|MSG_CTRUNC)||CMSG_FIRSTHDR(&m)){
  struct cmsghdr *h;
  for(h=CMSG_FIRSTHDR(&m);h;h=CMSG_NXTHDR(&m,h))if(h->cmsg_level==SOL_SOCKET&&h->cmsg_type==SCM_RIGHTS){
   size_t bytes=h->cmsg_len>=CMSG_LEN(0)?h->cmsg_len-CMSG_LEN(0):0,j;
   for(j=0;j+sizeof(int)<=bytes;j+=sizeof(int)){int fd;memcpy(&fd,(char *)CMSG_DATA(h)+j,sizeof(fd));close(fd);}
  }
  fatal=1;goto out;
 }
 op=cp_get32(packet);length=cp_get32(packet+4);generation=cp_get64(packet+8);
 if((op!=1&&op!=2)||length!=(op==1?0:56)||n!=16+(ssize_t)length||
    generation<=app.cfg.credential_generation||(op==2&&parse_material(packet+16,&material))){fatal=1;goto out;}
 if(app.pending.active)cancel_pending();
 if(fatal)goto out;
 (void)camera_access_revoke_account(&app.policy,CAMERA_ACCESS_ADMIN,app.cfg.account);clear_all();
 if(fatal)goto out;
 result=op==1?cp_revoke(app.child,CAMERA_ACCESS_ADMIN,app.cfg.account,generation):
  cp_put_ui(app.child,CAMERA_ACCESS_ADMIN,app.cfg.account,generation,&material);
 if(result==CP_OK){app.cfg.credential_generation=generation;
  if(op==2){account_template(&account);result=camera_access_put_account(&app.policy,&account);}
 }else fatal=1;
 cp_u32(reply,op);cp_u32(reply+4,result==0?0:1);
 if(send(7,reply,sizeof(reply),MSG_DONTWAIT|MSG_NOSIGNAL)!=sizeof(reply))fatal=1;
out:wipe(&material,sizeof(material));wipe(packet,sizeof(packet));wipe(&ancillary,sizeof(ancillary));
}
static int cert_valid(void)
{
 mbedtls_x509_crt crt;mbedtls_pk_context pk;int result=-1;
 mbedtls_md_type_t signature_md;mbedtls_pk_type_t signature_pk;
 mbedtls_x509_crt_init(&crt);mbedtls_pk_init(&pk);
 if(!mbedtls_x509_crt_parse(&crt,(unsigned char *)app.cert,app.cert_len+1)&&
    !mbedtls_pk_parse_key(&pk,(unsigned char *)app.key,app.key_len+1,NULL,0,rng,NULL)&&
    mbedtls_pk_can_do(&pk,MBEDTLS_PK_ECKEY)&&mbedtls_pk_get_bitlen(&pk)==256&&
    !mbedtls_oid_get_sig_alg(&crt.sig_oid,&signature_md,&signature_pk)&&
    signature_md==MBEDTLS_MD_SHA256&&signature_pk==MBEDTLS_PK_ECDSA&&
    !mbedtls_pk_check_pair(&crt.pk,&pk,rng,NULL))result=0;
 mbedtls_pk_free(&pk);mbedtls_x509_crt_free(&crt);return result;
}
static int launch(const char *child_path)
{
 struct rlimit core={0,0};struct __user_cap_header_struct caph={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct caps[2]={{0}};struct ucred peer;socklen_t peer_size=sizeof(peer);
 struct cw_ui_material dummy={0},material={0};unsigned char wire[112]={0},entropy[32]={0};
 struct camera_access_resource resources[3]={{0}};struct camera_access_account account;
 struct cp_options options={0};size_t bytes;uint64_t now;int result=-1,socket_type; socklen_t type_size=sizeof(socket_type);
 startup_stage=1;
 if(!getuid()||getuid()!=geteuid()||getgid()!=getegid()||
    setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||
    syscall(SYS_capset,&caph,caps)||syscall(SYS_capget,&caph,caps)||
    caps[0].effective||caps[0].permitted||caps[0].inheritable||
    caps[1].effective||caps[1].permitted||caps[1].inheritable||
    mlockall(MCL_CURRENT|MCL_FUTURE))goto out;
 /* FDs only from a trusted private launch owner. No filesystem-path search. */
 startup_stage=21;
 if(read_private(6,&app.cfg,sizeof(app.cfg),&bytes)||bytes!=sizeof(app.cfg))goto out;
 startup_stage=22;if(!config_valid())goto out;
 startup_stage=23;if(read_private(3,app.cert,sizeof(app.cert)-1,&app.cert_len))goto out;
 startup_stage=24;if(read_private(4,app.key,sizeof(app.key)-1,&app.key_len))goto out;
 startup_stage=25;if(read_private(5,wire,sizeof(wire),&bytes)||bytes!=sizeof(wire))goto out;
 startup_stage=26;if(parse_material(wire,&dummy)||parse_material(wire+56,&material))goto out;
 startup_stage=27;
 if(getsockopt(7,SOL_SOCKET,SO_TYPE,&socket_type,&type_size)||socket_type!=SOCK_SEQPACKET||
    getsockopt(7,SOL_SOCKET,SO_PEERCRED,&peer,&peer_size)||peer_size!=sizeof(peer)||
    (peer.uid!=getuid()&&peer.uid!=0)||peer.pid!=getppid())goto out;
 close(3);close(4);close(5);close(6);
 startup_stage=3;
 if(!mg_random(entropy,32)||psa_crypto_init()!=PSA_SUCCESS||cert_valid()||now_ns(&now))goto out;
 if(now>UINT64_MAX-3601*WEB_NS)goto out;
 resources[0].kind=CAMERA_ACCESS_DEVICE;memcpy(resources[0].id,app.cfg.device,64);
 resources[1].kind=CAMERA_ACCESS_SENSOR;memcpy(resources[1].id,app.cfg.sensor,64);
 resources[2].kind=CAMERA_ACCESS_PROFILE;memcpy(resources[2].id,app.cfg.profile,64);
 memcpy(resources[2].sensor_id,app.cfg.sensor,64);
 startup_stage=4;
 if(camera_access_init(&app.policy,app.cfg.device,app.cfg.boot_generation,now,resources,3))goto out;
 account_template(&account);if(camera_access_put_account(&app.policy,&account))goto out;
 options.child_generation=app.cfg.child_generation;options.global_spacing_ns=WEB_NS;
  options.account_spacing_ns=WEB_NS;options.max_job_lifetime_ns=40*WEB_NS;
 options.control_timeout_ns=2*WEB_NS;options.retirement_grace_ns=100000000ULL;
 startup_stage=5;
 if(cp_start(&app.child,child_path,&options,&dummy,&app.policy)!=CP_OK||
    cp_put_ui(app.child,CAMERA_ACCESS_ADMIN,app.cfg.account,app.cfg.credential_generation,&material)!=CP_OK)goto out;
 mg_mgr_init(&app.mgr);
 startup_stage=6;
 if(!mg_listen(&app.mgr,app.cfg.listen,handler,NULL)||web_parent_sandbox())goto out;
 if(now_ns(&now))goto out;
 app.start_ns=now;result=0;
out:wipe(wire,sizeof(wire));wipe(&dummy,sizeof(dummy));wipe(&material,sizeof(material));wipe(entropy,sizeof(entropy));return result;
}
int main(int argc,char **argv)
{
 struct sigaction action={0};int result=20;uint64_t now;unsigned i;struct cp_receipt receipt;
 action.sa_handler=stop_signal;sigemptyset(&action.sa_mask);
 if(argc!=2||sigaction(SIGTERM,&action,NULL)||sigaction(SIGINT,&action,NULL))return 21;
 signal(SIGPIPE,SIG_IGN);close(0);
 if(syscall(SYS_close_range,8U,~0U,0))return 21;
 if(launch(argv[1])){printf("web_startup=denied stage=%u os_errno=%d provisioning_or_hardening_not_ready=1\n",startup_stage,errno);goto end;}
 puts("web_ready=1 https_only=1 strict_child=1 product_accepted=0");fflush(stdout);
 while(!stopping&&!fatal){
  if(now_ns(&now))break;
   if(app.cfg.runtime_ns&&now-app.start_ns>=app.cfg.runtime_ns){result=0;break;}
  owner_control();if(fatal)break;
  cp_step(app.child,&receipt);
  if(!cp_healthy(app.child)){fatal=1;break;}
  if(now_ns(&now))break;
  complete_login(now,&receipt);
  /* issue_access observes a later clock inside the reviewed helper. Never feed
   * the earlier pre-completion timestamp back into policy sweep/effects. */
  if(now_ns(&now))break;
  for(i=0;i<SESSION_COUNT;i++)if(app.sessions[i].state&&
     (now>=app.sessions[i].absolute_until||now>=app.sessions[i].idle_until)){
    if(app.pending.active&&!app.pending.abandoned&&app.pending.session==i&&
       app.pending.session_serial==app.sessions[i].serial)cancel_pending();else clear_session(i);
  }
  if(camera_access_sweep(&app.policy,now))fatal=1;
  if(!fatal)mg_mgr_poll(&app.mgr,5);
 }
 if(stopping&&!fatal)result=0;
end:clear_all();
 /* Immediate socket teardown, not drain on revoke/clock/cancel/child failure. */
 mg_mgr_free(&app.mgr);
 if(app.child){if(cp_destroy(&app.child)!=CP_OK){puts("child_retirement=uncertain");result=22;}
  else puts("child_retirement=observed");}
 {
  struct rusage self={0},child={0};int known=!getrusage(RUSAGE_SELF,&self)&&!getrusage(RUSAGE_CHILDREN,&child);
  printf("runtime_metrics_known=%d self_peak_rss_kb=%ld child_peak_rss_kb=%ld self_cpu_us=%lld child_cpu_us=%lld\n",
   known,known?self.ru_maxrss:-1L,known?child.ru_maxrss:-1L,
   known?((long long)self.ru_utime.tv_sec+self.ru_stime.tv_sec)*1000000LL+self.ru_utime.tv_usec+self.ru_stime.tv_usec:-1LL,
   known?((long long)child.ru_utime.tv_sec+child.ru_stime.tv_sec)*1000000LL+child.ru_utime.tv_usec+child.ru_stime.tv_usec:-1LL);
 }
 printf("web_exit=%d requests=%u login_success=%u login_failure=%u peers_peak=%u handshake_peak=%u heap_peak_bytes=%llu fatal=%d\n",
  result,app.requests,app.login_success,app.login_failure,app.peers_peak,app.handshake_peak,
  (unsigned long long)web_heap_peak(),fatal);
 wipe(&app,sizeof(app));munlockall();return result;
}
