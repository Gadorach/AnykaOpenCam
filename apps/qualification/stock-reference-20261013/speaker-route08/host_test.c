/* Actual CLI under POSIX/MMIO mocks plus a raw period/DMA ring model.
 * Host never opens a device or receives target audio. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

enum { BAD_ROOT=1,BAD_RELEASE,FD3_LIVE,META_BAD,NODE_RACE,MAP_FAIL,HP_INVALID,
       MUX_BAD,OUT_BAD,DIR_BAD,NONBLOCK,PARAM_BAD,RATE_BAD,GAIN_CACHE_BAD,
       SET_ERROR_EFFECT,SET_CACHE_ONLY,RESTORE_ERROR,RESTORE_HW_BAD,
       GAIN_ACTIVE_BAD,HP_AFTER_CLOSE,HP_AFTER_EXIT,CLOSE_FAIL,STOP_FAIL,
       MUX_ENTER_FAIL,MUX_EXIT_FAIL,OUT_RESTORE_FAIL,OTHER_SYS,OTHER_OUT,OTHER_DIR,
       CANCEL_PRE,CANCEL_ACTIVE,CANCEL_CLEANUP,CLOCK_BACKWARDS,COUNTER_STALL,
       COUNTER_FRACTION,COUNTER_BACK,COUNTER_HIGH,COUNTER_JUMP,DELAY_WRITE,
       SHORT_WRITE,POSITIVE_IOCTL,CANARY_IOCTL,IRQ_TAIL_SKIP,AMBIENT_BAD,
       AMBIENT_ERROR,AMBIENT_CLOSE_FAIL,AMBIENT_MISSING,AMBIENT_NUL,UNMAP_FAIL,
        AMBIENT_ZERO,AMBIENT_MAX,AMBIENT_NEGATIVE,
        ENABLE_FAIL_BEFORE,ENABLE_FAIL_AFTER,ENABLE_HIGH,ENABLE_INPUT_HIGH,ENABLE_OUT_HIGH,
        PRE_ENABLE_LOW,RESUME_HIGH,MID_HIGH,PAUSE_HIGH,CLOSE_HIGH,OTHER_SYS78,
        CANCEL_ENABLE,CANCEL_AFTER_ENABLE,HUP_ACTIVE,PIPE_ACTIVE,RESET_TAPS,GPIO0_OUTPUT };
static unsigned fault,initial_gain,requested_gain,frequency,volume,duration,peak,tone_frames;
static uint32_t sysregs[1024],gpioregs[1024],params_cache[7],gain_cache;
static uint8_t backing[65536];static uint32_t tags[128];
static uint64_t now_us,next_irq;
static uint32_t producer,consumer,submitted,counter,dt=127808;
static unsigned pcm_opens,pcm_closes,pcm_live,mem_live,active,paused,sets,restores,restore_seen;
static unsigned enters,exits,outstores,writes,time_reads,ioctls,ro_maps,rw_maps,reg_probes;
static unsigned fdpos[3],adc_reads,adc_opens,adc_closes,adc_live,adc_stats,underflows,max_zero_end,wraps;
static unsigned failure_op,fail_occurrence,op_seen,stop_mode,tail_skip_done,tail_skip_armed;
static unsigned prepared,retained,mask_calls,write_delay_done;
static unsigned enable_attempts,enable_effects,power_transitions,mid_injected;
static sigset_t blocked,pend;static void (*handlers[64])(int);
static char output[4096];static jmp_buf escape;
static const uint32_t encoding[6]={16,8,4,2,1,0};

/* Different formulation from camera generator: quadrants in sample-phase,
 * then signed rational magnitude and integer fade. */
