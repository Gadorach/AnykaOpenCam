/* SPDX-License-Identifier: GPL-2.0-only
 * Offline RAM-only CRC-32/BZIP2 prototype. Not an MMIO accessor.
 * Caller validates [data,data+bytes), owns/quiesces DMA, and initializes private
 * tables before publication. No lazy initialization, allocation or shared state.
 */
#ifndef CRC_RAM01_H
#define CRC_RAM01_H
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/compiler.h>
typedef u32 crc_ram01_u32;
typedef u8 crc_ram01_u8;
typedef unsigned long crc_ram01_addr;
#elif defined(CRC_RAM01_FREESTANDING)
typedef __UINT32_TYPE__ crc_ram01_u32;
typedef __UINT8_TYPE__ crc_ram01_u8;
typedef __UINTPTR_TYPE__ crc_ram01_addr;
typedef __SIZE_TYPE__ size_t;
#else
#include <stdint.h>
#include <stddef.h>
typedef uint32_t crc_ram01_u32;
typedef uint8_t crc_ram01_u8;
typedef uintptr_t crc_ram01_addr;
#endif
typedef crc_ram01_u32 crc_ram01_alias_u32 __attribute__((__may_alias__));
typedef char crc_ram01_word_must_be_32_bits[sizeof(crc_ram01_u32) == 4 ? 1 : -1];
/* ARMv5 GCC -Os lowers __builtin_bswap32 to an external __bswapsi2 call.
 * This rotate/xor identity uses only scalar ARMv5 operations, no link helper.
 */
