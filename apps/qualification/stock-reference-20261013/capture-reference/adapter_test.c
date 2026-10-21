/* Fixed syscall-level public-synthetic adapter tests. No real opens/proc/device,
 * ioctl/mmap/signal/write occurs: every adapter OS symbol is interposed here. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <mbedtls/sha256.h>
#include "native_adapter.h"
#include "adapter_sys.h"
static _Alignas(4096) uint8_t output_mem[CR_MAP],ref_mem[CR_REF_BYTES];
static uint8_t tuning_blob[CR_BLOB];
static uint32_t control[1024];static struct cr_state core;static struct cr_adapter a;static struct cr_ops ops;
enum kind { NONE,TTY,MOD,BLOB,IR,ISP,VIDEO,ION,DMA_OUT,DMA_REF,PM,MEM };
enum fault { OK,INHERITED,NONROOT,KERNEL,MODULE,UNADMITTED,NODE_KIND,NODE_ID,NODE_RACE,
 BLOB_UID,BLOB_LINK,BLOB_SIZE,BLOB_RACE,ALLOC_POS,ALLOC_INTR,ALLOC_REF_FAIL,EXPORT_ALIAS,
 MAP_REF_FAIL,PFN_LAST_GAP,PFN_OUTSIDE,PFN_OVERLAP,REF_SET_POS,REF_GET_BAD_ADDR,REF_GET_BAD_LENGTH,
 IO_CANARY,CTRL_ACTIVE,SENSOR_COUNT,GEOM_BAD,WRITE_PARTIAL,WRITE_INTR,WRITE_AGAIN,WRITE_TIMEOUT,
 TTY_WRONG,IR_PARTIAL,IR_VERIFY_BAD,STOP_RETURN,REF_EXPORT_FAIL,REF_EXPORT_ALIAS,
 REF_HANDLE_DUP,REF_LENGTH_RETURN };
static struct {
    enum fault fault;enum kind fd[64];off_t offset[64];unsigned opens,closes,ioctls,allocs,maps,pages[2],stop,write_calls;
    unsigned module_reads,blob_stats,alloc_handles,ref_set,ref_get;uint64_t ms;int ir,isp_attempt;
    struct cr_reference reference;void (*handler[64])(int);char sent[2048];size_t sent_n;
} m;
static void put16(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void put32(uint8_t *p,uint32_t n){put16(p,n);put16(p+2,n>>16);}
static void dummy_blob(void)
{
    uint32_t base=0,pos;memset(tuning_blob,0,sizeof(tuning_blob));
    for(unsigned record=0;record<2;record++){
        put32(tuning_blob+base,3);put32(tuning_blob+base+0x14,0x1346);tuning_blob[base+0x1f]=(uint8_t)record;
        pos=base+512;
        for(unsigned i=0;i<24;i++){put16(tuning_blob+pos,i);put16(tuning_blob+pos+2,cr_block_lengths[i]);pos+=cr_block_lengths[i];}
        put16(tuning_blob+pos,0x1c);put16(tuning_blob+pos+2,560);pos+=4;
        put16(tuning_blob+pos,0x0103);put16(tuning_blob+pos+2,1);put16(tuning_blob+pos+4,0x0100);
        put16(tuning_blob+pos+556,0x0100);put16(tuning_blob+pos+558,1);base=pos+560;
    }assert(base==CR_BLOB);
}
static void stat_kind(enum kind k,struct stat *s)
{
    memset(s,0,sizeof(*s));s->st_dev=1;s->st_ino=(ino_t)k+100;s->st_uid=0;s->st_nlink=1;
    s->st_mode=S_IFCHR|0600;
    if(k==TTY)s->st_rdev=makedev(204,64);
    else if(k==ISP)s->st_rdev=makedev(10,60);
    else if(k==VIDEO)s->st_rdev=makedev(81,0);
    else if(k==ION)s->st_rdev=makedev(10,62);
    else if(k==MEM)s->st_rdev=makedev(1,1);
    else if(k==DMA_OUT || k==DMA_REF)s->st_mode=0600;
    else {s->st_mode=S_IFREG|0444;s->st_size=k==BLOB?CR_BLOB:4096;}
}
static enum kind path_kind(const char *p)
{
    if(!strcmp(p,"/proc/modules"))return MOD;
    if(!strcmp(p,CR_TUNING_PATH))return BLOB;
    if(!strcmp(p,CR_IR_PATH))return IR;
    if(!strcmp(p,"/dev/isp_char"))return ISP;
    if(!strcmp(p,"/dev/video0"))return VIDEO;
    if(!strcmp(p,"/dev/ion"))return ION;
    if(!strcmp(p,"/proc/self/pagemap"))return PM;
    if(!strcmp(p,"/dev/mem"))return MEM;
    if(!strcmp(p,"/dev/ttySAK0"))return TTY;
    assert(!"unallowlisted path");return NONE;
}
uid_t ca_getuid(void){return m.fault==NONROOT?1:0;}uid_t ca_geteuid(void){return 0;}pid_t ca_getpid(void){return 99;}
int ca_uname(struct utsname *u){memset(u,0,sizeof(*u));strcpy(u->sysname,"Linux");strcpy(u->release,m.fault==KERNEL?"6.18.55":"3.4.35");return 0;}
long ca_sysconf(int x){assert(x==_SC_PAGESIZE);return 4096;}
int ca_fcntl(int fd,int cmd,int value){assert(fd==3 && cmd==F_GETFD && !value);if(m.fault==INHERITED)return 0;errno=EBADF;return -1;}
int ca_lstat(const char *p,struct stat *s)
{
    enum kind k=path_kind(p);stat_kind(k,s);
    if(k==ISP && m.fault==NODE_KIND)s->st_mode=S_IFREG|0600;
    if(k==ISP && m.fault==NODE_ID)s->st_rdev=makedev(10,61);
    if(k==BLOB && m.fault==BLOB_UID)s->st_uid=1;
    if(k==BLOB && m.fault==BLOB_LINK)s->st_mode=S_IFLNK|0777;
    if(k==BLOB && m.fault==BLOB_SIZE)s->st_size++;
    if(k==TTY && m.fault==TTY_WRONG)s->st_rdev=makedev(5,1);
    return 0;
}
int ca_open(const char *p,int flags)
{
    enum kind k=path_kind(p);int fd;
    assert((flags&O_CLOEXEC) && (flags&O_NOFOLLOW));
    if(k==VIDEO)assert((flags&O_NONBLOCK) && (flags&O_ACCMODE)==O_RDWR);
    if(k==TTY)assert((flags&O_NONBLOCK) && (flags&O_NOCTTY) && (flags&O_ACCMODE)==O_WRONLY);
    if(k==MEM)assert((flags&O_ACCMODE)==O_RDONLY);
    for(fd=3;fd<64 && m.fd[fd];fd++){}
    assert(fd<64);m.fd[fd]=k;m.opens++;
    if(k==ISP)m.isp_attempt=1;
    return fd;
}
int ca_fstat(int fd,struct stat *s)
{
    assert(fd>=0 && fd<64 && m.fd[fd]);stat_kind(m.fd[fd],s);
    if(m.fd[fd]==ISP && m.fault==NODE_RACE)s->st_ino++;
    if(m.fd[fd]==BLOB && m.fault==BLOB_RACE && ++m.blob_stats==2)s->st_ino++;
    return 0;
}
int ca_close(int fd){assert(!m.isp_attempt);assert(fd>=3 && m.fd[fd]);m.fd[fd]=NONE;m.closes++;return 0;}
ssize_t ca_read(int fd,void *p,size_t n)
{
    if(m.fd[fd]==MOD){
        const char *text=m.fault==MODULE?"foreign 100 0 - Live 0x0\n":"akcamera 100 0 - Live 0x0\nsensor_sc1346 100 0 - Live 0x0\nak39_top_wdt 100 0 - Live 0x0\n";
        size_t len=strlen(text),off=(size_t)m.offset[fd];if(off>=len)return 0;
        size_t k=len-off;if(k>n)k=n;memcpy(p,text+off,k);m.offset[fd]+=(off_t)k;m.module_reads++;return (ssize_t)k;
    }
    assert(m.fd[fd]==IR && n==4);((char*)p)[0]=(char)('0'+m.ir);((char*)p)[1]='\n';
    if(m.fault==IR_VERIFY_BAD)((char*)p)[0]='2';
    return 2;
}
ssize_t ca_pread(int fd,void *p,size_t n,off_t offset)
{
    if(m.fd[fd]==BLOB){assert(offset>=0 && (uint64_t)offset+n<=CR_BLOB);memcpy(p,tuning_blob+(size_t)offset,n);return (ssize_t)n;}
    assert(m.fd[fd]==PM && n<=256 && !(n&7) && !(offset&7));
    uint64_t page=(uint64_t)offset/8;unsigned owner,index;
    if(page>=0x41000){owner=1;index=(unsigned)(page-0x41000);assert(index+n/8<=900);}
    else {owner=0;index=(unsigned)(page-0x40000);assert(index+n/8<=2048);}
    for(unsigned i=0;i<n/8;i++){
        uint64_t pfn=(owner?(m.fault==PFN_OVERLAP?0x80400:0x81000):0x80400)+index+i;
        if(owner && index+i==899 && m.fault==PFN_LAST_GAP)pfn++;
        if(owner && m.fault==PFN_OUTSIDE)pfn=0x81800;
        uint64_t entry=(1ULL<<63)|(12ULL<<55)|pfn;
        for(unsigned j=0;j<8;j++)((uint8_t*)p)[i*8+j]=(uint8_t)(entry>>(j*8));
        m.pages[owner]++;
    }return (ssize_t)n;
}
off_t ca_lseek(int fd,off_t off,int whence){assert(m.fd[fd]==IR && !off && whence==SEEK_SET);return 0;}
ssize_t ca_write(int fd,const void *p,size_t n)
{
    if(m.fd[fd]==IR){assert(n==2 && ((char*)p)[1]=='\n' && (((char*)p)[0]=='0'||((char*)p)[0]=='1'));
        m.ir=((char*)p)[0]-'0';return m.fault==IR_PARTIAL?1:2;}
    assert(m.fd[fd]==TTY && n<=512);m.write_calls++;
    if(m.fault==WRITE_TIMEOUT || (m.fault==WRITE_AGAIN && m.write_calls==1)){errno=EAGAIN;return -1;}
    if(m.fault==WRITE_INTR && m.write_calls==1){errno=EINTR;return -1;}
    if(m.fault==WRITE_PARTIAL && n>7)n=7;
    assert(m.sent_n+n<=sizeof(m.sent));memcpy(m.sent+m.sent_n,p,n);m.sent_n+=n;return (ssize_t)n;
}
int ca_ioctl(int fd,unsigned long request,void *p)
{
    m.ioctls++;
    if(request==0x80685600){assert(m.fd[fd]==VIDEO);memset(p,0,104);memcpy(p,"ak_camera\0",10);memcpy((uint8_t*)p+16,"AK_Camera\0",10);((uint32_t*)p)[20]=197667;((uint32_t*)p)[21]=0x4000001;return 0;}
    if(request==CR_ION_ALLOC){
        struct cr_alloc *q=p;assert(m.fd[fd]==ION && q->align==4096 && q->mask==2 && q->flags==3);
        assert(q->len==(m.allocs?CR_REF_BYTES:CR_MAP));q->handle=m.allocs?0x5678:0x1234;m.allocs++;
        if(m.allocs==2 && m.fault==REF_HANDLE_DUP)q->handle=0x1234;
        if(m.allocs==2 && m.fault==REF_LENGTH_RETURN)q->len=0x400000;
        if(m.fault==ALLOC_POS)return 1;
        if(m.fault==ALLOC_INTR){errno=EINTR;return -1;}
        if(m.fault==ALLOC_REF_FAIL && m.allocs==2){errno=ENOMEM;return -1;}
        if(m.fault==IO_CANARY)((uint32_t*)p)[5]=0;
        return 0;
    }
    if(request==CR_ION_MAP){
        struct cr_export *q=p;assert(m.fd[fd]==ION);int out;
        for(out=3;out<64 && m.fd[out];out++){}
        assert(out<64);
        m.fd[out]=q->handle==0x1234?DMA_OUT:DMA_REF;q->fd=out;
        if(m.fault==EXPORT_ALIAS)q->fd=fd;
        if(q->handle==0x5678 && m.fault==REF_EXPORT_FAIL){errno=EINTR;return -1;}
        if(q->handle==0x5678 && m.fault==REF_EXPORT_ALIAS){
            for(int old=3;old<out;old++)if(m.fd[old]==DMA_OUT){q->fd=old;break;}
        }
        return 0;
    }
    if(request==CR_REFERENCE_SET){assert(m.fd[fd]==ISP);memcpy(&m.reference,p,24);m.ref_set++;return m.fault==REF_SET_POS?1:0;}
    if(request==CR_REFERENCE_GET){assert(m.fd[fd]==ISP);memcpy(p,&m.reference,24);m.ref_get++;
        if(m.fault==REF_GET_BAD_ADDR)((uint32_t*)p)[2]+=4096;
        if(m.fault==REF_GET_BAD_LENGTH)((uint32_t*)p)[3]=0x96001;
        return 0;}
    if(request==CR_SENSOR_LOAD){struct cr_sensor *q=p;assert(m.fd[fd]==ISP && q->count==140 && q->ptr==0x100085fa);return 0;}
    if(request==CR_S_FMT || request==CR_S_PARM || request==CR_S_CROP){assert(m.fd[fd]==VIDEO);return 0;}
    if(request==CR_G_FMT){struct cr_format *q=p;memset(q,0,204);q->type=1;uint32_t w=m.fault==GEOM_BAD?640:1280,h=720;memcpy(q->data,&w,4);memcpy(q->data+4,&h,4);return 0;}
    if(request==CR_G_CROP){*(struct cr_crop*)p=(struct cr_crop){1,0,0,1280,720};return 0;}
    if(request==CR_SENSOR_GET){((struct cr_sensor_read*)p)->value=1;return 0;}
    if(request==CR_QBUF){struct cr_buffer *q=p;assert(q->type==1 && q->memory==2 && q->length==CR_SLOT && q->index<4 && q->userptr==0x40000000+q->index*CR_SLOT);return 0;}
    if(request==CR_REQBUFS){struct cr_reqbuf *q=p;assert(q->count==4 && q->type==1 && q->memory==2);return 0;}
    if(request==CR_STREAMON || request==CR_STREAMOFF){assert(*(uint32_t*)p==1);return 0;}
    if(request==CR_DQBUF){errno=EAGAIN;return -1;}
    for(unsigned i=0;i<34;i++)if(request==cr_bindings[i].request)return 0;
    assert(!"unallowlisted ioctl");return -1;
}
void *ca_mmap(void *addr,size_t n,int prot,int flags,int fd,off_t off)
{
    assert(!addr && flags==MAP_SHARED);m.maps++;
    if(m.fd[fd]==MEM){assert(n==4096 && prot==PROT_READ && off==(off_t)0x20000000);control[0]=m.fault==CTRL_ACTIVE?1U<<20:0;return control;}
    assert(!off && prot==(PROT_READ|PROT_WRITE));
    if(m.fd[fd]==DMA_OUT){assert(n==CR_MAP);return output_mem;}
    assert(m.fd[fd]==DMA_REF && n==CR_REF_BYTES);
    if(m.fault==MAP_REF_FAIL){errno=ENOMEM;return MAP_FAILED;}return ref_mem;
}
uint32_t ca_mock_va(const void *p){if(p==output_mem)return 0x40000000;if(p==ref_mem)return 0x41000000;assert(!"unexpected map");return 0;}
int ca_poll(struct pollfd *p,nfds_t n,int timeout){assert(n==1 && timeout==100);m.ms+=100;p->revents=p->events;return 1;}
int ca_clock_gettime(clockid_t id,struct timespec *t){assert(id==CLOCK_MONOTONIC);t->tv_sec=(time_t)(m.ms/1000);t->tv_nsec=(long)(m.ms%1000*1000000);return 0;}
int ca_sigaction(int sig,const struct sigaction *a0,struct sigaction *old){assert(!old && sig<64);m.handler[sig]=a0->sa_handler;return 0;}
int ca_sigprocmask(int how,const sigset_t *s,sigset_t *old){assert(how==SIG_UNBLOCK && s && !old);return 0;}
unsigned ca_alarm(unsigned n){assert(n==0 || n==25);return 0;}
int ca_kill(pid_t pid,int sig){assert(pid==99 && sig==SIGSTOP);m.stop++;return 0;}int ca_pause(void){assert(!"host pause");return -1;}
static void setup(enum fault f)
{
    memset(&m,0,sizeof(m));m.fault=f;m.ms=1000;m.fd[0]=m.fd[1]=m.fd[2]=TTY;
    memset(&core,0,sizeof(core));cr_adapter_bind(&a,&core,&ops);
    a.fixture_instance_sealed=a.fixture_clock_admitted=a.fixture_order_sealed=1;
    if(f==UNADMITTED)a.fixture_clock_admitted=0;
    assert(!mbedtls_sha256(tuning_blob,sizeof(tuning_blob),ops.synthetic_sha,0));
}
static int call(enum cr_op op,void *p){return ops.call(ops.opaque,op,p);}
static int owner(int ref)
{
    struct cr_alloc q={ref?CR_REF_BYTES:CR_MAP,4096,2,3,0};struct cr_export e={0,-1};struct cr_mapping map;
    if(call(ref?CR_REF_ALLOC:CR_ALLOC,&q))return -1;
    e.handle=q.handle;
    if(call(ref?CR_REF_EXPORT:CR_EXPORT,&e) || call(ref?CR_REF_MAP_RW:CR_MAP_RW,&map))return -1;
    for(unsigned i=0;i<(ref?900U:2048U);i+=32){struct cr_pages p={0};p.first=i;p.count=(ref?900U:2048U)-i;if(p.count>32)p.count=32;
        if(call(ref?CR_REF_PFN_CHUNK:CR_PFN_CHUNK,&p))return -1;}
    memset(map.data,ref?0:0xa5,map.length);return 0;
}
static int prepare(void)
{
    struct cr_entry e;int level=-1;struct cr_file file={core.blob,CR_BLOB,0,0,CR_TUNING_PATH};
    if(call(CR_ENTRY,&e) || !e.source_clock_admitted)return -1;
    if(call(CR_READ_TUNING,&file) || call(CR_IR_BASELINE,&level) || level!=0)return -1;
    if(call(CR_OPEN_ISP,0) || call(CR_OPEN_VIDEO,0) || owner(0) || owner(1))return -1;
    if(cr_make_reference(a.output.first_pfn,a.reference.first_pfn,&core.reference))return -1;
    return call(CR_REF_INSTALL,&core.reference);
}
static int geometry(void)
{
    struct cr_format f={1,{0}};struct cr_parm p={1,{0}};struct cr_geometry g={0};
    put32(f.data,1280);put32(f.data+4,720);put32(f.data+8,0x56595559);put32(f.data+12,1);
    put32(p.data,1);put32(p.data+4,640);put32(p.data+8,360);
    if(call(CR_SET_MAIN,&f) || call(CR_SET_SUB,&p) || call(CR_GEOMETRY,&g))return -1;
    assert(g.main_w==1280 && g.main_h==720 && g.sub_w==640 && g.sub_h==360 && g.sub_evidence==1);
    return 0;
}
int main(int argc,char **argv)
{
    unsigned cases=0;
    if(argc!=2 || strcmp(argv[1],"--host-adapter-synthetic-only"))return 2;
    dummy_blob();
    setup(OK);assert(!prepare());cases++;
    assert(m.pages[0]==2048 && m.pages[1]==900 && m.reference.words[0]==0x81000000 && m.reference.words[1]==0x258000);
    assert(call(CR_PRE_SENSOR_CHECK,&core.reference));assert(!geometry());assert(!call(CR_PRE_SENSOR_CHECK,&core.reference));
    {struct cr_load l={{140,0x100085fa},core.blob+0x85fa,560};assert(!call(CR_LOAD_SENSOR,&l));l.wire.count=139;assert(call(CR_LOAD_SENSOR,&l));}
    {struct cr_reference bad=core.reference;bad.words[3]++;assert(call(CR_REF_INSTALL,&bad));bad=core.reference;bad.words[0]+=4096;assert(call(CR_REF_INSTALL,&bad));}
    {struct cr_sensor_read r={0x0100,0xffff};assert(!call(CR_READ_SENSOR,&r));r.reg=0x1234;assert(call(CR_READ_SENSOR,&r));}
    {struct cr_buffer b={0};b.type=1;b.memory=2;b.length=CR_SLOT;b.userptr=0x40000000;assert(!call(CR_QUEUE,&b));}
    for(enum fault f=INHERITED;f<=CTRL_ACTIVE;f++){
        setup(f);assert(prepare()!=0);cases++;
        if(f==UNADMITTED)assert(!m.opens && !m.ioctls);
        if(a.retained){unsigned closes=m.closes;ops.retained_stop(ops.opaque);assert(m.stop==1 && m.closes==closes);}
    }
    for(enum fault f=WRITE_PARTIAL;f<=WRITE_TIMEOUT;f++){
        setup(f);assert(!prepare());int rc=ops.emit(ops.opaque,"fixed-public-packet",19);
        if(f==WRITE_TIMEOUT)assert(rc!=0 && m.ms<=11000);else assert(rc==0 && m.sent_n==19 && !memcmp(m.sent,"fixed-public-packet",19));
        cases++;
    }
    for(enum fault f=REF_EXPORT_FAIL;f<=REF_LENGTH_RETURN;f++){
        setup(f);assert(prepare()!=0 && a.retained);unsigned closes=m.closes;
        ops.retained_stop(ops.opaque);assert(m.stop==1 && m.closes==closes && a.output.handle);cases++;
    }
    setup(TTY_WRONG);assert(!prepare() && ops.emit(ops.opaque,"x",1));cases++;
    setup(IR_PARTIAL);assert(!prepare());{int v=1;assert(call(CR_IR_SET,&v));}cases++;
    setup(IR_VERIFY_BAD);{struct cr_entry e;int v;assert(!call(CR_ENTRY,&e) && call(CR_IR_BASELINE,&v));}cases++;
    setup(OK);assert(!prepare());m.handler[SIGTERM](SIGTERM);assert(ops.cancelled(ops.opaque));
    {int v=0;assert(!call(CR_IR_SET,&v));assert(!call(CR_IR_VERIFY,&v) && v==0);}
    ops.retained_stop(ops.opaque);assert(m.stop==1 && a.output.handle && a.reference.handle && a.isp>=3 && a.video>=3);cases++;
    setup(GEOM_BAD);assert(!prepare() && geometry());cases++;
    setup(OK);assert(cr_run(&core,&ops)==125 && core.reason==25 && m.stop==1);
    assert(a.output.pages_seen==2048 && a.reference.pages_seen==900 && a.reference_installed && a.pre_sensor_checked);
    assert(core.ir_restored && a.output.handle && a.reference.handle && m.ir==0);cases++;
    printf("ADAPTER_SYNTHETIC_PASS cases=%u output_pages=2048 reference_pages=900 total_bytes=12075008 no_real_device_syscalls=true native_HOLD=true\n",cases);
    return 0;
}
