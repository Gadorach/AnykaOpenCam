# QC07F app watchdog owner — focused ordinary SOURCE/API review

2026-10-22. Watchdog-lifetime dependency of RP-07/09, STREAM-04/07. Static review
of the exact final build04 derivative, not a security or runtime test campaign.

## Disposition

**CORRECTIONS REQUIRED; no narrow source-sufficiency acceptance for these pins.**
Three concrete ordinary API/lifecycle gaps remain: repeatable memory-session
initialization renews the supposedly fixed budget; early NEXT refusals leave
outward outputs unchanged; and NEXT rearms the watchdog before rejecting a known
held CPU loan. Minimum fresh revision is described below. No hypothetical runtime
failure is presented as an observed camera result.

The single-description ownership, validated progress callsite, outer postcheck
interval, late-disable HOLD treatment and close-once correction are real source
improvements. They do not close these gaps or establish full app/G2/G3/G8/native
or combined-memory acceptance. No adoption, integration, build or test is queued.

Only this new review directory was written. No old source/helper/AGENTS/Git/index/
central-plan/settings/model/provider/safeguard changes; no P1 inspection/retry/
review/adoption. No candidate/builder/generator/compiler/output/runtime/test
execution, target/UART/RPC/status/network/private input or hardware observation.
No adversarial/penetration/fuzz/mutation work or policy rejection occurred. Parent's
reported held original U-Boot and both radio/reset failure HOLDs remain unchanged;
no current Linux availability is inferred.

## Actual source, build path and custody

