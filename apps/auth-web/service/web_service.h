/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef AUTH_WEB_SERVICE_H
#define AUTH_WEB_SERVICE_H
#include <camera/access_policy.h>
#include <stdint.h>
/* Private trusted launch ABI, NOT an HTTP object. All chars NUL/zero padded.
 * Local little-endian Linux host/ARM launchers use this exact 768-byte layout.
 * trust_mode=1: parent-provisioned certificate, OOB/pinned trust; no UTC claim. */
#define WEB_CONFIG_MAGIC 0x41575331U
#define WEB_NS 1000000000ULL
struct web_config {
 uint32_t magic,version,bytes,trust_mode;
 char listen[128],host[128],origin[160];
 char device[64],sensor[64],profile[64],account[64];
 uint64_t boot_generation,child_generation,credential_generation;
 uint64_t absolute_ns,idle_ns,runtime_ns;
 /* Launch ABI v2: HTTP login wait only (1ms..35s), never CP CPU deadline. */
 uint64_t response_ns;
 unsigned char reserved[24];
};
_Static_assert(sizeof(struct web_config)==768,"private config ABI");
/* Owned-loop boundary. Callback only after the immutable principal and exact
 * resource grant pass. qualified=0 means explicit unsupported, no callback.
 * No producer/buffer is allocated here. Every future media effect/continuation
 * MUST call this boundary again, and tear down on deny/clock/child failure. */
typedef int (*web_media_effect)(void *);
int web_media_admit(struct camera_access_policy *,
 const struct camera_access_principal *,const struct camera_access_request *,
 uint64_t now,int healthy,int qualified,web_media_effect,void *);
int web_parent_sandbox(void);
#endif
