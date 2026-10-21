/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_ACCESS_POLICY_H
#define CAMERA_ACCESS_POLICY_H
#include <camera/backend.h>

#define CAMERA_ACCESS_ABI 1U
#define CAMERA_ACCESS_MAX_RESOURCES 16U
#define CAMERA_ACCESS_MAX_ACCOUNTS 8U
#define CAMERA_ACCESS_MAX_GRANTS 8U
#define CAMERA_ACCESS_MAX_SESSIONS 16U

enum camera_access_result {
 CAMERA_ACCESS_OK=0,CAMERA_ACCESS_DENIED=-1,CAMERA_ACCESS_INVALID=-2,
 CAMERA_ACCESS_FULL=-3,CAMERA_ACCESS_EXHAUSTED=-4
};
enum camera_access_domain {
 CAMERA_ACCESS_UI=1,CAMERA_ACCESS_ADMIN,CAMERA_ACCESS_RTSP,
 CAMERA_ACCESS_ONVIF,CAMERA_ACCESS_SETUP
};
enum camera_access_scope {
 CAMERA_ACCESS_DEVICE=1,CAMERA_ACCESS_SENSOR,CAMERA_ACCESS_PROFILE,CAMERA_ACCESS_PTZ
};
enum camera_access_role {
 CAMERA_ACCESS_VIEWER=1U<<0,CAMERA_ACCESS_OPERATOR=1U<<1,
 CAMERA_ACCESS_CONFIGURATOR=1U<<2,CAMERA_ACCESS_RECORDING_READER=1U<<3,
 CAMERA_ACCESS_RECORDING_MANAGER=1U<<4,CAMERA_ACCESS_ONBOARDER=1U<<5,
 CAMERA_ACCESS_RESET_REQUESTER=1U<<6,CAMERA_ACCESS_CREDENTIAL_MANAGER=1U<<7,
 CAMERA_ACCESS_DISCOVERER=1U<<8
};
/* Exactly one operation per request. No shell/file/MMIO/erase operation exists. */
enum camera_access_operation {
 CAMERA_ACCESS_LIVE=1U<<0,CAMERA_ACCESS_SNAPSHOT=1U<<1,
 CAMERA_ACCESS_EVENTS=1U<<2,CAMERA_ACCESS_PTZ_CONTROL=1U<<3,
 CAMERA_ACCESS_IMAGING_CONFIG=1U<<4,CAMERA_ACCESS_DEVICE_CONFIG=1U<<5,
 CAMERA_ACCESS_RECORDING_CONFIG=1U<<6,CAMERA_ACCESS_PLAYBACK=1U<<7,
 CAMERA_ACCESS_DOWNLOAD=1U<<8,CAMERA_ACCESS_ONBOARD=1U<<9,
 CAMERA_ACCESS_SETTINGS_RESET_REQUEST=1U<<10,
 CAMERA_ACCESS_CREDENTIAL_CONFIG=1U<<11,CAMERA_ACCESS_DISCOVERY=1U<<12
};
/* Canonical, zero-padded IDs reuse the trusted CORE graph. A profile MUST
 * name its physical sensor; a shared PTZ is one explicit resource, not a sensor.
 * Only PROFILE resources have sensor_id. No implicit parent/wildcard grant. */
struct camera_access_resource {
 uint32_t kind;char id[CAMERA_ID_BYTES],sensor_id[CAMERA_ID_BYTES];
};
struct camera_access_grant {
 uint32_t kind;char resource_id[CAMERA_ID_BYTES];uint32_t operations;
};
struct camera_access_account {
 uint32_t abi,struct_bytes,domain,roles;
 char account_id[CAMERA_ID_BYTES];uint64_t credential_generation;
 uint32_t grant_count,reserved;
 struct camera_access_grant grants[CAMERA_ACCESS_MAX_GRANTS];
};
/* NOT a wire object or authentication algorithm. Only the trusted auth adapter
 * may construct this AFTER actual protocol/session verification. verified=1
 * alone does not authenticate anything. All generations are nonzero; all times
 * are from the same trusted boot-local monotonic clock. session_generation must
 * strictly increase per (domain,account) throughout the boot, even on credential
 * change; replaying a previously issued fact cannot resurrect a revoked session. */
