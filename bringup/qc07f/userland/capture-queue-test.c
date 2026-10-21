/* SPDX-License-Identifier: GPL-2.0-only
 * Experimental persistent copied-frame queue qualification, NOT vb2/DMA-BUF.
 * Validate every delivery, exact token return, busy/stale rejection and clean
 * stop/reopen. Known SC1346 only; no flash/SD/GPIO/illumination operation.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <stdint.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "crc-bzip2.h"
static uint8_t pixels[1728000];
static int regread(int fd,uint16_t reg)
{uint8_t p[2]={reg>>8,reg},v=0;struct i2c_msg m[2]={{.addr=0x30,.len=2,.buf=p},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&v}};struct i2c_rdwr_ioctl_data q={m,2};return ioctl(fd,I2C_RDWR,&q)==2?v:-1;}
static int sensor_stream(int fd,uint8_t on)
{uint8_t p[3]={1,0,on};return write(fd,p,3)==3&&regread(fd,0x0100)==on?0:-1;}
static int expected(int fd,unsigned long cmd,struct qc_isp_queue_frame *q,int error)
{errno=0;int ret=ioctl(fd,cmd,q),actual=errno;if(ret==-1&&actual==error)return 0;fprintf(stderr,"QUEUE NEGATIVE mismatch cmd=%lx ret=%d errno=%d expected=%d\n",cmd,ret,actual,error);return -1;}
static uint64_t monotonic_ms(void)
{struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return UINT64_MAX;return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
static int delay_ms(unsigned milliseconds)
{
 uint64_t start=monotonic_ms();if(start==UINT64_MAX)return -1;
 for(;;){uint64_t now=monotonic_ms();if(now==UINT64_MAX)return -1;if(now-start>=milliseconds)return 0;
  int r=poll(NULL,0,(int)(milliseconds-(now-start)));if(r<0&&errno==EINTR)continue;if(r<0)return -1;}
}
int main(int argc,char **argv)
{
 int noinput=argc==2&&!strcmp(argv[1],"--noinput");
 int close_held=argc==2&&!strcmp(argv[1],"--close-held");
 int expire_held=argc==2&&!strcmp(argv[1],"--expire-held");
 const char *dump=argc==3&&!strcmp(argv[1],"--dump")?argv[2]:NULL;
 if(argc!=1&&!noinput&&!close_held&&!expire_held&&!dump)return 2;
 if(dump&&(strncmp(dump,"/run/continuous-queue",21)||strstr(dump,"..")))return 2;
 int sensor=-1,frame=-1,guard=-1,result=1,timeout=5,begun=0,out=-1;
 uint64_t previous=0;struct qc_isp_queue_frame last={0};
 sensor=open("/dev/i2c-0",O_RDWR);if(sensor<0||ioctl(sensor,I2C_SLAVE,0x30)||regread(sensor,0x3107)!=0xda||regread(sensor,0x3108)!=0x4d||regread(sensor,0x3208)!=5||regread(sensor,0x3209)!=0||regread(sensor,0x320a)!=2||regread(sensor,0x320b)!=0xd0)goto done;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 frame=open("/dev/qc07f-frame",O_RDONLY);if(frame<0)goto done;
 if(noinput){
  if(sensor_stream(sensor,0)||ioctl(frame,QC_ISP_QUEUE_BEGIN))goto done;begun=1;
  struct qc_isp_queue_frame q={.bytes=sizeof(pixels),.user_ptr=(uint32_t)(uintptr_t)pixels};
  if(expected(frame,QC_ISP_QUEUE_NEXT,&q,ETIMEDOUT))goto done;
  puts("QUEUE NOINPUT expectedETIMEDOUT; no accepted pixels; NOTV4L2");begun=0;result=0;goto done;
 }
 for(unsigned session=0;session<3;session++){
  if(ioctl(frame,QC_ISP_QUEUE_BEGIN)||sensor_stream(sensor,1))goto done;begun=1;
  for(unsigned n=0;n<16;n++){
   struct qc_isp_queue_frame q={.bytes=sizeof(pixels),.user_ptr=(uint32_t)(uintptr_t)pixels};
   if(ioctl(frame,QC_ISP_QUEUE_NEXT,&q)){perror("QUEUENEXT");goto done;}
   uint32_t delivered=qc_crc_bzip2(pixels,sizeof(pixels));
   if(!q.token||q.token<=previous||q.slot>=4||q.sequence!=n+1||!(q.status&1)||q.guard_errors||delivered!=q.crc32_bzip2)goto done;
   if(close_held){
    if(sensor_stream(sensor,0)||close(frame))goto done;frame=-1;begun=0;
    puts("QUEUE CLOSE-HELD validcopiedframe thenimplicitDMAstop; post-close mask/memory mustbeobserved separately");result=0;goto done;
   }
   if(expire_held){
    uint64_t before=monotonic_ms();if(before==UINT64_MAX)goto done;
    for(unsigned tick=0;tick<11;tick++){
     if(delay_ms(1000)||write(guard,".",1)!=1)goto done;
    }
    uint64_t after=monotonic_ms();if(after==UINT64_MAX||after-before<11000)goto done;
    printf("QUEUE EXPIRY elapsed_ms=%llu token=%llu poll_delay_verified=1\n",(unsigned long long)(after-before),(unsigned long long)q.token);
    if(expected(frame,QC_ISP_QUEUE_RETURN,&q,ETIMEDOUT))goto done;
    puts("QUEUE EXPIRED-HELD expectedETIMEDOUT and token invalidated; watchdogkeptalive, noframepublished");
    begun=0;result=0;goto done;
   }
   struct qc_isp_queue_frame busy={.bytes=sizeof(pixels),.user_ptr=(uint32_t)(uintptr_t)pixels},bad=q;
   if(expected(frame,QC_ISP_QUEUE_NEXT,&busy,EBUSY))goto done;
   bad.token++;if(expected(frame,QC_ISP_QUEUE_RETURN,&bad,ESTALE))goto done;
   if(delay_ms(100))goto done;
   if(ioctl(frame,QC_ISP_QUEUE_RETURN,&q))goto done;
   if(expected(frame,QC_ISP_QUEUE_RETURN,&q,ESTALE))goto done;
   printf("QUEUE FRAME session=%u seq=%u slot=%u token=%llu guards=%u crc=%08x events=%u dropped=%u validdelivery=1\n",session,q.sequence,q.slot,(unsigned long long)q.token,q.guard_errors,delivered,q.irq_events,q.dropped);
   previous=q.token;last=q;if(write(guard,".",1)!=1)goto done;
  }
  if(sensor_stream(sensor,0)||ioctl(frame,QC_ISP_QUEUE_END))goto done;begun=0;
  if(expected(frame,QC_ISP_QUEUE_RETURN,&last,EPERM))goto done;
  puts("QUEUE SESSION ended; stale token afterstop rejected; pool DMA retired");
 }
 if(dump){
  out=open(dump,O_WRONLY|O_CREAT|O_EXCL,0600);if(out<0)goto done;
  size_t offset=0;while(offset<sizeof(pixels)){ssize_t n=write(out,pixels+offset,sizeof(pixels)-offset);if(n<0&&errno==EINTR)continue;if(n<=0)goto done;offset+=n;}
  printf("QUEUE DUMP bytes=%zu last_crc=%08x; copiedframepreservedaftervalidatedRETURN/END\n",sizeof(pixels),last.crc32_bzip2);
 }
 puts("QUEUE48 deliveries/busy/stale/duplicate/stop/reopen PASS; copied frames, NOTvb2/DMA-BUF/encoder-overlap");result=0;
done:
 if(out>=0)close(out);
 if(sensor>=0){if(sensor_stream(sensor,0))result=1;close(sensor);}
 if(frame>=0){if(begun)ioctl(frame,QC_ISP_QUEUE_END);close(frame);}
 if(guard>=0){write(guard,"V",1);close(guard);}return result;
}
