/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded RAM-only UART SRAM-view probe. Called using U-Boot go; inherits
 * mux/baud/clocks. Does not touch SPI, GPIO, reset or interrupt configuration.
 * Addresses: retained REA U-Boot uart transmit literals and retail port table.
 */
typedef unsigned int u32;
static inline u32 load(u32 addr) { return *(volatile u32 *)addr; }
static inline void store(u32 addr, u32 value) { *(volatile u32 *)addr = value; }
static int put(u32 buffer, char ch)
{
	u32 i;
	for (i = 0; i < 1000000; i++)
		if (load(0x20130004) & (1U << 19))
			break;
	if (i == 1000000)
		return -1;
	store(0x2014008c, load(0x2014008c) | (1U << 16));
	store(buffer, (unsigned char)ch);
	store(buffer + 0x3c, 0);
	store(0x20130000, load(0x20130000) | (1U << 28));
	store(0x20130004, (load(0x20130004) & 0x3fe00000) | 0x10010);
	for (i = 0; i < 1000000; i++)
		if (load(0x20130004) & (1U << 19))
			return 0;
	return -1;
}
static int emit(u32 buffer, const char *text)
{
	while (*text)
		if (put(buffer, *text++))
			return -1;
	return 0;
}
__attribute__((section(".text.entry")))
int _start(int argc, char **argv)
{
	int first, second;
	first = emit(0x48001000, "\r\nQC6 UART view 48001000\r\n");
	second = emit(0x4800d000, "QC6 UART view 4800d000\r\n");
	emit(0x48001000, "QC6 UART probe returning\r\n");
	return first || second;
}
