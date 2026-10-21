/* SPDX-License-Identifier: GPL-2.0-only
 * TARGET fixture, never run on the host. idle changes desired RAM only;
 * owner-irq prepares hardware; owner-queue additionally captures one lease.
 * Parent must arrange idle camera ownership/recovery before running this.
 * Restores the initial desired configuration, including custom raw fields.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/watchdog.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "crc-bzip2.h"
#define PIXEL_BYTES ((1280U*720U+640U*360U)*3U/2U)

/* Same measured caller-owned sensor lifecycle as capture-queue-test.c.
 * QUEUE_BEGIN prepares ISP only: it does not start SC1346 input.
 */
static int sensor_read(int fd,uint16_t reg)
{
 uint8_t p[2]={reg>>8,reg},v=0;
 struct i2c_msg m[2]={{.addr=0x30,.len=2,.buf=p},{.addr=0x30,.flags=I2C_M_RD,.len=1,.buf=&v}};
 struct i2c_rdwr_ioctl_data q={m,2};return ioctl(fd,I2C_RDWR,&q)==2?v:-1;
}
static int sensor_stream(int fd,uint8_t on)
{
 uint8_t p[3]={1,0,on};return write(fd,p,3)==3&&sensor_read(fd,0x0100)==on?0:-1;
}

