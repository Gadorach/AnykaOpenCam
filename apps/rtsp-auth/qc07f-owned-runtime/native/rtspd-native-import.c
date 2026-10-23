/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <camera/backend.h>
#include <camera/rtsp_clock.h>
#include "qc07f_hwenc.h"
#include "h264-baseline.h"
#include "isp-stream.h"
#include "capture-receipt.h"
#include "crc-bzip2.h"
#include "sensor_baseline.h"
#include "watchdog_owner.h"
#include "owned-provider-types.h"
#define MAIN_PIXELS (1280U * 720U)
#define FRAME_BYTES 1728000U
static volatile sig_atomic_t stop;
static uint8_t raw[FRAME_BYTES];
static unsigned width=1280,height=720;
static int h264_p_frames=1,capture_irq=0;
static unsigned trace_every=1;
static unsigned long frame_count;
static unsigned sensor_baseline_requested;
static struct nh_sensor_report sensor_baseline_report;
/* SPDX-License-Identifier: GPL-2.0-only; exact selected rtspd functions */
struct qc_rtsp_context {
 int h264_stream_fd,continuous_sensor,continuous_frame,retirement_uncertain;
 struct qw_owner watchdog;int encoder_hold,legacy_control;
 struct qp_roles providers;
 int last_close_attempt;unsigned close_unknown;
 struct qc_capture_receipt receipt_key,receipt_evidence;
 uint64_t last_source_instance,last_file_instance,last_capture_generation;
 unsigned receipt_armed,capture_sequence,ownership_idr,ownership_idrs,receipt_renewals;
 unsigned h264_stream_frames;uint8_t *produced_storage;uint8_t *resident_storage;unsigned raw_valid;uint32_t produced_nal_crc32_bzip2;uint64_t producer_begin_ns;
 struct {uint64_t begin,captured,encoded;unsigned bytes;int keyframe;} produced_timing;
};
static void source_context_init(struct qc_rtsp_context *c)
{memset(c,0,sizeof(*c));c->h264_stream_fd=c->continuous_sensor=c->continuous_frame=c->encoder_hold=c->legacy_control=-1;c->last_close_attempt=-1;c->watchdog=(struct qw_owner)QW_OWNER_EMPTY;c->providers=(struct qp_roles)QP_ROLES_EMPTY;}
static void on_signal(int sig) { (void)sig; stop = 1; }
static struct camera_rtsp_clock source_clock;
static uint64_t milliseconds(void)
{return camera_rtsp_clock_ms(&source_clock);}
static int sensor_read(int fd,uint16_t reg){return nh_sensor_read_byte(fd,reg);}
static int sensor_stream(int fd,uint8_t on){return nh_sensor_stream(fd,on);}
static void provider_hold(struct qc_rtsp_context *);
static int capture_failed(struct qc_rtsp_context *c)
{c->retirement_uncertain=1;qw_owner_fail(&c->watchdog);provider_hold(c);return CAMERA_IO_ERROR;}
#include "provider-observe.inc"
#include "source-close-once.inc"
static int continuous_retire(struct qc_rtsp_context *c,unsigned expiry_only)
{
 struct qc_capture_receipt proof;
 if(c->retirement_uncertain)return CAMERA_IO_ERROR; /* never retry uncertain END/ACK/close */
 if(c->produced_storage)return CAMERA_BUSY; /* a held AU is not an idle boundary */
 if(c->continuous_frame<0)return c->continuous_sensor<0?CAMERA_OK:capture_failed(c);
 if(!c->receipt_armed||provider_receipt_query(c,&c->receipt_key,&proof))return capture_failed(c);
 c->receipt_evidence=proof;
 if(expiry_only){
  if(!qc_receipt_retired(&proof)||proof.reason!=QC_STOP_EXPIRY)return capture_failed(c);
 }else if(proof.state==QC_RECEIPT_ACTIVE){
  if(proof.reason!=QC_STOP_NONE||proof.stop_result||proof.stopped_ns)return capture_failed(c);
 }else return capture_failed(c); /* finite normal stop requires ACTIVE -> END */
 /* Do not stop somebody else's sensor on a bad/stale/missing proof. Existing
  * valid-owner stop order remains sensor-off then explicit ISP END if ACTIVE.
  */
 if(c->continuous_sensor<0||sensor_stream(c->continuous_sensor,0))return capture_failed(c);
 if(proof.state==QC_RECEIPT_ACTIVE){
  if(proof.reason!=QC_STOP_NONE||proof.stop_result||proof.stopped_ns||ioctl(c->continuous_frame,QC_ISP_END))return capture_failed(c);
  if(provider_receipt_query(c,&c->receipt_key,&proof))return capture_failed(c);
  c->receipt_evidence=proof;
  if(!qc_receipt_retired(&proof)||proof.reason!=QC_STOP_END)return capture_failed(c);
 }
 if(!qc_receipt_retired(&proof))return capture_failed(c);
 struct qc_capture_receipt ack=qc_receipt_request(&c->receipt_key);
 if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_ACK,&ack))return capture_failed(c);
 c->receipt_armed=0; /* ACK is the admission barrier, not close(fd)/ownerNULL. */
 if(source_close_once(c,&c->continuous_sensor))return CAMERA_IO_ERROR;
 if(source_close_once(c,&c->continuous_frame))return CAMERA_IO_ERROR;
 /* At this serialized boundary no produced packet is held. A fresh reference
  * owner must not consume a P frame based on the preceding capture generation.
  */
 if(c->produced_storage)return capture_failed(c);
 if(c->h264_stream_fd>=0){
  if(provider_encoder_observe(c)||source_close_once(c,&c->h264_stream_fd))return CAMERA_IO_ERROR;
 }
 c->providers.phase=QP_CLOSED;
 c->h264_stream_frames=0;c->ownership_idr=1;
 return CAMERA_OK;
}
static int continuous_close_checked(struct qc_rtsp_context *c)
{return continuous_retire(c,0);}
static int continuous_next(struct qc_rtsp_context *c)
{
 int result=-1;
 if(c->retirement_uncertain||!provider_shape(c)||qw_owner_check(&c->watchdog))return capture_failed(c);
 for(unsigned attempt=0;attempt<1;attempt++){
   if(c->providers.phase==QP_COMMITTED){ /* preloaded FD does NOT skip activation */
    if(provider_observe(c)||qw_owner_check(&c->watchdog))goto done;
     struct qc_capture_receipt bound=qc_receipt_request(NULL);
     /* Strict copied adapter: ENOTTY is unsupported, not guessed legacy proof.
      * This binary is not compatible with the current target73 receipt-less ABI.
      */
     if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_BIND,&bound)||provider_store_frame(c,&bound))goto done;
     if(!qc_receipt_header(&bound)||bound.generation||bound.state!=QC_RECEIPT_NONE||bound.reason||bound.stop_result||bound.stopped_ns||
        (c->last_source_instance&&(bound.source_instance!=c->last_source_instance||bound.file_instance<=c->last_file_instance)))goto done;
     /* Selector/register observations are effects ONLY under actual guard,
      * after all adopted identities/global lifetime and fresh BIND. */
     if(qw_owner_check(&c->watchdog)||ioctl(c->continuous_sensor,I2C_SLAVE,0x30)||
        sensor_read(c->continuous_sensor,0x3107)!=0xda||sensor_read(c->continuous_sensor,0x3108)!=0x4d||
        sensor_read(c->continuous_sensor,0x0100)!=0)goto done;
     c->last_source_instance=bound.source_instance;c->last_file_instance=bound.file_instance;
     c->receipt_key=qc_receipt_request(&bound);c->receipt_evidence=bound;
     struct qc_capture_receipt armed=qc_receipt_request(&bound);
     if(qw_owner_check(&c->watchdog)||ioctl(c->continuous_frame,QC_ISP_RECEIPT_ARM,&armed)||provider_store_frame(c,&armed))goto done;
     if(!qc_receipt_header(&armed)||armed.source_instance!=bound.source_instance||armed.file_instance!=bound.file_instance||
        !armed.generation||armed.generation<=c->last_capture_generation||armed.state!=QC_RECEIPT_ARMED||armed.reason||armed.stop_result||armed.stopped_ns)goto done;
     c->receipt_key=qc_receipt_request(&armed);c->receipt_evidence=armed;c->receipt_armed=1;
     c->last_capture_generation=armed.generation;c->capture_sequence=0;c->ownership_idr=1;
     struct qc_capture_receipt reserved;
     if(provider_receipt_query(c,&c->receipt_key,&reserved)||reserved.state!=QC_RECEIPT_ARMED||reserved.reason||reserved.stop_result||reserved.stopped_ns)goto done;
     c->receipt_evidence=reserved; /* prove exact reservation BEFORE table writes */
     if(!sensor_baseline_requested||nh_sensor_exact139(c->continuous_sensor,c->watchdog.fd,c->producer_begin_ns,&source_clock,&stop,&sensor_baseline_report))goto done;
     if(provider_receipt_query(c,&c->receipt_key,&reserved)||reserved.state!=QC_RECEIPT_ARMED||reserved.reason||reserved.stop_result||reserved.stopped_ns)goto done;
     c->receipt_evidence=reserved; /* exact reservation still ARMED after finite preparation */
     if(ioctl(c->continuous_frame,capture_irq?QC_ISP_BEGIN_IRQ:QC_ISP_BEGIN))goto done;
     struct qc_capture_receipt active;
     if(provider_receipt_query(c,&c->receipt_key,&active)||active.state!=QC_RECEIPT_ACTIVE||active.reason||active.stop_result||active.stopped_ns)goto done;
     c->receipt_evidence=active;
     if(sensor_stream(c->continuous_sensor,1))goto done;
     c->providers.phase=QP_ACTIVE;
  }
  struct qc_isp_next q={.bytes=sizeof(raw),.user_ptr=(uint32_t)(uintptr_t)raw};
   if(!ioctl(c->continuous_frame,QC_ISP_NEXT,&q)){
    if(q.bytes!=sizeof(raw)||q.user_ptr!=(uint32_t)(uintptr_t)raw||q.guard_errors||!(q.status&1)||q.sequence!=c->capture_sequence+1||q.sequence>128)goto done;
   uint64_t raw_crc_begin=camera_rtsp_clock_ns(&source_clock);
    uint32_t delivered=qc_crc_bzip2(raw,sizeof(raw));
    uint64_t raw_crc_end=camera_rtsp_clock_ns(&source_clock);
    if(raw_crc_begin==UINT64_MAX||raw_crc_end==UINT64_MAX)goto done;
    fprintf(stderr,"NH_RAW_CRC sequence=%u bytes=%zu begin_ns=%llu end_ns=%llu crc_bzip2=%08x\n",q.sequence,sizeof(raw),(unsigned long long)raw_crc_begin,(unsigned long long)raw_crc_end,delivered);
   if(delivered!=q.crc32_bzip2){
    fprintf(stderr,"ISP CRC MISMATCH target=%08x delivered=%08x; REFUSE liveframe attempt=%u\n",q.crc32_bzip2,delivered,attempt);
      goto done; /* data corruption is terminal; never disguised as expiry */
   }
   if(!(frame_count%trace_every))fprintf(stderr,"ISP GATED-CONTINUOUS seq=%u status=%08x guards=%u crc_bzip2=%08x\n",q.sequence,q.status,q.guard_errors,q.crc32_bzip2);
    struct qc_capture_receipt active;
    if(provider_receipt_query(c,&c->receipt_key,&active)||active.state!=QC_RECEIPT_ACTIVE||active.reason||active.stop_result||active.stopped_ns)goto done;
    c->receipt_evidence=active;c->capture_sequence=q.sequence;c->raw_valid=1;
    /* Full extent/status/CRC + keyed ACTIVE receipt succeeded BEFORE this sole ping. */
    if(qw_owner_capture_progress(&c->watchdog))goto done;result=0;break;
  }
  int failure=errno;
   fprintf(stderr,"ISP NEXT REJECT errno=%d attempt=%u; noframeencoded, receipt-required\n",failure,attempt);
   (void)failure;goto done; /* finite candidate: no renewal/retry */
 }
