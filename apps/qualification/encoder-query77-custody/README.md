# C1 query77 host custody — unreviewed SOURCE candidate

This directory contains one standalone library, `custody.py` (GPL-2.0-only).
It is **not** a controller, CLI, executable recipe, E2 gate or native authority.
No `__init__.py`, main, sample launcher, test or package bootstrap is supplied.
Import declares types/functions only; explicitly selected future host-library
calls can perform the host-local operations described below. Nothing was imported
or called to qualify this candidate: AST/syntax compilation only, no bytecode file.

Read the pre-code contract at
`docs/qualification-plans/encoder-query77-custody-20261022/C1_OWNERSHIP_CONTRACT.md`
and the source/control-flow inspection in that directory. Independent review is
still required. Frozen controller design is unreviewed input, not accepted proof.

## Minimal API, no live capability

| API | Conditional host-local meaning |
|---|---|
| `Profile` | Frozen strict source/profile digest constants and exact design/image/recorder/release pins. Caller must seal actual reviewed constants; hashing an arbitrary file is not review or executed-code identity. |
| `Epoch`, `Observation`, `Budget` | Frozen, strict-int/nonbool, bounded fields. Epoch is immutable supplied capture/peer/start/adapter/raw/boot/catch/source tuple, including an explicit required initial Catch enum, never an invented ARMED default. Observations are untrusted records, not authenticated current facts. No field is publicly repr-formatted. |
| `validate_observation` | Pure equality/nonregression/catch/clock/budget arithmetic only. Returns constrained numeric bookkeeping, not ready/GO. BOOKKEEPING_ONLY allows recording consumed/unarmed catch but no revival; every guard operation using this validator requires ARMED. |
| `Custody.open` | Explicit future acquisition of two existing safe roots, exact read-only locked historical marker names, then fresh exclusive private operation dir/marker/ledger. No old guard/boot imports. No native effect, target FD, current-state getter or process launch. |
| `verify_source_fd` | Only two fixed slots, LIBRARY and PROFILE. Borrow an explicit start-position O_RDONLY regular single-link host FD; validate/hash<=64KiB in4096B chunks, duplicate with CLOEXEC and retain private duplicate for repeated checks. Caller owns original; C1 never closes it. No path lookup or image/payload input here. |
| `observe` | Validate supplied observation against original and last, append privately bound clock/counter/catch fields plus opaque external receipt digest. No content/RX classifier or evidence authenticity proof. |
| `begin_local`, `finish_local` | Only SOURCE_PINS and EPOCH_RECORDS intents/completions, unique matching receipt digest. SOURCE_PINS completion requires both verified descriptors; EPOCH_RECORDS requires at least one constrained observation. These are C1 bookkeeping phases, not native phase receipts. |
| `complete_local` | Fresh supplied constrained observation, both local phases complete, no pending intent/failure; append C1_LOCAL_COMPLETE_NOT_NATIVE. Does not clear permanent initial HOLD marker or authorize an effect. |
| `close` | One-shot host teardown, including failed/completed objects. Calls before local completion report terminal HOLD. Teardown clears ownership slots before one close, no retries or numeric retention claims. After close even a repeated close is refused. |

Only `C1Error` with a fixed `Code` is emitted by effectful public calls; original
exception chaining is suppressed. Dataclass/pure API guards also use explicit
fixed-code exceptions, never assertions. There is no public dump/status/raw/error
exporter. Names prefixed `_` are private implementation, not callable authority
interfaces. Python introspection/monkeypatching/forged objects are not an isolation
boundary; caller serialization and independent source review are mandatory.

## Actual limits and remaining concrete blocks

- Linux directory-FD/O_NOFOLLOW/CLOEXEC and dir-fsync semantics required; unavailable
  secure operations refuse. Two caller-selected existing roots only; no broad mkdir.
  Paths are absolute ASCII components, max1024B/32 components; no slash aliases,
  `.`/`..`, symlinks or `.git` anywhere on the walked ancestors. Root/ancestor
  ownership/permissions and inode associations are repeatedly checked.
- Both selected roots must be outside Git and owned0700. **The historical Boot
  CONTROL location under this Git workspace cannot pass.** No relocation, copy,
  replacement marker, ignore-Git switch or exemption is supplied. Parent must
  resolve this conflicting operational prerequisite separately; source API has
  not proved custody of the real historical markers at any current location.
