/* SPDX-License-Identifier: GPL-2.0-only
 * Integrity of deliveredpixels, independent of kernel/DMA ownership checks.
 */
#ifndef QC_CRC_BZIP2_H
#define QC_CRC_BZIP2_H
#include <stdint.h>
#include <stddef.h>
#include "../../../framework/include/camera/dma_integrity.h"
static uint32_t qc_crc_bzip2(const uint8_t *data,size_t bytes)
{
 static uint32_t table[256];static int initialized;
 if(!initialized){
  for(unsigned i=0;i<256;i++){
   uint32_t c=i<<24;
   for(unsigned b=0;b<8;b++)c=(c<<1)^((c&0x80000000)?0x04c11db7:0);
   table[i]=c;
  }
  initialized=1;
 }
 return camera_crc_bzip2_ram(data,bytes,table);
}
#endif
