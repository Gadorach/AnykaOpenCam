/* Actual tone.c under POSIX-call mocks. No host device/PCM/register operation. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
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

enum { BAD_UID=1,BAD_KERNEL,FD3_OPEN,META_BAD,META_ERROR,NODE_BAD,MEM_RACE,
       MAP_FAIL,MUX_BAD,DIR_BAD,LATCH_BAD,INPUT_BAD,PLL_BAD,PCM_OPEN_FAIL,PCM_RACE,
       NONBLOCK,GAIN_BAD,SOURCE_BAD,DEVICE_BAD,READY_OLD,PARAM_BAD,RATE_BAD,
       SETUP_MUX,START_MUX,STOP_MUX,CLOSE_MUX,SHORT_WRITE,ERROR_WRITE,ZERO_WRITE,
       DRAIN_STALL,COUNTER_REVERSE,STOP_FAIL,GAIN_CHANGED,READY_AFTER_STOP,
       CANCEL_SETUP,CANCEL_WRITE,CANCEL_ACTIVE,CANCEL_STOP,CLOCK_BACKWARDS,
       CLOSE_FAIL,UNMAP_FAIL,INPUT_DURING,ALARM_ACTIVE,SIGINT_ACTIVE,BROKEN_PIPE,
       INPUT_HIGH,OTHER_OUT_ENTRY,OTHER_OUT_SETUP,OTHER_OUT_ACTIVE,OTHER_OUT_STOP,OTHER_OUT_CLOSE,
       DIR_SETUP,DIR_ACTIVE,DIR_CLOSE,MUX_OTHER_SETUP,MUX_OTHER_CLOSE,EARLY_LATCH_ZERO,
       RESTORE_FAIL,RESTORE_OTHER_OUT,RESTORE_DIR,RESTORE_MUX,
       CANCEL_CLOSE,CANCEL_RESTORE_PENDING,CANCEL_AFTER_STORE,CLOSE_DEADLINE,
       MASK_BLOCK_FAIL,MASK_RESTORE_FAIL,PENDING_FAIL,RESUME_SIDE_EFFECT_ERROR,
       CLOCK_FAIL,META_CLOSE_FAIL,PCM_FLAGS_FAIL,SIGNAL_SETUP_FAIL,
       COMMIT_OTHER_OUT,COMMIT_DIR,COMMIT_MUX,
       ENTER_STORE_FAIL,ENTER_OTHER_BITS,ENTER_GPIO2_CHANGE,ENTER_DIR_CHANGE,
       ENTER_PENDING_CANCEL,CANCEL_AFTER_ENTER,ENTER_PENDING_FAIL,ENTER_BLOCK_FAIL,
       ENTER_UNMASK_FAIL,ENTER_COMMIT_OTHER_OUT,EXIT_STORE_FAIL,EXIT_OTHER_BITS,
       EXIT_DIR_CHANGE,CANCEL_AFTER_EXIT,RW_SYS_MAP_FAIL,RW_GPIO_MAP_FAIL,
       CANCEL_BEFORE_ENTER,UNOPENED_RESTORE_TIMEOUT,HUP_ACTIVE,
       HP_INITIAL_BAD,HP_PREP_BAD,SET3_NEG_AFTER,SET3_POS_AFTER,SET3_CANARY_AFTER,
       SET3_CACHE_ONLY,SET3_HW_ONLY,SET3_NO_EFFECT,HP_RESUME_BAD,CACHE_RESUME_BAD,
       HP_PAUSE_BAD,RESTORE_NEG_AFTER,RESTORE_POS_AFTER,RESTORE_CANARY_AFTER,
       RESTORE_CACHE_ONLY,RESTORE_HW_ONLY,RESTORE_NO_EFFECT,HP_CLOSE_BAD,HP_FINAL_BAD,
       CANCEL_SET3,CANCEL_GAIN_RESTORE,GAIN_RESTORE_EXPIRED,GAIN_RESTORE_PIN_BAD };
static int fault,fail_io,io_failure_kind,write_fault_at;
static unsigned opens,writes,closes,io_count,stops,retained,meta_pos[3],now_ms,clock_count;
static int prepared,active,paused,pcm_open,mem_open,resume_attempts;
static uint32_t registers_sys[1024],registers_gpio[1024],cached[7];
static uint8_t ring[49152];
static unsigned queued,time_reads;
static unsigned mmio_count,mask_blocks,mask_restores;
static unsigned mux_enters,mux_exits,sys_maps,gpio_maps;
static uint32_t gain_cache;
static unsigned gain3_sets,gain1_sets,gain3_reads,gain1_restore_reads;
static sigset_t blocked_signals,pending_signals;
static char output[3072];
static void (*handlers[64])(int);
static jmp_buf escape;

static uid_t m_getuid(void){return fault==BAD_UID?1000:0;}
static uid_t m_geteuid(void){return 0;}
static pid_t m_getpid(void){return 123;}
static int m_uname(struct utsname *u){memset(u,0,sizeof(*u));strcpy(u->release,fault==BAD_KERNEL?"6.18.55":"3.4.35");return 0;}
static int m_fcntl(int fd,int cmd,...)
{
    if(fd==3){assert(cmd==F_GETFD);if(fault==FD3_OPEN)return 0;errno=EBADF;return -1;}
    assert(fd==21&&cmd==F_GETFL);
    if(fault==PCM_FLAGS_FAIL){errno=EIO;return -1;}
    return O_WRONLY|(fault==NONBLOCK?O_NONBLOCK:0);
}
static int m_sigaction(int s,const struct sigaction *a,struct sigaction *old)
{assert(!old&&a&&s<64&&!a->sa_flags);if(fault==SIGNAL_SETUP_FAIL)return -1;handlers[s]=a->sa_handler;return 0;}
static void signal_now(int s)
{
    assert(handlers[s]);
    if(sigismember(&blocked_signals,s)==1)sigaddset(&pending_signals,s);
    else handlers[s](s);
}
static int m_sigprocmask(int how,const sigset_t *s,sigset_t *old)
{
    assert(s);
    if(how==SIG_BLOCK){
        assert(old&&!pcm_open&&!active);++mask_blocks;assert(mask_blocks<=2);
        if(mask_blocks==1)assert(!opens&&!closes&&!mux_enters);
        else assert(mux_enters==1&&(!opens||closes==1));
        if((fault==MASK_BLOCK_FAIL&&mask_blocks==2)||(fault==ENTER_BLOCK_FAIL&&mask_blocks==1))return -1;
        *old=blocked_signals;
        for(int i=1;i<64;++i)if(sigismember(s,i)==1)sigaddset(&blocked_signals,i);
        if((fault==CANCEL_RESTORE_PENDING&&mask_blocks==2)||(fault==ENTER_PENDING_CANCEL&&mask_blocks==1))signal_now(SIGTERM);
        return 0;
    }
    assert(!old);
    if(how==SIG_SETMASK){
        ++mask_restores;
        if((fault==MASK_RESTORE_FAIL&&mask_restores==2)||(fault==ENTER_UNMASK_FAIL&&mask_restores==1))return -1;
        blocked_signals=*s;
    } else {
        assert(how==SIG_UNBLOCK);
        for(int i=1;i<64;++i)if(sigismember(s,i)==1)sigdelset(&blocked_signals,i);
    }
    for(int i=1;i<64;++i)if(sigismember(&pending_signals,i)==1&&sigismember(&blocked_signals,i)!=1){
        sigdelset(&pending_signals,i);handlers[i](i);
    }
    return 0;
}
static int m_sigpending(sigset_t *s)
{
    assert(!active&&(mask_blocks==1||mask_blocks==2));
    if((fault==PENDING_FAIL&&mask_blocks==2)||(fault==ENTER_PENDING_FAIL&&mask_blocks==1))return -1;
    *s=pending_signals;
    if((fault==COMMIT_OTHER_OUT&&mask_blocks==2)||(fault==ENTER_COMMIT_OTHER_OUT&&mask_blocks==1))registers_gpio[3]|=4;
    if(fault==COMMIT_DIR&&mask_blocks==2)registers_gpio[0]^=4;
    if(fault==COMMIT_MUX&&mask_blocks==2)registers_sys[0x74/4]^=0x100;
    return 0;
}
static unsigned m_alarm(unsigned n){assert(n==0||n==2);return 0;}
static int m_clock_gettime(clockid_t c,struct timespec *t)
{
    assert(c==CLOCK_MONOTONIC);++clock_count;now_ms+=5;
    if(fault==CLOCK_FAIL)return -1;
    if(fault==GAIN_RESTORE_EXPIRED&&gain3_sets&&!gain1_sets&&!active)now_ms=2500;
    if(fault==CANCEL_BEFORE_ENTER&&sys_maps==2&&!mux_enters)signal_now(SIGTERM);
    if(fault==CANCEL_SETUP && prepared && !active)signal_now(SIGTERM);
    if(fault==CANCEL_WRITE && queued>=512 && !active)signal_now(SIGTERM);
    if((fault==CANCEL_ACTIVE||fault==ALARM_ACTIVE||fault==SIGINT_ACTIVE)&&active)
        signal_now(fault==ALARM_ACTIVE?SIGALRM:fault==SIGINT_ACTIVE?SIGINT:SIGTERM);
    if(fault==HUP_ACTIVE&&active)signal_now(SIGHUP);
    if(fault==CLOCK_BACKWARDS && active){t->tv_sec=0;t->tv_nsec=0;return 0;}
    t->tv_sec=now_ms/1000;t->tv_nsec=(long)(now_ms%1000)*1000000;return 0;
}
static int m_nanosleep(const struct timespec *t,struct timespec *rem)
{assert(!rem&&t->tv_sec==0&&t->tv_nsec==2000000);now_ms+=2;return 0;}
static int m_lstat(const char *p,struct stat *s)
{
    memset(s,0,sizeof(*s));s->st_mode=S_IFCHR|0600;s->st_dev=9;
    if(!strcmp(p,"/dev/mem")){s->st_ino=1;s->st_rdev=makedev(1,1);}
    else {assert(!strcmp(p,"/dev/akpcm_cdev0"));s->st_ino=2;s->st_rdev=makedev(253,fault==NODE_BAD?1:0);}
    return 0;
}
static int m_fstat(int fd,struct stat *s)
{
    assert(fd==20||fd==21);m_lstat(fd==20?"/dev/mem":"/dev/akpcm_cdev0",s);
    if((fault==MEM_RACE&&fd==20)||(fault==PCM_RACE&&fd==21))++s->st_ino;
    return 0;
}
static int m_open(const char *p,int flags,...)
{
    assert((flags&O_CLOEXEC)&&(flags&O_NOFOLLOW));
    if(!strcmp(p,"/sys/class/akpcm_class0/akpcm_cdev0/dev")){assert((flags&O_ACCMODE)==O_RDONLY);return 10;}
    if(!strcmp(p,"/sys/class/akpcm_class1/akpcm_cdev1/dev")){assert((flags&O_ACCMODE)==O_RDONLY);return 11;}
    if(!strcmp(p,"/proc/devices")){assert((flags&O_ACCMODE)==O_RDONLY);return 12;}
    if(!strcmp(p,"/dev/mem")){assert((flags&O_ACCMODE)==O_RDWR&&(flags&O_SYNC));mem_open=1;return 20;}
    assert(!strcmp(p,"/dev/akpcm_cdev0")&&(flags&O_ACCMODE)==O_WRONLY&&!(flags&O_NONBLOCK));
    assert(mux_enters==1&&registers_sys[0x74/4]==0x02000184);
    if(fault==UNOPENED_RESTORE_TIMEOUT){now_ms=2500;errno=EBUSY;return -1;}
    if(fault==PCM_OPEN_FAIL){errno=EBUSY;return -1;}
    assert(!pcm_open);pcm_open=1;++opens;return 21;
}
static ssize_t m_read(int fd,void *p,size_t n)
{
    assert(fd>=10&&fd<=12&&n<=2047);
    if(fault==META_ERROR){errno=EIO;return -1;}
    const char *s=fd==10?(fault==META_BAD?"252:0\n":"253:0\n"):
        fd==11?"253:1\n":"Character devices:\n  1 mem\n253 pcmchar\n\nBlock devices:\n 31 mtdblock\n";
    size_t len=strlen(s)-meta_pos[fd-10];if(len>n)len=n;
    memcpy(p,s+meta_pos[fd-10],len);meta_pos[fd-10]+=(unsigned)len;return (ssize_t)len;
}
static void *m_mmap(void *p,size_t len,int prot,int flags,int fd,off_t off)
{
    assert(!p&&len==4096&&flags==MAP_SHARED&&fd==20);
    if(fault==MAP_FAIL){errno=EIO;return MAP_FAILED;}
    assert(off==0x08000000||off==0x20170000);
    unsigned n=off==0x08000000?++sys_maps:++gpio_maps;
    assert(n<=2&&prot==(n==1?PROT_READ:PROT_READ|PROT_WRITE));
    if(n==2&&((off==0x08000000&&fault==RW_SYS_MAP_FAIL)||(off==0x20170000&&fault==RW_GPIO_MAP_FAIL))){errno=EIO;return MAP_FAILED;}
    return off==0x08000000?(void*)registers_sys:(void*)registers_gpio;
}
static int m_munmap(void *p,size_t len)
{assert((p==registers_sys||p==registers_gpio)&&len==4096);return fault==UNMAP_FAIL?-1:0;}
static int m_close(int fd)
{
    if(fd>=10&&fd<=12)return fault==META_CLOSE_FAIL?-1:0;
    if(fd==20){assert(mem_open);mem_open=0;return 0;}
    assert(fd==21&&pcm_open&&!active);++closes;pcm_open=0;
    if(gain3_sets)assert(gain1_sets==1&&gain1_restore_reads&&gain_cache==1&&
                        ((registers_sys[0x9c/4]>>18)&31)==8);
    /* Last stock close samples INPUT and restores it to OUT, even preactive. */
    registers_gpio[3]=(registers_gpio[3]&~2U)|(registers_gpio[6]&2U);
    if(fault==CLOSE_MUX)registers_sys[0x74/4]^=2U;
    if(fault==MUX_OTHER_CLOSE)registers_sys[0x74/4]^=0x100;
    if(fault==DIR_CLOSE)registers_gpio[0]^=4;
    if(fault==OTHER_OUT_CLOSE)registers_gpio[3]|=4;
    if(fault==CANCEL_CLOSE)signal_now(SIGTERM);
    if(fault==CLOSE_DEADLINE)now_ms=2500;
    if(fault==HP_CLOSE_BAD)registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(2U<<18);
    return fault==CLOSE_FAIL?-1:0;
}
static void m_mmio_restore(volatile uint32_t *address,uint32_t value)
{
    assert(address==&registers_gpio[3]&&value==2&&mmio_count==0);
    assert(closes==1&&!pcm_open&&!active&&(!resume_attempts||paused)&&fault!=CLOSE_FAIL);
    assert(mux_enters==1&&!mux_exits&&registers_sys[0x74/4]==0x02000184&&registers_gpio[0]==0x008000e6&&registers_gpio[3]==0);
    assert(sigismember(&blocked_signals,SIGINT)==1&&sigismember(&blocked_signals,SIGTERM)==1&&
           sigismember(&blocked_signals,SIGALRM)==1&&sigismember(&blocked_signals,SIGHUP)==1);
    ++mmio_count;
    if(fault!=RESTORE_FAIL)*address=value;
    if(fault==RESTORE_OTHER_OUT)*address|=4;
    if(fault==RESTORE_DIR)registers_gpio[0]^=4;
    if(fault==RESTORE_MUX)registers_sys[0x74/4]^=0x100;
    if(fault==CANCEL_AFTER_STORE)signal_now(SIGTERM);
}
static void m_mux_enter(volatile uint32_t *address,uint32_t value)
{
    assert(address==&registers_sys[0x74/4]&&value==0x02000184&&mux_enters==0&&mux_exits==0);
    assert(!opens&&!closes&&!pcm_open&&!active&&sys_maps==2&&gpio_maps==2);
    assert(registers_sys[0x74/4]==0x02000186&&registers_gpio[0]==0x008000e6&&registers_gpio[3]==2);
    assert(sigismember(&blocked_signals,SIGTERM)==1);
    ++mux_enters;
    if(fault!=ENTER_STORE_FAIL)*address=value;
    if(fault==ENTER_OTHER_BITS)*address^=0x100;
    if(fault==ENTER_GPIO2_CHANGE)*address&=~4U;
    if(fault==ENTER_DIR_CHANGE)registers_gpio[0]^=4;
    if(fault==CANCEL_AFTER_ENTER)signal_now(SIGTERM);
}
static void m_mux_exit(volatile uint32_t *address,uint32_t value)
{
    assert(address==&registers_sys[0x74/4]&&value==0x02000186&&mux_enters==1&&mux_exits==0);
    assert(!pcm_open&&!active&&(!opens||closes==1)&&(!resume_attempts||paused)&&fault!=CLOSE_FAIL);
    assert(registers_sys[0x74/4]==0x02000184&&registers_gpio[0]==0x008000e6&&registers_gpio[3]==2);
    assert(sigismember(&blocked_signals,SIGTERM)==1);
    ++mux_exits;
    if(fault!=EXIT_STORE_FAIL)*address=value;
    if(fault==EXIT_OTHER_BITS)*address^=0x100;
    if(fault==EXIT_DIR_CHANGE)registers_gpio[0]^=4;
    if(fault==CANCEL_AFTER_EXIT)signal_now(SIGTERM);
    if(fault==HP_FINAL_BAD)registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(2U<<18);
}
static int m_printf(const char *fmt,...)
{
    assert(!output[0]);va_list ap;va_start(ap,fmt);
    int n=vsnprintf(output,sizeof(output),fmt,ap);va_end(ap);assert(n>0&&(size_t)n<sizeof(output));
    if(fault==BROKEN_PIPE){assert(handlers[SIGPIPE]==SIG_IGN);errno=EPIPE;return -1;}
    return n;
}
static int m_fflush(FILE *f){assert(f==stdout);return 0;}
static int m_kill(pid_t p,int sig)
{
    assert(p==123&&sig==SIGSTOP);++retained;assert(retained==1);longjmp(escape,125);
}

