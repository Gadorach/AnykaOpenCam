/* Explicit host-only syscall substitution; never forward device operations. */
#ifndef STOCK_VIDEO_MOCK_H
#define STOCK_VIDEO_MOCK_H
uid_t v_getuid(void);
uid_t v_geteuid(void);
pid_t v_getpid(void);
int v_uname(struct utsname *);
int v_sigaction(int, const struct sigaction *, struct sigaction *);
int v_sigprocmask(int, const sigset_t *, sigset_t *);
unsigned int v_alarm(unsigned int);
int v_clock(clockid_t, struct timespec *);
int v_lstat(const char *, struct stat *);
int v_open(const char *, int);
int v_fstat(int, struct stat *);
int v_ioctl(int, unsigned long, void *);
int v_close(int);
#endif
