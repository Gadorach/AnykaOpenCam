// SPDX-License-Identifier: GPL-2.0-or-later
/* Shared production policy + explicit host-only gpiolib/effect simulator.
 * NOT execution of kernel code, NOT native/electrical timing qualification.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "white_policy.h"
struct model {
	struct qw_policy p;
	uint32_t out, dir, in;
	unsigned int mux, foreign, owned, requested, writes, high_writes, timer, owner;
	unsigned int stuck_pad, direction_error, set_error, identity_bad;
	unsigned long long now;
};
static struct model m;
static unsigned int cases;
#define LINE (1U << 5)
static struct qw_sample sample(void)
{
	return (struct qw_sample) { !!(m.out & LINE), !!(m.in & LINE), !!(m.dir & LINE), m.mux };
}
static void reset(unsigned int input_high)
{
	memset(&m, 0, sizeof(m)); m.mux = 1;
	/* Every neighbour bit set: simulated driver RMW must preserve ALL of them. */
	m.out = m.dir = m.in = ~LINE;
	if (input_high) m.in |= LINE;
}
static void write_line(unsigned int value)
{
	assert(m.owned); m.writes++; m.high_writes += !!value;
	if (value) assert(m.timer && m.p.active && m.p.until == m.now + QW_LIMIT_NS);
	m.out = (m.out & ~LINE) | (value ? LINE : 0);
	if (!m.stuck_pad && (m.dir & LINE)) m.in = (m.in & ~LINE) | (m.out & LINE);
	if (m.set_error) m.p.fault |= QW_FAULT_GPIO;
	assert((m.out & ~LINE) == ~LINE && (m.dir & ~LINE) == ~LINE && (m.in & ~LINE) == ~LINE);
}
static int clear(void)
{
	if (!m.owned) return 0;
	write_line(0); m.p.active = 0; m.p.until = 0; m.owner = 0;
	return qw_valid(&m.p, sample());
}
static int off(void)
{
	if (!m.owned) {
		if (!m.mux || m.foreign) return -EBUSY;
		m.requested++;
		if (m.identity_bad) return -EOPNOTSUPP;
		m.owned = 1;
	}
	m.timer = 0; /* synchronous cancellation outside simulated effect lock */
	qw_prior(&m.p, sample());
	if (m.p.setup) qw_valid(&m.p, sample());
	if (!m.mux) { m.p.fault |= QW_FAULT_CONFIG; }
	else {
		write_line(0); /* latch before OE; initial HIGH input is expressly allowed */
		if (!m.direction_error) {
			m.dir |= LINE;
			if (!m.stuck_pad) m.in &= ~LINE;
		} else m.p.fault |= QW_FAULT_GPIO;
	}
	int ret = clear();
	if (m.mux && qw_low(sample())) m.p.setup = 1;
	m.p.sequence++; return ret;
}
static int on(unsigned int owner)
{
	int ret = qw_admit(&m.p, QW_ON);
	if (ret) return ret;
	ret = qw_valid(&m.p, sample());
	if (!ret) {
		m.p.until = m.now + QW_LIMIT_NS; m.p.active = 1; m.owner = owner;
		m.timer = 1; /* BEFORE high, same absolute deadline, NEVER rearm ON. */
		write_line(1); ret = qw_valid(&m.p, sample());
	}
	if (ret) clear();
	m.p.sequence++; return ret;
}
static void status(void)
{
	unsigned int writes = m.writes, timer = m.timer, owned = m.owned;
	if (m.p.setup) qw_valid(&m.p, sample());
	(void)qw_flags(&m.p, sample());
	assert(m.writes == writes && m.timer == timer && m.owned == owned);
}
static void close_fd(unsigned int owner)
{
	if (m.owner == owner) { m.timer = 0; clear(); m.p.sequence++; }
}
static void expire(void)
{
	assert(m.timer); m.timer = 0;
	if (m.p.active) { clear(); m.p.sequence++; }
}
static void unload(void)
{
	m.timer = 0;
	if (m.owned) clear();
	/* Explicit reviewed unload is the exception to reservation quarantine.
	 * A void module_exit cannot refuse; a fault is not electrical OFF proof.
	 */
	m.owned = 0;
}
int main(void)
{
	for (unsigned int prior_dir = 0; prior_dir < 2; prior_dir++)
	for (unsigned int prior_latch = 0; prior_latch < 2; prior_latch++)
	for (unsigned int prior_pad = 0; prior_pad < 2; prior_pad++) {
		reset(prior_pad); m.dir |= prior_dir ? LINE : 0; m.out |= prior_latch ? LINE : 0;
		status(); assert(!m.owned && !m.writes && !m.timer && !m.requested);
		assert(on(1) == -EPERM && !m.writes);
		assert(off() == 0 && m.owned && qw_low(sample()) && m.p.setup);
		assert(m.p.prior_pad == prior_pad && m.p.prior_direction == prior_dir);
		assert(m.high_writes == 0 && !m.timer);
		assert(qw_flags(&m.p, sample()) == 13);
		unsigned int writes = m.writes; close_fd(99); status();
		assert(m.writes == writes && m.owned); unload(); cases++;
	}
	for (unsigned int refusal = 0; refusal < 3; refusal++) {
		reset(1);
		if (refusal == 0) m.foreign = 1;
		if (refusal == 1) m.mux = 0;
		if (refusal == 2) m.identity_bad = 1;
		assert(off() < 0 && !m.owned && !m.writes && !m.timer);
		status(); unload(); assert(!m.writes && !m.high_writes); cases++;
	}
	reset(1); assert(!off()); m.now = 9000000000ULL;
	assert(!on(7) && m.p.active && m.owner == 7 && qw_flags(&m.p, sample()) == 7);
	unsigned long long deadline = m.p.until, sequence = m.p.sequence;
	unsigned int writes = m.writes;
	assert(on(8) == -EALREADY && m.p.until == deadline && m.p.sequence == sequence && m.writes == writes);
	close_fd(8); status(); assert(m.p.active && m.writes == writes);
	m.now = deadline; assert(qw_remaining(m.p.until, m.now) == 0 && m.p.active);
	/* A due-but-not-run timer is still ACTIVE: cannot extend with a second ON. */
	assert(on(8) == -EALREADY && m.p.until == deadline);
	expire(); assert(!m.p.active && !m.owner && qw_low(sample()) && m.owned); cases++;
	reset(1); assert(!off() && !on(7)); close_fd(7);
	assert(!m.p.active && !m.timer && qw_low(sample()) && m.owned); cases++;
	reset(0); assert(!off() && !on(1)); assert(!off());
	assert(!m.timer && !m.owner && qw_low(sample()) && m.owned); cases++;
	reset(0); assert(!off() && !on(1)); unload();
	assert(!m.owned && !m.timer && qw_low(sample())); cases++;
	/* Sticky source faults permanently block ON, but explicit OFF still tries
	 * only the owned line and does not drop the uncertain reservation.
	 */
	for (unsigned int fault = 0; fault < 6; fault++) {
		reset(1);
		if (fault == 0) m.direction_error = 1;
		if (fault == 1) m.stuck_pad = 1;
		if (fault == 2) m.set_error = 1;
		if (fault < 3) assert(off() == -EIO);
		else {
			assert(!off() && !on(1));
			if (fault == 3) m.mux = 0;
			if (fault == 4) m.dir &= ~LINE;
			if (fault == 5) m.in &= ~LINE;
			writes = m.writes; status(); assert(m.writes == writes && m.p.fault);
		}
		assert(m.p.fault && m.owned);
		unsigned int high = m.high_writes;
		assert(on(2) == -EIO && m.high_writes == high);
		writes = m.writes; (void)off();
		assert(m.owned && m.p.fault && m.writes > writes && m.high_writes == high);
		m.stuck_pad = m.direction_error = m.set_error = 0; m.mux = 1;
		assert(off() == -EIO && qw_low(sample()) && m.owned);
		assert(on(1) == -EIO); unload(); cases++;
	}
	reset(0); assert(!off()); m.stuck_pad = 1;
	assert(on(1) == -EIO && !m.p.active && m.owned && m.out == ~LINE);
	cases++;
	reset(1); assert(!off()); m.dir &= ~LINE; m.in |= LINE;
	/* OFF is allowed to establish LOW again, but cannot erase observed drift
	 * from a prior verified setup, even without an intervening STATUS.
	 */
	assert(off() == -EIO && qw_low(sample()) && m.p.fault && m.owned);
	assert(on(1) == -EIO); cases++;
	assert(qw_admit(&m.p, 0) == -ENOTTY);
	assert(qw_admit(&m.p, QW_STATUS) == 0 && qw_admit(&m.p, QW_OFF) == 0);
	assert(qw_remaining(4900000000ULL, 0) == 4900);
	assert(qw_remaining(4900000000ULL, 4899999999ULL) == 1);
	assert(qw_remaining(4900000000ULL, 4900000000ULL) == 0);
	printf("WHITE_POLICY_PASS cases=%u host_simulation_not_native\n", cases);
	return 0;
}
