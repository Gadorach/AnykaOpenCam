// SPDX-License-Identifier: GPL-2.0-only
/* SOURCE PROTOTYPE ONLY: no .ko built/loaded/staged by this unit.
 * Init always refuses admission. No device, sysfs, parameter or exported API.
 * No enabling compiler flag exists. A future reviewed source revision is needed
 * after exact ABI/timing/ownership gates; do not patch this at the bench ad hoc.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/preempt.h>
#include <linux/irqflags.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/map.h>

/* Static mappings are SDK facts; retail mapping/attributes still need admission.
 * No GPIO/mux/direction API is called: those can silently change other resources.
 */
#define IC_SYS74() readl(AK_VA_SYSCTRL + 0x74)
#define IC_SYS78() readl(AK_VA_SYSCTRL + 0x78)
#define IC_DIR0() readl(AK_VA_GPIO)
#define IC_OUT0() readl(AK_VA_GPIO + 0x0c)
#define IC_WRITE_OUT0(v) writel((v), AK_VA_GPIO + 0x0c)
#define IC_LOCK(f) do { preempt_disable(); local_irq_save(f); } while (0)
#define IC_UNLOCK(f) do { local_irq_restore(f); preempt_enable(); } while (0)
#define IC_DELAY_1000US() udelay(1000)
#define IC_ALLOWED 0
#define IC_NAME __maybe_unused ircut_pulse_disabled
#include "pulse_sequence.h"

static int __init ircut_no_admission(void)
{
    return -EOPNOTSUPP; /* No read or write on module initialization. */
}
module_init(ircut_no_admission);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("QC07F IR-cut disabled source prototype; NOT native admitted");