static int32_t expected_sample(uint32_t frame)
{
    if(frame>=tone_frames)return 0;
    uint32_t p=(uint32_t)(((uint64_t)frame*frequency)%8012),q=(4*p)/8012;
    int64_t n;
    if(q==0)n=4*(int64_t)p;
    else if(q==1)n=2*8012-4*(int64_t)p;
    else if(q==2)n=-(4*(int64_t)p-2*8012);
    else n=-(4*8012-4*(int64_t)p);
    int32_t sample=(int32_t)(n*peak/8012);
    unsigned ramp=frame<64?frame:64;if(tone_frames-1-frame<ramp)ramp=tone_frames-1-frame;
    return sample*(int32_t)ramp/64;
}
static void check_bytes(const uint8_t *b,uint32_t logical,unsigned n)
{
    for(unsigned i=0;i<n;i+=4){int32_t v=expected_sample((logical+i)/4);uint16_t u=(uint16_t)v;
        assert(b[i]==(uint8_t)u&&b[i+1]==(uint8_t)(u>>8));
        assert(b[i]==b[i+2]&&b[i+1]==b[i+3]);assert(v>=-(int32_t)peak&&v<=(int32_t)peak);}
}
static void zero_region(uint32_t physical,uint32_t logical,unsigned n)
{
    assert(physical+n<=53248&&physical+n<=sizeof(backing));
    if(physical+n>max_zero_end)max_zero_end=physical+n;
    memset(backing+physical,0,n);
    for(unsigned x=0;x<n;x+=512)tags[(physical+x)/512]=(logical+x)/512;
}
static void irq(void)
{
    uint32_t offset=consumer%49152;
    if(!underflows){
        for(unsigned b=0;b<4096;b+=512)assert(tags[(offset+b)/512]==(consumer+b)/512);
        check_bytes(backing+offset,consumer,4096);
    }
    consumer+=4096;counter+=dt;wraps=consumer/49152;
    uint32_t remain=producer>=consumer?producer-consumer:UINT32_MAX;
    if(remain<=4096){
        ++underflows;
        zero_region(consumer%49152+remain,consumer+remain,4096);
        producer=consumer+4096;
    }
    if(stop_mode==1){zero_region(consumer%49152,consumer,4096);stop_mode=2;}
    else if(stop_mode==2){active=0;paused=1;stop_mode=0;}
}
static void advance(unsigned us)
{
    now_us+=us;
    if(fault==COUNTER_STALL)return;
    while(active&&now_us>=next_irq){irq();next_irq+=dt;}
}
static void deliver(int s){assert(handlers[s]);if(sigismember(&blocked,s)==1)sigaddset(&pend,s);else handlers[s](s);}
static uid_t m_getuid(void){++reg_probes;return fault==BAD_ROOT?1000:0;}
static uid_t m_geteuid(void){return 0;}
static pid_t m_getpid(void){return 123;}
static int m_uname(struct utsname *u){memset(u,0,sizeof(*u));strcpy(u->release,fault==BAD_RELEASE?"6.18.55":"3.4.35");return 0;}
static int m_fcntl(int fd,int op,...)
{if(fd==3){assert(op==F_GETFD);if(fault==FD3_LIVE)return 0;errno=EBADF;return -1;}
 assert(fd==21&&op==F_GETFL);return O_WRONLY|(fault==NONBLOCK?O_NONBLOCK:0);}
