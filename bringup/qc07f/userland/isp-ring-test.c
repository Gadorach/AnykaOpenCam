/* SPDX-License-Identifier: GPL-2.0-only
 * Boundedfour-buffer/16frameIRQ diagnostic. Noencoder/consumerlease whileDMA
 * active; kernelquiesces/reset beforeper-slotCRC/guards andcopyoneframe.
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "crc-bzip2.h"
static uint8_t pixels[1728000];
static int regread(int fd,uint16_t reg)
{uint8_t p[2]={reg>>8,reg},v=0;struct i2c_msg m[2]={{.addr=0x30,.len=2,.buf=p},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&v}};struct i2c_rdwr_ioctl_data q={m,2};return ioctl(fd,I2C_RDWR,&q)==2?v:-1;}
static int stream(int fd,uint8_t on){uint8_t b[3]={1,0,on};return write(fd,b,3)==3&&regread(fd,0x0100)==on?0:-1;}
int main(int argc,char **argv)
{
 if(argc<2||argc>9||strncmp(argv[1],"/run/continuous-ring-",21)||strstr(argv[1],".."))return 2;
 unsigned selected=0,frames=16;int metadata_only=0,hold_selected=0,only_slot0=0,corrupt_shadow=0;
 for(int i=2;i<argc;i++){
  if(!strcmp(argv[i],"--metadata-only"))metadata_only=1;
  else if(!strcmp(argv[i],"--hold-slot0")){hold_selected=1;only_slot0=1;}
  else if(!strcmp(argv[i],"--hold-selected"))hold_selected=1;
  else if(!strcmp(argv[i],"--corrupt-shadow"))corrupt_shadow=1;
  else if(!strcmp(argv[i],"--slot")&&i+1<argc){char *end;unsigned long value=strtoul(argv[++i],&end,10);if(!argv[i][0]||*end||value>3)return 2;selected=(unsigned)value;}
  else if(!strcmp(argv[i],"--frames")&&i+1<argc){char *end;unsigned long value=strtoul(argv[++i],&end,10);if(!argv[i][0]||*end||!value||value>16)return 2;frames=(unsigned)value;}
  else return 2;
 }
 if((only_slot0&&selected)||(hold_selected&&frames<selected+8)||(corrupt_shadow&&!hold_selected))return 2;
 int sensor=-1,frame=-1,guard=-1,result=1,timeout=5,out=-1;
 sensor=open("/dev/i2c-0",O_RDWR);if(sensor<0||ioctl(sensor,I2C_SLAVE,0x30)||regread(sensor,0x3107)!=0xda||regread(sensor,0x3108)!=0x4d)goto done;
 if(regread(sensor,0x3208)!=5||regread(sensor,0x3209)!=0||regread(sensor,0x320a)!=2||regread(sensor,0x320b)!=0xd0)goto done;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 frame=open("/dev/qc07f-frame",O_RDONLY);if(frame<0||stream(sensor,1))goto done;
 if(corrupt_shadow)memset(pixels,0x6b,sizeof(pixels));
 struct qc_isp_ring_test q={.frames=frames,.slot=selected,.bytes=sizeof(pixels),.user_ptr=(uint32_t)(uintptr_t)pixels,.hold_selected=hold_selected,.corrupt_shadow=corrupt_shadow};
 if(ioctl(frame,QC_ISP_RING_TEST,&q)){
  int error=errno;fprintf(stderr,"RING FAIL errno=%d completed=%u; NOTproduction; metadata may be partial\n",error,q.completed);
  for(unsigned i=0;i<q.completed&&i<16;i++)fprintf(stderr,"RING PARTIAL IRQ n=%u status=%08x ctrl=%08x ms=%u\n",i,q.irq_status[i],q.irq_control[i],q.irq_elapsed_ms[i]);
  for(unsigned i=0;i<4;i++)fprintf(stderr,"RING PARTIAL SLOT index=%u crc=%08x guards=%u stat_guards=%u main_changed=%u sub_changed=%u\n",i,q.slot_crc[i],q.slot_guards[i],q.stat_guards[i],q.main_changed[i],q.sub_changed[i]);
  fprintf(stderr,"HELD PARTIAL pre=%08x post=%08x final=%08x shadow=%08x bracket=%u..%u active=%u\n",q.held_pre_crc,q.held_post_crc,q.held_final_crc,q.held_shadow_crc,q.held_bracket_start_events,q.held_bracket_end_events,q.held_active_bracket);
  if(corrupt_shadow&&error==EIO&&q.held_active_bracket&&q.held_pre_crc==q.held_post_crc&&
     q.held_pre_crc==q.held_final_crc&&q.held_pre_crc!=q.held_shadow_crc){
   for(unsigned i=0;i<4;i++)if(q.slot_guards[i]||q.stat_guards[i])goto done;
   for(size_t i=0;i<sizeof(pixels);i++)if(pixels[i]!=0x6b)goto done;
   puts("HELD CPU-SHADOW NEGATIVE PASS: expectedEIO, DMApixelsunchanged, rejectedpixelsnotcopied, nofilecreated; NOTDMAfaultinjection");result=0;
  }
  goto done;
 }
 if(corrupt_shadow){fprintf(stderr,"HELD CPU-SHADOW NEGATIVE unexpectedlyaccepted\n");goto done;}
 if(q.completed!=frames)goto done;
 for(unsigned i=0;i<q.completed;i++)printf("RING IRQ n=%u status=%08x hardware_id=%u ctrl=%08x ms=%u\n",i,q.irq_status[i],(q.irq_status[i]>>22)&3,q.irq_control[i],q.irq_elapsed_ms[i]);
 for(unsigned i=0;i<4;i++){printf("RING SLOT index=%u crc=%08x guards=%u stat_guards=%u main_changed=%u sub_changed=%u\n",i,q.slot_crc[i],q.slot_guards[i],q.stat_guards[i],q.main_changed[i],q.sub_changed[i]);if(q.slot_guards[i]||q.stat_guards[i])goto done;}
 if(hold_selected){
  printf("HELD SLOT%u pre=%08x post=%08x final=%08x shadow=%08x bracket=%u..%u active=%u; bracketincludesCRCs/copy, NOTbarememcpytiming\n",selected,q.held_pre_crc,q.held_post_crc,q.held_final_crc,q.held_shadow_crc,q.held_bracket_start_events,q.held_bracket_end_events,q.held_active_bracket);
  if(!q.held_active_bracket||q.held_pre_crc!=q.held_post_crc||q.held_pre_crc!=q.held_final_crc||q.held_pre_crc!=q.held_shadow_crc)goto done;
  for(unsigned i=selected+1;i<q.completed;i++)if(((q.irq_status[i]>>22)&3)==selected){fprintf(stderr,"HELD SELECTED SLOT unexpectedlyrevisited\n");goto done;}
 }
 if(qc_crc_bzip2(pixels,sizeof(pixels))!=q.slot_crc[q.slot]){fprintf(stderr,"RING DELIVERED CRC MISMATCH\n");goto done;}
 printf("RING DELIVERED slot=%u CRC_BZIP2=%08x bytes=%zu\n",q.slot,q.slot_crc[q.slot],sizeof(pixels));
 if(!metadata_only){
  out=open(argv[1],O_WRONLY|O_CREAT|O_EXCL,0600);if(out<0)goto done;
  size_t off=0;while(off<sizeof(pixels)){ssize_t n=write(out,pixels+off,sizeof(pixels)-off);if(n<0&&errno==EINTR)continue;if(n<=0)goto done;off+=n;}
 }
 puts("RING DIAGNOSTIC PASS; fourdistinctownedbuffers; quiescedcopy only; NOTfree-runningV4L2/zerocopy");result=0;
done:
 if(out>=0)close(out);if(sensor>=0){if(stream(sensor,0))result=1;close(sensor);}if(frame>=0)close(frame);if(guard>=0){write(guard,"V",1);close(guard);}return result;
}
