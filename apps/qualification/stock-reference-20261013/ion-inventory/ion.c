/* One owned ION allocation, metadata-only pagemap observation, no capture. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "wire.h"
#include "platform.h"

#define GA 0x93c51ea7U
#define GB 0x2e7bd409U
static volatile sig_atomic_t stopped;
static void signal_seen(int sig) { if (!stopped) stopped = sig; }
struct guarded_alloc { uint32_t a; struct alloc_wire p; uint32_t b; };
struct guarded_map { uint32_t a; struct map_wire p; uint32_t b; };
struct guarded_free { uint32_t a; struct free_wire p; uint32_t b; };
struct output { char bytes[768]; size_t used; int failed; };

static void text(struct output *o, const char *s)
{
    size_t n = strlen(s);
    if (n > sizeof(o->bytes) - o->used) { o->failed = 1; return; }
    memcpy(o->bytes + o->used, s, n); o->used += n;
}
static void number(struct output *o, uint64_t n)
{
    char s[21]; size_t k = sizeof(s)-1; s[k] = 0;
    do { s[--k] = (char)('0' + n%10); n /= 10; } while (n);
    text(o, s+k);
}
static uint64_t le64(const unsigned char *p)
{
    uint64_t n = 0; unsigned int k;
    for (k=0; k<8; ++k) n |= (uint64_t)p[k] << (8*k);
    return n;
}
static int report(const char *state, const char *reason, unsigned int pages,
                  uint32_t first, uint32_t last, int allocated, int mapped,
                  int proof, int cleaned, int code)
{
    struct output o = {{0},0,0};
    text(&o, "{\"record\":\"stock-ion-inventory-v1\",\"state\":\""); text(&o,state);
    text(&o,"\",\"reason\":\""); text(&o,reason);
    text(&o,"\",\"bytes\":8388608,\"alignment\":4096,\"heap_type\":2,\"heap_id\":1,\"mask\":2,\"flags\":3,\"pages_examined\":");
    number(&o,pages); text(&o,",\"first_pfn\":"); number(&o,first);
    text(&o,",\"last_pfn\":"); number(&o,last);
    text(&o,",\"allocation_ok\":"); text(&o,allocated?"true":"false");
    text(&o,",\"mapping_ok\":"); text(&o,mapped?"true":"false");
    text(&o,",\"pfn_contiguity_proved\":"); text(&o,proof?"true":"false");
    text(&o,",\"known_allocator_cleanup\":"); text(&o,cleaned?"true":"false");
    text(&o,",\"payload_accessed\":false,\"cache_or_capture_proved\":false}\n");
    if (o.failed || write(STDOUT_FILENO,o.bytes,o.used)!=(ssize_t)o.used) return 1;
    return code;
}
static _Noreturn void uncertain(const char *reason)
{
    struct output o = {{0},0,0};
    text(&o,"{\"record\":\"stock-ion-inventory-v1\",\"state\":\"UNCERTAIN_STOP\",\"reason\":\"");
    text(&o,reason);
    text(&o,"\",\"known_allocator_cleanup\":false,\"remaining_ownership\":\"retained_or_unknown\",\"payload_accessed\":false}\n");
    (void)write(STDOUT_FILENO,o.bytes,o.used);
    i_quarantine();
}
static int valid_time(const struct timespec *t)
{
    return t->tv_sec>=0 && t->tv_nsec>=0 && t->tv_nsec<1000000000L;
}
/* 0=in budget, 1=deadline/interrupted, -1=unavailable/backwards clock. */
static int budget(const struct timespec *start)
{
    struct timespec now;
    if (stopped) return 1;
    if (i_clock(CLOCK_MONOTONIC,&now) || !valid_time(&now) || now.tv_sec<start->tv_sec ||
        (now.tv_sec==start->tv_sec && now.tv_nsec<start->tv_nsec)) return -1;
    if (now.tv_sec-start->tv_sec>10 ||
        (now.tv_sec-start->tv_sec==10 && now.tv_nsec>=start->tv_nsec)) return 1;
    return 0;
}
static int handle_valid(uint32_t h) { return h!=0 && h<0xfffff001U; }
static int alloc_packet_ok(const struct guarded_alloc *a)
{
    return a->a==GA && a->b==GB && a->p.len==OWN_BYTES && a->p.align==PAGE_BYTES &&
           a->p.mask==HEAP_MASK && a->p.flags==ALLOC_FLAGS;
}
static int known_alloc_error(int e)
{
    /* EFAULT rolls back a successful alloc in exact retail ion_ioctl. */
    return e==ENOMEM || e==EINVAL || e==ENODEV || e==ENOTTY || e==EFAULT;
}
static int same_node(const struct stat *a,const struct stat *b)
{
    return S_ISCHR(b->st_mode) && a->st_dev==b->st_dev && a->st_ino==b->st_ino && a->st_rdev==b->st_rdev;
}

