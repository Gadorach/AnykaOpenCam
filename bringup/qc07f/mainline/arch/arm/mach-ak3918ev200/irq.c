// SPDX-License-Identifier: GPL-2.0-only
/* Register semantics from the GPL Anyka 3.4.35 mach-ak39/irq.c BSP.
 * Reimplemented using irq domains. Only main/system-controller IRQs supported;
 * GPIO/L2 subinterrupts, wakeup and power management are intentionally absent.
 */
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <asm/exception.h>
#include <asm/mach/irq.h>

struct ak_intc {
	void __iomem *base;
	struct irq_domain *domain;
	raw_spinlock_t lock;
	u32 mask_offset, status_offset, valid;
};
static struct ak_intc *root_intc;

static void ak_mask(struct irq_data *d)
{
	struct ak_intc *c = irq_data_get_irq_chip_data(d);
	unsigned long flags;
	raw_spin_lock_irqsave(&c->lock, flags);
	writel(readl(c->base + c->mask_offset) & ~BIT(d->hwirq), c->base + c->mask_offset);
	raw_spin_unlock_irqrestore(&c->lock, flags);
}
static void ak_unmask(struct irq_data *d)
{
	struct ak_intc *c = irq_data_get_irq_chip_data(d);
	unsigned long flags;
	raw_spin_lock_irqsave(&c->lock, flags);
	writel(readl(c->base + c->mask_offset) | BIT(d->hwirq), c->base + c->mask_offset);
	raw_spin_unlock_irqrestore(&c->lock, flags);
}
static struct irq_chip ak_chip = {
	.name = "ak39e2-intc", .irq_mask = ak_mask,
	.irq_mask_ack = ak_mask, .irq_unmask = ak_unmask,
};
static int ak_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	struct ak_intc *c = d->host_data;
	if (hw >= 32 || !(c->valid & BIT(hw)))
		return -EINVAL;
	irq_set_chip_data(irq, c);
	irq_set_chip_and_handler(irq, &ak_chip, handle_level_irq);
	irq_set_probe(irq);
	return 0;
}
static const struct irq_domain_ops ak_ops = {
	.map = ak_map, .xlate = irq_domain_xlate_onecell,
};
static void ak_dispatch(struct ak_intc *c)
{
	u32 pending = readl(c->base + c->status_offset) & readl(c->base + c->mask_offset) & c->valid;
	while (pending) {
		unsigned int hw = __ffs(pending);
		generic_handle_domain_irq(c->domain, hw);
		pending &= ~BIT(hw);
	}
}
static void __exception_irq_entry ak_handle_irq(struct pt_regs *regs)
{
	ak_dispatch(root_intc);
}
static void ak_cascade(struct irq_desc *desc)
{
	struct ak_intc *c = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	chained_irq_enter(chip, desc);
	ak_dispatch(c);
	chained_irq_exit(chip, desc);
}
static int __init ak_init(struct device_node *np, struct device_node *parent)
{
	struct ak_intc *c;
	unsigned int irq;
	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;
	c->base = of_iomap(parent ? parent : np, 0);
	if (!c->base) {
		kfree(c);
		return -ENOMEM;
	}
	raw_spin_lock_init(&c->lock);
	c->mask_offset = parent ? 0x2c : 0x24;
	c->status_offset = parent ? 0x30 : 0x4c;
	c->valid = parent ? GENMASK(10, 0) : GENMASK(19, 0);
	writel(0, c->base + c->mask_offset);
	if (!parent)
		writel(0, c->base + 0x28); /* FIQ mask */
	c->domain = irq_domain_create_linear(of_fwnode_handle(np), parent ? 11 : 32, &ak_ops, c);
	if (!c->domain)
		return -ENOMEM;
	if (parent) {
		irq = irq_of_parse_and_map(np, 0);
		if (!irq)
			return -EINVAL;
		irq_set_chained_handler_and_data(irq, ak_cascade, c);
	} else {
		root_intc = c;
		set_handle_irq(ak_handle_irq);
	}
	return 0;
}
IRQCHIP_DECLARE(ak39e2_intc, "anyka,ak3918ev200-intc", ak_init);
IRQCHIP_DECLARE(ak39e2_sysintc, "anyka,ak3918ev200-sysintc", ak_init);
