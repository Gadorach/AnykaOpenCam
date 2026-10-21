/* SPDX-License-Identifier: GPL-2.0-only; pure packing fixtures, no hardware. */
#include "../../bringup/qc07f/userland/sc1346-controls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
 uint8_t original[3]={0xa0,0x2e,0x8b},packed[3],gain[3];
 assert(sc1346_exposure_code(original)==744);
 for(uint32_t value=2;value<=744;value++){
  assert(!sc1346_exposure_pack(value,750,original,packed));
  assert(sc1346_exposure_code(packed)==value);
  assert((packed[0]&0xf0)==0xa0&&(packed[2]&15)==11);
 }
 assert(!sc1346_exposure_pack(186,750,original,packed));
 assert(packed[0]==0xa0&&packed[1]==11&&packed[2]==0xab);
 memset(packed,0x55,sizeof(packed));
 assert(sc1346_exposure_pack(1,750,original,packed)==-1&&packed[0]==0x55);
 assert(sc1346_exposure_pack(745,750,original,packed)==-1);
 assert(sc1346_exposure_pack(744,749,original,packed)==-1);
 assert(sc1346_exposure_pack(2,6,original,packed)==-1);
 assert(sc1346_exposure_pack(UINT32_MAX,750,original,packed)==-1);
 assert(!sc1346_vendor_gain_pack(256,gain)&&gain[0]==0&&gain[1]==0&&gain[2]==128);
 assert(!sc1346_vendor_gain_pack(384,gain)&&gain[0]==0&&gain[2]==192);
 const uint32_t values[]={512,1024,2048,4096,8192};
 const uint8_t stages[]={8,9,11,15,31};
 for(unsigned i=0;i<5;i++)assert(!sc1346_vendor_gain_pack(values[i],gain)&&gain[0]==stages[i]&&gain[1]==0&&gain[2]==128);
 assert(!sc1346_vendor_gain_pack(16383,gain)&&gain[0]==31&&gain[2]==252);
 assert(sc1346_vendor_gain_pack(255,gain)==-1&&sc1346_vendor_gain_pack(16384,gain)==-1);
 puts("SC1346 exposure field/range/reserved-bit and assembly-corroborated gain divisor fixtures PASS; no physical units inferred");return 0;
}
