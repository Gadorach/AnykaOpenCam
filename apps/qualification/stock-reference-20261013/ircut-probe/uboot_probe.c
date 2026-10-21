/* SPDX-License-Identifier: GPL-2.0-only
 * RAM-only GO prototype for unchanged QC07F original U-Boot, native UNEXECUTED.
 * Parent must verify BOOT native CRC452a21fe, callback code/GO ABI/ranges first.
 * Default STATUS; CALIBRATE_ONLY cannot output; exact PULSE6_ONCE/7 only.
 * No mux/direction/pull/clock/timer setup, UART/L2 access or persistent writes.
 */
typedef unsigned int u32;
#define SYS 0x08000000U
#define GPIO 0x20170000U
#define TOKEN 0x49524331U
#define SAMPLES 1024U
#define BLOCK 1024U
#define ITERATIONS (SAMPLES * BLOCK)
enum { OK, BAD_ABI, BAD_ARG, NOT_IDLE, NO_CALIBRATION, USED, BAD_TIMER,
       CAL_UNSTABLE, CONFIG_CHANGED, DRIVE_BAD, RELEASE_BAD };
struct snap { u32 sys74,sys78,dir,out,in,pull0,pull_pair,pll0,pll4,pll8,ctl; };
struct state { u32 token,used,calibrated,count,ticks_a,ticks_b; struct snap cal; };
/* Explicit BIN data, not uninitialized BSS. GO reentry MUST NOT reset it. */
static struct state state = { TOKEN,0,0,0,0,0,{0,0,0,0,0,0,0,0,0,0,0} };

#ifndef UBOOT_HOST
#if !defined(__arm__) || __ARM_ARCH != 5 || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error ARMv5 little-endian required
#endif
static u32 rd(u32 address) { return *(volatile u32 *)address; }
static u32 control(void) { u32 v; __asm__ volatile("mrc p15,0,%0,c1,c0,0":"=r"(v)); return v; }
static u32 cpsr(void) { u32 v; __asm__ volatile("mrs %0,cpsr":"=r"(v)); return v; }
static u32 mask(void) {
    u32 saved,tmp;
    __asm__ volatile("mrs %0,cpsr\norr %1,%0,#0xc0\nmsr cpsr_c,%1"
                     :"=&r"(saved),"=&r"(tmp)::"memory","cc");
    return saved;
}
static void restore(u32 saved) { __asm__ volatile("msr cpsr_c,%0"::"r"(saved):"memory","cc"); }
/* get_ticks-equivalent: exact finite original body, 12MHz accumulated counts.
 * It latches the timer via SYS+B8 and updates gd+80/+84; never called energized.
 * It is NOT an interrupt-maintained millisecond clock. See source evidence.
 */
static u32 ticks(void) { return ((u32(*)(void))0x80e003a0U)(); }
extern u32 uboot_cycle(u32 address,u32 neutral,u32 count,u32 mask);
#define ubprint ((int(*)(const char *,...))0x80e0a3f8U)
static int abi_ok(void) {
    return !(control() & 1U) && (cpsr() & 0x1fU)==0x13U &&
        rd(0x80e003a0U)==0xe3a03302U && rd(0x80e003a4U)==0xe59320b8U &&
        rd(0x80e003f0U)==0xe12fff1eU && rd(0x80e0040cU)==12000U &&
        rd(0x80e0a3f8U)==0xe92d000fU && rd(0x80e0a3fcU)==0xe92d4010U;
}
#define ENTRY _start
#else
#define ENTRY uboot_probe_entry
#endif

