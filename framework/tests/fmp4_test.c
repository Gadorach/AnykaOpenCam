/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/fmp4.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t nal[CAMERA_FMP4_MAX_NAL_BYTES+1];
static uint32_t read32(const uint8_t *p)
{return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void denied(struct camera_fmp4_sample s)
{
 struct camera_fmp4_fragment out,before;memset(&out,0xa5,sizeof(out));memcpy(&before,&out,sizeof(out));
 assert(camera_fmp4_fragment_prepare(&s,&out)==CAMERA_INVALID);
 assert(!memcmp(&before,&out,sizeof(out)));
}
int main(void)
{
 nal[0]=0x65;
 struct camera_fmp4_sample s={nal,137004,0,45720,1,1};
 struct camera_fmp4_fragment out;
 assert(camera_fmp4_fragment_prepare(&s,&out)==CAMERA_OK);
 assert(out.nal==nal&&out.nal_bytes==137004&&sizeof(out.header)==112);
 assert(read32(out.header)==100&&!memcmp(out.header+4,"moof",4));
 assert(read32(out.header+20)==1&&read32(out.header+40)==0x20000);
 assert(read32(out.header+56)==0x1000000&&read32(out.header+60)==0&&read32(out.header+64)==0);
 assert(read32(out.header+84)==108&&read32(out.header+88)==45720&&read32(out.header+92)==137008);
 assert(read32(out.header+96)==0x2000000&&read32(out.header+100)==137016&&read32(out.header+108)==137004);
 size_t offset=0;const uint8_t *data=NULL;size_t bytes=0;
 while(offset<112+s.nal_bytes){
  assert(camera_fmp4_fragment_span(&out,offset,8192,&data,&bytes)==CAMERA_OK);
  assert(bytes&&bytes<=8192);
  assert(data==(offset<112?out.header+offset:nal+offset-112));
  /* A short actual write advances only its own accepted bytes. */
  offset+=bytes>1?bytes/2:1;
 }
 assert(camera_fmp4_fragment_span(&out,offset,8192,&data,&bytes)==CAMERA_OK&&bytes==0);
 data=nal;bytes=99;
 assert(camera_fmp4_fragment_span(&out,SIZE_MAX,8192,&data,&bytes)==CAMERA_INVALID&&data==nal&&bytes==99);
 assert(camera_fmp4_fragment_span(&out,0,8193,&data,&bytes)==CAMERA_INVALID&&bytes==99);
 assert(camera_fmp4_fragment_span(&out,0,0,&data,&bytes)==CAMERA_INVALID&&bytes==99);
 nal[0]=0x41;s.keyframe=0;s.sequence=8;s.decode_ticks=UINT64_C(0x100000123);
 assert(camera_fmp4_fragment_prepare(&s,&out)==CAMERA_OK);
 assert(read32(out.header+60)==1&&read32(out.header+64)==0x123&&read32(out.header+96)==0x1010000);
 struct camera_fmp4_sample bad=s;
 bad.nal=NULL;denied(bad);bad=s;bad.nal_bytes=0;denied(bad);
 bad=s;bad.nal_bytes=CAMERA_FMP4_MAX_NAL_BYTES+1;denied(bad);
 bad=s;bad.sequence=0;denied(bad);bad=s;bad.sequence=1;denied(bad);
 bad=s;bad.duration_ticks=0;denied(bad);bad=s;bad.duration_ticks=0x80000000U;denied(bad);
 bad=s;bad.decode_ticks=UINT64_MAX;denied(bad);bad=s;bad.keyframe=2;denied(bad);
 bad=s;bad.keyframe=1;denied(bad);nal[0]=0xc1;denied(s);nal[0]=0x67;denied(s);nal[0]=0;
 denied(s);nal[0]=0x41;
 memset(&out,0x41,sizeof(out));s.nal=out.header;
 assert(camera_fmp4_fragment_prepare(&s,&out)==CAMERA_INVALID);
 assert(out.header[0]==0x41);
 assert(camera_fmp4_fragment_prepare(NULL,&out)==CAMERA_INVALID);
 assert(camera_fmp4_fragment_prepare(&s,NULL)==CAMERA_INVALID);
 puts("fMP4 fragment112/borrowed-native-sized-payload/64bit-timing/atomic-denials PASS; not live/auth/lease proof");
 return 0;
}
