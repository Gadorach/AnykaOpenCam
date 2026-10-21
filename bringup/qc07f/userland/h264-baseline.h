/* SPDX-License-Identifier: GPL-2.0-only
 * Parameter sets for the bounded retail-correlated ASIC profile, not a
 * software H264 encoder. Pixel/macroblock/slice coding happens only inASIC.
 * Baseline/CAVLC, frame-only720p, POCtype2,16-bit frame_num, PPS0/chromaQP+2.
 */
#ifndef QC_H264_BASELINE_H
#define QC_H264_BASELINE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
struct qc_bits {uint8_t data[128];unsigned count;};
static void qc_bits_put(struct qc_bits *b,unsigned value,unsigned count)
{
 for(unsigned i=count;i;i--){unsigned bit=(value>>(i-1))&1;b->data[b->count/8]|=bit<<(7-b->count%8);b->count++;}
}
static void qc_bits_ue(struct qc_bits *b,unsigned value)
{
 unsigned n=value+1,k=0,t=n;while(t>>=1)k++;
 qc_bits_put(b,0,k);qc_bits_put(b,n,k+1);
}
static void qc_bits_se(struct qc_bits *b,int value){qc_bits_ue(b,value<=0?(unsigned)(-value*2):(unsigned)(value*2-1));}
static size_t qc_h264_nal(uint8_t *dest,uint8_t type,struct qc_bits *b)
{
 qc_bits_put(b,1,1);while(b->count%8)qc_bits_put(b,0,1);
 size_t n=0;unsigned zero=0;dest[n++]=0;dest[n++]=0;dest[n++]=0;dest[n++]=1;dest[n++]=type;
 for(unsigned i=0;i<b->count/8;i++){
  uint8_t v=b->data[i];if(zero==2&&v<=3){dest[n++]=3;zero=0;}
  dest[n++]=v;zero=v==0?zero+1:0;
 }
 return n;
}
/* Caller provides >=256 bytes. Exact fixed values match ioctl profile. */
static size_t qc_h264_parameter_sets(uint8_t dest[256])
{
 struct qc_bits b={0};size_t n;
 qc_bits_put(&b,66,8);qc_bits_put(&b,0xe0,8);qc_bits_put(&b,31,8);
 qc_bits_ue(&b,0);qc_bits_ue(&b,12);qc_bits_ue(&b,2);qc_bits_ue(&b,1);
 qc_bits_put(&b,0,1);qc_bits_ue(&b,79);qc_bits_ue(&b,44);
 qc_bits_put(&b,1,1);qc_bits_put(&b,1,1);qc_bits_put(&b,0,1);qc_bits_put(&b,0,1);
 n=qc_h264_nal(dest,0x67,&b);memset(&b,0,sizeof(b));
 qc_bits_ue(&b,0);qc_bits_ue(&b,0);qc_bits_put(&b,0,1);qc_bits_put(&b,0,1);
 qc_bits_ue(&b,0);qc_bits_ue(&b,0);qc_bits_ue(&b,0);
 qc_bits_put(&b,0,1);qc_bits_put(&b,0,2);
 qc_bits_se(&b,0);qc_bits_se(&b,0);qc_bits_se(&b,2);
 qc_bits_put(&b,1,1);qc_bits_put(&b,0,1);qc_bits_put(&b,0,1);
 return n+qc_h264_nal(dest+n,0x68,&b);
}
#endif
