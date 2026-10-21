// SPDX-License-Identifier: GPL-2.0-or-later
/* Offline candidate. Exact kernel75 only; no optical/hardware qualification.
 * No MMIO stores: every GPIO write goes through the existing locked gpiochip.
 */
#include <linux/capability.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/user_namespace.h>
#include "optical_policy.h"

#define GPIO_PHYS 0x20170000
#define SYS_PHYS 0x08000000
#define MAP_SIZE 0x100

static DEFINE_MUTEX(command_lock); /* Never acquired by release timer. */
static DEFINE_RAW_SPINLOCK(effect_lock);
static struct qo_policy policy;
static struct gpio_desc *pins[3];
static const unsigned int offsets[] = { 6, 7, 23 };
static struct device_node *gpio_np, *sys_np;
static struct gpio_device *provider;
static struct device *consumer;
static struct gpiod_lookup_table *lookup;
static void __iomem *gpio_ro, *sys_ro;
static struct hrtimer ir_timer;
static struct file *ir_owner;
static unsigned int owned;

static unsigned int compact(u32 v)
{
	return ((v >> 6) & 3) | (((v >> 23) & 1) << 2);
}

/* Pinned bank0: DIR+00, OUT+0c, IN+18; GPIO mode6/7 SYS78=01/01,
 * GPIO23 SYS7c[23:22]=00. This alias is read-only BY CODE, not an MMU RO map.
 */
static struct qo_sample sample(void)
{
	struct qo_sample s;
	s.mux_ok = (readl(sys_ro + 0x78) & 0xf0) == 0x50 &&
		   !(readl(sys_ro + 0x7c) & 0xc00000);
	s.dir = compact(readl(gpio_ro));
	s.latch = compact(readl(gpio_ro + 0x0c));
	s.pad = compact(readl(gpio_ro + 0x18));
	return s;
}

/* effect_lock held, IRQs masked. Validated non-cansleep, active-high normal
 * descriptors only. Even fault handling writes only lines actually reserved.
 * If privileged external remux happened, clearing latches cannot fix the mux;
 * we still clear our latches, then quarantine rather than stealing pinctrl.
 */
static void clear_pair(void)
{
	int i;
	for (i = 0; i < 2; i++)
		if (owned & BIT(i))
			gpiod_set_value(pins[i], 0);
}

static void clear_ir(void)
{
	if (owned & BIT(2))
		gpiod_set_value(pins[2], 0);
	policy.ir_active = 0;
	policy.ir_until = 0;
	ir_owner = NULL;
}

static void verify_release(void)
{
	if (owned == QO_ALL)
		qo_valid(&policy, sample());
}

static void fault_off(void)
{
	clear_pair();
	clear_ir();
	verify_release();
}

static enum hrtimer_restart ir_expired(struct hrtimer *timer)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&effect_lock, flags);
	if (policy.ir_active) {
		clear_ir();
		verify_release();
		if (policy.fault)
			fault_off();
		policy.sequence++;
	}
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	return HRTIMER_NORESTART;
}

static void put_pins(void)
{
	int i;
	for (i = 2; i >= 0; i--)
		if (owned & BIT(i)) {
			gpiod_put(pins[i]);
			pins[i] = NULL;
		}
	owned = 0;
}

/* command_lock held. No timer exists before setup succeeds. Reservations are
 * ASIS: a refusal/foreign owner never gets direction/latch/mux writes.
 */
