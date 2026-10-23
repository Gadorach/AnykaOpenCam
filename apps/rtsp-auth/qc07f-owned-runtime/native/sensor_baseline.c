/* SPDX-License-Identifier: GPL-2.0-only
 * Typed, bounded counterpart of initramfs/init.c sensor_message/read/write/
 * validate and exact139 sensor_control(1). Original excerpt/diff pinned by build.
 * No monitor commands, raw syscalls, bus scan, retry or arbitrary write API.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "sensor_baseline.h"
#include "handoff.h"
#include "sc1346-init.h"
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
_Static_assert(sizeof(sc1346_init_table)/sizeof(sc1346_init_table[0])==NH_SENSOR_PAIRS,"exact139 table");
#if UINTPTR_MAX==0xffffffffU
_Static_assert(sizeof(struct i2c_msg)==12,"ARM32 typed I2C msg");
_Static_assert(sizeof(struct i2c_rdwr_ioctl_data)==8,"ARM32 typed I2C rdwr");
#endif
struct bound {uint64_t begin;struct camera_rtsp_clock *clock;const volatile sig_atomic_t *cancel;};
static int check(struct bound *b,uint64_t *now)
{
 if(!b)return 0;
 *now=camera_rtsp_clock_ns(b->clock);
 return *now==UINT64_MAX||*now<b->begin||*now-b->begin>=NH_OUTPUT_NS||*b->cancel?-1:0;
}
static int message(int fd,uint8_t *buffer,uint16_t length,uint16_t flags,struct bound *b)
{
 uint64_t now=0;if(check(b,&now))return -1;
 struct i2c_msg msg;struct i2c_rdwr_ioctl_data q;
 memset(&msg,0,sizeof(msg));memset(&q,0,sizeof(q)); /* monitor wire padding0 */
 msg.addr=0x30;msg.flags=flags;msg.len=length;msg.buf=buffer;q.msgs=&msg;q.nmsgs=1;
 if(ioctl(fd,I2C_RDWR,&q)!=1)return -1; /* no partial reply inspection/retry */
 return check(b,&now);
}
static int read_byte(int fd,uint16_t reg,struct bound *b)
{
 uint8_t address[2]={(uint8_t)(reg>>8),(uint8_t)reg},value=0;
 if(message(fd,address,2,0,b)||message(fd,&value,1,I2C_M_RD,b))return -1;
 return value;
}
static int write_byte(int fd,uint16_t reg,uint8_t value,struct bound *b)
{
 uint8_t bytes[3]={(uint8_t)(reg>>8),(uint8_t)reg,value};
 return message(fd,bytes,3,0,b);
}
int nh_sensor_read_byte(int fd,uint16_t reg){return read_byte(fd,reg,NULL);}
int nh_sensor_stream(int fd,uint8_t on)
{
 if(on>1||write_byte(fd,0x0100,on,NULL)||read_byte(fd,0x0100,NULL)!=on)return -1;
 return 0;
}
static uint8_t final_value(unsigned i)
{
 uint16_t reg=sc1346_init_table[i].reg;
 /* Reset is a command, not stable value1: require reset-complete readback0.
  * This explicit strict qualification policy remains native-unqualified. */
 if(reg==0x0103)return 0;
 for(unsigned j=NH_SENSOR_PAIRS;j;j--)if(sc1346_init_table[j-1].reg==reg)return sc1346_init_table[j-1].value;
 return sc1346_init_table[i].value;
}
int nh_sensor_exact139(int fd,int watchdog,uint64_t begin,struct camera_rtsp_clock *clock,
 const volatile sig_atomic_t *cancel,struct nh_sensor_report *r)
{
 if(!r||!clock||!cancel||fd<0||watchdog<0||r->attempted)return -1;
 memset(r,0,sizeof(*r));r->attempted=1;r->begin_ns=begin;r->failed_index=UINT32_MAX;
 for(unsigned i=0;i<NH_SENSOR_PAIRS;i++){
  r->rows[i].reg=sc1346_init_table[i].reg;r->rows[i].programmed=sc1346_init_table[i].value;
  r->rows[i].expected=final_value(i);
 }
 struct bound b={begin,clock,cancel};uint64_t now=0;int result=-1,timeout=0;
 if(check(&b,&now)||ioctl(watchdog,WDIOC_GETTIMEOUT,&timeout)||timeout!=5||check(&b,&now))goto done;
 r->guard_verified=1;
 /* Same fixed-ID validation as the maintained monitor, before ANY write. */
 int high=read_byte(fd,0x3107,&b);if(high!=0xda)goto done;
 int low=read_byte(fd,0x3108,&b);if(low!=0x4d||read_byte(fd,0x0100,&b)!=0)goto done;
 for(unsigned i=0;i<NH_SENSOR_PAIRS;i++){
  r->failed_index=i;
  if(write_byte(fd,sc1346_init_table[i].reg,sc1346_init_table[i].value,&b))goto done;
  r->written++;
  if(sc1346_init_table[i].reg==0x0103){
   struct timespec wait={.tv_sec=0,.tv_nsec=10000000L};uint64_t start,end;
   if(check(&b,&start)||nanosleep(&wait,NULL)||check(&b,&end))goto done; /* exactly once, never EINTR retry */
   r->reset_delay_ns=end-start;if(r->reset_delay_ns<10000000ULL)goto done;
  }
 }
 for(unsigned i=0;i<NH_SENSOR_PAIRS;i++){
  r->failed_index=i;int value=read_byte(fd,sc1346_init_table[i].reg,&b);
  if(value<0)goto done;
  r->rows[i].actual=(uint8_t)value;r->rows[i].valid=1;r->read_count++;
  if(value!=r->rows[i].expected)r->mismatches++;
 }
 if(r->mismatches)goto done;
 /* Entire final table matches, hence1280x720/HTS1800/VTS750/stream-off.
  * No sensor-on here; only the existing owned BEGIN path can start it. */
 r->ready=1;r->failed_index=UINT32_MAX;result=0;
done:
 r->end_ns=camera_rtsp_clock_ns(clock);
 if(r->end_ns==UINT64_MAX||r->end_ns<begin||r->end_ns-begin>=NH_OUTPUT_NS||*cancel){r->ready=0;result=-1;}
 return result;
}
void nh_sensor_log(const struct nh_sensor_report *r)
{
 fprintf(stderr,"NH_SENSOR_BASELINE exact139 attempted=%u guard5=%u written=%u read_count=%u mismatches=%u ready=%u failed_index=%u begin_ns=%llu end_ns=%llu reset_delay_ns=%llu expected_width=1280 expected_height=720 expected_HTS=1800 expected_VTS=750 expected_stream=0 capture_proved=0\n",
 r->attempted,r->guard_verified,r->written,r->read_count,r->mismatches,r->ready,r->failed_index,
 (unsigned long long)r->begin_ns,(unsigned long long)r->end_ns,(unsigned long long)r->reset_delay_ns);
 if(!r->attempted)return;
 for(unsigned i=0;i<NH_SENSOR_PAIRS;i++)fprintf(stderr,"NH_SENSOR_READBACK index=%u reg=%04x programmed=%02x expected_final=%02x actual=%02x valid=%u discrepancy=%u\n",
 i,r->rows[i].reg,r->rows[i].programmed,r->rows[i].expected,r->rows[i].actual,r->rows[i].valid,
 r->rows[i].valid&&r->rows[i].actual!=r->rows[i].expected);
}
int nh_sensor_cli_optin(int argc,char **argv)
{return argc==12&&!strcmp(argv[2],"--sensor-baseline")&&!strcmp(argv[3],"exact139");}
