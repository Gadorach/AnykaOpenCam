/* SPDX-License-Identifier: LGPL-2.0-or-later */
/*
 * Mu-Law conversion portion adapted from Linux sound/core/oss/mulaw.c:
 * Copyright (c) 1999 by Jaroslav Kysela <perex@perex.cz>
 *                       Uros Bizjak <uros@kss-loka.si>
 * Based on reference implementation by Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public License.
 * You should have received a copy of that license along with this library;
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA.
 *
 * Original complete source/notice/license and selected reference leaves are
 * preserved in evidence. A-law below is newly implemented integer segment/
 * mantissa arithmetic from G.711 conventions, licensed under the same LGPL;
 * this does NOT assert an ITU document/software redistribution license.
 */
#include "g711.h"

static unsigned segment(unsigned magnitude)
{
 unsigned s=0,upper=255;
 while(s<7&&magnitude>upper){s++;upper=(upper<<1)|1U;}
 return s;
}
uint8_t g711_encode_mu(int16_t sample)
{
 int32_t x=sample;unsigned magnitude,s,code,mask;
 mask=x<0?0x7fU:0xffU;magnitude=(unsigned)(x<0?132-x:132+x);
 if(magnitude>32767)magnitude=32767;
 s=segment(magnitude);code=(s<<4)|((magnitude>>(s+3))&15U);
 return (uint8_t)(code^mask); /* full16-bit Sun/ALSA convention; zero trap OFF */
}
int16_t g711_decode_mu(uint8_t code)
{
 unsigned inverse=(unsigned)(code^0xffU);
 int32_t magnitude=(int32_t)(((inverse&15U)*8U+132U)<<((inverse>>4)&7U));
 return (int16_t)((inverse&128U)?132-magnitude:magnitude-132);
}
uint8_t g711_encode_a(int16_t sample)
{
 int32_t x=sample;unsigned magnitude,s,code,mask;
 mask=x<0?0x55U:0xd5U;magnitude=(unsigned)(x<0?-x-1:x);
 s=segment(magnitude);code=(s<<4)|((magnitude>>(s?s+3:4))&15U);
 return (uint8_t)(code^mask);
}
int16_t g711_decode_a(uint8_t code)
{
 unsigned inverse=(unsigned)(code^0x55U),s=(inverse>>4)&7U;
 int32_t magnitude=(int32_t)((inverse&15U)*16U);
 magnitude=s?((magnitude+264)<<(s-1)):magnitude+8;
 return (int16_t)((inverse&128U)?magnitude:-magnitude);
}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!an||!bn)return 0;
 if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
int g711_encode_s16le(uint32_t law,const uint8_t *pcm,size_t bytes,uint8_t *codes,size_t capacity)
{
 size_t i;
 if((law!=G711_MULAW&&law!=G711_ALAW)||!pcm||!codes||!bytes||bytes>512||(bytes&1)||
    capacity<bytes/2||overlap(pcm,bytes,codes,bytes/2))return -1;
 for(i=0;i<bytes/2;i++){
  unsigned word=(unsigned)pcm[2*i]|((unsigned)pcm[2*i+1]<<8);
  int32_t signed_value=word>=32768?(int32_t)word-65536:(int32_t)word;
  codes[i]=law==G711_MULAW?g711_encode_mu((int16_t)signed_value):g711_encode_a((int16_t)signed_value);
 }
 return 0;
}
