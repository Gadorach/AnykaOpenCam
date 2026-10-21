/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QC07F_WHITE_POLICY_H
#define QC07F_WHITE_POLICY_H
#include "white_uapi.h"
#ifdef __KERNEL__
#include <linux/errno.h>
#else
#include <errno.h>
#endif
struct qw_sample { unsigned int latch, pad, dir, mux_ok; };
struct qw_policy {
	unsigned int setup, fault, active, prior_valid, prior_pad, prior_direction;
	unsigned long long sequence, until;
};
static inline unsigned int qw_remaining(unsigned long long until, unsigned long long now)
{
	return until > now ? (unsigned int)((until - now + 999999) / 1000000) : 0;
}
static inline int qw_low(struct qw_sample s)
{
	return s.mux_ok && s.dir == 1 && !s.latch && !s.pad;
}
static inline int qw_valid(struct qw_policy *p, struct qw_sample s)
{
	if (!s.mux_ok || s.dir != 1) p->fault |= QW_FAULT_CONFIG;
	if (s.latch != p->active || s.pad != p->active) p->fault |= QW_FAULT_READBACK;
	return p->fault ? -EIO : 0;
}
static inline void qw_prior(struct qw_policy *p, struct qw_sample s)
{
	p->prior_valid = 1;
	p->prior_pad = s.pad;
	p->prior_direction = s.dir;
}
static inline int qw_admit(struct qw_policy *p, unsigned int cmd)
{
	switch (cmd) {
	case QW_STATUS: case QW_OFF: return 0;
	case QW_ON:
		if (p->fault) return -EIO;
		if (!p->setup) return -EPERM;
		return p->active ? -EALREADY : 0;
	default: return -ENOTTY;
	}
}
static inline unsigned int qw_flags(const struct qw_policy *p, struct qw_sample s)
{
	return (p->setup ? QW_SETUP : 0) | (p->active ? QW_ACTIVE : 0) |
		(p->prior_valid ? QW_PRIOR_VALID : 0) |
		(p->setup && !p->active && qw_low(s) ? QW_OFF_VERIFIED : 0);
}
#endif
