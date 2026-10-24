/* SPDX-License-Identifier: GPL-2.0-only
 * Derived from framework/src/rtp_h264.c; see LICENSE_NOTICES.md.
 */
#include "rtp_h264_spans.h"
#include <string.h>

static void be16(uint8_t *p,uint16_t v){p[0]=v>>8;p[1]=v;}
static void be32(uint8_t *p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}

/* Called only after full shape/range validation, with nonzero bytes and
 * offset+bytes <= total. Nonempty spans guarantee progress and bounded index.
 * Only the current packet payload is copied, directly into its owned output.
 */
static void copy_range(const struct camera_rtp_h264_span_v1 *spans,
 size_t offset,size_t bytes,uint8_t *out)
{
 size_t i=0;
 while(offset>=spans[i].size){offset-=spans[i].size;i++;}
 while(bytes){
  size_t take=spans[i].size-offset;
  if(take>bytes)take=bytes;
  memcpy(out,spans[i].data+offset,take);
  out+=take;bytes-=take;offset=0;i++;
 }
}

int camera_rtp_h264_spans_packet_v1(const struct camera_rtp_session *s,
 uint32_t timestamp,const struct camera_rtp_h264_span_v1 *spans,size_t count,
 size_t total,uint32_t last,size_t offset,
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],size_t *packet_bytes,size_t *next_offset)
{
 if(!s||!spans||!count||count>CAMERA_RTP_H264_SPANS_V1_MAX||!total||
    total>CAMERA_RTP_H264_NAL_V1_BYTES||!packet||!packet_bytes||!next_offset)
  return CAMERA_INVALID;
 size_t sum=0;
 for(size_t i=0;i<count;i++){
  if(!spans[i].data||!spans[i].size||
     spans[i].size>CAMERA_RTP_H264_SPAN_V1_BYTES||spans[i].size>SIZE_MAX-sum)
   return CAMERA_INVALID;
  sum+=spans[i].size;
 }
 if(sum!=total||offset>=total||(total<=1388&&offset)||last>1||
    s->channel>254||s->payload_type>127)return CAMERA_INVALID;
 const uint8_t nal_header=spans[0].data[0];
 if((nal_header&0x80)||!(nal_header&31)||(nal_header&31)>=24)
  return CAMERA_INVALID;

 /* No refusal after this point; all output writes follow complete validation. */
 size_t header=16,payload;int end;
 if(total<=1388){payload=total;end=1;}
 else {
  if(!offset)offset=1;
  header=18;payload=total-offset;
  if(payload>CAMERA_RTP_PACKET_BYTES-header)payload=CAMERA_RTP_PACKET_BYTES-header;
  end=payload==total-offset;
  packet[16]=(nal_header&0xe0)|28;
  packet[17]=(nal_header&31)|(offset==1?0x80:0)|(end?0x40:0);
 }
 copy_range(spans,offset,payload,packet+header);
 packet[0]='$';packet[1]=s->channel;be16(packet+2,(uint16_t)(header+payload-4));
 packet[4]=0x80;packet[5]=s->payload_type|(last&&end?0x80:0);
 be16(packet+6,s->sequence);be32(packet+8,timestamp);be32(packet+12,s->ssrc);
 *packet_bytes=header+payload;*next_offset=offset+payload;
 return CAMERA_OK;
}
