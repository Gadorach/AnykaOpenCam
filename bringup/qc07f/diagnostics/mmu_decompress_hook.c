/* SPDX-License-Identifier: GPL-2.0-only
 * Exact source8 decompressor hook: validate then patch only uncompressed RAM
 * callsites. The source8 input remains non-overlapping at82408040. No SPI/SD
 * write or peripherals beyond inherited UART TX. Contract generated from
 * retained source8 ELF/Image and checked byte-for-byte before modification.
 */
#include "source8_mmu_contract.h"
#include "mmu_uart.h"
static u32 branch(u32 from,u32 to,u32 opcode)
{
	return opcode|(((to-(from+8))>>2)&0xffffff);
}
__attribute__((section(".text.entry")))
void _start(u32 output,u32 heap,u32 limit,u32 arch)
{
	u32 original=(u32)__builtin_return_address(0)+QC6_DECOMP_BIAS;
	diag_text("\r\nQC6 SOURCE8 DECOMP out=");diag_hex(output);diag_text("\r\n");
	((void(*)(u32,u32,u32,u32))original)(output,heap,limit,arch);
	diag_text("QC6 SOURCE8 DECOMP DONE\r\n");
	if(output!=0x81808000 || diag_read(output+8)!=QC6_HEAD_LOOKUP_WORD ||
	   diag_read(output+0x20)!=QC6_HEAD_VET_WORD ||
	   diag_read(output+0x28)!=QC6_HEAD_TABLE_WORD ||
	   diag_read(output+0x40)!=0xe1a0f00c ||
	   diag_read(output+0x44)!=QC6_HEAD_ENABLE_WORD ||
	   diag_read(output+QC6_TURN_OFFSET)!=0xe1a0f003 ||
	   diag_read(output+QC6_START_OFFSET)!=QC6_START_WORD)
		diag_halt("QC6 source8 head mismatch");
	diag_write(output+8,branch(output+8,0x82001000,0xeb000000));
	diag_write(output+0x20,branch(output+0x20,0x82001000,0xeb000000));
	diag_write(output+0x28,branch(output+0x28,0x82001000,0xeb000000));
	diag_write(output+0x40,branch(output+0x40,0x82001000,0xeb000000));
	diag_write(output+0x44,branch(output+0x44,0x82001000,0xeb000000));
	diag_write(output+QC6_TURN_OFFSET,branch(output+QC6_TURN_OFFSET,0x82001000,0xeb000000));
	/* Kernel executes start_kernel through its VA, so use the high debug alias. */
	diag_write(output+QC6_START_OFFSET,branch(QC6_START_VIRTUAL,0xc0801000,0xea000000));
	diag_text("QC6 SOURCE8 MMU CHECKPOINTS INSTALLED\r\n");
}
