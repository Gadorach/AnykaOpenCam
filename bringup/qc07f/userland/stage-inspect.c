/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded read-only QC07F telemetry collector. No capture/program/reset ioctl.
 * Helpers are reused by stage-query-test.c; target execution is ARMv5 fenced.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../hwenc/stage-telemetry-uapi.h"

enum {STAGE_ERROR=1,STAGE_USAGE=2,STAGE_MOVING=3,STAGE_INVALID=4,STAGE_UNQUALIFIED=77};
_Static_assert(sizeof(struct qc_stage_query)==296,"frozen version1 query ABI");
_Static_assert(offsetof(struct qc_stage_query,samples)==32,"samples ABI");
_Static_assert(offsetof(struct qc_stage_query,allocation_attempts)==216,"allocation ABI");
_Static_assert(offsetof(struct qc_stage_query,owner_generation)==288,"owner ABI");
_Static_assert(_IOC_SIZE(QC_ISP_STAGE_QUERY)==296&&_IOC_SIZE(QC_HWENC_STAGE_QUERY)==296,"ioctl size ABI");
__attribute__((used,section(".stage_abi"))) const uint32_t stage_abi[]={
 QC_STAGE_VERSION,sizeof(struct qc_stage_query),offsetof(struct qc_stage_query,samples),
 offsetof(struct qc_stage_query,allocation_attempts),offsetof(struct qc_stage_query,owner_generation),
 QC_STAGE_KINDS,QC_STAGE_PHASES,QC_ISP_STAGE_QUERY,QC_HWENC_STAGE_QUERY};
#ifndef STAGE_IOCTL
#define STAGE_IOCTL ioctl
#endif
#ifndef STAGE_OPEN
#define STAGE_OPEN open
#endif
#ifndef STAGE_CLOSE
#define STAGE_CLOSE close
#endif
struct stage_options {unsigned codec,kind,record,retry_once;int phase;};
struct stage_snapshot {struct qc_stage_query row[QC_STAGE_PHASES];unsigned count,moving,attempts;};

