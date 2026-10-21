// SPDX-License-Identifier: GPL-2.0-or-later
/* Own fixed sysfs transport; exclusive operation/HOLD/recovery belongs to parent.
 * No GPIO, ioctl, fork, watchdog, retries of pulse writes, or repair operation. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#define PTZ_SYSFS_MAGIC 0x62656572L
#define PTZ_SCHEMA "qc07f.ptz.sysfs.v1"
static const char *const nodes[2] = {
	"/sys/bus/platform/devices/soc:ptz-axis0",
	"/sys/bus/platform/devices/soc:ptz-axis1"
};
static const char compatible[] = "anyka,qc07f-ptz-diagnostic";
static const char driver[] = "qc07f-ptz-diagnostic";
struct state {
	uint32_t axis, issued_steps, phase;
	int32_t result;
	uint32_t coil_readback_ok, coils_off_verified;
};
struct receipt {
	const char *action, *nonce, *result, *stage;
	int axis, steps, attempted, completed, error, diagnostic_error, hold;
	struct state before[2], after[2];
	int have_before[2], have_after[2];
};

static int syserr(void) { return errno > 0 ? errno : EIO; }
static int close_checked(int fd, int error)
{
	int ret = close(fd); /* Never retry close, including EINTR. */
	return error ? error : (ret == 0 ? 0 : syserr());
}
static int fd_check(int fd, int directory)
{
	struct stat st;
	struct statfs fs;
	if (fstat(fd, &st) < 0 || fstatfs(fd, &fs) < 0)
		return syserr();
	if (st.st_uid != 0 || (st.st_mode & 0022) ||
	    (directory ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode)) ||
	    fs.f_type != PTZ_SYSFS_MAGIC)
		return EPROTO;
	return 0;
}
/* All leaf opens use NOFOLLOW. Only fixed platform device, of_node and driver
 * links are followed/read by design; no caller-supplied paths exist. */
