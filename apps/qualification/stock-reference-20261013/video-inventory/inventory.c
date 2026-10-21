/* Local fixed stock metadata probe. Parent must admit open/close side effects. */
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
#include "stock_wire.h"

#ifdef STOCK_VIDEO_MOCK
#include "mock.h"
#define SELECTOR "--mock-stock-video-inventory"
#else
#if !defined(__arm__) || __ARM_ARCH != 5 || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Native probe requires ARMv5 little-endian; host execution requires MOCK
#endif
_Static_assert(sizeof(void *) == 4 && sizeof(long) == 4, "native stock ABI32");
#define SELECTOR "--native-stock-video-inventory"
#define v_getuid getuid
#define v_geteuid geteuid
#define v_getpid getpid
#define v_uname uname
#define v_sigaction sigaction
#define v_sigprocmask sigprocmask
#define v_alarm alarm
#define v_clock clock_gettime
#define v_lstat lstat
#define v_open open
#define v_fstat fstat
#define v_ioctl ioctl
#define v_close close
#endif

#define CANARY_A 0x35a96ec1U
#define CANARY_B 0xc25e739aU
static volatile sig_atomic_t interrupted;
static void signal_seen(int sig) { if (!interrupted) interrupted = sig; }
struct guarded_cap { uint32_t a; struct stock_cap p; uint32_t b; };
struct guarded_fmt { uint32_t a; struct stock_format p; uint32_t b; };
struct guarded_enum { uint32_t a; struct stock_enum p; uint32_t b; };
struct output { char bytes[1536]; size_t used; int failed; };
_Static_assert(sizeof(struct guarded_cap) + sizeof(struct guarded_fmt) +
               sizeof(struct guarded_enum) + sizeof(struct output) +
               3 * sizeof(struct stat) + sizeof(struct utsname) +
               sizeof(struct sigaction) + 2 * sizeof(struct timespec) < 4096,
               "fixed local application storage cap");

