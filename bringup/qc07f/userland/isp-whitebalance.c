/* SPDX-License-Identifier: GPL-2.0-only
 * Two evidence-supported idle-only WB presets; no light/GPIO/sensor writes.
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
int main(int argc,char **argv)
{
 if(argc!=2||(strcmp(argv[1],"get")&&strcmp(argv[1],"snapshot")&&strcmp(argv[1],"retained-manual")))return 2;
 struct qc_isp_whitebalance q={.set=strcmp(argv[1],"get")!=0,.preset=!strcmp(argv[1],"retained-manual")};
 int fd=open("/dev/qc07f-frame",O_RDONLY);if(fd<0){perror("open");return 1;}
 int result=ioctl(fd,QC_ISP_WHITEBALANCE,&q);close(fd);
 if(result){perror("idle-whitebalance");return 1;}
 printf("ISP desired-next-table WB preset=%u RGB-fields=%u/%u/%u offsets-raw=%u/%u/%u; not live readback, AWB or calibrated daylight\n",q.preset,q.gain[0],q.gain[1],q.gain[2],q.offset_raw[0],q.offset_raw[1],q.offset_raw[2]);return 0;
}
