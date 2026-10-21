/* SPDX-License-Identifier: GPL-2.0-only
 * CANCELLED DRAFT: parent superseded this adapter with an explicit neutral-only
 * U-Boot MMIO recipe review. Not built/tested/admitted; do not stage or deploy.
 * Separate neutral bridge owner on a NEW RAM page; native UNEXECUTED.
 * Planned generated base was the helper prefix of our retained uboot_probe.c
 * (before its entry function), not a modified old probe or retail implementation.
 * No setup on STATUS/CALIBRATE/PULSE. Exact explicit setup/restore only.
 */
#include "uboot_bridge2_base.h"
#ifndef UBOOT_HOST
static void wr(u32 address,u32 value) { *(volatile u32 *)address=value; }
#define B2_ENTRY _start
#else
#define B2_ENTRY uboot_bridge2_entry
#endif
#define PAIR 0xc0U
#define MUX 0xf0U
#define T3 (SYS+0xc8U)
#define T4 (SYS+0xd0U)
#define ENABLE 0x10000000U
#define B2_TOKEN 0x42523231U
enum { NEED_SETUP=11, TIMER_OWNED=12, SETUP_HOLD=13 };
struct bridge_state {
    u32 token,setup_attempted,ready,restore_attempted,fault,stage,pulsing;
    struct snap original;
    u32 timer3,timer4;
};
static struct bridge_state bridge={B2_TOKEN,0,0,0,0,0,0,{0,0,0,0,0,0,0,0,0,0,0},0,0};

static int pads_neutral(const struct snap *s) {
    return !(s->sys74&1U) && !(s->dir&1U) && !(s->out&PAIR) && !(s->in&PAIR);
}
static int timers_off(void) { return !((rd(T3)|rd(T4))&ENABLE); }
/* mode0=original, mode1=owned GPIO outputs00, mode2=original mux/output DIR.
 * No repair: every unrelated register bit must match saved ownership exactly.
 */
