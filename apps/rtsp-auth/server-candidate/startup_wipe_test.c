/* SPDX-License-Identifier: GPL-2.0-only
 * HOST fault-injected clock/socket boundaries only; actual Mbed bootstrap/RNG/
 * backend initialization and zeroization. No crypto mocks, devices or listener. */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "candidate_auth.h"
#include <mbedtls/constant_time.h>
#include <sys/random.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
static unsigned checks,destroy_calls,init_with_secret,wiped,sockets,bound;
static int clock_fail;
static int injected_clock(clockid_t type,struct timespec *t)
{(void)type;if(clock_fail)return -1;t->tv_sec=1;t->tv_nsec=0;return 0;}
static int forbidden_socket(int domain,int type,int protocol)
{(void)domain;(void)type;(void)protocol;sockets++;return -1;}
static void observed_destroy(struct candidate_backend *b)
{
 static const unsigned char zero[sizeof(struct candidate_backend)]={0};
 destroy_calls++;if(b->initialized&&b->core.enabled&&b->core.generation)init_with_secret++;
 candidate_backend_destroy(b);if(mbedtls_ct_memcmp(b,zero,sizeof(*b))==0)wiped++;
}
#define clock_gettime injected_clock
#define socket forbidden_socket
#define candidate_backend_destroy observed_destroy
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation" /*included frozen/copy server style only*/
#include "rtsp_server_candidate.c"
#pragma GCC diagnostic pop
#undef clock_gettime
#undef socket
#undef candidate_backend_destroy
#define CHECK(x) do {++checks;if(!(x)){printf("HOST_ONLY FAIL check=%u line=%u\n",checks,(unsigned)__LINE__);return 1;}} while(0)
static int random_ready(void *ctx,unsigned char *out,size_t n)
{
 (void)ctx;if(getrandom(out,n,0)!=(ssize_t)n)return -1;
 clock_fail=1;return 0;
} /*real complete HOST CSPRNG; arm post-init clock fault*/
static int bootstrap(void *ctx,struct candidate_credential *c)
{
 (void)ctx;memset(c,0,sizeof(*c));c->domain=CAMERA_ACCESS_RTSP;c->algorithm=CAMERA_DIGEST_SHA256;c->account=7;
 c->username_bytes=11;c->realm_bytes=11;c->ha1_bytes=32;memcpy(c->username,"PUBLICDUMMY",11);memcpy(c->realm,"PUBLICDUMMY",11);
 return camera_rtsp_digest_ha1(c->algorithm,(struct camera_digest_span){c->username,11},(struct camera_digest_span){c->realm,11},
  (struct camera_digest_span){(const unsigned char *)"PUBLICDUMMY-not-product",sizeof("PUBLICDUMMY-not-product")-1},c->ha1);
}
static int source_ready(void *ctx,uint64_t connection,uint64_t now){(void)ctx;(void)connection;(void)now;return CAMERA_OK;}
static void binding(void *ctx,int (*gate)(void *),void *context){(void)ctx;(void)gate;(void)context;bound++;}
static void retain(void *ctx,const struct camera_packet_hub *h,const struct camera_producer_demand *d,unsigned slot){(void)ctx;(void)h;(void)d;(void)slot;}
static void frame_retain(void *ctx,struct camera_rtsp_frame_handle source,int result){(void)ctx;(void)source;(void)result;}
static int next(void *ctx,uint32_t force,struct camera_packet *p){(void)ctx;(void)force;(void)p;return CAMERA_IO_ERROR;}
static void release(void *ctx,const struct camera_packet *p){(void)ctx;(void)p;}
int main(void)
{
 static const struct camera_device devices[]={
  {"camera0","",CAMERA_DEVICE_SENSOR,CAMERA_PRESENT_UNQUALIFIED,0},
  {"encoder0","",CAMERA_DEVICE_ENCODER,CAMERA_PRESENT_UNQUALIFIED,0}};
 static const struct camera_profile profiles[]={{"camera0-main","camera0","encoder0",16,16,CAMERA_H264}};
 struct camera_graph graph={.devices=devices,.device_count=2,.profiles=profiles,.profile_count=1};
 struct candidate_config cfg={.abi=CANDIDATE_AUTH_ABI,.struct_bytes=sizeof(cfg),.host_fixture_only=1,.algorithm=CAMERA_DIGEST_SHA256,
  .boot_generation=42,.device_id="PUBLICDUMMY-device",.account_id="PUBLICDUMMY-rtsp",.profile_id="camera0-main",.graph=&graph,
  .nonce_ttl_ms=1000,.rate_window_ms=1000,.global_verifies=16,.global_challenges=16,.account_verifies=16,.account_challenges=16,
  .peer_verifies=16,.peer_challenges=16,.begin_contract=CANDIDATE_BEGIN_LOGICAL_ONLY,
  .bootstrap_rtsp=bootstrap,.random_ready=random_ready,.source_ready=source_ready,.source_guard_bind=binding,
  .retain_uncertain=retain,.retain_frame_uncertain=frame_retain};
 static unsigned char storage[CANDIDATE_HUB_BYTES];static volatile sig_atomic_t stop;
 static const uint8_t params[]={0,0,0,1,0x67,1,2,3,0,0,0,1,0x68,4,5};
 struct camera_rtsp_registry resources;CHECK(camera_rtsp_registry_init(&resources,"rtsp://127.0.0.1:34567/live","rtsp://127.0.0.1:34567/live/track0")==0);
 struct camera_rtsp_lifecycle life={0};struct camera_rtsp_producer producer={.next=next,.release=release,.parameter_sets=params,.parameter_set_bytes=sizeof(params),.lifecycle=&life};
 struct camera_rtsp_options options={.bind_address="127.0.0.1",.sdp="v=0\r\n",.port=34567,.stop=&stop,.resources=&resources,
  .send_deadline_ms=2000,.hub_slots=1,.packet_capacity=CANDIDATE_HUB_BYTES,.hub_storage=storage};
 CHECK(camera_rtsp_candidate_serve(&options,&producer,&cfg)==CAMERA_IO_ERROR);
 CHECK(init_with_secret==1&&destroy_calls==1&&wiped==1);
 CHECK(sockets==0&&bound==0); /* fault occurs at first clock check AFTER successful bootstrap */
 printf("HOST_ONLY PASS checks=%u post_init_secret_wipe=1 listener_effects=0 crypto_mocked=0\n",checks);return 0;
}
