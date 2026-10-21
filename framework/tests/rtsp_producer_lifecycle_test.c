/* SPDX-License-Identifier: GPL-2.0-only
 * Real shared-server paths, loopback-only peers and mocked QC retirement syscalls.
 * Does not execute a firmware binary, open a device or request a camera endpoint.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <assert.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include "../../bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "../src/rtsp_server.c"
#ifndef QC_FIXTURE_LEGACY
#define QC_FIXTURE_RECEIPTS 1
#include "receipt-model/capture-retirement.h"
#define reply qc_receipt_helper_reply
#include "receipt-model/capture-retirement.c"
#undef reply
#endif
static struct camera_rtsp_registry test_resources;
static struct camera_rtsp_options test_options={.sdp="v=0\r\n",.resources=&test_resources};

/* The fallback is used ONLY to compile positive regressions against frozen
 * pre-fix source. Current hooks require the distinct frame-handle type.
 */
#ifdef CAMERA_RTSP_FRAME_HANDLE_API
typedef struct camera_rtsp_frame_handle fixture_frame_handle;
#else
typedef struct camera_producer_token fixture_frame_handle;
#endif

/* Invoke QC source paths with synthetic descriptor/network mocks. The legacy
 * regression enters capture's error path, but never performs actual capture,
 * opens a real device, or reaches an encoder transaction.
 */
