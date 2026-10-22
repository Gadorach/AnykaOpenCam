# Independent C1 ordinary SOURCE/API review — corrections required

2026-10-22; ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Review of the frozen
`apps/qualification/encoder-query77-custody/` candidate only. **No SOURCE/API
acceptance yet.** Two correction-required findings below are static control-flow
findings, not executed observations. The separately acknowledged historical
CONTROL-location conflict remains an operational blocker, not either finding.

## Scope and exact input

- `custody.py`: 803 lines, 34,669 bytes, SHA256
  `7dc6d11ff7dee758f49b3b0e4f7e43fb8b45215cda3761a75732519079e12b2e`.
- Two-file source aggregate (sorted workspace-relative path, NUL, raw digest):
  `5c2d338973ab6384b504b6b52203b072e9ea04a3b9a357127638f1829b2ea286`.
- Read the entire pre-code C1 contract, README, module, author inspection and
  build/syntax/inventory/seal receipts; complete frozen design contract and source
  anchors, parent design disposition and historical public Boot definitions23–25.
- Only public reads/hash/JSON/AST inspection. No candidate/helper import, execution,
  compilation, CLI, tests, generators, mutation/fuzz/adversarial campaign, private
  filesystem exercise, original marker/root access, target/current-state query,
  UART/RPC/network/proc/dev access or hardware operation. No broad concurrency or
  filesystem architecture audit. Serialized ordinary Python/host assumptions are
  retained, not replaced with hostile monkeypatch or privileged-race assumptions.
- Only this new review directory is written. Source, design, actual HOLDs, Git/index,
  AGENTS/central plans, helpers, licenses and all failed evidence remain untouched.

## R1 — supplied previous observation displaces original catch/counter baseline

**Correction required, public pure API.** Anchors in `custody.py`:202–250,
especially209–215,220–239. Contract34–41 and author inspection15 promise checks
against the immutable original tuple as well as the previous observation.

The function checks the type and epoch equality of `previous`, but does not
establish its original-epoch counter/catch constraints. The loop220–228 applies
both anchors only to clocks. At231–237, counters and catch use **either** the epoch
or `previous`, never both. `Observation.__post_init__`183–188 validates only field
types/bounds, not these relationships. Thus an ordinary, well-typed supplied
previous record is incorrectly treated as a trusted replacement baseline.

Source-only reachable traces (not constructed or executed):

1. An Epoch whose initial catch is CONSUMED or UNARMED, a same-epoch previous
   record with ARMED, and an ARMED observation can reach the Budget return under
   REQUIRED. Choose equal original/previous/current clocks and an unchanged
   sufficiently large original cap: the clock and budget branches do not refuse;
   `old_catch` is ARMED, so236 is false and238 accepts ARMED. BOOKKEEPING_ONLY also
   does not restore the missing original-catch check. This contradicts no revival.
2. An Epoch with positive original RX/TX, a same-epoch previous record with lower
   counters, and a current record matching those lower counters passes231–234.
   Equal clocks and valid caps again do not refuse. Original nonregression is lost.

This is not a claim that supplied records are authentic or can authorize hardware.
It is an internal contradiction of the public pure validator's promised arithmetic
constraints on explicitly untrusted records. `Custody.open` currently requires
an armed epoch419 and guard `_last` is internally advanced only after validation;
no corresponding reachable guard-based catch revival is asserted here.

**Minimum fresh-revision correction:** validate the supplied previous record's
original-epoch clock/counter/catch/cap relationships, and enforce original as well
as previous counter/catch constraints on the new observation. In particular an
initial non-ARMED catch cannot become another state through a supplied previous
record, even BOOKKEEPING_ONLY. Preserve allowed ARMED-to-consumed/unarmed
bookkeeping without permitting later revival. Keep exact clocks, no epoch rebase,
original deadline/reserves and prior-cap-minus-elapsed decay. Do not introduce
authentication, live sampling or ready flags. A fresh static review can check
these branches without executing the validator.

## R2 — exceptional public-operation exits omit historical postchecks

**Correction required, failure-path custody/evidence contract.** Anchors:
`_call`664–684, `_failure`640–662 and `_check_all`600–608. The frozen design
contract85–89 explicitly requires dual-HOLD comparison after operations,
**including failure**; C1 contract50–54 and README50 also promise before/after
checks. These are not merely successful-call checks.

On an ordinary body exception (for example a refused argument after the successful
entry check), control jumps from675 directly to679. The postcheck678 is skipped.
`_failure` checks only the new private operation/ledger via `_check_private`, not
the historical descriptions/raw bytes/path metadata, source descriptors or full
roots. It may append FIRST_FAILURE without any attempted historical exit check.
This omission also occurs after an append/fsync exception. Later explicit `close`
does check all790, but a future close is not the promised failed-call boundary.

The object remains terminal, first failure is sticky and the permanent marker
remains HOLD: this is **not** a false native GO or a claim that old files are
written. Nevertheless the advertised exceptional-exit comparison is absent.

**Minimum fresh-revision correction:** on an admitted call's exceptional exit,
attempt the required read-only historical/root/source postchecks while preserving
the original failure as first failure. Keep secondary postcheck failure distinct
where safely recordable. Do not retry writes/fsync, append after unknown/partial
write, replace files, clear HOLD, or change teardown into a retry. Do not demand
new IO from a refused reentrant/closed/terminal call or from a constructor before
the relevant host descriptions were acquired. Specify that bounded distinction
in the new source contract rather than silently weakening failed-call custody.

## Other inspected paths — no additional correction-required finding

These are conditional static findings, not acceptance of the candidate as a whole:

