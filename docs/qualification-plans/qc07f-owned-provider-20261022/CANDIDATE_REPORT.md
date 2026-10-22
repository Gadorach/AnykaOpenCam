# Actual mandatory provider FD adoption — source candidate handback

2026-10-22; RP-07/09, STREAM-04/07 dependency. RP-04/08 and product gates open.

**Delivered: real staged sensor/frame/persistent-encoder FD adoption into the
reviewed guarded native producer. Final HOST/ARM20-command compile/archive/REL
PASS; nothing executed. Not independent source acceptance, full provider or native
GO. Backing, external failed-next retention and healthy never-activated role
handback remain blockers. Frozen RTSP glue was not rebased.**

## Scope, contract and exact starting point

Only fresh `apps/rtsp-auth/qc07f-owned-provider-candidate/` and this documentation
directory were written. `OWNERSHIP_CONTRACT.md` was written before implementation,
then clarified with the final pre-capture handback limitation. It specifies actual
transfer/partial failure, serialized context, stable/disjoint arguments, setup
ordering, identity/state, permitted effects and uncertainty boundaries.

The baseline is the actual reviewed watchdog-v2 build01 generated native source,
not an execution/regeneration of old overlays or a new/mock producer. Its full28
source/header files are copied byte-for-byte in `baseline-source/`; eight app
inputs in `baseline-app/`. Frame/persistent encoder custody C/H and actual query88/
telemetry UAPI headers are byte-imported from reviewed provider-v2/query sources,
with originals also copied in `baseline-helper/`. The six imports remain exact.
All licenses/notices and captured runtime license files are preserved. No source
is relicensed by this candidate; new code retains GPL-2.0-only notices.

Accepted watchdog-v2 source-tree SHA256:
`e511920a5c4cc53eac99d4d56e1b50e547c726764b790dbc881aa16ef94c7d8b`.
Watchdog-v2 review seal SHA256:
`aa73289cac397d740326527fd2be8b0b495f8056c377d477355ffa7f726a2bc8`.
`BASELINE_CUSTODY01.json` records3,649 prior source/evidence pins, including old
glue/provider/query/compiler/watchdog/review/build evidence. Original sources,
images and failures remain untouched. No original reader/generator/test was rerun.
The fresh app has18 files,1,388 lines /77,036 bytes, including imported files and
the compile-only builder. A separate same-process source API is used, not a binary
plugin or arbitrary caller pointer/Boolean qualification grant.

## Required bootstrap and setup order

Bootstrap independently establishes the exact admitted boot/image/source/provider
association, expected device/session/source/file tuples, sole stable open
descriptions and permitted effects/backing/recovery. No aliases, concurrent FD
changes, I/O, reentry, fork or competing provider dispatch. User storage must be
stable/disjoint, including outputs versus existing held-loan records. Metadata,
access mode, CLOEXEC and observational queries cannot prove those preconditions,
permission, global health or DMA retirement.

Order is **fresh native init -> watchdog UNKNOWN take -> original storage select
and exact139 opt-in -> provider transfer -> one-shot memory setup/fixed buffers ->
NEXT/release/normal retirement -> watchdog give-back**. Original storage/opt-in
guards still reject preloaded active FDs and are not disabled to accommodate a
bad order. Mandatory actual role presence is added to memory/effect admission.
Memory start retains its CAMERA_OK/CAMERA_NOT_READY return shape and repeat refusal
before observations/initializer; the original fixed10s session seal stays one-shot.

Selected mode is only original1280x720 H.264 IP stream (`h264_p_frames==1`). IDR-only
temporary role remains unimplemented and is refused before transfer/effects. No
new full profile selector or encoder role alias was invented. The original stream
branch sent its job exclusively to the persistent FD, leaving its separate
temporary open unused; that open and its per-frame close are removed only from
this selected derivative. This is deliberately different from retaining the old
temporary open/session/close side effects, not a claim of identical session counts.

## Actual nonatomic transfer / one owner per description

