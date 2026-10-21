/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NQ_SOURCE_ABI_H
#define NQ_SOURCE_ABI_H
#include "verifier.h"
#include <stddef.h>
#include <stdint.h>
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Private qualification SOURCE ABI is little-endian only"
#endif
#define NQ_MATERIAL_BYTES 1536U
#define NQ_PRIVATE_ACCOUNT UINT64_C(1)
#define NQ_DUMMY_ACCOUNT UINT64_C(0x505542444d59)
/* Actual reviewed header, not guessed/persistent/public format. Padding is
 * zeroed by generator. Assert both HOST and ARM native SOURCE layout. */
typedef char nq_size_assert[sizeof(struct rv_material)==NQ_MATERIAL_BYTES?1:-1];
typedef char nq_epoch_assert[offsetof(struct rv_material,epoch)==16?1:-1];
typedef char nq_gen_assert[offsetof(struct rv_material,credential_generation)==32?1:-1];
typedef char nq_account_assert[offsetof(struct rv_material,account)==40?1:-1];
typedef char nq_username_assert[offsetof(struct rv_material,username)==348?1:-1];
typedef char nq_realm_assert[offsetof(struct rv_material,realm)==412?1:-1];
typedef char nq_ha1_assert[offsetof(struct rv_material,ha1)==476?1:-1];
typedef char nq_live_assert[offsetof(struct rv_material,live)==508?1:-1];
typedef char nq_track_assert[offsetof(struct rv_material,track)==1021?1:-1];
struct nq_align_probe{char first;struct rv_material value;};
typedef char nq_align_assert[offsetof(struct nq_align_probe,value)==8?1:-1];
/* ONLY public fields needed by root helper; stops before lengths/IDs/username/
 * realm/HA1. rv_start internally owns its documented nonsecret-prefix read. */
struct nq_public_prefix {uint32_t abi,bytes,domain,algorithm;uint64_t epoch,boot_generation,credential_generation,account;};
typedef char nq_prefix_assert[sizeof(struct nq_public_prefix)==48?1:-1];
#endif
