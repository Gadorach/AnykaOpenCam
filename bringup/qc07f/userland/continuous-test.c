/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded8-frame gated continuous-input qualification. Not free-runningvb2.
 * Sensor0100 stayson between frames. Kernel must own exclusivegatedDMA,
 * report guards/CRC and quiesce busy14 before each delivered complete frame.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "crc-bzip2.h"
static uint8_t pixels[1728000];
static uint64_t ms(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
static int sensor_read(int fd,uint16_t reg)
{
 uint8_t p[2]={reg>>8,reg},v=0;struct i2c_msg m[2]={{.addr=0x30,.len=2,.buf=p},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&v}};
 struct i2c_rdwr_ioctl_data q={.msgs=m,.nmsgs=2};return ioctl(fd,I2C_RDWR,&q)==2?v:-1;
}
static int sensor_stream(int fd,uint8_t on){uint8_t b[3]={1,0,on};return write(fd,b,3)==3&&sensor_read(fd,0x0100)==on?0:-1;}
int main(int argc,char **argv)
{
 int metadata_only=0,irq_mode=0;
 if(argc<2||argc>4||strncmp(argv[1],"/run/continuous-",16)||strstr(argv[1],".."))return 2;
 for(int i=2;i<argc;i++){if(!strcmp(argv[i],"--metadata-only"))metadata_only=1;else if(!strcmp(argv[i],"--irq"))irq_mode=1;else return 2;}
 int sensor=-1,frame=-1,guard=-1,result=1,begun=0,timeout=5;
 sensor=open("/dev/i2c-0",O_RDWR);if(sensor<0||ioctl(sensor,I2C_SLAVE,0x30))goto done;
 if(sensor_read(sensor,0x3107)!=0xda||sensor_read(sensor,0x3108)!=0x4d||sensor_read(sensor,0x3208)!=5||sensor_read(sensor,0x3209)||sensor_read(sensor,0x320a)!=2||sensor_read(sensor,0x320b)!=0xd0)goto done;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 frame=open("/dev/qc07f-frame",O_RDONLY);if(frame<0)goto done;
 if(ioctl(frame,irq_mode?QC_ISP_BEGIN_IRQ:QC_ISP_BEGIN)){perror("continuousBEGIN");goto done;}begun=1;
 if(sensor_stream(sensor,1))goto done;
 uint64_t start=ms();
 for(unsigned i=0;i<8;i++){
  struct qc_isp_next q={.bytes=sizeof(pixels),.user_ptr=(uint32_t)(uintptr_t)pixels};uint64_t before=ms();
  if(write(guard,".",1)!=1||ioctl(frame,QC_ISP_NEXT,&q)){perror("continuousNEXT");goto done;}
  if(q.sequence!=i+1||q.guard_errors||!(q.status&1))goto done;
  uint32_t delivered=qc_crc_bzip2(pixels,sizeof(pixels));
  if(delivered!=q.crc32_bzip2){fprintf(stderr,"CONTINUOUS CRC MISMATCH target=%08x delivered=%08x; refuseframe\n",q.crc32_bzip2,delivered);goto done;}
  printf("CONTINUOUS FRAME seq=%u bytes=%u status=%08x guards=%u CRC_BZIP2=%08x delivery_ms=%llu sensor0100=%d\n",q.sequence,q.bytes,q.status,q.guard_errors,q.crc32_bzip2,(unsigned long long)(ms()-before),sensor_read(sensor,0x0100));
 }
 printf("CONTINUOUS BATCH frames=8 elapsed_ms=%llu no_per_frame_reset=1 gated_handoff=1 irq_mode=%d NOT-FREE-RUNNING-V4L2\n",(unsigned long long)(ms()-start),irq_mode);
 if(metadata_only){result=0;goto done;}
 int fd=open(argv[1],O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)goto done;
 size_t offset=0;while(offset<sizeof(pixels)){ssize_t n=write(fd,pixels+offset,sizeof(pixels)-offset);if(n<0&&errno==EINTR)continue;if(n<=0){close(fd);goto done;}offset+=n;}close(fd);
 result=0;
done:
 if(sensor>=0){if(sensor_stream(sensor,0))result=1;close(sensor);}
 if(frame>=0){if(begun&&ioctl(frame,QC_ISP_END))result=1;close(frame);}
 if(guard>=0){write(guard,"V",1);close(guard);}return result;
}
