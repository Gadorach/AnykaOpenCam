/* SPDX-License-Identifier: GPL-2.0-only
 * Actual copied adapter bodies, full public synthetic raw frame, real receipt
 * helper behind syscall mocks. No real device/socket/firmware execution.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <jpeglib.h>
#include "receipt-model/capture-retirement.h"
#include <camera/rtsp_server.h>
#include <camera/rtsp_clock.h>
static int mock_open(const char *,int,...),mock_ioctl(int,unsigned long,...),mock_close(int);
static ssize_t mock_write(int,const void *,size_t),mock_read(int,void *,size_t);
static ssize_t mock_send(int,const void *,size_t,int),mock_recv(int,void *,size_t,int);
static int mock_poll(struct pollfd *,nfds_t,int),mock_setsockopt(int,int,int,const void *,socklen_t);
static void *mock_malloc(size_t);
static void mock_free(void *);
#define open mock_open
#define ioctl mock_ioctl
#define close mock_close
#define write mock_write
#define read mock_read
#define send mock_send
#define recv mock_recv
#define poll mock_poll
#define setsockopt mock_setsockopt
#define malloc mock_malloc
#define free mock_free
#define main copied_lab_main
#ifndef QC_ADAPTER_SOURCE
#define QC_ADAPTER_SOURCE "../../bringup/qc07f/userland/rtspd.c"
#endif
#include QC_ADAPTER_SOURCE
#undef main
#undef open
#undef ioctl
#undef close
#undef write
#undef read
#undef send
#undef recv
#undef poll
#undef setsockopt
#undef malloc
#undef free

static struct qc_retirement_registry registry;
static struct qc_retirement_source kernel_source;
static struct qc_retirement_file files[16],*current,*physical_owner,*observer_file;
static unsigned file_count,stream_on,driver_sequence;
static uint64_t now_ms=1000,begin_ms;
static int live[128],encoder_seen[128];
static uint64_t encoder_generation[128];
static unsigned opens,begins,ends,queries,acks,binds,arms,resets,nexts,magic,pets,encoder_calls,client_closes;
static unsigned order,ack_order,bind_order,arm_order,begin_order,sensor_off_order;
static unsigned nal_keyframes[300],nal_count,p_run,max_p_run;
static unsigned bind_fault_once,query_flip,malformed_bind,malformed_arm;
static unsigned crc_tail,guard_bad,status_bad,sequence_bad,timeout_error,expire_every_next;
static unsigned stream_write_error,network_phase,network_end;
static unsigned fault_after_end,flip_after_end,next_partial,watchdog_error,sensor_geometry_error;
static unsigned bad_encoder_output,clock_fault,clock_after_next,clock_after_encoder;
static unsigned reply_schema;
static int reset_error,begin_error,next_error,end_error,close_error_fd,encoder_error;
static unsigned long fault_command;
static int fault_direction,fault_errno;
static uint8_t pattern[FRAME_BYTES];
static uint32_t reference_crc;
static void *output_allocation;
static unsigned frame_fd_reuses;

int __wrap_clock_gettime(clockid_t id,struct timespec *t)
{assert(id==CLOCK_MONOTONIC);if(clock_fault){errno=EIO;return -1;}t->tv_sec=(time_t)(now_ms/1000);t->tv_nsec=(long)(now_ms%1000)*1000000;return 0;}
static uint32_t independent_crc(const uint8_t *p,size_t bytes)
{
 uint32_t table[256],crc=~0U;
 for(unsigned i=0;i<256;i++){uint32_t v=i<<24;for(unsigned j=0;j<8;j++)v=(v<<1)^((v&0x80000000U)?0x04c11db7U:0);table[i]=v;}
 for(size_t i=0;i<bytes;i++)crc=(crc<<8)^table[((crc>>24)^p[i])&255];
 return ~crc;
}
struct ucopy {struct qc_capture_receipt *user;unsigned long command;};
static int from_user(void *context,struct qc_capture_receipt *q)
{
 struct ucopy *u=context;
 if(u->command==fault_command&&fault_direction==1&&(!fault_after_end||ends))return -1;
 *q=*u->user;
 if(u->command==fault_command&&fault_direction==3)q->generation++;
 assert(q->version==1&&!q->reserved&&!q->reserved2&&!q->state&&!q->reason&&!q->stop_result&&!q->stopped_ns);
 return 0;
}
static int to_user(void *context,const struct qc_capture_receipt *q)
{
 struct ucopy *u=context;*u->user=*q;
 if(u->command==fault_command&&fault_direction==2&&(!fault_after_end||ends)){
  /* A fully plausible-looking partial reply must still never become proof. */
  u->user->state=QC_RECEIPT_RETIRED;u->user->reason=QC_STOP_EXPIRY;
  u->user->stop_result=0;u->user->stopped_ns=now_ms*1000000ULL;return -1;
 }
 if(u->command==QC_ISP_RECEIPT_BIND&&malformed_bind){
  if(malformed_bind==1)u->user->version=2;
  else if(malformed_bind==2)u->user->generation=1;
  else if(malformed_bind==3)u->user->file_instance=0;
  else u->user->reserved2=1;
 }
 if(u->command==QC_ISP_RECEIPT_ARM&&malformed_arm){
  if(malformed_arm==1)u->user->generation=0;
  else if(malformed_arm==2)u->user->file_instance++;
  else if(malformed_arm==3)u->user->source_instance++;
  else if(malformed_arm==4)u->user->state=QC_RECEIPT_ACTIVE;
  else if(malformed_arm==5)u->user->reserved=1;
  else u->user->generation++;
 }
 if(u->command==QC_ISP_RECEIPT_QUERY&&query_flip&&(!flip_after_end||ends)){
  if(query_flip==1)u->user->source_instance++;
  else if(query_flip==2)u->user->file_instance++;
  else if(query_flip==3)u->user->generation++;
  else if(query_flip==4)u->user->state=QC_RECEIPT_UNCERTAIN;
  else if(query_flip==5)u->user->stop_result=-EIO;
  else if(query_flip==6)u->user->reserved=1;
  else if(query_flip==7)u->user->version=2;
  else if(query_flip==8)u->user->reason=QC_STOP_NONE;
  else u->user->stopped_ns=0;
 }
 return 0;
}
static int kernel_ioctl(struct qc_retirement_file *f,unsigned long cmd,struct qc_capture_receipt *q)
{
 struct ucopy copy={q,cmd};struct qc_retirement_uaccess access={from_user,to_user,&copy};
 return qc_retirement_ioctl(f,cmd,&access);
}
static int kernel_begin(void *context)
{struct qc_retirement_file *f=context;physical_owner=f;driver_sequence=0;begin_ms=now_ms;begins++;begin_order=++order;assert(arm_order<begin_order);return begin_error;}
static int kernel_reset(void *context)
{(void)context;resets++;if(!reset_error)physical_owner=NULL;return reset_error;}
static __u64 kernel_clock(void *context)
{(void)context;return now_ms*1000000ULL;}
static struct qc_retirement_ops ops(struct qc_retirement_file *f)
{struct qc_retirement_ops result={kernel_begin,kernel_reset,kernel_clock,f};return result;}
static int result(int r)
{if(r){errno=-r;return -1;}return 0;}
static int mock_open(const char *path,int flags,...)
{
 (void)flags;opens++;
 if(!strcmp(path,"/dev/i2c-0")){assert(!live[40]);live[40]=1;return 40;}
 if(!strcmp(path,"/dev/qc07f-frame")){
  assert(!live[41]&&file_count<16);current=&files[file_count++];
  assert(!qc_retirement_open(&kernel_source,current));live[41]=1;if(file_count>1)frame_fd_reuses++;return 41;
 }
 if(!strcmp(path,"/dev/watchdog0")){assert(!live[43]);live[43]=1;return 43;}
 assert(!strcmp(path,"/dev/qc07f-encoder"));
 for(int i=100;i<128;i++)if(!live[i]){live[i]=1;encoder_seen[i]=0;encoder_generation[i]=0;return i;}
 assert(0);return -1;
}
static int mock_ioctl(int fd,unsigned long cmd,...)
{
 va_list ap;va_start(ap,cmd);
 if(cmd==WDIOC_SETTIMEOUT){assert(fd==43&&live[fd]);int *value=va_arg(ap,int *);assert(*value==5);va_end(ap);if(watchdog_error){errno=EIO;return -1;}return 0;}
 if(cmd==I2C_SLAVE){assert(fd==40&&live[fd]&&va_arg(ap,int)==0x30);va_end(ap);return 0;}
 if(cmd==I2C_RDWR){
  assert(fd==40&&live[fd]);struct i2c_rdwr_ioctl_data *q=va_arg(ap,struct i2c_rdwr_ioctl_data *);
  assert(q->nmsgs==2);unsigned reg=((unsigned)q->msgs[0].buf[0]<<8)|q->msgs[0].buf[1];unsigned value=0;
  if(reg==0x0100)value=stream_on;
  else if(reg==0x3107)value=0xda;else if(reg==0x3108)value=0x4d;
  else if(reg==0x3208)value=5;else if(reg==0x320a)value=2;else if(reg==0x320b)value=0xd0;
  if(sensor_geometry_error&&reg==0x3208)value=4;
  q->msgs[1].buf[0]=(uint8_t)value;va_end(ap);return 2;
 }
 if(cmd==QC_ISP_RECEIPT_BIND||cmd==QC_ISP_RECEIPT_ARM||cmd==QC_ISP_RECEIPT_QUERY||cmd==QC_ISP_RECEIPT_ACK){
  assert(fd==41&&live[fd]);struct qc_capture_receipt *q=va_arg(ap,struct qc_capture_receipt *);va_end(ap);
  if(cmd==QC_ISP_RECEIPT_BIND){binds++;bind_order=++order;if(binds>1)assert(ack_order<bind_order&&ack_order);}
  if(cmd==QC_ISP_RECEIPT_ARM){arms++;arm_order=++order;assert(bind_order<arm_order);}
  if(cmd==QC_ISP_RECEIPT_QUERY)queries++;
  if(cmd==QC_ISP_RECEIPT_ACK)acks++;
  if(cmd==fault_command&&fault_errno){errno=fault_errno;return -1;}
  if(cmd==QC_ISP_RECEIPT_BIND&&bind_fault_once){bind_fault_once=0;errno=EFAULT;return -1;}
  int r=kernel_ioctl(current,cmd,q);
  if(!r&&cmd==QC_ISP_RECEIPT_ACK){ack_order=++order;assert(sensor_off_order<ack_order);}
  return result(r);
 }
 if(cmd==QC_ISP_BEGIN||cmd==QC_ISP_BEGIN_IRQ){
  assert(fd==41&&live[fd]);va_end(ap);
  if(current->state==QC_RECEIPT_NONE){
   /* Exact legacy kernel implicit-ARM path, for old-source counterexamples. */
   struct qc_capture_receipt q={.version=1,.source_instance=kernel_source.id,.file_instance=current->id};
   arm_order=++order;assert(!kernel_ioctl(current,QC_ISP_RECEIPT_ARM,&q));
  }
  struct qc_retirement_ops o=ops(current);return result(qc_retirement_begin(current,&o));
 }
 if(cmd==QC_ISP_NEXT){
  assert(fd==41&&live[fd]);struct qc_isp_next *q=va_arg(ap,struct qc_isp_next *);va_end(ap);nexts++;
  assert(q->bytes==FRAME_BYTES&&q->user_ptr==(uint32_t)(uintptr_t)raw);
  if(physical_owner!=current){errno=EPERM;return -1;}
  struct qc_retirement_ops o=ops(current);
  if(next_error){errno=next_error;return -1;}
  if(next_partial){memcpy(raw,pattern,sizeof(raw));q->sequence=driver_sequence+1;q->status=1;q->crc32_bzip2=reference_crc;errno=EFAULT;return -1;}
  if(timeout_error){assert(!qc_retirement_stop(current,QC_STOP_ERROR,&o));errno=ETIMEDOUT;return -1;}
  if(expire_every_next||now_ms-begin_ms>=10000||driver_sequence>=128){
   (void)qc_retirement_stop(current,QC_STOP_EXPIRY,&o);errno=ETIMEDOUT;return -1;
  }
  memcpy(raw,pattern,sizeof(raw));if(crc_tail)raw[sizeof(raw)-1]^=1;
  q->sequence=sequence_bad?driver_sequence:++driver_sequence;q->status=status_bad?0:1;q->guard_errors=guard_bad?1:0;q->crc32_bzip2=reference_crc;
  if(reply_schema==1)q->bytes=FRAME_BYTES-1;
  else if(reply_schema==2)q->bytes=MAIN_PIXELS*3/2;
  else if(reply_schema==3)q->user_ptr^=64;
  if(clock_after_next)clock_fault=1;
  return 0;
 }
 if(cmd==QC_ISP_END){
  assert((fd==41||fd==45)&&live[fd]);va_end(ap);ends++;
  struct qc_retirement_file *owner=fd==41?current:observer_file;
  if(physical_owner!=owner||end_error){errno=end_error?end_error:EPERM;return -1;}
  struct qc_retirement_ops o=ops(owner);return result(qc_retirement_stop(owner,QC_STOP_END,&o));
 }
 assert(cmd==QC_HWENC_H264_STREAM||cmd==QC_HWENC_H264_IDR);
 assert(fd>=100&&live[fd]&&output_allocation);unsigned keyframe=1;struct qc_hwenc_h264_job *job;
 if(cmd==QC_HWENC_H264_STREAM){struct qc_hwenc_h264_stream_job *q=va_arg(ap,struct qc_hwenc_h264_stream_job *);keyframe=q->keyframe;job=&q->frame;}
 else job=va_arg(ap,struct qc_hwenc_h264_job *);
 va_end(ap);assert(job->width==1280&&job->height==720&&job->input_bytes==1382400&&job->input_ptr==(uint32_t)(uintptr_t)raw&&job->output_ptr==(uint32_t)(uintptr_t)output_allocation&&job->output_capacity==0x100000);
 if(encoder_error){errno=EIO;return -1;}
 assert(current&&current->state==QC_RECEIPT_ACTIVE&&physical_owner==current);
 if(!encoder_seen[fd]){assert(keyframe);encoder_generation[fd]=current->generation;encoder_seen[fd]=1;}
 assert(encoder_generation[fd]==current->generation); /* never stale P reference */
 uint8_t *out=output_allocation;memset(out,0x55,32);out[0]=out[1]=out[2]=0;out[3]=1;out[4]=keyframe?0x65:0x41;job->output_bytes=32;
 if(bad_encoder_output==1)out[4]=keyframe?0x41:0x65;
 else if(bad_encoder_output==2)job->output_bytes=2;
 else if(bad_encoder_output==3)job->output_bytes=0x100001;
 if(clock_after_encoder)clock_fault=1;
 assert(nal_count<300);nal_keyframes[nal_count++]=keyframe;encoder_calls++;
 if(keyframe)p_run=0;else p_run++;if(p_run>max_p_run)max_p_run=p_run;assert(p_run<30);
 return 0;
}
static ssize_t mock_write(int fd,const void *data,size_t bytes)
{
 assert(live[fd]);
 if(fd==43){assert(bytes==1);if(*(const char *)data=='V')magic++;else{assert(*(const char *)data=='.');pets++;}return 1;}
 assert(fd==40&&bytes==3&&((const uint8_t *)data)[0]==1&&!((const uint8_t *)data)[1]);
 unsigned on=((const uint8_t *)data)[2];assert(on<=1);
 if(stream_write_error==on+1){errno=EIO;return -1;}
 stream_on=on;if(!on)sensor_off_order=++order;return 3;
}
static int mock_close(int fd)
{
 if(fd==50){client_closes++;return 0;}
 assert(fd>=0&&fd<128&&live[fd]);if(fd==close_error_fd){errno=EIO;return -1;}
 if(fd==41){struct qc_retirement_ops o=ops(current);assert(!qc_retirement_close(current,&o));}
 if(fd==40)assert(!stream_on);
 live[fd]=0;return 0;
}
static ssize_t mock_read(int fd,void *p,size_t n)
{(void)fd;(void)p;(void)n;assert(0);return -1;}
static void *mock_malloc(size_t bytes)
{void *p=malloc(bytes);assert(p);if(bytes==0x100000){assert(!output_allocation);output_allocation=p;}return p;}
static void mock_free(void *p)
{if(p==output_allocation)output_allocation=NULL;free(p);}
static ssize_t mock_send(int fd,const void *p,size_t n,int flags)
{(void)p;assert(fd==50&&n&&flags==MSG_NOSIGNAL);return (ssize_t)n;}
static ssize_t mock_recv(int fd,void *p,size_t capacity,int flags)
{
 assert(fd==50&&flags==MSG_DONTWAIT);
 const char *wire=network_phase++?
 "TEARDOWN rtsp://mock/live RTSP/1.0\r\nCSeq: 3\r\nSession: QC07F1\r\n\r\n":
 "SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\nPLAY rtsp://mock/live RTSP/1.0\r\nCSeq: 2\r\nSession: QC07F1\r\n\r\n";
 size_t n=strlen(wire);assert(n<=capacity);memcpy(p,wire,n);return (ssize_t)n;
}
static int mock_poll(struct pollfd *fds,nfds_t n,int timeout)
{assert(n==1&&fds[0].fd==50&&timeout>=0&&!network_end);fds[0].revents=POLLIN;return 1;}
static int mock_setsockopt(int fd,int level,int opt,const void *p,socklen_t bytes)
{(void)opt;(void)p;(void)bytes;assert(fd==50&&level==SOL_SOCKET);return 0;}
static void reset(struct qc_rtsp_context *c)
{
 memset(&registry,0,sizeof(registry));memset(&kernel_source,0,sizeof(kernel_source));
 memset(files,0,sizeof(files));memset(live,0,sizeof(live));memset(encoder_seen,0,sizeof(encoder_seen));
 memset(encoder_generation,0,sizeof(encoder_generation));memset(nal_keyframes,0,sizeof(nal_keyframes));
 current=physical_owner=observer_file=NULL;file_count=stream_on=driver_sequence=0;now_ms=1000;begin_ms=0;
 opens=begins=ends=queries=acks=binds=arms=resets=nexts=magic=pets=encoder_calls=client_closes=0;
 order=ack_order=bind_order=arm_order=begin_order=sensor_off_order=0;
 nal_count=p_run=max_p_run=frame_fd_reuses=0;
 bind_fault_once=query_flip=malformed_bind=malformed_arm=crc_tail=guard_bad=status_bad=sequence_bad=timeout_error=expire_every_next=0;
 stream_write_error=network_phase=network_end=fault_after_end=flip_after_end=next_partial=watchdog_error=sensor_geometry_error=0;
 bad_encoder_output=clock_fault=clock_after_next=clock_after_encoder=0;
 reply_schema=0;
 reset_error=begin_error=next_error=end_error=encoder_error=0;close_error_fd=-1;
 fault_command=0;fault_direction=fault_errno=0;output_allocation=NULL;
 assert(!qc_retirement_source_init(&registry,&kernel_source));
 for(size_t i=0;i<sizeof(pattern);i++)pattern[i]=(uint8_t)(i*31U+(i>>9));
 reference_crc=independent_crc(pattern,sizeof(pattern));assert(qc_crc_bzip2(pattern,sizeof(pattern))==reference_crc);
 source_context_init(c);source_clock=(struct camera_rtsp_clock){0};stop=0;frame_count=0;
 hardware_h264=h264_p_frames=continuous_capture=capture_irq=1;fixture=NULL;trace_every=1000;
 assert(!camera_rtsp_registry_init(&resources,"rtsp://mock/live","rtsp://mock/live/track1"));
}
static void frame(struct qc_rtsp_context *c,unsigned expected_keyframe)
{
 struct camera_packet packet={0};assert(produce_h264_frame(c,0,&packet)==CAMERA_OK);
 assert(packet.keyframe==expected_keyframe&&packet.bytes==28&&(packet.data[0]&31)==(expected_keyframe?5:1));
 assert(independent_crc(packet.data,packet.bytes)==qc_crc_bzip2(packet.data,packet.bytes));
 assert(packet.monotonic_ns==now_ms*1000000ULL);release_h264_frame(c,&packet);
 assert(frame_count==encoder_calls&&frame_count==nal_count&&nexts>=frame_count);
}
static void healthy_close(struct qc_rtsp_context *c)
{
 assert(idle_h264_checked(c)==CAMERA_OK&&start_h264_producer(c)==CAMERA_OK);
 assert(!physical_owner&&!kernel_source.hold&&!stream_on&&!live[40]&&!live[41]&&!live[43]);
 for(int i=100;i<128;i++)assert(!live[i]);assert(!output_allocation);
}
static void terminal(struct qc_rtsp_context *c,unsigned before_magic,unsigned before_begins)
{
 assert(c->retirement_uncertain&&start_h264_producer(c)!=CAMERA_OK);
 assert(c->continuous_frame==41&&live[41]&&c->continuous_sensor==40&&live[40]);
 unsigned old_opens=opens,old_ends=ends,old_acks=acks;
 assert(continuous_next(c)!=0&&idle_h264_checked(c)!=CAMERA_OK);
 assert(opens==old_opens&&ends==old_ends&&acks==old_acks&&begins==before_begins&&magic==before_magic);
#ifdef QC_RTSP_RECEIPT_API
 if(live[43])assert(c->watchdog_hold==43);
#endif
}
static void renewal(unsigned frame_limit)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);unsigned first_magic=magic;
 uint64_t generation=current->generation,file=current->id;
 if(frame_limit){for(unsigned i=1;i<128;i++)frame(&c,i%30==0);assert(driver_sequence==128);}
 else now_ms+=10001;
 frame(&c,1); /* OLD source fails here on redundant END/EPERM after selfstop. */
 assert(current->generation>generation&&current->id>file&&frame_fd_reuses==1&&acks==1&&ends==0&&pets==1);
