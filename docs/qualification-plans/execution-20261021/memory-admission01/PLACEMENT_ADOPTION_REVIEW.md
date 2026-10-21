# RP-04 independent placement/adoption review — 2026-10-21

**Conditional source derivation YES; present adoption/native entry NO-GO.**
The existing metadata can support excluding **only 6,160 KiB of proved,
retained ISP coherent CMA backing** from the *future-evacuation* term. It cannot
add that memory to Available/free CMA, retire DMA, pay codec debt, or establish
continuous/combined memory acceptance. No current placement receipt was read.
The existing v6 helper implements **no exclusion** and must not be launched with
its failing gate bypassed. RP-04 combined-memory **HOLD remains unchanged**.

## Scope and reproducibility

Independent source-only lane; parent alone owns hardware. Assignment's RAM76
boot/idle checkpoint is context, not this lane's current-runtime authority.
No target, RPC/status, procfs, network, private input, existing test execution,
build, ARM execution, policy/source/ABI/allocator edit, central ledger or Git
operation occurred. Only this new document and `placement-review/` were written.
Existing report/audit/run34/old failures were not edited.

New [source-audit.json](placement-review/source-audit.json) was generated once by:

```sh
python3 -B docs/qualification-plans/execution-20261021/memory-admission01/placement-review/audit_source.py
```

The authored script reads explicit public text/source/config/JSON paths only;
it neither imports project code nor opens image/ELF binaries. It records UTC,
Python, its own hash, reference seals, bounded verbatim source excerpts and
**78 matching source/config/manifest comparisons**. These recheck all 25 v6
local files and run34 copies, ten build audit pins, seven corresponding76 kernel
pins, nine framework inputs, run34 manifest and final76 config. This differs from
the prior audit's 80 comparisons because it does not reread its two binary
artifacts. Historical HOST passes remain historical, not rerun here.

The separately authored `placement-review/audit_headers.py`, invoked with
`python3 -B docs/qualification-plans/execution-20261021/memory-admission01/placement-review/audit_headers.py`,
records **11 additional matching comparisons** in
[header-audit.json](placement-review/header-audit.json): current/run34 consumed
receipt wrapper and receipt/stream/codec UAPI headers against manifest pins, and
the latter three exact76 headers against those same pins. This is text identity,
not actual running-provider association or execution/ABI acceptance.

Abbreviations:

- **K76**: `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.
- **M76**: `work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/package76-manifest.json`.
- **ISP/ENC**: K76 `arch/arm/mach-ak3918ev200/{isp.c,qc07f_hwenc.c}`.
- **NP**: `apps/web-media/native-producer/`.
- **V6**: `evidence/delegated/20261012-native-handoff/`.

## 1. Why these particular pages can be fixed, not merely eligible

| Required fact | Exact source support / limitation |
|---|---|
| Allocation identity and full extent | ISP53–55,282–295,1557–1563 allocate table `0x4000`, frame `0x200000`, reference `0x400000` via successful `dma_alloc_coherent(...,GFP_KERNEL)`. Probe fails unless **all three** exist and pass its System-RAM check. The driver's check is **not** a CMA-placement check. |
| Existing address receipt | ISP473–495 `state` emits `table_dma`, `frame_dma`, `ref_dma` under source mutex. Successful read rejects not-ready/disconnected/quarantined state at that instant. It also exposes queue/IRQ/owner/guards. Bytes1728000 are valid frame bytes, **not** frame extent. |
| No alternate pool/translation | ISP1509–1524 rejects PM/IOMMU/Xen/dynamic-OF builds, custom DMA ops, range map, declared coherent memory and per-device CMA area. Default direct ARM allocation only. Config/provider association must match; do not infer this from an address alone. |
| Coherent allocation really removes pages from free CMA | K76 `kernel/dma/mapping.c:628–668` → `kernel/dma/direct.c:205–221` → `arch/arm/mm/dma-mapping.c:1811–1816,533–599,391–424`. On this noncoherent ARM configuration, blocking allocation with default CMA selects `cma_allocator`; exact PAGE_ALIGN request pages are allocated, not a speculative movable-page loan. `include/linux/dma-map-ops.h:99–107` selects global CMA if no per-device area. |
| Fixed allocations are unavailable to subsequent CMA selection | K76 `mm/cma.c:782–859` searches **zero bitmap ranges**, marks the allocation exclusive before `alloc_contig_range`, and leaves successful allocations marked. Release990–1018 frees pages and clears bitmap. Retained coherent allocations are not the movable borrower pages evacuated to satisfy a later codec request. |
| DMA address is physical here | `include/linux/dma-direct.h:75–115`, absent arch-specific translation/encryption and with rejected `dma_range_map`, is identity translation. Effective config and actual provider must establish these premises. Do not generalize to another image/device. |
| A real lifetime pin exists without capture | ISP143–151,215–228: successful frame-device open creates per-file context and holds bundle kref; it does **not** BEGIN/capture/allocate pixel DMA. Bind returns source/file identity. ISP1492–1507 frees the three buffers only at final bundle release after disconnected/retired/no-session/no-open/no-quarantine checks. |
| No silent replacement while pinned | Registry admits one bundle, ISP1529–1536; release clears registry only at final disposal. Existing file pin survives remove; remove marks disconnected and drains callbacks/owners,1580–1602. This pin prevents address reuse, **not** continued healthy operation after removal. |

Total: `16 + 2048 + 4096 = 6160 KiB` / `6307840 B` /1540 pages.
Frame/ring slot0 alias counts once. No credit for alignment gaps, valid-length
tails, inactive references, ring statistics, atomic pool or other occupied CMA.
The source-default atomic256 KiB is **not** included in this proof: state does not
identify its placement/initialization. Partial/pre-existing codec backing also
remains unknown for this exclusion; a byte-total query supplies no extent list.

## 2. Small bounded proof contract, using existing metadata only

These are **future parent admission requirements**, not performed operations.
The smallest variant is all-or-nothing for the above three allocations: `F=6160`
only when every premise below holds; otherwise **no credit** (`F=0`), and deny if
the original gate fails. No user-controlled subtraction amount or generic pool
registry is needed.

1. **Associate the running source first.** Establish exact76 image/config/DT and
   built-in ISP/codec/provider identities (section5). Parent serializes the finite
   run: no competing capture, queue/ring diagnostic, remove/rebind/reset, module
   replacement, media worker or unaccounted allocation exercise. Existing counter
   sampling is not a lock on unrelated kernel/network demand.
2. **Pin before taking credit.** A successful open of the actual
   `/dev/qc07f-frame` must remain held continuously from before address/counter
   sampling through all uses of `F` and the prospective codec allocations. Bind
   its existing source/file identity; associate its sysfs parent/device number
   with the state provider. The same adopted producer FD is preferable to an
   independent pin that could be closed or substituted during handoff. A textual
   old receipt, PID, closed fd, or two equal address reads is **not a lifetime pin**.
   BIND alone is observational and not an exclusive stream reservation; do not
   invent QUERY of generation0 (the receipt implementation rejects it).
3. **Validate all intervals, not their bases only.** Parse the actual emitted
   `%pad` hex addresses with bounded complete-field grammar. Require page-aligned,
   nonzero bases, checked addition, pairwise disjoint half-open intervals of the
   exact three full extents, all wholly within `[0x82800000,0x83800000)`. Known
   `get_order`/CMA_ALIGNMENT8 start alignment is an additional consistency check,
   not permission to count gaps or power-of-two payloads. Never read the pages,
   /dev/mem, pagemap or raw image data to establish this.
4. **Certify healthy gated-only bundle.** Successful current state before/after
   sampling, same addresses/provider, guard_errors0, no unexpected IRQ/queue
   activity or foreign owner. Existing `QC_ISP_STAGE_QUERY` on the pinned FD can
   additionally corroborate live_requested_bytes6307840, zero ledger/clock/
   saturation/retirement flags, zero retire failures and a source-consistent
   allocation/free history. For untouched probe this is3 successes/no failures/
   no frees. Initial `result=-ENODATA` is source-normal at probe1565, not an
   invented capture success; later accepted state needs its actual qualified
   retirement history. Stage counters are observational, **not** a lease or a
   quarantine check; a successful state read and real receipt ownership remain
   necessary. Queue/ring backing or inconsistent history refuses this narrow
   contract rather than expanding its credit set.
5. **Keep samples and credit bound together.** With that same pin held, collect
   the existing fresh meminfo→zoneinfo→meminfo envelope at each existing gate;
   do not splice boot-idle/old WLAN counters into it. All three observations must
   have occupied CMA at least `F`; underflow/inconsistent geometry/boost/identity,
   stale/failed state, closed pin, source fault/removal or cancellation denies.
   `state_show` drops its temporary kref on return, so a sysfs read alone cannot
   provide this guarantee. Its mutex does not cover the subsequent proc sample;
   the durable FD pin and fixed-allocation source lifetime supply that bridge.
   Added metadata parsing/storage/opens must be bounded and charged, with clock/
   cancel checks; retain the existing100ms proc-sample envelope and2s/10s outer
   work/session limits. Extra proof reads cannot silently extend deadlines.
6. **No health/lifetime extension by arithmetic.** Start with idle ownership;
   after actual ARM/BEGIN retain the same source/file/generation and existing
   ACTIVE health checks. Once capture starts, no longer require owner_active0;
   require the exact owning receipt and gated path. Normal END/QUERY/ACK and all
   quarantine/retain-on-uncertainty rules stay intact. A fixed backing can remain
   physically allocated on a failed provider; that does not make a failed run
   admissible or authorize continuing/closing/freeing/resetting it.

**Why races need not defeat this particular exclusion:** the bundle pin proves a
lower bound of fixed occupied pages across the entire sampling/allocation interval;
unknown occupants may change but cannot turn those pinned allocations into movable
borrowers. Using worst sampled occupied CMA and subtracting that **same persistent
lower bound once** preserves every sampled unknown occupant. Equal snapshots
without the pin do not prove persistence. This does not turn nonsimultaneous free
counters into an absolute bound on later unrelated allocations; the existing
bounded-workload/overhead/reserve limitations remain, not silently solved here.

## 3. Conditional derivation — exclusion, never capacity credit

Use KiB and the existing v6 notation. For occupied CMA observations
`Oa=CmaTotal−CmaFree_a`, `Oz=4*(Normal.cma−Normal.free_cma)`,
`Ob=CmaTotal−CmaFree_b`, let `O=max(Oa,Oz,Ob)`.
After validating `F<=min(Oa,Oz,Ob)` and persistent placement:

```text
U = O − F                         # ALL other occupied CMA stays unknown
M_future = codec_paid ? 0 : min(U, codec_unpaid8256)
A >= CPU_unpaid + codec_unpaid + H512 + R1024      # UNCHANGED
N_above_W >= CPU_unpaid + H512 + R1024 + M_future
```

The `min(...,unpaid codec)` cap is inherited from v6, not newly justified as a
worst-case kernel-compaction/fragmentation theorem. This review just proves which
occupied pages cannot be its evacuation candidates. Bitmap occupancy, alignment,
contiguity, allocator retries/failures and other transient costs remain real; the
amendment cannot certify allocation success. Do not reduce W or count boost twice.

No subtraction from global codec8256, CPU capacity2844/3872, H512, R1024 or sampled
occupied/free counters. No Available+CMA sum. Never add F as free capacity or
subtract it again as baseline demand. Unknown includes own prefaulted CPU pages
that may borrow CMA; own loans receive no second exclusion. Codec payment still
requires the exact complete healthy existing8454144-byte lifetime condition after
first complete return. No credit from malloc success or partial allocations.

**Dated i arithmetic ONLY** (`O7168`, `N5476`, `A14756`):

| Mode | F | Unknown / M | Required N | Margin N | Required A |
|---|---:|---:|---:|---:|---:|
| raw reuse, original v6 |0 |7168 |11548 |−6072 |12636 |
| raw reuse, hypothetical full proof |6160 |1008 |5388 |**+88** |12636 |
| separate, hypothetical full proof |6160 |1008 |6416 |**−940** |13664 |

No receipt proves F for that historical observation; this table is **not a
reclassification of its preserved HOLD**. The narrow88 KiB margin is especially
not a measured peak/headroom result. Current76 counters, helper mapping size,
network demand and layout must be evaluated afresh. Nothing here promises a pass.

## 4. Exact parent v6 adoption/rerun operation — NOT executed

First independently review `V6/parent-review-v5-to-v6.diff`,
`RAW-REUSE-CONTRACT-v6.md`, `REPORT-v6.json`, current NP sources and generated
import/closure in run34. README's later **historical RAM-file CLI** is not the
v6 TCP command; use `main.c:18–21` and REPORT-v6 as the exact CLI authority.

From repository root, the existing offline reproduction command, with a fresh
run name (absent at this review), is:

```sh
python3 -B apps/web-media/native-producer/build.py runParentV6RP04A \
  --toolchain work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