`provider-take.inc` implements the exported `nh_native_provider_take` on the real
native singleton. It checks valid distinct numeric slots, all required expected
identities, empty original/staging slots, setup/mode and clear uncertainty before
moving anything. Sensor adoption observes only GET-mode/CLOEXEC/fstat/expected char
device; no I2C selector/register operation is performed in this API.

| Boundary | Actual ownership after failure |
| --- | --- |
| Initial argument/sensor metadata refusal | All bootstrap slots unchanged; no role moved. |
| Initial encoder helper observation refusal | All bootstrap slots unchanged; watchdog/native HOLD latched, no claimed uncommitted encoder snapshot or FD. |
| Encoder take succeeds, frame take fails | Encoder is actually in native stable encoder-staging record; bootstrap encoder slot=-1. Frame/sensor stay bootstrap-owned. HOLD retains moved encoder and its valid snapshot. |
| Both helper takes succeed | Encoder/frame are in native stable staging records. Their bootstrap slots=-1. Expected identities/node metadata/snapshots are retained in native provider metadata, not additional FD owners. |
| Sensor move, frame observed give-back fails | Sensor is in the original native sensor slot; frame/encoder actual FDs remain in staging. All moved bootstrap slots=-1. No rollback/close. |
| Frame give-back succeeds, encoder give-back fails | Sensor/frame are in original native slots; encoder remains in HOLD staging. The frame helper is EMPTY/fd=-1, not a second owner. |
| Both give-backs succeed, final observation fails | All three actual FDs are in original native slots; staging records are EMPTY/fd=-1. HOLD retains native custody and last valid expected snapshots. |

Helper give-back is an observed custody move, not close/drain or permission. There
is no returning local FD owner, unverified rollback, copied ownership flag or
destructor. Snapshot copies do not claim description ownership. Partial failure
does not mean bootstrap owns all: remaining nonnegative bootstrap slots still
belong to bootstrap; moved slots remain=-1 and their actual native/staged owners
and this real process must survive. The initial helper's uncommitted failed reply
is not exported by the unchanged helper and is not presented as a native snapshot.
There is no automatic take retry/cleanup. On full transfer, the actual native
slots are reobserved using per-open AND original global lifetime facts.

`qp_phase` records actual transfer/source history (EMPTY/MOVING/COMMITTED/ACTIVE/
CLOSED/HOLD), not authorization or health. Every effect boundary also checks real
slots, absence of staged owners, metadata and actual query/receipt identity.
The old `last_source_instance`/`last_file_instance` counters are not used as ready
flags. Expected identities and latest fully valid known snapshots survive HOLD.
Failed/malformed/identity-mismatched replies never overwrite valid snapshots.

## Real producer/lifetime integration

- `lifetime()` no longer opens an encoder on missing FD. It calls actual adopted
  role observation, requires version1/88-byte per-open session/reference query and
  retains the original version1/56-byte global quarantine/DMA/guard/reset/fault/
  live-byte facts. Global session_sequence is only checked for consistency with
  the expected already-known session, never substituted as per-open identity.
- Actual required88-byte UAPI is byte-imported. Old frozen76 returns ENOTTY and is
  unsupported; no guessed size/ABI, alternate query or read-only permission path.
  Query waits behind real synchronous jobs, so fixed-size copying is not latency
  or global-idle proof. Saturated telemetry is retained, not treated as uniqueness.
- The added role observer validates all active descriptor nodes/modes/CLOEXEC,
  exact frame source/file tuple and encoder session. COMMITTED requires fresh
  frame NONE/gen0/clean; ACTIVE requires the actual keyed ACTIVE receipt. Query
  snapshots with valid schema/session are retained before fault/OTHER refusal.
- `native_guard_begin` checks mandatory roles before clock/control; then records
  the original outer phase start and calls real lifetime observation BEFORE
  watchdog controls. Observation cost is included in that same <2s interval.
  Original legacy NEXT repeats lifetime inside the real guard, both before and
  after the producer; cancel/receipt/deadline postchecks remain before disable.
