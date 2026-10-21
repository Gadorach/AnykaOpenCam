/* OUR camera-local numeric tone, copied from OUR console7, route08 candidate.
 * Route02 supplies ABI/polarity facts, NOT a copied retail implementation.
 * Parent owns current image/RAW0/bank lease/UART isolation/recovery admission. */
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
#define FEATURES 0x804050f0UL
#define DEVICE_GET 0x80045070UL
#define SOURCE_GET 0x80045010UL
#define GAIN_GET 0x80045030UL
#define GAIN_SET 0x40045030UL
#define SPEAKER_ENABLE 0x40045081UL
#define PREPARE 0x50e0UL
#define RESUME 0x50e1UL
#define PAUSE 0x50e2UL
#define PLAY_TIME 0x800850e5UL
#define READY 0x800450e7UL
#define AMBIENT "/sys/kernel/ain/ain0"
enum { RATE=8000, PERIOD=4096, RING=49152, BLOCK=512, WINDOW=4, GUARDS=3 };
static const uint32_t hp_codes[6]={16,8,4,2,1,0};
static volatile sig_atomic_t cancelled;
static void mark(int s){(void)s;cancelled=1;}
struct run {
    int pcm,mem,resumed,stopped,closed,ever,close_bad,snapshot,rw,mux_bad,control_bad;
    int restored,mux_verified,gain_bound,gain_intent,gain_restored,hp_valid,phase,stream_error;
    int isolation_asserted,speaker_intent,speaker_return_zero,speaker_low_verified;
    int low_after_resume,low_after_pause,low_after_close,out_after_close,mux_after_out;
    int hp_source_valid;
    volatile uint32_t *sys,*gpio;
    uint64_t begun,played,last_progress,last_sample;
    uint32_t frequency,test_gain,volume,duration,peak,rate,frames,frames_generated;
    uint32_t bytes,total_periods,content_periods,completed,period_us,refills;
    uint32_t gain_before,gain_during,gain_after,gain_sets,gain_restores;
    uint32_t hp_before,hp_during,hp_after_close,hp_after;
    uint32_t mux_before,mux_gpio,mux_during,mux_after,dir_before,dir_after,out_before,out_after,in_before,in_after;
    uint32_t enters,exits,out_writes,speaker_attempts,sys78_before,hp_source_during;
};
static int decimal(const char *s,uint32_t *v)
{
    uint32_t n=0;unsigned count=0;
    if(!*s)return -1;
    for(;*s;++s){if(*s<'0'||*s>'9'||++count>10)return -1;unsigned d=(unsigned)(*s-'0');
        if(n>(UINT32_MAX-d)/10)return -1;
        n=n*10+d;}
    *v=n;return 0;
}
static int stamp(uint64_t *v)
{
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L)return -1;
    *v=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;return 0;
}
static int budget(struct run *r,unsigned extra)
{uint64_t t;return !stamp(&t)&&t>=r->begun&&t-r->begun<(uint64_t)r->duration+extra;}
static int within(struct run *r){return !cancelled&&budget(r,2800);}
static int mask_controls(sigset_t *old)
{
    sigset_t s;
    return sigemptyset(&s)||sigaddset(&s,SIGINT)||sigaddset(&s,SIGTERM)||
        sigaddset(&s,SIGALRM)||sigaddset(&s,SIGHUP)||sigprocmask(SIG_BLOCK,&s,old);
}
static int pending(void)
{
    sigset_t s;if(sigpending(&s))return -1;
    int a=sigismember(&s,SIGINT),b=sigismember(&s,SIGTERM),c=sigismember(&s,SIGALRM),d=sigismember(&s,SIGHUP);
    return a<0||b<0||c<0||d<0?-1:a||b||c||d;
}
static int root_runtime(void)
{
    struct utsname u;
    if(getuid()||geteuid()||getpid()==1||uname(&u)||strcmp(u.release,"3.4.35"))return -1;
    errno=0;return fcntl(3,F_GETFD)==-1&&errno==EBADF?0:-1;
}
static int signals(void)
{
    struct sigaction sa={0};sigset_t s;sa.sa_handler=mark;sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT,&sa,0)||sigaction(SIGTERM,&sa,0)||sigaction(SIGALRM,&sa,0)||sigaction(SIGHUP,&sa,0)||
       sigemptyset(&s)||sigaddset(&s,SIGINT)||sigaddset(&s,SIGTERM)||sigaddset(&s,SIGALRM)||
       sigaddset(&s,SIGHUP)||sigprocmask(SIG_UNBLOCK,&s,0))return -1;
    sa.sa_handler=SIG_IGN;return sigaction(SIGPIPE,&sa,0);
}
static int node(const char *p,unsigned maj,unsigned min,struct stat *s)
{return !lstat(p,s)&&S_ISCHR(s->st_mode)&&!s->st_uid&&major(s->st_rdev)==maj&&minor(s->st_rdev)==min;}
static int identity(const struct stat *a,const struct stat *b)
{return a->st_dev==b->st_dev&&a->st_ino==b->st_ino&&a->st_rdev==b->st_rdev&&S_ISCHR(b->st_mode)&&!b->st_uid;}
static int text(const char *p,char *b,size_t cap)
{
    int fd=open(p,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return -1;
    size_t used=0;int ok=0;
    while(used<cap-1){ssize_t n=read(fd,b+used,cap-1-used);if(n<0)break;if(!n){ok=1;break;}used+=(size_t)n;}
    b[used]=0;if(close(fd))ok=0;return ok?0:-1;
}
static int metadata(void)
{
    char b[2048];
    if(text("/sys/class/akpcm_class0/akpcm_cdev0/dev",b,sizeof(b))||strcmp(b,"253:0\n"))return -1;
    if(text("/sys/class/akpcm_class1/akpcm_cdev1/dev",b,sizeof(b))||strcmp(b,"253:1\n"))return -1;
    if(text("/proc/devices",b,sizeof(b))||strncmp(b,"Character devices:\n",19))return -1;
    char *end=strstr(b,"\nBlock devices:");if(end)*end=0;
    unsigned n=0;for(char *p=strchr(b,'\n');p;p=strchr(p+1,'\n'))
        if(!strncmp(p+1,"253 pcmchar\n",12)||!strcmp(p+1,"253 pcmchar"))++n;
    return n==1?0:-1;
}
static uint32_t dac_rate(uint32_t reg)
{
    uint32_t od=(reg>>12)&3,n=(reg>>8)&15,m=reg&254;
    if(!od||n<2||n>6||m<84)return 0;
    uint32_t pll=(m*12/(n<<od))*1000000U,best=0,error=RATE;
    for(unsigned div=256;div;--div){uint32_t t=pll/div;if(t>24000000)break;
        uint32_t v=t/256,e=v>RATE?v-RATE:RATE-v;if(e<error){error=e;best=v;}}
    return best;
}
static uint32_t hp_raw(struct run *r){const volatile uint32_t *p=r->sys;return p[0x9c/4];}
static uint32_t hp_field(uint32_t v){return (v&0x007c0000U)>>18;}
static int hp_decode(uint32_t raw)
{for(unsigned i=0;i<6;++i)if(hp_codes[i]==hp_field(raw))return (int)i;return -1;}
static int views(struct run *r,int writable)
{
    if(!writable){struct stat a,b;
        if(!node("/dev/mem",1,1,&a))return -1;
        /* Status gets only O_RDONLY. Tone upgrades via a separate open below. */
        r->mem=open("/dev/mem",O_RDONLY|O_SYNC|O_CLOEXEC|O_NOFOLLOW);
        if(r->mem<0||fstat(r->mem,&b)||!identity(&a,&b))return -1;
    }else{
        if(munmap((void*)r->gpio,4096))return -1;
        r->gpio=0;
        if(munmap((void*)r->sys,4096))return -1;
        r->sys=0;
        if(close(r->mem)){r->mem=-1;return -1;}r->mem=-1;
        struct stat a,b;if(!node("/dev/mem",1,1,&a))return -1;
        r->mem=open("/dev/mem",O_RDWR|O_SYNC|O_CLOEXEC|O_NOFOLLOW);
        if(r->mem<0||fstat(r->mem,&b)||!identity(&a,&b))return -1;
    }
    int prot=writable?PROT_READ|PROT_WRITE:PROT_READ;
    void *p=mmap(0,4096,prot,MAP_SHARED,r->mem,0x08000000);if(p==MAP_FAILED)return -1;r->sys=p;
    p=mmap(0,4096,prot,MAP_SHARED,r->mem,0x20170000);if(p==MAP_FAILED)return -1;r->gpio=p;
    r->rw=writable;return 0;
}
static int release_views(struct run *r)
{
    int bad=0;if(r->gpio&&munmap((void*)r->gpio,4096))bad=1;
    if(r->sys&&munmap((void*)r->sys,4096))bad=1;
    if(r->mem>=0&&close(r->mem))bad=1;
    r->gpio=r->sys=0;r->mem=-1;return bad;
}
static int pins(struct run *r)
{
    uint32_t mux=r->sys[0x74/4],dir=r->gpio[0],out=r->gpio[3],in=r->gpio[6];
    r->mux_after=mux;r->dir_after=dir;r->out_after=out;r->in_after=in;
    /* HIGH before the explicit enable attempt and after OUT restoration; LOW
     * only in its intentional epoch. Never blanket-accept both latch values.
     * UART-mode INPUT is not an output witness; GPIO-mode INPUT must track LOW
     * or HIGH at all userspace fences (kernel power transition is synchronous). */
    uint32_t want=r->speaker_intent&&!r->out_writes?0U:2U;
    return r->snapshot&&mux==(r->enters&&!r->exits?r->mux_gpio:r->mux_before)&&(mux&4)==4&&
        r->sys[0x78/4]==r->sys78_before&&dir==r->dir_before&&(dir&2)&&!(dir&1)&&out==want&&
        (!(r->enters&&!r->exits)||(in&2)==want);
}
static int baseline(struct run *r)
{
    r->mux_before=r->sys[0x74/4];r->dir_before=r->gpio[0];r->out_before=r->gpio[3];r->in_before=r->gpio[6];
    r->sys78_before=r->sys[0x78/4];
    if((r->mux_before&6)!=6||!(r->dir_before&2)||(r->dir_before&1)||r->out_before!=2)return 0;
    r->mux_gpio=r->mux_before&~2U;r->snapshot=1;return pins(r);
}
static void mux_enter_store(struct run *r)
{
#ifdef CONSOLE_MUX_ENTER
    CONSOLE_MUX_ENTER(&r->sys[0x74/4],r->mux_before&~2U);
#else
    r->sys[0x74/4]=r->mux_before&~2U;
#endif
}
static void mux_exit_store(struct run *r)
{
#ifdef CONSOLE_MUX_EXIT
    CONSOLE_MUX_EXIT(&r->sys[0x74/4],r->mux_before);
#else
    r->sys[0x74/4]=r->mux_before;
#endif
}
static void out_store(struct run *r)
{
#ifdef CONSOLE_OUT_STORE
    CONSOLE_OUT_STORE(&r->gpio[3],2U);
#else
    r->gpio[3]=2U;
#endif
}
static int enter_mux(struct run *r)
{
    sigset_t old;int ok=0;
    if(!r->rw||r->ever||r->enters||!within(r)||!pins(r)||mask_controls(&old))return 0;
    if(within(r)&&pending()==0&&pins(r)){r->enters=1;mux_enter_store(r);r->mux_during=r->sys[0x74/4];
        ok=pins(r)&&r->mux_during==r->mux_gpio;if(!ok)r->mux_bad=1;}
    if(sigprocmask(SIG_SETMASK,&old,0)){r->control_bad=1;return 0;}return ok;
}
static int restore_mux(struct run *r)
{
    sigset_t old;int ok=0;
    if(r->pcm!=-1||(r->ever&&!r->closed)||r->close_bad||r->mux_bad||r->control_bad||
       (r->resumed&&!r->stopped)||r->enters!=1||r->exits||r->out_writes||r->restored||!r->rw||!pins(r))return 0;
    if(mask_controls(&old))return 0;
    if(!budget(r,3000)||pending()<0||!pins(r))goto done;
    if(r->out_after==0){if(!r->closed||!r->speaker_intent||!r->speaker_low_verified)goto done;
        r->out_writes=1;out_store(r);
        if(pins(r)&&r->out_after==2)r->out_after_close=1;}
    if(!pins(r)||r->out_after!=2)goto done;
    r->restored=1;
    if(!budget(r,3000)||!pins(r))goto done;
    r->exits=1;mux_exit_store(r);ok=pins(r)&&r->mux_after==r->mux_before;r->mux_verified=ok;
    r->mux_after_out=ok&&r->closed&&r->out_after_close&&r->out_writes==1;
    if(!ok)r->mux_bad=1;
done:
    if(sigprocmask(SIG_SETMASK,&old,0)){r->control_bad=1;return 0;}return ok&&budget(r,3000);
}
static int packet(struct run *r,unsigned long op,uint32_t *p,unsigned words)
{
    uint32_t b[18]={0};if(words>16)return -1;b[0]=b[words+1]=0x937ea512U;
    memcpy(b+1,p,words*4);int rc=ioctl(r->pcm,op,b+1);
    if(rc||b[0]!=0x937ea512U||b[words+1]!=0x937ea512U)return -1;
    memcpy(p,b+1,words*4);return 0;
}
static int gain_match(struct run *r,uint32_t want,uint32_t *cache,uint32_t *raw)
{
    int rc=packet(r,GAIN_GET,cache,1);*raw=hp_raw(r);
    return !rc&&want<6&&*cache==want&&hp_field(*raw)==hp_codes[want];
}
static int restore_gain(struct run *r)
{
    if(!r->gain_intent)return 1;
    if(!r->gain_bound||r->gain_before>5||r->pcm<0||r->closed||r->close_bad||
       (r->resumed&&!r->stopped)||r->gain_restores||!budget(r,3000)||!pins(r)||r->mux_bad||r->control_bad)return 0;
    uint32_t v=r->gain_before;r->gain_restores=1;
    if(packet(r,GAIN_SET,&v,1)||!gain_match(r,r->gain_before,&r->gain_after,&r->hp_after)||!budget(r,3000)||!pins(r))return 0;
    r->gain_restored=1;return 1;
}
static int gain_closed(struct run *r,int final)
{
    r->hp_after=hp_raw(r);if(!final)r->hp_after_close=r->hp_after;
    if(!r->hp_valid||(r->gain_intent&&!r->gain_restored)||hp_field(r->hp_after)!=hp_field(r->hp_before)){
        r->gain_restored=0;return 0;}
    return 1;
}
static void record(struct run *r,const char *state)
{
    unsigned effects=r->enters+r->exits+r->out_writes+r->gain_sets+r->gain_restores+r->speaker_attempts;
    printf("{\"record\":\"stock-speaker-route-v8\",\"state\":\"%s\",\"phase\":%d,\"effects\":%u,"
      "\"frequency_hz\":%u,\"gain_test\":%u,\"volume_percent\":%u,\"duration_ms\":%u,\"digital_peak\":%u,"
      "\"frames_generated\":%u,\"calculated_DAC_rate\":%u,\"frequency_max_hz\":%u,\"channels\":2,\"bits\":16,"
      "\"bytes_submitted\":%u,\"kernel_ring_bytes\":49152,\"kernel_ring_backing_bytes\":65536,\"scratch_bytes\":512,\"periods_completed\":%u,\"refill_periods\":%u,\"stream_error\":%d,"
      "\"pause_completed\":%s,\"close_ok\":%s,\"close_uncertain\":%s,\"DMA_stop_evidence\":\"%s\","
      "\"gain_before\":%u,\"gain_during\":%u,\"gain_after\":%u,\"gain_restored\":%s,\"gain_SET_attempts\":%u,"
      "\"HP_encoded_before\":%u,\"HP_encoded_during\":%u,\"HP_encoded_after\":%u,\"HP9c_before\":%u,\"HP9c_during\":%u,\"HP9c_after\":%u,\"HP_direct_stores\":0,"
      "\"SYS74_before\":%u,\"SYS74_during\":%u,\"SYS74_after\":%u,\"DIR0_before\":%u,\"DIR0_after\":%u,\"OUT0_before\":%u,\"OUT0_after\":%u,"
      "\"mux_restoration_verified\":%s,\"latch_restoration_verified\":%s,\"MMIO_write_count\":%u,\"mux_enter_writes\":%u,\"OUT0_restore_writes\":%u,\"mux_exit_writes\":%u,"
      "\"speaker_enable_intent\":%s,\"speaker_enable_attempts\":%u,\"speaker_enable_word\":%u,\"speaker_enable_bytes\":%u,\"speaker_enable_return_zero\":%s,\"speaker_enabled_low\":%s,"
      "\"speaker_low_after_RESUME\":%s,\"speaker_low_after_PAUSE\":%s,\"speaker_low_after_close\":%s,\"OUT_restore_after_close\":%s,\"mux_restore_after_OUT_restore\":%s,"
      "\"HP_source_during\":%u,\"HP_source_during_valid\":%s,\"SYS78_before\":%u,\"INPUT0_latest\":%u,\"GPIO0_DIR_PULL_writes\":0,"
      "\"physical_heard\":\"AWAIT_USER\",\"bank_lease_verified_by_helper\":false,\"UART_isolation_token\":%s,\"physical_isolation_verified_by_helper\":false,\"UART_disconnection_admission\":\"USER_UART_SIGNALS_DISCONNECTED\"}\n",
      state,r->phase,effects,r->frequency,r->test_gain,r->volume,r->duration,r->peak,r->frames_generated,r->rate,r->rate?(r->rate-1)/2:0,
      r->bytes,r->completed,r->refills,r->stream_error,r->stopped?"true":"false",r->closed?"true":"false",r->close_bad?"true":"false",
      r->stopped?"SOURCE_CONDITIONAL_PAUSE_IRQ_COMPLETION":"NONE",r->gain_before,r->gain_during,r->gain_after,r->gain_restored?"true":"false",r->gain_sets+r->gain_restores,
      hp_field(r->hp_before),hp_field(r->hp_during),hp_field(r->hp_after),r->hp_before,r->hp_during,r->hp_after,
      r->mux_before,r->mux_during,r->mux_after,r->dir_before,r->dir_after,r->out_before,r->out_after,
      r->mux_verified?"true":"false",r->restored?"true":"false",r->enters+r->out_writes+r->exits,r->enters,r->out_writes,r->exits,
      r->speaker_intent?"true":"false",r->speaker_attempts,r->speaker_attempts?1U:0U,r->speaker_attempts*4,r->speaker_return_zero?"true":"false",r->speaker_low_verified?"true":"false",
      r->low_after_resume?"true":"false",r->low_after_pause?"true":"false",r->low_after_close?"true":"false",r->out_after_close?"true":"false",r->mux_after_out?"true":"false",
      r->hp_source_during,r->hp_source_valid?"true":"false",r->sys78_before,r->in_after,r->isolation_asserted?"true":"false");
}
static int retain(struct run *r) __attribute__((noreturn));
static int retain(struct run *r)
{record(r,r->close_bad?"CLOSE_UNCERTAIN_RESET_REQUIRED":"RETAINED_RESET_REQUIRED");fflush(stdout);for(;;)kill(getpid(),SIGSTOP);}
static int preactive_failure(struct run *r)
{
    if(r->resumed||!pins(r)||!budget(r,3000)||r->mux_bad||r->control_bad||!restore_gain(r))return retain(r);
    if(close(r->pcm)){r->close_bad=1;return retain(r);}r->pcm=-1;r->closed=1;
    r->low_after_close=r->speaker_low_verified&&pins(r);
    if(!gain_closed(r,0)||!restore_mux(r)||!gain_closed(r,1))return retain(r);
    alarm(0);int bad=release_views(r);record(r,bad?"CLEANUP_FAILED":"PREACTIVE_FAILED_CLOSED");return 1;
}

/* Rational phase, no waveform allocation. Rounding is symmetric about zero. */
static int32_t triangle(uint32_t phase,uint32_t peak,uint32_t rate)
{
    int64_t q=(int64_t)phase*4,num=q<rate?q:q<(int64_t)rate*3?(int64_t)rate*2-q:q-(int64_t)rate*4;
    return (int32_t)((int64_t)peak*num/rate);
}
static void make_block(struct run *r,uint32_t byte_offset,uint8_t out[BLOCK])
{
    for(unsigned i=0;i<BLOCK/4;++i){uint32_t frame=byte_offset/4+i;int32_t value=0;
        if(frame<r->frames){uint32_t phase=(uint32_t)((uint64_t)frame*r->frequency%r->rate);
            unsigned ramp=frame<64?frame:64,tail=r->frames-1-frame;if(tail<ramp)ramp=tail;
            value=triangle(phase,r->peak,r->rate)*(int32_t)ramp/64;}
        uint16_t u=(uint16_t)value;out[4*i]=out[4*i+2]=(uint8_t)u;out[4*i+1]=out[4*i+3]=(uint8_t)(u>>8);}
}

/* Counter low32 never wraps in this <=8s epoch. High word must stay0; raw ISR
 * increments by exact floor(1024e6/rate) each completion. UP primary IRQ tail
 * completes before userspace. No AVAILABLE/NONBLOCK assumption. */
static int progress(struct run *r,uint32_t published_before)
{
    uint32_t t[2]={0};uint64_t now;
    if(!within(r)||!pins(r)||stamp(&now)||now<r->last_sample){r->stream_error=1;return 0;}
    r->last_sample=now;
    if(packet(r,PLAY_TIME,t,2)||t[1]||t[0]%r->period_us||t[0]<r->played){r->stream_error=2;return 0;}
    uint32_t c=t[0]/r->period_us;
    if(c>r->total_periods||c+1>=published_before/PERIOD){r->stream_error=3;return 0;}
    if(t[0]>r->played)r->last_progress=now;
    else if(now-r->last_progress>1000){r->stream_error=4;return 0;}
    r->played=t[0];r->completed=c;return 1;
}
static int submit_period(struct run *r,int live)
{
    uint8_t block[BLOCK];
    for(unsigned i=0;i<PERIOD/BLOCK;++i){uint32_t before=r->bytes;
        if(!within(r)||!pins(r))return 0;
        if(live){
            if(!progress(r,before))return 0;
            uint32_t used=before-r->completed*PERIOD;
            if(used>WINDOW*PERIOD||RING-used<BLOCK+2*PERIOD){r->stream_error=5;return 0;}
        }
        make_block(r,before,block);
        if(write(r->pcm,block,BLOCK)!=BLOCK){r->stream_error=6;return 0;}
        r->bytes+=BLOCK;
        uint32_t generated=r->bytes/4;r->frames_generated=generated<r->frames?generated:r->frames;
        /* Old published prefix is the conservative low-water witness even if
         * the kernel write/poll was preempted. A failed witness retains. */
        if(live&&!progress(r,before))return 0;
    }
    if(live)++r->refills;
    return 1;
}
static int tone(struct run *r)
{
    struct stat a,b;uint32_t params[7]={RATE,2,16,PERIOD,12,PERIOD,0},got[7]={0},f[16]={0},v=0,hp=0,t[2]={0};
    if(root_runtime()||signals()||stamp(&r->begun))return 2;
    alarm((r->duration+3999)/1000);r->phase=1;
    if(metadata()||views(r,0))goto unopened;
    r->rate=dac_rate(r->sys[2]);
    if(!r->rate||r->frequency>(r->rate-1)/2)goto unopened;
    if(!r->duration){alarm(0);int bad=release_views(r);record(r,bad?"CLEANUP_FAILED":"TONE_NOOP_ZERO_DURATION");return bad?1:0;}
    if(!r->isolation_asserted)goto unopened;
    if(!baseline(r))goto unopened;
    r->hp_before=hp_raw(r);r->hp_valid=1;if(hp_decode(r->hp_before)<0)goto unopened;
    r->frames=(uint32_t)(((uint64_t)r->rate*r->duration+500)/1000);
    r->content_periods=(r->frames*4+PERIOD-1)/PERIOD;r->total_periods=r->content_periods+GUARDS;
    r->period_us=1024000000U/r->rate;
    if(!r->period_us||!within(r)||!node("/dev/akpcm_cdev0",253,0,&a)||views(r,1)||!pins(r))goto unopened;
    if(!enter_mux(r)){if(r->enters||r->control_bad)return retain(r);goto unopened;}
    if(!within(r))goto unopened;
    r->phase=2;r->pcm=open("/dev/akpcm_cdev0",O_WRONLY|O_CLOEXEC|O_NOFOLLOW);if(r->pcm<0)goto unopened;r->ever=1;
    if(fstat(r->pcm,&b)||!identity(&a,&b))return retain(r);
    int flags=fcntl(r->pcm,F_GETFL);
    if(flags<0||(flags&O_ACCMODE)!=O_WRONLY||(flags&O_NONBLOCK)||!pins(r)||!within(r))return preactive_failure(r);
    r->phase=3;
    if(packet(r,DEVICE_GET,&v,1)||v!=1||packet(r,SOURCE_GET,&v,1)||v!=4||packet(r,GAIN_GET,&r->gain_before,1))return preactive_failure(r);
    r->hp_before=hp_raw(r);
    if(r->gain_before>5||hp_field(r->hp_before)!=hp_codes[r->gain_before])return preactive_failure(r);
    r->gain_bound=1;
    if(packet(r,READY,&v,1)||v||ioctl(r->pcm,PAUSE,(void*)0)!=0||!within(r))return preactive_failure(r);
    r->phase=4;
    if(packet(r,PARAM_SET,params,7)||packet(r,PARAM_GET,got,7)||memcmp(got,params,28)||
       ioctl(r->pcm,PREPARE,(void*)0)!=0||!within(r)||!pins(r)||packet(r,FEATURES,f,16)||f[1]!=r->rate||
       !gain_match(r,r->gain_before,&v,&hp)||packet(r,PLAY_TIME,t,2)||t[0]||t[1]||packet(r,READY,&v,1)||v)return preactive_failure(r);
    r->gain_intent=1;r->gain_sets=1;v=r->test_gain;
    if(packet(r,GAIN_SET,&v,1)||!gain_match(r,r->test_gain,&r->gain_during,&r->hp_during)||!within(r)||!pins(r))return preactive_failure(r);
    /* Intent precedes syscall: even an error/canary fault may follow effects.
     * Unknown enable result retains, without close/repair. Known successful low
     * plus a later preactive error may use the existing zero-DMA cleanup path. */
    if(!within(r)||!pins(r))return preactive_failure(r);
    r->speaker_intent=1;r->speaker_attempts=1;v=1;
    if(packet(r,SPEAKER_ENABLE,&v,1))return retain(r);
    r->speaker_return_zero=1;
    if(!pins(r))return retain(r);
    r->speaker_low_verified=1;
    if(!within(r))return preactive_failure(r);
    r->phase=5;for(unsigned i=0;i<WINDOW;++i)if(!submit_period(r,0))return preactive_failure(r);
    if(packet(r,READY,&v,1)||v!=1||!within(r)||!pins(r))return preactive_failure(r);
    r->phase=6;r->resumed=1;
    if(ioctl(r->pcm,RESUME,(void*)0)!=0||!within(r)||!pins(r)||!gain_match(r,r->test_gain,&r->gain_during,&r->hp_during)||stamp(&r->last_progress))return retain(r);
    r->low_after_resume=1;r->hp_source_during=(hp_raw(r)>>14)&7U;r->hp_source_valid=1;
    r->last_sample=r->last_progress;r->phase=7;
    for(;;){
        if(!progress(r,r->bytes))return retain(r);
        if(r->completed>=r->content_periods)break;
        if(r->bytes/PERIOD<r->total_periods && r->bytes/PERIOD<r->completed+WINDOW){
            if(!submit_period(r,1))return retain(r);
        }else{struct timespec nap={0,2000000};if(nanosleep(&nap,0)&&errno!=EINTR)return retain(r);}
    }
    r->phase=8;
    if(!within(r)||ioctl(r->pcm,PAUSE,(void*)0)!=0)return retain(r);
    r->stopped=1;
    if(!within(r)||!pins(r)||packet(r,READY,&v,1)||v||!gain_match(r,r->test_gain,&r->gain_during,&r->hp_during)||packet(r,SOURCE_GET,&v,1)||v!=4)return retain(r);
    r->low_after_pause=1;
    r->phase=9;if(!restore_gain(r))return retain(r);
    if(close(r->pcm)){r->close_bad=1;return retain(r);}r->pcm=-1;r->closed=1;
    r->low_after_close=pins(r);if(!r->low_after_close)return retain(r);
    if(!gain_closed(r,0)||!restore_mux(r)||!gain_closed(r,1))return retain(r);
    alarm(0);r->phase=10;
    if(release_views(r)){record(r,"CLEANUP_FAILED");return 1;}
    record(r,cancelled?"CANCELLED_RESTORED":"TONE_COMPLETED_AWAIT_USER");return cancelled?1:0;
unopened:
    if(r->enters&&!restore_mux(r))return retain(r);
    alarm(0);release_views(r);record(r,"REFUSED_BEFORE_PCM_OPEN");return 2;
}
static int status(void)
{
    struct run r={.pcm=-1,.mem=-1};struct stat s;
    if(root_runtime()||views(&r,0)){release_views(&r);return 2;}
    uint32_t rate=dac_rate(r.sys[2]),sys74=r.sys[0x74/4],sys78=r.sys[0x78/4],dir=r.gpio[0],out=r.gpio[3];
    int gain=hp_decode(hp_raw(&r)),present=!lstat(AMBIENT,&s)&&S_ISREG(s.st_mode);
    if(release_views(&r)||!rate||gain<0)return 2;
    printf("{\"record\":\"speaker-status-v8\",\"state\":\"STATUS\",\"calculated_DAC_rate\":%u,\"frequency_max_hz\":%u,\"HP_gain_code\":%d,\"SYS74\":%u,\"SYS78\":%u,\"DIR0\":%u,\"OUT0\":%u,\"ambient_path_present\":%s}\n",rate,(rate-1)/2,gain,sys74,sys78,dir,out,present?"true":"false");return 0;
}
int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--capabilities")){
        puts("{\"record\":\"speaker-capabilities-v8\",\"state\":\"CAPABILITIES\",\"frequency_min_hz\":1,\"frequency_max_hz\":null,\"frequency_limit_rule\":\"floor((calculated_DAC_rate-1)/2)\",\"gain_min\":0,\"gain_max\":5,\"volume_min\":0,\"volume_max\":100,\"duration_min_ms\":0,\"duration_max_ms\":5000,\"generator\":\"camera_software_triangle_hardware_DAC\",\"positive_tone_requires_uart_isolated\":true,\"physical_isolation_verified_by_helper\":false,\"ambient_one_shot\":false}");return 0;}
    if(argc<2||argv[0][0]!='/')return 2;
    if(argc==2&&!strcmp(argv[1],"--status"))return status();
    struct run r={.pcm=-1,.mem=-1};
    if((argc!=6&&argc!=7)||(argc==7&&strcmp(argv[6],"--uart-isolated"))||strcmp(argv[1],"--tone")||decimal(argv[2],&r.frequency)||!r.frequency||
       decimal(argv[3],&r.test_gain)||r.test_gain>5||decimal(argv[4],&r.volume)||r.volume>100||
       decimal(argv[5],&r.duration)||r.duration>5000||(r.duration&&argc!=7)){puts("{\"record\":\"stock-speaker-route-v8\",\"state\":\"INVALID_REQUEST\",\"effects\":0}");return 2;}
    r.isolation_asserted=argc==7;
    r.peak=(32767U*r.volume+50)/100;return tone(&r);
}
