# C1 custody v2 — unreviewed R1/R2/path-role SOURCE candidate

Fresh derivative of frozen v1 `../encoder-query77-custody/custody.py`,34669B/SHA
`7dc6d11ff7dee758f49b3b0e4f7e43fb8b45215cda3761a75732519079e12b2e`.
Retains its GPL-2.0-only notice and bounded conditional host-local API. V1 and
its CORRECTIONS_REQUIRED_R1_R2 independent review remain frozen/unaccepted.
Read `docs/qualification-plans/encoder-query77-custody-v2-20261022/V2_CORRECTION_CONTRACT.md`
(written before code), static inspection/delta/syntax and method seals there.

This is not a CLI, controller, authority plugin, current-state collector, native
receipt or filesystem runtime qualification. No import/library/test/generator or
private IO occurs during this source delivery. Standard declaration decorators
only on import; explicit future library calls can acquire host files/dirs, never
target FDs, devices, processes, UART/RPC/network, boot/query or recovery effects.

## Exactly selected corrections

- **R1:** validate supplied previous record against original Epoch before using
  it. Current clocks/counters/catch must satisfy both anchors, original natural
  cap and previous decayed cap. Initial non-ARMED never revives/changes, even
  bookkeeping. REQUIRED remains default; strict ints, exact clock deltas and
  150s+cost reserve remain. Records are untrusted, not authentic current facts.
- **R2:** admitted failed operations attempt available read-only roots, each
  historical description, operation-dir association and each source slot without
  depending on private ledger integrity. Bounded distinct outcomes retained;
  first primary failure is never replaced. Constructor partial acquisition and
  close have explicit available-postcheck paths. Unavailable/unpinned/partial
  categories are not PASS. Refused busy/terminal/closed calls perform no new IO.
  Unknown-write poison forbids any error append/retry/fsync retry.
- **Path roles:** `Custody.open(private_root, operation_name, profile, epoch)` has
  **no caller-selected control-root argument**. Only exact historical
  `/home/gadorach/Desktop/AnykaOpenCam/work/qc-uart-live`, derived from pinned
  original Boot23–25, with exactly one pinned read-only Git boundary at the
  canonical workspace. No alternate/worktree/symlink root, copied markers,
  migration, ignore-Git flag or original write. New private root retains strict
  no-Git ancestry with no exception and cannot alias control. No actual path/
  marker metadata or current custody follows from these constants.

## Preserved API and evidence limits

Frozen `Profile`, `Epoch`, `Observation`, `Budget`, exact catch/source/local-phase
enums and fixed-safe `C1Error` codes remain. Two source slots LIBRARY/PROFILE only;
borrowed O_RDONLY regular host descriptors, <=64KiB hashed in<=4096B preads, owned
CLOEXEC duplicates in preallocated slots; caller keeps original FD ownership.
Future caller must bind these FDs to actual reviewed source/paths/executing code.
No source-FD pathname discovery or source/profile digest equals authorization.

`observe`, `begin_local`, `finish_local`, `complete_local` record only C1-local
bookkeeping. Opaque external digest/flags cannot grant E2/native PASS. A local
completion followed by failed exit/close remains failed. Permanent new marker is
IN_FLIGHT_UNKNOWN/HOLD and is never cleared. Existing operation dir is refused,
never adopted/resumed/repaired. No original marker create/write/chmod/truncate/
delete/move/replace/clear, no overwrite, no write/close/diagnostic retry.

Historical raw<=4096B each, new marker<=24KiB, ledger<=64KiB/64 records/4096B each;
bounded sequence/predecessor/hash chain and fixed-category exit evidence within
the same caps. Safe ledger alone may retain bounded builtin diagnostics and
separate postcheck outcomes; unsafe or poisoned ledger gets no further write.
No public raw/private hash/path/error export or original exception chain display.

Existing owned0700 selected roots; anchored NOFOLLOW/CLOEXEC directory traversal,
uid/mode and inode/path association checks; historical locks RDONLY/NONBLOCK,
EX|NB radio then reset. Exactly the named workspace Git boundary must exist and
be safe/stable; other Git markers refuse. Acquisition may initially pin that
boundary, failed-exit checks may not rebase/establish it. New private outputs are
always outside every Git worktree. Root directory timestamps/link counts change
on own mkdir and are not immutable; regular-file metadata and Git-boundary
metadata are pinned. No chmod repair for restrictive umask/mode mismatch.

Each owned host FD slot is cleared before one close; no retry or assertion that
an uncertain close retained the description. No destructor/external process
retainer, target FD/DMA retirement, signal/thread/interprocess/async-allocation
atomicity, privileged-mount or same-UID namespace race immunity. Parent must keep
serialized descriptor/source/namespace lifetime and independently admit actual
facts. O_NOFOLLOW is not an atomic regular-file-only-open defense to concurrent
replacement. Exact clock records have no admitted sampler/tolerance/calibration.
Python/stdlib total memory and native128KiB/host4MiB fit remain unproved; no cap
increase. Cleanup after ledger closure cannot safely persist a new diagnostic:
fixed non-OK and permanent HOLD remain authoritative.

No tests or actual marker/private-root/capture/current-state access, no Git/index/
central-plan/helper/safeguard edits. Independent review remains necessary. No
current owner/catch/manual readiness, native acceptance or automatic next queue.
