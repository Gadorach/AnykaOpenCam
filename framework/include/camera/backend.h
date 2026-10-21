/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_BACKEND_H
#define CAMERA_BACKEND_H
#include <stddef.h>
#include <stdint.h>
#define CAMERA_BACKEND_ABI 1U
#define CAMERA_ID_BYTES 64U
#define CAMERA_MAX_PLANES 3U
enum camera_result {
 CAMERA_OK=0,CAMERA_UNSUPPORTED=-1,CAMERA_NOT_READY=-2,CAMERA_INVALID=-3,
 CAMERA_BUSY=-4,CAMERA_IO_ERROR=-5,CAMERA_STALE_LEASE=-6,CAMERA_DISCONNECTED=-7
};
enum camera_device_kind {
 CAMERA_DEVICE_SENSOR=1,CAMERA_DEVICE_ENCODER,CAMERA_DEVICE_PTZ,
 CAMERA_DEVICE_LIGHT,CAMERA_DEVICE_AMBIENT,CAMERA_DEVICE_RESET_INPUT,
 CAMERA_DEVICE_RADIO,CAMERA_DEVICE_STORAGE
};
enum camera_evidence_state {CAMERA_UNAVAILABLE=0,CAMERA_PRESENT_UNQUALIFIED,CAMERA_QUALIFIED};
enum camera_codec {CAMERA_PIXELS=0,CAMERA_JPEG=1,CAMERA_H264=2};
enum camera_radio_capability {
 CAMERA_RADIO_STA=1U<<0,CAMERA_RADIO_AP=1U<<1,
 CAMERA_RADIO_AP_STA_CONCURRENT=1U<<2,CAMERA_RADIO_SCAN_ASSOCIATED=1U<<3
};
struct camera_device {
 char id[CAMERA_ID_BYTES],resource_group[CAMERA_ID_BYTES];
 enum camera_device_kind kind;
 enum camera_evidence_state state;
 uint64_t capabilities;
};
struct camera_association {char camera_id[CAMERA_ID_BYTES],device_id[CAMERA_ID_BYTES];};
/* Main/sub/quality outputs are profiles ofonephysicalsensor, not newcameras. */
struct camera_profile {
 char id[CAMERA_ID_BYTES],camera_id[CAMERA_ID_BYTES],encoder_id[CAMERA_ID_BYTES];
 uint32_t width,height;enum camera_codec codec;
};
struct camera_resource_group {
 char id[CAMERA_ID_BYTES];uint32_t qualified_concurrent_sessions;
 enum camera_evidence_state state;
};
struct camera_graph {
 const struct camera_device *devices;size_t device_count;
 const struct camera_association *associations;size_t association_count;
 const struct camera_profile *profiles;size_t profile_count;
 const struct camera_resource_group *groups;size_t group_count;
};
/* No physicaladdress crosses thisboundary. Backendvalidatesgeneration,
 * ownsallCPU/DMA transitions andkeepsstorage alive untilrelease/retiredDMA.
 */
struct camera_plane {uint32_t offset,stride,bytes;};
struct camera_frame {
 uint64_t lease_token,generation,monotonic_ns;
 int dma_buf_fd;uint32_t width,height,pixel_format,plane_count;
 struct camera_plane planes[CAMERA_MAX_PLANES];
};
struct camera_packet {
 uint64_t lease_token,generation,monotonic_ns;
 enum camera_codec codec;uint32_t keyframe;
 const uint8_t *data;size_t bytes;
};
struct camera_ptz_velocity {int32_t pan,tilt;uint32_t lease_ms;};
struct camera_sensor_value {int64_t value;uint64_t monotonic_ns;uint32_t unit;};
struct camera_radio_link {uint32_t associated,mode;uint64_t generation;char operation_id[CAMERA_ID_BYTES];};
struct camera_wifi_profile {
 const uint8_t *ssid;size_t ssid_bytes;
 const uint8_t *secret;size_t secret_bytes;uint32_t security;
};
struct camera_storage_info {uint64_t generation,capacity_bytes,free_bytes;uint32_t present,read_only;};
struct camera_capture_ops {
 uint32_t abi,struct_bytes;
 int (*open)(void *context,const char *camera_id,const char *profile_id,void **handle);
 int (*next)(void *handle,uint64_t deadline_ns,struct camera_frame *frame);
 int (*release)(void *handle,uint64_t token,uint64_t generation);
 int (*close)(void *handle);
};
struct camera_encoder_ops {
 uint32_t abi,struct_bytes;
 int (*open)(void *context,const char *encoder_id,const char *profile_id,void **handle);
 int (*encode)(void *handle,const struct camera_frame *frame,struct camera_packet *packet);
 int (*release)(void *handle,uint64_t token,uint64_t generation);
 int (*close)(void *handle);
};
struct camera_control_ops {
 uint32_t abi,struct_bytes;
 int (*ptz_velocity)(void *context,const char *axis_group,const struct camera_ptz_velocity *command);
 int (*ptz_stop)(void *context,const char *axis_group);
 int (*light_set)(void *context,const char *light_id,uint32_t intensity);
 int (*sensor_read)(void *context,const char *sensor_id,struct camera_sensor_value *value);
 int (*colour_mode)(void *context,const char *camera_id,uint32_t monochrome);
};
/* Sharednetworkservice owns pendingprofile/rollback/notification andresumable
 * operationIDs. Driver reports realcapabilities; secret buffersneverlogged.
 */
struct camera_radio_ops {
 uint32_t abi,struct_bytes;
 int (*status)(void *context,const char *radio_id,struct camera_radio_link *link);
 int (*scan_start)(void *context,const char *radio_id,uint32_t allow_disruption,const char *operation_id);
 int (*connect_start)(void *context,const char *radio_id,const struct camera_wifi_profile *profile,const char *operation_id);
 int (*ap_start)(void *context,const char *radio_id,const struct camera_wifi_profile *profile,const char *operation_id);
};
/* Platform ownsblock/mount lifecycle. Sharedrecorder opens onlyanowned
 * directoryfd andneverformats/probes arbitrarydevices orprunesotherfiles.
 */
struct camera_storage_ops {
 uint32_t abi,struct_bytes;
 int (*status)(void *context,const char *storage_id,struct camera_storage_info *info);
 int (*recording_directory)(void *context,const char *storage_id,int *directory_fd,uint64_t *generation);
};
/* Smallstatic adapters bydefault; optionaldynamic loadingmustvalidateABI and
 * structure size. Auth/UI/ONVIF/config/retention are sharedservices, notdrivers.
 */
struct camera_backend {
 uint32_t abi,struct_bytes;const char *name;void *context;
 int (*enumerate)(void *context,struct camera_graph *graph);
 const struct camera_capture_ops *capture;
 const struct camera_encoder_ops *encoder;
 const struct camera_control_ops *control;
 const struct camera_radio_ops *radio;
 const struct camera_storage_ops *storage;
};
int camera_backend_validate(const struct camera_backend *backend);
int camera_graph_validate(const struct camera_graph *graph);
size_t camera_graph_sensor_count(const struct camera_graph *graph);
int camera_control_stop(const struct camera_backend *backend,const char *id);
#endif
