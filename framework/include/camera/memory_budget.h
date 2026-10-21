/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_MEMORY_BUDGET_H
#define CAMERA_MEMORY_BUDGET_H
#include <camera/backend.h>
#define CAMERA_MEMORY_BUDGET_ABI 1U
/* One physical capture pool / one encoder resource, not one allocation per
 * RTSP client or main/sub profile. Inputs are platform measurements/plans;
 * there are no board defaults. Zero input-copy bytes does NOT qualify import.
 */
struct camera_memory_budget_request {
 uint32_t abi_version,struct_bytes;
 uint32_t capture_slots,stat_slots,encoder_refs,hub_slots;
 uint64_t capture_slot_bytes,capture_reference_bytes,capture_table_bytes,stat_slot_bytes;
 uint64_t encoder_input_bytes,encoder_output_bytes,encoder_ref_bytes,encoder_aux_bytes;
 uint64_t dma_fixed_bytes; /* allocator/atomic/sink/other reserved obligations */
 uint64_t user_raw_copy_bytes,user_encoded_copy_bytes,hub_slot_bytes,user_fixed_bytes;
 uint64_t dma_budget_bytes,user_budget_bytes;
};
struct camera_memory_budget_result {
 uint64_t dma_required_bytes,user_required_bytes;
 uint32_t fits_dma,fits_user;
};
/* Arithmetic/peak ownership accounting only. No allocation, overcommit,
 * fragmentation, DMA retirement/export lifetime or hardware-mask proof.
 * Failure leaves caller output unchanged. All additional worker/recording/
 * socket/kernel obligations must be supplied, not silently ignored.
 */
int camera_memory_budget(const struct camera_memory_budget_request *request,
 struct camera_memory_budget_result *result);
#endif
