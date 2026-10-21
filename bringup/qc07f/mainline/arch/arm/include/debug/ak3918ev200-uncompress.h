/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded decompressor diagnostics for QC07F; inherits U-Boot UART clocks/mux.
 * The SRAM view is the exact retained REA U-Boot TX literal, not a board GPIO.
 * No flash/reset/pinctrl/controller initialization is performed here.
 */
static inline unsigned int ak_decomp_read(unsigned long address)
{
	return *(volatile unsigned int *)address;
}
static inline void ak_decomp_write(unsigned long address, unsigned int value)
{
	*(volatile unsigned int *)address = value;
}
static inline void putc(int ch)
{
	unsigned int count;
	for (count = 0; count < 1000000; count++)
		if (ak_decomp_read(0x20130004) & (1U << 19))
			break;
	if (count == 1000000)
		return;
	ak_decomp_write(0x2014008c, ak_decomp_read(0x2014008c) | (1U << 16));
	ak_decomp_write(0x48001000, (unsigned char)ch);
	ak_decomp_write(0x4800103c, 0);
	ak_decomp_write(0x20130000, ak_decomp_read(0x20130000) | (1U << 28));
	ak_decomp_write(0x20130004, (ak_decomp_read(0x20130004) & 0x3fe00000) | 0x10010);
	for (count = 0; count < 1000000; count++)
		if (ak_decomp_read(0x20130004) & (1U << 19))
			break;
}
static inline void flush(void) {}
static inline void arch_decomp_setup(void) {}
