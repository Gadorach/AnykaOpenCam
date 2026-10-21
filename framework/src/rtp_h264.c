// SPDX-License-Identifier: GPL-2.0-only
#include <camera/rtp_h264.h>
#include <string.h>
static void be16(uint8_t *p,uint16_t v){p[0]=v>>8;p[1]=v;}
static void be32(uint8_t *p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int camera_rtp_h264_packet(const struct camera_rtp_session *s,uint32_t timestamp,
 const uint8_t *nal,size_t bytes,uint32_t last,size_t offset,
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],size_t *packet_bytes,size_t *next_offset)
{
 if(!s||!nal||!bytes||!packet||!packet_bytes||!next_offset||offset>=bytes||
    (bytes<=1388&&offset)||last>1||s->channel>254||s->payload_type>127||
     (nal[0]&0x80)||!(nal[0]&31)||(nal[0]&31)>=24)return CAMERA_INVALID;
 if(bytes>1388&&!offset)offset=1;
  size_t payload,header=16;int end;
  if(bytes<=1388){payload=bytes;end=1;memcpy(packet+header,nal,bytes);}
  else {
    header+=2;payload=bytes-offset;if(payload>CAMERA_RTP_PACKET_BYTES-header)payload=CAMERA_RTP_PACKET_BYTES-header;
   end=payload==bytes-offset;packet[16]=(nal[0]&0xe0)|28;
   packet[17]=(nal[0]&31)|(offset==1?0x80:0)|(end?0x40:0);
   memcpy(packet+header,nal+offset,payload);
  }
  packet[0]='$';packet[1]=s->channel;be16(packet+2,(uint16_t)(header+payload-4));
  packet[4]=0x80;packet[5]=s->payload_type|(last&&end?0x80:0);
  be16(packet+6,s->sequence);be32(packet+8,timestamp);be32(packet+12,s->ssrc);
 *packet_bytes=header+payload;*next_offset=offset+payload;return CAMERA_OK;
}
int camera_rtp_h264_nal(struct camera_rtp_session *s,uint32_t timestamp,
 const uint8_t *nal,size_t bytes,uint32_t last,camera_rtp_send send,void *context)
{
 uint8_t packet[CAMERA_RTP_PACKET_BYTES];size_t offset=0,n,next;
 if(!send)return CAMERA_INVALID;
 do {
  int result=camera_rtp_h264_packet(s,timestamp,nal,bytes,last,offset,packet,&n,&next);
  if(result!=CAMERA_OK)return result;
  if(send(context,packet,n))return CAMERA_IO_ERROR;
  s->sequence++;offset=next;
 }while(offset<bytes);
 return CAMERA_OK;
}
