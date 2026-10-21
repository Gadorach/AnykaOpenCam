/* SPDX-License-Identifier: GPL-2.0-only
 * RTSP-only isolated child, no listeners/producer/media/TLS/UI-KDF/persistence. */
#define _GNU_SOURCE
#include "verifier.h"
#include <mbedtls/constant_time.h>
#include <signal.h>
#include <poll.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>
static unsigned char storage[32768];
static struct rv_material material;
static struct rv_message request,response;
static volatile sig_atomic_t stopping;
static void stopped(int sig){(void)sig;stopping=1;}
int main(int argc,char **argv)
{
 unsigned flags=0,stage=0;int r,initialized=0,normal=0;struct sigaction action={0};unsigned char entropy[32];
 unsigned group_count=(unsigned)getgroups(0,NULL);
 (void)argv;if(argc!=1)return 70;
 action.sa_handler=stopped;sigemptyset(&action.sa_mask);
 if(sigaction(SIGTERM,&action,NULL)||sigaction(SIGINT,&action,NULL))return 70;
 r=rv_harden(&flags,&stage);response.kind=RV_READY;response.status=r?0:(int32_t)flags;response.flags=stage;response.bytes=16;
#ifdef RV_HOST_PUBLIC_ONLY
 rv_put32(response.payload,RV_HOST_PUBLIC_4M);rv_put32(response.payload+4,4U*1024U*1024U);
#else
 rv_put32(response.payload,RV_NATIVE);rv_put32(response.payload+4,2U*1024U*1024U);
#endif
 rv_put32(response.payload+8,group_count); /*measured before filter/secrets; HOST profile never claims native groups0*/
 if(rv_send(RV_FD,&response)||r){rv_wipe(&response,sizeof(response));return 71;}
 rv_wipe(&response,sizeof(response));
 /* READY flags15 precedes EVERY material read; no hash/log of this payload. */
 if(read(RV_MATERIAL_FD,&material,sizeof(material))!=(ssize_t)sizeof(material)||close(RV_MATERIAL_FD)||
    rv_engine_bytes()>sizeof(storage)||getrandom(entropy,sizeof(entropy),0)!=(ssize_t)sizeof(entropy)||
    rv_engine_init(storage,sizeof(storage),&material)!=RV_OK)goto done;
 initialized=1;
 response.kind=RV_ACK;response.epoch=material.epoch;response.generation=material.credential_generation;response.scope=1;response.flags=RV_FLAGS15;
 rv_wipe(&material,sizeof(material));rv_wipe(entropy,sizeof(entropy));
 if(rv_send(RV_FD,&response))goto done;
 rv_wipe(&response,sizeof(response)); /*provisioning ACK only AFTER actual child state creation*/
 for(;;){
  struct pollfd fd={RV_FD,POLLIN,0};
  if(stopping){normal=1;break;}
  if(poll(&fd,1,20)<0)continue;
  r=rv_receive(RV_FD,&request);if(r==1)continue;if(r<0)break;
#ifdef RV_HOST_DELAY_TEST
  /*Actual crypto, bounded delay before execution to exercise abandonment drain.
   * Separate PUBLICDUMMY binary, not a native scheduling/crypto fallback. */
  if(request.kind==RV_VERIFY)poll(NULL,0,200);
#endif
  r=rv_engine_command(storage,&request,&response);
  if(r!=RV_OK){response=request;rv_wipe(response.payload,sizeof(response.payload));response.bytes=0;response.kind=RV_ACK;response.status=r;}
  int quit=request.kind==RV_SHUTDOWN&&r==RV_OK;
  rv_wipe(&request,sizeof(request));
  if(rv_send(RV_FD,&response)){rv_wipe(&response,sizeof(response));break;}
  rv_wipe(&response,sizeof(response));if(quit){normal=1;break;}
 }
 done:
 if(initialized)rv_engine_destroy(storage,sizeof(storage));else rv_wipe(storage,sizeof(storage));
 rv_wipe(&material,sizeof(material));rv_wipe(&request,sizeof(request));rv_wipe(&response,sizeof(response));rv_wipe(entropy,sizeof(entropy));
 /* Actual self-check of wiped storage, not request-to-stop-as-retirement proof. */
 {static const unsigned char zero[sizeof(storage)]={0};
  response.kind=RV_BYE;response.status=mbedtls_ct_memcmp(storage,zero,sizeof(storage))?0:1;
  rv_send(RV_FD,&response);rv_wipe(&response,sizeof(response));}
 close(RV_FD);return normal?0:73;
}
