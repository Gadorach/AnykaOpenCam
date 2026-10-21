// SPDX-License-Identifier: GPL-2.0-or-later
/* QC07F sensor clock/reset bootstrap, not an ISP/capture driver.
 * Retail camera link metadata: privc041f038, pin-power=-1, reset41;
 * GPIO41 output/high corroborated in running stock GPIO snapshot. SC1346
 * REA reset low30ms/high20ms; camera_add_device sets PERIPLL2 divider
 * and update bit19. Sensor clock enable bit18 matches GPL clock provider
 * and live stock PERIPLL2=00146031 (600MHz/25=24MHz).
 * No PLL retuning, sensor register writes, motor/expander or flash operation.
 */
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/regmap.h>

static int ak_sensor_bootstrap_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *sys;
	struct gpio_desc *reset;
	u32 reg, m, n, od, rate, divider, requested = 24000000;
	int ret;

	sys = syscon_regmap_lookup_by_phandle(dev->of_node, "anyka,sysctrl");
	if (IS_ERR(sys))
		return dev_err_probe(dev, PTR_ERR(sys), "missing sysctrl\n");
	ret = regmap_read(sys, 0x14, &reg);
	if (ret)
		return ret;
	m = reg & 0xff;
	n = (reg >> 8) & 15;
	od = (reg >> 12) & 3;
	if (!od || n < 2 || n > 6 || m < 84 || m > 254)
		return dev_err_probe(dev, -EINVAL, "unsupported inherited peripheral PLL\n");
	rate = (12 * m / (n << od)) * 1000000;
	device_property_read_u32(dev, "clock-frequency", &requested);
	if (!requested || rate % requested)
		return dev_err_probe(dev, -EINVAL, "sensor clock not an exact inherited-PLL divider\n");
	divider = rate / requested;
	if (!divider || divider > 64)
		return -EINVAL;
	/* Assert the explicitly described active-low reset; never guess a pin. */
	reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(reset))
		return dev_err_probe(dev, PTR_ERR(reset), "sensor reset ownership unavailable\n");
	ret = regmap_update_bits(sys, 0x18, GENMASK(15, 10) | BIT(18) | BIT(19),
				 (divider - 1) << 10 | BIT(18) | BIT(19));
	if (ret)
		return ret;
	msleep(30);
	gpiod_set_value_cansleep(reset, 0); /* active-low descriptor => physical high */
	msleep(20);
	ret = regmap_read(sys, 0x18, &reg);
	if (ret)
		return ret;
	dev_info(dev, "sensor reset deasserted, inherited PLL %uHz MCLK %uHz reg18=%08x\n",
		 rate, requested, reg);
	return 0;
}
static const struct of_device_id ak_sensor_bootstrap_match[] = {
	{ .compatible = "anyka,qc07f-sensor-bootstrap" }, { }
};
MODULE_DEVICE_TABLE(of, ak_sensor_bootstrap_match);
static struct platform_driver ak_sensor_bootstrap_driver = {
	.probe = ak_sensor_bootstrap_probe,
	.driver = { .name = "qc07f-sensor-bootstrap", .of_match_table = ak_sensor_bootstrap_match },
};
module_platform_driver(ak_sensor_bootstrap_driver);
MODULE_DESCRIPTION("QC07F source-supported sensor clock/reset bootstrap");
MODULE_LICENSE("GPL");
