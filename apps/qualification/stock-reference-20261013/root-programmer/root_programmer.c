/* vi: set sw=4 ts=4: */
/*
 * busybox reimplementation of flashcp
 *
 * (C) 2009 Stefan Seyfried <seife@sphairon.com>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 *
 * Project-local 2026 narrow adaptation of BusyBox 1.37.0 miscutils/flashcp.c:
 * retain block-at-a-time MEMERASE, 4KiB copy, and complete byte verification.
 * Remove libbb, arbitrary device/length, padding, progress and unlock options.
 * Add fixed ROOT admission, mandatory verified backup, read-only default,
 * explicit geometry/board contract, interruption checks and fail-closed sync.
 * This is NOT a qualified programmer or a rollback/recovery implementation.
 */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#ifdef ROOT_PROGRAMMER_MOCK
#include "mock_io.h"
#else
#if !defined(__arm__) || __ARM_ARCH != 5 || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Native build requires little-endian ARMv5; host execution requires mock IO
#endif
_Static_assert(sizeof(void *) == 4, "ARM32 ABI");
#define r_open open
#define r_openat openat
#define r_close close
#define r_pread pread
#define r_pwrite pwrite
#define r_fstat fstat
#define r_stat stat
#define r_statfs statfs
#define r_fstatfs fstatfs
#define r_ioctl ioctl
#define r_flock flock
#define r_fsync fsync
#define r_getuid getuid
#define r_geteuid geteuid
#define r_sigaction sigaction
#define r_alarm alarm
#define r_setrlimit setrlimit
#define r_log write
#endif

#define ROOT_SIZE (1024U * 1024U)
#define BUFSIZE 4096U
#define DEVICE "/dev/mtd4"
#define PROC_MAGIC 0x9fa0UL
#define SYS_MAGIC 0x62656572UL
#define TMP_MAGIC 0x01021994UL
#define RAM_MAGIC 0x858458f6UL
#define SQUASH_MAGIC 0x73717368UL
#define MTDPARTS "mtdparts=spi0.0:384K@0K(boot),64K@384K(ENV),64K@448K(MAC),1536K@512K(KERNEL),1024K@2048K(ROOT),4608K@3072K(USR.SQUASH),512K@7680K(USR.JFFS2)"

/* No heap, image mapping, whole-image buffer, or payload/hash logging. */
static unsigned char buf[BUFSIZE], buf2[BUFSIZE];
static volatile sig_atomic_t interrupted;
_Static_assert(sizeof(struct mtd_info_user) == 32 && sizeof(struct erase_info_user) == 8,
               "Linux MTD user ABI");

struct options {
    const char *image, *backup;
    unsigned erase;
    int program, board;
};
struct session {
    int image, ro, rw, backup, directory;
    struct stat input_stat, device_stat;
    unsigned erased, written, verified;
};

static void stop(int sig) { interrupted = sig; }

static int error(const char *stage)
{
    static const char prefix[] = "root-programmer: HOLD ";
    static const char suffix[] = " ROOT[0x200000,0x300000); keep backup; no automatic recovery\n";
    (void)r_log(2, prefix, sizeof(prefix) - 1);
    (void)r_log(2, stage, strlen(stage)); /* stages are fixed literals only */
    (void)r_log(2, suffix, sizeof(suffix) - 1);
    return 1;
}

static int path_ok(const char *path)
{
    size_t n;
    if (!path || path[0] != '/') return 0;
    n = strnlen(path, 512);
    if (n < 2 || n >= 512 || path[n - 1] == '/') return 0;
    for (size_t i = 0; i < n; ++i)
        if ((unsigned char)path[i] < 32 || (unsigned char)path[i] == 127) return 0;
    return 1;
}