- Historical names only, fixed radio-then-reset LOCK_EX|LOCK_NB; regular owned0600,
  single-link, raw JSON<=4096B each. Duplicate/extra/malformed keys rejected; exact
  state/operation semantics and privately pinned bytes/metadata checked repeatedly.
  Original descriptions never receive write/create/chmod/truncate/unlink calls.
- Directories' inode/type/owner/mode associations are pinned; their timestamps/link
  counts are not frozen because exclusive creation changes them. All regular
  markers/ledger/source metadata includes size, link count, mode, uid/gid, inode,
  mtime/ctime (atime excluded because reads may change it). Root selected mode must
  be exactly0700; restrictive umask causing different new-file mode refuses rather
  than repairing permissions. No chmod is used, even for newly created files.
- Parent must exclude same-UID namespace replacement, privileged mount/bind/
  namespace changes and concurrent borrowed-FD mutation. O_NOFOLLOW plus advisory
  flock is not atomic regular-file-only open against an uncooperating namespace.
  The source prechecks regular metadata, opens NONBLOCK/NOFOLLOW then compares FD
  identity, but cannot claim no special-file open under a concurrent substitution.
  No interprocess/thread/signal/async-allocation-failure safety proof is offered.
  Same-object reentry is refused where observed, not a general concurrency lock.
- Source FDs are independently bound by a future caller to actual public paths and
  executing source; C1 cannot discover pathname/provenance from an FD and offers
  no `/proc` fallback. Duplicates share offset; C1 only preads and checks offset0;
  later caller offset mutation is a refusal. Repeated hashes do not prove whole
  source closure, no loaded-code mismatch or privileged modification immunity.
- Exact equal MONOTONIC/BOOTTIME/wall deltas required relative to original and
  previous records; clock divergence/regression refuses. No real sampler, clock
  tolerance, PID-start lookup, catch arm or hardware timer calibration is supplied.
  This strict record rule is not proof a future live sampler can satisfy it.
  Epoch/records may be fabricated by a caller; C1 is explicitly not an auth plugin.
- Original expiry/lifetime budget cannot renew: later cap<=original natural
  remaining and prior cap minus elapsed, reserves30s+120s+selected cost. Snapshot
  equality never establishes current source/owner, catch or recovery. U-Boot
  epoch has no Linux UUID; same Epoch cannot rebase into Linux or new boot/catch.
- New operation directory uses exclusive mkdir0700, file O_EXCL/APPEND0600; fsync
  files and parent dirs. No overwrites/deletes/truncation/resume/reader adoption.
  Permanent new marker is IN_FLIGHT_UNKNOWN/HOLD, with bounded original private
  pins/metadata and supplied original tuple. No "normal" result clears it.
- New marker<=24KiB; ledger<=64KiB/64 records, each<=4096B. Sequence/predecessor/
  canonical SHA256 chain, pinned expected file metadata and in-memory tail are
  checked streaming. A complete append gets one write/fsync and dir fsync. Short
  write/unknown result makes further writes impossible; no error-append retry.
  Partial constructor files are retained, never cleaned up/deleted. A safe ledger
  can record first failure and a bounded256-character original diagnostic for
  exact known builtin exception types only; no custom __str__, raw traceback,
  public error text or private digest export. Unsafe/unknown ledger => marker HOLD.
- Original marker locks remain until explicit teardown. A failed close clears the
  numeric slot and reports CLEANUP; it does not claim the description survived.
  Cleanup failure after the ledger FD closes cannot be persisted there: object
  failure/raised fixed code is non-OK, permanent marker still HOLD. Future external
  lifetime/observer must retain that outcome; no destructor or process retainer.
- Bounded buffers are not total Python/stdlib memory proof, native128KiB allocation
  admission or4MiB host process fit. No ceiling was raised and no image was read by
  the library. This is not watchdog ownership/disable, boot, DMA/media retention,
  volatile transfer, query88, radio/station/module, recovery or product acceptance.

No actual HOLD/root/private file/current state was read, no library imported/called,
and no tests/generators/controller/device/network/UART/RPC/process operation ran.
Only static public reads/hashes and in-memory HOST AST/syntax compilation were
selected. No Git/index/source-helper/central-plan/safeguard change. No next queue.