It builds/runs existing synthetic HOST optimized/sanitized cases and receiver
fixtures and builds/inspects, **never runs**, ARM output. It writes only its new
`V6/runParentV6RP04A/` tree, refuses an existing run, and pins compiler/runtime/
source closure. Parent must separately have authorization for that operation;
it is outside this lane. Expected historical suites42/35/67/24 (each opt/san)
and receiver16 are comparison targets, **not claimed new results**. Compare full
manifest, controlled import diff, consumed headers/framework/tool closure,
ELF attributes/map/stack/layout and artifact hashes; retain any changed result.
A different rebuilt ELF needs its own identity/layout seal, not run34's hash.

That command reproduces **unchanged v6**, not a placement amendment or76 adoption.
There is no existing `--kernel76`, `--fixed-ISP`, override amount or adoption
command. A detached parent spreadsheet cannot suppress its internal initial
failure: `main.c:30–32` gates before hub malloc/prefault; current adapter opens
the source later. Thus external proof alone cannot make the present executable
use F, nor keep its missing early source pin alive inside it.

Historical v6 native syntax, recorded for review **not a queued invocation**:

```text
native-handoff --parent-protected --sensor-baseline exact139 --peer 192.168.1.15 --port 17891 --memory-reserve normal75-placement-one-shot --producer-storage raw-reuse-k75
```

