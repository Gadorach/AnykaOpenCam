/* SPDX-License-Identifier: GPL-2.0-only
 * Compile-only candidate. Read SOURCE_CONTRACT.md before any future selection.
 * argv/metadata are NOT authorization, global-idle or armed-recovery proof.
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/watchdog.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "encoder-open-query.h"

_Static_assert(sizeof(struct qc_hwenc_open_query) == 88, "frozen query size");
_Static_assert(offsetof(struct qc_hwenc_open_query, session_identity) == 32,
               "frozen identity offset");
_Static_assert(offsetof(struct qc_hwenc_open_query, reserved) == 84,
               "frozen reserved offset");
_Static_assert(QC_HWENC_OPEN_QUERY == 0x80585166UL, "exact frozen command");

#define ENCODER "/dev/qc07f-encoder"
#define WATCHDOG "/dev/watchdog0"
#define PHASE_NS UINT64_C(1500000000)

/* No atexit/destructor/handler owns these slots. Only close_once clears them.
 * A negative slot after a close error NEVER means the FD is retained/owned.
 */
static int encoder_fd = -1;
static int watchdog_fd = -1;
static uint64_t phase_start, last_clock;

/* No output, service, retry, close, reset or attempted repair on uncertainty.
 * SIGKILL/kernel death cannot be prevented; this is not an external retainer.
 */
static _Noreturn void retain_uncertain(void)
{
    for (;;) (void)pause();
}

static int clock_ns(uint64_t *value)
{
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time) != 0 || time.tv_sec < 0 ||
        time.tv_nsec < 0 || time.tv_nsec >= 1000000000L ||
        (uint64_t)time.tv_sec >
            (UINT64_MAX - (uint64_t)time.tv_nsec) / UINT64_C(1000000000))
        return 0;
    *value = (uint64_t)time.tv_sec * UINT64_C(1000000000) +
             (uint64_t)time.tv_nsec;
    return 1;
}

static void within_budget(void)
{
    uint64_t now;
    if (!clock_ns(&now) || now < last_clock || now < phase_start ||
        now - phase_start >= PHASE_NS)
        retain_uncertain();
    last_clock = now;
}

static int close_once(int *slot)
{
    int fd = *slot;
    *slot = -1; /* close may release even when its return is an error. */
    return close(fd);
}

/* Fixed read-only sysfs metadata, one read and one close, never a device probe.
 * A short/unexpected read is a refusal, not retried or normalized into success.
 */
static int metadata(const char *path, char *buffer, size_t size)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ssize_t bytes;
    int closed;
    if (fd < 0) return -1;
    bytes = read(fd, buffer, size);
    closed = close_once(&fd);
    if (closed != 0 || bytes < 0 || (size_t)bytes >= size) return -1;
    buffer[bytes] = '\0';
    return (int)bytes;
}

static int exact_metadata(const char *path, const char *expected)
{
    char buffer[64];
    int bytes = metadata(path, buffer, sizeof(buffer));
    return bytes >= 0 && (size_t)bytes == strlen(expected) &&
           memcmp(buffer, expected, (size_t)bytes) == 0;
}

static int timeout_metadata(void)
{
    char buffer[8];
    int bytes = metadata("/sys/class/watchdog/watchdog0/timeout", buffer,
                         sizeof(buffer));
    unsigned value;
    if (bytes == 2 && buffer[0] >= '1' && buffer[0] <= '9' && buffer[1] == '\n')
        return 1;
    if (bytes != 3 || buffer[0] < '1' || buffer[0] > '6' ||
        buffer[1] < '0' || buffer[1] > '9' || buffer[2] != '\n') return 0;
    value = (unsigned)(buffer[0] - '0') * 10U + (unsigned)(buffer[1] - '0');
    return value <= 60U;
}

static int node_metadata(const char *path, struct stat *node)
{
    return lstat(path, node) == 0 && S_ISCHR(node->st_mode) &&
           (node->st_mode & 07777) == 0600 && node->st_uid == 0;
}

static void check_open_node(int fd, const struct stat *expected)
{
    struct stat actual;
    within_budget();
    if (fstat(fd, &actual) != 0 || !S_ISCHR(actual.st_mode) ||
        actual.st_dev != expected->st_dev || actual.st_ino != expected->st_ino ||
        actual.st_rdev != expected->st_rdev || actual.st_uid != 0 ||
        (actual.st_mode & 07777) != 0600)
        retain_uncertain();
    within_budget();
}

/* Deliberately narrower than the UAPI: selects a fresh, no-job query77 boot.
 * NONE/zero history is NOT global-idle/prior-DMA/authorization/alias proof.
 */
static int valid_fresh_snapshot(const struct qc_hwenc_open_query *q)
{
    return q->version == QC_HWENC_OPEN_QUERY_VERSION && q->struct_bytes == 88 &&
           q->reserved == 0 && q->session_identity != 0 &&
           q->reference_relation == QC_HWENC_REFERENCE_NONE &&
           q->reference_owner_identity == 0 && q->reference_frame_num == 0 &&
           q->quarantined == 0 && q->dma_uncertain == 0 &&
           q->guard_poisoned == 0 && q->telemetry_flags == 0 &&
           q->reference_generation == 0 && q->last_dispatch_identity == 0 &&
           q->operation_sequence == 0 && q->last_operation_result == 0 &&
           q->last_operation_kind == 0 && q->ledger_active == 0;
}

