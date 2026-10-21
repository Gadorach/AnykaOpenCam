/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* No OS/device access. These registers and elapsed values are just a model. */
static unsigned int s74, s78, dir0, out0, writes, delays, locked;
static unsigned int selected, fault_drive, fault_release, drift, detached;
static char trace[64];
static unsigned int trace_n;
static void event(char c) { assert(trace_n + 1 < sizeof(trace)); trace[trace_n++] = c; }
static void lock_model(unsigned long *f) { assert(!locked); *f = 7; locked = 1; event('L'); }
static void unlock_model(unsigned long f) { assert(locked && f == 7); locked = 0; event('U'); }
static unsigned int read_out(void) { assert(locked); event('R'); return out0; }
static void write_out(unsigned int value) {
    assert(locked);
    assert((value & ~0xc0U) == 2U); /* No GPIO0/1/2 etc changes. */
    assert((value & 0xc0U) != 0xc0U); /* Never simultaneous pair11. */
    ++writes; event(writes == 1 ? 'A' : 'Z');
    if (writes == 1) {
        assert(value == (2U | (1U << selected)));
        out0 = fault_drive ? 2U : value;
    } else {
        assert(writes == 2 && delays == 10 && value == 2U);
        out0 = fault_release ? (2U | (1U << selected)) : value;
    }
}
static void delay_model(void) {
    assert(locked && writes == 1 && delays < 10);
    ++delays; event('D');
    if (delays == 3) detached = 1; /* Client loss has no branch/hook in sequence. */
    if (drift && delays == 5) s78 ^= 0x100U;
}
#define IC_SYS74() (s74)
#define IC_SYS78() (s78)
#define IC_DIR0() (dir0)
#define IC_OUT0() read_out()
#define IC_WRITE_OUT0(v) write_out(v)
#define IC_LOCK(f) lock_model(&(f))
#define IC_UNLOCK(f) unlock_model(f)
#define IC_DELAY_1000US() delay_model()
#define IC_ALLOWED 0
#define IC_NAME disabled
#include "pulse_sequence.h"
#undef IC_ALLOWED
#define IC_ALLOWED 1
#undef IC_NAME
#define IC_NAME pulse6
#include "pulse_sequence.h"
#undef IC_NAME
#define IC_NAME pulse7
#include "pulse_sequence.h"
#undef IC_NAME
#define IC_NAME drive_failure
#include "pulse_sequence.h"
#undef IC_NAME
#define IC_NAME release_failure
#include "pulse_sequence.h"
#undef IC_NAME
#define IC_NAME config_failure
#include "pulse_sequence.h"

static void reset_model(void) {
    s74 = 0x186; s78 = 0x50; dir0 = 0x008000e6; out0 = 2;
    writes = delays = locked = trace_n = 0;
    fault_drive = fault_release = drift = detached = 0;
    memset(trace, 0, sizeof(trace));
}
static void no_writes(void) { assert(writes == 0 && delays == 0 && !locked); }
static void pulse_completed(void) {
    assert(writes == 2 && delays == 10 && !locked && detached);
    assert(strstr(trace, "ARDDDDDDDDDDZR") != NULL);
}
int main(void) {
    unsigned int i, cases = 0;
    _Static_assert(sizeof(unsigned int) == 4, "register word");
    reset_model(); assert(disabled(6) == IC_DISABLED); no_writes(); assert(trace_n == 0); ++cases;
    for (i = 0; i < 64; ++i) {
        if (i == 6 || i == 7) continue;
        reset_model(); assert(pulse6(i) == IC_BAD_PIN); no_writes(); assert(trace_n == 0); ++cases;
    }
    /* Reject each mismatched field and any initially energized state, no repair. */
    for (i = 0; i < 4; ++i) {
        reset_model(); out0 |= i << 6;
        if (i) { assert(pulse6(6) == IC_NOT_IDLE); no_writes(); ++cases; }
    }
    for (i = 4; i < 8; ++i) {
        reset_model(); s78 ^= 1U << i; assert(pulse6(6) == IC_NOT_IDLE); no_writes(); ++cases;
    }
    for (i = 0; i < 3; ++i) {
        reset_model(); dir0 ^= 1U << (i == 0 ? 0 : i + 5);
        assert(pulse6(6) == IC_NOT_IDLE); no_writes(); ++cases;
    }
    reset_model(); s74 |= 1; assert(pulse6(6) == IC_NOT_IDLE); no_writes(); ++cases;
    reset_model(); selected = 6; assert(pulse6(6) == IC_OK); pulse_completed(); assert(out0 == 2); ++cases;
    reset_model(); selected = 7; assert(pulse7(7) == IC_OK); pulse_completed(); assert(out0 == 2); ++cases;
    reset_model(); assert(pulse6(7) == IC_USED); no_writes(); ++cases;
    reset_model(); selected = 6; fault_drive = 1;
    assert(drive_failure(6) == IC_DRIVE_MISMATCH); pulse_completed(); assert(out0 == 2); ++cases;
    reset_model(); assert(drive_failure(6) == IC_USED); no_writes(); ++cases;
    reset_model(); selected = 7; fault_release = 1;
    assert(release_failure(7) == IC_RELEASE_MISMATCH); pulse_completed(); ++cases;
    reset_model(); assert(release_failure(7) == IC_USED); no_writes(); ++cases;
    reset_model(); selected = 6; drift = 1;
    assert(config_failure(6) == IC_CONFIG_CHANGED); pulse_completed(); assert(out0 == 2); ++cases;
    printf("{\"result\":\"HOST_MODEL_PASS\",\"cases\":%u,\"hardware_access\":false,\"measured_pulse\":false}\n", cases);
    return 0;
}
