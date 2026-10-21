// SPDX-License-Identifier: GPL-2.0-or-later
/* Local fixed transport; parent alone admits runtime/hardware. NO fork/PID lease. */
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
#include "white_uapi.h"
_Static_assert(sizeof(struct qw_status) == 64, "status size");
_Static_assert(offsetof(struct qw_status, sequence) == 48, "sequence offset");
_Static_assert(offsetof(struct qw_status, monotonic_ns) == 56, "clock offset");
_Static_assert(QW_STATUS == 0x80405170U, "STATUS bits");
_Static_assert(QW_OFF == 0x5171U && QW_ON == 0x5172U, "effect bits");
#ifdef __GLIBC__
#define REQUEST(value) ((unsigned long)(value))
#else
#define REQUEST(value) ((int)(value))
#endif
struct receipt {
	const char *action, *nonce, *lease_mode;
	int completed, command_errno, snapshot_errno, transport_errno;
	int verification_errno, close_errno, have_state;
	struct qw_status state;
};
static int saved_errno(void) { return errno > 0 ? errno : EIO; }
static int valid_nonce(const char *s)
{
	size_t i;
	if (strlen(s) != 32) return 0;
	for (i = 0; i < 32; i++)
		if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return 0;
	return 1;
}
static int snapshot(int fd, struct receipt *r)
{
	int ret = ioctl(fd, REQUEST(QW_STATUS), &r->state);
	if (ret != 0) return ret < 0 ? saved_errno() : EPROTO;
	r->have_state = 1;
	return r->state.abi == QW_ABI ? 0 : EPROTO;
}
static void hold(int fd, const struct timespec *deadline)
{
	unsigned int attempt;
	/* Main refuses fd<3: exactly TWO ranges, preserving only SAME device fd.
	 * Failure exits immediately; _exit closes inherited fds even on ENOSYS.
	 */
	if (syscall(SYS_close_range, 0U, (unsigned int)fd - 1U, 0U) < 0) goto done;
	if (syscall(SYS_close_range, (unsigned int)fd + 1U, UINT_MAX, 0U) < 0) goto done;
	for (attempt = 0; attempt < 64; attempt++)
		if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, deadline, NULL) != EINTR) break;
