/* SPDX-License-Identifier: LGPL-2.0-or-later */
#ifndef AUDIO_CANDIDATE_G711_H
#define AUDIO_CANDIDATE_G711_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum g711_law {G711_MULAW=1,G711_ALAW=2};
uint8_t g711_encode_mu(int16_t sample);
int16_t g711_decode_mu(uint8_t code);
uint8_t g711_encode_a(int16_t sample);
int16_t g711_decode_a(uint8_t code);
/* S16 little-endian bytes, no alignment/native-endian assumption or resampling.
 * Finite maximum512B input, even length, non-overlapping input/output. */
int g711_encode_s16le(uint32_t law,const uint8_t *pcm,size_t bytes,uint8_t *codes,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
