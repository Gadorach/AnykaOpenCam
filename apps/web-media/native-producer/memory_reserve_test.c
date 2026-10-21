/* SPDX-License-Identifier: GPL-2.0-only; public synthetic metadata, NO proc reads. */
#include "memory_reserve.h"
#include "memory_fixture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void change(char *s,const char *old,const char *new)
{char *p=strstr(s,old);assert(p);size_t tail=strlen(p+strlen(old));memmove(p+strlen(new),p+strlen(old),tail+1);memcpy(p,new,strlen(new));}
int main(void)
{
 struct nh_memory_values m;struct nh_memory_zones z;unsigned cases=0;
 assert(!nh_memory_parse_meminfo(NH_PUBLIC_MEMINFO,strlen(NH_PUBLIC_MEMINFO),&m));cases++;
 assert(!nh_memory_parse_zoneinfo(NH_PUBLIC_ZONEINFO,strlen(NH_PUBLIC_ZONEINFO),&z));assert(z.watermark_kib==5160);cases++;
 char buf[2048];
 const char *badmem[][2]={{"MemFree: 19040 kB","MemFree: -1 kB"},{"CmaFree: 9216 kB","CmaFree: 18446744073709551616 kB"},
 {"MemAvailable: 13972 kB","MemAvailable: 13972 MB"},{"SwapTotal: 0 kB","SwapTotal: 1 kB"},{"CmaTotal: 16384 kB","CmaTotal: 20000 kB"}};
 for(unsigned i=0;i<sizeof(badmem)/sizeof(*badmem);i++){strcpy(buf,NH_PUBLIC_MEMINFO);change(buf,badmem[i][0],badmem[i][1]);assert(nh_memory_parse_meminfo(buf,strlen(buf),&m));cases++;}
 strcpy(buf,NH_PUBLIC_MEMINFO);strcat(buf,"MemFree: 19040 kB\n");assert(nh_memory_parse_meminfo(buf,strlen(buf),&m));cases++;
 strcpy(buf,NH_PUBLIC_MEMINFO);change(buf,"MemAvailable: 13972 kB\n","");assert(nh_memory_parse_meminfo(buf,strlen(buf),&m));cases++;
 const char *badzone[][2]={{"Normal","DMA"},{"managed 8286","managed 8287"},{"protection: (0, 0)","protection: (0, 1)"},
 {"high 1290","high 1291"},{"Node 0, zone Movable","Node 1, zone Movable"},{"managed 0","managed 1"},{"cma 4096","cma 4095"}};
 for(unsigned i=0;i<sizeof(badzone)/sizeof(*badzone);i++){strcpy(buf,NH_PUBLIC_ZONEINFO);change(buf,badzone[i][0],badzone[i][1]);assert(nh_memory_parse_zoneinfo(buf,strlen(buf),&z));cases++;}
 strcpy(buf,NH_PUBLIC_ZONEINFO);strcat(buf,"Node 0, zone DMA\n");assert(nh_memory_parse_zoneinfo(buf,strlen(buf),&z));cases++;
 const char *bad_cma[][2]={{" nr_free_cma 2304\n",""},{"nr_free_cma 2304","nr_free_cma 2304\n nr_free_cma 2304"},
  {"nr_free_cma 2304","nr_free_cma 4097"},{"nr_free_cma 2304","nr_free_cma 18446744073709551616"}};
 for(unsigned i=0;i<sizeof(bad_cma)/sizeof(*bad_cma);i++){strcpy(buf,NH_PUBLIC_ZONEINFO);change(buf,bad_cma[i][0],bad_cma[i][1]);assert(nh_memory_parse_zoneinfo(buf,strlen(buf),&z));cases++;}
 /* A current dynamic boost is legal only when every unboosted base remains
  * sealed. high already includes it; never add boost or empty-zone32 twice. */
 strcpy(buf,NH_PUBLIC_ZONEINFO);change(buf,"boost 1024","boost 0");change(buf,"min 1202","min 178");change(buf,"low 1246","low 222");change(buf,"high 1290","high 266");change(buf,"promo 1334","promo 310");
 assert(!nh_memory_parse_zoneinfo(buf,strlen(buf),&z)&&z.watermark_kib==1064);cases++;
 assert(!nh_memory_parse_meminfo(NH_PUBLIC_MEMINFO,strlen(NH_PUBLIC_MEMINFO),&m));assert(!nh_memory_parse_zoneinfo(NH_PUBLIC_ZONEINFO,strlen(NH_PUBLIC_ZONEINFO),&z));
 struct nh_memory_ledger l;assert(!nh_memory_init(&l,3832,80));struct nh_memory_observation o;
 assert(nh_memory_evaluate(&l,&m,&m,&z,1000,2000,NH_MEM_INITIAL,&o)==NH_MEM_LOW_NONCMA);assert(o.required_available==13624&&o.noncma_free==4664&&o.required_noncma==12536&&o.migration_future_kib==7168);cases++;
 struct nh_memory_values bad=m;bad.available=13623;assert(nh_memory_evaluate(&l,&bad,&bad,&z,1000,2000,NH_MEM_INITIAL,&o)==NH_MEM_LOW_AVAILABLE);cases++;
 bad=m;bad.free=14000;assert(nh_memory_evaluate(&l,&bad,&bad,&z,1000,2000,NH_MEM_INITIAL,&o)==NH_MEM_LOW_NONCMA);cases++;
 assert(nh_memory_evaluate(&l,&m,&m,&z,1000,1000+NH_MEMORY_MAX_SAMPLE_NS+1,NH_MEM_INITIAL,&o)==NH_MEM_STALE);cases++;
 assert(nh_memory_evaluate(&l,&m,&m,&z,2000,1000,NH_MEM_INITIAL,&o)==NH_MEM_CLOCK);cases++;
 assert(nh_memory_credit_codec(&l,8256,1));assert(!nh_memory_credit_codec(&l,8454144,1));cases++;
 assert(!nh_memory_credit_owned(&l,0,(void *)(uintptr_t)0x100000,1728000));
 assert(!nh_memory_credit_owned(&l,1,(void *)(uintptr_t)0x400000,1048576));assert(!nh_memory_credit_owned(&l,2,(void *)(uintptr_t)0x800000,1048576));
 assert(nh_memory_credit_owned(&l,2,(void *)(uintptr_t)0x800000,1048576));cases++;
 assert(nh_memory_credit_owned(&l,0,(void *)(UINTPTR_MAX-3),100));cases++;
 assert(!nh_memory_evaluate(&l,&m,&m,&z,1000,2000,NH_MEM_PRE_NEXT,&o));assert(o.dma_debt==0&&o.cpu_debt==100&&o.required_available==1636&&o.required_noncma==1636&&!o.migration_future_kib);cases++;
 /* Actual h values must HOLD even after a prefault loan: resident does NOT
  * establish CMA survival. Unknown other occupants stay charged. */
 struct nh_memory_ledger forecast;assert(!nh_memory_init(&forecast,3864,88));
 assert(nh_memory_evaluate(&forecast,&m,&m,&z,1000,2000,NH_MEM_INITIAL,&o)==NH_MEM_LOW_NONCMA);assert(o.required_noncma>=5400);cases++;
 assert(!nh_memory_credit_owned(&forecast,0,(void *)(uintptr_t)0x100000,1728000));
 assert(nh_memory_evaluate(&forecast,&m,&m,&z,1000,2000,NH_MEM_PRE_NEXT,&o)==NH_MEM_LOW_NONCMA&&o.migration_future_kib==7168);cases++;
 struct nh_memory_ledger alias;assert(!nh_memory_init_mode(&alias,2836,88,1));
 assert(nh_memory_evaluate(&alias,&m,&m,&z,1000,2000,NH_MEM_INITIAL,&o)==NH_MEM_LOW_NONCMA);assert(o.required_available==12628&&o.required_noncma==11540);cases++;
 assert(!nh_memory_credit_owned(&alias,0,(void *)(uintptr_t)0x100000,1728000));
 assert(nh_memory_credit_owned(&alias,2,(void *)(uintptr_t)0x100000,1048576));cases++;
 printf("HOST strictMemoryReserve %u public synthetic parser/overflow/layout/boost/no-double-CMA/stale/debt/prefault-credit/floors cases PASS; NO_PROC/NO_TARGET\n",cases);return 0;
}
