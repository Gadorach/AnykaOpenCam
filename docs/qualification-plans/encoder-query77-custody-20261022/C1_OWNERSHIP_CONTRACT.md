# C1 ownership contract — before implementation

2026-10-22; ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Selected by parent for
**unreviewed SOURCE implementation only**, not C2, E2, native or manual action.
Frozen input: `../encoder-query77-controller-design-20261022/` (unreviewed design,
not an accepted safety proof). Only the two new custody directories are writable.

## Scope and explicit limitation

One Python library, no CLI/main/package bootstrap. Imports define types/constants/
functions only; no input/output, object acquisition, process, signal, current-state
getter, controller, callback, target effect or authority plugin. Future explicit
library use may acquire **host regular files/directories only**. This delivery
does not call that library, import/execute it, or exercise a private filesystem.
Only AST parsing and in-memory syntax compilation of the new text are selected.

Both caller-selected existing roots must be canonical absolute Linux paths walked
from `/` with directory descriptors, NOFOLLOW/CLOEXEC and no writable/untrusted
ancestors, no `.git` at any traversed ancestor, selected roots owned0700. Thus the
historical source `CONTROL` under this Git workspace is **refused**. No migration,
symlink exception or in-worktree exemption is authorized here. Resolving that
actual-location conflict needs a parent contract decision; no current location was
read and conditional source custody is not operationally usable on that old root.
`.git` checks are anchored marker checks, not immunity to privileged mount/namespace
changes or malicious noncooperating peers. Such changes are outside the explicitly
serialized host contract; changed observed associations fail closed.

## Proposed ownership/API before code

- `Profile`, `Epoch`, `Observation` are frozen predeclared evidence-shaped records,
  strict bounded integers (not bool), restricted strings/enums and exact selected
  public image/source/profile pins. No dataclass repr leaks their private fields.
  Record possession is **not** actual capture/owner/source authenticity or E2.
- Pure `validate_observation` compares the immutable initial tuple and previous
  observation: capture/peer/PID-start/adapter/raw identity/boot/release/source pins
  never change; RX/TX only advance, catch identity cannot rearm/rebase, unarmed or
  consumed catch refuses a request requiring catch. Original MONOTONIC/BOOTTIME/
  absolute expiry and reserves30+120+selected cost bound a remaining budget that
  only shrinks. Suspend/clock divergence/regression refuses; no measured timer or
  process lifetime is inferred. C1 accepts only a supplied already-armed epoch;
  creating a fresh recovery catch is outside C1, never a flag-changing API.
- `Custody.open(control_root, private_root, operation_name, profile, epoch)` uses
  explicit arguments, no callbacks. Existing roots only. Fixed historical names
  `peripheral-gui-operation.lock` then
  `peripheral-unattended22b-operation.lock`: O_RDONLY/NOFOLLOW/CLOEXEC/NONBLOCK,
  nonblocking exclusive flock in that order; no create/write/chmod/truncate.
  Require regular current-uid single-link0600 files, stable descriptor/path/stat
  and bounded raw JSON. Reject malformed/duplicate/extra keys and any semantics
  other than the exact two historical HOLD operations from frozen design.
- Guard owns and retains directory descriptors and both locked marker descriptions;
  each public operation rewalks roots and compares dev/inode plus stable metadata,
  checks `.git` absence and reads/compares original raw bytes before and after.
  Raw bounded bytes and their digests remain internal/private; no public snapshot
  exporter. Advisory locks do not exclude noncooperating writes. Reentry on the
  same object is rejected; no general thread/signal/interprocess safety promise.
- Under the existing safe private root, exclusively create one fresh operation
  directory0700 and marker/ledger0600 single-link NOFOLLOW/O_EXCL. fsync marker,
  ledger and operation/parent directories; marker initially and permanently
  `IN_FLIGHT_UNKNOWN/HOLD`. Raw historical pins/metadata are written only inside
  that owned private marker. A failure never deletes/replaces prior files.
  Directory uniqueness uses mkdir's exclusive existence check (Linux directories
  cannot be created by open O_EXCL); association rechecked after opening it.
- Ledger uses a single O_APPEND description, bounded canonical records, sequence,
  predecessor hash and hash chain. No truncation/removal/repair/reopen/resume,
  retry of uncertain writes/fsync or adoption of an existing operation directory.
  Partial/unknown append makes the object terminal HOLD; initial durable marker
  is authoritative regardless of later completion claims. Keep all previous files.
- `begin_local(phase, external_receipt_digest)`, `finish_local(...)` record only
  fixed C1 source-pins/epoch-bookkeeping phases. Digest is opaque external receipt
  bookkeeping, never verification/native PASS. No boolean grants continuation.
  `observe` updates only monotonic constrained supplied-record bookkeeping.
  `verify_source_fd` reads only an explicit borrowed regular read-only FD from its
  current start offset, capped64KiB, via bounded pread/hash; no caller path opens,
  full image, payload execution or image-size ceiling increase. Expected digests
  are sealed in Profile; exact final public profile/source pin selection stays
  caller responsibility, not a trust plugin. Two fixed source slots must verify
  before `complete_local`; no arbitrary verification list or huge input buffers.
- `complete_local` requires both local phases completed, source pins verified,
  no pending phase/fault, fresh dual-HOLD/source/clock bookkeeping checks; writes
  `C1_LOCAL_COMPLETE_NOT_NATIVE`. Initial marker remains HOLD, not cleared. This
  completes only local bookkeeping, supplies no actual hardware evidence and does
  not permit a next effect. Failed/closed/completed objects are one-shot terminal.
- Fixed safe `C1Error` codes only; never interpolate path/raw/hash/OS error. Suppress
  original exception chains. Private ledger, once safely owned, may record a fixed
  first-failure code, not unsafe traceback/exception data. First failure sticky;
  an unknown write cannot be followed by a retry/error append. C1 has no reader
  that adopts/resumes state; ledger integrity is checked while guard alive.
- `close` clears each owned slot before exactly one close; never retries an FD
  number or claims retention after close error. Close host files/dirs only,
  releasing locks only at teardown. Constructor failures retain filesystem files
  and release acquired host FDs. Cleanup errors produce fixed sticky non-OK.
  Future C2 must keep the guard/host lifetime externally owned across every effect;
  C1 does not retain a target FD, uncertain DMA, child or independent process.

## Resource/evidence boundary

No allocation-cap exception: historical marker<=4096B each, source input<=65536B,
hash chunks<=4096B, ledger record<=4096B, max64 records/65536B file; streaming
integrity checking, not whole ledger/image copies. Budgets nanosecond integers
bounded to signed63-bit; no bool/int coercion or arithmetic wrap. Host Python/
stdlib total memory is not proved under4MiB, and these caps do not prove target
128KiB/native fit. No library calls/tests/fuzz/mutation/runtime or private IO.

Preserve original two HOLDs, frozen design and all failed evidence. No boot parser,
TX allowlist/shell transfer/UAPI/provider/retention integration. Future caller
must supply separately validated current owner/capture/clock/source relationships;
record equality cannot manufacture those facts. Independent review remains
required before even source acceptance. No next worker/native/test queue.
