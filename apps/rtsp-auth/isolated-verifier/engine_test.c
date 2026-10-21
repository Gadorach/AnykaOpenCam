/* SPDX-License-Identifier: GPL-2.0-only
 * PUBLICDUMMY pure bounded engine/codec negatives, ASan/UBSan—not sandbox proof. */
#define _GNU_SOURCE
#include "verifier.h"
#include <camera/rtsp_digest.h>
#include <camera/access_policy.h>
#include <mbedtls/constant_time.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){printf("HOST_ONLY FAIL check=%u line=%u\n",checks,(unsigned)__LINE__);return 1;}}while(0)
int main(void)
{
 static unsigned char state[32768],zero[32768];struct rv_material m={0};struct rv_message in={0},out={0};uint64_t now;
 CHECK(rv_engine_bytes()<=sizeof(state));CHECK(rv_clock(&now)==0);
 m.abi=RV_ABI;m.bytes=sizeof(m);m.domain=CAMERA_ACCESS_RTSP;m.algorithm=1;m.epoch=888;m.boot_generation=777;m.credential_generation=7;m.account=9;
 strcpy(m.device,"PUBLICDUMMY-device");strcpy(m.account_id,"PUBLICDUMMY-rtsp");strcpy(m.sensor,"camera0");strcpy(m.profile,"camera0-main");
 memcpy(m.username,"PUBLICDUMMY",11);m.username_bytes=11;memcpy(m.realm,"PUBLICDUMMY RTSP",16);m.realm_bytes=16;m.ha1_bytes=32;
 m.nonce_ttl_ms=1000;m.window_ms=1000;m.global_verifies=m.account_verifies=m.peer_verifies=2;m.global_challenges=m.account_challenges=m.peer_challenges=2;
 strcpy(m.live,"rtsp://publicdummy.invalid/live");strcpy(m.track,"rtsp://publicdummy.invalid/live/track0");
 CHECK(camera_rtsp_digest_ha1(1,(struct camera_digest_span){m.username,11},(struct camera_digest_span){m.realm,16},
  (struct camera_digest_span){(unsigned char *)"PUBLICDUMMY-not-product",sizeof("PUBLICDUMMY-not-product")-1},m.ha1)==0);
 CHECK(rv_engine_init(state,sizeof(state),&m)==0);
 in.kind=RV_OPEN;in.epoch=888;in.generation=7;in.scope=1;in.sequence=1;in.deadline=now+1000000000ULL;
 CHECK(rv_engine_command(state,&in,&out)==0&&out.status==0&&out.connection!=0);uint64_t conn=out.connection;
 in.kind=RV_CHECK;in.connection=conn;in.sequence=2;in.token=1;CHECK(rv_engine_command(state,&in,&out)==RV_BAD&&out.status!=0); /*forged typed handle*/
 in.scope=2;in.sequence=3;CHECK(rv_engine_command(state,&in,&out)==RV_BAD);
 in.scope=1;in.generation=8;CHECK(rv_engine_command(state,&in,&out)==RV_BAD);in.generation=7;
 in.kind=RV_CLOSE;in.token=0;CHECK(rv_engine_command(state,&in,&out)==0&&out.status==0);
 in.kind=RV_OPEN;in.connection=0;in.sequence=4;CHECK(rv_engine_command(state,&in,&out)==0&&out.status==0&&out.connection!=conn);
 in.sequence=5;CHECK(rv_engine_command(state,&in,&out)==0&&out.status!=0); /*global/account churn quota cannot reset*/
 rv_engine_destroy(state,sizeof(state));CHECK(mbedtls_ct_memcmp(state,zero,sizeof(state))==0);
 m.domain=CAMERA_ACCESS_UI;CHECK(rv_engine_init(state,sizeof(state),&m)==RV_BAD);m.domain=CAMERA_ACCESS_RTSP;
 int pair[2];CHECK(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,0,pair)==0);
 struct rv_message message={.kind=RV_OPEN,.epoch=1,.sequence=1,.scope=1};
 CHECK(rv_send(pair[0],&message)==0&&rv_receive(pair[1],&out)==0&&out.kind==RV_OPEN);
 unsigned char bad[81]={0};CHECK(send(pair[0],bad,sizeof(bad),0)==sizeof(bad));CHECK(rv_receive(pair[1],&out)==-1);
 message.bytes=RV_PAYLOAD+1;CHECK(rv_send(pair[0],&message)==-1);close(pair[0]);close(pair[1]);rv_wipe(&m,sizeof(m));
 printf("HOST_ONLY PASS checks=%u engine_bytes=%zu ABI_metadata_max=%u native_isolation_claim=0\n",checks,rv_engine_bytes(),RV_PAYLOAD);return 0;
}
