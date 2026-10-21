/* SPDX-License-Identifier: GPL-2.0-only
 * Owned loopback control + synthetic legacy syscalls. No target/device access. */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <time.h>
#include <errno.h>
#include <stdint.h>
static int clock_error;
static uint64_t clock_ms=1000;
int __wrap_clock_gettime(clockid_t id,struct timespec *t)
{
 (void)id;if(clock_error){errno=EIO;return -1;} /* deliberately writes NOTHING */
 t->tv_sec=(time_t)(clock_ms/1000);t->tv_nsec=(long)(clock_ms%1000)*1000000;return 0;
}
#define CAMERA_RTSP_TEST_ENTRY retained_positive_entry
#include "rtsp_producer_lifecycle_test.c"
static void reset(void)
{
 clock_error=0;clock_ms=1000;qc_legacy_reset_mocks(2);
#ifdef CAMERA_RTSP_CLOCK_API
 server_clock=(struct camera_rtsp_clock){0};source_clock=(struct camera_rtsp_clock){0};
#endif
 assert(!camera_rtsp_registry_init(&resources,"rtsp://mock/live","rtsp://mock/live/track1"));
 assert(!camera_rtsp_registry_init(&test_resources,"rtsp://127.0.0.1/live","rtsp://127.0.0.1/live/track1"));
}
static void init_error(unsigned listener_entry)
{
 reset();struct qc_legacy_service service;source_context_init(&service.source);service.terminal_result=CAMERA_OK;
 clock_error=1;
 int r=listener_entry?serve_legacy_clients(&service,60):client_session(&service,50);
 assert(r!=CAMERA_OK&&service.terminal_result==r&&qc_client_closes==1&&!qc_opens&&!qc_ends&&!qc_recvs);
 unsigned accepts=qc_accepts,polls=qc_polls;stop=0;
 assert(serve_legacy_clients(&service,60)==r&&qc_accepts==accepts&&qc_polls==polls);
 assert(client_session(&service,50)==r&&qc_client_closes==2&&!qc_opens);
}
static void main_error(void)
{
 reset();clock_error=1;
 char *args[]={"rtspd","--resource-uri","rtsp://mock/live",NULL};
 assert(qc_lab_main(3,args)!=0&&qc_accepts==1&&qc_client_closes==1&&!qc_opens);
}
static unsigned idle_mode,idle_extra;
static uint64_t extra_at;
static int idle_poll(struct pollfd *fds,nfds_t n,int timeout)
{
 (void)n;
 if(!qc_request_sent){fds[0].revents=POLLIN;return 1;}
 clock_ms+=(unsigned)timeout;
 if(clock_ms>65000){stop=1;return 0;} /* bounded old-failure stop, NOT pass */
 if(idle_mode>=3&&clock_ms>=extra_at){idle_extra=1;fds[0].revents=POLLIN;extra_at=clock_ms+1000;return 1;}
 fds[0].revents=0;return 0;
}
static ssize_t idle_recv(void *data,size_t capacity)
{
 static const unsigned char rr[]={'$',1,0,8,0x80,201,0,1,0,0,0,1};
 const char *options="OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n\r\n";
 const char *setup="SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n";
 const char *paused="SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\nPLAY rtsp://mock/live RTSP/1.0\r\nCSeq: 2\r\nSession: QC07F1\r\n\r\nPAUSE rtsp://mock/live RTSP/1.0\r\nCSeq: 3\r\nSession: QC07F1\r\n\r\n";
 const char *unsupported="PLAYBACK rtsp://mock/live RTSP/1.0\r\nCSeq: 9\r\n\r\n";
 const void *wire;size_t bytes;
 if(!qc_request_sent){wire=idle_mode==0||idle_mode>=4?options:idle_mode==2?paused:setup;bytes=strlen(wire);qc_request_sent=1;}
 else if(idle_mode==3){wire=rr;bytes=sizeof(rr);}
 else {wire=idle_mode==4?unsupported:options;bytes=strlen(wire);idle_mode=0;}
 assert(bytes<=capacity);memcpy(data,wire,bytes);return (ssize_t)bytes;
}
static void idle(unsigned mode)
{
 reset();idle_mode=mode;idle_extra=0;extra_at=mode==3?2000:30000;
 qc_poll_hook=idle_poll;qc_recv_hook=idle_recv;
 struct qc_legacy_service service;source_context_init(&service.source);service.terminal_result=CAMERA_OK;
 assert(client_session(&service,50)==CAMERA_OK);
 assert(!stop&&clock_ms==(mode==5?60000U:31000U)&&qc_client_closes==1&&!qc_opens&&!qc_begins&&!qc_ends);
 if(mode>=3)assert(idle_extra);
}
static void setup_play(struct fixture *f,struct rtsp_client *c,int *writer)
{
 fixture_init(f);*writer=peer(f,c);uint32_t force=0;
 assert(!command_client(f,c,0,*writer,"SETUP",&force));
 assert(!command_client(f,c,0,*writer,"PLAY",&force));
 assert(c->demand_attached&&f->producer.demand_count==1&&!f->produced);
}
static void before_detach(unsigned regression)
{
 reset();struct fixture f;struct rtsp_client c;int writer;setup_play(&f,&c,&writer);
 struct camera_producer_demand demand=c.demand;uint64_t last=f.producer.last_ns;
 if(regression)clock_ms--;else clock_error=1;
 assert(camera_rtsp_input_tick(&c.input,milliseconds())<0);
 close_client(&c,&f.hub,0);close_client(&c,&f.hub,0);
 assert(c.fd<0&&c.lifecycle_error!=CAMERA_OK&&c.demand_attached&&c.demand.id==demand.id);
 assert(f.producer.demand_count==1&&f.producer.last_ns==last&&!f.detaches&&!f.idles&&!f.starts&&!f.produced);
 clock_error=0;close(writer);
}
static int fault_detach(void *context,struct camera_producer_demand d,uint64_t now)
{int r=fixture_detach(context,d,now);clock_error=1;return r;}
static void during_detach(void)
{
 reset();struct fixture f;struct rtsp_client c;int writer;setup_play(&f,&c,&writer);
 f.hooks.detach=fault_detach;
 assert(pause_client(&c,&f.hub,0)!=CAMERA_OK);close_client(&c,&f.hub,0);close_client(&c,&f.hub,0);
 assert(c.lifecycle_error!=CAMERA_OK&&f.detaches==1&&!f.producer.demand_count&&!f.idles&&!f.failures);
#ifdef CAMERA_RTSP_CLOCK_API
 assert(retire_checked(&f.hooks)!=CAMERA_OK&&!f.idles);
#endif
 clock_error=0;close(writer);
}
static int fault_start(void *context){int r=fixture_start(context);clock_error=1;return r;}
static int fault_idle(void *context){int r=fixture_idle(context);clock_error=1;return r;}
static void after_start(void)
{
 reset();struct fixture f;fixture_init(&f);struct camera_producer_demand d;
 assert(f.hooks.attach(f.hooks.context,monotonic_ns(),&d)==CAMERA_OK);f.bridge.start=fault_start;
 struct camera_rtsp_frame_handle token={0};uint32_t force=1;
 assert(f.hooks.begin(f.hooks.context,monotonic_ns(),&force,&token)!=CAMERA_OK);
 assert(f.starts==1&&!f.produced&&f.producer.active_present&&f.producer.needs_stop&&!camera_producer_quiescent(&f.producer));
#ifdef CAMERA_RTSP_CLOCK_API
 assert(f.bridge.clock_failed&&f.producer.last_ns!=UINT64_MAX);
 clock_error=0;
 assert(f.hooks.detach(f.hooks.context,d,clock_ms*1000000ULL)!=CAMERA_OK&&f.producer.demand_count==1);
 assert(f.hooks.begin(f.hooks.context,clock_ms*1000000ULL,&force,&token)!=CAMERA_OK&&f.starts==1);
 assert(f.hooks.retire(f.hooks.context,clock_ms*1000000ULL)!=CAMERA_OK&&!f.idles);
#endif
}
static void after_idle(void)
{
 reset();struct fixture f;fixture_init(&f);struct camera_producer_demand d;uint32_t force=1;
 assert(f.hooks.attach(f.hooks.context,monotonic_ns(),&d)==CAMERA_OK);frame(&f,&force);
 assert(f.hooks.detach(f.hooks.context,d,monotonic_ns())==CAMERA_OK);f.bridge.idle=fault_idle;
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())!=CAMERA_OK&&f.idles==1&&!camera_producer_quiescent(&f.producer));
#ifdef CAMERA_RTSP_CLOCK_API
 assert(f.bridge.clock_failed&&f.producer.active_present&&f.producer.needs_stop&&f.producer.last_ns!=UINT64_MAX);
 clock_error=0;assert(f.hooks.retire(f.hooks.context,clock_ms*1000000ULL)!=CAMERA_OK&&f.idles==1);
