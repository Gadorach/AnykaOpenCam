/* SPDX-License-Identifier: GPL-2.0-only */
/* HOST mock producer binding only: does NOT prove browser/encoder integration. */
#include "web_service.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned effects;
static int effect(void *p){(void)p;effects++;return 200;}
int main(void)
{
 struct camera_access_policy p;struct camera_access_resource r[3]={
  {CAMERA_ACCESS_DEVICE,"fixture-device",{0}},
  {CAMERA_ACCESS_SENSOR,"fixture-sensor",{0}},
  {CAMERA_ACCESS_PROFILE,"fixture-profile","fixture-sensor"}};
 struct camera_access_account a={0};struct camera_access_verified f={0};
 struct camera_access_principal s;struct camera_access_request q={0};
 assert(!camera_access_init(&p,r[0].id,1,100,r,3));
 a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_ADMIN;
 a.roles=CAMERA_ACCESS_VIEWER;strcpy(a.account_id,"fixture-user");a.credential_generation=1;
 a.grant_count=1;a.grants[0].kind=CAMERA_ACCESS_PROFILE;
 strcpy(a.grants[0].resource_id,"fixture-profile");a.grants[0].operations=CAMERA_ACCESS_LIVE;
 assert(!camera_access_put_account(&p,&a));
 f.abi=CAMERA_ACCESS_ABI;f.struct_bytes=sizeof(f);f.verified=1;f.domain=CAMERA_ACCESS_ADMIN;
 strcpy(f.device_id,"fixture-device");strcpy(f.account_id,"fixture-user");
 f.boot_generation=f.credential_generation=f.session_generation=1;f.issued_ns=100;f.expires_ns=200;
 assert(!camera_access_issue(&p,&f,100,&s));
 q.abi=CAMERA_ACCESS_ABI;q.struct_bytes=sizeof(q);q.domain=CAMERA_ACCESS_ADMIN;
 q.kind=CAMERA_ACCESS_PROFILE;q.operation=CAMERA_ACCESS_LIVE;
 strcpy(q.device_id,"fixture-device");strcpy(q.sensor_id,"fixture-sensor");
 strcpy(q.profile_id,"fixture-profile");
 assert(web_media_admit(&p,NULL,&q,101,1,1,effect,NULL)==401&&effects==0);
 assert(web_media_admit(&p,&s,&q,102,0,1,effect,NULL)==401&&effects==0);
 assert(web_media_admit(&p,&s,&q,103,1,0,effect,NULL)==501&&effects==0);
 assert(web_media_admit(&p,&s,&q,104,1,1,effect,NULL)==200&&effects==1);
 q.domain=CAMERA_ACCESS_RTSP;
 assert(web_media_admit(&p,&s,&q,105,1,1,effect,NULL)==401&&effects==1);
 q.domain=CAMERA_ACCESS_ADMIN;
 assert(!camera_access_revoke_account(&p,CAMERA_ACCESS_ADMIN,a.account_id));
 assert(web_media_admit(&p,&s,&q,106,1,1,effect,NULL)==401&&effects==1);
 a.credential_generation=2;assert(!camera_access_put_account(&p,&a));
 assert(web_media_admit(&p,&s,&q,107,1,1,effect,NULL)==401&&effects==1);
 f.credential_generation=2;f.session_generation=2;f.issued_ns=108;f.expires_ns=200;
 assert(!camera_access_issue(&p,&f,108,&s));
 assert(web_media_admit(&p,&s,&q,200,1,1,effect,NULL)==401&&effects==1);
 assert(web_media_admit(&p,&s,&q,199,1,1,effect,NULL)==401&&effects==1);
 puts("media_binding_mock=PASS auth_before_effect=1 domain_generation_revocation_expiry_clock=PASS actual_video=0");
 return 0;
}
