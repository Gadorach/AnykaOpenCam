/* SPDX-License-Identifier: GPL-2.0-only
 * REA-observed retail ARM32 sensor-load ABI, reference only, no live I/O.
 */
#ifndef QC07F_STOCK_ISP_SENSOR_H
#define QC07F_STOCK_ISP_SENSOR_H
#include <stdint.h>
#define QC07F_ISP_SENSOR_LOAD_WIRE 0x40044969UL
struct qc07f_sensor_load32_wire {
    uint32_t register_pair_count;
    uint32_t user_pointer32;
};
struct qc07f_sensor_register_pair { uint16_t address, value; };
_Static_assert(sizeof(struct qc07f_sensor_load32_wire)==8,"actual copied descriptor size");
_Static_assert(sizeof(struct qc07f_sensor_register_pair)==4,"sensor table entry size");
/* Stock akisp_ioctl copies8 descriptor bytes despite request encoding4, then
 * allocates/copies (count & 0xffff)*4 table bytes. sc1346_init uses that low16
 * count for register writes. Both retained config packs have139 entries and
 * 556 payload bytes, without the4-byte config-subpack header. Do not add an
 * extra entry based on a different upstream SDK wrapper's convention.
 * Count and pointer interpretation should still be checked against ARM
 * disassembly/runtime traces before implementing device ioctl compatibility.
 */
#endif
