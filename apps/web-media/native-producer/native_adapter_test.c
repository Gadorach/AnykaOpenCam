/* SPDX-License-Identifier: GPL-2.0-only
 * HOST synthetic syscall backend, no device/files/network/footage operations.
 * Include the exact candidate adapter so mocks can address static CPU buffers
 * without pretending that its fixed ARM32 user_ptr is a host pointer. */
#include "native_adapter.c"
#include "sensor_baseline.c"
#include "memory_fixture.h"
#include <assert.h>
#include <stdarg.h>
enum injected {GOOD,BIND_ERR,BIND_KEY,ARM_ERR,ARM_KEY,BEGIN_ERR,QUERY_ERR,
 QUERY_KEY,NEXT_ERR,RAW_CRC,RAW_GUARD,ENCODE_ERR,LIFETIME_OLD,LIFETIME_FAULT,
 END_ERR,END_QUERY_ERR,ACK_ERR,CLOSE_ERR,CLOCK_ERR,NO_IDR,OUTPUT_SIZE,POST_ENCODE_EFAULT,POST_ENCODE_EXPIRY,RETIRE_EXPIRY,
 OPTIN_ABSENT,WRONG_ID,SENSOR_ALREADY_ON,TABLE_FAIL,TABLE_TIMEOUT,READBACK_FAIL,WRONG_HTS,RESET_DELAY_FAIL,GUARD_WRONG,
 WRONG_ID_LOW,RESET_DELAY_SHORT,READBACK_TIMEOUT,TABLE_CLOCK_BACK};
static struct {
 enum injected fault;unsigned opens,closes,nexts,encodes,queries,acks,ends,freed;
 unsigned state,sensor,watchdog_active,table_writes,reset_sleeps,baseline_reads;
 uint16_t pointer;uint64_t clock;int allocated;
} mock;
static uint8_t synthetic_output[1048576];
static uint8_t sensor_registers[65536]; /* HOST synthetic register model only */
static uint8_t integrated_hub[1048576];
static size_t discarded_bytes;
static unsigned discarded_failures;
enum pressure {PRESSURE_GOOD,MISSING_MEM,LOW_INITIAL,LOW_POST,LOW_HELD,LOW_NONCMA_POST,BAD_WATERMARK,NEW_POPULATED_ZONE,SAMPLE_OLD,PREF_MALLOC_FAIL,CANCEL_POST,CLOCK_POST};
static enum pressure pressure;
static const char *test_storage_mode="separate";
enum alias_fault {ALIAS_GOOD,ALIAS_EARLY_WRITE,ALIAS_PARTIAL_EARLY_WRITE,ALIAS_OUTPUT_EFAULT,ALIAS_GUARD_ERROR,ALIAS_MAX_NAL};
static enum alias_fault alias_fault;
static uint8_t modeled_kernel_input[1382400]; /* HOST only: existing kernel DMA input, not target CPU storage */
static uint32_t modeled_main_crc,modeled_copy_crc;
static unsigned input_correlations;
static unsigned low_before_encode;
static int discard_output(void *c,const uint8_t *p,size_t n,size_t *accepted)
{(void)c;(void)p;assert(n&&n<=8192);*accepted=n;discarded_bytes+=n;return 0;}
static int discard_rollback(void *c,size_t n){(void)c;(void)n;assert(0);return -1;}
static int partial_sink_failure(void *c,const uint8_t *p,size_t n,size_t *accepted)
{(void)c;(void)p;assert(n>=3);*accepted=3;return CAMERA_IO_ERROR;}
static int close_failed_sink(void *c,size_t n){(void)c;assert(n==0||n==644);discarded_failures++;return 0;}
int __wrap_open(const char *path,int flags,...)
{
 (void)flags;mock.opens++;
 if(!strcmp(path,"/dev/qc07f-encoder"))return 10+(int)mock.opens;
 if(!strcmp(path,"/dev/qc07f-frame"))return 100;
 if(!strcmp(path,"/dev/i2c-0"))return 101;
 if(!strcmp(path,"/dev/watchdog0")){mock.watchdog_active=1;return 102;}
 assert(0);return -1;
}
int __wrap_close(int fd)
{(void)fd;mock.closes++;if(mock.fault==CLOSE_ERR&&mock.ends)return -1;return 0;}
ssize_t __wrap_write(int fd,const void *p,size_t n)
{
 assert(fd==102&&n==1);
 if(*(const char *)p=='V')mock.watchdog_active=0;
 return (ssize_t)n;
}
void *__wrap_malloc(size_t n)
 {assert(n==sizeof(synthetic_output)&&!mock.allocated);if(pressure==PREF_MALLOC_FAIL)return NULL;mock.allocated=1;return synthetic_output;}
