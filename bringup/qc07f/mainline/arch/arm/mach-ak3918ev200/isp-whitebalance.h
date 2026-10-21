/* SPDX-License-Identifier: GPL-2.0-only
 * Manual WB field order follows Anyka's public vpss_isp_mwb_attr contract
 * (R/G/B gain, then R/G/B offset). Exact QC07F packing is corroborated by
 * retained REA ak_isp_updata_wb_gain, not transplanted EV300 registers.
 * No gain-to-physical-units assumption and no claim of automatic WB.
 */
#ifndef QC07F_ISP_WHITEBALANCE_H
#define QC07F_ISP_WHITEBALANCE_H
#include <linux/types.h>
/* Raw field codes only: signed short in the public SDK does not by itself
 * prove QC07F signed-10-bit arithmetic. No gain unity/physical unit assumed.
 * Validate the whole transaction before touching any word; never clamp.
 */
static inline int qc_isp_wb_manual(__u32 words[3], const __u32 gain[3],
                                 const __u32 offset_raw[3])
{
 unsigned int i;
 for(i=0;i<3;i++)
  if(!gain[i] || gain[i]>4095U || offset_raw[i]>1023U)return -1;
 words[0]=(words[0]&~0x0fff0fffU)|gain[0]|(gain[1]<<16);
 words[1]=(words[1]&~0x03ff0fffU)|gain[2]|(offset_raw[0]<<16);
 words[2]=(words[2]&~0x03ff03ffU)|offset_raw[1]|(offset_raw[2]<<16);
 return 0;
}
/* Input words originate from the immutable, sanitized retail snapshot.
 * Preset 0 restores that snapshot; preset 1 uses the first retained SC1346
 * pack's manual WB fields. It is a comparison preset, NOT calibrated daylight.
 */
static inline int qc_isp_wb_preset(__u32 preset, __u32 words[3])
{
 if(preset>1)return -1;
 if(preset==1){
  words[0]=(words[0]&~0x0fff0fffU)|413U|(256U<<16);
  words[1]=(words[1]&~0x03ff0fffU)|310U; /* R offset=0 */
  words[2]&=~0x03ff03ffU; /* G/B offset=0 */
 }
 return 0;
}
static inline void qc_isp_wb_fields(const __u32 words[3], __u32 gain[3], __u32 offset[3])
{
 gain[0]=words[0]&0xfffU;gain[1]=(words[0]>>16)&0xfffU;gain[2]=words[1]&0xfffU;
 offset[0]=(words[1]>>16)&0x3ffU;offset[1]=words[2]&0x3ffU;offset[2]=(words[2]>>16)&0x3ffU;
}
#endif
