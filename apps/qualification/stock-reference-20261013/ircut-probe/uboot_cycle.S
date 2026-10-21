/* SPDX-License-Identifier: GPL-2.0-only
 * AAPCS ARMv5, preserves r4-r11 including original U-Boot r9.
 * r0=fixed OUT0 address, r1=neutral saved OUT0, r2=admitted positive count,
 * r3=0 for inert calibration, otherwise fixed 0x40 or0x80.
 * Caller already masked IRQ+FIQ. No stack, calls, clock reads or error paths.
 * Calibration and drive use EXACTLY the same aligned hot-loop instructions.
 */
.syntax unified
.arm
.text
.balign 32
.global uboot_cycle
.type uboot_cycle,%function
uboot_cycle:
    cmp r3, #0
    orr ip, r1, r3
.global uboot_assert_site
uboot_assert_site:
    strne ip, [r0]
    ldrne ip, [r0]
    b uboot_hot_loop
.balign 32
.global uboot_hot_loop
uboot_hot_loop:
    subs r2, r2, #1
    bne uboot_hot_loop
    cmp r3, #0
.global uboot_release_site
uboot_release_site:
    strne r1, [r0]
    ldrne r1, [r0]
    mov r0, ip
    bx lr
.size uboot_cycle,.-uboot_cycle
.section .note.GNU-stack,"",%progbits