static inline crc_ram01_u32 crc_ram01_swap(crc_ram01_u32 x)
{
 crc_ram01_u32 t = x ^ ((x >> 16) | (x << 16));
 t &= 0xff00ffffU;
 return ((x >> 8) | (x << 24)) ^ (t >> 8);
}
#ifndef CRC_RAM01_BIG_ENDIAN
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define CRC_RAM01_BIG_ENDIAN 1
#else
#define CRC_RAM01_BIG_ENDIAN 0
#endif
#endif
/* Test hooks observe every access; default builds have no hook overhead. */
#ifndef CRC_RAM01_ACCESS
#define CRC_RAM01_ACCESS(p, n) ((void)0)
#endif
#ifndef CRC_RAM01_NATIVE_WORD
#ifdef __KERNEL__
#define CRC_RAM01_NATIVE_WORD(p) READ_ONCE(*(const crc_ram01_alias_u32 *)(p))
#else
#define CRC_RAM01_NATIVE_WORD(p) (*(const volatile crc_ram01_alias_u32 *)(p))
#endif
#endif
static inline crc_ram01_u32 crc_ram01_word(const crc_ram01_u8 *p)
{
 crc_ram01_u32 w;
 CRC_RAM01_ACCESS(p, 4);
 w = CRC_RAM01_NATIVE_WORD(p); /* p is 4-aligned; exactly one volatile word. */
#if CRC_RAM01_BIG_ENDIAN
 w = crc_ram01_swap(w);
#endif
 return w; /* b0 | b1<<8 | b2<<16 | b3<<24 on either CPU byte order. */
}
static inline crc_ram01_u32 crc_ram01_byte(const crc_ram01_u8 *p)
{
 CRC_RAM01_ACCESS(p, 1);
 return *(const volatile crc_ram01_u8 *)p;
}
/* 8 KiB per private device/user context. Tables are immutable during calls. */
struct crc_ram01_context { crc_ram01_u32 t[8][256]; };
static inline void crc_ram01_init(struct crc_ram01_context *ctx)
{
 unsigned i, bit, s;
 for (i = 0; i < 256; i++) {
  crc_ram01_u32 c = (crc_ram01_u32)i << 24;
  for (bit = 0; bit < 8; bit++)
   c = (c << 1) ^ ((c & 0x80000000U) ? 0x04c11db7U : 0);
  ctx->t[0][i] = c;
 }
 for (s = 1; s < 8; s++) for (i = 0; i < 256; i++) {
  crc_ram01_u32 c = ctx->t[s - 1][i];
  ctx->t[s][i] = (c << 8) ^ ctx->t[0][c >> 24];
 }
}
static inline crc_ram01_u32 crc_ram01_step(crc_ram01_u32 c, crc_ram01_u32 b,
                                         const struct crc_ram01_context *ctx)
{ return (c << 8) ^ ctx->t[0][((c >> 24) ^ b) & 255]; }
/* Scalar comparator with precisely the same volatile RAM access contract. */
static inline crc_ram01_u32 crc_ram01_scalar(const void *data, size_t bytes,
                                            const struct crc_ram01_context *ctx)
{
 const crc_ram01_u8 *p = data;
 crc_ram01_u32 c = 0xffffffffU;
 while (bytes && ((crc_ram01_addr)p & 3)) {
  c = crc_ram01_step(c, crc_ram01_byte(p++), ctx); bytes--;
 }
 while (bytes >= 4) {
  crc_ram01_u32 w = crc_ram01_word(p);
  c = crc_ram01_step(c, w & 255, ctx);
  c = crc_ram01_step(c, (w >> 8) & 255, ctx);
  c = crc_ram01_step(c, (w >> 16) & 255, ctx);
  c = crc_ram01_step(c, w >> 24, ctx);
  p += 4; bytes -= 4;
 }
 while (bytes) { c = crc_ram01_step(c, crc_ram01_byte(p++), ctx); bytes--; }
 return c ^ 0xffffffffU;
}
static inline crc_ram01_u32 crc_ram01_four(crc_ram01_u32 c, crc_ram01_u32 w,
                                         const struct crc_ram01_context *ctx)
{
 return ctx->t[3][(c >> 24) ^ (w & 255)] ^
        ctx->t[2][((c >> 16) ^ (w >> 8)) & 255] ^
        ctx->t[1][((c >> 8) ^ (w >> 16)) & 255] ^
        ctx->t[0][(c ^ (w >> 24)) & 255];
}
static inline crc_ram01_u32 crc_ram01_slice4(const void *data, size_t bytes,
                                            const struct crc_ram01_context *ctx)
{
 const crc_ram01_u8 *p = data;
 crc_ram01_u32 c = 0xffffffffU;
 while (bytes && ((crc_ram01_addr)p & 3)) {
  c = crc_ram01_step(c, crc_ram01_byte(p++), ctx); bytes--;
 }
 while (bytes >= 4) { c = crc_ram01_four(c, crc_ram01_word(p), ctx); p += 4; bytes -= 4; }
 while (bytes) { c = crc_ram01_step(c, crc_ram01_byte(p++), ctx); bytes--; }
 return c ^ 0xffffffffU;
}
static inline crc_ram01_u32 crc_ram01_slice8(const void *data, size_t bytes,
                                            const struct crc_ram01_context *ctx)
{
 const crc_ram01_u8 *p = data;
 crc_ram01_u32 c = 0xffffffffU;
 while (bytes && ((crc_ram01_addr)p & 3)) {
  c = crc_ram01_step(c, crc_ram01_byte(p++), ctx); bytes--;
 }
 while (bytes >= 8) {
  crc_ram01_u32 a = crc_ram01_word(p), b = crc_ram01_word(p + 4);
  c = ctx->t[7][(c >> 24) ^ (a & 255)] ^
      ctx->t[6][((c >> 16) ^ (a >> 8)) & 255] ^
      ctx->t[5][((c >> 8) ^ (a >> 16)) & 255] ^
      ctx->t[4][(c ^ (a >> 24)) & 255] ^
      ctx->t[3][b & 255] ^ ctx->t[2][(b >> 8) & 255] ^
      ctx->t[1][(b >> 16) & 255] ^ ctx->t[0][b >> 24];
  p += 8; bytes -= 8;
 }
 if (bytes >= 4) { c = crc_ram01_four(c, crc_ram01_word(p), ctx); p += 4; bytes -= 4; }
 while (bytes) { c = crc_ram01_step(c, crc_ram01_byte(p++), ctx); bytes--; }
 return c ^ 0xffffffffU;
}
/* Generated GPL table: 4 KiB bounded .rodata, no per-call/table-init work.
 * State/table words are byte-swapped, NOT a reflected polynomial CRC.
 */
#include "crc_ram01_static_table.h"
static inline crc_ram01_u32 crc_ram01_static4(const void *data, size_t bytes)
{
 const crc_ram01_u8 *p = data;
 crc_ram01_u32 r = 0xffffffffU;
 while (bytes && ((crc_ram01_addr)p & 3)) {
  r = (r >> 8) ^ crc_ram01_static_t[0][(r ^ crc_ram01_byte(p++)) & 255]; bytes--;
 }
 while (bytes >= 4) {
  r ^= crc_ram01_word(p);
  r = crc_ram01_static_t[3][r & 255] ^ crc_ram01_static_t[2][(r >> 8) & 255] ^
      crc_ram01_static_t[1][(r >> 16) & 255] ^ crc_ram01_static_t[0][r >> 24];
  p += 4; bytes -= 4;
 }
 while (bytes) {
  r = (r >> 8) ^ crc_ram01_static_t[0][(r ^ crc_ram01_byte(p++)) & 255]; bytes--;
 }
 return crc_ram01_swap(r) ^ 0xffffffffU;
}
#endif
