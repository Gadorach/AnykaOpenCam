/* SPDX-License-Identifier: GPL-2.0-only
 * RAM-only compressed-kernel call hook; read/write only the inherited UART.
 * The original ARM BL callsite is0x2bc, LR points at0x2c0, and the original
 * decompress_kernel is at0x964 in the exact attempt5 compressed ELF. An
 * in-image absolute trampoline is used only for the non-overlapping staged
 * input/output experiment. The initial nop area is not copied on relocation;
 * this diagnostic is NOT qualified for a self-relocating decompressor.
 * Link at0x83f00000, outside both80008000 and81808000 kernel output windows.
 */
typedef unsigned int u32;
static inline u32 read32(u32 address) { return *(volatile u32 *)address; }
static inline void write32(u32 address, u32 value) { *(volatile u32 *)address = value; }
static void put(char ch)
{
	u32 i;
	for (i=0; i<1000000; i++)
		if (read32(0x20130004)&0x80000)
			break;
	if (i==1000000)
		return;
	write32(0x2014008c, read32(0x2014008c)|0x10000);
	write32(0x48001000, (unsigned char)ch);
	write32(0x4800103c, 0);
	write32(0x20130000, read32(0x20130000)|0x10000000);
	write32(0x20130004, (read32(0x20130004)&0x3fe00000)|0x10010);
	for (i=0; i<1000000; i++)
		if (read32(0x20130004)&0x80000)
			break;
}
static void text(const char *s) { while (*s) put(*s++); }
static void hex(u32 value)
{
	int shift;
	for (shift=28; shift>=0; shift-=4)
		put("0123456789abcdef"[(value>>shift)&15]);
}
__attribute__((section(".text.entry")))
void _start(u32 output, u32 heap, u32 limit, u32 arch)
{
	u32 cpu, cpsr;
	u32 original = (u32)__builtin_return_address(0) + 0x964 - 0x2c0;
	__asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(cpu));
	__asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
	text("\r\nQC6 DECOMP BEGIN output="); hex(output);
	text(" heap="); hex(heap); text(" limit="); hex(limit);
	text(" cpu="); hex(cpu); text(" cpsr="); hex(cpsr);
	text(" fn="); hex(original); text("\r\n");
	((void (*)(u32,u32,u32,u32))original)(output, heap, limit, arch);
	text("QC6 DECOMP DONE\r\n");
#ifdef QC6_HEAD_CHECKPOINTS
	/* Exact retained attempt5 ARM head. Test matches before changing RAM. */
	if (output != 0x81808000 || read32(output+8) != 0xeb00280c ||
	    read32(output+0x20) != 0xeb000046 ||
	    read32(output+0x28) != 0xeb000007 || read32(output+0x40) != 0xe1a0f00c) {
		text("QC6 unexpected kernel head; halted\r\n");
		for (;;) {}
	}
	write32(output+8,0xeb000000|(((0x82001000-(output+8+8))>>2)&0xffffff));
	write32(output+0x20,0xeb000000|(((0x82001000-(output+0x20+8))>>2)&0xffffff));
	write32(output+0x28,0xeb000000|(((0x82001000-(output+0x28+8))>>2)&0xffffff));
	write32(output+0x40,0xeb000000|(((0x82001000-(output+0x40+8))>>2)&0xffffff));
	text("QC6 kernel head checkpoints installed\r\n");
#endif
}
