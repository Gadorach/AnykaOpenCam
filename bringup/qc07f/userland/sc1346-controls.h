/* SPDX-License-Identifier: GPL-2.0-only
 * Numeric SC1346 contracts corroborated with REA stock sensor_sc1346.ko
 * SHA dd186edd6366fb50d39508c0162cce1b971fae3ec4b454f50e6132542e54eec8.
 * Exposure is an opaque vendor code, NOT microseconds/lines/lux. The gain
 * mapping is vendor Q8-scale arithmetic, not calibrated optical response.
 */
#ifndef QC07F_SC1346_CONTROLS_H
#define QC07F_SC1346_CONTROLS_H
#include <stdint.h>
#define SC1346_RETAIL_VTS 750U
#define SC1346_RETAIL_HTS 1800U
#define SC1346_RETAIL_EXPOSURE 744U
static inline uint32_t sc1346_exposure_code(const uint8_t registers[3])
{
 return ((uint32_t)(registers[0]&15)<<12)|((uint32_t)registers[1]<<4)|(registers[2]>>4);
}
/* Preserve bits outside the corroborated field; callers keep the sensor in
 * standby and hold capture ownership throughout write/readback/rollback.
 * This first experiment cannot increase beyond the qualified retail code.
 */
static inline int sc1346_exposure_pack(uint32_t code,uint32_t vts,const uint8_t original[3],uint8_t out[3])
{
 if(vts<=6||code<2||code>vts-6||code>SC1346_RETAIL_EXPOSURE)return -1;
 out[0]=(original[0]&0xf0)|((code>>12)&15);
 out[1]=(code>>4)&255;
 out[2]=(original[2]&15)|((code&15)<<4);
 return 0;
}
/* Retained assembly keeps r1=1000 in the unity interpolation branch. Avoid
 * the stock >=64x wrap-to-fine128 behavior: only its monotonic range is modeled.
 * This helper is fixture-only for now; no target gain writes use it.
 */
static inline int sc1346_vendor_gain_pack(uint32_t q8,uint8_t out[3])
{
 uint32_t stage,base=1000,value;
 if(q8<256||q8>=16384)return -1;
 value=(q8*1000U)>>8;
 if(value<2000)stage=0;
 else if(value<4000){stage=8;base=2000;}
 else if(value<8000){stage=9;base=4000;}
 else if(value<16000){stage=11;base=8000;}
 else if(value<32000){stage=15;base=16000;}
 else{stage=31;base=32000;}
 out[0]=stage; /* 3e09 */
 out[1]=0; /* 3e06 */
 out[2]=(128U+(value-base)*128U/base)&0xfc; /* 3e07 */
 return 0;
}
#endif
