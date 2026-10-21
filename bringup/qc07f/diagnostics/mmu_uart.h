/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded diagnostic-only inherited UART0. Before MMU: physical access.
 * After MMU: caller must have the two explicit UART/L2 and SRAM IO mappings.
 */
typedef unsigned int u32;
static inline u32 diag_read(u32 a) { return *(volatile u32 *)a; }
static inline void diag_write(u32 a,u32 v) { *(volatile u32 *)a=v; }
static void diag_put(char ch)
{
	u32 i;
	for(i=0;i<1000000;i++) if(diag_read(0x20130004)&0x80000) break;
	if(i==1000000)return;
	diag_write(0x2014008c,diag_read(0x2014008c)|0x10000);
	diag_write(0x48001000,(unsigned char)ch);diag_write(0x4800103c,0);
	diag_write(0x20130000,diag_read(0x20130000)|0x10000000);
	diag_write(0x20130004,(diag_read(0x20130004)&0x3fe00000)|0x10010);
	for(i=0;i<1000000;i++)if(diag_read(0x20130004)&0x80000)break;
}
static void diag_text(const char *s) { while(*s)diag_put(*s++); }
static void diag_hex(u32 v)
{
	int i;
	for(i=28;i>=0;i-=4)diag_put("0123456789abcdef"[(v>>i)&15]);
}
static __attribute__((noreturn)) void diag_halt(const char *s)
{
	diag_text(s);diag_text("; halted\r\n");for(;;){}
}