Peer above is an historical explicit endpoint, not current network authority.
`--parent-protected` is an assertion, not a recovery lease. An admitted76 variant
requires explicit parent source/identity adoption of the75-scoped policy/alias
contract; do not invent76 aliases or silently reinterpret those tokens. Review
must still cover all-main private input copy **before** user raw output overwrite,
raw validity revoked before ioctl/EFAULT, no subsequent raw-view consumers,
producer-loan release before next raw, full raw/NAL/oldhub integrity and exactly
13 produced/12 known-duration outputs with whole terminal retirement. No TLS or
continuous-DMA-import claim follows.

## 5. Exact76 identity/provider prerequisites, still unfulfilled here

- Seal M76 → retained QC6U76.UIM3603555B,
  SHA256 `bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f`;
  final config SHA `9e1f37788d3bd8580f2eb031ea2a7d795f4b61e2c1f89e1b91c11f1a87a6499c`;
  vmlinux SHA `24df07ee09121eeff50ab01a5aa002a67418e5593d955f79836447673d427b14`;
  appended retained attempt45 DTB SHA
  `d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`
  (PACKAGING_REVIEW.md). These are recorded seals, not binary rereads in this lane.
  Parent must associate actual booted bytes/build identity/config/DT and boot
  instance with these, not rely on `uname 6.18.55` or filename. M76's NOT_BOOTED
  field is historical packaging time; later boot evidence does not amend it.
