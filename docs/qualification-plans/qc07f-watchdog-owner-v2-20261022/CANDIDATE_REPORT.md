# Watchdog owner v2 — W1–W3 correction candidate

2026-10-22. Watchdog-lifetime dependency of RP-07/09, STREAM-04/07;
RP-04/08 and all provider/product/native gates remain open.

**Best-effort correction candidate delivered; HOST/ARM changed-unit compilation,
archive and relocatable link PASS. Ordinary functional checks and all runtime/tests
NOT RUN. Not independent source acceptance, adoption, or native GO.**

## Selected scope and frozen baseline

Only new `apps/rtsp-auth/qc07f-watchdog-owner-v2/` and this documentation
directory were written. The eight v1 app files were copied byte-for-byte into
both the fresh app directory and retained `baseline-v1-app/` before editing.
`BASELINE_CUSTODY01.json` records 1,213 verified prior source/evidence pins,
including all v1 build01–04/seals/reports, the review and 44 earlier frozen pins.
`baseline-provenance/` retains the v1 seal/report and review/seal bytes.

Exact baseline SOURCE_SEAL01 SHA256:
`96bc8a72e628c1f55daa75e49fcbe92622f8686d04faf06f13a2bccb29d87e75`.
Exact correction-required REVIEW_SEAL01 SHA256:
`1edf97af9a4d337e55ec5527d2e4ff7bdccc3989f136512fc8c16b301b54c727`.
The complete review was read; W1–W3 only were selected. V1/review and all old
sources, images, builds and original failure evidence remain untouched. No
review/adoption of P1 or unrelated work was attempted. No policy rejection occurred.

The fresh app remains eight files, 772 lines / 45,087 bytes. Existing SPDX and
captured upstream/runtime license notices are preserved; new correction code
retains GPL-2.0-only. Original driver GPL-2.0-or-later is not relicensed.

## W1 — one-shot setup and nonrenewable owned session

**Actual existing return shape:** the original exported
`int nh_native_memory_start(uint64_t start)` maps all initializer/gate failures
to CAMERA_NOT_READY, success to CAMERA_OK. It does not return NH_MEM_* reasons.
V2 retains the signature and exactly this return shape; no versioned public
adapter API or dummy success was introduced.

The only new overlay transformation renames that real body to static
`native_memory_start_legacy`. New public wrapper at `native-watchdog.inc:24–39`:

1. Preserves/latches already observed stop, retirement uncertainty, failed clock
   or watchdog HOLD; refuses missing singleton/guard/storage prerequisites.
2. Rejects either `memory_state.ledger.initialized` or an already sealed owner
   deadline **before** calling the unchanged initializer. A healthy repeated setup
   returns NOT_READY without a clock read, ledger/session/accounting/observation
   mutation, watchdog control, or renewed budget. Ignored replacement timestamps
   are not treated as new clock observations.
3. Calls `qw_owner_seal_session` before the real setup body. This requires the
   actual moved owner in UNKNOWN, no earlier deadline, a representable original
   start+10s, and real CLOCK_MONOTONIC observations before/after descriptor checks
   satisfying `start <= now` and `now - start < 2s`. Arbitrary future starts,
   unknown/backward clock, overflow/sentinel and expired setup are rejected as
   sticky HOLD before the initializer, never accepted as a fresh elapsed interval.
4. The owner stores the first admitted original `session_start_ns` and exactly
   `start+10s`. Setup cost consumes that interval. If the real initializer or
   initial memory gate fails afterward, the wrapper fences and returns NOT_READY;
   the sealed budget is not cleared and a failed setup cannot restart.

`native_guard_begin` checks initialized memory's original session_start equals
the sealed owner's start, checks current time inside its fixed interval, then
passes that same cached deadline. `qw_owner_begin:77–93` rejects an absent or
different deadline and phase starts before the sealed origin; it no longer writes
the session deadline on each begin. Its existing real pre/post-control clock and
descriptor checks still apply. A static assertion binds QW_SESSION_NS to the
unchanged NH_SESSION_NS = 10s. Phase remains <2s; timeout remains nominal5s.

