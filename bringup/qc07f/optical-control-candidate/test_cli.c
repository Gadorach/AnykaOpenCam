// SPDX-License-Identifier: GPL-2.0-or-later
/* Include the real implementation; ALL device/process/clock/stream calls mocked.
 * No real fork, sleep, device access or target binary execution is possible here.
 */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include "optical_uapi.h"

static uid_t m_geteuid(void);
static int m_open(const char *, int, ...);
static int m_fstat(int, struct stat *);
static int m_ioctl(int, unsigned long, ...);
static int m_clock_gettime(clockid_t, struct timespec *);
static pid_t m_fork(void);
static long m_syscall(long, ...);
static int m_clock_nanosleep(clockid_t, int, const struct timespec *, struct timespec *);
static int m_close(int);
static void m_exit(int) __attribute__((noreturn));
static ssize_t m_write(int, const void *, size_t);
#define geteuid m_geteuid
#define open m_open
#define fstat m_fstat
#define ioctl m_ioctl
#define clock_gettime m_clock_gettime
#define fork m_fork
#define syscall m_syscall
#define clock_nanosleep m_clock_nanosleep
#define close m_close
#define _exit m_exit
#define write m_write
#define main cli_main
#include "optical_cli.c"
#undef main
#undef geteuid
#undef open
#undef fstat
#undef ioctl
#undef clock_gettime
#undef fork
#undef syscall
#undef clock_nanosleep
#undef close
#undef _exit
#undef write

struct model {
	int root, fd, open_error, stat_error, regular, command_error, command_ret;
	int snapshot_error, snapshot_ret, fork_error, fork_child, close_error, clock_error;
	int opens, stats, commands, snapshots, forks, clocks, closes, writes;
	int child_mode, parent_open, child_open, ir_owned, ranges, sleeps;
	int self_mode, exited;
	int range_error, sleep_interrupts, sleep_error, write_interrupts;
	int write_error, write_zero, write_chunk, alter_snapshot;
	size_t alter_offset;
	uint32_t alter_value;
	int inherited[64], inherited_high;
	unsigned int lo[2], hi[2];
	unsigned long command, expected;
	struct timespec now, slept;
	struct qo_status state;
	char output[4096], events[128];
	size_t used, event_used;
};
static struct model m;
static jmp_buf child_exit;
static int case_count;
static const char nonce[] = "0123456789abcdef0123456789abcdef";

static void event(char c)
{
	assert(m.event_used + 1 < sizeof(m.events));
	m.events[m.event_used++] = c;
	m.events[m.event_used] = '\0';
}

static void reset(void)
{
	memset(&m, 0, sizeof(m));
	m.root = 1;
	for (int i = 0; i < 64; i++) m.inherited[i] = 1;
	m.inherited_high = 1;
	m.fd = 7;
	m.now = (struct timespec){ .tv_sec = 120, .tv_nsec = 456789123 };
	m.state = (struct qo_status){
		.abi = 1, .setup = 1, .owned_mask = 7, .direction_mask = 7,
		.mux_ok = 1, .release_verified = 1, .pulse_us = 10000,
		.ir_limit_ms = 4900, .cooldown_remaining_ms = 1876,
		.sequence = UINT64_C(18364758544493064720),
		.monotonic_ns = UINT64_C(121456789123),
	};
}

static uid_t m_geteuid(void) { return m.root ? 0U : 1000U; }
static int m_open(const char *path, int flags, ...)
{
	assert(!m.child_mode);
	assert(!strcmp(path, "/dev/qc07f-optical"));
	assert(flags == (O_RDWR | O_CLOEXEC | O_NOFOLLOW));
	m.opens++;
	event('O');
	if (m.open_error) { errno = m.open_error; return -1; }
	m.parent_open = 1;
	return m.fd;
}

static int m_fstat(int fd, struct stat *st)
{
	assert(!m.child_mode && fd == m.fd && m.parent_open);
	m.stats++;
	event('F');
	if (m.stat_error) { errno = m.stat_error; return -1; }
	memset(st, 0, sizeof(*st));
	st->st_mode = (mode_t)(m.regular ? S_IFREG : S_IFCHR);
	return 0;
}

