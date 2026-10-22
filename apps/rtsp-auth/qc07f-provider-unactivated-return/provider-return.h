/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QP_PRE_MEMORY_PROVIDER_RETURN_H
#define QP_PRE_MEMORY_PROVIDER_RETURN_H
#include "owned-provider.h"
/* ONLY healthy fully adopted COMMITTED, before memory/session/control/activation/
 * local job/history/backing/loan/close. Stop/uncertainty/HOLD/clock failure first.
 * Not post-memory or partial failed take, ACTIVE handback, END/ACK or recovery.
 *
 * Four distinct stable/disjoint writable int destinations, EACH -1. Original
 * trusted exact boot/image/provider/prior-DMA admission and sole serialized
 * nonaliased descriptions still required; metadata/NONE is NOT that proof.
 * No recipient IO, reentry or concurrent FD/provider changes until call returns.
 *
 * Observe actual nodes/frame56/query88/global56 then move sensor, frame, encoder,
 * UNKNOWN watchdog LAST. No controls/I2C selector/ARM/job/END/ACK/reset/write/close.
 * Same real whole-return start checked <2s, not finite syscall or timer-headroom
 * proof. Watchdog was already started by preopen and is returned UNKNOWN, not
 * DISABLED. Bootstrap must retain/service/recover it under its OWN admitted policy;
 * success is NOT close permission, electrical/DMA retirement or armed recovery.
 *
 * NONATOMIC: on later failure, populated destinations own ACTUAL earlier FDs;
 * their native slots are -1. Remaining native FDs/process/snapshots stay on HOLD.
 * Never retry/rollback/close/destruct or assume either side owns all. Caller's
 * destination records and native real process must survive partial failure.
 * Success is terminal RETURNED, not normal CLOSED, no setup/reinit/restart.
 */
int nh_native_return_pre_memory_unknown_bootstrap(int *sensor_slot,int *frame_slot,
 int *encoder_slot,int *unknown_watchdog_slot);
#endif