- Exact ISP C SHA `7d0ef80561df4ef058be1deb6698001c35066fab5533f492dec97d2b447bbd50`,
  codec C `e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f`,
  codec UAPI `6723c30c76a3fbd1feaf1ed9d1676a2b9b6289b352ffe7682372bf8e95123731`
  match v6's75 pins. Audit also seals76 stream/receipt/telemetry headers/helpers
  and allocator sources; actual consumer-header/build association must match.
  Matching a vermagic or shared function name is insufficient.
- Actual geometry remains40MiB System RAM, volatile
  `cma=16M@0x82800000-0x83800000`,4 KiB pages, sole populated Normal/empty Movable;
  strict parser's managed8286 pages, CMA4096 pages, protection0 and base watermarks
 178/222/266/310 plus current boost must be revalidated, not relaxed to fit76.
  Config confirms ISP/codec built-in, DMA_CMA, CMA_ALIGNMENT8, no HIGHMEM/PM;
  dynamic provider/IOMMU/translation changes invalidate this contract.
- Frame device must be this built-in static-DT `qc07f-isp-diagnostic` provider at
  ISP0x20000000, unique bundle/source; permanent syscon provider is the retained
  `anyka,ak3918ev200-intc`/`syscon` at0x08000000 with no dynamic clock/reset/power
  provider contract. Codec `qc07f-hwenc-dma` uses separately owned MMIO
  `[0x20020000,0x20020500)`, default coherent CMA and retained provider/code until
  reset (ENC460–504). Confirm actual devices, not merely existing /dev names.
- Only the reviewed exact139 controlled **stream-off** baseline; retail140 ends
  `0x0100=1` and must not be substituted. Guard/cache/reset/reference ownership,
  sensor identity and BIND/ARM/ACTIVE/END/RETIRED/ACK association stay mandatory.
  Healthy codec LIFETIME is not an encoded per-FD END receipt; exact synchronous
  source semantics and native retirement still need qualification.
- run34 helper is91236B,
  SHA `64434b3c3c7dc9bae28c6ff66bddbc01a4f08a9e6972d0082b0f418036ff2319`;
  M76's bundled359520B `/bin/rtspd` is **not** it. M76 neither embeds nor adopts
  the v6 helper/placement policy. Exact helper/linked ARMv5te EABI5 soft-float
  musl/runtime, staging and recalculated mapping debt must be parent sealed.
  New proof logic changes that ELF identity/debt; never retain2844 by fiat.
- Parent-only actual current owner/runtime/recovery/FD/lease gates precede every
  admitted operation. Preserve2s/10s/128-frame/watchdog5/outer protection limits;
  no prefault/churn/boost change to rescue denial, no retry or process kill as DMA
  retirement. No radio/network/media/native test is authorized by this document.

## 6. Smallest next task / handback

**Parent accept or reject this precise source contract first**, then independently
review/rerun unchanged v6 offline in a fresh run. If accepted and separately
authorized, the smallest implementation task is a bounded76-specific admission
amendment to the existing producer: move its existing source open/pin/identity
association before any F-dependent gate, validate the three state extents and
existing health metadata, carry that pin through every gate/first allocation,
and change only `M_future` by the proved fixed lower bound. No kernel/UAPI or
allocator change, new page probe/registry, policy reduction or native test needed
to review that increment. Its own proof storage/open overhead and changed ELF
mapping must be charged, and its fresh HOST coverage must include missing/stale/
duplicate/overlapping/wrapping/outside-CMA intervals, pin loss/provider swap,
quarantine/removal, sampling underflow and unknown-own-loan preservation.

This lane **does not implement or authorize that task**. If binding/persistence
cannot be established with the existing provider/FD/metadata contract, keep F0
and HOLD; do not build an ABI/page-probe framework to force a pass. If the original
gate naturally passes a genuinely fresh state, no exclusion is required.

Conclusion: source allows this small conditional exclusion; **no present proof,
parent v6 adoption/rerun,76 adoption or native memory acceptance is complete**.
Continuous capture/import, combined TLS/locked processes, fragmentation/reopen,
other product workloads, RP-04/05 and G2/G8 remain open.
