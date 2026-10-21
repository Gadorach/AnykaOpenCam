/* GPL-2.0-only. In-memory host fixtures; NO real device/proc/sys/file IO. */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include "mock_io.h"

#define N 1048576U
#define CHUNK 4096U
enum role {NONE, NAME, CMD, MTD, MOUNTS, IMAGE, RO, RW, BACKUP, DIRECTORY};
struct fd { enum role role; int live; };
static struct fd fds[128];
static unsigned char image[N], flash[N], backup[N], original[N];
static char mtd[1024], mounts[2048], output[1024], errors[1024];
static unsigned nextfd = 10, opened, closed, bad, ro_open, rw_open, creates, openat_calls;
static unsigned erases, erased, writes, written, backup_written, backup_size, backup_reads;
static unsigned readback, max_io, max_flash_end, gets, image_stats, mount_scans, backup_sync, dir_sync, mtd_sync;
static unsigned geometry = 65536, output_n, errors_n;
static const char *scenario;
static void (*handlers[64])(int);

static int is(const char *s) { return !strcmp(scenario, s); }
static int fail(int e) { errno = e; return -1; }
static enum role role(int fd)
{
    if (fd < 10 || (unsigned)fd >= nextfd || !fds[fd].live) { ++bad; return NONE; }
    return fds[fd].role;
}
static int alloc(enum role r)
{
    if (nextfd >= 128) { ++bad; return fail(EMFILE); }
    fds[nextfd] = (struct fd){r, 1}; ++opened;
    return (int)nextfd++;
}
static void bounds(size_t n, off_t off)
{
    if (n > max_io) max_io = (unsigned)n;
    if (n > CHUNK || off < 0 || (uint64_t)off + n > N + 1U) ++bad;
}

int r_open(const char *path, int flags, mode_t mode)
{
    (void)mode;
    int required = O_CLOEXEC;
    if (!(flags & required)) ++bad;
    if (!strcmp(path, "/sys/class/mtd/mtd4/name")) return is("name-open") ? fail(EIO) : alloc(NAME);
    if (!strcmp(path, "/proc/cmdline")) return alloc(CMD);
    if (!strcmp(path, "/proc/mtd")) return alloc(MTD);
    if (!strcmp(path, "/proc/mounts")) { ++mount_scans; return alloc(MOUNTS); }
    if (!strcmp(path, "/mock/input")) {
        if ((flags & O_ACCMODE) != O_RDONLY || !(flags & O_NOFOLLOW) || !(flags & O_NONBLOCK)) ++bad;
        return is("image-symlink") ? fail(ELOOP) : alloc(IMAGE);
    }
    if (!strcmp(path, "/mock")) {
        if (!(flags & O_DIRECTORY) || !(flags & O_NOFOLLOW) || (flags & O_ACCMODE) != O_RDONLY) ++bad;
        return is("backup-parent-open") ? fail(EIO) : alloc(DIRECTORY);
    }
    if (strcmp(path, "/dev/mtd4")) { ++bad; return fail(EPERM); }
    if (!(flags & O_NOFOLLOW) || !(flags & O_NONBLOCK)) ++bad;
    if ((flags & O_ACCMODE) == O_RDONLY) { ++ro_open; return is("device-open") ? fail(EIO) : alloc(RO); }
    if ((flags & O_ACCMODE) != O_RDWR || !(flags & O_SYNC)) ++bad;
    ++rw_open;
    if (backup_size != N || backup_reads < N || !backup_sync || !dir_sync) ++bad;
    if (is("backup-changed")) backup[0] ^= 1;
    if (is("flash-changed")) flash[0] ^= 1;
    return is("writable-open") ? fail(EIO) : alloc(RW);
}

int r_openat(int directory, const char *name, int flags, mode_t mode)
{
    ++openat_calls;
    if (role(directory) != DIRECTORY || strcmp(name, "backup") || mode != 0600 ||
        flags != (O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK)) ++bad;
    if (is("backup-exists")) return fail(EEXIST);
    if (is("backup-open")) return fail(EIO);
    ++creates; return alloc(BACKUP);
}