static void query_once(struct qc_hwenc_open_query *q)
{
    within_budget();
    /* ENOTTY/partial-copy/interruption are all failures, no fallback/retry. */
    if (ioctl(encoder_fd, QC_HWENC_OPEN_QUERY, q) != 0) retain_uncertain();
    within_budget();
    if (!valid_fresh_snapshot(q)) retain_uncertain();
}

/* Numeric struct fields only. No stdio, allocation, address or raw struct dump.
 * Maximum20 decimal digits per field + delimiters, bounded below512 bytes.
 */
static size_t number(char *out, size_t offset, uint64_t value, char separator)
{
    char reverse[20];
    size_t count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0);
    while (count != 0) out[offset++] = reverse[--count];
    out[offset++] = separator;
    return offset;
}

static size_t numeric_snapshot(char *out, const struct qc_hwenc_open_query *q)
{
    size_t n = 0;
#define FIELD(name) n = number(out, n, (uint64_t)q->name, ' ')
    FIELD(version); FIELD(struct_bytes); FIELD(reference_relation);
    FIELD(reference_frame_num); FIELD(quarantined); FIELD(dma_uncertain);
    FIELD(guard_poisoned); FIELD(telemetry_flags); FIELD(session_identity);
    FIELD(reference_owner_identity); FIELD(reference_generation);
    FIELD(last_dispatch_identity); FIELD(operation_sequence);
    /* Validated fresh result is exactly0; no signed conversion ambiguity. */
    FIELD(last_operation_result); FIELD(last_operation_kind); FIELD(ledger_active);
#undef FIELD
    return number(out, n, q->reserved, '\n');
}

int main(int argc, char **argv)
{
    struct utsname release;
    struct stat encoder_node, watchdog_node;
    struct watchdog_info support = {0};
    struct qc_hwenc_open_query first = {0}, second = {0};
    sigset_t blocked;
    int timeout = 5, actual = 0, disable = WDIOS_DISABLECARD;
    char output[384]; /* 17 fields *21 bytes <=357. No full payload buffer. */
    size_t bytes;

    /* No effects or metadata reads for the default/refused invocation. */
    if (argc != 2 || strcmp(argv[1], "--one-parent-admitted-query77-trial") != 0)
        return 2;
    if (getuid() != 0 || geteuid() != 0 || uname(&release) != 0 ||
        strcmp(release.release, "6.18.55-qc07f-encoder-query77") != 0)
        return 3;
    if (!exact_metadata("/sys/class/watchdog/watchdog0/nowayout", "0\n") ||
        !exact_metadata("/sys/class/watchdog/watchdog0/state", "inactive\n") ||
        !exact_metadata("/sys/class/watchdog/watchdog0/identity",
                        "AK3918EV200 TOP watchdog\n") ||
        !timeout_metadata() || !node_metadata(ENCODER, &encoder_node) ||
        !node_metadata(WATCHDOG, &watchdog_node))
        return 4;
    if (sigfillset(&blocked) != 0 ||
        sigprocmask(SIG_SETMASK, &blocked, NULL) != 0 || !clock_ns(&phase_start))
        return 5;
    last_clock = phase_start;

    /* First hardware effect. Failure may already have started hardware without
     * returning an FD: retain process, not an invented description/armed claim.
     */
    watchdog_fd = open(WATCHDOG, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    if (watchdog_fd < 0) retain_uncertain();
    within_budget();
    check_open_node(watchdog_fd, &watchdog_node);
    if (ioctl(watchdog_fd, WDIOC_GETSUPPORT, &support) != 0)
        retain_uncertain();
    within_budget();
    if (support.options != (WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE) ||
        support.firmware_version != 0 ||
        memcmp(support.identity, "AK3918EV200 TOP watchdog",
               sizeof("AK3918EV200 TOP watchdog")) != 0)
        retain_uncertain();
    /* Initial configuration only. SETTIMEOUT internally starts/pings; never
     * repeated and never used to renew the original whole-phase budget.
     */
    if (ioctl(watchdog_fd, WDIOC_SETTIMEOUT, &timeout) != 0 || timeout != 5)
        retain_uncertain();
    within_budget();
    if (ioctl(watchdog_fd, WDIOC_GETTIMEOUT, &actual) != 0 || actual != 5)
        retain_uncertain();
    within_budget();

    encoder_fd = open(ENCODER, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    if (encoder_fd < 0) retain_uncertain();
    within_budget();
    check_open_node(encoder_fd, &encoder_node);
    query_once(&first);
    query_once(&second);
    if (memcmp(&first, &second, sizeof(first)) != 0) retain_uncertain();
    within_budget();
    if (close_once(&encoder_fd) != 0) retain_uncertain();
    within_budget();

    /* Normal no-work retirement ONLY. A late/error result may already have
     * disabled hardware: subsequent failstop does NOT promise a reset.
     */
    if (ioctl(watchdog_fd, WDIOC_SETOPTIONS, &disable) != 0) retain_uncertain();
    within_budget();
    if (close_once(&watchdog_fd) != 0) retain_uncertain();
    within_budget();

    /* Output is after all device retirement. Parent supplies a bounded volatile
     * sink. A blocked/failed write cannot trigger another device action.
     */
    bytes = numeric_snapshot(output, &first);
    if (write(STDOUT_FILENO, output, bytes) != (ssize_t)bytes) return 6;
    bytes = numeric_snapshot(output, &second);
    if (write(STDOUT_FILENO, output, bytes) != (ssize_t)bytes) return 6;
    return 0;
}