The helper is additive within this fresh, separately compiled candidate; its
owner struct gains a cached original start. It is not binary drop-in adoption
of old objects that embed the owner. Both consuming changed units are recompiled.
There is no restart/unseal API. Original native init still refuses a repeated
singleton init. Healthy final give-back ends custody; it is not a source restart.

No edits to `memory_reserve.c`, `memory_runtime.c` or `memory_reader.c` were made.
Their unchanged actual initializer still memsets on first accepted invocation;
the wrapper prevents repeat entry rather than weakening that memory policy.

## W2 — zero outward NEXT outputs before every refusal

`native-watchdog.inc:57–81` clears each non-NULL caller output before pointer,
uncertainty, known-BUSY, preparation or guard-begin checks. Thus even a one-NULL
argument refusal clears the other valid output. Existing later zeroing after
real producer/postcheck failures remains.

Only caller output metadata is cleared. No release/free/retire, private `expected`
packet mutation, produced/resident/parked storage change or FD operation implements
this correction. The header documents stable, caller-owned outputs/slots disjoint
from each other, private owner/expected metadata, backing and still-owned loan
bytes. This is an explicit trusted input-storage contract, not runtime alias proof.
Callers must retain a held packet in separate storage rather than reusing its only
ownership record as NEXT's new output object.

## W3 — known-loan refusal before effects

After outward zeroing and valid-pointer checks, NEXT first checks stop, retirement
uncertainty, failed source clock and watchdog HOLD and calls the sticky fence on
those conditions. It then checks `native.produced_storage || parked_after_stop`
and returns CAMERA_BUSY **before** preparation, `native_guard_begin`, any clock
read, watchdog control, descriptor query/open, or legacy codec lifetime query.
The no-effect known-BUSY branch does not fence a healthy owner, service/rearm the
watchdog, change private `expected`/loan/backing, or touch actual FD custody.
Already uncertain/HOLD work cannot be relabeled healthy BUSY by that branch.
No claim of a current clock observation is made on the deliberately clock-free
BUSY path; later effect admission must still meet the fixed session deadline.

## Boundaries retained unchanged

- One actual moved O_WRONLY/CLOEXEC preopened watchdog description, initial UNKNOWN
  (opening already has effects); no second open/dup, V write, watchdog close,
  feeder/guardian, timeout growth or retry/recovery API.
- SETTIMEOUT5/ENABLE/GETTIMEOUT and existing real pre/postchecks; the sole KEEPALIVE
  remains after actual complete raw extent/status/sequence/full CRC and keyed
  ACTIVE receipt validation, not a caller-qualified Boolean.
- Outer ownership covers actual capture + encode, both legacy lifetime queries,
  cancellation/receipt and deadline postchecks. Same earlier outer producer start.
- Real normal source sensor-off/END/QUERY/ACK and provider close-once precede
  logical disable. CPU backing frees remain deferred until successful disable.
- On uncertainty, actual remaining watchdog/provider FDs, process and backing are
  retained. No re-enable/V/close/retry/feed on failure. Failed or late disable may
  already have stopped hardware: HOLD + retained FD is **not armed recovery**.
- Acknowledged disable is logical, not electrical/DMA retirement. Close-once still
  clears non-watchdog slots before one attempt and treats a failed former number
  only as NOT-owned diagnostics; no numeric-FD retry.

The full generated producer `rtspd-native-import.c`, sensor139 implementation/table,
three memory-policy C units and close-once helper are byte-identical to v1 build04.
Only five generated files differ: native adapter (one function rename), owner C/H,
native wrapper include and native ownership contract header. No sensor/legacy
core op, radio,128KiB/40MiB/CMA/strict child2MiB versus HOST4MiB/loopback policy
change was made. Original large v6 buffers remain a product-memory HOLD, not an
exception or a native fit claim.

## Compile-only method and receipts

