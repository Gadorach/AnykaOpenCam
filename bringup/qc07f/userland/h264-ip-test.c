/* SPDX-License-Identifier: GPL-2.0-only
 * Separate synthetic eight-frameIDR/P encoder fixture. Not live-camera proof.
 * Dedicated fd owns reference sequencing; firstframeIDR required; fixedQP26.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../hwenc/qc07f_hwenc.h"
#include "h264-baseline.h"
static uint8_t raw[1280*720*3/2],output[0x100000];
static int put(int fd,const void *data,size_t bytes){const uint8_t *p=data;while(bytes){ssize_t n=write(fd,p,bytes);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;p+=n;bytes-=n;}return 0;}
int main(int argc,char **argv)
{
 if(argc!=2||strncmp(argv[1],"/run/h264-ip-",13)||strstr(argv[1],".."))return 2;
 int fd=-1,encoder=-1,guard=-1,result=1,timeout=5;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 encoder=open("/dev/qc07f-encoder",O_RDWR);if(encoder<0)goto done;
 fd=open(argv[1],O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)goto done;
 uint8_t params[256];size_t header=qc_h264_parameter_sets(params);if(put(fd,params,header))goto done;
 for(unsigned i=0;i<8;i++){
  for(unsigned y=0;y<720;y++)for(unsigned x=0;x<1280;x++)raw[y*1280+x]=16+x*180/1280+y*32/720+i;
  memset(raw+1280*720,96,1280*720/4);memset(raw+1280*720*5/4,160,1280*720/4);
  struct qc_hwenc_h264_stream_job q={.frame={.width=1280,.height=720,.input_bytes=sizeof(raw),.input_ptr=(uint32_t)(uintptr_t)raw,.output_capacity=sizeof(output),.output_ptr=(uint32_t)(uintptr_t)output},.keyframe=i==0};
  if(write(guard,".",1)!=1||ioctl(encoder,QC_HWENC_H264_STREAM,&q)){perror("H264stream");goto done;}
  if(q.frame_num!=i||q.frame.output_bytes<2||q.frame.output_bytes>sizeof(output))goto done;
  size_t off=0;if(!output[0]&&!output[1]){if(output[2]==1)off=3;else if(q.frame.output_bytes>4&&!output[2]&&output[3]==1)off=4;}
  if((output[off]&31)!=(i?1:5))goto done;
  const uint8_t delimiter[4]={0,0,0,1};if(put(fd,delimiter,4)||put(fd,output+off,q.frame.output_bytes-off))goto done;
  printf("H264 IP FIXTURE-NOT-CAMERA index=%u frame_num=%u type=%s nal_bytes=%u status=%08x\n",i,q.frame_num,i?"P":"IDR",q.frame.output_bytes,q.frame.status);
 }
 result=0;
done:if(fd>=0)close(fd);if(encoder>=0)close(encoder);if(guard>=0){write(guard,"V",1);close(guard);}return result;
}