static int neutral_setup(void)
{
	struct qo_sample s;
	unsigned long flags;
	int i, ret;

	s = sample();
	if (!qo_neutral(s))
		return -EBUSY;
	for (i = 0; i < 3; i++) {
		pins[i] = gpiod_get_index(consumer, "optical", i, GPIOD_ASIS);
		if (IS_ERR(pins[i])) {
			ret = PTR_ERR(pins[i]);
			pins[i] = NULL;
			goto refuse;
		}
		owned |= BIT(i);
		if (gpiod_to_gpio_device(pins[i]) != provider ||
		    gpiod_cansleep(pins[i]) || gpiod_is_active_low(pins[i])) {
			ret = -EOPNOTSUPP;
			goto refuse;
		}
	}
	/* Repeat AFTER every reservation, before ANY direction change. */
	if (!qo_neutral(sample())) {
		ret = -EBUSY;
		goto refuse;
	}
	for (i = 0; i < 3; i++) {
		ret = gpiod_direction_output(pins[i], 0);
		if (ret) {
			policy.fault |= QO_FAULT_GPIO;
			goto failed_output;
		}
	}
	raw_spin_lock_irqsave(&effect_lock, flags);
	ret = qo_valid(&policy, sample());
	if (!ret) {
		policy.setup = 1;
		policy.sequence++;
	}
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	if (!ret)
		return 0;
failed_output:
	raw_spin_lock_irqsave(&effect_lock, flags);
	fault_off();
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	/* An output/readback failure is quarantine, not permission to hand an
	 * uncertain electrical state to another consumer. Retain reservations
	 * until parent-reviewed module unload; no in-module rearm exists.
	 */
	return ret;
refuse:
	put_pins();
	return ret;
}

static void snapshot(struct qo_status *out)
{
	struct qo_sample s = sample();
	u64 now = ktime_get_ns();
	/* STATUS is electrically inert. It may latch a sticky diagnostic fault;
 * it never silently attempts repair. Explicit OFF/CLOSE remain available.
 */
	if (policy.setup)
		qo_valid(&policy, s);
	*out = (struct qo_status) {
		.abi = QO_ABI, .setup = policy.setup, .fault = policy.fault,
		.owned_mask = owned, .pair_latch = s.latch & 3,
		.pair_pad = s.pad & 3, .ir_latch = s.latch >> 2,
		.ir_pad = s.pad >> 2, .direction_mask = s.dir,
		.mux_ok = s.mux_ok,
		.release_verified = policy.setup && !policy.fault && s.mux_ok &&
			s.dir == QO_ALL && !(s.latch & 3) && !(s.pad & 3),
		.ir_active = policy.ir_active,
		.ir_remaining_ms = qo_remaining(policy.ir_until, now),
		.cooldown_remaining_ms = qo_remaining(policy.cooldown_until, now),
		.pulse_us = QO_PULSE_US, .ir_limit_ms = QO_IR_MS,
		.sequence = policy.sequence, .monotonic_ns = now,
	};
}

static bool root_allowed(void)
{
	return uid_eq(current_euid(), GLOBAL_ROOT_UID) &&
		ns_capable(&init_user_ns, CAP_SYS_RAWIO);
}