```sh
python3 -B apps/rtsp-auth/qc07f-watchdog-owner-v2/build.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Fresh v2 build01 ran 13:20:29.904906–13:20:31.491312 UTC. All **18 commands**
returned0. Each platform records compiler version/target, compiles the two changed
real units (`native_adapter`, `watchdog_owner`), deterministic archive, relocatable
link and ELF/undefined/defined-symbol inspection. Compile logs are empty. HOST
GCC16.2.1 targets x86_64-pc-linux-gnu; retained pinned ARM GCC14.3.0 targets
arm-buildroot-linux-musleabi, ARMv5TE/soft-float. Same Wall/Wextra/Werror, stack
protector/FORTIFY and legacy-only compact-source indentation exception remain.

Four unchanged units per platform are reused byte-exact from sealed v1 build04:
sensor_baseline, memory_reserve, memory_runtime and memory_reader. Eight original
object pins, dependency and stack-usage files, original compiler/build/source
provenance are retained. Their current original dependency paths are checked
against v1's compile closure before reuse. This is not recompilation or fresh
functional validation of those units. Final new archives/REL outputs contain two
recompiled + four reused units per platform; unresolved libc/runtime symbols
remain, not a linked camera executable. No unchanged UAPI suites were invoked.

`runs/build01/REPORT.json` records 317 captured input paths, 242 source/header
paths (including retained-object dependencies), 28 generated source/header files,
1,214 frozen custody pins (1,213 prior pins + baseline receipt), all command logs,
compiler/tool hashes and artifact pins. The new builder emits both exact v1->v2
app and generated-source deltas, plus the complete original-v6 applied patch.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| HOST archive | 90816 | `9a0bdbc059bb4776f4483177e93384396c7672e935f99d5898838a7745648c56` |
| HOST REL | 82576 | `50d529550a7ff1ee8bffb3a3c83cde13286caea657c9f4a926ee67fbc813535f` |
| ARM archive | 62110 | `f1dbdd8bdb757eed927a916d3c1dd0776d8955b4f1f043acb3a2d24eceac0d4f` |
| ARM REL | 55028 | `c2ac140e8814d9eea859c09ad116e91e1ff893b4ff0cde722bdfb53ba96c4eaa` |

Exact deltas:

- `v1-to-v2-app.patch`:17,227B,
  `29f090418d651514f7005e3c1a5be1ad80a520e080550e08d1c32af216267ad9`.
- `v1-to-v2-generated.patch`:7,812B,
  `2206cc5789a3f273e9a9906c61d0c143dd57f33e930ed1510c8b9f5324f4b36f`.
- `applied-source.patch`:34,337B,
  `d64ef36ec9503400b536d30f9e38b2fec9243529fb01c8b1977ff65610a4bd2a`.
- Generated adapter:10,481B,
  `9be7c2d32eb87da654eaf268bf42b6777db26a2e95f6baedec7022f893bc2786`.
- Unchanged generated producer:16,399B,
  `a609c60448cd633772a67be9770320dec7a93745e20c258784c77045252f8bd6`.

`seal.py` performs read-only byte/diff/receipt verification and exclusively creates
`BUILD_SEAL01.json` and `SOURCE_SEAL01.json` here. It does not import a candidate,
run a generator/compiler, or execute functional tests/artifacts. Seals preserve
source/input/prior custody, exact deltas, logs, baseline/license copies and build
outputs; they do not constitute independent source sufficiency or runtime proof.

## Open gates / handback

No ordinary functional, synthetic, runtime, adversarial, penetration, fuzz or
mutation test was run. No target/UART/RPC/status/network/private IO, executable,
listener, driver/kernel/module/image/full app build or target observation. No
Git/index/central-plan/P1/model/provider/safeguard change or policy reroute.
Parent alone hardware; last reported target remains held original U-Boot with
radio and failed-reset HOLDs preserved, not a new Linux or recovery observation.

Frozen RTSP glue and persistent FD/query/compiler candidates remain separate and
unadopted here. Mandatory provider-FD adoption, backing/memory admission,
failed-next process retention, provisioning, owned listener, native guard timing/
electrical qualification and G2/G3/G8 remain blocked. Userspace pre/postchecks
cannot preempt a blocking ioctl or prove finite driver/scheduling latency.

**One next parent dependency:** focused ordinary source disposition of these exact
W1–W3 correction pins. No test, integration, full-build or native queue is selected.