#ifdef QC_RTSP_RECEIPT_API
 assert(c.receipt_renewals==1&&c.ownership_idrs==2&&c.last_capture_generation==current->generation);
#endif
 assert(magic>first_magic);frame(&c,0);healthy_close(&c);assert(ends==1&&acks==2&&max_p_run<=29);
}
static void admission_fault(unsigned long command,int direction,int error,unsigned malformed)
{
 struct qc_rtsp_context c;reset(&c);fault_command=command;fault_direction=direction;fault_errno=error;
 if(command==QC_ISP_RECEIPT_BIND)malformed_bind=malformed;else malformed_arm=malformed;
 struct camera_packet packet={0};assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&begins==0&&!encoder_calls&&!packet.data);
 terminal(&c,0,0);
 if(command==QC_ISP_RECEIPT_ARM&&direction==2)assert(!kernel_source.hold&&current->state==QC_RECEIPT_NONE&&kernel_source.next_generation==1);
}
static void next_fault(unsigned mode)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);unsigned before_magic=magic;
 if(mode==0)next_error=EPERM;else if(mode==1)next_error=EIO;else if(mode==2)next_error=EFAULT;
 else if(mode==3)crc_tail=1;else if(mode==4)guard_bad=1;else if(mode==5)status_bad=1;
 else if(mode==6)sequence_bad=1;else if(mode==7)timeout_error=1;
 else if(mode==8){now_ms+=10001;reset_error=-EIO;}
 else next_partial=1;
 struct camera_packet packet={0};assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&!packet.data&&encoder_calls==1);
 terminal(&c,before_magic,1);assert(acks==0&&ends==0&&kernel_source.hold==current);
}
static void close_fault(unsigned mode)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);unsigned before_magic=magic,old_queries=queries;
 if(mode<=2){fault_command=QC_ISP_RECEIPT_QUERY;fault_direction=(int)mode+1;}
 else if(mode<=9)query_flip=mode-2;
 else if(mode==10)end_error=EPERM;
 else if(mode==11)reset_error=-EIO;
 else if(mode==12){fault_command=QC_ISP_RECEIPT_ACK;fault_direction=1;}
 else if(mode==13){fault_command=QC_ISP_RECEIPT_ACK;fault_direction=3;}
 else if(mode==14){fault_command=QC_ISP_RECEIPT_QUERY;fault_errno=EPERM;}
 else if(mode==15){fault_command=QC_ISP_RECEIPT_QUERY;fault_direction=2;fault_after_end=1;}
 else {query_flip=3;flip_after_end=1;}
 assert(idle_h264_checked(&c)!=CAMERA_OK&&queries>old_queries);
 terminal(&c,before_magic,1);assert(kernel_source.hold==current);
 if(mode==12||mode==13)assert(current->state==QC_RECEIPT_RETIRED&&!stream_on);
 if(mode==15||mode==16)assert(current->state==QC_RECEIPT_RETIRED&&!acks&&ends==1&&!stream_on);
}
static void retired_skip(unsigned wrong)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);struct qc_retirement_ops o=ops(current);
 assert(!qc_retirement_stop(current,QC_STOP_EXPIRY,&o));assert(!physical_owner&&kernel_source.hold==current);
 if(wrong==1)query_flip=8;else if(wrong==2)query_flip=9;
 if(wrong){assert(idle_h264_checked(&c)!=CAMERA_OK);terminal(&c,magic,1);assert(!acks&&!ends);}
 else {healthy_close(&c);assert(!ends&&acks==1&&resets==1);}
}
static void fd_reuse_stale(unsigned key_part)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);
#ifdef QC_RTSP_RECEIPT_API
 struct qc_capture_receipt old=c.receipt_key;
