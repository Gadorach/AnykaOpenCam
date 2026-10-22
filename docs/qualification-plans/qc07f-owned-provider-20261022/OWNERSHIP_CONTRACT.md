# Mandatory provider adoption — contract before implementation

2026-10-22; RP-07/09, STREAM-04/07 dependency. SOURCE/compile candidate only.
This contract precedes implementation, not hardware/provider/backing admission.

## Trusted bootstrap / serialization

Bootstrap independently establishes the exact current boot/image/source/driver
association, expected device/session/source/file identities, permitted hardware
scope, admitted memory/backing, recovery and exclusive provider use. In particular
the selected encoder requires the real version1/88-byte OPEN_QUERY candidate;
frozen old76 returns ENOTTY and cannot use this source. Global56-byte lifetime is
required in addition, never substituted for per-open identity. Metadata, O_RDWR,
query results and caller pointer/flag values confer no authority or DMA health.

All native calls, staging, source callbacks and FD-table access are one serialized
context/thread, without signal reentry, concurrent close/I/O, fork/dup/SCM aliases
or competing provider operations. No lock/ref/guardian framework is introduced.
Arguments, expected identities, slots and output metadata are stable and disjoint
from each other and native private records/backing/loans. No runtime alias proof.

## Setup and supported scope

Fresh native singleton init -> watchdog take UNKNOWN -> original storage select
and explicit exact139 opt-in -> mandatory provider transfer -> original one-shot
memory start/fixed buffers -> NEXT/release/normal retirement -> healthy watchdog
give-back. Storage/opt-in guards are not disabled: call them before role transfer.
The memory API's arbitrary external hub pointer/backing is still NOT an admitted
owned provider/memory contract; this increment cannot remove that blocker.

Only the original1280x720 H.264 IP-stream path (h264_p_frames=1) is supported.
IDR-only temporary encoder role is unimplemented and refused before effects.
The selected stream path's temporary encoder open is unused by the job and may be
removed, not aliased to the persistent role. Persistent role remains one distinct
description across frames, released only on original healthy normal retirement.
No main, RTSP rebase, network/listener/provisioning or native operation is selected.

## Nonatomic transfer and actual FD custody

Native context gains stable staging records for the byte-imported reviewed frame
and encoder helpers, plus a sensor staging slot. No actual owned FD lives only in
a returning local variable. Expected identities and valid snapshots are retained
separately from active FD slots; snapshot copies are not extra FD ownership.

Precheck argument/role availability and sensor metadata (GET-only fcntl/fstat,
O_RDWR/CLOEXEC/expected char device), then helper-move encoder, helper-move frame,
then move sensor. Helper failures before their commit leave that bootstrap slot
owned by bootstrap. Once any move succeeds, any later failure fences the native
context and retains its actual moved descriptions/process; remaining nonnegative
bootstrap slots still belong to bootstrap. The call is not an atomic transaction.

For full activation transfer, sensor moves to the real native slot; helper
give-back reobserves fresh NONE and moves frame/encoder into their real native
slots. On successful helper give-back its old record is EMPTY/fd=-1. No native and
custody record simultaneously claim the same description. On failure the helper
retains its actual FD/HOLD; any earlier committed native slots remain owned.
There is no rollback, destructor, failed-close retry, cleanup/reset or take retry.
An initial helper observation failure also fences, even when no role moved;
all bootstrap slots then remain unchanged, not silently native-owned.
Expected identities and latest fully valid snapshots known to the context remain
on uncertainty. Failed/malformed/identity-mismatched replies never replace them.
Precommit helper failure does not export its uncommitted reply: that description
remains bootstrap-owned, not a claimed native snapshot/description.

## Admission, activation and actual use

COMMITTED means actual transfers completed, not a readiness/permission flag. Every
effect admission additionally requires all real active slots and no staged slots,
matching metadata, actual expected frame per-open receipt and encoder query, plus
original global quarantine/lifetime facts. This observational check precedes
watchdog begin; its cost is charged to the same outer source2s start. It is repeated
by the real guarded lifetime path before allocation/capture/encode.

Preloading frame FD must not skip setup: a separate explicit source phase enters
the real BIND/fresh NONE/exact expected source+file check, guarded I2C_SLAVE0x30 and
sensor register validation, ARM/reservation/exact139/BEGIN/sensor-on sequence.
No register/selecting I2C effect occurs during metadata-only adoption. Subsequent
capture uses that same source/file/generation and no internal reopen fallback.
Actual stream job goes to the adopted persistent encoder; successful returned
codec frame number and the real stream count constrain later THIS_OPEN snapshots.
OTHER_OPEN, unknown/fault schema or missing original global facts fail closed.
Saturating telemetry is retained, never treated as a unique operation identity.

## Retirement, failure and limits

Original keyed END/QUERY/RETIRED/ACK and sensor-off ordering remains. Before closing
the persistent reference, require real latest per-open/global facts; its actual
driver release clears that file's reference under enc.lock. This is not an app
reset, error recovery or a substitute for DMA/electrical retirement. Provider
close-once clears each active slot before its single Linux close attempt; a failed
former number is diagnostic NOT owned. Keep expected identities/snapshots on HOLD.
No failed numeric-FD retry or descriptor resurrection.
Healthy handback of adopted but never-activated roles is not implemented. Normal
retire APIs refuse that known COMMITTED phase before rearm/query/effects, retaining
all actual custody. No fake ACTIVE/END/ACK proof or cleanup-close is substituted;
an external real retention owner would still be required on abort before capture.

Reviewed watchdog keeps one actual description, one-shot original10s, phase<2s,
nominal5s, uncertainty-first/no-effect held-loan BUSY and zero outward failures.
Capture+encode+both lifetime/cancel/deadline postchecks precede logical disable;
real normal retirement precedes disable and deferred CPU free. Failed disable
does not guarantee armed recovery; metadata is not electrical stop/DMA drain.

Old large raw/1MiB allocations remain128KiB/product-memory HOLD. Original sensor139,
table, three memory-policy units,40MiB/CMA and strict native2MiB/HOST4MiB policies
stay unchanged. Native globals retain actual FDs/backing only while this actual
process lives: the common RTSP failed-next external process owner is still missing.
Provisioning, listener, backing, ordinary functional/native timing/guard/full app/
G2/G3/G8 acceptance remain absent. Compile/archive/REL only; no tests or execution.