static int m_ioctl(int fd, unsigned long cmd, ...)
{
	va_list ap;
	assert(!m.child_mode && fd == m.fd && m.parent_open);
	va_start(ap, cmd);
	if (cmd == QO_STATUS) {
		struct qo_status *out = va_arg(ap, struct qo_status *);
		m.snapshots++;
		event('S');
		m.now.tv_sec++; /* Delayed snapshot must not move entry+5s. */
		if (!m.snapshot_error && !m.snapshot_ret) {
			*out = m.state;
			if (m.alter_snapshot)
				memcpy((char *)out + m.alter_offset, &m.alter_value, sizeof(m.alter_value));
		}
	} else {
		assert(va_arg(ap, unsigned long) == 0UL);
		m.commands++;
		m.command = cmd;
		event('I');
		assert(cmd == m.expected && m.commands == 1);
		if (cmd == QO_IR_ON && !m.command_error && !m.command_ret) {
			m.ir_owned = 1;
			m.state.ir_active = 1;
			m.state.ir_latch = m.state.ir_pad = 1;
			m.state.ir_remaining_ms = 4876;
		}
	}
	va_end(ap);
	if (cmd == QO_STATUS) {
		if (m.snapshot_error) { errno = m.snapshot_error; return -1; }
		return m.snapshot_ret;
	}
	if (m.command_error) { errno = m.command_error; return -1; }
	return m.command_ret;
}

static int m_clock_gettime(clockid_t clock, struct timespec *ts)
{
	assert(!m.child_mode && clock == CLOCK_MONOTONIC);
	m.clocks++;
	event('T');
	if (m.clock_error) { errno = m.clock_error; return -1; }
	*ts = m.now;
	return 0;
}

static pid_t m_fork(void)
{
	assert(!m.child_mode && m.command == QO_IR_ON && m.commands == 1);
	assert(m.ir_owned && m.parent_open && !m.snapshots);
	m.forks++;
	event('K');
	if (m.fork_error) { errno = m.fork_error; return -1; }
	m.child_open = 1;
	if (m.fork_child) {
		m.child_mode = 1;
		m.parent_open = 0; /* This invocation now models only the child copy. */
		for (int i = 0; i < 64; i++) m.inherited[i] = 1;
		m.inherited_high = 1;
		m.now.tv_sec += 2; /* A delayed fork must NOT move the entry deadline. */
		return 0;
	}
	/* Positive branch returns before the modeled child retires; no real fork. */
	return 931;
}

static long m_syscall(long nr, ...)
{
	va_list ap;
	unsigned int first, last, flags;
	int i;
	assert((m.child_mode || m.self_mode) && nr == SYS_close_range && m.ranges < 2);
	if (m.self_mode) {
		assert(!m.forks && m.parent_open && m.used && m.output[m.used - 1] == '\n');
		assert(strstr(m.output, "\"result\":\"OK\""));
	}
	event('R');
	va_start(ap, nr);
	first = va_arg(ap, unsigned int);
	last = va_arg(ap, unsigned int);
	flags = va_arg(ap, unsigned int);
	va_end(ap);
	assert(flags == 0 && first <= last);
	m.lo[m.ranges] = first;
	m.hi[m.ranges] = last;
	m.ranges++;
	if (m.range_error == m.ranges) { errno = ENOSYS; return -1; }
	for (i = 0; i < 64; i++)
		if ((unsigned int)i >= first && (unsigned int)i <= last)
			m.inherited[i] = 0;
	if (first <= 10000U && last >= 10000U)
		m.inherited_high = 0;
	assert(m.inherited[m.fd]);
	return 0;
}

