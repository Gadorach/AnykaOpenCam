/* Fixed retail DAC-only qualification, independently written; no vendor code
 * copied. Conditional entry/retirement contract: speaker-tone06/CONTRACT.md.
 * Parent alone admits the exact boot/source digest, RAW0 epoch, sole ownership,
 * physical UART TX/RX disconnection, bank lease and recovery. No persistent install.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#define PARAM_SET 0x401c50f2UL
#define PARAM_GET 0x801c50f2UL
#define FEATURES  0x804050f0UL
#define DEVICE_GET 0x80045070UL
#define SOURCE_GET 0x80045010UL
#define GAIN_GET  0x80045030UL
#define GAIN_SET  0x40045030UL
#define PREPARE   0x50e0UL
#define RESUME    0x50e1UL
#define PAUSE     0x50e2UL
#define PLAY_TIME 0x800850e5UL
#define READY    0x800450e7UL /* data-ready, NOT a DMA-state query */
#define BANK_LEASE_NAME "SPEAKER_TONE04_PARENT_GPIO0_SYS74_LEASE_V1"

enum { RATE=8000, CHANNELS=2, BITS=16, PERIOD=4096, PERIODS=12,
       RING=49152, FRAMES=1792, PREFILL=8192, BLOCK=512, PEAK=8192, FADE=64 };
_Static_assert(PREFILL + 2*PERIOD < RING && RING <= 131072, "empty-ring admission");
_Static_assert(FRAMES*4 <= PREFILL && PREFILL%PERIOD == 0, "whole periods");
_Static_assert(FRAMES*4 <= RATE, "tone <=250ms at requested rate");
static volatile sig_atomic_t cancelled;
static void mark_cancel(int sig) { (void)sig; cancelled=1; }

/* Exactly 16 blocks; public identical L/R samples followed by 1024 zero bytes.
 * No file/voice input, full-frame allocation, channel conversion or gain ramp. */
static void make_block(unsigned block, uint8_t out[BLOCK])
{
    static const int16_t cycle[8]={0,4096,8192,4096,0,-4096,-8192,-4096};
    for(unsigned i=0;i<BLOCK/4;++i) {
        unsigned frame=block*(BLOCK/4)+i;
        int32_t value=0;
        if(frame<FRAMES) {
            unsigned ramp=frame<FADE?frame:FADE, tail=FRAMES-1-frame;
            if(tail<ramp)ramp=tail;
            value=(int32_t)cycle[frame%8]*(int32_t)ramp/FADE;
        }
        uint16_t v=(uint16_t)value;
        out[4*i]=out[4*i+2]=(uint8_t)v;
        out[4*i+1]=out[4*i+3]=(uint8_t)(v>>8);
    }
}

struct run {
    int pcm, mem, resumed, stopped, closed, phase, close_uncertain;
    int snapshot_valid, latch_effect, restored, gain_verified, pcm_ever_open;
    int writable, mux_uncertain, control_fault, mux_verified;
    volatile uint32_t *sys;
    volatile uint32_t *gpio;
    uint64_t begun, played;
    uint32_t rate, gain, bytes, mux_before, mux_after, latch_before, latch_after;
    uint32_t dir_before, dir_after, input_before, input_after, restore_before, restore_after, mmio_writes;
    uint32_t mux_gpio, mux_during, mux_enter_writes, mux_exit_writes;
    uint32_t dir_during, latch_during, input_during;
    int hp_before_valid, gain_before_valid, gain_intent, gain_test_verified, gain_restored;
    int gain_set3_ok, gain_restore_ok, hp_close_valid, hp_after_valid;
    uint32_t gain_during, gain_after, gain_set3_attempts, gain_restore_attempts;
    uint32_t hp_before, hp_during, hp_after_close, hp_after;
};

static int stamp(uint64_t *ms)
{
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L)return -1;
    *ms=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;
    return 0;
}
static int budget(struct run *r,uint64_t limit)
{
    uint64_t t;
    return !stamp(&t) && t>=r->begun && t-r->begun<limit;
}
static int within(struct run *r){return !cancelled && budget(r,1800);}

/* Baseline views are read-only, then replaced with fixed RW views. Stores are
 * confined to the three sites below. Reads do not exclude other writers.
 * INPUT is recorded, never equated to pad voltage or the saved OUT latch. */
