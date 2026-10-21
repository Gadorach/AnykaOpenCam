// SPDX-License-Identifier: GPL-2.0-or-later
/* Fixed local transport only. Parent owns boot/device/UART admission. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include "optical_uapi.h"

_Static_assert(sizeof(struct qo_status) == 80, "optical status ABI size");
_Static_assert(offsetof(struct qo_status, sequence) == 64, "sequence ABI offset");
_Static_assert(offsetof(struct qo_status, monotonic_ns) == 72, "clock ABI offset");
_Static_assert(QO_STATUS == 0x80505160U, "status command bits");
/* musl's request parameter is int, glibc's is unsigned long. Preserve the
 * exact low32 request bits with an explicit cast for each libc prototype.
 */
#ifdef __GLIBC__
#define QO_REQUEST(value) ((unsigned long)(value))
#else
#define QO_REQUEST(value) ((int)(value))
#endif

struct receipt {
	const char *action, *nonce, *lease_mode;
	int completed, command_errno, snapshot_errno, transport_errno;
	int keeper_errno, verification_errno, close_errno, have_state;
	pid_t keeper_pid;
	struct qo_status state;
};

static int saved_errno(void)
{
	return errno > 0 ? errno : EIO;
}

static int valid_nonce(const char *s)
{
	size_t i;
	if (strlen(s) != 32)
		return 0;
	for (i = 0; i < 32; i++)
		if (!((s[i] >= '0' && s[i] <= '9') ||
		      (s[i] >= 'a' && s[i] <= 'f')))
			return 0;
	return 1;
}

static int snapshot(int fd, struct receipt *r)
{
	int ret = ioctl(fd, QO_REQUEST(QO_STATUS), &r->state);
	if (ret != 0)
		return ret < 0 ? saved_errno() : EPROTO;
	r->have_state = 1;
	return r->state.abi == QO_ABI ? 0 : EPROTO;
}

/* Kernel75 implements close_range. No fallback FD limit/proc enumeration:
 * failure exits immediately, with the kernel closing ALL inherited FDs.
 * The keeper never signals anybody, issues an ioctl, outputs, or rearms.
 */
static void keeper(int fd, const struct timespec *deadline)
{
	unsigned int attempt;
	if (fd > 0 && syscall(SYS_close_range, 0U, (unsigned int)fd - 1U, 0U) < 0)
		goto done;
	if (syscall(SYS_close_range, (unsigned int)fd + 1U, UINT_MAX, 0U) < 0)
		goto done;
	/* Absolute monotonic sleep: EINTR never moves the original deadline.
 * A signal storm closes early instead of allowing an unbounded retry loop.
 */
	for (attempt = 0; attempt < 64; attempt++) {
		int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, deadline, NULL);
		if (ret != EINTR)
			break;
	}
done:
	(void)close(fd);
	_exit(0);
}

static int emit(const struct receipt *r, int ok)
{
	char state[1024], line[2048];
	int n, total;
	size_t offset = 0;
	unsigned int attempt;
	if (r->have_state) {
		const struct qo_status *s = &r->state;
		n = snprintf(state, sizeof(state),
			"{\"abi\":%u,\"setup\":%u,\"fault\":%u,\"owned_mask\":%u,"
			"\"pair_latch\":%u,\"pair_pad\":%u,\"ir_latch\":%u,\"ir_pad\":%u,"
			"\"direction_mask\":%u,\"mux_ok\":%u,\"release_verified\":%u,\"ir_active\":%u,"
			"\"ir_remaining_ms\":%u,\"cooldown_remaining_ms\":%u,\"pulse_us\":%u,\"ir_limit_ms\":%u,"
			"\"sequence\":%" PRIu64 ",\"monotonic_ns\":%" PRIu64 "}",
			s->abi, s->setup, s->fault, s->owned_mask,
			s->pair_latch, s->pair_pad, s->ir_latch, s->ir_pad,
			s->direction_mask, s->mux_ok, s->release_verified, s->ir_active,
			s->ir_remaining_ms, s->cooldown_remaining_ms, s->pulse_us, s->ir_limit_ms,
			(uint64_t)s->sequence, (uint64_t)s->monotonic_ns);
		if (n < 0 || (size_t)n >= sizeof(state))
			return 1;
	} else {
		memcpy(state, "null", 5);
	}
	/* Only allowlisted action and validated nonce may enter string values.
 * Bad inputs are never echoed. Everything else is public numeric UAPI data.
 */
	total = snprintf(line, sizeof(line),
		"{\"action\":%s%s%s,\"nonce\":%s%s%s,\"result\":\"%s\","
		"\"ioctl_completed\":%s,\"command_errno\":%d,\"snapshot_errno\":%d,"
		"\"transport_errno\":%d,\"keeper_errno\":%d,\"verification_errno\":%d,"
		"\"close_errno\":%d,\"keeper_pid\":%ld,\"lease_mode\":\"%s\",\"state\":%s}\n",
		r->action ? "\"" : "", r->action ? r->action : "null", r->action ? "\"" : "",
		r->nonce ? "\"" : "", r->nonce ? r->nonce : "null", r->nonce ? "\"" : "",
		ok ? "OK" : "FAILED", r->completed ? "true" : "false",
		r->command_errno, r->snapshot_errno, r->transport_errno,
		r->keeper_errno, r->verification_errno, r->close_errno, (long)r->keeper_pid,
		r->lease_mode, state);
	if (total < 0 || (size_t)total >= sizeof(line))
		return 1;
	/* Bounded bytes and syscall attempts; never retry an ioctl on write error.
 * Blocking UART transport itself is supervised by the parent's finite lease.
 */
	for (attempt = 0; offset < (size_t)total && attempt < 16; attempt++) {
		ssize_t written = write(STDOUT_FILENO, line + offset, (size_t)total - offset);
		if (written < 0) {
			if (errno == EINTR)
				continue;
			return 1;
		}
		if (written == 0 || (size_t)written > (size_t)total - offset)
			return 1;
		offset += (size_t)written;
	}
	return offset == (size_t)total && ok ? 0 : 1;
}

