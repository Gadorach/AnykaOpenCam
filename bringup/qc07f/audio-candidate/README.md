# QC07F ADC2 capture-only Linux6.18.55 candidate

**Source/HOST/ARM-object candidate; probe unconditionally refuses hardware.**
Microphone remains required, native capture capability unavailable/unqualified.
No actual mic/audio samples, files, IRQ/ADC/L2/GPIO/clock/bias/card/target operations.
Finished PTZ/reset units, other source/config/image/runner/plans are unchanged.

- `capture_core.h/.c`: portable bounded source-derived ADC2/L2 ownership core.
- `qc07f-adc2.c`: capture-only Linux6 ALSA negotiation/allocation/trigger/pointer/
  bounded copy/sync-stop/free glue, read-only raw gain metadata. No playback/
  DAC/HP/speaker/AEC routes or heavy codec. Native provider/ALSA notification and
  card/substream lifecycle integration are blocked, not stubbed as working.
- `Kconfig`, `Makefile`, `candidate.dtsi`: default-off/disabled review artifacts;
  no baseline integration or guessed reg/IRQ/clock/bias/GPIO properties.
- [SOURCE_AUDIT.md](SOURCE_AUDIT.md): actual licensed/public/retail evidence,
  adaptation, side effects and concrete routing/provider/ALSA blockers.

## Implemented lifetime / data contract

### V2 correction: every pending operation has an owner-driven deadline

The parent found V1's tick checked capture state/closing before its deadline,
leaving CLAIM/CONFIG waits unbounded and cleanup timeout unreported. V1 source,
HANDOFF/run02 and prior run01 are byte-exact under the own-lane `v1-snapshot/`;
that original source/fixture handback is historical, not adoption authority.

`ac_pump` now checks an exact pending-operation deadline **before** those state
early returns; `now == deadline` expires. The100ms candidate cap is unchanged.

| Pending operation/state | At expiry | Retained until |
|---|---|---|
| CLAIM / CLAIMING | One reserved exact-owned STOP, fault latched | Normal STOP/RELEASE fences and all returns |
| CONFIGURE / CONFIGURING | Same; no late config/clock continuation | Same |
| CACHE_DEVICE / DMA_PERIOD / CACHE_CPU | Same; XRUN, no stale period publication | Same |
| STOP / STOPPING or QUARANTINED | Fault/quarantine only; no duplicate STOP, retry or free | Real normal STOP ACK, then RELEASE/returns |
| RELEASE / RELEASING or QUARANTINED | Fault/quarantine only; no duplicate RELEASE/STOP or free | Real normal RELEASE ACK |
| WAIT_BORROW, operation0 after known STOP ACK | No obsolete hardware timeout/retry; backing retained | Exact borrowed returns, then a newly deadline-bound RELEASE |
| Known CLAIM NO_EFFECT / CLOSED | No owned operation: no timeout cleanup submission | Already safe to release caller backing; failure remains latched |

Actual late normal STOP/RELEASE ACK may retire uncertainty, but fault stays
latched. Definitive cleanup failure still forbids contradictory later ACK/free.
Stale/out-of-order receipts cannot extend a pending deadline or claim completion.
All API fields/time are trusted owner data, never untrusted wire readiness flags.

**Timer integration is still a native blocker:** the future provider/resource
broker must invoke `ac_pump` independently in *every* waiting state, including
when no callback arrives. Current ALSA completion glue is not that timer. This
unit creates no timer/thread/supervisor and cannot preempt a blocked submit or
turn100ms logical tick deadlines into physical STOP guarantees. Probe refusal,
native provider/notification/card/substream lifetime blockers remain unchanged.

Build toolchain paths now expand `~` and resolve relative CLI paths against the
entry working directory **before** pin checks/compiler commands change cwd.

