/* SPDX-License-Identifier: GPL-2.0-only
 * Initial camera-served RTSP/RTP-JPEG over TCP, using repeated bounded ISP
 * captures. Not production V4L2/vb2 or an H264 encoder. No flash/card writes.
 * --fixture is an explicit host-only transport test, never a live-camera claim.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <jpeglib.h>
#include "../hwenc/qc07f_hwenc.h"
#include "h264-baseline.h"
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "crc-bzip2.h"
#include "capture-receipt.h"
#include <camera/rtp_h264.h>
#include <camera/rtsp_server.h>
#include <camera/rtsp_clock.h>

#define SENSOR_WIDTH 1280U
#define SENSOR_HEIGHT 720U
#define MAIN_PIXELS (1280U * 720U)
#define FRAME_BYTES 1728000U
static volatile sig_atomic_t stop;
static uint8_t raw[FRAME_BYTES];
static uint8_t scaled[640U * 360U * 3U / 2U];
static unsigned width=1280,height=720,quality=70,interval_ms=0;
static int hardware_jpeg;
static int hardware_h264;
static int h264_p_frames;
static int continuous_capture;
static int capture_irq;
static int fanout;
static unsigned send_deadline_ms=500;
static int tcp_coalesce;
static unsigned trace_every=1;
/* Explicit public URI registration, not a filesystem route or auth realm. */
static const char *resource_uri;
static struct camera_rtsp_registry resources;
static unsigned request_deadline_ms=CAMERA_RTSP_REQUEST_DEADLINE_DEFAULT_MS;
/* One serial physical source context; this does not create another ISP/encoder
 * or migrate the existing raw/config globals into a concurrent camera adapter.
 */
struct qc_rtsp_context {
 int h264_stream_fd,continuous_sensor,continuous_frame,retirement_uncertain;
 int watchdog_hold,encoder_hold,legacy_control;
 struct qc_capture_receipt receipt_key,receipt_evidence;
 uint64_t last_source_instance,last_file_instance,last_capture_generation;
 unsigned receipt_armed,capture_sequence,ownership_idr,ownership_idrs,receipt_renewals;
 unsigned h264_stream_frames;uint8_t *produced_storage;
 struct {uint64_t begin,captured,encoded;unsigned bytes;int keyframe;} produced_timing;
};
#define QC_RTSP_RECEIPT_API 1U
#define QC_RTSP_LEGACY_SERVICE_API 1U
/* Stable listener-owned context, not a fresh physical owner per TCP session.
 * A terminal retirement result cannot be reset by accepting another client.
 */
