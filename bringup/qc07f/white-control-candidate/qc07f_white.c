// SPDX-License-Identifier: GPL-2.0-or-later
/* Independent exact75 GPIO5 consumer. Offline candidate, not native acceptance.
 * Aliases are read-only BY CODE (not MMU RO). All writes use locked gpiolib.
 */
#include <linux/capability.h>
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
#include "white_policy.h"
#define GPIO_PHYS 0x20170000
#define SYS_PHYS 0x08000000
#define MAP_SIZE 0x100

static DEFINE_MUTEX(command_lock);
static DEFINE_RAW_SPINLOCK(effect_lock);
static struct qw_policy policy;
static struct gpio_desc *pin;
static struct device_node *gpio_np, *sys_np;
static struct gpio_device *provider;
static struct device *consumer;
static struct gpiod_lookup_table *lookup;
static void __iomem *gpio_ro, *sys_ro;
static struct hrtimer white_timer;
static struct file *white_owner;

static struct qw_sample sample(void)
{
	return (struct qw_sample) {
		.mux_ok = !(readl(sys_ro + 0x74) & 0x60),
		.dir = !!(readl(gpio_ro) & BIT(5)),
		.latch = !!(readl(gpio_ro + 0x0c) & BIT(5)),
		.pad = !!(readl(gpio_ro + 0x18) & BIT(5)),
	};
}

/* effect_lock held; callback never takes command_lock. Clear only OUR line,
 * even on fault/remux. No pinctrl repair, no direction change in timer/close.
 */
static int clear_owned(void)
{
	if (!pin) return 0;
	if (gpiod_set_value(pin, 0)) policy.fault |= QW_FAULT_GPIO;
	policy.active = 0;
	policy.until = 0;
	white_owner = NULL;
	return qw_valid(&policy, sample());
}

static enum hrtimer_restart white_expired(struct hrtimer *timer)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&effect_lock, flags);
	if (policy.active) {
		clear_owned();
		policy.sequence++;
	}
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	return HRTIMER_NORESTART;
}

/* Explicit OFF is also setup: it deliberately accepts an initial HIGH pad or
 * input. ASIS reservation + mux/provider checks precede ANY output operation.
 * Exclusive ownership is retained through all OFF/fault/close paths until the
 * parent's reviewed unload. Refusals before effects release ASIS reservations.
 */
static int white_off(void)
{
	struct qw_sample prior;
	unsigned long flags;
	int ret;

	if (!pin) {
		if (!sample().mux_ok) return -EBUSY;
		pin = gpiod_get_index(consumer, "white", 0, GPIOD_ASIS);
		if (IS_ERR(pin)) { ret = PTR_ERR(pin); pin = NULL; return ret; }
		if (gpiod_to_gpio_device(pin) != provider ||
		    gpiod_cansleep(pin) || gpiod_is_active_low(pin) || !sample().mux_ok) {
			gpiod_put(pin);
			pin = NULL;
			return -EOPNOTSUPP;
		}
	}
	/* No cancellation under effect_lock: callback cannot deadlock on mutex. */
	hrtimer_cancel(&white_timer);
	raw_spin_lock_irqsave(&effect_lock, flags);
	prior = sample();
	qw_prior(&policy, prior);
	/* Initial input/HIGH is admitted. Once setup was verified, however, do
	 * not let explicit OFF hide a pre-existing config/readback source fault.
	 */
	if (policy.setup) qw_valid(&policy, prior);
	if (!prior.mux_ok) {
		policy.fault |= QW_FAULT_CONFIG;
		ret = clear_owned();
	} else {
		/* Exact provider: OUT latch low BEFORE DIR OE; per-bit RMW locking.
		 * Provider is non-cansleep; this fixed call has no allocation/sleep.
		 */
		if (gpiod_direction_output(pin, 0)) policy.fault |= QW_FAULT_GPIO;
		ret = clear_owned();
		if (qw_low(sample())) policy.setup = 1;
	}
	policy.sequence++;
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	return ret;
}