int r_close(int fd)
{
    enum role r = role(fd);
    if (r == NONE) return fail(EBADF);
    fds[fd].live = 0; ++closed;
    return is("close-fail") && r == RW ? fail(EIO) : 0;
}

static const char *text(enum role r)
{
    if (r == NAME) return is("name-wrong") ? "KERNEL\n" : "ROOT\n";
    if (r == CMD) {
        if (is("cmdline-wrong")) return "mtdparts=spi0.0:8M(all)\n";
        if (is("cmdline-duplicate")) return "mtdparts=x mtdparts=x\n";
        if (is("cmdline-missing")) return "console=ttySAK0 root=/dev/ram0\n";
        return "console=ttySAK0 root=/dev/ram0 mtdparts=spi0.0:384K@0K(boot),64K@384K(ENV),64K@448K(MAC),1536K@512K(KERNEL),1024K@2048K(ROOT),4608K@3072K(USR.SQUASH),512K@7680K(USR.JFFS2) ro\n";
    }
    if (r == MTD) {
        snprintf(mtd, sizeof(mtd), "dev:    size   erasesize  name\n"
            "mtd0: 00060000 00010000 \"boot\"\nmtd1: 00010000 00010000 \"ENV\"\n"
            "mtd2: 00010000 00010000 \"MAC\"\nmtd3: 00180000 00010000 \"KERNEL\"\n"
            "mtd4: %08x %08x \"%s\"\nmtd5: 00480000 00010000 \"USR.SQUASH\"\n"
            "mtd6: 00080000 00010000 \"USR.JFFS2\"\n%s", is("proc-size") ? N - 1 : N,
            is("proc-erase") ? geometry / 2 : geometry, is("proc-name") ? "BAD" : "ROOT",
            is("proc-extra") ? "mtd7: 00800000 00010000 \"all\"\n" : "");
        return mtd;
    }
    if (r == MOUNTS) {
        const char *source = "/dev/ram0", *type = "squashfs", *extra = "";
        if (is("tmpfs")) { source = "tmpfs"; type = "tmpfs"; }
        if (is("ramfs")) { source = "ramfs"; type = "ramfs"; }
        if (is("root-unknown")) { source = "overlay"; type = "overlay"; }
        if (is("root-alias")) source = "/dev/root";
        if (is("root-mounted") || (is("mount-late") && mount_scans >= 2) ||
            (is("mount-preerase") && mount_scans >= 3)) extra = "/dev/mtdblock4 /old squashfs ro 0 0\n";
        if (is("bind-mounted")) extra = "/dev/other /bind squashfs ro 0 0\n";
        if (is("bind-escaped")) extra = "/dev/other /bind\\040path squashfs ro 0 0\n";
        if (is("mount-stat")) extra = "tmpfs /missing tmpfs rw 0 0\n";
        if (is("mount-escape")) extra = "tmpfs /bad\\999 tmpfs rw 0 0\n";
        if (is("mount-stacked")) extra = "/dev/ram0 / squashfs ro 0 0\n";
        snprintf(mounts, sizeof(mounts), "%s / %s ro 0 0\nproc /proc proc rw 0 0\n"
                 "sysfs /sys sysfs ro 0 0\n%s", source, type, extra);
        return mounts;
    }
    return NULL;
}

