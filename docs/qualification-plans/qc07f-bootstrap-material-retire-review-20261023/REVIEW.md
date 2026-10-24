# Independent ordinary review — explicit bootstrap material retirement

Assigned20261023 · SEC-03/08/11, CORE-03/04, STREAM-07.

**Disposition: narrow conditional SOURCE/API/order/custody sufficiency. No required
source correction identified.** Not full SEC, runtime/native, filesystem, product,
release or adoption GO. No candidate function, helper, test or child was executed;
no compiler/linker/build recipe was rerun. Only this fresh review directory written.

## Exact reviewed delivery

`apps/rtsp-auth/qc07f-bootstrap-material-retire/`:

- Complete C,260 lines: `b89782a3522dd16d201a1dc3b24e64b8ce0a5c2e519af3a89e9ad45c7a054b60`.
- Complete header,121 lines: `aec5a98dd3d3a7cd867ebf93d224c7e049b44d2c8caff8fcf0fd2795c450f529`.
- Four-file source aggregate: `bb8cef1970e563f6f1091d99c3cc3a2e2f416a37be84252885c8752df3e6bbbe`.

Read complete contract, handback, recipe, both diffs and inverse proof in
`docs/qualification-plans/qc07f-bootstrap-material-retire-20261023/` (below,
**author docs**), and accepted OS-random bootstrap source/header/disposition.
Independently inverted both diffs in memory to exact accepted base C
`abd2ad799c635f011c726dee493b417bfebd672ff2fb6a0bba0528ca55894c94` and header
`f965a8075d6079f7d1d02a9e966ba17c2802b3e8e7221ae423e4fbbb1310c076`.
Begin/observe/finish bodies are byte-identical. Start changes only post-bind
capture and the actual-start-attempt write; remaining additions are helpers,
includes/static ABI assertion, explicit API and documented struct/enum/init extension.
Select ONLY this replacement C/new header and actual RNG object, never old bootstrap
objects/combined RELs alongside it. This is not old binary/native ABI compatibility.

## Source findings and actual dependency chain

- **Actual bind before snapshot/start:** C150–181 calls real bind, then capture,
  then actual device-free prepare and same-owner prestarted start. C50–102 ties
  snapshot to actual provisioner/config/routes/owner/directory, actual envelope
  inode, observed own wait0/no unknown wait or close, and emptied received slots.
  Snapshot failure is sticky HOLD retaining cfg FD/file; no prepare/start/cleanup.
  Actual `rtsp-provisioner-process/provisioner_process.c:192–229,273–359` creates
  material in the child, transfers readonly FD/public envelope, closes sender
  references, observes its known child and explicitly moves received[0] to cfg.
  `rtsp-private-handoff/private_handoff.c:128–181` supplies the real exclusive
  fixed-file write/fsync/writer-close/readonly-open/move; header7 fixes the name.
- **Metadata, not secret inspection:** C30–102,231,243 checks root-owned0700
  CLOEXEC borrowed directory associated with the real provisioner; actual
  root:root0600 regular1536-byte/nlink1 readonly CLOEXEC FD and matching no-follow
  `rtsp-startup.material` inode. Explicit comparisons cover dev/ino/full mode,
  UID/GID/nlink/size/mtime/ctime including nanoseconds, never struct padding.
  Captured material FD is historical, NOT a second owning slot. No added payload,
  prefix, hash, offset read/reset or atime comparison. This exclusion is necessary:
  actual `qc07f-owned-runtime/server/verifier_adapter_overlay.c:120–136,179–237`
  uses F_DUPFD_CLOEXEC→fork→dup2(FD4), sharing the original OFD; actual
  `isolated-verifier/child.c:32–40` reads1536 and closes FD4. Existing prefix preads
  do not reset that shared offset. Quiet directory mtime/ctime/size/nlink are
  compared only before unlink, not after unlink legitimately changes metadata.
- **Real normal finish, not metadata permission:** C183–230 requires this API's
  actual start attempt/success and finish attempt/return0+PS_FINISHED, recorded
  snapshot/bind/prepare, stable same owner/config/runtime, no sticky error/source
  hold and cleared backend state. Actual `qc07f-owned-runtime/owned_runtime.c:204–229`
  prepares callbacks without postfork/native activation. Actual server
  `candidate_auth.c:41–116,218–258` starts the verifier and finishes through backend
  destroy; normal initialized success requires observed wipe and known wait0
  captured BEFORE child destruction. Adapter302–344 retains IPC-close uncertainty
  through stop/destroy. Cleared backend fields corroborate these actual calls;
  they are not global native-ownership proof or a fabricated retirement receipt.
