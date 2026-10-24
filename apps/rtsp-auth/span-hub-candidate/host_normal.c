/* SPDX-License-Identifier: GPL-2.0-only
 * ONE normal numeric synthetic NAL, four consumers, no CLI/additional cases.
 * Not decodable slice syntax, footage, native media or driver-release evidence.
 */
#include "span_hub.h"
#include <stdio.h>

static uint8_t source_a[65536],source_b[65536],source_c[43008];
static struct camera_span_hub_v1 hub;
struct cursor {
 struct camera_span_hub_view_v1 view;
 struct camera_rtp_session session;
 size_t offset,packets,checked,wire;
 uint8_t packet[CAMERA_RTP_PACKET_BYTES];
};
static struct cursor cursors[CAMERA_SPAN_HUB_V1_CONSUMERS];
_Static_assert(sizeof(source_a)<=65536&&sizeof(source_b)<=65536&&
 sizeof(source_c)<=65536,"three separately bounded source objects");
_Static_assert(sizeof(struct cursor)<4096,"bounded per-client packet cursor");

static uint8_t formula(size_t i)
{return i?(uint8_t)((i*29U+(i>>7)+17U)&255U):0x65;}

/* Volatile stores make this actual synthetic-source wipe, not a dead memset.
 * It proves neither real DMA release nor secure erasure of any private data. */
static void wipe(uint8_t *data,size_t bytes)
{volatile uint8_t *p=data;for(size_t i=0;i<bytes;i++)p[i]=0;}

