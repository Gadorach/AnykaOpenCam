/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/fmp4.h>
#include <string.h>

static void be32(uint8_t *p,uint32_t n)
{p[0]=(uint8_t)(n>>24);p[1]=(uint8_t)(n>>16);p[2]=(uint8_t)(n>>8);p[3]=(uint8_t)n;}
static void be64(uint8_t *p,uint64_t n)
{be32(p,(uint32_t)(n>>32));be32(p+4,(uint32_t)n);}
static void atom(uint8_t *p,uint32_t bytes,const char type[4])
{be32(p,bytes);memcpy(p+4,type,4);}
static int overlap(const void *a,size_t na,const void *b,size_t nb)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return x<=y?y-x<na:x-y<nb;
}
int camera_fmp4_fragment_prepare(const struct camera_fmp4_sample *s,
                                 struct camera_fmp4_fragment *out)
{
 struct camera_fmp4_fragment result={0};uint8_t *h=result.header;
 if(!s||!out||!s->nal||!s->nal_bytes||s->nal_bytes>CAMERA_FMP4_MAX_NAL_BYTES||
    !s->sequence||!s->duration_ticks||s->duration_ticks>=0x80000000U||s->keyframe>1||
    s->decode_ticks>UINT64_MAX-s->duration_ticks||
    overlap(out,sizeof(*out),s,sizeof(*s))||
    overlap(out,sizeof(*out),s->nal,s->nal_bytes))return CAMERA_INVALID;
 if((s->nal[0]&0x80)||((s->nal[0]&31)!=1&&(s->nal[0]&31)!=5)||
    ((s->nal[0]&31)==5)!=(s->keyframe!=0)||
    (s->sequence==1&&!s->keyframe))return CAMERA_INVALID;
 /* moof100: mfhd16 + traf76(tfhd16 + tfdt20 + trun32), then mdat/prefix12.
  * trun data_offset108 points to the four-byte AVC NAL length, not NAL byte0.
  * All lengths are checked above, and every output byte is initialized. */
 atom(h,100,"moof");atom(h+8,16,"mfhd");be32(h+20,s->sequence);
 atom(h+24,76,"traf");atom(h+32,16,"tfhd");be32(h+40,0x20000U);be32(h+44,1);
 atom(h+48,20,"tfdt");be32(h+56,0x1000000U);be64(h+60,s->decode_ticks);
 atom(h+68,32,"trun");be32(h+76,0x701U);be32(h+80,1);be32(h+84,108);
 be32(h+88,s->duration_ticks);be32(h+92,(uint32_t)s->nal_bytes+4);
 be32(h+96,s->keyframe?0x2000000U:0x1010000U);
 atom(h+100,(uint32_t)s->nal_bytes+12,"mdat");be32(h+108,(uint32_t)s->nal_bytes);
 result.nal=s->nal;result.nal_bytes=s->nal_bytes;*out=result;return CAMERA_OK;
}
int camera_fmp4_fragment_span(const struct camera_fmp4_fragment *f,size_t offset,
                              size_t chunk,const uint8_t **data,size_t *bytes)
{
 const uint8_t *p;size_t n;
 if(!f||!f->nal||!f->nal_bytes||f->nal_bytes>CAMERA_FMP4_MAX_NAL_BYTES||!data||!bytes||
    !chunk||chunk>CAMERA_FMP4_SEND_CHUNK_BYTES||offset>sizeof(f->header)+f->nal_bytes||
    overlap(data,sizeof(*data),bytes,sizeof(*bytes))||overlap(data,sizeof(*data),f,sizeof(*f))||
    overlap(bytes,sizeof(*bytes),f,sizeof(*f))||overlap(data,sizeof(*data),f->nal,f->nal_bytes)||
    overlap(bytes,sizeof(*bytes),f->nal,f->nal_bytes))return CAMERA_INVALID;
 if(offset<sizeof(f->header)){p=f->header+offset;n=sizeof(f->header)-offset;}
 else {size_t at=offset-sizeof(f->header);p=f->nal+at;n=f->nal_bytes-at;}
 if(n>chunk)n=chunk;
 *data=p;*bytes=n;return CAMERA_OK;
}
