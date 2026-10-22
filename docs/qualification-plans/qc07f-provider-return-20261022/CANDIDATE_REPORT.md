# Pre-memory/nonactivated custody return — source candidate

2026-10-22; RP-07/09, STREAM-04/07 ownership dependency only.

**Delivered: real strict pre-memory COMMITTED return of sensor/frame/encoder and
the actual UNKNOWN watchdog description last. HOST/ARM18-command compile/archive/
REL PASS; not run. Not parent source acceptance, complete provider lifecycle,
native/full-app/G2/G3/G8 GO or a watchdog timing/recovery guarantee.**

## Baseline, scope and method

Only fresh `apps/rtsp-auth/qc07f-provider-unactivated-return/` and this documentation
directory were written. `RETURN_CONTRACT.md` was written before any candidate code;
its unchanged pre-code SHA256 is
`0280cc4ba3c35c666b9b2be96e0325876fd6cddbdefc92db94345e5478acf071`.
It specifies history/admission, trusted context, move/partial-failure semantics,
UNKNOWN watchdog return, terminal state and explicitly excluded abort boundaries.

Exact starting point is reviewed owned-provider **final build02** generated source,
not a new producer or regeneration/import of an old overlay. All38 generated files
are byte-copied to `baseline-source/`,18 app inputs to `baseline-app/`, and original
source/build/review seals/reports/contracts to `baseline-provenance/`. Original
SPDX/runtime licenses are preserved; new code retains GPL-2.0-only, no relicensing.

- Original source aggregate:
  `fb9cc61500beaad9c6a2cb518ebf06ce73cd1d7a3d9595e54b8153d8db58dedb`.
- Original SOURCE_SEAL01 SHA256:
  `64c66214b709df00383902d2045c557d7a81b1c71ba23abf30d341b3aa068300`.
- Accepted ownership review seal SHA256:
  `dfcbf0e46f3ceb0bc41a1878cebeeb2b51d1a404555387dad6471d8b9e6c0080`.

`BASELINE_CUSTODY01.json` records4,578 prior immutable source/evidence pins. All old
glue/provider/query/driver/compiler/watchdog/reviews/builds/seals/images remain
untouched. The separate query77 build lane was not inspected or changed. The new
20-file app tree is1,444 lines /77,464 bytes including copied dependencies/builder.
Actual new return implementation is the90-line include,29-line contract header and
18 additive watchdog C lines; the generated native adapter changes only terminal
entry guards and its new include. No original producer/device-operation body is
replaced. No main, allocator, retention owner, normal-profile or recovery API.

## Eligible source history, not a caller readiness flag

`provider-return.inc:5–44` checks real private native state. Observed stop,
retirement uncertainty, source-clock failure, watchdog/provider HOLD or staged
helper HOLD is refused/fenced first. Those facts are never cleared or classified
as a healthy ineligible request. Missing/ineligible healthy history returns
CAMERA_NOT_READY without descriptor/query/control activity; bad destinations
return CAMERA_INVALID before observations or moves.

| Required fact | Source predicate |
| --- | --- |
| Fully adopted but not activated | Actual qpCOMMITTED/provider_shape, all active role FDs>=3, no staged sensor/frame/encoder FD, helper states EMPTY. No partial failed take, RETURNING/RETURNED, ACTIVE/CLOSED/HOLD admission. |
| No capture/local job history | No armed receipt, last source/file/capture generation, sequence, renewals, ownership IDR history, stream count/global frame_count, returned codec frame number, raw-valid/NAL CRC, producer begin/timing/size/keyframe. Private receipt key/evidence are all-zero; expected packet has no lease/generation/timestamp/codec/key/data/size. |
| No backing/close/temporary role | No produced/resident/parked CPU backing, encoder_hold/legacy_control remain=-1, close_unknown0 and last_close_attempt=-1. |
| No memory setup | Ledger uninitialized and every capacity/paid/debt/range/raw-reuse field0; runtime has no observations/session/callbacks/context. No initializer, ledger reset, allocation or backing touch implements return. |
| No watchdog session/control | Actual owner UNKNOWN/fd>=3, session start/deadline0, phase begin/phase/progress0. Metadata clock sampling may update last_ns; it is not a session/control. |
| Caller destinations | Four non-NULL distinct stable/disjoint writable int objects, each=-1, disjoint from native/private/other storage. Explicit pointer distinction is not a general alias proof; trusted object-storage contract remains. |

Global encoder live_bytes, telemetry/history and dispatch counters may describe
earlier provider history. They are retained and are NOT recast as zero prior DMA,
new operation identity or permission. The absence above is actual **local source**
history. Exact boot/image/provider association, unknown earlier DMA/history
admission, stable sole nonaliased open descriptions and no competing provider/FD
changes remain independent trusted bootstrap prerequisites; BIND NONE cannot prove
them. All calls/moves are serialized, no signal reentry, recipient I/O or provider
dispatch until the return call completes, including after earlier destinations
have already received descriptions.

