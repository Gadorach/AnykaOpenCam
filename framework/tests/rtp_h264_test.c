/* SPDX-License-Identifier: GPL-2.0-only; transport fixtures, no hardware. */
#include <camera/rtp_h264.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct received {uint8_t nal[6000];size_t bytes;unsigned packets,markers,starts,ends;uint16_t next;};
static int accept_packet(void *ctx,const uint8_t *p,size_t n)
{
 struct received *r=ctx;assert(n<=1404&&n>=17&&p[0]=='$'&&p[1]==4);
 assert(((unsigned)p[2]<<8|p[3])+4==n&&p[4]==0x80&&(p[5]&127)==96);
 uint16_t seq=(uint16_t)((p[6]<<8)|p[7]);assert(seq==r->next++);
 assert(p[8]==0x12&&p[9]==0x34&&p[10]==0x56&&p[11]==0x78);
 assert(p[12]==0x10&&p[13]==0x20&&p[14]==0x30&&p[15]==0x40);
 size_t offset=16;
 if((p[16]&31)==28){
  if(p[17]&0x80){assert(!r->bytes);r->nal[r->bytes++]=(p[16]&0xe0)|(p[17]&31);r->starts++;}
  if(p[17]&0x40)r->ends++;
  offset=18;
 }
 assert(r->bytes+n-offset<=sizeof(r->nal));memcpy(r->nal+r->bytes,p+offset,n-offset);r->bytes+=n-offset;
 r->packets++;r->markers+=!!(p[5]&128);return 0;
}
static int fail_send(void *ctx,const uint8_t *p,size_t n){(void)ctx;(void)p;(void)n;return -1;}
int main(void)
{
 uint8_t nal[6000];for(unsigned i=0;i<sizeof(nal);i++)nal[i]=i*31;nal[0]=0x65;
 for(size_t n=1;n<=sizeof(nal);n+=n<20?1:137){
  struct camera_rtp_session s={.ssrc=0x10203040,.sequence=65535,.channel=4,.payload_type=96};
  struct received r={.next=65535};
  assert(camera_rtp_h264_nal(&s,0x12345678,nal,n,1,accept_packet,&r)==CAMERA_OK);
  assert(r.bytes==n&&!memcmp(r.nal,nal,n)&&r.markers==1);
  assert(r.starts==(n>1388)&&r.ends==(n>1388)&&s.sequence==r.next);
 }
 struct camera_rtp_session s={.ssrc=0x10203040,.sequence=2,.channel=4,.payload_type=96};
 struct received r={.next=2};assert(camera_rtp_h264_nal(&s,0x12345678,nal,3,0,accept_packet,&r)==CAMERA_OK&&r.markers==0);
 uint16_t old=s.sequence;assert(camera_rtp_h264_nal(&s,0,nal,3,1,fail_send,NULL)==CAMERA_IO_ERROR&&s.sequence==old);
 nal[0]=0x80;assert(camera_rtp_h264_nal(&s,0,nal,3,1,fail_send,NULL)==CAMERA_INVALID);
 nal[0]=28;assert(camera_rtp_h264_nal(&s,0,nal,3,1,fail_send,NULL)==CAMERA_INVALID);
 nal[0]=0x65;
 uint8_t packet[CAMERA_RTP_PACKET_BYTES],again[CAMERA_RTP_PACKET_BYTES];size_t packet_bytes,next,again_bytes,again_next;
 assert(camera_rtp_h264_packet(&s,0x12345678,nal,sizeof(nal),1,0,packet,&packet_bytes,&next)==CAMERA_OK);
 assert(s.sequence==old&&next>1&&next<sizeof(nal)&&(packet[17]&0x80)&&!(packet[5]&0x80));
 assert(camera_rtp_h264_packet(&s,0x12345678,nal,sizeof(nal),1,0,again,&again_bytes,&again_next)==CAMERA_OK);
 assert(packet_bytes==again_bytes&&next==again_next&&!memcmp(packet,again,packet_bytes));
 assert(camera_rtp_h264_packet(&s,0,nal,sizeof(nal),1,sizeof(nal),packet,&packet_bytes,&next)==CAMERA_INVALID);
 assert(camera_rtp_h264_packet(&s,0,nal,3,1,1,packet,&packet_bytes,&next)==CAMERA_INVALID);
 puts("RFC6184 single/FU-A reconstruct/marker/SSRC/timestamp/sequence wrap/error fixtures PASS");return 0;
}
