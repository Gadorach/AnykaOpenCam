# Driver entry24 v3 — independent ordinary method-correction review

2026-10-24 · BASE-01/05/07, PLAT-06/08; driver-first preparation.

**Narrow conditional SOURCE/METHOD/custody/order sufficiency for the selected
delta. No ordinary blocking defect identified.** This is not invocation permission,
parent adoption, a passing replay of24a, or native/product GO. V1's negative
disposition and the actual v2 attempt24a **FAILED/HOLD** remain unchanged.

## Basis and independently checked static evidence

Read all **1,190 lines** of the new source, README, CONTRACT, HANDBACK, both patches,
ORIGINAL_INPUTS, STATIC_RESULT and DELIVERY_PINS; the public ATTEMPT24A_RESULT and
SAVED_VENDOR_ENVELOPES; v2 parent disposition and public image selection/association/
parent context. Prior full v1/v2 composition and R1 reviews remain context at their
stated scope. No private capture, historical raw/GO record, image or DTB work artifact
was opened. Package-review file was hashed as a public closure input, not freshly
re-performed as an image/layout review.

Abbreviation **Q3** means
`docs/qualification-plans/driver-supervised-entry-v3-20261024/`.

| Exact public input | Bytes / SHA256 |
|---|---|
| `apps/qualification/qc07f-driver-entry24-v3/driver_entry.py` |67420 / `9c96a88627c1fee03e8f1177f9f45b5b91ca9808d71ffe40f3f0830c18bd9713` |
| Frozen v2 `apps/qualification/qc07f-driver-entry24-v2/driver_entry.py` |67303 / `38d9764bb29c942d15fba1e188d02f32780fe32ff635e101c7c3358d5ce04fb7` |
| Q3 `DELIVERY_PINS.tsv` |1387 / `d36cd919b32d5ef5ba814af81e0fd6211f88fe66fd0f94e3709b7d385f7acb76` |
| Q3 `ORIGINAL_INPUTS.json` |4921 / `97c2a26484bbd8a256b562dd2978b90b7c7a1954d630440a565834f19503f0f5` |
| Q3 `STATIC_RESULT.json` |2202 / `8168014724c42aada830294c7f5c94dcad4e5bce80fa1436344a673dfd18784a` |
| Q3 `SOURCE_DELTA.patch` |1297 / `367daab8bf4e8211cca710bf9b4792fcb7c56733329f9621fe1bbce08feca071` |
| Q3 `INVERSE.patch` |1297 / `c92670265b458aa2ff03a0d10903eac52d107c7364ee8ee5ab0779b340bb7e00` |
| `docs/qualification-plans/driver-supervised-run-20261024/ATTEMPT24A_RESULT.md` |2457 / `35580fb038f9242a1001a00f2ca3614a9680e73837c713a86ea535f3a94394ce` |
| Same directory `SAVED_VENDOR_ENVELOPES.md` |2620 / `a23e75d3d4425c5fccbdcde0e778ad24a6fb833fe3f0bb8c0ab2b25b25f7bb16` |

- Independently expanded the original closure: **29 inherited + eight v2 delivery
  + nine additional =46 unique frozen files**, all sizes/SHA matching. Both referenced
  manifests match their own pinned bytes. **21 exact members/seven original roots**
  match; no old pin or membership was rebased. All **eight new delivery records**
  match, with the delivery manifest separately pinned above.
- AST parsing and source-byte comparison: **39 complete function definitions**,
  including nested definitions; **37 exact**, only `classify_boot` and
  `boot_inventory` differ. Both signatures match. All three outside-function
  segments match: **25,022 +2,158 +32,615 =59,795 bytes**.
- The entire **10,522-byte `finish`** is exact, SHA256
  `05e205c7d144c017f02729e0e359d9b5e095b66bad64e81f6c2c61dfe421b96f`.
  Its corrected R1 branch, nested `record`, all original postchecks and success/
  failure dispositions remain unchanged.
- Actual minimal diff: **two hunks, four added/two removed lines**. Saved forward/
  inverse patches equal independently regenerated unified diffs; line-span byte
  reconstruction in each direction matches the actual sources. No patch application,
  pattern execution, synthetic case or candidate/helper call occurred.
- Initial reviewer metadata inspection failed with a list-versus-bytes `TypeError`
  in its text-reconstruction expression, before printing its result. A corrected
  stdlib-only join completed the comparisons above. The failure remains in the tool
  transcript; no candidate code, source, seal or earlier evidence was altered.

## Findings on the two actual changes

