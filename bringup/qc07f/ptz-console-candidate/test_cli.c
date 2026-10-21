// SPDX-License-Identifier: GPL-2.0-or-later
/* Host-only syscall model. The actual complete CLI body is included below.
 * No sysfs, device, ioctl, fork, network or target access occurs here. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

static uid_t m_geteuid(void);
static int m_open(const char *, int, ...);
static int m_openat(int, const char *, int, ...);
static int m_fstat(int, struct stat *);
static int m_fstatfs(int, struct statfs *);
static ssize_t m_readlinkat(int, const char *, char *, size_t);
static ssize_t m_read(int, void *, size_t);
static ssize_t m_write(int, const void *, size_t);
static int m_close(int);
#define geteuid m_geteuid
#define open m_open
#define openat m_openat
#define fstat m_fstat
#define fstatfs m_fstatfs
#define readlinkat m_readlinkat
#define read m_read
#define write m_write
#define close m_close
#define main ptz_main
#include "ptz_cli.c"
#undef main
#undef geteuid
#undef open
#undef openat
#undef fstat
#undef fstatfs
#undef readlinkat
#undef read
#undef write
#undef close

enum kind { DEV, OFNODE, COMPAT, AXIS, BOARD, STATE, PULSE };
struct model_fd {
	int alive, axis, post;
	enum kind kind;
	size_t cursor, len;
	char data[512];
};
static struct model_fd fds[128];
static int nextfd = 10, calls_open, pulse_calls, pulse_opens, stdout_calls;
static int state_reads[2], violations, close_calls, written_axis = -1;
static int target_axis;
static char payload[16];
static const char *scenario;
static int is(const char *s) { return !strcmp(scenario, s); }
static int fail(int e) { errno = e; return -1; }
static struct model_fd *getfd(int fd)
{
	if (fd < 10 || fd >= nextfd || !fds[fd].alive) {
		violations++; return NULL;
	}
	return &fds[fd];
}
static void state_data(struct model_fd *f)
{
	unsigned int axis = (unsigned int)f->axis, count = 0, phase = 0;
	unsigned int readback = 0, off = 0, bound = 4, period = 20, known = 0;
	int result = 0, selected = f->axis == target_axis;
	if (is("ready") || is("overflow") || is("wrap_edge") || is("uintmax") ||
	    is("bad_pre_off") || is("bad_pre_readback") || is("bad_pre_result") ||
	    is("bad_other_pre") || is("intmin") || is("phase3")) {
		count = 8; phase = is("phase3") ? 3U : 2U; readback = 1; off = 1;
	}
	if (!f->post) {
		if (selected && is("overflow")) count = UINT32_MAX - 3U;
		if (selected && is("wrap_edge")) count = UINT32_MAX - 4U;
		if (selected && is("uintmax")) count = UINT32_MAX;
		if (selected && is("bad_pre_off")) off = 0;
		if (selected && is("bad_pre_readback")) readback = 0;
		if (selected && is("bad_pre_result")) result = -5;
		if (!selected && is("bad_other_pre")) off = 0;
		if (selected && is("intmin")) result = INT32_MIN;
		if (is("untested_phase")) phase = 1;
	} else {
		if (selected) { count += 4; readback = 1; off = 1; }
		if (selected && is("wrap_edge")) count = UINT32_MAX;
		if (selected && is("after_count3")) count = 3;
		if (selected && is("after_count8")) count = 8;
		if (selected && is("after_phase")) phase = 1;
		if (selected && is("after_result")) result = -5;
		if (selected && is("after_positive_result")) result = 5;
		if (selected && is("after_off")) off = 0;
		if (selected && is("after_readback")) readback = 0;
		if (!selected && is("foreign_count")) count++;
		if (!selected && is("foreign_phase")) phase = 1;
		if (!selected && is("foreign_result")) result = -16;
		if (!selected && is("foreign_off")) off = 1;
		if (!selected && is("foreign_readback")) readback = 1;
		if (is("after_bad_period")) period = 21;
	}
	if (!f->post || is("after_malformed")) {
		if (is("bad_axis")) axis = 2;
		if (is("bad_phase")) phase = 4;
		if (is("bad_bound")) bound = 32;
		if (is("bad_period")) period = 19;
		if (is("bad_known")) known = 1;
		if (is("bad_flag")) readback = 2;
	}
	f->len = (size_t)snprintf(f->data, sizeof(f->data),
		"axis=%u issued_steps=%u phase=%u result=%d coil_readback_ok=%u coils_off_verified=%u max_pulse_steps=%u period_ms=%u physical_position_known=%u\n",
		axis, count, phase, result, readback, off, bound, period, known);
	if (is("empty_state")) f->len = 0;
	if (is("state_big")) { memset(f->data, 'x', sizeof(f->data)); f->len = sizeof(f->data); }
	if (is("missing_newline")) f->len--;
	if (is("state_nul")) f->data[5] = 0;
	if (is("state_trailing") || (f->post && is("after_malformed"))) f->data[f->len++] = 'x';
	if (is("state_overflow")) {
		const char *s = "axis=0 issued_steps=4294967296 phase=0 result=0 coil_readback_ok=0 coils_off_verified=0 max_pulse_steps=4 period_ms=20 physical_position_known=0\n";
		strcpy(f->data, s); f->len = strlen(s);
	}
	if (is("signed_overflow")) {
		const char *s = "axis=0 issued_steps=0 phase=0 result=-2147483649 coil_readback_ok=0 coils_off_verified=0 max_pulse_steps=4 period_ms=20 physical_position_known=0\n";
		strcpy(f->data, s); f->len = strlen(s);
	}
	if (is("leading_zero")) {
		memmove(f->data + 6, f->data + 5, f->len - 5); f->data[5] = '0'; f->len++;
	}
	if (is("state_reordered")) { f->data[0] = 'X'; }
}
static int newfd(enum kind kind, int axis)
{
	struct model_fd *f;
	int fd = nextfd++;
	if (fd >= 128) { violations++; return fail(EMFILE); }
	f = &fds[fd]; f->alive = 1; f->kind = kind; f->axis = axis;
	if (kind == STATE) { f->post = state_reads[axis]++ > 0; state_data(f); }
	if (kind == COMPAT) {
		memcpy(f->data, compatible, sizeof(compatible)); f->len = sizeof(compatible);
		if (is("bad_compatible")) f->data[0] = 'X';
		if (is("compatible_extra")) f->data[f->len++] = 0;
		if (is("compatible_nonul")) f->len--;
	}
	if (kind == AXIS || kind == BOARD) {
		f->len = 4; f->data[3] = (char)(kind == AXIS ? axis : 5);
		if (kind == AXIS && is("bad_axis_property")) f->data[3] = (char)(1 - axis);
		if (kind == AXIS && is("axis_little_endian")) { f->data[0] = 1; f->data[3] = 0; }
		if (kind == AXIS && is("axis_short")) f->len = 3;
		if (kind == BOARD && is("bad_board")) f->data[3] = 4;
	}
	return fd;
}
static uid_t m_geteuid(void) { return is("nonroot") ? 1000U : 0U; }
static int m_open(const char *path, int flags, ...)
{
	int axis;
	calls_open++;
	if (!strcmp(path, nodes[0])) axis = 0;
	else if (!strcmp(path, nodes[1])) axis = 1;
	else { violations++; return fail(EINVAL); }
	if (flags != (O_RDONLY | O_DIRECTORY | O_CLOEXEC)) violations++;
	if (axis == 1 && is("node_missing")) return fail(ENOENT);
	return newfd(DEV, axis);
}
static int m_openat(int dir, const char *name, int flags, ...)
{
	struct model_fd *p = getfd(dir);
	enum kind kind;
	int expected;
	if (!p) return fail(EBADF);
	calls_open++;
	if (p->kind == DEV && !strcmp(name, "of_node")) kind = OFNODE;
	else if (p->kind == DEV && !strcmp(name, "state")) kind = STATE;
	else if (p->kind == DEV && !strcmp(name, "pulse")) kind = PULSE;
	else if (p->kind == OFNODE && !strcmp(name, "compatible")) kind = COMPAT;
	else if (p->kind == OFNODE && !strcmp(name, "anyka,axis")) kind = AXIS;
	else if (p->kind == OFNODE && !strcmp(name, "anyka,retail-board-version")) kind = BOARD;
	else { violations++; return fail(EINVAL); }
	expected = (kind == OFNODE ? O_RDONLY | O_DIRECTORY :
	            (kind == PULSE ? O_WRONLY : O_RDONLY) | O_NOFOLLOW) | O_CLOEXEC;
	if (flags != expected) violations++;
	if (kind == PULSE) { pulse_opens++; if (is("pulse_open")) return fail(EACCES); }
	if (kind == STATE && is("state_open")) return fail(EACCES);
	if (kind == COMPAT && is("property_symlink")) return fail(ELOOP);
	if (kind == STATE && state_reads[p->axis] > 0 && is("after_open")) return fail(EIO);
	return newfd(kind, p->axis);
}
static int m_fstat(int fd, struct stat *st)
{
	struct model_fd *f = getfd(fd);
	if (!f) return fail(EBADF);
	if (is("stat_error")) return fail(EIO);
	memset(st, 0, sizeof(*st));
	st->st_mode = (mode_t)(f->kind <= OFNODE ? S_IFDIR | 0555 :
	                       S_IFREG | (f->kind == PULSE ? 0200 : 0444));
	if (is("bad_owner")) st->st_uid = 1000;
	if (is("bad_mode")) st->st_mode |= 0020;
	if (is("pulse_type") && f->kind == PULSE) st->st_mode = S_IFLNK | 0777;
	return 0;
}
static int m_fstatfs(int fd, struct statfs *fs)
{
	if (!getfd(fd)) return fail(EBADF);
	if (is("statfs_error")) return fail(EIO);
	memset(fs, 0, sizeof(*fs)); fs->f_type = is("bad_fs") ? 0x1234L : PTZ_SYSFS_MAGIC;
	return 0;
}
static ssize_t m_readlinkat(int fd, const char *name, char *out, size_t cap)
{
	struct model_fd *f = getfd(fd);
	const char *link = "../../../bus/platform/drivers/qc07f-ptz-diagnostic";
	size_t len;
	if (!f || f->kind != DEV || strcmp(name, "driver")) { violations++; return fail(EINVAL); }
	if (is("driver_error")) return fail(ENOENT);
	if (is("driver_wrong")) link = "../../../bus/platform/drivers/not-ptz";
	if (is("driver_prefix")) link = "../../../drivers/qc07f-ptz-diagnostic-extra";
	if (is("driver_truncated")) { memset(out, 'x', cap); return (ssize_t)cap; }
	len = strlen(link); if (len > cap) len = cap; memcpy(out, link, len); return (ssize_t)len;
}
static ssize_t m_read(int fd, void *out, size_t count)
{
	struct model_fd *f = getfd(fd);
	size_t n;
	if (!f || f->kind < COMPAT || f->kind == PULSE) { violations++; return fail(EINVAL); }
	if (f->kind == STATE && is("read_eintr")) return fail(EINTR);
	if (f->kind == STATE && f->post && is("after_read")) return fail(EIO);
	if (f->kind == STATE && f->post && f->axis == 0 && is("after_axis0_read")) return fail(EIO);
	n = f->len - f->cursor; if (n > count) n = count;
	if (is("read_chunks") && n > 13) n = 13;
	if (is("read_budget") && n > 1) n = 1;
	memcpy(out, f->data + f->cursor, n); f->cursor += n; return (ssize_t)n;
}
static ssize_t m_write(int fd, const void *buf, size_t count)
{
	struct model_fd *f;
	size_t n;
	if (fd == STDOUT_FILENO) {
		stdout_calls++;
		if (is("stdout_eintr") && stdout_calls < 4) return fail(EINTR);
		if (is("stdout_forever_eintr")) return fail(EINTR);
		if (is("stdout_error")) return fail(EPIPE);
		if (is("stdout_zero")) return 0;
		n = count; if (is("stdout_chunks") && n > 31) n = 31;
		return (ssize_t)fwrite(buf, 1, n, stdout);
	}
	f = getfd(fd);
	if (!f || f->kind != PULSE) { violations++; return fail(EINVAL); }
	pulse_calls++; written_axis = f->axis;
	if (pulse_calls != 1 || count >= sizeof(payload)) { violations++; return fail(EINVAL); }
	memcpy(payload, buf, count); payload[count] = 0;
	if (!((count == 2 && !memcmp(buf, "4\n", 2)) ||
	      (count == 3 && !memcmp(buf, "-4\n", 3)))) violations++;
	if (is("write_eintr")) return fail(EINTR);
	if (is("write_errno")) return fail(EIO);
	if (is("write_short")) return 1;
	if (is("write_zero")) return 0;
	return (ssize_t)count;
}
static int m_close(int fd)
{
	struct model_fd *f = getfd(fd);
	if (!f) return fail(EBADF);
	f->alive = 0; close_calls++;
	if (f->kind == PULSE && is("pulse_close")) return fail(EINTR);
	if (f->kind == DEV && is("device_close")) return fail(EIO);
	if (f->kind == STATE && is("state_close")) return fail(EIO);
	return 0;
}
int main(int argc, char **argv)
{
	int rc, live = 0, i;
	if (argc < 2) return 99;
	scenario = argv[1];
	target_axis = argc > 2 && !strncmp(argv[2], "motor1-", 7) ? 1 : 0;
	rc = ptz_main(argc - 1, argv + 1);
	for (i = 10; i < nextfd; i++) live += fds[i].alive;
	fprintf(stderr, "{\"pulse_calls\":%d,\"pulse_opens\":%d,\"opens\":%d,\"stdout_calls\":%d,\"state_reads\":[%d,%d],\"violations\":%d,\"live_fds\":%d,\"written_axis\":%d,\"payload_negative\":%s,\"closes\":%d}\n",
		pulse_calls, pulse_opens, calls_open, stdout_calls, state_reads[0], state_reads[1],
		violations, live, written_axis, payload[0] == '-' ? "true" : "false", close_calls);
	return rc;
}
