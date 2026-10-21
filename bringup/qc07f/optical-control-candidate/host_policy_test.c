/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Deterministic MODEL, not kernel scheduler/MMIO/electrical execution. Uses
 * exactly the module's admission/readback policy; fake transitions below.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "optical_policy.h"

struct model {
	struct qo_policy p;
	struct qo_sample s;
	unsigned int owned, foreign, requests, writes, owner, stuck_latch, stuck_pad;
	unsigned long long now;
};
static void init(struct model *m)
{
	memset(m, 0, sizeof(*m));
	m->s.mux_ok = 1;
}
static int setup(struct model *m)
{
	int i, r = qo_admit(&m->p, QO_SETUP, m->now);
	if (r) return r;
	if (!qo_neutral(m->s)) return -EBUSY;
	for (i = 0; i < 3; i++) {
		m->requests++;
		if (m->foreign & (1U << i)) { m->owned = 0; return -EBUSY; }
		m->owned |= 1U << i;
	}
	m->writes += 3;
	m->s.dir = QO_ALL;
	m->p.setup = 1;
	return qo_valid(&m->p, m->s);
}
static void off(struct model *m)
{
	m->s.latch = m->stuck_latch;
	m->s.pad = m->stuck_pad;
	m->p.ir_active = 0;
	m->p.ir_until = 0;
	m->owner = 0;
	m->writes++;
	qo_valid(&m->p, m->s);
}
static int effect(struct model *m, unsigned int cmd, unsigned int client)
{
	int r = qo_admit(&m->p, cmd, m->now);
	if (r) return r;
	r = qo_valid(&m->p, m->s);
	if (r) { off(m); return r; }
	if (cmd == QO_IR_ON) {
		m->p.ir_active = 1;
		m->p.ir_until = m->now + QO_IR_NS;
		m->owner = client;
		m->s.latch = m->s.pad = 4;
		m->writes++;
	} else {
		/* One serialized fake finite pulse, completion despite client death. */
		m->s.latch |= cmd == QO_PULSE_A ? 1 : 2;
		m->s.pad = m->s.latch;
		m->writes++;
		m->now += 10000000;
		m->s.latch &= ~3U;
		m->s.latch |= m->stuck_latch;
		m->s.pad = m->s.latch | m->stuck_pad;
		m->writes += 2;
		m->p.cooldown_until = m->now + QO_COOLDOWN_NS;
	}
	m->p.sequence++;
	return qo_valid(&m->p, m->s);
}
static void close_client(struct model *m, unsigned int client)
{
	if (m->owner == client) off(m);
}
static int close_device(struct model *m)
{
	off(m);
	if (m->p.fault) return -EIO; /* quarantine retains reservations */
	m->owned = 0;
	m->p.setup = 0;
	return 0;
}
static void tick(struct model *m, unsigned long long now)
{
	m->now = now;
	if (m->p.ir_active && now >= m->p.ir_until) off(m);
}
int main(void)
{
	struct model m;
	unsigned int i, before;
	unsigned long long deadline;
	_Static_assert(sizeof(struct qo_status) == 80, "UAPI size");
	_Static_assert(offsetof(struct qo_status, sequence) == 64, "UAPI seq");
	_Static_assert(offsetof(struct qo_status, monotonic_ns) == 72, "UAPI clock");
	_Static_assert(QO_STATUS == 0x80505160U, "Python status ioctl");
	_Static_assert(QO_SETUP == 0x5161 && QO_CLOSE == 0x5166, "Python ioctls");
	init(&m);
	assert(m.writes == 0 && qo_admit(&m.p, QO_STATUS, 0) == 0);
	assert(qo_admit(&m.p, QO_PULSE_A, 0) == -EPERM);
	assert(qo_admit(&m.p, 0xdead, 0) == -ENOTTY);
	assert(qo_admit(&m.p, QO_STATUS ^ (1U << 16), 0) == -ENOTTY);
	for (i = 0; i < 3; i++) {
		init(&m); m.foreign = 1U << i;
		assert(setup(&m) == -EBUSY && !m.owned && !m.writes);
	}
	init(&m); m.s.mux_ok = 0;
	assert(setup(&m) == -EBUSY && !m.writes && !m.requests);
	for (i = 0; i < 3; i++) {
		init(&m); m.s.latch = 1U << i;
		assert(setup(&m) == -EBUSY && !m.writes);
		init(&m); m.s.pad = 1U << i;
		assert(setup(&m) == -EBUSY && !m.writes);
	}
	init(&m); assert(setup(&m) == 0 && m.owned == 7);
	assert(setup(&m) == -EALREADY);
	assert(effect(&m, QO_PULSE_A, 1) == 0 && !(m.s.latch & 3));
	close_client(&m, 1); /* Client death after synchronous retirement. */
	assert(!(m.s.latch & 3) && !(m.s.pad & 3));
	assert(effect(&m, QO_PULSE_B, 2) == -EAGAIN); /* Serialized competing client. */
	tick(&m, m.p.cooldown_until - 1);
	assert(effect(&m, QO_PULSE_B, 2) == -EAGAIN);
	tick(&m, m.p.cooldown_until);
	assert(effect(&m, QO_PULSE_B, 2) == 0);
	assert(effect(&m, QO_IR_ON, 2) == 0);
	deadline = m.p.ir_until;
	assert(effect(&m, QO_IR_ON, 1) == -EALREADY && m.p.ir_until == deadline);
	before = m.writes; close_client(&m, 1);
	assert(m.writes == before && m.p.ir_active); /* unrelated STATUS close inert */
	close_client(&m, 2);
	assert(!m.p.ir_active && !m.s.latch);
	assert(effect(&m, QO_IR_ON, 3) == 0);
	tick(&m, m.p.ir_until - 1); assert(m.p.ir_active);
	tick(&m, m.p.ir_until); assert(!m.p.ir_active && !m.s.latch);
	assert(effect(&m, QO_IR_ON, 3) == 0);
	off(&m); /* concurrent OFF serialized, cancels pending target timer */
	tick(&m, m.now + QO_IR_NS); assert(!m.p.ir_active && !m.s.latch);
	assert(effect(&m, QO_IR_ON, 3) == 0);
	off(&m); m.owned = 0; m.p.setup = 0; /* unload/cancel: no later writes */
	before = m.writes; tick(&m, m.now + QO_IR_NS);
	assert(m.writes == before && !m.s.latch);
	init(&m); assert(!setup(&m)); m.s.pad = 1;
	assert(qo_valid(&m.p, m.s) == -EIO);
	m.s.pad = 0; assert(qo_valid(&m.p, m.s) == -EIO); /* sticky */
	assert(effect(&m, QO_PULSE_A, 1) == -EIO);
	assert(qo_admit(&m.p, QO_IR_OFF, 0) == 0);
	assert(qo_admit(&m.p, QO_CLOSE, 0) == 0);
	m.p.setup = 0; assert(setup(&m) == -EIO); /* no GUI rearm */
	init(&m); assert(!setup(&m)); m.s.mux_ok = 0;
	assert(effect(&m, QO_PULSE_A, 1) == -EIO && m.p.fault & QO_FAULT_CONFIG);
	init(&m); assert(!setup(&m)); m.s.dir &= ~1U;
	assert(effect(&m, QO_IR_ON, 1) == -EIO && m.p.fault & QO_FAULT_CONFIG);
	init(&m); assert(!setup(&m)); m.stuck_latch = 1;
	assert(effect(&m, QO_PULSE_A, 1) == -EIO);
	assert(m.p.fault & QO_FAULT_READBACK); /* release fails, never claim00 */
	assert(effect(&m, QO_PULSE_B, 2) == -EIO);
	assert(close_device(&m) == -EIO && m.owned == QO_ALL);
	init(&m); assert(!setup(&m)); assert(!effect(&m, QO_IR_ON, 1));
	m.stuck_pad = 4; off(&m); /* timer/unload release-fault model */
	assert(m.p.fault & QO_FAULT_READBACK);
	assert(effect(&m, QO_PULSE_A, 1) == -EIO);
	assert(qo_remaining(5000000000ULL, 0) == 5000);
	assert(qo_remaining(5000000000ULL, 4999999999ULL) == 1);
	assert(qo_remaining(5000000000ULL, 5000000001ULL) == 0);
	puts("PASS: shared policy + deterministic lifecycle model; NOT hardware/kernel execution");
	return 0;
}