## Observations and actual nonatomic ownership

`nh_native_return_pre_memory_unknown_bootstrap` is included after the real native
provider take/wrappers and operates on their actual singleton, not local helper
owners or arbitrary passed native pointers. It records one real monotonic begin,
checks UNKNOWN watchdog metadata/time, invokes the existing actual `lifetime()`
(all role nodes, frame56/NONE/expected source+file, encoder88/NONE/expected session,
original global56 quarantine/DMA/guard/reset/fault facts), then checks the same
interval again. Unsupported old76 query88 still fails without a fallback.

Only after preflight does it mark RETURNING. Sensor metadata is refreshed, then
frame metadata/BIND/fresh NONE, then the existing encoder-only schema/reference/
fault observer. The all-role observer is correctly not called after an earlier FD
has moved. Original global facts were observed before all moves; the documented
serialization/no competing dispatch/no recipient I/O prevents introducing an
encoder job between observations. Real expected identities and latest fully valid
known snapshots are retained; valid contradictory observations are stored before
refusal, failed/malformed/identity-mismatched replies do not replace them.

| Last successful move | Actual owner if a later observation/refusal fails |
| --- | --- |
| None | Native owns all four original actual FDs; destinations remain=-1. |
| Sensor | Sensor destination owns actual FD; native sensor=-1. Native retains frame/encoder/watchdog. |
| Frame | Sensor+frame destinations own actual FDs; their native slots=-1. Native retains encoder/watchdog. |
| Encoder | Three provider destinations own actual FDs; native provider slots=-1. Native retains watchdog, including on last metadata/deadline failure. |
| UNKNOWN watchdog | All four destinations own their actual descriptions; all native FD slots=-1 and source phase RETURNED. A final observed cancellation can still return error/fence, without pretending any returned FD stayed native-owned. |

Moves are direct sequential assignments to stable caller destinations followed by
native-slot invalidation, under the serialization contract. The temporary duplicate
numeric record during those assignments is not concurrent shared ownership or an
atomic transaction. No kernel dup/open, returning stack owner, unverified rollback,
cleanup/close/destructor or retry is used. Nonzero return NEVER means all FDs stayed
native or all returned: populated slots own actual earlier transfers, remaining
native slots retain actual remaining custody on HOLD. Bootstrap must preserve its
actual destination records/descriptions and the real process for any remaining
native owner. Static globals are not an external process-retention solution.

## UNKNOWN watchdog handoff is not disable or armed proof

Additive `qw_owner_unknown_bootstrap_observe` and
`qw_owner_return_unknown_bootstrap` are separate from the existing DISABLED healthy
handback. They require actual UNKNOWN, no seal/begin/phase/progress, same saved
device/node filesystem/inode, O_WRONLY/CLOEXEC character FD, and real CLOCK_MONOTONIC
samples before/after descriptor inspection. Every check uses the same caller's
original whole-return begin and rejects unknown/backward/future/elapsed>=2s time.
No fresh timeout/session budget is created. Native uncertainty/stop checks bracket
boundaries and follow the last move; no failure-clearing repair or retry.

The primitive performs clock/GET metadata only, **no watchdog ioctl**, I2C selector,
ARM/job/END/ACK/reset, disable/enable/keepalive, V/write or close. UNKNOWN remains
UNKNOWN until actual move; afterward the source record is fd=-1 and terminal
QW_RETURNED_UNKNOWN with identity metadata retained. The recipient FD is explicitly
UNKNOWN, possibly running at an unknown current timeout. Preopen already started
hardware: UNKNOWN is neither unstarted, unarmed nor logically DISABLED.

**Bootstrap must independently admit this control-free handoff and retain/service/
recover the returned watchdog under its own policy.** Return is not close/disarm
permission, electrical/DMA retirement or armed recovery. Pre/post checks cannot
preempt blocking clock/metadata/query syscalls or guarantee the current unknown
timeout will not expire. The <2s checked interval is not proven finite native
latency/timer headroom; there is no native handoff timing/guard qualification here.

## Terminal success and preserved normal path

After all four moves, source phase is RETURNED, distinct from normal CLOSED and
not a proved END. No initialized flag, opt-in/storage/accounting/history/snapshot
is reset to permit reuse. New terminal guards reject storage/opt-in/config and
watchdog take/give-back/release entries. Existing provider take requires EMPTY,
while memory/effect/NEXT admission requires COMMITTED/ACTIVE actual roles, so
RETURNED/RETURNING cannot restart those paths or renew a session. Reinit remains
forbidden and fences; provider_hold preserves RETURNED history on later misuse,
while still retaining/latching actual uncertainty. Diagnostics are not restart.