static int m_clock_nanosleep(clockid_t clock, int flags,
			     const struct timespec *deadline, struct timespec *remain)
{
	int i;
	assert((m.child_mode || m.self_mode) && clock == CLOCK_MONOTONIC && flags == TIMER_ABSTIME);
	assert(!remain && !m.inherited_high);
	for (i = 0; i < 64; i++)
		assert(m.inherited[i] == (i == m.fd));
	assert(deadline->tv_sec == 125 && deadline->tv_nsec == 456789123);
	if (m.sleeps)
		assert(m.slept.tv_sec == deadline->tv_sec && m.slept.tv_nsec == deadline->tv_nsec);
	m.slept = *deadline;
	m.sleeps++;
	event('L');
	if (m.sleeps <= m.sleep_interrupts)
		return EINTR;
	return m.sleep_error;
}

static int m_close(int fd)
{
	assert(fd == m.fd);
	m.closes++;
	event('C');
	if (m.child_mode) {
		assert(m.child_open);
		m.child_open = m.inherited[fd] = 0;
	} else {
		assert(m.parent_open);
		m.parent_open = 0;
		m.inherited[fd] = 0;
	}
	if (!m.child_open && !m.parent_open)
		m.ir_owned = 0;
	if (m.close_error) { errno = m.close_error; return -1; }
	return 0;
}

static void m_exit(int status)
{
	assert((m.child_mode || m.self_mode) && status == 0 && !m.child_open && !m.parent_open);
	m.exited = 1;
	event('X');
	/* Model _exit's close-all even after failed close_range. */
	memset(m.inherited, 0, sizeof(m.inherited));
	m.inherited_high = 0;
	longjmp(child_exit, 1);
}

static ssize_t m_write(int fd, const void *data, size_t len)
{
	assert(!m.child_mode && fd == STDOUT_FILENO);
	assert((!m.parent_open || m.self_mode) && len < 2048 && !m.ranges && !m.sleeps);
	m.writes++;
	event('W');
	m.now.tv_nsec += 100; /* Output delay must not move the fixed deadline. */
	if (m.writes <= m.write_interrupts) { errno = EINTR; return -1; }
	if (m.write_error) { errno = m.write_error; return -1; }
	if (m.write_zero) return 0;
	if (m.write_chunk && len > (size_t)m.write_chunk)
		len = (size_t)m.write_chunk;
	assert(m.used + len < sizeof(m.output));
	memcpy(m.output + m.used, data, len);
	m.used += len;
	m.output[m.used] = '\0';
	return (ssize_t)len;
}

static int invoke(const char *action, const char *request_nonce, int argc)
{
	char *argv[] = { (char *)"qc07f-optical-cli", (char *)action,
			(char *)request_nonce, NULL };
	m.self_mode = !strcmp(action, "--ir-on-hold");
	return cli_main(argc, argv);
}

static int invoke_self(void)
{
	if (!setjmp(child_exit))
		return invoke("--ir-on-hold", nonce, 3);
	assert(m.self_mode && m.exited);
	return 0;
}

static void record(const char *name, int ret)
{
	assert(m.opens <= 1 && m.commands <= 1 && m.snapshots <= 1 && m.forks <= 1);
	assert(!m.parent_open);
	case_count++;
	printf("RECEIPT %s %d %s", name, ret, m.output);
	assert(m.used && m.output[m.used - 1] == '\n');
}

static void action_case(const char *action, unsigned long cmd)
{
	int ret;
	reset(); m.expected = cmd;
	ret = invoke(action, nonce, 3);
	assert(ret == 0 && m.opens == 1 && m.stats == 1 && m.snapshots == 1);
	assert(m.commands == (cmd != QO_STATUS) && m.closes == 1);
	assert(m.forks == (cmd == QO_IR_ON));
	if (cmd == QO_IR_ON) {
		assert(m.ir_owned && m.child_open && !strcmp(m.events, "OFTIKSCW"));
		assert(!m.sleeps); /* Parent returned without waiting for holder. */
	} else {
		assert(!m.ir_owned && !m.child_open);
	}
	record(action, ret);
}

