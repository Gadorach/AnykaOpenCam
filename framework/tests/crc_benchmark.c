/* SPDX-License-Identifier: GPL-2.0-only
 * CPU-only comparison on a measured full frame. Does not operate ISP/DMA or
 * weaken integrity. CRC equality is mandatory; timing is not a hardware claim.
 */
#define _POSIX_C_SOURCE 200809L
#include <camera/dma_integrity.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static uint64_t ns(void)
{struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))abort();return (uint64_t)t.tv_sec*1000000000ULL+t.tv_nsec;}
static uint32_t bytewise(const uint8_t *p,size_t n,const uint32_t table[256])
{uint32_t c=0xffffffff;while(n--)c=(c<<8)^table[((c>>24)^*p++)&255];return c^0xffffffff;}
int main(int argc,char **argv)
{
 if(argc!=2)return 2;
 FILE *file=fopen(argv[1],"rb");if(!file)return 1;
 uint8_t *data=malloc(1728000);if(!data){fclose(file);return 1;}
 if(fread(data,1,1728000,file)!=1728000||fgetc(file)!=EOF){fclose(file);free(data);return 1;}fclose(file);
 uint32_t t[256],sliced[4][256];camera_crc_bzip2_table(t);camera_crc_bzip2_sliced_table(sliced);
 uint32_t expected=bytewise(data,1728000,t);unsigned repetitions=8;
 for(unsigned mode=0;mode<3;mode++){
  uint64_t start=ns();uint32_t actual=0;
  for(unsigned i=0;i<repetitions;i++){
   __asm__ __volatile__("" : : "r"(data) : "memory"); /* forbid loop-hoisted scans */
   actual=mode==0?bytewise(data,1728000,t):mode==1?camera_crc_bzip2_ram(data,1728000,t):camera_crc_bzip2_sliced_ram(data,1728000,sliced);
   if(actual!=expected){free(data);return 1;}
  }
  printf("CRC CPU mode=%u repetitions=%u mean_ns=%llu bytes=1728000 crc=%08x; NOdeviceDMAclaim\n",mode,repetitions,(unsigned long long)((ns()-start)/repetitions),actual);
 }
 free(data);return 0;
}
