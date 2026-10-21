// SPDX-License-Identifier: GPL-2.0-only
/* Explicit READ-ONLY observer of already-qualified camera registers.
 * No resource takeover, IRQ request, DMA allocation, clock/reset/write ioctl.
 * MMIO20000000/status10/table18/order1c from retainedretailREA contracts.
 * ExistingISP driver remains sole hardware/DMA owner. Reads only registers
 * already read withoutack side effects by that driver; never oldDMApixels.
 */
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/mfd/syscon.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>
#include "qc07f_isp_observer.h"
static void __iomem *base;
static struct regmap *sys;
static long observe(struct file *file,unsigned int command,unsigned long arg)
{
 struct qc_isp_observation o;
 if(command!=QC_ISP_OBSERVE)return -ENOTTY;
 o.control=readl(base);o.status=readl(base+0x10);
 o.table=readl(base+0x18);o.order=readl(base+0x1c);
 o.isp_irq_enable=readl(base+0x14);
 if(regmap_read(sys,0x1c,&o.clock_gate)||regmap_read(sys,0x20,&o.reset)||regmap_read(sys,0x14,&o.pll))return -EIO;
 if(regmap_read(sys,0x24,&o.parent_mask)||regmap_read(sys,0x4c,&o.parent_pending))return -EIO;
 return copy_to_user((void __user *)arg,&o,sizeof(o))?-EFAULT:0;
}
static const struct file_operations ops={.owner=THIS_MODULE,.unlocked_ioctl=observe};
static struct miscdevice node={.minor=MISC_DYNAMIC_MINOR,.name="qc07f-isp-observer",.fops=&ops,.mode=0400};
static int __init start(void)
{
 int result;
 sys=syscon_regmap_lookup_by_compatible("anyka,ak3918ev200-intc");if(IS_ERR(sys))return PTR_ERR(sys);
 base=ioremap(0x20000000,0x24);if(!base)return -ENOMEM;
 result=misc_register(&node);if(result)iounmap(base);
 return result;
}
static void __exit end(void){misc_deregister(&node);iounmap(base);}
module_init(start);module_exit(end);
MODULE_LICENSE("GPL");MODULE_DESCRIPTION("QC07F explicit read-only ISP register observer; no takeover or writes");
