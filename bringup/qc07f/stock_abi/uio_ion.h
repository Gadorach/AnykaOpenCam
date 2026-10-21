/* SPDX-License-Identifier: GPL-2.0-only
 * Observed ARM32 vendor ABI from REA libakuio.so. Reference definitions only;
 * no hardware calls and no claim of Linux6 compatibility or measured layout.
 */
#ifndef QC07F_STOCK_UIO_ION_ABI_H
#define QC07F_STOCK_UIO_ION_ABI_H
#include <stdint.h>

#define QC07F_ION_ALLOC_WIRE 0xc0144900UL
#define QC07F_ION_SHARE_WIRE 0xc0084902UL
#define QC07F_UIO_SYSREG_WRITE_WIRE 0x400c5564UL
#define QC07F_UIO_IRQ_WAIT_WIRE 0x80045565UL
#define QC07F_UIO_CACHE_INVALIDATE_WIRE 0x80045566UL
#define QC07F_UIO_NOOP_WIRE 0x80045567UL

struct qc07f_ion_alloc32_wire {
    uint32_t len, align, heap_id_mask, flags, handle;
};
struct qc07f_ion_share32_wire { uint32_t handle; int32_t fd; };
struct qc07f_uio_sysreg32_wire { uint32_t physical_address, value, mask; };
_Static_assert(sizeof(struct qc07f_ion_alloc32_wire)==20,"stock ARM32 alloc ioctl size");
_Static_assert(sizeof(struct qc07f_ion_share32_wire)==8,"stock ARM32 share ioctl size");
_Static_assert(sizeof(struct qc07f_uio_sysreg32_wire)==12,"stock sysreg ioctl size");

/* REA observes align=0x1000, heap mask=0xff, flags=3, mmap of shared fd, and
 * then a hardcoded physical pool base 0x80000000. Size is read from
 * /sys/ak_info_dump/reserved_mem_size. Library suballocations round to 4 bytes.
 * These are NOT a measured reserved-memory map; arbitrary dma-heap/CMA placement
 * is not compatible with the unmodified physical-address calculation.
 * The exported cache helper is a no-op here, not permission to skip DMA cache
 * maintenance on ARMv5. Stock kernel0x80045566 calls l2cache_invalidate and
 * arm926_flush_user_cache_all;0x80045567 succeeds without action. Inspect
 * actual kernel mapping/cache policy first.
 */
#define QC07F_STOCK_LIBRARY_POOL_BASE 0x80000000UL
#endif