static void refusals(void)
{
	static const char *const bad[] = { "", "0", "0123456789abcdef0123456789abcdeff",
		"0123456789abcdef0123456789abcdefff0", "0123456789ABCDEF0123456789ABCDEF",
		"0123456789abcdef0123456789abcdeg", "\"; reboot; #" };
	size_t i;
	int ret;
	for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		reset(); ret = invoke("pulse-a", bad[i], 3);
		assert(ret == 1 && !m.opens && !m.commands);
		record("bad-nonce", ret);
	}
	reset(); ret = invoke("--arbitrary-pin", nonce, 3);
	assert(ret == 1 && !m.opens); record("bad-action", ret);
	reset(); ret = invoke("setup", nonce, 3);
	assert(ret == 1 && !m.opens); record("unflagged-setup", ret);
	reset(); ret = invoke("close", nonce, 3);
	assert(ret == 1 && !m.opens); record("unsupported-close", ret);
	reset(); ret = invoke("status", nonce, 2);
	assert(ret == 1 && !m.opens); record("missing-nonce", ret);
	reset(); ret = invoke("ir-on", nonce, 4);
	assert(ret == 1 && !m.opens); record("extra-arg", ret);
	reset(); m.root = 0; ret = invoke("pulse-b", nonce, 3);
	assert(ret == 1 && !m.opens); record("nonroot", ret);
	reset(); m.open_error = ELOOP; ret = invoke("status", nonce, 3);
	assert(ret == 1 && !m.stats && !m.closes && !m.snapshots); record("open-refusal", ret);
	reset(); m.stat_error = EIO; ret = invoke("pulse-a", nonce, 3);
	assert(ret == 1 && m.closes == 1 && !m.commands); record("fstat-refusal", ret);
	reset(); m.regular = 1; ret = invoke("pulse-a", nonce, 3);
	assert(ret == 1 && m.closes == 1 && !m.commands); record("nonchar", ret);
	reset(); m.fd = 1; ret = invoke("pulse-a", nonce, 3);
	assert(ret == 1 && m.closes == 1 && !m.commands && !m.stats); record("low-fd", ret);
}

static void failures(void)
{
	int ret;
	reset(); m.expected = QO_PULSE_A; m.snapshot_error = EFAULT;
	ret = invoke("pulse-a", nonce, 3);
	assert(ret == 1 && m.commands == 1 && m.snapshots == 1); record("snapshot-failure", ret);
	reset(); m.expected = QO_PULSE_B; m.command_error = EINTR;
	ret = invoke("pulse-b", nonce, 3);
	assert(ret == 1 && m.commands == 1 && m.snapshots == 1); record("command-eintr", ret);
	reset(); m.expected = QO_IR_ON; m.command_error = EALREADY;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && m.commands == 1 && !m.forks); record("command-errno", ret);
	reset(); m.expected = QO_PULSE_A; m.command_ret = 2;
	ret = invoke("pulse-a", nonce, 3);
	assert(ret == 1 && m.commands == 1); record("unknown-command-result", ret);
	reset(); m.expected = QO_IR_ON; m.command_ret = 1;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && m.commands == 1 && !m.forks); record("unknown-on-result", ret);
	reset(); m.expected = QO_IR_ON; m.snapshot_error = EIO;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && m.forks == 1 && m.child_open); record("postfork-snapshot-failure", ret);
	reset(); m.expected = QO_IR_ON; m.fork_error = EAGAIN;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && m.commands == 1 && m.forks == 1 && !m.snapshots);
	assert(!m.ir_owned && !m.child_open && m.closes == 1); record("fork-failure", ret);
	reset(); m.expected = QO_IR_ON; m.state.abi = 2;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && m.forks == 1); record("unknown-abi", ret);
	reset(); m.snapshot_ret = 1; ret = invoke("status", nonce, 3);
	assert(ret == 1 && !m.commands && m.snapshots == 1); record("unknown-status-result", ret);
	reset(); m.snapshot_error = EFAULT; ret = invoke("status", nonce, 3);
	assert(ret == 1 && !m.commands && m.snapshots == 1); record("status-failure", ret);
	reset(); m.state.abi = 2; ret = invoke("status", nonce, 3);
	assert(ret == 1 && !m.commands && m.snapshots == 1); record("status-unknown-abi", ret);
	reset(); m.expected = QO_IR_ON; m.clock_error = EIO;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && !m.commands && !m.forks); record("clock-failure", ret);
	reset(); m.expected = QO_IR_ON; m.now.tv_sec = INT_MAX;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && !m.commands && !m.forks); record("clock-overflow", ret);
	reset(); m.expected = QO_IR_ON; m.now.tv_sec = -1;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && !m.commands); record("clock-negative", ret);
	reset(); m.expected = QO_IR_ON; m.now.tv_nsec = 1000000000L;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && !m.commands); record("clock-invalid-nsec", ret);
	reset(); m.expected = QO_IR_OFF; m.close_error = EIO;
	ret = invoke("ir-off", nonce, 3);
	assert(ret == 1 && m.closes == 1 && m.commands == 1); record("close-failure", ret);
}

