// SPDX-License-Identifier: GPL-2.0-only
#include <camera/lease_pool.h>
#include <string.h>
static int valid(const struct camera_lease_pool *p,uint32_t i){return p&&p->slots&&p->slots<=CAMERA_POOL_MAX&&i<p->slots;}
int camera_pool_init(struct camera_lease_pool *p,uint32_t n)
{if(!p||n<2||n>CAMERA_POOL_MAX)return CAMERA_INVALID;memset(p,0,sizeof(*p));p->slots=n;return CAMERA_OK;}
int camera_pool_queue(struct camera_lease_pool *p,uint32_t i)
{
 if(!valid(p,i))return CAMERA_INVALID;
 if(p->slot[i].state!=CAMERA_SLOT_FREE)return CAMERA_BUSY;
 if(p->slot[i].generation==UINT64_MAX)return CAMERA_NOT_READY;
 p->slot[i].generation++;p->slot[i].state=CAMERA_SLOT_QUEUED;return CAMERA_OK;
}
int camera_pool_device(struct camera_lease_pool *p,uint32_t i,uint64_t *g)
{if(!valid(p,i)||!g)return CAMERA_INVALID;if(p->slot[i].state!=CAMERA_SLOT_QUEUED)return CAMERA_BUSY;p->slot[i].state=CAMERA_SLOT_DEVICE;*g=p->slot[i].generation;return CAMERA_OK;}
int camera_pool_done(struct camera_lease_pool *p,uint32_t i,uint64_t g)
{if(!valid(p,i))return CAMERA_INVALID;if(p->slot[i].generation!=g)return CAMERA_STALE_LEASE;if(p->slot[i].state!=CAMERA_SLOT_DEVICE)return CAMERA_INVALID;p->slot[i].state=CAMERA_SLOT_DONE;return CAMERA_OK;}
int camera_pool_lease(struct camera_lease_pool *p,uint32_t i,uint64_t *g)
{if(!valid(p,i)||!g)return CAMERA_INVALID;if(p->slot[i].state!=CAMERA_SLOT_DONE)return CAMERA_BUSY;p->slot[i].state=CAMERA_SLOT_LEASED;*g=p->slot[i].generation;return CAMERA_OK;}
int camera_pool_release(struct camera_lease_pool *p,uint32_t i,uint64_t g)
{if(!valid(p,i))return CAMERA_INVALID;if(p->slot[i].generation!=g)return CAMERA_STALE_LEASE;if(p->slot[i].state!=CAMERA_SLOT_LEASED)return CAMERA_INVALID;p->slot[i].state=CAMERA_SLOT_FREE;return CAMERA_OK;}
int camera_pool_stop(struct camera_lease_pool *p,uint32_t retired)
{
 if(!p||!p->slots||p->slots>CAMERA_POOL_MAX)return CAMERA_INVALID;
 if(!retired)return CAMERA_BUSY;
 for(uint32_t i=0;i<p->slots;i++)if(p->slot[i].state!=CAMERA_SLOT_LEASED)p->slot[i].state=CAMERA_SLOT_FREE;
 return CAMERA_OK; /* outstandingleasesstillownstorage; caller mustnotfreeit */
}
uint32_t camera_pool_queued(const struct camera_lease_pool *p)
{if(!p||!p->slots||p->slots>CAMERA_POOL_MAX)return 0;uint32_t n=0;for(uint32_t i=0;i<p->slots;i++)n+=p->slot[i].state==CAMERA_SLOT_QUEUED;return n;}