static void snapshot(struct snap *s) {
    s->sys74=rd(SYS+0x74); s->sys78=rd(SYS+0x78);
    s->dir=rd(GPIO); s->out=rd(GPIO+0x0c); s->in=rd(GPIO+0x18);
    s->pull0=rd(SYS+0x80); s->pull_pair=rd(SYS+0x84);
    s->pll0=rd(SYS); s->pll4=rd(SYS+4); s->pll8=rd(SYS+8); s->ctl=control();
}
static int idle(const struct snap *s) {
    return !(s->sys74&1U) && (s->sys78&0xf0U)==0x50U &&
        (s->dir&0xc1U)==0xc0U && !(s->out&0xc0U);
}
static int same(const struct snap *a,const struct snap *b) {
    /* IN is observation only: GPIO0 press/other input transitions are allowed. */
    return a->sys74==b->sys74 && a->sys78==b->sys78 && a->dir==b->dir && a->out==b->out &&
        a->pull0==b->pull0 && a->pull_pair==b->pull_pair && a->pll0==b->pll0 &&
        a->pll4==b->pll4 && a->pll8==b->pll8 && a->ctl==b->ctl;
}
static int eq(const char *a,const char *b) {
    u32 i; if(!a)return 0;
    for(i=0;i<20;++i) { if(a[i]!=b[i])return 0; if(!a[i])return 1; }
    return 0;
}
static u32 calibration_pass(void) {
    u32 i,total=0,before,delta;
    /* No timer setup. Fresh original U-Boot timer1 must already be enabled,
     * auto-reload, zero prescaler; callback only uses its read-latch operation.
     */
    if((rd(SYS+0xb8)&0x13ff0000U)!=0x10000000U)return 0;
    (void)uboot_cycle(0,0,BLOCK,0); /* Warm the identical loop while neutral. */
    for(i=0;i<SAMPLES;++i) {
        before=ticks();
        (void)uboot_cycle(0,0,BLOCK,0);
        delta=ticks()-before;
        /* Require advancing hardware with IRQ/FIQ masked. The <0.5ms bound
         * relies on this fixed short block fitting within one 1ms rollover;
         * aliased multi-wrap/hardware-stall cases are NOT ruled out by delta.
         */
        if(delta<8U || delta>6000U)return 0;
        total+=delta;
    }
    return total;
}
static u32 scaled_count(u32 denominator) {
    /* Exact floor((1048576 *120000)/denominator), no runtime/library ABI.
     * Numerator =0x1d4c000000. Admitted denominator>=60000 keeps each
     * intermediate remainder below4800000. Exactly32 neutral iterations.
     */
    u32 rem=0x1dU,low=0x4c000000U,quotient=0,bit;
    for(bit=0x80000000U;bit;bit>>=1) {
        rem=(rem<<1)|((low&bit)!=0);
        if(rem>=denominator) { rem-=denominator; quotient|=bit; }
    }
    return quotient;
}
static int calibrate(const struct snap *s) {
    struct snap after;
    u32 a,b,lo,hi,count;
    state.calibrated=0;
    a=calibration_pass(); b=calibration_pass();
    state.ticks_a=a; state.ticks_b=b;
    if(a<60000U || b<60000U || a>2400000U || b>2400000U)return BAD_TIMER;
    lo=a<b?a:b; hi=a>b?a:b;
    if(hi-lo>lo/10U)return CAL_UNSTABLE;
    /* 12MHz *10ms. Fixed division is DEENERGIZED only.
     * Choose the slower pass (smaller loop count), avoiding a positive bias.
     */
    count=scaled_count(hi);
    if(count<1000U || count>2097152U)return BAD_TIMER;
    snapshot(&after);
    if(!same(s,&after))return CONFIG_CHANGED;
    state.count=count; state.cal=*s; state.calibrated=1;
    return OK;
}

__attribute__((section(".text.entry")))
int ENTRY(int argc,char **argv) {
    struct snap s,after;
    u32 pin=0,saved=0,active=0,residue=0,before=0;
    int command=0,result=OK;
    if(!abi_ok() || state.token!=TOKEN)return BAD_ABI; /* No unsafe UART fallback. */
    if(argc==1 || (argc==2 && eq(argv[1],"STATUS")))command=0;
    else if(argc==2 && eq(argv[1],"CALIBRATE_ONLY"))command=1;
    else if(argc==2 && eq(argv[1],"PULSE6_ONCE")) { command=2; pin=6; }
    else if(argc==2 && eq(argv[1],"PULSE7_ONCE")) { command=2; pin=7; }
    else return BAD_ARG;
    snapshot(&s);
    if(command==0) {
        ubprint("IC1 STATUS 74=%08x 78=%08x DIR=%08x OUT=%08x IN=%08x P0=%08x P67=%08x PLL=%08x/%08x/%08x CTL=%08x USED=%x CAL=%x\n",
                s.sys74,s.sys78,s.dir,s.out,s.in,s.pull0,s.pull_pair&0x30U,
                s.pll0,s.pll4,s.pll8,s.ctl,state.used,state.calibrated);
        return OK;
    }
    if(state.used)return USED;
    if(command==2)state.used=1; /* Consume explicit attempt even on refusal. */
    if(!idle(&s))result=NOT_IDLE;
    else if(command==2 && (!state.calibrated || !same(&s,&state.cal)))result=NO_CALIBRATION;
    else {
        saved=mask();
        snapshot(&after);
        if(!same(&s,&after))result=CONFIG_CHANGED;
        else result=calibrate(&s); /* Inert or fresh recheck before explicit pulse. */
        if(result==OK && command==2) {
            before=ticks(); /* Only while pair00. */
            snapshot(&after);
            if(!same(&s,&after))result=CONFIG_CHANGED;
            else {
                /* Assembly returns ONLY AFTER mandatory saved00 release/drain.
                 * No callbacks, polls, calls, logging or error exits energized.
                 */
                active=uboot_cycle(GPIO+0x0c,s.out,state.count,1U<<pin);
                snapshot(&after); /* Pair already released. */
                residue=ticks()-before; /* MODULO residue, NOT pulse elapsed! */
                if(after.out!=s.out)result=RELEASE_BAD;
                else if(!same(&s,&after))result=CONFIG_CHANGED;
                else if(active!=(s.out|(1U<<pin)))result=DRIVE_BAD;
            }
        }
        restore(saved); /* All possible paths here are neutral/release attempted. */
    }
    ubprint("IC1 RESULT rc=%x pin=%x loops=%x ta=%x tb=%x active=%x OUT=%x residue_NOT_elapsed=%x used=%x\n",
            result,pin,state.count,state.ticks_a,state.ticks_b,active,rd(GPIO+0x0c),residue,state.used);
    return result;
}