static int m_sigaction(int s,const struct sigaction *a,struct sigaction *old)
{assert(!old&&s<64&&!a->sa_flags);handlers[s]=a->sa_handler;return 0;}
static int m_sigprocmask(int how,const sigset_t *s,sigset_t *old)
{
    if(how==SIG_BLOCK){assert(old&&!active&&!pcm_live);++mask_calls;*old=blocked;
        for(int i=1;i<64;++i)if(sigismember(s,i)==1)sigaddset(&blocked,i);
        if(fault==CANCEL_CLEANUP&&pcm_closes)deliver(SIGTERM);
        return 0;}
    assert(!old);
    if(how==SIG_SETMASK)blocked=*s;
    else {assert(how==SIG_UNBLOCK);for(int i=1;i<64;++i)if(sigismember(s,i)==1)sigdelset(&blocked,i);}
    for(int i=1;i<64;++i)if(sigismember(&pend,i)==1&&sigismember(&blocked,i)!=1){sigdelset(&pend,i);handlers[i](i);}
    return 0;
}
static int m_sigpending(sigset_t *s){*s=pend;return 0;}
static unsigned m_alarm(unsigned n){assert(n<=8);return 0;}
static int m_clock_gettime(clockid_t c,struct timespec *t)
{
    assert(c==CLOCK_MONOTONIC);advance(1000);
    if(fault==CANCEL_PRE&&sets&&!active)deliver(SIGTERM);
    if(fault==CANCEL_ACTIVE&&active)deliver(SIGTERM);
    if(fault==HUP_ACTIVE&&active)deliver(SIGHUP);
    if(fault==PIPE_ACTIVE&&active)assert(handlers[SIGPIPE]==SIG_IGN);
    if(fault==CANCEL_AFTER_ENABLE&&enable_attempts&&!active)deliver(SIGTERM);
    if(fault==RESET_TAPS)gpioregs[6]^=1U;
    uint64_t v=fault==CLOCK_BACKWARDS&&active?0:now_us;
    t->tv_sec=(time_t)(v/1000000);t->tv_nsec=(long)(v%1000000)*1000;return 0;
}
static int m_nanosleep(const struct timespec *t,struct timespec *rem)
{assert(!rem&&t->tv_sec==0&&t->tv_nsec==2000000);advance(2000);return 0;}
static int m_lstat(const char *p,struct stat *s)
{
    memset(s,0,sizeof(*s));s->st_dev=9;s->st_uid=0;
    if(!strcmp(p,"/sys/kernel/ain/ain0")){++adc_stats;if(fault==AMBIENT_MISSING){errno=ENOENT;return -1;}s->st_mode=S_IFREG|0644;s->st_ino=30;return 0;}
    s->st_mode=S_IFCHR|0600;
    if(!strcmp(p,"/dev/mem")){s->st_ino=20;s->st_rdev=makedev(1,1);}
    else {assert(!strcmp(p,"/dev/akpcm_cdev0"));s->st_ino=21;s->st_rdev=makedev(253,0);}
    return 0;
}
static int m_fstat(int fd,struct stat *s)
{
    assert(fd==20||fd==21||fd==30);
    int rc=m_lstat(fd==20?"/dev/mem":fd==21?"/dev/akpcm_cdev0":"/sys/kernel/ain/ain0",s);
    if(fault==NODE_RACE&&fd==21)++s->st_ino;
    return rc;
}
static int m_open(const char *p,int flags,...)
{
    assert((flags&O_CLOEXEC)&&(flags&O_NOFOLLOW));
    if(!strcmp(p,"/sys/class/akpcm_class0/akpcm_cdev0/dev"))return 10;
    if(!strcmp(p,"/sys/class/akpcm_class1/akpcm_cdev1/dev"))return 11;
    if(!strcmp(p,"/proc/devices"))return 12;
    if(!strcmp(p,"/sys/kernel/ain/ain0")){assert(!pcm_opens&&!mem_live&&(flags&O_ACCMODE)==O_RDONLY);++adc_opens;adc_live=1;return 30;}
    if(!strcmp(p,"/dev/mem")){assert((flags&O_ACCMODE)==O_RDONLY||(flags&O_ACCMODE)==O_RDWR);assert(!mem_live);mem_live=1;return 20;}
    assert(!strcmp(p,"/dev/akpcm_cdev0")&&(flags&O_ACCMODE)==O_WRONLY&&!(flags&O_NONBLOCK));
    assert(enters==1&&!pcm_live&&sysregs[0x74/4]==0x02000184);++pcm_opens;pcm_live=1;return 21;
}
static ssize_t m_read(int fd,void *out,size_t n)
{
    if(fd==30){assert(adc_live&&!adc_reads&&!pcm_opens&&!enters);++adc_reads;
        if(fault==AMBIENT_ERROR){errno=EINTR;return -1;}
        if(fault==AMBIENT_NUL){const char bad[7]={'1',0,'x','x','x','x','\n'};memcpy(out,bad,7);return 7;}
        const char *s=fault==AMBIENT_BAD?"3000\n":fault==AMBIENT_ZERO?"0\n":fault==AMBIENT_MAX?"2999\n":fault==AMBIENT_NEGATIVE?"-1\n":"1536\n";
        assert(n>=strlen(s));memcpy(out,s,strlen(s));return (ssize_t)strlen(s);}
    assert(fd>=10&&fd<=12);
    const char *s=fd==10?(fault==META_BAD?"252:0\n":"253:0\n"):fd==11?"253:1\n":"Character devices:\n  1 mem\n253 pcmchar\n\nBlock devices:\n";
    size_t len=strlen(s)-fdpos[fd-10];if(len>n)len=n;memcpy(out,s+fdpos[fd-10],len);fdpos[fd-10]+=(unsigned)len;return (ssize_t)len;
}
static void *m_mmap(void *a,size_t n,int prot,int flags,int fd,off_t off)
{
    assert(!a&&n==4096&&flags==MAP_SHARED&&fd==20&&mem_live&&(off==0x08000000||off==0x20170000));
    if(fault==MAP_FAIL)return MAP_FAILED;
    if(prot==PROT_READ)++ro_maps;else{assert(prot==(PROT_READ|PROT_WRITE));++rw_maps;}
    return off==0x08000000?(void*)sysregs:(void*)gpioregs;
}
static int m_munmap(void *p,size_t n)
{assert((p==sysregs||p==gpioregs)&&n==4096);return fault==UNMAP_FAIL?-1:0;}
static int m_close(int fd)
{
    if(fd>=10&&fd<=12)return 0;
    if(fd==20){assert(mem_live);mem_live=0;return 0;}
    if(fd==30){assert(adc_live);adc_live=0;++adc_closes;return fault==AMBIENT_CLOSE_FAIL?-1:0;}
    assert(fd==21&&pcm_live&&!active);if(sets)assert(restores==1&&restore_seen&&gain_cache==initial_gain&&((sysregs[0x9c/4]>>18)&31)==encoding[initial_gain]);
    /* Real channel-source stop saves INPUT, briefly writes descriptor HIGH,
     * then restores saved INPUT. Last close is NOT an implicit disable. */
    uint32_t saved=gpioregs[6]&2U;
    if(enable_effects)assert(saved==0);
    gpioregs[3]|=2U;gpioregs[6]|=2U;++power_transitions;
    gpioregs[3]=(gpioregs[3]&~2U)|saved;gpioregs[6]=(gpioregs[6]&~2U)|saved;
    sysregs[0x9c/4]&=~0x1c000U;
    pcm_live=0;++pcm_closes;
    if(fault==CLOSE_HIGH)gpioregs[3]|=2U,gpioregs[6]|=2U;
    if(fault==HP_AFTER_CLOSE)sysregs[0x9c/4]^=0x00400000;
    return fault==CLOSE_FAIL?-1:0;
}
static void m_enter(volatile uint32_t *p,uint32_t v)
{assert(p==&sysregs[0x74/4]&&v==0x02000184&&!enters&&!pcm_opens&&ro_maps==2&&rw_maps==2);
 ++enters;if(fault!=MUX_ENTER_FAIL)*p=v;gpioregs[6]=(gpioregs[6]&~2U)|(gpioregs[3]&2U);}