/* Mock the exact raw setter's cache-equality shortcut and encoded hardware
 * field separately. The production helper may never write this field directly. */
static void kernel_gain_set(uint32_t value)
{
    static const uint32_t encoding[6]={16,8,4,2,1,0};
    assert(value==1||value==3);
    if(gain_cache!=value){
        gain_cache=value;
        registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(encoding[value]<<18);
    }
}
static int m_ioctl(int fd,unsigned long op,...)
{
    assert(fd==21&&pcm_open);va_list ap;va_start(ap,op);void *p=va_arg(ap,void*);va_end(ap);
    ++io_count;
    unsigned words=0;
    switch(op){
    case 0x401c50f2UL:case 0x801c50f2UL:words=7;break;
    case 0x804050f0UL:words=16;break;
    case 0x800850e5UL:words=2;break;
    case 0x80045070UL:case 0x80045010UL:case 0x80045030UL:case 0x40045030UL:case 0x800450e7UL:words=1;break;
    case 0x50e0UL:case 0x50e1UL:case 0x50e2UL:assert(!p);break;
    default:assert(!"unexpected ioctl (capture/NR/GPIO/available excluded)");
    }
    if(words)assert(p);
    if(op==0x50e1UL)++resume_attempts;
    uint32_t *v=p;
    if(op==0x40045030UL){
        assert(!active&&pcm_open&&!closes&&prepared);
        if(*v==3){
            assert(!gain3_sets&&!gain1_sets&&!resume_attempts);++gain3_sets;
            if(fault==SET3_CACHE_ONLY)gain_cache=3;
            else if(fault==SET3_HW_ONLY)registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(2U<<18);
            else if(fault!=SET3_NO_EFFECT)kernel_gain_set(3);
            if(fault==CANCEL_SET3)signal_now(SIGTERM);
            if(fault==SET3_NEG_AFTER){errno=EIO;return -1;}
            if(fault==SET3_POS_AFTER)return 1;
            if(fault==SET3_CANARY_AFTER){v[1]=0;return 0;}
        }else{
            assert(*v==1&&gain3_sets==1&&!gain1_sets&&(!resume_attempts||paused));++gain1_sets;
            if(fault==RESTORE_CACHE_ONLY)gain_cache=1;
            else if(fault==RESTORE_HW_ONLY)registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(8U<<18);
            else if(fault!=RESTORE_NO_EFFECT)kernel_gain_set(1);
            if(fault==CANCEL_GAIN_RESTORE)signal_now(SIGTERM);
            if(fault==GAIN_RESTORE_PIN_BAD)registers_sys[0x74/4]^=0x100;
            if(fault==RESTORE_NEG_AFTER){errno=EIO;return -1;}
            if(fault==RESTORE_POS_AFTER)return 1;
            if(fault==RESTORE_CANARY_AFTER){v[1]=0;return 0;}
        }
    }
    /* For setters, generic fault injection deliberately occurs AFTER effects. */
    if(fail_io==(int)io_count){
        if(io_failure_kind==2 && words){((uint32_t*)p)[words]=0;return 0;}
        if(io_failure_kind==1)return 1;
        errno=EIO;return -1;
    }
    switch(op){
    case 0x80045070UL:*v=fault==DEVICE_BAD?2:1;break;
    case 0x80045010UL:*v=fault==SOURCE_BAD?2:4;break;
    case 0x40045030UL:break;
    case 0x80045030UL:
        *v=fault==GAIN_BAD?5:fault==GAIN_CHANGED&&paused?2:fault==CACHE_RESUME_BAD&&active?1:gain_cache;
        if(gain_cache==3&&*v==3)++gain3_reads;
        if(gain1_sets&&*v==1&&((registers_sys[0x9c/4]>>18)&31)==8)++gain1_restore_reads;
        break;
    case 0x800450e7UL:
        *v=paused?(fault==READY_AFTER_STOP?1:0):queued>=4096?1:fault==READY_OLD?1:0;break;
    case 0x401c50f2UL:{
        const uint32_t expected[7]={8000,2,16,4096,12,4096,0};
        assert(!prepared&&!active&&!memcmp(v,expected,sizeof(expected)));memcpy(cached,v,28);break;}
    case 0x801c50f2UL:memcpy(v,cached,28);if(fault==PARAM_BAD)v[6]=1;break;
    case 0x50e0UL:assert(cached[3]==4096);prepared=1;
        if(fault==SETUP_MUX)registers_sys[0x74/4]^=2U;
        if(fault==MUX_OTHER_SETUP)registers_sys[0x74/4]^=0x100;
        if(fault==DIR_SETUP)registers_gpio[0]^=4;
        if(fault==OTHER_OUT_SETUP)registers_gpio[3]|=4;
        if(fault==EARLY_LATCH_ZERO)registers_gpio[3]=0;
        if(fault==HP_PREP_BAD)registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(4U<<18);
        break;
    case 0x804050f0UL:assert(prepared);memset(v,0,64);v[1]=fault==RATE_BAD?8000:8012;break;
    case 0x800850e5UL:
        v[0]=v[1]=0;
        if(active){++time_reads;v[0]=fault==DRAIN_STALL?0:(time_reads>=2?2U:1U)*(1024000000U/8012U);
            if(fault==COUNTER_REVERSE && time_reads>=2)v[0]=1;}
        break;
    case 0x50e1UL:
        assert(prepared&&queued==8192&&writes==16);active=1;
        assert(gain3_sets==1&&!gain1_sets&&gain3_reads&&gain_cache==3&&((registers_sys[0x9c/4]>>18)&31)==2);
        registers_gpio[3]=(registers_gpio[3]&~2U)|(registers_gpio[6]&2U);
        if(fault==START_MUX)registers_sys[0x74/4]^=2U;
        if(fault==INPUT_DURING)registers_gpio[6]^=2;
        if(fault==OTHER_OUT_ACTIVE)registers_gpio[3]|=4;
        if(fault==DIR_ACTIVE)registers_gpio[0]^=4;
        if(fault==HP_RESUME_BAD)registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(4U<<18);
        if(fault==RESUME_SIDE_EFFECT_ERROR){errno=EIO;return -1;}
        break;
    case 0x50e2UL:
        if(active){++stops;if(fault==STOP_FAIL||fault==BROKEN_PIPE){errno=EIO;return -1;}
            active=0;paused=1;
            if(fault==STOP_MUX)registers_sys[0x74/4]^=2U;
            if(fault==OTHER_OUT_STOP)registers_gpio[3]|=4;
            if(fault==HP_PAUSE_BAD)registers_sys[0x9c/4]=(registers_sys[0x9c/4]&~0x007c0000U)|(4U<<18);
            if(fault==CANCEL_STOP)signal_now(SIGALRM);}
        break;
    default:assert(0);
    }
    return 0;
}
static ssize_t m_write(int fd,const void *p,size_t n)
{
    assert(fd==21&&pcm_open&&prepared&&!active&&n==512&&queued+n<=8192);
    assert(gain3_sets==1&&!gain1_sets&&gain_cache==3&&((registers_sys[0x9c/4]>>18)&31)==2);
    const uint8_t *b=p;
    /* Independent waveform/duplicate-channel oracle over actual write bytes. */
    for(unsigned i=0;i<512;i+=4){
        unsigned frame=(queued+i)/4;
        int phase=(int)(frame%8),v=phase<2?phase*4096:phase<6?(4-phase)*4096:(phase-8)*4096;
        unsigned ramp=frame<64?frame:64;
        if(frame>=1792)v=0;
        else if(1791-frame<ramp)ramp=1791-frame;
        if(frame<1792)v=v*(int)ramp/64;
        uint16_t u=(uint16_t)v;
        assert(b[i]==(uint8_t)u&&b[i+1]==(uint8_t)(u>>8));
        assert(b[i]==b[i+2]&&b[i+1]==b[i+3]);
    }
    ++writes;
    if((int)writes==write_fault_at){
        if(fault==SHORT_WRITE)return 511;
        if(fault==ZERO_WRITE)return 0;
        if(fault==ERROR_WRITE){errno=EAGAIN;return -1;}
    }
    memcpy(ring+queued,p,n);queued+=(unsigned)n;return (ssize_t)n;
}

