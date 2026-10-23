// SPDX-License-Identifier: GPL-2.0-only
/* QC07F board5 direct-GPIO motor diagnostic. REA retail ak_motor.ko phase
 * bytes at11618:03,06,0c,09; board5 platform data identifies each4-pin group.
 * No GPIO requests or writes at probe. Explicit pulses are bounded to4 steps
 * at20ms/step, serialized across axes, with break-before-make and all coils
 * off on return. No homing, absolute-position or bypass-limit interface.
 * Issued/readback steps do NOT prove physical movement, orientation or limits.
 */
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>

static DEFINE_MUTEX(ak_ptz_actuation_lock);
static const u8 ak_ptz_phases[] = { 3, 6, 12, 9 };
struct ak_ptz {
	struct device *dev;
	u32 axis, phase, issued_steps;
	int last_result;
	bool coil_readback_ok, coils_off_verified;
};
static void ak_ptz_off(struct gpio_descs *lines, u32 initialized)
{
	u32 i;
	for (i = 0; i < lines->ndescs; i++)
		if (initialized & BIT(i))
			gpiod_set_value_cansleep(lines->desc[i], 0);
}
static ssize_t pulse_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct ak_ptz *ptz = dev_get_drvdata(dev);
	struct gpio_descs *lines;
	u32 step, pin, completed = 0, initialized = 0;
	int steps, direction, ret;

	ret = kstrtoint(buf, 10, &steps);
	if (ret)
		return ret;
	if (!steps || steps < -4 || steps > 4)
		return -ERANGE;
	direction = steps > 0 ? 1 : -1;
	steps = abs(steps);
	mutex_lock(&ak_ptz_actuation_lock);
	if (ptz->issued_steps > U32_MAX - steps) {
		mutex_unlock(&ak_ptz_actuation_lock);
		return -EOVERFLOW;
	}
	ptz->coil_readback_ok = false;
	ptz->coils_off_verified = false;
	lines = gpiod_get_array(dev, "phase", GPIOD_ASIS);
	if (IS_ERR(lines)) {
		ret = PTR_ERR(lines);
		goto unlock;
	}
	if (lines->ndescs != 4) {
		ret = -EINVAL;
		goto release;
	}
	for (pin = 0; pin < 4; pin++) {
		ret = gpiod_direction_output(lines->desc[pin], 0);
		if (ret)
			goto off;
		initialized |= BIT(pin);
	}
	for (step = 0; step < steps; step++) {
		u8 phase = ak_ptz_phases[ptz->phase];

		ak_ptz_off(lines, initialized);
		usleep_range(1000, 1500);
		for (pin = 0; pin < 4; pin++)
			gpiod_set_value_cansleep(lines->desc[pin], !!(phase & BIT(pin)));
		msleep(20);
		for (pin = 0; pin < 4; pin++) {
			ret = gpiod_get_value_cansleep(lines->desc[pin]);
			if (ret < 0 || ret != !!(phase & BIT(pin))) {
				ret = ret < 0 ? ret : -EIO;
				goto off;
			}
		}
		completed++;
		ptz->phase = (ptz->phase + (direction > 0 ? 1 : 3)) & 3;
	}
	ret = 0;
	ptz->coil_readback_ok = true;
off:
	ak_ptz_off(lines, initialized);
	if (initialized == 15) {
		ptz->coils_off_verified = true;
		for (pin = 0; pin < 4; pin++)
			if (gpiod_get_value_cansleep(lines->desc[pin]) != 0)
				ptz->coils_off_verified = false;
		if (!ptz->coils_off_verified && !ret)
			ret = -EIO;
	}
release:
	gpiod_put_array(lines);
unlock:
	ptz->issued_steps += completed;
	ptz->last_result = ret;
	mutex_unlock(&ak_ptz_actuation_lock);
	dev_info(dev, "bounded axis%u requested%u issued%u result%d coils_off_verified=%u; mechanical movement unconfirmed\n",
		 ptz->axis, steps, completed, ret, ptz->coils_off_verified);
	return ret ? ret : count;
}
static DEVICE_ATTR_WO(pulse);
static ssize_t state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ak_ptz *ptz = dev_get_drvdata(dev);
	ssize_t n;

	mutex_lock(&ak_ptz_actuation_lock);
	n = sysfs_emit(buf, "axis=%u issued_steps=%u phase=%u result=%d coil_readback_ok=%u coils_off_verified=%u max_pulse_steps=4 period_ms=20 physical_position_known=0\n",
		ptz->axis, ptz->issued_steps, ptz->phase, ptz->last_result,
		ptz->coil_readback_ok, ptz->coils_off_verified);
	mutex_unlock(&ak_ptz_actuation_lock);
	return n;
}
static DEVICE_ATTR_RO(state);
static struct attribute *ak_ptz_attrs[] = { &dev_attr_pulse.attr, &dev_attr_state.attr, NULL };
static const struct attribute_group ak_ptz_group = { .attrs = ak_ptz_attrs };
static int ak_ptz_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ak_ptz *ptz;
	u32 board;
	int ret;

	ret = device_property_read_u32(dev, "anyka,retail-board-version", &board);
	if (ret || board != 5)
		return -EINVAL;
	if (gpiod_count(dev, "phase") != 4)
		return -EINVAL;
	ptz = devm_kzalloc(dev, sizeof(*ptz), GFP_KERNEL);
	if (!ptz)
		return -ENOMEM;
	ptz->dev = dev;
	ret = device_property_read_u32(dev, "anyka,axis", &ptz->axis);
	if (ret || ptz->axis > 1)
		return -EINVAL;
	platform_set_drvdata(pdev, ptz);
	dev_info(dev, "bounded motor diagnostic only, no GPIO requests/writes at probe; axis orientation/limits unqualified\n");
	return devm_device_add_group(dev, &ak_ptz_group);
}
static const struct of_device_id ak_ptz_match[] = {
	{ .compatible = "anyka,qc07f-ptz-diagnostic" }, { }
};
MODULE_DEVICE_TABLE(of, ak_ptz_match);
static struct platform_driver ak_ptz_driver = {
	.probe = ak_ptz_probe,
	.driver = { .name = "qc07f-ptz-diagnostic", .of_match_table = ak_ptz_match },
};
module_platform_driver(ak_ptz_driver);
MODULE_DESCRIPTION("QC07F board5 bounded direct-GPIO motor diagnostic");
MODULE_LICENSE("GPL");
