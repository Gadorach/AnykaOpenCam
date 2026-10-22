# Station76 P2 v2 — focused ordinary SOURCE re-review

2026-10-22. RP-02 / NET-01–08 / K76-06, P2 / S3; R1–R3 correction only.

## Result

**Narrow SOURCE sufficiency: R1–R3 corrections accepted at static source/API scope.**
No further correction is required by this focused read of their actual delta and
ordinary surrounding lifecycle. This is not functional confirmation, complete S3,
composite attachment, paired-build, native, RF-silence or product GO. The v1
CHANGES REQUIRED review remains correct and frozen for its original pins.

P1 remains absent/unmet and was not reviewed, retried, adopted or completed. P3–P5
and ordinary functional confirmation remain outstanding. Nothing in this handback
selects integration, build, tests, staging or target operation.

Only this new report and `station-attach-init-p2-v2-review01/` were written. Static
public reading, diff generation and hashing only: no candidate script/compiler,
runtime/test/adversarial/fuzz/mutation/penetration execution, ARM/module build,
target/UART/RPC/network/private IO, Git/index/central-status, source or helper edit.
Parent's reported RAM76/idle outcome is not independently queried or used as a
station result. No policy rejection occurred.

## Exact custody and scope

`S/` = `apps/qualification/kernel76-station-attach-init-v2/source/`.
`K/` = retained public kernel source at
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.

- Supplied v2 seal SHA256 verified:
  `40928114e2de10bc84b0dc26301db2563f1d9a1df4273b6610a7cc7ba13317bb`.
- **88/88 sealed files** and the exact71-file source set verified, including no
  extra unsealed files in the candidate/author-evidence trees at collection.
- Source-map SHA256 (compact sorted-key UTF-8 JSON):
  `a23ec7ce1e9794762e086853655e286b3567db5ddb4e449c88bab25390b9c75f`.
- **67 source files unchanged from v1; four changed**. Independently rederived
  actual diff byte-matches the complete author `R1_R2_R3_DELTA.patch`.
- **161 frozen v1/review custody entries** verified: v1's153 sealed files, the
  prior review's six sealed outputs and both seals. **204 accepted A1/A2a/A2b
  source pins** also reverified against their accepted seals. No P1 input used.