struct camera_access_verified {
 uint32_t abi,struct_bytes,verified,domain;
 char device_id[CAMERA_ID_BYTES],account_id[CAMERA_ID_BYTES];
 uint64_t boot_generation,credential_generation,session_generation;
 uint64_t issued_ns,expires_ns;uint32_t reserved;
};
/* Immutable value snapshot, never a bearer token. Policy retains an exact
 * field-wise copy and rejects mutation. Only owner-held snapshots may be used. */
struct camera_access_principal {
 struct camera_access_verified fact;uint64_t policy_revision,snapshot_serial;
};
struct camera_access_request {
 uint32_t abi,struct_bytes,domain,kind,operation,reserved;
 char device_id[CAMERA_ID_BYTES],sensor_id[CAMERA_ID_BYTES];
 char profile_id[CAMERA_ID_BYTES],resource_id[CAMERA_ID_BYTES];
};
/* Storage is public only for caller-owned static allocation, not direct edits.
 * Serialize ALL calls and protected effects with the owner. No internal locks,
 * allocation, I/O, clock reads, secrets or callbacks. */
struct camera_access_record {
 struct camera_access_account account;uint64_t revision,last_session_generation;
 uint32_t used,enabled,nvr_peer; /* peer index + 1; zero means unmapped */
};
struct camera_access_session {
 struct camera_access_principal principal;uint32_t active;
};
struct camera_access_policy {
 uint32_t initialized,clock_failed,resource_count;
 char device_id[CAMERA_ID_BYTES];uint64_t boot_generation,last_ns,next_serial;
 struct camera_access_resource resources[CAMERA_ACCESS_MAX_RESOURCES];
 struct camera_access_record records[CAMERA_ACCESS_MAX_ACCOUNTS];
 struct camera_access_session sessions[CAMERA_ACCESS_MAX_SESSIONS];
};

/* init only at owner startup, never reset in a live boot; new boot generation
 * required after restart. Resources are copied; unsuccessful init is unusable. */
int camera_access_init(struct camera_access_policy *policy,
 const char device_id[CAMERA_ID_BYTES],uint64_t boot_generation,uint64_t now_ns,
 const struct camera_access_resource *resources,uint32_t resource_count);
int camera_access_put_account(struct camera_access_policy *policy,
 const struct camera_access_account *account);
int camera_access_revoke_account(struct camera_access_policy *policy,
 uint32_t domain,const char account_id[CAMERA_ID_BYTES]);
/* Explicit ONVIF-managed NVR opt-in: existing ONVIF and RTSP accounts only,
 * matching credential generations. Does not copy credentials or grants.
 * ONVIF must have a profile LIVE grant and every ONVIF media profile must have
 * an explicit LIVE grant in the designated RTSP account. Updating or
 * revoking either breaks the mapping and invalidates BOTH principal snapshots.
 * ONVIF is denied until mapped. No UI/admin account can be linked. */
int camera_access_map_nvr(struct camera_access_policy *policy,
 const char onvif_account_id[CAMERA_ID_BYTES],
 const char rtsp_account_id[CAMERA_ID_BYTES]);
int camera_access_issue(struct camera_access_policy *policy,
 const struct camera_access_verified *verified,uint64_t now_ns,
 struct camera_access_principal *principal);
int camera_access_revoke_session(struct camera_access_policy *policy,
 const struct camera_access_principal *principal);
int camera_access_check(struct camera_access_policy *policy,
 const struct camera_access_principal *principal,
 const struct camera_access_request *request,uint64_t now_ns);
/* Reclaims expired/stale session slots, not a socket/stream teardown callback.
 * Owner must recheck and terminate affected subscriptions/control leases. */
int camera_access_sweep(struct camera_access_policy *policy,uint64_t now_ns);
#endif
