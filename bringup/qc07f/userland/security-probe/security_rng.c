/* SPDX-License-Identifier: GPL-2.0-only */
#include "security_rng.h"
#include <errno.h>
#include <string.h>
#include <sys/random.h>
#ifdef SECURITY_WITH_MBEDTLS
#include "mongoose.h"
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#endif

void security_rng_zero(void *output,size_t bytes)
{
 volatile unsigned char *p=output;
 while(bytes){*p++=0;bytes--;}
}
static ssize_t os_getrandom(void *context,void *output,size_t bytes,
 unsigned flags,int *error)
{
 (void)context;errno=0;
 ssize_t n=getrandom(output,bytes,flags);*error=n<0?errno:0;return n;
}
static int fail(struct security_rng *r,int status,int error)
{
 if(r&&r->initialized){r->failed=1;r->ready=0;r->status=status;r->os_error=error;}
 return status;
}
int security_rng_init(struct security_rng *r,security_getrandom_fn call,void *context)
{
 if(!r)return SECURITY_RNG_INVALID;
 memset(r,0,sizeof(*r));r->syscall=call?call:os_getrandom;
 r->syscall_context=context;r->initialized=1;return SECURITY_RNG_OK;
}
int security_rng_fill(struct security_rng *r,void *output,size_t bytes)
{
 if(!output||!bytes||bytes>SECURITY_RNG_MAX_BYTES)
  return fail(r,SECURITY_RNG_INVALID,EINVAL);
 /* These bytes are valid caller-owned output, never trusted as previous entropy. */
 security_rng_zero(output,bytes);
 if(!r||!r->initialized||!r->syscall)return SECURITY_RNG_INVALID;
 r->last_calls=0;r->last_bytes=0;
 if(r->failed)return SECURITY_RNG_FATAL;
 r->os_error=0;
 size_t done=0;int status=SECURITY_RNG_RETRY_LIMIT,error=0;
 while(done<bytes&&r->last_calls<SECURITY_RNG_MAX_CALLS){
  size_t want=bytes-done;
  if(want>SECURITY_RNG_CHUNK_BYTES)want=SECURITY_RNG_CHUNK_BYTES;
  error=0;r->last_calls++;
  ssize_t n=r->syscall(r->syscall_context,(unsigned char *)output+done,want,
   GRND_NONBLOCK,&error);
  if(n>0&&(size_t)n<=want){done+=(size_t)n;continue;}
  if(n==-1&&error==EINTR)continue;
  status=n==-1&&error==EAGAIN?SECURITY_RNG_NOT_READY:SECURITY_RNG_IO;
  if(n>=0||n<-1)error=EIO;
  break;
 }
 if(done==bytes){r->last_bytes=bytes;r->status=SECURITY_RNG_OK;return SECURITY_RNG_OK;}
 security_rng_zero(output,bytes);
 return fail(r,status,error);
}
int security_rng_start(struct security_rng *r,security_psa_init_fn init,void *context)
{
 if(!r||!r->initialized)return SECURITY_RNG_INVALID;
 if(r->failed)return SECURITY_RNG_FATAL;
 if(r->start_called)return SECURITY_RNG_INVALID;
 r->start_called=1;
 if(!init)return fail(r,SECURITY_RNG_INVALID,EINVAL);
 unsigned char readiness[32];
 int result=security_rng_fill(r,readiness,sizeof(readiness));
 security_rng_zero(readiness,sizeof(readiness));
 if(result)return result;
 r->psa_status=init(context);
 /* A callback can return success after an RNG failure; success never clears it. */
 if(r->failed)return r->status;
 if(r->psa_status)return fail(r,SECURITY_RNG_PSA_INIT_FAILED,0);
 r->psa_initialized=1;r->ready=1;return SECURITY_RNG_OK;
}
bool security_rng_can_serve(const struct security_rng *r)
{
 return r&&r->initialized&&!r->failed&&r->ready&&r->psa_initialized;
}

#ifdef SECURITY_WITH_MBEDTLS
static struct security_rng *library_rng;
int security_rng_bind_library(struct security_rng *r)
{
 if(!r||!r->initialized||library_rng)return SECURITY_RNG_INVALID;
 library_rng=r;return SECURITY_RNG_OK;
}
/* Mongoose callback is bool, but ALL callers do not necessarily check it.
 * The persistent context failure latch is the independent service shutdown gate. */
bool mg_random(void *output,size_t bytes)
{
 return security_rng_fill(library_rng,output,bytes)==SECURITY_RNG_OK;
}
psa_status_t mbedtls_psa_external_get_random(
 mbedtls_psa_external_random_context_t *context,uint8_t *output,
 size_t bytes,size_t *output_length)
{
 (void)context;
 if(!output_length){
  if(output&&bytes&&bytes<=SECURITY_RNG_MAX_BYTES)security_rng_zero(output,bytes);
  fail(library_rng,SECURITY_RNG_INVALID,EINVAL);return PSA_ERROR_INVALID_ARGUMENT;
 }
 *output_length=0;
 if(security_rng_fill(library_rng,output,bytes)!=SECURITY_RNG_OK)
  return PSA_ERROR_INSUFFICIENT_ENTROPY;
 *output_length=bytes;return PSA_SUCCESS;
}
static int real_psa_init(void *context)
{(void)context;return (int)psa_crypto_init();}
int security_rng_library_start(struct security_rng *r)
{
 int result=security_rng_bind_library(r);
 return result?result:security_rng_start(r,real_psa_init,NULL);
}
#endif
