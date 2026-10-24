# QC07F span memory — independent ordinary review

2026-10-24. CAP-01/05/06/07, RP-04/09, B07. SOURCE/API/bounds/ownership/accounting only.

**Disposition: narrow conditional SOURCE/API/ledger/prefault sufficiency. No
blocking ordinary defect found within the declared trusted-caller contract.**
Parent adoption is separate. NOT RUN; no native/cap/readiness or product GO.

## Scope and exact identity

Read all eight files in `apps/rtsp-auth/qc07f-span-memory/`, the author's CONTRACT,
HANDOFF, AUTHOR_AUDIT, SOURCE_DELTA, both literal diffs, DELIVERY and full build
report/command metadata. Compared the original owned-runtime memory units and
actual native adapter, ready and provider-return paths; read the accepted v2 hub
header/body and parent disposition, plus the actual span packetizer header/body.
No dependency on or adoption of either separate sender candidate was inferred.

SHA256, with candidate filenames relative to `apps/rtsp-auth/qc07f-span-memory/`:

| File | SHA256 |
| --- | --- |
| `memory_reserve.c` | `3de7e750b5f846c1a77804113862881947074f42ed5718627efe70942337316a` |
| `memory_reserve.h` | `9893a15d37fd9867d1595bd0b5485fd7f19157b8ee28a7b4b3046cc94bd66e0e` |
| `memory_runtime.c` | `0e0434b5a8a80493e2582f8efa890fb634dd21543da35808ffd706c3a0a00332` |
| `memory_runtime.h` | `eafa9773488bdd03d0e4bcf2dfd218810c30351e5dc029275be0e0bac53dfcd5` |
| `memory_reader.c` | `c9bd8401476f3baf73b43051651e6c306f14a05c1d5a5317531302569d4261c0` |
| `handoff.h` | `9b88ce3e24a3a6c1c16336315323b70b29560c473ac9a48d36043bd60c840113` |
| `compile01.py` | `de0fc7f20eda25c2a4c174d760f80b11241345816659616ce44c471817f49ca7` |
| `LICENSE_NOTICES.md` | `27fd9954156ca49536e7b6c7056ed80534693abb0ede718647187213122d7593` |

Evidence root: `docs/qualification-plans/qc07f-span-memory-20261024/`.
`DELIVERY.json`: `fe3b8352b91ffa983d029858931f289cf94d54c056ebc6bdd36eeb56eb09df7f`.
`runs/build01/REPORT.json`:
`12327c7f7f084f85d92159323c2736e3d9683cc979255f252b7f988a7a7319d6`.
Accepted `span-hub-v2/span_hub.c` / `.h`:
`bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1` /
`310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454`.
Its `span-hub-parent-20261024/V2_DISPOSITION.md`:
`3bc9454af6953f074b11e3e8ec29e0109873126b16753c937205cd4ab0dd24a5`.

## Ordinary source findings

1. **Real typed whole-hub forecast and atomic metadata payment.**
   `memory_reserve.c:119–153` uses the actual v2 type, not a shadow/flat adapter.
   It checks self/ACTIVE, pristine generation/publication/client/token metadata,
   exact allocation history 8/1048576/131072, all eight canonical nonnull chunks,
   and absence of prior hub payment/range history. Each fixed131072-byte extent
   has checked pointer addition, pairwise byte disjointness and disjointness from
   every paid scalar extent. The output is assigned only after complete validation.
   The public credit API reforecasts, then commits owner/count/eight ranges/exact
   credit and paid[1] without a subsequent refusal or callback. This is logical
   atomicity under serialization, not machine-atomic or fault-recovery machinery.

2. **No flat, boundary, padding or duplicate credit.**
   `memory_reserve.c:97–117` and `memory_runtime.c:54–66` refuse scalar kind1 as
   their first action, before dereferencing ledger or purported backing. Raw(0)
   and producer(2) APIs remain; their credit checks ALL recorded hub ranges,
   not the deliberately zero scalar begin/end[1]. Full interior pages are exactly
   `[ceil(begin/4096), floor(end/4096))`; disjoint byte extents cannot share a
   credited full page. Division/remainder avoids address-rounding wrap. Range
   lengths bound individual credit arithmetic; the hub sum is explicitly checked.
   Paid<=capacity precedes subtraction and every payment fits the remaining
   capacity before any ledger commit, so the final paid addition cannot wrap.
   No hull, per-chunk repeated kind payment, source lease, release or DMA adoption.

3. **Actual all-eight touch transaction, including unused chunks.**
   `memory_runtime.c:81–112` fully forecasts before callbacks or backing access,
   requires two free observation slots, then retains exact owner/ranges/credit and
   latches TOUCHING before the first clock/cancel check or actual gate. The first
   gate still includes unpaid hub debt. Each of all eight chunks receives volatile
   SAME-byte read/write at offsets 0,4096,...,126976 and131071: every intersected
   page is touched, including the last unaligned boundary page. No current NAL,
   publication, subscriber, prior lease, copy or checksum substitutes for touching.
   A fresh final gate is also unpaid; forecast and retained association are checked
   again, then actual whole-hub credit occurs. Only PAID assignment/OK return follow
   a successful commit; there is no fallible postcredit callback/outward failure.