- First capture uses explicit COMMITTED state, not `continuous_frame<0`. Thus
  preloaded FD cannot bypass actual BIND/fresh NONE/exact expected tuple, then
  guarded I2C_SLAVE0x30/register validation, ARM/reservation/exact139/BEGIN/sensor-on.
  Every capture/query/job uses the same adopted native slots. No device-open
  fallback remains in generated native adapter or producer.
- The real persistent STREAM ioctl receives the adopted encoder. Returned codec
  frame number is retained; subsequent THIS_OPEN query must match that frame and
  the actual existing stream history. NONE is required before the first job;
  OTHER_OPEN/fault/unknown schema refuses without recovery or fallback.
- Memory start and fixed-buffer entry reobserve roles/global facts before setup
  or backing touches. Original memory initializer/forecast/prefault implementations
  remain byte-identical. This is not an ownership proof for their external pointer.

## Retirement, retention and explicit unsupported boundary

Real normal retirement remains guarded. Original global/per-open lifetime facts
are checked before the body, and original sensor-off -> END -> keyed RETIRED/END
QUERY -> ACK -> sequential source/provider close-once is preserved. Before the
persistent encoder close, its per-open reference/fault facts are reobserved.
Actual inspected driver release (`qc07f_hwenc.c:431–445` in the frozen query
candidate) clears only this file's reference under enc.lock before freeing its
private session. No app reset/recovery or DMA/electrical-stop proof is invented.

The close-once helper is byte-identical: clear actual slot BEFORE its one Linux
close attempt; an error's former number is diagnostic NOT owned and never retried.
Sequential close stops on uncertainty; actual remaining FDs/backing stay owned.
After real healthy normal source/close completion phase is CLOSED; outer logical
disable must succeed before resident/parked CPU backing is freed. Failed/late
disable may already have stopped hardware, so retained watchdog/HOLD is not an
armed recovery guarantee. Expected query/receipt/global snapshots remain available.
Quarantine also reports actual staged FDs and expected identities in addition to
the old actual active-FD/backing diagnostics; it is not an external process owner.

**Explicit limitation:** adopted but never-activated roles do not have a healthy
handback/close API. Retire wrappers refuse known COMMITTED before rearm/query,
retaining all actual custody; no fake ACTIVE/END/ACK evidence or cleanup close.
Caller abort before capture therefore still needs a real external retention/
recovery owner. This candidate does not silently claim the whole provider lifecycle
is complete. No failed-state handback/rollback API is added.

## Watchdog and immutable policies retained

Watchdog owner C/H are byte-identical to accepted v2. One actual moved description,
UNKNOWN bootstrap, exact original one-shot10s deadline, source<2s and nominal5s;
no second open, V/close, periodic feeder/guardian, retry or timeout growth. NEXT
still zeroes valid outward outputs on every refusal; sticky uncertainty precedes
known held/parked-loan BUSY with no clock/control/query/custody change on BUSY.
Validated full raw/status/sequence/CRC/keyed ACTIVE receipt remains the sole
KEEPALIVE progress callsite. Capture+encode+postchecks and normal retirement remain
inside ownership, logical disable before CPU free. Metadata is not armed proof,
electrical stop or DMA drain; blocking ioctls cannot be preempted by pre/postchecks.

Sensor139 implementation/table, three memory-policy C units, source-close-once,
128KiB/40MiB/CMA and strict native2MiB/HOST4MiB policies are unchanged. Old large
raw/resident/1MiB buffers remain product-memory HOLD: no allocation cap exception,
new backing/headroom grant, product-memory or native fit claim.

## HOST/ARM compile-only evidence

