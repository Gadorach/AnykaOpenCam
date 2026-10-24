/* SPDX-License-Identifier: GPL-2.0-only
 * Actual owned-runtime authenticated server derivative; new source entry only.
 * See LICENSE_NOTICES.md and the preimplementation CONTRACT.md. NOT RUN.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <camera/rtsp_server.h>
#include <camera/rtp_h264.h>
#include <camera/rtsp_clock.h>
#include "candidate_auth.h"
#include "adapter_state.h"
#include "listener_custody.h"
#include "owned_runtime_v1.h"
#include "rtsp_span_server_v1.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct span_binding {
 struct candidate_prestarted *owner;
 struct camera_span_hub_v1 *hub;
 const struct camera_rtsp_span_retention_v1 *sink;
};
struct rtsp_client {
 int fd,setup,playing;struct camera_rtsp_input input;
 uint64_t touched,send_deadline;struct camera_rtp_session rtp;
 size_t sent_bytes,packet_calls;int send_error,tcp_coalesce;
 int report_au;
 const struct camera_rtsp_lifecycle *lifecycle;
 struct camera_producer_demand demand;int demand_attached,lifecycle_error;
 struct candidate_backend *backend;struct candidate_peer *auth;
  int media_lease_active,deferred_close,pending_reply,lease_uncertain;unsigned pending_seq;
 struct span_binding *span;
};
static struct camera_rtsp_clock server_clock;
/* Serialized synchronous entry only, not a thread lock or ownership grant. */
static int owned_entry_active;
/* Hold the actual process/stack, not copied view metadata. No cleanup may unwind
 * a still-held cursor. Existing native cleanup can quarantine before fallback.
 * No exit, close, free, END retry or guessed external recovery here. */
static _Noreturn void retain_span(struct span_binding *s,
 const struct camera_producer_demand *demand,unsigned index)
{
 s->owner->source_hold=1;s->owner->phase=CANDIDATE_PS_HOLD;
 s->sink->retain(s->sink->context,s->hub,demand,index);
 qc07f_owned_runtime_cleanup_v1(s->owner->config->context,server_clock.failed,1);
 for(;;)raise(SIGSTOP);
}
static _Noreturn void retain_frame(struct span_binding *s,
 struct camera_rtsp_frame_handle source,int result)
{
 s->owner->source_hold=1;s->owner->phase=CANDIDATE_PS_HOLD;
 s->owner->config->retain_frame_uncertain(s->owner->config->context,source,result);
 retain_span(s,NULL,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);
}
/* Shape only: no malloc provenance, prefault/residency, fork-order or native
 * accounting claim. Only actual trusted fresh init establishes ownership. */
static int pristine_hub(const struct camera_span_hub_v1 *h)
{
 if(!h||h->self!=h||h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||h->generation||
    h->monotonic_ns||h->issued_token||h->keyframe||h->subscribers||
    h->need_keyframe||h->pending||h->acquired||h->bytes||h->codec||
    h->allocation_calls!=CAMERA_SPAN_HUB_V1_CHUNKS||
    h->allocated_bytes!=CAMERA_RTP_H264_NAL_V1_BYTES||
    h->maximum_allocation!=CAMERA_SPAN_HUB_V1_CHUNK_BYTES)return 0;
 for(unsigned i=0;i<CAMERA_SPAN_HUB_V1_CONSUMERS;i++)if(h->active_token[i])return 0;
 for(unsigned i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++){
  uintptr_t begin=(uintptr_t)h->chunks[i];
  if(!begin||begin>UINTPTR_MAX-CAMERA_SPAN_HUB_V1_CHUNK_BYTES)return 0;
  for(unsigned j=0;j<i;j++){
   uintptr_t other=(uintptr_t)h->chunks[j];
   if(begin<other+CAMERA_SPAN_HUB_V1_CHUNK_BYTES&&
      other<begin+CAMERA_SPAN_HUB_V1_CHUNK_BYTES)return 0;
  }
 }
 return 1;
}
_Static_assert(CAMERA_HUB_CONSUMERS==CAMERA_SPAN_HUB_V1_CONSUMERS,"four peers");
_Static_assert(CANDIDATE_HUB_BYTES==CAMERA_RTP_H264_NAL_V1_BYTES,"same NAL cap");
static uint64_t monotonic_ns(void);
static int guard_denied(struct rtsp_client *c)
{
 /* Composition-specific verifier custody fence. Ordinary safe auth/policy
  * denial still returns false; unknown backend/IPC may not unwind live media. */
 if(c->backend&&(c->backend->failed||c->backend->pending||c->backend->pending_peer))
  retain_span(c->span,&c->demand,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);
 return 0;
}
static int guarded(struct rtsp_client *c)
{
  uint64_t now=camera_rtsp_clock_ns(&server_clock);
  if(now==UINT64_MAX||candidate_backend_poll(c->backend,now)!=CAMERA_OK)return guard_denied(c);
  now=camera_rtsp_clock_ns(&server_clock);
   if(now==UINT64_MAX||candidate_effect(c->backend,c->auth,now)!=CAMERA_OK)return guard_denied(c);
 /* Callback/crypto latency never grants extra lease/nonce/send time. */
  now=camera_rtsp_clock_ns(&server_clock);
    return now!=UINT64_MAX&&candidate_effect_final(c->backend,c->auth,now)==CAMERA_OK?1:guard_denied(c);
}
struct capture_gate {
 struct rtsp_client *clients;struct candidate_backend *backend;
 const struct camera_rtsp_options *options;const struct camera_rtsp_lifecycle *life;
};
/* Called after every blocking BEGIN and inside the bound START/next adapter.
 * Complete all readiness/owner callbacks FIRST, then a single fresh-clock,
 * callback-free sweep catches peers that expired while another peer's check
 * blocked. No detach/end/producer reentry while a frame bracket is open. */
