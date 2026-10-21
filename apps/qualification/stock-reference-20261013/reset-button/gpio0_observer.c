/* QC07F board5 reset-input candidate: READ ONLY, fixed GPIO0, no reset action.
 * Retail ABI/register facts: reset-button01/SOURCE_AUDIT01.md.
 * No SET/request/export/IRQ/mux/direction/pull/output writes or arbitrary address.
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

#define GPIO_GET 0xc001662bUL
#define GUARD 16U
#define WIRE 12U
struct gpio_wire { int32_t pin; uint8_t down,up,value,dir,pol,pad[3]; };
_Static_assert(sizeof(struct gpio_wire)==12,"retail wire size");
_Static_assert(offsetof(struct gpio_wire,value)==6,"retail value offset");
_Static_assert(((GPIO_GET>>16)&0x3fff)==1,"retail misleading ioctl encoded size");
struct snap { unsigned mux,dir,out,pull_disabled,level; };
static int input_ok(struct snap s) { return s.mux==0 && s.dir==0; }
static int config_equal(struct snap a,struct snap b) {
    return a.mux==b.mux && a.dir==b.dir && a.out==b.out && a.pull_disabled==b.pull_disabled;
}
static void init_wire(uint8_t *a) { memset(a,0xa5,GUARD+WIRE+GUARD);memset(a+GUARD,0,WIRE);a[GUARD+6]=0xff; }
static int reply_ok(const uint8_t *a) {
    uint8_t expected[GUARD+WIRE+GUARD];init_wire(expected);
    if(a[GUARD+6]>1)return 0;
    for(unsigned i=0;i<sizeof(expected);++i)if(i!=GUARD+6 && a[i]!=expected[i])return 0;
    return 1;
}
static int parse_ms(const char *s,unsigned *out) {
    unsigned n=0;if(!s||!*s)return 0;
    for(;*s;++s) { if(*s<'0'||*s>'9'||n>6000)return 0;n=n*10U+(unsigned)(*s-'0');if(n>60000)return 0; }
    if(n<1000)return 0;
    *out=n;return 1;
}

#ifndef GPIO0_HOST_TEST
#if !defined(__arm__) || __ARM_ARCH != 5 || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Native observer requires ARMv5 little endian; host uses pure fixture mode
#endif
static volatile sig_atomic_t stopped;
static void stop_handler(int sig) { stopped=sig; }
struct views { int mem,gpiofd; const volatile uint32_t *sys,*gpio; };
static uint64_t now_ns(void) {
    struct timespec ts;if(clock_gettime(CLOCK_MONOTONIC,&ts))return UINT64_MAX;
    return (uint64_t)ts.tv_sec*1000000000ULL+(uint64_t)ts.tv_nsec;
}
static int node_ok(int fd,unsigned maj,unsigned min) {
    struct stat s;return fstat(fd,&s)==0 && S_ISCHR(s.st_mode) && s.st_uid==0 &&
        major(s.st_rdev)==maj && minor(s.st_rdev)==min;
}
static int open_views(struct views *v) {
    v->mem=open("/dev/mem",O_RDONLY|O_SYNC|O_CLOEXEC|O_NOFOLLOW);
    if(v->mem<0||!node_ok(v->mem,1,1))return 0;
    v->sys=mmap(NULL,4096,PROT_READ,MAP_SHARED,v->mem,0x08000000);
    if(v->sys==MAP_FAILED){v->sys=NULL;return 0;}
    v->gpio=mmap(NULL,4096,PROT_READ,MAP_SHARED,v->mem,0x20170000);
    if(v->gpio==MAP_FAILED){v->gpio=NULL;return 0;}
    return 1;
}
static struct snap snapshot(const struct views *v) {
    return (struct snap){v->sys[0x74/4]&1U,v->gpio[0]&1U,v->gpio[0x0c/4]&1U,
                        v->sys[0x80/4]&1U,v->gpio[0x18/4]&1U};
}
static int get_only(struct views *v,unsigned *value) {
    union { uint32_t align; uint8_t bytes[GUARD+WIRE+GUARD]; } a;
    init_wire(a.bytes);
    if(v->gpiofd<0) {
        v->gpiofd=open("/dev/akgpio",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
        if(v->gpiofd<0||!node_ok(v->gpiofd,10,61))return 0;
    }
    if(ioctl(v->gpiofd,GPIO_GET,a.bytes+GUARD)!=0||!reply_ok(a.bytes))return 0;
    *value=a.bytes[GUARD+6];return 1;
}
static int close_views(struct views *v) {
    int ok=1;
    if(v->gpio && munmap((void *)v->gpio,4096))ok=0;
    if(v->sys && munmap((void *)v->sys,4096))ok=0;
    if(v->gpiofd>=0 && close(v->gpiofd))ok=0;
    if(v->mem>=0 && close(v->mem))ok=0;
    return ok;
}
static void describe(const char *state,struct snap s,unsigned agree,unsigned closed) {
    printf("{\"record\":\"reset-gpio0-v1\",\"state\":\"%s\",\"pin\":0,\"mux_bit\":%u,"
           "\"direction_bit\":%u,\"output_latch\":%u,\"pull_up_disabled\":%u,\"raw_level\":%u,"
           "\"GET_MMIO_agree\":%s,\"close_ok\":%s,\"read_only\":true}\n",state,s.mux,s.dir,s.out,s.pull_disabled,s.level,agree?"true":"false",closed?"true":"false");
    fflush(stdout);
}
int main(int argc,char **argv) {
    unsigned duration=0,value=0;int status=(argc==2 && !strcmp(argv[1],"--status"));
    if(!status && !(argc==3 && !strcmp(argv[1],"--sample-ms") && parse_ms(argv[2],&duration)))return 2;
    struct utsname u;struct rlimit core={0,0};
    if(geteuid()!=0||uname(&u)||strcmp(u.release,"3.4.35")||setrlimit(RLIMIT_CORE,&core))return 2;
    struct sigaction sa;memset(&sa,0,sizeof(sa));sa.sa_handler=stop_handler;sigemptyset(&sa.sa_mask);
    if(sigaction(SIGTERM,&sa,NULL)||sigaction(SIGINT,&sa,NULL)||sigaction(SIGHUP,&sa,NULL))return 2;
    struct views v={.mem=-1,.gpiofd=-1,.sys=NULL,.gpio=NULL};
    const char *failure="OPEN_READONLY_VIEWS";int ok=0,bad_config=0;struct snap first={0},after={0},checked={0};
    uint64_t start=0,previous=0,next_heartbeat=1000000000ULL,max_gap=0,elapsed=0;
    unsigned previous_level=0,edges=0;uint64_t counts[2]={0,0},samples=0;
    if(!open_views(&v))goto done;
    first=snapshot(&v);failure="GPIO0_NOT_ALREADY_GPIO_INPUT";
    if(!input_ok(first)){bad_config=1;goto done;}
    if(stopped){failure="INTERRUPTED";goto done;}
    failure="GET_ABI_OR_MMIO_MISMATCH";
    if(!get_only(&v,&value))goto done;
    checked=snapshot(&v);
    if(stopped){failure="INTERRUPTED";goto done;}
    if(!input_ok(checked)||checked.level!=value||!config_equal(first,checked))goto done;
    first=checked; /* The receipt describes the actual checked post-GET sample. */
    if(status){ok=1;goto done;}
    start=now_ns();previous=start;
    if(start==UINT64_MAX){failure="CLOCK";goto done;}
    if(stopped){failure="INTERRUPTED";goto done;}
    previous_level=first.level;describe("READY",first,1,0);
    for(;;) {
        uint64_t now=now_ns();if(now==UINT64_MAX||now<previous){failure="CLOCK";goto done;}
        if(stopped){failure="INTERRUPTED";goto done;}
        if(now-previous>max_gap)max_gap=now-previous;
        previous=now;elapsed=now-start;
        if(elapsed>=(uint64_t)duration*1000000ULL)break;
        if(samples>=duration){failure="SAMPLE_BUDGET";goto done;}
        struct snap s=snapshot(&v);failure="GPIO0_CONFIGURATION_CHANGED";
        if(!input_ok(s)||!config_equal(first,s))goto done;
        samples++;counts[s.level]++;
        if(s.level!=previous_level) {
            if(edges>=512){failure="EDGE_BUDGET";goto done;}
            printf("{\"record\":\"reset-gpio0-v1\",\"state\":\"EDGE\",\"elapsed_ms\":%"PRIu64",\"raw_level\":%u}\n",(uint64_t)(elapsed/1000000ULL),s.level);
            fflush(stdout);previous_level=s.level;edges++;
        }
        if(elapsed>=next_heartbeat){printf("{\"record\":\"reset-gpio0-v1\",\"state\":\"HEARTBEAT\",\"samples\":%"PRIu64",\"raw_level\":%u}\n",samples,s.level);fflush(stdout);next_heartbeat+=1000000000ULL;}
        struct timespec wait={0,2000000};if(nanosleep(&wait,NULL)&&errno!=EINTR){failure="SLEEP";goto done;}
    }
    after=snapshot(&v);failure="FINAL_GET_OR_CONFIGURATION_CHANGED";
    if(stopped){failure="INTERRUPTED";goto done;}
    if(!input_ok(after)||!config_equal(first,after)||!get_only(&v,&value))goto done;
    checked=snapshot(&v);
    if(stopped){failure="INTERRUPTED";goto done;}
    if(!input_ok(checked)||!config_equal(first,checked)||checked.level!=value)goto done;
    ok=1;
done:
    ;int closed_ok=close_views(&v);
    if(!closed_ok){ok=0;failure="CLOSE";}
    if(stopped){ok=0;failure="INTERRUPTED";}
    if(!ok) {
        if(bad_config)describe("HOLD_CONFIG",first,0,(unsigned)closed_ok);
        fprintf(stderr,"{\"record\":\"reset-gpio0-v1\",\"state\":\"HOLD\",\"reason\":\"%s\",\"read_only\":true}\n",failure);
        return 1;
    }
    if(status)describe("STATUS",checked,1,1);
    else printf("{\"record\":\"reset-gpio0-v1\",\"state\":\"COMPLETED\",\"duration_ms\":%u,\"elapsed_ms\":%"PRIu64",\"samples\":%"PRIu64",\"level0_samples\":%"PRIu64",\"level1_samples\":%"PRIu64",\"edges\":%u,\"max_gap_ns\":%"PRIu64",\"config_unchanged\":true,\"GET_MMIO_agree\":true,\"close_ok\":true,\"read_only\":true}\n",duration,(uint64_t)(elapsed/1000000ULL),samples,counts[0],counts[1],edges,max_gap);
    return fflush(stdout)?1:0;
}
#endif
