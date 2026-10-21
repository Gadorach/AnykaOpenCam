/* SPDX-License-Identifier: GPL-2.0-only
 * RAM-only ARM head checkpoints for the exact attempt5 kernel at81808000.
 * Called at CPU lookup, atag vetting, page tables and CPU MMU setup. Preserve
 * all registers/flags, print through the inherited UART, then tail-call the
 * original routine. Head uses SP as a VIRTUAL continuation address before
 * enabling the MMU (not an accessible stack at that phase), so
 * every hook saves the original SP in private RAM and uses an explicit physical
 * scratch stack82003000..82004000. Interrupts stay masked; hooks are not nested.
 * No clock/reset/GPIO/SPI configuration is performed.
 */
typedef unsigned int u32;
__asm__(
".section .text.entry,\"ax\",%progbits\n"
".global _start\n"
".type _start,%function\n"
"_start:\n"
"str sp, .Lsaved_sp\n"
"ldr sp, .Lscratch_sp\n"
"stmdb sp!, {r0-r12, lr}\n"
"mrs r12, cpsr\n"
"sub sp, sp, #8\n"
"str r12, [sp]\n"
"mov r0, sp\n"
"bl head_report\n"
"str r0, .Ltail_target\n"
"ldr r12, [sp]\n"
"msr cpsr_f, r12\n"
"add sp, sp, #8\n"
"ldmia sp!, {r0-r12, lr}\n"
"ldr sp, .Lsaved_sp\n"
"ldr pc, .Ltail_target\n"
".Lsaved_sp: .word 0\n"
".Ltail_target: .word 0\n"
".Lscratch_sp: .word 0x82004000\n"
".size _start, .-_start\n"
".text\n");
static inline u32 read32(u32 a) { return *(volatile u32 *)a; }
static inline void write32(u32 a,u32 v) { *(volatile u32 *)a=v; }
static void put(char ch)
{
	u32 i;
	for (i=0;i<1000000;i++) if (read32(0x20130004)&0x80000) break;
	if (i==1000000) return;
	write32(0x2014008c,read32(0x2014008c)|0x10000);
	write32(0x48001000,(unsigned char)ch); write32(0x4800103c,0);
	write32(0x20130000,read32(0x20130000)|0x10000000);
	write32(0x20130004,(read32(0x20130004)&0x3fe00000)|0x10010);
	for (i=0;i<1000000;i++) if (read32(0x20130004)&0x80000) break;
}
static void text(const char *s) { while (*s) put(*s++); }
static void hex(u32 v)
{
	int i;
	for (i=28;i>=0;i-=4) put("0123456789abcdef"[(v>>i)&15]);
}
u32 head_report(const u32 *frame)
{
	u32 target=0, control;
	__asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(control));
	text("QC6 HEAD lr=");hex(frame[15]);text(" cpu=");hex(frame[11]);
	text(" r2=");hex(frame[4]);text(" r5=");hex(frame[7]);
	text(" sctlr=");hex(control);
	if (frame[4]>=0x81800000 && frame[4]<=0x83fffffc) {
		text(" tag0=");hex(read32(frame[4]));
	}
	text("\r\n");
	switch (frame[15]) {
	case 0x8180800c: target=0x81812040;break; /* __lookup_processor_type */
	case 0x81808024: target=0x81808140;break; /* __vet_atags */
	case 0x8180802c: target=0x8180804c;break; /* __create_page_tables */
	case 0x81808044: target=frame[14];break; /* discovered CPU init callback */
	default:
		text("QC6 HEAD unexpected return address; halted\r\n");
		for (;;) {}
	}
	return target;
}
