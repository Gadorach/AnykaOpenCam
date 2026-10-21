// SPDX-License-Identifier: GPL-2.0-or-later
/* UART-first experimental driver. Register/L2 protocol derives from the GPL
 * Anyka 3.4.35 ak39_uart.c. Keeps bootloader mux, clocks and baud divider.
 * Polled RX and synchronous TX are bring-up aids, not a production UART driver.
 */
#include <linux/console.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/serial_core.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <asm/fixmap.h>
#include "io.h"

#define CONF1 0
#define CONF2 4
#define DATA 8
#define THRESH 12
#define INT_MASK 0x3fe00000
static void __iomem *regs = (void __iomem *)AK_IO_UART_VA;
static void __iomem *l2 = (void __iomem *)AK_IO_L2_VA;
static void __iomem *buffer = (void __iomem *)AK_IO_UART_BUFFER_VA;
static DEFINE_RAW_SPINLOCK(tx_lock);
static struct tty_port ttyport;
static struct tty_driver *ttydrv;
static struct timer_list rx_timer;
static unsigned int rxpos;
static u32 rx_errors,rx_mem_busy,last_rx_status;

static bool ak_putchar(char ch)
{
	u32 value;
	unsigned long flags;
	if (!regs || !l2 || !buffer)
		return false;
	raw_spin_lock_irqsave(&tx_lock, flags);
	/* Bounded poll: a bad map/disabled UART must not hard-lock printk. */
	/* Retail console_send kicks TX after its bounded idle wait even if TX_END
	 * stayed clear. CONF2 initialization can clear that indication; refusing
	 * to start a first transfer would then suppress every later console byte.
	 */
	readl_poll_timeout_atomic(regs + CONF2, value, value & BIT(19), 1, 10000);
	writel(readl(regs + CONF1) | BIT(28), regs + CONF1);
	writel(readl(l2 + 0x8c) | BIT(16), l2 + 0x8c);
	writel((u8)ch, buffer);
	writel(0, buffer + 0x3c);
	writel(readl(regs + THRESH) | BIT(11), regs + THRESH);
	writel((readl(regs + CONF2) & INT_MASK) | BIT(4) | BIT(16), regs + CONF2);
	if (readl_poll_timeout_atomic(regs + CONF2, value, value & BIT(19), 1, 10000)) {
		raw_spin_unlock_irqrestore(&tx_lock, flags);
		return false;
	}
	raw_spin_unlock_irqrestore(&tx_lock, flags);
	return true;
}
static void ak_print(const char *s, unsigned int count)
{
	while (count--) {
		if (*s == '\n')
			ak_putchar('\r');
		ak_putchar(*s++);
	}
}
static int ak_map_uart(void)
{
	if (regs)
		return 0;
	regs = ioremap(0x20130000, 0x100);
	l2 = ioremap(0x20140000, 0x100);
	buffer = ioremap(0x4800d000, 0x100);
	return regs && l2 && buffer ? 0 : -ENOMEM;
}
static void ak_early_write(struct console *co, const char *s, unsigned int count)
{
	ak_print(s, count);
}
static int __init ak_early_setup(struct earlycon_device *dev, const char *options)
{
	/* All three resources have bootstrap section mappings in ARM head,
	 * retained by map_io. No early fixmap lifetime or pre-paging ioremap.
	 */
	dev->port.membase = regs;
	dev->con->write = ak_early_write;
	return 0;
}
OF_EARLYCON_DECLARE(ak39e2, "anyka,ak3918ev200-uart", ak_early_setup);

/* Diagnostic progress before normal console registration. Direct bounded
 * UART TX only, using the permanent bootstrap mappings. No GPIO/clock reset.
 * Keep this distinct from a production serial-core UART implementation.
 */
void __init ak39e2_boot_marker(unsigned int stage)
{
	const char *prefix = "\r\nQC6 BOOT STAGE ";
	/* Use the exact U-Boot TX view proven under enabled MMU by diagnostics.
	 * Keep the normal retail RX/TX view separate until independently tested.
	 */
	void __iomem *boot_buffer = (void __iomem *)(AK_IO_SRAM_VA + 0x1000);
	char text[24];
	unsigned int n = 0, i, retry;
	u32 value;
	unsigned long flags;

	while (*prefix)
		text[n++] = *prefix++;
	if (stage >= 10)
		text[n++] = '0' + stage / 10;
	text[n++] = '0' + stage % 10;
	text[n++] = '\r'; text[n++] = '\n';
	local_irq_save(flags);
	for (i = 0; i < n; i++) {
		for (retry = 0; retry < 1000000; retry++)
			if (readl(regs + CONF2) & BIT(19))
				break;
		writel(readl(l2 + 0x8c) | BIT(16), l2 + 0x8c);
		writel((u8)text[i], boot_buffer);
		writel(0, boot_buffer + 0x3c);
		writel(readl(regs + CONF1) | BIT(28), regs + CONF1);
		value = (readl(regs + CONF2) & INT_MASK) | BIT(4) | BIT(16);
		writel(value, regs + CONF2);
		for (retry = 0; retry < 1000000; retry++)
			if (readl(regs + CONF2) & BIT(19))
				break;
	}
	local_irq_restore(flags);
}

