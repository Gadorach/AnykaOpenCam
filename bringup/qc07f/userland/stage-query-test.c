/* SPDX-License-Identifier: GPL-2.0-only
 * Target QUERY-ONLY negative/observer fixture. Compile-only on host, ARMv5 fence.
 * Requires externally quiescent producers: moving telemetry is UNQUALIFIED.
 * No sensor access, INFO/reset, BEGIN/NEXT/END, JPEG/H264 or module unload.
 */
#define STAGE_INSPECT_NO_MAIN
#include "stage-inspect.c"
#include <sys/mman.h>
#ifndef STAGE_MMAP
#define STAGE_MMAP mmap
#endif
#ifndef STAGE_MPROTECT
#define STAGE_MPROTECT mprotect
#endif
#ifndef STAGE_MUNMAP
#define STAGE_MUNMAP munmap
#endif
int stage_expected_errno(int fd,unsigned codec,void *arg,int expected,const char *name)
{
 errno=0;int result=STAGE_IOCTL(fd,stage_command(codec),arg),error=errno;
 if(result!=-1||error!=expected){fprintf(stderr,"FAIL query-case=%s ret=%d errno=%d expected=%d\n",name,result,error,expected);return STAGE_ERROR;}
 printf("QUERY_CASE %s expected_errno=%d; protocol-only, not DMA proof\n",name,error);return 0;
}
int stage_fault_checks(int fd,const struct stage_options *o)
{
 static const char *names[]={"invalid-version","invalid-size","invalid-kind","invalid-phase","invalid-record","invalid-reserved"};
 for(unsigned test=0;test<6;test++){
  struct qc_stage_query q=stage_request(o->kind,QC_PHASE_FRAME_WAIT,o->record);
  switch(test){case 0:q.version++;break;case 1:q.struct_bytes--;break;case 2:q.kind=QC_STAGE_KINDS;break;
   case 3:q.phase=QC_STAGE_PHASES;break;case 4:q.record=2;break;default:q.reserved=1;break;}
  struct qc_stage_query saved;memcpy(&saved,&q,sizeof(q));
  if(stage_expected_errno(fd,o->codec,&q,EINVAL,names[test]))return STAGE_ERROR;
  if(memcmp(&saved,&q,sizeof(q))){fputs("FAIL invalid request modified caller buffer\n",stderr);return STAGE_ERROR;}
 }
 long page=sysconf(_SC_PAGESIZE);if(page<=0||(size_t)page<sizeof(struct qc_stage_query)){fputs("FAIL page size\n",stderr);return STAGE_ERROR;}
 void *area=STAGE_MMAP(NULL,(size_t)page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 if(area==MAP_FAILED){perror("owned query fault page");return STAGE_ERROR;}
 int result=STAGE_ERROR;struct qc_stage_query q=stage_request(o->kind,QC_PHASE_FRAME_WAIT,o->record);
 memcpy(area,&q,sizeof(q));
 if(STAGE_MPROTECT(area,(size_t)page,PROT_NONE)){perror("input protection");goto done;}
 if(stage_expected_errno(fd,o->codec,area,EFAULT,"input-EFAULT-owned-PROT_NONE"))goto done;
 if(STAGE_MPROTECT(area,(size_t)page,PROT_READ|PROT_WRITE)){perror("restore query page");goto done;}
 memcpy(area,&q,sizeof(q));
 if(STAGE_MPROTECT(area,(size_t)page,PROT_READ)){perror("reply protection");goto done;}
 if(stage_expected_errno(fd,o->codec,area,EFAULT,"reply-EFAULT-owned-PROT_READ"))goto done;
 result=0;
done:
 if(STAGE_MUNMAP(area,(size_t)page)){perror("query page munmap");result=STAGE_ERROR;}return result;
}
int stage_same_idle(int fd,const struct stage_options *o,const struct qc_stage_query *before)
{
 struct qc_stage_query after;int result=stage_query(fd,o,QC_PHASE_FRAME_WAIT,&after);if(result)return result;
 if(memcmp(before,&after,sizeof(after))){fputs("UNQUALIFIED moving/inconsistent observer evidence; no purity pass, retry=none\n",stderr);return STAGE_MOVING;}
 return 0;
}
int stage_coverage_checks(int fd,const struct stage_options *o)
{
 struct stage_options probe=*o;struct qc_stage_query q;
 probe.kind=o->codec?QC_STAGE_NEXT:QC_STAGE_H264;
 int result=stage_query(fd,&probe,QC_PHASE_FRAME_WAIT,&q);if(result)return result;
 if(q.supported_mask||q.samples){fputs("FAIL unused kind advertised/measured\n",stderr);return STAGE_ERROR;}
 printf("QUERY_COVERAGE unused_kind=%u unsupported=true samples=0 NOT measured0ns last_kind=%u\n",probe.kind,q.last_kind);
 probe.kind=o->codec?QC_STAGE_H264:QC_STAGE_NEXT_IRQ;
 result=stage_query(fd,&probe,QC_PHASE_METADATA_COPY,&q);if(result)return result;
 if(q.supported_mask&(1U<<QC_PHASE_METADATA_COPY)||q.samples){fputs("FAIL reserved metadata phase advertised/measured\n",stderr);return STAGE_ERROR;}
 result=stage_query(fd,&probe,QC_PHASE_FRAME_WAIT,&q);if(result)return result;
 if(!(q.supported_mask&(1U<<QC_PHASE_FRAME_WAIT))){fputs("FAIL reachable wait coverage missing\n",stderr);return STAGE_ERROR;}
 printf("QUERY_COVERAGE used_kind=%u phase=4 state=%s samples=%" PRIu64 " total_ns=%" PRIu64 "; no manufactured sample/zero cost\n",
  probe.kind,q.samples?"observed":"unobserved",(uint64_t)q.samples,(uint64_t)q.total_ns);
 return 0;
}
#ifndef STAGE_QUERY_TEST_NO_MAIN
int main(int argc,char **argv)
{
 if(argc==2&&!strcmp(argv[1],"--help")){stage_usage(1);return 0;}
 struct stage_options o;if(stage_parse(argc,argv,&o,1)){stage_usage(1);return STAGE_USAGE;}
 if(!stage_native_armv5()){fputs("UNQUALIFIED target fault fixture fenced on host; compile-only; no device/page operation\n",stderr);return STAGE_UNQUALIFIED;}
 int fd=stage_open_device(o.codec),observer=-1,result=STAGE_ERROR;
 if(fd<0){perror("fixed query fixture device open");return STAGE_ERROR;}
 struct qc_stage_query baseline;result=stage_query(fd,&o,QC_PHASE_FRAME_WAIT,&baseline);if(result)goto done;
 if(baseline.flags){fputs("UNQUALIFIED telemetry error flags; no fault/purity pass\n",stderr);result=STAGE_INVALID;goto done;}
 result=stage_coverage_checks(fd,&o);if(result)goto done;
 result=stage_fault_checks(fd,&o);if(result)goto done;
 result=stage_same_idle(fd,&o,&baseline);if(result)goto done;
 observer=stage_open_device(o.codec);if(observer<0){perror("observer open");result=STAGE_ERROR;goto done;}
 result=stage_same_idle(observer,&o,&baseline);if(result)goto done;
 if(STAGE_CLOSE(observer)){observer=-1;perror("observer close");result=STAGE_ERROR;goto done;}observer=-1;
 result=stage_same_idle(fd,&o,&baseline);if(result)goto done;
 if(STAGE_CLOSE(fd)){fd=-1;perror("first query fd close");result=STAGE_ERROR;goto done;}fd=-1;
 fd=stage_open_device(o.codec);if(fd<0){perror("query reopen");result=STAGE_ERROR;goto done;}
 result=stage_same_idle(fd,&o,&baseline);if(result)goto done;
done:
 if(observer>=0&&STAGE_CLOSE(observer)){perror("observer cleanup");result=STAGE_ERROR;}
 if(fd>=0&&STAGE_CLOSE(fd)){perror("query cleanup");result=STAGE_ERROR;}
 if(!result)puts("QUERY_ONLY fixture protocol faults/coverage + observer open/close/reopen stable PASS; no sensor/DMA start; retirement flag0 NOT electrical proof; delivered frames need separate evidence");
 return result;
}
#endif