static unsigned qc_closes,qc_ends,qc_sensor;
static volatile sig_atomic_t stop; /* same tentative object as included lab main */
static int qc_end_error,qc_close_error;
static unsigned qc_accepts,qc_client_closes,qc_listener_closes,qc_opens,qc_begins,qc_nexts,qc_watchdog_closes,qc_live_handles;
static unsigned qc_polls,qc_recvs,qc_sends;
static int qc_legacy_mode,qc_stream_on,qc_request_sent;
static const void *qc_request_override;
static size_t qc_request_override_bytes;
/* Finite review clock/fragment hooks; all paths are still synthetic syscalls. */
static int (*qc_poll_hook)(struct pollfd *,nfds_t,int);
static ssize_t (*qc_recv_hook)(void *,size_t);
static void (*qc_send_hook)(void);
#ifdef QC_FIXTURE_RECEIPTS
/* Same copied kernel receipt helper; fake callbacks perform no MMIO/DMA. */
static struct qc_retirement_registry qc_receipt_ids;
static struct qc_retirement_source qc_receipt_source;
static struct qc_retirement_file qc_receipt_file;
static unsigned qc_binds,qc_arms,qc_queries,qc_acks,qc_magic;
struct qc_receipt_copy {struct qc_capture_receipt *q;};
static int qc_receipt_from(void *context,struct qc_capture_receipt *q)
{*q=*((struct qc_receipt_copy *)context)->q;return 0;}
static int qc_receipt_to(void *context,const struct qc_capture_receipt *q)
{*((struct qc_receipt_copy *)context)->q=*q;return 0;}
static int qc_receipt_command(unsigned long command,struct qc_capture_receipt *q)
{
 struct qc_receipt_copy copy={q};struct qc_retirement_uaccess access={qc_receipt_from,qc_receipt_to,&copy};
 return qc_retirement_ioctl(&qc_receipt_file,command,&access);
}
static int qc_receipt_begin(void *context)
{(void)context;qc_begins++;return 0;}
static int qc_receipt_stop(void *context)
{(void)context;return 0;}
static __u64 qc_receipt_clock(void *context)
{struct timespec t;(void)context;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (__u64)t.tv_sec*1000000000ULL+t.tv_nsec;}
static struct qc_retirement_ops qc_receipt_ops(void)
{struct qc_retirement_ops ops={qc_receipt_begin,qc_receipt_stop,qc_receipt_clock,NULL};return ops;}
static void qc_fill_next(struct qc_isp_next *);
#endif
static int qc_mock_ioctl(int fd,unsigned long command,...)
{
 va_list ap;va_start(ap,command);
 if(command==I2C_RDWR){
  assert(fd==40);struct i2c_rdwr_ioctl_data *r=va_arg(ap,struct i2c_rdwr_ioctl_data *);
  assert(r->nmsgs==2);unsigned reg=((unsigned)r->msgs[0].buf[0]<<8)|r->msgs[0].buf[1];
  unsigned value=0;
  if(reg==0x0100)value=(unsigned)qc_stream_on;
  else if(qc_legacy_mode){
   if(reg==0x3107)value=0xda;else if(reg==0x3108)value=0x4d;
   else if(reg==0x3208)value=5;else if(reg==0x320a)value=2;else if(reg==0x320b)value=0xd0;
  }
  r->msgs[1].buf[0]=(uint8_t)value;qc_sensor++;va_end(ap);return 2;
 }
 if(command==WDIOC_SETTIMEOUT){assert(qc_legacy_mode&&fd==43);int *timeout=va_arg(ap,int *);assert(*timeout==5);va_end(ap);return 0;}
 if(command==I2C_SLAVE){assert(qc_legacy_mode&&fd==40);va_end(ap);return 0;}
#ifdef QC_FIXTURE_RECEIPTS
 if(command==QC_ISP_RECEIPT_BIND||command==QC_ISP_RECEIPT_ARM||command==QC_ISP_RECEIPT_QUERY||command==QC_ISP_RECEIPT_ACK){
  assert(fd==41&&(qc_live_handles&2));struct qc_capture_receipt *q=va_arg(ap,struct qc_capture_receipt *);va_end(ap);
  if(command==QC_ISP_RECEIPT_BIND)qc_binds++;else if(command==QC_ISP_RECEIPT_ARM)qc_arms++;
  else if(command==QC_ISP_RECEIPT_QUERY)qc_queries++;else{assert(!qc_stream_on);qc_acks++;}
  int r=qc_receipt_command(command,q);if(r){errno=-r;return -1;}return 0;
 }
 if(command==QC_ISP_BEGIN||command==QC_ISP_BEGIN_IRQ){
  assert(qc_legacy_mode&&fd==41&&qc_binds==1&&qc_arms==1&&qc_queries>=1);
  va_end(ap);struct qc_retirement_ops ops=qc_receipt_ops();int r=qc_retirement_begin(&qc_receipt_file,&ops);if(r){errno=-r;return -1;}return 0;
 }
 if(command==QC_ISP_NEXT){
  assert((qc_legacy_mode==1||qc_legacy_mode==3)&&fd==41&&qc_receipt_file.state==QC_RECEIPT_ACTIVE);
  struct qc_isp_next *q=va_arg(ap,struct qc_isp_next *);qc_nexts++;va_end(ap);
  if(qc_legacy_mode==3){qc_fill_next(q);return 0;}errno=EIO;return -1;
 }
#else
 if(command==QC_ISP_BEGIN||command==QC_ISP_BEGIN_IRQ){assert(qc_legacy_mode&&fd==41);qc_begins++;va_end(ap);return 0;}
 if(command==QC_ISP_NEXT){assert(qc_legacy_mode==1&&fd==41);qc_nexts++;va_end(ap);errno=EIO;return -1;}
#endif
 va_end(ap);assert(fd==41&&command==QC_ISP_END);qc_ends++;
 if(qc_end_error){errno=qc_end_error;return -1;}
#ifdef QC_FIXTURE_RECEIPTS
 assert(!qc_stream_on);struct qc_retirement_ops ops=qc_receipt_ops();int r=qc_retirement_stop(&qc_receipt_file,QC_STOP_END,&ops);if(r){errno=-r;return -1;}
#endif
 return 0;
}
static ssize_t qc_mock_write(int fd,const void *data,size_t bytes)
{
 if(fd==43){assert(qc_legacy_mode&&bytes==1);
#ifdef QC_FIXTURE_RECEIPTS
  if(*(const char *)data=='V')qc_magic++;
#endif
  return 1;}
 assert(fd==40&&bytes==3&&((const uint8_t *)data)[0]==1&&((const uint8_t *)data)[1]==0);
 qc_stream_on=((const uint8_t *)data)[2];assert(qc_stream_on==0||qc_stream_on==1);return (ssize_t)bytes;
}
static int qc_mock_close(int fd)
{
 if(fd==50){qc_client_closes++;if(qc_client_closes>=2)stop=1;return 0;}
 if(fd==60){qc_listener_closes++;return 0;}
 if(fd==43){assert(qc_legacy_mode);qc_watchdog_closes++;qc_live_handles&=~8U;return 0;}
 assert(fd>=40&&fd<=42);qc_closes++;
#ifdef QC_FIXTURE_RECEIPTS
 if(qc_close_error&&fd==42){errno=EIO;return -1;}
 if(fd==40)assert(!qc_stream_on);
 if(fd==41){assert(qc_receipt_file.state==QC_RECEIPT_NONE&&!qc_receipt_source.hold&&qc_acks==1);struct qc_retirement_ops ops=qc_receipt_ops();assert(!qc_retirement_close(&qc_receipt_file,&ops));}
#endif
 qc_live_handles&=~(1U<<(fd-40));
 if(qc_close_error&&fd==42){errno=EIO;return -1;}return 0;
}
static int qc_mock_open(const char *path,int flags,...)
{
 (void)flags;
#ifdef QC_FIXTURE_RECEIPTS
 assert(qc_legacy_mode==1||qc_legacy_mode==3);
#else
 assert(qc_legacy_mode==1);
#endif
 qc_opens++;
 if(!strcmp(path,"/dev/i2c-0")){qc_live_handles|=1;return 40;}
 if(!strcmp(path,"/dev/qc07f-frame")){
#ifdef QC_FIXTURE_RECEIPTS
  assert(!qc_retirement_open(&qc_receipt_source,&qc_receipt_file));
#endif
  qc_live_handles|=2;return 41;}
 assert(!strcmp(path,"/dev/watchdog0"));qc_live_handles|=8;return 43;
}
static int qc_mock_socket(int domain,int type,int protocol)
{assert(qc_legacy_mode&&domain==AF_INET&&type==SOCK_STREAM&&!protocol);return 60;}
static int qc_mock_bind(int fd,const struct sockaddr *address,socklen_t bytes)
{(void)address;assert(qc_legacy_mode&&fd==60&&bytes==sizeof(struct sockaddr_in));return 0;}
static int qc_mock_listen(int fd,int backlog)
{assert(qc_legacy_mode&&fd==60&&backlog==1);return 0;}
static int qc_mock_setsockopt(int fd,int level,int name,const void *value,socklen_t bytes)
{(void)name;(void)value;(void)bytes;assert(qc_legacy_mode&&(fd==50||fd==60)&&level==SOL_SOCKET);return 0;}
static int qc_mock_poll(struct pollfd *fds,nfds_t count,int timeout)
{assert(qc_legacy_mode&&count==1&&(fds[0].fd==50||fds[0].fd==60)&&timeout>=0);qc_polls++;if(qc_poll_hook)return qc_poll_hook(fds,count,timeout);fds[0].revents=POLLIN;return 1;}
static int qc_mock_accept(int fd,struct sockaddr *address,socklen_t *bytes)
{(void)address;(void)bytes;assert(qc_legacy_mode&&fd==60&&qc_accepts<2);qc_accepts++;qc_request_sent=0;return 50;}
static ssize_t qc_mock_recv(int fd,void *data,size_t bytes,int flags)
{
 assert(qc_legacy_mode&&fd==50&&flags==MSG_DONTWAIT);
 if(qc_recv_hook){qc_recvs++;return qc_recv_hook(data,bytes);}
 assert(!qc_request_sent);qc_request_sent=1;qc_recvs++;
 const char *request=qc_legacy_mode==1?
  "SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\nPLAY rtsp://mock/live RTSP/1.0\r\nCSeq: 2\r\nSession: QC07F1\r\n\r\n":
  "SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\nTEARDOWN rtsp://mock/live RTSP/1.0\r\nCSeq: 2\r\nSession: QC07F1\r\n\r\n";
 const void *wire=qc_request_override?qc_request_override:request;
 size_t n=qc_request_override?qc_request_override_bytes:strlen(request);
 assert(n<bytes);memcpy(data,wire,n);return (ssize_t)n;
}
static ssize_t qc_mock_send(int fd,const void *data,size_t bytes,int flags)
{(void)data;assert(qc_legacy_mode&&fd==50&&bytes&&flags==MSG_NOSIGNAL);qc_sends++;if(qc_send_hook)qc_send_hook();return (ssize_t)bytes;}
typedef void (*qc_signal_handler)(int);
static qc_signal_handler qc_mock_signal(int signal_number,qc_signal_handler handler)
{(void)signal_number;(void)handler;assert(qc_legacy_mode);return SIG_DFL;}
#define open qc_mock_open
#define socket qc_mock_socket
#define bind qc_mock_bind
#define listen qc_mock_listen
#define setsockopt qc_mock_setsockopt
#define poll qc_mock_poll
#define accept qc_mock_accept
#define recv qc_mock_recv
#define send qc_mock_send
#define signal qc_mock_signal
#define ioctl qc_mock_ioctl
#define write qc_mock_write
#define close qc_mock_close
#define milliseconds qc_lab_milliseconds
#define reply qc_lab_reply
#define main qc_lab_main
#ifndef QC_FIXTURE_ADAPTER_SOURCE
#define QC_FIXTURE_ADAPTER_SOURCE "../../bringup/qc07f/userland/rtspd.c"
#endif
#include QC_FIXTURE_ADAPTER_SOURCE
#undef ioctl
#undef write
#undef close
#undef milliseconds
#undef reply
#undef main
#undef open
#undef socket
#undef bind
#undef listen
#undef setsockopt
#undef poll
#undef accept
#undef recv
#undef send
#undef signal
#ifdef QC_FIXTURE_RECEIPTS
static void qc_fill_next(struct qc_isp_next *q)
{
 assert(q->bytes==sizeof(raw)&&q->user_ptr==(uint32_t)(uintptr_t)raw);
 memset(raw,0x69,sizeof(raw));q->sequence=qc_nexts;q->status=1;q->guard_errors=0;
 q->crc32_bzip2=qc_crc_bzip2(raw,sizeof(raw));
}
#endif

