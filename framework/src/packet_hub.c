// SPDX-License-Identifier: GPL-2.0-only
#include <camera/packet_hub.h>
#include <string.h>

static int valid(const struct camera_packet_hub *h)
{return h&&h->slot_count&&h->slot_count<=CAMERA_HUB_SLOTS&&h->slots[0].storage&&h->slots[0].capacity;}
static int consumer_valid(const struct camera_packet_hub *h,uint32_t c)
{return valid(h)&&c<CAMERA_HUB_CONSUMERS&&(h->subscribers&(1U<<c));}
static int acquired(const struct camera_packet_hub *h,uint32_t bit)
{
 for(unsigned i=0;i<h->slot_count;i++)if(h->slots[i].acquired&bit)return 1;
 return 0;
}
static void lose_backlog(struct camera_packet_hub *h,uint32_t consumers)
{
 for(unsigned i=0;i<h->slot_count;i++)
  h->slots[i].pending&=~(consumers&~h->slots[i].acquired);
 h->need_keyframe|=consumers;
}
int camera_hub_init(struct camera_packet_hub *h,uint8_t *storage,size_t bytes)
{return camera_hub_init_count(h,storage,bytes,CAMERA_HUB_SLOTS);}
int camera_hub_init_count(struct camera_packet_hub *h,uint8_t *storage,size_t bytes,uint32_t slots)
{
 if(!h||!storage||!bytes||!slots||slots>CAMERA_HUB_SLOTS||bytes>SIZE_MAX/slots)return CAMERA_INVALID;
 memset(h,0,sizeof(*h));h->next_sequence=1;h->slot_count=slots;
 for(unsigned i=0;i<slots;i++){h->slots[i].storage=storage+i*bytes;h->slots[i].capacity=bytes;}
 return CAMERA_OK;
}
int camera_hub_subscribe(struct camera_packet_hub *h,uint32_t c)
{
 if(!valid(h)||c>=CAMERA_HUB_CONSUMERS)return CAMERA_INVALID;
 if(h->stopped)return CAMERA_NOT_READY;
 uint32_t bit=1U<<c;if(h->subscribers&bit)return CAMERA_BUSY;
 h->subscribers|=bit;h->need_keyframe|=bit;return CAMERA_OK;
}
int camera_hub_unsubscribe(struct camera_packet_hub *h,uint32_t c)
{
 if(!consumer_valid(h,c))return CAMERA_INVALID;
 uint32_t bit=1U<<c;if(acquired(h,bit))return CAMERA_BUSY;
 for(unsigned i=0;i<h->slot_count;i++)h->slots[i].pending&=~bit;
 h->subscribers&=~bit;h->need_keyframe&=~bit;return CAMERA_OK;
}
int camera_hub_publish(struct camera_packet_hub *h,const struct camera_packet *p)
{
 if(!valid(h)||!p||!p->data||!p->bytes||p->bytes>h->slots[0].capacity||
    p->keyframe>1||(p->codec!=CAMERA_H264&&p->codec!=CAMERA_JPEG)||
    (p->codec==CAMERA_JPEG&&!p->keyframe))return CAMERA_INVALID;
 if(h->stopped||h->next_sequence==UINT64_MAX)return CAMERA_NOT_READY;
 uint32_t eligible=h->subscribers&~h->need_keyframe;
 if(p->keyframe)eligible=h->subscribers;
 if(!eligible)return CAMERA_OK; /* no consumers can decode this P frame */
 struct camera_hub_slot *slot=NULL;
 for(unsigned i=0;i<h->slot_count;i++){
  struct camera_hub_slot *s=&h->slots[i];
  if(!s->pending&&!s->acquired&&s->generation!=UINT64_MAX){slot=s;break;}
 }
 if(!slot){
  for(unsigned i=0;i<h->slot_count;i++){
   struct camera_hub_slot *s=&h->slots[i];
   if(!s->acquired&&s->generation!=UINT64_MAX&&(!slot||s->sequence<slot->sequence))slot=s;
  }
  if(!slot){lose_backlog(h,h->subscribers);return CAMERA_BUSY;}
  lose_backlog(h,slot->pending);
  eligible=h->subscribers&~h->need_keyframe;
  if(p->keyframe)eligible=h->subscribers;
  if(!eligible)return CAMERA_OK;
 }
 memcpy(slot->storage,p->data,p->bytes);
 slot->packet=*p;slot->packet.data=slot->storage;
 slot->generation++;slot->sequence=h->next_sequence++;
 slot->packet.lease_token=(uint64_t)(slot-h->slots)+1;
 slot->packet.generation=slot->generation;
 slot->pending=eligible;slot->acquired=0;
 if(p->keyframe)h->need_keyframe&=~eligible;
 return CAMERA_OK;
}
int camera_hub_acquire(struct camera_packet_hub *h,uint32_t c,struct camera_packet *p)
{
 if(!consumer_valid(h,c)||!p)return CAMERA_INVALID;
 if(h->stopped)return CAMERA_NOT_READY;
 uint32_t bit=1U<<c;if(acquired(h,bit))return CAMERA_BUSY;
 struct camera_hub_slot *slot=NULL;
 for(unsigned i=0;i<h->slot_count;i++){
  struct camera_hub_slot *s=&h->slots[i];
  if((s->pending&bit)&&(!slot||s->sequence<slot->sequence))slot=s;
 }
 if(!slot)return CAMERA_NOT_READY;
 slot->acquired|=bit;*p=slot->packet;return CAMERA_OK;
}
int camera_hub_release(struct camera_packet_hub *h,uint32_t c,uint64_t token,uint64_t generation)
{
 if(!consumer_valid(h,c)||token<1||token>h->slot_count)return CAMERA_INVALID;
 struct camera_hub_slot *s=&h->slots[token-1];uint32_t bit=1U<<c;
 if(s->generation!=generation)return CAMERA_STALE_LEASE;
 if(!(s->acquired&bit))return CAMERA_INVALID;
 s->acquired&=~bit;s->pending&=~bit;return CAMERA_OK;
}
int camera_hub_lease_valid(const struct camera_packet_hub *h,uint32_t c,const struct camera_packet *p)
{
 if(!consumer_valid(h,c)||!p||h->stopped||p->lease_token<1||p->lease_token>h->slot_count)return 0;
 const struct camera_hub_slot *s=&h->slots[p->lease_token-1];
 return (s->acquired&(1U<<c))&&s->generation==p->generation&&
        s->packet.data==p->data&&s->packet.bytes==p->bytes&&s->packet.codec==p->codec&&
        s->packet.keyframe==p->keyframe&&s->packet.monotonic_ns==p->monotonic_ns;
}
int camera_hub_stop(struct camera_packet_hub *h)
{
 if(!valid(h))return CAMERA_INVALID;
 h->stopped=1;
 for(unsigned i=0;i<h->slot_count;i++)h->slots[i].pending=h->slots[i].acquired;
 return CAMERA_OK; /* held packet memory still belongs to consumers */
}
int camera_hub_drained(const struct camera_packet_hub *h)
{return valid(h)&&h->stopped&&!acquired(h,(1U<<CAMERA_HUB_CONSUMERS)-1);}
