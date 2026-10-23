/* SPDX-License-Identifier: GPL-2.0-only
 * ONE bounded source-integrated metadata gate, not a daemon/probe/guardian.
 * Fresh meminfo -> zoneinfo -> meminfo; conservative min-free/available and
 * max-CMA readings, current boosted high used ONCE; no reclaim/swap credit.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "memory_runtime.h"
#include "handoff.h"
#include <fcntl.h>
#include <linux/magic.h>
#include <stdio.h>
#include <string.h>
#include <sys/statfs.h>
#include <unistd.h>
static int checked(struct nh_memory_runtime *r,uint64_t start,uint64_t *now)
{
 *now=r->now(r->context);
 if(*now==UINT64_MAX||*now<start||*now<r->session_start)return NH_MEM_CLOCK;
 if(*now-start>=NH_OUTPUT_NS||*now-r->session_start>=NH_SESSION_NS)return NH_MEM_STALE;
 if(r->cancelled(r->context))return NH_MEM_CANCEL;
 return NH_MEM_OK;
}
int nh_memory_runtime_init(struct nh_memory_runtime *r,uint64_t start,
 uint64_t (*now)(void *),int (*cancelled)(void *),void *context,unsigned raw_reuse)
{
 uint64_t capacity,nonmovable;if(!r||!now||!cancelled||start==UINT64_MAX||nh_memory_own_layout(&capacity,&nonmovable,raw_reuse))return NH_MEM_LAYOUT;
 memset(r,0,sizeof(*r));if(nh_memory_init_mode(&r->ledger,capacity,nonmovable,raw_reuse))return NH_MEM_LAYOUT;
 r->session_start=start;r->now=now;r->cancelled=cancelled;r->context=context;return NH_MEM_OK;
}
int nh_memory_runtime_gate(struct nh_memory_runtime *r,enum nh_memory_stage stage,uint64_t unit_start)
{
 if(!r||!r->ledger.initialized||r->count>=64)return NH_MEM_FORMAT;
 struct nh_memory_observation o={.stage=stage};struct nh_memory_values first,last;struct nh_memory_zones zones;
 uint64_t now;size_t bytes;int reason=checked(r,unit_start,&o.begin_ns);if(reason)goto failed;
 if(nh_memory_read_fixed(0,r->meminfo,sizeof(r->meminfo),&bytes)){reason=NH_MEM_IO;goto failed;}
 if(nh_memory_parse_meminfo(r->meminfo,bytes,&first)){reason=NH_MEM_FORMAT;goto failed;}
 reason=checked(r,unit_start,&now);if(reason)goto failed;
 if(now<o.begin_ns||now-o.begin_ns>NH_MEMORY_MAX_SAMPLE_NS){reason=now<o.begin_ns?NH_MEM_CLOCK:NH_MEM_STALE;goto failed;}
 if(nh_memory_read_fixed(1,r->zoneinfo,sizeof(r->zoneinfo),&bytes)){reason=NH_MEM_IO;goto failed;}
 if(nh_memory_parse_zoneinfo(r->zoneinfo,bytes,&zones)){reason=NH_MEM_LAYOUT;goto failed;}
 reason=checked(r,unit_start,&now);if(reason)goto failed;
 if(now<o.begin_ns||now-o.begin_ns>NH_MEMORY_MAX_SAMPLE_NS){reason=now<o.begin_ns?NH_MEM_CLOCK:NH_MEM_STALE;goto failed;}
 if(nh_memory_read_fixed(0,r->meminfo,sizeof(r->meminfo),&bytes)){reason=NH_MEM_IO;goto failed;}
 if(nh_memory_parse_meminfo(r->meminfo,bytes,&last)){reason=NH_MEM_FORMAT;goto failed;}
 reason=checked(r,unit_start,&o.end_ns);if(reason)goto failed;
 reason=nh_memory_evaluate(&r->ledger,&first,&last,&zones,o.begin_ns,o.end_ns,stage,&o);
 r->observations[r->count++]=o;return reason;
failed:
 o.reason=(enum nh_memory_reason)reason;o.end_ns=r->now(r->context);r->observations[r->count++]=o;return reason;
}
int nh_memory_prefault_owned(struct nh_memory_runtime *r,unsigned kind,void *buffer,size_t bytes,uint64_t start)
{
 if(!r||!buffer||!bytes||bytes>1728000||kind>=3||r->ledger.paid[kind])return NH_MEM_CREDIT;
 /* Only newly owned raw/hub/producer buffers, never an acquired/current AU.
  * Volatile read/write of SAME byte faults private writable backing; no copy,
  * CRC shortcut, mlock policy or pixel/value modification. */
 volatile unsigned char *p=buffer;uint64_t now;int result=checked(r,start,&now);if(result)return result;
 for(size_t i=0;i<bytes;i+=4096){unsigned char old=p[i];p[i]=old;if((i/4096)%16==0){result=checked(r,start,&now);if(result)return result;}}
 {unsigned char old=p[bytes-1];p[bytes-1]=old;}
 result=checked(r,start,&now);if(result)return result;
 return nh_memory_credit_owned(&r->ledger,kind,buffer,bytes)?NH_MEM_CREDIT:NH_MEM_OK;
}
void nh_memory_log(const struct nh_memory_runtime *r)
{
  fprintf(stderr,"NH_MEMORY_POLICY normal75-placement-one-shot producer_storage=%s CPU_capacity_kib=%llu CPU_paid_buffer_kib=%llu RX_staging_debt_kib=%llu codec_paid=%u overhead_envelope_kib=512 reserve_kib=1024 paid_buffers_not_RSS_peak=1\n",
  r->ledger.raw_reuse?"raw-reuse-k75":"separate",(unsigned long long)r->ledger.cpu_capacity_kib,(unsigned long long)r->ledger.cpu_paid_kib,(unsigned long long)r->ledger.nonmovable_cpu_debt_kib,r->ledger.codec_paid);
 for(unsigned i=0;i<r->count;i++){
  const struct nh_memory_observation *o=&r->observations[i];
  fprintf(stderr,"NH_MEMORY_GATE stage=%u reason=%u begin_ns=%llu end_ns=%llu available_kib=%llu nonCMA_above_W_kib=%llu W_kib=%llu CPU_unpaid_kib=%llu DMA_unpaid_kib=%llu require_A_kib=%llu require_nonCMA_kib=%llu future_migration_kib=%llu\n",
  (unsigned)o->stage,(unsigned)o->reason,(unsigned long long)o->begin_ns,(unsigned long long)o->end_ns,
  (unsigned long long)o->available,(unsigned long long)o->noncma_free,(unsigned long long)o->watermark_kib,
  (unsigned long long)o->cpu_debt,(unsigned long long)o->dma_debt,(unsigned long long)o->required_available,(unsigned long long)o->required_noncma,(unsigned long long)o->migration_future_kib);
 }
}
