/* SPDX-License-Identifier: GPL-2.0-only
 * Bootstrap mappings installed by ARM head and retained through map_io.
 * UART/L2 share a1MiB controller section; UART SRAM is a separate IO section.
 * These are address mappings, not GPIO/clock/peripheral initialization.
 */
#ifndef AK3918EV200_BOOTSTRAP_IO_H
#define AK3918EV200_BOOTSTRAP_IO_H
#include <linux/init.h>
#define AK_IO_CONTROLLER_VA 0xfef00000
#define AK_IO_CONTROLLER_PA 0x20100000
#define AK_IO_SRAM_VA 0xfee00000
#define AK_IO_SRAM_PA 0x48000000
#define AK_IO_UART_VA (AK_IO_CONTROLLER_VA + 0x30000)
#define AK_IO_L2_VA (AK_IO_CONTROLLER_VA + 0x40000)
#define AK_IO_UART_BUFFER_VA (AK_IO_SRAM_VA + 0xd000)
void __init ak39e2_boot_marker(unsigned int stage);
#endif