static int pins(struct run *r)
{
    uint32_t mux=r->sys[0x74/4], dir=r->gpio[0], out=r->gpio[0x0c/4], in=r->gpio[0x18/4];
    r->mux_after=mux; r->dir_after=dir;r->latch_after=out;r->input_after=in;
    uint32_t expected=r->mux_enter_writes&&!r->mux_exit_writes?r->mux_gpio:r->mux_before;
    return r->snapshot_valid && mux==expected && (mux&4)==4 &&
           dir==r->dir_before && (dir&2)==2 &&
           (out==2 || (r->latch_effect && out==0));
}

static int baseline(struct run *r)
{
    r->mux_before=r->sys[0x74/4];r->dir_before=r->gpio[0];
    r->latch_before=r->gpio[0x0c/4];r->input_before=r->gpio[0x18/4];
    if((r->mux_before&6)!=6 || !(r->dir_before&2) || r->latch_before!=2)return 0;
    r->mux_gpio=r->mux_before&~2U;
    r->snapshot_valid=1;
    return pins(r);
}

/* Three fixed production sites, no arbitrary physical address/value API. */
static inline void store_out1(volatile uint32_t *gpio)
{
#ifdef TONE_HOST_MMIO_STORE
    TONE_HOST_MMIO_STORE(&gpio[0x0c/4],2U);
#else
    gpio[0x0c/4]=2U;
#endif
}

static inline void store_mux_enter(struct run *r)
{
#ifdef TONE_HOST_MUX_ENTER
    TONE_HOST_MUX_ENTER(&r->sys[0x74/4],r->mux_before&~2U);
#else
    r->sys[0x74/4]=r->mux_before&~2U;
#endif
}
static inline void store_mux_exit(struct run *r)
{
#ifdef TONE_HOST_MUX_EXIT
    TONE_HOST_MUX_EXIT(&r->sys[0x74/4],r->mux_before);
#else
    r->sys[0x74/4]=r->mux_before;
#endif
}
static int block_controls(sigset_t *old)
{
    sigset_t block;
    return sigemptyset(&block)||sigaddset(&block,SIGINT)||sigaddset(&block,SIGTERM)||
           sigaddset(&block,SIGALRM)||sigaddset(&block,SIGHUP)||sigprocmask(SIG_BLOCK,&block,old);
}
static int controls_pending(void)
{
    sigset_t pending;
    if(sigpending(&pending))return -1;
    int a=sigismember(&pending,SIGINT),b=sigismember(&pending,SIGTERM),c=sigismember(&pending,SIGALRM),d=sigismember(&pending,SIGHUP);
    if(a<0||b<0||c<0||d<0)return -1;
    return a||b||c||d;
}
static int enter_mux(struct run *r)
{
    sigset_t old;int ok=0;
    if(!r->writable||r->pcm_ever_open||r->resumed||r->mux_enter_writes||r->mux_exit_writes||
       !within(r)||!pins(r))return 0;
    if(block_controls(&old))return 0;
    if(!within(r)||controls_pending()!=0||!pins(r))goto done;
    r->mux_enter_writes=1;
    store_mux_enter(r);
    r->mux_during=r->sys[0x74/4];
    ok=pins(r)&&r->mux_during==r->mux_gpio;
    r->dir_during=r->dir_after;r->latch_during=r->latch_after;r->input_during=r->input_after;
    if(!ok)r->mux_uncertain=1;
done:
    if(sigprocmask(SIG_SETMASK,&old,0)){r->control_fault=1;return 0;}
    return ok;
}

/* Known quiescence only: either never opened PCM, or confirmed last close after
 * inactive setup / normal PAUSE. Cancellation requests bounded rollback here;
 * it never authorizes repair of an uncertain active/close/store epoch. */
