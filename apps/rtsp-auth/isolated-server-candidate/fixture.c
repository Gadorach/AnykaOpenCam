/* SPDX-License-Identifier: GPL-2.0-only
 * PUBLICDUMMY owned-loopback synthetic NAL/lifecycle fixture. NOT target media,
 * a software-encoder fallback, valid decoded frames, product defaults or NVR.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#ifndef IS_SYNTHETIC_SOURCE_ONLY
#error "Fixture requires explicit PUBLIC synthetic source build; never native deployment"
#endif
#include "candidate_auth.h"
#include <camera/dma_integrity.h>
#include <mbedtls/platform_util.h>
#include <sys/random.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <poll.h>
#ifdef IS_HOST_PUBLIC_ONLY
#ifdef IS_HOST_DELAY_FIXTURE
#include "delay_pin.h"
#define IS_FIXTURE_BYTES IS_DELAY_BYTES
#define is_fixture_sha256 is_delay_sha256
#else
#include "host_pin.h"
#define IS_FIXTURE_BYTES IS_HOST_BYTES
#define is_fixture_sha256 is_host_sha256
#endif
#else
#include "native_pin.h"
#define IS_FIXTURE_BYTES IS_NATIVE_BYTES
#define is_fixture_sha256 is_native_sha256
#endif
unsigned is_owned_exits(void);unsigned is_owned_wait0(void);unsigned is_owned_signaled(void);
void is_watch_material(int);size_t is_material_reads(void);size_t is_material_max_read(void);

static volatile sig_atomic_t stopped;
static uint8_t nal[CANDIDATE_HUB_BYTES+32],hub[CANDIDATE_HUB_BYTES+32];
static uint32_t crc_table[256];
static const uint8_t params[]={0,0,0,1,0x67,1,2,3,0,0,0,1,0x68,4,5};
void candidate_test_clock_fail(void); /*HOST fixture link wrapper, real clocks unless armed*/
void candidate_test_clock_regress(void);
struct fixture {
 struct camera_producer producer;struct camera_rtsp_common_producer bridge;
  struct candidate_config config;unsigned algorithm,bootstrap_bad,random_bad,blocked,verifier_retained;
  struct rv_client *volatile retained_verifier;
  unsigned arm_discard,arm_busy,arm_revoke_partial;
 unsigned begin_mode,begin_pending,abort_end_fail;
 unsigned release_mode,release_wipes,issued_keyframe,issued_valid;
 unsigned produced_idr,produced_p,forced_nexts,end_idr,end_p,end_key_mismatch;
 uint64_t issued_token,issued_generation;size_t issued_bytes;
 unsigned starts,idles,nexts,releases,retained,guard_errors,cancel_hits,arm_partial;uint32_t crc;
 uint32_t crc_p;
 unsigned attaches,detaches,begins,ends,crc_reads,frame_retained,source_gate_calls,source_gate_denied;
 int (*source_gate)(void *);void *source_gate_context;
 struct camera_rtsp_frame_handle retained_frame;int retained_end_result;
 uint64_t cancel_after_bytes;
 struct camera_packet_hub retained_hub;struct camera_producer_demand retained_demand;
};
static uint64_t clock_ns(void *ctx)
{struct timespec t;(void)ctx;if(clock_gettime(CLOCK_MONOTONIC,&t))return UINT64_MAX;return (uint64_t)t.tv_sec*1000000000ULL+(uint64_t)t.tv_nsec;}
static int random_ready(void *ctx,unsigned char *out,size_t n)
{struct fixture *f=ctx;if(f->random_bad)return -1;return getrandom(out,n,0)==(ssize_t)n?0:-1;} /*HOST, not cold target entropy*/
static int guards(void)
{unsigned i;for(i=0;i<16;i++)if(nal[i]!=0xa5||nal[16+CANDIDATE_HUB_BYTES+i]!=0x5a||hub[i]!=0xa5||hub[16+CANDIDATE_HUB_BYTES+i]!=0x5a)return -1;return 0;}
static int source_ready(void *ctx,uint64_t connection,uint64_t now)
{
 struct fixture *f=ctx;(void)connection;(void)now;
 if(f->blocked){struct timespec t={0,450000000};f->blocked=0;nanosleep(&t,NULL);}
 if(guards()){f->guard_errors++;return -1;}return 0; /* pure synthetic guard check, NOT source-qualified hardware */
}
static int owner_poll(void *ctx,struct candidate_backend *b,uint64_t now)
{
  struct fixture *f=ctx;char command;ssize_t n;(void)now;
  if(b->pending&&f->arm_busy){
   static const char request[]="OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n\r\n";
   f->arm_busy=0;
   if(rv_submit(b->child,b->pending_peer->connection,request,sizeof(request)-1,now+1000000000ULL)!=RV_BUSY)return -1;
   b->stats.busy++;
  }
  if(b->pending&&f->arm_discard){
   f->arm_discard=0;b->pending_peer->canceled=1;
   if(!b->discarded){if(rv_discard(b->child)!=RV_OK)return -1;b->discarded=1;b->stats.discarded++;}
  }
 if(f->begin_pending){
  f->begin_pending=0;
  if(f->begin_mode==2){if(candidate_backend_revoke(b))return -1;}
  else if(f->begin_mode==3){if(candidate_backend_cancel(b,1)==CAMERA_OK)f->cancel_hits++;}
 }
  if(f->arm_partial&&b->stats.rtp_bytes>f->cancel_after_bytes){
  f->arm_partial=0;if(candidate_backend_cancel(b,1)==CAMERA_OK)f->cancel_hits++;
  }
  if(f->arm_revoke_partial&&b->stats.rtp_bytes>f->cancel_after_bytes){f->arm_revoke_partial=0;if(candidate_backend_revoke(b))return -1;}
 for(unsigned count=0;count<16;count++) {
  n=read(STDIN_FILENO,&command,1);if(n<0&&(errno==EAGAIN||errno==EINTR))return 0;if(n<=0)return 0;
   if(command=='Q'){candidate_backend_cancel(b,0);stopped=1;return 0;}
  if(command=='C'){if(candidate_backend_cancel(b,0)==CAMERA_OK)f->cancel_hits++;}
  else if(command=='1'){if(candidate_backend_cancel(b,1)==CAMERA_OK)f->cancel_hits++;}
   else if(command=='P'){f->arm_partial=1;f->cancel_after_bytes=b->stats.rtp_bytes;}
   else if(command=='V'){f->arm_revoke_partial=1;f->cancel_after_bytes=b->stats.rtp_bytes;}
   else if(command=='R'){int r=candidate_backend_revoke(b);if(r!=CAMERA_OK&&r!=CAMERA_BUSY)return -1;}
   else if(command=='D')f->arm_discard=1;
   else if(command=='J')f->arm_busy=1;
   else if(command=='K'){if(rv_owned_signal(b->child,SIGKILL)!=RV_OK)return -1;}
   else if(command=='T')candidate_test_clock_regress();
   else if(command=='B')f->blocked=1;
  else if(command=='X'){f->random_bad=1;}
 }
 return 0;
}
static void ready(void *ctx,uint16_t port)
{(void)ctx;printf("HOST_ONLY READY port=%u\n",port);fflush(stdout);}
static void retained(void *ctx,const struct camera_packet_hub *h,const struct camera_producer_demand *d,unsigned slot)
 {struct fixture *f=ctx;(void)slot;f->retained++;f->retained_hub=*h;f->retained_demand=*d;}
