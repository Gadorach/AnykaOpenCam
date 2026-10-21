/* Public synthetic-only syscall model. It never opens a file, device or proc. */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <mbedtls/sha256.h>
#include "capture.h"
static uint8_t memory[CR_MAP],reference_memory[CR_REF_BYTES],dummy[CR_BLOB];
static struct cr_state state;
static struct fixture {
    uint64_t ms; unsigned counts[CR_STOP_STREAM+1],frames,slot_seq[4];
    int fail_op,fail_nth,positive,cancel_op,scenario,ir,stopped,allow_packets;
    unsigned blocks,queues,stream,retained_bytes,emitted,restores,readbacks;
    uint64_t verified_ms[3];
    unsigned stale_sample_reads,old_drains[3],verified_reads,processing_delay_used;
    char output[CR_OUTPUT+1];
} f;
enum { GOOD, FD3, FILE_ID, HASH, SHORT_BLOB, PFN_GAP, PFN_OUTSIDE, PFN_MISSING,
       PFN_SHIFT, MAP_ALIGN, BAD_HANDLE, BAD_EXPORT, BAD_GEOMETRY, BAD_COUNT,
       BAD_INDEX, BAD_ERROR, BAD_ZERO_TS, BAD_USEC, BAD_PTR, BAD_LENGTH,
       DUP_TS, SEQ_WRAP, GUARD, ZERO_Y, FILL_Y, CLOCK_BACK, DEADLINE, RESTORE_FAIL,
       OUTPUT_FAIL, NO_FRAMES, STALE_BASELINE, GAPS, CHANGING_AE,
       SOURCE_CLOCK_MISSING, SOURCE_CLOCK_BAD_FLAG, SOURCE_CLOCK_WIDE_BOUND,
       SOURCE_FUTURE, SOURCE_FUTURE_BOUND, SOURCE_FUTURE_EXCESS,
       SOURCE_LAG1500, SOURCE_LAG3500, VERIFY_DELAY, SOURCE_VERIFY_LAG,
       VERIFY0_MISMATCH, VERIFY1_MISMATCH, VERIFY2_MISMATCH, RESTORE_VERIFY_MISMATCH,
       VERIFY_CLOCK_FAILURE, PROCESS_CROSSES_PHASE,
       REF_BAD_LENGTH, REF_LAST_GAP, REF_OUTSIDE, REF_OVERLAP, REF_BAD_PACKET, DARK_ABA };