static int restore_quiescent(struct run *r)
{
    sigset_t old;
    int ok=0;
    if(r->pcm!=-1 || (r->pcm_ever_open&&!r->closed) || r->close_uncertain || r->mux_uncertain ||
       r->control_fault || (r->resumed&&!r->stopped) || r->mux_enter_writes!=1 ||
       r->mux_exit_writes || r->mmio_writes || r->restored || !r->writable)return 0;
    if(!pins(r))return 0;
    r->restore_before=r->restore_after=r->latch_after;
    if(block_controls(&old))return 0;
    /* Already-cancelled/pending controls are allowed for this known-zero-DMA
     * rollback, bounded by2s total. Mask covers only register checks/stores.
     * Source/owner exclusion, not this mask, protects against kernel writers. */
    if(!budget(r,2000)||controls_pending()<0||!pins(r))goto done;
    r->restore_before=r->latch_after;
    if(r->restore_before==0) {
        if(!r->closed)goto done; /* no unowned latch fix on unopened failure */
        r->mmio_writes=1;
        store_out1(r->gpio);
    }
    r->restore_after=r->gpio[0x0c/4];
    if(!pins(r)||r->restore_after!=2||r->latch_after!=2)goto done;
    r->restored=1;
    if(!budget(r,2000)||!pins(r))goto done;
    r->mux_exit_writes=1;
    store_mux_exit(r);
    ok=pins(r)&&r->mux_after==r->mux_before;
    r->mux_verified=ok;
    if(!ok)r->mux_uncertain=1;
done:
    if(sigprocmask(SIG_SETMASK,&old,0)){r->control_fault=1;return 0;}
    return ok && budget(r,2000);
}

/* Exact DAC request8000 uses only OSR table[0]=256; descending div256..1,
 * reject intermediate clocks >24MHz, strictly smaller error wins.
 * PLL fields follow retained ak_get_asic_pll_clk, not an ADC8012 constant. */
static uint32_t dac_rate(uint32_t reg)
{
    uint32_t od=(reg>>12)&3,n=(reg>>8)&15,m=reg&254;
    if(!od||n<2||n>6||m<84)return 0;
    uint32_t pll=(m*12/(n<<od))*1000000U, best=0, err=RATE;
    for(unsigned div=256;div;--div) {
        uint32_t intermediate=pll/div;
        if(intermediate>24000000)break;
        uint32_t rate=intermediate/256, e=rate>RATE?rate-RATE:RATE-rate;
        if(e<err){err=e;best=rate;}
    }
    return best>=7900 && best<=8100 ? best : 0;
}

static int packet(struct run *r,unsigned long op,uint32_t *data,unsigned words)
{
    uint32_t box[18]={0};
    if(words>16)return -1;
    box[0]=box[words+1]=0x937ea512U;
    memcpy(box+1,data,words*4);
    int rc=ioctl(r->pcm,op,box+1);
    if(box[0]!=0x937ea512U||box[words+1]!=0x937ea512U)return -2;
    if(rc!=0)return -1;
    memcpy(data,box+1,words*4);
    return 0;
}

/* Exact resource binding places HP bits22:18 at the already mapped SYS+0x9c.
 * Read witness only: no direct HP MMIO write is permitted. */
static uint32_t hp_raw(const struct run *r)
{
    const volatile uint32_t *view=r->sys;
    return view[0x9c/4];
}
static uint32_t hp_field(uint32_t raw){return (raw&0x007c0000U)>>18;}
static int gain_matches(struct run *r,uint32_t expected,uint32_t *cache,uint32_t *raw)
{
    int rc=packet(r,GAIN_GET,cache,1);
    *raw=hp_raw(r);
    return !rc && *cache==expected && hp_field(*raw)==(expected==1?8U:2U);
}
static int restore_gain_before_close(struct run *r)
{
    if(!r->gain_intent)return 1; /* Never normalize an unknown initial gain. */
    if(!r->gain_before_valid||r->gain!=1||r->pcm<0||r->closed||r->close_uncertain||
       (r->resumed&&!r->stopped)||r->gain_restore_attempts||!budget(r,2000)||
       !pins(r)||r->mux_uncertain||r->control_fault)return 0;
    uint32_t original=r->gain;
    r->gain_restore_attempts=1; /* No retry even on nonzero return/readback failure. */
    int rc=packet(r,GAIN_SET,&original,1);
    r->gain_restore_ok=rc==0;
    if(rc || !gain_matches(r,1,&r->gain_after,&r->hp_after) ||
       !budget(r,2000)||!pins(r))return 0;
    r->gain_restored=1;r->hp_after_valid=1;return 1;
}
static int gain_after_close(struct run *r,int final)
{
    uint32_t raw=hp_raw(r);
    r->hp_after=raw;r->hp_after_valid=1;
    if(!final){r->hp_after_close=raw;r->hp_close_valid=1;}
    uint32_t expected=r->gain_intent?8U:hp_field(r->hp_before);
    if(!r->hp_before_valid || (r->gain_intent&&!r->gain_restored)||hp_field(raw)!=expected){
        r->gain_restored=0;return 0;
    }
    return 1;
}