#define getuid m_getuid
#define geteuid m_geteuid
#define getpid m_getpid
#define uname m_uname
#define fcntl m_fcntl
#define sigaction m_sigaction
#define sigprocmask m_sigprocmask
#define sigpending m_sigpending
#define TONE_HOST_MMIO_STORE m_mmio_restore
#define TONE_HOST_MUX_ENTER m_mux_enter
#define TONE_HOST_MUX_EXIT m_mux_exit
#define alarm m_alarm
#define clock_gettime m_clock_gettime
#define nanosleep m_nanosleep
#define lstat m_lstat
#define fstat m_fstat
#define open m_open
#define read m_read
#define mmap m_mmap
#define munmap m_munmap
#define close m_close
#define printf m_printf
#define fflush m_fflush
#define kill m_kill
#define ioctl m_ioctl
#define write m_write
#define main speaker_main
/* Function-like macro avoids renaming the struct sigaction tag. */
#undef sigaction
#define sigaction(...) m_sigaction(__VA_ARGS__)
#include "tone.c"
#undef main
#undef printf

static unsigned cases;
static int execute(int f,int failed_io,int kind,int failed_write,int argc,char **argv)
{
    fault=f;fail_io=failed_io;io_failure_kind=kind;write_fault_at=failed_write;
    opens=writes=closes=io_count=stops=retained=now_ms=clock_count=queued=time_reads=0;
    mmio_count=mask_blocks=mask_restores=0;
    mux_enters=mux_exits=sys_maps=gpio_maps=0;
    gain_cache=1;gain3_sets=gain1_sets=gain3_reads=gain1_restore_reads=0;
    sigemptyset(&blocked_signals);sigemptyset(&pending_signals);
    prepared=active=paused=pcm_open=mem_open=resume_attempts=0;cancelled=0;
    memset(meta_pos,0,sizeof(meta_pos));memset(output,0,sizeof(output));memset(handlers,0,sizeof(handlers));
    memset(registers_sys,0,sizeof(registers_sys));memset(registers_gpio,0,sizeof(registers_gpio));
    memset(ring,0xcc,sizeof(ring));memset(cached,0,sizeof(cached));
    registers_sys[0x74/4]=f==MUX_BAD?0x02000184:0x02000186;
    registers_sys[2]=f==PLL_BAD?0:0x13c8; /* M200/N3/OD1 =>400MHz */
    registers_sys[0x78/4]=0x50;
    registers_sys[0x9c/4]=f==HP_INITIAL_BAD?0xfe120008:0xfe220008;
    registers_gpio[0]=f==DIR_BAD?0:0x008000e6;
    registers_gpio[3]=f==LATCH_BAD?0:f==OTHER_OUT_ENTRY?6:2;
    registers_gpio[6]=f==INPUT_HIGH?0xe0000013:0xe0000011;
    int result=setjmp(escape);
    if(!result)result=speaker_main(argc,argv);
    assert(strlen(output)<3072 && !strstr(output,"base64") && !strstr(output,"raw_hex"));
    if(opens)assert(output[0]);
    assert(closes<=1&&resume_attempts<=1&&writes<=16&&stops<=1&&mmio_count<=1);
    assert(mux_enters<=1&&mux_exits<=1&&mux_enters+mmio_count+mux_exits<=3);
    assert(gain3_sets<=1&&gain1_sets<=1);
    assert(registers_sys[0x78/4]==0x50); /* no IR-cut/SYS78 write */
    if(f!=ENTER_GPIO2_CHANGE&&f!=MUX_BAD)assert((registers_sys[0x74/4]&4)==4);
    if(mmio_count)assert(closes==1&&!active&&fault!=CLOSE_FAIL&&(!resume_attempts||paused));
    if(active||fault==CLOSE_FAIL)assert(mmio_count==0&&mux_exits==0);
    if(active)assert(gain1_sets==0);
    if(output[0]){
        assert(strstr(output,"\"record\":\"stock-speaker-tone-v6\""));
        assert(strstr(output,"\"digital_peak\":8192"));
        char needle[64];snprintf(needle,sizeof(needle),"\"MMIO_write_count\":%u",mux_enters+mmio_count+mux_exits);
        assert(strstr(output,needle));
        assert(strstr(output,"USER_UART_SIGNALS_DISCONNECTED"));
        assert(strstr(output,"\"bank_lease_verified_by_helper\":false"));
        snprintf(needle,sizeof(needle),"\"gain_set3_attempts\":%u",gain3_sets);assert(strstr(output,needle));
        snprintf(needle,sizeof(needle),"\"gain_restore_attempts\":%u",gain1_sets);assert(strstr(output,needle));
        assert(strstr(output,"\"HP_direct_stores\":0"));
    }
    if(retained && !strstr(output,"CLOSE_UNCERTAIN") && !strstr(output,"\"close_ok\":true"))assert(closes==0);
    if(result==0){assert(closes==1&&paused&&!active&&queued==8192&&registers_gpio[3]==2&&mux_exits==1&&registers_sys[0x74/4]==0x02000186);
        assert(gain3_sets==1&&gain1_sets==1&&gain_cache==1&&((registers_sys[0x9c/4]>>18)&31)==8);
        assert(strstr(output,"TONE_COMPLETED_AWAIT_USER")&&strstr(output,"\"latch_restoration_verified\":true")&&strstr(output,"\"gain_restored\":true"));}
    else assert(!strstr(output,"TONE_COMPLETED_AWAIT_USER"));
    if(resume_attempts && !paused && result!=0)assert(!closes);
    /* Return0/1/2 permits parent watchdog release only for no-effect or fully
     * retired/restored outcomes. Retained paths longjmp from SIGSTOP, not return. */
    if(result==0||result==1||result==2){
        assert(!active&&!pcm_open);
        if(mux_enters)assert(mux_exits==1&&registers_sys[0x74/4]==0x02000186&&registers_gpio[3]==2);
        if(gain3_sets)assert(gain1_sets==1&&gain1_restore_reads&&gain_cache==1&&((registers_sys[0x9c/4]>>18)&31)==8);
    }
    if(queued==8192)for(unsigned i=7168;i<8192;++i)assert(ring[i]==0);
    for(unsigned i=queued;i<49152;++i)assert(ring[i]==0xcc);
    ++cases;return result;
}
int main(void)
{
    char *args[]={"/fixed/stock-speaker-tone","--native-stock-speaker-tone-gain3-uart-isolated",0};
    char example_restored[3072],example_noop[3072]={0},example_retained[3072]={0},example_unopened[3072]={0};
    char example_failed_set[3072]={0},example_failed_restore[3072]={0};
    assert(execute(0,0,0,0,2,args)==0);
    strcpy(example_restored,output);
    unsigned healthy_io=io_count;
    assert(healthy_io==25);
    /* Check the actual submitted LE samples independently of the generator and
     * arithmetic waveform oracle: exact signed peaks, bounded amplitude and
     * zero summed DC per channel over tone/fades/padding. Not physical audio. */
    int64_t sum=0;int32_t minimum=0,maximum=0;
    for(unsigned i=0;i<8192;i+=4){
        uint16_t u=(uint16_t)(ring[i]|(uint16_t)ring[i+1]<<8);
        int32_t s=u<32768?u:(int32_t)u-65536;
        assert(s>=-8192&&s<=8192&&ring[i]==ring[i+2]&&ring[i+1]==ring[i+3]);
        if(s<minimum)minimum=s;
        if(s>maximum)maximum=s;
        sum+=s;
    }
    assert(minimum==-8192&&maximum==8192&&sum==0);++cases;
    assert(mmio_count==1&&registers_gpio[3]==2&&mux_enters==1&&mux_exits==1);
    for(int f=BAD_UID;f<=GAIN_RESTORE_PIN_BAD;++f){
        int result=execute(f,0,0,1,2,args);
        if(f==INPUT_BAD||f==INPUT_DURING||f==INPUT_HIGH){assert(result==0);assert(mmio_count==(f==INPUT_BAD?1U:0U));}
        else assert(result!=0);
        if(f==INPUT_HIGH)strcpy(example_noop,output);
        if(f==STOP_FAIL)strcpy(example_retained,output);
        if(f==SET3_NEG_AFTER)strcpy(example_failed_set,output);
        if(f==RESTORE_CACHE_ONLY)strcpy(example_failed_restore,output);
        if(f==PCM_OPEN_FAIL){strcpy(example_unopened,output);assert(!opens&&!closes&&!mmio_count&&mux_exits==1);}
        if(f==SETUP_MUX||f==START_MUX||f==STOP_MUX||f==CLOSE_MUX)assert(result==125);
        if(f==OTHER_OUT_SETUP||f==OTHER_OUT_ACTIVE||f==OTHER_OUT_STOP||f==OTHER_OUT_CLOSE||
           f==DIR_SETUP||f==DIR_ACTIVE||f==DIR_CLOSE||f==MUX_OTHER_SETUP||f==MUX_OTHER_CLOSE||
           f==CLOSE_DEADLINE||f==MASK_BLOCK_FAIL||
           f==PENDING_FAIL||f==EARLY_LATCH_ZERO||f==COMMIT_OTHER_OUT||f==COMMIT_DIR||f==COMMIT_MUX){assert(result==125&&mmio_count==0);}
        if(f==CANCEL_SETUP||f==CANCEL_WRITE||f==CANCEL_CLOSE||f==CANCEL_RESTORE_PENDING||f==CANCEL_AFTER_STORE||f==CANCEL_AFTER_EXIT)
            assert(result==1&&mux_exits==1&&registers_gpio[3]==2&&registers_sys[0x74/4]==0x02000186);
        if(f==ENTER_STORE_FAIL||f==ENTER_OTHER_BITS||f==ENTER_GPIO2_CHANGE||f==ENTER_DIR_CHANGE||f==ENTER_UNMASK_FAIL)
            assert(result==125&&mux_enters==1&&mux_exits==0&&!opens);
        if(f==CANCEL_AFTER_ENTER)assert(result==2&&mux_enters==1&&mux_exits==1&&!opens&&!mmio_count);
        if(f==ENTER_PENDING_CANCEL||f==ENTER_PENDING_FAIL||f==ENTER_BLOCK_FAIL||f==ENTER_COMMIT_OTHER_OUT||f==CANCEL_BEFORE_ENTER)
            assert(result==2&&!mux_enters&&!opens);
        if(f==HP_INITIAL_BAD)assert(result==2&&!opens&&!gain3_sets&&!gain1_sets);
        if(f==SET3_NEG_AFTER||f==SET3_POS_AFTER||f==SET3_CANARY_AFTER||f==SET3_CACHE_ONLY||f==SET3_NO_EFFECT||f==CANCEL_SET3)
            assert(result==1&&gain3_sets==1&&gain1_sets==1&&!resume_attempts&&closes==1&&gain_cache==1);
        if(f==SET3_HW_ONLY||f==RESTORE_NEG_AFTER||f==RESTORE_POS_AFTER||f==RESTORE_CANARY_AFTER||
           f==RESTORE_CACHE_ONLY||f==RESTORE_HW_ONLY||f==RESTORE_NO_EFFECT||f==GAIN_RESTORE_PIN_BAD)
            assert(result==125&&gain3_sets==1&&gain1_sets==1&&!closes&&!mux_exits);
        if(f==HP_RESUME_BAD||f==CACHE_RESUME_BAD||f==HP_PAUSE_BAD||f==GAIN_RESTORE_EXPIRED)
            assert(result==125&&gain3_sets==1&&!gain1_sets&&!closes);
        if(f==HP_CLOSE_BAD)assert(result==125&&closes==1&&!mmio_count&&!mux_exits&&strstr(output,"\"HP_encoded_after\":2"));
        if(f==HP_FINAL_BAD)assert(result==125&&closes==1&&mux_exits==1&&!strstr(output,"\"gain_restored\":true"));
        if(f==CANCEL_GAIN_RESTORE)assert(result==1&&closes==1&&mux_exits==1&&gain1_sets==1);
    }
    for(unsigned i=1;i<=healthy_io;++i)for(int k=0;k<3;++k)
        assert(execute(0,(int)i,k,0,2,args)!=0);
    /* Faults during SET1 restoration also apply on the preactive cleanup path. */
    for(int f=RESTORE_NEG_AFTER;f<=RESTORE_NO_EFFECT;++f)
        assert(execute(f,14,0,0,2,args)==125&&gain3_sets==1&&gain1_sets==1&&!closes&&!resume_attempts);
    for(int f=SHORT_WRITE;f<=ZERO_WRITE;++f)for(int at=8;at<=16;at+=8)
        assert(execute(f,0,0,at,2,args)==1);
    /* Both ordinary and preactive close must finish restoration; failure there
     * retains remaining owners and never attempts another store. */
    for(int f=RESTORE_FAIL;f<=RESTORE_MUX;++f){
        assert(execute(f,3,0,0,2,args)==125&&closes==1&&mmio_count==1);
    }
    assert(execute(INPUT_HIGH,3,0,0,2,args)==1&&closes==1&&mmio_count==0);
    assert(execute(0,3,0,0,2,args)==1&&closes==1&&mmio_count==1);
    assert(execute(CLOSE_FAIL,3,0,0,2,args)==125&&closes==1&&mmio_count==0);
    assert(execute(CANCEL_CLOSE,3,0,0,2,args)==1&&closes==1&&mmio_count==1&&mux_exits==1);
    assert(execute(0,0,0,0,1,args)==2);
    char *bad[]={"/fixed/stock-speaker-tone","--native-stock-speaker-tone-gain3-uart-isolated","repeat",0};
    assert(execute(0,0,0,0,3,bad)==2);
    char *relative[]={"relative","--native-stock-speaker-tone-gain3-uart-isolated",0};
    assert(execute(0,0,0,0,2,relative)==2);
    char *obsolete[]={"/fixed/stock-speaker-tone","--native-stock-speaker-tone",0};
    assert(execute(0,0,0,0,2,obsolete)==2);
    char *tone04[]={"/fixed/stock-speaker-tone","--native-stock-speaker-tone-uart-isolated",0};
    assert(execute(0,0,0,0,2,tone04)==2&&!opens&&!mux_enters);
    char *tone05[]={"/fixed/stock-speaker-tone","--native-stock-speaker-tone-louder-uart-isolated",0};
    assert(execute(0,0,0,0,2,tone05)==2&&!opens&&!mux_enters);
    /* Rate algorithm separate from kernel payload/generator, including invalid PLL. */
    assert(dac_rate(0x13c8)==8012&&dac_rate(0)==0&&dac_rate(0x1100)==0);
    printf("{\"host_cases\":%u,\"actual_syscall_sequence\":true,\"healthy_ioctls\":%u,"
            "\"prefill_writes\":16,\"prefill_bytes\":8192,\"scratch_bytes\":512,\"digital_peak\":8192,\"per_channel_sample_sum\":0,"
            "\"max_helper_MMIO_writes\":3,\"mux_enter_before_PCM\":true,\"HP_direct_stores\":0,\"max_GAIN_SET_attempts\":2,\"normal_return_restoration_checked\":true,"
            "\"baseline_maps_readonly\":true,\"SYS78_writes\":false,\"native_executed\":false,\"physical_heard\":\"AWAIT_USER\","
            "\"mock_result_examples\":{\"restored\":%s,\"already_restored\":%s,\"active_failure\":%s,\"unopened_rollback\":%s,"
            "\"set3_failed_restored\":%s,\"restore_mismatch_retained\":%s}}\n",
            cases,healthy_io,example_restored,example_noop,example_retained,example_unopened,example_failed_set,example_failed_restore);
    return 0;
}