static void verification(int self)
{
	/* Vary every postfork ON verification field; no second ioctl/ON. */
	size_t i;
	static const size_t offsets[] = {
		offsetof(struct qo_status, setup), offsetof(struct qo_status, fault),
		offsetof(struct qo_status, owned_mask), offsetof(struct qo_status, ir_limit_ms),
		offsetof(struct qo_status, pair_latch), offsetof(struct qo_status, pair_pad),
		offsetof(struct qo_status, direction_mask), offsetof(struct qo_status, mux_ok),
		offsetof(struct qo_status, release_verified),
	};
	for (i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
		int ret;
		uint32_t value;
		reset(); m.expected = QO_IR_ON;
		memcpy(&value, (char *)&m.state + offsets[i], sizeof(value));
		value ^= 1U;
		memcpy((char *)&m.state + offsets[i], &value, sizeof(value));
		ret = invoke(self ? "--ir-on-hold" : "ir-on", nonce, 3);
		assert(ret == 1 && m.forks == !self && m.snapshots == 1);
		if (self) assert(!m.ir_owned && !m.ranges && !m.sleeps);
		record(self ? "self-invalid-on-snapshot" : "invalid-on-snapshot", ret);
	}
	/* These fields normally get set by our mocked command; corrupt only the
	 * separate postfork snapshot (expiry/foreign-root race, never auto-retry).
	 */
	for (i = 0; i < 5; i++) {
		static const size_t on_offsets[] = {
			offsetof(struct qo_status, ir_active), offsetof(struct qo_status, ir_latch),
			offsetof(struct qo_status, ir_pad), offsetof(struct qo_status, ir_remaining_ms),
			offsetof(struct qo_status, ir_remaining_ms),
		};
		int ret;
		reset(); m.expected = QO_IR_ON; m.alter_snapshot = 1;
		m.alter_offset = on_offsets[i]; m.alter_value = i == 4 ? 4901U : 0U;
		ret = invoke(self ? "--ir-on-hold" : "ir-on", nonce, 3);
		assert(ret == 1 && m.forks == !self && m.snapshots == 1);
		if (self) assert(!m.ir_owned && !m.ranges && !m.sleeps);
		record(self ? "self-invalid-on-snapshot" : "invalid-on-snapshot", ret);
	}
}