static int get(int fd,struct qc_isp_whitebalance_manual *q)
{
 *q=(struct qc_isp_whitebalance_manual){.version=1};
 return ioctl(fd,QC_ISP_WHITEBALANCE_MANUAL,q);
}
static int restore(int fd,const struct qc_isp_whitebalance_manual *saved)
{
 if(saved->preset<=QC_ISP_WB_RETAINED_MANUAL) {
  struct qc_isp_whitebalance p={.set=1,.preset=saved->preset};
  return ioctl(fd,QC_ISP_WHITEBALANCE,&p);
 }
 struct qc_isp_whitebalance_manual q={.version=1,.set=1};
 memcpy(q.gain,saved->gain,sizeof(q.gain));
 memcpy(q.offset_raw,saved->offset_raw,sizeof(q.offset_raw));
 return ioctl(fd,QC_ISP_WHITEBALANCE_MANUAL,&q);
}
static int same_desired(const struct qc_isp_whitebalance_manual *a,
                        const struct qc_isp_whitebalance_manual *b)
{
 return a->preset==b->preset && !memcmp(a->gain,b->gain,sizeof(a->gain)) &&
        !memcmp(a->offset_raw,b->offset_raw,sizeof(a->offset_raw));
}
static int same_prepared(const struct qc_isp_whitebalance_manual *a,
                         const struct qc_isp_whitebalance_manual *b)
{
 return a->table_prepared==b->table_prepared && a->last_prepared_preset==b->last_prepared_preset &&
        a->last_prepared_ns==b->last_prepared_ns &&
        !memcmp(a->last_prepared_gain,b->last_prepared_gain,sizeof(a->last_prepared_gain)) &&
        !memcmp(a->last_prepared_offset_raw,b->last_prepared_offset_raw,sizeof(a->last_prepared_offset_raw));
}
static int unchanged(int fd,const struct qc_isp_whitebalance_manual *before)
{
 struct qc_isp_whitebalance_manual after;
 return !get(fd,&after) && same_desired(before,&after) && same_prepared(before,&after) &&
        before->desired_changed_ns==after.desired_changed_ns && before->owner_active==after.owner_active;
}
static int reject(int fd,struct qc_isp_whitebalance_manual *q,int error,
                  const struct qc_isp_whitebalance_manual *before)
{
 return ioctl(fd,QC_ISP_WHITEBALANCE_MANUAL,q)==-1 && errno==error && unchanged(fd,before);
}
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL line %d: %s (errno=%d)\n",__LINE__,#x,errno);goto done;} } while(0)
int main(int argc,char **argv)
{
 int fd=-1,owner=-1,guard=-1,sensor=-1,sensor_touched=0,result=1,have_saved=0,owner_started=0;
 int queue=0,with_owner=0;
 long page=0;
 void *readonly=MAP_FAILED,*pixels=NULL;
 struct qc_isp_whitebalance_manual saved,before,q,after;
 struct qc_isp_whitebalance legacy;
 struct qc_isp_queue_frame frame={0};
 if(argc!=2 || (strcmp(argv[1],"idle") && strcmp(argv[1],"owner-irq") && strcmp(argv[1],"owner-queue"))) {
  fprintf(stderr,"usage: %s idle|owner-irq|owner-queue (TARGET ONLY; owner modes prepare ISP)\n",argv[0]);return 2;
 }
 queue=!strcmp(argv[1],"owner-queue");with_owner=strcmp(argv[1],"idle")!=0;
 fd=open("/dev/qc07f-frame",O_RDONLY);CHECK(fd>=0);
 CHECK(!get(fd,&saved));
 /* Never interfere with an existing owner or attempt cleanup writes to it. */
 CHECK(!saved.owner_active);have_saved=1;
 CHECK(saved.camera_id==2 && saved.preset<=QC_ISP_WB_CUSTOM);
 legacy=(struct qc_isp_whitebalance){.set=1,.preset=0};
 CHECK(!ioctl(fd,QC_ISP_WHITEBALANCE,&legacy));
 CHECK(legacy.gain[0]==396 && legacy.gain[1]==258 && legacy.gain[2]==498);
 CHECK(!get(fd,&before));
 q=(struct qc_isp_whitebalance_manual){.version=1,.set=1,.gain={401,257,311}};
 CHECK(!ioctl(fd,QC_ISP_WHITEBALANCE_MANUAL,&q));
 CHECK(q.preset==QC_ISP_WB_CUSTOM && same_prepared(&q,&before));
 CHECK(!get(fd,&before) && same_desired(&before,&q));
 legacy=(struct qc_isp_whitebalance){0};
 CHECK(!ioctl(fd,QC_ISP_WHITEBALANCE,&legacy));
 CHECK(legacy.preset==QC_ISP_WB_CUSTOM && !memcmp(legacy.gain,q.gain,sizeof(q.gain)));
 for(unsigned i=0;i<3;i++) {
  q=before;q.set=1;q.gain[i]=0;CHECK(reject(fd,&q,EINVAL,&before));
  q=before;q.set=1;q.gain[i]=4096;CHECK(reject(fd,&q,EINVAL,&before));
  q=before;q.set=1;q.offset_raw[i]=1024;CHECK(reject(fd,&q,EINVAL,&before));
 }
 q=before;q.set=2;CHECK(reject(fd,&q,EINVAL,&before));
 q=before;q.version=0;CHECK(reject(fd,&q,EINVAL,&before));
 q=before;q.reserved=1;CHECK(reject(fd,&q,EINVAL,&before));
 legacy=(struct qc_isp_whitebalance){.set=1,.preset=QC_ISP_WB_CUSTOM};
 CHECK(ioctl(fd,QC_ISP_WHITEBALANCE,&legacy)==-1 && errno==EINVAL && unchanged(fd,&before));
 CHECK(ioctl(fd,QC_ISP_WHITEBALANCE_MANUAL,(void *)1)==-1 && errno==EFAULT && unchanged(fd,&before));
 page=sysconf(_SC_PAGESIZE);CHECK(page>0 && (size_t)page>=sizeof(q));
 readonly=mmap(NULL,(size_t)page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 CHECK(readonly!=MAP_FAILED);
 q=before;q.set=1;q.gain[0]=402;
 memcpy(readonly,&q,sizeof(q));CHECK(!mprotect(readonly,(size_t)page,PROT_READ));
 CHECK(reject(fd,readonly,EFAULT,&before));
 CHECK(!munmap(readonly,(size_t)page));readonly=MAP_FAILED;
  if(with_owner) {
   int timeout=8;
   guard=open("/dev/watchdog0",O_WRONLY);CHECK(guard>=0);
   CHECK(!ioctl(guard,WDIOC_SETTIMEOUT,&timeout) && timeout==8);
   if(queue) {
    sensor=open("/dev/i2c-0",O_RDWR);CHECK(sensor>=0 && !ioctl(sensor,I2C_SLAVE,0x30));
    CHECK(sensor_read(sensor,0x3107)==0xda && sensor_read(sensor,0x3108)==0x4d);
    CHECK(sensor_read(sensor,0x3208)==5 && sensor_read(sensor,0x3209)==0 &&
          sensor_read(sensor,0x320a)==2 && sensor_read(sensor,0x320b)==0xd0);
    CHECK(sensor_read(sensor,0x0100)==0); /* Refuse preexisting unowned input. */
   }
  owner=open("/dev/qc07f-frame",O_RDONLY);CHECK(owner>=0);
  CHECK(!ioctl(owner,queue?QC_ISP_QUEUE_BEGIN:QC_ISP_BEGIN_IRQ));owner_started=1;
   if(queue) {
    sensor_touched=1;CHECK(!sensor_stream(sensor,1));
   pixels=malloc(PIXEL_BYTES);CHECK(pixels && (uintptr_t)pixels<=UINT32_MAX);
   frame.bytes=PIXEL_BYTES;frame.user_ptr=(__u32)(uintptr_t)pixels;
   CHECK(!ioctl(owner,QC_ISP_QUEUE_NEXT,&frame));
    CHECK(frame.token && frame.bytes==PIXEL_BYTES && !frame.guard_errors);
    CHECK(qc_crc_bzip2(pixels,PIXEL_BYTES)==frame.crc32_bzip2);
    printf("IMG-04 held frame bytes=%u sequence=%u guards=%u CRC_BZIP2=%08x\n",frame.bytes,frame.sequence,frame.guard_errors,frame.crc32_bzip2);
  }
  CHECK(!get(fd,&before) && before.owner_active && before.table_prepared);
  CHECK(before.last_prepared_preset==before.preset && before.last_prepared_ns>=before.desired_changed_ns);
  CHECK(!memcmp(before.last_prepared_gain,before.gain,sizeof(before.gain)));
  CHECK(!memcmp(before.last_prepared_offset_raw,before.offset_raw,sizeof(before.offset_raw)));
  q=before;q.set=1;q.gain[0]=402;CHECK(reject(fd,&q,EBUSY,&before));
  CHECK(reject(owner,&q,EBUSY,&before));
  legacy=(struct qc_isp_whitebalance){.set=1,.preset=0};
  CHECK(ioctl(fd,QC_ISP_WHITEBALANCE,&legacy)==-1 && errno==EBUSY && unchanged(fd,&before));
  legacy=(struct qc_isp_whitebalance){0};CHECK(!ioctl(fd,QC_ISP_WHITEBALANCE,&legacy));
  CHECK(legacy.preset==QC_ISP_WB_CUSTOM && legacy.gain[0]==401);
   if(queue)CHECK(!ioctl(owner,QC_ISP_QUEUE_RETURN,&frame));
   if(queue){CHECK(!sensor_stream(sensor,0));sensor_touched=0;}
  CHECK(!ioctl(owner,queue?QC_ISP_QUEUE_END:QC_ISP_END));owner_started=0;
  CHECK(!close(owner));owner=-1;
 }
 CHECK(!get(fd,&before));
 legacy=(struct qc_isp_whitebalance){.set=1,.preset=1};
 CHECK(!ioctl(fd,QC_ISP_WHITEBALANCE,&legacy));
 CHECK(!get(fd,&after) && after.preset==1 && after.gain[0]==413 && after.gain[1]==256 && after.gain[2]==310);
 CHECK(!after.offset_raw[0]&&!after.offset_raw[1]&&!after.offset_raw[2] && same_prepared(&before,&after));
 legacy=(struct qc_isp_whitebalance){.set=1,.preset=0};
 CHECK(!ioctl(fd,QC_ISP_WHITEBALANCE,&legacy));
 CHECK(!get(fd,&after) && after.preset==0 && after.gain[0]==396 && after.gain[1]==258 && after.gain[2]==498);
 CHECK(!after.offset_raw[0]&&!after.offset_raw[1]&&!after.offset_raw[2] && same_prepared(&before,&after));
 result=0;
done:
 if(readonly!=MAP_FAILED)munmap(readonly,(size_t)page);
 if(sensor>=0){if(sensor_touched && sensor_stream(sensor,0))result=1;close(sensor);}
 if(owner>=0) {
  if(owner_started && ioctl(owner,queue?QC_ISP_QUEUE_END:QC_ISP_END))result=1;
  close(owner);
 }
 free(pixels);
 if(have_saved) {
  if(restore(fd,&saved) || get(fd,&after) || !same_desired(&saved,&after)) {
   fputs("FAIL restoring original desired WB\n",stderr);result=1;
  }
 }
 if(fd>=0)close(fd);
 if(guard>=0){if(write(guard,"V",1)!=1)result=1;close(guard);}
 if(!result)printf("IMG-04 %s raw desired/custom/legacy/fault/restore fixture PASS; not colour calibration or AWB\n",argv[1]);
 return result;
}
