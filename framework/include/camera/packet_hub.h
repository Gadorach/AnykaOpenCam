/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_PACKET_HUB_H
#define CAMERA_PACKET_HUB_H
#include <camera/backend.h>
#define CAMERA_HUB_SLOTS 4U
#define CAMERA_HUB_CONSUMERS 4U
struct camera_hub_slot {
 uint8_t *storage;size_t capacity;
 struct camera_packet packet;
 uint64_t sequence,generation;
 uint32_t pending,acquired;
};
struct camera_packet_hub {
 struct camera_hub_slot slots[CAMERA_HUB_SLOTS];
 uint64_t next_sequence;
 uint32_t subscribers,need_keyframe,stopped,slot_count;
};
/* Caller serializes operations. Publish copies codec output to bounded owned
 * storage; an acquired packet is NEVER overwritten. Slow pending consumers
 * may lose their backlog and must restart from an IDR. Each physical stream
 * gets its own hub; this does not invent extra capture/encoder engines.
 * Initialize only fresh storage with no old leases/references.
 */
int camera_hub_init(struct camera_packet_hub *hub,uint8_t *storage,size_t slot_bytes);
int camera_hub_init_count(struct camera_packet_hub *hub,uint8_t *storage,size_t slot_bytes,uint32_t slots);
int camera_hub_subscribe(struct camera_packet_hub *hub,uint32_t consumer);
int camera_hub_unsubscribe(struct camera_packet_hub *hub,uint32_t consumer);
int camera_hub_publish(struct camera_packet_hub *hub,const struct camera_packet *packet);
int camera_hub_acquire(struct camera_packet_hub *hub,uint32_t consumer,struct camera_packet *packet);
int camera_hub_release(struct camera_packet_hub *hub,uint32_t consumer,uint64_t token,uint64_t generation);
/* Under the same caller serialization, verify this exact acquired CPU view at
 * every deferred effect. This is not authentication, DMA retirement or source
 * health/profile-generation proof. A stopped hub denies new effects even while
 * acquired backing remains retained for eventual release. */
int camera_hub_lease_valid(const struct camera_packet_hub *hub,uint32_t consumer,
                          const struct camera_packet *packet);
int camera_hub_stop(struct camera_packet_hub *hub);
int camera_hub_drained(const struct camera_packet_hub *hub);
#endif
