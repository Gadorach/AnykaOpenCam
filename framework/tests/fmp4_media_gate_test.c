/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual media admission/policy plus synthetic CPU hub/chunk effects. No crypto,
 * TLS, DMA, device or authenticated video proof; route501 is not changed. */
#include <camera/fmp4.h>
#include <camera/packet_hub.h>
#include "../../apps/auth-web/service/web_service.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t original[137004],storage[137004];
struct fixture {
 struct camera_access_policy policy;
 struct camera_access_principal principal;
 struct camera_access_request request;
 struct camera_packet_hub hub;
 struct camera_packet held;
 struct camera_fmp4_fragment fragment;
 size_t offset,effect_bytes;unsigned effects;
 uint64_t profile_generation;
};
static void init(struct fixture *f)
{
 memset(f,0,sizeof(*f));original[0]=0x65;
 struct camera_access_resource r[3]={{CAMERA_ACCESS_DEVICE,"fixture-device",""},
  {CAMERA_ACCESS_SENSOR,"fixture-sensor",""},{CAMERA_ACCESS_PROFILE,"fixture-profile","fixture-sensor"}};
 assert(camera_access_init(&f->policy,r[0].id,1,100,r,3)==CAMERA_ACCESS_OK);
 struct camera_access_account a={.abi=CAMERA_ACCESS_ABI,.struct_bytes=sizeof(a),.domain=CAMERA_ACCESS_ADMIN,
  .roles=CAMERA_ACCESS_VIEWER,.credential_generation=1,.grant_count=1};
 strcpy(a.account_id,"fixture-user");a.grants[0].kind=CAMERA_ACCESS_PROFILE;
 strcpy(a.grants[0].resource_id,"fixture-profile");a.grants[0].operations=CAMERA_ACCESS_LIVE;
 assert(camera_access_put_account(&f->policy,&a)==CAMERA_ACCESS_OK);
 struct camera_access_verified v={.abi=CAMERA_ACCESS_ABI,.struct_bytes=sizeof(v),.verified=1,
  .domain=CAMERA_ACCESS_ADMIN,.boot_generation=1,.credential_generation=1,.session_generation=1,
  .issued_ns=100,.expires_ns=1000};
 strcpy(v.device_id,"fixture-device");strcpy(v.account_id,"fixture-user");
 assert(camera_access_issue(&f->policy,&v,100,&f->principal)==CAMERA_ACCESS_OK);
 f->request=(struct camera_access_request){.abi=CAMERA_ACCESS_ABI,.struct_bytes=sizeof(f->request),
  .domain=CAMERA_ACCESS_ADMIN,.kind=CAMERA_ACCESS_PROFILE,.operation=CAMERA_ACCESS_LIVE};
 strcpy(f->request.device_id,"fixture-device");strcpy(f->request.sensor_id,"fixture-sensor");strcpy(f->request.profile_id,"fixture-profile");
 assert(camera_hub_init_count(&f->hub,storage,sizeof(storage),1)==CAMERA_OK);
 assert(camera_hub_subscribe(&f->hub,0)==CAMERA_OK);
 struct camera_packet p={.codec=CAMERA_H264,.keyframe=1,.data=original,.bytes=sizeof(original),.monotonic_ns=100};
 assert(camera_hub_publish(&f->hub,&p)==CAMERA_OK&&camera_hub_acquire(&f->hub,0,&f->held)==CAMERA_OK);
 struct camera_fmp4_sample s={f->held.data,f->held.bytes,0,45720,1,1};
 assert(camera_fmp4_fragment_prepare(&s,&f->fragment)==CAMERA_OK);f->profile_generation=1;
}
static int effect(void *context)
{
 struct fixture *f=context;const uint8_t *span;size_t n;
 /* Revalidate the exact acquired CPU view and profile at the final effect, not
  * just when the request/fragment began. Serialized with release/STOP/publish. */
 if(f->profile_generation!=1||!camera_hub_lease_valid(&f->hub,0,&f->held))return 409;
 assert(camera_fmp4_fragment_span(&f->fragment,f->offset,8192,&span,&n)==CAMERA_OK);
 assert(n&&n<=8192&&span);f->offset+=n;f->effect_bytes+=n;f->effects++;return 200;
}
static int step(struct fixture *f,uint64_t now,int healthy,int qualified)
{return web_media_admit(&f->policy,&f->principal,&f->request,now,healthy,qualified,effect,f);}
static void retire(struct fixture *f)
{
 assert(camera_hub_release(&f->hub,0,f->held.lease_token,f->held.generation)==CAMERA_OK);
 assert(camera_hub_unsubscribe(&f->hub,0)==CAMERA_OK&&camera_hub_stop(&f->hub)==CAMERA_OK&&camera_hub_drained(&f->hub));
}
int main(void)
{
 struct fixture f;init(&f);
 for(uint64_t now=101;f.offset<112+sizeof(original);now++)assert(step(&f,now,1,1)==200);
 assert(f.effect_bytes==112+sizeof(original)&&f.effects==18);retire(&f);
 init(&f);assert(step(&f,101,1,1)==200);size_t written=f.effect_bytes;
 assert(step(&f,1000,1,1)==401&&f.effect_bytes==written);retire(&f);
 init(&f);assert(step(&f,101,1,1)==200);written=f.effect_bytes;
 assert(camera_access_revoke_account(&f.policy,CAMERA_ACCESS_ADMIN,f.principal.fact.account_id)==CAMERA_ACCESS_OK);
 assert(step(&f,102,1,1)==401&&f.effect_bytes==written);retire(&f);
 init(&f);assert(step(&f,101,1,1)==200);written=f.effect_bytes;
 assert(step(&f,102,0,1)==401&&f.effect_bytes==written);
 assert(step(&f,103,1,0)==501&&f.effect_bytes==written);retire(&f);
 init(&f);assert(step(&f,101,1,1)==200);written=f.effect_bytes;f.profile_generation=2;
 assert(step(&f,102,1,1)==409&&f.effect_bytes==written);retire(&f);
 init(&f);assert(step(&f,101,1,1)==200);written=f.effect_bytes;
 assert(camera_hub_stop(&f.hub)==CAMERA_OK&&!camera_hub_drained(&f.hub));
 assert(step(&f,102,1,1)==409&&f.effect_bytes==written);retire(&f);
 init(&f);written=f.effect_bytes;f.request.domain=CAMERA_ACCESS_RTSP;
 assert(step(&f,101,1,1)==401&&f.effect_bytes==written);retire(&f);
 init(&f);struct camera_packet changed=f.held;changed.monotonic_ns++;
 assert(!camera_hub_lease_valid(&f.hub,0,&changed));changed=f.held;changed.data=original;
 assert(!camera_hub_lease_valid(&f.hub,0,&changed));changed=f.held;changed.generation++;
 assert(!camera_hub_lease_valid(&f.hub,0,&changed));retire(&f);
 assert(!camera_hub_lease_valid(&f.hub,0,&f.held));
 puts("fMP4 every8192chunk policy/expiry/revoke/health/domain/profile/exactCPUlease gates PASS; no native or realTLS media claim");return 0;
}
