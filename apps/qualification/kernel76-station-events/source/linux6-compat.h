/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef HGIC_LINUX6_COMPAT_H
#define HGIC_LINUX6_COMPAT_H
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#define PDE_DATA pde_data
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 16, 0)
#define del_timer timer_delete
#define del_timer_sync timer_delete_sync
#define from_timer timer_container_of
#endif
#endif
