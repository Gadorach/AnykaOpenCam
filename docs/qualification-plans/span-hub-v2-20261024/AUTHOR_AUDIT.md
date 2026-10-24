# R1 pre-build ordinary source audit (author, not independent review)

2026-10-24. Read frozen V1 C/header/CONTRACT/HANDOFF, full independent review,
actual accepted packetizer C/header and PARENT_DISPOSITION, current plan/index/
execution context and relevant CAP/STREAM plans. CONTRACT preceded v2 C/header.
No production/helper/test import or call is part of this audit.

## Minimum identity correction

- Acquire retains every prior validation and adds only issued-token exhaustion
  refusal before creating or committing output. The increment expression is
  bounded by `issued_token != UINT64_MAX`, therefore produces 1..UINT64_MAX
  from a true-fresh zero counter without overflow. Fixed-view construction has
  no callback/refusal; only success commits issued_token, active_token[c], the
  acquired bit and output. There is no other issued-token assignment.
- Each successful acquisition receives a globally distinct token within this
  stable one-use owner. Generation still identifies a publication, not a lease.
  The canonical active_token[c] is set only for that acquiring consumer. View
  validation now checks both consumer tag and that canonical slot, preserving
  owner/phase/subscription/acquired/nonzero-generation/all-metadata/all-span
  checks. Scalar release checks that slot independently of view validation;
  it clears the canonical slot before clearing acquired/pending bits.
- Ordinary R1 reasoning only, NOT an executed case: if consumers0 and1 hold one
  publication, their tokens differ because the counter advances at each acquire.
  Consumer0's unchanged view fails consumer1's tag and canonical-token checks;
  its unchanged scalar token cannot equal active_token[1]. Releasing consumer0
  clears only slot0/bits0, and leaves consumer1's token/backing held. The packet
  wrapper uses the corrected validator, not an independent weaker guard.
- Exact release remains accepted only once. A cleared zero canonical slot cannot
  match any nonzero old token. Unsubscribe refuses held consumers; unsubscribe/
  resubscribe cannot cause reuse because issued_token is never reset. Publication
  and generation advancement do not reset it either. Neither exhaustion path
  mutates its output/state/masks: publication's unchanged generation check is
  before drop/copy; acquisition's new counter check is before token/mask/output.

## STOP, disposal and preserved boundaries

Initially zero canonical slots correspond to zero acquired bits. Acquire sets
one slot and its bit together under the existing serialization contract; release
clears its slot before its bit. No other token-slot writer exists. Publication
can assign acquired=0 only after refusing all acquired bits; STOP keeps acquired
unchanged; backlog/resync preserve held bits; held unsubscribe refuses. Therefore
the old bit-based STOPPED/drained disposal also implies zero active tokens.
INIT_FAILED never acquired/published, so its slots/counter remain fresh-zero.
No disposal code change or new permission boolean is needed. Disposal preserves
the issued counter; post-disposal calls/reinit remain forbidden, not a new epoch.
STOP preserves held tokens and metadata, denies new effects/acquires, and permits
old exact releases because consumer_valid admits STOPPED and release has no
ACTIVE-only check. This is source reasoning, not runtime retirement evidence.

The byte comparison recorded in SOURCE_DELTA.json checks the entire C prefix
before acquire and suffix starting at packet wrapper against V1: allocation,
input validation, copy, publication, packet wrapper, STOP/drain/dispose remain
byte-identical. Only acquire, scalar release and lease validation C bodies change
(7 added/4 removed lines). Header is 16 added/4 removed lines: small fields and
accurate contract/replacement comments. Forward and inverse unified diffs were
applied to text in memory and each exactly reproduced the opposite original;
no original file was patched. GPL provenance is retained. No V1 harness/runner
is copied, compiled or used.

Counter tokens are owner-local identity, not security capabilities/randomness or
arbitrary-pointer defense. Existing valid stable disjoint objects, self identity,
true-fresh one-use owner, caller serialization/no aliases/moves/reentry/signals/
concurrency/fork remain required. V1 names/guard are retained for exclusive
replacement only: owner/view layouts changed and are not V1 binary ABI. Never
include both headers, link both hubs, or reuse a V1 combined REL.

## Fixed compile-only recipe audit

Fresh build01 mkdir refuses an existing destination; files/logs use exclusive
creation. compile01.py calls only pinned compiler metadata/compile commands,
retained direct ld -r, readelf and nm. It does not call production code, load any
candidate as Python, build main/an executable or read any device/private input.
Four C compiles are v2 hub plus unchanged accepted packetizer for HOST and musl
ARMv5TE soft float. Two RELs combine exactly those real objects; no old REL/stub/
runtime source/harness. Twenty fixed tool commands are expected. Every command
records argv/cwd/explicit environment/start/end/returncode/log hash, and failure
stops without retry. Compiler temporaries are scoped under the fresh build tree.

Selected input hashes, consumed dependency hashes, tool hashes matched against
retained V1 tool records, logs and output artifacts are recorded. Tools are
rechecked at the end; source inputs must remain unchanged. This is not a complete
compiler closure, executable link, native ABI/loadability, transitive stack or
whole-memory proof. No functional/negative/wrong-consumer/adversarial/refusal/
mutation/fuzz campaign or old normal HOST rerun is selected. No camera operation,
Git/index/central-plan edit, subworker or hardware queue is selected. Separate
parent independent ordinary review is required before source adoption.