struct qc_legacy_service {
 struct qc_rtsp_context source;
 int terminal_result;
};
static void source_context_init(struct qc_rtsp_context *c)
{memset(c,0,sizeof(*c));c->h264_stream_fd=c->continuous_sensor=c->continuous_frame=c->watchdog_hold=c->encoder_hold=c->legacy_control=-1;}
static uint8_t hw_quant[128];
static unsigned char *hw_header;
static unsigned long hw_header_bytes;
static const char *fixture;
static unsigned long frame_count;
static void on_signal(int sig) { (void)sig; stop = 1; }
static struct camera_rtsp_clock source_clock;
static uint64_t milliseconds(void)
{return camera_rtsp_clock_ms(&source_clock);}
static int read_all(int fd, void *data, size_t n)
{
 uint8_t *p=data;
 while(n){ssize_t r=read(fd,p,n);if(r<0&&errno==EINTR)continue;if(r<=0)return -1;p+=r;n-=r;}
 return 0;
}
static int send_all(int fd, const void *data, size_t n)
{
 const uint8_t *p=data;
 while(n){ssize_t r=send(fd,p,n,MSG_NOSIGNAL);if(r<0&&errno==EINTR)continue;if(r<=0)return -1;p+=r;n-=r;}
 return 0;
}
static int sensor_read(int fd, uint16_t reg)
{
 uint8_t pointer[2]={reg>>8,reg},value=0;
 struct i2c_msg msgs[2]={{.addr=0x30,.len=2,.buf=pointer},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&value}};
 struct i2c_rdwr_ioctl_data request={.msgs=msgs,.nmsgs=2};
 return ioctl(fd,I2C_RDWR,&request)==2?value:-1;
}
static int sensor_stream(int fd, uint8_t on)
{ uint8_t bytes[3]={1,0,on};return write(fd,bytes,3)==3&&sensor_read(fd,0x0100)==on?0:-1; }
static int capture_failed(struct qc_rtsp_context *c)
{c->retirement_uncertain=1;return CAMERA_IO_ERROR;}
static int protected_guard_finish(struct qc_rtsp_context *c,int fd,int proved)
{
 if(fd<0)return proved?CAMERA_OK:CAMERA_IO_ERROR;
 if(!proved||c->retirement_uncertain){c->watchdog_hold=fd;return capture_failed(c);}
 if(write(fd,"V",1)!=1){c->watchdog_hold=fd;return capture_failed(c);}
 if(close(fd)){c->watchdog_hold=fd;return capture_failed(c);}
 return CAMERA_OK;
}
static int continuous_retire(struct qc_rtsp_context *c,unsigned expiry_only)
{
 struct qc_capture_receipt proof;
 if(c->retirement_uncertain)return CAMERA_IO_ERROR; /* never retry uncertain END/ACK/close */
 if(c->produced_storage)return CAMERA_BUSY; /* a held AU is not an idle boundary */
 if(c->continuous_frame<0)return c->continuous_sensor<0?CAMERA_OK:capture_failed(c);
 if(!c->receipt_armed||qc_receipt_query(c->continuous_frame,&c->receipt_key,&proof))return capture_failed(c);
 c->receipt_evidence=proof;
 if(expiry_only){
  if(!qc_receipt_retired(&proof)||proof.reason!=QC_STOP_EXPIRY)return capture_failed(c);
 }else if(proof.state==QC_RECEIPT_ACTIVE){
  if(proof.reason!=QC_STOP_NONE||proof.stop_result||proof.stopped_ns)return capture_failed(c);
 }else if(!qc_receipt_retired(&proof))return capture_failed(c);
 /* Do not stop somebody else's sensor on a bad/stale/missing proof. Existing
  * valid-owner stop order remains sensor-off then explicit ISP END if ACTIVE.
  */
 if(c->continuous_sensor<0||sensor_stream(c->continuous_sensor,0))return capture_failed(c);
 if(proof.state==QC_RECEIPT_ACTIVE){
  if(proof.reason!=QC_STOP_NONE||proof.stop_result||proof.stopped_ns||ioctl(c->continuous_frame,QC_ISP_END))return capture_failed(c);
  if(qc_receipt_query(c->continuous_frame,&c->receipt_key,&proof))return capture_failed(c);
  c->receipt_evidence=proof;
  if(!qc_receipt_retired(&proof)||proof.reason!=QC_STOP_END)return capture_failed(c);
 }
 if(!qc_receipt_retired(&proof))return capture_failed(c);
 struct qc_capture_receipt ack=qc_receipt_request(&c->receipt_key);
 if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_ACK,&ack))return capture_failed(c);
 c->receipt_armed=0; /* ACK is the admission barrier, not close(fd)/ownerNULL. */
 if(close(c->continuous_sensor))return capture_failed(c);c->continuous_sensor=-1;
 if(close(c->continuous_frame))return capture_failed(c);c->continuous_frame=-1;
 /* At this serialized boundary no produced packet is held. A fresh reference
  * owner must not consume a P frame based on the preceding capture generation.
  */
 if(c->produced_storage)return capture_failed(c);
 if(c->h264_stream_fd>=0){if(close(c->h264_stream_fd))return capture_failed(c);c->h264_stream_fd=-1;}
 c->h264_stream_frames=0;c->ownership_idr=1;
 return CAMERA_OK;
}
static int continuous_close_checked(struct qc_rtsp_context *c)
{return continuous_retire(c,0);}
static int continuous_next(struct qc_rtsp_context *c)
{
 int watchdog=-1,timeout=5,result=-1;
 if(c->retirement_uncertain||c->watchdog_hold>=0)return -1;
 watchdog=open("/dev/watchdog0",O_WRONLY);if(watchdog<0||ioctl(watchdog,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 for(unsigned attempt=0;attempt<2;attempt++){
   if(c->continuous_frame<0){
    c->continuous_sensor=open("/dev/i2c-0",O_RDWR);
    if(c->continuous_sensor<0||ioctl(c->continuous_sensor,I2C_SLAVE,0x30)||sensor_read(c->continuous_sensor,0x3107)!=0xda||sensor_read(c->continuous_sensor,0x3108)!=0x4d||sensor_read(c->continuous_sensor,0x3208)!=5||sensor_read(c->continuous_sensor,0x3209)!=0||sensor_read(c->continuous_sensor,0x320a)!=2||sensor_read(c->continuous_sensor,0x320b)!=0xd0)goto done;
     c->continuous_frame=open("/dev/qc07f-frame",O_RDONLY);
     if(c->continuous_frame<0)goto done;
     struct qc_capture_receipt bound=qc_receipt_request(NULL);
     /* Strict copied adapter: ENOTTY is unsupported, not guessed legacy proof.
      * This binary is not compatible with the current target73 receipt-less ABI.
      */
     if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_BIND,&bound))goto done;
     if(!qc_receipt_header(&bound)||bound.generation||bound.state!=QC_RECEIPT_NONE||bound.reason||bound.stop_result||bound.stopped_ns||
        (c->last_source_instance&&(bound.source_instance!=c->last_source_instance||bound.file_instance<=c->last_file_instance)))goto done;
     c->last_source_instance=bound.source_instance;c->last_file_instance=bound.file_instance;
     c->receipt_key=qc_receipt_request(&bound);c->receipt_evidence=bound;
     struct qc_capture_receipt armed=qc_receipt_request(&bound);
     if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_ARM,&armed))goto done;
     if(!qc_receipt_header(&armed)||armed.source_instance!=bound.source_instance||armed.file_instance!=bound.file_instance||
        !armed.generation||armed.generation<=c->last_capture_generation||armed.state!=QC_RECEIPT_ARMED||armed.reason||armed.stop_result||armed.stopped_ns)goto done;
     c->receipt_key=qc_receipt_request(&armed);c->receipt_evidence=armed;c->receipt_armed=1;
     c->last_capture_generation=armed.generation;c->capture_sequence=0;c->ownership_idr=1;
     struct qc_capture_receipt reserved;
     if(qc_receipt_query(c->continuous_frame,&c->receipt_key,&reserved)||reserved.state!=QC_RECEIPT_ARMED||reserved.reason||reserved.stop_result||reserved.stopped_ns)goto done;
     c->receipt_evidence=reserved; /* prove the exact ARM key before hardware BEGIN */
     if(ioctl(c->continuous_frame,capture_irq?QC_ISP_BEGIN_IRQ:QC_ISP_BEGIN))goto done;
     struct qc_capture_receipt active;
     if(qc_receipt_query(c->continuous_frame,&c->receipt_key,&active)||active.state!=QC_RECEIPT_ACTIVE||active.reason||active.stop_result||active.stopped_ns)goto done;
     c->receipt_evidence=active;
     if(sensor_stream(c->continuous_sensor,1))goto done;
  }
  struct qc_isp_next q={.bytes=sizeof(raw),.user_ptr=(uint32_t)(uintptr_t)raw};
   if(!ioctl(c->continuous_frame,QC_ISP_NEXT,&q)){
    if(q.bytes!=sizeof(raw)||q.user_ptr!=(uint32_t)(uintptr_t)raw||q.guard_errors||!(q.status&1)||q.sequence!=c->capture_sequence+1||q.sequence>128)goto done;
   uint32_t delivered=qc_crc_bzip2(raw,sizeof(raw));
   if(delivered!=q.crc32_bzip2){
    fprintf(stderr,"ISP CRC MISMATCH target=%08x delivered=%08x; REFUSE liveframe attempt=%u\n",q.crc32_bzip2,delivered,attempt);
      goto done; /* data corruption is terminal; never disguised as expiry */
   }
   if(!(frame_count%trace_every))fprintf(stderr,"ISP GATED-CONTINUOUS seq=%u status=%08x guards=%u crc_bzip2=%08x\n",q.sequence,q.status,q.guard_errors,q.crc32_bzip2);
    struct qc_capture_receipt active;
    if(qc_receipt_query(c->continuous_frame,&c->receipt_key,&active)||active.state!=QC_RECEIPT_ACTIVE||active.reason||active.stop_result||active.stopped_ns)goto done;
    c->receipt_evidence=active;c->capture_sequence=q.sequence;result=0;break;
  }
  int failure=errno;
   fprintf(stderr,"ISP NEXT REJECT errno=%d attempt=%u; noframeencoded, receipt-required\n",failure,attempt);
   if(failure!=ETIMEDOUT||attempt||continuous_retire(c,1)!=CAMERA_OK)goto done;
   c->receipt_renewals++;
   if(write(watchdog,".",1)!=1)goto done;
 }
