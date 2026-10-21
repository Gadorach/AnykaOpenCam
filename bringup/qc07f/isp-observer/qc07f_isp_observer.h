/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_ISP_OBSERVER_H
#define QC_ISP_OBSERVER_H
#include <linux/types.h>
#include <linux/ioctl.h>
struct qc_isp_observation { __u32 control,status,table,order,clock_gate,reset,pll,isp_irq_enable,parent_mask,parent_pending; };
#define QC_ISP_OBSERVE _IOR('I',0x76,struct qc_isp_observation)
#endif
