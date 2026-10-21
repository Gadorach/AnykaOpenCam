#ifndef STOCK_ION_PLATFORM_H
#define STOCK_ION_PLATFORM_H
#ifdef STOCK_ION_MOCK
uid_t i_uid(void);
uid_t i_euid(void);
pid_t i_pid(void);
int i_uname(struct utsname *);
long i_pagesize(void);
int i_action(int, const struct sigaction *, struct sigaction *);
int i_sigmask(int, const sigset_t *, sigset_t *);
unsigned int i_alarm(unsigned int);
int i_clock(clockid_t, struct timespec *);
int i_lstat(const char *, struct stat *);
int i_open(const char *, int);
int i_fstat(int, struct stat *);
int i_ioctl(int, unsigned long, void *);
void *i_mmap(void *, size_t, int, int, int, off_t);
ssize_t i_pread(int, void *, size_t, off_t);
int i_munmap(void *, size_t);
int i_close(int);
_Noreturn void i_quarantine(void);
#define SELECTOR "--mock-stock-ion-inventory"
#else
#if !defined(__arm__) || __ARM_ARCH != 5 || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Native requires ARMv5 little-endian; host execution must be MOCK
#endif
_Static_assert(sizeof(void *) == 4 && sizeof(long) == 4, "stock native ARM32");
#define SELECTOR "--native-stock-ion-inventory"
#define i_uid getuid
#define i_euid geteuid
#define i_pid getpid
#define i_uname uname
#define i_pagesize() sysconf(_SC_PAGESIZE)
#define i_action sigaction
#define i_sigmask sigprocmask
#define i_alarm alarm
#define i_clock clock_gettime
#define i_lstat lstat
#define i_open open
#define i_fstat fstat
#define i_ioctl ioctl
#define i_mmap mmap
#define i_pread pread
#define i_munmap munmap
#define i_close close
static _Noreturn void i_quarantine(void)
{
    /* No cleanup guesses, fd scan or retry. Parent owns a bounded reset.
     * If externally continued, remain inert and retain all remaining owners. */
    (void)alarm(0);
    (void)kill(getpid(), SIGSTOP);
    for (;;) (void)pause();
}
#endif
#endif
