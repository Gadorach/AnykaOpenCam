/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/fmp4.h>
static int overlaps(const void *a,size_t na,const void *b,size_t nb)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<na:x-y<nb;}
int camera_fmp4_timeline_init(struct camera_fmp4_timeline *t,uint64_t generation,uint32_t rtp)
{
 if(!t||!generation)return CAMERA_INVALID;
 *t=(struct camera_fmp4_timeline){generation,0,rtp,1,1};return CAMERA_OK;
}
int camera_fmp4_timeline_step(struct camera_fmp4_timeline *t,uint64_t generation,
                              uint32_t current,uint32_t next,const uint8_t *nal,size_t n,
                              uint32_t keyframe,struct camera_fmp4_fragment *out)
{
 if(!t||!out||!nal||!n||n>CAMERA_FMP4_MAX_NAL_BYTES||!t->initialized||!t->generation||
    overlaps(t,sizeof(*t),out,sizeof(*out))||overlaps(t,sizeof(*t),nal,n))return CAMERA_INVALID;
 if(generation!=t->generation||current!=t->expected_rtp)return CAMERA_STALE_LEASE;
 if(!t->next_sequence||t->next_sequence==UINT32_MAX)return CAMERA_NOT_READY;
 uint32_t delta=next-current;
 if(!delta||delta>=0x80000000U||t->decode_ticks>UINT64_MAX-delta)return CAMERA_INVALID;
 struct camera_fmp4_sample s={nal,n,t->decode_ticks,delta,t->next_sequence,keyframe};
 int r=camera_fmp4_fragment_prepare(&s,out);if(r!=CAMERA_OK)return r;
 t->decode_ticks+=delta;t->expected_rtp=next;t->next_sequence++;return CAMERA_OK;
}