int main(int argc,char **argv)
{
    struct utsname u;
    struct stat before,after;
    struct sigaction sa;
    sigset_t mask;
    struct timespec start;
    struct guarded_alloc a = {GA,{OWN_BYTES,PAGE_BYTES,HEAP_MASK,ALLOC_FLAGS,0},GB};
    struct guarded_map m = {GA,{0,-1},GB};
    struct guarded_free f = {GA,{0},GB};
    unsigned char entries[256];
    unsigned int pages=0, readcalls=0, chunk, pos, attempt, k;
    uint32_t first=0,last=0;
    uint64_t entry,pfn;
    uintptr_t va;
    off_t offset;
    ssize_t n;
    void *mapping=MAP_FAILED;
    int ion=-1,dma=-1,pm=-1,rc,e,have_handle=0,allocated=0,mapped=0,proof=0,code=1;
    const char *reason="entry";

    if (argc!=2 || !argv[0] || argv[0][0]!='/' || strcmp(argv[1],SELECTOR))
        return report("ERROR","exact_absolute_selector",0,0,0,0,0,0,1,1);
    if (i_uid() || i_euid() || i_pid()==1 || i_uname(&u) || strcmp(u.sysname,"Linux") ||
        strcmp(u.release,"3.4.35") || i_pagesize()!=PAGE_BYTES)
        return report("ERROR","root_child_stock_kernel_page_size",0,0,0,0,0,0,1,1);
    memset(&sa,0,sizeof(sa)); sa.sa_handler=signal_seen;
    if (sigemptyset(&sa.sa_mask) || i_action(SIGALRM,&sa,0) || i_action(SIGINT,&sa,0) ||
        i_action(SIGTERM,&sa,0) || sigemptyset(&mask) || sigaddset(&mask,SIGALRM) ||
        sigaddset(&mask,SIGINT) || sigaddset(&mask,SIGTERM) || i_sigmask(SIG_UNBLOCK,&mask,0))
        return report("ERROR","signal_setup",0,0,0,0,0,0,1,1);
    (void)i_alarm(10);
    if (i_clock(CLOCK_MONOTONIC,&start) || !valid_time(&start)) { reason="clock_start"; goto cleanup; }
    if (i_lstat("/dev/ion",&before) || !S_ISCHR(before.st_mode)) { reason="ion_character_node"; goto cleanup; }
    if (stopped) { reason="signal"; goto cleanup; }
    ion=i_open("/dev/ion",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if (ion<0) { reason="ion_open"; goto cleanup; }
    if (ion<3) { reason="stdio_not_reserved"; goto cleanup; }
    if (i_fstat(ion,&after) || !same_node(&before,&after)) { reason="ion_identity"; goto cleanup; }
    rc=budget(&start); if (rc) { reason=rc<0?"clock":"deadline"; goto cleanup; }
    rc=i_ioctl(ion,I_ALLOC,&a.p); e=errno;
    if (!alloc_packet_ok(&a)) uncertain("alloc_packet_corrupt");
    if (rc!=0) {
        if (rc>0 || !known_alloc_error(e)) uncertain("alloc_result_unknown");
        reason=e==EFAULT?"alloc_EFAULT_rollback":"alloc_failed";
        goto cleanup; /* never FREE failed-ALLOC output */
    }
    if (!handle_valid(a.p.handle)) uncertain("alloc_handle_invalid");
    have_handle=1; allocated=1;
    if (stopped) { reason="signal"; goto cleanup; }
    m.p.handle=a.p.handle;
    rc=i_ioctl(ion,I_MAP,&m.p); e=errno;
    if (m.a!=GA || m.b!=GB || m.p.handle!=a.p.handle) uncertain("map_packet_corrupt");
    if (rc!=0) {
        if (rc>0 || e==EFAULT || e==EINTR || m.p.fd>=0 ||
            !(e==ENOMEM || e==EINVAL || e==ENODEV || e==ENOTTY)) uncertain("map_result_unknown");
        reason="map_failed"; goto cleanup;
    }
    if (m.p.fd<3 || m.p.fd==ion) uncertain("map_fd_invalid");
    dma=m.p.fd;
    /* Stock3.4 anon_inode has permissions but no S_IFREG/S_IFCHR type bits. */
    if (i_fstat(dma,&after) || (after.st_mode & 0170000)!=0) uncertain("dma_fd_identity");
    if (stopped) { reason="signal"; goto cleanup; }
    mapping=i_mmap(0,OWN_BYTES,PROT_READ,MAP_SHARED,dma,0);
    if (mapping==MAP_FAILED) { reason="mmap_failed"; goto cleanup; }
    mapped=1;
    va=(uintptr_t)mapping;
    if ((va & (PAGE_BYTES-1)) || va>UINT32_MAX-OWN_BYTES) uncertain("mapping_range_invalid");
    if (stopped) { reason="signal"; goto cleanup; }
    pm=i_open("/proc/self/pagemap",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if (pm<0) { reason="pagemap_unavailable"; code=2; goto cleanup; }
    if (pm==ion || pm==dma || pm<3) uncertain("pagemap_fd_invalid");
    offset=(off_t)(va/PAGE_BYTES)*8;
    for (chunk=0; chunk<OWN_PAGES/32; ++chunk) {
        rc=budget(&start); if (rc) { reason=rc<0?"clock":"deadline"; goto cleanup; }
        pos=0;
        for (attempt=0; attempt<2 && pos<sizeof(entries); ++attempt) {
            if (stopped) { reason="signal"; goto cleanup; }
            ++readcalls;
            n=i_pread(pm,entries+pos,sizeof(entries)-pos,offset+(off_t)chunk*256+pos);
            if (n<0 && errno==EINTR && !stopped) continue; /* at most one retry per chunk */
            if (n<=0 || (size_t)n>sizeof(entries)-pos || (n & 7)) {
                reason="pagemap_read_incomplete"; code=2; goto cleanup;
            }
            pos+=(unsigned int)n;
        }
        if (pos!=sizeof(entries) || readcalls>128) { reason="pagemap_read_bound"; code=2; goto cleanup; }
        for (k=0;k<32;++k) {
            entry=le64(entries+k*8); ++pages;
            if ((entry>>61)!=4) { reason="pagemap_not_present_or_status"; code=2; goto cleanup; }
            if (((entry>>55)&63)!=12) { reason="pagemap_not_stock_page_shift"; code=2; goto cleanup; }
            pfn=entry & ((1ULL<<55)-1);
            if (pfn<0x80000 || pfn>=0x81800) { reason="pfn_outside_reserved"; code=2; goto cleanup; }
            if (pages==1) first=(uint32_t)pfn;
            else if (pfn!=(uint64_t)last+1) { reason="pfn_noncontiguous"; code=2; goto cleanup; }
            last=(uint32_t)pfn;
        }
    }
    if (pages!=OWN_PAGES || last-first+1!=OWN_PAGES || (uint64_t)first+OWN_PAGES>0x81800)
        { reason="pfn_extent"; code=2; goto cleanup; }
    proof=1; code=0; reason="owned_mapping_pfn_verified";

cleanup:
    /* No queue, DMA consumer or mapped-byte access exists in this program. */
    if (pm>=0) { if (i_close(pm)) uncertain("pagemap_close_uncertain"); pm=-1; }
    if (mapping!=MAP_FAILED) {
        if (i_munmap(mapping,OWN_BYTES)) uncertain("munmap_uncertain");
        mapping=MAP_FAILED;
    }
    if (dma>=0) { if (i_close(dma)) uncertain("dma_close_uncertain"); dma=-1; }
    if (have_handle) {
        f.p.handle=a.p.handle;
        rc=i_ioctl(ion,I_FREE,&f.p);
        if (rc!=0 || f.a!=GA || f.b!=GB || f.p.handle!=a.p.handle) uncertain("free_uncertain");
        have_handle=0;
    }
    if (ion>=0) { if (i_close(ion)) uncertain("ion_close_uncertain"); ion=-1; }
    if (stopped) { code=stopped==SIGALRM?124:128+stopped; reason="signal_after_known_cleanup"; }
    else if (code==0) {
        rc=budget(&start);
        if (rc) { code=rc>0?124:1; reason=rc>0?"deadline_after_cleanup":"clock_after_cleanup"; }
    }
    (void)i_alarm(0);
    return report(code==0?"PASS":code==2?"HOLD":"ERROR",reason,pages,first,last,
                  allocated,mapped,proof,1,code);
}