4. **Attempt history and borrowed lifetime are preserved conditionally.**
   Preflight refusal admits no attempt and changes no retained association/ledger.
   Every returned clock/cancel/read/gate/association/credit failure after admission
   latches HOLD with its reason, unpaid ledger and exact borrowed backing retained.
   Subsequent whole calls cannot retry or return idempotent success; runtime gates
   and scalar prefaults do not resume work from HOLD. Runtime init checks initialized
   ledger/non-FRESH attempt before memset (`memory_runtime.c:25–30`), so ordinary
   reinitialization cannot erase this new HOLD. Direct ledger initialization/credit
   remain trusted one-time/metadata APIs, not a guardian: resetting/editing storage
   or paying partial touches through them is outside the contract. A synchronous
   address fault is not caught; TOUCHING remains nonretryable, not a survived-fault
   claim. There is no cleanup/disposal/owner release in this transaction.

5. **Timing and conservative debt policy are not relaxed.**
   Original checked() is byte-identical: original unit/session starts, 2s/10s bounds,
   real now/cancel callbacks. The real fixed meminfo→zoneinfo→meminfo gate retains
   100ms sample/64-observation bounds; only its HOLD fence is added. No cached gate,
   renewed deadline or strict C1 whole-interval guarantee follows. Parsers and
   codec-credit/evaluate region are byte-identical; reader/handoff are identical
   whole files. ARM ELF32/ET_EXEC layout guards remain and HOST layout refuses;
   the public-fixture branch is removed, not used to fabricate a HOST layout.
   Worst user-extent page cover changes flat1MiB257 pages/1028KiB to8×33 pages/
   1056KiB: +28KiB. The formula replaces that hub term ONCE and raises minima
   2804→2832 raw-reuse /3832→3860 separate, retaining mapped image, staging and
   separate producer debt. Full-interior credit leaves boundary debt unpaid.
   This is not allocator header/arena/fragmentation measurement. The unqualified
   512KiB envelope,1024KiB reserve, exact8454144-byte codec policy and full unpaid
   CPU/occupied-CMA future-migration policy remain; no ISP/CMA exclusion or cap rise.

## Read-only evidence checks

Fresh size/SHA256 checks matched19 delivery,29 selected-input,98 dependency-path,
12 tool,89 artifact,26 log and15 frozen-reference records (288 overlapping checks,
not288 distinct files or a complete compiler-execution closure). Resolved paths
matched where recorded. Ten actual `.d` files reproduce exact98-path membership.
All26 command JSONs equal the report entries and record exit0 with explicit C
locale/environment/timestamps. Recomputed all six source comparisons/opcodes,
both literal forward/inverse diffs and all five unchanged-region comparisons.

Cited author evidence, NOT rerun:10 C compiles,2 actual rooted five-object REL links,
26 commands,2026-10-24T04:31:47–49Z; HOST GCC16.2.1, retained musl ARM GCC14.3.0.
Actual units per role are reserve/runtime/reader/v2hub/span, no fixture defines,
stubs or old builders; compile logs are empty and link traces name exactly those
five objects. Independently inspected all ten object/two combined ELF headers and
both REL symbol/attribute outputs: HOST ELF64/x86-64 REL; ARM ELF32/ARMv5TE/EABI5
REL, real new APIs/reader/hub/packetizer defined, ARM `__ehdr_start` normally
undefined with libc/compiler dependencies. Not an executable/native ABI/layout
or transitive-stack proof. Initial reviewer nm byte comparison used inherited
locale and failed on symbol ordering; explicit `LC_ALL=C` inspection exactly
matched both saved outputs. This was metadata inspection, not a build/test retry.

## Mandatory conditions and unclosed integration

- Truly fresh zeroed persistent storage, actual accepted v2 allocation history,
  valid stable disjoint runtime/ledger/owner/output/private writable chunks; no
  copies, mutable aliases, mutating callbacks, reentry, signals, fork or concurrency.
  Shape/history is not hostile-pointer, allocator, residency or permission proof.
  Public credit is ONLY the metadata half after actual whole touching, never a
  ready bool. Exact owner/chunks remain held through every admitted outcome until
  separately proved real retirement; this module cannot enforce external lifetime.
- Same names/guards have NEW layouts: rebuild all consumers, use only replacement
  memory units and v2 hub; NEVER old memory units/hubs/combined RELs together.
  Original `native/native_adapter.c:83–94` still invokes flat kind1 and now refuses.
  `owned_runtime.c:61–75` only checks old paid[1], not new attempt/association proof;
  `native/provider-return.inc:39–42` omits new history. None is integrated/admitted
  by this review. One paid flag is not native memory readiness.
- Native raw/output1MiB sites,128KiB allocation-cap compliance, allocator/library/
  stack/socket pressure, process2MiB/40MiB/CMA/combined-load fit, external retention/
  recovery/backing and actual adapter/ready/return integration remain open.

Reviewer ran no build, production/test program or candidate/helper function, no
prefault/credit/reader/proc/malloc/native/callback/socket/RNG/credential operation,
and imported no candidate. Python standard-library metadata scripts and read-only
readelf/nm tools were used for the evidence checks above.
No negative/adversarial/mutation/fuzz matrix or subworker. Only public source/
metadata/hash/ELF reads and this fresh review file were produced; no source-code,
AGENTS/central-plan/Git/index/private/device/network/hardware changes. Last OFF
after finalized23h, RAM75 recovery/C1 block and old failed HOLDs are historical
and unchanged, not fresh state. **Parent disposition only; no automatic next work,
runtime/adoption or hardware queue.**