static void snapshot(struct qw_status *out)
{
	struct qw_sample s = sample();
	u64 now = ktime_get_ns();
	/* Diagnostic sticky fault is allowed, but STATUS has NO electrical effect. */
	if (policy.setup) qw_valid(&policy, s);
	*out = (struct qw_status) {
		.abi = QW_ABI, .flags = qw_flags(&policy, s), .fault = policy.fault,
		.owned = !!pin, .white_latch = s.latch, .white_pad = s.pad,
		.direction = s.dir, .mux_ok = s.mux_ok,
		.remaining_ms = qw_remaining(policy.until, now), .limit_ms = QW_LIMIT_MS,
		.prior_pad = policy.prior_pad, .prior_direction = policy.prior_direction,
		.sequence = policy.sequence, .monotonic_ns = now,
	};
}

static bool root_allowed(void)
{
	return uid_eq(current_euid(), GLOBAL_ROOT_UID) &&
		ns_capable(&init_user_ns, CAP_SYS_RAWIO);
}

static long white_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct qw_status out;
	unsigned long flags;
	int ret;
	if (!root_allowed()) return -EPERM;
	if (cmd != QW_STATUS && arg) return -EINVAL;
	mutex_lock(&command_lock);
	raw_spin_lock_irqsave(&effect_lock, flags);
	ret = qw_admit(&policy, cmd);
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	if (ret) goto done;
	if (cmd == QW_OFF) { ret = white_off(); goto done; }
	raw_spin_lock_irqsave(&effect_lock, flags);
	if (cmd == QW_STATUS) {
		snapshot(&out);
	} else {
		ret = qw_valid(&policy, sample());
		if (!ret) {
			/* Hard monotonic 4900ms timer ARMED BEFORE high; never extend ON.
			 * LOWRES HZ100/PREEMPT_NONE: NOT an electrical <=5s guarantee.
			 */
			policy.until = ktime_get_ns() + QW_LIMIT_NS;
			policy.active = 1;
			white_owner = file;
			hrtimer_start(&white_timer, ns_to_ktime(policy.until), HRTIMER_MODE_ABS_HARD);
			if (gpiod_set_value(pin, 1)) policy.fault |= QW_FAULT_GPIO;
			ret = qw_valid(&policy, sample());
		}
		if (ret) clear_owned();
		policy.sequence++;
	}
	raw_spin_unlock_irqrestore(&effect_lock, flags);
done:
	mutex_unlock(&command_lock);
	/* Effects have no post-effect usercopy; unknown completion NEVER replayed. */
	if (!ret && cmd == QW_STATUS && copy_to_user((void __user *)arg, &out, sizeof(out)))
		return -EFAULT;
	return ret;
}

