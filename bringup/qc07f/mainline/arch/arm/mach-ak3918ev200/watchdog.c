// SPDX-License-Identifier: GPL-2.0-or-later
/* Modern watchdog-core port of the retail AK3918EV200 keyed TOP watchdog.
 * REA akdog_keepalive: SYSCTRL+e4 =55xxxxxx, +e8 =aa000003,46875 ticks/sec.
 * No register write at probe, no private daemon and no GPIO/flash operation.
 * Probe/timeout/reset behavior still requires target qualification; the
 * prototype DT keeps this controller disabled. Inherited running hardware is
 * reported to watchdog core, which may then service it using the configured
 * timeout. This is intentional only when the node is explicitly enabled.
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/watchdog.h>

#define AK_WDT_RELOAD 0
#define AK_WDT_CONTROL 4
#define AK_WDT_LOAD_KEY 0x55000000
#define AK_WDT_CTRL_KEY 0xaa000000
#define AK_WDT_COUNT_MASK 0x00ffffff
#define AK_WDT_ENABLE BIT(0)
#define AK_WDT_RESET BIT(1)

struct ak_watchdog {
	void __iomem *base;
	raw_spinlock_t lock;
	u32 rate;
	struct watchdog_device wdd;
};
static unsigned int timeout = 60;
module_param(timeout, uint, 0444);
MODULE_PARM_DESC(timeout, "Watchdog timeout in seconds");
static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0444);
MODULE_PARM_DESC(nowayout, "Watchdog cannot stop after start");

static void ak_wdt_feed(struct ak_watchdog *wdt, u32 ticks)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&wdt->lock, flags);
	writel(AK_WDT_LOAD_KEY | ticks, wdt->base + AK_WDT_RELOAD);
	writel(AK_WDT_CTRL_KEY | AK_WDT_ENABLE | AK_WDT_RESET,
	       wdt->base + AK_WDT_CONTROL);
	raw_spin_unlock_irqrestore(&wdt->lock, flags);
}
static int ak_wdt_start(struct watchdog_device *wdd)
{
	struct ak_watchdog *wdt = watchdog_get_drvdata(wdd);

	ak_wdt_feed(wdt, wdd->timeout * wdt->rate);
	return 0;
}
static int ak_wdt_stop(struct watchdog_device *wdd)
{
	struct ak_watchdog *wdt = watchdog_get_drvdata(wdd);
	unsigned long flags;

	raw_spin_lock_irqsave(&wdt->lock, flags);
	writel(AK_WDT_CTRL_KEY, wdt->base + AK_WDT_CONTROL);
	writel(AK_WDT_LOAD_KEY | AK_WDT_COUNT_MASK, wdt->base + AK_WDT_RELOAD);
	raw_spin_unlock_irqrestore(&wdt->lock, flags);
	return 0;
}
static int ak_wdt_set_timeout(struct watchdog_device *wdd, unsigned int value)
{
	/* watchdog core checks min/max; reject invalid direct calls as well. */
	if (value < wdd->min_timeout || value > wdd->max_timeout)
		return -EINVAL;
	wdd->timeout = value;
	if (watchdog_active(wdd) || watchdog_hw_running(wdd))
		return ak_wdt_start(wdd);
	return 0;
}
static int ak_wdt_restart(struct watchdog_device *wdd, unsigned long action, void *data)
{
	struct ak_watchdog *wdt = watchdog_get_drvdata(wdd);

	/* Deliberate restart only: one nominal watchdog tick. No timer/IRQ needed. */
	ak_wdt_feed(wdt, 1);
	return 0;
}
static const struct watchdog_info ak_wdt_info = {
	.identity = "AK3918EV200 TOP watchdog",
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};
static const struct watchdog_ops ak_wdt_ops = {
	.owner = THIS_MODULE,
	.start = ak_wdt_start,
	.stop = ak_wdt_stop,
	.ping = ak_wdt_start,
	.set_timeout = ak_wdt_set_timeout,
	.restart = ak_wdt_restart,
};
static int ak_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ak_watchdog *wdt;
	int ret;

	wdt = devm_kzalloc(dev, sizeof(*wdt), GFP_KERNEL);
	if (!wdt)
		return -ENOMEM;
	wdt->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(wdt->base))
		return PTR_ERR(wdt->base);
	ret = device_property_read_u32(dev, "clock-frequency", &wdt->rate);
	if (ret || !wdt->rate || wdt->rate > AK_WDT_COUNT_MASK)
		return dev_err_probe(dev, -EINVAL, "missing/invalid watchdog tick rate\n");
	raw_spin_lock_init(&wdt->lock);
	wdt->wdd.info = &ak_wdt_info;
	wdt->wdd.ops = &ak_wdt_ops;
	wdt->wdd.parent = dev;
	wdt->wdd.min_timeout = 1;
	wdt->wdd.max_timeout = AK_WDT_COUNT_MASK / wdt->rate;
	wdt->wdd.timeout = min_t(unsigned int, 60, wdt->wdd.max_timeout);
	ret = watchdog_init_timeout(&wdt->wdd, timeout, dev);
	if (ret)
		return dev_err_probe(dev, ret, "invalid watchdog timeout\n");
	watchdog_set_drvdata(&wdt->wdd, wdt);
	watchdog_set_nowayout(&wdt->wdd, nowayout);
	watchdog_set_restart_priority(&wdt->wdd, 128);
	watchdog_stop_on_reboot(&wdt->wdd);
	watchdog_stop_on_unregister(&wdt->wdd);
	if (readl(wdt->base + AK_WDT_CONTROL) & AK_WDT_ENABLE)
		set_bit(WDOG_HW_RUNNING, &wdt->wdd.status);
	return devm_watchdog_register_device(dev, &wdt->wdd);
}
static const struct of_device_id ak_wdt_match[] = {
	{ .compatible = "anyka,ak3918ev200-wdt" }, { }
};
MODULE_DEVICE_TABLE(of, ak_wdt_match);
static struct platform_driver ak_wdt_driver = {
	.probe = ak_wdt_probe,
	.driver = { .name = "ak3918ev200-wdt", .of_match_table = ak_wdt_match },
};
module_platform_driver(ak_wdt_driver);
MODULE_DESCRIPTION("Anyka AK3918EV200 keyed TOP watchdog first port");
MODULE_LICENSE("GPL");