static long optical_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct qo_status out;
	unsigned long flags;
	u64 now;
	int ret, i;

	if (!root_allowed())
		return -EPERM;
	/* Exact command constants reject malformed size/direction/number too. */
	if (cmd != QO_STATUS && arg)
		return -EINVAL;
	mutex_lock(&command_lock);
	now = ktime_get_ns();
	raw_spin_lock_irqsave(&effect_lock, flags);
	ret = qo_admit(&policy, cmd, now);
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	if (ret)
		goto done;
	if (cmd == QO_SETUP) {
		ret = neutral_setup();
		goto done;
	}
	/* Cancel synchronously OUTSIDE effect_lock; callback never takes mutex.
 * OFF/CLOSE do not need fault-free admission. No repeating/restarting timer.
 */
	if (cmd == QO_IR_OFF || cmd == QO_CLOSE)
		hrtimer_cancel(&ir_timer);
	raw_spin_lock_irqsave(&effect_lock, flags);
	if (cmd == QO_STATUS) {
		snapshot(&out);
		goto unlock;
	}
	if (cmd == QO_IR_OFF || cmd == QO_CLOSE) {
		clear_pair();
		clear_ir();
		verify_release();
		policy.sequence++;
		ret = policy.fault ? -EIO : 0;
		goto unlock;
	}
	ret = qo_valid(&policy, sample());
	if (ret) {
		fault_off();
		goto unlock;
	}
	if (cmd == QO_PULSE_A || cmd == QO_PULSE_B) {
		/* Intentional ~10ms IRQ blackout. No sleeping, allocation, usercopy,
		 * mutex acquisition or user-selected width inside energized interval.
		 * Busy delay is calibrated, NOT measured electrical width or protection
		 * against CPU/bus failure. Existing driver's set uses per-bit RMW lock.
		 */
		gpiod_set_value(pins[cmd == QO_PULSE_A ? 0 : 1], 1);
		for (i = 0; i < 10; i++)
			udelay(1000);
		clear_pair();
		policy.cooldown_until = ktime_get_ns() + QO_COOLDOWN_NS;
		verify_release();
	} else if (cmd == QO_IR_ON) {
		/* Arm BEFORE asserting. Timer callback shares only effect_lock.
		 * Deadline cannot be extended by another ON. A late callback is
		 * possible with IRQ starvation, especially kernel75's lowres clock.
		 */
		policy.ir_until = ktime_get_ns() + QO_IR_NS;
		policy.ir_active = 1;
		ir_owner = file;
		hrtimer_start(&ir_timer, ns_to_ktime(policy.ir_until), HRTIMER_MODE_ABS_HARD);
		gpiod_set_value(pins[2], 1);
		qo_valid(&policy, sample());
	}
	policy.sequence++;
	ret = policy.fault ? -EIO : 0;
	if (ret)
		fault_off();
unlock:
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	if (cmd == QO_CLOSE && !ret) {
		/* Retain sticky fault and cooldown across CLOSE -> SETUP. */
		policy.setup = 0;
		put_pins();
	}
done:
	mutex_unlock(&command_lock);
	/* Commands have no usercopy after effects. Status-copy failure never
	 * authorizes retry of an effect; bridge must treat completion separately.
	 */
	if (!ret && cmd == QO_STATUS && copy_to_user((void __user *)arg, &out, sizeof(out)))
		return -EFAULT;
	return ret;
}

static int optical_open(struct inode *inode, struct file *file)
{
	return root_allowed() ? nonseekable_open(inode, file) : -EPERM;
}

static int optical_release(struct inode *inode, struct file *file)
{
	unsigned long flags;
	bool mine;
	mutex_lock(&command_lock);
	raw_spin_lock_irqsave(&effect_lock, flags);
	mine = ir_owner == file;
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	if (mine)
		hrtimer_cancel(&ir_timer);
	raw_spin_lock_irqsave(&effect_lock, flags);
	/* A pulse cannot outlive its ioctl or wait on this thread. Closing an
	 * unrelated status fd must not shut off another client's IR lease.
	 */
	if (policy.setup && mine) {
		clear_pair();
		clear_ir();
		verify_release();
		if (policy.fault)
			fault_off();
		policy.sequence++;
	}
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	mutex_unlock(&command_lock);
	return 0;
}

static const struct file_operations optical_fops = {
	.owner = THIS_MODULE, .open = optical_open, .release = optical_release,
	.unlocked_ioctl = optical_ioctl,
};
static struct miscdevice optical_misc = {
	.minor = MISC_DYNAMIC_MINOR, .name = "qc07f-optical",
	.fops = &optical_fops, .mode = 0600,
};

struct chip_match { const char *label; int count; };
static int count_chip(struct gpio_chip *gc, const void *data)
{
	struct chip_match *m = (struct chip_match *)data;
	if (gc->label && !strcmp(gc->label, m->label))
		m->count++;
	return 0; /* enumerate all; do not select arbitrary first label */
}
static int exact_chip(struct gpio_chip *gc, const void *data)
{
	return gc->parent && gc->parent->of_node == data && !gc->can_sleep &&
		gc->ngpio == 79 && gc->parent->driver &&
		!strcmp(gc->parent->driver->name, "ak3918ev200-gpio");
}

