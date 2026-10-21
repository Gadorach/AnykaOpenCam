/* Compile-time syscall interposition for PUBLIC host fixtures only. */
#ifndef CR_ADAPTER_SYS_H
#define CR_ADAPTER_SYS_H
#ifdef CR_ADAPTER_MOCK
uid_t ca_getuid(void); uid_t ca_geteuid(void); pid_t ca_getpid(void);
int ca_uname(struct utsname *); long ca_sysconf(int);
int ca_fcntl(int,int,int); int ca_lstat(const char *,struct stat *);
int ca_open(const char *,int); int ca_fstat(int,struct stat *);
int ca_close(int); ssize_t ca_read(int,void *,size_t); ssize_t ca_write(int,const void *,size_t);
ssize_t ca_pread(int,void *,size_t,off_t); off_t ca_lseek(int,off_t,int);
int ca_ioctl(int,unsigned long,void *); void *ca_mmap(void *,size_t,int,int,int,off_t);
int ca_poll(struct pollfd *,nfds_t,int); int ca_clock_gettime(clockid_t,struct timespec *);
int ca_sigaction(int,const struct sigaction *,struct sigaction *);
int ca_sigprocmask(int,const sigset_t *,sigset_t *); unsigned ca_alarm(unsigned);
int ca_kill(pid_t,int); int ca_pause(void);
uint32_t ca_mock_va(const void *);
#else
#define ca_getuid getuid
#define ca_geteuid geteuid
#define ca_getpid getpid
#define ca_uname uname
#define ca_sysconf sysconf
#define ca_fcntl fcntl
#define ca_lstat lstat
#define ca_open open
#define ca_fstat fstat
#define ca_close close
#define ca_read read
#define ca_write write
#define ca_pread pread
#define ca_lseek lseek
#define ca_ioctl ioctl
#define ca_mmap mmap
#define ca_poll poll
#define ca_clock_gettime clock_gettime
#define ca_sigaction sigaction
#define ca_sigprocmask sigprocmask
#define ca_alarm alarm
#define ca_kill kill
#define ca_pause pause
#endif
#endif