#endif
 healthy_close(&c);frame(&c,1);assert(frame_fd_reuses==1);
#ifdef QC_RTSP_RECEIPT_API
 if(key_part==0)c.receipt_key.source_instance=old.source_instance+1;
 else if(key_part==1)c.receipt_key.file_instance=old.file_instance;
 else c.receipt_key.generation=old.generation;
#else
 (void)key_part;
#endif
 unsigned old_ends=ends,old_acks=acks;assert(idle_h264_checked(&c)!=CAMERA_OK);
 terminal(&c,magic,2);assert(ends==old_ends&&acks==old_acks&&kernel_source.hold==current&&stream_on);
}
static void held_and_observer(void)
{
 struct qc_rtsp_context c;reset(&c);struct camera_packet p={0};assert(!produce_h264_frame(&c,0,&p));
 unsigned old_queries=queries;assert(idle_h264_checked(&c)==CAMERA_BUSY&&queries==old_queries&&!acks&&!ends);
 struct qc_retirement_file observer;assert(!qc_retirement_open(&kernel_source,&observer));
 struct qc_capture_receipt q={.version=1,.source_instance=kernel_source.id,.file_instance=current->id,.generation=current->generation};
 unsigned old_resets=resets;assert(kernel_ioctl(&observer,QC_ISP_RECEIPT_QUERY,&q)==-ESTALE);
 q=(struct qc_capture_receipt){.version=1,.source_instance=kernel_source.id,.file_instance=observer.id};
 assert(kernel_ioctl(&observer,QC_ISP_RECEIPT_QUERY,&q)==-ESTALE&&resets==old_resets&&kernel_source.hold==current);
 /* Kernel END permission for a nonowner cannot be used as receipt evidence. */
 observer_file=&observer;live[45]=1;
 assert(mock_ioctl(45,QC_ISP_END)==-1&&errno==EPERM&&resets==old_resets);
 release_h264_frame(&c,&p);healthy_close(&c);
 struct qc_retirement_ops o=ops(&observer);assert(!qc_retirement_close(&observer,&o));
 live[45]=0;observer_file=NULL;
}
static void close_fd_error(unsigned which)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);close_error_fd=which==0?40:which==1?41:c.h264_stream_fd;
 unsigned old_magic=magic,old_binds=binds;
 assert(idle_h264_checked(&c)!=CAMERA_OK&&c.retirement_uncertain&&start_h264_producer(&c)!=CAMERA_OK);
 assert(idle_h264_checked(&c)!=CAMERA_OK&&binds==old_binds&&magic==old_magic&&acks==1);
 assert(live[close_error_fd]);
}
static void sessions(void)
{
 struct qc_legacy_service service;reset(&service.source);service.terminal_result=CAMERA_OK;
 for(unsigned i=0;i<2;i++){network_phase=0;assert(client_session(&service,50)==CAMERA_OK&&service.terminal_result==CAMERA_OK);assert(file_count==i+1&&acks==i+1&&ends==i+1&&!physical_owner&&!kernel_source.hold&&!stream_on);}
 assert(client_closes==2&&encoder_calls==2&&nal_keyframes[0]&&nal_keyframes[1]);
 network_phase=0;fault_command=QC_ISP_RECEIPT_ARM;fault_direction=2;
 assert(client_session(&service,50)!=CAMERA_OK&&service.terminal_result!=CAMERA_OK);
 unsigned old_begins=begins,old_opens=opens;network_phase=0;
 assert(client_session(&service,50)==service.terminal_result&&begins==old_begins&&opens==old_opens);
}
static void common_namespaces(void)
{
 struct qc_rtsp_context c;reset(&c);struct camera_producer p;struct camera_producer_config config={.operation_timeout_ns=10000000000ULL,.keyframe_supported=1};
 assert(!camera_producer_init(&p,&config));
 struct camera_rtsp_common_producer bridge={.producer=&p,.context=&c,.start=start_h264_producer,.idle=idle_h264_checked,.clock_ns=producer_clock_ns};
 struct camera_rtsp_lifecycle hooks=camera_rtsp_common_hooks(&bridge);
 for(unsigned session=0;session<2;session++){
  struct camera_producer_demand demand;assert(!hooks.attach(hooks.context,producer_clock_ns(&c),&demand));
  uint32_t force=1;struct camera_rtsp_frame_handle token;assert(!hooks.begin(hooks.context,producer_clock_ns(&c),&force,&token));
  struct camera_packet packet={0};assert(!produce_h264_frame(&c,force,&packet));release_h264_frame(&c,&packet);
  struct camera_rtsp_frame_handle bad=token;bad.namespace_context=&c;
  assert(hooks.end(hooks.context,bad,CAMERA_OK,packet.keyframe,producer_clock_ns(&c))==CAMERA_STALE_LEASE&&bridge.frame_open);
  assert(!hooks.end(hooks.context,token,CAMERA_OK,packet.keyframe,producer_clock_ns(&c)));
  assert(!hooks.detach(hooks.context,demand,producer_clock_ns(&c)));assert(!hooks.retire(hooks.context,producer_clock_ns(&c)));
 }
 assert(acks==2&&ends==2&&begins==2&&bridge.frame_sequence==2&&p.state==CAMERA_PRODUCER_STOPPED&&!p.needs_stop);
 assert(!camera_producer_shutdown(&p,producer_clock_ns(&c))&&camera_producer_quiescent(&p));
}
static void prebegin_query(unsigned direction)
{
 struct qc_rtsp_context c;reset(&c);fault_command=QC_ISP_RECEIPT_QUERY;fault_direction=(int)direction;
 struct camera_packet packet={0};assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&!packet.data&&!begins&&!resets&&!encoder_calls);
 assert(current->state==QC_RECEIPT_ARMED&&kernel_source.hold==current);terminal(&c,0,0);
}
static void extra_failure(unsigned mode)
{
 struct qc_rtsp_context c;reset(&c);
 if(mode==0)begin_error=-EIO;
 else if(mode==1)stream_write_error=2; /* sensor-on fail after exact BEGIN */
 else if(mode==2)encoder_error=1;
 else if(mode==3)watchdog_error=1;
 else sensor_geometry_error=1;
 struct camera_packet packet={0};assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&!packet.data&&c.retirement_uncertain&&!encoder_calls);
 unsigned old_opens=opens,old_magic=magic;assert(continuous_next(&c)!=0&&start_h264_producer(&c)!=CAMERA_OK&&opens==old_opens&&magic==old_magic&&live[43]);
