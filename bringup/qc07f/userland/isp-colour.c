/* SPDX-License-Identifier: GPL-2.0-only
 * Explicitidle-onlyretailUVgainpresets; noLED/IR-cut/GPIO orarbitraryregisters.
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
int main(int argc,char **argv)
{
 if(argc!=2||(strcmp(argv[1],"colour")&&strcmp(argv[1],"mono")))return 2;
 struct qc_isp_colour q={.monochrome=!strcmp(argv[1],"mono")};
 int fd=open("/dev/qc07f-frame",O_RDONLY);if(fd<0)return 1;
 int result=ioctl(fd,QC_ISP_COLOUR,&q);close(fd);
 if(result){perror("idlecolourpreset");return 1;}
 printf("ISP desiredmode=%s UVgain=%u; takes effectatnextcapturetable; colouraccuracy/lightingnotproved\n",argv[1],q.uv_gain_readback);return 0;
}