static void retain_verifier(void *ctx,struct rv_client *child)
{struct fixture *f=ctx;f->retained_verifier=child;f->verifier_retained++;}
static void bind_source_guard(void *ctx,int (*gate)(void *),void *context)
{struct fixture *f=ctx;f->source_gate=gate;f->source_gate_context=context;}
static void retain_frame(void *ctx,struct camera_rtsp_frame_handle source,int result)
{
 struct fixture *f=ctx;f->frame_retained++;f->retained_frame=source;f->retained_end_result=result;
 if(source.owner!=&f->producer||source.namespace_context!=&f->bridge||!source.id||
    source.id!=f->bridge.frame.id||source.generation!=f->bridge.frame.generation)f->guard_errors++;
}
static int effect_guard(struct fixture *f)
{
 f->source_gate_calls++;
 if(!f->source_gate||f->source_gate(f->source_gate_context)!=CAMERA_OK){f->source_gate_denied++;return CAMERA_NOT_READY;}
 return CAMERA_OK;
}
static int start(void *ctx)
{struct fixture *f=ctx;if(effect_guard(f))return CAMERA_NOT_READY;f->starts++;return guards()?CAMERA_IO_ERROR:CAMERA_OK;} /*logical only, NOT native start*/
static int idle(void *ctx){struct fixture *f=ctx;f->idles++;return guards()?CAMERA_IO_ERROR:CAMERA_OK;}
static int attach(void *ctx,uint64_t now,struct camera_producer_demand *d)
{struct camera_rtsp_common_producer *bridge=ctx;struct fixture *f=bridge->context;int r=camera_rtsp_common_attach(ctx,now,d);if(!r)f->attaches++;return r;}
static int detach(void *ctx,struct camera_producer_demand d,uint64_t now)
{struct camera_rtsp_common_producer *bridge=ctx;struct fixture *f=bridge->context;f->detaches++;return camera_rtsp_common_detach(ctx,d,now);}
static int begin(void *ctx,uint64_t now,uint32_t *force,struct camera_rtsp_frame_handle *source)
{
 struct camera_rtsp_common_producer *bridge=ctx;struct fixture *f=bridge->context;f->begins++;
 if(f->begin_mode==5){struct timespec t={0,450000000};nanosleep(&t,NULL);now=clock_ns(f);} /*delay BEFORE START's bound guard*/
 int r=camera_rtsp_common_begin(ctx,now,force,source);
 if(!r&&f->begin_mode&&f->begin_mode!=5) {
  if(f->begin_mode==1||f->begin_mode==4){struct timespec t={0,450000000};nanosleep(&t,NULL);}
  f->begin_pending=1; /*trusted owner command delivered at post-BEGIN poll boundary*/
 }
 return r;
}
static int frame_end(void *ctx,struct camera_rtsp_frame_handle source,int result,uint32_t keyframe,uint64_t now)
{
 struct camera_rtsp_common_producer *bridge=ctx;struct fixture *f=bridge->context;f->ends++;
 if(result==CAMERA_OK&&f->issued_valid){
  if(keyframe!=f->issued_keyframe)f->end_key_mismatch++;
  if(keyframe)f->end_idr++;else f->end_p++;
 }
 f->issued_valid=0;
 if(f->abort_end_fail&&result==CAMERA_NOT_READY)return CAMERA_IO_ERROR; /*logical END rejection; no fake retirement*/
 return camera_rtsp_common_end(ctx,source,result,keyframe,now);
}
static int retire(void *ctx,uint64_t now)
{struct camera_rtsp_common_producer *bridge=ctx;struct fixture *f=bridge->context;if(f->release_mode==6&&f->releases)return CAMERA_IO_ERROR;return camera_rtsp_common_retire(ctx,now);}
static int next(void *ctx,uint32_t force,struct camera_packet *packet)
{
 struct fixture *f=ctx;
 if(effect_guard(f))return CAMERA_NOT_READY;
 f->nexts++;f->crc_reads++;
 unsigned keyframe=f->release_mode==1?(force?1U:0U):1U;
 if(force)f->forced_nexts++;
 nal[16]=(uint8_t)(keyframe?0x65:0x41);
 uint32_t current_crc=camera_crc_bzip2_ram(nal+16,CANDIDATE_HUB_BYTES,crc_table);
 /* P differs only by its public NAL header; bytes are checked independently by
  * the client. No encoded-footage/reference correctness claim is implied. */
 if(guards()||current_crc!=(keyframe?f->crc:f->crc_p)){f->guard_errors++;return CAMERA_IO_ERROR;}
 if(keyframe)f->produced_idr++;else f->produced_p++;
 *packet=(struct camera_packet){.codec=CAMERA_H264,.keyframe=keyframe,.data=nal+16,.bytes=CANDIDATE_HUB_BYTES,
  .lease_token=f->nexts,.generation=UINT64_C(0x4000000000000000)+f->nexts,.monotonic_ns=clock_ns(f)}; /*opaque PRODUCER namespace, not common/hub gen*/
 if(f->release_mode==4)packet->keyframe=2; /*owned malformed export negative*/
 if(f->release_mode==5)packet->bytes=CANDIDATE_HUB_BYTES+1U;
 f->issued_keyframe=packet->keyframe;f->issued_token=packet->lease_token;f->issued_generation=packet->generation;f->issued_bytes=packet->bytes;f->issued_valid=1;
 if(f->release_mode==2)candidate_test_clock_fail(); /*immediate release branch after successful next*/
 return CAMERA_OK;
}
static void release(void *ctx,const struct camera_packet *p)
{
 struct fixture *f=ctx;f->releases++;
 if(p->data!=nal+16||p->bytes!=f->issued_bytes||p->keyframe!=f->issued_keyframe||p->lease_token!=f->issued_token||p->generation!=f->issued_generation||guards())f->guard_errors++;
 /* Actual API permits the backend's destructive release: invalidate ALL packet
  * metadata. This is NOT an independent DMA-stop/idle acknowledgement. */
 memset((void *)p,0,sizeof(*p));f->release_wipes++;
 if(f->release_mode==3)candidate_test_clock_fail(); /*clock failure after the normal destructive release*/
}
static void stop_signal(int sig){(void)sig;stopped=1;}
int main(int argc,char **argv)
{
  if(argc!=10||!getuid())return 2; /*fixture CLI NEVER executes as root/native*/
  char *end;
  struct rlimit core={0,0};struct sigaction normal={0};normal.sa_handler=SIG_DFL;sigemptyset(&normal.sa_mask);
  if(setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||sigaction(SIGCHLD,&normal,NULL))return 2;
  int image_fd=(int)strtol(argv[8],&end,10);if(*end||image_fd<3)return 2;
  int material_fd=(int)strtol(argv[9],&end,10);if(*end||material_fd<3||material_fd==image_fd)return 2;
  is_watch_material(material_fd);
  unsigned long port=strtoul(argv[2],&end,10);
 if(*end||port<1024||port>65535)return 2;
 unsigned long ttl=strtoul(argv[3],&end,10);if(*end||ttl<100||ttl>300000)return 2;
 unsigned long rate=strtoul(argv[4],&end,10);if(*end||!rate||rate>1024)return 2;
  unsigned long chunk=strtoul(argv[5],&end,10);if(*end||chunk>1404)return 2;
  unsigned long begin_mode=strtoul(argv[6],&end,10);if(*end||begin_mode>5)return 2;
  unsigned long release_mode=strtoul(argv[7],&end,10);if(*end||release_mode>6)return 2;
 struct fixture f={0};
 f.begin_mode=(unsigned)begin_mode;f.abort_end_fail=begin_mode==4;
 f.release_mode=(unsigned)release_mode;
 if(!strcmp(argv[1],"SHA-256"))f.algorithm=CAMERA_DIGEST_SHA256;
 else if(!strcmp(argv[1],"MD5"))f.algorithm=CAMERA_DIGEST_MD5_LEGACY;
 else if(!strcmp(argv[1],"BAD-DOMAIN")){f.algorithm=CAMERA_DIGEST_SHA256;f.bootstrap_bad=1;}
 else if(!strcmp(argv[1],"BAD-RNG")){f.algorithm=CAMERA_DIGEST_SHA256;f.random_bad=1;}
 else return 2;
 for(size_t i=0;i<sizeof(nal);i++)nal[i]=(uint8_t)((i-16)*31U^((i-16)>>8));
 nal[16]=0x65;
 memset(nal,0xa5,16);memset(nal+16+CANDIDATE_HUB_BYTES,0x5a,16);memset(hub,0xa5,16);memset(hub+16+CANDIDATE_HUB_BYTES,0x5a,16);
 camera_crc_bzip2_table(crc_table);f.crc=camera_crc_bzip2_ram(nal+16,CANDIDATE_HUB_BYTES,crc_table);
 nal[16]=0x41;f.crc_p=camera_crc_bzip2_ram(nal+16,CANDIDATE_HUB_BYTES,crc_table);nal[16]=0x65;
 struct camera_producer_config pc={.operation_timeout_ns=3000000000ULL,.keyframe_supported=1};
 if(camera_producer_init(&f.producer,&pc))return 2;
 f.bridge=(struct camera_rtsp_common_producer){.producer=&f.producer,.context=&f,.start=start,.idle=idle,.clock_ns=clock_ns};
 struct camera_rtsp_lifecycle life=camera_rtsp_common_hooks(&f.bridge);
  life.attach=attach;life.detach=detach;life.begin=begin;life.end=frame_end;life.retire=retire;
 struct camera_rtsp_producer p={.context=&f,.next=next,.release=release,.parameter_sets=params,.parameter_set_bytes=sizeof(params),.lifecycle=&life};
 static const struct camera_device devices[]={
  {"camera0","host-group",CAMERA_DEVICE_SENSOR,CAMERA_PRESENT_UNQUALIFIED,0},
  {"host-encoder","host-group",CAMERA_DEVICE_ENCODER,CAMERA_PRESENT_UNQUALIFIED,0}};
 static const struct camera_profile profiles[]={{"camera0-main","camera0","host-encoder",16,16,CAMERA_H264}};
 static const struct camera_resource_group groups[]={{"host-group",0,CAMERA_PRESENT_UNQUALIFIED}};
 struct camera_graph graph={.devices=devices,.device_count=2,.profiles=profiles,.profile_count=1,.groups=groups,.group_count=1};
  f.config=(struct candidate_config){.abi=CANDIDATE_AUTH_ABI,.struct_bytes=sizeof(f.config),.host_fixture_only=1,.algorithm=f.algorithm,.boot_generation=42,.epoch=444,.credential_generation=1,
   .device_id="PUBLICDUMMY-device",.account_id="PUBLICDUMMY-rtsp",.profile_id="camera0-main",.graph=&graph,.nonce_ttl_ms=(uint32_t)ttl,
  .rate_window_ms=1000,.global_verifies=(uint32_t)rate,.global_challenges=(uint32_t)rate,.account_verifies=(uint32_t)rate,.account_challenges=(uint32_t)rate,
   .peer_verifies=(uint32_t)rate,.peer_challenges=(uint32_t)rate,.host_send_chunk=(uint32_t)chunk,.begin_contract=CANDIDATE_BEGIN_LOGICAL_ONLY,.context=&f,.random_ready=random_ready,
   .source_ready=source_ready,.owner_poll=owner_poll,.listener_ready=ready,.retain_uncertain=retained,
   .source_guard_bind=bind_source_guard,.retain_frame_uncertain=retain_frame,.retain_verifier=retain_verifier,.material_fd=material_fd,
   .image={.abi=RV_ABI,.profile=RV_HOST_PUBLIC_4M,.fd=image_fd,.bytes=IS_FIXTURE_BYTES}};
  memcpy(f.config.image.sha256,is_fixture_sha256,32);
#ifndef IS_HOST_PUBLIC_ONLY
  f.config.host_fixture_only=0;f.config.image.profile=RV_NATIVE;
#endif
 char live[128],track[160];snprintf(live,sizeof(live),"rtsp://127.0.0.1:%lu/live",port);snprintf(track,sizeof(track),"%s/track0",live);
 struct camera_rtsp_registry registry;if(camera_rtsp_registry_init(&registry,live,track))return 2;
 const char *sdp="v=0\r\ns=PUBLICDUMMY-SYNTHETIC-NOT-DECODED-NOT-NATIVE\r\nt=0 0\r\nm=video 0 RTP/AVP 96\r\na=control:track0\r\n";
 struct camera_rtsp_options o={.bind_address="127.0.0.1",.sdp=sdp,.port=(uint16_t)port,.interval_ms=25,.send_deadline_ms=2000,
  .hub_storage=hub+16,.packet_capacity=CANDIDATE_HUB_BYTES,.hub_slots=1,.tcp_coalesce=0,.trace_every=1000,.stop=&stopped,.resources=&registry,.request_deadline_ms=2000};
 int flags=fcntl(STDIN_FILENO,F_GETFL);if(flags<0||fcntl(STDIN_FILENO,F_SETFL,flags|O_NONBLOCK))return 2;
 signal(SIGTERM,stop_signal);signal(SIGINT,stop_signal);signal(SIGPIPE,SIG_IGN);
  int result=camera_rtsp_isolated_candidate_serve(&o,&p,&f.config);
  close(image_fd);close(material_fd);
  printf("HOST_ONLY VERIFIER owned_exits=%u wait0=%u signaled=%u retained=%u functional_RTSP_verify_native_NOTtested=1\n",is_owned_exits(),is_owned_wait0(),is_owned_signaled(),f.verifier_retained);
  printf("HOST_ONLY MATERIAL parent_read_calls=%zu parent_max_read_bytes=%zu parent_secret_bytes_read=0 prefix_bound_bytes=%zu\n",is_material_reads(),is_material_max_read(),offsetof(struct rv_material,username));
  if(is_material_max_read()>offsetof(struct rv_material,username))f.guard_errors++;
  if(f.verifier_retained){fflush(stdout);for(;;)poll(NULL,0,100);} /*parent OWN fixture recovery, no unknown free*/
 printf("HOST_ONLY SOURCE starts=%u idles=%u next=%u releases=%u demands=%u frame_open=%u retained=%u guard_errors=%u cancel_hits=%u raw_NAL_CRC=%u result=%d attaches=%u detaches=%u begins=%u ends=%u crc_reads=%u frame_retained=%u needs_stop=%u stop_pending=%u active_present=%u source_state=%u source_gate_calls=%u source_gate_denied=%u guard_bound=%u release_wipes=%u produced_idr=%u produced_p=%u forced_nexts=%u end_idr=%u end_p=%u end_key_mismatch=%u\n",
  f.starts,f.idles,f.nexts,f.releases,f.producer.demand_count,f.bridge.frame_open,f.retained,f.guard_errors,f.cancel_hits,f.crc,result,
  f.attaches,f.detaches,f.begins,f.ends,f.crc_reads,f.frame_retained,f.producer.needs_stop,f.producer.stop_pending,f.producer.active_present,
  (unsigned)f.producer.state,f.source_gate_calls,f.source_gate_denied,f.source_gate!=NULL,
  f.release_wipes,f.produced_idr,f.produced_p,f.forced_nexts,f.end_idr,f.end_p,f.end_key_mismatch);
 fflush(stdout);return result==CAMERA_OK&&!f.guard_errors&&!f.retained?0:2;
}
