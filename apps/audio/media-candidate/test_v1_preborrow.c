/* SPDX-License-Identifier: GPL-2.0-only */
/* Compiled ONLY against preserved V1 packet API/source to reproduce defect. */
#include <audio_packet.h>
#include <camera/access_policy.h>
#include <stdio.h>
#include <string.h>
struct test {struct ac_request pending;struct ac_core capture;struct camera_access_policy policy;
 struct camera_access_principal revoked;struct camera_access_request permission;
 uint64_t now;};
static int submit(void *ctx,const struct ac_request *q){((struct test *)ctx)->pending=*q;return AC_ACCEPTED;}
static int deny_real_policy(void *ctx,const struct am_key *key,uint64_t now,uint32_t effect,uint32_t offset,uint32_t bytes)
{
 struct test *t=ctx;(void)key;(void)effect;(void)offset;(void)bytes;
 return camera_access_check(&t->policy,&t->revoked,&t->permission,now)==CAMERA_ACCESS_OK;
}
static void id(char *dst,const char *src){memset(dst,0,CAMERA_ID_BYTES);memcpy(dst,src,strlen(src));}
static int ack(struct test *t)
{
 struct ac_completion r={0};r.ticket=t->pending.ticket;r.operation=t->pending.operation;t->now++;
 if(r.operation==AC_DMA_PERIOD){r.bytes=512;r.timestamp_valid=1;r.first_sample_ns=1000000000;r.irq_ns=t->now;}
 return ac_complete(&t->capture,&r,t->now);
}
int main(void)
{
 struct test t={0};struct am_packet media={0};struct am_key key;
 struct ac_provider provider={AC_ABI,sizeof(provider),&t,submit};
 struct ac_params params={8000,1,16,512,4};struct ac_clock clock={98304000,1,AC_RESOURCE_REQUIRED,0};
 struct am_seed seed={1,1,1,0,1};_Alignas(64) unsigned char ring[AC_RING_BYTES]={0};int result;
 struct camera_access_resource resources[3]={0};struct camera_access_account a={0};struct camera_access_verified v={0};char device[CAMERA_ID_BYTES];
 t.now=1000000000;
 if(ac_init(&t.capture,&provider,1,t.now)||ac_prepare(&t.capture,&params,&clock,ring,sizeof(ring),1,t.now)||ack(&t)||ack(&t)||
    ac_start(&t.capture,t.now)||ack(&t)||ack(&t)||ack(&t))return 2;
 resources[0].kind=CAMERA_ACCESS_DEVICE;id(resources[0].id,"PUBLICDUMMY-device");
 resources[1].kind=CAMERA_ACCESS_SENSOR;id(resources[1].id,"PUBLICDUMMY-camera0");
 resources[2].kind=CAMERA_ACCESS_PROFILE;id(resources[2].id,"PUBLICDUMMY-profile");id(resources[2].sensor_id,"PUBLICDUMMY-camera0");id(device,"PUBLICDUMMY-device");
 if(camera_access_init(&t.policy,device,77,t.now,resources,3))return 2;
 a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_RTSP;a.roles=CAMERA_ACCESS_VIEWER;a.credential_generation=1;a.grant_count=1;
 id(a.account_id,"PUBLICDUMMY-viewer");a.grants[0].kind=CAMERA_ACCESS_PROFILE;a.grants[0].operations=CAMERA_ACCESS_LIVE;id(a.grants[0].resource_id,"PUBLICDUMMY-profile");
 if(camera_access_put_account(&t.policy,&a))return 2;
 v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;v.domain=CAMERA_ACCESS_RTSP;v.boot_generation=77;v.credential_generation=1;v.session_generation=1;
 v.issued_ns=t.now;v.expires_ns=t.now+1000000000;id(v.device_id,"PUBLICDUMMY-device");id(v.account_id,"PUBLICDUMMY-viewer");
 if(camera_access_issue(&t.policy,&v,t.now,&t.revoked)||camera_access_revoke_session(&t.policy,&t.revoked))return 2;
 t.permission.abi=CAMERA_ACCESS_ABI;t.permission.struct_bytes=sizeof(t.permission);t.permission.domain=CAMERA_ACCESS_RTSP;
 t.permission.kind=CAMERA_ACCESS_PROFILE;t.permission.operation=CAMERA_ACCESS_LIVE;id(t.permission.device_id,"PUBLICDUMMY-device");
 id(t.permission.sensor_id,"PUBLICDUMMY-camera0");id(t.permission.profile_id,"PUBLICDUMMY-profile");
 if(am_init(&media,&t.capture,deny_real_policy,&t,&seed,G711_MULAW,t.now))return 2;
 result=am_begin(&media,&key,t.now);
 printf("V1 authority regression result=%d borrowed=%u packet_serial=%llu ready_state=%u\n",result,t.capture.borrowed,(unsigned long long)media.packet_serial,t.capture.slots[0].state);
 return result==AM_DENIED&&t.capture.borrowed==1&&media.packet_serial==1?1:2;
}
