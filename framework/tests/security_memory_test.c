/* SPDX-License-Identifier: GPL-2.0-only */
#include "security_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct backend {
 unsigned calls,frees,fail;void *duplicate;
 struct security_allocation live[SECURITY_MEMORY_SLOTS];
};
static void *allocate(void *context,size_t bytes)
{
 struct backend *b=context;b->calls++;
 if(b->fail)return NULL;
 if(b->duplicate)return b->duplicate;
 void *p=malloc(bytes);assert(p);memset(p,0xff,bytes);
 unsigned i;for(i=0;i<SECURITY_MEMORY_SLOTS&&b->live[i].pointer;i++);
 assert(i<SECURITY_MEMORY_SLOTS);b->live[i]=(struct security_allocation){p,bytes};return p;
}
static void deallocate(void *context,void *pointer)
{
 struct backend *b=context;unsigned i;
 for(i=0;i<SECURITY_MEMORY_SLOTS&&b->live[i].pointer!=pointer;i++);
 assert(i<SECURITY_MEMORY_SLOTS);
 const unsigned char *p=pointer;for(size_t n=0;n<b->live[i].bytes;n++)assert(!p[n]);
 b->live[i]=(struct security_allocation){0};b->frees++;free(pointer);
}
static void allocator_fixtures(void)
{
 struct security_memory m;struct backend b={0};
 assert(security_memory_init(&m,64,32,allocate,deallocate,&b)==0);
 void *p=security_memory_calloc(&m,2,16);assert(p&&m.used==32&&m.high_water==32);
 for(unsigned i=0;i<32;i++)assert(!((unsigned char *)p)[i]);
 void *q=security_memory_calloc(&m,1,32);assert(q&&m.used==64&&m.active==2);
 assert(!security_memory_calloc(&m,1,1)&&m.last_status==SECURITY_RESOURCE_LIMIT);
 assert(!security_memory_calloc(&m,1,33)&&m.last_status==SECURITY_RESOURCE_LIMIT);
 assert(!security_memory_calloc(&m,SIZE_MAX,2)&&m.last_status==SECURITY_RESOURCE_INVALID);
 assert(!security_memory_calloc(&m,0,2)&&m.last_status==SECURITY_RESOURCE_INVALID);
 assert(!security_memory_calloc(&m,1,0)&&m.last_status==SECURITY_RESOURCE_INVALID);
 assert(b.calls==2&&m.used==64&&m.high_water==64&&m.high_active==2);
 memset(p,0xa5,32);assert(security_memory_free(&m,p)==0&&m.used==32&&b.frees==1);
 assert(security_memory_free(&m,q)==0&&m.used==0&&m.high_water==64);
 assert(security_memory_free(&m,NULL)==0);
 b.fail=1;assert(!security_memory_calloc(&m,1,16));
 assert(m.last_status==SECURITY_RESOURCE_ALLOC_FAILED&&m.used==0&&!m.fatal);
 b.fail=0;p=security_memory_calloc(&m,1,16);assert(p);
 assert(security_memory_free(&m,p)==0);
 /* Double free: compare ownership only, never touch pointer's former bytes. */
 assert(security_memory_free(&m,p)==SECURITY_RESOURCE_OWNERSHIP&&b.frees==3);
 assert(m.fatal&&!security_memory_calloc(&m,1,1));

 b=(struct backend){0};assert(security_memory_init(&m,64,32,allocate,deallocate,&b)==0);
 p=security_memory_calloc(&m,1,16);assert(p);
 unsigned char foreign[16];assert(security_memory_free(&m,foreign)==SECURITY_RESOURCE_OWNERSHIP);
 assert(b.frees==0&&m.used==16&&m.fatal);
 assert(security_memory_free(&m,p)==0&&!m.used); /* Cleanup even with integrity latch. */

 b=(struct backend){0};assert(security_memory_init(&m,64,32,allocate,deallocate,&b)==0);
 p=security_memory_calloc(&m,1,16);assert(p);memset(p,0xa5,16);b.duplicate=p;
 assert(!security_memory_calloc(&m,1,16)&&m.fatal&&m.used==16&&m.active==1);
 assert(((unsigned char *)p)[0]==0xa5&&b.frees==0);
 assert(security_memory_free(&m,p)==0);b.duplicate=NULL;

 b=(struct backend){0};assert(security_memory_init(&m,1024,16,allocate,deallocate,&b)==0);
 void *slots[SECURITY_MEMORY_SLOTS];
 for(unsigned i=0;i<SECURITY_MEMORY_SLOTS;i++){slots[i]=security_memory_calloc(&m,1,1);assert(slots[i]);}
 assert(!security_memory_calloc(&m,1,1)&&m.last_status==SECURITY_RESOURCE_LIMIT);
 assert(m.used==SECURITY_MEMORY_SLOTS&&m.active==SECURITY_MEMORY_SLOTS);
 for(unsigned i=0;i<SECURITY_MEMORY_SLOTS;i++)assert(security_memory_free(&m,slots[i])==0);
 assert(m.used==0&&m.active==0&&m.high_active==SECURITY_MEMORY_SLOTS);
 assert(security_memory_init(NULL,1,1,NULL,NULL,NULL)==SECURITY_RESOURCE_INVALID);
 assert(security_memory_init(&m,0,1,NULL,NULL,NULL)==SECURITY_RESOURCE_INVALID);
 assert(security_memory_init(&m,SECURITY_HEAP_LIMIT_BYTES+1,1,NULL,NULL,NULL)==SECURITY_RESOURCE_INVALID);
 assert(security_memory_init(&m,SECURITY_HEAP_LIMIT_BYTES,SECURITY_ALLOCATION_LIMIT_BYTES+1,
  NULL,NULL,NULL)==SECURITY_RESOURCE_INVALID);
 assert(security_memory_init(&m,1,2,NULL,NULL,NULL)==SECURITY_RESOURCE_INVALID);
 assert(security_memory_init(&m,1,1,allocate,NULL,&b)==SECURITY_RESOURCE_INVALID);
 assert(!security_memory_calloc(NULL,1,1));
 assert(security_memory_free(NULL,NULL)==SECURITY_RESOURCE_INVALID);
 puts("PASS bounded allocator: zeroing/quota/high-water/multiply-overflow/fault/slots/foreign-double-free/duplicate-backend");
}
static ssize_t fake_random(void *context,void *output,size_t bytes,unsigned flags,int *error)
{(void)context;(void)flags;*error=0;memset(output,0xa5,bytes);return (ssize_t)bytes;}
static int init_psa(void *context){(void)context;return 0;}
static void admission_fixtures(void)
{
 struct security_rng r;struct security_memory m;struct security_admission a,other;
 struct security_peer_token first,second,extra;
 assert(SECURITY_USABLE_RAM_BYTES==0x2800000U&&SECURITY_NONCMA_BYTES==0x1800000U);
 assert(security_rng_init(&r,fake_random,NULL)==0);
 assert(security_memory_init(&m,SECURITY_HEAP_LIMIT_BYTES,SECURITY_ALLOCATION_LIMIT_BYTES,
  NULL,NULL,NULL)==0);
 assert(security_admission_init(&a,&r,&m)==0);
 assert(security_admission_open(&a,0,&first)==SECURITY_RESOURCE_NOT_READY&&a.stopped);
 assert(security_rng_start(&r,init_psa,NULL)==0);
 assert(security_admission_init(&a,&r,&m)==0);
 assert(security_admission_open(&a,1,&first)==0);
 assert(security_admission_open(&a,1,&second)==0);
 assert(security_admission_open(&a,1,&extra)==SECURITY_RESOURCE_LIMIT&&!extra.id);
 assert(a.connections==2&&a.high_connections==2);
 assert(security_admission_send_reserve(&a,first,1)==SECURITY_RESOURCE_NOT_READY);
 assert(security_admission_handshake_begin(&a,first,2)==0);
 assert(security_admission_handshake_begin(&a,first,2)==SECURITY_RESOURCE_INVALID);
 assert(security_admission_handshake_begin(&a,second,2)==SECURITY_RESOURCE_LIMIT);
 assert(a.handshakes==1&&a.high_handshakes==1);
 assert(security_admission_handshake_finish(&a,first,3,1)==0&&!a.handshakes);
 assert(security_admission_handshake_begin(&a,second,3)==0);
 assert(security_admission_handshake_finish(&a,second,4,1)==0);
 assert(security_admission_send_reserve(&a,first,SECURITY_SEND_PEER_BYTES)==0);
 assert(security_admission_send_reserve(&a,first,1)==SECURITY_RESOURCE_LIMIT);
 assert(security_admission_send_reserve(&a,second,SECURITY_SEND_PEER_BYTES)==0);
 assert(a.send_bytes==SECURITY_SEND_TOTAL_BYTES&&a.high_send==SECURITY_SEND_TOTAL_BYTES);
 assert(security_admission_send_reserve(&a,second,SIZE_MAX)==SECURITY_RESOURCE_LIMIT);
 assert(security_admission_send_release(&a,first,SECURITY_SEND_PEER_BYTES+1)==SECURITY_RESOURCE_INVALID);
 assert(a.send_bytes==SECURITY_SEND_TOTAL_BYTES);
 assert(security_admission_send_release(&a,first,4096)==0);
 assert(security_admission_send_reserve(&a,first,4096)==0);
 assert(security_admission_expire(&a,6000)==0); /* Completed TLS is not pre-TLS. */
 assert(security_admission_close(&a,first)==0&&a.send_bytes==SECURITY_SEND_PEER_BYTES);
 assert(security_admission_close(&a,first)==SECURITY_RESOURCE_STALE);
 assert(security_admission_open(&a,6000,&extra)==0&&extra.id!=first.id);
 assert(security_admission_send_release(&a,first,1)==SECURITY_RESOURCE_STALE);
 assert(security_admission_close(&a,extra)==0);
 assert(security_admission_init(&other,&r,&m)==0);
 assert(security_admission_close(&other,second)==SECURITY_RESOURCE_STALE);
 assert(security_admission_close(&a,second)==0&&!a.connections&&!a.send_bytes);

 assert(security_admission_open(&a,6001,&first)==0);
 assert(security_admission_handshake_begin(&a,first,6002)==0);
 assert(security_admission_handshake_finish(&a,first,6003,0)==SECURITY_RESOURCE_NOT_READY);
 assert(!a.connections&&!a.handshakes);
 assert(security_admission_open(&a,6004,&first)==0);
 assert(security_admission_handshake_begin(&a,first,6005)==0);
 assert(security_admission_handshake_finish(&a,first,11004,1)==SECURITY_RESOURCE_TIMEOUT);
 assert(!a.connections&&!a.handshakes);
 assert(security_admission_open(&a,11005,&first)==0);
 assert(security_admission_open(&a,11005,&second)==0);
 assert(security_admission_handshake_begin(&a,first,11006)==0);
 assert(security_admission_expire(&a,16004)==0);
 assert(security_admission_expire(&a,16005)==2&&!a.connections&&!a.handshakes);
 assert(security_admission_open(&a,UINT64_MAX,&first)==SECURITY_RESOURCE_LIMIT);

 assert(security_admission_init(&a,&r,&m)==0);
 assert(security_admission_open(&a,1,&first)==0);
 assert(security_admission_handshake_begin(&a,first,0)==SECURITY_RESOURCE_INVALID);
 assert(a.stopped&&!a.connections);
 assert(security_admission_init(&a,&r,&m)==0);
 assert(security_admission_open(&a,1,&first)==0);
 /* Allocator integrity failure closes the independent service gate. */
 int foreign;assert(security_memory_free(&m,&foreign)==SECURITY_RESOURCE_OWNERSHIP);
 assert(security_admission_send_reserve(&a,first,1)==SECURITY_RESOURCE_NOT_READY);
 assert(a.stopped&&!a.connections&&!a.handshakes&&!a.send_bytes);
 assert(security_admission_expire(&a,2)==SECURITY_RESOURCE_NOT_READY);
 puts("PASS admission: startup gate/two-peers/one-handshake/deadlines/send-cap/overflow/stale-token/clock-regression/fatal-halt");
}
int main(void){allocator_fixtures();admission_fixtures();return 0;}
