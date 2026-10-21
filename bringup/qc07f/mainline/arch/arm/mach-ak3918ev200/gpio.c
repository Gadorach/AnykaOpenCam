// SPDX-License-Identifier: GPL-2.0-or-later
/* Retail AK3918EV200 GPIO register contract recovered through REA.
 * No board defaults or outputs are written at probe. Mux ownership is not
 * stolen: consumers must arrange native-GPIO pinctrl state first. GPIO49
 * is input-only. This first port deliberately has no GPIO IRQ/wakeup support.
 * Compilation/fixture validation is not target qualification.
 */
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include "gpio-data.h"

#define AK_GPIO_COUNT 79
#define AK_GPIO_INPUT_ONLY 49
#define AK_GPIO_DIR 0x00
#define AK_GPIO_OUT 0x0c
#define AK_GPIO_IN 0x18

struct ak_gpio {
	struct gpio_chip chip;
	void __iomem *base;
	struct regmap *sysctrl;
	raw_spinlock_t lock;
};

static unsigned int ak_bank_reg(unsigned int base, unsigned int offset)
{
	return base + (offset / 32) * 4;
}

static void ak_update(struct ak_gpio *gpio, unsigned int reg, u32 mask, u32 value)
{
	unsigned long flags;
	u32 old;

	raw_spin_lock_irqsave(&gpio->lock, flags);
	old = readl(gpio->base + reg);
	writel((old & ~mask) | (value & mask), gpio->base + reg);
	raw_spin_unlock_irqrestore(&gpio->lock, flags);
}

static int ak_request(struct gpio_chip *chip, unsigned int offset)
{
	struct ak_gpio *gpio = gpiochip_get_data(chip);
	const struct ak_gpio_pin *pin = &ak_gpio_pins[offset];
	unsigned int value;
	int ret;

	/* Selector fields may be shared by several pins. Never remux a neighbour
	 * or the boot console merely because one GPIO line was requested.
	 */
	if (!pin->mux_mask)
		return 0;
	ret = regmap_read(gpio->sysctrl, pin->mux_reg, &value);
	if (ret)
		return ret;
	return (value & pin->mux_mask) == pin->gpio_value ? 0 : -EBUSY;
}

static int ak_get_direction(struct gpio_chip *chip, unsigned int offset)
{
	struct ak_gpio *gpio = gpiochip_get_data(chip);
	u32 value = readl(gpio->base + ak_bank_reg(AK_GPIO_DIR, offset));

	return value & BIT(offset % 32) ? GPIO_LINE_DIRECTION_OUT : GPIO_LINE_DIRECTION_IN;
}

static int ak_direction_input(struct gpio_chip *chip, unsigned int offset)
{
	struct ak_gpio *gpio = gpiochip_get_data(chip);

	ak_update(gpio, ak_bank_reg(AK_GPIO_DIR, offset), BIT(offset % 32), 0);
	return 0;
}

static int ak_direction_output(struct gpio_chip *chip, unsigned int offset, int value)
{
	struct ak_gpio *gpio = gpiochip_get_data(chip);
	u32 bit = BIT(offset % 32);

	if (offset == AK_GPIO_INPUT_ONLY)
		return -EINVAL;
	/* Set the latch before enabling output, avoiding a transient old level. */
	ak_update(gpio, ak_bank_reg(AK_GPIO_OUT, offset), bit, value ? bit : 0);
	ak_update(gpio, ak_bank_reg(AK_GPIO_DIR, offset), bit, bit);
	return 0;
}

static int ak_get(struct gpio_chip *chip, unsigned int offset)
{
	struct ak_gpio *gpio = gpiochip_get_data(chip);

	return !!(readl(gpio->base + ak_bank_reg(AK_GPIO_IN, offset)) & BIT(offset % 32));
}

static int ak_set(struct gpio_chip *chip, unsigned int offset, int value)
{
	struct ak_gpio *gpio = gpiochip_get_data(chip);
	u32 bit = BIT(offset % 32);

	if (offset == AK_GPIO_INPUT_ONLY)
		return -EINVAL;
	ak_update(gpio, ak_bank_reg(AK_GPIO_OUT, offset), bit, value ? bit : 0);
	return 0;
}

static int ak_set_config(struct gpio_chip *chip, unsigned int offset, unsigned long config)
{
	struct ak_gpio *gpio = gpiochip_get_data(chip);
	const struct ak_gpio_pin *pin = &ak_gpio_pins[offset];
	enum pin_config_param param = pinconf_to_config_param(config);
	bool enable;

	if (!pin->pull_mask)
		return -ENOTSUPP;
	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		enable = false;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		if (pin->pull_kind != 0)
			return -EINVAL;
		enable = true;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		if (pin->pull_kind != 1)
			return -EINVAL;
		enable = true;
		break;
	default:
		/* gpiolib optional persistence/drive setup specifically recognizes
		 * ENOTSUPP; EOPNOTSUPP would incorrectly reject descriptor requests.
		 */
		return -ENOTSUPP;
	}
	/* Retail pull controls are active-low enable bits. */
	return regmap_update_bits(gpio->sysctrl, pin->pull_reg, pin->pull_mask,
				  enable ? 0 : pin->pull_mask);
}

static int ak_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ak_gpio *gpio;

	gpio = devm_kzalloc(dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;
	gpio->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(gpio->base))
		return PTR_ERR(gpio->base);
	gpio->sysctrl = syscon_regmap_lookup_by_phandle(dev->of_node, "anyka,sysctrl");
	if (IS_ERR(gpio->sysctrl))
		return dev_err_probe(dev, PTR_ERR(gpio->sysctrl), "missing shared sysctrl\n");
	raw_spin_lock_init(&gpio->lock);
	gpio->chip = (struct gpio_chip) {
		.label = dev_name(dev), .parent = dev, .owner = THIS_MODULE,
		.base = -1, .ngpio = AK_GPIO_COUNT,
		.request = ak_request, .get_direction = ak_get_direction,
		.direction_input = ak_direction_input, .direction_output = ak_direction_output,
		.get = ak_get, .set = ak_set, .set_config = ak_set_config,
	};
	return devm_gpiochip_add_data(dev, &gpio->chip, gpio);
}

static const struct of_device_id ak_gpio_match[] = {
	{ .compatible = "anyka,ak3918ev200-gpio" }, { }
};
MODULE_DEVICE_TABLE(of, ak_gpio_match);

static struct platform_driver ak_gpio_driver = {
	.probe = ak_gpio_probe,
	.driver = { .name = "ak3918ev200-gpio", .of_match_table = ak_gpio_match },
};
module_platform_driver(ak_gpio_driver);
MODULE_DESCRIPTION("Anyka AK3918EV200 retail-register GPIO first port");
MODULE_LICENSE("GPL");
