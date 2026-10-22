# QC07F watchdog owner v2 — focused W1–W3 SOURCE/API and reuse review

2026-10-22. RP-07/09, STREAM-04/07 watchdog-lifetime dependency only.

## Result

**Narrow SOURCE/API sufficiency for W1–W3; changed/reused-object custody sufficient
at the reviewed compile-metadata scope.** No additional correction-required defect
was identified in this delta/context. V1's corrections-required disposition remains
correct and frozen for its original pins. This does not requalify every inherited
path, execute functional checks or adopt this derivative into the frozen RTSP glue.

The result assumes the documented stable/disjoint argument storage and sole,
serialized watchdog-description custody. It is not runtime alias/FD ownership,
finite ioctl latency, electrical stop, DMA retirement, armed recovery, full app,
native/combined-memory or G2/G3/G8 acceptance. All those gates remain open.

Only this fresh review directory was written. Public static reading, diff, hash,
dependency/receipt and archive-byte metadata inspection only. No candidate import,
generator/compiler/build/test/UAPI suite/runtime execution; no target/UART/RPC/
status/network/private IO, original source/helper/Git/index/central-plan/settings/
safeguard/model/provider work or P1 inspection/retry/review/adoption. No policy
rejection occurred. Parent alone owns hardware; neither old results nor this review
imply current Linux availability or clear either radio/reset failure HOLD.

## Exact inputs and integrity