static int capture_admitted(void *context)
{
  struct capture_gate *gate=context;unsigned checked=0;
 if(!gate||*gate->options->stop)return CAMERA_NOT_READY;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++) {
  struct rtsp_client *c=&gate->clients[i];
  if(c->fd>=0&&c->playing&&c->demand_attached&&c->demand.owner==gate->life->owner&&c->demand.id&&guarded(c))checked|=1U<<i;
 }
  uint64_t now=camera_rtsp_clock_ns(&server_clock);
  if(now==UINT64_MAX||*gate->options->stop)return CAMERA_IO_ERROR;
  for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if((checked&(1U<<i))&&
      candidate_effect_final(gate->backend,gate->clients[i].auth,monotonic_ns())==CAMERA_OK)return CAMERA_OK;
  return CAMERA_NOT_READY;
}
static uint64_t milliseconds(void)
{return camera_rtsp_clock_ms(&server_clock);}
static uint64_t monotonic_ns(void)
{return camera_rtsp_clock_ns(&server_clock);}
static int write_counted(struct rtsp_client *c,const uint8_t *data,size_t bytes,uint64_t deadline,int flags,size_t *sent,int protect)
{
 while(bytes){
  uint64_t now=milliseconds();if(now==UINT64_MAX||now>=deadline){errno=now==UINT64_MAX?EIO:ETIMEDOUT;return -1;}
    if((protect&&!guarded(c))||(!protect&&candidate_admission_check(c->backend,c->auth,monotonic_ns()))){errno=EACCES;return -1;}
   now=milliseconds();if(now==UINT64_MAX||now>=deadline){errno=ETIMEDOUT;return -1;}
   ssize_t n=send(c->fd,data,bytes,MSG_NOSIGNAL|MSG_DONTWAIT|flags);
  if(n<0&&errno==EINTR)continue;
  if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
    struct pollfd p={.fd=c->fd,.events=POLLOUT};int left=(int)(deadline-now);if(left>20)left=20;
    int ready=poll(&p,1,left);
   if(ready<0&&errno==EINTR)continue;
    if(ready<0)return -1;if(!ready)continue;
   if(p.revents&(POLLERR|POLLHUP|POLLNVAL)){errno=EPIPE;return -1;}continue;
  }
  if(n<=0){if(!n)errno=EPIPE;return -1;}data+=n;bytes-=n;if(sent)*sent+=(size_t)n;
 }
 return 0;
}
static int write_all(struct rtsp_client *c,const uint8_t *data,size_t bytes,uint64_t deadline,int protect)
{return write_counted(c,data,bytes,deadline,0,NULL,protect);}
static int reply(struct rtsp_client *c,unsigned seq,unsigned status,const char *extra,const char *body)
{
 char header[2048];size_t bytes=body?strlen(body):0;
 int n=snprintf(header,sizeof(header),"RTSP/1.0 %u %s\r\nCSeq: %u\r\n%sContent-Length: %zu\r\n\r\n",status,status==200?"OK":"Unsupported",seq,extra?extra:"",bytes);
 uint64_t now=milliseconds();if(now==UINT64_MAX)return -1;
 uint64_t deadline=now+100;
  return n<0||(size_t)n>=sizeof(header)||write_all(c,(uint8_t *)header,(size_t)n,deadline,1)||(bytes&&write_all(c,(const uint8_t *)body,bytes,deadline,1))?-1:0;
}
static int challenge_reply(struct rtsp_client *c,unsigned seq,enum camera_digest_wire_reason reason)
{
 unsigned char header[512];size_t bytes=0;uint64_t now=monotonic_ns();
  if(now==UINT64_MAX||candidate_challenge(c->backend,c->auth,seq,now,reason,header,&bytes))return -1;
 int r=write_all(c,header,bytes,now/1000000U+100,0);explicit_bzero(header,sizeof(header));return r;
}
static void failure_client(struct rtsp_client *c,struct camera_rtsp_frame_handle source,
                           enum camera_rtsp_failure_stage stage,int result,uint64_t happened)
{
 if(happened==UINT64_MAX||server_clock.failed){c->lifecycle_error=CAMERA_IO_ERROR;return;}
 if(c->lifecycle&&c->lifecycle->failure){
  struct camera_rtsp_failure e={source,c->demand,stage,result,happened};
  c->lifecycle->failure(c->lifecycle->context,&e);
  if(monotonic_ns()==UINT64_MAX)c->lifecycle_error=CAMERA_IO_ERROR;
 }
}
static int pause_client(struct rtsp_client *c,struct camera_span_hub_v1 *hub,unsigned index)
{
  if(server_clock.failed||c->lifecycle_error||c->lease_uncertain)
   retain_span(c->span,&c->demand,index);
  if(monotonic_ns()==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,index);}
  int r=CAMERA_OK;
  if(hub->subscribers&(1U<<index))r=camera_span_hub_unsubscribe_v1(hub,index);
  if(r!=CAMERA_OK)retain_span(c->span,&c->demand,index);
 c->playing=0;
 if(c->demand_attached){
   uint64_t now=monotonic_ns();
    if(now==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,index);}
    int detached=c->lifecycle->detach(c->lifecycle->context,c->demand,now);
    uint64_t after=monotonic_ns();if(after==UINT64_MAX&&detached==CAMERA_OK)detached=CAMERA_IO_ERROR;
   if(detached!=CAMERA_OK){
    struct camera_rtsp_frame_handle source={c->lifecycle->owner,0,c->lifecycle->owner->generation,c->lifecycle->context};
     c->lifecycle_error=detached;failure_client(c,source,CAMERA_RTSP_DETACH_FAILURE,detached,after);
     /* Notification is not an ACK; keep the actual detach error and ownership. */
     c->lifecycle_error=detached;retain_span(c->span,&c->demand,index);
   }
    c->demand_attached=0; /* ONE acknowledged call plus valid post-clock only. */
 }
 return r;
}
static void close_client(struct rtsp_client *c,struct camera_span_hub_v1 *hub,unsigned index)
{
   if(c->backend&&(c->backend->failed||c->backend->pending||c->backend->pending_peer))
    retain_span(c->span,&c->demand,index);
  /* Native clock/ownership uncertainty cannot unwind live source/server state.
   * Ordinary auth/socket errors still follow the original cleanup policy. */
   if(server_clock.failed||c->lifecycle_error||c->lease_uncertain)
    retain_span(c->span,&c->demand,index);
   if(monotonic_ns()==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,index);}
   if(c->backend)qc07f_owned_runtime_cleanup_v1(c->backend->config->context,server_clock.failed,0);
  if(c->fd<0)return;
  if(c->lease_uncertain){
    retain_span(c->span,&c->demand,index);
  }else {
   int paused=pause_client(c,hub,index);
   if(paused!=CAMERA_OK)c->lifecycle_error=paused;
    if(c->lifecycle_error)retain_span(c->span,&c->demand,index);
  }
  int lifecycle_error=c->lifecycle_error;
  if(monotonic_ns()==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,index);}
  close(c->fd);camera_rtsp_input_clear(&c->input);
     candidate_peer_close(c->backend,c->auth);
   if(c->backend->failed||c->backend->pending||c->backend->pending_peer)
    retain_span(c->span,&c->demand,index);
  /* Peer retirement may block; no newly observed fault may clear this slot. */
  if(server_clock.failed||c->lifecycle_error||c->lease_uncertain)
   retain_span(c->span,&c->demand,index);
  if(monotonic_ns()==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,index);}
 /* Keep nonsecret ownership tokens after uncertainty. Socket close is not a
  * detach or STOP receipt. Never retry an attempted callback on this slot. */
   if(server_clock.failed||lifecycle_error||(c->backend->pending&&c->backend->pending_peer==c->auth)){c->fd=-1;c->setup=c->playing=0;c->lifecycle_error=lifecycle_error?lifecycle_error:CAMERA_IO_ERROR;return;}
  struct span_binding *span=c->span;
  memset(c,0,sizeof(*c));c->fd=-1;c->lifecycle_error=lifecycle_error;c->span=span;
}
static int input_failure(struct rtsp_client *c,unsigned index,enum camera_rtsp_input_failure code)
{
 if(c->input.failure==CAMERA_RTSP_INPUT_NONE)c->input.failure=code;
 fprintf(stderr,"RTSP INPUT consumer=%u code=%u\n",index,(unsigned)c->input.failure);return -1;
}
/* HOST candidate adoption boundary. Fresh borrowed Request-Line views bind real
 * Digest before SDP/transport/subscription/demand effects; no native deployment. */