done:
  if(result)capture_failed(c);
  if(protected_guard_finish(c,watchdog,!result)!=CAMERA_OK)result=-1;
  return result;
}
static int capture(struct qc_rtsp_context *c)
{
 int fd=-1,sensor=-1,watchdog=-1,control=-1,result=-1,timeout=5;
 if(fixture){fd=open(fixture,O_RDONLY);if(fd<0)return -1;result=read_all(fd,raw,sizeof(raw));close(fd);return result;}
 if(continuous_capture)return continuous_next(c);
 sensor=open("/dev/i2c-0",O_RDWR);if(sensor<0)goto done;
 if(ioctl(sensor,I2C_SLAVE,0x30)||sensor_read(sensor,0x3107)!=0xda||sensor_read(sensor,0x3108)!=0x4d)goto done;
 /* Caller initializes the measured139-pair table once. Refuse other geometry. */
 if(sensor_read(sensor,0x3208)!=5||sensor_read(sensor,0x3209)!=0||sensor_read(sensor,0x320a)!=2||sensor_read(sensor,0x320b)!=0xd0)goto done;
 watchdog=open("/dev/watchdog0",O_WRONLY);if(watchdog<0||ioctl(watchdog,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 control=open("/sys/bus/platform/devices/20000000.isp/capture",O_WRONLY);if(control<0)goto done;
 if(sensor_stream(sensor,1)||write(control,"1\n",2)!=2)goto done;
 if(sensor_stream(sensor,0))goto done;
 fd=open("/dev/qc07f-frame",O_RDONLY);if(fd<0)goto done;
 result=read_all(fd,raw,sizeof(raw));
done:
 if(sensor>=0&&sensor_stream(sensor,0))result=-1;
 if(result){c->continuous_sensor=sensor;c->continuous_frame=fd;c->legacy_control=control;capture_failed(c);protected_guard_finish(c,watchdog,0);return result;}
 if(sensor>=0)close(sensor);
 if(fd>=0)close(fd);if(control>=0)close(control);
 if(protected_guard_finish(c,watchdog,!result)!=CAMERA_OK)result=-1;
 return result;
}
struct jpeg_failure { struct jpeg_error_mgr base; jmp_buf jump; };
static void jpeg_error(j_common_ptr common)
{ struct jpeg_failure *failure=(struct jpeg_failure *)common->err;longjmp(failure->jump,1); }
static int encode(unsigned char **jpeg, unsigned long *size)
{
 struct jpeg_compress_struct c={0};struct jpeg_failure failure;
 const uint8_t *planes[3]={raw,raw+MAIN_PIXELS,raw+MAIN_PIXELS*5/4};
 unsigned strides[3]={SENSOR_WIDTH,SENSOR_WIDTH/2,SENSOR_WIDTH/2};
 c.err=jpeg_std_error(&failure.base);failure.base.error_exit=jpeg_error;
 if(setjmp(failure.jump)){jpeg_destroy_compress(&c);return -1;}
 /* Native mode passes the actual coherent-capture Y/U/V planes directly
  * to libjpeg: no upscale, RGB expansion, crop or chroma reinterpretation.
  * Lower modes remain explicit fallbacks, never silently selected by Wi-Fi.
  */
 if(width!=SENSOR_WIDTH){
  unsigned step=SENSOR_WIDTH/width,crop=height==176?8:0;
  uint8_t *dest[3]={scaled,scaled+width*height,scaled+width*height*5/4};
  for(unsigned p=0;p<3;p++){
   unsigned w=p?width/2:width,h=p?height/2:height,y0=p?crop/2:crop;
   for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++)
    dest[p][y*w+x]=planes[p][(y0+y*step)*strides[p]+x*step];
   planes[p]=dest[p];strides[p]=w;
  }
 }
 jpeg_create_compress(&c);jpeg_mem_dest(&c,jpeg,size);
 c.image_width=width;c.image_height=height;c.input_components=3;c.in_color_space=JCS_YCbCr;
 jpeg_set_defaults(&c);jpeg_set_quality(&c,quality,TRUE);c.optimize_coding=FALSE;
 c.raw_data_in=TRUE;c.dct_method=JDCT_IFAST;
 jpeg_start_compress(&c,TRUE);
 while(c.next_scanline<c.image_height){
  JSAMPROW y[16],u[8],v[8];JSAMPARRAY rows[3]={y,u,v};
  unsigned first=c.next_scanline;
  for(unsigned i=0;i<16;i++){unsigned row=first+i;if(row>=height)row=height-1;y[i]=(JSAMPROW)(planes[0]+row*strides[0]);}
  for(unsigned i=0;i<8;i++){unsigned row=first/2+i;if(row>=height/2)row=height/2-1;u[i]=(JSAMPROW)(planes[1]+row*strides[1]);v[i]=(JSAMPROW)(planes[2]+row*strides[2]);}
  if(jpeg_write_raw_data(&c,rows,16)!=16){jpeg_destroy_compress(&c);return -1;}
 }
 jpeg_finish_compress(&c);jpeg_destroy_compress(&c);return 0;
}
static int prepare_hw_header(void)
{
 struct jpeg_compress_struct c={0};struct jpeg_failure failure;unsigned char *tables=NULL;unsigned long bytes=0;
 c.err=jpeg_std_error(&failure.base);failure.base.error_exit=jpeg_error;
 if(setjmp(failure.jump)){jpeg_destroy_compress(&c);free(tables);return -1;}
 jpeg_create_compress(&c);jpeg_mem_dest(&c,&tables,&bytes);
 c.image_width=width;c.image_height=height;c.input_components=3;c.in_color_space=JCS_YCbCr;
 jpeg_set_defaults(&c);jpeg_set_quality(&c,quality,TRUE);
 for(unsigned p=0;p<2;p++)for(unsigned i=0;i<64;i++)hw_quant[p*64+i]=c.quant_tbl_ptrs[p]->quantval[i];
 /* Marker/standard Huffman tables only; no software image/DCT/entropy encode. */
 jpeg_write_tables(&c);jpeg_destroy_compress(&c);
 if(bytes<4||tables[bytes-2]!=255||tables[bytes-1]!=0xd9){free(tables);return -1;}
 hw_header_bytes=bytes-2+19+14;hw_header=malloc(hw_header_bytes);
 if(!hw_header){free(tables);return -1;}
 memcpy(hw_header,tables,bytes-2);free(tables);
 uint8_t frame[19]={255,0xc0,0,17,8,0,0,0,0,3,1,0x22,0,2,0x11,1,3,0x11,1};
 frame[5]=height>>8;frame[6]=height;frame[7]=width>>8;frame[8]=width;
 uint8_t scan[14]={255,0xda,0,12,3,1,0,2,0x11,3,0x11,0,63,0};
 memcpy(hw_header+hw_header_bytes-33,frame,sizeof(frame));memcpy(hw_header+hw_header_bytes-14,scan,sizeof(scan));
 return 0;
}
static int encode_hw(struct qc_rtsp_context *source,unsigned char **jpeg,unsigned long *size)
{
 int fd=-1,guard=-1,result=-1,timeout=5;
 if(width!=1280||height!=720||fixture){fprintf(stderr,"Refuse unqualified hardware profile/fixture\n");return -1;}
 if(!hw_header&&prepare_hw_header())return -1;
 *jpeg=malloc(hw_header_bytes+0x100000);if(!*jpeg)return -1;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0)goto done;
 if(ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 fd=open("/dev/qc07f-encoder",O_RDWR);if(fd<0)goto done;
 struct qc_hwenc_job job={.width=width,.height=height,.input_bytes=MAIN_PIXELS*3/2,.input_ptr=(uint32_t)(uintptr_t)raw,.output_capacity=0x100000,.output_ptr=(uint32_t)(uintptr_t)(*jpeg+hw_header_bytes)};
 memcpy(job.quant,hw_quant,sizeof(hw_quant));
 if(ioctl(fd,QC_HWENC_JPEG,&job)){fprintf(stderr,"Hardware JPEG ioctl failed errno=%d status=%08x bytes=%u; NO software fallback\n",errno,job.status,job.output_bytes);goto done;}
 if(job.output_bytes<2||job.output_bytes>0x100000||(*jpeg)[hw_header_bytes+job.output_bytes-2]!=255||(*jpeg)[hw_header_bytes+job.output_bytes-1]!=0xd9)goto done;
 memcpy(*jpeg,hw_header,hw_header_bytes);*size=hw_header_bytes+job.output_bytes;result=0;
done:
 if(fd>=0){
  if(result){source->encoder_hold=fd;}
  else if(close(fd)){source->encoder_hold=fd;result=-1;}
 }
 if(result)source->retirement_uncertain=1;
 if(protected_guard_finish(source,guard,!result)!=CAMERA_OK)result=-1;
 if(result){source->retirement_uncertain=1;free(*jpeg);*jpeg=NULL;}return result;
}
static int jpeg_scan(const uint8_t *jpeg,size_t size,const uint8_t **scan,size_t *bytes,uint8_t quant[128])
{
 size_t pos=2;unsigned found=0;
 if(size<4||jpeg[0]!=255||jpeg[1]!=0xd8)return -1;
 while(pos+4<=size){
  if(jpeg[pos++]!=255)return -1;
  unsigned marker=jpeg[pos++];size_t length=(jpeg[pos]<<8)|jpeg[pos+1];
  if(length<2||length>size-pos)return -1;
  if(marker==0xdb){
   size_t q=pos+2;
   while(q<pos+length){unsigned id=jpeg[q++];if(id>1||q+64>pos+length)return -1;memcpy(quant+id*64,jpeg+q,64);found|=1U<<id;q+=64;}
  }
  if(marker==0xda){*scan=jpeg+pos+length;*bytes=size-(pos+length);if(*bytes<2||jpeg[size-2]!=255||jpeg[size-1]!=0xd9||found!=3)return -1;*bytes-=2;return 0;}
  pos+=length;
 }
 return -1;
}
static void be16(uint8_t *p,unsigned v){p[0]=v>>8;p[1]=v;}
static void be32(uint8_t *p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
/* RFC6184 packetization-mode1. No software slice encoding; independent
 * boundedIDR frames until reference/P-frame sequencing is qualified.
 */
static int send_rtp_packet(void *context,const uint8_t *data,size_t bytes)
{return send_all(*(int *)context,data,bytes);}
static int send_h264_nal(int fd,unsigned channel,uint16_t *sequence,uint32_t timestamp,const uint8_t *nal,size_t bytes,int last)
{
 if(channel>254)return -1;
 struct camera_rtp_session session={.ssrc=0x51433037,.sequence=*sequence,.channel=channel,.payload_type=96};
 int result=camera_rtp_h264_nal(&session,timestamp,nal,bytes,last,send_rtp_packet,&fd);
 *sequence=session.sequence;return result==CAMERA_OK?0:-1;
}
static int produce_h264_frame(void *context,uint32_t force_keyframe,struct camera_packet *packet)
{
   struct qc_rtsp_context *c=context;if(!c)return CAMERA_INVALID;
  uint64_t begin=milliseconds(),captured,encoded;
   uint8_t *output=NULL;
  int encoder=-1,guard=-1,result=CAMERA_IO_ERROR,timeout=5;
   if(c->produced_storage)return CAMERA_BUSY;
   if(c->retirement_uncertain)return CAMERA_IO_ERROR;
   if(begin==UINT64_MAX){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}
   if(capture(c)){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}captured=milliseconds();
   if(captured==UINT64_MAX){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}
  output=malloc(0x100000);if(!output)return CAMERA_IO_ERROR;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 encoder=open("/dev/qc07f-encoder",O_RDWR);if(encoder<0)goto done;
 struct qc_hwenc_h264_job job={.width=width,.height=height,.input_bytes=MAIN_PIXELS*3/2,.input_ptr=(uint32_t)(uintptr_t)raw,.output_capacity=0x100000,.output_ptr=(uint32_t)(uintptr_t)output};
   int ownership_keyframe=c->ownership_idr;
   int keyframe=force_keyframe||ownership_keyframe||!h264_p_frames||!(c->h264_stream_frames%30);
 if(h264_p_frames){
   if(c->h264_stream_fd<0)c->h264_stream_fd=open("/dev/qc07f-encoder",O_RDWR);
   if(c->h264_stream_fd<0)goto done;
  struct qc_hwenc_h264_stream_job q={.frame=job,.keyframe=keyframe};
   if(ioctl(c->h264_stream_fd,QC_HWENC_H264_STREAM,&q))goto done;job=q.frame;
 }else if(ioctl(encoder,QC_HWENC_H264_IDR,&job))goto done;
  encoded=milliseconds();
  if(encoded==UINT64_MAX)goto done;
  if(job.output_bytes<3||job.output_bytes>0x100000)goto done;
 size_t start=0;
 if(!output[0]&&!output[1]){if(output[2]==1)start=3;else if(job.output_bytes>4&&!output[2]&&output[3]==1)start=4;}
  if(job.output_bytes<=start+1||(output[start]&31)!=(keyframe?5:1))goto done;
   *packet=(struct camera_packet){.codec=CAMERA_H264,.keyframe=keyframe,.data=output+start,.bytes=job.output_bytes-start,.monotonic_ns=captured*1000000ULL};
   /* Always compute the same full-NAL diagnostic CRC; sparse logs are not
    * permission to weaken raw delivery/guard checks or skip checked bytes.
    */
   uint32_t nal_crc=qc_crc_bzip2(packet->data,packet->bytes);
   if(!(frame_count%trace_every))fprintf(stderr,"RTSP SOURCE timestamp=%u nal_bytes=%zu crc_bzip2=%08x keyframe=%u\n",(uint32_t)(captured*90),packet->bytes,nal_crc,packet->keyframe);
   if(keyframe&&c->receipt_armed)fprintf(stderr,"RTSP CAPTURE KEY source=%llu file=%llu generation=%llu IDR=%s LAB-NOT-AUTH\n",
    (unsigned long long)c->receipt_key.source_instance,(unsigned long long)c->receipt_key.file_instance,(unsigned long long)c->receipt_key.generation,
    ownership_keyframe?"ownership":force_keyframe?"demand":!h264_p_frames?"independent":"max-GOP30");
   c->produced_storage=output;output=NULL;
   c->produced_timing.begin=begin;c->produced_timing.captured=captured;c->produced_timing.encoded=encoded;
   c->produced_timing.bytes=job.output_bytes;c->produced_timing.keyframe=keyframe;
   frame_count++;if(keyframe)c->h264_stream_frames=0;c->h264_stream_frames++;
   if(ownership_keyframe&&keyframe){c->ownership_idr=0;c->ownership_idrs++;}
   if(!((frame_count-1)%trace_every))fprintf(stderr,"RTSP PRODUCED n=%lu width=%u height=%u h264_bytes=%u capture_ms=%llu encode_ms=%llu ACTUAL-CAMERA encoder=%s nal=%s\n",frame_count,width,height,job.output_bytes,(unsigned long long)(captured-begin),(unsigned long long)(encoded-captured),h264_p_frames?"HARDWARE-H264-IP":"HARDWARE-H264-IDR",keyframe?"IDR":"P");
 result=0;
done:
   if(encoder>=0){
    if(result!=CAMERA_OK)c->encoder_hold=encoder;
    else if(close(encoder)){c->encoder_hold=encoder;c->retirement_uncertain=1;result=CAMERA_IO_ERROR;}
   }
   if(result!=CAMERA_OK)c->retirement_uncertain=1;
   if(protected_guard_finish(c,guard,result==CAMERA_OK)!=CAMERA_OK)result=CAMERA_IO_ERROR;
   free(output);
   /* Error returns do not establish reset/DMA retirement on this diagnostic
    * ABI. Do not convert a descriptor-less failed transaction into idle OK.
    * Parent-owned, qualified recovery must resolve this uncertainty explicitly.
    */
   if(result!=CAMERA_OK){
    c->retirement_uncertain=1;
    free(c->produced_storage);c->produced_storage=NULL;memset(packet,0,sizeof(*packet));
   }
   return result;
}
static void release_h264_frame(void *context,const struct camera_packet *packet)
{struct qc_rtsp_context *c=context;(void)packet;free(c->produced_storage);c->produced_storage=NULL;}
static int idle_h264_checked(void *context)
{
 struct qc_rtsp_context *c=context;
  if(!c||c->produced_storage)return CAMERA_BUSY;
 if(source_clock.failed){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}
 int r=continuous_close_checked(c);if(r)return r;
 if(c->h264_stream_fd>=0){int fd=c->h264_stream_fd;c->h264_stream_fd=-1;if(close(fd))c->retirement_uncertain=1;}
 if(c->retirement_uncertain)return CAMERA_IO_ERROR;
 c->h264_stream_frames=0;return CAMERA_OK;
}
static void idle_h264_producer(void *context)
{
 if(idle_h264_checked(context)!=CAMERA_OK)fprintf(stderr,"RTSP checked idle failed; not producer retirement evidence\n");
}
static int start_h264_producer(void *context)
{
 struct qc_rtsp_context *c=context;
 /* Logical admission for a lazy synchronous source, not a hardware boot/result.
  * Actual capture/encode remains next(), with unchanged guard and CRC checks.
  */
 return c&&!source_clock.failed&&!c->retirement_uncertain&&!c->produced_storage&&c->continuous_frame<0&&
  c->continuous_sensor<0&&c->h264_stream_fd<0?CAMERA_OK:CAMERA_NOT_READY;
}
static uint64_t producer_clock_ns(void *context)
{(void)context;return camera_rtsp_clock_ns(&source_clock);}
static void producer_event(void *context,const struct camera_producer_event *e)
{
 (void)context;
 if(e->result!=CAMERA_OK)fprintf(stderr,"RTSP lifecycle generation=%llu operation=%llu kind=%u state=%u result=%d reason=%u\n",(unsigned long long)e->work.token.generation,(unsigned long long)e->work.token.id,(unsigned)e->work.kind,(unsigned)e->state,e->result,(unsigned)e->reason);
}
static void producer_failure(void *context,const struct camera_rtsp_failure *e)
{
 (void)context;
 fprintf(stderr,"RTSP dependent failure generation=%llu frame=%llu demand=%llu stage=%u result=%d time_ns=%llu\n",(unsigned long long)e->source.generation,(unsigned long long)e->source.id,(unsigned long long)e->demand.id,(unsigned)e->stage,e->result,(unsigned long long)e->monotonic_ns);
}
static int send_h264_frame(struct qc_rtsp_context *c,int fd,unsigned channel,uint16_t *sequence)
{
 struct camera_packet packet;int result=-1;
 if(produce_h264_frame(c,0,&packet))return -1;
 uint8_t params[256];size_t n=qc_h264_parameter_sets(params),split=4;
 while(split+4<n&&memcmp(params+split,"\0\0\0\1",4))split++;
 uint32_t timestamp=(uint32_t)(packet.monotonic_ns/1000000ULL*90);
 if(split+4>=n||(packet.keyframe&&(send_h264_nal(fd,channel,sequence,timestamp,params+4,split-4,0)||
    send_h264_nal(fd,channel,sequence,timestamp,params+split+4,n-split-4,0)))||
    send_h264_nal(fd,channel,sequence,timestamp,packet.data,packet.bytes,1))goto done;
  uint64_t sent=milliseconds();
 if(sent==UINT64_MAX){c->retirement_uncertain=1;goto done;}
 fprintf(stderr,"RTSP FRAME n=%lu width=%u height=%u h264_bytes=%u capture_ms=%llu encode_ms=%llu send_ms=%llu total_ms=%llu ACTUAL-CAMERA encoder=%s nal=%s\n",frame_count,width,height,c->produced_timing.bytes,(unsigned long long)(c->produced_timing.captured-c->produced_timing.begin),(unsigned long long)(c->produced_timing.encoded-c->produced_timing.captured),(unsigned long long)(sent-c->produced_timing.encoded),(unsigned long long)(sent-c->produced_timing.begin),h264_p_frames?"HARDWARE-H264-IP":"HARDWARE-H264-IDR",c->produced_timing.keyframe?"IDR":"P");
 result=0;
 done:release_h264_frame(c,&packet);return result;
}
static int send_frame(struct qc_rtsp_context *c,int fd,unsigned channel,uint16_t *sequence)
{
 unsigned char *jpeg=NULL;unsigned long size=0;const uint8_t *scan;size_t bytes;
  uint8_t quant[128],packet[1404];uint64_t begin=milliseconds(),captured,encoded,sent;
 if(begin==UINT64_MAX){c->retirement_uncertain=1;goto fail;}
  if(capture(c)){c->retirement_uncertain=1;goto fail;}captured=milliseconds();
 if(captured==UINT64_MAX){c->retirement_uncertain=1;goto fail;}
 uint32_t timestamp=(uint32_t)(captured*90);
  if((hardware_jpeg?encode_hw(c,&jpeg,&size):encode(&jpeg,&size)))goto fail;encoded=milliseconds();
 if(encoded==UINT64_MAX){c->retirement_uncertain=1;goto fail;}
 if(jpeg_scan(jpeg,size,&scan,&bytes,quant)||bytes>=0x1000000)goto fail;
 for(size_t offset=0;offset<bytes;){
  size_t header=24,payload=bytes-offset;
  if(!offset){header+=132;packet[24]=0;packet[25]=0;be16(packet+26,128);memcpy(packet+28,quant,128);}
  if(payload>sizeof(packet)-header)payload=sizeof(packet)-header;
  packet[0]='$';packet[1]=channel;be16(packet+2,header+payload-4);
  packet[4]=0x80;packet[5]=26|((offset+payload==bytes)?0x80:0);
  be16(packet+6,(*sequence)++);be32(packet+8,timestamp);be32(packet+12,0x51433037);
  packet[16]=0;packet[17]=offset>>16;packet[18]=offset>>8;packet[19]=offset;
  packet[20]=1;packet[21]=255;packet[22]=width/8;packet[23]=height/8;
  memcpy(packet+header,scan+offset,payload);if(send_all(fd,packet,header+payload))goto fail;
  offset+=payload;
 }
 sent=milliseconds();
 if(sent==UINT64_MAX){c->retirement_uncertain=1;goto fail;}
 frame_count++;
 fprintf(stderr,"RTSP FRAME n=%lu width=%u height=%u quality=%u jpeg_bytes=%lu capture_ms=%llu encode_ms=%llu send_ms=%llu total_ms=%llu%s encoder=%s\n",
  frame_count,width,height,quality,size,(unsigned long long)(captured-begin),(unsigned long long)(encoded-captured),(unsigned long long)(sent-encoded),(unsigned long long)(sent-begin),fixture?" FIXTURE-NOT-LIVE":" ACTUAL-CAMERA",hardware_jpeg?"HARDWARE-JPEG":"SOFTWARE-JPEG");
 free(jpeg);return 0;
fail:free(jpeg);return -1;
}
static char sdp[512];
static int reply(int fd,unsigned cseq,unsigned code,const char *extra,const char *body)
{
 char header[2048];size_t length=body?strlen(body):0;
 int count=snprintf(header,sizeof(header),"RTSP/1.0 %u %s\r\nCSeq: %u\r\n%sContent-Length: %zu\r\n\r\n",code,code==200?"OK":"Unsupported",cseq,extra?extra:"",length);
 return count<0||(size_t)count>=sizeof(header)||send_all(fd,header,count)||(length&&send_all(fd,body,length))?-1:0;
}
static int client_session(struct qc_legacy_service *service,int fd)
{
 if(service->terminal_result!=CAMERA_OK){close(fd);return service->terminal_result;}
 struct qc_rtsp_context *source=&service->source;
 if(start_h264_producer(source)!=CAMERA_OK){
  service->terminal_result=CAMERA_NOT_READY;close(fd);return service->terminal_result;
 }
 struct camera_rtsp_input input;
 uint64_t touched=milliseconds();
 if(camera_rtsp_input_init(&input,touched,request_deadline_ms)){
  service->terminal_result=touched==UINT64_MAX?CAMERA_IO_ERROR:CAMERA_INVALID;
  close(fd);return service->terminal_result; /* input is NOT initialized; no clear/idle */
 }
 int playing=0,setup=0;unsigned channel=0;uint16_t sequence=1;uint64_t next=0;
 struct timeval timeout={5,0};setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
 while(!stop){
  /* A dueframe mustnotacquire an unconditional50mscontrolpoll delay.
   * Stillcheck queuedRTSP/RTCP beforecapture; onlywaitwhenpaced/notplaying.
   */
  int wait_ms=500;
  if(playing){uint64_t now=milliseconds();wait_ms=now>=next?0:(next-now>50?50:(int)(next-now));}
  uint64_t checked_now=milliseconds();
  if(camera_rtsp_input_tick(&input,checked_now))goto done;
  if(!playing&&checked_now-touched>=30000U)goto done;
  if(!playing){int left=(int)(30000U-(checked_now-touched));if(left<wait_ms)wait_ms=left;}
  if(input.deadline_active){
   uint64_t elapsed=checked_now-input.started_ms;
   int left=elapsed>=input.deadline_ms?0:(int)(input.deadline_ms-elapsed);
   if(left<wait_ms)wait_ms=left;
  }
  struct pollfd pollfd={.fd=fd,.events=POLLIN};int ready=poll(&pollfd,1,wait_ms);
  if(ready<0&&errno==EINTR)continue;if(ready<0||pollfd.revents&(POLLERR|POLLHUP|POLLNVAL))break;
  checked_now=milliseconds();if(camera_rtsp_input_tick(&input,checked_now))goto done;
  if(!playing&&checked_now-touched>=30000U)goto done;
  if(pollfd.revents&POLLIN){
   unsigned char incoming[2048];size_t capacity=sizeof(input.data)-input.used;
   if(!capacity){input.failure=CAMERA_RTSP_INPUT_FRAMING;goto done;}
   if(capacity>sizeof(incoming))capacity=sizeof(incoming);
   ssize_t count=recv(fd,incoming,capacity,MSG_DONTWAIT);
   if(count<0&&(errno==EINTR||errno==EAGAIN||errno==EWOULDBLOCK))continue;
   if(count<=0)break;
   int appended=camera_rtsp_input_append(&input,incoming,(size_t)count,milliseconds());
   explicit_bzero(incoming,sizeof(incoming));if(appended)goto done;
   while(input.used){
    struct camera_rtsp_request request;size_t n=0;enum camera_rtsp_method method;
    enum camera_rtsp_input_result parsed=camera_rtsp_input_next(&input,(unsigned)setup,milliseconds(),&request,&n);
    if(parsed==CAMERA_RTSP_INPUT_NEED_MORE)break;
    if(parsed==CAMERA_RTSP_INPUT_INVALID)goto done;
    if(parsed==CAMERA_RTSP_INPUT_RTCP){if(camera_rtsp_input_retire(&input,n,milliseconds()))goto done;continue;}
    enum camera_rtsp_input_failure bad=camera_rtsp_request_policy(&request,&resources,
     (unsigned)setup,(unsigned)playing,"QC07F1",&method);
    if(bad){input.failure=bad;goto done;}
    unsigned cseq=request.cseq;int result=0;
    if(method==CAMERA_RTSP_METHOD_OPTIONS)result=reply(fd,cseq,200,"Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, GET_PARAMETER, TEARDOWN\r\n",NULL);
    else if(method==CAMERA_RTSP_METHOD_DESCRIBE)result=reply(fd,cseq,200,"Content-Type: application/sdp\r\n",sdp);
    else if(method==CAMERA_RTSP_METHOD_SETUP){
     channel=0;setup=1;result=reply(fd,cseq,200,"Session: QC07F1;timeout=30\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n",NULL);
    }
    else if(method==CAMERA_RTSP_METHOD_PLAY){playing=1;next=milliseconds();result=reply(fd,cseq,200,"Session: QC07F1\r\nRange: npt=0.000-\r\n",NULL);}
    else if(method==CAMERA_RTSP_METHOD_PAUSE){playing=0;result=reply(fd,cseq,200,"Session: QC07F1\r\n",NULL);}
    else if(method==CAMERA_RTSP_METHOD_GET_PARAMETER)result=reply(fd,cseq,200,setup?"Session: QC07F1\r\n":NULL,NULL);
    else if(method==CAMERA_RTSP_METHOD_TEARDOWN){reply(fd,cseq,200,NULL,NULL);goto done;}
    else result=reply(fd,cseq,501,NULL,NULL);
    if(result||camera_rtsp_input_retire(&input,n,milliseconds()))goto done;
    uint64_t accepted=milliseconds();if(accepted==UINT64_MAX)goto done;
    if(method!=CAMERA_RTSP_METHOD_UNSUPPORTED)touched=accepted;
   }
  }
   checked_now=milliseconds();if(camera_rtsp_input_tick(&input,checked_now))goto done;
   if(playing&&checked_now>=next){if(hardware_h264?send_h264_frame(source,fd,channel,&sequence):send_frame(source,fd,channel,&sequence)){fprintf(stderr,"RTSP FRAME FAILURE errno=%d; closesessionwithoutsoftwarefallback\n",errno);break;}uint64_t scheduled=milliseconds();if(scheduled==UINT64_MAX)goto done;next=scheduled+interval_ms;}
 }
 done:
 if(input.failure)fprintf(stderr,"RTSP LEGACY INPUT code=%u\n",(unsigned)input.failure);
 camera_rtsp_input_clear(&input);
 /* No fresh clock means no invented detach/idle evidence and no END retry. */
 if(source_clock.failed||input.failure==CAMERA_RTSP_INPUT_CLOCK){
  source->retirement_uncertain=1;service->terminal_result=CAMERA_IO_ERROR;
 }else service->terminal_result=idle_h264_checked(source);
 if(service->terminal_result!=CAMERA_OK){
  source->retirement_uncertain=1;
  fprintf(stderr,"RTSP legacy retirement failed=%d sensor_fd=%d frame_fd=%d encoder_fd=%d; halt admission, retain ownership; process exit is NOT proven retirement\n",
   service->terminal_result,source->continuous_sensor,source->continuous_frame,source->h264_stream_fd);
 }
 close(fd);return service->terminal_result;
}
static int serve_legacy_clients(struct qc_legacy_service *service,int listener)
{
 while(!stop&&service->terminal_result==CAMERA_OK){
  struct pollfd p={.fd=listener,.events=POLLIN};
  if(poll(&p,1,500)>0){
   int fd=accept(listener,NULL,NULL);
   if(fd>=0){int r=client_session(service,fd);if(r!=CAMERA_OK){service->terminal_result=r;break;}}
  }
 }
 return service->terminal_result;
}
int main(int argc,char **argv)
{
 unsigned port=8554;const char *bind_address="0.0.0.0";
 for(int i=1;i<argc;i++){
  if(!strcmp(argv[i],"--fixture")&&i+2<argc){fixture=argv[++i];port=strtoul(argv[++i],NULL,10);bind_address="127.0.0.1";}
  else if(!strcmp(argv[i],"--size")&&i+1<argc){if(sscanf(argv[++i],"%ux%u",&width,&height)!=2)return 2;}
  else if(!strcmp(argv[i],"--quality")&&i+1<argc)quality=strtoul(argv[++i],NULL,10);
   else if(!strcmp(argv[i],"--interval-ms")&&i+1<argc)interval_ms=strtoul(argv[++i],NULL,10);
   else if(!strcmp(argv[i],"--resource-uri")&&i+1<argc)resource_uri=argv[++i];
   else if(!strcmp(argv[i],"--request-deadline-ms")&&i+1<argc){
    char *end;unsigned long value=strtoul(argv[++i],&end,10);
    if(!argv[i][0]||*end||value<500||value>5000)return 2;request_deadline_ms=(unsigned)value;
   }
   else if(!strcmp(argv[i],"--hardware-jpeg"))hardware_jpeg=1;
   else if(!strcmp(argv[i],"--hardware-h264"))hardware_h264=1;
   else if(!strcmp(argv[i],"--h264-p-frames"))h264_p_frames=1;
   else if(!strcmp(argv[i],"--continuous-capture"))continuous_capture=1;
   else if(!strcmp(argv[i],"--capture-irq"))capture_irq=1;
    else if(!strcmp(argv[i],"--fanout"))fanout=1;
    else if(!strcmp(argv[i],"--tcp-coalesce"))tcp_coalesce=1;
    else if(!strcmp(argv[i],"--trace-every")&&i+1<argc){
     char *end;unsigned long value=strtoul(argv[++i],&end,10);
     if(!argv[i][0]||*end||value<1||value>1000)return 2;trace_every=(unsigned)value;
    }
    else if(!strcmp(argv[i],"--send-deadline-ms")&&i+1<argc){
     char *end;unsigned long value=strtoul(argv[++i],&end,10);
     if(!argv[i][0]||*end||value<10||value>2000)return 2;send_deadline_ms=(unsigned)value;
    }
  else {fprintf(stderr,"Usage: rtspd --resource-uri PUBLIC-RTSP-URI [--request-deadline-ms 500..5000] [--size 1280x720|640x360|320x176] [--quality 1..95] [--interval-ms 0..5000] [--hardware-jpeg] [--fixture RAW-FRAME HOST-PORT]\n");return 2;}
 }
 if(!((width==1280&&height==720)||(width==640&&height==360)||(width==320&&height==176))||quality<1||quality>95||interval_ms>5000)return 2;
 if(port<1024||port>65535)return 2;
 if(hardware_jpeg&&(width!=1280||height!=720||fixture))return 2;
 if(hardware_h264&&(hardware_jpeg||width!=1280||height!=720||fixture))return 2;
 if(h264_p_frames&&!hardware_h264)return 2;
 if(continuous_capture&&(fixture||width!=1280||height!=720))return 2;
 if(capture_irq&&!continuous_capture)return 2;
  if(fanout&&!hardware_h264)return 2;
  if((tcp_coalesce||send_deadline_ms!=500)&&!fanout)return 2;
  if(trace_every!=1&&!fanout)return 2;
 char track_uri[CAMERA_RTSP_REQUEST_MAX_TARGET+1U];
 if(!resource_uri)return 2;
 int registered=snprintf(track_uri,sizeof(track_uri),"%s/track1",resource_uri);
 if(registered<0||(size_t)registered>=sizeof(track_uri)||camera_rtsp_registry_init(&resources,resource_uri,track_uri))return 2;
 signal(SIGINT,on_signal);signal(SIGTERM,on_signal);signal(SIGPIPE,SIG_IGN);
 if(fanout){
  uint8_t params[256];size_t n=qc_h264_parameter_sets(params);
  const char *description="v=0\r\no=- 1 1 IN IP4 0.0.0.0\r\ns=Camera hardware H264 shared LAB producer\r\nt=0 0\r\na=control:*\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1;profile-level-id=42e01f\r\na=control:track1\r\n";
  /* Match the verified hardware output limit. Serialized network sends release
   * every consumer before the next publish, so one slot bounds storage at1MiB
   * rather than reserving four worst-case frames. No clipping/software fallback.
   */
  size_t capacity=0x100000;uint8_t *storage=malloc(capacity);if(!storage)return 1;
  struct qc_rtsp_context source;source_context_init(&source);
  struct camera_producer lifecycle;
  struct camera_producer_config config={.operation_timeout_ns=10000000000ULL,.keyframe_supported=1,.event=producer_event,.event_context=&source};
  if(camera_producer_init(&lifecycle,&config)!=CAMERA_OK){free(storage);return 1;}
  struct camera_rtsp_common_producer bridge={.producer=&lifecycle,.context=&source,.start=start_h264_producer,.idle=idle_h264_checked,.clock_ns=producer_clock_ns};
  struct camera_rtsp_lifecycle hooks=camera_rtsp_common_hooks(&bridge);hooks.failure=producer_failure;
  struct camera_rtsp_producer producer={.context=&source,.next=produce_h264_frame,.release=release_h264_frame,.idle=idle_h264_producer,.parameter_sets=params,.parameter_set_bytes=n,.idle_checked=idle_h264_checked,.lifecycle=&hooks};
  /* Actual a15 target sends occasionally took373ms; host-only100ms budget
   * cannot be assumed valid on the MTU384 radio. Explicit bounded500ms lab.
   */
   struct camera_rtsp_options options={.bind_address=bind_address,.sdp=description,.port=port,.interval_ms=interval_ms,.send_deadline_ms=send_deadline_ms,.hub_storage=storage,.packet_capacity=capacity,.hub_slots=1,.tcp_coalesce=tcp_coalesce,.trace_every=trace_every,.stop=&stop,.resources=&resources,.request_deadline_ms=request_deadline_ms};
  int r=camera_rtsp_serve(&options,&producer);
  /* This lab main owns the whole context (no recorder/UI demands). Successful
   * serve cleanup needs no second idle, only logical admission shutdown.
   * Never turn an error return into an implicit retirement/recovery retry.
   */
  if(r==CAMERA_OK){
   uint64_t now=producer_clock_ns(&source);
   r=now==UINT64_MAX?CAMERA_IO_ERROR:camera_producer_shutdown(&lifecycle,now);
   if(r==CAMERA_OK&&!camera_producer_quiescent(&lifecycle))r=CAMERA_NOT_READY;
  }
  if(r!=CAMERA_OK)fprintf(stderr,"RTSP lifecycle result=%d; no successful shutdown/retirement claim\n",r);
  free(storage);return r==CAMERA_OK?0:1;
 }
 int listener=socket(AF_INET,SOCK_STREAM,0),one=1;if(listener<0)return 1;
 setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
 struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(port)};inet_pton(AF_INET,bind_address,&address.sin_addr);
 if(bind(listener,(struct sockaddr *)&address,sizeof(address))||listen(listener,1)){perror("RTSP listener");close(listener);return 1;}
 snprintf(sdp,sizeof(sdp),"v=0\r\no=- 1 1 IN IP4 0.0.0.0\r\ns=QC07F replacement capture %ux%u\r\nt=0 0\r\na=control:*\r\nm=video 0 RTP/AVP 26\r\na=rtpmap:26 JPEG/90000\r\na=control:track1\r\na=framesize:26 %u-%u\r\n",width,height,width,height);
 if(hardware_h264)snprintf(sdp,sizeof(sdp),"v=0\r\no=- 1 1 IN IP4 0.0.0.0\r\ns=QC07F hardwareH264 independentIDR\r\nt=0 0\r\na=control:*\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1;profile-level-id=42e01f\r\na=control:track1\r\n");
 fprintf(stderr,"RTSP TCP %ux%u quality%u interval_ms%u %s encoder=%s; sensor must be initialized first\n",width,height,quality,interval_ms,fixture?"FIXTURE-NOT-LIVE":"BOUNDED CAMERA CAPTURE",hardware_h264?"HARDWARE-H264-IDR-NO-FALLBACK":hardware_jpeg?"HARDWARE-JPEG-NO-FALLBACK":"SOFTWARE-JPEG");
 struct qc_legacy_service service;source_context_init(&service.source);service.terminal_result=CAMERA_OK;
 int result=serve_legacy_clients(&service,listener);
 close(listener);free(hw_header);
 /* Terminal lab policy: do not admit/reinitialize another owner or silently
  * retry END. Nonzero exit asks the parent for qualified explicit recovery.
  */
 return result==CAMERA_OK?0:1;
}
