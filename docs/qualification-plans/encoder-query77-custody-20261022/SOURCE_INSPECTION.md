# C1 source/callsite inspection — author, not independent acceptance

2026-10-22. One new `apps/qualification/encoder-query77-custody/custody.py` library;
pre-code contract written first. This report is static author inspection only;
not path/locking/fsync/clock/cleanup runtime proof. No candidate import/call/test,
private IO, current-state access, process, target, Git/index or helper mutation.

## Control-flow inventory

| Source symbol | Inspected property / remaining condition |
|---|---|
| `Code`, `C1Error`, `_refuse`, `_code` | Fixed safe codes; effectful public boundaries convert OS/parse errors, suppress cause display. No interpolation of paths/raw/private digests. Private helpers are not public bypass APIs. No traceback safety claim against Python introspection. |
| Frozen `Profile` | Strict public source/profile hex digests, exact frozen design/image/recorder/release constants. No loaded-code/image/source authenticity inferred. Constructors have no IO; supplied values cannot authorize anything. |
| Frozen `Epoch`, `Observation`, `Budget` | Explicit field schemas; type(x) is int excludes bool; bounded signed63 clocks/counters, bounded PID/uid/gid/string schema, strict enum types; no private-field repr. No unstructured readiness dictionary or callable hook. |
| `validate_observation` | Exact same original Profile/Epoch, immutable peer/start/adapter/raw/boot/catch identity; counters nonregressing. Clock deltas must agree exactly from original and previous; previous lowered deadline decays by elapsed. Min of absolute expiry/lifetime cannot renew. Required catch rejects consumed/unarmed; pure BOOKKEEPING_ONLY cannot rearm or mutate original. No OS clock/PID lookup. |
| `_canonical`, `_Root.acquire/check`, `_git_absent` | Linux rooted descriptor traversal, canonical ASCII path, fixed component/count bounds, NOFOLLOW/CLOEXEC/DIRECTORY/NONBLOCK, ancestor owner/currentuid-or-root and no022, selected0700, `.git` absence per descriptor. Retained parent descriptors/path entry compare; root inode association rechecked. Directory ctime/link count intentionally not pinned as immutable across our mkdir. |
| `Custody.open` | Validates supplied profile/epoch arithmetic before file acquisition; existing roots only. Precheck regular historical path metadata, open radio then reset RDONLY/NOFOLLOW/CLOEXEC/NONBLOCK; retain slot then LOCK_EX|LOCK_NB. Exact bounded JSON, duplicate-key rejection, raw bytes and metadata checks. No old import, CLI, socket/serial/boot recipe or live getter. |
| `_check_holds` | Regular owned single-link0600, stat/path-to-FD full metadata match, exact raw read then fstat/path recheck. Both guards held alive across every public operation. These observed checks do not prevent noncooperating writes or concurrent device substitution. |
| Constructor new-file branch | Fresh operation mkdir0700 and retained dir FD, identity check/fsync parent. Only new marker/ledger open O_RDWR|APPEND|CREAT|EXCL|NOFOLLOW|CLOEXEC|NONBLOCK0600. Permanent marker binds private raw original pins, original supplied epoch and source/image/profile pins. Files/op/parent fsynced before return. Any partial failure preserves files; no deletion. |
| `_private_file`, `_check_private` | Directory path identity, exact modes/single-link; marker raw+full metadata immutable; ledger expected size/full metadata, bounded streaming line parse, exact keys/strict observation schema, SHA chain/sequence and in-memory tail, post-read metadata/path check. No adoption/resume/read-from-old-dir entrypoint. |
| `_source_hash`, `verify_source_fd` | Only borrowed regular O_RDONLY source descriptors, <=64KiB, zero offset,4096B pread chunks/stable metadata/digest. Two fixed slots. Duplicate CLOEXEC after initial checks; repeated hash inside every guard boundary. No source path discovery/execution, full image copy or arbitrary private payload reader. Caller source-to-running-code relation remains unproved. |
| `_write_once`, `_append` | Only callsites are new marker once and owned new ledger appends. Poison set before write; no partial-write retries or fsync retries. Canonical sequence/prev/hash; max4096B record/65536B ledger/64 records. Per-success expected metadata updated for known own append; dir-fsync/readback failure terminal. |
| `_failure` | First fixed failure sticky, never overwrites original HOLD. Formats bounded known builtin error detail only after private ledger ownership/integrity check. Unknown/partial write skips diagnostic append entirely. A failed diagnostic write poisons, never retries. No user-callback __str__/repr. |
| `_call` | Same-object busy/terminal refusal, all-roots/HOLD/private/source checks before and after each operation; error -> sticky failure and safe exception. Internal contextmanager only, no caller effect callback. No general thread/signal safety claim. |
| `observe`, local intent/completion APIs | Evidence-shaped clock/counter records and external receipt digest bookkeeping only. No native phase ID/PASS/GO. Fixed SOURCE_PINS/EPOCH_RECORDS, single pending intent, matching digest, real local source/record preconditions. No caller True accepted as completion. |
| `complete_local` | Both local phases and source slots plus constrained new supplied observation, no pending/failure; C1_LOCAL_COMPLETE_NOT_NATIVE only. No original/new marker clearance. Crash at any point still has permanent HOLD; ledger alone never grants continuation. |
| `_teardown`, `_Root.close`, `close` | Pop/clear each owned host slot before one os.close, continue remaining teardown on close error, never retry former number or claim retained FD. Constructor failure tears down host descriptions, leaves files. Terminal completed/failed object only close allowed; second close refuses. No implicit destructor/target close/child kill. |

