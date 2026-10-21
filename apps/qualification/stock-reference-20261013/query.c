/* Project-local qualification code. No vendor implementation is copied. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#ifdef STOCK_QUERY_MOCK
#include "mock_syscalls.h"
#define ENTRY "--mock-stock-get"
#else
#if !defined(__arm__) || __ARM_ARCH != 5 || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Native payload requires little-endian ARMv5; host execution requires explicit mock build
#endif
_Static_assert(sizeof(void *) == 4 && sizeof(long) == 4, "stock ARM32 ABI");
#define ENTRY "--native-stock-get"
#define q_getuid getuid
#define q_geteuid geteuid
#define q_uname uname
#define q_sigaction sigaction
#define q_sigprocmask sigprocmask
#define q_alarm alarm
#define q_clock_gettime clock_gettime
#define q_lstat lstat
#define q_open open
#define q_fstat fstat
#define q_ioctl ioctl
#define q_close close
#endif

struct mode {
    const char *name, *path, *kind;
    unsigned long request;
    unsigned int bytes;
    int motor;
};
static const struct mode modes[] = {
    {"motor0-query", "/dev/motor0", "motor_software_state", 0x40046d43UL, 36, 1},
    {"motor1-query", "/dev/motor1", "motor_software_state", 0x40046d43UL, 36, 1},
    {"sensor-fps", "/dev/isp_char", "sensor_cached_fps", 0x8004497bUL, 4, 0},
    {"isp-fps", "/dev/isp_char", "isp_frame_rate_block_uninterpreted", 0x8004494fUL, 24, 0}
};
struct packet { uint32_t before; unsigned char raw[36]; uint32_t after; };
struct output { char data[1536]; size_t used; int failed; };
_Static_assert(sizeof(struct packet) + sizeof(struct output) + 2 * sizeof(struct stat)
               + 2 * sizeof(struct timespec) + sizeof(struct sigaction) < 4096,
               "fixed application working-set budget");

static void interrupted(int sig)
{
    /* exit closes descriptors in the kernel, including the driver's release.
     * It cannot preempt an uninterruptible syscall or motor release/STOP. */
    _exit(sig == SIGALRM ? 124 : 128 + sig);
}

static int error(const char *stage)
{
    static const char prefix[] = "stock-get: failure at ";
    (void)write(STDERR_FILENO, prefix, sizeof(prefix) - 1);
    (void)write(STDERR_FILENO, stage, strlen(stage));
    (void)write(STDERR_FILENO, "\n", 1);
    return 1;
}

static void text(struct output *out, const char *value)
{
    size_t n = strlen(value);
    if (n > sizeof(out->data) - out->used) { out->failed = 1; return; }
    memcpy(out->data + out->used, value, n);
    out->used += n;
}

static void hex(struct output *out, uint32_t value, unsigned int digits)
{
    static const char chars[] = "0123456789abcdef";
    char s[9];
    unsigned int i;
    for (i = 0; i < digits; ++i) {
        s[digits - i - 1] = chars[value & 15U];
        value >>= 4;
    }
    s[digits] = 0;
    text(out, s);
}

static void decimal(struct output *out, uint64_t value)
{
    char s[21];
    size_t i = sizeof(s) - 1;
    s[i] = 0;
    do { s[--i] = (char)('0' + value % 10); value /= 10; } while (value);
    text(out, s + i);
}

