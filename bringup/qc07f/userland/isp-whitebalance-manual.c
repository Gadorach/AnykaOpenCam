/* SPDX-License-Identifier: GPL-2.0-only
 * Raw, idle-only ISP WB prototype. Not calibrated colour or AWB.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"

/* Decimal digits only: reject signs, whitespace, hex, overflow and suffixes. */
static int number(const char *s, unsigned min, unsigned max, __u32 *out)
{
 unsigned value=0;
 if(!*s)return -1;
 for(;*s;s++) {
  unsigned digit=(unsigned char)*s-'0';
  if(digit>9 || value>(max-digit)/10 || digit>max)return -1;
  value=value*10+digit;
 }
 if(value<min || value>max)return -1;
 *out=value;return 0;
}

int main(int argc,char **argv)
{
 struct qc_isp_whitebalance_manual q={.version=QC_ISP_WB_MANUAL_VERSION};
 int preset=-1,fd,result,saved_errno;
 if(argc==2 && !strcmp(argv[1],"get")) {
  /* All metadata is read-only; no device configuration on GET. */
 } else if(argc==2 && !strcmp(argv[1],"snapshot"))preset=0;
 else if(argc==2 && !strcmp(argv[1],"retained-manual"))preset=1;
 else if(argc==8 && !strcmp(argv[1],"set-raw")) {
  for(unsigned i=0;i<3;i++)
   if(number(argv[2+i],1,4095,&q.gain[i]) ||
      number(argv[5+i],0,1023,&q.offset_raw[i]))goto usage;
  q.set=1;
 } else goto usage;
 fd=open("/dev/qc07f-frame",O_RDONLY);
 if(fd<0){perror("open");return 1;}
 if(preset>=0) {
  struct qc_isp_whitebalance p={.set=1,.preset=(unsigned)preset};
  result=ioctl(fd,QC_ISP_WHITEBALANCE,&p);
  if(result)goto failed;
 }
 result=ioctl(fd,QC_ISP_WHITEBALANCE_MANUAL,&q);
 if(result)goto failed;
 close(fd);
 printf("camera=%u desired-preset=%u RGB-raw=%u/%u/%u offsets-raw=%u/%u/%u desired-monotonic-ns=%" PRIu64 " owner-active=%u\n",
        q.camera_id,q.preset,q.gain[0],q.gain[1],q.gain[2],q.offset_raw[0],q.offset_raw[1],q.offset_raw[2],(uint64_t)q.desired_changed_ns,q.owner_active);
 printf("last-table-prepared=%u preset=%u RGB-raw=%u/%u/%u offsets-raw=%u/%u/%u monotonic-ns=%" PRIu64 "; RAM metadata, not live readback, calibrated colour or AWB\n",
        q.table_prepared,q.last_prepared_preset,q.last_prepared_gain[0],q.last_prepared_gain[1],q.last_prepared_gain[2],q.last_prepared_offset_raw[0],q.last_prepared_offset_raw[1],q.last_prepared_offset_raw[2],(uint64_t)q.last_prepared_ns);
 return 0;
failed:
 saved_errno=errno;close(fd);errno=saved_errno;perror("idle-manual-whitebalance");return 1;
usage:
 fprintf(stderr,"usage: %s get|snapshot|retained-manual|set-raw R G B Roff Goff Boff\nraw gains 1..4095; raw offsets 0..1023 (no signed arithmetic claim)\n",argv[0]);
 return 2;
}