int stage_native_armv5(void)
{
#if defined(__arm__) && !defined(__aarch64__) && defined(__ARM_ARCH) && __ARM_ARCH == 5 && !defined(STAGE_HOST_MODEL)
 return 1;
#else
 return 0;
#endif
}
const char *stage_path(unsigned codec)
{return codec?"/dev/qc07f-encoder":"/dev/qc07f-frame";}
unsigned long stage_command(unsigned codec)
{return codec?QC_HWENC_STAGE_QUERY:QC_ISP_STAGE_QUERY;}
int stage_number(const char *p,unsigned max,unsigned *out)
{
 unsigned n=0;if(!p||!*p||(p[0]=='0'&&p[1]))return -1;
 for(;*p;p++){if(*p<'0'||*p>'9'||n>max/10)return -1;n=n*10+(unsigned)(*p-'0');if(n>max)return -1;}
 *out=n;return 0;
}
int stage_parse(int argc,char **argv,struct stage_options *out,int fixture)
{
 struct stage_options o={.phase=-1};unsigned seen=0;
 for(int i=1;i<argc;i++){
  unsigned bit=0,value=0;const char *arg=argv[i];
  if(!strcmp(arg,"--qc07f-query-only"))bit=1;
  else if(!strcmp(arg,"--device")){
   bit=2;if(++i>=argc)return -1;
   if(!strcmp(argv[i],"isp"))o.codec=0;else if(!strcmp(argv[i],"codec"))o.codec=1;else return -1;
  }else if(!strcmp(arg,"--kind")){
   bit=4;if(++i>=argc||stage_number(argv[i],8,&o.kind))return -1;
  }else if(!strcmp(arg,"--record")){
   bit=8;if(++i>=argc||stage_number(argv[i],1,&o.record))return -1;
  }else if(!strcmp(arg,"--phase")&&!fixture){
   bit=16;if(++i>=argc||stage_number(argv[i],16,&value))return -1;o.phase=(int)value;
  }else if(!strcmp(arg,"--retry-once")&&!fixture){bit=32;o.retry_once=1;}
  else return -1;
  if(seen&bit)return -1;
  seen|=bit;
 }
 if((seen&7)!=7)return -1;
 *out=o;return 0;
}
uint32_t stage_expected_mask(unsigned codec,unsigned kind)
{
#define P(n) (1U<<(n))
 uint32_t pool=P(QC_PHASE_ALLOC)|P(QC_PHASE_FREE),ret=P(QC_PHASE_RESET_RETIRE);
 if(codec){
  if(kind==QC_STAGE_INFO)return P(QC_PHASE_RESET);
  if(kind==QC_STAGE_JPEG)return P(QC_PHASE_RESET)|pool|ret;
  if(kind==QC_STAGE_END)return P(QC_PHASE_FREE)|ret;
  if(kind==QC_STAGE_H264)return P(QC_PHASE_RESET)|P(QC_PHASE_PREPARE)|P(QC_PHASE_PROGRAM)|
   P(QC_PHASE_FRAME_WAIT)|P(QC_PHASE_GUARDS)|P(QC_PHASE_PIXEL_COPY)|P(QC_PHASE_OUTPUT_COPY)|pool|ret;
  return 0;
 }
 if(kind==QC_STAGE_OTHER||kind==QC_STAGE_QUEUE)return P(QC_PHASE_RESET)|P(QC_PHASE_PREPARE)|pool|ret;
 if(kind==QC_STAGE_BEGIN)return P(QC_PHASE_RESET)|P(QC_PHASE_PREPARE)|ret;
 if(kind==QC_STAGE_END)return P(QC_PHASE_FREE)|ret;
 if(kind==QC_STAGE_NEXT||kind==QC_STAGE_NEXT_IRQ)return P(QC_PHASE_PRE_IDLE)|P(QC_PHASE_PROGRAM)|
  P(QC_PHASE_FRAME_WAIT)|P(QC_PHASE_PP_COMMIT)|P(QC_PHASE_PRE_GATE_DRAIN)|P(QC_PHASE_POST_GATE_DRAIN)|
  P(QC_PHASE_GUARDS)|P(QC_PHASE_CRC_PRE)|P(QC_PHASE_PIXEL_COPY)|P(QC_PHASE_CRC_POST)|ret;
 return 0;
#undef P
}
struct qc_stage_query stage_request(unsigned kind,unsigned phase,unsigned record)
{
 struct qc_stage_query q;memset(&q,0,sizeof(q));
 q.version=QC_STAGE_VERSION;q.struct_bytes=sizeof(q);q.kind=kind;q.phase=phase;q.record=record;return q;
}
int stage_validate(const struct qc_stage_query *q,unsigned codec,unsigned kind,unsigned phase,unsigned record)
{
 const uint32_t all=(1U<<QC_STAGE_PHASES)-1,known=QC_STAGE_F_SATURATED|QC_STAGE_F_CLOCK_ERROR|QC_STAGE_F_LEDGER_ERROR|QC_STAGE_F_RETIRE_FAILED;
 if(q->version!=1||q->struct_bytes!=sizeof(*q)||q->kind!=kind||q->phase!=phase||q->record!=record||q->reserved||
    q->clock_source!=QC_STAGE_CLOCK_MONOTONIC_KTIME_NS||q->flags&~known||q->supported_mask!=stage_expected_mask(codec,kind)||
    q->last_kind>=QC_STAGE_KINDS||q->owner_active>1||q->last_retirement_failed>1||q->last_phase_mask&~(uint64_t)all)return -1;
 if(!(q->supported_mask&(1U<<phase))&&q->samples)return -1;
 if(!q->samples&&(q->failed_samples||q->total_ns||q->min_ns||q->max_ns||q->work_bytes||q->failed_total_ns))return -1;
 if(q->samples&&(q->failed_samples>q->samples||q->min_ns>q->max_ns||q->max_ns>q->total_ns||q->failed_total_ns>q->total_ns))return -1;
 if(!(q->last_phase_mask&(UINT64_C(1)<<phase))&&(q->last_phase_ns||q->last_phase_bytes||q->last_phase_start_ns||q->last_phase_end_ns))return -1;
 if(!q->last_operation&&(q->last_start_ns||q->last_end_ns||q->last_phase_mask||q->last_owner_generation||q->last_result||q->last_kind||q->last_retirement_failed))return -1;
 if(q->last_operation&&!stage_expected_mask(codec,q->last_kind))return -1;
 if(!(q->flags&QC_STAGE_F_SATURATED)){
  if(q->operation_ok>q->operation_count||q->operation_failed!=q->operation_count-q->operation_ok||q->delivered>q->operation_ok||
     q->last_operation>q->operation_count||q->last_owner_generation>q->owner_generation)return -1;
  if(!record&&q->last_operation!=q->operation_count)return -1;
  if(record&&((q->operation_failed&&!q->last_operation)||(!q->operation_failed&&q->last_operation)))return -1;
  if(q->allocation_successes>q->allocation_attempts||q->allocation_failures!=q->allocation_attempts-q->allocation_successes||
     q->free_calls>q->allocation_successes||q->live_requested_bytes>q->peak_requested_bytes||q->total_freed_bytes>q->total_requested_bytes)return -1;
 }
 if(q->last_operation&&(q->last_phase_mask&~(uint64_t)stage_expected_mask(codec,q->last_kind)))return -1;
 if(record&&q->last_operation&&!q->last_result)return -1;
 if(!(q->flags&(QC_STAGE_F_CLOCK_ERROR|QC_STAGE_F_SATURATED|QC_STAGE_F_LEDGER_ERROR))){
  if(q->last_end_ns<q->last_start_ns)return -1;
  if(q->last_phase_mask&(UINT64_C(1)<<phase)){
   if(q->last_phase_start_ns<q->last_start_ns||q->last_phase_end_ns>q->last_end_ns||q->last_phase_end_ns<q->last_phase_start_ns||
      q->last_phase_ns>q->last_phase_end_ns-q->last_phase_start_ns)return -1;
  }
 }
 return 0;
}
int stage_shared_equal(const struct qc_stage_query *a,const struct qc_stage_query *b)
{
 struct qc_stage_query x,y;memcpy(&x,a,sizeof(x));memcpy(&y,b,sizeof(y));
#define CLEAR(q) do {(q).phase=0;(q).samples=0;(q).failed_samples=0;(q).total_ns=0;(q).min_ns=0;(q).max_ns=0; \
 (q).work_bytes=0;(q).failed_total_ns=0;(q).last_phase_ns=0;(q).last_phase_bytes=0;(q).last_phase_start_ns=0;(q).last_phase_end_ns=0;}while(0)
 CLEAR(x);CLEAR(y);