static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int main(int argc, char **argv)
{
    const struct mode *mode = 0;
    struct packet packet = {0};
    struct output out = {{0}, 0, 0};
    struct stat before, after;
    struct timespec start, end;
    struct sigaction action;
    struct utsname kernel;
    sigset_t unblocked;
    int fd, result, saved_errno, closed, close_errno;
    unsigned int i;
    uint64_t elapsed;

    /* No default selector/mode; reject before any device-related syscall. */
    if (argc != 3 || !argv[0] || argv[0][0] != '/' || strcmp(argv[1], ENTRY))
        return error("explicit-absolute-entry");
    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
        if (!strcmp(argv[2], modes[i].name)) mode = &modes[i];
    if (!mode) return error("fixed-mode");
    if (q_getuid() != 0 || q_geteuid() != 0) return error("real-and-effective-root");
    if (q_uname(&kernel) || strcmp(kernel.sysname, "Linux") ||
        strcmp(kernel.release, "3.4.35")) return error("stock-kernel-release");

    memset(&action, 0, sizeof(action));
    action.sa_handler = interrupted;
    if (sigemptyset(&action.sa_mask) ||
        q_sigaction(SIGALRM, &action, 0) || q_sigaction(SIGINT, &action, 0) ||
        q_sigaction(SIGTERM, &action, 0)) return error("signal-setup");
    if (sigemptyset(&unblocked) || sigaddset(&unblocked, SIGALRM) ||
        sigaddset(&unblocked, SIGINT) || sigaddset(&unblocked, SIGTERM) ||
        q_sigprocmask(SIG_UNBLOCK, &unblocked, 0)) return error("signal-mask");
    (void)q_alarm(10);
    if (q_clock_gettime(CLOCK_MONOTONIC, &start)) return error("clock-start");
    if (start.tv_sec < 0 || start.tv_nsec < 0 || start.tv_nsec >= 1000000000L)
        return error("clock-start-range");
    if (q_lstat(mode->path, &before) || !S_ISCHR(before.st_mode))
        return error("fixed-character-node");
    fd = q_open(mode->path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return error("open");
    if (q_fstat(fd, &after) || !S_ISCHR(after.st_mode) ||
        before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
        before.st_rdev != after.st_rdev) {
        (void)q_close(fd); /* no retry; release can have driver side effects */
        return error("opened-node-identity");
    }
    packet.before = 0x519ac3e7U;
    packet.after = 0xa732e19cU;
    result = q_ioctl(fd, mode->request, packet.raw); /* exactly one fixed GET */
    saved_errno = errno;
    closed = q_close(fd); /* includes motor STOP / possible last ISP release */
    close_errno = errno;
    if (result != 0) {
        if (result < 0 && saved_errno == ENODEV) return error("ioctl-ENODEV");
        if (result < 0 && saved_errno == EFAULT) return error("ioctl-EFAULT-short-copy");
        return error("ioctl-nonzero");
    }
    if (closed != 0) {
        (void)close_errno;
        return error("close-no-retry");
    }
    if (packet.before != 0x519ac3e7U || packet.after != 0xa732e19cU)
        return error("packet-canary");
    for (i = mode->bytes; i < sizeof(packet.raw); ++i)
        if (packet.raw[i] != 0) return error("packet-tail");
    if (mode->motor && le32(packet.raw) != 0) return error("motor-not-idle");
    if (q_clock_gettime(CLOCK_MONOTONIC, &end)) return error("clock-end");
    if (start.tv_nsec < 0 || start.tv_nsec >= 1000000000L ||
        end.tv_nsec < 0 || end.tv_nsec >= 1000000000L || start.tv_sec < 0 ||
        end.tv_sec < start.tv_sec || end.tv_sec - start.tv_sec > 10 ||
        (end.tv_sec == start.tv_sec && end.tv_nsec < start.tv_nsec))
        return error("clock-range-deadline");
    elapsed = (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL;
    if (end.tv_nsec >= start.tv_nsec) elapsed += (uint64_t)(end.tv_nsec - start.tv_nsec);
    else elapsed -= (uint64_t)(start.tv_nsec - end.tv_nsec);
    if (elapsed >= 10000000000ULL) return error("deadline");

    text(&out, "{\"schema\":\"stock-get-v1\",\"mode\":\""); text(&out, mode->name);
    text(&out, "\",\"path\":\""); text(&out, mode->path);
    text(&out, "\",\"kind\":\""); text(&out, mode->kind);
    text(&out, "\",\"request\":\"0x"); hex(&out, (uint32_t)mode->request, 8);
    text(&out, "\",\"bytes\":"); decimal(&out, mode->bytes);
    text(&out, " ,\"raw_hex\":\"");
    for (i = 0; i < mode->bytes; ++i) hex(&out, packet.raw[i], 2);
    text(&out, "\",\"words_le_hex\":[");
    for (i = 0; i < mode->bytes; i += 4) {
        if (i) text(&out, ",");
        text(&out, "\"0x"); hex(&out, le32(packet.raw + i), 8); text(&out, "\"");
    }
    text(&out, "],\"elapsed_ns\":"); decimal(&out, elapsed);
    text(&out, ",\"physical_fps_measured\":false,\"close_ok\":true}\n");
    if (out.failed || write(STDOUT_FILENO, out.data, out.used) != (ssize_t)out.used)
        return error("output");
    (void)q_alarm(0);
    return 0;
}
