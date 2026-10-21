/* SPDX-License-Identifier: GPL-2.0-only */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <mbedtls/platform_util.h>
/* Link wrappers cover actual Mongoose/Mbed TLS/application explicit allocations,
 * not libc internal allocations, kernel buffers, stack or allocator overhead.
 * Single owned parent loop only. Child uses original strict broker allocator. */
void *__real_malloc(size_t);
void __real_free(void *);
static struct {void *ptr;size_t size;} slots[1024];
static size_t used,peak;
static int failed;
size_t web_heap_peak(void){return peak;}
int web_heap_failed(void){return failed;}
void *__wrap_malloc(size_t n)
{
 size_t i;void *p;
 if(!n)n=1;
 if(n>131072||n>1048576-used){failed=1;return NULL;}
 for(i=0;i<1024&&slots[i].ptr;i++){}
 if(i==1024){failed=1;return NULL;}
 p=__real_malloc(n);if(!p){failed=1;return NULL;}
 slots[i].ptr=p;slots[i].size=n;used+=n;if(used>peak)peak=used;return p;
}
void *__wrap_calloc(size_t n,size_t m)
{
 void *p;if(m&&n>SIZE_MAX/m){failed=1;return NULL;}
 p=__wrap_malloc(n*m);if(p)memset(p,0,n*m);return p;
}
void __wrap_free(void *p)
{
 size_t i;if(!p)return;
 for(i=0;i<1024;i++)if(slots[i].ptr==p){
  mbedtls_platform_zeroize(p,slots[i].size);used-=slots[i].size;
  slots[i].ptr=NULL;slots[i].size=0;__real_free(p);return;
 }
 /* A foreign pointer is an ownership failure, never inspect/free it. */
 failed=1;
}
void *__wrap_realloc(void *p,size_t n)
{
 size_t i;void *q;if(!p)return __wrap_malloc(n);
 if(!n){__wrap_free(p);return NULL;}
 for(i=0;i<1024&&slots[i].ptr!=p;i++){}
 if(i==1024){failed=1;return NULL;}
 q=__wrap_malloc(n);if(!q)return NULL;
 memcpy(q,p,n<slots[i].size?n:slots[i].size);__wrap_free(p);return q;
}
