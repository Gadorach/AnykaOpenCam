# Explicit bootstrap material retirement — author handback, review pending

Assigned20261023 · SEC-03/08/11, CORE-03/04, STREAM-07. Implemented SOURCE
replacement + HOST/ARM compile/REL evidence only. **Independent ordinary review
and explicit parent adoption required before use. Nothing was called.**

## Delivery / actual API

`apps/rtsp-auth/qc07f-bootstrap-material-retire/{bootstrap.c,bootstrap_v1.h}`.
Choose ONLY this translation unit and this header instead of either earlier
bootstrap unit/object/combined REL. Same four previous exports, plus
`qc07f_bootstrap_retire_material_v1(struct qc07f_bootstrap_v1 *)`; same one
process attempt, not two independent contexts. New metadata/result fields and
enum values extend the source layout: NOT old binary/native ABI compatibility.
`QC07F_BOOTSTRAP_INIT` initializes the new historical/borrowed numbers to -1.

- C SHA256: `b89782a3522dd16d201a1dc3b24e64b8ce0a5c2e519af3a89e9ad45c7a054b60`.
- Header SHA256: `aec5a98dd3d3a7cd867ebf93d224c7e049b44d2c8caff8fcf0fd2795c450f529`.
- Design written before implementation: [CONTRACT.md](CONTRACT.md).
- Exact base-to-derivative diffs and parsed inverse proof:
  `bootstrap.c.diff`, `bootstrap_v1.h.diff`, `DELTA_PROOF.json`. Both inverse
  patches recover the specified accepted base hashes exactly. Begin/observe/
  finish function bodies are byte-identical; start adds only post-bind metadata
  capture and the actual-start-attempt record. Helpers and the new explicit API
  are added, original dependencies remain unchanged. No giant source snapshot,
  new main/framework or external cleanup function is supplied.

## Real custody / metadata rationale

Actual provisioner child: real private-handoff create uses exclusive fixed-name
O_EXCL write, fsync and one writer close, then readonly open and actual h move;
sendmsg transfers that FD plus public inode envelope. Child closes sender refs,
exits0; actual parent observe checks receive/known own wait0/known parent closes.
Actual bind rechecks unchanged original config/unused owner/metadata, moves
received[0] to cfg.material_fd and sets BOUND. We never fabricate h or process
state, use a payload read/hash, or add an alternate filename selector.

Immediately AFTER real bind0/BOUND and BEFORE prepare/start, capture actual kernel
directory/file stat and borrowed directory association. Corroborate actual bound
provisioner identities, envelope inode, known wait/close/empty received slots.
Directory root-owned0700/CLOEXEC, same original provisioner dev/inode; material
root:root0600 regular/nlink1/1536, readonly/CLOEXEC, actual received inode equals
fixed no-follow named inode. Snapshot failure latches MATERIAL_SNAPSHOT/HOLD with
original cfg FD/file retained; no prepare/start/cleanup. Historical FD numbers are
NOT additional owning slots. Borrowed directory remains caller-owned in ALL states.

Actual owned-runtime prepare remains device-free; original same-owner start follows.
rv_start's F_DUPFD_CLOEXEC plus fork/dup2 gives child FD4 a SHARED open-file
description; actual child `read(...,1536)` advances the original offset before its
own close. Parent routing-prefix pread does not reset offset. Thus this new module
never checks/resets post-consumption offset. It excludes atime; it compares dev/
inode/type/full mode/UID/GID/nlink/size/mtime/ctime including nanoseconds. No writer
or namespace changes are allowed, so file and directory mtime/ctime/size/nlink
should remain stable until unlink. Metadata mismatch is HOLD, never repair.
Unlink can legitimately change those fields afterward; no pre-unlink immutable
comparison is applied after acknowledged unlink.

Actual prestarted finish -> backend destroy requires rv_stop success + actual child
retired, captures normal wipe and own wait0 before rv_destroy clears child, and
returns error on missing wipe/wait0 for initialized backend. Adapter's IPC close
outcome remains sticky through stop/destroy. No new receipt is fabricated from the
already-cleared child. Outer FINISHED_DEVICE_FREE is still set ONLY by actual
finish return0 + PS_FINISHED; retirement ALSO requires recorded actual start attempt/
success, finish attempt/success, no sticky outer/provisioner error, same real owner/
config/runtime addresses, normal owner start/finish/serve results, no source hold,
backend child/peers/pending/initialized/failed state and known provisioner retirement.
These checks corroborate actual calls; they do NOT prove global native-free state.
The trusted closed API and no earlier native/media ownership ANYWHERE are essential.
Inherited adapter close handling of temporary duplicates/pidfd is unchanged; no new
claim that finish0 certifies every process descriptor or all resources is made.

## Explicit retirement / outcomes

One identity-valid request is consumed even on ineligible state/metadata refusal;
busy or foreign identity does not affect another owner. Repeated requests refuse
without changing retained history. READY/PROVISIONING/START_FAILED/HOLD/unknown/
early prepare failure cannot close/unlink. Stable objects persist, including HOLD.

1. Gate exact normal FINISHED_DEVICE_FREE, then recheck cfg number/flags, captured
   directory and actual FD/fixed-name immutable metadata, with no payload access.
2. Set MATERIAL_RETIRING, clear canonical cfg.material_fd=-1 BEFORE exactly ONE
   close. Record attempted/result/errno/former-not-owned number. Any error/EINTR
   sets close_unknown + sticky HOLD; linked file untouched, no numerical retry.
