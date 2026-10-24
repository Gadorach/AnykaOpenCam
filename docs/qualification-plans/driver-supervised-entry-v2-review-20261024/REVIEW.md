# Driver entry24 v2 — independent ordinary correction review

2026-10-24 · BASE-01/05/07, PLAT-06/08; preparation for RP-02/11/12.

**R1 CLOSED at SOURCE/METHOD/custody/order scope. Narrow conditional sufficiency;
no further ordinary blocker identified in this correction and its reviewed
composition.** Not execution admission, image pairing, parent adoption or native/
product GO. V1 and its negative review/disposition remain frozen **NOT ADOPTED**.

## Basis, exact pins and independently checked static counts

Read the full new **1,188-line** source, README, CONTRACT, HANDBACK, both patches,
ORIGINAL_INPUTS, STATIC_RESULT and DELIVERY_PINS; read the parent's V1_DISPOSITION.
This correction review retains the full v1 source/composition review as context,
including its conditions and limitations, not as adoption of the defective v1.

| Public input | Bytes / SHA256 |
|---|---|
| `apps/qualification/qc07f-driver-entry24-v2/driver_entry.py` |67303 / `38d9764bb29c942d15fba1e188d02f32780fe32ff635e101c7c3358d5ce04fb7` |
| Frozen v1 `apps/qualification/qc07f-driver-entry24/driver_entry.py` |65982 / `262ec18013905e2774eb197c4d509061bf928724188c77337c8a1ddb030bf765` |
| `docs/qualification-plans/driver-supervised-entry-review-20261024/REVIEW.md` |11641 / `93f9d6b175709e3a23a1066a9fbecbeebddd186379d6ef54b5804cafb8f5fcea` |
| `docs/qualification-plans/driver-supervised-entry-parent-20261024/V1_DISPOSITION.md` |1303 / `3984a14085f26bf710d6ffd0f70b53ee5570c0c43f49b9002a786ad389078dcf` |
| v2 `ORIGINAL_INPUTS.json` |6062 / `9a67cac2ec597729cd31eaac4fa1e1ac8df09cf35239773dc0023a9948fbb689` |
| v2 `DELIVERY_PINS.tsv` |1376 / `45e544b54cbf28b43c3a6742b1b46868ed233e7c07abd811cacf5d6bb48eb2c7` |
| v2 `SOURCE_DELTA.patch` |1866 / `edb35c4dbe92ca5ee373c464beb885d31707a2f3dea1d27840ae9a111ea2581d` |
| v2 `INVERSE.patch` |1866 / `fef4c17502c1954221f9f4131c23e4206ffce785b4c7b99937f375051fa3dc51` |
| v2 `STATIC_RESULT.json` |1924 / `1956cd2c6547815d483281c93f7a1eab13dcf43f839f2ee1671baa306390b3de` |

Here, v2 metadata paths abbreviate
`docs/qualification-plans/driver-supervised-entry-v2-20261024/`.

- All **29 original file pins**, including the original manifest, review and parent
  context, independently match size/SHA. All **26 original manifest records** agree
  with that snapshot; exact membership remains **10 files/four original directories**.
- All **eight delivery records** independently match size/SHA; the delivery manifest
  excludes itself and is separately pinned above. Original pins were not rebased.
- AST parsing only: **39 complete function definitions**, including nested ones;
  **38 byte-identical**, with only `Entry.finish` changed. Its signature is exact.
  The **55,238-byte prefix** and **1,543-byte suffix** outside the method are exact.
- Exactly **22 lines inserted, zero removed**, at new source1083–1104. Forward and
  inverse saved patches equal independently regenerated unified text diffs. Removing
  only that insertion from the new bytes reconstructs the old bytes exactly. No
  patch was applied and no candidate code, branch or fixture was executed.

## R1 finding and source-order proof

The added `elif`1083 attaches to the existing live-child `if`1039, after its entire
passive-retention body. This closes the v1 counterexample without duplicating its
other cleanup paths:

