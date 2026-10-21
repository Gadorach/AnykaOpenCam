/* Explicit host MOCK. The fake mapping is deliberately not host-mapped memory:
 * any accidental payload dereference must fail rather than read real bytes. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "wire.h"
#include "platform.h"

static int client,handle,exported,mapped,pm,clocks,reads,allocs,maps,frees;
static void (*handler)(int);
static const uint32_t cookie=0x12340000U;
static int is(const char *s) { const char *c=getenv("ION_MOCK_CASE"); return c && !strcmp(c,s); }
static void event(const char *s) { fprintf(stderr,"MOCK %s\n",s); fflush(stderr); }
uid_t i_uid(void) { return is("uid")?1000:0; }
uid_t i_euid(void) { return is("euid")?1000:0; }
pid_t i_pid(void) { return is("pid1")?1:123; }
int i_uname(struct utsname *u)
{
    memset(u,0,sizeof(*u)); strcpy(u->sysname,"Linux");
    strcpy(u->release,is("kernel")?"6.18.55":"3.4.35"); return is("uname")?-1:0;
}
long i_pagesize(void) { return is("pagesize")?8192:4096; }
int i_action(int s,const struct sigaction *a,struct sigaction *old)
{
    assert(!old && !a->sa_flags && (s==SIGALRM || s==SIGINT || s==SIGTERM));
    handler=a->sa_handler; return is("sigsetup")?-1:0;
}
int i_sigmask(int how,const sigset_t *s,sigset_t *old)
{
    assert(how==SIG_UNBLOCK && !old && sigismember(s,SIGALRM) && sigismember(s,SIGINT) && sigismember(s,SIGTERM));
    return is("sigmask")?-1:0;
}
unsigned int i_alarm(unsigned int n) { assert(n==10 || n==0); return 0; }
int i_clock(clockid_t id,struct timespec *t)
{
    assert(id==CLOCK_MONOTONIC); ++clocks; t->tv_sec=clocks==1?100:101; t->tv_nsec=0;
    if (is("clockinvalid")) t->tv_nsec=-1;
    if (clocks>1 && is("clockback")) t->tv_sec=99;
    if (mapped && is("deadline")) t->tv_sec=110;
    return is("clockstart") || (mapped && is("clocklater"))?-1:0;
}
static void inode(struct stat *s)
{
    memset(s,0,sizeof(*s)); s->st_mode=S_IFCHR|0600; s->st_ino=2; s->st_dev=3; s->st_rdev=4;
}
int i_lstat(const char *p,struct stat *s)
{
    assert(!strcmp(p,"/dev/ion")); inode(s);
    if (is("symlink")) s->st_mode=S_IFLNK;
    if (is("regular")) s->st_mode=S_IFREG;
    return is("missing")?-1:0;
}
int i_open(const char *p,int flags)
{
    assert(flags==(O_RDONLY|O_CLOEXEC|O_NOFOLLOW));
    if (!strcmp(p,"/dev/ion")) {
        assert(!client); event("open_ion");
        if (is("open")) { errno=ENODEV; return -1; } client=1; return 31;
    }
    assert(!strcmp(p,"/proc/self/pagemap") && client && handle && exported && mapped && !pm);
    event("open_pm");
    if (is("pmopen")) { errno=EACCES; return -1; }
    pm=1; return 33;
}
int i_fstat(int fd,struct stat *s)
{
    assert(client); inode(s);
    if (fd==31) {
        if (is("race")) s->st_ino=99;
        return is("fstat")?-1:0;
    }
    assert(fd==32 && exported);
    s->st_mode=is("dmachr")?S_IFCHR:0600; /* exact old anon_inode has no type bits */
    return is("dmafstat")?-1:0;
}
int i_ioctl(int fd,unsigned long req,void *arg)
{
    assert(fd==31 && client);
    if (req==I_ALLOC) {
        struct alloc_wire *a=arg;
        assert(++allocs==1 && !handle && !maps);
        assert(a->len==OWN_BYTES && a->align==PAGE_BYTES && a->mask==2 && a->flags==3 && !a->handle);
        event("alloc");
        if (is("allocnomem")) { errno=ENOMEM; return -1; }
        if (is("alloceinval")) { errno=EINVAL; return -1; }
        if (is("allocefault")) { a->handle=cookie; errno=EFAULT; return -1; } /* rollback */
        handle=1;
        if (is("alloceintr")) { errno=EINTR; return -1; }
        a->handle=is("badhandle")?0:is("errhandle")?0xfffffff4U:cookie;
        if (is("allocpacket")) a->mask=4;
        if (is("allocoverrun")) ((unsigned char *)arg)[20]^=1;
        if (is("alarmalloc")) handler(SIGALRM);
        return is("allocpositive")?1:0;
    }
    if (req==I_MAP) {
        struct map_wire *m=arg;
        assert(++maps==1 && handle && m->handle==cookie && m->fd==-1 && !exported);
        event("map");
        if (is("mapnomem")) { m->fd=-ENOMEM; errno=ENOMEM; return -1; }
        if (is("mapeinval")) { errno=EINVAL; return -1; }
        exported=1;
        if (is("mapeintr") || is("mapeFAULT")) { errno=is("mapeintr")?EINTR:EFAULT; return -1; }
        m->fd=is("badfd")?1:is("aliasfd")?31:32;
        if (is("mappacket")) m->handle^=1;
        if (is("mapoverrun")) ((unsigned char *)arg)[8]^=1;
        if (is("alarmmap")) handler(SIGALRM);
        return is("mappositive")?1:0;
    }
    assert(req==I_FREE && handle && !exported && !mapped && !pm);
    assert(++frees==1 && ((struct free_wire *)arg)->handle==cookie); event("free");
    if (is("freefail")) { errno=EINTR; return -1; }
    handle=0;
    if (is("freeoverrun")) ((unsigned char *)arg)[4]^=1;
    if (is("alarmfree")) handler(SIGALRM);
    return is("freepositive")?1:0;
}
void *i_mmap(void *addr,size_t n,int prot,int flags,int fd,off_t off)
{
    assert(!addr && n==OWN_BYTES && prot==PROT_READ && flags==MAP_SHARED && fd==32 && !off);
    assert(handle && exported && !mapped); event("mmap");
    if (is("mmapfail")) { errno=ENOMEM; return MAP_FAILED; }
    mapped=1;
    if (is("alarmmmap")) handler(SIGALRM);
    return (void *)(uintptr_t)(is("mapunaligned")?0x40000001:is("mapoverflow")?0xffc00000:0x40000000);
}
ssize_t i_pread(int fd,void *out,size_t n,off_t off)
{
    unsigned char *p=out;
    unsigned int j,k,idx;
    uint64_t word,pfn;
    assert(fd==33 && pm && mapped && exported && handle && n<=256 && n>0 && !(n&7));
    assert(off>=0x200000 && off+(off_t)n<=0x204000 && !(off&7));
    assert(++reads<=128); event("pread");
    if (is("readerror")) { errno=EIO; return -1; }
    if (is("readeintr") || (is("eintronce") && reads==1)) { errno=EINTR; return -1; }
    if (is("readzero")) return 0;
    if (is("readmisaligned")) return 7;
    if (is("tinyreads")) n=8;
    if (is("shortread") && reads==1) n=128;
    if (is("alarmread")) handler(SIGALRM);
    if (is("interrupt")) handler(SIGINT);
    if (is("terminate")) handler(SIGTERM);
    for (j=0;j<n/8;++j) {
        idx=(unsigned int)((off-0x200000)/8)+j; pfn=0x80400+idx;
        if (is("noncontig") && idx==17) ++pfn;
        if (is("outofrange") && idx==17) pfn=0x81800;
        if (is("badend")) pfn=0x817ff+idx;
        word=(4ULL<<61)|(12ULL<<55)|pfn;
        if (is("missingpfn") && idx==17) word=12ULL<<55;
        if (is("swapped") && idx==17) word=(2ULL<<61)|(12ULL<<55)|pfn;
        if (is("format") && idx==17) word=(4ULL<<61)|(1ULL<<55)|pfn;
        if (is("reservedstatus") && idx==17) word|=1ULL<<61;
        for(k=0;k<8;++k) p[j*8+k]=(unsigned char)(word>>(k*8));
    }
    return (ssize_t)n;
}
int i_munmap(void *addr,size_t n)
{
    assert(addr==(void *)(uintptr_t)0x40000000 && n==OWN_BYTES && mapped && !pm);
    event("munmap");
    if (is("munmapfail")) { errno=EINVAL; return -1; }
    mapped=0; return 0;
}
int i_close(int fd)
{
    if (fd==33) { assert(pm); event("close_pm"); pm=0; if(is("pmclose")){errno=EINTR;return -1;} return 0; }
    if (fd==32) { assert(exported && !mapped && !pm); event("close_dma"); exported=0; if(is("dmaclose")){errno=EINTR;return -1;} return 0; }
    assert(fd==31 && client && !handle && !exported && !mapped && !pm); event("close_ion"); client=0;
    if(is("ionclose")){errno=EINTR;return -1;} return 0;
}
_Noreturn void i_quarantine(void)
{
    event("STOP"); fflush(stdout); _exit(125); /* mock only: never stop the host process */
}
