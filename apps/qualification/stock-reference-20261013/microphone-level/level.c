/* Fixed stock capture-only diagnostic. No audio bytes/hashes/files or playback.
 * Source admission: microphone-source01..03 + parent init/params reviews.
 * Only for a fresh unused controlled raw-state instance with external watchdog.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#define DEVICE_SET 0x40045070UL
#define DEVICE_GET 0x80045070UL
#define SOURCE_GET 0x80045010UL
#define PARAM_SET 0x401c50f2UL
#define PARAM_GET 0x801c50f2UL
#define FEATURES 0x804050f0UL
#define GAIN_GET 0x80045030UL
#define PREPARE 0x50e0UL
#define RESUME 0x50e1UL
#define PAUSE 0x50e2UL
#define RUNNING 0x800450e7UL
#define AVAILABLE 0x800450e8UL
static volatile sig_atomic_t cancel;
/* Exact retail get_adc2_osr_div uses OSR256 for request8000 and chooses the
 * closest of256 integer dividers. Current stock ASIC PLL is400MHz, observed in
 * the first native setup. Keep requested/calculated/physical rates distinct. */
static uint32_t expected_rate(void){uint32_t best=0,error=8000;for(uint32_t div=1;div<=256;div++){uint32_t v=400000000U/div/256U,d=v>8000?v-8000:8000-v;if(d<error){best=v;error=d;}}return best;}
struct stats { uint32_t n,nonzero,clipped;int32_t min,max;int64_t sum;uint64_t squares; };
static struct stats total,half[2];
#ifdef MIC_MOCK
static unsigned elapsed,bytes,closed,running,step;
static uint32_t cached[7];
static int io(int fd,unsigned long op,void *p){
    (void)fd;
    if(MIC_FAULT==1 && op==PREPARE)return -1;
    if(MIC_FAULT==2 && op==PARAM_SET)return 1;
    if(op==PARAM_SET){memcpy(cached,p,28);cached[5]=6144;}
    else if(op==PARAM_GET){memcpy(p,cached,28);if(MIC_FAULT==3)((uint32_t*)p)[6]=99;}
    else if(op==FEATURES){memset(p,0,64);((uint32_t*)p)[1]=MIC_FAULT==4?0:expected_rate();}
    else if(op==DEVICE_GET)*(uint32_t*)p=1;
    else if(op==SOURCE_GET)*(uint32_t*)p=2;
    else if(op==GAIN_GET)*(uint32_t*)p=MIC_FAULT==5?8:3;
    else if(op==RESUME){if(*(uint32_t*)p)return -1;running=1;}
    else if(op==PAUSE){if(MIC_FAULT==8)return -1;running=MIC_FAULT==9?1:0;}
    else if(op==RUNNING)*(uint32_t*)p=running;
    else if(op==AVAILABLE)*(uint32_t*)p=MIC_FAULT==6?0:MIC_FAULT==7?32769:4096;
    return 0;
}
static ssize_t rd(int fd,void *p,size_t n){(void)fd;if(MIC_FAULT==10)return (ssize_t)n-1;
    for(size_t i=0;i<n;i+=2){int16_t v=(MIC_FAULT==11)?0:(MIC_FAULT==12)?1234:(MIC_FAULT==13)?(step++&1?32767:-32768):(int16_t)(((step++*73)%20001)-10000);((uint8_t*)p)[i]=(uint8_t)v;((uint8_t*)p)[i+1]=(uint8_t)((uint16_t)v>>8);}bytes+=(unsigned)n;return (ssize_t)n;}
