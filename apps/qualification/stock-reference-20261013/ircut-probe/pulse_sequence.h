/* SPDX-License-Identifier: GPL-2.0-only
 * Our fixed-pair pulse sequence. Hook substitution is HOST MODEL ONLY unless
 * exact kernel ABI, timing and exclusive hardware ownership are qualified.
 * Include with IC_NAME, IC_ALLOWED and the IC_* fixed-register/atomic hooks.
 * No arbitrary register, pin, polarity, duration or repeated-command interface.
 */
#ifndef QC_IRCUT_RESULT_TYPES
#define QC_IRCUT_RESULT_TYPES
enum ic_result {
    IC_OK = 0, IC_DISABLED, IC_BAD_PIN, IC_USED, IC_NOT_IDLE,
    IC_DRIVE_MISMATCH, IC_RELEASE_MISMATCH, IC_CONFIG_CHANGED
};
#endif

static enum ic_result IC_NAME(unsigned int pin)
{
    static unsigned int used;
    unsigned long flags;
    unsigned int sys74, sys78, dir, out, active, released, i;
    enum ic_result result;

    /* Native adapter fixes this to zero, with no runtime enabling parameter. */
    if (!IC_ALLOWED)
        return IC_DISABLED;
    if (pin != 6 && pin != 7)
        return IC_BAD_PIN;

    IC_LOCK(flags);
    if (used) {
        IC_UNLOCK(flags);
        return IC_USED;
    }
    sys74 = IC_SYS74();
    sys78 = IC_SYS78();
    dir = IC_DIR0();
    out = IC_OUT0();
    /* GPIO0 must remain GPIO INPUT. GPIO6/7 must ALREADY be GPIO outputs00.
     * Never repair mux/direction, clear a pre-existing energized pair, or touch
     * GPIO0/1/2/41/42. Rejected preflight performs no register writes.
     */
    if ((sys74 & 1U) || (sys78 & 0xf0U) != 0x50U ||
        (dir & 0xc1U) != 0xc0U || (out & 0xc0U)) {
        IC_UNLOCK(flags);
        return IC_NOT_IDLE;
    }
    used = 1; /* Consume before drive; failed drive/release cannot be retried. */

    /* Critical interval: NO conditional exit, logs, copies, allocation, sleep,
     * scheduling, mutex acquisition, userspace access or callback dispatch.
     * Each hook in the kernel adapter must be a fixed MMIO operation or the
     * audited finite calibrated delay. Readback drains the drive write.
     * Saved whole OUT0 is valid ONLY with exclusive writers on a UP target.
     */
    IC_WRITE_OUT0(out | (1U << pin));
    active = IC_OUT0();
    for (i = 0; i < 10; ++i)
        IC_DELAY_1000US(); /* ARM3.4 udelay(10000) exceeds MAX_UDELAY_MS=2. */
    IC_WRITE_OUT0(out);   /* Both pair bits were checked zero, including peer. */
    released = IC_OUT0();

    /* Every diagnostic decision happens AFTER unconditional release attempt. */
    if (released != out)
        result = IC_RELEASE_MISMATCH;
    else if (IC_SYS74() != sys74 || IC_SYS78() != sys78 || IC_DIR0() != dir)
        result = IC_CONFIG_CHANGED;
    else if (active != (out | (1U << pin)))
        result = IC_DRIVE_MISMATCH;
    else
        result = IC_OK;
    IC_UNLOCK(flags);
    return result;
}