static int parse(int argc, char **argv, struct options *o)
{
    unsigned seen = 0;
    memset(o, 0, sizeof(*o));
    for (int i = 1; i < argc; ++i) {
        const char *s = argv[i];
        unsigned bit;
        if (!strcmp(s, "--check")) { bit = 1; }
        else if (!strcmp(s, "--program-root-only")) { bit = 1; o->program = 1; }
        else if (!strcmp(s, "--board-nor-root-only")) { bit = 2; o->board = 1; }
        else if (!strcmp(s, "--image")) {
            bit = 4; if (++i == argc) return 0; o->image = argv[i];
        } else if (!strcmp(s, "--backup")) {
            bit = 8; if (++i == argc) return 0; o->backup = argv[i];
        } else if (!strcmp(s, "--expected-erasesize")) {
            bit = 16; if (++i == argc) return 0;
            s = argv[i];
            if (!*s) return 0;
            while (*s) {
                if (*s < '0' || *s > '9' || o->erase > ROOT_SIZE / 10U) return 0;
                o->erase = o->erase * 10U + (unsigned)(*s++ - '0');
                if (o->erase > ROOT_SIZE) return 0;
            }
        } else return 0;
        if (seen & bit) return 0;
        seen |= bit;
    }
    return o->board && (seen & 16) && path_ok(o->image) &&
           o->erase && !(o->erase & (o->erase - 1U)) && !(ROOT_SIZE % o->erase) &&
           (o->program ? path_ok(o->backup) : o->backup == NULL);
}

static int same_stat(const struct stat *a, const struct stat *b)
{
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino &&
           a->st_rdev == b->st_rdev && a->st_mode == b->st_mode &&
           a->st_uid == b->st_uid && a->st_gid == b->st_gid &&
           a->st_nlink == b->st_nlink && a->st_size == b->st_size &&
           a->st_mtim.tv_sec == b->st_mtim.tv_sec && a->st_mtim.tv_nsec == b->st_mtim.tv_nsec &&
           a->st_ctim.tv_sec == b->st_ctim.tv_sec && a->st_ctim.tv_nsec == b->st_ctim.tv_nsec;
}

static int input_ok(int fd, struct stat *st)
{
    return r_fstat(fd, st) == 0 && S_ISREG(st->st_mode) &&
           (st->st_mode & 07777) == 0600 && st->st_uid == 0 && st->st_nlink == 1 &&
           st->st_size == ROOT_SIZE;
}

static int input_unchanged(struct session *s)
{
    struct stat st;
    return !interrupted && input_ok(s->image, &st) && same_stat(&st, &s->input_stat);
}

static int device_ok(int fd, unsigned erase, struct stat *st)
{
    struct mtd_info_user m;
    memset(&m, 0, sizeof(m));
    return !interrupted && r_fstat(fd, st) == 0 && S_ISCHR(st->st_mode) &&
           st->st_uid == 0 && major(st->st_rdev) == 90 && minor(st->st_rdev) == 8 &&
           r_ioctl(fd, MEMGETINFO, &m) == 0 && m.type == MTD_NORFLASH &&
           (m.flags & MTD_WRITEABLE) && m.size == ROOT_SIZE && m.erasesize == erase &&
           m.writesize == 1 && m.oobsize == 0;
}

static int read_text(const char *path, unsigned long magic)
{
    struct statfs fs;
    size_t n = 0;
    int fd = r_open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK, 0);
    if (fd < 0) return 0;
    int ok = r_fstatfs(fd, &fs) == 0 && (unsigned long)fs.f_type == magic;
    while (ok && !interrupted && n < BUFSIZE - 1U) {
        ssize_t count = r_pread(fd, buf + n, BUFSIZE - 1U - n, (off_t)n);
        if (count < 0) { ok = 0; break; }
        if (!count) break;
        if ((size_t)count > BUFSIZE - 1U - n || memchr(buf + n, 0, (size_t)count)) {
            ok = 0; break;
        }
        n += (size_t)count;
    }
    if (interrupted || !n || n == BUFSIZE - 1U) ok = 0;
    buf[n] = 0;
    if (r_close(fd) != 0) ok = 0;
    return ok;
}

static int cmdline_ok(void)
{
    unsigned found = 0;
    if (!read_text("/proc/cmdline", PROC_MAGIC)) return 0;
    char *p = (char *)buf, *token;
    while ((token = strsep(&p, " \t\r\n")) != NULL)
        if (!strncmp(token, "mtdparts=", 9)) {
            if (++found != 1 || strcmp(token, MTDPARTS)) return 0;
        }
    return found == 1;
}

