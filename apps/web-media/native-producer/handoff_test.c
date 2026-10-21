/* SPDX-License-Identifier: GPL-2.0-only
 * HOST synthetic bytes only. NOT camera, encoded-receipt or decode proof. */
#include "handoff.h"
#include "h264-baseline.h"
#include "crc-bzip2.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t producer[CAMERA_FMP4_MAX_NAL_BYTES],hub[CAMERA_FMP4_MAX_NAL_BYTES];
enum fault {NONE,EOF_NEXT,EFAULT_NEXT,RETIRE_FAIL,RELEASE_FAIL,SOURCE,FILE_ID,GENERATION,
 SEQUENCE,NOT_IDR,NAL_BAD,ZERO_BYTES,OVERSIZE,SAME_TIME,BACK_TIME,WIDE_TIME,
 CLOCK_FAIL,CLOCK_BACK,CANCEL,WRITE_FAIL,PARTIAL_FAIL,ZERO_WRITE,BAD_COUNT,
 BUDGET,SESSION_EXPIRE,OUTPUT_EXPIRE,LATE_PARTIAL_FAIL,
 MUTATE_FIRST,MUTATE_LATE,MUTATE_LAST,MUTATE_EOF,SOURCE_CRC_BAD,
 EARLY_NEXT_STORAGE_FAIL,FUTURE_NEXT_STORAGE_FAIL,PTR_OVERFLOW,ALIAS_HUB,
 TIMESTAMP_MAX,TIMESTAMP_NEAR_MAX,INITIAL_CLOCK_FAIL,BUSY_OUTPUT_TIMEOUT,ROLLBACK_FAIL};
struct mock {enum fault fault;unsigned n,held,releases,retire_calls,rollback_calls,writes;
 uint64_t clock;size_t accepted,rolled_to;unsigned checks,fenced;};
