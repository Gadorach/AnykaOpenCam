/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded WB ioctl ownership fixture: no sensor streaming/pixels, no GPIO.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
static int fields(int fd,unsigned preset,unsigned r,unsigned g,unsigned b)
{
 struct qc_isp_whitebalance q={0};
 return ioctl(fd,QC_ISP_WHITEBALANCE,&q)||q.preset!=preset||q.gain[0]!=r||q.gain[1]!=g||q.gain[2]!=b||q.offset_raw[0]||q.offset_raw[1]||q.offset_raw[2];
}
int main(void)
{
 int fd=open("/dev/qc07f-frame",O_RDONLY),owner=-1,result=1;
 if(fd<0)return 1;
 struct qc_isp_whitebalance q={.set=1,.preset=0};
 if(ioctl(fd,QC_ISP_WHITEBALANCE,&q)||fields(fd,0,396,258,498))goto done;
 q=(struct qc_isp_whitebalance){.set=1,.preset=1};
 if(ioctl(fd,QC_ISP_WHITEBALANCE,&q)||fields(fd,1,413,256,310))goto done;
 q=(struct qc_isp_whitebalance){.set=1,.preset=2};
 if(ioctl(fd,QC_ISP_WHITEBALANCE,&q)!=-1||errno!=EINVAL||fields(fd,1,413,256,310))goto done;
 q=(struct qc_isp_whitebalance){.set=2,.preset=0};
 if(ioctl(fd,QC_ISP_WHITEBALANCE,&q)!=-1||errno!=EINVAL||fields(fd,1,413,256,310))goto done;
 /* Input-copy failure leaves desired state alone. */
 if(ioctl(fd,QC_ISP_WHITEBALANCE,(void *)1)!=-1||errno!=EFAULT||fields(fd,1,413,256,310))goto done;
 /* Readable but nonwritable argument reaches the reply-copy path. */
 long page=sysconf(_SC_PAGESIZE);if(page<=0||(unsigned long)page<sizeof(q))goto done;
 struct qc_isp_whitebalance *readonly=mmap(NULL,(size_t)page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 if(readonly==MAP_FAILED)goto done;*readonly=(struct qc_isp_whitebalance){.set=1,.preset=0};
 int fault_ok=!mprotect(readonly,(size_t)page,PROT_READ)&&ioctl(fd,QC_ISP_WHITEBALANCE,readonly)==-1&&errno==EFAULT;
 munmap(readonly,(size_t)page);if(!fault_ok||fields(fd,1,413,256,310))goto done;
 owner=open("/dev/qc07f-frame",O_RDONLY);if(owner<0||ioctl(owner,QC_ISP_BEGIN_IRQ))goto done;
 q=(struct qc_isp_whitebalance){.set=1,.preset=0};
 if(ioctl(fd,QC_ISP_WHITEBALANCE,&q)!=-1||errno!=EBUSY||fields(fd,1,413,256,310))goto done;
 if(ioctl(owner,QC_ISP_END))goto done;close(owner);owner=-1;
 q=(struct qc_isp_whitebalance){.set=1,.preset=0};
 if(ioctl(fd,QC_ISP_WHITEBALANCE,&q)||fields(fd,0,396,258,498))goto done;
 result=0;puts("WB idle snapshot/manual/get/invalid/EFAULT/busy/get-held/stop/restore PASS; no pixels or AWB qualification");
 done:
 if(owner>=0){ioctl(owner,QC_ISP_END);close(owner);}
 q=(struct qc_isp_whitebalance){.set=1,.preset=0};
 if(ioctl(fd,QC_ISP_WHITEBALANCE,&q))result=1;
 close(fd);return result;
}
