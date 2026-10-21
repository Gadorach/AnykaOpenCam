/* SPDX-License-Identifier: GPL-2.0-only
 * Partial stock HGICS wire header inferred through REA RX/TX routines.
 * Byte arrays avoid host ABI/bitfield/endian assumptions. No live device I/O.
 * This is NOT the complete TXW801 protocol or a replacement radio firmware.
 */
#ifndef QC07F_TXW801_WIRE_H
#define QC07F_TXW801_WIRE_H
#include <stdint.h>

#define QC07F_HGIC_RX_MAGIC 0x2b1aU
#define QC07F_HGIC_COMMON_BYTES 8U
#define QC07F_HGIC_FRAME_BYTES 32U
#define QC07F_HGIC_COOKIE_MASK 0x7fffU
struct qc07f_hgic_header_bytes {
    uint8_t magic_le[2];
    uint8_t type;
    uint8_t interface_and_flags;
    uint8_t length_le[2];
    uint8_t cookie_le[2];
};
_Static_assert(sizeof(struct qc07f_hgic_header_bytes)==8,"observed common header size");
static inline uint16_t qc07f_hgic_le16(const uint8_t p[2])
{ return (uint16_t)p[0] | ((uint16_t)p[1]<<8); }

/* Observed RX cases: 1 TX completion/cookies; 2 frame; 0x12 aggregation;
 * 3/4/7/0xc/0xd/0xe control/event path; 0xa test data; 0xb completion/window.
 * High nibble of byte3 contributes padding removal. Complete per-type length,
 * flags, authentication/key commands and firmware-download checks must be
 * reconstructed/validated before sending any packet. Never substitute the
 * public TXW8301 FMAC firmware for this physically identified TXW801-100.
 */
#endif
