/* SPDX-License-Identifier: GPL-2.0-only; host numeric fixtures, no hardware. */
#include "../../bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-whitebalance.h"
#include "../../bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-stream.h"
#include "../../bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-reference.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const __u32 mask[3]={0x0fff0fffU,0x03ff0fffU,0x03ff03ffU};
static void check(const __u32 original[3],const __u32 gain[3],const __u32 offset[3])
{
 __u32 words[3],g[3],o[3];
 memcpy(words,original,sizeof(words));
 assert(!qc_isp_wb_manual(words,gain,offset));
 qc_isp_wb_fields(words,g,o);
 assert(!memcmp(g,gain,sizeof(g)) && !memcmp(o,offset,sizeof(o)));
 for(unsigned i=0;i<3;i++)assert((words[i]&~mask[i])==(original[i]&~mask[i]));
}
static void invalid(const __u32 gain[3],const __u32 offset[3])
{
 __u32 words[3]={0xdeadbeef,0x01234567,0x89abcdef},before[3];
 memcpy(before,words,sizeof(words));
 assert(qc_isp_wb_manual(words,gain,offset)==-1);
 assert(!memcmp(words,before,sizeof(words)));
}
int main(void)
{
 __u32 gain[3]={396,258,498},offset[3]={0},words[3],g[3],o[3];
 __u32 all[3]={0xffffffffU,0xffffffffU,0xffffffffU};
 _Static_assert(sizeof(struct qc_isp_whitebalance_manual)==96,"stable ioctl size");
 _Static_assert(offsetof(struct qc_isp_whitebalance_manual,desired_changed_ns)==80,"stable timestamp alignment");
 _Static_assert(_IOC_SIZE(QC_ISP_WHITEBALANCE_MANUAL)==96,"ioctl size");
 assert(QC_ISP_WHITEBALANCE_MANUAL!=QC_ISP_WHITEBALANCE);
 memcpy(words,&qc07f_isp_reference[0xd34/4],sizeof(words));
 qc_isp_wb_fields(words,g,o);
 assert(!memcmp(g,gain,sizeof(g)) && !memcmp(o,offset,sizeof(o)));
 for(unsigned field=0;field<3;field++) {
  for(unsigned n=1;n<=4095;n++) {gain[field]=n;check(all,gain,offset);}
  gain[field]=256;
  for(unsigned n=0;n<=1023;n++) {offset[field]=n;check(all,gain,offset);}
  offset[field]=0;
  gain[field]=0;invalid(gain,offset);
  gain[field]=4096;invalid(gain,offset);
  gain[field]=0xffffffffU;invalid(gain,offset);gain[field]=256;
  offset[field]=1024;invalid(gain,offset);
  offset[field]=0xffffffffU;invalid(gain,offset);offset[field]=0;
 }
 /* Deterministic varied reserved bits and simultaneous fields. */
 __u32 seed=0x73b41209U;
 for(unsigned n=0;n<20000;n++) {
  for(unsigned i=0;i<3;i++) {
   seed=seed*1664525U+1013904223U;words[i]=seed;
   gain[i]=1+(seed%4095);offset[i]=(seed>>12)&1023;
  }
  check(words,gain,offset);
 }
 /* Restore presets from the immutable source, NOT the previous custom words. */
 assert(!qc_isp_wb_manual(words,gain,offset));
 memcpy(words,&qc07f_isp_reference[0xd34/4],sizeof(words));
 assert(!qc_isp_wb_preset(QC_ISP_WB_RETAINED_MANUAL,words));
 qc_isp_wb_fields(words,g,o);assert(g[0]==413&&g[1]==256&&g[2]==310);
 assert(!o[0]&&!o[1]&&!o[2]);
 memcpy(words,&qc07f_isp_reference[0xd34/4],sizeof(words));
 assert(!qc_isp_wb_preset(QC_ISP_WB_SNAPSHOT,words));
 qc_isp_wb_fields(words,g,o);assert(g[0]==396&&g[1]==258&&g[2]==498);
 assert(!o[0]&&!o[1]&&!o[2]);
 puts("IMG-04 host raw WB exhaustive boundaries/packing/reserved bits/atomic rejection/source presets/ABI PASS; no hardware evidence");
 return 0;
}
