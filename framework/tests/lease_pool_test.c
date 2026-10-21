/* SPDX-License-Identifier: GPL-2.0-only; simulatedownership, notDMAproof. */
#include <camera/lease_pool.h>
#include <assert.h>
#include <stdio.h>
int main(void)
{
 struct camera_lease_pool a,b;assert(camera_pool_init(&a,4)==CAMERA_OK);assert(camera_pool_init(&b,4)==CAMERA_OK);
 for(unsigned i=0;i<4;i++)assert(camera_pool_queue(&a,i)==CAMERA_OK);
 uint64_t g,old;assert(camera_pool_device(&a,0,&g)==CAMERA_OK);
 assert(camera_pool_lease(&a,0,&old)==CAMERA_BUSY);assert(camera_pool_queue(&a,0)==CAMERA_BUSY);
 assert(camera_pool_done(&a,0,g)==CAMERA_OK);assert(camera_pool_lease(&a,0,&old)==CAMERA_OK);
 assert(camera_pool_queue(&a,0)==CAMERA_BUSY);assert(camera_pool_stop(&a,0)==CAMERA_BUSY);
 assert(camera_pool_stop(&a,1)==CAMERA_OK&&a.slot[0].state==CAMERA_SLOT_LEASED);
 assert(camera_pool_release(&a,0,old)==CAMERA_OK);assert(camera_pool_queue(&a,0)==CAMERA_OK);
 assert(camera_pool_release(&a,0,old)==CAMERA_STALE_LEASE);
 assert(camera_pool_device(&a,0,&g)==CAMERA_OK&&g!=old);
 assert(camera_pool_done(&a,0,old)==CAMERA_STALE_LEASE);
 assert(camera_pool_queued(&b)==0);assert(camera_pool_queue(&b,0)==CAMERA_OK);
 assert(camera_pool_queued(&b)==1&&camera_pool_queued(&a)==0);
 puts("Leasepool device/consumer/stop/stalegeneration/twocamera isolation fixtures PASS");return 0;
}
