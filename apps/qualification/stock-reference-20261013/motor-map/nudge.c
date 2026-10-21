/* Fixed retail motor mapping test: motor0 retry +64 from16, motor1 +16 from0.
 * Exact source SHA2ac16460...bec58. Host MOCK never accesses a device.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <assert.h>
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
#define GET 0x40046d43UL
#define RATE 0x40046d20UL
#define MOVE 0x40046d40UL
#define STOP 0x40046d42UL
static volatile sig_atomic_t cancelled;
#ifndef MOTOR_MOCK
static void cancel(int s){(void)s;cancelled=1;}
#endif
#ifdef MOTOR_MOCK
static int stage,fault,position,rate=4,queries,goal,test_hz,requested_steps;
static unsigned moves,closes;
static uint64_t tick;
static unsigned axis;
static int mock_ioctl(int fd,unsigned long req,void *p){
    (void)fd;
    if(req==GET){uint32_t *w=p;memset(w,0,36);w[1]=(uint32_t)position;w[2]=(uint32_t)rate;w[4]=4096;w[5]=axis?600:2400;w[6]=30;w[7]=axis;w[8]=4;
        if(stage && (++queries<20 || fault==4))w[0]=1;else if(stage){position=goal;w[1]=(uint32_t)goal;}
        if(fault==1 && !stage)w[0]=1;
        return 0;}
    if(req==RATE){assert(*(int32_t*)p==test_hz || *(int32_t*)p==4);if(fault==2 || (fault==6 && *(int32_t*)p==4))return -1;rate=*(int32_t*)p;return 0;}
    if(req==MOVE){assert(*(int32_t*)p==requested_steps && rate==test_hz);moves++;stage=1;return fault==3?1:0;}
    if(req==STOP)return fault==5?-1:0;
    assert(0);return -1;
}
#define IO mock_ioctl
static int now(uint64_t *p){tick+=20;*p=tick;return 0;}
static int close_motor(int fd){(void)fd;closes++;return 0;}
static int retained(void){printf("MOCK_RETAINED moves=%u closes=%u\n",moves,closes);return 125;}
static uint32_t gpio_read(unsigned word){return word==0?0x8198ece6U:2U|(stage && queries<20?0x00100800U:0);}
#else
#define IO ioctl
static int now(uint64_t *p){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return -1;*p=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;return 0;}
static int close_motor(int fd){return close(fd);}
static int retained(void) __attribute__((noreturn));
static int retained(void){for(;;){kill(getpid(),SIGSTOP);pause();}}
static volatile const uint32_t *gpio;
static uint32_t gpio_read(unsigned word){return gpio[word];}
#endif
static int query(int fd,uint32_t w[9]){
    struct {uint32_t before,w[9],after;} b={0x937ea512,{0},0x937ea512};
    if(IO(fd,GET,b.w)!=0||b.before!=0x937ea512||b.after!=0x937ea512)return -1;
    memcpy(w,b.w,36);return 0;
}
int main(int argc,char **argv){
    unsigned which;int fd,effect=0,rc=0,idle=0,status_only=0;uint32_t before[9],after[9],out_or=0,last_out,changes=0,dir,mask;int32_t hz,steps,restore;
    uint64_t start,t;struct timespec nap={0,20000000};
#ifdef MOTOR_GUI
    if(argc!=2||argv[0][0]!='/')return 2;
    const char *selectors[]={"motor0-plus32","motor0-minus32","motor1-plus32","motor1-minus32","motor0-status","motor1-status"};
    unsigned sel;for(sel=0;sel<6;sel++)if(!strcmp(argv[1],selectors[sel]))break;
    if(sel==6)return 2;
    which=sel<4?sel/2:sel-4;status_only=sel>=4;steps=status_only?0:(sel&1?-32:32);hz=128;
#else
    if(argc!=2||argv[0][0]!='/'||(!strcmp(argv[1],"motor0-positive64")?0:strcmp(argv[1],"motor1-positive16")))return 2;
    which=(unsigned)(argv[1][5]-'0');
    steps=which?16:64;hz=which?64:256;
#endif
    mask=which?0x0100e000U:0x00180c00U;
#ifdef MOTOR_MOCK
    axis=which;fault=MOCK_FAULT;position=which?0:16;
#ifdef MOTOR_GUI
    position=MOCK_START;
#endif
    goal=position+steps;requested_steps=steps;test_hz=hz;fd=5;
#else
    struct stat pre,post;struct utsname u;struct sigaction sa={0};
    if(getuid()||geteuid()||getpid()==1||uname(&u)||strcmp(u.release,"3.4.35")||fcntl(3,F_GETFD)!=-1||errno!=EBADF)return 2;
    sa.sa_handler=cancel;sigemptyset(&sa.sa_mask);
    if(sigaction(SIGALRM,&sa,0)||sigaction(SIGINT,&sa,0)||sigaction(SIGTERM,&sa,0))return 2;
    const char *path=which?"/dev/motor1":"/dev/motor0";
    if(lstat(path,&pre)||!S_ISCHR(pre.st_mode)||pre.st_uid||major(pre.st_rdev)!=251||minor(pre.st_rdev)!=which)return 2;
    fd=open(path,O_RDWR|O_CLOEXEC|O_NOFOLLOW);
    if(fd<3)return 2;
    if(fstat(fd,&post)||pre.st_dev!=post.st_dev||pre.st_ino!=post.st_ino||pre.st_rdev!=post.st_rdev)return retained();
    struct stat mem_pre,mem_post;
    if(lstat("/dev/mem",&mem_pre)||!S_ISCHR(mem_pre.st_mode)||mem_pre.st_uid||major(mem_pre.st_rdev)!=1||minor(mem_pre.st_rdev)!=1)return retained();
    int mem=open("/dev/mem",O_RDONLY|O_SYNC|O_CLOEXEC|O_NOFOLLOW);
    if(mem<3||fstat(mem,&mem_post)||mem_pre.st_dev!=mem_post.st_dev||mem_pre.st_ino!=mem_post.st_ino||mem_pre.st_rdev!=mem_post.st_rdev)return retained();
    gpio=mmap(0,4096,PROT_READ,MAP_SHARED,mem,0x20170000);
    if(gpio==MAP_FAILED)return retained();
    alarm(2);
#endif
    if(query(fd,before)||before[0]||before[2]!=4||before[4]!=4096||before[5]!=(which?600U:2400U)||before[6]!=30||before[7]!=which||before[8]!=4||now(&start))return retained();
#ifndef MOTOR_GUI
    if(before[1]!=(which?0U:16U))return retained();
#endif
    dir=gpio_read(0);last_out=gpio_read(3);
    if((dir&mask)!=mask || (last_out&mask))return retained();
#ifdef MOTOR_GUI
    if(status_only || (int64_t)before[1]+steps<31 || (int64_t)before[1]+steps>=(int64_t)before[5]-before[6]){
#else
    if(status_only){
#endif
#ifndef MOTOR_MOCK
        alarm(0);
#endif
        if(close_motor(fd))return retained();
#ifndef MOTOR_MOCK
        if(munmap((void*)gpio,4096)||close(mem))return 1;
#endif
        printf("{\"record\":\"motor-gui-v1\",\"state\":\"%s\",\"motor\":%u,\"software_position\":%u,\"rate\":%u,\"travel\":%u,\"margin\":%u,\"phases\":%u,\"idle\":true}\n",status_only?"STATUS":"LIMIT_BLOCKED",which,before[1],before[2],before[5],before[6],before[8]);return 0;
    }
    restore=(int32_t)before[2];effect=1;
    if(IO(fd,RATE,&hz)!=0||query(fd,after)||after[0]||after[2]!=(uint32_t)hz)rc=1;
    if(!rc && IO(fd,MOVE,&steps)!=0)rc=1;
    while(!rc){if(now(&t)||t<start||t-start>=1500||cancelled){rc=1;break;}
        uint32_t out=gpio_read(3);out_or|=out&mask;if((out&mask)!=(last_out&mask))changes++;last_out=out;
        if(query(fd,after)){rc=1;break;}if(!after[0]){idle=1;break;}
#ifndef MOTOR_MOCK
        if(nanosleep(&nap,0)&&errno!=EINTR){rc=1;break;}
#endif
    }
    if(effect && IO(fd,STOP,0)!=0)rc=1;
    if(query(fd,after)||after[0]){printf("{\"record\":\"motor-map-v1\",\"motor\":%u,\"state\":\"RETAINED_UNKNOWN_STOP\"}\n",which);fflush(stdout);return retained();}
    if(IO(fd,RATE,&restore)!=0||query(fd,after)||after[0]||after[2]!=before[2]){printf("{\"record\":\"motor-map-v1\",\"motor\":%u,\"state\":\"RETAINED_RATE_RESTORE_FAILURE\"}\n",which);fflush(stdout);return retained();}
    if(!idle || after[1]!=before[1]+(uint32_t)steps || (gpio_read(3)&mask))rc=1;
    if(now(&t)||t<start)rc=1;
#ifndef MOTOR_MOCK
    alarm(0);
#else
    (void)nap;
#endif
    if(rc){printf("{\"record\":\"motor-map-v1\",\"motor\":%u,\"state\":\"RETAINED_FAILED_MOVE\",\"idle\":true,\"rate_restored\":true}\n",which);fflush(stdout);return retained();}
    if(close_motor(fd))return retained();
#ifndef MOTOR_MOCK
    if(munmap((void*)gpio,4096)||close(mem))return 1;
#endif
    printf("{\"record\":\"motor-map-v2\",\"motor\":%u,\"requested_signed_steps\":%d,\"test_timer_hz\":%d,\"software_position_before\":%u,\"software_position_after\":%u,\"elapsed_ms\":%llu,\"gpio_dir\":\"%08x\",\"gpio_motor_mask\":\"%08x\",\"gpio_out_or\":\"%08x\",\"gpio_phase_changes_sampled\":%u,\"idle\":true,\"stop_return_zero\":true,\"rate_restored\":4,\"physical_direction\":\"AWAIT_USER\"}\n",which,steps,hz,before[1],after[1],(unsigned long long)(t-start),dir,mask,out_or,changes);return 0;
}