done:
	(void)close(fd); /* never retry close */
	_exit(0);
}
static int emit(const struct receipt *r, int ok)
{
	char state[768], line[1536];
	int n, total;
	size_t offset = 0;
	unsigned int attempt;
	if (r->have_state) {
		const struct qw_status *s = &r->state;
		n = snprintf(state, sizeof(state),
			"{\"abi\":%u,\"flags\":%u,\"fault\":%u,\"owned\":%u,"
			"\"white_latch\":%u,\"white_pad\":%u,\"direction\":%u,\"mux_ok\":%u,"
			"\"remaining_ms\":%u,\"limit_ms\":%u,\"prior_pad\":%u,\"prior_direction\":%u,"
			"\"sequence\":%" PRIu64 ",\"monotonic_ns\":%" PRIu64 "}",
			s->abi, s->flags, s->fault, s->owned, s->white_latch, s->white_pad,
			s->direction, s->mux_ok, s->remaining_ms, s->limit_ms, s->prior_pad,
			s->prior_direction, (uint64_t)s->sequence, (uint64_t)s->monotonic_ns);
		if (n < 0 || (size_t)n >= sizeof(state)) return 1;
	} else memcpy(state, "null", 5);
	total = snprintf(line, sizeof(line),
		"{\"action\":%s%s%s,\"nonce\":%s%s%s,\"result\":\"%s\","
		"\"ioctl_completed\":%s,\"command_errno\":%d,\"snapshot_errno\":%d,"
		"\"transport_errno\":%d,\"verification_errno\":%d,\"close_errno\":%d,"
		"\"lease_mode\":\"%s\",\"state\":%s}\n",
		r->action ? "\"" : "", r->action ? r->action : "null", r->action ? "\"" : "",
		r->nonce ? "\"" : "", r->nonce ? r->nonce : "null", r->nonce ? "\"" : "",
		ok ? "OK" : "FAILED", r->completed ? "true" : "false", r->command_errno,
		r->snapshot_errno, r->transport_errno, r->verification_errno, r->close_errno,
		r->lease_mode, state);
	if (total < 0 || (size_t)total >= sizeof(line)) return 1;
	for (attempt = 0; offset < (size_t)total && attempt < 16; attempt++) {
		ssize_t written = write(STDOUT_FILENO, line + offset, (size_t)total - offset);
		if (written < 0) { if (errno == EINTR) continue; return 1; }
		if (!written || (size_t)written > (size_t)total - offset) return 1;
		offset += (size_t)written;
	}
	return offset == (size_t)total && ok ? 0 : 1;
}
static int verified(const struct qw_status *s, unsigned int on)
{
	unsigned int expected = QW_SETUP | QW_PRIOR_VALID | (on ? QW_ACTIVE : QW_OFF_VERIFIED);
	return s->abi == QW_ABI && s->flags == expected && !s->fault && s->owned == 1 &&
		s->white_latch == on && s->white_pad == on && s->direction == 1 && s->mux_ok == 1 &&
		s->limit_ms == QW_LIMIT_MS && s->prior_pad <= 1 && s->prior_direction <= 1 &&
		(on ? s->remaining_ms > 0 && s->remaining_ms <= QW_LIMIT_MS : !s->remaining_ms);
}
int main(int argc, char **argv)
{
	struct receipt r = { .lease_mode = "none" };
	struct timespec deadline = {0};
	struct stat st;
	unsigned long cmd = 0;
	int fd = -1, ret, ok;
	if (argc != 3) { r.transport_errno = EINVAL; goto output; }
	if (valid_nonce(argv[2])) r.nonce = argv[2];
	if (!strcmp(argv[1], "status")) { cmd = QW_STATUS; r.action = "status"; }
	if (!strcmp(argv[1], "white-off")) { cmd = QW_OFF; r.action = "white-off"; }
	if (!strcmp(argv[1], "--white-on-hold")) {
		cmd = QW_ON; r.action = "white-on"; r.lease_mode = "self";
	}
	if (!r.nonce || !cmd) { r.transport_errno = EINVAL; goto output; }
	if (geteuid() != 0) { r.transport_errno = EPERM; goto output; }
	fd = open("/dev/qc07f-white", O_RDWR | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0) { r.transport_errno = saved_errno(); goto output; }
	if (fd < 3) { r.transport_errno = EBADF; goto finish; }
	if (fstat(fd, &st) < 0) { r.transport_errno = saved_errno(); goto finish; }
	if (!S_ISCHR(st.st_mode)) { r.transport_errno = ENODEV; goto finish; }
	if (cmd == QW_ON) {
		/* Original pre-IOCTL deadline. Snapshot/output delays never extend it. */
		if (clock_gettime(CLOCK_MONOTONIC, &deadline) < 0) {
			r.transport_errno = saved_errno(); goto finish;
		}
		if (deadline.tv_sec < 0 || deadline.tv_sec > INT_MAX - 5 ||
		    deadline.tv_nsec < 0 || deadline.tv_nsec >= 1000000000L) {
			r.transport_errno = EOVERFLOW; goto finish;
		}
		deadline.tv_sec += 5;
	}
	if (cmd == QW_STATUS) {
		r.snapshot_errno = snapshot(fd, &r);
		r.command_errno = r.have_state ? 0 : r.snapshot_errno;
		r.completed = r.have_state;
		goto finish;
	}
	ret = ioctl(fd, REQUEST(cmd), 0UL); /* EXACTLY once, including EINTR/unknown. */
	r.completed = ret == 0;
	if (ret != 0) r.command_errno = ret < 0 ? saved_errno() : EPROTO;
	r.snapshot_errno = snapshot(fd, &r); /* One diagnostic status even on command error. */
	if (r.completed && !r.snapshot_errno && !verified(&r.state, cmd == QW_ON))
		r.verification_errno = EPROTO;
	if (cmd == QW_ON && r.completed && !r.command_errno && !r.snapshot_errno &&
	    !r.transport_errno && !r.verification_errno) {
		/* Verified ON receipt FIRST. Same process/FD then isolates and holds.
		 * Failed/partial output closes early, with NO second receipt or replay.
		 */
		if (!emit(&r, 1)) hold(fd, &deadline);
		(void)close(fd);
		return 1;
	}
finish:
	if (close(fd) < 0) r.close_errno = saved_errno();
output:
	ok = r.completed && !r.command_errno && !r.snapshot_errno &&
		!r.transport_errno && !r.verification_errno && !r.close_errno;
	return emit(&r, ok);
}
