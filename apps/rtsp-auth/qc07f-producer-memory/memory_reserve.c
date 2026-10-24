/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded public metadata parser + conservative debt/reserve arithmetic.
 * Known sealed75 layout: Node0 Normal + unpopulated Node0 Movable.
 * zoneinfo high/min/low/promo ALREADY include boost (pinned mmzone/vmstat).
 */
#include "memory_reserve.h"
#include <elf.h>
#include <string.h>
struct line {const char *p,*end;};
static int white(char c){return c==' '||c=='\t'||c=='\r';}
static void skip(struct line *l){while(l->p<l->end&&white(*l->p))l->p++;}
static int word(struct line *l,const char *s)
{size_t n=strlen(s);skip(l);if((size_t)(l->end-l->p)<n||memcmp(l->p,s,n))return -1;l->p+=n;return 0;}
static int number(struct line *l,uint64_t *n)
{
 skip(l);uint64_t v=0;unsigned count=0;
 while(l->p<l->end&&*l->p>='0'&&*l->p<='9'){
  unsigned d=(unsigned)(*l->p++-'0');if(v>(UINT64_MAX-d)/10)return -1;v=v*10+d;count++;
 }
 if(!count)return -1;*n=v;return 0;
}
static int finish(struct line *l){skip(l);return l->p==l->end?0:-1;}
static int next_line(const char **p,const char *end,struct line *l)
{
 if(*p>=end)return 0;l->p=*p;const char *q=memchr(*p,'\n',(size_t)(end-*p));
 if(!q)return -1;l->end=q;*p=q+1;if(l->end-l->p>255)return -1;return 1;
}
static int scalar(struct line l,const char *key,uint64_t *v,int unit)
{if(word(&l,key)||number(&l,v)||(unit&&word(&l,"kB"))||finish(&l))return -1;return 0;}
int nh_memory_parse_meminfo(const char *data,size_t bytes,struct nh_memory_values *out)
{
 if(!data||!out||!bytes||bytes>NH_MEMINFO_CAP||memchr(data,0,bytes))return -1;
 const char *p=data,*end=data+bytes;struct nh_memory_values r={0};unsigned seen=0;struct line l;int status;
 const char *keys[]={"MemTotal:","MemFree:","MemAvailable:","CmaTotal:","CmaFree:","SwapTotal:"};
 uint64_t *values[]={&r.total,&r.free,&r.available,&r.cma_total,&r.cma_free,&r.swap_total};
 while((status=next_line(&p,end,&l))>0){
  struct line t=l;skip(&t);
  for(unsigned i=0;i<6;i++)if((size_t)(t.end-t.p)>=strlen(keys[i])&&!memcmp(t.p,keys[i],strlen(keys[i]))){
   if(seen&(1U<<i)||scalar(l,keys[i],values[i],1))return -1;seen|=1U<<i;break;
  }
 }
 if(status<0||seen!=63||r.total!=8286ULL*4||r.free>r.total||r.available>r.total||
    r.cma_total!=16384||r.cma_free>r.cma_total||r.cma_free>r.free||r.swap_total)return -1;
 *out=r;return 0;
}
int nh_memory_parse_zoneinfo(const char *data,size_t bytes,struct nh_memory_zones *out)
{
 if(!data||!out||!bytes||bytes>NH_ZONEINFO_CAP||memchr(data,0,bytes))return -1;
 struct nh_memory_zones r={0};struct nh_memory_zone *z=NULL;unsigned zones=0,seen=0;int status;struct line l;
 const char *p=data,*end=data+bytes;
 const char *keys[]={"pages free","boost","min","low","high","promo","spanned","present","managed","cma"};
 while((status=next_line(&p,end,&l))>0){
  struct line t=l;skip(&t);
  if((size_t)(t.end-t.p)>=5&&!memcmp(t.p,"Node ",5)){
   if(z&&seen!=(zones==1?4095U:2047U))return -1;
   if(zones==0){if(word(&t,"Node")||word(&t,"0,")||word(&t,"zone")||word(&t,"Normal")||finish(&t))return -1;z=&r.normal;}
   else if(zones==1){if(word(&t,"Node")||word(&t,"0,")||word(&t,"zone")||word(&t,"Movable")||finish(&t))return -1;z=&r.movable;}
   else return -1;
   zones++;seen=0;continue;
  }
  if(!z){if(finish(&t))return -1;continue;}
  if((size_t)(t.end-t.p)>=11&&!memcmp(t.p,"protection:",11)){
   if(seen&1024||word(&t,"protection:")||word(&t,"(")||number(&t,&z->protection[0])||word(&t,",")||number(&t,&z->protection[1])||word(&t,")")||finish(&t))return -1;
   seen|=1024;continue;
  }
  if((size_t)(t.end-t.p)>11&&!memcmp(t.p,"nr_free_cma",11)&&white(t.p[11])){
   if(z!=&r.normal||(seen&2048)||scalar(l,"nr_free_cma",&z->free_cma,0))return -1;seen|=2048;continue;
  }
  uint64_t *v[]={&z->free,&z->boost,&z->min,&z->low,&z->high,&z->promo,&z->spanned,&z->present,&z->managed,&z->cma};
  for(unsigned i=0;i<10;i++){
   size_t n=strlen(keys[i]);
   if((size_t)(t.end-t.p)>n&&!memcmp(t.p,keys[i],n)&&white(t.p[n])){
    if(seen&(1U<<i)||scalar(l,keys[i],v[i],0))return -1;seen|=1U<<i;break;
   }
  }
 }
 if(status<0||zones!=2||seen!=2047)return -1;
 z=&r.normal;
 if(z->spanned!=10240||z->present!=10240||z->managed!=8286||z->cma!=4096||z->free>z->managed||z->free_cma>z->free||z->free_cma>z->cma||
    z->protection[0]||z->protection[1]||z->min<z->boost||z->low<z->boost||z->high<z->boost||z->promo<z->boost||
    z->min-z->boost!=178||z->low-z->boost!=222||z->high-z->boost!=266||z->promo-z->boost!=310)return -1;
 z=&r.movable;
 if(z->free||z->boost||z->spanned||z->present||z->managed||z->cma||z->protection[0]||z->protection[1]||
    z->min!=32||z->low!=32||z->high!=32||z->promo!=32)return -1;
 /* calculate_totalreserve_pages: min(managed, max(protection)+high).
  * high already includes boost. Empty Movable has managed0 => zero reserve. */
 uint64_t w=r.normal.high<r.normal.managed?r.normal.high:r.normal.managed;
 r.watermark_kib=w*4;*out=r;return 0;
}
int nh_memory_init(struct nh_memory_ledger *l,uint64_t capacity,uint64_t nonmovable)
{return nh_memory_init_mode(l,capacity,nonmovable,0);}
int nh_memory_init_mode(struct nh_memory_ledger *l,uint64_t capacity,uint64_t nonmovable,unsigned raw_reuse)
{
 if(!l||raw_reuse>1||capacity<(raw_reuse?NH_MEMORY_RAW_REUSE_MIN_CPU_KIB:NH_MEMORY_MIN_CPU_KIB)||capacity>16384||nonmovable>capacity)return -1;
 memset(l,0,sizeof(*l));l->cpu_capacity_kib=capacity;l->nonmovable_cpu_debt_kib=nonmovable;l->initialized=1;l->raw_reuse=raw_reuse;return 0;
}
static int overlaps(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d)
{return a<d&&b>c;}
/* Division/remainder avoids a+4095 overflow on a 64-bit HOST uintptr_t. */
static uint64_t interior_kib(uintptr_t a,uintptr_t b)
{
 uint64_t first=a/4096+(a%4096!=0),last=b/4096;
 return first<last?(last-first)*4:0;
}
int nh_memory_credit_owned(struct nh_memory_ledger *l,unsigned kind,const void *data,size_t bytes)
{
 if(kind==1)return -1; /* never inspect/touch a purported flat hub */
 if(!l||!l->initialized||kind>=3||(l->raw_reuse&&kind==2)||l->paid[kind]||!data||!bytes||bytes>1728000||bytes>UINTPTR_MAX-(uintptr_t)data||
    l->cpu_paid_kib>l->cpu_capacity_kib||l->hub.count>NH_MEMORY_HUB_RANGES)return -1;
 uintptr_t a=(uintptr_t)data,b=a+bytes;
 /* Credit full interior pages only; no double credit of shared boundary pages
  * or allocator padding whose residency was not directly touched. */
 uint64_t credit=interior_kib(a,b);if(!credit)return -1;
 for(unsigned i=0;i<3;i++)if(l->paid[i]&&overlaps(a,b,l->begin[i],l->end[i]))return -1;
 for(unsigned i=0;i<l->hub.count;i++)if(overlaps(a,b,l->hub.begin[i],l->hub.end[i]))return -1;
 if(credit>l->cpu_capacity_kib-l->cpu_paid_kib)return -1;
 l->begin[kind]=a;l->end[kind]=b;l->paid[kind]=1;l->cpu_paid_kib+=credit;return 0;
}
_Static_assert(CAMERA_SPAN_HUB_LAYOUT_VERSION==3U&&CAMERA_SPAN_HUB_PRODUCER_API_VERSION==3U,
 "exclusive producer-layout source selection; rebuild every consumer");
