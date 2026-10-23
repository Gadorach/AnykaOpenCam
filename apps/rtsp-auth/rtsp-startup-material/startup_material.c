/* SPDX-License-Identifier: GPL-2.0-only */
#include "startup_material.h"
#include <camera/access_policy.h>
#include <mbedtls/platform_util.h>
#include <string.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "The existing verifier startup SOURCE ABI is little endian only"
#endif
_Static_assert(sizeof(struct rv_material)==1536,"actual verifier material ABI");
_Static_assert(offsetof(struct rv_material,username)==348,"actual routing prefix");
_Static_assert(offsetof(struct rv_material,ha1)==476,"actual secret field");

static int identifier(const char value[CAMERA_ID_BYTES])
{
 size_t i;
 if(!value[0])return 0;
 for(i=0;i<CAMERA_ID_BYTES;i++){
  unsigned char c=(unsigned char)value[i];
  if(!c){
   for(;i<CAMERA_ID_BYTES;i++)if(value[i])return 0;
   return 1;
  }
  if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||
       c=='_'||c=='-'||c=='.'))return 0;
 }
 return 0;
}

void qc07f_rtsp_startup_material_clear_v1(struct rv_material *material)
{if(material)mbedtls_platform_zeroize(material,sizeof(*material));}

int qc07f_rtsp_startup_material_build_v1(const struct candidate_config *cfg,
 const struct camera_rtsp_registry *routes,struct camera_digest_span username,
 struct camera_digest_span realm,struct camera_digest_span password,
 struct rv_material *out)
{
 if(!out)return CAMERA_INVALID;
 qc07f_rtsp_startup_material_clear_v1(out);
 if(!cfg||!routes||cfg->abi!=CANDIDATE_AUTH_ABI||cfg->struct_bytes!=sizeof(*cfg)||
    cfg->host_fixture_only||cfg->image.abi!=RV_ABI||cfg->image.profile!=RV_NATIVE||
    cfg->begin_contract!=CANDIDATE_BEGIN_LOGICAL_ONLY||
    !cfg->epoch||!cfg->boot_generation||!cfg->credential_generation||
    !identifier(cfg->device_id)||!identifier(cfg->account_id)||!identifier(cfg->profile_id)||
    (cfg->algorithm!=CAMERA_DIGEST_SHA256&&cfg->algorithm!=CAMERA_DIGEST_MD5_LEGACY)||
    !cfg->nonce_ttl_ms||cfg->nonce_ttl_ms>CAMERA_DIGEST_TTL_MAX_MS||!cfg->graph||
    cfg->graph->profile_count>8||cfg->graph->device_count>16||
    cfg->graph->association_count>16||cfg->graph->group_count>8||
    camera_graph_validate(cfg->graph)!=CAMERA_OK)return CAMERA_INVALID;

 const struct camera_profile *profile=NULL;
 for(size_t i=0;i<cfg->graph->profile_count;i++)
  if(!memcmp(cfg->graph->profiles[i].id,cfg->profile_id,CAMERA_ID_BYTES))
   profile=&cfg->graph->profiles[i];
 if(!profile||!identifier(profile->camera_id)||profile->codec!=CAMERA_H264||
    profile->width!=1280||profile->height!=720)return CAMERA_INVALID;

 struct camera_digest_wire_limit checked_limit;
 if(camera_digest_wire_limit_init(&checked_limit,0,cfg->rate_window_ms,
      cfg->global_verifies,cfg->global_challenges)||
    camera_digest_wire_limit_init(&checked_limit,0,cfg->rate_window_ms,
      cfg->account_verifies,cfg->account_challenges)||
    camera_digest_wire_limit_init(&checked_limit,0,cfg->rate_window_ms,
      cfg->peer_verifies,cfg->peer_challenges))return CAMERA_INVALID;

 /* Bounded termination checks precede the existing string-based validator.
  * Rebuild its public routes to avoid copying unused caller storage/padding. */
 struct camera_rtsp_registry checked_routes;
 if(!memchr(routes->live_uri,0,sizeof(routes->live_uri))||
    !memchr(routes->track_uri,0,sizeof(routes->track_uri))||
    camera_rtsp_registry_init(&checked_routes,routes->live_uri,routes->track_uri))
  return CAMERA_INVALID;

 /* Existing helper validates all borrowed credential lengths/characters and
  * uses the maintained Mbed TLS implementation. No password copy or defaults. */
 int result=camera_rtsp_digest_ha1(cfg->algorithm,username,realm,password,out->ha1);
 if(result){
  qc07f_rtsp_startup_material_clear_v1(out);
  return result==CAMERA_DIGEST_INVALID?CAMERA_INVALID:CAMERA_IO_ERROR;
 }
 out->abi=RV_ABI;out->bytes=sizeof(*out);out->domain=CAMERA_ACCESS_RTSP;
 out->algorithm=cfg->algorithm;out->epoch=cfg->epoch;out->boot_generation=cfg->boot_generation;
 out->credential_generation=cfg->credential_generation;
 out->account=1; /* exact current native backend; never PUBLICDUMMY or UI/admin */
 out->username_bytes=(uint32_t)username.length;out->realm_bytes=(uint32_t)realm.length;
 out->ha1_bytes=cfg->algorithm==CAMERA_DIGEST_SHA256?32U:16U;
 out->nonce_ttl_ms=cfg->nonce_ttl_ms;out->window_ms=cfg->rate_window_ms;
 out->global_verifies=cfg->global_verifies;out->global_challenges=cfg->global_challenges;
 out->account_verifies=cfg->account_verifies;out->account_challenges=cfg->account_challenges;
 out->peer_verifies=cfg->peer_verifies;out->peer_challenges=cfg->peer_challenges;
 memcpy(out->device,cfg->device_id,sizeof(out->device));
 memcpy(out->account_id,cfg->account_id,sizeof(out->account_id));
 memcpy(out->sensor,profile->camera_id,sizeof(out->sensor));
 memcpy(out->profile,cfg->profile_id,sizeof(out->profile));
 memcpy(out->username,username.data,username.length);
 memcpy(out->realm,realm.data,realm.length);
 memcpy(out->live,checked_routes.live_uri,sizeof(out->live));
 memcpy(out->track,checked_routes.track_uri,sizeof(out->track));
 return CAMERA_OK;
}