- LICENSE remains
  `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
  V1 copy/seal/review provenance and inherited notices remain retained; no global
  license grant or publication authorization follows.

| Changed S/path | SHA256 |
|---|---|
| `attach_init.h` | `1fda50d95f29848c4e2331760f21d57fd087b64023af1e2001a8f19b3f9f7d92` |
| `hgic_smac/attach_init.c` | `7dabfba55124df17a344e8f60e61f92fe575768fe29eb356cbd7c0ec054e8864` |
| `hgic_smac/attach_status.c` | `ebfc114e60aba328f927d0a13787221ee5b42183f6a578c2d371270f0350bee6` |
| `utils/fwctrl.c` | `4a556c8d66097b1599b4743ede0db1e6ea44ac56a537cab33d3bb696c6d042d3` |

| Kernel API input K/path, unchanged from v1 review | SHA256 |
|---|---|
| `net/mac80211/main.c` | `38047a679e48375f6d8cae18b6b2746d89dac38cde4ee62602ed3e56ca957e74` |
| `include/net/mac80211.h` | `43b4be700a0aa3d5b4e5ac3483db02d2506ba637e2686343b053a1d05949067f` |
| `net/core/dev.c` | `83a1a83bdf7a979b0aa27170272748c5a71070ad52227243a8f391008ebed80c` |
| `net/mac80211/iface.c` | `461e696280b214723762d85aad60641d9792f57d72a8f1f3c08c90f724aaec6f` |

All author kernel excerpts were independently reconstructed from full-file bytes
and matched range text/hashes. New review excerpts and exact input pins are in
`public-excerpts.json` and `reader-receipt.json` in the owned evidence directory.

## R1 — allocator callback group: corrected

`S/hgic_smac/attach_init.c:225–239` supplies the exact assign/unassign signatures,
including `struct ieee80211_bss_conf *link_conf`, required by
`K/include/net/mac80211.h:4770–4777`. `attach_ops:254–258` now supplies all five
private channel-context callbacks, satisfying the non-emulation completeness
condition in `K/net/mac80211/main.c:813–832`. The remaining allocator callback
consistency conditions:801–811 also remain satisfied by the selected table.

Assignment returns existing `attach_refuse()` (`-EPERM`, sticky effect-refused
cause) before any device/channel effect. Unassignment performs no action because
assignment never succeeds; this is cleanup, not a fabricated successful channel
operation. No emulation fallback, opcode, hardware action or UP path was added.
The identified deterministic ops-table rejection is removed; successful allocation
on a real target is **not** claimed.

## R2 — surviving netdev and expected cleanup: corrected

Schema2 additions are cached scalars, not device references:
`S/attach_init.h:32–33,53–59` appends NETDEV_LOST without renumbering prior phases
and adds present/removed/teardown/first-removal-expected fields. Existing milestone
`netdev_registered` stays historical. `attach_status.c:12–25` snapshots once under
the existing receipt lock and clearly labels schema2/new policy, printing all four
fields without a device query or retained netdev pointer.

`attach_init.c:571–595` sets presence only on the admitted first REGISTER; an
owned UNREGISTER or PRE_UNINIT clears it and makes removal sticky. The first
removal records whether teardown was already marked; repeated notifications do
not reclassify an earlier unexpected removal. Unexpected loss latches local
`-ENODEV`/NETDEV_LOST but returns NOTIFY_DONE, not an unregister veto or the
unavailable original later-notifier errno.

The actual kernel rollback path at `K/net/core/dev.c:11401–11410` can follow an
earlier REGISTER notification; normal removal emits UNREGISTER/PRE_UNINIT
(:12353–12376). Earlier registration unwind can emit PRE_UNINIT (:11422–11430).
These now prevent a historical REGISTER alone from becoming init success, even
when mac80211 only warns on default-interface failure and later returns0
(`K/net/mac80211/main.c:1607–1636`). Missing registration without any REGISTER
notification also fails the final presence predicate.

Final publication at `attach_init.c:377–404` takes **RTNL then receipt lock** after
`ieee80211_register_hw` returns, and requires present, no removal, no teardown/
REMOVE, plus previous strict FWINFO/error-free/registration/DOWN conditions.
Kernel removal holds RTNL before unlisting and notifying
(`K/net/core/dev.c:12280–12288,12328–12376`; mac80211 interface removal likewise
requires RTNL at `iface.c:2393–2407`). Thus removal cannot be between unlist and its
notification while final publication accepts stale cached presence. If publication
wins RTNL first, a later removal is a later error, not retroactive failure of a
historical SUCCEEDED milestone. This is a source serialization argument, not an
executed race test or continuing-presence guarantee.

`attach_core_remove:433–451` marks teardown and cancels pending init under the
receipt lock **before** ordinary hardware unregister, then releases that lock
before cancellation/wait/unregister. Expected cleanup therefore does not generate
a new spurious loss error following healthy terminal init; pending init remains
FAILED with cancellation. A prior unexpected loss/first-error stays immutable.
The marker means cleanup entry, not completed callback/URB/reference retirement.

The added RTNL acquisition is a blocking wait, not a finite deadline. In this
selected source, the worker does not hold the receipt lock while acquiring RTNL;
core removal does not hold RTNL or the receipt lock across its cancel_work_sync.
No new direct lock-order cycle is introduced by this delta. Full P4 teardown and
all-context lifetime proof remain expressly outside this result.

## R3 — negative CMD observation before completion: corrected

`S/utils/fwctrl.c:224–255` still checks envelope, association, prior `resp->done`
and exact existing CMD/CMD2 status validation. Only after successful validation,
and while the **existing FWCTRL lock is held**, :249–250 latches negative signed
status with COMMAND phase before publishing `resp->skb`, done and completion
at:252–254. Malformed/unassociated replies retain their existing rejection path;
the new status branch cannot use an uninitialized status after validation failure.

`attach_init.c:34–53` preserves an earlier first-error and makes the pending init
terminally failed. The new branch does not call generic malformed-reply handling
or replace the associated first response. The waiter checks the immutable cause
before copy (`fwctrl.c:123–142`), frees the response normally and returns that
negative cause (or an earlier immutable cause), without copying error payload.
The unchanged A1 helper `control_receipt.h:49–96` remains the signed validation/
copy contract; duplicate first-response behavior is unchanged.

FWCTRL→receipt is the same nesting already used by `bad_receipt`; receipt helpers
never acquire FWCTRL. No receipt→FWCTRL acquisition, stack-waiter async reference,
request allocation, retry, new wire token or submit ownership mechanism was added.
This closes the reported **deferred-until-waiter** latching gap; it does not claim
a total real-time order for independent concurrent observations or P3's atomic
first-error-versus-submit fence.

## Compile evidence, limitations and one next source task

The sealed author's `syntax01` records **3/3 returncode0** commands for corrected
attach_init, attach_status and fwctrl, with the changed receipt header pinned in
dependencies. Its71-file input map exactly matches this reviewed source. Compiler
log: Clang23.1.3/x86_64-pc-linux-gnu; installed Linux6.18.55-2-cachyos-lts headers.
Actual `-fsyntax-only` and implicit-function/incompatible-pointer error flags are
recorded. Warnings are retained; this is not warning-free or paired ARM evidence.
These results are cited/metadata-verified, **not rerun**. External compiler/header
installation hashes were not freshly validated by this reviewer. No syntax rerun
of unchanged units, object/link/module result or runtime pass is asserted.

Integrity reader was invoked twice; attempt1 had a reviewer-only lexical dependency
path mismatch (`hgic_smac/../attach_init.h` versus normalized `attach_init.h`), not
a candidate defect or safeguard rejection. `READER_ATTEMPT01.md` preserves the
failure/initial reader pin, and its partial diff remains unchanged. Corrected
reader normalized names and matched both header dependency rows. No author input
was altered. `public-pin-verification.json`, `actual-v1-v2-diff.patch`, excerpts,
reader receipt and final `review-seal.json` retain the successful static custody.
Finalizer only rehashes reader inputs and seals the new report/evidence.

The four-file delta leaves build selection, USB, one-owned-FWINFO command/TX
admission, original source/license/validation files, rejected UP/effect paths and
disabled recovery unchanged. No new ordinary correctness/lifetime defect was
identified in this correction scope. This does not requalify every inherited path.

**Still unmet:** P1 integration/independent acceptance; ordinary init/DOWN/rollback/
cleanup/negative-status functional confirmation; P3 absolute deadlines and owned
UNSENT cancellation/request lifetime; P4 exact allocation/descriptor/RX-start/
submitter/callback/retirement proof; P5 bounded controller/watchdog/generation
contract; paired/native/full product gates. Status/milestones do not authorize a
watchdog feed. Original radio FAILED/HOLD and accepted evidence are unchanged.

**ONE next ordinary source task:** parent may select a fresh isolated **P3 source
ownership/deadline design** for the sole command, including separately owned async
request lifetime, queued-unsent cancellation and the actual submit transition.
Account for existing500ms response/1000-jiffy token waits and newly explicit RTNL
blocking in later overall budgets; do not increase timeouts or attach a stack
waiter to an async skb. This is a prerequisite recommendation, not P3
implementation/test/native authorization or a reason to retry blocked P1.