static const uint8_t parameters[]={0,0,0,1,0x67,1,2,3,0,0,0,1,0x68,4,5};
struct fixture {
 struct camera_producer producer;
 struct camera_rtsp_common_producer bridge;
 struct camera_rtsp_lifecycle hooks;
 struct camera_rtsp_producer source;
 struct camera_packet_hub hub;uint8_t storage[128],nal[16];
 unsigned starts,idles,produced,released,owned,keyframes,detaches,failures;
 struct camera_rtsp_failure failure[4];
 struct camera_producer_event event[64];unsigned events;
 int start_error,idle_error,source_error,invalid_packet,detach_error,stop_after_frame;
 volatile sig_atomic_t stop_requested;int serve_result;
};
static uint64_t fixture_clock(void *context)
{(void)context;return monotonic_ns();}
static int fixture_start(void *context)
{struct fixture *f=context;f->starts++;assert(!f->owned);return f->start_error;}
static int fixture_idle(void *context)
{struct fixture *f=context;f->idles++;assert(!f->owned);return f->idle_error;}
static void fixture_event(void *context,const struct camera_producer_event *e)
{struct fixture *f=context;assert(f->events<64);f->event[f->events++]=*e;}
static void fixture_failure(void *context,const struct camera_rtsp_failure *e)
{
 struct camera_rtsp_common_producer *a=context;struct fixture *f=a->context;
 assert(f->failures<4);f->failure[f->failures++]=*e;
}
static int fixture_detach(void *context,struct camera_producer_demand demand,uint64_t now)
{
 struct camera_rtsp_common_producer *a=context;struct fixture *f=a->context;
 f->detaches++;int r=camera_rtsp_common_detach(context,demand,now);
 return r?r:f->detach_error; /* injected error AFTER actual reference release */
}
static int fixture_next(void *context,uint32_t keyframe,struct camera_packet *p)
{
 struct fixture *f=context;assert(!f->owned&&(!f->source.lifecycle||f->producer.state==CAMERA_PRODUCER_RUNNING));
 if(f->producer.demand_count>=2&&f->source_error)return f->source_error;
 f->nal[0]=f->invalid_packet&&f->producer.demand_count>=2?0x67:keyframe?0x65:0x41;
 *p=(struct camera_packet){.codec=CAMERA_H264,.data=f->nal,.bytes=sizeof(f->nal),
  .keyframe=keyframe,.monotonic_ns=monotonic_ns()};
 f->produced++;f->owned=1;f->keyframes+=!!keyframe;
 if(f->stop_after_frame)f->stop_requested=1;return CAMERA_OK;
}
static void fixture_release(void *context,const struct camera_packet *p)
{struct fixture *f=context;assert(f->owned&&p->data==f->nal);f->owned=0;f->released++;}
static void fixture_init(struct fixture *f)
{
 memset(f,0,sizeof(*f));
 struct camera_producer_config c={.operation_timeout_ns=10000000000ULL,.keyframe_supported=1,
  .event=fixture_event,.event_context=f};
 assert(camera_producer_init(&f->producer,&c)==CAMERA_OK);
 f->bridge=(struct camera_rtsp_common_producer){.producer=&f->producer,.context=f,
  .start=fixture_start,.idle=fixture_idle,.clock_ns=fixture_clock};
 f->hooks=camera_rtsp_common_hooks(&f->bridge);f->hooks.detach=fixture_detach;f->hooks.failure=fixture_failure;
 f->source=(struct camera_rtsp_producer){.context=f,.next=fixture_next,.release=fixture_release,
  .idle_checked=fixture_idle,.parameter_sets=parameters,.parameter_set_bytes=sizeof(parameters),.lifecycle=&f->hooks};
 assert(camera_hub_init_count(&f->hub,f->storage,sizeof(f->storage),1)==CAMERA_OK);
}
static uint16_t listener(int *fd)
{
 *fd=socket(AF_INET,SOCK_STREAM,0);assert(*fd>=0);
 struct sockaddr_in a={.sin_family=AF_INET,.sin_addr={htonl(INADDR_LOOPBACK)},.sin_port=0};
 assert(!bind(*fd,(struct sockaddr *)&a,sizeof(a))&&!listen(*fd,4));
 socklen_t n=sizeof(a);assert(!getsockname(*fd,(struct sockaddr *)&a,&n));return ntohs(a.sin_port);
}
static int connect_loopback(uint16_t port)
{
 int fd=socket(AF_INET,SOCK_STREAM,0);assert(fd>=0);
 struct sockaddr_in a={.sin_family=AF_INET,.sin_addr={htonl(INADDR_LOOPBACK)},.sin_port=htons(port)};
 if(connect(fd,(struct sockaddr *)&a,sizeof(a))){close(fd);return -1;}return fd;
}
static int peer(struct fixture *f,struct rtsp_client *c)
{
 int fd;uint16_t port=listener(&fd);int writer=connect_loopback(port);assert(writer>=0);
 memset(c,0,sizeof(*c));c->fd=accept(fd,NULL,NULL);assert(c->fd>=0);close(fd);
 assert(!camera_rtsp_input_init(&c->input,milliseconds(),2000));
 c->lifecycle=&f->hooks;c->rtp=(struct camera_rtp_session){.sequence=1,.payload_type=96};return writer;
}
static void request(int fd,const char *method,unsigned index)
{
 char text[256],extra[96];
 if(!strcmp(method,"SETUP"))snprintf(extra,sizeof(extra),"Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n");
 else snprintf(extra,sizeof(extra),"Session: CAM%u\r\n",index+1);
 int n=snprintf(text,sizeof(text),"%s rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 1\r\n%s\r\n",method,extra);
 assert(n>0&&(size_t)n<sizeof(text));assert(send(fd,text,(size_t)n,MSG_NOSIGNAL)==n);
}
static int command_client(struct fixture *f,struct rtsp_client *c,unsigned index,int writer,const char *method,uint32_t *force)
{request(writer,method,index);return control(c,index,&f->hub,&test_options,force);}
static fixture_frame_handle frame(struct fixture *f,uint32_t *force)
{
 fixture_frame_handle t;assert(f->hooks.begin(f->hooks.context,monotonic_ns(),force,&t)==CAMERA_OK);
 struct camera_packet p;assert(fixture_next(f,*force,&p)==CAMERA_OK);
 assert(camera_hub_publish(&f->hub,&p)==CAMERA_OK);fixture_release(f,&p);
 assert(f->hooks.end(f->hooks.context,t,CAMERA_OK,p.keyframe,monotonic_ns())==CAMERA_OK);return t;
}
/* R1: the review's retired KEYFRAME id2 collides numerically with ordinary
 * frame2. A wrong-domain handle must NOT consume the genuine bracket.
 */
