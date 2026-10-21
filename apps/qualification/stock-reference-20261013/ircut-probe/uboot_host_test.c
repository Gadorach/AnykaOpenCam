/* SPDX-License-Identifier: GPL-2.0-only
 * Full C control-flow fixture. MMIO, timer, CPSR and assembly are plain models.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef unsigned int u32;
static u32 sys74,sys78,dir0,out0,pull0,pull67,pll[3],cpctl,psr;
static u32 now_ticks,cycles,timer_calls,stores,prints,masks,restores,energized;
static u32 drive_fault,release_fault,config_fault,clock_fault,timer_fault,unstable;
static u32 inert_blocks,selected,irq_pause_simulated;
static u32 rd(u32 a) {
    switch(a) {
    case 0x08000074:return sys74;
    case 0x08000078:return sys78;
    case 0x20170000:return dir0;
    case 0x2017000c:return out0;
    case 0x20170018:return 1; /* GPIO0 input is not its latch. */
    case 0x08000080:return pull0;
    case 0x08000084:return pull67;
    case 0x08000000:return pll[0];
    case 0x08000004:return pll[1];
    case 0x08000008:return pll[2];
    case 0x080000b8:return timer_fault==3?0:0x14000000;
    default:assert(!"non-allowlisted read");return 0;
    }
}
static u32 control(void) { return cpctl; }
static int abi_ok(void) { return cpctl==0 && (psr&0x1f)==0x13; }
static u32 mask(void) { u32 s=psr; assert(!energized);psr|=0xc0;++masks;return s; }
static void restore(u32 s) { assert(!energized && (psr&0xc0)==0xc0);psr=s;++restores; }
static u32 ticks(void) { assert(!energized && (psr&0xc0)==0xc0);++timer_calls;return now_ticks; }
static u32 uboot_cycle(u32 address,u32 neutral,u32 count,u32 pairmask) {
    assert((psr&0xc0)==0xc0 && count>0);
    ++cycles;
    if(!pairmask) {
        assert(address==0 && neutral==0 && !energized);
        ++inert_blocks;
        if(timer_fault!=1)now_ticks+=timer_fault==2?7000:count/8;
        if(unstable && inert_blocks>1025)now_ticks+=count/8;
        if(config_fault && inert_blocks==2000)sys74^=4;
        return 0;
    }
    assert(address==0x2017000c && neutral==out0 && !(neutral&0xc0));
    assert(pairmask==0x40 || pairmask==0x80);
    assert(count>=1000 && count<=2097152);
    selected=pairmask==0x40?6:7;
    energized=1;out0=neutral|pairmask;++stores;
    /* No callback, simulated PC/client pause cannot change finite sequence. */
    irq_pause_simulated=1;
    u32 active=drive_fault?neutral:out0;
    now_ticks+=count/8;
    out0=neutral;energized=0;++stores;
    if(release_fault)out0|=pairmask; /* Bad readback, release was attempted. */
    if(clock_fault)pll[0]^=1;
    return active;
}
static int ubprint(const char *fmt,...) {
    assert(!energized && fmt && strchr(fmt,'\033')==NULL);
    ++prints;return 0;
}
#define UBOOT_HOST 1
#include "uboot_probe.c"

