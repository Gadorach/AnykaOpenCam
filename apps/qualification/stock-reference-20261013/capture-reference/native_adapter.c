/* Fixed syscall adapter for one retained reference client. Native entry is HOLD.
 * No generic allocator, user paths/requests, module load, ION_FREE or munmap.
 * Runtime stock-image, source-clock and complete setup-order seals are NOT
 * manufactured from uname/PFNs/metadata. Only explicit host fixtures admit them.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "native_adapter.h"
#include "adapter_sys.h"

static volatile sig_atomic_t signal_seen;
static void cancel_signal(int sig){if(!signal_seen)signal_seen=sig;}
static int fail(int e){errno=e;return -1;}
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t get64(const uint8_t *p){return (uint64_t)get32(p)|(uint64_t)get32(p+4)<<32;}
static int same(const struct stat *x,const struct stat *y)
{
    return x->st_dev==y->st_dev && x->st_ino==y->st_ino && x->st_rdev==y->st_rdev &&
           x->st_mode==y->st_mode && x->st_uid==y->st_uid;
}
static int character(const struct stat *s,unsigned major_id,unsigned minor_id)
{
    return S_ISCHR(s->st_mode) && s->st_uid==0 && major(s->st_rdev)==major_id && minor(s->st_rdev)==minor_id;
}
static int node(const char *path,int flags,unsigned maj,unsigned min,int *fd)
{
    struct stat pre,post;
    if(ca_lstat(path,&pre) || !character(&pre,maj,min))return fail(ENODEV);
    *fd=ca_open(path,flags|O_CLOEXEC|O_NOFOLLOW);
    if(*fd<3)return fail(EBADF); /* retain any obtained fd on identity failure */
    if(ca_fstat(*fd,&post) || !same(&pre,&post))return fail(ENODEV);
    return 0;
}
static int checked_ioctl(int fd,unsigned long request,void *packet,size_t bytes)
{
    uint32_t bounce[54];int rc;const uint32_t guard=0x937ea512U;
    if(bytes>204 || (bytes&3))return fail(EINVAL);
    memset(bounce,0,sizeof(bounce));bounce[0]=guard;bounce[bytes/4+1]=guard;
    memcpy(bounce+1,packet,bytes);rc=ca_ioctl(fd,request,bounce+1);
    memcpy(packet,bounce+1,bytes); /* retain returned handle/fd, even on error */
    if(bounce[0]!=guard || bounce[bytes/4+1]!=guard)return fail(EFAULT);
    if(rc!=0)return fail(rc>0?EPROTO:errno);
    return 0;
}
int cr_adapter_clock(void *ctx,uint64_t *out)
{
    struct cr_adapter *a=ctx;struct timespec t;
    if(ca_clock_gettime(CLOCK_MONOTONIC,&t) || t.tv_sec<0 || t.tv_nsec<0 || t.tv_nsec>=1000000000L ||
       (uint64_t)t.tv_sec>UINT64_MAX/1000-1)return -1;
    *out=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;
    if(*out<a->last_ms)return fail(ERANGE);
    a->last_ms=*out;return 0;
}
int cr_adapter_cancelled(void *ctx){(void)ctx;return signal_seen!=0;}
static int read_exact(int fd,void *dst,size_t n,off_t offset)
{
    size_t done=0;unsigned retries=0,calls=0;
    while(done<n && calls++<140){size_t count=n-done;if(count>512)count=512;
        ssize_t k=ca_pread(fd,(uint8_t*)dst+done,count,offset+(off_t)done);
        if(k<0 && errno==EINTR && retries++==0)continue;
        if(k<=0 || (size_t)k>count)return fail(EIO);
        done+=(size_t)k;
    }return done==n?0:fail(EIO);
}
static int modules(void)
{
    char data[2049];int fd=ca_open("/proc/modules",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);size_t n=0;unsigned mask=0,lines=0;
    if(fd<3)return -1;
    for(unsigned calls=0;calls<8;calls++){
        ssize_t got=ca_read(fd,data+n,sizeof(data)-1-n);
        if(got<0){(void)ca_close(fd);return -1;}
        if(got==0)break;
        if((size_t)got>sizeof(data)-1-n){(void)ca_close(fd);return fail(EOVERFLOW);}
        n+=(size_t)got;if(n==sizeof(data)-1){(void)ca_close(fd);return fail(EOVERFLOW);}
    }
    if(ca_close(fd))return -1;
    data[n]=0;
    for(char *p=data;*p;){char *end=strchr(p,'\n'),*space=strchr(p,' ');unsigned bit;
        if(!end || !space || space>=end)return fail(EPROTO);
        size_t len=(size_t)(space-p);
        if(len==8 && !memcmp(p,"akcamera",8))bit=1;
        else if(len==13 && !memcmp(p,"sensor_sc1346",13))bit=2;
        else if(len==12 && !memcmp(p,"ak39_top_wdt",12))bit=4;
        else return fail(ENODEV);
        if(mask&bit)return fail(EPROTO);
        mask|=bit;lines++;p=end+1;
    }
    return mask==7 && lines==3?0:fail(ENODEV); /* inventory, NOT a hash proof */
}
static int entry(struct cr_adapter *a,struct cr_entry *e)
{
    struct utsname u;struct sigaction sa;sigset_t mask;uint64_t now;
    memset(e,0,sizeof(*e));
    if(ca_fcntl(3,F_GETFD,0)!=-1 || errno!=EBADF)return fail(EBADF);
    e->fd3_closed=1;
    /* No timing assumption or launch-enabling native switch. The native main
     * also refuses before bind/run. Missing proof rejects BEFORE file/device I/O. */
#ifndef CR_ADAPTER_MOCK
    return fail(ENOTSUP);
#else
    if(!a->fixture_instance_sealed || !a->fixture_clock_admitted || !a->fixture_order_sealed)return fail(ENOTSUP);
#endif
    if(ca_getuid() || ca_geteuid() || ca_getpid()==1 || ca_uname(&u) || strcmp(u.sysname,"Linux") ||
       strcmp(u.release,"3.4.35") || ca_sysconf(_SC_PAGESIZE)!=4096)return fail(ENODEV);
    if(modules())return -1;
    memset(&sa,0,sizeof(sa));sa.sa_handler=cancel_signal;
    if(sigemptyset(&sa.sa_mask) || ca_sigaction(SIGALRM,&sa,0) || ca_sigaction(SIGINT,&sa,0) || ca_sigaction(SIGTERM,&sa,0) ||
       sigemptyset(&mask) || sigaddset(&mask,SIGALRM) || sigaddset(&mask,SIGINT) || sigaddset(&mask,SIGTERM) ||
       ca_sigprocmask(SIG_UNBLOCK,&mask,0) || cr_adapter_clock(a,&now))return -1;
    a->start_ms=now;(void)ca_alarm(25);a->entered=1;
    e->root_child=e->stock_instance_pins=e->source_clock_admitted=1;
#ifdef CR_ADAPTER_MOCK
    e->source_clock_error_us=a->fixture_clock_error_us;
#endif
    return 0;
}
static int tuning(struct cr_file *f)
{
    struct stat pre,post,last;int fd,rc;
    if(!f || f->capacity!=CR_BLOB || !f->path || strcmp(f->path,CR_TUNING_PATH))return fail(EINVAL);
    if(ca_lstat(CR_TUNING_PATH,&pre) || !S_ISREG(pre.st_mode) || pre.st_uid!=0 || pre.st_size!=CR_BLOB)return fail(EPERM);
    fd=ca_open(CR_TUNING_PATH,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<3)return -1;
    rc=ca_fstat(fd,&post);
    if(!rc && (!same(&pre,&post) || post.st_size!=CR_BLOB))rc=-1;
    if(!rc)rc=read_exact(fd,f->data,CR_BLOB,0);
    if(!rc)rc=ca_fstat(fd,&last);
    if(!rc && (!same(&post,&last) || last.st_size!=CR_BLOB ||
       last.st_mtim.tv_sec!=post.st_mtim.tv_sec || last.st_mtim.tv_nsec!=post.st_mtim.tv_nsec ||
       last.st_ctim.tv_sec!=post.st_ctim.tv_sec || last.st_ctim.tv_nsec!=post.st_ctim.tv_nsec))rc=-1;
    if(ca_close(fd))rc=-1;
    if(rc)return fail(EIO);
    f->length=CR_BLOB;f->root_regular_nofollow_same_inode=1;return 0; /* SHA in core before ISP open */
}
static int ir_open(struct cr_adapter *a)
{
    struct stat pre,post;
    if(a->ir>=0)return 0;
    if(ca_lstat(CR_IR_PATH,&pre) || !S_ISREG(pre.st_mode) || pre.st_uid!=0)return fail(ENODEV);
    a->ir=ca_open(CR_IR_PATH,O_RDWR|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
    if(a->ir<3 || ca_fstat(a->ir,&post) || !same(&pre,&post))return fail(ENODEV);
    return 0;
}
static int ir_read(struct cr_adapter *a,int *value)
{
    char text[4];ssize_t n;
    if(ir_open(a) || ca_lseek(a->ir,0,SEEK_SET)!=0)return -1;
    n=ca_read(a->ir,text,sizeof(text));
    if(n!=2 || text[1]!='\n' || (text[0]!='0' && text[0]!='1'))return fail(EPROTO);
    *value=text[0]-'0';return 0;
}
static int alloc_owner(struct cr_adapter *a,struct cr_owner *o,struct cr_alloc *p,uint32_t size)
{
    int rc;
    if(p->len!=size || p->align!=4096 || p->mask!=2 || p->flags!=3 || o->handle)return fail(EINVAL);
    if(a->ion<0 && node("/dev/ion",O_RDONLY,10,62,&a->ion))return -1;
    rc=checked_ioctl(a->ion,CR_ION_ALLOC,p,20);o->handle=p->handle;o->bytes=size;
    if(rc || !p->handle || p->handle>=0xfffff001U || p->len!=size || p->align!=4096 || p->mask!=2 || p->flags!=3)return fail(EPROTO);
    if(o==&a->reference && o->handle==a->output.handle)return fail(EPROTO);
    return 0;
}
static int export_owner(struct cr_adapter *a,struct cr_owner *o,struct cr_export *p)
{
    struct stat st;int rc;
    if(!o->handle || p->handle!=o->handle)return fail(EINVAL);
    rc=checked_ioctl(a->ion,CR_ION_MAP,p,8);o->dma=p->fd;
    if(rc || p->handle!=o->handle || p->fd<3 || p->fd==a->ion || p->fd==a->isp || p->fd==a->video ||
       p->fd==a->ir || p->fd==a->pm || p->fd==a->mem || p->fd==a->tty ||
       (o==&a->reference && p->fd==a->output.dma) || ca_fstat(p->fd,&st) || (st.st_mode&0170000)!=0)return fail(EPROTO);
    return 0;
}
static int map_owner(struct cr_owner *o,struct cr_mapping *m)
{
    uintptr_t va;
    if(o->dma<3 || o->mapping)return fail(EINVAL);
    o->mapping=ca_mmap(0,o->bytes,PROT_READ|PROT_WRITE,MAP_SHARED,o->dma,0);
    if(o->mapping==MAP_FAILED)return -1;
#ifdef CR_ADAPTER_MOCK
    va=ca_mock_va(o->mapping);
#else
    va=(uintptr_t)o->mapping;
#endif
    if(!va || (va&4095) || va>UINT32_MAX-o->bytes)return fail(ERANGE);
    o->user_base=(uint32_t)va;*m=(struct cr_mapping){o->mapping,o->user_base,o->bytes};return 0;
}
static int pages(struct cr_adapter *a,struct cr_owner *o,struct cr_pages *p)
{
    uint8_t raw[256];uint32_t last=o->last_pfn;unsigned i;
    if(!o->user_base || p->first!=o->pages_seen || !p->count || p->count>32 || p->first+p->count>o->bytes/4096)return fail(EINVAL);
    if(a->pm<0){a->pm=ca_open("/proc/self/pagemap",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(a->pm<3)return -1;}
    if(read_exact(a->pm,raw,p->count*8,(off_t)(o->user_base/4096+p->first)*8))return -1;
    for(i=0;i<p->count;i++){
        p->entries[i]=get64(raw+i*8);
        if(cr_validate_pfn(p->entries[i],p->first+i,last,&last))return fail(ERANGE);
        if(!p->first && !i)o->first_pfn=last;
    }
    o->last_pfn=last;o->pages_seen+=p->count;return 0;
}
static int gate_off(struct cr_adapter *a)
{
    if(a->isp<3 || a->video<3)return fail(EPERM);
    if(!a->control){
        if(node("/dev/mem",O_RDONLY|O_SYNC,1,1,&a->mem))return -1;
        void *p=ca_mmap(0,4096,PROT_READ,MAP_SHARED,a->mem,(off_t)CR_ISP_PHYSICAL_BASE);
        if(p==MAP_FAILED || !p)return fail(ENOMEM);
        a->control=p;
    }
    /* EXACT fixed word, physical20000000. No other register, hash or MMIO write. */
    return (*a->control&CR_ISP_CAPTURE_GATE)?fail(EBUSY):0;
}
static int reference_check(struct cr_adapter *a,const struct cr_reference *p,int install)
{
    struct cr_reference expected,got;int rc;
    if(a->output.pages_seen!=2048 || a->reference.pages_seen!=900 ||
       cr_make_reference(a->output.first_pfn,a->reference.first_pfn,&expected) || memcmp(p,&expected,24))return fail(ERANGE);
    if(install){
        if(gate_off(a))return -1;
        got=expected;
        rc=checked_ioctl(a->isp,CR_REFERENCE_SET,&got,24);
        if(rc || memcmp(&got,&expected,24))return fail(EPROTO);
    }
    memset(&got,0xa5,sizeof(got));
    if(checked_ioctl(a->isp,CR_REFERENCE_GET,&got,24) || memcmp(&got,&expected,24))return fail(EPROTO);
    a->installed=expected;a->reference_installed=1;return 0;
}
int cr_adapter_call(void *ctx,enum cr_op op,void *arg)
{
    struct cr_adapter *a=ctx;
    if(op==CR_ENTRY)return entry(a,arg);
    if(!a->entered)return fail(EPERM);
    switch(op){
    case CR_READ_TUNING:return a->retained?fail(EPERM):tuning(arg);
    case CR_IR_BASELINE:case CR_IR_VERIFY:return ir_read(a,arg);
    case CR_OPEN_ISP:a->retained=1;return node("/dev/isp_char",O_RDWR,10,60,&a->isp);
    case CR_OPEN_VIDEO: {
        uint32_t cap[26]={0};
        if(a->isp<3 || node("/dev/video0",O_RDWR|O_NONBLOCK,81,0,&a->video))return -1;
        if(checked_ioctl(a->video,0x80685600,cap,104))return -1;
        if(memcmp(cap,"ak_camera\0",10) || memcmp((uint8_t*)cap+16,"AK_Camera\0",10) ||
           cap[20]!=197667 || (cap[21]&0x4000001)!=0x4000001)return fail(ENODEV);
        return gate_off(a);}
    case CR_ALLOC:return alloc_owner(a,&a->output,arg,CR_MAP);
    case CR_EXPORT:return export_owner(a,&a->output,arg);
    case CR_MAP_RW:return map_owner(&a->output,arg);
    case CR_PFN_CHUNK:return pages(a,&a->output,arg);
    case CR_REF_ALLOC:return alloc_owner(a,&a->reference,arg,CR_REF_BYTES);
    case CR_REF_EXPORT:return export_owner(a,&a->reference,arg);
    case CR_REF_MAP_RW:return map_owner(&a->reference,arg);
    case CR_REF_PFN_CHUNK:return pages(a,&a->reference,arg);
    case CR_TUNING_BLOCK: {const struct cr_block *b=arg;
        return b->id<24 && b->offset==a->core->block_offsets[b->id] && b->length==cr_block_lengths[b->id] &&
               b->data==a->core->blob+b->offset?0:fail(EINVAL);} /* metadata only */
    case CR_ISP_SET: {
        struct cr_set *p=arg;unsigned i;
        for(i=0;i<34;i++){const struct cr_binding *b=&cr_bindings[i];
            if(p->request==b->request && p->bytes==b->bytes && p->data==a->core->blob+a->core->block_offsets[b->block]+b->offset)break;}
        if(i==34 || gate_off(a))return fail(EINVAL);
        /* Proven SET-only source span. No modern IOC-size4 assumption/copy. */
        return ca_ioctl(a->isp,p->request,(void*)p->data)==0?0:fail(EPROTO);}
    case CR_REF_INSTALL:return reference_check(a,arg,1);
    case CR_PRE_SENSOR_CHECK:
        if(!a->reference_installed || !a->geometry_checked || reference_check(a,arg,0) || gate_off(a))return fail(EPERM);
        a->pre_sensor_checked=1;return 0;
    case CR_LOAD_SENSOR: {
        struct cr_load *l=arg;struct cr_sensor packet=l->wire;
        if(l->bytes!=560 || l->wire.count!=140 || l->payload!=a->core->blob+0x85fa ||
           !a->reference_installed || !a->pre_sensor_checked || gate_off(a))return fail(EINVAL);
#ifdef CR_ADAPTER_MOCK
        if(packet.ptr!=0x100085fa)return fail(EINVAL);
#else
        if(packet.ptr!=(uint32_t)(uintptr_t)l->payload)return fail(EINVAL);
#endif
        return checked_ioctl(a->isp,CR_SENSOR_LOAD,&packet,8) || memcmp(&packet,&l->wire,8)?fail(EPROTO):0;}
    case CR_SET_MAIN: {
        struct cr_format *f=arg;struct cr_crop crop={1,0,0,1280,720};
        if(f->type!=1 || get32(f->data)!=1280 || get32(f->data+4)!=720 || get32(f->data+8)!=0x56595559 || gate_off(a))return fail(EINVAL);
        if(checked_ioctl(a->video,CR_S_CROP,&crop,20) || checked_ioctl(a->video,CR_S_FMT,f,204))return -1;
        a->main_set=1;
        return reference_check(a,&a->installed,0);}
    case CR_SET_SUB: {
        struct cr_parm *p=arg;
        if(p->type!=1 || get32(p->data)!=1 || get32(p->data+4)!=640 || get32(p->data+8)!=360 || gate_off(a))return fail(EINVAL);
        if(checked_ioctl(a->video,CR_S_PARM,p,204))return -1;
        a->sub_set=1;
        return reference_check(a,&a->installed,0);}
    case CR_GEOMETRY: {
        struct cr_format f={1,{0}};struct cr_crop c={1,0,0,0,0};
        if(!a->main_set || !a->sub_set || checked_ioctl(a->video,CR_G_FMT,&f,204) || checked_ioctl(a->video,CR_G_CROP,&c,20))return -1;
        if(f.type!=1 || get32(f.data)!=1280 || get32(f.data+4)!=720 || c.type!=1 || c.left || c.top || c.width!=1280 || c.height!=720)return fail(EPROTO);
        /* Parent proposal01: exact SET writes cached640x360 fields. There is
         * no recovered G_PARM operation. Label source-set evidence explicitly. */
        *(struct cr_geometry*)arg=(struct cr_geometry){1280,720,640,360,1};
        if(reference_check(a,&a->installed,0) || gate_off(a))return -1;
        a->geometry_checked=1;return 0;}
    case CR_REQUEST: {
        struct cr_reqbuf *r=arg;
        if(r->count!=4 || r->type!=1 || r->memory!=2 || gate_off(a))return fail(EINVAL);
        return checked_ioctl(a->video,CR_REQBUFS,r,20);}
    case CR_QUEUE: {
        struct cr_buffer *b=arg;
        if(b->index>=4 || b->type!=1 || b->memory!=2 || b->userptr!=a->output.user_base+b->index*CR_SLOT || b->length!=CR_SLOT)return fail(EINVAL);
        return checked_ioctl(a->video,CR_QBUF,b,68);}
    case CR_START:case CR_STOP_STREAM:
        if(*(uint32_t*)arg!=1)return fail(EINVAL);
        return checked_ioctl(a->video,op==CR_START?CR_STREAMON:CR_STREAMOFF,arg,4);
    case CR_POLL: {
        struct pollfd p={a->video,POLLIN,0};int rc;
        if(*(int*)arg!=100)return fail(EINVAL);
        rc=ca_poll(&p,1,100);
        return rc<0 || (p.revents&(POLLERR|POLLHUP|POLLNVAL))?fail(EIO):0;}
    case CR_DEQUEUE:return checked_ioctl(a->video,CR_DQBUF,arg,68);
    case CR_READ_SENSOR: {
        struct cr_sensor_read *r=arg;unsigned i;uint16_t reg=r->reg;
        for(i=0;i<CR_REGS;i++)if(reg==cr_sensor_regs[i])break;
        if(i==CR_REGS)return fail(EINVAL);
        if(checked_ioctl(a->isp,CR_SENSOR_GET,r,4) || r->reg!=reg || r->value>255)return fail(EPROTO);
        return 0;}
    case CR_IR_SET: {
        int v=*(int*)arg;char text[2]={(char)('0'+v),'\n'};
        if((v!=0 && v!=1) || a->ir<3 || ca_lseek(a->ir,0,SEEK_SET)!=0)return fail(EINVAL);
        return ca_write(a->ir,text,2)==2?0:fail(EIO); /* no partial store continuation */}
    default:return fail(EINVAL);
    }
}
int cr_adapter_emit(void *ctx,const char *p,size_t n)
{
    struct cr_adapter *a=ctx;uint64_t now;size_t done=0;unsigned calls=0;
    if(n>512 || n>CR_OUTPUT-a->emit_bytes || cr_adapter_clock(a,&now) || now-a->start_ms>=40000)return -1;
    if(!a->emit_end_ms)a->emit_end_ms=now+10000;
    if(a->tty<0){
        struct stat before,after,stdout_info;
        if(ca_lstat("/dev/ttySAK0",&before) || ca_fstat(1,&stdout_info) || !S_ISCHR(before.st_mode) || before.st_uid ||
           !S_ISCHR(stdout_info.st_mode) || before.st_rdev!=stdout_info.st_rdev)return fail(ENODEV);
        a->tty=ca_open("/dev/ttySAK0",O_WRONLY|O_NONBLOCK|O_NOCTTY|O_CLOEXEC|O_NOFOLLOW);
        if(a->tty<3 || ca_fstat(a->tty,&after) || !same(&before,&after))return fail(ENODEV);
    }
    while(done<n && calls++<128){ssize_t k;
        if(cr_adapter_clock(a,&now) || now>=a->emit_end_ms || now-a->start_ms>=40000)return fail(ETIMEDOUT);
        k=ca_write(a->tty,p+done,n-done);
        if(k>0 && (size_t)k<=n-done){done+=(size_t)k;a->emit_bytes+=(uint32_t)k;continue;}
        if(k<0 && (errno==EINTR || errno==EAGAIN)){
            struct pollfd wait={a->tty,POLLOUT,0};int rc=ca_poll(&wait,1,100);
            if(rc<0 && errno!=EINTR)return -1;
            if(wait.revents&(POLLERR|POLLHUP|POLLNVAL))return fail(EIO);
            continue;
        }return fail(EIO);
    }return done==n?0:fail(ETIMEDOUT);
}
void cr_adapter_stop(void *ctx)
{
    (void)ctx;(void)ca_alarm(0);
#ifdef CR_ADAPTER_MOCK
    (void)ca_kill(ca_getpid(),SIGSTOP); /* test returns to inspect retained owners */
#else
    for(;;){(void)ca_kill(ca_getpid(),SIGSTOP);(void)ca_pause();}
#endif
}
void cr_adapter_bind(struct cr_adapter *a,struct cr_state *s,struct cr_ops *o)
{
    memset(a,0,sizeof(*a));memset(o,0,sizeof(*o));signal_seen=0;a->core=s;
    a->isp=a->video=a->ion=a->ir=a->pm=a->mem=a->tty=-1;a->output.dma=a->reference.dma=-1;
    o->opaque=a;o->call=cr_adapter_call;o->clock_ms=cr_adapter_clock;o->cancelled=cr_adapter_cancelled;
    o->emit=cr_adapter_emit;o->retained_stop=cr_adapter_stop;
}