static void self_cases(void)
{
	int ret;
	/* Whole actual CLI body, with successful receipt before FD isolation,
	 * absolute sleep, last close and _exit. Never a fork or second ioctl.
	 */
	reset(); m.expected = QO_IR_ON;
	ret = invoke_self();
	assert(ret == 0 && m.exited && !m.forks && m.clocks == 1);
	assert(m.commands == 1 && m.snapshots == 1 && m.closes == 1 && !m.ir_owned);
	assert(!strcmp(m.events, "OFTISWRRLCX"));
	assert(m.ranges == 2 && m.sleeps == 1 && !m.inherited_high);
	for (int i = 0; i < 64; i++) assert(!m.inherited[i]);
	record("self-on", ret);
	reset(); m.expected = QO_IR_ON; m.write_chunk = 200; m.write_interrupts = 2;
	ret = invoke_self();
	assert(ret == 0 && m.exited && !m.forks && m.writes > 3 && m.sleeps == 1);
	record("self-short-write", ret);
	/* Failures never enter the holder. Unknown ioctl outcomes never replay. */
	for (int fault = 0; fault < 12; fault++) {
		static const char *const names[] = {
			"self-status-error", "self-status-positive", "self-abi-error",
			"self-command-error", "self-command-positive", "self-fstat-error",
			"self-fstat-close-error", "self-nonchar", "self-clock-error",
			"self-clock-overflow", "self-open-error", "self-low-fd",
		};
		reset(); m.expected = QO_IR_ON;
		switch (fault) {
		case 0: m.snapshot_error = EIO; break;
		case 1: m.snapshot_ret = 1; break;
		case 2: m.state.abi = 2; break;
		case 3: m.command_error = EINTR; break;
		case 4: m.command_ret = 1; break;
		case 5: m.stat_error = EIO; break;
		case 6: m.stat_error = m.close_error = EIO; break;
		case 7: m.regular = 1; break;
		case 8: m.clock_error = EIO; break;
		case 9: m.now.tv_sec = INT_MAX; break;
		case 10: m.open_error = ELOOP; break;
		case 11: m.fd = 1; break;
		}
		ret = invoke_self();
		assert(ret == 1 && !m.exited && !m.forks && !m.ranges && !m.sleeps);
		assert(!m.parent_open && !m.ir_owned && m.closes == (fault != 10));
		assert(m.commands == (fault < 5) && m.snapshots == (fault < 5));
		record(names[fault], ret);
	}
	for (int fault = 0; fault < 5; fault++) {
		reset(); m.expected = QO_IR_ON;
		switch (fault) {
		case 0: m.write_error = EPIPE; break;
		case 1: m.write_zero = 1; break;
		case 2: m.write_interrupts = 20; break;
		case 3: m.write_chunk = 1; break; /* Partial receipt, budget exhausted. */
		case 4: m.write_error = EIO; m.close_error = EIO; break;
		}
		ret = invoke_self();
		assert(ret == 1 && !m.exited && !m.forks && !m.ranges && !m.sleeps);
		assert(m.commands == 1 && m.snapshots == 1 && m.closes == 1 && !m.ir_owned);
		assert(m.writes == ((fault == 2 || fault == 3) ? 16 : 1));
		case_count++; /* No complete JSON receipt to parse. */
	}
	/* Holder failures after a positive receipt are not rewritten as FAILED.
	 * Kernel timer remains primary; exit/cleanup does not prove physical OFF.
	 */
	for (int fault = 0; fault < 6; fault++) {
		reset(); m.expected = QO_IR_ON;
		switch (fault) {
		case 0: m.range_error = 1; break;
		case 1: m.range_error = 2; break;
		case 2: m.sleep_error = EINVAL; break;
		case 3: m.sleep_interrupts = 100; break;
		case 4: m.close_error = EIO; break;
		case 5: m.fd = 3; m.sleep_interrupts = 4; break;
		}
		ret = invoke_self();
		assert(ret == 0 && m.exited && !m.forks && !m.ir_owned && !m.inherited_high);
		assert(m.commands == 1 && m.snapshots == 1 && m.closes == 1);
		if (fault < 2) assert(!m.sleeps);
		if (fault == 3) assert(m.sleeps == 64);
		if (fault == 5) assert(m.sleeps == 5);
		record("self-holder-exit", ret);
	}
}