int main(void)
{
 const size_t total=174080;
 for(size_t i=0;i<sizeof(source_a);i++)source_a[i]=formula(i);
 for(size_t i=0;i<sizeof(source_b);i++)source_b[i]=formula(sizeof(source_a)+i);
 for(size_t i=0;i<sizeof(source_c);i++)source_c[i]=formula(sizeof(source_a)+sizeof(source_b)+i);
 const struct camera_rtp_h264_span_v1 sources[]={
  {source_a,sizeof(source_a)},{source_b,sizeof(source_b)},{source_c,sizeof(source_c)}
 };
 const struct camera_span_hub_input_v1 input={.codec=CAMERA_H264,.keyframe=1,
  .monotonic_ns=123456789000ULL,.bytes=total,.span_count=3,.spans=sources};
 if(camera_span_hub_init_v1(&hub)!=CAMERA_OK)return 1;
 const uint32_t allocations=hub.allocation_calls;
 const size_t allocated=hub.allocated_bytes,maximum=hub.maximum_allocation;
 if(allocations!=8||allocated!=1048576||maximum!=131072)return 2;
 for(size_t i=0;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++){
  if(!hub.chunks[i])return 3;
  for(size_t j=0;j<i;j++)if(hub.chunks[i]==hub.chunks[j])return 3;
 }
 for(uint32_t c=0;c<CAMERA_SPAN_HUB_V1_CONSUMERS;c++)
  if(camera_span_hub_subscribe_v1(&hub,c)!=CAMERA_OK)return 4;
 if(camera_span_hub_publish_v1(&hub,&input)!=CAMERA_OK)return 5;
 /* Original storage may now be overwritten ONLY because this harness owns these
  * numeric CPU objects. Real producer/DMA release still needs its real contract. */
 wipe(source_a,sizeof(source_a));wipe(source_b,sizeof(source_b));wipe(source_c,sizeof(source_c));
 for(uint32_t c=0;c<CAMERA_SPAN_HUB_V1_CONSUMERS;c++){
  struct cursor *p=&cursors[c];
  if(camera_span_hub_acquire_v1(&hub,c,&p->view)!=CAMERA_OK)return 6;
  if(p->view.owner!=&hub||p->view.lease_token!=1||p->view.generation!=1||
     p->view.codec!=CAMERA_H264||p->view.keyframe!=1||
     p->view.monotonic_ns!=123456789000ULL||p->view.bytes!=total||
     p->view.span_count!=2||p->view.spans[0].data!=hub.chunks[0]||
     p->view.spans[0].size!=131072||p->view.spans[1].data!=hub.chunks[1]||
     p->view.spans[1].size!=43008)return 7;
  for(size_t i=2;i<CAMERA_SPAN_HUB_V1_CHUNKS;i++)
   if(p->view.spans[i].data||p->view.spans[i].size)return 7;
  if(c&&(p->view.spans[0].data!=cursors[0].view.spans[0].data||
         p->view.spans[1].data!=cursors[0].view.spans[1].data))return 7;
  p->session=(struct camera_rtp_session){.ssrc=0x90abcd00U+c,
   .sequence=(uint16_t)(1000U+256U*c),.channel=(uint8_t)(2U*c),.payload_type=96};
 }
 size_t packets=0,checked=0,wire=0;
 /* Fair synchronous packet checks; each consumer owns only one 1404B packet.
  * All four acquired leases stay held throughout packetization. */
 while(cursors[0].offset<total){
  for(uint32_t c=0;c<CAMERA_SPAN_HUB_V1_CONSUMERS;c++){
   struct cursor *p=&cursors[c];
   const size_t start=p->offset?p->offset:1;
   size_t payload=total-start;
   if(payload>1386)payload=1386;
   const int end=start+payload==total;
   const uint16_t sequence=(uint16_t)(1000U+256U*c+p->packets);
   const size_t interleaved=payload+14;
   const uint8_t expected[18]={
    '$',(uint8_t)(2U*c),(uint8_t)(interleaved>>8),(uint8_t)interleaved,
    0x80,(uint8_t)(96|(end?0x80:0)),(uint8_t)(sequence>>8),(uint8_t)sequence,
    0x12,0x34,0x56,0x78,0x90,0xab,0xcd,(uint8_t)c,
    0x7c,(uint8_t)(5|(start==1?0x80:0)|(end?0x40:0))
   };
   size_t bytes=0,next=0;
   if(!camera_span_hub_lease_valid_v1(&hub,c,&p->view))return 8;
   /* Wrapper independently rechecks this exact view before the actual accepted
    * camera_rtp_h264_spans_packet_v1 production call; no old packetizer. */
   if(camera_span_hub_packet_v1(&hub,c,&p->view,&p->session,0x12345678U,1,
      p->offset,p->packet,&bytes,&next)!=CAMERA_OK)return 9;
   /* This synchronous formula check is the only consumption effect here; no
    * network send/partial writes/ACK or real clock/driver authority is claimed. */
   if(!camera_span_hub_lease_valid_v1(&hub,c,&p->view))return 10;
   if(p->session.ssrc!=0x90abcd00U+c||p->session.sequence!=sequence||
      p->session.channel!=2U*c||p->session.payload_type!=96||
      bytes!=18+payload||bytes>sizeof(p->packet)||next!=start+payload)return 11;
   for(size_t i=0;i<sizeof(expected);i++)if(p->packet[i]!=expected[i])return 12;
   for(size_t i=0;i<payload;i++)if(p->packet[18+i]!=formula(start+i))return 13;
   p->checked+=payload;p->wire+=bytes;p->packets++;
   /* Commit only AFTER the entire packet is checked synchronously. */
   p->offset=next;p->session.sequence++;
  }
 }
 for(uint32_t c=0;c<CAMERA_SPAN_HUB_V1_CONSUMERS;c++){
  struct cursor *p=&cursors[c];
  if(p->offset!=total||p->packets!=126||p->checked!=174079||p->wire!=176347||
     p->session.sequence!=1126U+256U*c)return 14;
  packets+=p->packets;checked+=p->checked;wire+=p->wire;
  if(camera_span_hub_release_v1(&hub,c,p->view.lease_token,p->view.generation)!=CAMERA_OK)
   return 15;
 }
 if(camera_span_hub_stop_v1(&hub)!=CAMERA_OK)return 16;
 const int drained=camera_span_hub_drained_v1(&hub);
 if(!drained)return 17;
 const int disposed=camera_span_hub_dispose_v1(&hub)==CAMERA_OK;
 if(!disposed)return 18;
 /* No owner, chunk or borrowed view is read or called after dispose. */
 printf("HOST_NORMAL_PASS allocations=%u total_requested=%zu max_request=%zu hub_bytes=%zu view_bytes=%zu cursor_bytes=%zu clients=4 packets=%zu nal_bytes=%zu checked_body_bytes=%zu wire_bytes=%zu drained=%d disposed=%d NOT_NATIVE\n",
  (unsigned)allocations,allocated,maximum,sizeof(struct camera_span_hub_v1),
  sizeof(struct camera_span_hub_view_v1),sizeof(struct cursor),packets,total,
  checked,wire,drained,disposed);
 return 0;
}
