/* SPDX-License-Identifier: GPL-2.0-only */
#include "security_rng.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/random.h>
#ifdef SECURITY_WITH_MBEDTLS
#include "security_memory.h"
#include "mongoose.h"
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <stdlib.h>
#endif
struct step {ssize_t result;int error;};
struct fake {
 struct step steps[SECURITY_RNG_MAX_CALLS];unsigned count,calls,dirty;
 int error;size_t largest;
};
static ssize_t fake_random(void *context,void *output,size_t bytes,unsigned flags,int *error)
{
 struct fake *f=context;assert(flags==GRND_NONBLOCK&&bytes<=SECURITY_RNG_CHUNK_BYTES);
 if(bytes>f->largest)f->largest=bytes;
 struct step s=f->calls<f->count?f->steps[f->calls]:(struct step){(ssize_t)bytes,f->error};
 f->calls++;*error=s.error;
 if(f->error)s=(struct step){-1,f->error},*error=f->error;
 if(s.result>0&&(size_t)s.result<=bytes)memset(output,0xa5,(size_t)s.result);
 else if(f->dirty)memset(output,0xa5,bytes);
 return s.result;
}
static int zeroed(const void *data,size_t n)
{const unsigned char *p=data;for(size_t i=0;i<n;i++)if(p[i])return 0;return 1;}
struct startup {int calls,result;struct security_rng *rng;struct fake *fake;};
static int init_psa(void *context)
{
 struct startup *s=context;s->calls++;
 if(s->rng){unsigned char output[16];s->fake->error=EAGAIN;
  assert(security_rng_fill(s->rng,output,sizeof(output))==SECURITY_RNG_NOT_READY);
  assert(zeroed(output,sizeof(output)));}
 return s->result;
}
static void pure_fixtures(void)
{
 struct security_rng r;struct fake f={0};unsigned char out[SECURITY_RNG_MAX_BYTES];
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,out,32)==0&&r.last_bytes==32&&r.last_calls==1);
 assert(!security_rng_can_serve(&r));security_rng_zero(out,32);assert(zeroed(out,32));
 struct startup start={0};assert(security_rng_start(&r,init_psa,&start)==0);
 assert(start.calls==1&&security_rng_can_serve(&r));
 unsigned calls=f.calls;assert(security_rng_start(&r,init_psa,&start)==SECURITY_RNG_INVALID);
 assert(f.calls==calls&&start.calls==1);

 f=(struct fake){.count=4,.steps={{-1,EINTR},{3,0},{-1,EINTR},{13,0}}};
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,out,16)==0&&r.last_calls==4&&r.last_bytes==16);
 for(unsigned i=0;i<16;i++)assert(out[i]==0xa5);

 f=(struct fake){0};assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,out,sizeof(out))==0);
 assert(r.last_calls==16&&f.largest==256);security_rng_zero(out,sizeof(out));

 const int errors[]={EAGAIN,EIO,ENOSYS,EPERM};
 for(unsigned i=0;i<sizeof(errors)/sizeof(errors[0]);i++){
  f=(struct fake){.count=2,.dirty=1,.steps={{3,0},{-1,errors[i]}}};
  assert(security_rng_init(&r,fake_random,&f)==0);memset(out,0xff,16);
  int expected=errors[i]==EAGAIN?SECURITY_RNG_NOT_READY:SECURITY_RNG_IO;
  assert(security_rng_fill(&r,out,16)==expected);
  assert(zeroed(out,16)&&r.last_bytes==0&&r.last_calls==2&&r.failed);
  calls=f.calls;memset(out,0xff,16);
  assert(security_rng_fill(&r,out,16)==SECURITY_RNG_FATAL&&zeroed(out,16));
  assert(f.calls==calls&&r.last_calls==0&&r.last_bytes==0);
 }
 f=(struct fake){.error=EINTR};assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,out,32)==SECURITY_RNG_RETRY_LIMIT);
 assert(zeroed(out,32)&&r.last_calls==SECURITY_RNG_MAX_CALLS);
 f=(struct fake){.count=SECURITY_RNG_MAX_CALLS};
 for(unsigned i=0;i<f.count;i++)f.steps[i]=(struct step){1,0};
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,out,64)==SECURITY_RNG_RETRY_LIMIT&&zeroed(out,64));
 const ssize_t invalid[]={0,33,-2};
 for(unsigned i=0;i<3;i++){
  f=(struct fake){.count=1,.dirty=1,.steps={{invalid[i],0}}};
  assert(security_rng_init(&r,fake_random,&f)==0);
  assert(security_rng_fill(&r,out,32)==SECURITY_RNG_IO&&zeroed(out,32));
 }
 assert(security_rng_init(NULL,NULL,NULL)==SECURITY_RNG_INVALID);
 assert(security_rng_fill(NULL,out,32)==SECURITY_RNG_INVALID&&zeroed(out,32));
 f=(struct fake){0};assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,NULL,32)==SECURITY_RNG_INVALID&&f.calls==0);
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,out,0)==SECURITY_RNG_INVALID&&f.calls==0);
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_fill(&r,out,SIZE_MAX)==SECURITY_RNG_INVALID&&f.calls==0);

 f=(struct fake){.error=EAGAIN};start=(struct startup){0};
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_start(&r,init_psa,&start)==SECURITY_RNG_NOT_READY);
 assert(!start.calls&&!security_rng_can_serve(&r));
 f=(struct fake){0};start=(struct startup){.result=-123};
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_start(&r,init_psa,&start)==SECURITY_RNG_PSA_INIT_FAILED);
 assert(start.calls==1&&r.psa_status==-123&&!security_rng_can_serve(&r));
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_rng_start(&r,NULL,NULL)==SECURITY_RNG_INVALID&&!security_rng_can_serve(&r));
 assert(security_rng_init(&r,fake_random,&f)==0);
 start=(struct startup){.rng=&r,.fake=&f};
 assert(security_rng_start(&r,init_psa,&start)==SECURITY_RNG_NOT_READY);
 assert(!security_rng_can_serve(&r));
 puts("PASS RNG: exact/nonblocking/short/EINTR/EAGAIN/error/zeroization/call-limit/startup-latch");
}
#ifdef SECURITY_WITH_MBEDTLS
struct allocator_fault {unsigned fail;};
static void *fault_allocate(void *context,size_t bytes)
{struct allocator_fault *f=context;return f->fail?NULL:malloc(bytes);}
static void fault_free(void *context,void *pointer){(void)context;free(pointer);}
static void library_fixtures(void)
{
 static struct security_rng r;static struct fake f;
 static struct security_memory m;static struct allocator_fault allocation;
 unsigned char output[32];size_t written=99;
 assert(security_rng_init(&r,fake_random,&f)==0);
 assert(security_memory_init(&m,SECURITY_HEAP_LIMIT_BYTES,SECURITY_ALLOCATION_LIMIT_BYTES,
  fault_allocate,fault_free,&allocation)==0);
 assert(security_memory_bind_library(&m)==0);
 assert(security_memory_bind_library(&m)==SECURITY_RESOURCE_INVALID);
 assert(security_rng_library_start(&r)==0&&security_rng_can_serve(&r));
 assert(security_rng_bind_library(&r)==SECURITY_RNG_INVALID);
 assert(mg_random(output,sizeof(output)));security_rng_zero(output,sizeof(output));
 assert(psa_generate_random(output,sizeof(output))==PSA_SUCCESS);
 security_rng_zero(output,sizeof(output));

 struct mg_mgr manager;mg_mgr_init(&manager);
 struct mg_connection connection={0};connection.mgr=&manager;
 struct mg_tls_opts opts={0};
 /* Allocation failure before mg_tls_init can set TLS: void return is NOT success. */
 allocation.fail=1;mg_tls_init(&connection,&opts);allocation.fail=0;
 assert(!connection.tls&&!connection.is_tls&&!connection.is_tls_hs);
 assert(m.last_status==SECURITY_RESOURCE_ALLOC_FAILED&&security_rng_can_serve(&r));
 /* Invalid DER certificate, no handshake/socket: failure must not become HTTP. */
 const char bad_der[]={0x30,0x00};opts.cert=mg_str_n(bad_der,sizeof(bad_der));
 mg_tls_init(&connection,&opts);assert(!connection.tls&&!connection.is_tls);
 mg_mgr_free(&manager);

 struct security_admission a;struct security_peer_token token;
 assert(security_admission_init(&a,&r,&m)==0);
 assert(security_admission_open(&a,0,&token)==0);
 f.error=EAGAIN;memset(output,0xff,sizeof(output));
 assert(!mg_random(output,sizeof(output))&&zeroed(output,sizeof(output)));
 assert(!security_rng_can_serve(&r));
 assert(mbedtls_psa_external_get_random(NULL,output,sizeof(output),&written)==
  PSA_ERROR_INSUFFICIENT_ENTROPY&&written==0&&zeroed(output,sizeof(output)));
 memset(output,0xff,sizeof(output));
 assert(psa_generate_random(output,sizeof(output))!=PSA_SUCCESS&&zeroed(output,sizeof(output)));
 memset(output,0xff,sizeof(output));
 assert(mbedtls_psa_external_get_random(NULL,output,sizeof(output),NULL)==
  PSA_ERROR_INVALID_ARGUMENT&&zeroed(output,sizeof(output)));
 assert(security_admission_send_reserve(&a,token,1)==SECURITY_RESOURCE_NOT_READY);
 assert(a.stopped&&!a.connections&&!a.handshakes&&!a.send_bytes);
 mbedtls_psa_crypto_free();assert(!m.active&&!m.used&&!m.fatal&&m.allocations>0&&m.high_water>0);
 puts("PASS public-library adapters: PSA-init/random/error, Mongoose void-TLS allocation/cert failure, fatal admission gate");
}
#endif
int main(void)
{
 pure_fixtures();
#ifdef SECURITY_WITH_MBEDTLS
 library_fixtures();
#endif
 return 0;
}
