/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded read-only120samples/10s; unchangedhardware/IRQ/clock/DMA ownership.
 */
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include "../isp-observer/qc07f_isp_observer.h"
static unsigned long ms(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1000UL+t.tv_nsec/1000000;}
int main(void)
{
 int fd=open("/dev/qc07f-isp-observer",O_RDONLY);if(fd<0)return 1;
 unsigned long start=ms();
 for(unsigned i=0;i<120&&ms()-start<10000;i++){
  struct qc_isp_observation o;
  if(ioctl(fd,QC_ISP_OBSERVE,&o)){close(fd);return 1;}
  printf("ISP_RO elapsed=%lu ctrl=%08x status=%08x busy14=%u done0=%u table=%08x order=%08x gate=%08x reset=%08x pll=%08x irqen=%08x parent_mask=%08x parent_pending=%08x\n",ms()-start,o.control,o.status,(o.status>>14)&1,o.status&1,o.table,o.order,o.clock_gate,o.reset,o.pll,o.isp_irq_enable,o.parent_mask,o.parent_pending);
  /* Priorobserver's nanosleep didnotdelay onthisminimalRAMimage. Use the
   * alreadyqualified poll timer and rejecterrors instead of busy-sampling.
   */
  if(poll(NULL,0,50)<0){close(fd);return 1;}
 }
 close(fd);return 0;
}
