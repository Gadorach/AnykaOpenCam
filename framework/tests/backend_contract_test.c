/* SPDX-License-Identifier: GPL-2.0-only
 * Topology/ABI fixtures only; no camera/peripheral hardwareclaim.
 */
#include <camera/backend.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
static int enumerate(void *ctx,struct camera_graph *g){(void)ctx;memset(g,0,sizeof(*g));return CAMERA_OK;}
static int stops;
static int stop(void *ctx,const char *id){assert(ctx==&stops&&!strcmp(id,"head0"));stops++;return CAMERA_OK;}
int main(void)
{
 struct camera_device devices[]={
  {.id="camera0",.kind=CAMERA_DEVICE_SENSOR,.state=CAMERA_QUALIFIED},
  {.id="camera1",.kind=CAMERA_DEVICE_SENSOR,.state=CAMERA_PRESENT_UNQUALIFIED},
  {.id="encoder0",.resource_group="media0",.kind=CAMERA_DEVICE_ENCODER,.state=CAMERA_QUALIFIED},
  {.id="head0",.kind=CAMERA_DEVICE_PTZ,.state=CAMERA_PRESENT_UNQUALIFIED}
 };
 struct camera_resource_group groups[]={{.id="media0",.qualified_concurrent_sessions=1,.state=CAMERA_QUALIFIED}};
 struct camera_association associations[]={{.camera_id="camera0",.device_id="head0"},{.camera_id="camera1",.device_id="head0"}};
 struct camera_profile profiles[]={
  {.id="camera0-main",.camera_id="camera0",.encoder_id="encoder0",.width=1280,.height=720,.codec=CAMERA_H264},
  {.id="camera0-sub",.camera_id="camera0",.encoder_id="encoder0",.width=640,.height=360,.codec=CAMERA_H264},
  {.id="camera1-main",.camera_id="camera1",.encoder_id="encoder0",.width=1280,.height=720,.codec=CAMERA_H264}
 };
 struct camera_graph g={devices,4,associations,2,profiles,3,groups,1};
 assert(camera_graph_validate(&g)==CAMERA_OK&&camera_graph_sensor_count(&g)==2);
 devices[1].state=CAMERA_UNAVAILABLE;assert(camera_graph_sensor_count(&g)==1);devices[1].state=CAMERA_PRESENT_UNQUALIFIED;
 strcpy(profiles[2].camera_id,"missing");assert(camera_graph_validate(&g)==CAMERA_INVALID);strcpy(profiles[2].camera_id,"camera1");
 strcpy(devices[1].id,"camera0");assert(camera_graph_validate(&g)==CAMERA_INVALID);strcpy(devices[1].id,"camera1");
 groups[0].state=CAMERA_PRESENT_UNQUALIFIED;assert(camera_graph_validate(&g)==CAMERA_INVALID);groups[0].state=CAMERA_QUALIFIED;
 struct camera_backend b={.abi=CAMERA_BACKEND_ABI,.struct_bytes=sizeof(b),.name="fixture-only",.context=&stops,.enumerate=enumerate};
 assert(camera_backend_validate(&b)==CAMERA_OK&&camera_control_stop(&b,"head0")==CAMERA_UNSUPPORTED);
 struct camera_control_ops ops={.abi=CAMERA_BACKEND_ABI,.struct_bytes=sizeof(ops),.ptz_stop=stop};b.control=&ops;assert(camera_control_stop(&b,"head0")==CAMERA_OK&&stops==1);
 ops.struct_bytes--;assert(camera_backend_validate(&b)==CAMERA_INVALID);ops.struct_bytes++;
 b.abi++;assert(camera_backend_validate(&b)==CAMERA_INVALID);
 puts("BackendABI/dualphysicalcamera/sharedhead/sharedencoder/profilecount/unsupported fixtures PASS");return 0;
}
