/* SPDX-License-Identifier: GPL-2.0-only
 * ONE public synthetic long-NAL observation, no CLI or additional cases.
 * Formula bytes are not footage or a claim of decodable H.264 slice syntax.
 */
#include "rtp_h264_spans.h"
#include <stdio.h>

/* Three distinct objects, not views over a pre-existing full-AU allocation. */
static uint8_t chunk_a[65536],chunk_b[65536],chunk_c[43008];
static uint8_t packet[CAMERA_RTP_PACKET_BYTES];
_Static_assert(sizeof(chunk_a)<=131072&&sizeof(chunk_b)<=131072&&
 sizeof(chunk_c)<=131072,"individual source object cap");

static uint8_t formula(size_t i)
{return i?(uint8_t)((i*29U+(i>>7)+17U)&255U):0x65;}

int main(void)
{
 /* Only initialization writes source bytes; production API receives const spans. */
 for(size_t i=0;i<sizeof(chunk_a);i++)chunk_a[i]=formula(i);
 for(size_t i=0;i<sizeof(chunk_b);i++)chunk_b[i]=formula(sizeof(chunk_a)+i);
 for(size_t i=0;i<sizeof(chunk_c);i++)chunk_c[i]=formula(sizeof(chunk_a)+sizeof(chunk_b)+i);
 const struct camera_rtp_h264_span_v1 spans[]={
  {chunk_a,sizeof(chunk_a)},{chunk_b,sizeof(chunk_b)},{chunk_c,sizeof(chunk_c)}
 };
 const size_t total=174080;
 struct camera_rtp_session s={.ssrc=0x90abcdefU,.sequence=65490,.channel=6,.payload_type=96};
 size_t offset=0,packets=0,checked=0,wire=0,crossings=0;
 while(offset<total){
  const size_t start=offset?offset:1;
  size_t payload=total-start;
  if(payload>1386)payload=1386;
  const int end=start+payload==total;
  const uint16_t sequence=(uint16_t)(65490U+packets);
  const size_t interleaved=payload+14;
  /* Independent fixed RFC6184/RTP expectation; no original builder call. */
  const uint8_t expected[18]={
   '$',6,(uint8_t)(interleaved>>8),(uint8_t)interleaved,
   0x80,(uint8_t)(96|(end?0x80:0)),(uint8_t)(sequence>>8),(uint8_t)sequence,
   0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,
   0x7c,(uint8_t)(5|(start==1?0x80:0)|(end?0x40:0))
  };
  size_t bytes=0,next=0;
  if(camera_rtp_h264_spans_packet_v1(&s,0x12345678U,spans,3,total,1,
     offset,packet,&bytes,&next)!=CAMERA_OK)return 1;
  if(s.ssrc!=0x90abcdefU||s.sequence!=sequence||s.channel!=6||s.payload_type!=96||
     bytes!=18+payload||bytes>CAMERA_RTP_PACKET_BYTES||next!=start+payload)
   return 2;
  for(size_t i=0;i<sizeof(expected);i++)if(packet[i]!=expected[i])return 3;
  for(size_t i=0;i<payload;i++)if(packet[18+i]!=formula(start+i))return 4;
  if(start<65536&&next>65536)crossings++;
  if(start<131072&&next>131072)crossings++;
  checked+=payload;wire+=bytes;packets++;
  /* Entire packet synchronously checked above. No sockets or ACK inference. */
  offset=next;s.sequence++;
 }
 /* FU-A reconstructed header 0x60|5 == original 0x65; every body byte checked. */
 if((uint8_t)((0x7c&0xe0)|5)!=formula(0)||checked+1!=total||offset!=total||
    packets!=126||wire!=176347||crossings!=2||s.sequence!=80)return 5;
 printf("HOST_NORMAL_PASS packets=%zu nal_bytes=%zu payload_bytes=%zu wire_bytes=%zu boundary_crossings=%zu final_offset=%zu final_sequence=%u NOT_NATIVE\n",
  packets,total,checked,wire,crossings,offset,(unsigned)s.sequence);
 return 0;
}
