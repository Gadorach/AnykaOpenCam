/* SPDX-License-Identifier: GPL-2.0-only
 * Kernellease negativefixture. ExplicitBEGINidle, noNEXT/sensor/GPIO/DMAstart.
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
int main(void)
{
 int owner=-1,other=-1,guard=-1,result=1,timeout=5,begun=0;
 guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;
 owner=open("/dev/qc07f-frame",O_RDONLY);other=open("/dev/qc07f-frame",O_RDONLY);if(owner<0||other<0)goto done;
 if(ioctl(owner,QC_ISP_BEGIN))goto done;begun=1;
 struct qc_isp_colour q={.monochrome=1};errno=0;
 int r=ioctl(other,QC_ISP_COLOUR,&q),error=errno;
 printf("COLOUR HELD_LEASE negative result=%d errno=%d expectedEBUSY=%d; noframeDMA/noGPIO\n",r,error,EBUSY);
 if(r==-1&&error==EBUSY)result=0;
done:
 if(other>=0)close(other);if(owner>=0){if(begun&&ioctl(owner,QC_ISP_END))result=1;close(owner);}
 if(guard>=0){write(guard,"V",1);close(guard);}return result;
}
