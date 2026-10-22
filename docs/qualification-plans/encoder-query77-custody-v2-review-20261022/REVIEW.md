# Independent C1 v2 review — narrow conditional SOURCE/API sufficiency

2026-10-22; ENC-02 / BASE-01/05/07 / PLAT-01/06/08. **R1, R2 and the explicitly
selected historical/private path-role correction are SOURCE/API sufficient under
the stated serialized host/Python contract. No additional correction-required
ordinary defect was found in the selected delta and its caller/failure closure.**
This is not runtime qualification, operational custody, full-controller/C2/native
admission, a recovery proof or a new effect queue. V1 source and its failed review
remain frozen/unaccepted; this disposition does not revise either failure.

## Exact selection and method

- Candidate: `apps/qualification/encoder-query77-custody-v2/custody.py`,962 lines,
  42,123 bytes, SHA256
  `d60176e2d2725460a22c82fee6ce1068a82a07a976daddb7287c52daf3ade16f`.
- Two-file source aggregate:
  `acb2933fd4f9e6a7f52722499905f5c855f31f674cadf52b49bb95c2f4c7d153`.
- Read the complete new correction contract, BUILD_RESULT, README, author source
  inspection, delta/method/syntax/input receipts and actual module; inspected every
  changed/new definition and its callers against retained v1. Read the parent path
  amendment and v1 parent disposition; public Boot23–25 confirms symbolic lineage.
- Public read/SHA256/JSON/AST/text-diff inspection only. No candidate/helper import,
  library call, code-object execution, compiler/syntax rerun, generator, test, CLI,
  controller, process/device/target operation or private filesystem exercise.
  No actual HOLD/control/capture/current-state/proc/dev/UART/RPC/network access.
  Static inline stdlib inspection did not invoke any candidate code.
- Only this new review directory is written. No v1/v2 source, old review, design,
  AGENTS/central-plan, Git/index, model/provider/helper/safeguard or license edit.
  No broad filesystem/security/concurrency audit or hostile monkeypatch/privileged
  race exercise. The stated namespace/descriptor serialization assumptions remain.

## R1 closure — original and previous constraints both enforced

Source anchors refer to the exact v2 module:

| Branch / callers | Independent ordinary source finding |
| --- | --- |
| `_validate_previous`209–220; caller239 | Public type checks230–234 and same-Epoch checks235–236 precede helper use. Previous clocks cannot precede original and must have equal deltas; previous RX/TX cannot fall below original; original non-ARMED cannot change state; previous cap must fit original natural remaining and base150s. This closes the unvalidated replacement-baseline defect. |
| `validate_observation`240–251 | Both original and previous anchors apply to current clocks, counters and catch. With no previous, `baseline=epoch` safely handles both None anchors. An initially CONSUMED/UNARMED epoch cannot change through supplied previous records, including BOOKKEEPING_ONLY. An initially ARMED epoch can record consumption/unarming, but a non-ARMED previous anchor then prevents any change/revival. REQUIRED still defaults to requiring current ARMED. |
| Budget237,252–262; types62–65,99–206 | Natural remaining derives from original expiry/lifetime; prior lowered cap decays by elapsed time, never restarts. Reserve150s plus bounded selected cost remains. Exact built-in ints exclude bool, bounded signed63 fields and final Budget checks stay; Python arithmetic does not wrap. Earlier selected-cost evidence is not fabricated. |
| Actual guard callers472,858,889 | Constructor still requires an armed initial supplied epoch; observe and complete use internal prior state and unchanged default REQUIRED. No new flag, sampler, source authority or callback. |

The earlier v1 counter/catch findings are corrected at the pure API, not merely
hidden by the guard. Record equality is still only constrained bookkeeping:
fabricated records are not authentic physical ownership, clock or catch evidence.
Exact three-clock deltas still have no admitted actual sampler or tolerance.

## R2 closure — admitted failures and close retain available-owner checks

| Branch / callers | Independent ordinary source finding |
| --- | --- |
| `_call`813–831 | Busy/closed/complete/failed refusal remains before admission and performs no new IO. Admitted entry, body or normal-exit exception reaches `_failed_exit`828; the postcheck is no longer skipped on exceptional exits. Completion set before a failing context exit cannot return OK. |
| `_failed_exit`747–753 | Selects/stores the primary first failure before `_postchecks`; secondary categories do not replace it. Then attempts safe private evidence and returns the fixed primary code for the public exception. |
| `_available_owner_postchecks`687–736 | Two roots, operation directory, each historical description and each of two source slots have independent bounded checks. No ledger dependency, new open, lock reacquisition, write, fsync or repair. A failed category does not skip the other available categories. Roots retain anchored path checks; holds retain raw/metadata/path checks; sources retain bounded hash/metadata checks. |
| Partial constructor445–542, root304–390, hold553–565, op571–579 | Initialized slots precede acquisition. Absent descriptions report UNAVAILABLE without probing a new path. Partial roots cannot claim full PASS. Acquired unpinned hold/op/source paths cannot establish a baseline or successful custody during reconciliation. `_HistoricalRoot.acquire` resets its capture flag in finally, so failed-exit checks cannot first pin/rebase the workspace Git boundary. Constructor failure calls `_failed_exit` before one-shot host teardown and retains partial files. |
| `_postchecks`738–745; ledger632–647,763–810 | Fixed bounded category/status/code tuples, three fixed in-memory role slots, no public exporter or authority return. Infrastructure failure is fixed POSTCHECK_INTERNAL/FAIL, not success. New exit_checks field is hash-covered and read back with bounded count, uniqueness and status/code validation. Same4096B record/64-record/64KiB limits. |
| First failure / poison755–810 | `_failure` preserves primary, skips any append if poisoned/no ledger/closed, and formats limited exact-builtin diagnostics only after private ownership/integrity checks. Safe ledger may retain FIRST_FAILURE and later distinct EXIT_POSTCHECKS. Unknown/partial write or fsync does not cause a diagnostic append or write/fsync retry. Failed evidence recording itself poisons further writes. |
| `close`926–962 | Busy/closed refusal has no IO. Finally attempts independent available role checks even if `_check_all` failed halfway or the ledger is poisoned, before host teardown. Earlier error remains primary; a postcheck failure without earlier error is non-OK. Completed flag cannot override a failed call, close entry or postcheck. |
| `_teardown`894–924; `_Root.close`349–357 | Unchanged clear/pop-before-one-close host-only ownership. Remaining closes continue after an error; no numeric-FD retry or retention claim. Cleanup errors remain fixed non-OK; after ledger closure they cannot be durably appended there. No destructor or automatic resume/reconciliation loop. |

