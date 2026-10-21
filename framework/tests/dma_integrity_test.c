/* SPDX-License-Identifier: GPL-2.0-only
 * Host/ARMfixtures; fixtures do not establish targetDMA/streaming results.
 */
#include <camera/dma_integrity.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t oracle(const uint8_t *p,size_t bytes)
{
 uint32_t c=0xffffffff;
 while(bytes--){c^=(uint32_t)*p++<<24;for(unsigned i=0;i<8;i++)c=(c<<1)^((c&0x80000000)?0x04c11db7:0);}
 return c^0xffffffff;
}
int main(int argc,char **argv)
{
 uint32_t table[256],sliced[4][256];camera_crc_bzip2_table(table);camera_crc_bzip2_sliced_table(sliced);
 assert(camera_crc_bzip2_ram("123456789",9,table)==0xfc891918);
 assert(camera_crc_bzip2_sliced_ram("123456789",9,sliced)==0xfc891918);
 for(size_t n=0;n<4096;n+=n<64?1:37){
  for(size_t off=0;off<4;off++){
   uint8_t *storage=malloc(n+off+1);assert(storage);uint8_t *p=storage+off;
   for(size_t i=0;i<n;i++)p[i]=(i*37+n*19)&255;
   assert(camera_crc_bzip2_ram(p,n,table)==oracle(p,n));
   assert(camera_crc_bzip2_sliced_ram(p,n,sliced)==oracle(p,n));
   size_t count=0;for(size_t i=0;i<n;i++)count+=p[i]!=0xa5;
   assert(camera_guard_errors_ram(p,n)==count);
   memset(p,0xa5,n);assert(camera_guard_errors_ram(p,n)==0);
   if(n){p[n-1]^=1;assert(camera_guard_errors_ram(p,n)==1);}
   free(storage);
  }
 }
 if(argc==2){
  FILE *file=fopen(argv[1],"rb");assert(file);uint8_t *p=malloc(1728000);assert(p);
  assert(fread(p,1,1728000,file)==1728000&&fgetc(file)==EOF);fclose(file);
  assert(camera_crc_bzip2_ram(p,1728000,table)==oracle(p,1728000));
  assert(camera_crc_bzip2_sliced_ram(p,1728000,sliced)==oracle(p,1728000));
  uint32_t c=camera_crc_bzip2_ram(p,1728000,table);p[1727999]^=1;
  assert(camera_crc_bzip2_ram(p,1728000,table)!=c);
  assert(camera_crc_bzip2_sliced_ram(p,1728000,sliced)!=c);
  assert(camera_crc_bzip2_sliced_ram(p,1728000,sliced)==oracle(p,1728000));free(p);
 }
 puts("WordCRC/byteoracle/unaligned/tail/guard/corruption fixtures PASS");return 0;
}
