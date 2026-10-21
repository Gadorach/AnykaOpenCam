/* SPDX-License-Identifier: GPL-2.0-only; host fan-out fixtures, not live video. */
#include <camera/packet_hub.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void release(struct camera_packet_hub *h,unsigned c,struct camera_packet *p)
{assert(camera_hub_release(h,c,p->lease_token,p->generation)==CAMERA_OK);}
int main(void)
{
 uint8_t storage[4*32],data[8]={1,2,3,4};struct camera_packet_hub h;
 struct camera_packet p={.codec=CAMERA_H264,.keyframe=0,.data=data,.bytes=8},a,b,old;
 assert(camera_hub_init(&h,storage,32)==CAMERA_OK);
 assert(!camera_hub_drained(&h)); /* Only a stopped hub can release storage. */
 assert(camera_hub_subscribe(&h,0)==CAMERA_OK&&camera_hub_subscribe(&h,1)==CAMERA_OK);
 assert(camera_hub_publish(&h,&p)==CAMERA_OK&&camera_hub_acquire(&h,0,&a)==CAMERA_NOT_READY);
 p.keyframe=1;assert(camera_hub_publish(&h,&p)==CAMERA_OK);
 assert(camera_hub_acquire(&h,0,&a)==CAMERA_OK&&camera_hub_acquire(&h,1,&b)==CAMERA_OK);
 assert(a.data==b.data&&!memcmp(a.data,data,8));old=a;
 assert(camera_hub_unsubscribe(&h,0)==CAMERA_BUSY);release(&h,0,&a);release(&h,1,&b);
 p.keyframe=0;data[0]=17;assert(camera_hub_publish(&h,&p)==CAMERA_OK);
 assert(camera_hub_release(&h,0,old.lease_token,old.generation)==CAMERA_STALE_LEASE);
 assert(camera_hub_acquire(&h,0,&a)==CAMERA_OK);release(&h,0,&a);
 /* Slow consumer1 loses backlog; consumer0 remains continuous. */
 for(unsigned i=0;i<8;i++){
  data[0]++;assert(camera_hub_publish(&h,&p)==CAMERA_OK);
  assert(camera_hub_acquire(&h,0,&a)==CAMERA_OK&&a.data[0]==data[0]);release(&h,0,&a);
 }
 assert(camera_hub_acquire(&h,1,&b)==CAMERA_NOT_READY);
 p.keyframe=1;assert(camera_hub_publish(&h,&p)==CAMERA_OK);
 assert(camera_hub_acquire(&h,0,&a)==CAMERA_OK&&camera_hub_acquire(&h,1,&b)==CAMERA_OK);
 uint8_t held[8];memcpy(held,b.data,8);release(&h,0,&a);p.keyframe=0;
 for(unsigned i=0;i<8;i++){
  data[0]++;assert(camera_hub_publish(&h,&p)==CAMERA_OK);
  assert(camera_hub_acquire(&h,0,&a)==CAMERA_OK);release(&h,0,&a);
 }
 assert(!memcmp(held,b.data,8));
 assert(camera_hub_stop(&h)==CAMERA_OK&&!camera_hub_drained(&h));
 assert(camera_hub_acquire(&h,0,&a)==CAMERA_NOT_READY);release(&h,1,&b);
 assert(camera_hub_drained(&h)&&camera_hub_unsubscribe(&h,1)==CAMERA_OK);
 /* Four different acquired slots cannot be reclaimed, even for a new IDR. */
 struct camera_packet_hub full;uint8_t full_storage[4*32];struct camera_packet leases[4];
 assert(camera_hub_init(&full,full_storage,32)==CAMERA_OK);p.keyframe=1;
 for(unsigned i=0;i<4;i++){
  assert(camera_hub_subscribe(&full,i)==CAMERA_OK);
  data[0]=(uint8_t)(50+i);assert(camera_hub_publish(&full,&p)==CAMERA_OK);
  assert(camera_hub_acquire(&full,i,&leases[i])==CAMERA_OK);
 }
 data[0]=99;assert(camera_hub_publish(&full,&p)==CAMERA_BUSY);
 for(unsigned i=0;i<4;i++)assert(leases[i].data[0]==50+i);
 for(unsigned i=0;i<4;i++)release(&full,i,&leases[i]);
 p.keyframe=0;assert(camera_hub_publish(&full,&p)==CAMERA_OK);
 for(unsigned i=0;i<4;i++)assert(camera_hub_acquire(&full,i,&a)==CAMERA_NOT_READY);
 p.keyframe=1;assert(camera_hub_publish(&full,&p)==CAMERA_OK);
 for(unsigned i=0;i<4;i++){assert(camera_hub_acquire(&full,i,&a)==CAMERA_OK&&a.data[0]==99);release(&full,i,&a);}
 assert(camera_hub_stop(&full)==CAMERA_OK&&camera_hub_drained(&full));
 struct camera_packet_hub single;uint8_t one_storage[32];
 assert(camera_hub_init_count(&single,one_storage,32,0)==CAMERA_INVALID);
 assert(camera_hub_init_count(&single,one_storage,32,5)==CAMERA_INVALID);
 assert(camera_hub_init_count(&single,one_storage,32,1)==CAMERA_OK);
 assert(camera_hub_subscribe(&single,0)==CAMERA_OK&&camera_hub_subscribe(&single,1)==CAMERA_OK);
 p.keyframe=1;assert(camera_hub_publish(&single,&p)==CAMERA_OK);
 assert(camera_hub_acquire(&single,0,&a)==CAMERA_OK&&camera_hub_acquire(&single,1,&b)==CAMERA_OK);
 uint8_t original=a.data[0];data[0]^=1;
 assert(camera_hub_publish(&single,&p)==CAMERA_BUSY&&a.data[0]==original&&b.data[0]==original);
 assert(camera_hub_release(&single,0,2,a.generation)==CAMERA_INVALID);
 release(&single,0,&a);release(&single,1,&b);
 assert(camera_hub_publish(&single,&p)==CAMERA_OK);
 assert(camera_hub_acquire(&single,0,&a)==CAMERA_OK&&a.data[0]==data[0]);release(&single,0,&a);
 assert(camera_hub_acquire(&single,1,&b)==CAMERA_OK);release(&single,1,&b);
 assert(camera_hub_stop(&single)==CAMERA_OK&&camera_hub_drained(&single));
 puts("Packet hub shared payload/IDR join/slow consumer/stale lease/held memory/stop fixtures PASS");return 0;
}