#ifdef QC_RTSP_RECEIPT_API
 assert(c.watchdog_hold==43);
 if(mode==2)assert(c.encoder_hold>=100&&live[c.encoder_hold]&&c.h264_stream_fd>=100&&live[c.h264_stream_fd]);
#endif
 if(mode<3)assert(live[40]&&live[41]);
 else assert(!live[41]&&!begins&&!kernel_source.hold);
 assert(magic==(mode==2?1U:0U)); /* encoder failure never magic-disarms its guard */
}
static void double_expiry(void)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);unsigned before_magic=magic;expire_every_next=1;
 struct camera_packet packet={0};assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&!packet.data&&begins==2&&acks==1&&pets==1&&kernel_source.hold==current);
 terminal(&c,before_magic,2);assert(current->state==QC_RECEIPT_RETIRED&&current->reason==QC_STOP_EXPIRY);
}
static void generation_wrap(void)
{
 struct qc_rtsp_context c;reset(&c);kernel_source.next_generation=~(__u64)0;
 struct camera_packet p={0};assert(produce_h264_frame(&c,0,&p)!=CAMERA_OK&&!begins&&!kernel_source.hold);terminal(&c,0,0);
}
static void output_fault(unsigned mode)
{
 struct qc_rtsp_context c;reset(&c);bad_encoder_output=mode;
 struct camera_packet packet={0};assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&!packet.data&&encoder_calls==1&&!output_allocation);
 terminal(&c,1,1);