ssize_t r_pread(int fd, void *dest, size_t n, off_t off)
{
    enum role r = role(fd);
    bounds(n, off);
    if ((r == RO || r == RW) && (off < 0 || (uint64_t)off + n > N)) ++bad;
    const char *t = text(r);
    if (t) {
        if (is("metadata-read")) return fail(EIO);
        if (is("metadata-bound")) { memset(dest, 'x', n); return (ssize_t)n; }
        size_t size = strlen(t);
        if ((uint64_t)off >= size) return 0;
        if (n > size - (size_t)off) n = size - (size_t)off;
        memcpy(dest, t + off, n);
        if (is("metadata-nul") && !off && n) ((char *)dest)[0] = 0;
        return (ssize_t)n;
    }
    if (r != IMAGE && r != RO && r != RW && r != BACKUP) { ++bad; return fail(EBADF); }
    if ((uint64_t)off >= N) return is("image-grow") && r == IMAGE ? 1 : 0;
    if (n > N - (unsigned)off) { ++bad; return fail(EIO); }
    if (is("image-truncate") && r == IMAGE && off >= CHUNK) return 0;
    if (is("root-short") && r == RO && off >= CHUNK) return 0;
    if (r == RW && written) {
        if (is("readback-io")) return fail(EIO);
        readback += (unsigned)n;
    }
    if (r == BACKUP) backup_reads += (unsigned)n;
    const unsigned char *data = r == IMAGE ? image : r == BACKUP ? backup : flash;
    memcpy(dest, data + off, n);
    if ((is("backup-verify") && r == BACKUP) ||
        (is("readback-mismatch") && r == RW && written)) ((unsigned char *)dest)[0] ^= 1;
    return (ssize_t)n;
}

ssize_t r_pwrite(int fd, const void *data, size_t n, off_t off)
{
    enum role r = role(fd);
    bounds(n, off);
    if (n != CHUNK || off < 0 || (uint64_t)off + n > N || (unsigned)off % CHUNK) ++bad;
    if (r == BACKUP) {
        if (rw_open || erased) ++bad;
        if (is("backup-write")) return fail(ENOSPC);
        if (is("backup-short")) n /= 2;
        memcpy(backup + off, data, n); backup_written += (unsigned)n;
        backup_size = (unsigned)off + (unsigned)n;
        if (is("interrupt-backup") && handlers[SIGALRM]) handlers[SIGALRM](SIGALRM);
    } else if (r == RW) {
        ++writes;
        if (erased != N / geometry || backup_reads < 2U * N) ++bad;
        if (is("write-fail") && writes == 3) return fail(EIO);
        if (is("write-short") && writes == 3) n /= 2;
        memcpy(flash + off, data, n); written += (unsigned)n;
        if ((unsigned)off + n > max_flash_end) max_flash_end = (unsigned)off + (unsigned)n;
    } else { ++bad; return fail(EBADF); }
    return (ssize_t)n;
}