static void metadata_free(void)
{
	if (lookup) {
		gpiod_remove_lookup_table(lookup);
		kfree(lookup);
	}
	if (consumer)
		root_device_unregister(consumer);
	if (provider)
		gpio_device_put(provider);
	if (gpio_ro) iounmap(gpio_ro);
	if (sys_ro) iounmap(sys_ro);
	of_node_put(sys_np);
	of_node_put(gpio_np);
}

static int __init optical_init(void)
{
	struct device_node *other;
	struct resource res;
	struct chip_match match;
	int ret = -ENODEV, i;

	BUILD_BUG_ON(sizeof(struct qo_status) != 80);
	/* Strict singleton/provider/resource contract, no dynamic GPIO base. */
	gpio_np = of_find_compatible_node(NULL, NULL, "anyka,ak3918ev200-gpio");
	if (!gpio_np)
		goto fail;
	other = of_find_compatible_node(of_node_get(gpio_np), NULL, "anyka,ak3918ev200-gpio");
	if (other) { of_node_put(other); goto fail; }
	if (!of_device_is_available(gpio_np) || of_address_to_resource(gpio_np, 0, &res) ||
	    res.start != GPIO_PHYS || resource_size(&res) != MAP_SIZE)
		goto fail;
	sys_np = of_parse_phandle(gpio_np, "anyka,sysctrl", 0);
	if (!sys_np || !of_device_is_compatible(sys_np, "anyka,ak3918ev200-intc") ||
	    !of_device_is_compatible(sys_np, "syscon") ||
	    of_address_to_resource(sys_np, 0, &res) || res.start != SYS_PHYS ||
	    resource_size(&res) != MAP_SIZE)
		goto fail;
	provider = gpio_device_find(gpio_np, exact_chip);
	if (!provider)
		goto fail;
	match = (struct chip_match) { .label = gpio_device_get_label(provider) };
	/* A label is used only as gpiolib's lookup key AFTER exact DT validation.
	 * Ensure uniqueness, then verify every acquired desc belongs to provider.
	 */
	if (!match.label)
		goto fail;
	gpio_device_find(&match, count_chip);
	if (match.count != 1)
		goto fail;
	gpio_ro = ioremap(GPIO_PHYS, MAP_SIZE);
	sys_ro = ioremap(SYS_PHYS, MAP_SIZE);
	if (!gpio_ro || !sys_ro) { ret = -ENOMEM; goto fail; }
	/* A root consumer device has no platform probe/default pinctrl state. */
	consumer = root_device_register("qc07f-optical-consumer");
	if (IS_ERR(consumer)) { ret = PTR_ERR(consumer); consumer = NULL; goto fail; }
	lookup = kzalloc(struct_size(lookup, table, 4), GFP_KERNEL);
	if (!lookup) { ret = -ENOMEM; goto fail; }
	lookup->dev_id = dev_name(consumer);
	for (i = 0; i < 3; i++)
		lookup->table[i] = GPIO_LOOKUP_IDX(match.label, offsets[i], "optical", i,
						 GPIO_ACTIVE_HIGH | GPIO_PERSISTENT);
	gpiod_add_lookup_table(lookup);
	hrtimer_setup(&ir_timer, ir_expired, CLOCK_MONOTONIC, HRTIMER_MODE_ABS_HARD);
	ret = misc_register(&optical_misc);
	if (!ret)
		return 0; /* No request, output, mux, config or timer activation. */
fail:
	metadata_free();
	return ret;
}

static void __exit optical_exit(void)
{
	unsigned long flags;
	misc_deregister(&optical_misc);
	/* fops.owner pins this module for every open fd; forced unload unsupported. */
	hrtimer_cancel(&ir_timer);
	raw_spin_lock_irqsave(&effect_lock, flags);
	if (owned) {
		fault_off();
		if (policy.fault)
			pr_err("qc07f-optical: release fault %u; optical state UNKNOWN\n", policy.fault);
	}
	policy.setup = 0;
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	put_pins();
	metadata_free();
}
module_init(optical_init);
module_exit(optical_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("QC07F kernel75 fixed optical controls, unqualified candidate");