`A/` = `apps/rtsp-auth/qc07f-watchdog-owner-candidate/`.
`B/` = `docs/qualification-plans/qc07f-watchdog-owner-20261022/runs/build04/`.
`G/` = **B/source/**, the actual generated-source directory, not `generated/`.
Complete actual patch: **B/applied-source.patch**.
`K/` = `work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts/`.

- Supplied eight-file source-tree pin verified:
  `2f9e50840a58bbcf8025df1b4e6850278eee0ff5d9a48e0354c531f218b7872a`.
  Hash format: sorted workspace-relative UTF-8 path + NUL + raw SHA256 bytes.
- SOURCE_SEAL01's **8 source files,5 report pins and1,146 retained evidence files**
  matched; build01–04 and candidate inputs were not changed.
  Exact seal-file SHA256:
  `96bc8a72e628c1f55daa75e49fcbe92622f8686d04faf06f13a2bccb29d87e75`.
- Build04's **28 generated source/header files**, **276 captured public input
  paths**, **234 consumed source/header copies**, **44 prior frozen pins**,26
  command logs and four artifact pins matched. All eight live candidate files
  equal their captured build04 copies.
- Independently reconstructed the diff between retained original and final source,
  plus the five new included source/header files; it exactly matches the sealed
  applied-source.patch. This is a diff of existing bytes, **not execution/import
  of overlay.py or regeneration**. Only native_adapter.c/rtspd-native-import.c
  differ among retained original units. Sensor139/table and three memory-policy C
  units remain byte-identical. The findings are in admission/wrapping around them,
  not adoption of a rewritten memory policy or sensor table.
- Run34 source provenance was checked through its captured public manifest/source
  copies. Original acquisition/runtime paths were not opened. Current external
  compiler/header installations were not rehashed or executed. Other prior frozen
  source/evidence paths were hash-verified only; this is not their re-review.
- New code retains GPL-2.0-only notices, original driver GPL-2.0-or-later and
  captured compiler/runtime license notices are preserved. Full source/license/
  receipt/artifact hashes are in `reader-receipt.json`; no global license grant or
  publication authorization follows.

| Exact input | SHA256 |
|---|---|
| A/watchdog_owner.c | `86278200eb4432b9e5805e444be2323dfe3dd6560e89e2c9a204294712325c51` |
| A/watchdog_owner.h | `7b3d51faef4b8c11283da07532bca362aa06922d18c1876e621f2a13893f5f09` |
| A/native-watchdog.inc | `c6d2c7433e7e149302c25715a88caa659fc7b7a498402007375ec25954e72d11` |
| A/native_watchdog.h | `3ce07f20af4a652767bd90fd61b98c70ce9a5281a22ee360c7147305d121ed3b` |
| A/source-close-once.inc | `f537ca9df2856c66dc60ff8765b96b956f199a4be09a80dfeb11a1b3b088d8cc` |
| A/overlay.py | `40d6fda09770937dbc80dc98d9d40c72966485e234af1cfec28a1703ca7c2e96` |
| G/native_adapter.c | `a32d2cb5c48ec96c9c32106f967c8ac75d7ac7b6e045ec86ae45ab5a6fa34c7c` |
| G/rtspd-native-import.c | `a609c60448cd633772a67be9770320dec7a93745e20c258784c77045252f8bd6` |
| B/applied-source.patch | `c7eb5c52c5bd0cd4d79e9ede7a387831a8001163cd1da5393dc4162e7e63a506` |

## W1 — fixed session budget is mutable through repeated memory_start

**Anchors:** `G/native_adapter.c:17–19,71–78`, `G/memory_runtime.c:24–29`,
`A/native-watchdog.inc:30–35`, `A/watchdog_owner.c:65–72`.

`nh_native_memory_start(start)` accepts an already-initialized memory_state whenever
the guard is present. UNKNOWN and healthy DISABLED both satisfy guard presence;
there is no one-shot test on ledger.initialized. Actual nh_memory_runtime_init
then **memsets the entire runtime/ledger**, replacing session_start, paid-buffer
records and observation count. Later native_guard_begin derives its deadline from
that replacement start, and qw_owner_begin overwrites its cached deadline.

Thus multiple ordinary serialized calls to the exported setup API can replace the
original session budget and discard accounting, without resetting the native
singleton or entering HOLD. The code does not enforce README's "no reinitialize
while owned" or the requested nonrenewable original10s session. Other capture/
sensor/memory gates may still refuse later work; they do not make this state
mutation correct. No call sequence was executed or additional capture claimed.

**Smallest correction:** in a fresh overlay revision, reject an already-initialized
memory runtime **before** invoking the unchanged initializer, leaving its original
start/accounting intact. Seal the first accepted session deadline for this owned
lifetime; subsequent begin calls must use that same deadline, not a refreshed
setup time. Do not modify the frozen memory-policy implementation, add a restart
API or extend the10s/2s budgets. An already-expired/uncertain owner remains HOLD,
not newly admitted by setup replay.

## W2 — early NEXT failure does not zero outward packet/key

**Anchor:** `A/native-watchdog.inc:39–54` (identical included G/file).

With valid non-NULL packet/key pointers, failed preparation prerequisites return
CAMERA_NOT_READY at:44–45, and a failed native_guard_begin returns at:46, without
clearing either caller output. Zeroing exists only after entering the legacy body
(:50–53). An existing populated caller packet/key therefore survives these failure
returns, contrary to the selected **fail-zero-outward-packet** boundary. This is an
API output-contract issue, not an executed stale-frame delivery observation.

**Smallest correction:** initialize valid outward packet/key objects to zero before
any such early return, or funnel all valid-output failure paths through one
zero-output epilogue. Keep native.produced_storage, resident/parked backing, private
expected receipt and all actual still-owned FDs unchanged on failure. Clearing
outward metadata must not be implemented as release/free/retire or a close retry.

## W3 — NEXT changes watchdog/codec state before known-held-loan refusal

**Anchors:** `A/native-watchdog.inc:39–54`; `G/native_adapter.c:122–130`;
`G/rtspd-native-import.c:173–174`; compare `A/native-watchdog.inc:71–74`.

A successful NEXT can leave the watchdog logically DISABLED and a CPU loan held
until release. A second serialized NEXT currently calls native_guard_begin first:
SETTIMEOUT5/ENABLE/GETTIMEOUT can execute, and native_next_legacy performs lifetime
open/query, **before** produce_h264_frame discovers produced_storage and returns
CAMERA_BUSY. The wrapper then fences this known busy state as HOLD.

That is an ordinary lifecycle regression: a refusal the real producer already
knows before work now reactivates the watchdog/queries codec state and converts
known ownership into terminal uncertainty. It is not a periodic feeder claim;
the first erroneous invocation itself suffices. The RETIRE wrapper already has
the appropriate no-effect known-loan BUSY check before guard begin.

**Smallest correction:** before native_guard_begin in NEXT, reject a known held or
parked CPU loan with CAMERA_BUSY and zero outward NEXT outputs, with no control/
query and no alteration of the original private loan/expected receipt/FD custody.
Keep uncertainty/HOLD checks sticky; do not use this known-busy branch to clear an
already uncertain owner. The independently missing failed-next process owner must
still retain any actual unreturned loan/backing.

## Other reviewed boundaries — real improvements, not runtime passes

| Boundary | Actual selected source/API observation |
|---|---|
| Sole moved descriptor | A/watchdog_owner.c:34–43 requires empty owner, FD>=3, O_WRONLY/CLOEXEC and expected character rdev; source slot is cleared only after validation. State becomes UNKNOWN. Metadata does not prove sole open-description custody/current image/recovery; those remain explicit trusted, serialized/no-alias/no-reentry prerequisites in the header. |
| Guard establishment | :65–80 enters ENABLING before SETTIMEOUT5, ENABLECARD, GETTIMEOUT5; all controls pre/post-check real monotonic time/descriptor identity. No second watchdog open/dup, V write, final close or background feeder exists in selected generated code. |
| Strict phase check | :45–63 checks real monotonic clock before and after descriptor metadata and after ioctl, rejects backward/unknown clock, phase elapsed>=2s or session expiry. No syscall preemption/finite scheduling proof follows; W1 concerns the mutability of the session supplied to those otherwise real checks. |
| Capture-qualified sole KEEPALIVE | G/rtspd-native-import.c:137–154 checks actual complete extent/pointer, guard/status/sequence, computes full raw CRC and requires matching keyed ACTIVE receipt before its only progress call. A/watchdog_owner.c:82–88 allows at most one KEEPALIVE per NEXT. No caller "qualified" Boolean substitutes for the real path. |
| Sensor139 uses same FD | G/rtspd-native-import.c:128 passes c->watchdog.fd directly to unchanged nh_sensor_exact139; G/sensor_baseline.c:66–103 only GETTIMEOUT-checks that borrowed same numeric FD plus original bounds/cancel checks. No second open/dup/owner or extra ping is introduced. The unchanged139 table is not newly native-qualified. |
| Capture + encode + postchecks remain inside outer interval | G/native_adapter.c:122–135 calls first lifetime, real producer, second lifetime, cancellation/receipt and encoded-duration check before returning to the wrapper's disable. Generated producer retains capture/encode checks; the two old separate guard open/V-close brackets are absent. producer_begin_ns uses the earlier outer start (:175). This is not complete provider-FD adoption or an all-signal-order proof. |
| Post-acquisition failure retention | Once legacy NEXT runs, A/native-watchdog.inc:50–54 zeroes packet/key and fences on failure, leaving produced/resident/parked backing and actual remaining FDs retained. W2 covers missing early refusals. No synthetic armed recovery follows from HOLD. |
| Normal retirement | G/rtspd-native-import.c:61–96,242–250 performs real owned query, sensor-off, END, keyed RETIRED/END proof and ACK before provider close; errors stop rather than retry END/ACK. No-resource-before-capture is an empty local retirement, not invented sensor proof. A/native-watchdog.inc:71–93 calls real bodies inside RETIRE guard and only disables on successful proof/clear uncertainty. |
| CPU backing free | Original generated retirement bodies now retain resident/parked storage through their final checks (G/native_adapter.c:144–168). Only A/native-watchdog.inc:64–93 frees/clears it after successful logical disable. Failed/late disable retains actual parked/resident backing; no observed DMA/electrical-stop claim is made. |
| Healthy custody handback | :95–101 requires no armed receipt, provider/codec FDs, loan/resident/parked backing, uncertainty or close_unknown before qw_owner_give_back. Owner :100–114 requires healthy DISABLED, actual metadata/clock checks and an empty recipient slot. Move only; no final close. Static raw/external hub backing and missing provider/backing adoption are not thereby qualified or released. |
| Sticky uncertainty | qw_owner_fail and native fence retain FD and HOLD without another watchdog control/feed/free. Failed or late SETOPTIONS disable may already have stopped hardware: a retained FD is not an armed recovery guarantee. Original SIGSTOP quarantine reports actual watchdog FD/state and failed-close diagnostics separately; it is not the missing common RTSP failed-next process owner. |

## Actual kernel watchdog and Linux close semantics

Current inspected watchdog core and Anyka source matched their retained76 source
pins, **not** a live kernel/config/ABI association:

- `K/drivers/watchdog/watchdog_dev.c`:
  `07e4291f639506d558d9eaf0176f7b901ab132580f1800c58dc2340c70814bf2`.
- `bringup/qc07f/variants/security75/mach-ak3918ev200/watchdog.c`:
  `d6646a2b894f822ea486c4d24db7a49df2003a5706cf25e9908bc1fe15fe06e9`.

Core:784–825 supports the actual controls; SETTIMEOUT invokes timeout update then
watchdog_ping, and the Anyka active/running set_timeout also reloads (:67–75).
ENABLE may already be active (core:248–279). Thus begin is genuinely effectful,
which is relevant to W3; it is not merely metadata. Core open:854–913 is single
open and starts hardware. Stop:292–319 honors nowayout, updates logical flags and
calls real driver stop; Anyka:56–65 writes keyed registers and returns0 without
electrical stop/DMA proof. Release:942–988 may stop or ping depending on active/
magic-close/nowayout outcome. None of these allow a failed disable to be called
armed recovery or justify a fallback V/close/re-enable.

**source-close-once is consistent with real Linux close semantics.**
K/fs/file.c:692–708 clears the descriptor-table entry/releases its number before
flush/final file release. K/fs/open.c:1574–1601 gets that removed file, flushes and
fput_close_sync's it even on flush error; it explicitly converts restart errors to
EINTR because the table entry is already cleared. The file-table final put invokes
the driver's release, independently of whether close reports an error.

A/source-close-once.inc:4–14 checks guard before attempting close, clears its actual
slot **before the one attempt**, records a non-owned diagnostic number/close_unknown
on failure and fences. Sequential provider close chains stop on failure. The local
encoder case at G/rtspd-native-import.c:222–225 retains encoder_hold only if the
pre-close refusal left the actual slot intact; it does not re-own/retry a numeric
FD after close error. The actual watchdog slot is excluded and never closed here.
The real fs/open.c and fs/file.c contracts, not file_table.c alone, are pinned in
the new review evidence. No close or device operation was executed.

## Compile evidence and remaining gates

Author build04 records **26/26 successful commands**, including six actual units
per HOST/ARM platform, archive and relocatable link/inspection. Metadata, logs,
generated source, captured dependencies and artifact pins matched. HOST GCC16.2.1
and ARM GCC14.3.0/ARMv5TE soft-float are author-recorded tools, not rerun validation.
These are archives/REL objects, not an executable, full paired kernel/module/image
build, complete provider/main or actual native guard result. Earlier build01
failure and superseded build02/03 remain preserved, not substituted for build04.

Strict2s/5s, original10s,128KiB,40MiB/CMA and native2MiB/HOST4MiB policies are not
waived. The real derivative still has old large allocations/raw backing and sensor/
frame/encoder opens: preserving them is **not**128KiB compliance or a memory-fit
result. Frozen RTSP glue, persistent frame/encoder custody helpers and observational
query candidates remain separate; the generated adapter still uses its original
56-byte lifetime ABI and has not adopted the new separate per-open query helper.
Failed-next process retention, admitted backing, mandatory provider descriptors,
provisioning/listener, functional/native/combined-memory and full G2/G3/G8 remain
unmet. P1 is not reviewed or accepted by this disjoint task.

Static reader command:

```sh
python3 -B docs/qualification-plans/qc07f-watchdog-owner-review-20261022/read_public_inputs.py
```

`verification.json`, `captured-public-input-map.json`, `public-excerpts.json`,
`actual-build04-applied-source.patch` and `reader-receipt.json` retain exact custody.
Finalizer rehashes inputs and seals this fresh directory only. Two initial lookup
misses (`build04/generated/` and nonexistent plan `10-RTSP.md`) produced no files;
actual `build04/source/` and `08-STREAMING.md` were used. They were path lookup
errors, not policy rejection or a reason to rerun any source generator/build.

**ONE next ordinary source dependency:** a fresh watchdog-owner revision addressing
W1–W3, keeping this build04/report/seal and all other frozen increments intact.
Do not proceed to RTSP/provider/native adoption on these pins. This handback
selects no runtime/adversarial test, full build, target queue or automatic recovery.