static uint64_t clock_now(void *v)
{
 struct mock *m=v;
 if(m->fault==INITIAL_CLOCK_FAIL)return UINT64_MAX;
 if(m->n==2&&m->fault==CLOCK_FAIL)return UINT64_MAX;
 if(m->n==2&&m->fault==CLOCK_BACK)return 1;
 m->clock+=100;
 if(m->n==2&&m->fault==SESSION_EXPIRE)return NH_SESSION_NS+1000000000ULL;
 if(m->writes>2&&m->fault==OUTPUT_EXPIRE)m->clock+=NH_OUTPUT_NS;
 return m->clock;
}
static int cancel(void *v){struct mock *m=v;return m->fault==CANCEL&&m->n==2;}
static int next(void *v,struct camera_packet *p,struct nh_key *k)
{
 struct mock *m=v;assert(!m->held);assert(m->n<NH_PRODUCED_LIMIT);
 if((m->fault==EARLY_NEXT_STORAGE_FAIL&&!m->n)||(m->fault==FUTURE_NEXT_STORAGE_FAIL&&m->n==1)){
  m->held=1;m->n++;
  *p=(struct camera_packet){.data=(const uint8_t *)(uintptr_t)1,.bytes=SIZE_MAX,.monotonic_ns=UINT64_MAX};
  *k=(struct nh_key){0,0,0,UINT32_MAX,0};return CAMERA_IO_ERROR;
 }
 if(m->n){
  /* Native order must leave prior hub bytes unchanged while producer storage
   * (poisoned on release) is reused for next encode. Full scalar CRC oracle. */
  uint32_t before=qc_crc_bzip2(hub,137004);
  assert(hub[1]==m->n);memset(producer,0xee,sizeof(producer));
   assert(before==qc_crc_bzip2(hub,137004));m->checks++;
   /* Deliberate corruption AFTER the model assertion: only production nh_run
    * can reject it. Modify the last old-NAL byte, not just the NAL header. */
   if((m->fault==MUTATE_FIRST&&m->n==1)||(m->fault==MUTATE_LATE&&m->n==2)||
      (m->fault==MUTATE_LAST&&m->n==12)||(m->fault==MUTATE_EOF&&m->n==1))hub[137003]^=1;
  }
 if(m->fault==MUTATE_EOF&&m->n==1){
  *p=(struct camera_packet){.data=(const uint8_t *)(uintptr_t)1,.bytes=SIZE_MAX};return CAMERA_IO_ERROR;
 }
 if(m->n==1&&(m->fault==EOF_NEXT||m->fault==EFAULT_NEXT))return CAMERA_IO_ERROR;
 m->held=1;m->n++;memset(producer,(int)m->n,137004);producer[0]=m->n==1?0x65:0x41;
 *p=(struct camera_packet){.codec=CAMERA_H264,.keyframe=m->n==1,.data=producer,.bytes=137004,
 .monotonic_ns=1000000000ULL+(m->n-1)*450000000ULL};
 *k=(struct nh_key){101,202,303,m->n,0};
 if(m->n==1&&m->fault==NOT_IDR){p->keyframe=0;producer[0]=0x41;}
 if(m->fault==BUDGET){p->bytes=1048576;memset(producer,(int)m->n,p->bytes);producer[0]=p->keyframe?0x65:0x41;}
 k->nal_crc32_bzip2=qc_crc_bzip2(producer,p->bytes);
 if(m->fault==SOURCE_CRC_BAD&&m->n==1)k->nal_crc32_bzip2^=1;
 if(m->fault==PTR_OVERFLOW){p->data=(const uint8_t *)(UINTPTR_MAX-1);p->bytes=4;}
 if(m->fault==ALIAS_HUB)p->data=hub;
 if(m->fault==TIMESTAMP_MAX)p->monotonic_ns=UINT64_MAX;
 if(m->fault==TIMESTAMP_NEAR_MAX)p->monotonic_ns=UINT64_MAX-6000000000ULL+(m->n-1)*450000000ULL;
 if(m->n==2)switch(m->fault){
 case SOURCE:k->source++;break;case FILE_ID:k->file++;break;case GENERATION:k->generation++;break;
 case SEQUENCE:k->sequence--;break;case NAL_BAD:producer[0]=0x67;break;
 case ZERO_BYTES:p->bytes=0;break;case OVERSIZE:p->bytes=1048577;break;
 case SAME_TIME:p->monotonic_ns=1000000000;break;case BACK_TIME:p->monotonic_ns=999999999;break;
 case WIDE_TIME:p->monotonic_ns=4000000000;break;default:break;
 }
 return CAMERA_OK;
}
static int release(void *v,const struct camera_packet *p)
{
 struct mock *m=v;assert(m->held);
 if(m->fault==PTR_OVERFLOW||m->fault==ALIAS_HUB)return CAMERA_IO_ERROR;
 assert(p->data==producer);
 if(m->fault==RELEASE_FAIL)return CAMERA_IO_ERROR;
 memset(producer,0xcc,sizeof(producer));m->held=0;m->releases++;return CAMERA_OK;
}
static int retire(void *v)
{
 struct mock *m=v;assert(!m->retire_calls++);
 if(m->fault==EARLY_NEXT_STORAGE_FAIL||m->fault==FUTURE_NEXT_STORAGE_FAIL){assert(m->held);return CAMERA_IO_ERROR;}
 assert(!m->held);
 return m->fault==RETIRE_FAIL||m->fault==EFAULT_NEXT||m->fault==CLOCK_FAIL||m->fault==CLOCK_BACK?CAMERA_IO_ERROR:CAMERA_OK;
}
static int write_mock(void *v,const uint8_t *p,size_t n,size_t *accepted)
{
 struct mock *m=v;(void)p;assert(n&&n<=8192);m->writes++;
 if(m->fault==BUSY_OUTPUT_TIMEOUT){m->clock+=NH_OUTPUT_NS/2;*accepted=0;return CAMERA_BUSY;}
 *accepted=n>37?37:n;
 if(m->fault==LATE_PARTIAL_FAIL&&m->accepted>=644+112+137004){m->accepted+=*accepted;return CAMERA_IO_ERROR;}
 if(m->writes==2)switch(m->fault){
 case WRITE_FAIL:*accepted=0;return CAMERA_IO_ERROR;
 case PARTIAL_FAIL:m->accepted+=*accepted;return CAMERA_IO_ERROR;
 case ZERO_WRITE:*accepted=0;return CAMERA_OK;
 case BAD_COUNT:*accepted=n+1;return CAMERA_OK;
 default:break;
 }
 m->accepted+=*accepted;return CAMERA_OK;
}
static int rollback(void *v,size_t n)
{struct mock *m=v;m->rollback_calls++;m->rolled_to=n;return m->fault==ROLLBACK_FAIL?CAMERA_IO_ERROR:CAMERA_OK;}
static void fence(void *v){struct mock *m=v;assert(!m->fenced++);}
int main(void)
{
 uint8_t params[256];size_t n=qc_h264_parameter_sets(params),split=4;
 while(split+4<n&&memcmp(params+split,"\0\0\0\1",4))split++;
 struct camera_fmp4_config config={.sps=params+4,.sps_bytes=split-4,.pps=params+split+4,.pps_bytes=n-split-4};
 unsigned cases=0;
 for(enum fault f=NONE;f<ROLLBACK_FAIL;f++){
  struct mock m={.fault=f,.clock=1000000000};struct nh_report r;
  struct nh_backend b={&m,next,release,retire,clock_now,cancel,write_mock,rollback,fence,0,NULL,NULL,NULL};
  int result=nh_run(&b,&config,hub,sizeof(hub),&r);
  if(f==NONE||f==TIMESTAMP_NEAR_MAX){
   assert(result==CAMERA_OK&&r.produced==13&&r.output==12&&r.boundary_retired&&r.retired&&!r.quarantined);
   assert(m.releases==13&&!m.held&&m.checks==12&&m.retire_calls==1&&!m.rollback_calls);
   assert(r.committed==644+12*(112+137004)&&r.committed==r.accepted&&r.accepted==m.accepted);
   assert(hub[1]==12); /* final boundary AU13 was retired, not published/exported */
   for(unsigned i=0;i<12;i++){
    assert(r.samples[i].key.sequence==i+1&&r.samples[i].key.generation==303);
    assert(r.samples[i].next_source_ns-r.samples[i].source_ns==450000000);
    assert(r.samples[i].next_end_ns>=r.samples[i].next_begin_ns&&r.samples[i].output_end_ns>=r.samples[i].output_begin_ns);
    assert(r.samples[i].wire_bytes==112+137004);
    assert(r.integrity[i].after_checked&&r.integrity[i].source_crc==r.integrity[i].before_crc&&
           r.integrity[i].before_crc==r.integrity[i].after_crc);
   }
  }else{
   assert(result!=CAMERA_OK);assert(r.output<=12&&r.produced<=13);
   assert(r.committed<=NH_OUTPUT_LIMIT);assert(m.retire_calls<=1);
   if(f==RETIRE_FAIL||f==EFAULT_NEXT||f==CLOCK_FAIL||f==CLOCK_BACK||f==RELEASE_FAIL)assert(r.quarantined);
   if(f==RELEASE_FAIL){assert(m.held&&!m.retire_calls);}
   if(f==BUDGET){assert(r.output==1&&r.produced==3&&r.retired);}
   if(f==EOF_NEXT){assert(r.output==0&&r.produced==1&&m.releases==1&&r.retired);}
   if(f==LATE_PARTIAL_FAIL){assert(r.output==1&&r.committed==644+112+137004&&m.rolled_to==r.committed&&m.accepted>r.committed);}
   if(f==MUTATE_FIRST||f==MUTATE_LATE||f==MUTATE_LAST||f==MUTATE_EOF){
    unsigned prior=f==MUTATE_LATE?1:f==MUTATE_LAST?11:0;
    assert(r.quarantined&&!r.retired&&!m.retire_calls&&m.fenced==1&&r.retained_hub);
    assert(r.output==prior&&r.committed==644+prior*(112+137004)&&m.accepted==r.committed);
    assert(r.integrity[prior].after_checked&&r.integrity[prior].after_crc!=r.integrity[prior].before_crc);
    assert(m.releases==prior+1&&!r.boundary_retired);
    if(f!=MUTATE_EOF)assert(m.held&&r.retained_producer);
   }
   if(f==SOURCE_CRC_BAD){assert(r.quarantined&&m.held&&!m.releases&&!m.retire_calls&&m.fenced==1&&r.committed==644);}
   if(f==EARLY_NEXT_STORAGE_FAIL||f==FUTURE_NEXT_STORAGE_FAIL){
    assert(r.quarantined&&r.failed_next&&m.held&&m.fenced==1&&m.retire_calls==1);
    assert(r.output==0&&m.releases==(f==EARLY_NEXT_STORAGE_FAIL?0:1));
   }
   if(f==INITIAL_CLOCK_FAIL)assert(r.quarantined&&m.fenced==1&&!m.n&&!m.retire_calls&&!m.accepted);
   if(f==BUSY_OUTPUT_TIMEOUT)assert(m.writes==2&&r.output==0&&r.accepted==0&&r.retired);
  }
  cases++;
 }
 /* Actual rollback failure, atomic no-retry quarantine. */
 struct mock m={.fault=ROLLBACK_FAIL,.clock=1000000000};struct nh_report r;
 struct nh_backend b={&m,next,release,retire,clock_now,cancel,write_mock,rollback,fence,0,NULL,NULL,NULL};
 /* Force config mismatch after acquiring one complete AU. */
 config.generation=304;assert(nh_run(&b,&config,hub,sizeof(hub),&r)!=CAMERA_OK);
 assert(r.quarantined&&!r.retired&&!m.retire_calls&&m.rollback_calls==1&&!m.held);cases++;
 /* Earlier connect/start time is included, not a renewed10s source budget. */
 struct mock early={.clock=NH_SESSION_NS+1000000000ULL};struct nh_backend eb={&early,next,release,retire,clock_now,cancel,write_mock,rollback,fence,1000000000ULL,NULL,NULL,NULL};
 config.generation=0;assert(nh_run(&eb,&config,hub,sizeof(hub),&r)!=CAMERA_OK);
 assert(!early.n&&!early.accepted&&r.retired);cases++;
 puts("HOST synthetic native-handoff: progress/borrowed CPU preservation/partial accounting/clock/namespace/generation/EOF/failure/quarantine PASS");
 printf("cases=%u NOT_CAMERA_TARGET\n",cases);return 0;
}
