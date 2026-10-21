// SPDX-License-Identifier: GPL-2.0-or-later
/* Bounded16-byte PIO adapter using retail REA ak39_i2c_xfer/init and GPL
 * AK39 controller bits. Bootstrap-only clock/reset/pin ownership: preserve
 * all unrelated SYSCTRL bits, never retune PLLs, claim only I2C27/28.
 * A full CCF/reset/pinctrl provider and verified NACK-status semantics remain
 * required production work. The adapter is not advertised as SMBus/10-bit.
 */
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#define AK_I2C_CTRL 0x00
#define AK_I2C_CMD 0x10
#define AK_I2C_DATA 0x20
#define AK_I2C_DONE BIT(4)
#define AK_I2C_IRQ BIT(5)
#define AK_I2C_DIV512 BIT(6)
#define AK_I2C_DELAY GENMASK(8, 7)
#define AK_I2C_COUNT GENMASK(12, 9)
#define AK_I2C_READ BIT(13)
#define AK_I2C_START BIT(14)
#define AK_I2C_ACK BIT(15)
#define AK_I2C_NOSTOP BIT(16)
#define AK_I2C_ADDR_START BIT(17)
#define AK_I2C_ADDR_ENABLE BIT(18)

struct ak_i2c {
	void __iomem *base;
	struct regmap *sys;
	struct i2c_adapter adapter;
	u32 control, input_rate, bus_rate;
};
static int ak_i2c_rate(struct ak_i2c *i2c)
{
	u32 reg, m, n, od, div, rate;
	int ret = regmap_read(i2c->sys, 0x08, &reg);

	if (ret)
		return ret;
	m = reg & 0xfe;
	n = (reg >> 8) & 15;
	od = (reg >> 12) & 3;
	if (!od || n < 2 || n > 6 || m < 84 || m > 254)
		return -EINVAL;
	/* Preserve the retail integer-MHz arithmetic and PLL update bit mask. */
	rate = (12 * m / (n << od)) * 1000000;
	div = (reg >> 17) & 7;
	rate >>= div ? div : 1;
	if (reg & BIT(24))
		rate >>= 1;
	i2c->input_rate = rate;
	return 0;
}
static int ak_i2c_divider(struct ak_i2c *i2c, u32 requested)
{
	u32 div, count, rate, best = 0, control = 0;

	for (div = 16; div <= 512; div += 496)
		for (count = 1; count <= 16; count++) {
			rate = i2c->input_rate / (2 * div * count);
			if (rate <= requested && rate > best) {
				best = rate;
				control = count - 1;
				if (div == 512)
					control |= AK_I2C_DIV512;
			}
		}
	if (!best)
		return -EINVAL;
	/* Retail polling initializes8020: pending/status generation may require
	 * peripheral interrupt-enable5 even when the parent CPU IRQ is masked.
	 * This adapter does not request/unmask a CPU IRQ; completion is polled.
	 */
	i2c->control = control | AK_I2C_ACK | AK_I2C_IRQ;
	i2c->bus_rate = best;
	return 0;
}
static void ak_i2c_pack(struct ak_i2c *i2c, const u8 *buffer, u16 count)
{
	u32 value, word, byte;

	for (word = 0; word < 4; word++) {
		value = 0;
		for (byte = 0; byte < 4; byte++)
			if (word * 4 + byte < count)
				value |= (u32)buffer[word * 4 + byte] << (8 * byte);
		writel(value, i2c->base + AK_I2C_DATA + word * 4);
	}
}
static int ak_i2c_xfer(struct i2c_adapter *adapter, struct i2c_msg *msgs, int num)
{
	struct ak_i2c *i2c = i2c_get_adapdata(adapter);
	u32 control, value, word, byte;
	int i, ret;

	if (num < 1 || num > 2)
		return -EOPNOTSUPP;
	for (i = 0; i < num; i++)
		if (msgs[i].addr > 0x7f || msgs[i].flags & ~(I2C_M_RD | I2C_M_DMA_SAFE | I2C_M_STOP) ||
		    !msgs[i].len || msgs[i].len > 16 || !msgs[i].buf)
			return -EOPNOTSUPP;
	if (num == 2 && (msgs[0].addr != msgs[1].addr ||
	    msgs[0].flags & I2C_M_RD || !(msgs[1].flags & I2C_M_RD)))
		return -EOPNOTSUPP;
	for (i = 0; i < num; i++) {
		struct i2c_msg *msg = &msgs[i];

		if (!(msg->flags & I2C_M_RD))
			ak_i2c_pack(i2c, msg->buf, msg->len);
		writel(AK_I2C_ADDR_ENABLE | AK_I2C_ADDR_START | (msg->addr << 1) |
		       !!(msg->flags & I2C_M_RD), i2c->base + AK_I2C_CMD);
		control = i2c->control | AK_I2C_START | ((msg->len - 1) << 9);
		if (msg->flags & I2C_M_RD)
			control |= AK_I2C_READ | AK_I2C_DELAY;
		/* Retail PIO completes each message with STOP. Completion may
		 * require bus release; NOSTOP/repeated START is not qualified.
		 */
		writel(control, i2c->base + AK_I2C_CTRL);
		ret = readl_poll_timeout(i2c->base + AK_I2C_CTRL, value,
					 value & AK_I2C_DONE, 5, 50000);
		if (ret) {
			writel(i2c->control, i2c->base + AK_I2C_CTRL);
			return ret;
		}
		if (msg->flags & I2C_M_RD)
			for (word = 0; word * 4 < msg->len; word++) {
				value = readl(i2c->base + AK_I2C_DATA + word * 4);
				for (byte = 0; byte < 4 && word * 4 + byte < msg->len; byte++)
					msg->buf[word * 4 + byte] = value >> (byte * 8);
			}
	}
	writel(i2c->control, i2c->base + AK_I2C_CTRL);
	return num;
}
static u32 ak_i2c_functionality(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}
static const struct i2c_algorithm ak_i2c_algorithm = {
	.xfer = ak_i2c_xfer, .functionality = ak_i2c_functionality,
};
static const struct i2c_adapter_quirks ak_i2c_quirks = {
	.flags = I2C_AQ_NO_ZERO_LEN | I2C_AQ_NO_REP_START,
	/* Repeated START is deliberately not advertised by this first port. */
	.max_num_msgs = 2, .max_write_len = 16, .max_read_len = 16,
	.max_comb_1st_msg_len = 16, .max_comb_2nd_msg_len = 16,
};
static ssize_t registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ak_i2c *i2c = dev_get_drvdata(dev);
	u32 pll, gate, reset, mux, pull;

	if (regmap_read(i2c->sys, 0x08, &pll) ||
	    regmap_read(i2c->sys, 0x1c, &gate) ||
	    regmap_read(i2c->sys, 0x20, &reset) ||
	    regmap_read(i2c->sys, 0x74, &mux) ||
	    regmap_read(i2c->sys, 0x80, &pull))
		return -EIO;
	return sysfs_emit(buf, "asic_hz=%u bus_hz=%u ctrl=%08x cmd=%08x pll=%08x gate=%08x reset=%08x mux=%08x pull=%08x\n",
		i2c->input_rate, i2c->bus_rate,
		readl(i2c->base + AK_I2C_CTRL), readl(i2c->base + AK_I2C_CMD),
		pll, gate, reset, mux, pull);
}
static DEVICE_ATTR_RO(registers);
static struct attribute *ak_i2c_attrs[] = { &dev_attr_registers.attr, NULL };
static const struct attribute_group ak_i2c_attr_group = { .attrs = ak_i2c_attrs };
static int ak_i2c_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ak_i2c *i2c;
	u32 frequency = 100000;
	int ret;

	i2c = devm_kzalloc(dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;
	i2c->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(i2c->base))
		return PTR_ERR(i2c->base);
	i2c->sys = syscon_regmap_lookup_by_phandle(dev->of_node, "anyka,sysctrl");
	if (IS_ERR(i2c->sys))
		return dev_err_probe(dev, PTR_ERR(i2c->sys), "missing sysctrl\n");
	ret = ak_i2c_rate(i2c);
	if (ret)
		return dev_err_probe(dev, ret, "unsupported inherited ASIC clock\n");
	device_property_read_u32(dev, "clock-frequency", &frequency);
	if (frequency < 10000 || frequency > 400000)
		return -EINVAL;
	ret = ak_i2c_divider(i2c, frequency);
	if (ret)
		return ret;
	/* Bootstrap ownership only: I2C clock10, reset10, pin27/28 mux/pulls.
	 * No PLL retuning, no camera power/reset or expander/motor commands.
	 */
	ret = regmap_update_bits(i2c->sys, 0x1c, BIT(10), 0);
	if (ret)
		return ret;
	ret = regmap_update_bits(i2c->sys, 0x20, BIT(10), BIT(10));
	if (ret)
		return ret;
	usleep_range(125000, 130000);
	ret = regmap_update_bits(i2c->sys, 0x20, BIT(10), 0);
	if (ret)
		return ret;
	ret = regmap_update_bits(i2c->sys, 0x74, 0x180, 0x180);
	if (ret)
		return ret;
	ret = regmap_update_bits(i2c->sys, 0x80, 0xc0, 0);
	if (ret)
		return ret;
	writel(i2c->control, i2c->base + AK_I2C_CTRL);
	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.algo = &ak_i2c_algorithm;
	i2c->adapter.quirks = &ak_i2c_quirks;
	i2c->adapter.dev.parent = dev;
	i2c->adapter.dev.of_node = dev->of_node;
	i2c->adapter.timeout = msecs_to_jiffies(100);
	i2c->adapter.retries = 0;
	strscpy(i2c->adapter.name, "AK3918EV200 bounded PIO", sizeof(i2c->adapter.name));
	i2c_set_adapdata(&i2c->adapter, i2c);
	dev_info(dev, "inherited ASIC %uHz, bounded PIO bus %uHz\n",
		 i2c->input_rate, i2c->bus_rate);
	ret = devm_i2c_add_adapter(dev, &i2c->adapter);
	if (ret)
		return ret;
	platform_set_drvdata(pdev, i2c);
	return devm_device_add_group(dev, &ak_i2c_attr_group);
}
static const struct of_device_id ak_i2c_match[] = {
	{ .compatible = "anyka,ak3918ev200-i2c" }, { }
};
MODULE_DEVICE_TABLE(of, ak_i2c_match);
static struct platform_driver ak_i2c_driver = {
	.probe = ak_i2c_probe,
	.driver = { .name = "ak3918ev200-i2c", .of_match_table = ak_i2c_match },
};
module_platform_driver(ak_i2c_driver);
MODULE_DESCRIPTION("Anyka AK3918EV200 bounded I2C bootstrap port");
MODULE_LICENSE("GPL");