```sh
python3 -B apps/rtsp-auth/qc07f-owned-provider-candidate/build.py build02 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Final build02 ran13:51:50.532468–13:51:52.640488 UTC;20 commands returned0.
HOST GCC16.2.1/x86_64-pc-linux-gnu and retained pinned ARM GCC14.3.0/ARMv5TE/soft-float.
Three actual units per platform compile: native_adapter (including all changed
embedding/producer/wrapper source), frame custody and encoder custody. Five exact
unchanged units/platform are reused: sensor_baseline, three memory-policy units
and watchdog_owner. Ten object/.d/.su originals and their source/header/tool closure
are checked and copied; no old native embedding/layout object is reused. Fresh
helper compilation does not execute UAPI tests. Compiler logs are empty; original
warnings/FORTIFY/stack protection remain, with the legacy compact-unit indentation
exception restricted to native_adapter. Commands also archive, relocatable-link,
and inspect version/target/ELF/symbols; no executable is linked or run.

`runs/build02/REPORT.json` captures344 input paths,253 consumed source/header paths
(including reused dependencies),38 generated source/header files,3,650 frozen pins
(3,649 prior + baseline receipt), tool versions/hashes/environment and artifacts.
Source/read-only metadata verification is retained by `seal.py`; no functional
check/test/candidate execution is performed by that finalizer.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| HOST archive |111170| `3196770557fd9e2d4e5d9e3128cebe764eab2ed3d34e4107962659a3ba8ff7bd` |
| HOST REL |99928| `b379cfe3faa240928c277d4e4242fdeb21bafda7f37ec1b8e0025111877374d3` |
| ARM archive |74254| `dbd4cd9e4109dc5718a1cfa7976c009e5860693245e8b0e754c7f73346c3aa5d` |
| ARM REL |64748| `7c3d4ba094861ba895df239858790991de100eb6648505145e8edf942d559da5` |

- Exact app delta:74,942B,
  `fcd2fc4b3b9cf4cc7601206f0792d407d476c79d7f9b26040ef9e5db7b9fedbc`.
- Exact generated delta:53,379B,
  `0e9ed76c0c5c698bc6902a9128798fedc6019ea807dd8a2de117feadc023a3f1`.
- Applied source patch from reviewed watchdog-v2 baseline:52,738B,
  `2ee3fe892a58ee68f886748a078200289dbb98b8eb14f67dc96a370e56023b5a`.
- Final actual native adapter:11,386B,
  `742c7f251b0c3368fdc6c824bd75006b4f5ff0de355b224fa04c800d92b487dd`.
- Final actual producer:16,881B,
  `00ee594e91442b3fb256834f0069d653d71d379e781c4bce7a4390e68f3339be`.

Initial build01 also passed20 commands and is preserved as superseded source,
not substituted for final pins. Build02 additionally checks global facts after
role transfer and before memory setup/backing touches, and refuses never-activated
retirement before rearm. No failed compile or policy rejection occurred. One patch
context mismatch and earlier read-offset/path misses were tool lookup/edit errors,
not policy rejection or a reroute. No candidate source test was run.

## Remaining blockers / parent handback

The original memory_fixed_buffers(void *hub) accepts an external pointer, but does
not own its process/backing lifetime. Its ledger/prefault records are not FD-backed
custody or a128KiB-compatible allocation design. Failed NEXT can retain unreturned
native backing/FDs in globals only while this actual process exists; the common
RTSP external failed-next retention/process owner is still absent. Never-activated
healthy handback, provisioning, listener, native guard/driver/query pairing,
ordinary functional/native timing and combined-load/memory remain open. Frozen
RTSP glue and all old FD/query/compiler candidates remain separate and unchanged.

No runtime, ordinary functional, synthetic, adversarial/penetration/fuzz/mutation
test, listener/app main, target/UART/RPC/status/network/private IO, kernel/module/
image/full app/native build or hardware observation. No Git/index/central-plan/P1
change or model/provider/safeguard configuration change. Parent alone hardware; last reported held original
U-Boot/radio+reset HOLD state is unchanged, not freshly observed here.

**One next dependency:** parent focused SOURCE/ownership disposition of this exact
integration contract, partial-transfer and real callsite delta. No adoption,
RTSP rebase, full provider/app/G2/G3/G8 acceptance, test or native queue is selected.