static uint32_t le32(const void *input)
{
    const unsigned char *p = input;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void text(struct output *o, const char *s)
{
    size_t n = strlen(s);
    if (n > sizeof(o->bytes) - o->used) { o->failed = 1; return; }
    memcpy(o->bytes + o->used, s, n); o->used += n;
}
static void number(struct output *o, uint64_t n)
{
    char s[21]; size_t i = sizeof(s) - 1; s[i] = 0;
    do { s[--i] = (char)('0' + n % 10); n /= 10; } while (n);
    text(o, s + i);
}
static void field(struct output *o, const char *name, uint32_t n)
{
    text(o, "\""); text(o, name); text(o, "\":"); number(o, n);
}
static void hexbytes(struct output *o, const unsigned char *p, size_t n)
{
    static const char h[] = "0123456789abcdef";
    size_t i; char s[3] = {0};
    for (i = 0; i < n; ++i) { s[0] = h[p[i] >> 4]; s[1] = h[p[i] & 15]; text(o, s); }
}
static int label_is(const unsigned char *p, size_t n, const char *s)
{
    size_t len = strlen(s);
    return len < n && memcmp(p, s, len) == 0 && p[len] == 0;
}
static const char *public_label(const unsigned char *p, size_t n)
{
    if (label_is(p, n, "AK_Camera")) return "AK_Camera";
    if (label_is(p, n, "ak_camera")) return "ak_camera";
    if (label_is(p, n, "soc-camera")) return "soc-camera";
    return "unrecognized_omitted";
}
static int failure(const char *stage, int err)
{
    char digits[12]; size_t i = sizeof(digits) - 1;
    unsigned int value = err > 0 ? (unsigned int)err : 0;
    digits[i] = '\n';
    do { digits[--i] = (char)('0' + value % 10); value /= 10; } while (value);
    (void)write(STDERR_FILENO, "stock-video-inventory: ", 23);
    (void)write(STDERR_FILENO, stage, strlen(stage));
    (void)write(STDERR_FILENO, " errno=", 7);
    (void)write(STDERR_FILENO, digits + i, sizeof(digits) - i);
    return 1;
}
static int same_node(const struct stat *a, const struct stat *b)
{
    return S_ISCHR(b->st_mode) && a->st_dev == b->st_dev &&
           a->st_ino == b->st_ino && a->st_rdev == b->st_rdev;
}
static int open_checked(const char *path, int flags, const struct stat *before, int *fd)
{
    struct stat after;
    *fd = v_open(path, flags);
    if (*fd < 0) return -1;
    if (interrupted) { errno = EINTR; return -1; }
    if (v_fstat(*fd, &after)) return -1;
    if (!same_node(before, &after)) { errno = ENODEV; return -1; }
    return 0;
}
static int valid_clock(const struct timespec *t)
{
    return t->tv_sec >= 0 && t->tv_nsec >= 0 && t->tv_nsec < 1000000000L;
}

int main(int argc, char **argv)
{
    struct guarded_cap cap = {CANARY_A, {{0}, {0}, {0}, 0, 0, 0, {0}}, CANARY_B};
    struct guarded_fmt fmt = {CANARY_A, {1, {0}}, CANARY_B};
    struct guarded_enum en = {CANARY_A, {0, 1, 0, {0}, 0, {0}}, CANARY_B};
    struct stat isp_node, video_node;
    struct utsname kernel;
    struct sigaction action;
    sigset_t signals;
    struct timespec start, end;
    struct output o = {{0}, 0, 0};
    const char *stage = 0;
    int isp = -1, video = -1, err = 0, rc, close_error = 0;
    uint64_t elapsed;
    unsigned int i;
    static const char *const names[8] = {
        "width", "height", "pixelformat", "field", "bytesperline",
        "sizeimage", "colorspace", "priv"
    };

    if (argc != 2 || !argv[0] || argv[0][0] != '/' || strcmp(argv[1], SELECTOR))
        return failure("explicit-absolute-selector", 0);
    if (v_getuid() || v_geteuid() || v_getpid() == 1)
        return failure("root-child-required", 0);
    if (v_uname(&kernel) || strcmp(kernel.sysname, "Linux") || strcmp(kernel.release, "3.4.35"))
        return failure("stock-kernel-release", 0);
    memset(&action, 0, sizeof(action)); action.sa_handler = signal_seen;
    if (sigemptyset(&action.sa_mask) || v_sigaction(SIGALRM, &action, 0) ||
        v_sigaction(SIGINT, &action, 0) || v_sigaction(SIGTERM, &action, 0))
        return failure("signal-setup", errno);
    if (sigemptyset(&signals) || sigaddset(&signals, SIGALRM) ||
        sigaddset(&signals, SIGINT) || sigaddset(&signals, SIGTERM) ||
        v_sigprocmask(SIG_UNBLOCK, &signals, 0)) return failure("signal-mask", errno);
    (void)v_alarm(10);
    if (v_clock(CLOCK_MONOTONIC, &start) || !valid_clock(&start)) {
        stage = "clock-start"; err = errno; goto cleanup;
    }
    /* Check both fixed nodes before the first device open. */
    if (v_lstat("/dev/isp_char", &isp_node) || !S_ISCHR(isp_node.st_mode) ||
        v_lstat("/dev/video0", &video_node) || !S_ISCHR(video_node.st_mode)) {
        stage = "fixed-character-nodes"; err = errno; goto cleanup;
    }
    if (interrupted) goto cleanup;
    if (open_checked("/dev/isp_char", O_RDONLY | O_CLOEXEC | O_NOFOLLOW, &isp_node, &isp)) {
        stage = "isp-open-identity"; err = errno; goto cleanup;
    }
    if (interrupted) goto cleanup;
    if (open_checked("/dev/video0", O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW,
                     &video_node, &video)) {
        stage = "video-open-identity"; err = errno; goto cleanup;
    }
    if (interrupted) goto cleanup;
    rc = v_ioctl(video, STOCK_QUERYCAP, &cap.p); err = errno;
    if (rc != 0) { stage = "QUERYCAP-nonzero"; goto cleanup; }
    if (cap.a != CANARY_A || cap.b != CANARY_B) { stage = "QUERYCAP-canary"; goto cleanup; }
    if (interrupted) goto cleanup;
    rc = v_ioctl(video, STOCK_G_FMT, &fmt.p); err = errno;
    if (rc != 0) { stage = "G_FMT-nonzero"; goto cleanup; }
    if (fmt.a != CANARY_A || fmt.b != CANARY_B || le32(&fmt.p.type) != 1) {
        stage = "G_FMT-packet"; goto cleanup;
    }
    if (interrupted) goto cleanup;
    rc = v_ioctl(video, STOCK_ENUM_FMT, &en.p); err = errno;
    if (rc != 0) { stage = rc < 0 && err == ENOTTY ? "ENUM_FMT-ENOTTY" : "ENUM_FMT-nonzero"; goto cleanup; }
    if (en.a != CANARY_A || en.b != CANARY_B || le32(&en.p.type) != 1 || le32(&en.p.index) != 0)
        stage = "ENUM_FMT-packet";

cleanup:
    /* Always attempt video close before ISP close, including interrupted paths.
     * No close retry; no claim that a return value acknowledges DMA retirement. */
    if (video >= 0) { if (v_close(video) != 0) close_error = errno ? errno : EIO; video = -1; }
    if (isp >= 0) { if (v_close(isp) != 0 && !close_error) close_error = errno ? errno : EIO; isp = -1; }
    if (interrupted) {
        (void)v_alarm(0);
        return interrupted == SIGALRM ? 124 : 128 + interrupted;
    }
    if (close_error) return failure("ordered-close-no-retry", close_error);
    if (stage) return failure(stage, err);
    if (v_clock(CLOCK_MONOTONIC, &end) || !valid_clock(&end) ||
        end.tv_sec < start.tv_sec || end.tv_sec - start.tv_sec > 10 ||
        (end.tv_sec == start.tv_sec && end.tv_nsec < start.tv_nsec))
        return failure("clock-end-range", errno);
    elapsed = (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL;
    if (end.tv_nsec >= start.tv_nsec) elapsed += (uint64_t)(end.tv_nsec - start.tv_nsec);
    else elapsed -= (uint64_t)(start.tv_nsec - end.tv_nsec);
    if (elapsed >= 10000000000ULL || interrupted) return failure("deadline", 0);

    text(&o, "{\"record\":\"stock-video-inventory-v1\",\"querycap\":{\"driver_label\":\"");
    text(&o, public_label(cap.p.driver, sizeof(cap.p.driver)));
    text(&o, "\",\"card_label\":\""); text(&o, public_label(cap.p.card, sizeof(cap.p.card)));
    text(&o, "\",\"bus_info_omitted\":true,"); field(&o, "version", le32(&cap.p.version));
    text(&o, ","); field(&o, "capabilities", le32(&cap.p.capabilities));
    text(&o, ","); field(&o, "device_caps_raw", le32(&cap.p.device_caps));
    text(&o, "},\"cached_format\":{\"type\":1,");
    for (i = 0; i < 8; ++i) { if (i) text(&o, ","); field(&o, names[i], le32(fmt.p.pix_and_opaque + i * 4)); }
    text(&o, ",\"pix32_hex\":\""); hexbytes(&o, fmt.p.pix_and_opaque, 32);
    text(&o, "\"},\"enum0\":{\"index\":0,\"type\":1,"); field(&o, "flags", le32(&en.p.flags));
    text(&o, ","); field(&o, "pixelformat", le32(&en.p.pixelformat));
    text(&o, "},\"expected_wire_bytes\":[104,204,64],\"elapsed_ns\":"); number(&o, elapsed);
    text(&o, ",\"close_order\":\"video_then_isp\",\"capture_started_by_helper\":false,"
             "\"physical_frame_or_ir_proof\":false,\"sizeimage_is_full_slot_proof\":false}\n");
    if (o.failed || interrupted) return failure("output-precondition", 0);
    if (write(STDOUT_FILENO, o.bytes, o.used) != (ssize_t)o.used) return failure("output", errno);
    (void)v_alarm(0);
    return interrupted ? (interrupted == SIGALRM ? 124 : 128 + interrupted) : 0;
}
