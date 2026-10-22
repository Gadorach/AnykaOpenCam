# C1 path-role clarification — proposed fresh revision contract

2026-10-22; BASE-01/05/07, ENC-02 / PLAT-01/08. Parent source-design decision only;
no filesystem/private-state access, migration or operational admission. C1 v1 and
its contract/seals stay frozen while independent static review runs.

## Conflict and its cause

The C1 selection required both roots outside Git. Its implementation correctly
refuses the historical root as a consequence. The immutable source
`scripts/qc07f_boot.py:23–25` defines `CONTROL = ROOT / "work/qc-uart-live"`.
Both historical HOLD names are fixed in the original22b guard. Demanding that
those actual files already live outside the workspace is incompatible with this
source association. A copied marker, replacement directory, compatibility symlink
or unobserved relocation cannot repair that conflict or preserve actual custody.

The requirement for **new private outputs** outside every Git worktree remains
unchanged. It does not require relocating pre-existing historical lock files that
must only be opened read-only and locked in place. This distinction revises the
parent's overbroad C1 root contract, not the historical failure or privacy policy.

## Permitted future source revision — two noninterchangeable roles

1. **Historical control root:** only the canonical workspace-relative source-bound
   `work/qc-uart-live`, at the exact canonical workspace used by the pinned original
   Boot source. No alternate caller-selected control directory or marker names,
   compatibility symlink, source-root discovery/import or `.git`-ignore switch.
   A revision must spell out and pin that association in its source contract.
   Descriptor-anchored traversal still refuses symlinks, aliases, changed path/FD
   identities, unsafe ownership/modes and unexpected Git markers. The one expected
   workspace Git boundary is a narrowly named read-only traversal condition, not a
   blanket exemption for arbitrary roots. No claim of current mode/identity exists.
2. **Private operation root:** remains a separate existing owned0700 canonical
   directory outside **every** Git worktree. All new operation directories, sticky
   markers, ledger, raw historical bytes/digests/metadata and errors go only here.
   Never use the control root, workspace or an alias as this destination. Retain
   strict anchored ancestry, no `.git`, identity, ownership and mode checks.

Only the two original fixed files may be opened under the historical root, using
read-only/NOFOLLOW/CLOEXEC/NONBLOCK and fixed-order nonblocking exclusive flock.
Never create, write, truncate, clear, chmod, replace, move, unlink or repair them.
No operation output/socket/recorder file is created there by C1. Directory opens,
stat/read and advisory lock/unlock are the only historical-side actions. A failure
refuses; it does not normalize metadata or obtain custody of a substitute marker.

The revision must check both role associations through teardown, including failure
paths; read-only historical permission must not leak into private-output ancestry
checks or authorize arbitrary source-FD/path opens. Cooperating locks and inode/
byte comparisons remain conditional on the serialized namespace contract, not
immunity to privileged mounts or proof of an actual current camera epoch.

## Selection boundary

This amendment supports a **fresh source candidate only after parent selection**.
It does not edit/adopt/run C1 v1, waive its independent review or assert that any
current root/file passes metadata checks. Any additional ordinary defect found in
v1 must be preserved and separately corrected in fresh source. Syntax compilation
is not filesystem/lock/durability or hardware qualification. No tests, private
root operation, C2 controller, UART/RPC, capture, staging, boot, query, reset or
physical action is selected by this document. Both actual HOLDs remain untouched.
