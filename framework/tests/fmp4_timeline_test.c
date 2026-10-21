/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/fmp4.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t read32(const uint8_t *p)
{return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void deny(struct camera_fmp4_timeline *t,uint64_t gen,uint32_t now,uint32_t next,int want)
{
 const uint8_t nal[2]={0x41,0x80};struct camera_fmp4_timeline before=*t;
 struct camera_fmp4_fragment out,old;memset(&out,0xa5,sizeof(out));memcpy(&old,&out,sizeof(out));
 assert(camera_fmp4_timeline_step(t,gen,now,next,nal,2,0,&out)==want);
 assert(!memcmp(t,&before,sizeof(*t))&&!memcmp(&out,&old,sizeof(out)));
}
int main(void)
{
 struct camera_fmp4_timeline t;struct camera_fmp4_fragment f;uint8_t nal[2]={0x65,0x80};
 assert(camera_fmp4_timeline_init(&t,4,0xfffffff0U)==CAMERA_OK);
 assert(camera_fmp4_timeline_step(&t,4,0xfffffff0U,0x20,nal,2,1,&f)==CAMERA_OK);
 assert(read32(f.header+88)==48&&read32(f.header+20)==1&&t.decode_ticks==48&&t.expected_rtp==0x20);
 nal[0]=0x41;
 assert(camera_fmp4_timeline_step(&t,4,0x20,0x50,nal,2,0,&f)==CAMERA_OK);
 assert(read32(f.header+64)==48&&t.decode_ticks==96);
 deny(&t,3,0x50,0x70,CAMERA_STALE_LEASE);deny(&t,4,0x20,0x50,CAMERA_STALE_LEASE);
 deny(&t,4,0x50,0x50,CAMERA_INVALID);deny(&t,4,0x50,0x40,CAMERA_INVALID);
 deny(&t,4,0x50,0x80000050U,CAMERA_INVALID);
 t.decode_ticks=UINT64_MAX-15;deny(&t,4,0x50,0x70,CAMERA_INVALID);
 t.decode_ticks=96;t.next_sequence=UINT32_MAX;deny(&t,4,0x50,0x70,CAMERA_NOT_READY);
 assert(camera_fmp4_timeline_init(&t,5,100)==CAMERA_OK);
 deny(&t,5,100,200,CAMERA_INVALID); /* restart requires IDR */
 nal[0]=0x65;assert(camera_fmp4_timeline_step(&t,5,100,200,nal,2,1,&f)==CAMERA_OK);
 assert(read32(f.header+64)==0&&read32(f.header+20)==1);
 assert(camera_fmp4_timeline_init(&t,0,100)==CAMERA_INVALID);
 puts("fMP4 measuredRTP wrap/relative64bitDTS/generation/IDR/atomic clock and overflow denials PASS");return 0;
}