static void output_cases(void)
{
	int ret;
	reset(); m.write_interrupts = 2; m.write_chunk = 200;
	ret = invoke("status", nonce, 3);
	assert(ret == 0 && m.writes > 3 && m.writes <= 16); record("short-write-eintr", ret);
	reset(); m.write_interrupts = 20; ret = invoke("status", nonce, 3);
	assert(ret == 1 && m.writes == 16 && m.snapshots == 1 && !m.used); case_count++;
	reset(); m.expected = QO_PULSE_A; m.write_error = EPIPE;
	ret = invoke("pulse-a", nonce, 3);
	assert(ret == 1 && m.writes == 1 && m.commands == 1); case_count++;
	reset(); m.expected = QO_IR_ON; m.write_error = EPIPE;
	ret = invoke("ir-on", nonce, 3);
	assert(ret == 1 && m.writes == 1 && m.commands == 1 && m.child_open && m.ir_owned);
	case_count++;
	reset(); m.write_zero = 1; ret = invoke("status", nonce, 3);
	assert(ret == 1 && m.writes == 1 && m.snapshots == 1); case_count++;
}

static void numeric_fields(void)
{
	uint32_t fields[16];
	int ret;
	reset();
	for (size_t i = 0; i < 16; i++) fields[i] = i == 0 ? 1U : (uint32_t)i * 11U;
	memcpy(&m.state, fields, sizeof(fields));
	ret = invoke("status", nonce, 3);
	assert(ret == 0); record("distinct-fields", ret);
	reset();
	for (size_t i = 0; i < 16; i++) fields[i] = UINT32_MAX;
	memcpy(&m.state, fields, sizeof(fields));
	m.state.sequence = m.state.monotonic_ns = UINT64_MAX;
	ret = invoke("status", nonce, 3);
	assert(ret == 1 && m.used < 2048); record("max-fields", ret);
}

static void included_child_branch(void)
{
	reset(); m.expected = QO_IR_ON; m.fork_child = 1;
	if (!setjmp(child_exit)) {
		(void)invoke("ir-on", nonce, 3);
		assert(0 && "child must _exit, never emit a parent receipt");
	}
	assert(m.commands == 1 && m.forks == 1 && !m.snapshots && !m.writes);
	assert(m.ranges == 2 && m.sleeps == 1 && m.closes == 1 && !m.ir_owned);
	assert(m.slept.tv_sec == 125); /* Not127 after simulated scheduling delay. */
	case_count++;
}

static void holder_case(int fd, int interrupted, int sleep_error, int range_error)
{
	const struct timespec deadline = { .tv_sec = 125, .tv_nsec = 456789123 };
	reset();
	m.fd = fd;
	m.child_mode = m.child_open = m.ir_owned = 1;
	m.sleep_interrupts = interrupted;
	m.sleep_error = sleep_error;
	m.range_error = range_error;
	for (int i = 0; i < 64; i++) m.inherited[i] = 1;
	m.inherited_high = 1;
	if (!setjmp(child_exit))
		keeper(fd, &deadline);
	assert(!m.opens && !m.stats && !m.commands && !m.snapshots && !m.forks);
	assert(!m.clocks && !m.writes && m.closes == 1 && !m.child_open && !m.ir_owned);
	assert(!m.inherited_high);
	for (int i = 0; i < 64; i++) assert(!m.inherited[i]);
	if (range_error)
		assert(!m.sleeps);
	else {
		assert(m.sleeps == (interrupted >= 64 ? 64 : interrupted + 1));
		assert(m.hi[m.ranges - 1] == UINT_MAX);
	}
	case_count++;
}

int main(void)
{
	action_case("status", QO_STATUS);
	action_case("pulse-a", QO_PULSE_A);
	action_case("pulse-b", QO_PULSE_B);
	action_case("ir-on", QO_IR_ON);
	action_case("ir-off", QO_IR_OFF);
	action_case("--setup", QO_SETUP);
	refusals(); failures(); verification(0); verification(1);
	output_cases(); numeric_fields(); self_cases();
	included_child_branch();
	holder_case(7, 0, 0, 0);
	holder_case(3, 4, 0, 0);
	holder_case(7, 100, 0, 0);
	holder_case(7, 0, EINVAL, 0);
	holder_case(7, 0, 0, 1);
	holder_case(7, 0, 0, 2);
	/* Defensive direct helper coverage for FD0; main never admits low FDs. */
	holder_case(0, 0, 0, 0);
	printf("CLI_MOCK_PASS cases=%d no_real_devices_or_forks\n", case_count);
	return 0;
}