static int mtd_table_ok(unsigned erase)
{
    static const unsigned sizes[] = {0x60000, 0x10000, 0x10000, 0x180000, 0x100000, 0x480000, 0x80000};
    static const char *const names[] = {"boot", "ENV", "MAC", "KERNEL", "ROOT", "USR.SQUASH", "USR.JFFS2"};
    char *p, *line, name[32];
    unsigned seen = 0;
    if (!read_text("/proc/mtd", PROC_MAGIC)) return 0;
    p = (char *)buf;
    line = strsep(&p, "\n");
    if (!line || strcmp(line, "dev:    size   erasesize  name")) return 0;
    while ((line = strsep(&p, "\n")) != NULL) {
        unsigned index, size, block;
        int end = 0;
        if (!*line) { if (p != NULL) return 0; break; }
        if (sscanf(line, "mtd%u: %8x %8x \"%31[^\"]\" %n", &index, &size, &block, name, &end) != 4 ||
            end <= 0 || line[end] || index != seen || index >= 7 ||
            size != sizes[index] || strcmp(name, names[index]) || !block ||
            (block & (block - 1U)) || size % block || (index == 4 && block != erase)) return 0;
        ++seen;
    }
    return seen == 7;
}

static int unescape(char *s)
{
    char *w = s;
    for (char *p = s; *p; ++p) {
        if (*p != '\\') { *w++ = *p; continue; }
        if (!strncmp(p, "\\040", 4)) *w++ = ' ';
        else if (!strncmp(p, "\\011", 4)) *w++ = '\t';
        else if (!strncmp(p, "\\012", 4)) *w++ = '\n';
        else if (!strncmp(p, "\\134", 4)) *w++ = '\\';
        else return 0;
        p += 3;
    }
    *w = 0;
    return 1;
}

static int mounts_ok(void)
{
    struct stat root, st;
    struct statfs fs;
    char source[256], point[512], type[32], options[256];
    char *p, *line;
    unsigned roots = 0;
    if (r_stat("/", &root) != 0 || r_statfs("/", &fs) != 0 ||
        !read_text("/proc/mounts", PROC_MAGIC)) return 0;
    p = (char *)buf;
    while ((line = strsep(&p, "\n")) != NULL) {
        int dump, pass, end = 0;
        if (!*line) { if (p != NULL) return 0; break; }
        if (sscanf(line, "%255s %511s %31s %255s %d %d %n", source, point, type, options,
                   &dump, &pass, &end) != 6 || end <= 0 || line[end] ||
            !unescape(source) || !unescape(point) || point[0] != '/' ||
            r_stat(point, &st) != 0 || (major(st.st_dev) == 31 && minor(st.st_dev) == 4) ||
            !strcmp(source, "/dev/mtdblock4") || !strcmp(source, "/dev/mtd4") ||
            !strcmp(source, "mtd4") || !strcmp(source, "mtd:ROOT") || !strcmp(source, "ROOT")) return 0;
        if (!strcmp(point, "/")) {
            if (++roots != 1 || st.st_dev != root.st_dev || st.st_ino != root.st_ino) return 0;
            if (!strcmp(type, "squashfs")) {
                if (strcmp(source, "/dev/ram0") || major(root.st_dev) != 1 || minor(root.st_dev) != 0 ||
                    (unsigned long)fs.f_type != SQUASH_MAGIC) return 0;
            } else if (!strcmp(type, "tmpfs")) {
                if (major(root.st_dev) != 0 || (unsigned long)fs.f_type != TMP_MAGIC) return 0;
            } else if (!strcmp(type, "ramfs")) {
                if (major(root.st_dev) != 0 || (unsigned long)fs.f_type != RAM_MAGIC) return 0;
            } else return 0;
        }
    }
    return roots == 1;
}

static int runtime_ok(unsigned erase)
{
    return !interrupted && r_getuid() == 0 && r_geteuid() == 0 &&
           read_text("/sys/class/mtd/mtd4/name", SYS_MAGIC) && !strcmp((char *)buf, "ROOT\n") &&
           cmdline_ok() && mtd_table_ok(erase) && mounts_ok();
}