_Static_assert(CAMERA_SPAN_HUB_V1_CHUNKS==NH_MEMORY_HUB_RANGES&&
 CAMERA_SPAN_HUB_V1_CHUNK_BYTES==NH_MEMORY_HUB_BYTES&&
 NH_MEMORY_HUB_RANGES*NH_MEMORY_HUB_BYTES==1048576U,"exact accepted producer backing");
_Static_assert(sizeof(struct nh_memory_hub_ranges)<512&&sizeof(struct nh_memory_ledger)<1024,
 "bounded metadata, no full-AU allocation");
int nh_memory_span_hub_forecast(const struct nh_memory_ledger *l,
 const struct camera_span_hub_v1 *h,struct nh_memory_hub_ranges *out)
{
 if(!l||!l->initialized||!h||!out||l->paid[1]||l->begin[1]||l->end[1]||
    l->hub.owner||l->hub.count||l->hub.credit_kib||l->cpu_paid_kib>l->cpu_capacity_kib)return -1;
 if(h->layout_version!=3U||h->writer_phase!=CAMERA_SPAN_WRITER_V3_IDLE||
    h->issued_writer_token||h->writer_token||h->writer_generation||
    h->armed_writer_token||h->writer_error||h->data_offset)return -1;
 if(h->self!=h||h->phase!=CAMERA_SPAN_HUB_V1_ACTIVE||h->generation||h->issued_token||
    h->monotonic_ns||h->codec||h->keyframe||h->subscribers||h->need_keyframe||h->pending||h->acquired||h->bytes||
    h->allocation_calls!=8||h->allocated_bytes!=1048576||h->maximum_allocation!=131072)return -1;
 for(unsigned i=0;i<CAMERA_SPAN_HUB_V1_CONSUMERS;i++)if(h->active_token[i])return -1;
 struct nh_memory_hub_ranges next={.owner=h,.count=NH_MEMORY_HUB_RANGES};
 for(unsigned i=0;i<NH_MEMORY_HUB_RANGES;i++){
  if(l->hub.begin[i]||l->hub.end[i]||!h->chunks[i])return -1;
  uintptr_t a=(uintptr_t)h->chunks[i];if(NH_MEMORY_HUB_BYTES>UINTPTR_MAX-a)return -1;
  uintptr_t b=a+NH_MEMORY_HUB_BYTES;
  for(unsigned j=0;j<i;j++)if(overlaps(a,b,next.begin[j],next.end[j]))return -1;
  for(unsigned j=0;j<3;j++)if(l->paid[j]&&overlaps(a,b,l->begin[j],l->end[j]))return -1;
  uint64_t credit=interior_kib(a,b);
  if(!credit||credit>UINT64_MAX-next.credit_kib)return -1;
  next.begin[i]=a;next.end[i]=b;next.credit_kib+=credit;
 }
 if(next.credit_kib>l->cpu_capacity_kib-l->cpu_paid_kib)return -1;
 *out=next;return 0;
}
int nh_memory_credit_span_hub(struct nh_memory_ledger *l,const struct camera_span_hub_v1 *h)
{
 struct nh_memory_hub_ranges next;
 if(nh_memory_span_hub_forecast(l,h,&next))return -1;
 /* Logical atomic whole-kind commit: no fallible operation/callback from here.
  * Scalar hub slots remain empty. Never pays unrequested boundary/padding pages. */
 l->hub=next;l->cpu_paid_kib+=next.credit_kib;l->paid[1]=1;return 0;
}
int nh_memory_credit_codec(struct nh_memory_ledger *l,uint64_t bytes,unsigned healthy)
{
 if(!l||!l->initialized||!healthy||bytes!=8454144)return -1;l->codec_paid=1;return 0;
}
int nh_memory_evaluate(const struct nh_memory_ledger *l,const struct nh_memory_values *a,
 const struct nh_memory_values *b,const struct nh_memory_zones *z,uint64_t begin,uint64_t end,
 enum nh_memory_stage stage,struct nh_memory_observation *out)
{
 if(!l||!a||!b||!z||!out||!l->initialized||l->cpu_paid_kib>l->cpu_capacity_kib)return NH_MEM_FORMAT;
 struct nh_memory_observation o={.begin_ns=begin,.end_ns=end,.stage=stage,.reason=NH_MEM_OK};
 if(begin==UINT64_MAX||end==UINT64_MAX||end<begin)o.reason=NH_MEM_CLOCK;
 else if(end-begin>NH_MEMORY_MAX_SAMPLE_NS)o.reason=NH_MEM_STALE;
 o.available=a->available<b->available?a->available:b->available;
 uint64_t free=a->free<b->free?a->free:b->free,cma=a->cma_free>b->cma_free?a->cma_free:b->cma_free;
 o.watermark_kib=z->watermark_kib;o.cpu_debt=l->cpu_capacity_kib-l->cpu_paid_kib;o.dma_debt=l->codec_paid?0:NH_MEMORY_CODEC_KIB;
 o.required_available=o.cpu_debt+o.dma_debt+NH_MEMORY_OVERHEAD_KIB+NH_MEMORY_RESERVE_KIB;
 /* Full unpaid CPU may land in Normal. Resident owned pages may have borrowed
  * CMA and can be evacuated by initial codec allocation. Without qualified
  * placement proof, ALL current occupied CMA is potentially migratable, capped
  * by the still-unpaid codec ranges. This includes own loans without double
  * adding them; known-nonmovable ISP is NOT subtracted on a placement guess. */
 uint64_t minimum_cma=a->cma_free<b->cma_free?a->cma_free:b->cma_free;
 if(z->normal.free_cma*4<minimum_cma)minimum_cma=z->normal.free_cma*4;
 uint64_t occupied=a->cma_total>=minimum_cma?a->cma_total-minimum_cma:UINT64_MAX;
 o.migration_future_kib=l->codec_paid?0:(occupied<o.dma_debt?occupied:o.dma_debt);
 o.required_noncma=o.cpu_debt+o.migration_future_kib+NH_MEMORY_OVERHEAD_KIB+NH_MEMORY_RESERVE_KIB;
 uint64_t zone_noncma=(z->normal.free-z->normal.free_cma)*4;
 if(free>=cma&&zone_noncma<free-cma)free=zone_noncma+cma;
 if(free<cma||free-cma<z->watermark_kib){o.noncma_free=0;if(!o.reason)o.reason=NH_MEM_LOW_NONCMA;}
 else o.noncma_free=free-cma-z->watermark_kib;
 if(!o.reason&&o.available<o.required_available)o.reason=NH_MEM_LOW_AVAILABLE;
 if(!o.reason&&o.noncma_free<o.required_noncma)o.reason=NH_MEM_LOW_NONCMA;
 *out=o;return (int)o.reason;
}
int nh_memory_own_layout(uint64_t *capacity,uint64_t *nonmovable,unsigned raw_reuse)
{
 if(raw_reuse>1)return -1;
#if defined(__arm__)
 extern const unsigned char __ehdr_start[];const Elf32_Ehdr *e=(const Elf32_Ehdr *)__ehdr_start;
 if(memcmp(e->e_ident,ELFMAG,4)||e->e_ident[EI_CLASS]!=ELFCLASS32||e->e_ident[EI_DATA]!=ELFDATA2LSB||e->e_machine!=EM_ARM||e->e_type!=ET_EXEC||
    e->e_phentsize!=sizeof(Elf32_Phdr)||!e->e_phnum||e->e_phnum>8||e->e_phoff>4096||e->e_phoff+e->e_phnum*sizeof(Elf32_Phdr)>4096||
    e->e_shentsize!=sizeof(Elf32_Shdr)||!e->e_shnum||e->e_shnum>64)return -1;
 const Elf32_Phdr *p=(const Elf32_Phdr *)(__ehdr_start+e->e_phoff);uint64_t mapped=0,rx_file_pages=0;unsigned loads=0;
 for(unsigned i=0;i<e->e_phnum;i++)if(p[i].p_type==PT_LOAD){
  uint64_t v=p[i].p_vaddr,size=p[i].p_memsz,offset=p[i].p_offset,file=p[i].p_filesz;
  if(!size||size<file||v+size>UINT32_MAX||offset+file>UINT32_MAX||loads>=2)return -1;
  mapped+=((v+size+4095)/4096-v/4096)*4;loads++;
  if((p[i].p_flags&(PF_X|PF_W))==PF_X)rx_file_pages+=(offset+file+4095)/4096-offset/4096;
 }
 uint64_t file_end=(uint64_t)e->e_shoff+(uint64_t)e->e_shnum*e->e_shentsize,file_pages=(file_end+4095)/4096;
 if(loads!=2||!rx_file_pages||file_pages<rx_file_pages||file_pages>256)return -1;
 uint64_t stage=(file_pages-rx_file_pages)*4; /* nonshared data/trailer staging; RX not counted twice */
 uint64_t minimum=raw_reuse?2832:3860;
 /* Eight user extents: 8*33 pages vs old flat257; NOT allocator overhead. */
 *capacity=mapped+1056+(raw_reuse?0:1028)+stage;if(*capacity<minimum)*capacity=minimum;
 *nonmovable=rx_file_pages*4+stage;return 0;
#else
 (void)capacity;(void)nonmovable;return -1; /* no unqualified HOST proc/runtime fallback */
#endif
}
