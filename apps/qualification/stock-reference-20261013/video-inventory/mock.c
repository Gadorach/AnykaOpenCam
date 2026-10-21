#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "stock_wire.h"
#include "mock.h"

static int isp_live, video_live, opens, closes, ioctls, clocks;
static void (*handler)(int);
static int is(const char *name) { const char *c = getenv("VIDEO_MOCK_CASE"); return c && !strcmp(c, name); }
static void event(const char *kind, int n) { fprintf(stderr, "MOCK %s %d\n", kind, n); fflush(stderr); }
static void put(unsigned char *p, uint32_t value)
{
    p[0] = value & 255; p[1] = (value >> 8) & 255;
    p[2] = (value >> 16) & 255; p[3] = (value >> 24) & 255;
}
uid_t v_getuid(void) { return is("uid") ? 1000 : 0; }
uid_t v_geteuid(void) { return is("euid") ? 1000 : 0; }
pid_t v_getpid(void) { return is("pid1") ? 1 : 123; }
int v_uname(struct utsname *u)
{
    memset(u, 0, sizeof(*u)); strcpy(u->sysname, "Linux");
    strcpy(u->release, is("kernel") ? "6.18.55" : "3.4.35");
    return is("uname") ? -1 : 0;
}
int v_sigaction(int sig, const struct sigaction *a, struct sigaction *old)
{
    assert(!old && !a->sa_flags && (sig == SIGALRM || sig == SIGINT || sig == SIGTERM));
    handler = a->sa_handler; return is("sigsetup") ? -1 : 0;
}
int v_sigprocmask(int how, const sigset_t *set, sigset_t *old)
{
    assert(how == SIG_UNBLOCK && !old && sigismember(set, SIGALRM) &&
           sigismember(set, SIGINT) && sigismember(set, SIGTERM));
    return is("sigmask") ? -1 : 0;
}
unsigned int v_alarm(unsigned int n) { assert(n == 10 || n == 0); return 0; }
int v_clock(clockid_t id, struct timespec *t)
{
    assert(id == CLOCK_MONOTONIC); ++clocks;
    t->tv_sec = clocks == 1 ? 100 : 101; t->tv_nsec = 0;
    if (is("clockinvalid")) t->tv_nsec = -1;
    if (is("backward") && clocks == 2) t->tv_sec = 99;
    if (is("deadline") && clocks == 2) t->tv_sec = 110;
    return is("clockstart") || (is("clockend") && clocks == 2) ? -1 : 0;
}
static void node(struct stat *s, int video)
{
    memset(s, 0, sizeof(*s)); s->st_mode = S_IFCHR | 0600;
    s->st_dev = 1; s->st_ino = video ? 2 : 3; s->st_rdev = video ? 4 : 5;
}
int v_lstat(const char *path, struct stat *s)
{
    int video = !strcmp(path, "/dev/video0");
    assert(video || !strcmp(path, "/dev/isp_char")); node(s, video);
    if ((video && is("video-link")) || (!video && is("isp-link"))) s->st_mode = S_IFLNK;
    if (is("regular")) s->st_mode = S_IFREG;
    return (video && is("video-missing")) || (!video && is("isp-missing")) ? -1 : 0;
}
int v_open(const char *path, int flags)
{
    ++opens;
    if (!strcmp(path, "/dev/isp_char")) {
        assert(opens == 1 && !isp_live && !video_live);
        assert(flags == (O_RDONLY | O_CLOEXEC | O_NOFOLLOW)); event("open", 41);
        if (is("isp-open")) { errno = ENODEV; return -1; }
        isp_live = 1;
        if (is("alarm-isp-open")) handler(SIGALRM);
        return 41;
    }
    assert(!strcmp(path, "/dev/video0") && opens == 2 && isp_live && !video_live);
    assert(flags == (O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW)); event("open", 42);
    if (is("video-open")) { errno = ENODEV; return -1; }
    video_live = 1;
    if (is("alarm-video-open")) handler(SIGALRM);
    return 42;
}
int v_fstat(int fd, struct stat *s)
{
    assert(isp_live && (fd == 41 || (fd == 42 && video_live))); node(s, fd == 42);
    if ((fd == 41 && is("isp-race")) || (fd == 42 && is("video-race"))) s->st_ino = 99;
    if (fd == 42 && is("video-notchar")) s->st_mode = S_IFREG;
    return (fd == 41 && is("isp-fstat")) || (fd == 42 && is("video-fstat")) ? -1 : 0;
}
int v_ioctl(int fd, unsigned long req, void *packet)
{
    static const unsigned long expected[] = {STOCK_QUERYCAP, STOCK_G_FMT, STOCK_ENUM_FMT};
    static const unsigned int sizes[] = {104, 204, 64};
    unsigned char *p = packet; unsigned int i, n;
    char c[32];
    assert(isp_live && video_live && fd == 42 && ioctls < 3 && !closes);
    assert(req == expected[ioctls]); n = sizes[ioctls]; ++ioctls;
    for (i = 0; i < n; ++i) {
        unsigned char expected_byte = (ioctls == 2 && i == 0) || (ioctls == 3 && i == 4) ? 1 : 0;
        assert(p[i] == expected_byte);
    }
    fprintf(stderr, "MOCK request 0x%08lx bytes=%u zeroed=1\n", req, n);
    event("ioctl", ioctls);
    snprintf(c, sizeof(c), "fail%d", ioctls);
    if (is(c)) { errno = EIO; return -1; }
    snprintf(c, sizeof(c), "short%d", ioctls);
    if (is(c)) { p[0] = 7; errno = EFAULT; return -1; }
    snprintf(c, sizeof(c), "positive%d", ioctls);
    if (is(c)) return 1;
    if (ioctls == 3 && is("enum-enotty")) { errno = ENOTTY; return -1; }
    if (ioctls == 3 && is("enum-einval")) { errno = EINVAL; return -1; }
    if (ioctls == 1) {
        memcpy(p, is("private-labels") ? "PRIVATE_DRIVER" : "ak_camera", is("private-labels") ? 15 : 10);
        memcpy(p+16, is("private-labels") ? "PRIVATE_CARD" : "AK_Camera", is("private-labels") ? 13 : 10);
        memcpy(p+48, "PRIVATE_BUS_NEVER_OUTPUT", 24);
        put(p+80, 0x00030423); put(p+84, 0x04000001);
    } else if (ioctls == 2) {
        static const uint32_t values[] = {1280,720,0x56595559,1,2560,1843200,7,0x11223344};
        for (i = 0; i < 8; ++i) put(p+4+i*4, is("zero-format") ? 0 : values[i]);
        if (is("unknown-fourcc")) put(p+12, 0x01020304);
        if (is("fmt-type")) put(p, 2);
    } else {
        put(p+8, 2); memcpy(p+12, "PRIVATE_DESCRIPTION", 20); put(p+44, 0x56595559);
        if (is("enum-index")) put(p, 1);
        if (is("enum-type")) put(p+4, 2);
    }
    snprintf(c, sizeof(c), "overrun%d", ioctls); if (is(c)) p[n] ^= 1;
    snprintf(c, sizeof(c), "underrun%d", ioctls); if (is(c)) p[-1] ^= 1;
    snprintf(c, sizeof(c), "alarm%d", ioctls); if (is(c)) handler(SIGALRM);
    if (ioctls == 2 && is("interrupt")) handler(SIGINT);
    if (ioctls == 2 && is("terminate")) handler(SIGTERM);
    return 0;
}
int v_close(int fd)
{
    ++closes;
    if (fd == 42) {
        assert(video_live && isp_live && closes == 1); video_live = 0;
        if (is("alarm-close")) handler(SIGALRM);
    } else { assert(fd == 41 && isp_live && !video_live); isp_live = 0; }
    event("close", fd);
    if ((fd == 42 && is("video-close")) || (fd == 41 && is("isp-close")) || is("both-close")) {
        errno = EINTR; return -1;
    }
    return 0;
}
