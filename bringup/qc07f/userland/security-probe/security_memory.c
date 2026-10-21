/* SPDX-License-Identifier: GPL-2.0-only */
#include "security_memory.h"
#include <stdlib.h>
#include <string.h>
#ifdef SECURITY_WITH_MBEDTLS
#include "mongoose.h"
#include <mbedtls/platform.h>
#endif
static void *os_allocate(void *context,size_t bytes)
{(void)context;return malloc(bytes);}
static void os_deallocate(void *context,void *pointer)
{(void)context;free(pointer);}
static void increment(uint64_t *value)
{if(*value!=UINT64_MAX)(*value)++;}
static void *allocation_failure(struct security_memory *m,int status)
{if(m&&m->initialized){m->last_status=status;increment(&m->failures);}return NULL;}
int security_memory_init(struct security_memory *m,size_t limit,size_t single,
 security_allocate_fn allocate,security_deallocate_fn deallocate,void *context)
{
 if(!m||!limit||limit>SECURITY_HEAP_LIMIT_BYTES||!single||single>limit||
  single>SECURITY_ALLOCATION_LIMIT_BYTES||(!allocate!=!deallocate))return SECURITY_RESOURCE_INVALID;
 memset(m,0,sizeof(*m));m->allocate=allocate?allocate:os_allocate;
 m->deallocate=deallocate?deallocate:os_deallocate;m->context=context;
 m->limit=limit;m->single_limit=single;m->initialized=1;return SECURITY_RESOURCE_OK;
}
void *security_memory_calloc(struct security_memory *m,size_t count,size_t bytes)
{
 if(!m||!m->initialized)return NULL;
 if(m->fatal)return allocation_failure(m,SECURITY_RESOURCE_OWNERSHIP);
 if(!count||!bytes||bytes>SIZE_MAX/count)return allocation_failure(m,SECURITY_RESOURCE_INVALID);
 size_t total=count*bytes;
 if(total>m->single_limit||total>m->limit-m->used||m->active==SECURITY_MEMORY_SLOTS)
  return allocation_failure(m,SECURITY_RESOURCE_LIMIT);
 unsigned slot;for(slot=0;slot<SECURITY_MEMORY_SLOTS&&m->owned[slot].pointer;slot++);
 if(slot==SECURITY_MEMORY_SLOTS)return allocation_failure(m,SECURITY_RESOURCE_LIMIT);
 void *p=m->allocate(m->context,total);
 if(!p)return allocation_failure(m,SECURITY_RESOURCE_ALLOC_FAILED);
 for(unsigned i=0;i<SECURITY_MEMORY_SLOTS;i++)if(m->owned[i].pointer==p){
  m->fatal=1;return allocation_failure(m,SECURITY_RESOURCE_OWNERSHIP);
 }
 memset(p,0,total);m->owned[slot]=(struct security_allocation){p,total};
 m->used+=total;m->active++;increment(&m->allocations);
 if(m->used>m->high_water)m->high_water=m->used;
 if(m->active>m->high_active)m->high_active=m->active;
 m->last_status=SECURITY_RESOURCE_OK;return p;
}
int security_memory_free(struct security_memory *m,void *p)
{
 if(!m||!m->initialized)return SECURITY_RESOURCE_INVALID;
 if(!p)return SECURITY_RESOURCE_OK;
 for(unsigned i=0;i<SECURITY_MEMORY_SLOTS;i++)if(m->owned[i].pointer==p){
  size_t bytes=m->owned[i].bytes;
  security_rng_zero(p,bytes);m->owned[i]=(struct security_allocation){0};
  m->used-=bytes;m->active--;m->deallocate(m->context,p);
  m->last_status=SECURITY_RESOURCE_OK;return SECURITY_RESOURCE_OK;
 }
 m->fatal=1;allocation_failure(m,SECURITY_RESOURCE_OWNERSHIP);
 return SECURITY_RESOURCE_OWNERSHIP;
}
#ifdef SECURITY_WITH_MBEDTLS
static struct security_memory *library_memory;
static void *library_calloc(size_t count,size_t bytes)
{return security_memory_calloc(library_memory,count,bytes);}
static void library_free(void *p)
{(void)security_memory_free(library_memory,p);}
int security_memory_bind_library(struct security_memory *m)
{
 if(!m||!m->initialized||m->active||library_memory)return SECURITY_RESOURCE_INVALID;
 /* Setter performs no allocations. Bind BEFORE PSA makes any global objects. */
 library_memory=m;
 if(mbedtls_platform_set_calloc_free(library_calloc,library_free)){
  library_memory=NULL;return SECURITY_RESOURCE_INVALID;
 }
 return SECURITY_RESOURCE_OK;
}
void *mg_calloc(size_t count,size_t bytes){return library_calloc(count,bytes);}
void mg_free(void *p){library_free(p);}
#endif
static int gate(struct security_admission *a)
{
 if(!a||!a->initialized||a->stopped)return SECURITY_RESOURCE_NOT_READY;
 if(!security_rng_can_serve(a->rng)||!a->memory->initialized||a->memory->fatal){
  security_admission_halt(a);return SECURITY_RESOURCE_NOT_READY;
 }
 return SECURITY_RESOURCE_OK;
}
static struct security_peer *peer(struct security_admission *a,struct security_peer_token t)
{
 if(!a||!a->initialized||t.owner!=a||!t.id||t.slot>=SECURITY_HTTPS_PEERS||
  a->peers[t.slot].id!=t.id)return NULL;
 return &a->peers[t.slot];
}
static void retire(struct security_admission *a,struct security_peer *p)
{
 a->send_bytes-=p->send_bytes;a->handshakes-=p->handshake;
 a->connections--;memset(p,0,sizeof(*p));
}
static int time_check(struct security_admission *a,uint64_t now)
{
 if(now<a->last_ms){security_admission_halt(a);return SECURITY_RESOURCE_INVALID;}
 a->last_ms=now;return SECURITY_RESOURCE_OK;
}
int security_admission_init(struct security_admission *a,const struct security_rng *r,
 const struct security_memory *m)
{
 if(!a||!r||!r->initialized||!m||!m->initialized)return SECURITY_RESOURCE_INVALID;
 memset(a,0,sizeof(*a));a->rng=r;a->memory=m;a->next_id=1;a->initialized=1;
 return SECURITY_RESOURCE_OK;
}
void security_admission_halt(struct security_admission *a)
{
 if(!a||!a->initialized)return;
 memset(a->peers,0,sizeof(a->peers));a->connections=a->handshakes=0;
 a->send_bytes=0;a->stopped=1;
}
int security_admission_open(struct security_admission *a,uint64_t now,
 struct security_peer_token *token)
{
 if(!token)return SECURITY_RESOURCE_INVALID;
 *token=(struct security_peer_token){0};int result=gate(a);if(result)return result;
 if(time_check(a,now))return SECURITY_RESOURCE_INVALID;
 if(now>UINT64_MAX-SECURITY_PRETLS_TIMEOUT_MS||!a->next_id)return SECURITY_RESOURCE_LIMIT;
 if(a->connections==SECURITY_HTTPS_PEERS)return SECURITY_RESOURCE_LIMIT;
 unsigned i;for(i=0;i<SECURITY_HTTPS_PEERS&&a->peers[i].id;i++);
 a->peers[i].id=a->next_id++;a->peers[i].deadline_ms=now+SECURITY_PRETLS_TIMEOUT_MS;
 a->connections++;if(a->connections>a->high_connections)a->high_connections=a->connections;
 *token=(struct security_peer_token){a,a->peers[i].id,i};return SECURITY_RESOURCE_OK;
}
int security_admission_handshake_begin(struct security_admission *a,
 struct security_peer_token t,uint64_t now)
{
 int result=gate(a);if(result)return result;
 if(time_check(a,now))return SECURITY_RESOURCE_INVALID;
 struct security_peer *p=peer(a,t);if(!p)return SECURITY_RESOURCE_STALE;
 if(now>=p->deadline_ms){retire(a,p);return SECURITY_RESOURCE_TIMEOUT;}
 if(p->handshake||p->tls_ready)return SECURITY_RESOURCE_INVALID;
 if(a->handshakes==SECURITY_HTTPS_HANDSHAKES)return SECURITY_RESOURCE_LIMIT;
 p->handshake=1;a->handshakes++;
 if(a->handshakes>a->high_handshakes)a->high_handshakes=a->handshakes;
 return SECURITY_RESOURCE_OK;
}
int security_admission_handshake_finish(struct security_admission *a,
 struct security_peer_token t,uint64_t now,int success)
{
 int result=gate(a);if(result)return result;
 if(time_check(a,now))return SECURITY_RESOURCE_INVALID;
 struct security_peer *p=peer(a,t);if(!p)return SECURITY_RESOURCE_STALE;
 if(!p->handshake)return SECURITY_RESOURCE_INVALID;
 if(now>=p->deadline_ms){retire(a,p);return SECURITY_RESOURCE_TIMEOUT;}
 if(success!=1){retire(a,p);return SECURITY_RESOURCE_NOT_READY;}
 p->handshake=0;p->tls_ready=1;a->handshakes--;return SECURITY_RESOURCE_OK;
}
int security_admission_send_reserve(struct security_admission *a,
 struct security_peer_token t,size_t bytes)
{
 int result=gate(a);if(result)return result;
 struct security_peer *p=peer(a,t);if(!p)return SECURITY_RESOURCE_STALE;
 if(!p->tls_ready||p->handshake)return SECURITY_RESOURCE_NOT_READY;
 if(bytes>SECURITY_SEND_PEER_BYTES-p->send_bytes||
  bytes>SECURITY_SEND_TOTAL_BYTES-a->send_bytes)return SECURITY_RESOURCE_LIMIT;
 p->send_bytes+=bytes;a->send_bytes+=bytes;
 if(a->send_bytes>a->high_send)a->high_send=a->send_bytes;
 return SECURITY_RESOURCE_OK;
}
int security_admission_send_release(struct security_admission *a,
 struct security_peer_token t,size_t bytes)
{
 struct security_peer *p=peer(a,t);if(!p)return SECURITY_RESOURCE_STALE;
 if(bytes>p->send_bytes)return SECURITY_RESOURCE_INVALID;
 p->send_bytes-=bytes;a->send_bytes-=bytes;return SECURITY_RESOURCE_OK;
}
int security_admission_close(struct security_admission *a,struct security_peer_token t)
{
 struct security_peer *p=peer(a,t);if(!p)return SECURITY_RESOURCE_STALE;
 retire(a,p);return SECURITY_RESOURCE_OK;
}
int security_admission_expire(struct security_admission *a,uint64_t now)
{
 int result=gate(a);if(result)return result;
 if(time_check(a,now))return SECURITY_RESOURCE_INVALID;
 int count=0;
 for(unsigned i=0;i<SECURITY_HTTPS_PEERS;i++)if(a->peers[i].id&&
  !a->peers[i].tls_ready&&now>=a->peers[i].deadline_ms){retire(a,&a->peers[i]);count++;}
 return count;
}
