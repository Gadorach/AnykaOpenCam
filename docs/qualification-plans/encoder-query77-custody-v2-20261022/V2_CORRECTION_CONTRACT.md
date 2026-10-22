# C1 custody v2 correction contract — before code

2026-10-22; ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Parent explicitly selected
one **fresh unreviewed SOURCE revision** addressing independent R1/R2 plus the
parent path-role amendment. No v1 SOURCE acceptance or native/controller/E2 GO.
Only new custody-v2 source/docs may change. V1 source/design/contracts/review/
seals/failures remain immutable. No library/import/test/generator/helper execution,
private/current filesystem exercise, process/device/network/UART/RPC/hardware,
Git/index/central-plan/safeguard action. HOST AST/in-memory syntax only, no pyc.

## Frozen lineage and narrowly changed source

V1 module34669B/SHA `7dc6d11ff7dee758f49b3b0e4f7e43fb8b45215cda3761a75732519079e12b2e`,
two-file source aggregate `5c2d338973ab6384b504b6b52203b072e9ea04a3b9a357127638f1829b2ea286`;
v1 method seal SHA `6fbf6f024096ce20e34a120ebb7899a798e14ab726ee580ad3fb2e02e582f597`.
Review seal SHA `239b007833ca839d1c391b3594b4a654f915a0d53e856795c333e366ce4d58fd`,
disposition CORRECTIONS_REQUIRED_R1_R2. Preserve GPL-2.0-only source notice and
copy provenance. No broad typing/style/cleanup refactor, new effect profile,
timing sampler/tolerance, source-closure authority, shell/boot/query/retention.

## R1 — supplied previous is untrusted, never replacement authority

Before using any previous record, validate its same immutable Epoch, nonregressing
equal MONOTONIC/BOOTTIME/wall deltas from that Epoch, RX/TX>=Epoch, possible catch
transition from Epoch and cap<=original natural remaining. Previous must meet the
base150s reserve too; no past selected-cost claim is invented. Then current must
check clocks/counters/catch against **both** original and previous, plus original
natural cap and prior cap-minus-elapsed. Original non-ARMED can never revive or
change, even BOOKKEEPING_ONLY. Original ARMED may become CONSUMED or UNARMED only
for bookkeeping; after either state, no change/rearm. REQUIRED still defaults to
armed; strict int-not-bool/signed63/bounded cost/150s+cost and immutable tuple stay.
No synthetic observations/traces/tests executed. Arithmetic is not authenticity,
current sampling, a ready flag, renewed lease or hardware-time calibration.

## R2 — admitted exceptional exits get independent read-only owner postchecks

Split owner-side read-only comparisons from private ledger integrity. A failed
ledger/partial write/fsync must not prevent attempted available root/historical/
source checks. Each root, each acquired historical description and each retained
source slot is attempted independently, bounded by fixed counts; operation-dir
association is also checked if acquired. One failure does not skip other available
categories. Reports use fixed category/status/code fields only; no fabricated
PASS for unacquired or unpinned descriptions/partial root acquisition.

- `_call`: busy/closed/terminal refusal stays before admission with no new IO.
  Once admitted, exception from entry/body/normal-exit checks preserves its primary
  first failure, attempts available owner checks before rethrow, and retains their
  distinct secondary outcomes in bounded memory/private ledger when safe.
- Constructor: before relevant descriptors exist, mark categories unavailable,
  do not probe paths to fill context. After partial acquisition, attempt only
  acquired descriptions/anchors before host teardown. Never establish a new raw
  baseline, Git boundary pin or ready state during failure reconciliation.
- `close`: independently attempt available owner postchecks after its entry path,
  whether entry/body failed or succeeded, before releasing host descriptions.
  An earlier failed operation or failed context exit cannot be overridden by a
  prior C1-local-complete record/flag. No post-close FD-number retry or invented
  description retention. Cleanup errors remain fixed non-OK.
- First primary failure is sticky and separate from postcheck category failures.
  Known builtin original diagnostics may be formatted only after safe private
  ledger verification. A safe ledger may append FIRST_FAILURE or later distinct
  close/postcheck evidence; an unknown/partial write or fsync poisons every further
  append, including diagnostics. No write/fsync retry or cleared marker.
- Postchecks are source-scoped observations immediately after the admitted failed
  work, not atomic immunity across later diagnostic bookkeeping/teardown. No
  unbounded reconciliation loop, lock reacquisition or original marker write.

## Parent-selected path roles — narrow source amendment, not bypass

Only historical control root is the **constant**
`/home/gadorach/Desktop/AnykaOpenCam/work/qc-uart-live`, from immutable
`scripts/qc07f_boot.py:23–25` (source pin retained). Remove caller-selected control
root argument. No alternate root, compatibility symlink, worktree, copied marker,
migration, `ignore_git` Boolean or source discovery/import. Anchored read-only
directory traversal/NOFOLLOW, existing owned0700 selected root and ancestor
uid/mode/path-to-FD identity checks stay. Exactly at the canonical workspace,
require the one expected `.git` boundary by read-only metadata observation and
pin/recheck it; no symlink/unsafe owner/mode. Unexpected Git marker anywhere else
refuses. Failure postchecks cannot first establish/rebase this pin.

Private operation root is separately caller-selected existing owned0700 canonical
root, strictly outside every Git worktree, with **no** historical exception. It
cannot be the same/inode alias of control. All new directories/marker/ledger/raw
pins/metadata/errors go only there. Original fixed radio/reset files remain
RDONLY/NOFOLLOW/CLOEXEC/NONBLOCK, EX|NB flock in radio-then-reset order; no
create/write/truncate/chmod/clear/move/unlink/repair/replace. Both roles rechecked
before teardown, including failed operations. Constants/metadata observations do
not prove actual current root association; same-UID namespace/privileged mount
exclusion remains a future serialized caller prerequisite, not a new guarantee.

## Unchanged limits and delivery evidence

Keep no CLI/main/import-time IO or external effect callbacks. Existing two source
FD slots only, regular read-only<=64KiB in<=4096B chunks; source/profile constants
remain caller-sealed evidence bookkeeping, not loaded-code authentication. Keep
historical raw4096B each/new marker24KiB/ledger64KiB,64 records/4096B each; new
postcheck records must fit those same caps. Strict safe fixed public codes,
bounded private diagnostics, hash chain and permanent IN_FLIGHT_UNKNOWN/HOLD,
O_EXCL fresh outputs, fsync, no overwrite/delete/resume/adoption/retry. All owned
host FD slots clear before one close; no target FD/DMA or external lifetime claim.

Deliver source/README, exact before/after frozen input pins, selected source diff,
author static changed-callsite/closure inspection, final HOST syntax source pin,
and method/output seals. Contextual parent document drift recorded, not rebased.
No runtime/lock/durability/memory-fit or independent acceptance. Historical12:12UTC
held U-Boot/consumed catch/both failures remain unchanged; no current owner,
expiry/manual-OFF readiness, native/test/worker/physical next queue is inherited.
