/* Fixed finite qualification candidate. NO_NATIVE_LAUNCH; see README.
 * Fixed syscall adapter is separate; native entry remains unconditionally HOLD.
 * No whole frame copies, frame files, allocator release or general command input.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include "capture.h"
const uint16_t cr_block_lengths[24]={186,192,876,2268,1088,86,22,876,136,106,9966,11096,206,64,6,14,4188,260,46,528,240,30,306,996};
const uint16_t cr_sensor_regs[CR_REGS]={0x0100,0x320c,0x320d,0x320e,0x320f,0x3e00,0x3e01,0x3e02,0x3e06,0x3e07,0x3e08,0x3e09};
/* Parent exact-copy-extent review01. IOC encoded size4 is NOT payload size.
 * Block16 sdpc(+88) is a stock success NOOP and has no entry/request here. */
const struct cr_binding cr_bindings[34]={
 {0,4,182,0x40044902},{1,4,188,0x40044904},{2,4,872,0x4004490a},
 {3,4,1782,0x4004490c},{3,1786,482,0x4004492b},{4,4,1084,0x4004492f},
 {5,4,82,0x40044906},{6,4,18,0x4004490e},{7,4,872,0x4004491a},
 {8,4,132,0x40044912},{9,4,102,0x40044936},{10,4,9962,0x4004491b},
 {11,4,11062,0x40044925},{11,11066,30,0x40044927},{12,4,202,0x4004493b},
 {13,4,60,0x40044939},{14,4,2,0x4004493f},{15,4,10,0x40044941},
 {16,4,82,0x40044910},{17,4,256,0x4004495e},{18,4,42,0x40044960},
 {19,4,2,0x40044953},{19,6,12,0x40044963},{19,18,184,0x40044955},
 {19,204,324,0x40044973},{20,4,2,0x40044943},{20,6,2,0x40044946},
 {20,8,2,0x40044949},{20,10,2,0x4004494c},{20,12,24,0x4004494e},
 {20,36,204,0x40044950},{21,4,26,0x4004496e},{22,4,302,0x40044975},
 {23,4,992,0x40044977}
};
#ifndef CR_HOST
static const uint8_t original_sha[32]={0x71,0x9e,0x84,0xd0,0x09,0x38,0x87,0xd1,0x0c,0x81,0x13,0x9b,0xab,0xdc,0x94,0x10,0x60,0x2c,0x38,0x4d,0x18,0x74,0xc6,0x4e,0x23,0x38,0xc1,0xa6,0x9c,0x52,0x93,0x32};
#endif
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t u32(const uint8_t *p){return (uint32_t)u16(p)|((uint32_t)u16(p+2)<<16);}
static void put32(uint8_t *p,uint32_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);}
int cr_entry_allowed(int argc,char **argv,int fd3_closed)
{
    return argc==2 && argv && argv[0] && argv[0][0]=='/' && argv[1] &&
        !strcmp(argv[1],"--native-stock-capture-reference") && fd3_closed;
}
int cr_validate_blob(const uint8_t *p,size_t n,const uint8_t hash[32],uint32_t offsets[24])
{
    uint8_t h[32]; uint32_t pos=512; unsigned k;
    if(n!=CR_BLOB || mbedtls_sha256(p,n,h,0) || memcmp(h,hash,32))return -1;
    if(u32(p)!=3 || u32(p+0x14)!=0x1346 || p[0x1f]!=0 || p[0x20]!=0)return -1;
    for(k=0;k<24;k++){
        if(pos>n-4 || u16(p+pos)!=k || u16(p+pos+2)!=cr_block_lengths[k] || cr_block_lengths[k]>n-pos)return -1;
        offsets[k]=pos; pos+=cr_block_lengths[k];
    }
    if(pos!=0x85f6 || u16(p+pos)!=0x1c || u16(p+pos+2)!=560 || pos+4+560!=0x882a)return -1;
    p+=pos+4;
    if(u16(p)!=0x0103 || u16(p+2)!=1 || u16(p+4)!=0x0100 || u16(p+6)!=0 ||
       u16(p+556)!=0x0100 || u16(p+558)!=1)return -1;
    /* Full pinned-file SHA, not an assumption that the second pack is identical,
     * binds every other byte. No sensor data are altered or appended. */
    return 0;
}
int cr_validate_pfn(uint64_t e,uint32_t i,uint32_t last,uint32_t *pfn)
{
    uint64_t p=e&((1ULL<<55)-1);
    if((e>>61)!=4 || ((e>>55)&63)!=12 || p<0x80000 || p>=0x81800 ||
       (i && p!=(uint64_t)last+1))return -1;
    *pfn=(uint32_t)p;return 0;
}
int cr_make_reference(uint32_t output_pfn,uint32_t reference_pfn,struct cr_reference *r)
{
    uint64_t output=(uint64_t)output_pfn*4096,ref=(uint64_t)reference_pfn*4096;
    if(output<0x80000000ULL || output+CR_MAP>0x81800000ULL ||
       ref<0x80000000ULL || ref+CR_REF_BYTES>0x81800000ULL ||
       !(ref+CR_REF_BYTES<=output || output+CR_MAP<=ref))return -1;
    *r=(struct cr_reference){{(uint32_t)ref,0x258000,(uint32_t)(ref+0x258000),0x96000,
                             (uint32_t)(ref+0x2ee000),0x96000}};
    return 0;
}
int cr_validate_dq(struct cr_state *s,const struct cr_buffer *b)
{
    uint64_t t; unsigned i=b->index;
    if(i>=4 || b->type!=1 || b->memory!=2 || (b->flags&0x40) ||
       b->userptr!=s->user_base+i*CR_SLOT || b->length!=CR_MAIN || b->bytesused!=CR_MAIN ||
       b->sec<0 || b->usec<0 || b->usec>=1000000)return -1;
    t=(uint64_t)(uint32_t)b->sec*1000000+(uint32_t)b->usec;
    if(!t || t<=s->last_stamp || (s->slot_gen[i] &&
       (t<=s->slot_stamp[i] || b->sequence<s->slot_seq[i])))return -1;
    /* field_count>>1 may repeat on a slot. Equal sequence with a NEW timestamp
     * is allowed. Timestamp repetition or sequence wrap/backwards is rejected. */
    if(s->slot_gen[i] && (uint64_t)b->sequence>(uint64_t)s->slot_seq[i]+1)s->gaps++;
    if(s->last_stamp && t-s->last_stamp>1000000)s->gaps++;
    if(s->slot_gen[i]==UINT32_MAX)return -1;
    s->last_stamp=t;s->slot_stamp[i]=t;s->slot_seq[i]=b->sequence;s->slot_gen[i]++;s->ordinal++;
    return 0; /* DONE is deliberately not required: legacy DQ clears it. */
}
int cr_frame_attribution(const struct cr_state *s,int phase)
{
    uint64_t now,transition,minimum;const struct cr_phase *p;
    uint32_t error=s->source_clock_error_us;
    if(s->source_clock_admitted!=1 || s->last_ms>(UINT64_MAX-error)/1000)return -1;
    now=s->last_ms*1000;
    if(s->last_stamp>now+error)return -1; /* reject even before any phase exists */
    if(phase<0)return 0;
    if(phase>=3)return -1;
    p=&s->phase[phase];
    if(!p->verified || p->verified_level!=(uint32_t)(phase==1))return 0;
    if(p->verified_transition_ms>(UINT64_MAX-500000-error)/1000)return -1;
    transition=p->verified_transition_ms*1000;
    minimum=transition+500000+error;
    /* Earliest possible source completion must be >= verified latch +500ms.
     * A positive monotonic but backlogged frame is drained, not re-labelled. */
    return s->last_stamp>=minimum;
}
static void crc_init(struct cr_state *s)
{
    unsigned i,k;for(i=0;i<256;i++){uint32_t c=i;for(k=0;k<8;k++)c=(c>>1)^((c&1)?0xedb88320U:0);s->crc_table[i]=c;}
}
uint32_t cr_crc(struct cr_state *s,const uint8_t *p,size_t n)
{
    uint32_t c=~0U;size_t i;for(i=0;i<n;i++)c=s->crc_table[(c^p[i])&255]^(c>>8);return ~c;
}
static int budget(struct cr_state *s,const struct cr_ops *o,uint64_t limit)
{
    uint64_t t;if(o->cancelled(o->opaque)){s->cancel_seen=1;return -1;}
    if(o->clock_ms(o->opaque,&t) || t<s->last_ms || t<s->start_ms || t-s->start_ms>=limit)return -1;
    s->last_ms=t;return 0;
}
static int call(struct cr_state *s,const struct cr_ops *o,enum cr_op op,void *p)
{
    if(budget(s,o,25000))return -1;
    if(o->call(o->opaque,op,p)!=0)return -1;
    return budget(s,o,25000);
}
static int sensor(struct cr_state *s,const struct cr_ops *o,uint16_t values[CR_REGS])
{
    unsigned i;for(i=0;i<CR_REGS;i++){
        struct cr_sensor_read r={cr_sensor_regs[i],0xffff};
        if(call(s,o,CR_READ_SENSOR,&r) || r.reg!=cr_sensor_regs[i] || r.value>255)return -1;
        values[i]=r.value;
    }return 0;
}
static int guard(struct cr_state *s,const struct cr_ops *o,const uint8_t *p)
{
    uint32_t i;for(i=CR_EXTENT;i<CR_SLOT;i++){
        if(!(i&65535) && budget(s,o,25000))return -1;
        if(p[i]!=0xa5)return -1;
    }return 0;
}
static int sample(struct cr_state *s,const struct cr_ops *o,unsigned phase,const struct cr_buffer *b)
{
    struct cr_phase *r=&s->phase[phase];const uint8_t *p=s->mapping+b->index*CR_SLOT;
    uint32_t i,c=~0U,sum=0;unsigned x,y,xx,yy,minimum=255,maximum=0;int notfill=0;
    uint32_t hist[16]={0};uint16_t before[CR_REGS],after[CR_REGS];
    if(sensor(s,o,before))return -1;
    for(i=0;i<CR_Y;i++){
        if(!(i&65535) && budget(s,o,25000))return -1;
        c=s->crc_table[(c^p[i])&255]^(c>>8);sum+=p[i];hist[p[i]>>4]++;
        if(p[i]<minimum)minimum=p[i];
        if(p[i]>maximum)maximum=p[i];
        notfill|=p[i]!=0xa5;
    }
    if(!notfill)return -1;
    /* Owned/fresh/checked zero is a negative image observation, not proof of
     * working optics or IR absence. Untouched0xa5 remains rejected. */
    r->flat=minimum==maximum;r->zero=maximum==0;
    /* 8x8 box means: only the main contiguous Y span, never UV or AE trailer. */
    for(y=0;y<90;y++){
        if(budget(s,o,25000))return -1;
        for(x=0;x<160;x++){
            unsigned total=0;for(yy=0;yy<8;yy++)for(xx=0;xx<8;xx++)total+=p[(y*8+yy)*1280+x*8+xx];
            r->thumb[y*160+x]=(uint8_t)((total+32)/64);
        }
    }
    if(sensor(s,o,after))return -1;
    /* Keep last selected sample per phase; eight sample CRCs are folded, not
     * eight retained images. Readbacks bracket processing, not sensor exposure. */
    r->crc=~c;r->fold=(r->fold<<1|r->fold>>31)^r->crc;
    r->mean_x1000=(uint32_t)((uint64_t)sum*1000/CR_Y);
    memcpy(r->histogram,hist,sizeof(hist));memcpy(r->sensor_before,before,sizeof(before));memcpy(r->sensor_after,after,sizeof(after));
    r->ordinal=s->ordinal;r->slot=b->index;r->generation=s->slot_gen[b->index];r->sequence=b->sequence;
    r->sec=b->sec;r->usec=b->usec;r->samples++;return 0;
}
static int emit(struct cr_state *s,const struct cr_ops *o,const char *p,size_t n,uint64_t end)
{
    if(n>CR_OUTPUT-s->output_bytes)return -1;
    while(n){size_t k=n>512?512:n;
        if(budget(s,o,40000) || s->last_ms>=end || o->emit(o->opaque,p,k))return -1;
        s->output_bytes+=(uint32_t)k;p+=k;n-=k;
    }return 0;
}
static int publish(struct cr_state *s,const struct cr_ops *o)
{
    unsigned phase,i;uint64_t end;char h[1024],b64[513],sha[65],pre[CR_REGS*2+1],post[CR_REGS*2+1];
    static const char hex[]="0123456789abcdef";
    if(budget(s,o,40000))return -1;
    end=s->last_ms+10000;
    for(phase=0;phase<3;phase++){
        struct cr_phase *r=&s->phase[phase];uint8_t d[32];size_t pos,n,used;int k;
        if(!r->samples || !r->verified || mbedtls_sha256(r->thumb,CR_THUMB,d,0))return -1;
        for(i=0;i<32;i++){sha[2*i]=hex[d[i]>>4];sha[2*i+1]=hex[d[i]&15];}sha[64]=0;
        for(i=0;i<CR_REGS;i++){pre[i*2]=hex[r->sensor_before[i]>>4];pre[i*2+1]=hex[r->sensor_before[i]&15];post[i*2]=hex[r->sensor_after[i]>>4];post[i*2+1]=hex[r->sensor_after[i]&15];}pre[CR_REGS*2]=post[CR_REGS*2]=0;
        k=snprintf(h,sizeof(h),"{\"record\":\"stock-ir-thumb-v2\",\"class\":\"QUALITATIVE_AE_CONFOUNDED\",\"phase\":%u,\"level\":%u,\"verified_transition_ms\":%llu,\"source_clock_error_us\":%u,\"samples\":%u,\"flat\":%s,\"zero\":%s,\"image_confirmed\":false,\"ordinal\":%u,\"slot\":%u,\"generation\":%u,\"sequence\":%u,\"sec\":%d,\"usec\":%d,\"mean_x1000\":%u,\"raw_y_crc32\":\"%08x\",\"phase_crc_fold\":\"%08x\",\"sensor_before_hex\":\"%s\",\"sensor_after_hex\":\"%s\",\"hist16\":[",phase,r->verified_level,(unsigned long long)r->verified_transition_ms,s->source_clock_error_us,r->samples,r->flat?"true":"false",r->zero?"true":"false",r->ordinal,r->slot,r->generation,r->sequence,r->sec,r->usec,r->mean_x1000,r->crc,r->fold,pre,post);
        if(k<0 || (size_t)k>=sizeof(h))return -1;
        used=(size_t)k;
        for(i=0;i<16;i++){k=snprintf(h+used,sizeof(h)-used,"%s%u",i?",":"",r->histogram[i]);if(k<0 || (size_t)k>=sizeof(h)-used)return -1;used+=(size_t)k;}
        k=snprintf(h+used,sizeof(h)-used,"],\"width\":160,\"height\":90,\"bytes\":14400,\"sha256\":\"%s\",\"encoding\":\"base64\",\"data\":\"",sha);
        if(k<0 || (size_t)k>=sizeof(h)-used)return -1;
        used+=(size_t)k;
        if(emit(s,o,h,used,end))return -1;
        for(pos=0;pos<CR_THUMB;pos+=n){n=CR_THUMB-pos;if(n>384)n=384;
            if(mbedtls_base64_encode((unsigned char *)b64,sizeof(b64),&used,r->thumb+pos,n) || emit(s,o,b64,used,end))return -1;
        }
        if(emit(s,o,"\"}\n",3,end))return -1;
    }
    return 0;
}
static int finish(struct cr_state *s,const struct cr_ops *o,int reason,int pictures)
{
    int zero=0,v=-1;char line[512];int n;
    s->reason=reason;
    /* Restoration is attempted once even when the normal budget/cancel checks
     * have fired. Its syscall is still subject to parent full-system recovery. */
    if(s->ir_attempt){
        int rc=o->call(o->opaque,CR_IR_SET,&zero);
        if(rc==0 && o->call(o->opaque,CR_IR_VERIFY,&v)==0 && v==0)s->ir_restored=1;
    }else s->ir_restored=s->ir_baseline;
    if(s->stream_attempt){uint32_t type=1;if(o->call(o->opaque,CR_STOP_STREAM,&type)!=0 && !s->reason)s->reason=90;}
    if(!s->ir_restored && s->ir_attempt)s->reason=91;
    if(pictures && !s->reason && publish(s,o))s->reason=92;
    n=snprintf(line,sizeof(line),"{\"record\":\"stock-capture-end-v1\",\"state\":\"%s\",\"reason\":%d,\"ordinal\":%u,\"gaps\":%u,\"ir_restored\":%s,\"activation_attempt\":%s,\"cleanup\":false,\"recovery\":\"FULL_SYSTEM_RESET_WITH_OWNERS_RETAINED\"}\n",s->retained?"RETAINED_STOP":"PRE_ENTRY_ERROR",s->reason,s->ordinal,s->gaps,s->ir_restored?"true":"false",s->active_attempt?"true":"false");
    if(n>0 && (size_t)n<sizeof(line) && (uint32_t)n<=CR_OUTPUT-s->output_bytes){
        /* Best effort only; no unbounded reporting after an output failure. */
        uint64_t now;
        /* A cancellation must not itself suppress the one final scalar receipt.
         * Unlike image publication this one <=512-byte attempt ignores cancel,
         * but still requires a valid monotonic global output deadline. */
        if(s->reason!=92 && !o->clock_ms(o->opaque,&now) && now>=s->last_ms &&
           now>=s->start_ms && now-s->start_ms<40000 &&
           !o->emit(o->opaque,line,(size_t)n))s->output_bytes+=(uint32_t)n;
    }
    if(s->retained){o->retained_stop(o->opaque);
#ifndef CR_HOST
        /* An erroneously returning backend must not exit and release owners. */
        for(;;){(void)kill(getpid(),SIGSTOP);(void)pause();}
#else
        return 125; /* host-only simulated STOP, never a native success */
#endif
    }return 2;
}
int cr_run(struct cr_state *s,const struct cr_ops *o)
{
    struct cr_entry entry={0};struct cr_file file;struct cr_mapping map={0};
    struct cr_alloc alloc={CR_MAP,4096,2,3,0},ref_alloc={CR_REF_BYTES,4096,2,3,0};
    struct cr_export exp={0,-1},ref_exp={0,-1};
    struct cr_reqbuf req={4,1,2,{0,0}};struct cr_buffer b;uint32_t type=1;
    struct cr_format fmt;struct cr_parm parm;struct cr_geometry geom={0};
    unsigned i,j;int value=-1,phase=-1;uint64_t elapsed;uint32_t pfn=0;
    memset(s,0,sizeof(*s));crc_init(s);
    if(o->clock_ms(o->opaque,&s->start_ms))return 2;
    s->last_ms=s->start_ms;
    if(call(s,o,CR_ENTRY,&entry) || !entry.root_child || !entry.stock_instance_pins || !entry.fd3_closed)return finish(s,o,1,0);
    if(entry.source_clock_admitted!=1 ||
       s->last_ms>(UINT64_MAX-500000-entry.source_clock_error_us)/1000)return finish(s,o,26,0);
    s->source_clock_admitted=1;s->source_clock_error_us=entry.source_clock_error_us;
    file=(struct cr_file){s->blob,sizeof(s->blob),0,0,CR_TUNING_PATH};
    if(call(s,o,CR_READ_TUNING,&file) || !file.root_regular_nofollow_same_inode || file.data!=s->blob || file.capacity!=CR_BLOB || !file.path || strcmp(file.path,CR_TUNING_PATH) ||
       cr_validate_blob(s->blob,file.length,
#ifdef CR_HOST
                        o->synthetic_sha,
#else
                        original_sha,
#endif
                        s->block_offsets))return finish(s,o,2,0);
    if(call(s,o,CR_IR_BASELINE,&value) || value!=0)return finish(s,o,3,0);
    s->ir_baseline=1;
    /* Most conservative retention boundary: the FIRST ISP-open attempt. */
    s->retained=1;
    if(call(s,o,CR_OPEN_ISP,0) || call(s,o,CR_OPEN_VIDEO,0))return finish(s,o,4,0);
    if(call(s,o,CR_ALLOC,&alloc) || alloc.len!=CR_MAP || alloc.align!=4096 || alloc.mask!=2 || alloc.flags!=3 || !alloc.handle || alloc.handle>=0xfffff001U)return finish(s,o,5,0);
    exp.handle=alloc.handle;
    if(call(s,o,CR_EXPORT,&exp) || exp.handle!=alloc.handle || exp.fd<3)return finish(s,o,6,0);
    if(call(s,o,CR_MAP_RW,&map) || !map.data || map.length!=CR_MAP || (map.user_base&4095) || map.user_base>UINT32_MAX-CR_MAP)return finish(s,o,7,0);
    s->mapping=map.data;s->user_base=map.user_base;
    for(i=0;i<CR_PAGES;i+=32){struct cr_pages pg={0};pg.first=i;pg.count=32;
        if(call(s,o,CR_PFN_CHUNK,&pg) || pg.first!=i || pg.count!=32)return finish(s,o,8,0);
        for(j=0;j<32;j++){if(cr_validate_pfn(pg.entries[j],i+j,pfn,&pfn))return finish(s,o,8,0);if(!i&&!j)s->first_pfn=pfn;}
    }s->last_pfn=pfn;if(s->last_pfn-s->first_pfn+1!=CR_PAGES)return finish(s,o,8,0);
    if(call(s,o,CR_REF_ALLOC,&ref_alloc) || ref_alloc.len!=CR_REF_BYTES || ref_alloc.align!=4096 ||
       ref_alloc.mask!=2 || ref_alloc.flags!=3 || !ref_alloc.handle || ref_alloc.handle>=0xfffff001U ||
       ref_alloc.handle==alloc.handle)return finish(s,o,28,0);
    ref_exp.handle=ref_alloc.handle;
    if(call(s,o,CR_REF_EXPORT,&ref_exp) || ref_exp.handle!=ref_alloc.handle || ref_exp.fd<3 || ref_exp.fd==exp.fd)return finish(s,o,28,0);
    memset(&map,0,sizeof(map));
    if(call(s,o,CR_REF_MAP_RW,&map) || !map.data || map.length!=CR_REF_BYTES || (map.user_base&4095) ||
       map.user_base>UINT32_MAX-CR_REF_BYTES ||
       !((uint64_t)map.user_base+CR_REF_BYTES<=s->user_base || (uint64_t)s->user_base+CR_MAP<=map.user_base))return finish(s,o,28,0);
    s->reference_mapping=map.data;s->reference_user_base=map.user_base;pfn=0;
    for(i=0;i<CR_REF_PAGES;i+=32){struct cr_pages pg={0};unsigned count=CR_REF_PAGES-i;if(count>32)count=32;
        pg.first=i;pg.count=count;
        if(call(s,o,CR_REF_PFN_CHUNK,&pg) || pg.first!=i || pg.count!=count)return finish(s,o,28,0);
        for(j=0;j<count;j++){if(cr_validate_pfn(pg.entries[j],i+j,pfn,&pfn))return finish(s,o,28,0);if(!i&&!j)s->reference_first_pfn=pfn;}
    }
    s->reference_last_pfn=pfn;
    if(s->reference_last_pfn-s->reference_first_pfn+1!=CR_REF_PAGES ||
       cr_make_reference(s->first_pfn,s->reference_first_pfn,&s->reference))return finish(s,o,28,0);
    /* Only the newly owned mapping. Exact flags3 CACHE_DIRECT source is closed;
     * native complete initialization/pre-sensor DMA gating is still HOLD. */
    for(i=0;i<CR_MAP;i+=65536){if(budget(s,o,25000))return finish(s,o,9,0);memset(s->mapping+i,0xa5,65536);}
    /* Separate cold reference owner: exact900 pages, no padding/guard reuse. */
    for(i=0;i<CR_REF_BYTES;i+=65536){unsigned n=CR_REF_BYTES-i;if(n>65536)n=65536;
        if(budget(s,o,25000))return finish(s,o,9,0);
        memset(s->reference_mapping+i,0,n);}
    for(i=0;i<24;i++){
        struct cr_block block={i,s->block_offsets[i],cr_block_lengths[i],s->blob+s->block_offsets[i]};
        if(call(s,o,CR_TUNING_BLOCK,&block))return finish(s,o,10,0);
        for(j=0;j<34;j++)if(cr_bindings[j].block==i){
            const struct cr_binding *v=&cr_bindings[j];
            struct cr_set set={v->request,v->bytes,block.data+v->offset};
            if((uint32_t)v->offset+v->bytes>block.length || call(s,o,CR_ISP_SET,&set))return finish(s,o,10,0);
        }
        /* Deliberately omit stock APP FPS-policy globals/thread synchronization.
         * This fixed reference keeps the pinned device tuning and sensor init. */
    }
    {struct cr_reference expected=s->reference;
        if(call(s,o,CR_REF_INSTALL,&s->reference) || memcmp(&expected,&s->reference,sizeof(expected)))return finish(s,o,12,0);}
    /* Parent proposal01: explicit controlled-client readiness BEFORE sensor-on.
     * This is not a claim that the normal stock APP used this reordered path. */
    memset(&fmt,0,sizeof(fmt));fmt.type=1;put32(fmt.data,1280);put32(fmt.data+4,720);put32(fmt.data+8,0x56595559);put32(fmt.data+12,1);
    memset(&parm,0,sizeof(parm));parm.type=1;put32(parm.data,1);put32(parm.data+4,640);put32(parm.data+8,360);
    if(call(s,o,CR_SET_MAIN,&fmt) || call(s,o,CR_SET_SUB,&parm) || call(s,o,CR_GEOMETRY,&geom) ||
       geom.main_w!=1280 || geom.main_h!=720 || geom.sub_w!=640 || geom.sub_h!=360 || geom.sub_evidence!=1)return finish(s,o,14,0);
    {struct cr_reference expected=s->reference;
        if(call(s,o,CR_PRE_SENSOR_CHECK,&s->reference) || memcmp(&expected,&s->reference,sizeof(expected)))return finish(s,o,12,0);}
    {
        struct cr_load load={{140,0},s->blob+0x85fa,560};
#ifndef CR_HOST
        load.wire.ptr=(uint32_t)(uintptr_t)load.payload;
#else
        load.wire.ptr=0x100085fa; /* public fixture token, never a real host pointer */
#endif
        s->active_attempt=1;
        if(call(s,o,CR_LOAD_SENSOR,&load))return finish(s,o,13,0);
    }
    if(call(s,o,CR_REQUEST,&req) || req.count!=4 || req.type!=1 || req.memory!=2)return finish(s,o,15,0);
    for(i=0;i<4;i++){memset(&b,0,sizeof(b));b.index=i;b.type=1;b.memory=2;b.userptr=s->user_base+i*CR_SLOT;b.length=CR_SLOT;
        s->active_attempt=1;if(call(s,o,CR_QUEUE,&b))return finish(s,o,16,0);
    }
    s->stream_attempt=1;
    if(call(s,o,CR_START,&type))return finish(s,o,17,0);
    s->stream_ms=s->last_ms;
    for(i=0;i<CR_MAX_DQ;i++){
        int wanted,rc,attribution,timeout=100;
        if(budget(s,o,25000))return finish(s,o,18,0);
        elapsed=s->last_ms-s->stream_ms;if(elapsed>=11000)break;
        wanted=elapsed<2000?-1:(int)((elapsed-2000)/3000);
        if(wanted>=0 && wanted!=phase){value=wanted==1;s->ir_attempt=1;
            int observed=-1;
            if(call(s,o,CR_IR_SET,&value))return finish(s,o,19,0);
            if(call(s,o,CR_IR_VERIFY,&observed) || observed!=(wanted==1))return finish(s,o,19,0);
            /* call() obtained a bounded monotonic timestamp AFTER verification.
             * Neither the requested level nor the write-return time is used. */
            s->phase[wanted].verified_transition_ms=s->last_ms;
            s->phase[wanted].verified_level=(uint32_t)observed;
            s->phase[wanted].verified=1;
            phase=wanted;
        }
        if(call(s,o,CR_POLL,&timeout))return finish(s,o,20,0);
        /* Re-evaluate phase after any blocking poll before attributing a frame.
         * Transition-crossing completion is drained but not a phase sample. */
        memset(&b,0,sizeof(b));b.type=1;b.memory=2;
        rc=o->call(o->opaque,CR_DEQUEUE,&b);
        if(rc==-1 && errno==EAGAIN)continue;
        if(rc!=0 || budget(s,o,25000) || cr_validate_dq(s,&b))return finish(s,o,21,0);
        attribution=cr_frame_attribution(s,phase);
        if(attribution<0)return finish(s,o,27,0);
        if(guard(s,o,s->mapping+b.index*CR_SLOT))return finish(s,o,22,0);
        elapsed=s->last_ms-s->stream_ms;
        if(attribution==1 && phase>=0 && elapsed>=2000U+(unsigned)phase*3000U && elapsed<5000U+(unsigned)phase*3000U &&
           s->phase[phase].samples<8 && elapsed>=2500U+(unsigned)phase*3000U+s->phase[phase].samples*300U){
            if(sample(s,o,(unsigned)phase,&b))return finish(s,o,23,0);
        }
        {unsigned index=b.index;memset(&b,0,sizeof(b));b.index=index;b.type=1;b.memory=2;b.userptr=s->user_base+index*CR_SLOT;b.length=CR_SLOT;}
        if(call(s,o,CR_QUEUE,&b))return finish(s,o,24,0);
    }
    if(i==CR_MAX_DQ || !s->phase[0].samples || !s->phase[1].samples || !s->phase[2].samples)return finish(s,o,25,0);
    return finish(s,o,0,1);
}