static int owned(const struct snap *current,u32 mode) {
    struct snap expected=bridge.original;
    if(mode==1)expected.sys78=(expected.sys78&~MUX)|0x50U;
    if(mode)expected.dir|=PAIR;
    return pads_neutral(current) && same(current,&expected) && timers_off() &&
        rd(T3)==bridge.timer3 && rd(T4)==bridge.timer4;
}
static int setup_neutral(void) {
    struct snap s;
    if(bridge.setup_attempted || bridge.restore_attempted || state.used || bridge.fault)return USED;
    bridge.setup_attempted=1;
    snapshot(&s);
    /* This unit admits only the observed alternate0/input baseline, not every
     * hypothetical original mux. Pads/latches00 and GPIO0 INPUT are mandatory.
     */
    if(!pads_neutral(&s) || (s.sys78&MUX) || (s.dir&PAIR))return NOT_IDLE;
    if(!timers_off())return TIMER_OWNED;
    bridge.original=s; bridge.timer3=rd(T3); bridge.timer4=rd(T4); bridge.stage=1;
    if(!owned(&s,0))return CONFIG_CHANGED;
    /* OUT pair was checked00; never clear somebody else's energized output.
     * DIR first, then both mux fields together. Neither write can request HIGH.
     */
    wr(GPIO,s.dir|PAIR);
    snapshot(&s);
    if(!owned(&s,2))return CONFIG_CHANGED;
    bridge.stage=2;
    wr(SYS+0x78,(s.sys78&~MUX)|0x50U);
    snapshot(&s);
    if(!owned(&s,1))return CONFIG_CHANGED;
    bridge.stage=3; bridge.ready=1; state.calibrated=0;
    return OK;
}
static int restore_neutral(void) {
    struct snap s;
    if(bridge.restore_attempted)return USED;
    bridge.restore_attempted=1;
    if(!bridge.ready || bridge.fault || bridge.pulsing)return SETUP_HOLD;
    snapshot(&s);
    if(!owned(&s,1))return CONFIG_CHANGED;
    /* Return to already-observed inactive alternate pads FIRST; never alter
     * timers. Verify mux+neutral pads before changing direction to original.
     */
    wr(SYS+0x78,(s.sys78&~MUX)|(bridge.original.sys78&MUX));
    snapshot(&s);
    if(!owned(&s,2))return CONFIG_CHANGED;
    bridge.stage=4;
    wr(GPIO,(s.dir&~PAIR)|(bridge.original.dir&PAIR));
    snapshot(&s);
    if(!owned(&s,0))return CONFIG_CHANGED;
    bridge.stage=5; bridge.ready=0; state.calibrated=0;
    return OK;
}
static int calibration_or_pulse(u32 pin,u32 *active,u32 *residue) {
    struct snap s,after;
    u32 before;
    int result;
    if(state.used)return USED;
    if(pin)state.used=1; /* Consume even a refused explicit PULSE; no rearm. */
    if(!bridge.ready || bridge.fault || bridge.restore_attempted)return NEED_SETUP;
    snapshot(&s);
    if(!owned(&s,1))return CONFIG_CHANGED;
    if(pin && (!state.calibrated || !same(&s,&state.cal)))return NO_CALIBRATION;
    result=calibrate(&s); /* Our old finite calibration helpers, unchanged. */
    snapshot(&after);
    if(result==OK && !owned(&after,1))result=CONFIG_CHANGED;
    if(result!=OK || !pin)return result;
    before=ticks(); /* NEUTRAL only. Timer3/4 checked again before drive. */
    snapshot(&after);
    if(!owned(&after,1))return CONFIG_CHANGED;
    bridge.pulsing=1;
    *active=uboot_cycle(GPIO+0x0c,s.out,state.count,1U<<pin);
    bridge.pulsing=0; /* Assembly has ALREADY attempted00 release and drained it. */
    snapshot(&after);
    *residue=ticks()-before; /* Modulo residue, not actual pulse elapsed time. */
    if(after.out!=s.out)return RELEASE_BAD;
    if(!owned(&after,1))return CONFIG_CHANGED;
    if(*active!=(s.out|(1U<<pin)))return DRIVE_BAD;
    return OK;
}
static void report(u32 result,u32 pin,u32 active,u32 residue) {
    struct snap s;
    snapshot(&s);
    ubprint("BR2 rc=%x pin=%x SET=%x READY=%x REST=%x FAULT=%x STAGE=%x USED=%x CAL=%x\n",
            result,pin,bridge.setup_attempted,bridge.ready,bridge.restore_attempted,
            bridge.fault,bridge.stage,state.used,state.calibrated);
    ubprint("BR2 74=%x 78=%x DIR=%x OUT=%x IN=%x P0=%x P67=%x PLL=%x/%x/%x CTL=%x T3=%x T4=%x\n",
            s.sys74,s.sys78,s.dir,s.out,s.in,s.pull0,s.pull_pair,s.pll0,s.pll4,s.pll8,s.ctl,rd(T3),rd(T4));
    ubprint("BR2 SAVED78=%x DIR=%x OUT=%x P0=%x P67=%x T3=%x T4=%x loops=%x ta=%x tb=%x active=%x residue_NOT_elapsed=%x\n",
            bridge.original.sys78,bridge.original.dir,bridge.original.out,
            bridge.original.pull0,bridge.original.pull_pair,bridge.timer3,bridge.timer4,
            state.count,state.ticks_a,state.ticks_b,active,residue);
}
__attribute__((section(".text.bridge2_entry")))
int B2_ENTRY(int argc,char **argv) {
    u32 command=0,pin=0,saved,result=OK,active=0,residue=0;
    /* Fresh page requires D-cache OFF as well as old physical-address ABI. */
    if(!abi_ok() || (control()&5U) || state.token!=TOKEN || bridge.token!=B2_TOKEN)return BAD_ABI;
    if(argc==1 || (argc==2 && eq(argv[1],"STATUS")))command=0;
    else if(argc==2 && eq(argv[1],"SETUP_NEUTRAL"))command=1;
    else if(argc==2 && eq(argv[1],"RESTORE_NEUTRAL"))command=2;
    else if(argc==2 && eq(argv[1],"CALIBRATE_ONLY"))command=3;
    else if(argc==2 && eq(argv[1],"PULSE6_ONCE")) { command=4; pin=6; }
    else if(argc==2 && eq(argv[1],"PULSE7_ONCE")) { command=4; pin=7; }
    else return BAD_ARG;
    if(command) {
        saved=mask();
        if(command==1)result=setup_neutral();
        else if(command==2)result=restore_neutral();
        else result=calibration_or_pulse(pin,&active,&residue);
        if(result!=OK)bridge.fault=1; /* HOLD; no hidden retries/repair. */
        restore(saved);
    }
    report(result,pin,active,residue); /* No pulse/release pending. */
    return (int)result;
}