static int white_open(struct inode *inode, struct file *file)
{
	return root_allowed() ? nonseekable_open(inode, file) : -EPERM;
}
static int white_release(struct inode *inode, struct file *file)
{
	unsigned long flags;
	bool mine;
	mutex_lock(&command_lock);
	raw_spin_lock_irqsave(&effect_lock, flags);
	mine = white_owner == file;
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	if (mine) hrtimer_cancel(&white_timer);
	raw_spin_lock_irqsave(&effect_lock, flags);
	if (mine) { clear_owned(); policy.sequence++; }
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	mutex_unlock(&command_lock);
	return 0; /* Closing status/other fd is electrically inert. */
}
static const struct file_operations white_fops = {
	.owner = THIS_MODULE, .open = white_open, .release = white_release,
	.unlocked_ioctl = white_ioctl,
};
static struct miscdevice white_misc = {
	.minor = MISC_DYNAMIC_MINOR, .name = "qc07f-white", .fops = &white_fops, .mode = 0600,
};
struct chip_match { const char *label; int count; };
static int count_chip(struct gpio_chip *gc, const void *data)
{
	struct chip_match *m = (struct chip_match *)data;
	if (gc->label && !strcmp(gc->label, m->label)) m->count++;
	return 0;
}
static int exact_chip(struct gpio_chip *gc, const void *data)
{
	return gc->parent && gc->parent->of_node == data && !gc->can_sleep &&
		gc->ngpio == 79 && gc->parent->driver &&
		!strcmp(gc->parent->driver->name, "ak3918ev200-gpio");
}
static void metadata_free(void)
{
	if (lookup) { gpiod_remove_lookup_table(lookup); kfree(lookup); }
	if (consumer) root_device_unregister(consumer);
	if (provider) gpio_device_put(provider);
	if (gpio_ro) iounmap(gpio_ro);
	if (sys_ro) iounmap(sys_ro);
	of_node_put(sys_np);
	of_node_put(gpio_np);
}
static int __init white_init(void)
{
	struct device_node *other;
	struct resource res;
	struct chip_match match;
	int ret = -ENODEV;
	BUILD_BUG_ON(sizeof(struct qw_status) != 64);
	BUILD_BUG_ON(offsetof(struct qw_status, sequence) != 48);
	BUILD_BUG_ON(offsetof(struct qw_status, monotonic_ns) != 56);
	gpio_np = of_find_compatible_node(NULL, NULL, "anyka,ak3918ev200-gpio");
	if (!gpio_np) goto fail;
	other = of_find_compatible_node(of_node_get(gpio_np), NULL, "anyka,ak3918ev200-gpio");
	if (other) { of_node_put(other); goto fail; }
	if (!of_device_is_available(gpio_np) || of_address_to_resource(gpio_np, 0, &res) ||
	    res.start != GPIO_PHYS || resource_size(&res) != MAP_SIZE) goto fail;
	sys_np = of_parse_phandle(gpio_np, "anyka,sysctrl", 0);
	if (!sys_np || !of_device_is_compatible(sys_np, "anyka,ak3918ev200-intc") ||
	    !of_device_is_compatible(sys_np, "syscon") ||
	    of_address_to_resource(sys_np, 0, &res) || res.start != SYS_PHYS ||
	    resource_size(&res) != MAP_SIZE) goto fail;
	provider = gpio_device_find(gpio_np, exact_chip);
	if (!provider) goto fail;
	match = (struct chip_match) { .label = gpio_device_get_label(provider) };
	if (!match.label) goto fail;
	gpio_device_find(&match, count_chip);
	if (match.count != 1) goto fail;
	gpio_ro = ioremap(GPIO_PHYS, MAP_SIZE);
	sys_ro = ioremap(SYS_PHYS, MAP_SIZE);
	if (!gpio_ro || !sys_ro) { ret = -ENOMEM; goto fail; }
	consumer = root_device_register("qc07f-white-consumer");
	if (IS_ERR(consumer)) { ret = PTR_ERR(consumer); consumer = NULL; goto fail; }
	lookup = kzalloc(struct_size(lookup, table, 2), GFP_KERNEL);
	if (!lookup) { ret = -ENOMEM; goto fail; }
	lookup->dev_id = dev_name(consumer);
	lookup->table[0] = GPIO_LOOKUP_IDX(match.label, 5, "white", 0,
					 GPIO_ACTIVE_HIGH | GPIO_PERSISTENT);
	gpiod_add_lookup_table(lookup);
	hrtimer_setup(&white_timer, white_expired, CLOCK_MONOTONIC, HRTIMER_MODE_ABS_HARD);
	ret = misc_register(&white_misc);
	if (!ret) return 0; /* NO line request, direction, latch, mux, pull or timer. */
fail:
	metadata_free();
	return ret;
}
static void __exit white_exit(void)
{
	unsigned long flags;
	misc_deregister(&white_misc);
	/* Open fds pin module; forced unload unsupported. Parent-reviewed unload is
	 * the ONLY reservation release after any effect (including a quarantine).
	 * Exit cannot fail: unknown pad is logged, NEVER claimed electrically safe.
	 */
	hrtimer_cancel(&white_timer);
	raw_spin_lock_irqsave(&effect_lock, flags);
	if (pin && clear_owned())
		pr_err("qc07f-white: unload fault %u, GPIO5 state UNKNOWN\n", policy.fault);
	policy.setup = 0;
	raw_spin_unlock_irqrestore(&effect_lock, flags);
	if (pin) { gpiod_put(pin); pin = NULL; }
	metadata_free();
}
module_init(white_init);
module_exit(white_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("QC07F exact kernel75 GPIO5 white control, offline candidate");