All methods are trusted-owner calls, not wire/backend/auth adapters. Single
caller serialization; provider submit must copy fixed requests, return without
blocking/reentering, and own resource/cache/DMA/IRQ/callback lifetime. One active
ADC2 work namespace plus strictly increasing generation and nonwrapping sequence;
typed resource admission masks are not physical evidence. Native-qualified stays0.
No physical address, arbitrary MMIO/path/PID/credentials crosses the core ABI.

CLAIM→CONFIGURE→PREPARED→cache-device→512B BUF2MEM DMA→cache-CPU→READY are actual
state transitions with exact tickets. Early/wrong-generation/late/out-of-order
receipts do not publish data. Missing/invalid period or clock/sample index is
XRUN/fail-closed. STOP and RELEASE tickets are reserved before effects so counter
exhaustion cannot block cleanup. STOP ACK fences **all** prior partial init,
DMA/cache/IRQ/callback effects; only RELEASE ACK plus all borrowed returns permits
coherent ring/provider/code retirement. Uncertain/failing submit/stop/release
retains/quarantines context/backing, never force-free/reinit/reset/retry. Definitive
STOP/RELEASE failure is terminal; no later contradictory ACK can free resources.

Four512B slots=**2048B** ring (family maximum64KiB is not this allocation). One
period request in flight, cache transitions explicit, borrowed spans never
overwritten. Full ring/slow-held consumer stops audio with XRUN, not implicit
data drops or video stalls. Close keeps held backing; exact return is needed.
Mute advances the nonwrapping track boundary, immediately denies new/old span
effects, and requests owned capture STOP; existing raw borrows remain allocated
until returned. Consumers must validate each borrowed effect; bytes already sent
cannot be recalled. There is no GPIO mute or fake silence track. Later protected
STREAM/WEB/STORE integration and actual mute/capture proof remain open.

Candidate finite rates8/16/24/48k are a narrow policy subset of public mono
5500–96000Hz family source, **not measured retail supported modes**. S16_LE mono
only, period256 samples. No default parent clock: provider must supply reviewed
parent epoch/Hz; exact parent/(OSR512 or256 × divider1..64)==requested is required.
No rounding/PLL retune. Synthetic98.304MHz clock is PUBLICDUMMY data only.
Actual first-sample monotonic epoch + explicit sample index + rational clock
rate produce PTS; IRQ/dequeue time is not the sample epoch. Period/generation
loss rejects data; no nominal/UTC guess or150ms A/V acceptance claim.

Kernel allocator and copy paths are implemented behind broker admission, but
unreachable from the permanently refusing probe. Ring is driver/broker-owned,
not automatically ALSA-freed; code/module backing is pinned until known fences.
The future broker must retain context/card/substream references and dispatch
completion/period notifications under the same owner. No remove/private-free
routine assumes that an EAGAIN async-close marker is physical retirement.
That native lifecycle/provider integration remains a qualification dependency.

## Offline build / retained evidence

`python3 bringup/qc07f/audio-candidate/build.py RUN --toolchain EXISTING_PIN`
writes only a fresh `evidence/delegated/20261012-audio-candidate/RUN/`. Copies
exact source/licensing/REA annotations; runs optimized+ASan/UBSan synthetic PCM
tests, C++ C-API link/run and pinned musl ARM static link (never executes ARM).
Replays the sealed MMC Kbuild compiler argv with only owned source/include/
output/dependency destinations changed, and verifies its read-only header/tool/
config closure before/after. No kernel/header preparation or full tree/image/
module build. Kernel-object/relocatable link != full runtime/active ALSA ABI.
All failed runs, command times, source/compiler/header/library/runtime/license
hashes and actual object/storage costs remain preserved; no audio output files.

One parent next step: the **read-only retail mic/ADC2 route/bias/clock/shared-L2/
cache/IRQ resource receipt** described in SOURCE_AUDIT.md, under fresh actual
ownership/runtime/recovery. Physical/provider uncertainty blocks native operation,
not this source lane. Existing40MiB/CMA16/default-CMA0/videoCRC/2s+10s/2MiBHTTPS/
600k credential limits remain untouched. AUDIO/G4/G7/G8 stay open.