- **One explicit request:** C222–231 consumes one identity-valid request even on
  bad phase; READY/PROVISIONING/START_FAILED/HOLD/UNKNOWN/prepare failure cannot
  reach close/unlink. Repeats refuse without changing retained history; foreign
  identity/reentry does not touch another owner. No failed-start cleanup adoption.
- **Custody and syscall order:** C232–259 clears canonical cfg.material_fd BEFORE
  ONE close; attempted/result/errno/unknown/former-not-owned fields distinguish
  unattempted default0 from actual success. Any close error/EINTR means sticky
  uncertainty, file kept, no unlink/numeric retry. Only close0 permits fresh exact
  name/quiet-directory recheck and ONE unlinkat(fixedname,0); only unlink0 permits
  ONE borrowed-directory fsync. Named mismatch retains the file after known close;
  unlink failure does not fsync; fsync failure retains acknowledged unlink0/HOLD,
  never recreation/retry. Success means ONLY cfg FD close0 + this staging-name
  unlink0 + directory syscall ACK, not erasure/durability/rotation/all resources.

## Evidence independently checked, read-only

Matched916 build-input records and their absolute-path SHA manifest,56 delivery
pins,4 source pins,221 new dependency records,871 reused dependency records,
37 artifacts,24 successful command/log pins and17 retained tool-input records
(plus report tool subset). Checked88 frozen/live source byte pairs; directly read
46 GNU archive member bodies and matched original object bytes; checked58 original
production compile argv/source/object/exit/log associations. Counts overlap.
HOST objects remain production-role, not PUBLICDUMMY/native fallback; no fixture
defines, old bootstrap unit/REL, duplicate zeroizer or extra crypto objects selected.

Author evidence is **2 HOST/ARM changed-unit compiles +2 real REL links,24 commands
PASS, NOT RUN**, cited not repeated. Read actual ELF headers: HOST ELF64/x86-64
337504B; ARM ELF32/ARM230864B; both REL/zero program headers. Pinned symbol logs
show12 integration APIs plus actual zeroizer once, no duplicate globals/main/fixture
or unresolved project/crypto APIs. Sole new undefined libc symbol versus base is
unlinkat; libc/compiler/final-link symbols remain unresolved. Not final executable,
native ABI, full dynamic compiler closure or execution proof. Retained build UTC
is Oct24 despite assigned Oct23 path; no clock correction/admission inferred.

Two reviewer-only metadata reader mistakes are retained here: base unresolved
entries were dictionaries, not strings (TypeError); SHA manifest uses absolute
paths, not relative JSON spelling (comparison assertion). Corrected read-only
metadata continuations established symbol/path associations; no candidate/build
operation, source change or policy-refusal retry occurred.

## Mandatory conditions and remaining gaps

Contract75–88/header34–42,71–84,108–118 remain essential: once-initialized stable
private object/config/graph/routes/context, canonical borrowed directory live in
ALL states including HOLD/terminal, no external FD aliases/reuse, writers,
namespace/ancestor mutations, threads/signals/extra forks/reapers/reentry or
mutable-handle borrowing. Only the reviewed underlying child/duplicate lifecycle
is admitted. No earlier native/media ownership ANYWHERE; callbacks device-free;
actual authorized ingress, secret-free parent/launch hygiene, real retain_verifier
and exact native image/hardening still required. C fields and readonly flags do NOT
prove these conditions or resist arbitrary writer/namespace/hostile-root races.
No bootstrap native-activation/serve/mutable-getter/global-permission API is added.

No destructor/autocleanup/rmdir, unrelated private-config/evidence deletion or
image-FD/root-context/borrowed-directory retirement. Inherited unchecked temporary
duplicate/pidfd closes do not become all-descriptor retirement proof. Stable
ownership must persist; success does not authorize native cleanup/process exit.
Native syscall/filesystem/entropy/ABI qualification, durable generation/storage/
rotation, real external ingress/launch/recovery/backing/main,128KiB/combined-load
and product gates remain open. This review is not an execution or adoption queue.
Inherited23h is clean finalized cold observation ending OFF, not a live lease;
older failures stay independent. RAM75 method remains recovery/C1 BLOCKED before
code. No hardware/current-state/private-capture observation was made here.

Selected immutable review anchors are in `INPUT_PINS.sha256`; author manifests
retain the wider checked graphs. Parent disposition remains separate.