static int full_read(int fd, int require_eof)
{
    for (unsigned off = 0; off < ROOT_SIZE; off += BUFSIZE)
        if (interrupted || r_pread(fd, buf, BUFSIZE, off) != BUFSIZE) return 0;
    /* Only a regular input gets an EOF probe. Never issue an out-of-range MTD read. */
    return !interrupted && (!require_eof || r_pread(fd, buf, 1, ROOT_SIZE) == 0);
}

static int compare(int a, int b, unsigned *verified)
{
    *verified = 0;
    for (unsigned off = 0; off < ROOT_SIZE; off += BUFSIZE) {
        if (interrupted || r_pread(a, buf, BUFSIZE, off) != BUFSIZE ||
            r_pread(b, buf2, BUFSIZE, off) != BUFSIZE || memcmp(buf, buf2, BUFSIZE)) return 0;
        *verified += BUFSIZE;
    }
    return !interrupted;
}

static int backup_open(struct session *s, const char *path)
{
    struct stat st;
    struct statfs fs;
    char parent[512];
    const char *base = strrchr(path, '/') + 1;
    size_t n = (size_t)(base - path - 1);
    if (!strcmp(base, ".") || !strcmp(base, "..")) return 0;
    if (!n) n = 1;
    memcpy(parent, path, n); parent[n] = 0;
    s->directory = r_open(parent, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC, 0);
    if (s->directory < 0 || r_fstat(s->directory, &st) != 0 || !S_ISDIR(st.st_mode) ||
        st.st_uid != 0 || (st.st_mode & 0022) || r_fstatfs(s->directory, &fs) != 0 ||
        ((unsigned long)fs.f_type != TMP_MAGIC && (unsigned long)fs.f_type != RAM_MAGIC)) return 0;
    s->backup = r_openat(s->directory, base, O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0600);
    if (s->backup < 0 || r_fstat(s->backup, &st) != 0 || !S_ISREG(st.st_mode) ||
        st.st_uid != 0 || (st.st_mode & 07777) != 0600 || st.st_nlink != 1 || st.st_size != 0 ||
        r_fstatfs(s->backup, &fs) != 0 ||
        ((unsigned long)fs.f_type != TMP_MAGIC && (unsigned long)fs.f_type != RAM_MAGIC)) return 0;
    return r_flock(s->backup, LOCK_EX | LOCK_NB) == 0;
}

static int backup_copy(struct session *s)
{
    unsigned verified;
    struct stat st;
    for (unsigned off = 0; off < ROOT_SIZE; off += BUFSIZE)
        if (interrupted || r_pread(s->ro, buf, BUFSIZE, off) != BUFSIZE ||
            r_pwrite(s->backup, buf, BUFSIZE, off) != BUFSIZE) return 0;
    return !interrupted && r_fsync(s->backup) == 0 && input_ok(s->backup, &st) &&
           compare(s->ro, s->backup, &verified) && verified == ROOT_SIZE && r_fsync(s->directory) == 0;
}

static int finish(struct session *s)
{
    int ok = 1;
    int *fds[] = {&s->rw, &s->backup, &s->directory, &s->ro, &s->image};
    for (unsigned i = 0; i < sizeof(fds) / sizeof(fds[0]); ++i)
        if (*fds[i] >= 0) { if (r_close(*fds[i]) != 0) ok = 0; *fds[i] = -1; }
    return ok;
}

