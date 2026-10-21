/* Full native C control flow, but ALL device/runtime/time calls replaced.
 * Derived from the independent review's project-local host witness pattern.
 * Architecture macros select plain C only; this is not ARM emulation/execution.
 */
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
static uint32_t sys_page[1024],gpio_page[1024];
static uint64_t fake_now;
static unsigned gets,closed,unmapped;
static int scenario;
static int mock_open(const char *p,int flags,...) {
    if((flags&O_ACCMODE)!=O_RDONLY||!(flags&O_NOFOLLOW))abort();
    if(!strcmp(p,"/dev/mem"))return 100;
    if(!strcmp(p,"/dev/akgpio"))return 101;
    abort();
}
static int mock_fstat(int fd,struct stat *s) {
    if(fd!=100&&fd!=101)abort();
    memset(s,0,sizeof(*s));s->st_mode=0020000|0600;s->st_uid=0;
    s->st_rdev=fd==100?makedev(1,1):makedev(10,61);return 0;
}
static void *mock_mmap(void *a,size_t n,int prot,int flags,int fd,off_t off) {
    (void)a;if(n!=4096||prot!=PROT_READ||flags!=MAP_SHARED||fd!=100)abort();
    if(off==0x08000000)return sys_page;
    if(off==0x20170000)return gpio_page;
    abort();
}
static int mock_munmap(void *a,size_t n) {
    if(n!=4096||(a!=sys_page&&a!=gpio_page))abort();
    unmapped++;return 0;
}
static int mock_close(int fd);
static uid_t mock_geteuid(void){return 0;}
static int mock_uname(struct utsname *u){memset(u,0,sizeof(*u));strcpy(u->release,"3.4.35");return 0;}
static int mock_setrlimit(int resource,const struct rlimit *r){(void)resource;(void)r;return 0;}
static int mock_sigaction(int sig,const struct sigaction *a,struct sigaction *b){(void)sig;(void)a;(void)b;return 0;}
static int mock_clock(clockid_t c,struct timespec *t) {
    if(c!=CLOCK_MONOTONIC)abort();
    if(scenario==4){errno=EINVAL;return -1;}
    t->tv_sec=(time_t)(fake_now/1000000000ULL);t->tv_nsec=(long)(fake_now%1000000000ULL);return 0;
}
static int mock_sleep(const struct timespec *a,struct timespec *b) {
    (void)b;if(a->tv_sec||a->tv_nsec!=2000000)abort();
    if(scenario==8)return 0;
    if(scenario==9){fake_now=1000000000ULL;return 0;}
    fake_now+=2000000ULL;
    if(scenario==0)gpio_page[0x18/4]=(fake_now>=200000000ULL&&fake_now<400000000ULL)?0:1;
    return 0;
}
static int mock_ioctl(int fd,unsigned long req,...);
#define __arm__ 1
#define __ARM_ARCH 5
#define open mock_open
#define fstat mock_fstat
#define mmap mock_mmap
#define munmap mock_munmap
#define close mock_close
#define geteuid mock_geteuid
#define uname mock_uname
#define setrlimit mock_setrlimit
#define sigaction(...) mock_sigaction(__VA_ARGS__)
#define clock_gettime mock_clock
#define nanosleep mock_sleep
#define ioctl mock_ioctl
#define main observer_main
#include "gpio0_observer.c"
#undef main
static int mock_close(int fd) {
    if(fd!=100&&fd!=101)abort();
    closed++;
    if(scenario==5&&fd==101)stopped=SIGTERM;
    return 0;
}
static int mock_ioctl(int fd,unsigned long req,...) {
    if(fd!=101||req!=GPIO_GET)abort();
    va_list ap;va_start(ap,req);uint8_t *p=va_arg(ap,uint8_t *);va_end(ap);
    if(p[6]!=0xff)abort();
    gets++;
    if(scenario==1&&gets==2)sys_page[0x74/4]=1;
    if((scenario==2&&gets==2)||(scenario==6&&gets==1))stopped=SIGTERM;
    if(scenario==3&&gets==1)gpio_page[0x18/4]=0;
    if(scenario!=7)p[6]=(uint8_t)(gpio_page[0x18/4]&1);
    return 0;
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    scenario=atoi(argv[1]);if(scenario<0||scenario>9)return 2;
    gpio_page[0x18/4]=1;
    char *sample[]={"mock","--sample-ms","1000",NULL};char *status[]={"mock","--status",NULL};
    int status_case=scenario==3||scenario==5;
    int rc=status_case?observer_main(2,status):observer_main(3,sample);
    int expected=(scenario==0||scenario==3||scenario==9)?0:1;
    fprintf(stderr,"{\"scenario\":%d,\"observer_rc\":%d,\"expected_rc\":%d,\"gets\":%u,\"closed\":%u,\"unmapped\":%u,\"hardware_access\":false}\n",scenario,rc,expected,gets,closed,unmapped);
    return rc==expected&&closed==2&&unmapped==2?0:1;
}
