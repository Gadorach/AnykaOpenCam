/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QC07F_OPTICAL_POLICY_H
#define QC07F_OPTICAL_POLICY_H
#include "optical_uapi.h"
#ifdef __KERNEL__
#include <linux/errno.h>
#else
#include <errno.h>
#endif

#define QO_PULSE_US 10000U
#define QO_IR_MS 4900U /* 100ms margin below requested 5s native envelope. */
#define QO_COOLDOWN_NS 2000000000ULL
#define QO_IR_NS 4900000000ULL
#define QO_ALL 7U

struct qo_sample {
	unsigned int latch, pad, dir, mux_ok; /* compact GPIO6/7/23 => bits0/1/2 */
};
struct qo_policy {
	unsigned int setup, fault, ir_active;
	unsigned long long sequence, cooldown_until, ir_until;
};
static inline unsigned int qo_remaining(unsigned long long until,
					unsigned long long now)
{
	return until > now ? (unsigned int)((until - now + 999999) / 1000000) : 0;
}
static inline int qo_neutral(struct qo_sample s)
{
	return s.mux_ok && !s.latch && !s.pad;
}
static inline int qo_valid(struct qo_policy *p, struct qo_sample s)
{
	if (!s.mux_ok || s.dir != QO_ALL)
		p->fault |= QO_FAULT_CONFIG;
	if ((s.latch & 3) || (s.pad & 3) ||
	    (s.latch >> 2) != p->ir_active || (s.pad >> 2) != p->ir_active)
		p->fault |= QO_FAULT_READBACK;
	return p->fault ? -EIO : 0;
}
static inline int qo_admit(struct qo_policy *p, unsigned int cmd,
			  unsigned long long now)
{
	switch (cmd) {
	case QO_STATUS: case QO_IR_OFF: case QO_CLOSE:
		return 0;
	case QO_SETUP:
		return p->fault ? -EIO : p->setup ? -EALREADY : 0;
	case QO_PULSE_A: case QO_PULSE_B: case QO_IR_ON:
		if (p->fault) return -EIO;
		if (!p->setup) return -EPERM;
		if (cmd == QO_IR_ON)
			return p->ir_active ? -EALREADY : 0;
		return now < p->cooldown_until ? -EAGAIN : 0;
	default:
		return -ENOTTY;
	}
}
#endif