int main(int argc, char **argv)
{
	static const struct { const char *name; unsigned long cmd; } actions[] = {
		{ "status", QO_STATUS }, { "pulse-a", QO_PULSE_A },
		{ "pulse-b", QO_PULSE_B }, { "ir-on", QO_IR_ON },
		{ "ir-off", QO_IR_OFF }, { "--setup", QO_SETUP },
		{ "--ir-on-hold", QO_IR_ON },
	};
	struct receipt r = { .lease_mode = "none" };
	struct stat st;
	struct timespec deadline = {0};
	unsigned long cmd = 0;
	size_t i;
	int fd = -1, ret, ok, self_hold = 0;
	if (argc != 3) {
		r.transport_errno = EINVAL;
		goto output;
	}
	if (valid_nonce(argv[2]))
		r.nonce = argv[2];
	for (i = 0; i < sizeof(actions) / sizeof(actions[0]); i++)
		if (!strcmp(argv[1], actions[i].name)) {
			r.action = actions[i].cmd == QO_SETUP ? "setup" : actions[i].name;
			cmd = actions[i].cmd;
			if (cmd == QO_IR_ON) {
				self_hold = !strcmp(actions[i].name, "--ir-on-hold");
				r.action = "ir-on";
				r.lease_mode = self_hold ? "self" : "fork";
			}
			break;
		}
	if (!r.nonce || !cmd) {
		r.transport_errno = EINVAL;
		goto output;
	}
	if (geteuid() != 0) {
		r.transport_errno = EPERM;
		goto output;
	}
	fd = open("/dev/qc07f-optical", O_RDWR | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0) {
		r.transport_errno = saved_errno();
		goto output;
	}
	/* Missing standard streams must not turn JSON writes into device writes. */
	if (fd < 3) {
		r.transport_errno = EBADF;
		goto finish;
	}
	if (fstat(fd, &st) < 0) {
		r.transport_errno = saved_errno();
		goto finish;
	}
	if (!S_ISCHR(st.st_mode)) {
		r.transport_errno = ENODEV;
		goto finish;
	}
	if (cmd == QO_IR_ON) {
		/* Backup deadline begins BEFORE command entry, never from fork/snapshot. */
		if (clock_gettime(CLOCK_MONOTONIC, &deadline) < 0) {
			r.transport_errno = saved_errno();
			goto finish;
		}
		if (deadline.tv_sec < 0 || deadline.tv_sec > INT_MAX - 5 ||
		    deadline.tv_nsec < 0 || deadline.tv_nsec >= 1000000000L) {
			r.transport_errno = EOVERFLOW;
			goto finish;
		}
		deadline.tv_sec += 5;
	}
	if (cmd == QO_STATUS) {
		r.snapshot_errno = snapshot(fd, &r);
		r.command_errno = r.have_state ? 0 : r.snapshot_errno;
		r.completed = r.have_state;
		goto finish;
	}
	ret = ioctl(fd, QO_REQUEST(cmd), 0UL);
	r.completed = ret == 0;
	if (ret != 0)
		r.command_errno = ret < 0 ? saved_errno() : EPROTO;
	if (cmd == QO_IR_ON && r.completed && !self_hold) {
		r.keeper_pid = fork();
		if (r.keeper_pid == 0)
			keeper(fd, &deadline);
		if (r.keeper_pid < 0) {
			r.keeper_errno = saved_errno();
			r.keeper_pid = 0;
			r.snapshot_errno = ECANCELED;
			/* Last close releases our owner; do NOT emit a pre-close ON state. */
			goto finish;
		}
	}
	/* Even an errno/unknown result gets at most ONE diagnostic status, never
	 * a replay of the command. Historical IR_ON snapshots AFTER the fork;
	 * self-held IR_ON never forks, and snapshots before emitting/holding.
 */
	r.snapshot_errno = snapshot(fd, &r);
	if (cmd == QO_IR_ON && r.completed && !r.snapshot_errno &&
	    (r.state.setup != 1 || r.state.fault || r.state.owned_mask != 7 ||
	     r.state.ir_active != 1 || r.state.ir_remaining_ms == 0 ||
	     r.state.ir_remaining_ms > 4900 || r.state.ir_limit_ms != 4900 ||
	     r.state.ir_latch != 1 || r.state.ir_pad != 1 ||
	     r.state.pair_latch || r.state.pair_pad || r.state.direction_mask != 7 ||
	     r.state.mux_ok != 1 || r.state.release_verified != 1))
		r.verification_errno = EPROTO;
	if (self_hold && r.completed && !r.command_errno && !r.snapshot_errno &&
	    !r.transport_errno && !r.verification_errno) {
		/* A direct persistent-ash job, not an orphan. Receipt FIRST; keep only
		 * our optical FD afterwards. Failed/partial emission closes immediately
		 * without holding or emitting a second receipt. No command replay.
		 */
		if (emit(&r, 1) == 0)
			keeper(fd, &deadline);
		(void)close(fd);
		return 1;
	}
finish:
	/* Never retry close: on Linux even an error can have retired the FD. */
	if (close(fd) < 0)
		r.close_errno = saved_errno();
output:
	ok = r.completed && !r.command_errno && !r.snapshot_errno &&
	     !r.transport_errno && !r.keeper_errno && !r.verification_errno && !r.close_errno;
	return emit(&r, ok);
}