int root_programmer_main(int argc, char **argv)
{
    struct options o;
    struct session s = {.image = -1, .ro = -1, .rw = -1, .backup = -1, .directory = -1};
    struct stat st;
    struct sigaction sa;
    struct rlimit core = {0, 0};
    const char *failure = "arguments";
    int sync_ok, sync_errno;
    interrupted = 0;
    if (!parse(argc, argv, &o)) return error(failure);
    if (r_setrlimit(RLIMIT_CORE, &core) != 0) return error("core-dump-disable");
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = stop;
    sigemptyset(&sa.sa_mask);
    if (r_sigaction(SIGINT, &sa, NULL) || r_sigaction(SIGTERM, &sa, NULL) ||
        r_sigaction(SIGHUP, &sa, NULL) || r_sigaction(SIGALRM, &sa, NULL)) return error("signal-setup");
    (void)r_alarm(180); /* bounded userspace; cannot preempt a stuck kernel/driver */
#define REQUIRE(condition, stage) do { if (!(condition)) { failure = stage; goto fail; } } while (0)
    REQUIRE(runtime_ok(o.erase), "runtime-map-mounts");
    s.image = r_open(o.image, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0);
    REQUIRE(s.image >= 0 && input_ok(s.image, &s.input_stat) &&
            r_flock(s.image, LOCK_EX | LOCK_NB) == 0, "image-admission");
    REQUIRE(full_read(s.image, 1) && input_unchanged(&s), "image-complete-stable");
    s.ro = r_open(DEVICE, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0);
    REQUIRE(s.ro >= 0 && device_ok(s.ro, o.erase, &s.device_stat) &&
            r_flock(s.ro, LOCK_EX | LOCK_NB) == 0, "root-device-admission");
    REQUIRE(full_read(s.ro, 0), "root-complete-read");
    if (o.program) {
        REQUIRE(backup_open(&s, o.backup), "fresh-ram-backup-admission");
        REQUIRE(backup_copy(&s), "backup-copy-sync-verify");
        REQUIRE(runtime_ok(o.erase) && input_unchanged(&s) &&
                device_ok(s.ro, o.erase, &st) && same_stat(&st, &s.device_stat), "pre-writable-recheck");
        s.rw = r_open(DEVICE, O_RDWR | O_SYNC | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0);
        REQUIRE(s.rw >= 0 && device_ok(s.rw, o.erase, &st) &&
                same_stat(&st, &s.device_stat), "writable-device-identity");
        REQUIRE(runtime_ok(o.erase) && full_read(s.image, 1) && input_unchanged(&s) &&
                compare(s.rw, s.backup, &s.verified) && s.verified == ROOT_SIZE, "pre-erase-recheck");
        s.verified = 0;
        /* flashcp: whole blocks, one MEMERASE per block, no unlock or fallback. */
        for (unsigned off = 0; off < ROOT_SIZE; off += o.erase) {
            struct erase_info_user e = {.start = off, .length = o.erase};
            REQUIRE(!interrupted && r_ioctl(s.rw, MEMERASE, &e) == 0, "erase-root-block");
            s.erased++;
        }
        /* flashcp's copy / compare passes; exact 1MiB means no final padding. */
        for (unsigned off = 0; off < ROOT_SIZE; off += BUFSIZE) {
            REQUIRE(!interrupted && r_pread(s.image, buf, BUFSIZE, off) == BUFSIZE &&
                    r_pwrite(s.rw, buf, BUFSIZE, off) == BUFSIZE, "write-root-chunk");
            s.written += BUFSIZE;
        }
        sync_ok = r_fsync(s.rw) == 0; sync_errno = errno;
        /* Even an unsupported sync must not skip the complete byte readback. */
        REQUIRE(compare(s.image, s.rw, &s.verified) && s.verified == ROOT_SIZE &&
                input_unchanged(&s), "complete-readback-stable-image");
        REQUIRE(sync_ok, sync_errno == EINVAL ? "mtd-fsync-unsupported" : "mtd-fsync");
    } else REQUIRE(input_unchanged(&s), "check-image-stable");
    REQUIRE(!interrupted, "interrupted");
    REQUIRE(finish(&s), "close");
    (void)r_alarm(0);
    char receipt[320];
    int n = snprintf(receipt, sizeof(receipt),
        "{\"result\":\"%s\",\"device\":\"/dev/mtd4\",\"physical_start\":2097152,"
        "\"bytes\":1048576,\"erasesize\":%u,\"blocks_erased\":%u,\"bytes_written\":%u,"
        "\"bytes_verified\":%u,\"backup_verified\":%s,\"native_qualified\":false}\n",
        o.program ? "PROGRAM_COMPLETE" : "CHECK_ONLY", o.erase, s.erased, s.written,
        s.verified, o.program ? "true" : "false");
    if (n <= 0 || (size_t)n >= sizeof(receipt) || r_log(1, receipt, (size_t)n) != n)
        return error("receipt");
    return 0;
fail:
    (void)finish(&s);
    (void)r_alarm(0);
    return error(failure);
#undef REQUIRE
}

#ifndef ROOT_PROGRAMMER_MOCK
int main(int argc, char **argv) { return root_programmer_main(argc, argv); }
#endif