done:
  if(result)capture_failed(c);
  if(!result&&qw_owner_check(&c->watchdog))result=capture_failed(c);
  return result;
}
static int prepare_output(struct qc_rtsp_context *,uint64_t);
static int before_encode_gate(uint64_t);
static int produce_h264_frame(void *context,uint32_t force_keyframe,struct camera_packet *packet)
{
   struct qc_rtsp_context *c=context;if(!c)return CAMERA_INVALID;
   if(!provider_shape(c))return CAMERA_NOT_READY;
  uint64_t begin=milliseconds(),captured,encoded;
   uint8_t *output=NULL;
  int result=CAMERA_IO_ERROR;
   if(c->produced_storage)return CAMERA_BUSY; c->raw_valid=0; /* no stale RAW view; next complete capture required */
   if(c->retirement_uncertain)return CAMERA_IO_ERROR;
   if(begin==UINT64_MAX){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}c->producer_begin_ns=c->watchdog.start_ns; /* include outer lifetime/admission cost */
   if(qw_owner_check(&c->watchdog))return capture_failed(c);
   if(prepare_output(c,c->producer_begin_ns)!=CAMERA_OK)return CAMERA_NOT_READY;
   if(continuous_next(c)){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}captured=milliseconds();
   if(captured==UINT64_MAX){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}
  output=c->resident_storage;if(!output)return CAMERA_NOT_READY; /* one prefaulted persistent CPU buffer */
 if(before_encode_gate(c->producer_begin_ns)!=CAMERA_OK)return CAMERA_NOT_READY; /* no encode/DMA allocation on forecast failure; raw remains owned */
  if(qw_owner_check(&c->watchdog))return capture_failed(c);
  if(!provider_shape(c)||provider_encoder_observe(c))goto done;
 struct qc_hwenc_h264_job job={.width=width,.height=height,.input_bytes=MAIN_PIXELS*3/2,.input_ptr=(uint32_t)(uintptr_t)raw,.output_capacity=0x100000,.output_ptr=(uint32_t)(uintptr_t)output};
   if(!c->raw_valid)goto done;int ownership_keyframe=c->ownership_idr;
   int keyframe=force_keyframe||ownership_keyframe||!h264_p_frames||!(c->h264_stream_frames%30);
   if(h264_p_frames==1){
  struct qc_hwenc_h264_stream_job q={.frame=job,.keyframe=keyframe};
   c->raw_valid=0; /* output may overwrite raw even on partial EFAULT; never claim raw after encode */
    if(ioctl(c->h264_stream_fd,QC_HWENC_H264_STREAM,&q))goto done;
    if(q.frame_num>65535)goto done;
    c->providers.codec_frame_num=q.frame_num;job=q.frame;
 }else goto done; /* temporary IDR-only role deliberately unsupported */
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
   uint64_t nal_crc_begin=camera_rtsp_clock_ns(&source_clock);
    uint32_t nal_crc=qc_crc_bzip2(packet->data,packet->bytes);
    uint64_t nal_crc_end=camera_rtsp_clock_ns(&source_clock);
    if(nal_crc_begin==UINT64_MAX||nal_crc_end==UINT64_MAX)goto done;
    c->produced_nal_crc32_bzip2=nal_crc; /* exact existing full-NAL scalar evidence */
    fprintf(stderr,"NH_NAL_CRC sequence=%u bytes=%zu begin_ns=%llu end_ns=%llu crc_bzip2=%08x\n",c->capture_sequence,packet->bytes,(unsigned long long)nal_crc_begin,(unsigned long long)nal_crc_end,nal_crc);
   if(!(frame_count%trace_every))fprintf(stderr,"RTSP SOURCE timestamp=%u nal_bytes=%zu crc_bzip2=%08x keyframe=%u\n",(uint32_t)(captured*90),packet->bytes,nal_crc,packet->keyframe);
   if(keyframe&&c->receipt_armed)fprintf(stderr,"RTSP CAPTURE KEY source=%llu file=%llu generation=%llu IDR=%s LAB-NOT-AUTH\n",
    (unsigned long long)c->receipt_key.source_instance,(unsigned long long)c->receipt_key.file_instance,(unsigned long long)c->receipt_key.generation,
    ownership_keyframe?"ownership":force_keyframe?"demand":!h264_p_frames?"independent":"max-GOP30");
   c->produced_storage=output;output=NULL;
   c->produced_timing.begin=begin;c->produced_timing.captured=captured;c->produced_timing.encoded=encoded;
   c->produced_timing.bytes=job.output_bytes;c->produced_timing.keyframe=keyframe;
   frame_count++;if(keyframe)c->h264_stream_frames=0;c->h264_stream_frames++;
   if(ownership_keyframe&&keyframe){c->ownership_idr=0;c->ownership_idrs++;}
   if(!((frame_count-1)%trace_every))fprintf(stderr,"RTSP PRODUCED n=%lu width=%u height=%u h264_bytes=%u capture_ms=%llu encode_ms=%llu NATIVE-CANDIDATE encoder=%s nal=%s\n",frame_count,width,height,job.output_bytes,(unsigned long long)(captured-begin),(unsigned long long)(encoded-captured),h264_p_frames?"HARDWARE-H264-IP":"HARDWARE-H264-IDR",keyframe?"IDR":"P");
 result=0;
done:
   /* Selected STREAM used only the adopted persistent description.
    * No unused temporary open, alias or per-frame close/session claim. */
   if(result!=CAMERA_OK)c->retirement_uncertain=1;
   if(result==CAMERA_OK&&qw_owner_check(&c->watchdog))result=CAMERA_IO_ERROR;
   if(result!=CAMERA_OK&&output){c->produced_storage=output;output=NULL;}
    (void)output; /* persistent CPU backing survives loan release and all uncertainty */
   /* Error returns do not establish reset/DMA retirement on this diagnostic
    * ABI. Do not convert a descriptor-less failed transaction into idle OK.
    * Parent-owned, qualified recovery must resolve this uncertainty explicitly.
    */
   if(result!=CAMERA_OK){
    c->retirement_uncertain=1;
         /* Retain uncertain CPU backing and all descriptors. */ memset(packet,0,sizeof(*packet));
   }
   return result;
}
static void release_h264_frame(void *context,const struct camera_packet *packet)
{struct qc_rtsp_context *c=context;(void)packet;c->produced_storage=NULL; /* retire CPU loan, keep charged backing until proven source stop */}
static int idle_h264_checked(void *context)
{
 struct qc_rtsp_context *c=context;
  if(!c||c->produced_storage)return CAMERA_BUSY;
 if(source_clock.failed){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}
 int r=continuous_close_checked(c);if(r)return r;
 if(c->h264_stream_fd>=0&&source_close_once(c,&c->h264_stream_fd))return CAMERA_IO_ERROR;
 if(c->retirement_uncertain)return CAMERA_IO_ERROR;
 c->h264_stream_frames=0;return CAMERA_OK;
}
static uint64_t producer_clock_ns(void *context)
{(void)context;return camera_rtsp_clock_ns(&source_clock);}