The actual producer is byte-identical to reviewed owned-provider build02, as are
provider-take, frame/encoder helpers/query88/telemetry UAPI, source-close-once,
sensor139 C/H/table and three memory-policy C units/policy headers. Existing ACTIVE
BIND/ARM/exact139/BEGIN/sensor-on/per-open+global checks remain. Original watchdog
C is an exact prefix of the new file: normal one-shot10s/<2s/nominal5s control,
validated sole progress, disable and healthy DISABLED handback bodies are unchanged.
Both actual consumers of the changed header/terminal enums are rebuilt, not reused.
NEXT keeps zero outward outputs and uncertainty-first/no-effect known-loan BUSY.
Real normal END/QUERY/RETIRED/ACK/close-once and disable-before-CPU-free stay intact.
Failed disable still may leave hardware stopped/unknown and is not armed recovery.

The new API does NOT cover post-memory nonactivated abort, partial failed adoption,
activated FD handback, uncertainty cleanup or recovery. Normal retire still refuses
COMMITTED with no fake END/ACK. Those excluded boundaries still need external
retention/recovery or separately reviewed lifecycle work.

## Compile-only evidence and frozen deltas

```sh
python3 -B apps/rtsp-auth/qc07f-provider-unactivated-return/build.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Build01 ran14:29:10.003550–14:29:14.144809 UTC;18 recorded commands returned0.
HOST GCC16.2.1/x86_64-pc-linux-gnu; retained pinned ARM GCC14.3.0/ARMv5TE/soft-float.
Two units/platform compile: actual native_adapter including all changed embedding/
wrapper/return code, and watchdog_owner. Six exact unchanged units/platform reuse
12 original object/.d/.su files: sensor, three memory-policy units, frame and
encoder custody. Actual source/header/layout/tool closure is checked against the
reviewed original compilation, including inherited provenance paths; no reused
object consumes any changed header/include. Old native/watchdog consumers are not
silently reused. Original warning/FORTIFY/stack-protection flags remain; only the
legacy compact native unit has the indentation exception. Compile logs are empty.
Archive, relocatable link and version/target/ELF/symbol inspections only. No
executable, tests or UAPI suite; inherited libc/runtime dependencies remain.

`runs/build01/REPORT.json` captures368 input paths,259 consumed source/header paths,
40 generated source/header files,4,579 frozen pins (4,578 prior + baseline receipt),
all18 command logs, tool versions/hashes, environment, reused objects and artifacts.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| HOST archive |119394| `5cbc05c22c9d311001ce5a025a5e6a4934bc8a72582355e4fcd83c79621441ec` |
| HOST REL |107904| `8f2cdb4f66caa7d34ed59513e087cfc3b1b6444e2ddb4cd1a7308310c97451fd` |
| ARM archive |78878| `9e57e778035f397d3a01623b7206fdfe373170ca54bb80a6d6994633c78a7ccf` |
| ARM REL |69140| `499c790163df920b38d9b87b8789f006c1b20e9e33ca04076b2009cf42c9602d` |

- App delta:38,130B,
  `9ed18b696b03010fc037163cba05e17d4b983af2c42961057c5cf1e731e1443b`.
- Generated delta:15,114B,
  `89f2487225b137e9d5b2126ef0181dd1e42cdd7a2176f99c1fa9d0b38700a1fd`.
- Applied baseline-source patch:14,836B,
  `6a3c151dc859a9eee415ecd03de09bc03c94e7c57720b87d9812efe3633e705e`.
- Actual native adapter:11,586B,
  `0908226cef49ccc31837f4dd3657d0ed2bc5d8621aa4f0da1f397eae60a8049e`.
- Unchanged actual producer:16,881B,
  `00ee594e91442b3fb256834f0069d653d71d379e781c4bce7a4390e68f3339be`.

`seal.py` only reads bytes/diffs/receipts/dependencies and exclusively writes new
BUILD_SEAL01/SOURCE_SEAL01 here; no candidate/compiler/generator/test is executed by
that finalizer. No initial compile failure, policy rejection or reroute occurred.
Old failures, builds and reviews remain unchanged; no old result is advanced.

## Remaining constraints / handback

Ordinary functional checks and all runtime/adversarial/penetration/fuzz/mutation
tests NOT RUN. No target/UART/RPC/status/network/private IO or fresh hardware
observation; parent alone hardware, last reported held original U-Boot/radio+reset
HOLDs remain unchanged. No Git/index/central-plan/P1, model/provider/safeguard
configuration changes. No query77-lane work, kernel/module/full image/app build,
listener/main, RTSP rebase or automatic next operation.

128KiB/40MiB/CMA/native2MiB/HOST4MiB policies are unchanged; old large allocations/
external hub backing remain memory HOLD with no cap/headroom waiver. External
failed-next/partial-handoff process retention, post-memory/nonactivated/failed-state
handback, backing, provisioning/listener and native query/watchdog/timing/combined
load remain blocked. No full provider/G2/G3/G8 or native acceptance.

**One next parent dependency:** focused SOURCE/ownership disposition of this exact
strict return/history/partial-result/UNKNOWN-terminal piece. No automatic review
worker, test, full build, integration or native queue is selected by this handback.
