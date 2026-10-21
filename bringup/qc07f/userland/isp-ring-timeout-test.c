/* SPDX-License-Identifier: GPL-2.0-only
 * Deliberate no-input ring timeout. Requires measured SC1346 ID/geometry;
 * no frame accepted, no illumination/GPIO/flash operation. Kernel must retire
 * and release allocations on ETIMEDOUT; observer/fresh capture tests follow.
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
static uint8_t pixels[1728000];
static int regread(int fd,uint16_t reg)
{
 uint8_t p[2]={reg>>8,reg},v=0;
 struct i2c_msg m[2]={{.addr=0x30,.len=2,.buf=p},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&v}};
 struct i2c_rdwr_ioctl_data q={m,2};return ioctl(fd,I2C_RDWR,&q)==2?v:-1;
}
int main(void)
{
 int sensor=-1,frame=-1,guard=-1,result=1,timeout=5;
 sensor=open("/dev/i2c-0",O_RDWR);
 if(sensor<0||ioctl(sensor,I2C_SLAVE,0x30)||regread(sensor,0x3107)!=0xda||regread(sensor,0x3108)!=0x4d||
    regread(sensor,0x3208)!=5||regread(sensor,0x3209)!=0||regread(sensor,0x320a)!=2||regread(sensor,0x320b)!=0xd0)goto done;
 uint8_t off[3]={1,0,0};if(write(sensor,off,3)!=3||regread(sensor,0x0100)!=0)goto done;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 frame=open("/dev/qc07f-frame",O_RDONLY);if(frame<0)goto done;
 struct qc_isp_ring_test q={.frames=16,.slot=0,.bytes=sizeof(pixels),.user_ptr=(uint32_t)(uintptr_t)pixels};errno=0;
 int r=ioctl(frame,QC_ISP_RING_TEST,&q),error=errno;
 printf("RING NOINPUT NEGATIVE result=%d errno=%d expectedETIMEDOUT=%d completed=%u; noframeaccepted\n",r,error,ETIMEDOUT,q.completed);
 if(r!=-1||error!=ETIMEDOUT||q.completed)goto done;
 for(unsigned i=0;i<4;i++){
  printf("RING NOINPUT SLOT index=%u guards=%u stat_guards=%u main_changed=%u sub_changed=%u\n",i,q.slot_guards[i],q.stat_guards[i],q.main_changed[i],q.sub_changed[i]);
  if(q.slot_guards[i]||q.stat_guards[i]||q.main_changed[i]||q.sub_changed[i])goto done;
 }
 result=0;
done:
 if(frame>=0)close(frame);if(sensor>=0)close(sensor);if(guard>=0){write(guard,"V",1);close(guard);}return result;
}
