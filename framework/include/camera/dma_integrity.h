/* SPDX-License-Identifier: GPL-2.0-only
 * Exact RAM integrity helpers for platformdrivers, not MMIO accessors.
 * Read aligned words to avoid four uncachedbus reads per four bytes;
 * everybyte stillparticipates in the originalCRC/guard result. Caller owns
 * the allocation, validatesbounds andquiescesDMA beforecalling.
 */
#ifndef CAMERA_DMA_INTEGRITY_H
#define CAMERA_DMA_INTEGRITY_H
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/byteorder/generic.h>
typedef u32 camera_integrity_u32;
typedef u8 camera_integrity_u8;
static inline camera_integrity_u32 camera_ram_le_word(const void *p)
{return le32_to_cpu(READ_ONCE(*(const __le32 *)p));}
#else
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint32_t camera_integrity_u32;
typedef uint8_t camera_integrity_u8;
static inline camera_integrity_u32 camera_ram_le_word(const void *p)
{
 uint32_t word;memcpy(&word,p,4);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
 word=__builtin_bswap32(word);
#endif
 return word;
}
#endif
static inline void camera_crc_bzip2_table(camera_integrity_u32 table[256])
{
 for(unsigned i=0;i<256;i++){
  camera_integrity_u32 c=(camera_integrity_u32)i<<24;
  for(unsigned bit=0;bit<8;bit++)c=(c<<1)^((c&0x80000000U)?0x04c11db7U:0);
  table[i]=c;
 }
}
static inline camera_integrity_u32 camera_crc_bzip2_ram(const void *data,size_t bytes,const camera_integrity_u32 table[256])
{
 const camera_integrity_u8 *p=data;camera_integrity_u32 c=0xffffffffU;
 while(bytes&&((unsigned long)p&3)){c=(c<<8)^table[((c>>24)^*p++)&255];bytes--;}
 while(bytes>=4){
  camera_integrity_u32 word=camera_ram_le_word(p);
  c=(c<<8)^table[((c>>24)^(word&255))&255];
  c=(c<<8)^table[((c>>24)^((word>>8)&255))&255];
  c=(c<<8)^table[((c>>24)^((word>>16)&255))&255];
  c=(c<<8)^table[((c>>24)^(word>>24))&255];
  p+=4;bytes-=4;
 }
 while(bytes--){c=(c<<8)^table[((c>>24)^*p++)&255];}
 return c^0xffffffffU;
}
/* Optional slicing-by-four: same byte order/polynomial and every byte, with
 * independent cached table lookups. Caller initializes once before publishing
 * the tables; there is no shared lazy-init race inside these helpers.
 */
static inline void camera_crc_bzip2_sliced_table(camera_integrity_u32 table[4][256])
{
 camera_crc_bzip2_table(table[0]);
 for(unsigned slice=1;slice<4;slice++)for(unsigned i=0;i<256;i++){
  camera_integrity_u32 c=table[slice-1][i];
  table[slice][i]=(c<<8)^table[0][c>>24];
 }
}
static inline camera_integrity_u32 camera_crc_bzip2_sliced_ram(const void *data,size_t bytes,const camera_integrity_u32 table[4][256])
{
 const camera_integrity_u8 *p=data;camera_integrity_u32 c=0xffffffffU;
 while(bytes&&((unsigned long)p&3)){c=(c<<8)^table[0][((c>>24)^*p++)&255];bytes--;}
 while(bytes>=4){
  camera_integrity_u32 word=camera_ram_le_word(p);
  word=((word&0xffU)<<24)|((word&0xff00U)<<8)|((word&0xff0000U)>>8)|(word>>24);
  c^=word;
  c=table[3][c>>24]^table[2][(c>>16)&255]^table[1][(c>>8)&255]^table[0][c&255];
  p+=4;bytes-=4;
 }
 while(bytes--){c=(c<<8)^table[0][((c>>24)^*p++)&255];}
 return c^0xffffffffU;
}
static inline size_t camera_guard_errors_ram(const void *data,size_t bytes)
{
 const camera_integrity_u8 *p=data;size_t errors=0;
 while(bytes&&((unsigned long)p&3)){errors+=*p++!=0xa5;bytes--;}
 while(bytes>=4){
  camera_integrity_u32 word=camera_ram_le_word(p);
  if(word!=0xa5a5a5a5U){
   errors+=(word&255)!=0xa5;errors+=((word>>8)&255)!=0xa5;
   errors+=((word>>16)&255)!=0xa5;errors+=(word>>24)!=0xa5;
  }
  p+=4;bytes-=4;
 }
 while(bytes--)errors+=*p++!=0xa5;
 return errors;
}
#endif
