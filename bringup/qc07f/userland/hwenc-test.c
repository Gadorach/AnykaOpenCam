/* SPDX-License-Identifier: GPL-2.0-only
 * Explicit bounded hardware JPEG qualification. Synthetic mode is an
 * encoder fixture only; --camera snapshots actual retained ISP frame.
 * Writes exclusive /run files only, no card/SPI/OTA/OTP or stock binaries.
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
#include <jpeglib.h>
#include "../hwenc/qc07f_hwenc.h"
#define W 1280U
#define H 720U
#define PIXELS (W*H)
#define BYTES (PIXELS*3/2)
static uint8_t raw[BYTES],encoded[0x100000];
static void put16(uint8_t *p,unsigned n){p[0]=n>>8;p[1]=n;}
static int full_write(int fd,const void *data,size_t bytes)
{const uint8_t *p=data;while(bytes){ssize_t n=write(fd,p,bytes);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;p+=n;bytes-=n;}return 0;}
int main(int argc,char **argv)
{
 int encoder=-1,guard=-1,result=1,timeout=5;char magic='V';struct qc_hwenc_info info={0};
 int camera=argc==3&&!strcmp(argv[1],"--camera"),synthetic=argc==3&&!strcmp(argv[1],"--synthetic");
 if(!(argc==2&&!strcmp(argv[1],"--info"))&&!camera&&!synthetic)return 2;
 if((camera||synthetic)&&(strncmp(argv[2],"/run/hwenc-",11)||strstr(argv[2],"..")))return 2;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0){perror("watchdog");goto done;}
 if(ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5){perror("watchdog timeout");goto done;}
 encoder=open("/dev/qc07f-encoder",O_RDWR);if(encoder<0){perror("encoder");goto done;}
 if(ioctl(encoder,QC_HWENC_INFO,&info)){perror("encoder identity");goto done;}
 printf("HWENC INFO id=%08x config=%08x jpeg=%u h264=%u max_width=%u guard_errors=%u\n",info.id,info.config,(info.config>>25)&1,(info.config>>27)&1,info.config&0xfff,info.guard_errors);
 if(!camera&&!synthetic){result=0;goto done;}
 if(camera){
  int fd=open("/dev/qc07f-frame",O_RDONLY);if(fd<0){perror("actual captured frame");goto done;}
  /* One complete protected read, not cross-capture/padded chunk assembly. */
  ssize_t n=read(fd,raw,sizeof(raw));close(fd);if(n!=sizeof(raw)){fprintf(stderr,"No exact readable main frame, bytes=%ld\n",(long)n);goto done;}
 }else{
  for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++)raw[y*W+x]=16+(x*180/W+y*32/H);
  memset(raw+PIXELS,96,PIXELS/4);memset(raw+PIXELS*5/4,160,PIXELS/4);
 }
 struct jpeg_compress_struct c;struct jpeg_error_mgr error;unsigned char *tables=NULL;unsigned long tables_bytes=0;
 c.err=jpeg_std_error(&error);jpeg_create_compress(&c);jpeg_mem_dest(&c,&tables,&tables_bytes);
 c.image_width=W;c.image_height=H;c.input_components=3;c.in_color_space=JCS_YCbCr;jpeg_set_defaults(&c);jpeg_set_quality(&c,85,TRUE);
 struct qc_hwenc_job job={.width=W,.height=H,.input_bytes=sizeof(raw),.input_ptr=(uint32_t)(uintptr_t)raw,.output_capacity=sizeof(encoded),.output_ptr=(uint32_t)(uintptr_t)encoded};
 for(unsigned p=0;p<2;p++)for(unsigned i=0;i<64;i++)job.quant[p*64+i]=c.quant_tbl_ptrs[p]->quantval[i];
 /* Standard DQT/DHT marker data only: no software pixel/entropy encode. */
 jpeg_write_tables(&c);jpeg_destroy_compress(&c);
 if(tables_bytes<4||tables[tables_bytes-2]!=255||tables[tables_bytes-1]!=0xd9){free(tables);goto done;}
 write(guard,".",1);
 if(ioctl(encoder,QC_HWENC_JPEG,&job)){fprintf(stderr,"HWENC failed errno=%d status=%08x bytes=%u\n",errno,job.status,job.output_bytes);free(tables);goto done;}
 printf("HWENC COMPLETE status=%08x entropy_bytes=%u fixture=%s\n",job.status,job.output_bytes,camera?"ACTUAL-CAMERA":"SYNTHETIC-NOT-CAMERA");
 if(job.output_bytes<2||job.output_bytes>sizeof(encoded)||encoded[job.output_bytes-2]!=255||encoded[job.output_bytes-1]!=0xd9){fprintf(stderr,"Hardware output lacks finalEOI; no JPEG success claim\n");free(tables);goto done;}
 uint8_t frame[19]={255,0xc0,0,17,8,0,0,0,0,3,1,0x22,0,2,0x11,1,3,0x11,1};put16(frame+5,H);put16(frame+7,W);
 uint8_t scan[14]={255,0xda,0,12,3,1,0,2,0x11,3,0x11,0,63,0};
 int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0){perror("exclusive RAM JPEG");free(tables);goto done;}
 if(full_write(fd,tables,tables_bytes-2)||full_write(fd,frame,sizeof(frame))||full_write(fd,scan,sizeof(scan))||full_write(fd,encoded,job.output_bytes)){close(fd);free(tables);goto done;}
 close(fd);free(tables);printf("HWENC RAM JPEG ready %s, validate with real decoder before RTSP switch\n",argv[2]);result=0;
done:
 if(encoder>=0)close(encoder);
 if(guard>=0){write(guard,&magic,1);close(guard);}
 return result;
}