static void le16(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void le32(uint8_t *p,uint32_t v){le16(p,v);le16(p+2,v>>16);}
static void make_dummy(void)
{
    unsigned record,k;uint32_t base=0,pos;
    memset(dummy,0,sizeof(dummy));
    for(record=0;record<2;record++){
        le32(dummy+base,3);le32(dummy+base+0x14,0x1346);dummy[base+0x1f]=(uint8_t)record;
        pos=base+512;
        for(k=0;k<24;k++){le16(dummy+pos,k);le16(dummy+pos+2,cr_block_lengths[k]);pos+=cr_block_lengths[k];}
        le16(dummy+pos,0x1c);le16(dummy+pos+2,560);pos+=4;
        le16(dummy+pos,0x0103);le16(dummy+pos+2,1);le16(dummy+pos+4,0x0100);
        le16(dummy+pos+556,0x0100);le16(dummy+pos+558,1);
        base=pos+560;
    }
    assert(base==CR_BLOB);
}
static int clock_ms(void *unused,uint64_t *t)
{
    (void)unused;*t=f.ms;
    if(f.scenario==CLOCK_BACK && f.frames>2)*t=1;
    if(f.scenario==VERIFY_CLOCK_FAILURE && f.counts[CR_IR_VERIFY])return -1;
    return 0;
}
static int cancelled(void *unused)
{
    (void)unused;return f.cancel_op>=0 && f.counts[f.cancel_op]>0;
}
static void stop(void *unused){(void)unused;f.stopped++;f.retained_bytes=state.mapping?CR_MAP:0;}
static int emit(void *unused,const char *p,size_t n)
{
    (void)unused;assert(n<=512);if(f.scenario==OUTPUT_FAIL)return -1;
    assert(f.emitted+n<=CR_OUTPUT);memcpy(f.output+f.emitted,p,n);f.emitted+=(unsigned)n;return 0;
}
static void frame(struct cr_buffer *b)
{
    uint8_t *p;unsigned i,slot=f.frames%4;uint32_t seed;
    f.frames++;p=memory+slot*CR_SLOT;seed=0x12345678U^f.frames;
    for(i=0;i<CR_Y;i++){seed=seed*1664525U+1013904223U;p[i]=(uint8_t)((f.ir?100:20)+((seed>>24)&31));}
    memset(p+CR_Y,0x77,CR_EXTENT-CR_Y); /* UV and live AE trailer never published */
    if(f.scenario==ZERO_Y || (f.scenario==DARK_ABA && !f.ir))memset(p,0,CR_Y);
    if(f.scenario==FILL_Y)memset(p,0xa5,CR_Y);
    if(f.scenario==GUARD)p[CR_EXTENT]=0;
    memset(b,0,sizeof(*b));b->index=slot;b->type=1;b->memory=2;
    b->length=b->bytesused=CR_MAIN;b->userptr=0x40000000U+slot*CR_SLOT;
    b->sec=(int32_t)(f.ms/1000);b->usec=(int32_t)(f.ms%1000*1000);
    b->sequence=f.slot_seq[slot]++/2; /* valid per-slot equal sequence, DONE absent */
    if(f.scenario==GAPS)b->sequence*=3;
    if(f.scenario==BAD_INDEX)b->index=4;
    if(f.scenario==BAD_ERROR)b->flags=0x40;
    if(f.scenario==BAD_ZERO_TS)b->sec=b->usec=0;
    if(f.scenario==BAD_USEC)b->usec=1000000;
    if(f.scenario==BAD_PTR)b->userptr++;
    if(f.scenario==BAD_LENGTH)b->length=1843200;
    if(f.scenario==DUP_TS && f.frames>1){b->sec=(int32_t)(state.last_stamp/1000000);b->usec=(int32_t)(state.last_stamp%1000000);}
    if(f.scenario==SEQ_WRAP){b->sequence=f.frames<=4?0xffffffffU:0;}
    if(f.scenario==SOURCE_LAG1500 || f.scenario==SOURCE_LAG3500 || f.scenario==SOURCE_VERIFY_LAG){
        /* Parent counterexample: all timestamps positive/monotonic, but queued
         * completions can predate the current latch state by1500ms. */
        uint64_t lag=f.scenario==SOURCE_LAG3500?3500:1500;
        uint64_t source=f.ms>=lag+500?f.ms-lag:f.ms/(lag==3500?8:4);
        b->sec=(int32_t)(source/1000);b->usec=(int32_t)(source%1000*1000);
    }
    if(f.scenario==SOURCE_FUTURE || f.scenario==SOURCE_FUTURE_BOUND || f.scenario==SOURCE_FUTURE_EXCESS){
        uint64_t source=f.ms*1000+(f.scenario==SOURCE_FUTURE?1000:f.scenario==SOURCE_FUTURE_BOUND?10000:10001);
        b->sec=(int32_t)(source/1000000);b->usec=(int32_t)(source%1000000);
    }
    if(f.verified_reads && f.verified_reads<=3){
        unsigned phase=f.verified_reads-1;
        uint64_t stamp=(uint64_t)(uint32_t)b->sec*1000000+(uint32_t)b->usec;
        if(stamp<f.verified_ms[phase]*1000+500000)f.old_drains[phase]++;
    }
}
static int operation(void *unused,enum cr_op op,void *arg)
{
    (void)unused;assert(op>=CR_ENTRY && op<=CR_STOP_STREAM);f.counts[op]++;
    if((int)op==f.fail_op && (int)f.counts[op]==f.fail_nth){errno=EIO;return f.positive?1:-1;}
    switch(op){
    case CR_ENTRY: {
        struct cr_entry *e=arg;
        *e=(struct cr_entry){.root_child=1,.stock_instance_pins=1,.fd3_closed=f.scenario!=FD3,
            .source_clock_admitted=1,.source_clock_error_us=0}; /* explicit dummy correspondence */
        if(f.scenario==SOURCE_CLOCK_MISSING)e->source_clock_admitted=0;
        if(f.scenario==SOURCE_CLOCK_BAD_FLAG)e->source_clock_admitted=2;
        if(f.scenario==SOURCE_CLOCK_WIDE_BOUND)e->source_clock_error_us=UINT32_MAX;
        if(f.scenario==SOURCE_FUTURE_BOUND || f.scenario==SOURCE_FUTURE_EXCESS)e->source_clock_error_us=10000;
        break;}
    case CR_READ_TUNING: {
        struct cr_file *r=arg;assert(r->capacity==CR_BLOB && !strcmp(r->path,CR_TUNING_PATH));memcpy(r->data,dummy,CR_BLOB);
        r->length=f.scenario==SHORT_BLOB?CR_BLOB-1:CR_BLOB;r->root_regular_nofollow_same_inode=f.scenario!=FILE_ID;
        if(f.scenario==HASH)r->data[999]^=1;
        break;}
    case CR_IR_BASELINE: *(int*)arg=f.scenario==STALE_BASELINE?1:0;break;
    case CR_OPEN_ISP: break;
    case CR_OPEN_VIDEO: assert(f.counts[CR_OPEN_ISP]==1);break;
    case CR_ALLOC: {
        struct cr_alloc *a=arg;assert(a->len==CR_MAP && a->align==4096 && a->mask==2 && a->flags==3);
        a->handle=f.scenario==BAD_HANDLE?0:0x1234;break;}
    case CR_EXPORT: {struct cr_export *e=arg;assert(e->handle==0x1234);e->fd=f.scenario==BAD_EXPORT?0:7;break;}
    case CR_MAP_RW: *(struct cr_mapping*)arg=(struct cr_mapping){memory,0x40000000U+(f.scenario==MAP_ALIGN),CR_MAP};break;
    case CR_PFN_CHUNK: {
        struct cr_pages *p=arg;unsigned i;assert(p->count==32 && p->first<CR_PAGES);
        for(i=0;i<32;i++){uint64_t pfn=0x80400+p->first+i;
            if(f.scenario==PFN_GAP && p->first+i==2000)pfn++;
            if(f.scenario==PFN_OUTSIDE)pfn=0x81800;
            p->entries[i]=(1ULL<<63)|(12ULL<<55)|pfn;
            if(f.scenario==PFN_MISSING)p->entries[i]&=~(1ULL<<63);
            if(f.scenario==PFN_SHIFT)p->entries[i]^=1ULL<<55;
        }break;}
    case CR_TUNING_BLOCK: {
        struct cr_block *b=arg;assert(b->id==f.blocks && b->length==cr_block_lengths[b->id]);
        assert(b->offset==state.block_offsets[b->id] && b->data==state.blob+b->offset);f.blocks++;break;}
    case CR_ISP_SET: {
        unsigned index=f.counts[CR_ISP_SET]-1;struct cr_set *s=arg;const struct cr_binding *b;
        assert(index<34);b=&cr_bindings[index];assert((unsigned)b->block+1==f.blocks);
        assert(s->request==b->request && s->bytes==b->bytes && s->data==state.blob+state.block_offsets[b->block]+b->offset);break;}
    case CR_REF_ALLOC: {
        struct cr_alloc *a=arg;assert(a->len==CR_REF_BYTES && a->align==4096 && a->mask==2 && a->flags==3);
        a->handle=0x5678;if(f.scenario==REF_BAD_LENGTH)a->len=0x400000;break;}
    case CR_REF_EXPORT: {struct cr_export *e=arg;assert(e->handle==0x5678);e->fd=8;break;}
    case CR_REF_MAP_RW: *(struct cr_mapping*)arg=(struct cr_mapping){reference_memory,0x41000000,CR_REF_BYTES};break;
    case CR_REF_PFN_CHUNK: {
        struct cr_pages *p=arg;unsigned i;assert(p->count<=32 && p->first+p->count<=900);
        for(i=0;i<p->count;i++){
            uint64_t pfn=(f.scenario==REF_OVERLAP?0x80400:0x81000)+p->first+i;
            if(f.scenario==REF_LAST_GAP && p->first+i==899)pfn++;
            if(f.scenario==REF_OUTSIDE)pfn=0x81800;
            p->entries[i]=(1ULL<<63)|(12ULL<<55)|pfn;
        }break;}
    case CR_REF_INSTALL: {
        struct cr_reference expected={{0x81000000,0x258000,0x81258000,0x96000,0x812ee000,0x96000}};
        assert(f.blocks==24 && !memcmp(arg,&expected,24));
        for(unsigned i=0;i<CR_REF_BYTES;i++)assert(reference_memory[i]==0);
        if(f.scenario==REF_BAD_PACKET)((struct cr_reference*)arg)->words[1]=0x400000;
        break;}
    case CR_PRE_SENSOR_CHECK: assert(f.counts[CR_REF_INSTALL]==1);break;
    case CR_LOAD_SENSOR: {
        struct cr_load *l=arg;assert(f.blocks==24 && f.counts[CR_MAP_RW]==1);
        assert(l->wire.count==140 && l->wire.ptr==0x100085fa && l->bytes==560 && l->payload==state.blob+0x85fa);
        assert(l->payload[556]==0 && l->payload[557]==1 && l->payload[558]==1 && l->payload[559]==0);
        assert(state.active_attempt);break;}
    case CR_SET_MAIN: {struct cr_format *v=arg;assert(v->type==1);break;}
    case CR_SET_SUB: {struct cr_parm *v=arg;assert(v->type==1 && v->data[0]==1);break;}
    case CR_GEOMETRY: *(struct cr_geometry*)arg=(struct cr_geometry){1280,720,640,f.scenario==BAD_GEOMETRY?480:360,1};break;
    case CR_REQUEST: {struct cr_reqbuf *r=arg;assert(r->count==4 && r->type==1 && r->memory==2);if(f.scenario==BAD_COUNT)r->count=3;break;}
    case CR_QUEUE: {
        struct cr_buffer expected={0},*b=arg;assert(b->index<4);
        expected.index=b->index;expected.type=1;expected.memory=2;expected.userptr=0x40000000U+b->index*CR_SLOT;expected.length=CR_SLOT;
        assert(!memcmp(&expected,b,sizeof(expected)));f.queues++;break;}
    case CR_START: assert(f.queues==4 && *(uint32_t*)arg==1);f.stream=1;break;
    case CR_POLL: assert(f.stream && *(int*)arg==100);f.ms+=f.scenario==DEADLINE?26000:250;break;
    case CR_DEQUEUE: if(f.scenario==NO_FRAMES){errno=EAGAIN;return -1;}frame(arg);break;
    case CR_READ_SENSOR: {
        struct cr_sensor_read *r=arg;unsigned i;for(i=0;i<CR_REGS;i++)if(r->reg==cr_sensor_regs[i])break;
        assert(f.verified_reads>=1 && f.verified_reads<=3);
        if(state.last_stamp<f.verified_ms[f.verified_reads-1]*1000+500000+state.source_clock_error_us)
            f.stale_sample_reads++;
        assert(state.last_stamp<=f.ms*1000+state.source_clock_error_us);
        assert(i<CR_REGS);r->value=(uint16_t)(i?0x20+i:1);
        if(f.scenario==CHANGING_AE && i==6)r->value=(uint16_t)(f.readbacks&255);
        if(f.scenario==PROCESS_CROSSES_PHASE && !f.processing_delay_used && f.verified_reads==2 &&
           f.ms>=state.stream_ms+7750 && f.ms%1000==750){
            f.ms+=500;f.processing_delay_used=1; /* no IR transition during processing */
        }
        f.readbacks++;break;}
    case CR_IR_SET: {
        int v=*(int*)arg;assert(v==0 || v==1);if(v==0)f.restores++;
        if(f.scenario==RESTORE_FAIL && f.ir==1 && v==0){errno=EIO;return -1;}
        f.ir=v;break;}
    case CR_IR_VERIFY: {
        unsigned n=f.counts[CR_IR_VERIFY];
        if((f.scenario==VERIFY_DELAY || f.scenario==SOURCE_VERIFY_LAG) && n<=3)f.ms+=300;
        *(int*)arg=f.ir;
        if(f.scenario>=VERIFY0_MISMATCH && f.scenario<=RESTORE_VERIFY_MISMATCH &&
           n==(unsigned)(f.scenario-VERIFY0_MISMATCH+1))*(int*)arg=1-f.ir;
        if(n<=3){f.verified_ms[n-1]=f.ms;f.verified_reads=n;}
        break;}
    case CR_STOP_STREAM: assert(*(uint32_t*)arg==1);f.stream=0;break;
    }
    return 0;
}
static unsigned cases;
static int run_case(int scenario,int op,int nth,int positive,int cancel,int print)
{
    struct cr_ops o={0};int rc;
    memset(&f,0,sizeof(f));f.ms=1000;f.scenario=scenario;f.fail_op=op;f.fail_nth=nth;f.positive=positive;f.cancel_op=cancel;
    o.call=operation;o.clock_ms=clock_ms;o.cancelled=cancelled;o.emit=emit;o.retained_stop=stop;
    assert(!mbedtls_sha256(dummy,sizeof(dummy),o.synthetic_sha,0));
    rc=cr_run(&state,&o);cases++;
    if(state.retained){assert(rc==125 && f.stopped==1);if(state.mapping)assert(f.retained_bytes==CR_MAP);}
    else assert(rc==2 && f.stopped==0);
    if(state.ir_attempt && scenario!=RESTORE_FAIL && !(op==CR_IR_SET && (int)f.counts[CR_IR_SET]==nth))assert(f.ir==0);
    assert(state.output_bytes<=CR_OUTPUT && f.emitted<=CR_OUTPUT);
    assert(f.stale_sample_reads==0);
    if(cancel>=0 && state.retained)assert(strstr(f.output,"RETAINED_STOP"));
    if(scenario==RESTORE_FAIL)assert(state.reason==91 && !state.ir_restored);
    if(print){assert(state.reason==0 && state.ir_restored && state.phase[0].samples==8 && state.phase[1].samples==8 && state.phase[2].samples==8);fwrite(f.output,1,f.emitted,stdout);}
    if(state.reason==0){
        unsigned p;assert(f.counts[CR_IR_VERIFY]==4 && f.ir==0);
        for(p=0;p<3;p++){
            const struct cr_phase *r=&state.phase[p];
            uint64_t stamp=(uint64_t)(uint32_t)r->sec*1000000+(uint32_t)r->usec;
            assert(r->verified && r->verified_level==(p==1) && r->verified_transition_ms==f.verified_ms[p]);
            assert(r->samples && stamp>=f.verified_ms[p]*1000+500000+state.source_clock_error_us);
        }
    }
    fprintf(stderr,"MOCK case=%u scenario=%d op=%d nth=%d positive=%d cancel=%d exit=%d reason=%d retained=%d frames=%u\n",cases,scenario,op,nth,positive,cancel,rc,state.reason,state.retained,f.frames);
    return state.reason;
}
int main(int argc,char **argv)
{
    unsigned i;uint8_t sha[32],save;uint32_t offsets[24],pfn;char *valid[]={"/fixed/candidate","--native-stock-capture-reference",0};
    if(argc!=2)return 2;
    make_dummy();
    if(!strcmp(argv[1],"--host-dark-only")){
        assert(run_case(DARK_ABA,-1,0,0,-1,1)==0);
        for(i=0;i<3;i++){assert(state.phase[i].zero==(i!=1) && state.phase[i].flat==(i!=1));}
        fprintf(stderr,"HOST_DARK_ABA_PASS zero=true/false/true flat=true/false/true samples=8/8/8 clock_verified=true\n");return 0;
    }
    if(strcmp(argv[1],"--host-synthetic-only"))return 2;
    assert(cr_entry_allowed(2,valid,1));assert(!cr_entry_allowed(2,valid,0));assert(!cr_entry_allowed(3,valid,1));
    valid[1]="--native-stock-capture-reference --enable";assert(!cr_entry_allowed(2,valid,1));
    assert(!mbedtls_sha256(dummy,sizeof(dummy),sha,0));assert(!cr_validate_blob(dummy,sizeof(dummy),sha,offsets));
    for(i=0;i<24;i++)assert(offsets[i]+cr_block_lengths[i]<=0x85f6);
    assert(cr_validate_blob(dummy,sizeof(dummy)-1,sha,offsets));
    save=dummy[0x8829];dummy[0x8829]=1;assert(cr_validate_blob(dummy,sizeof(dummy),sha,offsets));
    assert(!mbedtls_sha256(dummy,sizeof(dummy),sha,0));assert(cr_validate_blob(dummy,sizeof(dummy),sha,offsets));dummy[0x8829]=save;
    assert(!cr_validate_pfn((1ULL<<63)|(12ULL<<55)|0x80400,0,0,&pfn) && pfn==0x80400);
    assert(cr_validate_pfn((1ULL<<63)|(12ULL<<55)|0x80402,1,pfn,&pfn));
    assert(run_case(GOOD,-1,0,0,-1,1)==0);
    assert(cr_crc(&state,(const uint8_t*)"123456789",9)==0xcbf43926U);
    for(i=FD3;i<=STALE_BASELINE;i++){
        if(i==ZERO_Y){assert(run_case((int)i,-1,0,0,-1,0)==0);for(unsigned p=0;p<3;p++)assert(state.phase[p].zero && state.phase[p].flat);}
        else assert(run_case((int)i,-1,0,0,-1,0)!=0);
    }
    for(i=CR_ENTRY;i<=CR_STOP_STREAM;i++){
        assert(run_case(GOOD,(int)i,1,0,-1,0)!=0);
        assert(run_case(GOOD,(int)i,1,1,-1,0)!=0);
    }
    for(i=2;i<=24;i++)assert(run_case(GOOD,CR_TUNING_BLOCK,(int)i,0,-1,0)!=0);
    for(i=2;i<=34;i++)assert(run_case(GOOD,CR_ISP_SET,(int)i,0,-1,0)!=0);
    for(i=2;i<=4;i++)assert(run_case(GOOD,CR_QUEUE,(int)i,0,-1,0)!=0);
    for(i=CR_OPEN_ISP;i<=CR_IR_SET;i++)assert(run_case(GOOD,-1,0,0,(int)i,0)!=0);
    assert(run_case(GAPS,-1,0,0,-1,0)==0 && state.gaps>0);
    assert(run_case(CHANGING_AE,-1,0,0,-1,0)==0);
    for(i=SOURCE_CLOCK_MISSING;i<=SOURCE_CLOCK_BAD_FLAG;i++){
        assert(run_case((int)i,-1,0,0,-1,0)==26);
        assert(!state.retained && !f.counts[CR_READ_TUNING] && !f.counts[CR_IR_SET]);
    }
    assert(run_case(SOURCE_CLOCK_WIDE_BOUND,-1,0,0,-1,0)==25 && !f.readbacks);
    assert(run_case(SOURCE_FUTURE,-1,0,0,-1,0)==27 && !f.readbacks);
    assert(run_case(SOURCE_FUTURE_BOUND,-1,0,0,-1,0)==0);
    assert(run_case(SOURCE_FUTURE_EXCESS,-1,0,0,-1,0)==27 && !f.readbacks);
    assert(run_case(SOURCE_LAG1500,-1,0,0,-1,0)==0);
    for(i=0;i<3;i++)assert(f.old_drains[i]>0 && state.phase[i].samples>=1 && state.phase[i].samples<8);
    fprintf(stderr,"CORRECTED_PARENT_COUNTEREXAMPLE stale_sample_reads=%u samples=%u/%u/%u old_drains=%u/%u/%u retained=%d\n",f.stale_sample_reads,state.phase[0].samples,state.phase[1].samples,state.phase[2].samples,f.old_drains[0],f.old_drains[1],f.old_drains[2],state.retained);
    assert(run_case(SOURCE_LAG3500,-1,0,0,-1,0)==25 && !f.readbacks);
    assert(run_case(VERIFY_DELAY,-1,0,0,-1,0)==0 && state.phase[0].verified_transition_ms==3300);
    assert(run_case(SOURCE_VERIFY_LAG,-1,0,0,-1,0)==0);
    for(i=VERIFY0_MISMATCH;i<=RESTORE_VERIFY_MISMATCH;i++){
        assert(run_case((int)i,-1,0,0,-1,0)==(i==RESTORE_VERIFY_MISMATCH?91:19));
        if(i!=RESTORE_VERIFY_MISMATCH)assert(!state.phase[i-VERIFY0_MISMATCH].verified);
        else assert(!state.ir_restored);
    }
    for(i=2;i<=4;i++){
        assert(run_case(GOOD,CR_IR_VERIFY,(int)i,0,-1,0)==(i==4?91:19));
        assert(run_case(GOOD,CR_IR_VERIFY,(int)i,1,-1,0)==(i==4?91:19));
    }
    assert(run_case(VERIFY_CLOCK_FAILURE,-1,0,0,-1,0)==19 && !state.phase[0].verified && !f.readbacks);
    assert(run_case(GOOD,-1,0,0,CR_IR_VERIFY,0)!=0);
    assert(run_case(PROCESS_CROSSES_PHASE,-1,0,0,-1,0)==0 && f.processing_delay_used);
    for(i=REF_BAD_LENGTH;i<=REF_BAD_PACKET;i++)assert(run_case((int)i,-1,0,0,-1,0)!=0);
    {
        struct cr_state small={0}; /* direct boundary oracle, no capture allocation */
        small.source_clock_admitted=1;small.source_clock_error_us=10000;small.last_ms=2000;
        small.phase[0].verified=1;small.phase[0].verified_transition_ms=1000;
        small.last_stamp=1509999;assert(cr_frame_attribution(&small,0)==0);
        small.last_stamp=1510000;assert(cr_frame_attribution(&small,0)==1);
        small.last_stamp=2010000;assert(cr_frame_attribution(&small,0)==1);
        small.last_stamp=2010001;assert(cr_frame_attribution(&small,0)==-1);
        small.last_ms=UINT64_MAX;assert(cr_frame_attribution(&small,0)==-1);
        small.last_ms=2000;small.source_clock_admitted=0;assert(cr_frame_attribution(&small,0)==-1);
    }
    fprintf(stderr,"HOST_SYNTHETIC_PASS cases=%u context_bytes=%zu no_devices_or_proc_access=true\n",cases,sizeof(state));return 0;
}
