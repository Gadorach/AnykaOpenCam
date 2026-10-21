/* Test-only ABI shim; never linked into the native payload. */
#ifndef STOCK_QUERY_MOCK_SYSCALLS_H
#define STOCK_QUERY_MOCK_SYSCALLS_H
uid_t q_getuid(void);
uid_t q_geteuid(void);
int q_uname(struct utsname *);
int q_sigaction(int, const struct sigaction *, struct sigaction *);
int q_sigprocmask(int, const sigset_t *, sigset_t *);
unsigned int q_alarm(unsigned int);
int q_clock_gettime(clockid_t, struct timespec *);
int q_lstat(const char *, struct stat *);
int q_open(const char *, int);
int q_fstat(int, struct stat *);
int q_ioctl(int, unsigned long, void *);
int q_close(int);
#endif
