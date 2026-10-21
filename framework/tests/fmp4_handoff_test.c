/* SPDX-License-Identifier: GPL-2.0-only */
/* Source/hub order model only: no device, real DMA, TLS or authentication proof.
 * Reuses the existing producer buffer + one hub slot; no per-peer AU copies. */
#include <camera/fmp4.h>
#include <camera/packet_hub.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t producer_data[CAMERA_FMP4_MAX_NAL_BYTES];
static uint8_t hub_data[CAMERA_FMP4_MAX_NAL_BYTES];
static unsigned outstanding,produced,released;
static uint8_t pattern(uint64_t time){return (uint8_t)(time/1000000%251);}
static struct camera_packet next(uint64_t time,uint32_t keyframe)
{
 assert(!outstanding);outstanding=1;produced++;
 memset(producer_data,pattern(time),137004);producer_data[0]=keyframe?0x65:0x41;
 return (struct camera_packet){.codec=CAMERA_H264,.keyframe=keyframe,
  .data=producer_data,.bytes=137004,.monotonic_ns=time};
}
static void release(void)
{assert(outstanding);memset(producer_data,0xcc,137004);outstanding=0;released++;}
static void finish(struct camera_packet_hub *h,struct camera_packet *old,
                   struct camera_packet *future,struct camera_fmp4_timeline *time)
{
 struct camera_fmp4_fragment fragment;
 uint32_t a=(uint32_t)(old->monotonic_ns/1000000*90),b=(uint32_t)(future->monotonic_ns/1000000*90);
 assert(camera_fmp4_timeline_step(time,7,a,b,old->data,old->bytes,old->keyframe,&fragment)==CAMERA_OK);
 assert(fragment.nal==hub_data&&fragment.nal!=producer_data);
 assert(fragment.nal[1]==pattern(old->monotonic_ns)); /* fixture pattern encodes identity */
 assert(future->data[1]==pattern(future->monotonic_ns));
 /* Model already-authorized complete delivery in bounded chunks. Actual auth/
  * expiry/health and partial TLS cleanup are separately mandatory at each send. */
 size_t consumed=0;
 while(consumed<fragment.nal_bytes){size_t n=fragment.nal_bytes-consumed;if(n>8192)n=8192;consumed+=n;}
 assert(consumed==old->bytes);
 assert(camera_hub_release(h,0,old->lease_token,old->generation)==CAMERA_OK);
 assert(camera_hub_publish(h,future)==CAMERA_OK);release();
}
int main(void)
{
 struct camera_packet_hub h;struct camera_fmp4_timeline time;
 assert(camera_hub_init_count(&h,hub_data,sizeof(hub_data),1)==CAMERA_OK);
 assert(camera_hub_subscribe(&h,0)==CAMERA_OK);
 uint64_t t0=1000000000,t1=1508000000,t2=1943000000;
 struct camera_packet first=next(t0,1);
 assert(camera_hub_publish(&h,&first)==CAMERA_OK);release();
 struct camera_packet old;assert(camera_hub_acquire(&h,0,&old)==CAMERA_OK);
 assert(camera_fmp4_timeline_init(&time,7,(uint32_t)(t0/1000000*90))==CAMERA_OK);
 /* Old hub storage is held, but no producer storage is held. Next acquisition
  * therefore proceeds without trying to publish over the acquired hub slot. */
 struct camera_packet future=next(t1,0);assert(outstanding&&produced==2);
 assert(old.data!=future.data&&old.data[1]==pattern(t0));
 finish(&h,&old,&future,&time);
 assert(camera_hub_acquire(&h,0,&old)==CAMERA_OK);
 future=next(t2,0);finish(&h,&old,&future,&time);
 assert(produced==3&&released==3&&!outstanding&&time.decode_ticks==84870);
 assert(camera_hub_acquire(&h,0,&old)==CAMERA_OK);
 /* EOS/source failure has no known next timestamp: don't invent duration. Drop
  * the owned whole pending AU and retire it; don't export a partial sample. */
 assert(camera_hub_release(&h,0,old.lease_token,old.generation)==CAMERA_OK);
 assert(camera_hub_unsubscribe(&h,0)==CAMERA_OK&&camera_hub_stop(&h)==CAMERA_OK&&camera_hub_drained(&h));
 assert(!outstanding);
 /* Disconnect/stale generation while next producer CPU output is pending.
  * STOP preserves acquired hub bytes until explicit release; pending producer
  * storage must also be retired, not published after the receiver is gone. */
 struct camera_packet_hub stopped;
 assert(camera_hub_init_count(&stopped,hub_data,sizeof(hub_data),1)==CAMERA_OK);
 assert(camera_hub_subscribe(&stopped,0)==CAMERA_OK);
 first=next(t0,1);assert(camera_hub_publish(&stopped,&first)==CAMERA_OK);release();
 assert(camera_hub_acquire(&stopped,0,&old)==CAMERA_OK);future=next(t1,0);
 assert(camera_fmp4_timeline_init(&time,8,(uint32_t)(t0/1000000*90))==CAMERA_OK);
 struct camera_fmp4_fragment denied;
 assert(camera_fmp4_timeline_step(&time,7,(uint32_t)(t0/1000000*90),
  (uint32_t)(t1/1000000*90),old.data,old.bytes,1,&denied)==CAMERA_STALE_LEASE);
 assert(camera_hub_stop(&stopped)==CAMERA_OK&&!camera_hub_drained(&stopped));
 release();assert(old.data[1]==pattern(t0));
 assert(camera_hub_release(&stopped,0,old.lease_token,old.generation)==CAMERA_OK&&camera_hub_drained(&stopped));
 assert(camera_hub_unsubscribe(&stopped,0)==CAMERA_OK&&!outstanding&&produced==released);
 puts("fMP4 lookahead using existing producer+onehub storage/no acquired-slot overwrite/no producer-held deadlock/EOS retirement PASS; model only");
 return 0;
}