The postchecks are bounded observations of the failed work **before** diagnostic
bookkeeping/teardown, not an atomic immunity claim extending through all later
instructions. The new contract explicitly states that limitation. Poison can
leave primary/secondary evidence only in object memory; the permanent initial
IN_FLIGHT_UNKNOWN/HOLD remains authoritative. No missing ledger evidence grants GO.

## Path-role amendment closure — source conflict addressed, no operational PASS

- Constants21–23 bind only canonical
  `/home/gadorach/Desktop/AnykaOpenCam/work/qc-uart-live` and pinned original Boot
  source. Public `Custody.open`445 no longer accepts a control-root argument.
  `_control` uses `_HistoricalRoot`, `_private` uses separate strict `_Root`449.
  No Boot import/discovery, compatibility path, alternate root/worktree, marker
  copy/migration or caller Git-exemption switch is introduced.
- `_HistoricalRoot`360–390 invokes base traversal only with that exact constant.
  At the exact canonical workspace only, an existing safe regular/directory `.git`
  marker is observed with NOFOLLOW stat, trusted ownership/no022 (regular single
  link), full metadata initially pinned only during acquisition and compared later.
  All other traversed nodes require `.git` absence. No Git contents are opened or
  interpreted; no boundary can first be pinned during failure checks.
- `_Root._git_condition`324–326 remains unconditional `.git` absence for all private
  ancestry. Existing owned0700 selected roots, safe ancestor modes/ownership,
  directory-FD/path associations and NOFOLLOW traversal remain. Same control/private
  inode refusal475–476 remains. The historical exception cannot flow to new output
  policy; operation directory also requires `.git` absence579.
- Historical opens478–499 remain exactly two fixed in-place names, RDONLY/
  NOFOLLOW/CLOEXEC/NONBLOCK, radio-then-reset EX|NB flock; original contents are
  never created/written/truncated/chmodded/moved/deleted/cleared. All new opens508,
  510 and mkdir501 are under the distinct private role. Role associations are
  checked normally and independently before failed-call/constructor/close teardown.

This closes v1's **source contract** contradiction under the explicit amendment.
It does not prove that the canonical path, Git boundary, actual markers, modes,
aliases, locks or supplied epoch currently match. No current metadata was read.
Only genuine in-place original-marker association can support future custody;
other markers/constants/typed records are not substitutes. Namespace/privileged
mount/borrowed-FD exclusions and actual source-to-executing-code provenance remain
separate future caller prerequisites, not guarantees manufactured by C1.

## Frozen lineage, delta and receipts

`INPUT_PINS01.tsv` records35 explicit public inputs. The first fresh reviewer hash
snapshot18:07:20UTC is after substantive source reading, before this report, not a
pre-use/build seal. Static JSON/AST checks are in `STATIC_CHECK01.json`; final
read/hash comparison and output pins are in `FINAL_SEAL01.json`.

- All26 author input rows compared:23 match; three contextual central checkpoint
  files changed after author freeze. Exact expected/current pins are recorded,
  not rebased. Harness steering describes the parent's v2-delivered/active-review
  checkpoint; full historical byte-diff attribution is not claimed.
- All eight v2 METHOD output pins match, including both source files and six
  reports/receipts. V1 module/method, failed-review seal/three outputs, eight v1
  METHOD outputs, six frozen design inputs/five design seal outputs, Boot and parent
  amendment pins match. No transitive46-input design requalification is claimed.
- Both source aggregates match. Actual reproduced textual delta is +212/-53,
  21,207 bytes, SHA256
  `fe39190cc97076198a62613490695b212c3460534b4e76b9b9f8cc235b05f627`.
  AST receipt lists match exactly:14 changed,11 new,0 removed,46 unchanged
  definitions, with unchanged stdlib imports. All new/changed helper callers were
  independently enumerated. Hash/AST agreement is lineage evidence, not semantic
  proof; the ordinary control-flow findings above supply the focused reasoning.
- Final source/syntax receipt pins match; author HOST AST/syntax PASS is cited,
  **not rerun**. Reviewer parsed text AST only, no compile/import/behavioral traces.
  SPDX GPL-2.0-only remains. Tool/policy errors and refusals: none; no reroute.

## Boundaries retained

No actual custody/admission, filesystem durability/locking/runtime result, total
Python/stdlib4MiB RSS or native128KiB fit, real clock/source/owner/catch authenticity,
alias/retention/DMA/recovery proof, manual-OFF readiness, method/C2/native/interface/
main/product acceptance follows. Last12:12UTC held U-Boot/catch facts are historical
only, not fresh runtime/owner/expiry coverage. Both actual HOLDs remain untouched;
parent alone owns hardware. No source adoption/deployment, automatic implementation,
retry/build/test/worker/native queue or physical action is selected by this review.