static int read_leaf(int dir, const char *name, char *buf, size_t cap, size_t *len)
{
	int fd = openat(dir, name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	int error;
	unsigned int calls;
	size_t used = 0;
	if (fd < 0) return syserr();
	error = fd_check(fd, 0);
	for (calls = 0; !error && calls < 64; calls++) {
		ssize_t n;
		if (used == cap) { error = EOVERFLOW; break; }
		n = read(fd, buf + used, cap - used);
		if (n < 0) { error = syserr(); break; }
		if (n == 0) break;
		if ((size_t)n > cap - used) { error = EPROTO; break; }
		used += (size_t)n;
	}
	if (!error && calls == 64) error = EOVERFLOW;
	*len = used;
	return close_checked(fd, error);
}
static int identity(int dir, unsigned int axis)
{
	char link[256], data[128];
	size_t len;
	ssize_t n;
	const char *base;
	int ofd, error = fd_check(dir, 1);
	if (error) return error;
	n = readlinkat(dir, "driver", link, sizeof(link) - 1);
	if (n < 0) return syserr();
	if (n == 0 || (size_t)n >= sizeof(link) - 1) return EPROTO;
	link[n] = 0;
	base = strrchr(link, '/');
	if (!base || strcmp(base + 1, driver)) return EPROTO;
	ofd = openat(dir, "of_node", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (ofd < 0) return syserr();
	error = fd_check(ofd, 1);
	if (!error) error = read_leaf(ofd, "compatible", data, sizeof(data), &len);
	if (!error && (len != sizeof(compatible) || memcmp(data, compatible, len)))
		error = EPROTO;
	if (!error) error = read_leaf(ofd, "anyka,axis", data, sizeof(data), &len);
	if (!error && (len != 4 || data[0] || data[1] || data[2] ||
	               (unsigned char)data[3] != axis)) error = EPROTO;
	if (!error) error = read_leaf(ofd, "anyka,retail-board-version", data, sizeof(data), &len);
	if (!error && (len != 4 || data[0] || data[1] || data[2] || data[3] != 5))
		error = EPROTO;
	return close_checked(ofd, error);
}
static int literal(const char **p, const char *end, const char *text)
{
	size_t n = strlen(text);
	if ((size_t)(end - *p) < n || memcmp(*p, text, n)) return 0;
	*p += n;
	return 1;
}
static int number(const char **p, const char *end, uint32_t *out)
{
	uint32_t v = 0;
	const char *start = *p;
	while (*p < end && **p >= '0' && **p <= '9') {
		uint32_t d = (uint32_t)(**p - '0');
		if (v > (UINT32_MAX - d) / 10U) return 0;
		v = v * 10U + d;
		(*p)++;
	}
	if (*p == start || (*p - start > 1 && *start == '0')) return 0;
	*out = v;
	return 1;
}
static int parse_state(const char *data, size_t len, unsigned int axis, struct state *s)
{
	const char *p = data, *end = data + len;
	uint32_t r, max_steps, period, known;
	int negative = 0;
#define FIELD(text, dest) (literal(&p, end, text) && number(&p, end, &(dest)))
	if (!FIELD("axis=", s->axis) || !FIELD(" issued_steps=", s->issued_steps) ||
	    !FIELD(" phase=", s->phase) || !literal(&p, end, " result=")) return EPROTO;
	if (p < end && *p == '-') { negative = 1; p++; }
	if (!number(&p, end, &r) || r > (negative ? 2147483648U : 2147483647U) ||
	    (negative && !r)) return EPROTO;
	s->result = negative ? (int32_t)(-(int64_t)r) : (int32_t)r;
	if (!FIELD(" coil_readback_ok=", s->coil_readback_ok) ||
	    !FIELD(" coils_off_verified=", s->coils_off_verified) ||
	    !FIELD(" max_pulse_steps=", max_steps) || !FIELD(" period_ms=", period) ||
	    !FIELD(" physical_position_known=", known) || !literal(&p, end, "\n") ||
	    p != end || s->axis != axis || s->phase > 3 || s->coil_readback_ok > 1 ||
	    s->coils_off_verified > 1 || max_steps != 4 || period != 20 || known != 0)
		return EPROTO;
#undef FIELD
	return 0;
}
static int snapshot(int dir, unsigned int axis, struct state *s)
{
	char buf[256];
	size_t len;
	int error = read_leaf(dir, "state", buf, sizeof(buf), &len);
	return error ? error : parse_state(buf, len, axis, s);
}
static const char *health(const struct state *s)
{
	if (!s->issued_steps && !s->phase && !s->result &&
	    !s->coil_readback_ok && !s->coils_off_verified) return "UNTESTED";
	if (!s->result && s->coil_readback_ok && s->coils_off_verified) return "READY";
	return "HOLD";
}
static int same(const struct state *a, const struct state *b)
{
	return a->axis == b->axis && a->issued_steps == b->issued_steps &&
	       a->phase == b->phase && a->result == b->result &&
	       a->coil_readback_ok == b->coil_readback_ok &&
	       a->coils_off_verified == b->coils_off_verified;
}
static int state_json(char *buf, size_t cap, const struct state *s, int have)
{
	if (!have) return snprintf(buf, cap, "null");
	return snprintf(buf, cap,
		"{\"axis\":%"PRIu32",\"issued_steps\":%"PRIu32",\"phase\":%"PRIu32
		",\"result\":%"PRId32",\"coil_readback_ok\":%"PRIu32
		",\"coils_off_verified\":%"PRIu32",\"max_pulse_steps\":4,\"period_ms\":20,"
		"\"physical_position_known\":0,\"health\":\"%s\"}",
		s->axis, s->issued_steps, s->phase, s->result,
		s->coil_readback_ok, s->coils_off_verified, health(s));
}
static int emit(const struct receipt *r)
{
	char states[4][384], line[4096];
	int n;
	size_t offset = 0;
	unsigned int i;
	for (i = 0; i < 4; i++) {
		const struct state *s = i < 2 ? &r->before[i] : &r->after[i - 2];
		int have = i < 2 ? r->have_before[i] : r->have_after[i - 2];
		n = state_json(states[i], sizeof(states[i]), s, have);
		if (n < 0 || (size_t)n >= sizeof(states[i])) return 4;
	}
	n = snprintf(line, sizeof(line),
		"{\"schema\":\""PTZ_SCHEMA"\",\"action\":\"%s\",\"nonce\":\"%s\","
		"\"result\":\"%s\",\"stage\":\"%s\",\"requested_steps\":%d,"
		"\"write_attempted\":%s,\"write_completed\":%s,\"errno\":%d,"
		"\"diagnostic_errno\":%d,\"hold\":%s,\"before\":[%s,%s],\"after\":[%s,%s]}\n",
		r->action, r->nonce, r->result, r->stage, r->steps,
		r->attempted ? "true" : "false", r->completed ? "true" : "false",
		r->error, r->diagnostic_error, r->hold ? "true" : "false",
		states[0], states[1], states[2], states[3]);
	if (n < 0 || (size_t)n >= sizeof(line)) return 4;
	for (i = 0; offset < (size_t)n && i < 64; i++) {
		ssize_t written = write(STDOUT_FILENO, line + offset, (size_t)n - offset);
		if (written < 0 && errno == EINTR) continue; /* stdout only, not pulse. */
		if (written <= 0 || (size_t)written > (size_t)n - offset) return 4;
		offset += (size_t)written;
	}
	if (offset != (size_t)n) return 4;
	return !strcmp(r->result, "OK") ? 0 : (!strcmp(r->result, "UNKNOWN") ? 3 : 2);
}
static int nonce_valid(const char *s)
{
	size_t i;
	if (strlen(s) != 32) return 0;
	for (i = 0; i < 32; i++)
		if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return 0;
	return 1;
}
int main(int argc, char **argv)
{
	struct receipt r = { .action = "invalid", .nonce = "", .result = "FAILED",
		.stage = "arguments", .axis = -1, .error = EINVAL, .hold = 1 };
	int dirs[2] = { -1, -1 }, pulse = -1;
	unsigned int i;
	if (argc != 3 || !nonce_valid(argv[2])) return emit(&r);
	if (!strcmp(argv[1], "status")) r.action = "status";
	else if (!strcmp(argv[1], "motor0-minus")) { r.action = "motor0-minus"; r.axis = 0; r.steps = -4; }
	else if (!strcmp(argv[1], "motor0-plus")) { r.action = "motor0-plus"; r.axis = 0; r.steps = 4; }
	else if (!strcmp(argv[1], "motor1-minus")) { r.action = "motor1-minus"; r.axis = 1; r.steps = -4; }
	else if (!strcmp(argv[1], "motor1-plus")) { r.action = "motor1-plus"; r.axis = 1; r.steps = 4; }
	else return emit(&r);
	r.nonce = argv[2]; r.error = 0; r.stage = "euid";
	if (geteuid() != 0) { r.error = EPERM; goto done; }
	r.stage = "identity";
	for (i = 0; i < 2; i++) {
		dirs[i] = open(nodes[i], O_RDONLY | O_DIRECTORY | O_CLOEXEC);
		if (dirs[i] < 0) { r.error = syserr(); goto done; }
		r.error = identity(dirs[i], i);
		if (r.error) goto done;
	}
	r.stage = "before";
	for (i = 0; i < 2; i++) {
		r.error = snapshot(dirs[i], i, &r.before[i]);
		if (r.error) goto done;
		r.have_before[i] = 1;
	}
	r.hold = !strcmp(health(&r.before[0]), "HOLD") || !strcmp(health(&r.before[1]), "HOLD");
	if (r.axis < 0) { r.result = "OK"; r.stage = "status"; goto done; }
	r.stage = "admission";
	if (r.hold) { r.error = EBUSY; goto done; }
	/* No wrap, even though the current kernel counter itself wraps modulo32. */
	if (r.before[r.axis].issued_steps > UINT32_MAX - 4U) {
		r.error = EOVERFLOW; r.hold = 1; goto done;
	}
	r.stage = "pulse-open";
	pulse = openat(dirs[r.axis], "pulse", O_WRONLY | O_NOFOLLOW | O_CLOEXEC);
	if (pulse < 0) { r.error = syserr(); r.hold = 1; goto done; }
	r.error = fd_check(pulse, 0);
	if (r.error) { r.hold = 1; goto done; }
	{
		const char *payload = r.steps < 0 ? "-4\n" : "4\n";
		size_t size = r.steps < 0 ? 3U : 2U;
		ssize_t n;
		r.stage = "pulse-write"; r.attempted = 1; r.hold = 1; r.result = "UNKNOWN";
		n = write(pulse, payload, size); /* EXACTLY ONCE, even EINTR/short/errno. */
		if (n == (ssize_t)size) r.completed = 1;
		else r.error = n < 0 ? syserr() : EIO;
		r.error = close_checked(pulse, r.error); pulse = -1;
	}
	/* At most one read-only diagnostic snapshot per axis after any attempted
 * write. An error never invokes another pulse, OFF, remux or repair. */
	for (i = 0; i < 2; i++) {
		int e = snapshot(dirs[i], i, &r.after[i]);
		if (e) { if (!r.diagnostic_error) r.diagnostic_error = e; }
		else r.have_after[i] = 1;
	}
	if (r.error || r.diagnostic_error) goto done;
	r.stage = "verify";
	if (!same(&r.before[1 - r.axis], &r.after[1 - r.axis]) ||
	    r.after[r.axis].issued_steps != r.before[r.axis].issued_steps + 4U ||
	    r.after[r.axis].phase != r.before[r.axis].phase ||
	    strcmp(health(&r.after[r.axis]), "READY")) { r.error = EPROTO; goto done; }
	r.result = "OK"; r.stage = "complete"; r.hold = 0;
done:
	if (pulse >= 0) r.error = close_checked(pulse, r.error);
	for (i = 0; i < 2; i++) if (dirs[i] >= 0) r.error = close_checked(dirs[i], r.error);
	if (r.error && !strcmp(r.result, "OK")) {
		r.result = r.attempted ? "UNKNOWN" : "FAILED"; r.hold = 1; r.stage = "close";
	}
	return emit(&r);
}
