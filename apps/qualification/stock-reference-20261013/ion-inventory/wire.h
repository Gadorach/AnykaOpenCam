/* Local ARM32 interface description; original vendor/license files untouched. */
#ifndef STOCK_ION_WIRE_H
#define STOCK_ION_WIRE_H
#include <stdint.h>
#include <stddef.h>
#define I_ALLOC 0xc0144900UL
#define I_MAP 0xc0084902UL
#define I_FREE 0xc0044901UL
#define OWN_BYTES 0x00800000U
#define PAGE_BYTES 4096U
#define OWN_PAGES (OWN_BYTES / PAGE_BYTES)
#define HEAP_MASK 2U /* heap id1, NOT the enum type2 mask4 */
#define ALLOC_FLAGS 3U
struct alloc_wire { uint32_t len, align, mask, flags, handle; };
struct map_wire { uint32_t handle; int32_t fd; };
struct free_wire { uint32_t handle; };
_Static_assert(sizeof(struct alloc_wire) == 20 && offsetof(struct alloc_wire, handle) == 16, "ALLOC20");
_Static_assert(sizeof(struct map_wire) == 8 && offsetof(struct map_wire, fd) == 4, "MAP8");
_Static_assert(sizeof(struct free_wire) == 4, "FREE4");
#endif