## Exhaustive external effect callsite classes (source, not executed)

- `os.open`: `/` and relative directory components in `_Root.acquire`; fixed two
  historical names RDONLY only; fresh operation dir; fixed new marker/ledger names.
  No caller image/payload path, source path, proc/dev path or arbitrary payload IO.
- `os.stat/fstat/pread/lseek(FD,0,SEEK_CUR)`: descriptor/path association and bounded
  regular-file reads/position query only. `.git` existence includes symlink markers.
- `os.mkdir`: one supplied restricted fresh basename under validated private root.
- `fcntl.flock`: historical descriptions only, radio then reset, EX|NB.
  `fcntl(F_GETFL/F_DUPFD_CLOEXEC)`: explicit regular read-only source FDs only.
- `os.write`: `_write_once` only, reached from new permanent marker or new ledger.
  No callsite accepts a historical description. No chmod/truncate/unlink/remove/
  rename, chmod repair, lock-file creation for originals, resume or clear.
- `os.fsync`: newly created files and validated owned private/op directories only;
  never original marker content/metadata. No OS clock getter or deadline renewal.
- `os.close`: host source duplicates, read-only historical descriptions and created/
  retained host marker/ledger/directory descriptions only; clear-before-close.
- `os.getuid`: explicit constructor only. No top-level IO calls, source self-read,
  socket/network/UART/RPC/subprocess/signal/process/PID/status/device getter, exec/
  eval, importlib, CLI/default invocation, callback-based effect or test entrypoint.

## Honest unresolved source/operational conditions

1. Both roots outside Git means actual historical `CONTROL` workspace path is
   rejected. Do not migrate/copy/fake markers or bypass `.git`; parent contract
   resolution is required before any operational application. No current markers
   were inspected; this candidate has not proved it locks genuine current markers.
2. O_NOFOLLOW is not an atomic regular-file-only open against same-uid namespace
   mutation. Caller must serialize namespace/descriptor lifetime and exclude
   privileged mount changes; advisory locks do not stop noncooperating writers.
   This implementation refuses observed substitution but cannot prove absence.
3. Explicit supplied record equality is not fresh physical owner/capture/source/
   clock authenticity. Exact equal clock deltas have no admitted actual sampler.
   New Linux boot or catch requires a separate future method, not epoch rebasing.
4. Python exceptions/signals/MemoryError can interrupt acquisition/ownership
   bookkeeping; no async-signal/interprocess/reentry atomicity or4MiB host total
   memory proof is claimed. Explicit serialized lifecycle and external owner remain
   necessary. No descriptor-number retention claim after uncertain close.
5. A failed cleanup after closing the ledger cannot safely append another error;
   fixed non-OK and object first failure remain, permanent marker HOLD survives.
   Filesystem durability semantics and every runtime success/failure path remain
   unexercised. No independent review or source acceptance follows author inspection.

Native/manual/operator action, watchdog staging guard, new boot parser, transfer,
query, lifetime/retention, combined-load/G3/G8 and tests stay outside this packet.
