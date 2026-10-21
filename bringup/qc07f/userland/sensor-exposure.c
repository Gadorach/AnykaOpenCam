/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded SC1346 standby exposure-code control. Capture owner+watchdog held;
 * no gain/VTS/orientation/light writes. Absolute time units are unqualified.
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "sc1346-transaction.h"
static int regread(void *context,uint16_t reg)
{
 int fd=*(int *)context;uint8_t p[2]={reg>>8,reg},v=0;
 struct i2c_msg m[2]={{.addr=0x30,.len=2,.buf=p},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&v}};
 struct i2c_rdwr_ioctl_data q={m,2};return ioctl(fd,I2C_RDWR,&q)==2?v:-1;
}
static int regwrite(void *context,uint16_t reg,uint8_t value)
{
 int fd=*(int *)context;uint8_t p[3]={reg>>8,reg,value};
 return write(fd,p,3)==3?0:-1;
}
int main(int argc,char **argv)
{
 int get=argc==2&&!strcmp(argv[1],"get");uint32_t code=0;
 if(!get){
  if(argc!=3||strcmp(argv[1],"set-code"))return 2;
  char *end;errno=0;unsigned long n=strtoul(argv[2],&end,10);
  if(errno||!argv[2][0]||*end||n<2||n>SC1346_RETAIL_EXPOSURE)return 2;code=(uint32_t)n;
 }
 int sensor=-1,capture=-1,guard=-1,owned=0,result=1,timeout=5,signals_blocked=0;
 sigset_t saved,blocked;
 sensor=open("/dev/i2c-0",O_RDWR);if(sensor<0||ioctl(sensor,I2C_SLAVE,0x30))goto done;
 if(regread(&sensor,0x3107)!=0xda||regread(&sensor,0x3108)!=0x4d||
    regread(&sensor,0x3208)!=5||regread(&sensor,0x3209)!=0||
    regread(&sensor,0x320a)!=2||regread(&sensor,0x320b)!=0xd0)goto done;
 struct sc1346_control_bus bus={.context=&sensor,.read=regread,.write=regwrite};
 if(get){
  uint8_t value[3];if(sc1346_read_exposure(&bus,value))goto done;
  int stream=regread(&sensor,0x0100);if(stream<0)goto done;
  printf("SC1346 actual exposure_code=%u regs=%02x/%02x/%02x stream=%d; raw vendor code, NOT us/lines/lux\n",sc1346_exposure_code(value),value[0],value[1],value[2],stream);result=0;goto done;
 }
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 capture=open("/dev/qc07f-frame",O_RDONLY);if(capture<0||ioctl(capture,QC_ISP_BEGIN_IRQ))goto done;owned=1;
 sigemptyset(&blocked);sigaddset(&blocked,SIGINT);sigaddset(&blocked,SIGTERM);sigaddset(&blocked,SIGQUIT);
 if(sigprocmask(SIG_BLOCK,&blocked,&saved))goto done;signals_blocked=1;
 struct sc1346_exposure_result r={0};int status=sc1346_exposure_transaction(&bus,code,&r);
 printf("SC1346 exposure transaction status=%d before=%u requested=%u after=%u vts=%u writes=%d rollback=%d rollback_verified=%d standby_only=1 gain_vts_flip_verified_unchanged=%d; code units NOT physical time\n",status,r.before_code,r.requested_code,r.after_code,r.vts,r.writes_started,r.rollback_attempted,r.rollback_verified,status==SC1346_CONTROL_OK||r.rollback_verified);
 if(status==SC1346_CONTROL_OK)result=0;
 else if(status==SC1346_CONTROL_ROLLBACK_FAILED)fputs("REFUSE capture: exposure rollback/unrelated-state verification failed; requires explicit recovery\n",stderr);
 done:
 if(owned&&ioctl(capture,QC_ISP_END)){perror("capture-retirement");result=1;}
 if(capture>=0)close(capture);
 if(guard>=0){if(write(guard,"V",1)!=1)result=1;close(guard);}
 if(sensor>=0)close(sensor);
 if(result)fputs("SC1346 control failed/refused; no accepted exposure result\n",stderr);
 fflush(stdout);fflush(stderr);
 if(signals_blocked)sigprocmask(SIG_SETMASK,&saved,NULL);
 return result;
}
