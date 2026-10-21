/* SPDX-License-Identifier: GPL-2.0-only; arithmetic fixtures, never hardware. */
#include <camera/memory_budget.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void)
{
 const uint64_t mib=1048576;
 struct camera_memory_budget_request q={.abi_version=1,.struct_bytes=sizeof(q),
  .capture_slots=4,.capture_slot_bytes=2*mib,.capture_reference_bytes=4*mib,
  .capture_table_bytes=16384,.stat_slots=4,.stat_slot_bytes=12288,
  .encoder_input_bytes=2*mib,.encoder_output_bytes=2*mib,.encoder_refs=2,
  .encoder_ref_bytes=2*mib,.encoder_aux_bytes=65536,
  .user_raw_copy_bytes=1728000,.user_encoded_copy_bytes=mib,.hub_slots=1,.hub_slot_bytes=mib,
  .dma_budget_bytes=16*mib,.user_budget_bytes=4*mib};
 struct camera_memory_budget_result r;
 assert(camera_memory_budget(&q,&r)==CAMERA_OK);
 assert(r.dma_required_bytes==20*mib+131072&&!r.fits_dma);
 assert(r.user_required_bytes==1728000+2*mib&&r.fits_user);
 q.encoder_input_bytes=0; /* unqualified import proposal, not an active port */
 assert(camera_memory_budget(&q,&r)==CAMERA_OK&&r.dma_required_bytes==18*mib+131072&&!r.fits_dma);
 q.capture_slots=2;
 assert(camera_memory_budget(&q,&r)==CAMERA_OK&&r.dma_required_bytes==14*mib+131072&&r.fits_dma);
 q.dma_fixed_bytes=2*mib; /* Any needed extra sink must be counted. */
 assert(camera_memory_budget(&q,&r)==CAMERA_OK&&!r.fits_dma);
 q.dma_fixed_bytes=0;q.hub_slots=4;
 assert(camera_memory_budget(&q,&r)==CAMERA_OK&&!r.fits_user);
 q.capture_slot_bytes=UINT64_MAX;q.capture_slots=2;r.dma_required_bytes=123;
 assert(camera_memory_budget(&q,&r)==CAMERA_INVALID&&r.dma_required_bytes==123);
 q.capture_slots=1;q.capture_slot_bytes=UINT64_MAX;
 assert(camera_memory_budget(&q,&r)==CAMERA_INVALID&&r.dma_required_bytes==123);
 q.abi_version=2;assert(camera_memory_budget(&q,&r)==CAMERA_INVALID);
 assert(camera_memory_budget(NULL,&r)==CAMERA_INVALID&&camera_memory_budget(&q,NULL)==CAMERA_INVALID);
 puts("Measured footprint arithmetic, old-pool refusal, conditional import/two-slot/sink/user-budget/overflow fixtures PASS");return 0;
}
