// SPDX-License-Identifier: GPL-2.0-only
#include <camera/memory_budget.h>
#include <stdint.h>
static int add(uint64_t *total,uint64_t bytes,uint32_t count)
{
 if(count&&bytes>UINT64_MAX/count)return CAMERA_INVALID;
 uint64_t product=bytes*count;
 if(product>UINT64_MAX-*total)return CAMERA_INVALID;
 *total+=product;return CAMERA_OK;
}
int camera_memory_budget(const struct camera_memory_budget_request *q,struct camera_memory_budget_result *out)
{
 if(!q||!out||q->abi_version!=CAMERA_MEMORY_BUDGET_ABI||q->struct_bytes<sizeof(*q))return CAMERA_INVALID;
 struct camera_memory_budget_result result={0};
 if(add(&result.dma_required_bytes,q->capture_slot_bytes,q->capture_slots)||
    add(&result.dma_required_bytes,q->capture_reference_bytes,1)||
    add(&result.dma_required_bytes,q->capture_table_bytes,1)||
    add(&result.dma_required_bytes,q->stat_slot_bytes,q->stat_slots)||
    add(&result.dma_required_bytes,q->encoder_input_bytes,1)||
    add(&result.dma_required_bytes,q->encoder_output_bytes,1)||
    add(&result.dma_required_bytes,q->encoder_ref_bytes,q->encoder_refs)||
    add(&result.dma_required_bytes,q->encoder_aux_bytes,1)||
    add(&result.dma_required_bytes,q->dma_fixed_bytes,1)||
    add(&result.user_required_bytes,q->user_raw_copy_bytes,1)||
    add(&result.user_required_bytes,q->user_encoded_copy_bytes,1)||
    add(&result.user_required_bytes,q->hub_slot_bytes,q->hub_slots)||
    add(&result.user_required_bytes,q->user_fixed_bytes,1))return CAMERA_INVALID;
 result.fits_dma=result.dma_required_bytes<=q->dma_budget_bytes;
 result.fits_user=result.user_required_bytes<=q->user_budget_bytes;
 *out=result;return CAMERA_OK;
}