static void token_domain_regression(void)
{
 struct fixture f;fixture_init(&f);struct camera_producer_demand d;uint32_t force=1;
 assert(f.hooks.attach(f.hooks.context,monotonic_ns(),&d)==CAMERA_OK);
 fixture_frame_handle first=frame(&f,&force);struct camera_producer_token primitive=f.bridge.keyframe;
 assert(first.id==1&&primitive.id==2);
 fixture_frame_handle current;force=0;
 assert(f.hooks.begin(f.hooks.context,monotonic_ns(),&force,&current)==CAMERA_OK);
 assert(current.id==primitive.id&&current.owner==primitive.owner&&current.generation==primitive.generation);
 struct camera_packet p;assert(fixture_next(&f,force,&p)==CAMERA_OK);fixture_release(&f,&p);
 fixture_frame_handle crossover={0};
 crossover.owner=primitive.owner;crossover.id=primitive.id;crossover.generation=primitive.generation;
 assert(f.hooks.end(f.hooks.context,crossover,CAMERA_OK,p.keyframe,monotonic_ns())==CAMERA_STALE_LEASE);
 assert(f.bridge.frame_open&&f.producer.state==CAMERA_PRODUCER_RUNNING);
#ifdef CAMERA_RTSP_FRAME_HANDLE_API
 struct camera_rtsp_common_producer other={.producer=&f.producer};
 fixture_frame_handle foreign_namespace=current;foreign_namespace.namespace_context=&other;
 assert(f.hooks.end(f.hooks.context,foreign_namespace,CAMERA_OK,p.keyframe,monotonic_ns())==CAMERA_STALE_LEASE);
 assert(f.bridge.frame_open&&current.namespace_context==&f.bridge);
#endif
 assert(f.hooks.end(f.hooks.context,current,CAMERA_OK,p.keyframe,monotonic_ns())==CAMERA_OK);
 assert(!f.bridge.frame_open&&f.produced==f.released);
 assert(f.hooks.detach(f.hooks.context,d,monotonic_ns())==CAMERA_OK);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==1);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==1);
 puts("PASS token-domain rejection: colliding primitive fields and foreign bridge leave bracket open; genuine frame retires; one STOP");
}
static void qc_legacy_reset_mocks(int mode)
{
 qc_legacy_mode=mode;qc_stream_on=qc_request_sent=0;stop=0;
 qc_closes=qc_ends=qc_sensor=qc_accepts=qc_client_closes=qc_listener_closes=0;
 qc_opens=qc_begins=qc_nexts=qc_watchdog_closes=qc_live_handles=qc_polls=qc_recvs=qc_sends=0;
 qc_close_error=0;qc_end_error=mode==1?EPERM:0;
 qc_request_override=NULL;qc_request_override_bytes=0;
 qc_poll_hook=NULL;qc_recv_hook=NULL;qc_send_hook=NULL;
#ifdef QC_FIXTURE_RECEIPTS
 memset(&qc_receipt_ids,0,sizeof(qc_receipt_ids));memset(&qc_receipt_source,0,sizeof(qc_receipt_source));memset(&qc_receipt_file,0,sizeof(qc_receipt_file));
 assert(!qc_retirement_source_init(&qc_receipt_ids,&qc_receipt_source));qc_binds=qc_arms=qc_queries=qc_acks=qc_magic=0;
#endif
 hardware_h264=hardware_jpeg=h264_p_frames=continuous_capture=capture_irq=fanout=0;
 fixture=NULL;send_deadline_ms=500;trace_every=1;tcp_coalesce=0;
}
/* R2: execute actual legacy main/listener/session code, with ALL QC networking,
 * device opens and ioctls replaced by synthetic descriptor mocks. No encoder
 * transaction or successful capture is performed; NEXT deliberately fails.
 */