static int process_input(struct rtsp_client *c,unsigned index,struct camera_span_hub_v1 *hub,
 const struct camera_rtsp_options *o,uint32_t *force)
{
 while(c->input.used){
  struct camera_rtsp_request request;size_t bytes=0;enum camera_rtsp_method method;
  enum camera_rtsp_input_result parsed=camera_rtsp_input_next(&c->input,(unsigned)c->setup,milliseconds(),&request,&bytes);
  if(parsed==CAMERA_RTSP_INPUT_NEED_MORE)break;
  if(parsed==CAMERA_RTSP_INPUT_INVALID)return input_failure(c,index,c->input.failure);
   if(parsed==CAMERA_RTSP_INPUT_RTCP){
    if(!guarded(c))return -1;
    c->backend->stats.rtcp++;
   if(camera_rtsp_input_retire(&c->input,bytes,milliseconds()))return input_failure(c,index,c->input.failure);
   continue; /* validated bounded envelope only; no callbacks/keepalive effects */
  }
   if((!request.target_is_star&&
       !((request.target.length==strlen(o->resources->live_uri)&&!memcmp(request.target.data,o->resources->live_uri,request.target.length))||
         (request.target.length==strlen(o->resources->track_uri)&&!memcmp(request.target.data,o->resources->track_uri,request.target.length)))))return -1;
   enum camera_digest_wire_reason reason;uint64_t auth_now=monotonic_ns();
     int authenticated=candidate_request_verify(c->backend,c->auth,c->input.data,bytes,c->fd,auth_now,&reason);
   if(authenticated){
    /* Never insert a challenge into a partial RTP packet, or let a bad command
     * keep an old authenticated stream alive. No SDP/source/hub effects. */
    if(c->playing||c->media_lease_active||reason==CAMERA_DIGEST_WIRE_NO_CHALLENGE||challenge_reply(c,request.cseq,reason))return -1;
    if(camera_rtsp_input_retire(&c->input,bytes,milliseconds()))return -1;
    continue;
   }
    char session[128];const char *token=c->auth->session;
  enum camera_rtsp_input_failure bad=camera_rtsp_request_policy(&request,o->resources,
   (unsigned)c->setup,(unsigned)c->playing,token,&method);
  if(bad)return input_failure(c,index,bad);
   unsigned seq=request.cseq;int r=0;
   if(c->media_lease_active){
    /* One bounded pending control. Metadata replies wait for a full RTP packet;
     * PAUSE/TEARDOWN wait for the current AU's SAME absolute deadline. No reply
     * or detach can split a packet or release an acquired CPU lease early. */
    if(method!=CAMERA_RTSP_METHOD_GET_PARAMETER&&method!=CAMERA_RTSP_METHOD_OPTIONS&&
       method!=CAMERA_RTSP_METHOD_PAUSE&&method!=CAMERA_RTSP_METHOD_TEARDOWN)return -1;
    c->pending_reply=(int)method;c->pending_seq=seq;
    if(camera_rtsp_input_retire(&c->input,bytes,milliseconds()))return -1;
    return 0;
   }
  snprintf(session,sizeof(session),"Session: %s;timeout=30\r\n",token);
  if(method==CAMERA_RTSP_METHOD_OPTIONS)r=reply(c,seq,200,"Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, GET_PARAMETER, TEARDOWN\r\n",NULL);
   else if(method==CAMERA_RTSP_METHOD_DESCRIBE){if(!guarded(c))return -1;c->backend->stats.sdp++;r=reply(c,seq,200,"Content-Type: application/sdp\r\n",o->sdp);}
  else if(method==CAMERA_RTSP_METHOD_SETUP){
   char transport[256];snprintf(transport,sizeof(transport),"%sTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n",session);
    if(!guarded(c))return -1;c->backend->stats.setup++;
    c->rtp.channel=0;c->setup=1;r=reply(c,seq,200,transport,NULL);
  }
   else if(method==CAMERA_RTSP_METHOD_PLAY){
    if(!c->playing){
       if(!guarded(c)||camera_span_hub_subscribe_v1(hub,index)!=CAMERA_OK)return -1;
      if(c->lifecycle){
       uint64_t now=monotonic_ns();if(now==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;return -1;}
          if(!guarded(c)){
           /* Guard refusal is not permission to retire observed-uncertain state. */
           if(server_clock.failed||c->lifecycle_error||c->lease_uncertain||monotonic_ns()==UINT64_MAX)
            retain_span(c->span,&c->demand,index);
           if(camera_span_hub_unsubscribe_v1(hub,index)!=CAMERA_OK)retain_span(c->span,&c->demand,index);return -1;
          }
         int attached=c->lifecycle->attach(c->lifecycle->context,now,&c->demand);
         if(attached!=CAMERA_OK){
          /* The actual callback may itself have discovered the clock fault. */
          if(server_clock.failed||c->lifecycle_error||c->lease_uncertain||monotonic_ns()==UINT64_MAX)
           retain_span(c->span,&c->demand,index);
          if(camera_span_hub_unsubscribe_v1(hub,index)!=CAMERA_OK)retain_span(c->span,&c->demand,index);return -1;
         }
       c->demand_attached=1;
       if(monotonic_ns()==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;return -1;}
      if(c->demand.owner!=c->lifecycle->owner||!c->demand.id||c->demand.slot>=CAMERA_PRODUCER_CONSUMERS)return -1;
     }
      if(!guarded(c))return -1;c->backend->stats.play++;
      c->playing=1;*force=1;
    }
   r=reply(c,seq,200,session,NULL);
  }
   else if(method==CAMERA_RTSP_METHOD_PAUSE){
    if(!guarded(c))return -1;
    if(c->playing&&pause_client(c,hub,index)!=CAMERA_OK)return -1;
   r=reply(c,seq,200,session,NULL);
  }
  else if(method==CAMERA_RTSP_METHOD_GET_PARAMETER)r=reply(c,seq,200,c->setup?session:NULL,NULL);
  else if(method==CAMERA_RTSP_METHOD_TEARDOWN){reply(c,seq,200,NULL,NULL);return -1;}
  else r=reply(c,seq,501,NULL,NULL);
  if(r)return -1;
  if(camera_rtsp_input_retire(&c->input,bytes,milliseconds()))return input_failure(c,index,c->input.failure);
  uint64_t now=milliseconds();if(now==UINT64_MAX)return input_failure(c,index,CAMERA_RTSP_INPUT_CLOCK);
  if(method!=CAMERA_RTSP_METHOD_UNSUPPORTED)c->touched=now; /* accepted request only, never RTCP/partial bytes/501 */
 }
 return 0;
}
static int control(struct rtsp_client *c,unsigned index,struct camera_span_hub_v1 *hub,
 const struct camera_rtsp_options *o,uint32_t *force)
{
  unsigned char incoming[2048];size_t capacity=sizeof(c->input.data)-c->input.used;
  uint64_t admission=milliseconds();
   if(admission==UINT64_MAX||camera_digest_wire_admit(&c->backend->global,&c->backend->account,&c->auth->rate,CAMERA_DIGEST_WIRE_VERIFY,admission))return -1;
 if(!capacity)return input_failure(c,index,CAMERA_RTSP_INPUT_FRAMING);
 if(capacity>sizeof(incoming))capacity=sizeof(incoming);
 ssize_t n=recv(c->fd,incoming,capacity,MSG_DONTWAIT);
 if(n<0&&(errno==EINTR||errno==EAGAIN||errno==EWOULDBLOCK))return 0;
 if(n<=0)return -1;
 int appended=camera_rtsp_input_append(&c->input,incoming,(size_t)n,milliseconds());
 /* Use the same established libc zeroization as the owned receive storage. */
 explicit_bzero(incoming,sizeof(incoming));
 if(appended)return input_failure(c,index,c->input.failure);
 return process_input(c,index,hub,o,force);
}
static int pending_control(struct rtsp_client *c,struct camera_span_hub_v1 *hub,unsigned index)
{
 int method=c->pending_reply;unsigned seq=c->pending_seq;c->pending_reply=0;
 if(!method)return 0;
  char session[128];snprintf(session,sizeof(session),"Session: %s;timeout=30\r\n",c->auth->session);
 if(method==CAMERA_RTSP_METHOD_PAUSE||method==CAMERA_RTSP_METHOD_TEARDOWN){
  if(!guarded(c)||pause_client(c,hub,index)!=CAMERA_OK)return -1;
 }
 const char *extra=method==CAMERA_RTSP_METHOD_OPTIONS?"Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, GET_PARAMETER, TEARDOWN\r\n":session;
 if(reply(c,seq,200,extra,NULL))return -1;
 return method==CAMERA_RTSP_METHOD_TEARDOWN?-1:0;
}
/* One retained packet cursor per peer, not a second worst-case AU buffer.
 * All leases are released before another producer call. Writable peers make
 * progress independently while a blocked peer waits on its whole-AU deadline.
 */