static void m_exit(volatile uint32_t *p,uint32_t v)
{
    assert(p==&sysregs[0x74/4]&&v==0x02000186&&enters==1&&!exits&&!active&&!pcm_live&&gpioregs[3]==2&&fault!=CLOSE_FAIL);
    ++exits;if(fault!=MUX_EXIT_FAIL)*p=v;if(fault==HP_AFTER_EXIT)sysregs[0x9c/4]^=0x00400000;
}
static void m_out(volatile uint32_t *p,uint32_t v)
{assert(p==&gpioregs[3]&&v==2&&!outstores&&pcm_closes==1&&!active&&!pcm_live&&*p==0&&fault!=CLOSE_FAIL&&enable_attempts==1);
 ++outstores;if(fault!=OUT_RESTORE_FAIL){*p=v;gpioregs[6]|=2U;}}
static void gain_set(uint32_t v)
{assert(v<6);if(gain_cache!=v){gain_cache=v;sysregs[0x9c/4]=(sysregs[0x9c/4]&~0x007c0000U)|(encoding[v]<<18);}}
static int m_ioctl(int fd,unsigned long op,...)
{
    assert(fd==21&&pcm_live);va_list ap;va_start(ap,op);uint32_t *v=va_arg(ap,uint32_t*);va_end(ap);++ioctls;advance(500);
    unsigned words=0;
    switch(op){case 0x401c50f2UL:case 0x801c50f2UL:words=7;break;case 0x804050f0UL:words=16;break;
    case 0x800850e5UL:words=2;break;case 0x80045070UL:case 0x80045010UL:case 0x80045030UL:case 0x40045030UL:case 0x800450e7UL:case 0x40045081UL:words=1;break;
    case 0x50e0UL:case 0x50e1UL:case 0x50e2UL:assert(!v);break;default:assert(!"unapproved ioctl");}
    if(words)assert(v&&v[-1]==0x937ea512U&&v[words]==0x937ea512U);
    if(op==0x40045081UL){
        assert(words==1&&((op>>16)&0x3fff)==4&&*v==1&&prepared&&!active&&!submitted&&!pcm_closes&&sets==1&&!enable_attempts);
        assert(gpioregs[3]==2&&(gpioregs[6]&2)==2&&sysregs[0x74/4]==0x02000184);
        ++enable_attempts;
        if(fault==ENABLE_FAIL_BEFORE){errno=EIO;return -1;}
        ++enable_effects;gpioregs[3]&=~2U;gpioregs[6]&=~2U;
        if(fault==ENABLE_FAIL_AFTER){errno=EIO;return -1;}
        if(fault==ENABLE_HIGH)gpioregs[3]|=2U,gpioregs[6]|=2U;
        if(fault==ENABLE_INPUT_HIGH)gpioregs[6]|=2U;
        if(fault==ENABLE_OUT_HIGH)gpioregs[3]|=2U;
        if(fault==CANCEL_ENABLE){deliver(SIGTERM);errno=EINTR;return -1;}
    }
    if(op==0x40045030UL){assert(!active&&!pcm_closes&&prepared);
        if(!sets){assert(*v==requested_gain);++sets;if(fault==SET_CACHE_ONLY)gain_cache=*v;else gain_set(*v);
             if(fault==PRE_ENABLE_LOW){gpioregs[3]&=~2U;gpioregs[6]&=~2U;}
             if(fault==SET_ERROR_EFFECT){errno=EIO;return -1;}}
        else{assert(!restores&&*v==initial_gain);++restores;gain_set(*v);
            if(fault==RESTORE_HW_BAD)sysregs[0x9c/4]^=0x00400000;
            if(fault==RESTORE_ERROR){errno=EIO;return -1;}}}
    if(op==failure_op&&++op_seen==fail_occurrence){
        if(fault==CANARY_IOCTL&&words){v[words]=0;return 0;}if(fault==POSITIVE_IOCTL)return 1;errno=EIO;return -1;}
    switch(op){
    case 0x80045070UL:*v=1;break;case 0x80045010UL:*v=4;break;
    case 0x80045030UL:*v=fault==GAIN_CACHE_BAD?6:gain_cache;if(restores&&*v==initial_gain&&((sysregs[0x9c/4]>>18)&31)==encoding[initial_gain])restore_seen=1;break;
    case 0x40045030UL:case 0x40045081UL:break;
    case 0x800450e7UL:*v=paused?0:submitted>=4096?1:0;break;
    case 0x401c50f2UL:{const uint32_t want[7]={8000,2,16,4096,12,4096,0};assert(!memcmp(v,want,28));memcpy(params_cache,v,28);break;}
    case 0x801c50f2UL:memcpy(v,params_cache,28);if(fault==PARAM_BAD)v[6]=99;break;
    case 0x50e0UL:prepared=1;break;
    case 0x804050f0UL:memset(v,0,64);v[1]=fault==RATE_BAD?0:8012;break;
    case 0x50e1UL:
        assert(submitted==16384&&writes==32&&sets==1&&gain_cache==requested_gain&&((sysregs[0x9c/4]>>18)&31)==encoding[requested_gain]);
        assert(enable_attempts==1&&enable_effects==1&&gpioregs[3]==0&&!(gpioregs[6]&2U));
        /* Synchronous HP power path HIGH transient then restored saved LOW. */
        gpioregs[3]|=2U;gpioregs[6]|=2U;++power_transitions;
        gpioregs[3]&=~2U;gpioregs[6]&=~2U;
        sysregs[0x9c/4]=(sysregs[0x9c/4]&~0x1c000U)|(4U<<14);
        active=1;next_irq=now_us+dt;
        if(fault==RESUME_HIGH)gpioregs[3]|=2U,gpioregs[6]|=2U;
        if(fault==GAIN_ACTIVE_BAD)sysregs[0x9c/4]^=0x00400000;
        if(fault==OTHER_SYS)sysregs[0x74/4]^=0x100;
        if(fault==OTHER_OUT)gpioregs[3]|=4;
        if(fault==OTHER_DIR)gpioregs[0]^=4;
        if(fault==OTHER_SYS78)sysregs[0x78/4]^=1;
        break;
    case 0x800850e5UL:
        if(active&&fault==MID_HIGH&&!mid_injected&&counter>=dt){++mid_injected;gpioregs[3]|=2U;gpioregs[6]|=2U;}
        if(active&&fault==IRQ_TAIL_SKIP&&!tail_skip_done&&consumer==8*4096&&submitted==12*4096){
            /* Let the just-finished write's conservative old-prefix fence pass;
             * inject on the following loop poll, whose prefix is the full12P. */
            if(!tail_skip_armed)tail_skip_armed=1;
            else {tail_skip_done=1;advance(2*dt);}}
        *v=counter;v[1]=0;
        if(active){++time_reads;
            if(fault==COUNTER_FRACTION)*v=1;
            if(fault==COUNTER_HIGH)v[1]=1;
            if(fault==COUNTER_JUMP)*v=100*dt;
            if(fault==COUNTER_BACK&&counter>=2*dt)*v=0;}
        break;
    case 0x50e2UL:
        if(active){if(fault==STOP_FAIL){errno=EIO;return -1;}
             stop_mode=1;while(active)advance(dt);assert(paused);}
        if(fault==PAUSE_HIGH&&enable_attempts)gpioregs[3]|=2U,gpioregs[6]|=2U;
        break;
    default:assert(0);
    }
    return 0;
}
static ssize_t m_write(int fd,const void *b,size_t n)
{
    assert(fd==21&&pcm_live&&prepared&&n==512&&sets==1&&!restores&&enable_attempts==1&&enable_effects==1&&gpioregs[3]==0&&!(gpioregs[6]&2U));check_bytes(b,submitted,512);++writes;
    if(fault==SHORT_WRITE&&writes==3)return 511;
    assert(producer>=consumer&&producer-consumer<=49152&&49152-(producer-consumer)>=512+8192);
    uint32_t saved=producer,offset=saved%49152;assert(offset+512<=49152);
    if(fault==DELAY_WRITE&&active&&!write_delay_done){write_delay_done=1;advance(6*dt);}else advance(2000);
    memcpy(backing+offset,b,512);tags[offset/512]=saved/512;producer=saved+512;submitted+=512;return 512;
}
static int m_printf(const char *fmt,...)
{assert(!output[0]);va_list ap;va_start(ap,fmt);int n=vsnprintf(output,sizeof(output),fmt,ap);va_end(ap);assert(n>0&&(size_t)n<sizeof(output));return n;}
static int m_puts(const char *s){assert(!output[0]&&strlen(s)+2<sizeof(output));strcpy(output,s);strcat(output,"\n");return 0;}
static int m_fflush(FILE *f){assert(f==stdout);return 0;}
static int m_kill(pid_t p,int sig){assert(p==123&&sig==SIGSTOP);++retained;longjmp(escape,125);}

