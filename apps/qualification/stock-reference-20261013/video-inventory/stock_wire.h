/* Project-local explicit ARM32 wire descriptions, derived from stock3.4 headers.
 * No vendor driver implementation is copied. Upstream notices stay in originals. */
#ifndef STOCK_VIDEO_WIRE_H
#define STOCK_VIDEO_WIRE_H
#include <stddef.h>
#include <stdint.h>
struct stock_cap {
    unsigned char driver[16], card[32], bus_info[32];
    uint32_t version, capabilities, device_caps, reserved[3];
};
struct stock_format { uint32_t type; unsigned char pix_and_opaque[200]; };
struct stock_enum {
    uint32_t index, type, flags;
    unsigned char description[32];
    uint32_t pixelformat, reserved[4];
};
#define STOCK_QUERYCAP 0x80685600UL
#define STOCK_G_FMT 0xc0cc5604UL
#define STOCK_ENUM_FMT 0xc0405602UL
_Static_assert(sizeof(struct stock_cap) == 104, "QUERYCAP extent");
_Static_assert(offsetof(struct stock_cap, version) == 80, "cap version");
_Static_assert(offsetof(struct stock_cap, capabilities) == 84, "cap capabilities");
_Static_assert(sizeof(struct stock_format) == 204, "G_FMT extent");
_Static_assert(offsetof(struct stock_format, pix_and_opaque) == 4, "format union");
_Static_assert(sizeof(struct stock_enum) == 64, "ENUM_FMT extent");
_Static_assert(offsetof(struct stock_enum, pixelformat) == 44, "enum fourcc");
#endif