#endif
}
static int fault_next(void *context,uint32_t force,struct camera_packet *p)
{int r=fixture_next(context,force,p);clock_error=1;return r;}
static void fault_release(void *context,const struct camera_packet *p)
{fixture_release(context,p);clock_error=1;}
static int fault_attach(void *context,uint64_t now,struct camera_producer_demand *d)
{int r=camera_rtsp_common_attach(context,now,d);clock_error=1;return r;}
static void serve_clock(unsigned mode)
{
 reset();qc_legacy_mode=0; /* shared serve uses actual owned loopback, not mocks */
 struct fixture *f=mmap(NULL,sizeof(*f),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
 assert(f!=MAP_FAILED);fixture_init(f);
 if(mode==0)f->hooks.attach=fault_attach;
 if(mode==1)f->source.next=fault_next;
 if(mode==2)f->source.release=fault_release;
 if(mode==3){f->stop_after_frame=1;f->bridge.idle=fault_idle;}
 int fd;uint16_t port=listener(&fd);close(fd);pid_t child=fork();assert(child>=0);
 if(!child){alarm(5);struct camera_rtsp_options o={.bind_address="127.0.0.1",.sdp="v=0\r\n",.port=port,.send_deadline_ms=500,.hub_storage=f->storage,.packet_capacity=sizeof(f->storage),.hub_slots=1,.stop=&f->stop_requested,.resources=&test_resources};f->serve_result=camera_rtsp_serve(&o,&f->source);_exit(0);}
 int writer=-1;for(unsigned i=0;i<200&&writer<0;i++){writer=connect_loopback(port);if(writer<0){struct timespec t={0,1000000};nanosleep(&t,NULL);}}
 assert(writer>=0);request(writer,"SETUP",0);request(writer,"PLAY",0);
 int status;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 assert(f->serve_result!=CAMERA_OK&&f->produced==f->released&&!f->owned&&f->producer.last_ns!=UINT64_MAX);
 if(mode==0)assert(!f->starts&&!f->produced&&!f->detaches&&f->producer.demand_count==1);
 if(mode==1||mode==2)assert(f->produced==1&&!f->detaches&&!f->idles&&f->bridge.frame_open&&f->producer.demand_count==1);
  if(mode==3){
   assert(f->idles==1&&f->detaches==1&&!camera_producer_quiescent(&f->producer));
#ifdef CAMERA_RTSP_CLOCK_API
   assert(f->bridge.clock_failed);
#endif
  }
 close(writer);assert(!munmap(f,sizeof(*f)));
}
int main(int argc,char **argv)
{
 if(argc==1){
  static const char *const cases[]={"init","listener-init","main-init","idle-options","idle-setup","idle-pause","idle-rtcp","idle-unsupported","idle-refresh","before-detach","regression","during-detach","after-start","after-idle","serve-attach","serve-next","serve-release","serve-idle","clock-helper"};
  for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){char *args[]={argv[0],(char *)cases[i],NULL};assert(main(2,args)==0);}
  return 0;
 }
 alarm(20);assert(argc==2);
 if(!strcmp(argv[1],"init"))init_error(0);
 else if(!strcmp(argv[1],"listener-init"))init_error(1);
 else if(!strcmp(argv[1],"main-init"))main_error();
 else if(!strcmp(argv[1],"idle-options"))idle(0);
 else if(!strcmp(argv[1],"idle-setup"))idle(1);
 else if(!strcmp(argv[1],"idle-pause"))idle(2);
 else if(!strcmp(argv[1],"idle-rtcp"))idle(3);
 else if(!strcmp(argv[1],"idle-unsupported"))idle(4);
 else if(!strcmp(argv[1],"idle-refresh"))idle(5);
 else if(!strcmp(argv[1],"before-detach"))before_detach(0);
 else if(!strcmp(argv[1],"regression"))before_detach(1);
 else if(!strcmp(argv[1],"during-detach"))during_detach();
 else if(!strcmp(argv[1],"after-start"))after_start();
 else if(!strcmp(argv[1],"after-idle"))after_idle();
 else if(!strcmp(argv[1],"serve-attach"))serve_clock(0);
 else if(!strcmp(argv[1],"serve-next"))serve_clock(1);
 else if(!strcmp(argv[1],"serve-release"))serve_clock(2);
 else if(!strcmp(argv[1],"serve-idle"))serve_clock(3);
 else if(!strcmp(argv[1],"clock-helper")){reset();clock_error=1;assert(monotonic_ns()==UINT64_MAX);}
 else return 2;
 printf("RTSP fix regression PASS %s (host only)\n",argv[1]);return 0;
}
