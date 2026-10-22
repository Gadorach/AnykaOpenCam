/* SPDX-License-Identifier: GPL-2.0-only
 * P2 fixed cached module status. No device pointer, raw event or writer endpoint.
 */
#include <linux/module.h>
#include "hgics.h"

static struct proc_dir_entry *attach_root;
static struct proc_dir_entry *attach_status;

static int attach_status_show(struct seq_file *seq, void *unused)
{
    struct hgic_attach_receipt r;
    hgic_attach_snapshot(&r);
    seq_puts(seq, "policy=control-owned-p3-source01 schema=2\n");
    seq_printf(seq, "generation=%u init=%u queued=%u starts=%u done=%u fwinfo=%u\n",
               r.generation, r.init_state, r.init_queued, r.init_starts,
               r.init_done, r.fwinfo_commands);
    seq_printf(seq, "first_error=%d first_phase=%u hw_registered=%u netdev_registered=%u "
               "netdev_up_seen=%u proc_registered=%u usb_registered=%u\n",
               r.first_error, r.first_phase, r.hw_registered, r.netdev_registered,
               r.netdev_up_seen, r.proc_registered, r.usb_registered);
    seq_printf(seq, "netdev_present=%u netdev_removed=%u netdev_teardown=%u "
               "netdev_removal_expected=%u\n",
               r.netdev_present, r.netdev_removed, r.netdev_teardown,
               r.netdev_removal_expected);
    return 0;
}

static int attach_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, attach_status_show, NULL);
}

static const struct proc_ops attach_status_ops = {
    .proc_open = attach_status_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

int hgics_attach_status_init(void)
{
    attach_root = proc_mkdir("hgics", NULL);
    if (!attach_root)
        return -ENOMEM; /* proc_mkdir exposes NULL, not a more specific errno */
    attach_status = proc_create_data("status", 0444, attach_root, &attach_status_ops, NULL);
    if (!attach_status) {
        proc_remove(attach_root);
        attach_root = NULL;
        return -ENOMEM;
    }
    return 0;
}

void hgics_attach_status_exit(void)
{
    if (attach_status) {
        proc_remove(attach_status);
        attach_status = NULL;
    }
    if (attach_root) {
        proc_remove(attach_root);
        attach_root = NULL;
    }
}
