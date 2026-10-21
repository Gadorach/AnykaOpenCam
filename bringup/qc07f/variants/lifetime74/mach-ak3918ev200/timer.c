// SPDX-License-Identifier: GPL-2.0-or-later
/* Protocol derived from GPL Anyka 3.4.35 mach-ak39/time.c.
 * Timer1 is the clockevent; Timer2 is a free-running clocksource.
 * Assumes the BSP's 12-MHz input; retail timing remains unmeasured.
 */
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>

#define AK_CLEAR BIT(30)
#define AK_FEED BIT(29)
#define AK_ENABLE BIT(28)
#define AK_PENDING BIT(27)
#define AK_READ BIT(26)
#define AK_ONESHOT BIT(24)
struct ak_timer {
	void __iomem *base;
	struct clocksource source;
	struct clock_event_device event;
	raw_spinlock_t lock;
};
static u64 ak_read(struct clocksource *cs)
{
	struct ak_timer *t = container_of(cs, struct ak_timer, source);
	unsigned long flags;
	u32 control, value;
	raw_spin_lock_irqsave(&t->lock, flags);
	control = readl(t->base + 12);
	writel(control | AK_READ, t->base + 12);
	value = readl(t->base + 8);
	writel(control & ~AK_READ, t->base + 12);
	raw_spin_unlock_irqrestore(&t->lock, flags);
	return (u32)~value;
}
static int ak_shutdown(struct clock_event_device *ev)
{
	struct ak_timer *t = container_of(ev, struct ak_timer, event);
	writel(AK_CLEAR, t->base + 4);
	return 0;
}
static int ak_next(unsigned long delta, struct clock_event_device *ev)
{
	struct ak_timer *t = container_of(ev, struct ak_timer, event);
	writel(AK_CLEAR, t->base + 4);
	writel(delta - 1, t->base);
	writel(AK_FEED | AK_ENABLE | AK_ONESHOT, t->base + 4);
	return 0;
}
static irqreturn_t ak_tick(int irq, void *data)
{
	struct ak_timer *t = data;
	u32 status = readl(t->base + 4);
	if (!(status & AK_PENDING))
		return IRQ_NONE;
	writel(status | AK_CLEAR, t->base + 4);
	t->event.event_handler(&t->event);
	return IRQ_HANDLED;
}
static int __init ak_timer_init(struct device_node *np)
{
	struct ak_timer *t;
	u32 hz;
	int irq, ret;
	if (of_property_read_u32(np, "clock-frequency", &hz) || !hz)
		return -EINVAL;
	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		return -ENOMEM;
	t->base = of_iomap(np, 0);
	irq = irq_of_parse_and_map(np, 0);
	if (!t->base || !irq)
		return -EINVAL;
	raw_spin_lock_init(&t->lock);
	writel(AK_CLEAR, t->base + 12);
	writel(~0U, t->base + 8);
	writel(AK_ENABLE | AK_FEED, t->base + 12);
	t->source.name = "ak39e2-timer2";
	t->source.rating = 200;
	t->source.read = ak_read;
	t->source.mask = CLOCKSOURCE_MASK(32);
	t->source.flags = CLOCK_SOURCE_IS_CONTINUOUS;
	ret = clocksource_register_hz(&t->source, hz);
	if (ret)
		return ret;
	t->event.name = "ak39e2-timer1";
	t->event.rating = 200;
	t->event.features = CLOCK_EVT_FEAT_ONESHOT;
	t->event.set_next_event = ak_next;
	t->event.set_state_shutdown = ak_shutdown;
	t->event.set_state_oneshot = ak_shutdown;
	t->event.cpumask = cpumask_of(0);
	t->event.irq = irq;
	ret = request_irq(irq, ak_tick, IRQF_TIMER, "ak39e2-timer1", t);
	if (ret)
		return ret;
	clockevents_config_and_register(&t->event, hz, 0xf, ~0U);
	return 0;
}
TIMER_OF_DECLARE(ak39e2_timer, "anyka,ak3918ev200-timer", ak_timer_init);
