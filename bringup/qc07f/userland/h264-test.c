/* SPDX-License-Identifier: GPL-2.0-only
 * Explicit one-shot hardware H264 IDR test; synthetic fixture separate from
 * actualcamera; independent decoding required. RAM-only exclusive output.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../hwenc/qc07f_hwenc.h"
#include "h264-baseline.h"
static uint8_t raw[1280*720*3/2],encoded[0x100000];
static int full_write(int fd,const void *data,size_t bytes)
{const uint8_t *p=data;while(bytes){ssize_t n=write(fd,p,bytes);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;p+=n;bytes-=n;}return 0;}
int main(int argc,char **argv)
{
 uint8_t params[256];size_t header=qc_h264_parameter_sets(params);
 if(argc==3&&!strcmp(argv[1],"--headers")){
  int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)return 1;
  int r=full_write(fd,params,header);close(fd);return r?1:0;
 }
 int camera=argc==3&&!strcmp(argv[1],"--camera"),synthetic=argc==3&&!strcmp(argv[1],"--synthetic");
 if((!camera&&!synthetic)||strncmp(argv[2],"/run/h264-",10)||strstr(argv[2],".."))return 2;
 int guard=-1,encoder=-1,result=1,timeout=5,fd=-1;char magic='V';
 if(camera){
  fd=open("/dev/qc07f-frame",O_RDONLY);if(fd<0){perror("actual frame");goto done;}
  ssize_t n=read(fd,raw,sizeof(raw));close(fd);fd=-1;if(n!=sizeof(raw)){fprintf(stderr,"Refuse short frame\n");goto done;}
 }else{
  for(unsigned y=0;y<720;y++)for(unsigned x=0;x<1280;x++)raw[y*1280+x]=16+x*180/1280+y*32/720;
  memset(raw+1280*720,96,1280*720/4);memset(raw+1280*720*5/4,160,1280*720/4);
 }
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5){perror("watchdog");goto done;}
 encoder=open("/dev/qc07f-encoder",O_RDWR);if(encoder<0){perror("encoder");goto done;}
 struct qc_hwenc_h264_job job={.width=1280,.height=720,.input_bytes=sizeof(raw),.input_ptr=(uint32_t)(uintptr_t)raw,.output_capacity=sizeof(encoded),.output_ptr=(uint32_t)(uintptr_t)encoded};
 if(ioctl(encoder,QC_HWENC_H264_IDR,&job)){fprintf(stderr,"H264 FAIL errno=%d status=%08x bytes=%u\n",errno,job.status,job.output_bytes);goto done;}
 /* Preserve exact ASICbytes separately; start-code insertion is packaging,
  * not macroblock/entropy encoding. Retail output may be a bareNAL unit.
  */
 char nalpath[256];if(snprintf(nalpath,sizeof(nalpath),"%s.nal",argv[2])>=(int)sizeof(nalpath))goto done;
 if(job.output_bytes>sizeof(encoded))goto done;
 fd=open(nalpath,O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)goto done;
 if(full_write(fd,encoded,job.output_bytes))goto done;
 close(fd);fd=-1;
 int bare=job.output_bytes>1&&!(encoded[0]&0x80)&&(encoded[0]&0x60)&&(encoded[0]&31)==5;
 int annexb=job.output_bytes>=5&&!encoded[0]&&!encoded[1]&&((encoded[2]==1&&(encoded[3]&31)==5)||(encoded[2]==0&&encoded[3]==1&&(encoded[4]&31)==5));
 if(!bare&&!annexb){
  fprintf(stderr,"H264 output notAnnexB IDR, bytes=%u first=%02x%02x%02x%02x%02x\n",job.output_bytes,encoded[0],encoded[1],encoded[2],encoded[3],encoded[4]);goto done;
 }
 fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0){perror("exclusive RAM output");goto done;}
 const uint8_t delimiter[4]={0,0,0,1};
 if(full_write(fd,params,header)||(bare&&full_write(fd,delimiter,4))||full_write(fd,encoded,job.output_bytes))goto done;
 printf("H264 ASIC IDR status=%08x bytes=%u header_bytes=%zu %s output=%s; decoder qualification pending\n",job.status,job.output_bytes,header,camera?"ACTUAL-CAMERA":"SYNTHETIC-NOT-CAMERA",argv[2]);result=0;
done:
 if(fd>=0)close(fd);if(encoder>=0)close(encoder);if(guard>=0){write(guard,&magic,1);close(guard);}return result;
}