3. Only after close0, recheck captured quiet directory and exact fixed-name inode.
   Mismatch -> HOLD/known cfg close0, no unlink. Then ONE unlinkat(dirfd,
   RTSP_PRIVATE_HANDOFF_FILE,0); error -> HOLD/unlink outcome uncertain, no fsync.
4. Only after unlink0, ONE directory fsync. Failure -> HOLD with unlink0 recorded
   and directory acknowledgement uncertain. No recreate/rename/retry/restoration.
5. Only close0 + unlink0 + fsync0 -> MATERIAL_RETIRED. This means original cfg FD
   closed and that staging name logically unlinked with directory syscall ACK,
   NOT secure erasure, flash/power-loss durability, rotation, native/whole-process
   release or exit authority. Image FD/root context/borrowed directory/native memory
   are not retired. Original start cannot run again; original config FD remains -1.

All three syscall outcomes have separate attempted/result/errno and uncertainty
fields, distinguishing unattempted default zero from actual zero success. First
error stays sticky. No destructor/atexit/automatic unlink/stop/kill/watchdog or
numeric close/unlink/fsync retry is introduced. Existing failed-start/unknown
ownership lifetimes are unchanged; no unrelated/early-failure file is adopted.

## Mandatory trusted caller conditions

Read full CONTRACT before any future use: stable private once-initialized disjoint
object/config/graph/routes/context; no mutable-handle borrowing/copy/reset/reinit/
scope-exit, aliases/FD reuse, threads/signals/extra forks/competing reapers/reentry.
Caller holds explicit borrowed directory throughout ALL states, and exclusively
owns this generation's namespace/ancestors/fixed name with no other writer or
namespace mutation. Metadata/readonly flags do NOT prove writer exclusion or
hostile-root TOCTOU freedom. No arbitrary private config/permanent credentials/
history/acquisition/other entries or directory removal is selected. Existing real
authorized ingress, secret-free parent, global FD/env/cap hygiene, native image/
hardening and real retain_verifier conditions remain. No permission flag, default
credentials, fixture state or fresh native activation/serve export was added.

## Actual bounded compile evidence, never execution

Audited fixed recipe `compile_recipe.sh`; fresh refuse-overwrite `runs/build01/`.
Actual build UTC **2026-10-24T02:47:38.574965+00:00** to
**2026-10-24T02:47:39.831683+00:00**; assigned20261023 path preserved, no clock
correction/admission implied. `INVOCATION.json`, `runs/build01/{REPORT.json,
COMMANDS.txt,TIMES.txt,DEPENDENCIES.json}` retain exact argv, timestamps, exits,
logs and pins. Counts overlap; not complete dynamic compiler execution closure.

- **2 actual changed-unit C compilations + 2 real integration REL links; 24 tool
  commands PASS**, no compile/link failure. HOST cc -O2, retained musl ARMv5 GCC
  -Os/-march=armv5te/-marm/-mfloat-abi=soft with real headers and warnings-as-errors.
- **916 input paths** matched before/after, **221 new dependency records / 184
  unique resolved paths**, **871 reused addition dependency records**, **378 runtime
  dependency paths**, **37 build artifacts** pinned. `BUILD_AUDIT.json` retains
  source/flag roles and current live/frozen byte matches.
- **46 archive member bodies** equal actual runtime build02 objects; **58 original
  compile argv/source/exit/log associations** checked (46 runtime +12 additions).
  Real production provisioner objects0,1,2,3,9 and actual OS-random object added.
  No old bootstrap object/combined REL, zeroizer addition, stub or fixture used.
- Both RELs define **12 integration APIs plus actual zeroizer once**, no duplicate
  global/main/PUBLICDUMMY/fixture or unresolved project/crypto symbols. HOST ELF64
  x86-64 / ARM ELF32 EABI5/v5TE, both REL/zero program headers. Libc/compiler/final-
  link symbols remain unresolved; unlinkat is the sole newly unresolved libc
  symbol versus base closure. This is not final linking/native ABI or app proof.
- `BUILD_AUDIT.json` preserves two public metadata-reader mistakes (schema display
  KeyError, extra-newline comparison assertion), corrected without changing source
  or invoking candidates; existing dependency/diff outputs preserved, no overwrite.

Zero candidate function/helper/test/fixture/bootstrap/provisioner/verifier/OS-random/
credential/material-retirement/native/media/hardware execution, including HOST.
No private/proc/dev/state/HOLD/capture/network/UART/RPC/HTTP/Git/index/AGENTS/central-
plan edits or subworkers; no adversarial/mutation/fuzz campaign. Only assigned app/
docs outputs. GPL-2.0-only remains derivative-scoped; upstream notices retained.

## Open gates / one next task

Independent ordinary source review of this exact derivative and its custody/delta/
build pins, then parent disposition/adoption decision. **No self-GO or use queued.**
Native syscall/filesystem semantics/ABI/entropy and combined memory/128KiB remain
unqualified. Real ingress/launch hygiene, durable backend/generation/rotation,
external recovery/backing/main/app deployment remain absent. No secure-erasure,
whole-process resource release, SEC/STREAM/G3/G8 completion or restart acceptance.
Inherited23h clean finalized OFF and older independent failures remain; no new
hardware observation/lease. RAM75 method remains blocked on recovery/C1 before
code; no clock/recovery waiver or native queue.