struct au_cursor {
  struct camera_span_hub_view_v1 lease;
  const uint8_t *parameters[2];size_t nal_bytes[3],offset,next_offset;
 uint8_t packet[CAMERA_RTP_PACKET_BYTES];size_t packet_bytes,written;
 uint64_t started;uint32_t timestamp;unsigned phase;int active;
};
static void finish_au(struct rtsp_client *c,struct au_cursor *a,struct camera_span_hub_v1 *hub,unsigned i,int failed)
{
  if(server_clock.failed||c->lifecycle_error||c->lease_uncertain)
   retain_span(c->span,&c->demand,i);
  uint64_t now=milliseconds();
  uint64_t elapsed=now==UINT64_MAX||now<a->started?0:now-a->started;
  if(now==UINT64_MAX){c->send_error=EIO;c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,i);}
 if(failed||elapsed>500){
  struct tcp_info info={0};socklen_t bytes=sizeof(info);
  if(!getsockopt(c->fd,IPPROTO_TCP,TCP_INFO,&info,&bytes)&&bytes>=104)
   fprintf(stderr,"SHARED RTSP TCP consumer=%u timestamp=%u state=%u rto_us=%u rtt_us=%u unacked=%u lost=%u retransmits=%u total_retrans=%u snd_cwnd=%u snd_mss=%u rcv_space=%u\n",i,a->timestamp,info.tcpi_state,info.tcpi_rto,info.tcpi_rtt,info.tcpi_unacked,info.tcpi_lost,info.tcpi_retransmits,info.tcpi_total_retrans,info.tcpi_snd_cwnd,info.tcpi_snd_mss,info.tcpi_rcv_space);
 }
  if(c->report_au||failed||elapsed>500)
   fprintf(stderr,"SHARED RTSP AU consumer=%u timestamp=%u nal_bytes=%zu keyframe=%u send_ms=%llu wire_bytes=%zu packet_calls=%zu complete=%u errno=%d\n",i,a->timestamp,a->lease.bytes,a->lease.keyframe,(unsigned long long)elapsed,c->sent_bytes,c->packet_calls,!failed,c->send_error);
   /* Diagnostics may block. Observe again BEFORE exact release/flag retirement. */
   if(server_clock.failed||c->lifecycle_error||c->lease_uncertain)
    retain_span(c->span,&c->demand,i);
   if(milliseconds()==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,i);}
   int released=camera_span_hub_release_v1(hub,i,a->lease.lease_token,a->lease.generation);
   if(released!=CAMERA_OK){c->lifecycle_error=released;c->lease_uncertain=1;retain_span(c->span,&c->demand,i);}
  else c->backend->stats.lease_releases++;
   a->active=0;c->media_lease_active=0;
   if(!failed&&c->pending_reply&&pending_control(c,hub,i))failed=1;
   /* Control/auth/notification can discover a NEW fault after safe CPU release. */
   if(server_clock.failed||c->lifecycle_error||c->lease_uncertain)
    retain_span(c->span,&c->demand,i);
   if(milliseconds()==UINT64_MAX){c->lifecycle_error=CAMERA_IO_ERROR;retain_span(c->span,&c->demand,i);}
  if(failed&&a->written){c->backend->stats.partial_discard++;explicit_bzero(a->packet,sizeof(a->packet));a->written=a->packet_bytes=0;}
 if(failed){fprintf(stderr,"SHARED RTSP consumer%u disconnected on bounded send failure; partial AU is not usable video\n",i);close_client(c,hub,i);}
}
static void send_access_unit(struct rtsp_client clients[CAMERA_HUB_CONSUMERS],struct camera_span_hub_v1 *hub,const struct camera_rtsp_producer *p,const struct camera_rtsp_options *o,uint32_t budget,int report)
{
 struct au_cursor cursors[CAMERA_HUB_CONSUMERS];memset(cursors,0,sizeof(cursors));
 const uint8_t *params=p->parameter_sets;size_t count=p->parameter_set_bytes,split=4;
 int valid=count>=9&&!memcmp(params,"\0\0\0\1",4);
 if(valid){while(split+4<count&&memcmp(params+split,"\0\0\0\1",4))split++;valid=split+4<count;}
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++){
  struct rtsp_client *c=&clients[i];struct au_cursor *a=&cursors[i];
   if(!c->playing)continue;
   if(!guarded(c)){close_client(c,hub,i);continue;}
    if(camera_span_hub_acquire_v1(hub,i,&a->lease)!=CAMERA_OK)continue;
   a->active=1;c->media_lease_active=1;a->timestamp=(uint32_t)(a->lease.monotonic_ns/1000000ULL*90);
   a->started=milliseconds();
   if(a->started==UINT64_MAX){c->send_error=EIO;finish_au(c,a,hub,i,1);continue;}
   c->send_deadline=a->started+budget;
  c->sent_bytes=c->packet_calls=0;c->send_error=0;
  c->report_au=report;
  if(!valid){c->send_error=EINVAL;finish_au(c,a,hub,i,1);continue;}
   a->parameters[0]=params+4;a->nal_bytes[0]=split-4;
   a->parameters[1]=params+split+4;a->nal_bytes[1]=count-split-4;
   a->nal_bytes[2]=a->lease.bytes;
  a->phase=a->lease.keyframe?0:2;
 }
 for(;;){
   unsigned active=0;int progress=0,wait=20;
  struct pollfd fds[CAMERA_HUB_CONSUMERS];
  for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++){
   struct rtsp_client *c=&clients[i];struct au_cursor *a=&cursors[i];
    fds[i]=(struct pollfd){.fd=-1,.events=POLLOUT|POLLIN};if(!a->active)continue;
    uint64_t now=milliseconds();
     if(*o->stop||!guarded(c)||!camera_span_hub_lease_valid_v1(hub,i,&a->lease)||!c->lifecycle||c->lifecycle->owner->state!=CAMERA_PRODUCER_RUNNING){c->send_error=EACCES;finish_au(c,a,hub,i,1);continue;}
    struct pollfd input={.fd=c->fd,.events=POLLIN};uint32_t unused_force=0;
    if(!c->pending_reply&&poll(&input,1,0)>0&&(input.revents&POLLIN)&&control(c,i,hub,o,&unused_force)){c->send_error=ECANCELED;finish_au(c,a,hub,i,1);continue;}
    if((c->pending_reply==CAMERA_RTSP_METHOD_GET_PARAMETER||c->pending_reply==CAMERA_RTSP_METHOD_OPTIONS)&&(!a->packet_bytes||!a->written)){
     if(pending_control(c,hub,i)){c->send_error=EACCES;finish_au(c,a,hub,i,1);continue;}
    }
     now=milliseconds();
     if(now==UINT64_MAX||now>=c->send_deadline){c->send_error=now==UINT64_MAX?EIO:ETIMEDOUT;finish_au(c,a,hub,i,1);continue;}
    if(!a->packet_bytes){
      if(!guarded(c)||!camera_span_hub_lease_valid_v1(hub,i,&a->lease)){c->send_error=EACCES;finish_au(c,a,hub,i,1);continue;}
     now=milliseconds();if(now==UINT64_MAX||now>=c->send_deadline){c->send_error=ETIMEDOUT;finish_au(c,a,hub,i,1);continue;}
     int built=a->phase==2?
      camera_span_hub_packet_v1(hub,i,&a->lease,&c->rtp,a->timestamp,1,a->offset,a->packet,&a->packet_bytes,&a->next_offset):
      camera_rtp_h264_packet(&c->rtp,a->timestamp,a->parameters[a->phase],a->nal_bytes[a->phase],0,a->offset,a->packet,&a->packet_bytes,&a->next_offset);
     if(built!=CAMERA_OK){c->send_error=EINVAL;finish_au(c,a,hub,i,1);continue;}
    a->written=0;c->packet_calls++;
   }
   int flags=c->tcp_coalesce&&!(a->packet[5]&0x80)?MSG_MORE:0;
    /* Recheck the exact CPU lease + real auth/policy immediately before EVERY
     * packet/partial-write continuation. No whole-AU auth or TLS-buffer copies. */
     if(!guarded(c)||!camera_span_hub_lease_valid_v1(hub,i,&a->lease)){c->send_error=EACCES;finish_au(c,a,hub,i,1);continue;}
    now=milliseconds();if(now==UINT64_MAX||now>=c->send_deadline){c->send_error=ETIMEDOUT;finish_au(c,a,hub,i,1);continue;}
    c->backend->stats.rtp_calls++;
    size_t outstanding=a->packet_bytes-a->written;
    uint32_t forced=c->backend->config->host_send_chunk;
    if(forced&&outstanding>forced)outstanding=forced; /*real TCP, not a mocked send/crypto result*/
    ssize_t n=send(c->fd,a->packet+a->written,outstanding,MSG_NOSIGNAL|MSG_DONTWAIT|flags);
   if(n>0){
     progress=1;c->sent_bytes+=(size_t)n;a->written+=(size_t)n;c->backend->stats.rtp_bytes+=(size_t)n;
    if(a->written==a->packet_bytes){
     c->rtp.sequence++;a->offset=a->next_offset;a->packet_bytes=0;
     if(a->offset==a->nal_bytes[a->phase]){a->phase++;a->offset=0;}
     if(a->phase==3){finish_au(c,a,hub,i,0);continue;}
    }
   }else if(n<0&&errno==EINTR)progress=1;
   else if(n==0||(errno!=EAGAIN&&errno!=EWOULDBLOCK)){
    c->send_error=n==0?EPIPE:errno;finish_au(c,a,hub,i,1);continue;
   }
   active++;fds[i].fd=c->fd;
   now=milliseconds();int left=now>=c->send_deadline?0:(int)(c->send_deadline-now);
   if(left<wait)wait=left;
  }
  if(!active)break;if(progress)continue;
  int ready=poll(fds,CAMERA_HUB_CONSUMERS,wait);
  if(ready<0&&errno!=EINTR){
   int error=errno;
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(cursors[i].active){clients[i].send_error=error;finish_au(&clients[i],&cursors[i],hub,i,1);}
   break;
  }
 }
}
static int retire_checked(const struct camera_rtsp_lifecycle *life)
{
 uint64_t now=monotonic_ns();if(now==UINT64_MAX)return CAMERA_IO_ERROR;
 int r=life->retire(life->context,now);
 return monotonic_ns()==UINT64_MAX?CAMERA_IO_ERROR:r;
}
static int idle_checked(const struct camera_rtsp_producer *p)
{
 if(monotonic_ns()==UINT64_MAX)return CAMERA_IO_ERROR;
 int r=CAMERA_OK;if(p->idle_checked)r=p->idle_checked(p->context);else p->idle(p->context);
 return monotonic_ns()==UINT64_MAX?CAMERA_IO_ERROR:r;
}
static int begin_checked(const struct camera_rtsp_lifecycle *life,uint32_t *force,struct camera_rtsp_frame_handle *source)
{
 uint64_t now=monotonic_ns();if(now==UINT64_MAX)return CAMERA_IO_ERROR;
 int r=life->begin(life->context,now,force,source);
 return monotonic_ns()==UINT64_MAX?CAMERA_IO_ERROR:r;
}
static int end_checked(const struct camera_rtsp_lifecycle *life,struct camera_rtsp_frame_handle source,int result,uint32_t keyframe)
{
 uint64_t now=monotonic_ns();if(now==UINT64_MAX)return CAMERA_IO_ERROR;
 int r=life->end(life->context,source,result,keyframe,now);
 return monotonic_ns()==UINT64_MAX?CAMERA_IO_ERROR:r;
}
struct source_packet_facts {
 uint32_t keyframe;uint64_t lease_token,generation;size_t bytes;
};
/* Producer lease token/generation are opaque producer-owned scalar identities,
 * NOT the common frame-handle or hub CPU-lease namespace. Validate that common
 * bracket explicitly; do not invent equality/nonzero/DMA receipt semantics for
 * packet fields. Native next/release must validate their own export internally.
 * No payload pointer survives release; only validated scalar facts do. */