| Actual own-child observation at1039 | Resulting source path |
|---|---|
| No child | Both live and added branches excluded; original input postchecks follow. |
| Unknown OFF; own `poll()` returns terminal status | `returncode` is available from the actual retained Popen; new1083–1104 runs. |
| Unknown OFF; own `poll()` returns `None`, then child exits | Existing live branch remains selected; actual wait and eventual audit run there. The later `elif` cannot also run. |
| OFF proved | Existing controlled cleanup is unchanged; `not self.off_proved` excludes the added branch. Any still-live retirement failure retains the existing passive path. |

This is textual control-flow reasoning, **not executed timing/race coverage**.
It assumes the retained single foreground owner and trusted dependency/syscall
conditions already stated in the original contract; no competing reaper is added.

On the new branch:

- 1087 explicitly keeps `final_ok=False`;1088 records the actual cached child status
  as uncertainty metadata, not a natural-expiry diagnosis, controlled stop or OFF
  proof. Existing1038 already records unknown power. `record`973–976 appends errors
  without replacing an existing first HOLD/failed phase.
- Each PID/socket/visible-owner absence check1089–1096 has its own exception handler.
  An individual failure does not skip the other checks or the final evidence attempt.
- 1097–1104 retains the available0600 final manifest and calls the unchanged
  `audit_final(..., controlled=False)`. That audit915–967 validates actual argv/cwd,
  private file membership/hashes, RX coverage, event ordering and TX intent/completion
  accounting, with original cold/cumulative checks where available. It expects no
  controlled-stop event; a contradictory event or other invalid evidence remains
  failure, not a relaxed success. Missing/invalid manifest or audit failure is
  explicitly recorded; the temporary descriptor closes even on validation error.
- The branch adds **no RPC/power/UART request, kill, unlink, takeover, retry, clock
  check or deadline renewal**. The fuser call is the existing host absence helper,
  not a target status request. Unknown OFF never enters the known-OFF stop RPC.
- All original retained-input/marker and ancestry postchecks1105 onward still
  follow, each independently attempted before descriptor/lock release. Live-child
  custody is not shortened: a child dying just after the own poll still follows
  the old actual-wait path. Known terminal status alone grants no power authority.
- Even exit0 plus valid absence/hash/accounting cannot pass: `final_ok` remains
  false, `off_proved` remains false, and sticky errors/HOLD independently defeat
  the unchanged success predicate1127. Manual OFF remains required in the result
  when ON was attempted without OFF proof1137.

No minimum source correction remains for **this R1**. That finding does not reopen
or relabel v1 as accepted, nor qualify runtime behavior of either version.

## Conditions and evidence boundary

All unmodified source/method conditions from the full v1 review remain: trusted
immutable source/config/dependency custody during the lease, original historical
prerequisite/marker checks, fresh sole owner/cold catch, exact closed transport/
console schemas, fixed deadlines and postblocking-cost detection. Static equality
does not establish throughput, hard preemption, normal-path console acceptance,
whole-interval timing or target-observed behavior.

Actual independently reviewed paired-image provenance/selection, parent disposition
and fresh current supervised admission remain mandatory. Smart switch is normal
cutoff while the host operates; a genuinely present user is host-loss fallback.
Supervised discontinuity detection/abort is not strict C1 prevention or unattended
power recovery. No complete process-FD/DMA, physical OFF/STOP/thermal/zeroing,
memory-fit, Wi-Fi/MTU1500, microphone or product qualification follows. Speaker
tests remain DEFERRED; backend expansion remains PARKED. No next queue or self-GO.

Only this fresh review directory was authored. Public reads/hashes/text/AST only;
**zero candidate/helper imports, executable compilations, calls/tests/AST execution,
clock/private/proc/device/current-state reads, RPC/outlet/UART/network/hardware or
operator operations**. No build/recipe/help invocation or builder/output polling,
original/source/seal/Git/index/AGENTS/plan/settings edit, subagent, adversarial case,
or provider/tool/model/safeguard change.