#ifdef QC_RTSP_RECEIPT_API
 assert(c.encoder_hold>=100&&live[c.encoder_hold]&&c.watchdog_hold==43&&c.h264_stream_fd>=100&&live[c.h264_stream_fd]);
#endif
}
static void clock_failure(unsigned mode)
{
 struct qc_rtsp_context c;reset(&c);
 if(mode==0)clock_fault=1;
 else if(mode==1){frame(&c,1);now_ms--;}
 else if(mode==2)clock_after_next=1;
 else if(mode==3)clock_after_encoder=1;
 else {frame(&c,1);clock_fault=1;assert(producer_clock_ns(&c)==UINT64_MAX);assert(idle_h264_checked(&c)!=CAMERA_OK);}
 struct camera_packet packet={0};assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&!packet.data&&c.retirement_uncertain);
 assert(source_clock.failed&&!acks&&!ends);
 unsigned old_magic=magic,old_opens=opens;clock_fault=0;
 assert(produce_h264_frame(&c,0,&packet)!=CAMERA_OK&&opens==old_opens&&magic==old_magic);
 if(mode==0)assert(!opens&&!magic&&!kernel_source.hold);
 else {assert(live[40]&&live[41]&&kernel_source.hold==current);}
#ifdef QC_RTSP_RECEIPT_API
 if(mode==3)assert(c.watchdog_hold==43&&c.encoder_hold>=100&&live[c.encoder_hold]);
