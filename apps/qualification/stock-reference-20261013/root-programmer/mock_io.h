/* GPL-2.0-only. Test-only IO seam, never linked into the ARM executable. */
#ifndef ROOT_PROGRAMMER_MOCK_IO_H
#define ROOT_PROGRAMMER_MOCK_IO_H
int r_open(const char *, int, mode_t);
int r_openat(int, const char *, int, mode_t);
int r_close(int);
ssize_t r_pread(int, void *, size_t, off_t);
ssize_t r_pwrite(int, const void *, size_t, off_t);
int r_fstat(int, struct stat *);
int r_stat(const char *, struct stat *);
int r_statfs(const char *, struct statfs *);
int r_fstatfs(int, struct statfs *);
int r_ioctl(int, unsigned long, void *);
int r_flock(int, int);
int r_fsync(int);
uid_t r_getuid(void);
uid_t r_geteuid(void);
int r_sigaction(int, const struct sigaction *, struct sigaction *);
unsigned r_alarm(unsigned);
int r_setrlimit(int, const struct rlimit *);
ssize_t r_log(int, const void *, size_t);
int root_programmer_main(int, char **);
#endif
