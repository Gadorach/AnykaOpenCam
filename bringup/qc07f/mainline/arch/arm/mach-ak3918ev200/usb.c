// SPDX-License-Identifier: GPL-2.0-or-later
/* Anyka indexed MUSB host bootstrap glue. Controller map/PHY sequence comes
 * from GPL AK39 usb-reg-define.h/usb-hc.h and retained retail REA evidence.
 * Reuse upstream USB scheduler, EP0 and PIO handling; no Anyka/L2 DMA, external
 * VBUS switch, radio firmware upload or Wi-Fi configuration. An explicitly
 * described retail GPIO42 power/reset sequence may enable the onboard radio.
 * Register compatibility, IRQ routing and enumeration require target tests.
 */
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/gpio/consumer.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/usb/musb.h>
#include <linux/usb/phy.h>
#include <linux/usb/usb_phy_generic.h>
#include "../../../drivers/usb/musb/musb_core.h"
#include "../../../drivers/usb/musb/musb_host.h"

struct ak_usb {
	struct regmap *sys;
	struct platform_device *musb, *nop;
	struct gpio_desc *radio_power;
};
static irqreturn_t ak_usb_interrupt(int irq, void *data)
{
	struct musb *musb = data;
	unsigned long flags;
	irqreturn_t result = IRQ_NONE;

	spin_lock_irqsave(&musb->lock, flags);
	/* This controller's interrupt registers are read-clear. Read exactly
	 * once, then let upstream handle the saved status and endpoint state.
	 */
	musb->int_usb = musb_readb(musb->mregs, MUSB_INTRUSB);
	musb->int_tx = musb_readw(musb->mregs, MUSB_INTRTX);
	musb->int_rx = musb_readw(musb->mregs, MUSB_INTRRX);
	if (musb->int_usb || musb->int_tx || musb->int_rx)
		result = musb_interrupt(musb);
	spin_unlock_irqrestore(&musb->lock, flags);
	return result;
}
static void ak_usb_vbus(struct musb *musb, int on)
{
	u8 value = musb_readb(musb->mregs, MUSB_DEVCTL);

	/* Session signalling only. No external power-switch GPIO is assumed. */
	if (on) {
		value |= MUSB_DEVCTL_SESSION;
		MUSB_HST_MODE(musb);
		musb->is_active = 1;
		musb->xceiv->otg->state = OTG_STATE_A_WAIT_VRISE;
	} else {
		value &= ~MUSB_DEVCTL_SESSION;
		musb->is_active = 0;
	}
	musb_writeb(musb->mregs, MUSB_DEVCTL, value);
}
static int ak_usb_mode(struct musb *musb, u8 mode)
{
	return mode == MUSB_HOST ? 0 : -EOPNOTSUPP;
}
static void ak_usb_enable(struct musb *musb)
{
	u8 power = musb_readb(musb->mregs, MUSB_POWER);
	u32 ep;
	/* Retail Anyka maps (device endpoint,direction) to distinct FIFO slots
	 * and never flips a live shared slot between OUT/IN. Generic scheduling
	 * otherwise chooses the same free slot for successive bulk directions.
	 * Restrict software eligibility, not the measured fixed FIFO sizes or
	 * hardware register layout: even nonreserved slots OUT, odd slots IN.
	 */
	for (ep = 2; ep < musb->nr_endpoints; ep++) {
		if (ep & 1)
			musb->endpoints[ep].max_packet_sz_tx = 0;
		else
			musb->endpoints[ep].max_packet_sz_rx = 0;
	}
	/* Retail akotg_usbhc_start writes21, unlike generic MUSB's A0.
	 * This owned onboard radio needs no isochronous scheduling.
	 */
	power &= ~MUSB_POWER_ISOUPDATE;
	power |= MUSB_POWER_ENSUSPEND;
	musb_writeb(musb->mregs, MUSB_POWER, power);
}
static int ak_usb_init(struct musb *musb)
{
	struct ak_usb *usb = dev_get_drvdata(musb->controller->parent);
	u32 before, after;
	int ret;

	musb->xceiv = usb_get_phy(USB_PHY_TYPE_USB2);
	if (IS_ERR_OR_NULL(musb->xceiv))
		/* Built-in NOP PHY registration can follow the MUSB child driver. */
		return -EPROBE_DEFER;
	ret = regmap_read(usb->sys, 0x58, &before);
	if (ret)
		goto fail;
	ret = regmap_update_bits(usb->sys, 0x1c, BIT(15), 0);
	if (ret)
		goto fail;
	ret = regmap_update_bits(usb->sys, 0x20, BIT(15), BIT(15));
	if (ret)
		goto fail;
	usleep_range(125000, 130000);
	ret = regmap_update_bits(usb->sys, 0x20, BIT(15), 0);
	if (ret)
		goto fail;
	/* Retail start: (op58 & ffffc038) |17c6, including PHY bits1/2.
	 * Older AK39 BSP only17c0 is insufficient for this retail generation.
	 * No PLL divider or unrelated SYSCTRL bits are overwritten.
	 */
	ret = regmap_update_bits(usb->sys, 0x58, GENMASK(13, 6) | GENMASK(2, 0) | BIT(14),
				 0x17c6 | BIT(14));
	if (ret)
		goto fail;
	usleep_range(1000, 2000);
	ret = regmap_update_bits(usb->sys, 0x58, BIT(14) | BIT(0), 0);
	if (ret)
		goto fail;
	msleep(5);
	ret = regmap_read(usb->sys, 0x58, &after);
	if (ret)
		goto fail;
	musb->isr = ak_usb_interrupt;
	dev_info(musb->controller, "Anyka indexed USB PIO bootstrap op58 %08x..%08x power=%02x devctl=%02x\n",
		 before, after, musb_readb(musb->mregs, MUSB_POWER),
		 musb_readb(musb->mregs, MUSB_DEVCTL));
	return 0;
fail:
	usb_put_phy(musb->xceiv);
	musb->xceiv = NULL;
	return ret;
}
static int ak_usb_exit(struct musb *musb)
{
	usb_put_phy(musb->xceiv);
	return 0;
}
static u16 ak_usb_get_toggle(struct musb_qh *qh, int is_out)
{
	return is_out ? qh->hw_ep->anyka_tx_toggle : qh->hw_ep->anyka_rx_toggle;
}
static u16 ak_usb_set_toggle(struct musb_qh *qh, int is_out, struct urb *urb)
{
	struct musb_hw_ep *ep = qh->hw_ep;
	u8 *shadow = is_out ? &ep->anyka_tx_toggle : &ep->anyka_rx_toggle;

	/* Only the retail-supported CLRDATATOG bit is writable. Directional
	 * FIFO assignment preservesDATA1; no reserved WR_DATATOGGLE bits.
	 */
	if (usb_gettoggle(urb->dev, qh->epnum, is_out)) {
		WARN_ON_ONCE(!*shadow);
		return 0;
	}
	*shadow = 0;
	return is_out ? MUSB_TXCSR_CLRDATATOG : MUSB_RXCSR_CLRDATATOG;
}
static void ak_usb_write_fifo(struct musb_hw_ep *ep, u16 len, const u8 *buf)
{
	/* Match retail PIO access width; never route through L2/DMA. */
	/* Retail usb-hc.c fill_epfifo clearsTXCSR1 before EVERY PIO packet,
	 * including the next packet of an existing URB. Keep CSR2/toggle intact.
	 */
	if (ep->epnum)
		musb_writeb(ep->regs, MUSB_TXCSR, 0);
	while (len--)
		writeb(*buf++, ep->fifo);
}
static void ak_usb_read_fifo(struct musb_hw_ep *ep, u16 len, u8 *buf)
{
	while (len--)
		*buf++ = readb(ep->fifo);
}
static u16 ak_usb_readw(void __iomem *addr, u32 offset)
{
	return readb(addr + offset) | (readb(addr + offset + 1) << 8);
}
static void ak_usb_writew(void __iomem *addr, u32 offset, u16 value)
{
	/* Retail programs CSR2 separately from CSR1. Set the mode/high byte
	 * before a low-byte TXPKTRDY/REQPKT can start the transaction.
	 */
	writeb(value >> 8, addr + offset + 1);
	writeb(value, addr + offset);
}
static const struct musb_platform_ops ak_usb_ops = {
	.quirks = MUSB_INDEXED_EP | MUSB_ANYKA_PIO_TOGGLE,
	.get_toggle = ak_usb_get_toggle, .set_toggle = ak_usb_set_toggle,
	.write_fifo = ak_usb_write_fifo, .read_fifo = ak_usb_read_fifo,
	.readw = ak_usb_readw, .writew = ak_usb_writew,
	.init = ak_usb_init, .exit = ak_usb_exit,
	.enable = ak_usb_enable,
	.set_mode = ak_usb_mode, .set_vbus = ak_usb_vbus,
};
static const struct musb_fifo_cfg ak_usb_fifos[] = {
	MUSB_EP_FIFO_SINGLE(1, FIFO_RXTX, 512),
	MUSB_EP_FIFO_SINGLE(2, FIFO_RXTX, 512),
	MUSB_EP_FIFO_SINGLE(3, FIFO_RXTX, 512),
	MUSB_EP_FIFO_SINGLE(4, FIFO_RXTX, 512),
	MUSB_EP_FIFO_SINGLE(5, FIFO_RXTX, 512),
};
static const struct musb_hdrc_config ak_usb_config = {
	.num_eps = 6, .ram_bits = 10, .multipoint = 0,
	.fifo_cfg = ak_usb_fifos, .fifo_cfg_size = ARRAY_SIZE(ak_usb_fifos),
};
static void ak_usb_unregister(void *data)
{
	platform_device_unregister(data);
}
static ssize_t registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ak_usb *usb = dev_get_drvdata(dev);
	struct musb *musb = platform_get_drvdata(usb->musb);
	u32 op, gate;
	u8 power, devctl, irq_enable;
	u16 frame, tx_enable, rx_enable;
	unsigned long flags;

	if (!musb)
		return -ENODEV;
	if (regmap_read(usb->sys, 0x58, &op) || regmap_read(usb->sys, 0x1c, &gate))
		return -EIO;
	spin_lock_irqsave(&musb->lock, flags);
	power = musb_readb(musb->mregs, MUSB_POWER);
	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
	frame = musb_readw(musb->mregs, MUSB_FRAME);
	irq_enable = musb_readb(musb->mregs, MUSB_INTRUSBE);
	tx_enable = musb_readw(musb->mregs, MUSB_INTRTXE);
	rx_enable = musb_readw(musb->mregs, MUSB_INTRRXE);
	spin_unlock_irqrestore(&musb->lock, flags);
	/* Never read the read-clear interrupt status or change endpoint INDEX. */
	return sysfs_emit(buf, "op58=%08x gate1c=%08x power=%02x devctl=%02x frame=%04x irq_enable=%02x tx_enable=%04x rx_enable=%04x hwvers=%04x dynfifo=%u\n",
		op, gate, power, devctl, frame, irq_enable, tx_enable, rx_enable,
		musb->hwvers, musb->dyn_fifo);
}
static DEVICE_ATTR_RO(registers);
static ssize_t endpoints_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ak_usb *usb = dev_get_drvdata(dev);
	struct musb *musb = platform_get_drvdata(usb->musb);
	unsigned long flags;
	u32 ep, used = 0;
	u8 index;

	if (!musb)
		return -ENODEV;
	spin_lock_irqsave(&musb->lock, flags);
	index = musb_readb(musb->mregs, MUSB_INDEX);
	for (ep = 1; ep < musb->nr_endpoints; ep++) {
		struct musb_hw_ep *hw = &musb->endpoints[ep];
		musb_ep_select(musb->mregs, ep);
		used += sysfs_emit_at(buf, used,
			"ep%u fifo_size=%02x max_tx=%u max_rx=%u txcsr=%04x rxcsr=%04x txtype=%02x rxtype=%02x shadow_tx=%u shadow_rx=%u\n",
			ep, musb_readb(hw->regs, MUSB_FIFOSIZE), hw->max_packet_sz_tx,
			hw->max_packet_sz_rx, musb_readw(hw->regs, MUSB_TXCSR),
			musb_readw(hw->regs, MUSB_RXCSR), musb_readb(hw->regs, MUSB_TXTYPE),
			musb_readb(hw->regs, MUSB_RXTYPE), hw->anyka_tx_toggle, hw->anyka_rx_toggle);
	}
	musb_ep_select(musb->mregs, index);
	spin_unlock_irqrestore(&musb->lock, flags);
	return used;
}
static DEVICE_ATTR_RO(endpoints);
static struct attribute *ak_usb_attrs[] = { &dev_attr_registers.attr, &dev_attr_endpoints.attr, NULL };
static const struct attribute_group ak_usb_attr_group = { .attrs = ak_usb_attrs };
static int ak_usb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ak_usb *usb;
	struct resource resources[2];
	struct musb_hdrc_platform_data pdata = {
		.mode = MUSB_HOST, .config = &ak_usb_config,
		.platform_ops = &ak_usb_ops,
		/* Owned retail radio declares400mA (descriptorc8); ordinary factory
		 * host supported it. Allocation policy only, not measured consumption
		 * or an external VBUS regulator/switch change. No external hubs.
		 */
		.power = 200,
	};
	struct platform_device_info info = {
		.parent = dev, .name = "musb-hdrc", .id = PLATFORM_DEVID_AUTO,
		.res = resources, .num_res = ARRAY_SIZE(resources),
		.data = &pdata, .size_data = sizeof(pdata), .dma_mask = DMA_BIT_MASK(32),
	};
	struct resource *memory;
	int irq, ret;

	usb = devm_kzalloc(dev, sizeof(*usb), GFP_KERNEL);
	if (!usb)
		return -ENOMEM;
	usb->sys = syscon_regmap_lookup_by_phandle(dev->of_node, "anyka,sysctrl");
	if (IS_ERR(usb->sys))
		return PTR_ERR(usb->sys);
	memory = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!memory)
		return -EINVAL;
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	resources[0] = *memory;
	resources[1] = (struct resource) { .start = irq, .end = irq,
		.flags = IORESOURCE_IRQ, .name = "mc" };
	platform_set_drvdata(pdev, usb);
	/* Retail board4/5 initialization: GPIO42 output low2s then high.
	 * user_gpio pdatae8c8 -> entrye8f0 names gpio-wifi_power/pin42;
	 * this is independent of sensor reset41 and motor phase pins.
	 */
	usb->radio_power = devm_gpiod_get_optional(dev, "radio-power", GPIOD_OUT_LOW);
	if (IS_ERR(usb->radio_power))
		return dev_err_probe(dev, PTR_ERR(usb->radio_power), "radio power GPIO unavailable\n");
	if (usb->radio_power) {
		msleep(2000);
		gpiod_set_value_cansleep(usb->radio_power, 1);
		msleep(100);
		dev_info(dev, "retail onboard radio power42 enabled after2s reset interval\n");
	}
	usb->nop = usb_phy_generic_register();
	if (IS_ERR(usb->nop))
		return PTR_ERR(usb->nop);
	ret = devm_add_action_or_reset(dev, ak_usb_unregister, usb->nop);
	if (ret)
		return ret;
	usb->musb = platform_device_register_full(&info);
	if (IS_ERR(usb->musb))
		return PTR_ERR(usb->musb);
	ret = devm_add_action_or_reset(dev, ak_usb_unregister, usb->musb);
	if (ret)
		return ret;
	return devm_device_add_group(dev, &ak_usb_attr_group);
}
static const struct of_device_id ak_usb_match[] = {
	{ .compatible = "anyka,ak3918ev200-musb" }, { }
};
MODULE_DEVICE_TABLE(of, ak_usb_match);
static struct platform_driver ak_usb_driver = {
	.probe = ak_usb_probe,
	.driver = { .name = "ak3918ev200-musb", .of_match_table = ak_usb_match },
};
module_platform_driver(ak_usb_driver);
MODULE_DESCRIPTION("Anyka AK3918EV200 upstream MUSB PIO bootstrap glue");
MODULE_LICENSE("GPL");