**1. Fixed SF-read envelope — source580.** The sole predicate now begins
`\n(?:\n)?SF: 393216 bytes @ 0x0 Read: OK\n\nanyka\$` under the same `re.fullmatch`
and existing `data.replace(b'\r', b'')`. By inspection the added group enumerates
exactly one or two leading LF, not zero or unbounded blank lines. The entire remaining
reply stays literal: one success record,393216 bytes/address0x0/status OK, two LF
before the held prompt and no suffix. No strip, search, prompt offset or unknown-byte
discard is introduced. The whole-response cursor advances only afterward at581;
independent original BOOT CRC remains the next call582. Cold-prefix/trailer/quiet
admission is untouched. This addresses the exact parent-reported24a discrepancy,
not arbitrary RX anomalies or proof of successful hardware reading.

**2. Received boot vocabulary — source495–501.** Only these two string constants
are added to `classify_boot.fixed`:

```
*  kernel: image load address = 0x81808000
   XIP Kernel Image ... OK
```

The second has exactly three leading spaces. They enter the existing pre-kernel
fixed-record branch, including duplicate rejection/header membership; phase transition,
unknown-line refusal, existing size/header-name conditions and actual new Linux/
help checks remain byte-identical. No new order normalization or free-form prelude,
kernel/address wildcard appears. Inherited header ordering rules are preserved,
not strengthened into a new complete vendor-format theorem. The old generic
`Loading Kernel Image ... OK` alternative remains unchanged. GO/READY and COMMIT
envelopes in `_text`/`boot_inventory` are unchanged; the only changed statement in
the latter is the SF predicate.

These literals are **received framing**, not TX commands or image selection. The
81808000 diagnostic's internal meaning remains unproved here; it does not change
DEST82408000, entry82408040, independent whole-RAM CRC, DTB/decompression bounds or
CMA policy. XIP text proves neither no-copy operation nor physical memory fit.

## Custody, history and conditions

Parent public evidence reports24a ended OFF/finalized after845RX/163TX:57TX catch
plus106TX for the first four fixed commands. Its SF reply had the unique fixed
success line/two normalized leading LF/no post-prompt RX. No subsequent BOOT CRC,
neutral reads, loader/QCL/GO, image/Linux boot or driver effect occurred. Actual
ON/OFF acknowledgements/GETs, owned wait0/final hashes,20 retained inputs/three
markers and independent OFF/absence checks are **parent observations**, not fresh
observations by this reviewer.24a remains FAILED/HOLD, not harmless-noise resolution.
The22b formatting associations likewise depend on the parent's saved-evidence
provenance, not a capture inspected here or a current lease.

All other globals/functions/bytes are exact: canonical ROOT role at the same app
depth, source pins/helper selection/CLI, strict image schema, clock/cost checks,
original5400s caps/reserves, CAS/no-retry, cold classifier, sole actual Popen/peer/
start, one ON/OFF, known-OFF stop, unknown-power retention and complete R1 available-
evidence audit. No new command, callback, transport retry or deadline renewal.

The four public image records retain their original pins. They report selected
QC6DF01.UIM,3403371B/SHA256
`d37615e86699ef5d80b159eb690ec130049c4ee023785e9d6d80fc169f543cee`, BZIP2
`f8371c93`, load82408000/entry82408040. This review neither requalifies those work
artifacts nor substitutes file hashes for parent provenance. The old context's
specific v2 trial selection is **not v3 invocation authority**. Parent disposition,
reviewed current v3 source/image context, genuinely current manual supervision and
fresh sole ownership/admission remain required before any separately selected trial.
No resume of24a/23h/22b, automatic retry or next queue follows.

Inherited trusted immutable source/config/dependency conditions and all full-review
limitations remain. Static matching does not establish runtime console acceptance,
throughput, hard deadline/preemption, independent host-loss recovery or strict C1
prevention. Smart-switch cutoff needs an operating host; current human intervention
is its host-loss fallback. Physical STOP/thermal/zeroing, complete FD/DMA ownership,
combined memory/capacity, Wi-Fi/MTU1500, microphone and product gates remain OPEN.
Speaker tests DEFERRED; backend expansion PARKED. **No self-GO.**

Only this fresh review directory was authored. Public read/hash/text/AST work only;
zero candidate/helper imports, executable compilations, calls/tests/main/help,
AST/pattern execution, private/current-state/proc/device/clock/network/outlet/RPC/
UART/hardware or build effects. No old source/report/seal/Git/index/AGENTS/plan/
settings edit, subagent, new framework/test matrix, provider/model/tool/safeguard
change or operator request.
