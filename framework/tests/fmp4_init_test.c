/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/fmp4.h>
#include "../../bringup/qc07f/userland/h264-baseline.h"
#include <assert.h>
#include <stdio.h>

static uint8_t sets[256],sps[256],pps[256];
static size_t ns,np;
static void parameters(void)
{
 size_t n=qc_h264_parameter_sets(sets),split=0;
 for(size_t i=5;i+4<n;i++)if(!sets[i]&&!sets[i+1]&&!sets[i+2]&&sets[i+3]==1){split=i;break;}
 assert(split);ns=split-4;np=n-split-4;memcpy(sps,sets+4,ns);memcpy(pps,sets+split+4,np);
}
static void denied(struct camera_fmp4_config c,int result)
{
 struct camera_fmp4_init out,before;memset(&out,0xa5,sizeof(out));memcpy(&before,&out,sizeof(out));
 assert(camera_fmp4_init_prepare(&c,&out)==result);assert(!memcmp(&out,&before,sizeof(out)));
}
int main(void)
{
 parameters();struct camera_fmp4_profile p={0};
 assert(camera_fmp4_profile_read(sps,ns,pps,np,&p)==CAMERA_OK);
 assert(p.width==1280&&p.height==720&&p.profile==66&&p.compatibility==0xe0&&p.level==31&&p.sps_id==0&&p.pps_id==0);
 struct camera_fmp4_config c={sps,pps,ns,np,42,450720};struct camera_fmp4_init out;
 assert(camera_fmp4_init_prepare(&c,&out)==CAMERA_OK&&out.bytes==644&&out.generation==42);
 assert(!memcmp(out.data+4,"ftyp",4)&&!memcmp(out.data+32,"moov",4));
 c.duration_ticks=0;assert(camera_fmp4_init_prepare(&c,&out)==CAMERA_OK&&out.bytes==644);
 struct camera_fmp4_config bad=c;bad.generation=0;denied(bad,CAMERA_INVALID);
 bad=c;bad.sps=NULL;denied(bad,CAMERA_INVALID);bad=c;bad.sps_bytes=0;denied(bad,CAMERA_INVALID);
 bad=c;bad.sps_bytes=257;denied(bad,CAMERA_INVALID);bad=c;bad.pps_bytes=257;denied(bad,CAMERA_INVALID);
 bad=c;bad.sps_bytes=ns-1;denied(bad,CAMERA_INVALID);
 bad=c;bad.pps_bytes=np-1;denied(bad,CAMERA_INVALID);
 sps[1]=100;denied(c,CAMERA_UNSUPPORTED);parameters();sps[0]|=0x80;denied(c,CAMERA_INVALID);parameters();
 pps[0]=0x67;denied(c,CAMERA_INVALID);parameters();
 /* Every single-bit mutation must either validate as this narrow syntax subset
  * or leave the entire output unchanged; sanitizer checks every bounded read. */
 for(size_t i=0;i<ns;i++)for(unsigned bit=0;bit<8;bit++){
  struct camera_fmp4_init before;memset(&out,0x5a,sizeof(out));memcpy(&before,&out,sizeof(out));sps[i]^=(uint8_t)(1U<<bit);
  int r=camera_fmp4_init_prepare(&c,&out);assert(r==CAMERA_OK||r==CAMERA_INVALID||r==CAMERA_UNSUPPORTED);
  if(r!=CAMERA_OK)assert(!memcmp(&out,&before,sizeof(out)));
  sps[i]^=(uint8_t)(1U<<bit);
 }
 memset(&out,0x67,sizeof(out));bad=c;bad.sps=out.data;bad.sps_bytes=ns;
 assert(camera_fmp4_init_prepare(&bad,&out)==CAMERA_INVALID&&out.data[0]==0x67);
 assert(camera_fmp4_init_prepare(NULL,&out)==CAMERA_INVALID);
 assert(camera_fmp4_init_prepare(&c,NULL)==CAMERA_INVALID);
 puts("fMP4 baselineSPS/PPS geometry/atomic bounded init/live-duration/mutations PASS; not hardware capability proof");return 0;
}