static void legacy_ownership_regression(void)
{
 char *argv[]={"rtspd","--hardware-h264","--continuous-capture","--resource-uri","rtsp://mock/live",NULL};
 qc_legacy_reset_mocks(2);
 assert(qc_lab_main(5,argv)==0);
 assert(qc_accepts==2&&qc_client_closes==2&&qc_listener_closes==1&&!qc_opens&&!qc_begins&&!qc_ends);
 puts("PASS healthy legacy listener still admits two mock sessions and preserves TEARDOWN cleanup");
 qc_legacy_reset_mocks(1);
 int result=qc_lab_main(5,argv);
 assert(result!=0); /* pre-fix main wrongly accepts again and returns success */
 assert(qc_accepts==1&&qc_client_closes==1&&qc_listener_closes==1);
#ifdef QC_RTSP_RECEIPT_API
 assert(qc_begins==1&&qc_nexts==1&&qc_opens==3&&!qc_watchdog_closes&&!qc_magic);
 assert(!qc_ends&&!qc_closes&&qc_live_handles==11&&qc_binds==1&&qc_arms==1&&!qc_acks&&qc_receipt_source.hold==&qc_receipt_file);
 puts("PASS receipt terminal NEXT error: one admission/NEXT; zero END/ACK/close/magic; sensor/frame/watchdog retained; no restart");
#else
 assert(qc_begins==1&&qc_nexts==1&&qc_opens==3&&qc_watchdog_closes==1);
 assert(qc_ends==2&&!qc_closes&&qc_live_handles==3); /* no END ACK or hidden retry */
 puts("PASS terminal legacy error: one admission/NEXT, two existing checked END attempts, zero source closes, sensor/frame references retained; no restart");
#endif
#ifdef QC_RTSP_LEGACY_SERVICE_API
 qc_legacy_reset_mocks(1);
 hardware_h264=continuous_capture=1; /* same mode selected by lab main above */
 struct qc_legacy_service service;source_context_init(&service.source);service.terminal_result=CAMERA_OK;
 struct qc_rtsp_context *stable=&service.source;
 assert(client_session(&service,50)==CAMERA_IO_ERROR);
 assert(service.terminal_result==CAMERA_IO_ERROR&&service.source.retirement_uncertain);
 assert(service.source.continuous_sensor==40&&service.source.continuous_frame==41&&service.source.h264_stream_fd==-1);
 unsigned opens=qc_opens,ends=qc_ends,polls=qc_polls,recvs=qc_recvs;
 assert(client_session(&service,50)==CAMERA_IO_ERROR); /* refused, socket only closed */
 stop=0; /* prove terminal admission gating, not the fixture's stop counter */
 assert(serve_legacy_clients(&service,60)==CAMERA_IO_ERROR);
 assert(&service.source==stable&&qc_opens==opens&&qc_ends==ends&&qc_polls==polls&&qc_recvs==recvs);
#ifdef QC_RTSP_RECEIPT_API
 assert(!qc_accepts&&qc_client_closes==2&&!qc_closes&&qc_live_handles==11&&service.source.watchdog_hold==43&&!qc_magic&&!qc_ends&&!qc_acks);
#else
 assert(!qc_accepts&&qc_client_closes==2&&!qc_closes&&qc_live_handles==3);
#endif
 assert(start_h264_producer(&service.source)==CAMERA_NOT_READY);
 puts("PASS stable legacy context retains exact 40/41 descriptors and sticky uncertainty; subsequent session/accept blocked without retirement retry");
#endif
 qc_legacy_mode=0;stop=0;qc_end_error=qc_close_error=qc_stream_on=0;
}
static void demand_paths(void)
{
 struct fixture f;fixture_init(&f);struct rtsp_client clients[4]={{0}};
 for(unsigned i=0;i<4;i++)clients[i].fd=-1;
 int a=peer(&f,&clients[0]),b=peer(&f,&clients[1]);uint32_t force=0;
 assert(!command_client(&f,&clients[0],0,a,"SETUP",&force));
 assert(!command_client(&f,&clients[0],0,a,"PLAY",&force));
 struct camera_producer_demand first=clients[0].demand;
 assert(!command_client(&f,&clients[0],0,a,"PLAY",&force));
 assert(f.producer.demand_count==1&&clients[0].demand.id==first.id);
 assert(!command_client(&f,&clients[1],1,b,"SETUP",&force));
 assert(!command_client(&f,&clients[1],1,b,"PLAY",&force));
 assert(f.producer.demand_count==2);fixture_frame_handle old=frame(&f,&force);
 assert(f.starts==1&&f.keyframes==1&&f.released==1); /* coalesced two joins */
 send_access_unit(clients,&f.hub,&f.source,2000,0);
 assert(!command_client(&f,&clients[0],0,a,"PAUSE",&force));
 assert(f.producer.demand_count==1&&f.detaches==1);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==0);
 assert(!command_client(&f,&clients[0],0,a,"PLAY",&force));
 assert(clients[0].demand.id!=first.id&&f.producer.demand_count==2);
 fixture_frame_handle current=frame(&f,&force);
 assert(current.id!=old.id&&f.keyframes==2);
 assert(f.hooks.end(f.hooks.context,old,CAMERA_IO_ERROR,0,monotonic_ns())==CAMERA_STALE_LEASE);
 send_access_unit(clients,&f.hub,&f.source,2000,0);
 assert(command_client(&f,&clients[0],0,a,"TEARDOWN",&force)==-1);
 close_client(&clients[0],&f.hub,0);close_client(&clients[0],&f.hub,0);
 assert(f.producer.demand_count==1&&f.detaches==2);
 close(b);b=-1;assert(control(&clients[1],1,&f.hub,&test_options,&force)==-1);
 close_client(&clients[1],&f.hub,1);assert(f.detaches==3&&f.producer.demand_count==0);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==1);
 assert(f.producer.state==CAMERA_PRODUCER_STOPPED);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==1);
 close(a);
 /* Recorder is an actual common demand, not a fabricated RTSP client. */
 struct camera_producer_demand recorder;
 assert(camera_producer_attach(&f.producer,CAMERA_PRODUCER_RECORDER,monotonic_ns(),&recorder)==CAMERA_OK);
 a=peer(&f,&clients[0]);assert(!command_client(&f,&clients[0],0,a,"SETUP",&force));
 assert(!command_client(&f,&clients[0],0,a,"PLAY",&force));frame(&f,&force);
 close_client(&clients[0],&f.hub,0);close(a);
 assert(f.producer.demand_count==1&&f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==1);
 assert(camera_producer_detach(&f.producer,recorder,monotonic_ns())==CAMERA_OK);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==2);
}
static void fail_closed_hooks(void)
{
 struct fixture f;fixture_init(&f);struct camera_producer_demand d;
 assert(f.hooks.attach(f.hooks.context,monotonic_ns(),&d)==CAMERA_OK);
 uint32_t force=1;fixture_frame_handle t;f.start_error=CAMERA_IO_ERROR;
 assert(f.hooks.begin(f.hooks.context,monotonic_ns(),&force,&t)==CAMERA_IO_ERROR);
 assert(f.producer.state==CAMERA_PRODUCER_FAILED&&!f.produced&&!f.bridge.frame_open);
 assert(f.hooks.detach(f.hooks.context,d,monotonic_ns())==CAMERA_OK);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==1);
 fixture_init(&f);assert(f.hooks.attach(f.hooks.context,monotonic_ns(),&d)==CAMERA_OK);
 t=frame(&f,&force);assert(f.hooks.detach(f.hooks.context,d,monotonic_ns())==CAMERA_OK);
 f.idle_error=CAMERA_IO_ERROR;
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_IO_ERROR);
 assert(f.producer.state==CAMERA_PRODUCER_FAILED&&f.idles==1);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_NOT_READY&&f.idles==1);
 assert(!camera_producer_quiescent(&f.producer));
 /* Failure at exact deadline is not acknowledged as logical RUNNING. */
 struct fixture timed;fixture_init(&timed);timed.producer.config.operation_timeout_ns=1;
 assert(timed.hooks.attach(timed.hooks.context,monotonic_ns(),&d)==CAMERA_OK);
 assert(timed.hooks.begin(timed.hooks.context,monotonic_ns(),&force,&t)==CAMERA_PRODUCER_TIMEOUT);
 assert(timed.producer.state==CAMERA_PRODUCER_FAILED&&timed.produced==0);
 struct fixture exhausted;fixture_init(&exhausted);exhausted.bridge.frame_sequence=UINT64_MAX;
 assert(exhausted.hooks.attach(exhausted.hooks.context,monotonic_ns(),&d)==CAMERA_OK);
 assert(exhausted.hooks.begin(exhausted.hooks.context,monotonic_ns(),&force,&t)==CAMERA_NOT_READY);
 assert(!exhausted.bridge.frame_open&&!exhausted.starts&&exhausted.bridge.frame_sequence==UINT64_MAX);
 assert(exhausted.hooks.detach(exhausted.hooks.context,d,monotonic_ns())==CAMERA_OK);
}
static void qc_retirement_paths(void)
{
#ifdef QC_RTSP_RECEIPT_API
 struct qc_rtsp_context failed;qc_legacy_reset_mocks(3);source_context_init(&failed);
 assert(continuous_next(&failed)==0&&qc_binds==1&&qc_arms==1&&qc_begins==1&&qc_nexts==1);
 failed.h264_stream_fd=42;failed.h264_stream_frames=9;qc_live_handles|=4;qc_end_error=EIO;
 assert(idle_h264_checked(&failed)==CAMERA_IO_ERROR&&qc_ends==1&&!qc_acks&&!qc_closes);
 assert(failed.retirement_uncertain&&failed.continuous_frame==41&&failed.continuous_sensor==40&&failed.h264_stream_fd==42);
 unsigned queries=qc_queries;qc_end_error=0;
 assert(idle_h264_checked(&failed)==CAMERA_IO_ERROR&&qc_ends==1&&qc_queries==queries&&!qc_closes&&!qc_acks);
 assert(start_h264_producer(&failed)==CAMERA_NOT_READY&&qc_receipt_source.hold==&qc_receipt_file);
 /* Separate synthetic boot/scenario, NOT resetting this uncertain owner's flag. */
 struct qc_rtsp_context c;qc_legacy_reset_mocks(3);source_context_init(&c);assert(continuous_next(&c)==0);
 c.h264_stream_fd=42;c.h264_stream_frames=9;qc_live_handles|=4;
 assert(idle_h264_checked(&c)==CAMERA_OK&&qc_ends==1&&qc_acks==1&&qc_closes==3);
 assert(c.continuous_frame==-1&&c.continuous_sensor==-1&&c.h264_stream_fd==-1&&c.h264_stream_frames==0&&!qc_receipt_source.hold);
 assert(idle_h264_checked(&c)==CAMERA_OK&&qc_closes==3);
 uint8_t held=1;c.produced_storage=&held;
 assert(idle_h264_checked(&c)==CAMERA_BUSY&&qc_closes==3);c.produced_storage=NULL;
 struct qc_rtsp_context close_failed;qc_legacy_reset_mocks(3);source_context_init(&close_failed);assert(continuous_next(&close_failed)==0);
 close_failed.h264_stream_fd=42;qc_live_handles|=4;qc_close_error=1;
 assert(idle_h264_checked(&close_failed)==CAMERA_IO_ERROR&&close_failed.retirement_uncertain&&close_failed.h264_stream_fd==42&&(qc_live_handles&4));
 unsigned closes=qc_closes;assert(idle_h264_checked(&close_failed)==CAMERA_IO_ERROR&&qc_closes==closes&&qc_acks==1);
 struct qc_rtsp_context uncertain;source_context_init(&uncertain);uncertain.retirement_uncertain=1;
 assert(idle_h264_checked(&uncertain)==CAMERA_IO_ERROR&&start_h264_producer(&uncertain)==CAMERA_NOT_READY);
 qc_legacy_mode=0;stop=0;qc_end_error=qc_close_error=qc_stream_on=0;
 puts("PASS receipt END error sticky/no retry; independent proved END/QUERY/ACK cleanup; held packet/close failure retained");
#else
 struct qc_rtsp_context c;source_context_init(&c);assert(start_h264_producer(&c)==CAMERA_OK);
 c.continuous_sensor=40;c.continuous_frame=41;c.h264_stream_fd=42;c.h264_stream_frames=9;
 qc_end_error=EIO;qc_closes=qc_ends=qc_sensor=0;
 assert(idle_h264_checked(&c)==CAMERA_IO_ERROR);
 assert(qc_ends==1&&!qc_closes&&c.continuous_frame==41&&c.continuous_sensor==40&&c.h264_stream_fd==42);
 assert(start_h264_producer(&c)==CAMERA_NOT_READY);
 qc_end_error=0;assert(idle_h264_checked(&c)==CAMERA_OK);
 assert(qc_ends==2&&qc_closes==3&&c.continuous_frame==-1&&c.h264_stream_fd==-1&&c.h264_stream_frames==0);
 assert(idle_h264_checked(&c)==CAMERA_OK&&qc_closes==3); /* no duplicate close */
 uint8_t held=1;c.produced_storage=&held;
 assert(idle_h264_checked(&c)==CAMERA_BUSY&&qc_closes==3);c.produced_storage=NULL;
 c.h264_stream_fd=42;qc_close_error=1;assert(idle_h264_checked(&c)==CAMERA_IO_ERROR);
 assert(idle_h264_checked(&c)==CAMERA_IO_ERROR);qc_close_error=0;
 struct qc_rtsp_context uncertain;source_context_init(&uncertain);uncertain.retirement_uncertain=1;
 assert(idle_h264_checked(&uncertain)==CAMERA_IO_ERROR&&start_h264_producer(&uncertain)==CAMERA_NOT_READY);
#endif
}
static void cleanup_and_isolation(void)
{
 struct fixture f,g;fixture_init(&f);fixture_init(&g);
 struct camera_producer_demand df,dg;uint32_t force=1;fixture_frame_handle tf,tg;
 assert(f.hooks.attach(f.hooks.context,monotonic_ns(),&df)==CAMERA_OK);
 assert(g.hooks.attach(g.hooks.context,monotonic_ns(),&dg)==CAMERA_OK);
 assert(f.hooks.begin(f.hooks.context,monotonic_ns(),&force,&tf)==CAMERA_OK);
 assert(g.hooks.begin(g.hooks.context,monotonic_ns(),&force,&tg)==CAMERA_OK);
 assert(tf.id==tg.id&&tf.owner!=tg.owner);
 assert(f.hooks.end(f.hooks.context,tg,CAMERA_IO_ERROR,0,monotonic_ns())==CAMERA_STALE_LEASE);
 assert(g.hooks.end(g.hooks.context,tf,CAMERA_OK,1,monotonic_ns())==CAMERA_STALE_LEASE);
 fixture_frame_handle stale=tf;stale.generation++;
 assert(f.hooks.end(f.hooks.context,stale,CAMERA_OK,1,monotonic_ns())==CAMERA_STALE_LEASE);
 assert(f.hooks.end(f.hooks.context,tf,CAMERA_OK,1,monotonic_ns())==CAMERA_OK);
 assert(g.hooks.end(g.hooks.context,tg,CAMERA_OK,1,monotonic_ns())==CAMERA_OK);
 assert(f.hooks.end(f.hooks.context,tf,CAMERA_IO_ERROR,0,monotonic_ns())==CAMERA_STALE_LEASE);
 assert(f.hooks.detach(f.hooks.context,df,monotonic_ns())==CAMERA_OK);
 assert(g.hooks.detach(g.hooks.context,dg,monotonic_ns())==CAMERA_OK);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&g.hooks.retire(g.hooks.context,monotonic_ns())==CAMERA_OK);
 fixture_init(&f);struct rtsp_client clients[4];memset(clients,0,sizeof(clients));
 for(unsigned i=0;i<4;i++)clients[i].fd=-1;
 int writer=peer(&f,&clients[0]);force=0;
 assert(!command_client(&f,&clients[0],0,writer,"SETUP",&force));
 assert(!command_client(&f,&clients[0],0,writer,"PLAY",&force));frame(&f,&force);
 struct linger reset={1,0};assert(!setsockopt(writer,SOL_SOCKET,SO_LINGER,&reset,sizeof(reset)));close(writer);
 send_access_unit(clients,&f.hub,&f.source,2000,0);
 assert(clients[0].fd==-1&&f.detaches==1&&f.released==1&&f.producer.demand_count==0);
 close_client(&clients[0],&f.hub,0);assert(f.detaches==1);
 camera_hub_stop(&f.hub);assert(camera_hub_drained(&f.hub));
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK&&f.idles==1);
 /* Unsupported admission rolls back before returning a demand to the server. */
 fixture_init(&f);f.producer.config.keyframe_supported=0;writer=peer(&f,&clients[0]);
 assert(!command_client(&f,&clients[0],0,writer,"SETUP",&force));
 assert(command_client(&f,&clients[0],0,writer,"PLAY",&force)==-1);
 close_client(&clients[0],&f.hub,0);close(writer);
 assert(!f.producer.demand_count&&!f.detaches&&!f.starts&&!f.produced);
 /* Detach callback error is reported with the exact token, not retried. */
 fixture_init(&f);writer=peer(&f,&clients[0]);
 assert(!command_client(&f,&clients[0],0,writer,"SETUP",&force));
 assert(!command_client(&f,&clients[0],0,writer,"PLAY",&force));df=clients[0].demand;
 f.detach_error=CAMERA_IO_ERROR;
 assert(command_client(&f,&clients[0],0,writer,"PAUSE",&force)==-1);
 close_client(&clients[0],&f.hub,0);close(writer);
 assert(f.detaches==1&&f.failures==1&&f.failure[0].demand.id==df.id&&f.failure[0].stage==CAMERA_RTSP_DETACH_FAILURE);
 assert(clients[0].lifecycle_error==CAMERA_IO_ERROR&&!f.produced);
}
static void serve_failure(unsigned packet_error)
{
 struct fixture *f=mmap(NULL,sizeof(*f),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
 assert(f!=MAP_FAILED);fixture_init(f);
 if(packet_error)f->invalid_packet=1;else f->source_error=CAMERA_IO_ERROR;
 int fd;uint16_t port=listener(&fd);close(fd);
 pid_t child=fork();assert(child>=0);
 if(!child){
  alarm(5);
  struct camera_rtsp_options o={.bind_address="127.0.0.1",.sdp="v=0\r\n",.port=port,
   .send_deadline_ms=2000,.hub_storage=f->storage,.packet_capacity=sizeof(f->storage),
    .hub_slots=1,.trace_every=1000,.stop=&f->stop_requested,.resources=&test_resources};
  f->serve_result=camera_rtsp_serve(&o,&f->source);_exit(0);
 }
 int a=-1;for(unsigned i=0;i<200&&a<0;i++){a=connect_loopback(port);if(a<0){struct timespec t={0,1000000};nanosleep(&t,NULL);}}
 assert(a>=0);request(a,"SETUP",0);request(a,"PLAY",0);
 int b=connect_loopback(port);assert(b>=0);request(b,"SETUP",1);request(b,"PLAY",1);
 int status;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 assert(f->serve_result!=CAMERA_OK&&f->failures==2&&f->detaches==2&&f->idles==1);
 assert(f->produced==f->released&&!f->owned&&f->producer.state==CAMERA_PRODUCER_STOPPED);
 assert(f->failure[0].source.owner==&f->producer&&f->failure[0].source.generation==f->failure[1].source.generation);
 assert(f->failure[0].source.id==f->failure[1].source.id&&f->failure[0].demand.id!=f->failure[1].demand.id);
 assert(f->failure[0].monotonic_ns==f->failure[1].monotonic_ns&&f->failure[0].result==f->failure[1].result);
 assert(f->failure[0].stage==(packet_error?CAMERA_RTSP_PACKET_FAILURE:CAMERA_RTSP_SOURCE_FAILURE));
 assert(f->failure[1].stage==f->failure[0].stage);
 close(a);close(b);assert(!munmap(f,sizeof(*f)));
}
static void serve_normal_stop(unsigned legacy)
{
 struct fixture *f=mmap(NULL,sizeof(*f),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
 assert(f!=MAP_FAILED);fixture_init(f);f->stop_after_frame=1;
 if(legacy)f->source.lifecycle=NULL;
 int fd;uint16_t port=listener(&fd);close(fd);pid_t child=fork();assert(child>=0);
 if(!child){
  alarm(5);struct camera_rtsp_options o={.bind_address="127.0.0.1",.sdp="v=0\r\n",.port=port,
   .send_deadline_ms=2000,.hub_storage=f->storage,.packet_capacity=sizeof(f->storage),
    .hub_slots=1,.stop=&f->stop_requested,.resources=&test_resources};
  f->serve_result=camera_rtsp_serve(&o,&f->source);_exit(0);
 }
 int a=-1;for(unsigned i=0;i<200&&a<0;i++){a=connect_loopback(port);if(a<0){struct timespec t={0,1000000};nanosleep(&t,NULL);}}
 assert(a>=0);request(a,"SETUP",0);request(a,"PLAY",0);
 int status;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 assert(f->serve_result==CAMERA_OK&&f->produced==1&&f->released==1&&f->idles==1&&!f->owned&&!f->failures);
 assert(f->detaches==(legacy?0U:1U)&&f->starts==(legacy?0U:1U));
 close(a);assert(!munmap(f,sizeof(*f)));
}
#ifndef CAMERA_RTSP_TEST_ENTRY
#define CAMERA_RTSP_TEST_ENTRY main
#endif
int CAMERA_RTSP_TEST_ENTRY(int argc,char **argv)
{
 assert(!camera_rtsp_registry_init(&test_resources,"rtsp://127.0.0.1/live","rtsp://127.0.0.1/live/track1"));
 if(argc==2){
  alarm(20);
  if(!strcmp(argv[1],"token-domain"))token_domain_regression();
  else if(!strcmp(argv[1],"legacy-ownership"))legacy_ownership_regression();
  else return 2;
  return 0;
 }
 if(argc!=1)return 2;
 alarm(20);demand_paths();fail_closed_hooks();qc_retirement_paths();cleanup_and_isolation();
 serve_failure(0);serve_failure(1);serve_normal_stop(0);serve_normal_stop(1);
 token_domain_regression();legacy_ownership_regression();
 printf("Host model sizes: producer=%zu bridge=%zu hooks=%zu client=%zu cursor=%zu QC_context=%zu bytes\n",
  sizeof(struct camera_producer),sizeof(struct camera_rtsp_common_producer),sizeof(struct camera_rtsp_lifecycle),
   sizeof(struct rtsp_client),sizeof(struct au_cursor),sizeof(struct qc_rtsp_context));
#ifdef QC_RTSP_LEGACY_SERVICE_API
 printf("Domain/service sizes: frame_handle=%zu primitive_token=%zu legacy_service=%zu bytes\n",
  sizeof(struct camera_rtsp_frame_handle),sizeof(struct camera_producer_token),sizeof(struct qc_legacy_service));
#endif
 puts("RTSP common demand/lifecycle/IDR/retirement/failure fanout loopback+QC syscall mock fixtures PASS");return 0;
}