#define getuid m_getuid
#define geteuid m_geteuid
#define getpid m_getpid
#define uname m_uname
#define fcntl m_fcntl
#define sigaction(...) m_sigaction(__VA_ARGS__)
#define sigprocmask m_sigprocmask
#define sigpending m_sigpending
#define alarm m_alarm
#define clock_gettime m_clock_gettime
#define nanosleep m_nanosleep
#define lstat m_lstat
#define fstat m_fstat
#define open m_open
#define read m_read
#define close m_close
#define mmap m_mmap
#define munmap m_munmap
#define ioctl m_ioctl
#define write m_write
#define printf m_printf
#define puts m_puts
#define fflush m_fflush
#define kill m_kill
#define CONSOLE_MUX_ENTER m_enter
#define CONSOLE_MUX_EXIT m_exit
#define CONSOLE_OUT_STORE m_out
#define main console_main
#include "console.c"
#undef main
#undef printf
#undef puts

static unsigned cases;
static int run_case(int argc,char **argv,unsigned f,unsigned original,unsigned op,unsigned occurrence)
{
    fault=f;initial_gain=original;failure_op=op;fail_occurrence=occurrence;op_seen=0;
    pcm_opens=pcm_closes=pcm_live=mem_live=active=paused=sets=restores=restore_seen=0;
    enters=exits=outstores=writes=time_reads=ioctls=ro_maps=rw_maps=reg_probes=0;
    adc_reads=adc_opens=adc_closes=adc_live=adc_stats=underflows=max_zero_end=wraps=0;
    prepared=retained=mask_calls=write_delay_done=stop_mode=tail_skip_done=tail_skip_armed=0;
    enable_attempts=enable_effects=power_transitions=mid_injected=0;
    producer=consumer=submitted=counter=0;now_us=1000;next_irq=0;cancelled=0;
    memset(sysregs,0,sizeof(sysregs));memset(gpioregs,0,sizeof(gpioregs));memset(fdpos,0,sizeof(fdpos));
    memset(params_cache,0,sizeof(params_cache));memset(backing,0xa5,sizeof(backing));memset(tags,0xff,sizeof(tags));
    memset(handlers,0,sizeof(handlers));sigemptyset(&blocked);sigemptyset(&pend);output[0]=0;
    gain_cache=original;sysregs[2]=0x13c8;sysregs[0x74/4]=f==MUX_BAD?0x02000184:0x02000186;sysregs[0x78/4]=0x50;
    sysregs[0x9c/4]=0xfe020008|((f==HP_INVALID?31:encoding[original])<<18);
    gpioregs[0]=f==DIR_BAD?0:0x008000e6;gpioregs[3]=f==OUT_BAD?6:2;gpioregs[6]=0xe0000011;
    if(f==GPIO0_OUTPUT)gpioregs[0]|=1;
    /* Sentinel pull/bank registers are never written by the candidate. */
    for(unsigned i=7;i<1024;++i)gpioregs[i]=0xa5a50000U+i;
    if(argc==6||argc==7){frequency=(unsigned)strtoul(argv[2],0,10);requested_gain=(unsigned)strtoul(argv[3],0,10);volume=(unsigned)strtoul(argv[4],0,10);duration=(unsigned)strtoul(argv[5],0,10);}
    else frequency=requested_gain=volume=duration=0;
    peak=(32767U*volume+50)/100;tone_frames=(uint32_t)(((uint64_t)8012*duration+500)/1000);
    int result=setjmp(escape);if(!result)result=console_main(argc,argv);
    assert(enters<=1&&exits<=1&&outstores<=1&&sets<=1&&restores<=1&&enable_attempts<=1&&!adc_reads&&!adc_opens&&strlen(output)<4000);
    assert((sysregs[0x78/4]==0x50||f==OTHER_SYS78)&&((sysregs[0x74/4]&4)==4));
    for(unsigned i=7;i<1024;++i)assert(gpioregs[i]==0xa5a50000U+i);
    assert(!(gpioregs[3]&1U));
    if(active)assert(!restores&&!pcm_closes&&!exits&&!outstores);
    if(result==0||result==1||result==2){assert(!active&&!pcm_live);
        if(enters)assert(exits==1&&sysregs[0x74/4]==0x02000186&&gpioregs[3]==2);
        if(sets)assert(restores==1&&restore_seen&&gain_cache==original&&((sysregs[0x9c/4]>>18)&31)==encoding[original]);}
    if(result==0&&argc==7&&duration){assert(consumer>=((tone_frames+1023)/1024)*4096&&submitted>tone_frames*4);
        assert(enable_attempts==1&&outstores==1&&power_transitions==2);
        assert(strstr(output,"TONE_COMPLETED_AWAIT_USER")&&strstr(output,"\"gain_restored\":true")&&
               strstr(output,"\"speaker_enabled_low\":true")&&strstr(output,"\"speaker_low_after_close\":true")&&
               strstr(output,"\"mux_restore_after_OUT_restore\":true"));}
    if(retained)assert(result==125);
    ++cases;return result;
}
static int tone_case(const char *freq,const char *gain,const char *vol,const char *ms,unsigned f,unsigned original,unsigned op,unsigned n)
{char *a[]={"/fixed/speaker-route08","--tone",(char*)freq,(char*)gain,(char*)vol,(char*)ms,"--uart-isolated",0};return run_case(7,a,f,original,op,n);}
int main(void)
{
    char capabilities_example[4096],status_example[4096],first_example[4096];
    char *cap[]={"console","--capabilities",0};assert(run_case(2,cap,0,1,0,0)==0&&!reg_probes&&!pcm_opens&&!ro_maps&&!adc_reads);
    strcpy(capabilities_example,output);
    char *stat[]={"/fixed/console","--status",0};
    for(unsigned original=0;original<6;++original){assert(run_case(2,stat,0,original,0,0)==0&&!pcm_opens&&!adc_reads&&ro_maps==2&&!rw_maps);}
    strcpy(status_example,output);
    assert(run_case(2,stat,AMBIENT_MISSING,1,0,0)==0&&strstr(output,"\"ambient_path_present\":false"));
    char *amb[]={"/fixed/speaker-route08","--ambient",0};
    assert(run_case(2,amb,0,1,0,0)==2&&!reg_probes&&!adc_reads);
    for(unsigned guard=BAD_ROOT;guard<=FD3_LIVE;++guard){
        assert(tone_case("1000","1","25","224",guard,1,0,0)==2);
        assert(!pcm_opens&&!mem_live&&!ro_maps&&!rw_maps&&!ioctls&&!enters&&!writes&&!enable_attempts);
    }
    assert(tone_case("1000","1","25","224",0,1,0,0)==0);strcpy(first_example,output);
    char g[2]={0};
    for(unsigned original=0;original<6;++original)for(unsigned gain=0;gain<6;++gain){g[0]=(char)('0'+gain);
        assert(tone_case("1000",g,"25","250",0,original,0,0)==0);}
    char long_example[4096],noop_example[4096];unsigned total_wraps=0;
    const char *freqs[]={"1","4005"},*volumes[]={"0","25","100"},*durations[]={"1","5000"};
    for(unsigned f=0;f<2;++f)for(unsigned v=0;v<3;++v)for(unsigned d=0;d<2;++d){
        assert(tone_case(freqs[f],f?"5":"0",volumes[v],durations[d],0,f?0:5,0,0)==0);
        if(d){assert(wraps>=3);total_wraps+=wraps;strcpy(long_example,output);}}
    assert(tone_case("4005","5","100","0",0,0,0,0)==0&&!pcm_opens&&!enters&&!writes);strcpy(noop_example,output);
    assert(tone_case("1","0","0","0",0,5,0,0)==0&&!pcm_opens&&!enters&&!writes);
    const char *invalid[][4]={{"0","3","25","250"},{"-1","3","25","250"},{"+1","3","25","250"},
        {"1e3","3","25","250"},{"1000","6","25","250"},{"1000","-1","25","250"},
        {"1000","3","101","250"},{"1000","3","25","5001"},{"1000","3","25","1.5"},
        {"4294967296","3","25","250"},{"1000","3","25 ","250"},{"","3","25","250"}};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i){assert(tone_case(invalid[i][0],invalid[i][1],invalid[i][2],invalid[i][3],0,1,0,0)==2&&!ro_maps&&!pcm_opens);}
    assert(tone_case("4006","3","25","250",0,1,0,0)==2&&!pcm_opens&&!rw_maps&&!enters);
    for(unsigned f=BAD_ROOT;f<=CANARY_IOCTL;++f){
        if(f==POSITIVE_IOCTL||f==CANARY_IOCTL)continue;
        assert(tone_case("1000","3","25","5000",f,1,0,0)!=0);
    }
    const unsigned ops[]={DEVICE_GET,SOURCE_GET,GAIN_GET,PARAM_SET,PARAM_GET,PREPARE,FEATURES,PLAY_TIME,READY,GAIN_SET,SPEAKER_ENABLE,RESUME,PAUSE};
    for(unsigned i=0;i<sizeof(ops)/sizeof(ops[0]);++i)for(unsigned kind=0;kind<3;++kind)
        assert(tone_case("1000","3","25","250",kind==1?POSITIVE_IOCTL:kind==2?CANARY_IOCTL:0,1,ops[i],1)!=0);
    for(unsigned n=2;n<=5;++n)assert(tone_case("1000","3","25","250",0,1,GAIN_GET,n)!=0);
    assert(tone_case("1000","3","25","250",0,1,GAIN_SET,2)==125&&!pcm_closes);
    assert(tone_case("1000","3","25","250",0,1,PAUSE,2)==125&&!restores);
    for(unsigned f=ENABLE_FAIL_BEFORE;f<=GPIO0_OUTPUT;++f){
        int rc=tone_case("1000","1","25","224",f,1,0,0);
        if(f==PIPE_ACTIVE||f==RESET_TAPS)assert(rc==0);
        else assert(rc!=0);
        if(f>=ENABLE_FAIL_BEFORE&&f<=ENABLE_OUT_HIGH)assert(rc==125&&!pcm_closes&&!restores&&!exits&&!outstores);
        if(f==CANCEL_AFTER_ENABLE)assert(rc==1&&pcm_closes&&outstores&&exits);
        if(f==CLOSE_HIGH)assert(rc==125&&pcm_closes&&!outstores&&!exits);
    }
    /* Positive requests without exact explicit external isolation token never
     * reach even runtime checks. Token is an assertion, not physical proof. */
    char *cli[]={"/fixed/speaker-route08","--tone","1000","1","25","224",0,0,0};
    assert(run_case(6,cli,0,1,0,0)==2&&!reg_probes&&!ioctls&&!enters);
    cli[6]="--uart-isolated-NO";assert(run_case(7,cli,0,1,0,0)==2&&!reg_probes);
    cli[6]="--uart-isolated";cli[7]="extra";assert(run_case(8,cli,0,1,0,0)==2&&!reg_probes);
    cli[7]=0;cli[5]="0";cli[6]=0;
    assert(run_case(6,cli,0,1,0,0)==0&&!enable_attempts&&!ioctls&&!rw_maps&&!enters);
    cli[5]="224";cli[6]="--uart-isolated";cli[0]="relative";
    assert(run_case(7,cli,0,1,0,0)==2&&!reg_probes);
    char *hidden[]={"/fixed/speaker-route08","--gpio","1","0",0};
    assert(run_case(4,hidden,0,1,0,0)==2&&!reg_probes&&!enters&&!enable_attempts);
    char *address[]={"/fixed/speaker-route08","--tone","0x08000074","1","25","224","--uart-isolated",0};
    assert(run_case(7,address,0,1,0,0)==2&&!reg_probes&&!enters&&!enable_attempts);
    char *old[]={"/fixed/speaker-route08","--native-stock-speaker-tone-gain3-uart-isolated",0};
    assert(run_case(2,old,0,1,0,0)==2&&!reg_probes&&!enable_attempts);
    /* Every frequency, volume, duration and gain in the declared domains is
     * CLI-accepted. Exhaustive frequencies use zero effects; exhaustive positive
     * durations stop at a mocked root refusal (not device operations). */
    char number[16];
    for(unsigned f=1;f<=4005;++f){snprintf(number,sizeof(number),"%u",f);
        assert(tone_case(number,"0","0","0",0,0,0,0)==0&&!enable_attempts&&!ioctls&&!rw_maps);}
    for(unsigned d=1;d<=5000;++d){snprintf(number,sizeof(number),"%u",d);
        assert(tone_case("1000","1","25",number,BAD_ROOT,1,0,0)==2&&reg_probes==1&&!ro_maps);}
    for(unsigned v=0;v<=100;++v){snprintf(number,sizeof(number),"%u",v);
        assert(tone_case("1000","1",number,"224",0,1,0,0)==0);}
    /* IRQ skip before final stop reaches the retail logical-end spill; prove
     * owned64KiB backing suffices without enlarging49KiB ring or waveform. */
    assert(tone_case("1000","3","25","1250",IRQ_TAIL_SKIP,1,0,0)==0&&tail_skip_done);
    assert(max_zero_end==53248);
    /* Steady rational triangle over a complete sampled cycle is balanced;
     * arbitrary requested partial cycles with fades are NOT claimed zero-DC. */
    for(unsigned v=0;v<=100;v+=25){int64_t sum=0;for(unsigned p=0;p<8012;++p)sum+=triangle(p,(32767*v+50)/100,8012);assert(sum==0);}
    ++cases;
    printf("{\"host_cases\":%u,\"real_CLI_POSIX_MMIO\":true,\"five_second_wraps_total\":%u,\"logical_ring_bytes\":49152,\"backing_bytes\":65536,\"max_ISR_zero_end\":%u,\"tone_scratch_bytes\":512,\"native_executed\":false,\"examples\":{\"long_tone\":%s,\"noop\":%s,\"capabilities\":%s,\"status\":%s,\"first224ms\":%s}}\n",cases,total_wraps,max_zero_end,long_example,noop_example,capabilities_example,status_example,first_example);
    return 0;
}
