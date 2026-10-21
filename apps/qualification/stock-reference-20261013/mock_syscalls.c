/* Host-only mock: none of these functions open a device or issue an ioctl. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "mock_syscalls.h"

static const char *opened;
static unsigned int calls, clocks, closes;
static void (*handler)(int);
static int is(const char *s) { const char *v = getenv("MOCK_CASE"); return v && !strcmp(v, s); }
uid_t q_getuid(void) { return is("nonroot") ? 1000 : 0; }
uid_t q_geteuid(void) { return is("euid") ? 1000 : 0; }
int q_uname(struct utsname *u)
{
    memset(u, 0, sizeof(*u));
    strcpy(u->sysname, "Linux");
    strcpy(u->release, is("wrongkernel") ? "6.18.55" : "3.4.35");
    return is("unamefail") ? -1 : 0;
}
int q_sigaction(int n, const struct sigaction *a, struct sigaction *old)
{
    assert(!old && (n == SIGALRM || n == SIGINT || n == SIGTERM));
    assert(a->sa_flags == 0);
    handler = a->sa_handler;
    return is("sigfail") ? -1 : 0;
}
int q_sigprocmask(int how, const sigset_t *set, sigset_t *old)
{
    assert(how == SIG_UNBLOCK && !old && sigismember(set, SIGALRM) == 1);
    return is("maskfail") ? -1 : 0;
}
unsigned int q_alarm(unsigned int seconds) { assert(seconds == 10 || seconds == 0); return 0; }
int q_clock_gettime(clockid_t id, struct timespec *t)
{
    assert(id == CLOCK_MONOTONIC);
    ++clocks;
    if (is("clockstart") || (is("clockend") && clocks == 2)) return -1;
    t->tv_sec = clocks == 1 ? 100 : 101;
    t->tv_nsec = 0;
    if (is("clockinvalid")) t->tv_nsec = -1;
    if (is("backwards") && clocks == 2) t->tv_sec = 99;
    if (is("deadline") && clocks == 2) t->tv_sec = 110;
    return 0;
}
static void node(struct stat *s)
{
    memset(s, 0, sizeof(*s));
    s->st_mode = S_IFCHR | 0600; s->st_dev = 1; s->st_ino = 2; s->st_rdev = 3;
}
int q_lstat(const char *path, struct stat *s)
{
    assert(!strcmp(path, "/dev/motor0") || !strcmp(path, "/dev/motor1") ||
           !strcmp(path, "/dev/isp_char"));
    node(s);
    if (is("regular")) s->st_mode = S_IFREG | 0600;
    if (is("symlink")) s->st_mode = S_IFLNK | 0600;
    return is("lstatfail") ? -1 : 0;
}
int q_open(const char *path, int flags)
{
    assert(!opened && flags == (O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    opened = path;
    fprintf(stderr, "MOCK open %s flags=%d\n", path, flags);
    if (is("openfail")) { errno = ENODEV; return -1; }
    return 71;
}
int q_fstat(int fd, struct stat *s)
{
    assert(fd == 71 && opened); node(s);
    if (is("race")) s->st_ino = 99;
    return is("fstatfail") ? -1 : 0;
}
int q_ioctl(int fd, unsigned long req, void *arg)
{
    unsigned char *raw = arg;
    unsigned int n, i;
    assert(fd == 71 && opened && ++calls == 1 && closes == 0);
    if (!strcmp(opened, "/dev/motor0") || !strcmp(opened, "/dev/motor1")) {
        assert(req == 0x40046d43UL); n = 36;
    } else {
        assert(!strcmp(opened, "/dev/isp_char"));
        assert(req == 0x8004497bUL || req == 0x8004494fUL);
        n = req == 0x8004497bUL ? 4 : 24;
    }
    for (i = 0; i < 36; ++i) assert(raw[i] == 0);
    fprintf(stderr, "MOCK ioctl req=0x%08lx bytes=%u zero=1\n", req, n);
    fflush(stderr);
    if (is("alarm")) handler(SIGALRM);
    if (is("interrupt")) handler(SIGINT);
    if (is("terminate")) handler(SIGTERM);
    if (is("enodev")) { errno = ENODEV; return -1; }
    if (is("negative")) { errno = ENOTTY; return -1; }
    if (is("shortcopy")) { raw[0] = 0xab; errno = EFAULT; return -1; }
    for (i = 0; i < n; ++i) raw[i] = is("zero") ? 0 : (unsigned char)(i + 1);
    if (n == 36) memset(raw, 0, 4); /* caller eligibility: idle */
    if (is("busy")) raw[0] = 1;
    if (is("overrun")) raw[n] ^= 1;
    return is("positive") ? 1 : 0;
}
int q_close(int fd)
{
    assert(fd == 71 && opened && ++closes == 1);
    fprintf(stderr, "MOCK close\n");
    if (is("closefail")) { errno = EINTR; return -1; }
    return 0;
}
