/* SPDX-License-Identifier: GPL-2.0-only
 * Exact source8 RAM-only CPU/MMU/start_kernel checkpoints. No peripheral
 * initialization: inherited UART TX only. Four bootstrap section entries
 * are added to the existing PGD, using the selected CPU's RAM/IO flags.
 * The low identity debug mappings are transient until paging_init; no hook
 * is invoked after the one-time start_kernel entry checkpoint.
 * Private physical scratch stack avoids SP's pre-MMU virtual continuation.
 */
#include "source8_mmu_contract.h"
#include "mmu_uart.h"
__asm__(
".section .text.entry,\"ax\",%progbits\n"
".global _start\n.type _start,%function\n_start:\n"
"str sp, .Lsaved_sp\nldr sp, .Lscratch_sp\n"
"stmdb sp!, {r0-r12, lr}\nmrs r12, cpsr\nsub sp, sp, #8\n"
"str r12, [sp]\nmov r0, sp\nbl mmu_report\n"
"str r0, .Ltail_target\nldr r12, [sp]\nmsr cpsr_f, r12\n"
"add sp, sp, #8\nldmia sp!, {r0-r12, lr}\n"
"ldr sp, .Lsaved_sp\nldr pc, .Ltail_target\n"
".Lsaved_sp: .word 0\n.Ltail_target: .word 0\n.Lscratch_sp: .word 0x82004000\n"
".size _start, .-_start\n.text\n");

static void bootstrap_maps(const u32 *frame)
{
	u32 table=frame[6], cpu=frame[12], ram, io, i;
	const u32 va[4]={0x82000000,0xc0800000,0x20100000,0x48000000};
	const u32 pa[4]={0x82000000,0x82000000,0x20100000,0x48000000};
	if(table!=0x81804000 || cpu<0x81800000 || cpu>0x83fffff0)
		diag_halt("QC6 unexpected PGD/CPU descriptor");
	ram=diag_read(cpu+8);io=diag_read(cpu+12);
	if(ram!=0xc1e || io!=0xc12)
		diag_halt("QC6 unexpected ARM926 section flags");
	for(i=0;i<4;i++){
		u32 slot=table+(va[i]>>20)*4, value=pa[i]|(i<2?ram:io);
		if(diag_read(slot)!=0 && diag_read(slot)!=value)
			diag_halt("QC6 debug map conflicts with existing PGD");
		diag_write(slot,value);
	}
	diag_text("QC6 DEBUG MAPS pgd=");diag_hex(table);
	diag_text(" id=");diag_hex(diag_read(table+(0x81800000>>20)*4));
	diag_text(" linear=");diag_hex(diag_read(table+(0xc0000000>>20)*4));
	diag_text("\r\n");
}
u32 mmu_report(const u32 *frame)
{
	u32 control, target=0, caller=(u32)__builtin_return_address(0);
	__asm__ volatile("mrc p15, 0, %0, c1, c0, 0":"=r"(control));
	if((frame[0]&0xc0)!=0xc0)diag_halt("QC6 diagnostic requires masked IRQ/FIQ");
	diag_text("QC6 MMU lr=");diag_hex(frame[15]);
	diag_text(" ctrl=");diag_hex(control);diag_text(" r0=");diag_hex(frame[2]);
	diag_text(" r4=");diag_hex(frame[6]);diag_text("\r\n");
	/* Only the start_kernel hook enters through the high linear debug alias.
	 * It replaces PUSH with B, preserving the original LR; the tail trampoline
	 * executes that original PUSH before continuing at start_kernel+4.
	 */
	if((control&1) && caller>=0xc0801000 && caller<0xc0802000){
		diag_text("QC6 START_KERNEL ENTERED\r\n");return 0xc0802000;
	}
	switch(frame[15]){
	case 0x8180800c:target=QC6_LOOKUP_TARGET;break;
	case 0x81808024:target=QC6_VET_TARGET;break;
	case 0x8180802c:target=QC6_TABLE_TARGET;break;
	case 0x81808044:
		if(control&1)diag_halt("QC6 bootstrap maps require MMU off");
		bootstrap_maps(frame);target=frame[14];break;
	case 0x81808048:
		diag_text("QC6 CPU SETUP RETURNED\r\n");target=QC6_ENABLE_TARGET;break;
	case QC6_TURN_RETURN:
		if(!(control&1))diag_halt("QC6 MMU did not enable");
		diag_text("QC6 MMU ENABLED\r\n");target=frame[5];break;
	default:diag_halt("QC6 unexpected MMU checkpoint");
	}
	return target;
}
