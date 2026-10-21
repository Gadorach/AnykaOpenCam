/* SPDX-License-Identifier: GPL-2.0-only
 * Read-only register inspection for the already identified SC1346 at0x30.
 * I2C messages select a register then read it; never write a sensor value.
 * Raw exposure/gain codes are not measured lux or proof of physical units.
 */
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
static int regread(int fd,uint16_t reg)
{
 uint8_t p[2]={reg>>8,reg},value=0;
 struct i2c_msg m[2]={{.addr=0x30,.len=2,.buf=p},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&value}};
 struct i2c_rdwr_ioctl_data q={m,2};return ioctl(fd,I2C_RDWR,&q)==2?value:-1;
}
int main(void)
{
 int fd=open("/dev/i2c-0",O_RDWR);if(fd<0)return 1;
 if(regread(fd,0x3107)!=0xda||regread(fd,0x3108)!=0x4d){close(fd);return 1;}
 const uint16_t regs[]={0x0100,0x3208,0x3209,0x320a,0x320b,0x320c,0x320d,0x320e,0x320f,
  0x3e00,0x3e01,0x3e02,0x3e03,0x3e06,0x3e07,0x3e08,0x3e09};
 for(unsigned i=0;i<sizeof(regs)/sizeof(regs[0]);i++){
  int value=regread(fd,regs[i]);if(value<0){close(fd);return 1;}
  printf("SC1346 RO reg=%04x value=%02x\n",regs[i],value);
 }
 puts("SC1346 RO no sensor-value writes; raw codes NOT ambient-sensor/lux measurements");
 close(fd);return 0;
}
