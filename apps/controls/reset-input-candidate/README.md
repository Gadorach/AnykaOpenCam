# Input-only deliberate reset gesture candidate — CTRL-08/10

**Source/HOST candidate for parent review. Native button/reset and CTRL/G4/G6/G8
remain unqualified.** This unit does not touch PTZ V2, framework/policy, kernel
GPIO/input, SETUP, networking, recording, credentials, runner or plans.

## Actual C boundary / authority

`reset_input.h/.c` is a bounded caller-driven debouncer/event engine with one
static queued gesture. Its only callback is a parent-trusted **input-only read**
returning fixed logical pressed/valid/source-class/source-epoch/monotonic-sample
metadata. No public feed-samples endpoint exists. Wire/admin clients cannot
install a provider, call owner poll or claim `pressed=1`. Source class/presence/
valid flags are trusted software declarations, not authentication or proof of
retail hardware. `native_qualified` always remains zero. Absence, stale, unknown,
EOF, unavailable and fault are explicit, never coerced into RELEASE.

The input owner captures/timestamps an observation **before** it calls poll;
the read callback returns that already-captured snapshot (`sampled_ns <= now_ns`).
It must not block on a future device observation or fabricate sample time from
delivery time. Delayed/stale snapshots fail closed. Future parent integration
must audit the original source of that cache, not infer a passive input path
merely because the callback is named `read`.

No GPIO number, polarity/voltage, ioctl, MMIO, clock write, path/command/arbitrary
payload/PID, format/erase/network/reset action or action callback is accepted.
The stock GPIO0 read-like wrapper writes direction/output before reading;
`docs/QC07F_RETAIL_ILLUMINATION_ROUTING.md:265–277` is the retained evidence.
**It is not reused**, nor is GPIO0 routing/polarity assumed. Logical normalization
belongs to a future audited input-only provider. HOST high/low fixtures test that
normalization contract only, not electrical hardware. Unknown mic/ambient/lights
and their privileges are outside this unit.

## Gesture semantics and boundedness

1. Startup/source restart/loss/expired event/consumption requires a **new stable
   RELEASE** before a press is eligible. Startup-held input never starts a reset
   timer. A short press or bounce cannot publish a long-press event.
2. Continuous fresh valid observations must debounce PRESS, then observe the
   full long threshold while pressed. Reaching that threshold **does not emit**.
   A stable RELEASE must follow **before the stuck cap**. This release-complete
   deliberate gesture avoids treating a permanently stuck button as permission.
   Short release bounce cannot restart the original held/stuck timer.
3. Missing samples, invalid/partial observations, EOF/error, source change, future
   or stale samples and contradictory equal-timestamp samples cancel eligibility
   and any pending event. Identical equal-timestamp observations are idempotent,
   never time progress. Clock regression/source-epoch reuse latch fail-closed.
   Higher source epoch requires post-change stable release; no old token survives.
4. One event queue only. While occupied, no new press is admitted. Peek preserves
   the exact event; consume succeeds once on an exact current fresh unexpired
   receipt. Wrong/old/modified/null/known-storage-aliased receipts are inert and
   do not mutate current queue/time. Expiry/lost receipt never automatically
   repeats a gesture. Consumption while a newer press is held cannot arm it.
5. Namespace, engine generation and source epoch plus nonwrapping sequence bind
   receipts. Namespace/generation must be lifecycle-unique from the trusted owner,
   not PID, a wire assertion or unit identity. Initialized storage is address-bound,
   cannot be copied/reinitialized, and disposal remains a tombstone. Counter/time
   exhaustion fails closed without emitting a partial event.

Default HOST proposal: debounce20ms, long5s, stuck15s, maximum fresh gap100ms and
event TTL1s. Header/config admit finite bounded engineering values; the small
fixture configuration is for exhaustive tests, **not retail calibration**.
All timing uses one trusted monotonic domain; no UTC/private time is logged.
Sampling cadence and physical switch response still need qualification. Provider
read must return promptly and never retain output pointers/reenter; portable C
cannot preempt a blocking read. There are no threads/sleep/signals/supervisor,
heap allocations or background activity. Only explicit caller polls advance it.
Pointers must designate complete accessible typed objects; arbitrary invalid-
pointer protection is not a C guarantee. Callback reentry returns BUSY.

## State and event evidence

`ri_view` separates desired policy state (wait-release/armed/debounce/held/event/
failed-closed), observed logical pressed-or-UNKNOWN, observation validity,
sample/receipt monotonic timestamps, source epoch, queue/reason and unqualified
presence. Units are **logical pressed Boolean**, not a GPIO latch or voltage;
no physical-input capability is advertised. An event records stable press/release,
observed held duration, expiry and declaration-only evidence. Neither event nor
ACK/consumption claims a settings transaction or actual physical reset.

SETUP later owns an authorized, independently idempotent **settings-only**
transaction preserving **all footage, firmware, identity, calibration and recovery**.
This engine cannot choose files/partitions/keys, provision default credentials,
erase anything or notify untrusted reset executors. It does not add UI/admin
permissions or accept a wire physical-presence claim. Lost transaction receipts
require SETUP recovery, not re-emission of a consumed gesture.

## Reproducible isolated qualification

`python3 apps/controls/reset-input-candidate/build.py RUN --toolchain EXISTING_PIN`
creates a fresh own-lane evidence directory, copies/hashes source/annotations,
preserves GPL-2.0-only per-file notices, then runs optimized and ASan/UBSan HOST
tests plus C++ header→C API linkage. ARMv5 musl static linking/attributes/sizes
are **inspected only, never executed**. Commands/times/source/header/compiler/
runtime/license closure are recorded; core undefined-symbol whitelist proves no
filesystem/GPIO/reset action dependency. Tests contain PUBLICDUMMY logical
providers only, including an independent oracle for all4096 twelve-sample Boolean
patterns, bounded metadata fuzz, thresholds/startup/stuck/loss/epoch/clock/queue/
receipt/exhaustion/alias/reentry/partial-init cases. No global runner is changed.

**One next parent step:** qualify a genuinely read-only retail button input path
and its routing/shared-owner/polarity plus sampled-clock provenance, under freshly
validated owner/runtime/recovery. Start with source/binding metadata only; do not
invoke stock polling, request/reconfigure a GPIO, press a button under stock reset
actions, or assume historical pin values. Missing physical continuity/level input
correlation remains an explicit user-coordinated dependency. Native debounce/
gesture evidence and a real protected settings-only SETUP transaction are later
qualification, not a result of this source/HOST package.