int __wrap_nh_memory_read_fixed(unsigned kind,char *p,size_t cap,size_t *bytes)
{
 const char *text=kind?"Node 0, zone Normal\n pages free 8286\n boost 1024\n min 1202\n low 1246\n high 1290\n promo 1334\n spanned 10240\n present 10240\n managed 8286\n cma 4096\n protection: (0, 0)\n nr_free_cma 4096\nNode 0, zone Movable\n pages free 0\n boost 0\n min 32\n low 32\n high 32\n promo 32\n spanned 0\n present 0\n managed 0\n cma 0\n protection: (0, 0)\n":
  "MemTotal: 33144 kB\nMemFree: 33144 kB\nMemAvailable: 30000 kB\nCmaTotal: 16384 kB\nCmaFree: 16384 kB\nSwapTotal: 0 kB\n";size_t n=strlen(text);assert(n<cap);
 if(!kind){
  if(pressure==MISSING_MEM)text="MemFree: 19040 kB\n";
   if((low_before_encode&&mock.nexts&&!mock.encodes)||pressure==LOW_INITIAL||(pressure==LOW_POST&&mock.encodes)||(pressure==LOW_HELD&&mock.encodes&&memory_state.count&&memory_state.observations[memory_state.count-1].stage==NH_MEM_POST_FIRST))
   text="MemTotal: 33144 kB\nMemFree: 19040 kB\nMemAvailable: 1000 kB\nCmaTotal: 16384 kB\nCmaFree: 9216 kB\nSwapTotal: 0 kB\n";
  if(pressure==LOW_NONCMA_POST&&mock.encodes)text="MemTotal: 33144 kB\nMemFree: 14000 kB\nMemAvailable: 13972 kB\nCmaTotal: 16384 kB\nCmaFree: 9216 kB\nSwapTotal: 0 kB\n";
 }
 n=strlen(text);memcpy(p,text,n);
 if(kind&&pressure==BAD_WATERMARK){char *q=strstr(p,"high 1290");assert(q);q[8]='1';}
 if(kind&&pressure==NEW_POPULATED_ZONE){const char *extra="Node 1, zone Normal\n";memcpy(p+n,extra,strlen(extra));n+=strlen(extra);}
 if(pressure==SAMPLE_OLD)mock.clock+=NH_MEMORY_MAX_SAMPLE_NS;
 if(mock.encodes&&pressure==CANCEL_POST)stop=1;
 if(mock.encodes&&pressure==CLOCK_POST)mock.clock=1;
 *bytes=n;return 0; /* never opens HOST/target proc */
}
 void __wrap_free(void *p)
{if(!p)return;assert(p==synthetic_output&&mock.allocated);if(parked_after_stop==p)assert(mock.ends==1&&mock.acks==1);mock.allocated=0;mock.freed++;}
int __wrap_clock_gettime(clockid_t id,struct timespec *t)
{
 assert(id==CLOCK_MONOTONIC);if(mock.fault==CLOCK_ERR)return -1;
 mock.clock+=1000000;t->tv_sec=(time_t)(mock.clock/1000000000);t->tv_nsec=(long)(mock.clock%1000000000);return 0;
}
int __wrap_nanosleep(const struct timespec *t,struct timespec *remaining)
{
 assert(t->tv_sec==0&&t->tv_nsec==10000000&&remaining==NULL&&mock.watchdog_active&&mock.table_writes==1);
 mock.reset_sleeps++;if(mock.fault==RESET_DELAY_FAIL)return -1;
 mock.clock+=mock.fault==RESET_DELAY_SHORT?5000000:10000000;return 0;
}
static int fail(void){errno=EFAULT;return -1;}
int __wrap_ioctl(int fd,unsigned long request,...)
{
 (void)fd;va_list ap;va_start(ap,request);
 /* No third argument exists for BEGIN/END. */
 void *arg=NULL;
 if(request==I2C_SLAVE){assert(va_arg(ap,int)==0x30);va_end(ap);return 0;}
 if(request!=QC_ISP_BEGIN&&request!=QC_ISP_BEGIN_IRQ&&request!=QC_ISP_END)arg=va_arg(ap,void *);
 va_end(ap);
 if(request==QC_HWENC_LIFETIME_QUERY){
  if(mock.fault==LIFETIME_OLD){errno=ENOTTY;return -1;}
  struct qc_hwenc_lifetime *q=arg;*q=(struct qc_hwenc_lifetime){.version=1,.struct_bytes=sizeof(*q),.session_sequence=1,.live_bytes=8454144};
  if(mock.fault==LIFETIME_FAULT)q->dma_uncertain=1;
  return 0;
 }
 if(request==WDIOC_SETTIMEOUT){assert(*(int *)arg==5&&mock.watchdog_active);return 0;}
 if(request==WDIOC_GETTIMEOUT){assert(fd==102&&mock.watchdog_active);*(int *)arg=mock.fault==GUARD_WRONG?4:5;return 0;}
 if(request==I2C_RDWR){
  struct i2c_rdwr_ioctl_data *q=arg;assert(q->nmsgs==1&&q->msgs[0].addr==0x30);
  struct i2c_msg *m=q->msgs;
  if(!m->flags&&m->len==2){mock.pointer=(uint16_t)(m->buf[0]*256U+m->buf[1]);return 1;}
  if(m->flags==I2C_M_RD){
   assert(m->len==1);uint8_t value=sensor_registers[mock.pointer];
   if(mock.table_writes==139&&!sensor_baseline_report.ready){
    if(mock.fault==READBACK_FAIL&&mock.baseline_reads==40)return fail();
    if(mock.fault==READBACK_TIMEOUT&&mock.baseline_reads==40)mock.clock+=NH_OUTPUT_NS;
    mock.baseline_reads++;
    if(mock.fault==WRONG_HTS&&mock.pointer==0x320c)value=6;
    if(mock.fault==WRONG_HTS&&mock.pointer==0x320d)value=0x72;
   }
   *m->buf=value;return 1;
  }
  assert(!m->flags&&m->len==3);
  uint16_t reg=(uint16_t)(m->buf[0]*256U+m->buf[1]);uint8_t value=m->buf[2];
  if(!sensor_baseline_report.ready){
   assert(mock.watchdog_active&&mock.state==QC_RECEIPT_ARMED&&mock.table_writes<139);
   assert(reg==sc1346_init_table[mock.table_writes].reg&&value==sc1346_init_table[mock.table_writes].value);
   if(mock.fault==TABLE_FAIL&&mock.table_writes==70)return fail();
   mock.table_writes++;
   if(mock.fault==TABLE_TIMEOUT&&mock.table_writes==71)mock.clock+=NH_OUTPUT_NS;
   if(mock.fault==TABLE_CLOCK_BACK&&mock.table_writes==71)mock.clock=1;
  }else{
   assert(reg==0x0100&&value<=1&&mock.state==QC_RECEIPT_ACTIVE);
  }
  sensor_registers[reg]=reg==0x0103?0:value;
  if(reg==0x0100)mock.sensor=value;
  return 1;
 }
 if(request==QC_ISP_RECEIPT_BIND){
  struct qc_capture_receipt *q=arg;assert(q->version==1&&!q->source_instance&&!q->file_instance&&!q->generation);
  if(mock.fault==BIND_ERR){memset(q,0xff,sizeof(*q));return fail();}
  *q=qc_receipt_request(NULL);q->source_instance=101;q->file_instance=202;
  if(mock.fault==BIND_KEY)q->generation=1;return 0;
 }
 if(request==QC_ISP_RECEIPT_ARM){
  struct qc_capture_receipt *q=arg;assert(q->source_instance==101&&q->file_instance==202&&!q->generation);
  if(mock.fault==ARM_ERR){memset(q,0xff,sizeof(*q));return fail();}
  q->generation=303;q->state=QC_RECEIPT_ARMED;mock.state=QC_RECEIPT_ARMED;
  if(mock.fault==ARM_KEY)q->file_instance++;return 0;
 }
 if(request==QC_ISP_BEGIN||request==QC_ISP_BEGIN_IRQ){
  assert(mock.state==QC_RECEIPT_ARMED&&sensor_baseline_report.ready&&mock.table_writes==139&&mock.baseline_reads==139&&!mock.sensor&&mock.reset_sleeps==1);
  if(mock.fault==BEGIN_ERR)return fail();
  mock.state=QC_RECEIPT_ACTIVE;return 0;
 }
 if(request==QC_ISP_RECEIPT_QUERY){
  mock.queries++;struct qc_capture_receipt *q=arg;
  assert(q->source_instance==101&&q->file_instance==202&&q->generation==303);
  if(mock.fault==QUERY_ERR||(mock.fault==END_QUERY_ERR&&mock.ends)||
     (mock.fault==POST_ENCODE_EFAULT&&mock.encodes)){memset(q,0xff,sizeof(*q));return fail();}
  q->state=mock.state;
  if(mock.ends){q->reason=QC_STOP_END;q->stopped_ns=1000000000;}
  if(mock.fault==QUERY_KEY)q->source_instance++;
  if(mock.fault==POST_ENCODE_EXPIRY&&mock.encodes){q->state=QC_RECEIPT_RETIRED;q->reason=QC_STOP_EXPIRY;q->stopped_ns=1000000000;}
  if(mock.fault==RETIRE_EXPIRY&&mock.state==QC_RECEIPT_RETIRED){q->reason=QC_STOP_EXPIRY;q->stopped_ns=1000000000;}
  return 0;
 }
 if(request==QC_ISP_NEXT){
  assert(mock.state==QC_RECEIPT_ACTIVE&&mock.sensor);
  mock.nexts++;struct qc_isp_next *q=arg;
  assert(q->bytes==sizeof(raw)&&q->user_ptr==(uint32_t)(uintptr_t)raw);
  if(mock.fault==NEXT_ERR){memset(q,0xff,sizeof(*q));return fail();}
  assert(!native.produced_storage);memset(raw,(int)mock.nexts,sizeof(raw));q->sequence=mock.nexts;q->status=1;
  modeled_main_crc=qc_crc_bzip2(raw,1382400);
  q->crc32_bzip2=qc_crc_bzip2(raw,sizeof(raw));
  if(mock.fault==RAW_CRC)q->crc32_bzip2^=1;
  if(mock.fault==RAW_GUARD)q->guard_errors=1;return 0;
 }
 if(request==QC_HWENC_H264_STREAM){
  mock.encodes++;struct qc_hwenc_h264_stream_job *q=arg;
   assert((mock.allocated||raw_reuse_requested)&&q->frame.width==1280&&q->frame.height==720&&q->frame.input_bytes==1382400);
   uint8_t *destination=raw_reuse_requested?raw:synthetic_output;
   assert(q->frame.output_capacity==1048576&&q->frame.output_ptr==(uint32_t)(uintptr_t)destination);
   assert(q->frame.input_ptr==(uint32_t)(uintptr_t)raw);
  assert(q->keyframe==(mock.encodes==1));
   if(mock.fault==ENCODE_ERR){memset(q,0xff,sizeof(*q));return fail();}
   /* Model EXACT audited source order: all input copied privately before ASIC,
    * reset/guards, then output overlays only userspace backing. Deliberate early
    * overlap destroys input correlation and cannot be accepted as this contract. */
   if(alias_fault==ALIAS_EARLY_WRITE||alias_fault==ALIAS_PARTIAL_EARLY_WRITE){
    size_t n=alias_fault==ALIAS_EARLY_WRITE?1048576:19;memset(destination,0xff,n);
   }
   memcpy(modeled_kernel_input,raw,sizeof(modeled_kernel_input));modeled_copy_crc=qc_crc_bzip2(modeled_kernel_input,sizeof(modeled_kernel_input));
   if(modeled_copy_crc!=modeled_main_crc){assert(raw_reuse_requested);return fail();}input_correlations++;
   if(alias_fault==ALIAS_GUARD_ERROR)return fail();
   size_t nal_size=alias_fault==ALIAS_MAX_NAL?1048576:137004;
   memset(destination,(int)mock.encodes,nal_size);destination[0]=q->keyframe?0x65:0x41;
   if(alias_fault==ALIAS_OUTPUT_EFAULT){memset(q,0xff,sizeof(*q));return fail();}
   q->frame.output_bytes=137004;q->frame.status=4;q->frame_num=mock.encodes-1;
   if(alias_fault==ALIAS_MAX_NAL)q->frame.output_bytes=1048576;
   if(mock.fault==NO_IDR)destination[0]=0x41;
  if(mock.fault==OUTPUT_SIZE)q->frame.output_bytes=1048577;return 0;
 }
 if(request==QC_ISP_END){
    assert(!mock.sensor&&(!mock.allocated||parked_after_stop==synthetic_output||(!native.produced_storage&&native.resident_storage==synthetic_output))&&mock.state==QC_RECEIPT_ACTIVE);mock.ends++;
  if(mock.fault==END_ERR)return fail();mock.state=QC_RECEIPT_RETIRED;return 0;
 }
 if(request==QC_ISP_RECEIPT_ACK){
  const struct qc_capture_receipt *q=arg;mock.acks++;
  assert(mock.state==QC_RECEIPT_RETIRED&&q->generation==303);
  if(mock.fault==ACK_ERR)return fail();return 0;
 }
 assert(0);return -1;
}
int main(void)
{
 unsigned cases=0;
 for(enum injected f=GOOD;f<=TABLE_CLOCK_BACK;f++){
  memset(&mock,0,sizeof(mock));mock.fault=f;mock.clock=1000000000;
  memset(&expected,0,sizeof(expected));memset(&source_clock,0,sizeof(source_clock));stop=0;frame_count=0;
  memset(sensor_registers,0,sizeof(sensor_registers));sensor_registers[0x3107]=f==WRONG_ID?0:0xda;sensor_registers[0x3108]=f==WRONG_ID_LOW?0:0x4d;
  sensor_registers[0x3208]=5;sensor_registers[0x320a]=2;sensor_registers[0x320b]=0xd0;
  sensor_registers[0x320c]=6;sensor_registers[0x320d]=0x72;sensor_registers[0x320e]=2;sensor_registers[0x320f]=0xee;
  if(f==SENSOR_ALREADY_ON){mock.sensor=1;sensor_registers[0x0100]=1;}
  nh_native_init();assert(!nh_native_storage_select(test_storage_mode));int memory_init=nh_native_memory_start(nh_native_now(NULL));
  if(f==CLOCK_ERR)assert(memory_init!=CAMERA_OK);else assert(!memory_init);
  if(f!=OPTIN_ABSENT&&f!=CLOCK_ERR)assert(!nh_native_optin_exact139());
  struct camera_packet p={0};struct nh_key key={0};
  int r=nh_native_next(NULL,&p,&key);
  if(f==OPTIN_ABSENT){assert(r==CAMERA_NOT_READY&&!mock.opens&&!mock.table_writes&&!native.retirement_uncertain);cases++;continue;}
  if(f==GOOD||f==END_ERR||f==END_QUERY_ERR||f==ACK_ERR||f==CLOSE_ERR||f==RETIRE_EXPIRY){
   assert(!r&&key.source==101&&key.file==202&&key.generation==303&&key.sequence==1);
   assert(key.nal_crc32_bzip2==qc_crc_bzip2(p.data,p.bytes));
   assert(p.bytes==137004&&p.keyframe&&mock.allocated);
   assert(sensor_baseline_report.ready&&sensor_baseline_report.written==139&&sensor_baseline_report.read_count==139&&!sensor_baseline_report.mismatches);
   assert(sensor_registers[0x320c]==7&&sensor_registers[0x320d]==8&&mock.reset_sleeps==1);
   assert(!nh_native_release(NULL,&p)&&mock.allocated); /* resident mapping survives loan release */
   if(f==GOOD){
    for(unsigned i=2;i<=13;i++){
     assert(!nh_native_next(NULL,&p,&key)&&key.sequence==i&&!p.keyframe);
     assert(!nh_native_release(NULL,&p));
    }
    assert(!nh_native_retire(NULL)&&mock.ends==1&&mock.acks==1&&mock.freed==1);
    assert(!native.receipt_armed&&!native.retirement_uncertain&&native.continuous_frame<0&&native.h264_stream_fd<0);
   }else{
    if(f==RETIRE_EXPIRY)mock.state=QC_RECEIPT_RETIRED;
    assert(nh_native_retire(NULL)&&native.retirement_uncertain);
    if(f==RETIRE_EXPIRY)assert(!mock.ends&&!mock.acks);
    unsigned ends=mock.ends,acks=mock.acks,closes=mock.closes;
    assert(nh_native_retire(NULL)&&mock.ends==ends&&mock.acks==acks&&mock.closes==closes);
   }
  }else{
   assert(r&&native.retirement_uncertain&&!mock.ends&&!mock.acks);
   if(f==ENCODE_ERR||f==NO_IDR||f==OUTPUT_SIZE||f==POST_ENCODE_EFAULT||f==POST_ENCODE_EXPIRY)assert(mock.allocated&&native.produced_storage==synthetic_output&&!mock.freed);
   unsigned opens=mock.opens,nexts=mock.nexts;
   assert(nh_native_next(NULL,&p,&key)&&mock.opens==opens&&mock.nexts==nexts);
   assert(nh_native_retire(NULL)&&!mock.ends&&!mock.acks);
   if(f>=WRONG_ID){
    assert(mock.watchdog_active&&native.watchdog_hold==102&&!mock.nexts&&!mock.encodes);
    assert(!sensor_baseline_report.ready);
    if(f==WRONG_ID||f==WRONG_ID_LOW||f==SENSOR_ALREADY_ON||f==GUARD_WRONG)assert(!mock.table_writes);
    if(f==TABLE_FAIL)assert(mock.table_writes==70&&sensor_baseline_report.written==70);
    if(f==TABLE_TIMEOUT)assert(mock.table_writes==71&&sensor_baseline_report.written==70);
    if(f==READBACK_FAIL)assert(sensor_baseline_report.read_count==40);
    if(f==WRONG_HTS)assert(sensor_baseline_report.read_count==139&&sensor_baseline_report.mismatches==2);
    if(f==READBACK_TIMEOUT)assert(sensor_baseline_report.read_count==40);
    if(f==TABLE_CLOCK_BACK)assert(source_clock.failed&&mock.table_writes==71&&sensor_baseline_report.written==70);
    if(f==RESET_DELAY_SHORT)assert(mock.table_writes==1&&mock.reset_sleeps==1&&sensor_baseline_report.reset_delay_ns<10000000);
   }
  }
  cases++;
 }
 char *cli[]={"native-handoff","--parent-protected","--sensor-baseline","exact139","--peer","192.168.1.15","--port","17891","--memory-reserve","normal75-placement-one-shot","--producer-storage","raw-reuse-k75"};
 assert(nh_sensor_cli_optin(12,cli)&&!nh_sensor_cli_optin(10,cli));cli[3]="arbitrary";assert(!nh_sensor_cli_optin(12,cli));cases++;
 /* Actual candidate core + actual imported adapter + exact139 mocked syscalls:
  * one integrated13/12 fMP4/source-CRC/receipt-generation path, no real file. */
 memset(&mock,0,sizeof(mock));mock.clock=1000000000;stop=0;frame_count=0;
 memset(sensor_registers,0,sizeof(sensor_registers));sensor_registers[0x3107]=0xda;sensor_registers[0x3108]=0x4d;
 sensor_registers[0x320c]=6;sensor_registers[0x320d]=0x72; /* preserved observed mismatch model, not a device read */
 nh_native_init();assert(!nh_native_storage_select(test_storage_mode));assert(!nh_native_memory_start(nh_native_now(NULL)));assert(!nh_native_memory_fixed_buffers(integrated_hub));assert(!nh_native_optin_exact139());struct camera_fmp4_config config;assert(!nh_native_config(&config));
 struct nh_backend backend={.next=nh_native_next,.release=nh_native_release,.retire=nh_native_retire,.now=nh_native_now,
 .cancelled=nh_native_cancelled,.write=discard_output,.discard_output=discard_rollback,.quarantine=nh_native_fence,.retire_held=nh_native_retire_held,.memory_gate=nh_native_memory_gate};
 struct nh_report report;assert(!nh_run(&backend,&config,integrated_hub,sizeof(integrated_hub),&report));
 assert(report.produced==13&&report.output==12&&report.boundary_retired&&report.retired&&!report.quarantined);
 assert(discarded_bytes==644+12*(112+137004)&&discarded_bytes==report.committed&&report.committed==report.accepted);
 assert(mock.table_writes==139&&mock.reset_sleeps==1&&sensor_baseline_report.ready&&mock.nexts==13&&mock.encodes==13&&mock.freed==1);
 assert(mock.ends==1&&mock.acks==1&&!mock.watchdog_active&&report.key.generation==303&&report.key.sequence==13);
 for(unsigned i=0;i<12;i++)assert(report.integrity[i].after_checked&&report.integrity[i].source_crc==report.integrity[i].before_crc&&report.integrity[i].before_crc==report.integrity[i].after_crc);
 cases++;
 /* Actual core/native partial sink failure: known producer CPU stays backed
  * through normal END/QUERY/ACK; an END failure retains it instead of free. */
 for(unsigned fail_end=0;fail_end<2;fail_end++){
  memset(&mock,0,sizeof(mock));mock.clock=1000000000;mock.fault=fail_end?END_ERR:GOOD;stop=0;frame_count=0;
  memset(sensor_registers,0,sizeof(sensor_registers));sensor_registers[0x3107]=0xda;sensor_registers[0x3108]=0x4d;
  nh_native_init();assert(!nh_native_storage_select(test_storage_mode));assert(!nh_native_memory_start(nh_native_now(NULL)));assert(!nh_native_memory_fixed_buffers(integrated_hub));assert(!nh_native_optin_exact139());assert(!nh_native_config(&config));
  backend.write=partial_sink_failure;backend.discard_output=close_failed_sink;
  assert(nh_run(&backend,&config,integrated_hub,sizeof(integrated_hub),&report)!=CAMERA_OK);
  assert(report.accepted==3&&report.committed==0&&report.output==0&&mock.ends==1);
  if(fail_end)assert(report.quarantined&&!report.retired&&mock.allocated&&!mock.freed&&native.produced_storage==synthetic_output&&mock.acks==0);
  else assert(report.retired&&!report.quarantined&&!mock.allocated&&mock.freed==1&&mock.acks==1);
  cases++;
 }
 assert(discarded_failures==2);
 /* Actual native/core memory denials, with known old hub held. Never NEXT on
  * pre-acquire denial, no false DMA quarantine; exact stop/ACK before free. */
 for(enum pressure fault=MISSING_MEM;fault<=CLOCK_POST;fault++){
  pressure=fault;memset(&mock,0,sizeof(mock));mock.clock=1000000000;stop=0;frame_count=0;
  memset(sensor_registers,0,sizeof(sensor_registers));sensor_registers[0x3107]=0xda;sensor_registers[0x3108]=0x4d;
  nh_native_init();assert(!nh_native_storage_select(test_storage_mode));int ready=nh_native_memory_start(nh_native_now(NULL));
  if(fault==MISSING_MEM||fault==LOW_INITIAL||fault==BAD_WATERMARK||fault==NEW_POPULATED_ZONE||fault==SAMPLE_OLD){
   assert(ready&&mock.opens==0&&mock.nexts==0&&!native.retirement_uncertain);cases++;continue;
  }
  assert(!ready&&!nh_native_memory_fixed_buffers(integrated_hub)&&!nh_native_optin_exact139()&&!nh_native_config(&config));
  backend.write=discard_output;backend.discard_output=close_failed_sink;
  assert(nh_run(&backend,&config,integrated_hub,sizeof(integrated_hub),&report)!=CAMERA_OK);
  if(fault==CLOCK_POST){assert(report.quarantined&&!report.retired&&mock.allocated&&!mock.freed&&native.retirement_uncertain&&!mock.ends&&!mock.acks);cases++;continue;}
  assert(report.retired&&!report.quarantined&&!mock.allocated&&!native.retirement_uncertain);
  if(fault==PREF_MALLOC_FAIL)assert(!mock.nexts&&!mock.table_writes&&!mock.ends&&!mock.acks);
  else {assert(mock.nexts==1&&mock.encodes==1&&mock.ends==1&&mock.acks==1&&mock.freed==1);}
  if(fault==LOW_HELD)assert(report.produced==1&&report.output==0&&report.committed==644&&!report.failed_next);
  cases++;
 }
 /* A low-memory refusal does not turn an actual END failure into clean stop. */
 pressure=LOW_POST;memset(&mock,0,sizeof(mock));mock.clock=1000000000;mock.fault=END_ERR;stop=0;frame_count=0;
 memset(sensor_registers,0,sizeof(sensor_registers));sensor_registers[0x3107]=0xda;sensor_registers[0x3108]=0x4d;
 nh_native_init();assert(!nh_native_storage_select(test_storage_mode));assert(!nh_native_memory_start(nh_native_now(NULL))&&!nh_native_memory_fixed_buffers(integrated_hub)&&!nh_native_optin_exact139()&&!nh_native_config(&config));
 assert(nh_run(&backend,&config,integrated_hub,sizeof(integrated_hub),&report)!=CAMERA_OK);
 assert(report.quarantined&&!report.retired&&mock.allocated&&!mock.freed&&mock.ends==1&&!mock.acks);cases++;
 pressure=PRESSURE_GOOD;
 /* Reviewed opt-in raw-overlay variant: actual imported producer/kernel-I/O
  * mock plus actual core/hub, not an allocation-count-only hypothesis. */
 for(enum alias_fault af=ALIAS_GOOD;af<=ALIAS_MAX_NAL;af++){
  alias_fault=af;memset(&mock,0,sizeof(mock));mock.clock=1000000000;stop=0;frame_count=0;input_correlations=0;
  memset(sensor_registers,0,sizeof(sensor_registers));sensor_registers[0x3107]=0xda;sensor_registers[0x3108]=0x4d;
  nh_native_init();assert(!nh_native_storage_select("raw-reuse-k75")&&!nh_native_memory_start(nh_native_now(NULL))&&!nh_native_memory_fixed_buffers(integrated_hub)&&!nh_native_optin_exact139()&&!nh_native_config(&config));
  assert(memory_state.ledger.raw_reuse&&!memory_state.ledger.paid[2]);
  backend.write=discard_output;backend.discard_output=close_failed_sink;
  if(af==ALIAS_GOOD){
   discarded_bytes=0;assert(!nh_run(&backend,&config,integrated_hub,sizeof(integrated_hub),&report));
   assert(report.produced==13&&report.output==12&&report.boundary_retired&&report.retired&&!report.quarantined);
   assert(input_correlations==13&&!mock.allocated&&!mock.freed&&mock.ends==1&&mock.acks==1&&!native.raw_valid);
   assert(!memory_state.ledger.paid[2]&&discarded_bytes==644+12*(112+137004));
  }else{
   struct camera_packet packet={0},second={0};struct nh_key key={0};int result=nh_native_next(NULL,&packet,&key);
   if(af==ALIAS_MAX_NAL){
    assert(!result&&packet.data==raw&&packet.bytes==1048576&&key.nal_crc32_bzip2==qc_crc_bzip2(raw,1048576));
    unsigned captures=mock.nexts;assert(nh_native_next(NULL,&second,&key)==CAMERA_BUSY&&mock.nexts==captures); /* loan blocks overwrite */
    assert(!nh_native_release(NULL,&packet)&&!nh_native_retire(NULL)&&mock.ends==1&&mock.acks==1&&!mock.freed);
   }else{
    assert(result&&native.retirement_uncertain&&native.resident_storage==raw&&!mock.allocated&&!mock.freed&&!native.raw_valid);
    assert(nh_native_next(NULL,&second,&key)&&mock.nexts==1&&nh_native_retire(NULL)&&!mock.ends&&!mock.acks);
    if(af==ALIAS_EARLY_WRITE||af==ALIAS_PARTIAL_EARLY_WRITE)assert(modeled_copy_crc!=modeled_main_crc&&!input_correlations);
   }
  }
  cases++;
 }
 alias_fault=ALIAS_GOOD;
 /* Focused raw-alias stop/forecast/lifetime negatives for the finalized mode,
  * using the same actual imported adapter/core, no standalone alias surrogate. */
 for(unsigned scenario=0;scenario<8;scenario++){
  memset(&mock,0,sizeof(mock));mock.clock=1000000000;stop=0;frame_count=0;
  pressure=scenario==0?LOW_HELD:scenario==1?CLOCK_POST:scenario==2?LOW_INITIAL:PRESSURE_GOOD;
  low_before_encode=scenario==3;
  mock.fault=scenario==4?LIFETIME_OLD:scenario==5?QUERY_KEY:scenario==6?END_ERR:scenario==7?ACK_ERR:GOOD;
  memset(sensor_registers,0,sizeof(sensor_registers));sensor_registers[0x3107]=0xda;sensor_registers[0x3108]=0x4d;
  nh_native_init();assert(!nh_native_storage_select("raw-reuse-k75"));int ready=nh_native_memory_start(nh_native_now(NULL));
  if(scenario==2){assert(ready&&!mock.opens&&!mock.nexts&&!mock.encodes&&!mock.table_writes);cases++;continue;}
  assert(!ready&&!nh_native_memory_fixed_buffers(integrated_hub)&&!nh_native_optin_exact139()&&!nh_native_config(&config));
  backend.write=scenario>=6?partial_sink_failure:discard_output;backend.discard_output=close_failed_sink;
  assert(nh_run(&backend,&config,integrated_hub,sizeof(integrated_hub),&report)!=CAMERA_OK);
  assert(!mock.allocated&&!mock.freed&&!memory_state.ledger.paid[2]);
  if(scenario==0){assert(report.retired&&!report.quarantined&&report.produced==1&&!report.output&&mock.nexts==1&&mock.ends==1&&mock.acks==1);}
  else if(scenario==3){assert(report.retired&&!report.quarantined&&mock.nexts==1&&!mock.encodes&&mock.ends==1&&mock.acks==1);}
  else {assert(report.quarantined&&!report.retired&&native.retirement_uncertain);if(mock.encodes)assert(native.resident_storage==raw);}
  cases++;
 }
 low_before_encode=0;pressure=PRESSURE_GOOD;
 printf("HOST imported native adapter: %u synthetic syscall/explicit-exact139 cases PASS; raw/NAL CRC and strict source receipts; NOT_CAMERA_TARGET\n",cases);
 return 0;
}