static int source_packet_validate(const struct camera_packet *packet,const struct camera_rtsp_lifecycle *life,
 struct camera_rtsp_frame_handle source,size_t capacity,struct source_packet_facts *facts)
{
 memset(facts,0,sizeof(*facts));
 if(!life||source.owner!=life->owner||source.namespace_context!=life->context||!source.id||
    source.generation!=life->owner->generation||life->owner->state!=CAMERA_PRODUCER_RUNNING)return CAMERA_STALE_LEASE;
 if(packet->codec!=CAMERA_H264||!packet->data||packet->bytes<2||packet->bytes>capacity||packet->keyframe>1||
    (packet->data[0]&31)!=(packet->keyframe?5U:1U))return CAMERA_INVALID;
 facts->keyframe=packet->keyframe;facts->lease_token=packet->lease_token;facts->generation=packet->generation;facts->bytes=packet->bytes;
 return CAMERA_OK;
}
/* Borrowed contiguous source -> <=8 copy-input spans, not allocation-cap proof
 * for the actual producer. No input pointer is stored by successful publish. */
static int publish_source(struct camera_span_hub_v1 *hub,const struct camera_packet *packet)
{
 if(!packet||!packet->data||!packet->bytes||packet->bytes>CAMERA_RTP_H264_NAL_V1_BYTES)return CAMERA_INVALID;
 struct camera_rtp_h264_span_v1 spans[CAMERA_SPAN_HUB_V1_CHUNKS];
 size_t offset=0,count=0;
 while(offset<packet->bytes){
  size_t take=packet->bytes-offset;
  if(take>CAMERA_SPAN_HUB_V1_CHUNK_BYTES)take=CAMERA_SPAN_HUB_V1_CHUNK_BYTES;
  spans[count++]=(struct camera_rtp_h264_span_v1){packet->data+offset,take};
  offset+=take;
 }
 const struct camera_span_hub_input_v1 input={.codec=packet->codec,
  .keyframe=packet->keyframe,.monotonic_ns=packet->monotonic_ns,
  .bytes=packet->bytes,.span_count=count,.spans=spans};
 return camera_span_hub_publish_v1(hub,&input);
}
static int serve_prestarted_body(struct candidate_prestarted *owner,const struct camera_rtsp_options *o,const struct camera_rtsp_producer *p,
 struct camera_span_hub_v1 *hub,const struct camera_rtsp_span_retention_v1 *sink,
 int *caller_listener,struct camera_rtsp_owned_listener_outcome_v1 *outcome)
{
 struct candidate_backend *backend=&owner->backend;
 const struct candidate_config *config=owner->config;
 if(!o||!p||!o->bind_address||!o->sdp||o->port<1024||!o->stop||o->interval_ms>5000||
      !o->resources||(o->request_deadline_ms&&(o->request_deadline_ms<500||o->request_deadline_ms>5000))||
     (o->send_deadline_ms&&(o->send_deadline_ms<10||o->send_deadline_ms>2000))||o->tcp_coalesce>1||o->trace_every>1000||
      !p->next||!p->release||(!p->idle&&!p->idle_checked&&!p->lifecycle)||!p->parameter_sets||!p->parameter_set_bytes||
      p->parameter_set_bytes>256||o->hub_storage||!sink->context||!sink->retain||!pristine_hub(hub))return CAMERA_INVALID;
   const struct camera_rtsp_lifecycle *life=p->lifecycle;
   if(strcmp(o->bind_address,"127.0.0.1")||o->send_deadline_ms!=2000||o->hub_slots!=1||o->packet_capacity!=CANDIDATE_HUB_BYTES||!life)return CAMERA_INVALID;
   if(*caller_listener<0||*caller_listener==config->image.fd||
      *caller_listener==config->material_fd)return CAMERA_INVALID;
  struct camera_rtsp_registry checked_resources;
    if(camera_rtsp_registry_init(&checked_resources,o->resources->live_uri,o->resources->track_uri))return CAMERA_INVALID;
    if(life&&(!life->owner||!life->attach||!life->detach||!life->begin||!life->end||!life->retire))return CAMERA_INVALID;
 uint32_t send_budget=o->send_deadline_ms?o->send_deadline_ms:100;
   struct span_binding span={owner,hub,sink};
   struct owned_listener_observation borrowed;
   int result=owned_listener_observe(*caller_listener,o->port,&borrowed);
   if(result!=CAMERA_OK)return result; /* Caller still owns actual listener. */
   if(candidate_backend_poll(backend,monotonic_ns())!=CAMERA_OK)return CAMERA_NOT_READY;
   /* Poll may call owner policy. Require actual unchanged backend association,
    * ready child and empty peer/pending state AFTER that callback, before move. */
   if(backend->config!=config||!backend->initialized||backend->failed||backend->revoked||
      backend->pending||backend->pending_peer||rv_ready_flags(backend->child)!=RV_FLAGS15||
      rv_ready_profile(backend->child)!=config->image.profile||!is_rv_running(backend->child))
    return CAMERA_NOT_READY;
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(backend->peers[i])return CAMERA_NOT_READY;
    if(monotonic_ns()==UINT64_MAX)return CAMERA_IO_ERROR;
    if(!pristine_hub(hub))return CAMERA_NOT_READY; /* after owner callbacks */
   int listener=-1;
   result=owned_listener_move(caller_listener,o->port,&borrowed,&listener,outcome);
   if(result!=CAMERA_OK)return result;
   result=CAMERA_IO_ERROR; /* Every path below consumed actual caller FD. */
   struct rtsp_client clients[CAMERA_HUB_CONSUMERS];memset(clients,0,sizeof(clients));
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++){clients[i].fd=-1;clients[i].span=&span;}
   struct capture_gate gate={clients,backend,o,life};
  config->source_guard_bind(config->context,capture_admitted,&gate);
  uint64_t next=0;uint32_t force=0;int was_active=0,retirement_failed=0;unsigned long trace_frame=0;
  uint32_t trace_every=o->trace_every?o->trace_every:1;
   fprintf(stderr,"HOST_ONLY RTSP DIGEST CANDIDATE fanout=4 TCP-unencrypted whole_AU_ms=2000 hub_slots=1 NOT-NATIVE NOT-NVR\n");
   if(config->listener_ready)config->listener_ready(config->context,o->port);
 while(!*o->stop){
  struct pollfd fds[CAMERA_HUB_CONSUMERS+1];fds[0]=(struct pollfd){.fd=listener,.events=POLLIN};
  unsigned active=0;for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++){fds[i+1]=(struct pollfd){.fd=clients[i].fd,.events=POLLIN};active+=clients[i].playing;}
   uint64_t now=milliseconds();if(now==UINT64_MAX){result=CAMERA_IO_ERROR;goto clients_done;}
     if(candidate_backend_poll(backend,monotonic_ns())!=CAMERA_OK){result=CAMERA_IO_ERROR;goto clients_done;}
    int wait_ms=!active?20:now>=next?0:(next-now>20?20:(int)(next-now));
  for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].fd>=0&&clients[i].input.deadline_active){
   uint64_t elapsed=now>=clients[i].input.started_ms?now-clients[i].input.started_ms:UINT64_MAX;
   int left=elapsed>=clients[i].input.deadline_ms?0:(int)(clients[i].input.deadline_ms-elapsed);
   if(left<wait_ms)wait_ms=left;
  }
  int ready=poll(fds,CAMERA_HUB_CONSUMERS+1,wait_ms);
  if(ready<0&&errno==EINTR)continue;if(ready<0)break;
  if(fds[0].revents&POLLIN){
   for(unsigned attempt=0;attempt<CAMERA_HUB_CONSUMERS+1;attempt++){
    int fd=accept(listener,NULL,NULL);if(fd<0)break;
     unsigned i;for(i=0;i<CAMERA_HUB_CONSUMERS&&(clients[i].fd>=0||clients[i].lifecycle_error);i++);
    if(i==CAMERA_HUB_CONSUMERS){close(fd);continue;}
    int send_bytes=32768;
    if(setsockopt(fd,SOL_SOCKET,SO_SNDBUF,&send_bytes,sizeof(send_bytes))){close(fd);continue;}
    struct timeval timeout={0,100000};setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
       clients[i].fd=fd;clients[i].touched=milliseconds();
        clients[i].backend=backend;clients[i].auth=&owner->peers[i];
         clients[i].span=&span;
         if(candidate_peer_open(backend,clients[i].auth,i,monotonic_ns())){close_client(&clients[i],hub,i);continue;}
       if(camera_rtsp_input_init(&clients[i].input,clients[i].touched,o->request_deadline_ms)){close_client(&clients[i],hub,i);continue;}
      clients[i].tcp_coalesce=(int)o->tcp_coalesce;
      clients[i].lifecycle=life;
    clients[i].rtp=(struct camera_rtp_session){.ssrc=(uint32_t)milliseconds()^(0x43410000+i),.sequence=1,.payload_type=96};
   }
  }
  for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++){
   struct rtsp_client *c=&clients[i];if(c->fd<0)continue;
      if((c->auth->grant_present&&!guarded(c))||c->auth->canceled||(fds[i+1].revents&(POLLERR|POLLHUP|POLLNVAL))||
       (camera_rtsp_input_tick(&c->input,milliseconds())&&input_failure(c,i,c->input.failure))||
        ((fds[i+1].revents&POLLIN)&&control(c,i,hub,o,&force))||
         (!c->playing&&milliseconds()-c->touched>=30000))close_client(c,hub,i);
     if(c->fd>=0&&!c->pending_reply&&c->input.used&&process_input(c,i,hub,o,&force))close_client(c,hub,i);
   }
   if(server_clock.failed){result=CAMERA_IO_ERROR;goto clients_done;}
   active=0;for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)active+=clients[i].playing;
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].lifecycle_error){result=clients[i].lifecycle_error;goto clients_done;}
   if(life){int retired=retire_checked(life);if(retired!=CAMERA_OK){retirement_failed=1;result=retired;goto clients_done;}}
   if(!active){
    if(!life&&was_active){
      int retired=idle_checked(p);if(retired!=CAMERA_OK){retirement_failed=1;result=retired;goto clients_done;}
    }
    was_active=0;continue;
   }
   was_active=1;uint64_t due=milliseconds();if(due==UINT64_MAX){result=CAMERA_IO_ERROR;goto clients_done;}if(due<next)continue;
     for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].playing&&!guarded(&clients[i]))close_client(&clients[i],hub,i);
    active=0;for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)active+=clients[i].playing;
    if(!active)continue;
    struct camera_rtsp_frame_handle source={0};
     if(capture_admitted(&gate)!=CAMERA_OK){result=CAMERA_NOT_READY;goto clients_done;}
      backend->stats.begin_calls++;
    int prepared=life?begin_checked(life,&force,&source):CAMERA_OK;
   if(life&&(source.namespace_context!=life->context||source.owner!=life->owner||source.generation!=life->owner->generation)){
    if(prepared==CAMERA_OK)prepared=CAMERA_STALE_LEASE;
    source=(struct camera_rtsp_frame_handle){life->owner,0,life->owner->generation,life->context};
   }
   if(prepared!=CAMERA_OK){
    fprintf(stderr,"SHARED RTSP lifecycle begin failed=%d generation=%llu; fail closed\n",prepared,(unsigned long long)source.generation);
    uint64_t happened=monotonic_ns();
     for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].playing){failure_client(&clients[i],source,CAMERA_RTSP_START_FAILURE,prepared,happened);close_client(&clients[i],hub,i);}
    result=prepared;goto clients_done;
   }
    /* BEGIN may consume the nonce/owner/readiness interval. Never let its prior
     * admission authorize driver acquisition. An abort ends EXACT source with
     * a failure result, not a fabricated P/keyframe success or reference reset. */
    int admitted=capture_admitted(&gate);
    if(admitted!=CAMERA_OK||life->owner->state!=CAMERA_PRODUCER_RUNNING||source.generation!=life->owner->generation){
      backend->stats.boundary_denied++;backend->stats.frame_ends++;
     int ended=end_checked(life,source,CAMERA_NOT_READY,0);
      if(ended!=CAMERA_OK)retain_frame(&span,source,ended);
     for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].demand_attached){
      if(ended!=CAMERA_OK)clients[i].lifecycle_error=ended;
       close_client(&clients[i],hub,i);
     }
     result=ended!=CAMERA_OK?ended:admitted!=CAMERA_OK?admitted:CAMERA_STALE_LEASE;
     goto clients_done; /* no next/release/publish, no automatic END/STOP retry */
    }
      backend->stats.next_calls++;
     struct camera_packet packet={0};int produced=p->next(p->context,force,&packet);
    struct source_packet_facts facts={0};
    int packet_valid=produced==CAMERA_OK?source_packet_validate(&packet,life,source,o->packet_capacity,&facts):CAMERA_INVALID;
     for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].lifecycle_error||clients[i].lease_uncertain)
      retain_frame(&span,source,clients[i].lifecycle_error?clients[i].lifecycle_error:CAMERA_IO_ERROR);
     if(server_clock.failed||monotonic_ns()==UINT64_MAX){
       /* Keep the actual packet/native owner/frame bracket live on uncertainty. */
       retain_frame(&span,source,CAMERA_IO_ERROR); /*no fabricated END time / independent DMA-stop claim*/
    }
   if(produced!=CAMERA_OK){
       if(life){backend->stats.frame_ends++;int ended=end_checked(life,source,produced,0);if(ended!=CAMERA_OK)retain_frame(&span,source,ended);}
    fprintf(stderr,"SHARED RTSP producer failed=%d; no silent fallback\n",produced);
    uint64_t happened=monotonic_ns();
     for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].playing){failure_client(&clients[i],source,CAMERA_RTSP_SOURCE_FAILURE,produced,happened);close_client(&clients[i],hub,i);}
    if(life){result=produced;goto clients_done;}
    {int retired=idle_checked(p);if(retired!=CAMERA_OK){retirement_failed=1;result=retired;goto clients_done;}}
    was_active=0;force=1;continue;
  }
   unsigned allowed=0;
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].playing){
    if(guarded(&clients[i]))allowed++;
      else {
       if(server_clock.failed||clients[i].lifecycle_error||clients[i].lease_uncertain||monotonic_ns()==UINT64_MAX)
        retain_frame(&span,source,clients[i].lifecycle_error?clients[i].lifecycle_error:CAMERA_IO_ERROR);
       if(camera_span_hub_unsubscribe_v1(hub,i)!=CAMERA_OK)retain_span(&span,&clients[i].demand,i);clients[i].playing=0;clients[i].deferred_close=1;
      }
   }
    int published=packet_valid;
    if(published==CAMERA_OK&&allowed){
     /* Revalidate source generation/namespace after readiness callbacks, before
      * owned copy. Hub publish establishes its own token/generation; deferred
       * RTP effects continue to use v2 consumer lease validation, never producer facts. */
     published=source_packet_validate(&packet,life,source,o->packet_capacity,&facts);
      /*Each child CHECK may block. Require one CURRENT peer immediately before
       *publish, not an earlier count whose last peer has expired in other IPC.*/
       if(published==CAMERA_OK)published=capture_admitted(&gate);
       /* A precopy guard may observe MAX even when another peer was admitted. */
       for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].lifecycle_error||clients[i].lease_uncertain)
        retain_frame(&span,source,clients[i].lifecycle_error?clients[i].lifecycle_error:CAMERA_IO_ERROR);
       if(server_clock.failed||monotonic_ns()==UINT64_MAX)retain_frame(&span,source,CAMERA_IO_ERROR);
         if(published==CAMERA_OK){backend->stats.hub_publish_calls++;published=publish_source(hub,&packet);}
     }
     /* Safe denials still release/END normally; observed uncertainty never does. */
     for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].lifecycle_error||clients[i].lease_uncertain)
      retain_frame(&span,source,clients[i].lifecycle_error?clients[i].lifecycle_error:CAMERA_IO_ERROR);
     if(server_clock.failed||monotonic_ns()==UINT64_MAX)retain_frame(&span,source,CAMERA_IO_ERROR);
     p->release(p->context,&packet);
    /* RELEASE is void and may invalidate/zero all packet metadata and payload.
     * From here on only independent scalars / hub-owned copies may be used. */
    if(monotonic_ns()==UINT64_MAX){
      retain_frame(&span,source,CAMERA_IO_ERROR);
    }
       if(life){backend->stats.frame_ends++;int ended=end_checked(life,source,published,facts.keyframe);if(ended!=CAMERA_OK)retain_frame(&span,source,ended);}
   if(published!=CAMERA_OK){
   fprintf(stderr,"SHARED RTSP packet rejected=%d; disconnect affected consumers\n",published);
    uint64_t happened=monotonic_ns();
     for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].playing){failure_client(&clients[i],source,CAMERA_RTSP_PACKET_FAILURE,published,happened);close_client(&clients[i],hub,i);}
    if(life){result=published;goto clients_done;}
    {int retired=idle_checked(p);if(retired!=CAMERA_OK){retirement_failed=1;result=retired;goto clients_done;}}
    was_active=0;force=1;continue;
  }
  force=0;
   int report=!(trace_frame++%trace_every);
    if(report)fprintf(stderr,"SHARED RTSP published consumers=%u keyframe=%u one-producer\n",active,facts.keyframe);
      for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].deferred_close)close_client(&clients[i],hub,i);
      send_access_unit(clients,hub,p,o,send_budget,report);
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].lifecycle_error){result=clients[i].lifecycle_error;goto clients_done;}
   if(life){int retired=retire_checked(life);if(retired!=CAMERA_OK){retirement_failed=1;result=retired;goto clients_done;}}
   uint64_t scheduled=milliseconds();if(scheduled==UINT64_MAX){result=CAMERA_IO_ERROR;goto clients_done;}next=scheduled+o->interval_ms;
  }
  result=*o->stop?CAMERA_OK:CAMERA_IO_ERROR;
    clients_done:
     qc07f_owned_runtime_cleanup_v1(config->context,server_clock.failed,0);
     if(retirement_failed||server_clock.failed||owner->source_hold||backend->failed||
        backend->revoked||backend->revoke_pending||backend->pending||backend->pending_peer)
     retain_span(&span,NULL,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);
    /* Failed global retirement was retained above BEFORE detach/STOP/FD cleanup. */
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)close_client(&clients[i],hub,i);
   if(camera_span_hub_stop_v1(hub)!=CAMERA_OK||!camera_span_hub_drained_v1(hub))
    retain_span(&span,NULL,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(hub->active_token[i])
    retain_span(&span,NULL,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].lifecycle_error)result=clients[i].lifecycle_error;
   for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)if(clients[i].lease_uncertain||clients[i].lifecycle_error)retirement_failed=1;
  /* Do not retry an already failed retirement in final cleanup. Common context
   * remains caller-owned/nonquiescent; the owner must handle actual recovery.
   */
  if(server_clock.failed){result=CAMERA_IO_ERROR;fprintf(stderr,"RTSP clock fault; terminal, retain owner demands/work; NO retirement claim\n");}
  else if(life){
    if(!retirement_failed&&life->owner->state!=CAMERA_PRODUCER_FAILED){int retired=retire_checked(life);if(retired!=CAMERA_OK)retain_span(&span,NULL,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);}
  }else if(!retirement_failed){int retired=idle_checked(p);if(retired!=CAMERA_OK)result=retired;}
   /* Preserve source/client/hub cleanup order; unbind while the gate is live,
    * then clear private listener ownership before the one close attempt. */
      if(life->owner->state!=CAMERA_PRODUCER_STOPPED||life->owner->demand_count||life->owner->active_present)
       retain_span(&span,NULL,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);
      /* Actual serialized tail, not a caller-supplied drained Boolean. Every
       * send_access_unit returned only after its cursors' exact CPU releases;
       * close_client then retired the corresponding demands and peer slots. */
      for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++){
       const struct rtsp_client *c=&clients[i];
       if(c->fd!=-1||c->setup||c->playing||c->demand_attached||c->demand.id||
          c->demand.owner||c->media_lease_active||c->deferred_close||c->pending_reply||
          c->lease_uncertain||c->lifecycle_error||c->auth||c->backend)
        retain_span(&span,&c->demand,i);
      }
      if(server_clock.failed||monotonic_ns()==UINT64_MAX||
         gate.clients!=clients||gate.backend!=backend||gate.options!=o||gate.life!=life)
       retain_span(&span,NULL,CAMERA_RTSP_SPAN_SERVER_SOURCE_INDEX_V1);
      qc07f_owned_runtime_span_end_borrow_v1(config->context,hub,capture_admitted,&gate);
      qc07f_owned_runtime_cleanup_v1(config->context,server_clock.failed,1);
    config->source_guard_bind(config->context,NULL,NULL);
   int listener_close_result=owned_listener_close_once(&listener,outcome);
   fprintf(stderr,"HOST_ONLY SUMMARY opened=%llu closed=%llu auth_denied=%llu verified=%llu sdp=%llu setup=%llu play=%llu rtp_calls=%llu rtp_bytes=%llu lease_releases=%llu partial_discard=%llu begin_calls=%llu next_calls=%llu frame_ends=%llu hub_publish_calls=%llu boundary_denied=%llu\n",
    (unsigned long long)backend->stats.opened,(unsigned long long)backend->stats.closed,(unsigned long long)backend->stats.auth_denied,
    (unsigned long long)backend->stats.verified,(unsigned long long)backend->stats.sdp,(unsigned long long)backend->stats.setup,
    (unsigned long long)backend->stats.play,(unsigned long long)backend->stats.rtp_calls,(unsigned long long)backend->stats.rtp_bytes,
     (unsigned long long)backend->stats.lease_releases,(unsigned long long)backend->stats.partial_discard,
     (unsigned long long)backend->stats.begin_calls,(unsigned long long)backend->stats.next_calls,(unsigned long long)backend->stats.frame_ends,
     (unsigned long long)backend->stats.hub_publish_calls,(unsigned long long)backend->stats.boundary_denied);
   fprintf(stderr,"HOST_ONLY IPC checks=%llu admission_checks=%llu submitted=%llu retired_results=%llu discarded=%llu busy=%llu IPC_total_ns=%llu IPC_max_ns=%llu backend_bytes=%zu peer_bytes=%zu clock_same_boot=1 principal_export_NOTsupported=1 rotation_NOTsupported=1\n",
     (unsigned long long)backend->stats.checks,(unsigned long long)backend->stats.admission_checks,(unsigned long long)backend->stats.submitted,
     (unsigned long long)backend->stats.retired_results,(unsigned long long)backend->stats.discarded,(unsigned long long)backend->stats.busy,
     (unsigned long long)backend->stats.IPC_total_ns,(unsigned long long)backend->stats.IPC_max_ns,sizeof(*backend),sizeof(struct candidate_peer));
    if(server_clock.failed||life->owner->state!=CAMERA_PRODUCER_STOPPED||
       life->owner->demand_count||life->owner->active_present)owner->source_hold=1;
   if(listener_close_result!=CAMERA_OK){
    owner->source_hold=1; /* Includes close error even if source loop was clean. */
    if(result==CAMERA_OK)result=CAMERA_IO_ERROR;
   }
   return result; /* actual stable backend left for explicit owner finish */
}