static void ak_rx(struct timer_list *timer)
{
	u32 value, head;
	unsigned int copied = 0;
	value = readl(regs + CONF2);
	last_rx_status = value;
	/* Anyka GPL ak39_uart.c uart_subint_clear(RECVDATA_ERR_INT):
	 * acknowledge receive error (CONF2 bit3,W1C) before testing MEM_RDY.
	 * MEM_RDY bit17 is read-only; never invent a reset/write to that bit.
	 * RX errors mean bytes may be lost, not a successful complete acquisition.
	 */
	if (value & BIT(3)) {
		writel((value & INT_MASK) | BIT(3), regs + CONF2);
		rx_errors++;
		value = readl(regs + CONF2);
	}
	if (value & BIT(17)) {
		rx_mem_busy++;
		goto rearm;
	}
	head = (readl(regs + DATA) >> 13) & 0x7f;
	while (rxpos != head && copied < 128) {
		tty_insert_flip_char(&ttyport, readb(buffer + 0x80 + rxpos), TTY_NORMAL);
		rxpos = (rxpos + 1) & 0x7f;
		copied++;
	}
	if (copied)
		tty_flip_buffer_push(&ttyport);
	value = readl(regs + CONF2);
	writel((value & INT_MASK) | BIT(30) | BIT(2), regs + CONF2);
	writel(readl(regs + THRESH) | BIT(31), regs + THRESH);
rearm:
	mod_timer(&rx_timer, jiffies + 1);
}
static int ak_open(struct tty_struct *tty, struct file *file)
{
	return tty_port_open(&ttyport, tty, file);
}
static void ak_close(struct tty_struct *tty, struct file *file)
{
	tty_port_close(&ttyport, tty, file);
}
static ssize_t ak_write(struct tty_struct *tty, const u8 *buf, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++)
		if (!ak_putchar(buf[i]))
			break;
	return i;
}
static unsigned int ak_room(struct tty_struct *tty) { return 128; }
static const struct tty_operations ak_tty_ops = {
	.open = ak_open, .close = ak_close, .write = ak_write, .write_room = ak_room,
};
/* tty_port_open dereferences ops even when no activate callback is required. */
static const struct tty_port_operations ak_port_ops = { };
static ssize_t rx_state_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	return sysfs_emit(buf,"rx_error_events=%u mem_busy_ticks=%u last_status=%08x rxpos=%u; cached diagnostic, lost bytes cannot be reconstructed\n",READ_ONCE(rx_errors),READ_ONCE(rx_mem_busy),READ_ONCE(last_rx_status),READ_ONCE(rxpos));
}
static DEVICE_ATTR_RO(rx_state);
static struct tty_driver *ak_console_device(struct console *co, int *index)
{
	*index = 0;
	return ttydrv;
}
static void ak_console_write(struct console *co, const char *s, unsigned int count)
{
	ak_print(s, count);
}
static struct console ak_console = {
	.name = "ttyAK", .index = 0, .flags = CON_PRINTBUFFER,
	.write = ak_console_write, .device = ak_console_device,
};
static int __init ak_uart_init(void)
{
	struct device *dev;
	int ret;
	if (!of_machine_is_compatible("anyka,qc07f"))
		return -ENODEV;
	ret = ak_map_uart();
	if (ret)
		return ret;
	tty_port_init(&ttyport);
	ttyport.ops = &ak_port_ops;
	ttydrv = tty_alloc_driver(1, TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV);
	if (IS_ERR(ttydrv))
		return PTR_ERR(ttydrv);
	ttydrv->driver_name = "ak39e2-bringup";
	ttydrv->name = "ttyAK";
	ttydrv->type = TTY_DRIVER_TYPE_SERIAL;
	ttydrv->subtype = SERIAL_TYPE_NORMAL;
	ttydrv->init_termios = tty_std_termios;
	ttydrv->init_termios.c_cflag = B115200 | CS8 | CREAD | CLOCAL;
	ttydrv->init_termios.c_ispeed = 115200;
	ttydrv->init_termios.c_ospeed = 115200;
	tty_set_operations(ttydrv, &ak_tty_ops);
	tty_port_link_device(&ttyport, ttydrv, 0);
	ret = tty_register_driver(ttydrv);
	if (ret)
		return ret;
	dev = tty_port_register_device(&ttyport, ttydrv, 0, NULL);
	if (IS_ERR(dev))
		return PTR_ERR(dev);
	ret = device_create_file(dev, &dev_attr_rx_state);
	if (ret)
		return ret;
	writel(readl(l2 + 0x84) | BIT(29), l2 + 0x84);
	writel(readl(l2 + 0x8c) | BIT(17), l2 + 0x8c);
	writel(readl(regs + CONF1) | BIT(21) | BIT(23) | BIT(29), regs + CONF1);
	writel(0, regs + CONF2); /* RX polling; no UART IRQ or modem control */
	rxpos = (readl(regs + DATA) >> 13) & 0x7f;
	timer_setup(&rx_timer, ak_rx, 0);
	mod_timer(&rx_timer, jiffies + 1);
	register_console(&ak_console);
	pr_info("ak39e2: experimental polling ttyAK0; inherited baud/mux/clock\n");
	return 0;
}
device_initcall(ak_uart_init);