static void record(struct run *r,const char *state)
{
    unsigned total=r->mux_enter_writes+r->mmio_writes+r->mux_exit_writes;
    printf("{\"record\":\"stock-speaker-tone-v6\",\"state\":\"%s\",\"phase\":%d,"
           "\"requested_rate\":8000,\"calculated_DAC_rate\":%u,\"channels\":2,\"bits\":16,"
           "\"encoding\":\"S16_LE_IDENTICAL_LR\",\"tone_frames\":1792,\"digital_peak\":8192,"
           "\"nominal_tone_ms\":224,\"prefill_bytes\":%u,\"kernel_ring_bytes\":49152,"
           "\"period_time_us\":%llu,\"pause_completed\":%s,"
           "\"DMA_stop_evidence\":\"%s\",\"close_ok\":%s,\"close_uncertain\":%s,"
           "\"gain_before\":%u,\"gain_test\":3,\"gain_during\":%u,\"gain_after\":%u,"
           "\"gain_before_valid\":%s,\"gain_change_intent\":%s,\"gain_test_verified\":%s,\"gain_restored\":%s,"
           "\"gain_SET_attempts\":%u,\"gain_set3_attempts\":%u,\"gain_restore_attempts\":%u,"
           "\"gain_set3_return_zero\":%s,\"gain_restore_return_zero\":%s,"
           "\"HP9c_before\":%u,\"HP9c_during\":%u,\"HP9c_after_close\":%u,\"HP9c_after\":%u,"
           "\"HP_encoded_before\":%u,\"HP_encoded_during\":%u,\"HP_encoded_after\":%u,"
           "\"HP_before_valid\":%s,\"HP_after_close_valid\":%s,\"HP_after_valid\":%s,\"HP_direct_stores\":0,"
           "\"SYS74_before\":%u,\"SYS74_during\":%u,\"SYS74_after\":%u,\"DIR0_before\":%u,\"DIR0_during\":%u,\"DIR0_after\":%u,"
           "\"OUT0_before\":%u,\"OUT0_during\":%u,\"OUT0_after\":%u,\"INPUT0_before\":%u,\"INPUT0_during\":%u,\"INPUT0_after\":%u,"
           "\"MMIO_writes\":%s,\"MMIO_write_count\":%u,"
           "\"mux_enter_writes\":%u,\"OUT0_restore_writes\":%u,\"mux_exit_writes\":%u,"
           "\"mux_write_address\":\"0x08000074\",\"OUT0_write_address\":\"0x2017000c\",\"changed_mask\":2,"
           "\"mux_enter_after_close\":false,\"OUT0_restore_after_close\":%s,\"mux_exit_after_close\":%s,"
           "\"unopened_mux_rollback\":%s,\"mux_restoration_verified\":%s,\"restore_before\":%u,\"restore_after\":%u,"
           "\"latch_restoration_verified\":%s,\"bank_lease\":\"" BANK_LEASE_NAME "\",\"bank_lease_verified_by_helper\":false,"
           "\"UART_disconnection_admission\":\"USER_UART_SIGNALS_DISCONNECTED\",\"physical_disconnection_verified_by_helper\":false,"
           "\"amp_enable_pin\":1,\"amp_route\":\"SOURCE_EXPECTED_GPIO1_NOT_AUDIBILITY_PROOF\",\"physical_heard\":\"AWAIT_USER\","
           "\"physical_rate_measured\":false,\"image_digest_admission\":\"PARENT_EXTERNAL\"}\n",
           state,r->phase,r->rate,r->bytes,(unsigned long long)r->played,
           r->stopped?"true":"false",r->stopped?"SOURCE_CONDITIONAL_PAUSE_IRQ_COMPLETION":"NONE",
           r->closed?"true":"false",r->close_uncertain?"true":"false",
           r->gain,r->gain_during,r->gain_after,r->gain_before_valid?"true":"false",
           r->gain_intent?"true":"false",r->gain_test_verified?"true":"false",r->gain_restored?"true":"false",
           r->gain_set3_attempts+r->gain_restore_attempts,r->gain_set3_attempts,r->gain_restore_attempts,
           r->gain_set3_ok?"true":"false",r->gain_restore_ok?"true":"false",
           r->hp_before,r->hp_during,r->hp_after_close,r->hp_after,
           hp_field(r->hp_before),hp_field(r->hp_during),hp_field(r->hp_after),
           r->hp_before_valid?"true":"false",r->hp_close_valid?"true":"false",r->hp_after_valid?"true":"false",
           r->mux_before,r->mux_during,r->mux_after,r->dir_before,r->dir_during,r->dir_after,r->latch_before,r->latch_during,r->latch_after,
           r->input_before,r->input_during,r->input_after,total?"true":"false",total,
           r->mux_enter_writes,r->mmio_writes,r->mux_exit_writes,
           r->mmio_writes&&r->closed?"true":"false",r->mux_exit_writes&&r->closed?"true":"false",
           r->mux_exit_writes&&!r->pcm_ever_open?"true":"false",r->mux_verified?"true":"false",
           r->restore_before,r->restore_after,r->restored?"true":"false");
}

