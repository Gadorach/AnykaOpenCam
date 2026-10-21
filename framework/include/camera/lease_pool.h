/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_LEASE_POOL_H
#define CAMERA_LEASE_POOL_H
#include <camera/backend.h>
#define CAMERA_POOL_MAX 8U
enum camera_slot_state {CAMERA_SLOT_FREE,CAMERA_SLOT_QUEUED,CAMERA_SLOT_DEVICE,CAMERA_SLOT_DONE,CAMERA_SLOT_LEASED};
struct camera_slot {enum camera_slot_state state;uint64_t generation;};
struct camera_lease_pool {uint32_t slots;struct camera_slot slot[CAMERA_POOL_MAX];};
/* Caller serializesalloperations. Thismodel neverallocates/freesbuffers or
 * guessesDMAcompletion; platformdriver supplies actualretirement evidence.
 */
int camera_pool_init(struct camera_lease_pool *pool,uint32_t slots);
int camera_pool_queue(struct camera_lease_pool *pool,uint32_t index);
int camera_pool_device(struct camera_lease_pool *pool,uint32_t index,uint64_t *generation);
int camera_pool_done(struct camera_lease_pool *pool,uint32_t index,uint64_t generation);
int camera_pool_lease(struct camera_lease_pool *pool,uint32_t index,uint64_t *generation);
int camera_pool_release(struct camera_lease_pool *pool,uint32_t index,uint64_t generation);
int camera_pool_stop(struct camera_lease_pool *pool,uint32_t device_dma_retired);
uint32_t camera_pool_queued(const struct camera_lease_pool *pool);
#endif
