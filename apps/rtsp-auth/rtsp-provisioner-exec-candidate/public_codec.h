/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_EXEC_PUBLIC_CODEC_H
#define QC07F_RTSP_EXEC_PUBLIC_CODEC_H
#include "../rtsp-private-handoff/private_handoff.h"
#define PE_VERSION 1U
#define PE_PREFIX 1374U
#define PE_MAX_REQUEST 7934U
#define PE_PACKET 8192U
#define PE_REPLY 1392U
#define PE_RIGHTS_MAX 8U
#define PE_MEMORY (2U*1024U*1024U)
#define PE_ARGV0 "qc07f-rtsp-provisioner-worker-v1"
struct pe_bindings {
 uint64_t started,deadline,device[3],inode[3]; /* ingress, directory, child control */
};
/* Local, never sent as C storage; graph pointers refer ONLY to these arrays. */
struct pe_public {
 struct candidate_config config;
 struct camera_rtsp_registry routes;
 struct camera_graph graph;
 struct camera_device devices[16];
 struct camera_association associations[16];
 struct camera_profile profiles[8];
 struct camera_resource_group groups[8];
 struct pe_bindings binding;
};
uint32_t pe_get32(const unsigned char *);
uint64_t pe_get64(const unsigned char *);
/* Typed valid/disjoint immutable storage, bounded counts. No crypto/credentials. */
int pe_config_class(const struct candidate_config *);
int pe_public_validate(const struct candidate_config *,const struct camera_rtsp_registry *);
int pe_encode_request(unsigned char [PE_PACKET],size_t *,
 const struct candidate_config *,const struct camera_rtsp_registry *,const struct pe_bindings *);
int pe_decode_request(struct pe_public *,const unsigned char *,size_t);
int pe_encode_reply(unsigned char [PE_REPLY],const struct candidate_config *,
 const struct camera_rtsp_registry *,uint64_t,uint64_t);
#endif
