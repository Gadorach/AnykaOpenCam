// SPDX-License-Identifier: GPL-2.0-or-later
/* The ACTUAL CLI body with every external syscall mocked. Host execution ONLY. */
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
#include "white_uapi.h"
static uid_t m_geteuid(void);
static int m_open(const char *, int, ...);
static int m_fstat(int, struct stat *);
static int m_ioctl(int, unsigned long, ...);
static int m_clock_gettime(clockid_t, struct timespec *);
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
#define syscall m_syscall
#define clock_nanosleep m_clock_nanosleep
#define close m_close
#define _exit m_exit
#define write m_write
#define main cli_main
#include "white_cli.c"
#undef main
#undef geteuid
#undef open
#undef fstat
#undef ioctl
#undef clock_gettime
#undef syscall
#undef clock_nanosleep
#undef close
#undef _exit
#undef write
struct model {
	int root, fd, opened, open_error, stat_error, regular, command_error, command_ret;
	int status_error, status_ret, close_error, clock_error, write_error, write_zero;
	int write_interrupts, write_chunk, range_error, sleep_interrupts, sleep_error;
	int opens, stats, commands, snapshots, closes, clocks, writes, ranges, sleeps, exited;
	int inherited[64], highfd, held, self;
	int alter; size_t alter_offset; uint32_t alter_value;
	unsigned long expected;
	struct timespec now;
	struct qw_status state;
	char output[4096], events[256]; size_t used, event_used;
};
static struct model m;
static jmp_buf done;
static unsigned int cases, receipts;
static const char nonce[] = "0123456789abcdef0123456789abcdef";
static void event(char c)
{
	assert(m.event_used + 1 < sizeof(m.events));
	m.events[m.event_used++] = c; m.events[m.event_used] = 0;
}
static void reset(void)
{
	memset(&m, 0, sizeof(m)); m.root = 1; m.fd = 7; m.highfd = 1;
	for (int i = 0; i < 64; i++) m.inherited[i] = 1;
	m.now = (struct timespec) { .tv_sec = 120, .tv_nsec = 456789123 };
	m.state = (struct qw_status) {
		.abi = 1, .flags = QW_SETUP | QW_PRIOR_VALID | QW_OFF_VERIFIED,
		.owned = 1, .direction = 1, .mux_ok = 1, .limit_ms = 4900,
		.prior_pad = 1, .prior_direction = 0,
		.sequence = UINT64_C(18364758544493064720), .monotonic_ns = UINT64_C(121456789123),
	};
}
static uid_t m_geteuid(void) { return m.root ? 0U : 1000U; }
static int m_open(const char *path, int flags, ...)
{
	assert(!strcmp(path, "/dev/qc07f-white") && flags == (O_RDWR | O_NOFOLLOW | O_CLOEXEC));
	event('O'); m.opens++;
	if (m.open_error) { errno = m.open_error; return -1; }
	m.opened = 1; return m.fd;
}
static int m_fstat(int fd, struct stat *st)
{
	assert(fd == m.fd && m.opened); event('F'); m.stats++;
	if (m.stat_error) { errno = m.stat_error; return -1; }
	memset(st, 0, sizeof(*st)); st->st_mode = (mode_t)(m.regular ? S_IFREG : S_IFCHR); return 0;
}
static int m_ioctl(int fd, unsigned long cmd, ...)
{
	va_list ap;
	assert(fd == m.fd && m.opened && !m.ranges);
	va_start(ap, cmd);
	if (cmd == QW_STATUS) {
		struct qw_status *out = va_arg(ap, struct qw_status *);
		m.snapshots++; event('S'); m.now.tv_sec += 2; /* Never move holder deadline. */
		if (!m.status_error && !m.status_ret) {
			*out = m.state;
			if (m.alter) memcpy((char *)out + m.alter_offset, &m.alter_value, sizeof(m.alter_value));
		}
	} else {
		assert(va_arg(ap, unsigned long) == 0UL);
		m.commands++; event('I'); assert(cmd == m.expected && m.commands == 1);
		if (!m.command_error && !m.command_ret && cmd == QW_ON) {
			m.held = 1; m.state.flags = QW_SETUP | QW_ACTIVE | QW_PRIOR_VALID;
			m.state.white_latch = m.state.white_pad = 1; m.state.remaining_ms = 4876;
		}
	}
	va_end(ap);
	if (cmd == QW_STATUS) {
		if (m.status_error) { errno = m.status_error; return -1; } return m.status_ret;
	}
	if (m.command_error) { errno = m.command_error; return -1; } return m.command_ret;
}
static int m_clock_gettime(clockid_t clock, struct timespec *ts)
{
	assert(clock == CLOCK_MONOTONIC && m.opened && !m.commands);
	m.clocks++; event('T');
	if (m.clock_error) { errno = m.clock_error; return -1; } *ts = m.now; return 0;
}
static long m_syscall(long nr, ...)
{
	va_list ap; unsigned int first, last, flags;
	assert(nr == SYS_close_range && m.self && m.opened && m.ranges < 2);
	assert(m.output[m.used - 1] == '\n' && strstr(m.output, "\"result\":\"OK\""));
	assert(m.commands == 1 && m.snapshots == 1 && m.held);
	va_start(ap, nr); first = va_arg(ap, unsigned int); last = va_arg(ap, unsigned int);
	flags = va_arg(ap, unsigned int); va_end(ap);
	assert(!flags);
	if (!m.ranges) assert(first == 0 && last == (unsigned int)m.fd - 1U);
	else assert(first == (unsigned int)m.fd + 1U && last == UINT_MAX);
	m.ranges++; event('R');
	if (m.range_error == m.ranges) { errno = ENOSYS; return -1; }
	for (int i = 0; i < 64; i++)
		if ((unsigned int)i >= first && (unsigned int)i <= last) m.inherited[i] = 0;
	if (first <= 10000U && last >= 10000U) m.highfd = 0;
	assert(m.inherited[m.fd]); return 0;
}
static int m_clock_nanosleep(clockid_t clock, int flags, const struct timespec *ts, struct timespec *remain)
{
	assert(clock == CLOCK_MONOTONIC && flags == TIMER_ABSTIME && !remain);
	assert(m.ranges == 2 && !m.highfd && m.opened && m.held);
	for (int i = 0; i < 64; i++) assert(m.inherited[i] == (i == m.fd));
	assert(ts->tv_sec == 125 && ts->tv_nsec == 456789123);
	m.sleeps++; event('L'); return m.sleeps <= m.sleep_interrupts ? EINTR : m.sleep_error;
}
static int m_close(int fd)
{
	assert(fd == m.fd && m.opened && !m.closes);
	m.closes++; event('C'); m.opened = m.held = m.inherited[fd] = 0;
	if (m.close_error) { errno = m.close_error; return -1; } return 0;
}
static void m_exit(int code)
{
	assert(code == 0 && m.self && !m.opened && !m.held && m.closes == 1);
	m.exited = 1; event('X'); memset(m.inherited, 0, sizeof(m.inherited)); m.highfd = 0;
	longjmp(done, 1);
}
static ssize_t m_write(int fd, const void *data, size_t length)
{
	assert(fd == STDOUT_FILENO && (!m.opened || m.self) && !m.ranges && !m.sleeps);
	assert(length < 1536); m.writes++; event('W'); m.now.tv_sec++;
	if (m.writes <= m.write_interrupts) { errno = EINTR; return -1; }
	if (m.write_error) { errno = m.write_error; return -1; }
	if (m.write_zero) return 0;
	if (m.write_chunk && length > (size_t)m.write_chunk) length = (size_t)m.write_chunk;
	assert(m.used + length < sizeof(m.output)); memcpy(m.output + m.used, data, length);
	m.used += length; m.output[m.used] = 0; return (ssize_t)length;
}
static int invoke(const char *action, const char *n, int argc)
{
	char *argv[] = { (char *)"qc07f-white-cli", (char *)action, (char *)n, NULL };
	m.self = !strcmp(action, "--white-on-hold");
	if (!setjmp(done)) return cli_main(argc, argv);
	assert(m.exited); return 0;
}
static void record(const char *name, int ret)
{
	assert(!m.opened && m.opens <= 1 && m.commands <= 1 && m.snapshots <= 1 && m.closes <= 1);
	cases++;
	if (m.used && m.output[m.used - 1] == '\n') {
		receipts++; printf("RECEIPT %s %d %s", name, ret, m.output);
	}
}
static void successes(void)
{
	int ret;
	reset(); ret = invoke("status", nonce, 3);
	assert(ret == 0 && !m.commands && !strcmp(m.events, "OFSCW")); record("status", ret);
	reset(); m.expected = QW_OFF; ret = invoke("white-off", nonce, 3);
	assert(ret == 0 && !m.held && !m.ranges && !m.clocks && !strcmp(m.events, "OFISCW"));
	assert(m.state.prior_pad == 1 && !m.state.prior_direction); record("input-high-off", ret);
	reset(); m.expected = QW_ON; ret = invoke("--white-on-hold", nonce, 3);
	assert(ret == 0 && m.exited && m.ranges == 2 && m.sleeps == 1);
	assert(!strcmp(m.events, "OFTISWRRLCX")); record("self-on", ret);
	reset(); m.expected = QW_ON; m.write_chunk = 200; m.write_interrupts = 2;
	ret = invoke("--white-on-hold", nonce, 3);
	assert(ret == 0 && m.exited && m.writes > 3); record("self-short-write", ret);
}
static void refusals(void)
{
	static const char *const bad[] = { "", "x", "0123456789ABCDEF0123456789ABCDEF",
		"0123456789abcdef0123456789abcdef0", "0123456789abcdef0123456789abcdeg", "\"; reboot; #" };
	for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		int ret; reset(); ret = invoke("white-off", bad[i], 3);
		assert(ret == 1 && !m.opens); record("bad-nonce", ret);
	}
	static const char *const actions[] = { "white-on", "--setup", "close", "pulse-a", "pin5", "ir-off" };
	for (size_t i = 0; i < sizeof(actions) / sizeof(actions[0]); i++) {
		int ret; reset(); ret = invoke(actions[i], nonce, 3);
		assert(ret == 1 && !m.opens); record("bad-action", ret);
	}
	for (int i = 0; i < 2; i++) {
		int ret; reset(); ret = invoke("status", nonce, i ? 4 : 2);
		assert(ret == 1 && !m.opens); record("bad-argc", ret);
	}
	reset(); m.root = 0; int ret = invoke("white-off", nonce, 3);
	assert(ret == 1 && !m.opens); record("nonroot", ret);
}
static void faults(void)
{
	/* Failures before/after effects never replay an ioctl or fabricate lease. */
	for (int fault = 0; fault < 18; fault++) {
		int ret; reset(); m.expected = QW_ON;
		switch (fault) {
		case 0: m.open_error = ELOOP; break;
		case 1: m.stat_error = EIO; break;
		case 2: m.regular = 1; break;
		case 3: m.fd = 1; break;
		case 4: m.clock_error = EIO; break;
		case 5: m.now.tv_sec = INT_MAX; break;
		case 6: m.now.tv_sec = -1; break;
		case 7: m.now.tv_nsec = 1000000000L; break;
		case 8: m.now.tv_nsec = -1; break;
		case 9: m.command_error = EINTR; break;
		case 10: m.command_ret = 1; break;
		case 11: m.command_error = EALREADY; break;
		case 12: m.status_error = EFAULT; break;
		case 13: m.status_ret = 1; break;
		case 14: m.state.abi = 2; break;
		case 15: m.stat_error = m.close_error = EIO; break;
		case 16: m.command_error = EBUSY; break;
		case 17: m.command_error = m.close_error = EIO; break;
		}
		ret = invoke("--white-on-hold", nonce, 3);
		assert(ret == 1 && !m.ranges && !m.sleeps && !m.held && !m.exited);
		assert(m.commands == (fault >= 9 && fault != 15));
		record("self-failure", ret);
	}
	for (int fault = 0; fault < 5; fault++) {
		int ret; reset();
		if (fault == 0) m.status_error = EINTR;
		if (fault == 1) m.status_ret = 1;
		if (fault == 2) m.state.abi = 2;
		if (fault == 3) m.close_error = EIO;
		if (fault == 4) m.open_error = ENOENT;
		ret = invoke("status", nonce, 3);
		assert(ret == 1 && !m.commands); record("status-failure", ret);
	}
}
static void verification_cases(void)
{
	static const struct { size_t offset; uint32_t value; } corrupt[] = {
		{offsetof(struct qw_status, flags), QW_SETUP | QW_PRIOR_VALID},
		{offsetof(struct qw_status, flags), 0xffffffffU},
		{offsetof(struct qw_status, fault), 1}, {offsetof(struct qw_status, owned), 0},
		{offsetof(struct qw_status, white_latch), 0}, {offsetof(struct qw_status, white_pad), 0},
		{offsetof(struct qw_status, direction), 0}, {offsetof(struct qw_status, mux_ok), 0},
		{offsetof(struct qw_status, remaining_ms), 0}, {offsetof(struct qw_status, remaining_ms), 4901},
		{offsetof(struct qw_status, limit_ms), 5000}, {offsetof(struct qw_status, prior_pad), 2},
		{offsetof(struct qw_status, prior_direction), 2},
	};
	for (size_t i = 0; i < sizeof(corrupt) / sizeof(corrupt[0]); i++) {
		int ret; reset(); m.expected = QW_ON; m.alter = 1;
		m.alter_offset = corrupt[i].offset; m.alter_value = corrupt[i].value;
		ret = invoke("--white-on-hold", nonce, 3);
		assert(ret == 1 && !m.held && !m.ranges && m.commands == 1 && m.snapshots == 1);
		record("invalid-on", ret);
	}
	for (size_t i = 0; i < sizeof(corrupt) / sizeof(corrupt[0]); i++) {
		int ret; reset(); m.expected = QW_OFF; m.alter = 1;
		m.alter_offset = corrupt[i].offset; m.alter_value = corrupt[i].value;
		/* Corrupt the OFF-specific fields to values that are invalid OFF. */
		if (i == 4 || i == 5 || i == 8) m.alter_value = 1;
		ret = invoke("white-off", nonce, 3);
		assert(ret == 1 && !m.ranges && m.commands == 1); record("invalid-off", ret);
	}
}
static void holder_and_output(void)
{
	for (int fault = 0; fault < 6; fault++) {
		int ret; reset(); m.expected = QW_ON;
		if (fault < 2) m.range_error = fault + 1;
		if (fault == 2) m.sleep_error = EINVAL;
		if (fault == 3) m.sleep_interrupts = 100;
		if (fault == 4) m.close_error = EIO;
		if (fault == 5) { m.fd = 3; m.sleep_interrupts = 4; }
		ret = invoke("--white-on-hold", nonce, 3);
		assert(ret == 0 && m.exited && !m.held && !m.highfd && m.closes == 1);
		if (fault < 2) assert(!m.sleeps);
		if (fault == 3) assert(m.sleeps == 64);
		if (fault == 5) assert(m.sleeps == 5);
		for (int i = 0; i < 64; i++) assert(!m.inherited[i]);
		record("holder-exit", ret);
	}
	for (int fault = 0; fault < 5; fault++) {
		int ret; reset(); m.expected = QW_ON;
		if (fault == 0) m.write_error = EPIPE;
		if (fault == 1) m.write_zero = 1;
		if (fault == 2) m.write_interrupts = 20;
		if (fault == 3) m.write_chunk = 1;
		if (fault == 4) m.write_error = m.close_error = EIO;
		ret = invoke("--white-on-hold", nonce, 3);
		assert(ret == 1 && !m.held && !m.exited && !m.ranges && m.closes == 1);
		assert(m.writes == ((fault == 2 || fault == 3) ? 16 : 1)); record("output-failure", ret);
	}
}
int main(void)
{
	successes(); refusals(); faults(); verification_cases(); holder_and_output();
	/* Exhaustive numeric transport, including exact uint64 integer roundtrip. */
	reset(); uint32_t words[12];
	for (size_t i = 0; i < 12; i++) words[i] = i ? (uint32_t)i * 11U : 1U;
	memcpy(&m.state, words, sizeof(words)); int ret = invoke("status", nonce, 3);
	assert(ret == 0); record("distinct-fields", ret);
	reset(); memset(&m.state, 0xff, sizeof(m.state)); ret = invoke("status", nonce, 3);
	assert(ret == 1 && m.used < 1536); record("max-fields", ret);
	printf("CLI_MOCK_PASS cases=%u receipts=%u no_real_syscalls_no_forks\n", cases, receipts);
	return 0;
}