static int serve_bound_runtime(struct candidate_prestarted *owner,
  const struct camera_rtsp_options *options,const struct camera_rtsp_producer *producer,
 struct camera_span_hub_v1 *hub,const struct camera_rtsp_span_retention_v1 *sink,
 int *listener_fd,struct camera_rtsp_owned_listener_outcome_v1 *outcome)
{
 /* FIRST: never clobber active caller state/outcome, even for nested NULL args. */
 if(owned_entry_active)return CAMERA_BUSY;
  if(!owner||!listener_fd||!outcome||!hub||!sink||owner->self!=owner||!owner->config)return CAMERA_INVALID;
 if(owner->phase!=CANDIDATE_PS_READY)return CAMERA_NOT_READY;
 owned_entry_active=1;
 owner->phase=CANDIDATE_PS_SERVING; /* consumes even an entry/option refusal */
 *outcome=(struct camera_rtsp_owned_listener_outcome_v1){.result=CAMERA_INVALID};
 struct candidate_backend *b=&owner->backend;
 int result=CAMERA_NOT_READY;
 if(b->config==owner->config&&b->initialized&&!b->failed&&!b->revoked&&
    !b->pending&&!b->pending_peer&&rv_ready_flags(b->child)==RV_FLAGS15&&
    rv_ready_profile(b->child)==owner->config->image.profile&&is_rv_running(b->child)){
  unsigned occupied=0;
  for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)occupied|=b->peers[i]!=NULL;
   if(!occupied)result=serve_prestarted_body(owner,options,producer,hub,sink,listener_fd,outcome);
 }
 unsigned unresolved=b->pending||b->pending_peer||b->failed||b->revoked||owner->source_hold;
 for(unsigned i=0;i<CAMERA_HUB_CONSUMERS;i++)unresolved|=b->peers[i]!=NULL;
 int final=result?result:unresolved?CAMERA_NOT_READY:CAMERA_OK;
 owner->serve_result=final;
 owner->phase=final?CANDIDATE_PS_HOLD:CANDIDATE_PS_SERVED;
 outcome->result=final;
 owned_entry_active=0; /* Result/custody publication precedes reentry release. */
  return final;
}

int qc07f_owned_runtime_span_serve_v1(struct qc07f_owned_runtime_v1 *r,
 const struct camera_rtsp_options *options,int *listener_fd,
 struct camera_rtsp_owned_listener_outcome_v1 *outcome)
{
 /* Preserve BUSY-first/no-outcome-touch. Trusted caller prohibits reentry. */
 if(owned_entry_active)return CAMERA_BUSY;
 int result=qc07f_owned_runtime_span_entry_v1(r);
 if(result!=CAMERA_OK){
  qc07f_owned_runtime_cleanup_v1(r,0,1);
  return result;
 }
 /* Only THIS runtime's stable actual source/owner/paid hub/typed native sink. */
 result=serve_bound_runtime(r->owner,options,&r->source,r->span_hub,&r->span_sink,
                           listener_fd,outcome);
 /* Includes ordinary pre-move option/backend refusal. Live borrowed/native
  * custody cannot unwind into an owner's ordinary destructor/finish path.
  * After acknowledged normal borrow end this checks native + scalar terminal
  * history without ever following a retired hub/ledger owner pointer. */
 qc07f_owned_runtime_cleanup_v1(r,server_clock.failed,1);
 return result;
}
