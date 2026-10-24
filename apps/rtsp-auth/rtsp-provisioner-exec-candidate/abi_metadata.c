/* SPDX-License-Identifier: GPL-2.0-only */
/* Offline object metadata ONLY, not linked into either role; no functions. */
#include "exec_owner.h"
_Static_assert(PE_PREFIX==1374&&PE_REPLY==1392&&PE_MAX_REQUEST==7934,"public protocol");
_Static_assert(sizeof(struct rv_material)==1536,"real material ABI");
const uint32_t pe_public_abi_metadata[]={
 PE_VERSION,PE_PREFIX,PE_MAX_REQUEST,PE_PACKET,PE_REPLY,144,128,204,72,
 sizeof(struct rv_material),sizeof(struct candidate_config),
 sizeof(struct candidate_prestarted),sizeof(struct pe_public),sizeof(struct pe_exec_owner),
 PE_MEMORY,131072
};