static int retain(struct run *r) __attribute__((noreturn));
static int retain(struct run *r)
{
    record(r,r->close_uncertain?"CLOSE_UNCERTAIN_RESET_REQUIRED":"RETAINED_RESET_REQUIRED");
    fflush(stdout);
    /* SIGCONT cannot make this process release its remaining owners. A stuck
     * kernel ioctl/hardIRQ may never reach here: external reset is independent. */
    for(;;)kill(getpid(),SIGSTOP);
}

static int same_node(const struct stat *a,const struct stat *b)
{
    return a->st_dev==b->st_dev && a->st_ino==b->st_ino && a->st_rdev==b->st_rdev &&
           S_ISCHR(b->st_mode) && !b->st_uid;
}
static int node(const char *path,unsigned maj,unsigned min,struct stat *s)
{
    return !lstat(path,s)&&S_ISCHR(s->st_mode)&&!s->st_uid&&
           major(s->st_rdev)==maj&&minor(s->st_rdev)==min;
}
static int text(const char *path,char *s,size_t cap)
{
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return -1;
    size_t used=0; int ok=0;
    while(used<cap-1) {
        ssize_t n=read(fd,s+used,cap-1-used);
        if(n<0)break;
        if(!n){ok=1;break;}
        used+=(size_t)n;
    }
    s[used]=0;
    if(close(fd))ok=0;
    return ok?0:-1;
}
static int metadata(void)
{
    char s[2048];
    if(text("/sys/class/akpcm_class0/akpcm_cdev0/dev",s,sizeof(s))||strcmp(s,"253:0\n"))return -1;
    if(text("/sys/class/akpcm_class1/akpcm_cdev1/dev",s,sizeof(s))||strcmp(s,"253:1\n"))return -1;
    if(text("/proc/devices",s,sizeof(s)))return -1;
    char *block=strstr(s,"Character devices:\n");
    if(block!=s)return -1;
    char *end=strstr(s,"\nBlock devices:");
    if(end)*end=0;
    unsigned found=0;
    for(char *line=strchr(s,'\n');line;line=strchr(line+1,'\n')) {
        if(!strncmp(line+1,"253 pcmchar\n",12) || !strcmp(line+1,"253 pcmchar"))++found;
    }
    return found==1?0:-1;
}

static int release_views(struct run *r)
{
    int fail=0;
    if(r->gpio && munmap((void *)r->gpio,4096))fail=1;
    if(r->sys && munmap((void *)r->sys,4096))fail=1;
    if(r->mem>=0 && close(r->mem))fail=1;
    r->sys=r->gpio=0; r->mem=-1;
    return fail;
}

static int writable_views(struct run *r)
{
    if(munmap((void *)r->gpio,4096))return -1;
    r->gpio=0;
    if(munmap((void *)r->sys,4096))return -1;
    r->sys=0;
    void *p=mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED,r->mem,0x08000000);
    if(p==MAP_FAILED)return -1;
    r->sys=p;
    p=mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED,r->mem,0x20170000);
    if(p==MAP_FAILED)return -1;
    r->gpio=p;r->writable=1;
    return pins(r)?0:-1;
}

/* Only before RESUME was even attempted. PAUSE is then a known inactive no-op;
 * last close's stop_hw is bound. Changed mux/DIR/other OUT bits never grant
 * release. After healthy close, only the admitted GPIO1 latent latch is restored. */