`A/` = `apps/rtsp-auth/qc07f-watchdog-owner-v2/`.
`B/` = `docs/qualification-plans/qc07f-watchdog-owner-v2-20261022/runs/build01/`.
`G/` = **B/source/**. V1 comparison root is
`docs/qualification-plans/qc07f-watchdog-owner-20261022/runs/build04/`.

- User-supplied eight-file source-tree SHA256 matched:
  `e511920a5c4cc53eac99d4d56e1b50e547c726764b790dbc881aa16ef94c7d8b`.
  Method: sorted workspace-relative UTF-8 path + NUL + raw SHA256 bytes.
- SOURCE_SEAL01 SHA256:
  `0517c435cc36a304df100cb74e9dfb18cb16e8572c93617669d5aad47b0ac4a6`.
  Its8 app files,5 report pins and391 evidence pins matched.
- BUILD_SEAL01 SHA256:
  `96f9405ab05a607d69a64c3cbe44ede75b282c55e586eb0259e9a94dec17f3e5`.
  Its exact retained run file set and object/artifact pins matched.
- All1,213 baseline custody pins and1,214 build frozen pins (including the new
  baseline receipt) matched. Known v1 source/review seal hashes remained
  `96bc8a72e628c1f55daa75e49fcbe92622f8686d04faf06f13a2bccb29d87e75` /
  `1edf97af9a4d337e55ec5527d2e4ff7bdccc3989f136512fc8c16b301b54c727`.
- 317 captured public inputs,242 consumed source/header copies,28 generated source/
  header files,18 successful command/log records and four final artifact pins were
  verified. Original acquisition/runtime paths and current external compiler/header
  installations were not opened/rehash-qualified; captured public copies were used.
- Independently diffed retained bytes, without running/importing overlay.py:
  exact `v1-to-v2-app.patch`, `v1-to-v2-generated.patch` and complete original-v6
  `applied-source.patch` all match. Only five generated files changed: owner C/H,
  native wrapper/ownership header and native adapter's single function rename.
  Producer, sensor139/table, three memory-policy C units and source-close-once
  remain byte-identical to v1 build04. Original notices and captured runtime/license
  material are retained; no GPL-2.0-only/GPL-2.0-or-later relicensing is inferred.

| Exact input | SHA256 |
|---|---|
| A/watchdog_owner.c | `f2a6a6c605248f0b9c985302b0515f612a97180ee86c404ed1a475b9a75d5850` |
| A/watchdog_owner.h | `5823a250c81c117c9bc5359563a71551d12cea5f6208f6b5b722c9299c011c01` |
| A/native-watchdog.inc | `fa9d1daa9bbf2185b2e78c7d94da665aeae5822888ce71d0ac0957d5875770a3` |
| A/native_watchdog.h | `568690f2447cfe9f521b777dbe4d19a89b7707933dc83ec42f275ea9403053ef` |
| G/native_adapter.c | `9be7c2d32eb87da654eaf268bf42b6777db26a2e95f6baedec7022f893bc2786` |
| G/rtspd-native-import.c (unchanged) | `a609c60448cd633772a67be9770320dec7a93745e20c258784c77045252f8bd6` |
| B/v1-to-v2-generated.patch | `2206cc5789a3f273e9a9906c61d0c143dd57f33e930ed1510c8b9f5324f4b36f` |
| B/applied-source.patch | `d64ef36ec9503400b536d30f9e38b2fec9243529fb01c8b1977ff65610a4bd2a` |

Full per-source/license/receipt/tool-provenance pins, commands and exact line/range
hashes are in `reader-receipt.json`, `verification.json`, `public-excerpts.json`,
the three `actual-*.patch` files and final `REVIEW_SEAL01.json` here. Static equality
and metadata consistency are not independently executed compile/runtime results.

## W1 — one-shot, real fixed session: corrected

**Actual return shape is preserved.** G/native_adapter.c:71–78 retains the original
body as static native_memory_start_legacy: initializer/gate failures map to
CAMERA_NOT_READY, success to CAMERA_OK. It does not expose NH_MEM_* values.
G/framework/include/camera/backend.h:9–12 confirms OK=0, NOT_READY=-2. New exported
wrapper A/native-watchdog.inc:24–39 treats a nonzero legacy result as failure,
fences and returns NOT_READY; it does not invert success or mistake a camera result
for a raw memory-policy reason. The original public int signature is unchanged.

Ordering at:28–35 is significant:

1. Observed stop/retirement uncertainty/source-clock failure/watchdog HOLD is
   preserved via fence and NOT_READY; no effects repair an uncertain owner.
2. Missing singleton/guard/storage is refused. A healthy repeat with initialized
   ledger **or sealed deadline** returns NOT_READY before any clock read, descriptor
   call, initializer, memory memset/accounting mutation or watchdog control.
3. First setup seals the session before entering the unchanged initializer. Failure
   afterward fences, retaining the seal; there is no setup restart path.

A/watchdog_owner.c:45–55 requires actual moved UNKNOWN state and no earlier seal.
`start >= UINT64_MAX-QW_SESSION_NS` is rejected, excluding overflow and a sentinel
deadline. Real monotonic samples before/after descriptor checks reject unknown/
backward clock, future start and elapsed>=2s. Only then are original start and
exact start+10s stored. These checks perform no watchdog control/feed. The unchanged
initializer's first memset/session assignment (G/memory_runtime.c:24–29) receives
the same accepted start, and the initial memory gate continues to check original
phase/session bounds (:16–22). Setup cost is included rather than restarting time.

A/native-watchdog.inc:47–54 requires initialized memory, a sealed deadline,
ledger/runtime start equal to cached owner origin, and current time inside that
fixed interval. A/watchdog_owner.c:77–93 requires the supplied deadline equal the
seal and phase start not before the original origin; it never overwrites the
session deadline on begin. Existing control pre/postchecks at:57–75 retain strict
<2s phase and session expiry. The static assertion at include:6 binds both10s
constants. Nominal timeout remains5s.

There is no unseal/restart/repeated native singleton-init API. Healthy final
give-back ends custody; it does not reset the native memory owner to start another
session. The deliberately clock-free repeat/BUSY paths do not assert a fresh time
observation; any later effect admission still checks the unchanged deadline.

## W2 — outward zeroing, including one-NULL calls: corrected

A/native-watchdog.inc:57–79 clears **each non-NULL output independently** at:60–61
before pointer validity, uncertainty, BUSY, preparation and guard-begin refusals.
With one NULL pointer, the other valid output is cleared before CAMERA_INVALID;
with both NULL there is no write. The later real-producer/postcheck failure path
still zeroes packet/key before fencing.

This touches only caller-owned writable output metadata, not private expected,
produced/resident/parked storage or FD slots. A/native_watchdog.h:16–20 explicitly
requires stable disjoint outputs/slots, disjoint from private owner/expected data
and still-owned loan bytes. The caller must retain its original held-loan record
separately. This is a trusted API storage precondition, not runtime alias proof.
No release/free/close or fabricated retirement implements zeroing.

## W3 — uncertainty precedence and no-effect known BUSY: corrected

A/native-watchdog.inc:65–67 checks observed stop, retirement uncertainty, failed
source clock and watchdog HOLD **before** produced/parked loan BUSY. An uncertain
owner is fenced, not relabeled healthy BUSY. The healthy known-loan branch precedes
preparation checks, native_guard_begin (:72), legacy lifetime/open/query and all
clock/descriptor/watchdog operations. It returns CAMERA_BUSY without rearming,
servicing/fencing or changing private expected/backing/loan/FD custody; only outward
NEXT outputs were cleared. The real producer's later BUSY guard remains unchanged
but is no longer the first held-loan admission check.

No new ordinary correctness defect was identified in these corrected paths or
their changed surrounding context. This is a static statement, not executed
boundary, cancellation, one-NULL, clock-failure or ioctl-latency confirmation.

## Changed versus reused objects — actual dependencies, not filenames alone

The owner struct gains original session_start_ns (A/watchdog_owner.h:9–16).
Both actual consumers are rebuilt: watchdog_owner.c and native_adapter.c, whose
included producer embeds the owner in qc_rtsp_context. The generated producer's
text is unchanged, but it is **compiled inside the changed native adapter**, not
reused as an old producer object. Its changed layout is therefore not silently
combined with an old embedding object.

Author build01 records18/18 successful commands: two changed units per platform,
archive/REL links and version/target/ELF/symbol inspection. Four unchanged units
per platform come from v1 build04. Their8 actual object files and copied .d/.su
files were independently matched byte-for-byte against the sealed originals.
Every reused .d was parsed as metadata; all dependencies matched both old/new
captured compile-closure pins. Every project dependency also equals the corresponding
v2 generated file. **None includes any of the five changed owner/header/include
files**, so the struct/header delta does not change those units' compiled layout.

| Reused unit | HOST dependency paths | ARM dependency paths | Project contract retained |
|---|---:|---:|---|
| sensor_baseline |107|39| Sensor C/H, rtsp_clock, handoff/camera packet headers, memory_reserve.h, sensor table |
| memory_reserve |29|12| Exact C/H |
| memory_runtime |72|25| Exact C/H, memory_reserve.h, handoff/camera packet headers |
| memory_reader |47|18| Exact C, memory_runtime.h, memory_reserve.h |

`reused-object-custody.json` retains per-object pins, exact dependency lists/project
paths and the original successful per-platform compile command. Reused .d targets
still naming v1 paths are intentional original provenance, not a claim they were
recompiled in v2. External source/header copies match the old sealed closure;
current external installations were not rehashed or executed by this review.

Recorded compiler/ar/nm/readelf identity hashes, retained ARM pins and build
environment match v1, except the intended fresh TMPDIR path. Platform flags remain
HOST O2 and ARM Os/ARMv5TE/soft-float with original warnings/FORTIFY/stack-protector
policy. Changed-unit compile logs are empty. These are verified author records,
not fresh compiler/toolchain validation or a paired kernel ABI result.

As an additional static composition check, the reader parsed normal ar member
headers/bytes without running ar: both final archives contain exactly the six
reviewed objects per platform, and all12 member bodies equal their respective
new/reused .o bytes. Link argv/artifact/log custody also matched. This does not
execute the REL outputs or establish functional linking into a complete camera app.

## Unchanged safety boundaries and remaining dependency

Exact producer/source/header and context diffs preserve:

- One actual preopened O_WRONLY/CLOEXEC character FD moved UNKNOWN; sole serialized
  custody remains a prerequisite, not metadata proof. No watchdog open/dup/V-close,
  final close, unconditional feeder, timeout growth, reboot or recovery API added.
- SETTIMEOUT5/ENABLE/GET5 and sole KEEPALIVE after real raw extent/pointer/status/
  sequence/full CRC/keyed ACTIVE proof. Sensor139 GETTIMEOUT borrows that same FD.
- Actual capture+encode, both lifetime calls and cancellation/receipt/deadline
  postchecks remain before healthy outer disable. Failure zeroes outward metadata
  while retaining actual unreturned loan/backing/remaining FDs.
- Real sensor-off/END/QUERY/RETIRED/ACK and close-once remain required on owned
  normal retirement. Resident/parked CPU release remains after successful logical
  disable. Close-once clears the actual non-watchdog FD slot before one attempt;
  a failed former number is only a diagnostic, never retained/retried as an FD.
- HOLD is sticky. Failed/late disable may already have stopped hardware; retaining
  FD/HOLD is **not armed recovery**. Logical disable is not electrical stop/DMA
  retirement, and userspace pre/postchecks cannot preempt a blocking syscall.

No128KiB exception or40MiB/CMA/native2MiB/HOST4MiB/loopback change was made. Old large
buffers, missing admitted backing/provider-FD adoption and failed-next process
retention remain blockers. Frozen RTSP glue and persistent frame/encoder/query
increments remain separate, not adopted here. Provisioning, owned listener,
ordinary functional/native timing/combined-load, electrical/guard qualification and
full app/G2/G3/G8 remain unmet; no current kernel/image/module or target result is
advanced. P1 remains absent/unmet and outside this review.

**ONE next ordinary dependency:** parent source disposition before selecting an
isolated real provider/backing/failed-next ownership integration design using these
frozen correction pins. No automatic implementation, RTSP rebase, compiler/test/
runtime/full-kernel build or target queue follows from this review.

Reader and finalizer here only perform public source/diff/hash/metadata operations.
`reader-receipt.json` and final `REVIEW_SEAL01.json` preserve exact inputs, UTC,
commands and output pins; finalization revalidates every recorded input unchanged.
