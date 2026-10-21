/* SPDX-License-Identifier: GPL-2.0-only; numeric field fixtures, no hardware. */
#include "../../bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-whitebalance.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
 __u32 snapshot[3]={0x0102018c,0x000001f2,0},words[3],gain[3],offset[3];
 memcpy(words,snapshot,sizeof(words));assert(!qc_isp_wb_preset(0,words));
 assert(!memcmp(words,snapshot,sizeof(words)));qc_isp_wb_fields(words,gain,offset);
 assert(gain[0]==396&&gain[1]==258&&gain[2]==498);
 memcpy(words,snapshot,sizeof(words));assert(!qc_isp_wb_preset(1,words));
 qc_isp_wb_fields(words,gain,offset);assert(gain[0]==413&&gain[1]==256&&gain[2]==310);
 assert(!offset[0]&&!offset[1]&&!offset[2]);
 __u32 all[3]={0xffffffffU,0xffffffffU,0xffffffffU};assert(!qc_isp_wb_preset(1,all));
 assert((all[0]&~0x0fff0fffU)==(~0x0fff0fffU));
 assert((all[1]&~0x03ff0fffU)==(~0x03ff0fffU));
 assert((all[2]&~0x03ff03ffU)==(~0x03ff03ffU));
 memcpy(words,snapshot,sizeof(words));assert(qc_isp_wb_preset(2,words)==-1);
 assert(!memcmp(words,snapshot,sizeof(words)));
 puts("QC07F WB snapshot/manual/field-order/reserved-bit/invalid fixtures PASS");return 0;
}