static void metadata(struct stat *st)
{
    memset(st, 0, sizeof(*st));
    st->st_uid = 0; st->st_gid = 0; st->st_nlink = 1;
    st->st_mtim.tv_sec = st->st_ctim.tv_sec = 1;
}
int r_fstat(int fd, struct stat *st)
{
    enum role r = role(fd);
    metadata(st); st->st_ino = 42; st->st_dev = makedev(0, 5);
    if (r == IMAGE || r == BACKUP) {
        st->st_mode = S_IFREG | 0600; st->st_size = r == IMAGE ? N : backup_size;
        if (r == IMAGE) {
            ++image_stats;
            if (is("image-stat")) return fail(EIO);
            if (is("image-small")) --st->st_size;
            if (is("image-large")) ++st->st_size;
            if (is("image-owner")) st->st_uid = 1000;
            if (is("image-mode")) st->st_mode |= 0040;
            if (is("image-fifo")) st->st_mode = S_IFIFO | 0600;
            if (is("image-hardlink")) st->st_nlink = 2;
            if ((is("image-change") && image_stats >= 2) ||
                (is("image-change-preerase") && image_stats >= 4) ||
                (is("image-change-written") && written)) st->st_mtim.tv_nsec = 1;
        } else if (is("backup-mode")) st->st_mode |= 0040;
        return 0;
    }
    if (r == RO || r == RW) {
        if (is("device-stat")) return fail(EIO);
        st->st_mode = S_IFCHR | 0600; st->st_rdev = makedev(90, 8);
        if (is("device-regular")) st->st_mode = S_IFREG | 0600;
        if (is("device-owner")) st->st_uid = 1000;
        if (is("device-major")) st->st_rdev = makedev(91, 8);
        if (is("device-minor") || (is("writable-minor") && r == RW)) st->st_rdev = makedev(90, 10);
        if (is("writable-inode") && r == RW) st->st_ino++;
        return 0;
    }
    if (r == DIRECTORY) {
        st->st_mode = S_IFDIR | (is("backup-parent-mode") ? 0777 : 0700);
        if (is("backup-parent-owner")) st->st_uid = 1000;
        return 0;
    }
    ++bad; return fail(EBADF);
}
int r_stat(const char *path, struct stat *st)
{
    metadata(st); st->st_ino = 7; st->st_dev = makedev(0, 5);
    st->st_mode = S_IFDIR | 0700;
    if (!strcmp(path, "/")) {
        if (is("root-stat")) return fail(EIO);
        st->st_dev = is("tmpfs") || is("ramfs") ? makedev(0, 5) : makedev(1, 0);
        if (is("root-mtd")) st->st_dev = makedev(31, 4);
        if (is("root-other-ram")) st->st_dev = makedev(1, 1);
    }
    if (!strcmp(path, "/bind") || !strcmp(path, "/bind path")) st->st_dev = makedev(31, 4);
    if (!strcmp(path, "/missing")) return fail(ENOENT);
    return 0;
}
int r_statfs(const char *path, struct statfs *fs)
{
    if (strcmp(path, "/")) ++bad;
    if (is("root-statfs")) return fail(EIO);
    memset(fs, 0, sizeof(*fs));
    fs->f_type = is("tmpfs") ? 0x01021994 : is("ramfs") ? 0x858458f6 : 0x73717368;
    if (is("root-wrong-fs")) fs->f_type = 0xef53;
    return 0;
}
int r_fstatfs(int fd, struct statfs *fs)
{
    enum role r = role(fd);
    memset(fs, 0, sizeof(*fs));
    fs->f_type = r == NAME ? 0x62656572 : r == BACKUP || r == DIRECTORY ? 0x01021994 : 0x9fa0;
    if (is("metadata-fs") && r == CMD) fs->f_type = 0xef53;
    if (is("backup-parent-fs") && r == DIRECTORY) fs->f_type = 0xef53;
    if (is("backup-fs") && r == BACKUP) fs->f_type = 0xef53;
    return 0;
}
int r_ioctl(int fd, unsigned long request, void *arg)
{
    enum role r = role(fd);
    if (request == MEMGETINFO) {
        ++gets;
        if (r != RO && r != RW) ++bad;
        if (is("info-fail")) return fail(EIO);
        struct mtd_info_user *m = arg;
        memset(m, 0, sizeof(*m));
        m->type = is("nand") ? MTD_NANDFLASH : is("unknown-type") ? MTD_ABSENT : MTD_NORFLASH;
        m->size = is("info-size") ? N * 2 : N;
        m->erasesize = is("info-erase") || (is("writable-geometry") && r == RW) ? geometry / 2 : geometry;
        m->writesize = is("info-writesize") ? 2048 : 1;
        m->oobsize = is("info-oob") ? 64 : 0;
        m->flags = is("info-readonly") ? 0 : MTD_WRITEABLE;
        return 0;
    }
    if (request == MEMERASE) {
        struct erase_info_user *e = arg;
        ++erases;
        if (r != RW || e->length != geometry || e->start != erased * geometry ||
            e->start > N - geometry || backup_reads < 2U * N || written) ++bad;
        if (is("erase-fail") && erases == 3) return fail(EIO);
        memset(flash + e->start, 255, e->length); ++erased;
        if (is("interrupt-erase") && erased == 2 && handlers[SIGTERM]) handlers[SIGTERM](SIGTERM);
        return 0;
    }
    ++bad; return fail(ENOTTY); /* no unlock, OTP, status, whole-chip or other ioctl */
}
int r_flock(int fd, int operation)
{
    enum role r = role(fd);
    if ((r != IMAGE && r != RO && r != BACKUP) || operation != (LOCK_EX | LOCK_NB)) ++bad;
    return (is("owner-busy") && r == RO) || (is("image-busy") && r == IMAGE) ? fail(EWOULDBLOCK) : 0;
}
int r_fsync(int fd)
{
    enum role r = role(fd);
    if (r == BACKUP) { ++backup_sync; if (is("backup-sync")) return fail(EIO); }
    else if (r == DIRECTORY) { ++dir_sync; if (is("backup-dir-sync")) return fail(EIO); }
    else if (r == RW) {
        ++mtd_sync;
        if (is("mtd-sync-einval")) return fail(EINVAL);
        if (is("mtd-sync-eio")) return fail(EIO);
    } else ++bad;
    return 0;
}
uid_t r_getuid(void) { return is("uid") ? 1000 : 0; }
uid_t r_geteuid(void) { return is("euid") ? 1000 : 0; }
int r_sigaction(int sig, const struct sigaction *sa, struct sigaction *old)
{
    (void)old;
    if (is("signals")) return fail(EINVAL);
    if (sig < 0 || sig >= 64) { ++bad; return fail(EINVAL); }
    handlers[sig] = sa->sa_handler;
    return 0;
}
unsigned r_alarm(unsigned seconds) { if (seconds != 0 && seconds != 180) ++bad; return 0; }
int r_setrlimit(int resource, const struct rlimit *limit)
{
    if (resource != RLIMIT_CORE || limit->rlim_cur != 0 || limit->rlim_max != 0) ++bad;
    return is("core-limit") ? fail(EPERM) : 0;
}
ssize_t r_log(int fd, const void *data, size_t n)
{
    if (fd == 1) {
        if (is("receipt-fail")) return fail(EIO);
        if (n >= sizeof(output) - output_n) { ++bad; return fail(EIO); }
        memcpy(output + output_n, data, n); output_n += (unsigned)n;
    } else if (fd == 2) {
        if (n >= sizeof(errors) - errors_n) { ++bad; return fail(EIO); }
        memcpy(errors + errors_n, data, n); errors_n += (unsigned)n;
    } else { ++bad; return fail(EBADF); }
    return (ssize_t)n;
}
static void json_string(const char *s)
{
    putchar('"');
    for (; *s; ++s) {
        if (*s == '"' || *s == '\\') putchar('\\');
        if (*s == '\n') fputs("\\n", stdout);
        else if ((unsigned char)*s < 32) printf("\\u%04x", (unsigned char)*s);
        else putchar(*s);
    }
    putchar('"');
}
int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    scenario = argv[1];
    if (is("erase4k")) geometry = 4096;
    if (is("erase1m")) geometry = N;
    for (unsigned i = 0; i < N; ++i) {
        image[i] = (unsigned char)((i * 7U + i / 8191U) & 255U);
        flash[i] = original[i] = (unsigned char)((i * 13U + i / 4093U + 17U) & 255U);
    }
    argv[1] = argv[0];
    int rc = root_programmer_main(argc - 1, argv + 1);
    printf("{\"returncode\":%d,\"bad\":%u,\"opened\":%u,\"closed\":%u,\"ro_open\":%u,\"rw_open\":%u,"
           "\"creates\":%u,\"openat_calls\":%u,\"erase_calls\":%u,\"blocks_erased\":%u,\"write_calls\":%u,"
           "\"written\":%u,\"backup_written\":%u,\"backup_size\":%u,\"backup_reads\":%u,"
           "\"readback\":%u,\"max_io\":%u,\"max_flash_end\":%u,\"mtd_sync\":%u,"
           "\"flash_equals_image\":%s,\"flash_unchanged\":%s,\"backup_equals_original\":%s,\"out\":",
           rc, bad, opened, closed, ro_open, rw_open, creates, openat_calls, erases, erased, writes, written,
           backup_written, backup_size, backup_reads, readback, max_io, max_flash_end, mtd_sync,
           memcmp(flash, image, N) ? "false" : "true", memcmp(flash, original, N) ? "false" : "true",
           backup_size == N && !memcmp(backup, original, N) ? "true" : "false");
    json_string(output); fputs(",\"errors\":", stdout); json_string(errors); puts("}");
    return bad ? 3 : 0;
}