| Area / source anchors | Ordinary inspection result |
| --- | --- |
| Top-level1–16 and declarations | Standard-library declarations/decorators only; no authored import-time host IO, CLI/main, target/controller/process/current-state interface. GPL-2.0-only retained. |
| Types55–199; clocks216–250 | Exact built-in int excludes bool; signed63 bounds, immutable typed records, bounded strings, fixed pins/enums. `previous=None` safely takes the epoch anchor twice in220–228, not a None dereference. Python arithmetic does not wrap; Budget validates final bounds. Exact deltas intentionally lack any admitted live sampler. |
| Budget240–250 | Current cap bounded by original natural remaining and prior cap minus elapsed, reserves150s plus bounded selected cost. Repeating records cannot extend the deadline. Subject to R1's previous-record correction. |
| Roots263–339 | Linux directory-FD walk from `/`, canonical bounded ASCII components, NOFOLLOW/CLOEXEC/DIRECTORY/NONBLOCK; ancestor current-uid/root ownership and no group/other write, selected owned0700; anchored `.git` and descriptor/path associations. Directory timestamps/link counts deliberately not pinned. |
| Historical files425–446,496–508 | Fixed radio-then-reset RDONLY/NOFOLLOW/CLOEXEC/NONBLOCK, EX/NB flock, regular owned0600 single-link prechecks and FD/path comparison. <=4096-byte JSON rejects duplicate/extra keys and wrong exact operations; raw and full metadata repeatedly checked. No write/create/chmod/truncate/unlink of originals. R2 covers missing exceptional postchecks. |
| New files448–483,510–579 | Exclusive operation mkdir0700 and O_EXCL/APPEND0600 marker/ledger, association checks, file and directory fsync. Permanent initial IN_FLIGHT_UNKNOWN/HOLD; partial constructor files retained, no existing-operation adoption/resume. Bounded marker and streaming ledger sequence/hash/tail/readback checks. |
| Source581–598,686–705 | Borrowed regular O_RDONLY single-link FD, <=64KiB, offset0, <=4096-byte preads, metadata/hash checks. CLOEXEC owned duplicate installed in preallocated slot and retained/rechecked; original caller FD never closed. Shared-offset mutation is a refusal, not a separate-file-description claim. Caller must establish source/path/executed-code provenance. |
| Writes610–662 | Poison before one write and file fsync; short/unknown writes terminal, no retry/error append after poisoned state. Directory-fsync/readback uncertainty terminal. First failure fixed/sticky; diagnostic formatting only after safe private-ledger checks and only exact known built-in exceptions, bounded private detail, no public path/raw/digest or exception-chain display. |
| Local phases707–745 | Only two C1 phases; matching pending receipt, source pins and observation required. Digests are opaque bookkeeping, not verified external receipts. Completion has no True/native grant. After complete flag is set, a failing context exit still raises, marks first failure and cannot return OK; close preserves that outcome. |
| Teardown331–339,747–803 | Owned source duplicates/locks/files/dirs cleared or popped before one close, remaining closes attempted on error, no numeric-FD retry, no destructor. Constructor failure closes owned host descriptions without deleting files. Completed/failed guard still requires explicit close. Close/cleanup errors are non-OK; ledger already closed cannot persist a later cleanup error. |

## Separate operational/path contract blocker — not resolved here

Both v1 selected roots must be outside Git. Public `scripts/qc07f_boot.py`23–25
defines the historical CONTROL as ROOT/work/qc-uart-live, under this workspace.
It therefore cannot pass v1 `_Root.check`/`.git` rules. The candidate correctly
refuses its own conflicting contract; no real original-marker custody was proved.
Generic typed records, other markers or copied files do not repair that association.

Parent checkpoint steering now describes a possible separate read-only historical
root/new-private-output path-role amendment. It is contextual steering only for
this frozen v1 review: no amendment implementation or amended path contract was
reviewed, no source revision selected by this review, no migration/ignore-Git
switch/copied marker authorized. Parent must resolve and separately review actual
in-place original-marker association before any operational application.

## Receipts, drift and evidence limits

`INPUT_PINS01.tsv` seals20 explicit public inputs. `STATIC_CHECK01.json` records
the fresh comparisons. All six frozen design pins match; all eight author METHOD
output pins (including two source files and six reports/receipts) match. Final
source/inventory/syntax pins and all effect-callsite line lists match. Every listed
Custody inventory method matches; AST additionally finds the deliberately refusing
`__init__`390, which is not listed in the author's method table. This inventory
scope difference is recorded, not treated as source drift or a runtime failure.

All nine author input rows were compared: six frozen design matches and three
changed contextual checkpoint files, not nine unchanged. Exact old/current pins
are retained in STATIC_CHECK01; the original manifest is not rebased. A harness
AGENTS update explicitly describes the parent's review/path-role checkpoint.
Full historical byte-diff attribution is not claimed. The fresh20-input snapshot
is after substantive source reading, before this report, not a pre-use/build seal.
Final read/hash comparison is in FINAL_SEAL01; further context drift, if any, is
recorded separately without changing frozen inputs.

Three author AST/syntax revision PASS receipts are cited, not rerun. This reviewer
parsed public source AST only; no compile/code-object/library/test behavior was
executed. Tool/policy errors: none. No policy refusal or reroute occurred.

No filesystem durability/lock/cleanup runtime proof, total Python/stdlib4MiB RSS
proof, native128KiB fit, actual owner/clock/source authenticity, E2/native/method/
recovery/manual-OFF readiness or product PASS follows. Last12:12UTC U-Boot fact is
historical only; both actual HOLDs remain untouched. Parent alone owns hardware.
**Handoff: R1/R2 require a fresh bounded revision and ordinary independent review;
this review schedules no implementation, retry, test, build, worker or native queue.**