#undef CLEAR
 return !memcmp(&x,&y,sizeof(x));
}
int stage_query(int fd,const struct stage_options *o,unsigned phase,struct qc_stage_query *out)
{
 struct qc_stage_query q=stage_request(o->kind,phase,o->record);
 if(STAGE_IOCTL(fd,stage_command(o->codec),&q)){
  int error=errno;fprintf(stderr,"UNQUALIFIED query phase=%u errno=%d%s\n",phase,error,error==ENOTTY?" unsupported-kernel-or-module (kernel70 is not a pass)":"");
  return error==ENOTTY?STAGE_UNQUALIFIED:STAGE_ERROR;
 }
 if(stage_validate(&q,o->codec,o->kind,phase,o->record)){
  fprintf(stderr,"BAD_RESPONSE phase=%u; refuse telemetry interpretation\n",phase);return STAGE_ERROR;
 }
 memcpy(out,&q,sizeof(q));return 0;
}
int stage_collect(int fd,const struct stage_options *o,struct stage_snapshot *s)
{
 memset(s,0,sizeof(*s));
 for(unsigned attempt=0;attempt<=o->retry_once;attempt++){
  s->count=0;s->moving=0;s->attempts=attempt+1;
  unsigned count=o->phase<0?QC_STAGE_PHASES:1;
  for(unsigned i=0;i<count;i++){
   unsigned phase=o->phase<0?i:(unsigned)o->phase;
   int r=stage_query(fd,o,phase,&s->row[i]);if(r)return r;s->count++;
   if(i&&!stage_shared_equal(&s->row[0],&s->row[i]))s->moving=1;
  }
  if(!s->moving)break;
 }
 if(s->moving)return STAGE_MOVING;
 for(unsigned i=0;i<s->count;i++)if(s->row[i].flags)return STAGE_INVALID;
 return 0;
}
int stage_open_device(unsigned codec)
{
 if(!stage_native_armv5()){errno=EPERM;return -1;}
 int fd=STAGE_OPEN(stage_path(codec),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return -1;
 struct stat st;int status=fstat(fd,&st);
 if(status||!S_ISCHR(st.st_mode)){int error=status?errno:ENODEV;STAGE_CLOSE(fd);errno=error;return -1;}return fd;
}
void stage_print(const struct stage_options *o,const struct stage_snapshot *s,int result)
{
 const char *coherence=o->phase>=0?"single-query-no-sweep-claim":s->moving?"MOVING-do-not-coalesce":
  result==STAGE_INVALID?"invalid-interpretation-do-not-coalesce":"matching-metadata-not-atomic";
 printf("STAGE_QUERY_ONLY device=%s kind=%u record=%u rows=%u attempts=%u coherence=%s result=%d clock=ktime_MONOTONIC_ns (not hardware-clocksource proof)\n",
  o->codec?"codec":"isp",o->kind,o->record,s->count,s->attempts,coherence,result);
 for(unsigned i=0;i<s->count;i++){
  const struct qc_stage_query *q=&s->row[i];unsigned supported=!!(q->supported_mask&(1U<<q->phase)),present=!!(q->last_phase_mask&(UINT64_C(1)<<q->phase));
  const char *state=!supported?"unsupported":q->flags?"invalid-flags":!q->samples?"unobserved":"observed";
  printf("phase=%u aggregate=%s supported=%u last_present=%u flags=0x%x samples=%" PRIu64 " failed=%" PRIu64
   " min_ns=%" PRIu64 " max_ns=%" PRIu64 " total_ns=%" PRIu64 " failed_ns=%" PRIu64 " work_bytes=%" PRIu64
   " op=%" PRIu64 " ops=%" PRIu64 " ok=%" PRIu64 " bad=%" PRIu64 " delivered=%" PRIu64 " last_kind=%u last_result=%d"
   " start_ns=%" PRIu64 " end_ns=%" PRIu64 " phase_ns=%" PRIu64 " phase_bytes=%" PRIu64 " phase_start_ns=%" PRIu64 " phase_end_ns=%" PRIu64
   " last_owner_generation=%" PRIu64 " owner_generation=%" PRIu64 " owner_active=%u alloc_attempt=%" PRIu64 " alloc_fail=%" PRIu64
   " alloc_ok=%" PRIu64 " free_calls=%" PRIu64 " live_requested=%" PRIu64 " peak_requested=%" PRIu64 " total_requested=%" PRIu64 " freed_requested=%" PRIu64
   " retire_failures=%" PRIu64 " last_retire_failed=%u\n",
   q->phase,state,supported,present,q->flags,(uint64_t)q->samples,(uint64_t)q->failed_samples,(uint64_t)q->min_ns,(uint64_t)q->max_ns,
   (uint64_t)q->total_ns,(uint64_t)q->failed_total_ns,(uint64_t)q->work_bytes,(uint64_t)q->last_operation,(uint64_t)q->operation_count,
   (uint64_t)q->operation_ok,(uint64_t)q->operation_failed,(uint64_t)q->delivered,q->last_kind,q->last_result,
   (uint64_t)q->last_start_ns,(uint64_t)q->last_end_ns,(uint64_t)q->last_phase_ns,(uint64_t)q->last_phase_bytes,
   (uint64_t)q->last_phase_start_ns,(uint64_t)q->last_phase_end_ns,(uint64_t)q->last_owner_generation,(uint64_t)q->owner_generation,q->owner_active,
   (uint64_t)q->allocation_attempts,(uint64_t)q->allocation_failures,(uint64_t)q->allocation_successes,(uint64_t)q->free_calls,
   (uint64_t)q->live_requested_bytes,(uint64_t)q->peak_requested_bytes,(uint64_t)q->total_requested_bytes,(uint64_t)q->total_freed_bytes,
   (uint64_t)q->retire_failures,q->last_retirement_failed);
 }
 puts("last_kind may differ from aggregate kind; no samples is not measured zero; retirement flag0 is NOT electrical retirement proof; no FPS/acceptance claim");
}
void stage_usage(int fixture)
{
 fprintf(stderr,"usage: %s --qc07f-query-only --device isp|codec --kind 0..8 [--record 0|1]%s\n",
  fixture?"stage-query-test":"stage-inspect",fixture?"":" [--phase 0..16] [--retry-once]");
}
#ifndef STAGE_INSPECT_NO_MAIN
int main(int argc,char **argv)
{
 if(argc==2&&!strcmp(argv[1],"--help")){stage_usage(0);return 0;}
 struct stage_options o;if(stage_parse(argc,argv,&o,0)){stage_usage(0);return STAGE_USAGE;}
 if(!stage_native_armv5()){fputs("UNQUALIFIED host execution fenced; native ARMv5 required; no device opened\n",stderr);return STAGE_UNQUALIFIED;}
 int fd=stage_open_device(o.codec);if(fd<0){perror("fixed telemetry device open");return STAGE_ERROR;}
 struct stage_snapshot s;int result=stage_collect(fd,&o,&s);
 if(s.count)stage_print(&o,&s,result);
 if(STAGE_CLOSE(fd)){perror("observer close");return STAGE_ERROR;}return result;
}
#endif