#endif
}
static void schema_failure(unsigned mode)
{
 struct qc_rtsp_context c;reset(&c);frame(&c,1);unsigned before_magic=magic;
 reply_schema=mode;assert(mode>=1&&mode<=3);
 struct camera_packet packet={0};int r=produce_h264_frame(&c,0,&packet);
 /* Synthetic reply mutation only: the entire raw data and full CRC are valid.
  * This does not assert observed kernel corruption or a real-world exploit. */
 assert(independent_crc(raw,sizeof(raw))==reference_crc&&qc_crc_bzip2(raw,sizeof(raw))==reference_crc);
 assert(r!=CAMERA_OK&&!packet.data&&encoder_calls==1&&frame_count==1);
 terminal(&c,before_magic,1);assert(!acks&&!ends&&kernel_source.hold==current);
}
int main(int argc,char **argv)
{
 assert(argc==2);alarm(60);const char *name=argv[1];
 if(!strcmp(name,"ttl"))renewal(0);
 else if(!strcmp(name,"limit128"))renewal(1);
 else if(!strcmp(name,"sessions"))sessions();
 else if(!strcmp(name,"namespace"))common_namespaces();
 else if(!strcmp(name,"held-observer"))held_and_observer();
 else if(!strcmp(name,"double-expiry"))double_expiry();
 else if(!strcmp(name,"generation-wrap"))generation_wrap();
 else if(!strncmp(name,"output-",7))output_fault((unsigned)strtoul(name+7,NULL,10));
 else if(!strncmp(name,"clock-",6))clock_failure((unsigned)strtoul(name+6,NULL,10));
 else if(!strncmp(name,"schema-",7))schema_failure((unsigned)strtoul(name+7,NULL,10));
 else if(!strncmp(name,"prequery-",9))prebegin_query((unsigned)strtoul(name+9,NULL,10));
 else if(!strncmp(name,"extra-",6))extra_failure((unsigned)strtoul(name+6,NULL,10));
 else if(!strncmp(name,"bind-",5)){unsigned n=(unsigned)strtoul(name+5,NULL,10);admission_fault(QC_ISP_RECEIPT_BIND,n<2?(int)n+1:0,n==2?ENOTTY:0,n>2?n-2:0);}
 else if(!strncmp(name,"arm-",4)){unsigned n=(unsigned)strtoul(name+4,NULL,10);admission_fault(QC_ISP_RECEIPT_ARM,n<2?(int)n+1:0,0,n>=2?n-1:0);}
 else if(!strncmp(name,"next-",5))next_fault((unsigned)strtoul(name+5,NULL,10));
 else if(!strncmp(name,"close-",6))close_fault((unsigned)strtoul(name+6,NULL,10));
 else if(!strncmp(name,"retired-",8))retired_skip((unsigned)strtoul(name+8,NULL,10));
 else if(!strncmp(name,"stale-",6))fd_reuse_stale((unsigned)strtoul(name+6,NULL,10));
 else if(!strncmp(name,"fdclose-",8))close_fd_error((unsigned)strtoul(name+8,NULL,10));
 else return 2;
 printf("PASS actual-adapter %s raw_bytes=%u CRC=%08x generations=%u begins=%u END=%u ACK=%u watchdog_magic=%u max_P_run=%u HOST-MOCK-NOT-HARDWARE\n",name,FRAME_BYTES,reference_crc,file_count,begins,ends,acks,magic,max_p_run);
 return 0;
}