static int clk(uint64_t *ms){elapsed+=10;*ms=elapsed;return 0;}
static int endfd(int fd){(void)fd;closed++;return 0;}
static int retain(int reason){printf("MOCK_RETAINED reason=%d closed=%u rawbytesprinted=0\n",reason,closed);return 125;}
#else
static void mark(int s){(void)s;cancel=1;}
static int io(int fd,unsigned long op,void *p){return ioctl(fd,op,p);}
static ssize_t rd(int fd,void *p,size_t n){return read(fd,p,n);}
static int clk(uint64_t *ms){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L)return -1;*ms=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;return 0;}
static int endfd(int fd){return close(fd);}
static int retain(int reason) __attribute__((noreturn));
static int retain(int reason){printf("{\"record\":\"mic-level-v1\",\"state\":\"RETAINED_FAILURE\",\"reason\":%d,\"raw_audio_output\":false}\n",reason);fflush(stdout);for(;;){kill(getpid(),SIGSTOP);pause();}}
#endif
static int packet(int fd,unsigned long req,uint32_t *p,unsigned words){
    uint32_t b[18]={0};int rc;if(words>16)return -1;b[0]=0x937ea512;b[words+1]=0x937ea512;
    memcpy(b+1,p,words*4);rc=io(fd,req,b+1);memcpy(p,b+1,words*4);
    return rc==0 && b[0]==0x937ea512 && b[words+1]==0x937ea512?0:-1;
}
static void add(struct stats *s,int32_t v){if(!s->n || v<s->min)s->min=v;if(!s->n || v>s->max)s->max=v;s->n++;s->nonzero+=(v!=0);s->clipped+=(v==-32768||v==32767);s->sum+=v;s->squares+=(uint64_t)((int64_t)v*v);}
static uint32_t root(uint64_t n){uint32_t lo=0,hi=32768;while(lo<hi){uint32_t m=(lo+hi+1)/2;if((uint64_t)m*m<=n)lo=m;else hi=m-1;}return lo;}
static uint32_t ac_rms(const struct stats *s){uint64_t a=s->squares*s->n,b=(uint64_t)(s->sum*s->sum);return a>=b?root((a-b)/((uint64_t)s->n*s->n)):0;}
int main(int argc,char **argv){
    int fd;uint32_t params[7]={8000,1,16,2048,16,2048,0},got[7],features[16]={0},v=1,gain=0;
    uint8_t buffer[512];uint64_t start,now;unsigned nbytes=0;struct timespec nap={0,10000000};
    if(argc!=2||argv[0][0]!='/'||strcmp(argv[1],"--native-stock-mic-level1s"))return 2;
#ifdef MIC_MOCK
    fd=5;
#else
    struct stat pre,post;struct utsname u;struct sigaction sa={0};sigset_t mask;
    if(getuid()||geteuid()||getpid()==1||uname(&u)||strcmp(u.release,"3.4.35")||fcntl(3,F_GETFD)!=-1||errno!=EBADF)return 2;
    sa.sa_handler=mark;sigemptyset(&sa.sa_mask);
    if(sigaction(SIGALRM,&sa,0)||sigaction(SIGINT,&sa,0)||sigaction(SIGTERM,&sa,0)||sigemptyset(&mask)||sigaddset(&mask,SIGALRM)||sigaddset(&mask,SIGINT)||sigaddset(&mask,SIGTERM)||sigprocmask(SIG_UNBLOCK,&mask,0))return 2;
    if(lstat("/dev/akpcm_cdev1",&pre)||!S_ISCHR(pre.st_mode)||pre.st_uid||major(pre.st_rdev)!=253||minor(pre.st_rdev)!=1)return 2;
    fd=open("/dev/akpcm_cdev1",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<3)return 2;
    if(fstat(fd,&post)||pre.st_dev!=post.st_dev||pre.st_ino!=post.st_ino||pre.st_rdev!=post.st_rdev)return retain(1);
    alarm(2);
#endif
    if(packet(fd,DEVICE_SET,&v,1)||packet(fd,DEVICE_GET,&v,1)||v!=1)return retain(2);
    if(packet(fd,SOURCE_GET,&v,1)||v!=2||packet(fd,GAIN_GET,&gain,1)||gain>7)return retain(3);
    if(packet(fd,PARAM_SET,params,7))return retain(4);
    memset(got,0,sizeof(got));if(packet(fd,PARAM_GET,got,7))return retain(5);
    params[5]=6144;
    if(memcmp(params,got,28))return retain(17);
    if(packet(fd,FEATURES,features,16)||features[1]!=expected_rate())return retain(6);
    if(io(fd,PREPARE,0)!=0)return retain(7);
    v=0;if(clk(&start)||packet(fd,RESUME,&v,1))return retain(8);
    while(nbytes<16000){
        unsigned n=16000-nbytes;if(n>sizeof(buffer))n=sizeof(buffer);
        if(clk(&now)||now<start||now-start>=1500||cancel)return retain(9);
        v=0;if(packet(fd,AVAILABLE,&v,1)||v>32768)return retain(10);
        if(v<n){
#ifndef MIC_MOCK
            if(nanosleep(&nap,0)&&errno!=EINTR)return retain(11);
#endif
            continue;
        }
        if(rd(fd,buffer,n)!=(ssize_t)n)return retain(12);
        for(unsigned i=0;i<n;i+=2){uint16_t w=(uint16_t)(buffer[i]|(uint16_t)buffer[i+1]<<8);int32_t s=w>=32768?(int32_t)w-65536:w;add(&half[total.n>=4000],s);add(&total,s);}
        nbytes+=n;
    }
    if(io(fd,PAUSE,0)!=0)return retain(13);
    v=99;if(packet(fd,RUNNING,&v,1)||v)return retain(14);
    if(clk(&now)||now<start)return retain(15);
    if(endfd(fd))return retain(16);
#ifndef MIC_MOCK
    alarm(0);
#else
    (void)nap;
#endif
    printf("{\"record\":\"mic-level-v3\",\"state\":\"CAPTURE_COMPLETED_NEEDS_USER_SOUND_CORRELATION\",\"requested_rate\":8000,\"calculated_rate_cache\":%u,\"physical_rate_independently_measured\":false,\"requested_channels\":1,\"requested_bits\":16,\"interpretation\":\"S16_LE_MONO_CANDIDATE\",\"kernel_ring_bytes\":32768,\"samples\":%u,\"bytes\":%u,\"elapsed_ms_including_start_stop\":%llu,\"gain_code_preserved\":%u,\"min\":%d,\"max\":%d,\"mean\":%lld,\"rms\":%u,\"half_rms\":[%u,%u],\"ac_rms\":%u,\"half_ac_rms\":[%u,%u],\"half_mean\":[%lld,%lld],\"nonzero\":%u,\"clipped\":%u,\"stop_return_zero\":true,\"DMA_state_zero\":true,\"close_ok\":true,\"raw_audio_output\":false,\"audio_hash_output\":false,\"playback_opened\":false}\n",features[1],total.n,nbytes,(unsigned long long)(now-start),gain,total.min,total.max,(long long)(total.sum/(int64_t)total.n),root(total.squares/total.n),root(half[0].squares/half[0].n),root(half[1].squares/half[1].n),ac_rms(&total),ac_rms(&half[0]),ac_rms(&half[1]),(long long)(half[0].sum/(int64_t)half[0].n),(long long)(half[1].sum/(int64_t)half[1].n),total.nonzero,total.clipped);return 0;
}