static void reset_model(void) {
    sys74=0x186;sys78=0x50;dir0=0x008000e6;out0=2;pull0=0;pull67=0x30;
    pll[0]=1;pll[1]=2;pll[2]=3;cpctl=0;psr=0x13;
    now_ticks=cycles=timer_calls=stores=prints=masks=restores=energized=0;
    drive_fault=release_fault=config_fault=clock_fault=timer_fault=unstable=0;
    inert_blocks=selected=irq_pause_simulated=0;
    state=(struct state){TOKEN,0,0,0,0,0,{0,0,0,0,0,0,0,0,0,0,0}};
}
static int cmd(const char *text) {
    char *args[3]={(char *)"82001000",(char *)text,NULL};
    return uboot_probe_entry(text?2:1,args);
}
static void neutral_no_output(void) {
    assert(!energized && stores==0 && masks==restores && psr==0x13);
}
int main(void) {
    unsigned cases=0,i;
    for(i=60000;i<=2400000;i+=30000) {
        assert(scaled_count(i)==(u32)((1048576ULL*120000ULL)/i));++cases;
    }
    reset_model();assert(cmd(NULL)==OK);neutral_no_output();assert(!cycles && !timer_calls && !masks && prints==1);++cases;
    reset_model();assert(cmd("STATUS")==OK);neutral_no_output();assert(!cycles && !timer_calls);++cases;
    const char *bad[]={"PULSE0_ONCE","PULSE1_ONCE","PULSE2_ONCE","PULSE41_ONCE","PULSE42_ONCE", "PULSE6_ONCE ","pulse6_once","PULSE6","","7","CALIBRATE_ONLYx"};
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        reset_model();assert(cmd(bad[i])==BAD_ARG);neutral_no_output();assert(!state.used && !prints);++cases;
    }
    reset_model();cpctl=1;assert(cmd(NULL)==BAD_ABI);neutral_no_output();assert(!prints);++cases;
    for(i=0;i<8;++i) {
        reset_model();
        if(i<4)sys78^=1U<<(i+4);
        else if(i==4)sys74|=1;
        else if(i==5)dir0|=1;
        else dir0^=1U<<i;
        assert(cmd("CALIBRATE_ONLY")==NOT_IDLE);neutral_no_output();++cases;
    }
    for(i=1;i<4;++i) {
        reset_model();out0|=i<<6;assert(cmd("CALIBRATE_ONLY")==NOT_IDLE);neutral_no_output();++cases;
    }
    reset_model();assert(cmd("PULSE6_ONCE")==NO_CALIBRATION);neutral_no_output();assert(state.used);assert(cmd("PULSE7_ONCE")==USED);++cases;
    for(i=1;i<4;++i) {
        reset_model();timer_fault=i;assert(cmd("CALIBRATE_ONLY")==BAD_TIMER);neutral_no_output();assert(!state.calibrated);++cases;
    }
    reset_model();unstable=1;assert(cmd("CALIBRATE_ONLY")==CAL_UNSTABLE);neutral_no_output();++cases;
    reset_model();config_fault=1;assert(cmd("CALIBRATE_ONLY")==CONFIG_CHANGED);neutral_no_output();++cases;
    reset_model();assert(cmd("CALIBRATE_ONLY")==OK);neutral_no_output();
    assert(state.count==960000 && state.ticks_a==131072 && state.ticks_b==131072 && state.calibrated);++cases;
    reset_model();now_ticks=0xfffff000U;assert(cmd("CALIBRATE_ONLY")==OK);neutral_no_output();assert(state.count==960000);++cases;
    /* All unrelated OUT0 patterns retained, selected pair never11. */
    for(i=0;i<32;++i) {
        u32 baseline;
        if(i==6 || i==7)continue;
        reset_model();out0|=1U<<i;baseline=out0;
        assert(cmd("CALIBRATE_ONLY")==OK);
        assert(cmd(i&1?"PULSE7_ONCE":"PULSE6_ONCE")==OK);
        assert(stores==2 && out0==baseline && selected==((i&1)?7U:6U));
        assert(!energized && masks==restores && psr==0x13 && irq_pause_simulated);
        assert(cmd("PULSE6_ONCE")==USED && cmd("PULSE7_ONCE")==USED && stores==2);++cases;
    }
    reset_model();assert(cmd("CALIBRATE_ONLY")==OK);pll[0]^=1;
    assert(cmd("PULSE6_ONCE")==NO_CALIBRATION);neutral_no_output();assert(state.used);++cases;
    reset_model();assert(cmd("CALIBRATE_ONLY")==OK);timer_fault=1;
    assert(cmd("PULSE6_ONCE")==BAD_TIMER);neutral_no_output();assert(state.used);++cases;
    reset_model();assert(cmd("CALIBRATE_ONLY")==OK);drive_fault=1;
    assert(cmd("PULSE6_ONCE")==DRIVE_BAD);assert(stores==2 && out0==2 && !energized && masks==restores);++cases;
    reset_model();assert(cmd("CALIBRATE_ONLY")==OK);release_fault=1;
    assert(cmd("PULSE7_ONCE")==RELEASE_BAD);assert(stores==2 && !energized && masks==restores);++cases;
    reset_model();assert(cmd("CALIBRATE_ONLY")==OK);clock_fault=1;
    assert(cmd("PULSE6_ONCE")==CONFIG_CHANGED);assert(stores==2 && out0==2 && masks==restores);++cases;
    printf("{\"result\":\"HOST_PASS\",\"cases\":%u,\"hardware_access\":false,\"native_executed\":false}\n",cases);
    return 0;
}