static int preactive_failure(struct run *r)
{
    if(r->resumed || !pins(r)||!budget(r,2000)||r->mux_uncertain||r->control_fault)return retain(r);
    if(!restore_gain_before_close(r))return retain(r);
    r->latch_effect=1;
    if(close(r->pcm)){r->close_uncertain=1;return retain(r);}
    r->pcm=-1;r->closed=1;
    if(!gain_after_close(r,0)||!restore_quiescent(r)||!gain_after_close(r,1))return retain(r);
    alarm(0);int fail=release_views(r);record(r,fail?"CLEANUP_FAILED":"PREACTIVE_FAILED_CLOSED");
    return 1;
}

int main(int argc,char **argv)
{
    struct run r={.pcm=-1,.mem=-1};
    struct stat before,after;struct utsname u;struct sigaction sa={0};sigset_t mask;
    uint32_t params[7]={RATE,CHANNELS,BITS,PERIOD,PERIODS,PERIOD,0},got[7]={0},features[16]={0};
    uint32_t v=0,expected=0,time_words[2]={0},hp=0;uint8_t block[BLOCK];
    if(argc!=2 || argv[0][0]!='/' || strcmp(argv[1],"--native-stock-speaker-tone-gain3-uart-isolated"))return 2;
    if(getuid()||geteuid()||getpid()==1||uname(&u)||strcmp(u.release,"3.4.35"))return 2;
    errno=0;if(fcntl(3,F_GETFD)!=-1||errno!=EBADF)return 2;
    sa.sa_handler=mark_cancel;sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT,&sa,0)||sigaction(SIGTERM,&sa,0)||sigaction(SIGALRM,&sa,0)||sigaction(SIGHUP,&sa,0)||
       sigemptyset(&mask)||sigaddset(&mask,SIGINT)||sigaddset(&mask,SIGTERM)||
       sigaddset(&mask,SIGALRM)||sigaddset(&mask,SIGHUP)||sigprocmask(SIG_UNBLOCK,&mask,0)||stamp(&r.begun))return 2;
    /* A broken result pipe must not terminate a retained epoch and implicitly
     * close its PCM fd. It is not evidence that DMA has stopped. */
    sa.sa_handler=SIG_IGN;
    if(sigaction(SIGPIPE,&sa,0))return 2;
    alarm(2);
    r.phase=1;
    if(metadata()||!node("/dev/mem",1,1,&before))goto unopened;
    r.mem=open("/dev/mem",O_RDWR|O_SYNC|O_CLOEXEC|O_NOFOLLOW);
    if(r.mem<0)goto unopened;
    if(fstat(r.mem,&after)||!same_node(&before,&after))goto unopened;
    void *p=mmap(0,4096,PROT_READ,MAP_SHARED,r.mem,0x08000000);
    if(p==MAP_FAILED)goto unopened;
    r.sys=p;
    p=mmap(0,4096,PROT_READ,MAP_SHARED,r.mem,0x20170000);
    if(p==MAP_FAILED)goto unopened;
    r.gpio=p;
    if(!baseline(&r))goto unopened;
    r.hp_before=hp_raw(&r);r.hp_before_valid=1;
    if(hp_field(r.hp_before)!=8)goto unopened;
    expected=dac_rate(r.sys[8/4]);
    if(!expected||!within(&r)||!node("/dev/akpcm_cdev0",253,0,&before))goto unopened;
    if(writable_views(&r)||!within(&r))goto unopened;
    if(!enter_mux(&r)){
        if(r.mux_enter_writes||r.control_fault)return retain(&r);
        goto unopened;
    }
    if(!within(&r))goto unopened;
    r.phase=2;
    r.pcm=open("/dev/akpcm_cdev0",O_WRONLY|O_CLOEXEC|O_NOFOLLOW);
    if(r.pcm<0)goto unopened;
    r.pcm_ever_open=1;
    if(fstat(r.pcm,&after)||!same_node(&before,&after))return retain(&r);
    /* No O_NONBLOCK claim: all writes fit the source-proved fresh empty ring. */
    int flags=fcntl(r.pcm,F_GETFL);
    if(flags<0 || (flags&O_ACCMODE)!=O_WRONLY || (flags&O_NONBLOCK))return preactive_failure(&r);
    if(!pins(&r)||!within(&r))return preactive_failure(&r);
    r.phase=3;
    if(packet(&r,DEVICE_GET,&v,1)||v!=1||packet(&r,SOURCE_GET,&v,1)||v!=4||
       !gain_matches(&r,1,&r.gain,&r.hp_before))
        return preactive_failure(&r);
    r.gain_before_valid=1;
    if(packet(&r,READY,&v,1)||v!=0)return preactive_failure(&r);
    if(ioctl(r.pcm,PAUSE,(void *)0)!=0||!within(&r))return preactive_failure(&r);
    r.phase=4;
    if(packet(&r,PARAM_SET,params,7)||packet(&r,PARAM_GET,got,7)||memcmp(params,got,28))
        return preactive_failure(&r);
    if(ioctl(r.pcm,PREPARE,(void *)0)!=0||!within(&r)||!pins(&r))return preactive_failure(&r);
    if(packet(&r,FEATURES,features,16))return preactive_failure(&r);
    r.rate=features[1];
    if(r.rate!=expected||!gain_matches(&r,1,&v,&hp)||packet(&r,PLAY_TIME,time_words,2)||
       time_words[0]||time_words[1]||packet(&r,READY,&v,1)||v!=0||!within(&r)||!pins(&r))return preactive_failure(&r);
    /* Any SET attempt may have side effects even if its return/canary fails. */
    r.gain_intent=1;r.gain_set3_attempts=1;v=3;
    int gain_rc=packet(&r,GAIN_SET,&v,1);r.gain_set3_ok=gain_rc==0;
    if(gain_rc||!gain_matches(&r,3,&r.gain_during,&r.hp_during)||!within(&r)||!pins(&r))return preactive_failure(&r);
    r.gain_test_verified=1;
    r.phase=5;
    for(unsigned i=0;i<PREFILL/BLOCK;++i) {
        if(!within(&r)||!pins(&r))return preactive_failure(&r);
        make_block(i,block);
        if(write(r.pcm,block,BLOCK)!=BLOCK)return preactive_failure(&r);
        r.bytes+=BLOCK;
    }
    if(packet(&r,READY,&v,1)||v!=1||!within(&r)||!pins(&r))return preactive_failure(&r);
    r.phase=6;r.resumed=1;r.latch_effect=1; /* attempted RESUME must retain on failure */
    if(ioctl(r.pcm,RESUME,(void *)0)!=0||!pins(&r)||!within(&r))return retain(&r);
    if(!gain_matches(&r,3,&r.gain_during,&r.hp_during))return retain(&r);
    r.dir_during=r.dir_after;r.latch_during=r.latch_after;r.input_during=r.input_after;
    r.phase=7;
    uint64_t previous=0,period_us=(uint64_t)(PERIOD/(CHANNELS*BITS/8))*1000000/r.rate;
    while(r.played<2*period_us) {
        if(!within(&r)||!pins(&r)||packet(&r,PLAY_TIME,time_words,2))return retain(&r);
        r.played=(uint64_t)time_words[0]|(uint64_t)time_words[1]<<32;
        if(r.played<previous || r.played>1000000)return retain(&r);
        previous=r.played;
        if(r.played<2*period_us) {
            struct timespec nap={0,2000000};
            if(nanosleep(&nap,0) && errno!=EINTR)return retain(&r);
        }
    }
    r.phase=8;
    if(!within(&r)||ioctl(r.pcm,PAUSE,(void *)0)!=0)return retain(&r);
    r.stopped=1; /* source-conditional hardIRQ completion, not READY inference */
    if(!within(&r)||!pins(&r)||packet(&r,READY,&v,1)||v!=0||
       !gain_matches(&r,3,&r.gain_during,&r.hp_during)||packet(&r,SOURCE_GET,&v,1)||v!=4)return retain(&r);
    r.gain_verified=1;r.phase=9;
    if(!restore_gain_before_close(&r))return retain(&r);
    if(close(r.pcm)){r.close_uncertain=1;return retain(&r);}
    r.pcm=-1;r.closed=1;
    if(!gain_after_close(&r,0)||!restore_quiescent(&r)||!gain_after_close(&r,1))return retain(&r);
    alarm(0);r.phase=10;
    if(release_views(&r)){record(&r,"CLEANUP_FAILED");return 1;}
    record(&r,cancelled?"CANCELLED_RESTORED":"TONE_COMPLETED_AWAIT_USER");return cancelled?1:0;
unopened:
    if(r.mux_enter_writes&&!restore_quiescent(&r))return retain(&r);
    alarm(0);release_views(&r);record(&r,"REFUSED_BEFORE_PCM_OPEN");return 2;
}
