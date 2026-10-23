# Separate RTSP provisioner process — independent ordinary review

2026-10-23 · SEC-01/03/05/11, STREAM-07.

**Disposition: narrow conditional SOURCE/API/custody sufficiency. No required
correction found in this bounded implementation.** This is not full SEC, launch,
main, native hardening, runtime or product acceptance; parent adoption is separate.

## Scope and exact reviewed inputs

Read HANDOFF.md, CONTRACT.md and AUTHOR_AUDIT.md in
`docs/qualification-plans/rtsp-provisioner-process-20261023/`; the complete new
`provisioner_process.c/.h`, `positive_publicdummy.c` and `compile_recipe.sh`; the
complete frozen `private_handoff.c/.h` and `startup_material.c/.h`; actual
owned-runtime `server/candidate_auth.c/.h` and `verifier_adapter_overlay.c`;
actual verifier ABI, child, isolation and material-initialization checks; relevant
graph, registry and Digest construction code. Findings below use these actual
files, not declarations alone or a fabricated receiver handoff object.

SHA256 pins (paths relative to the workspace):

| Input | SHA256 |
| --- | --- |
| `docs/qualification-plans/rtsp-provisioner-process-20261023/DELIVERY_PINS.json` | `285fa37602511d16306f3b53682b6df6dddf91dea4ed69b94f5b8da97baac95b` |
| `apps/rtsp-auth/rtsp-provisioner-process/provisioner_process.c` | `d929b392110d6563b818aa00993301e07d5542afb3a2f4c37cafebe6f28b3724` |
| `apps/rtsp-auth/rtsp-provisioner-process/provisioner_process.h` | `7208455b184a4a2ed9384b16cff1a05c17217789189ef3fa4812c397ff817bc6` |
| `apps/rtsp-auth/rtsp-private-handoff/private_handoff.c` | `cd3e08600841502f1373c30ea80b43c736aa07e705bed7ec83f20f19d5f5903c` |
| `apps/rtsp-auth/rtsp-private-handoff/private_handoff.h` | `4ad3136dfcf4e8fff5b3a26ab794279a96ee908466bfa547819dd87546fefa44` |
| `apps/rtsp-auth/rtsp-startup-material/startup_material.c` | `5eb23e473750c0b3ae0daecd472c2091ec1a6e8287001c65b9d1634fdd3aceae` |
| `apps/rtsp-auth/rtsp-startup-material/startup_material.h` | `76fb100e06c6b7508b6011400bf35734d273615fd26a3aa8cc57cd2150630206` |
| `docs/qualification-plans/rtsp-provisioner-process-20261023/SOURCE_PINS.json` | `57fd8e39817870527c62ad99a878cd512550959c75d61bc2058e8dc33f4b2993` |
| `docs/qualification-plans/rtsp-provisioner-process-20261023/runs/build01/REPORT.json` | `49115dc14df26b9b2372a4f00a9b6e0b46b40409960ef28344108fb3bd45c283` |
| `docs/qualification-plans/rtsp-provisioner-process-20261023/runs/positive01/OBSERVATION.json` | `f3013f2c7acb27f12870f803600438553d6d2529e98c3233a80644d3062ec2e7` |

## Ordinary source findings

Locations in this section are under `apps/rtsp-auth/` unless stated otherwise.

1. **Actual separate child and canonical ingress move.**
   `rtsp-provisioner-process/provisioner_process.c:232–270` consumes one stable
   process-local attempt, moves the caller ingress slot before fallible preflight,
   checks identity/pipe flags/default SIGCHLD, creates the private socketpair and
   forks once. Preflight/fork failure retains moved ingress and acquired sockets;
   it does not return ownership to an old numeric alias. Successful fork stores
   actual child ownership before the parent's two single-close attempts. Parent
   code never reads ingress. `close_once` at135–139 clears its canonical slot before
   close; parent close uncertainty is sticky, without retry or replacement.

2. **Real constructor/file custody and child-COW config move.**
   `provisioner_process.c:192–229` calls the actual native hardening helper before
   its first read, then the actual create and move with its real local `h`, original
   config address in the fork-private address space and actual fresh prestarted
   owner. `rtsp-private-handoff/private_handoff.c:98–182` invokes the real startup
   constructor, exclusively creates the fixed file, bounds writes, fsyncs, performs
   the sole writer close and owns the readonly FD before checking it. Its move
   checks same config address, unused owner and inode, then clears the source slot.
   No receiver reconstruction of `h` or fake readiness object is involved.

3. **SCM_RIGHTS is duplication, followed by real sender retirement.**
   `provisioner_process.c:177–229` sends exactly once, with one FD and the fixed
   public envelope. After successful send, child clears/closes its config FD,
   socket and directory aliases once. Only complete success sets result0; every
   returning child path crypto-wipes the whole owned header/payload/extra/material
   scratch and calls `_exit`, never returning into parent execution. On a failed
   close, subsequent explicit closes may be skipped, but `_exit(nonzero)` retires
   remaining child aliases and cannot authorize parent bind. The real constructor
   also clears material scratch on return. This is source control-flow evidence,
   not independent observation of dead child memory or all inherited memory.

4. **Received rights represented before validation, including truncation.**
   `provisioner_process.c:273–309` uses `MSG_CMSG_CLOEXEC`; it records all delivered
   SCM_RIGHTS integers before rejecting length, truncation, extra control/FDs,
   envelope or inode. The supplied kernel ancillary capacity represents at most
   eight delivered rights; kernel-normalized control layout, not arbitrary raw
   sender bytes, is parsed. Undelivered excess rights are kernel-discarded and
   `MSG_CTRUNC` still forces HOLD. Only one receive can install rights: any completed
   receive sets `message_received`, and any failed validation is permanently HOLD.
   No overwrite, error cleanup, unrepresented extra FD or numeric close retry is
   introduced. Queued rights/resources remain retained on HOLD.

5. **Owned lifetime, stable HOLD observer, no PID-reuse retry.**
   `provisioner_process.c:312–359` waits only on the still-unreaped PID returned by
   this fork. One WNOHANG call per observation treats EINTR as pending, exact own
   wait as retirement, and ECHILD/other unknown as sticky `wait_unknown`, never
   retried. Known reap clears `child_unreaped`; later calls cannot wait on a reused
   PID. HOLD may collect the still-known child but never clears HOLD or binds.
   READY requires actual wait0 and known parent closes, not send/receive/ACK or
   metadata alone. Explicit bind additionally requires same config/owner addresses,
   unchanged inputs, no unreaped/unknown child and empty parent alias slots. The
   received slot is cleared before assigning real parent `cfg->material_fd`.

6. **Exact input framing and bounded work, not a recovery guarantee.**
   `provisioner_process.c:141–175,204–210,258–259,328–337` implements the16-byte
   little-endian header, version, username/realm1..64 and password1..128 bounds,
   bounded payload and exact EOF. Header/payload/EOF share the2048 read-attempt loop
   budget and the same pre-fork nominal monotonic deadline; EINTR consumes attempts.
   Parent uses that deadline and detects observed regression. Poll/syscall/scheduler
   duration, constructor/fsync completion and whole-interval clock continuity are
   not thereby bounded. The stated exclusions are accurate; no watchdog, automatic
   kill, reset, cleanup or renewed admission is supplied. Small scratch and the
   recorded ARM3712-byte child frame are not a total-stack/load proof.

7. **Public binding and actual downstream checks remain distinct.**
   `provisioner_process.c:48–132,303–308,347–359` checks fresh actual owner, exact
   config/routes snapshots and regenerated public envelope. It binds class/ABI,
   RTSP domain/account, algorithm, generations, limits, identifiers, selected graph
   profile/sensor and normalized routes, plus child inode and exclusive directory
   association. Parent reads no material contents or secret-derived hash. Full
   graph/context immutability is a mandatory caller condition, not a deep snapshot
   supplied by this module. Content correspondence depends on this trusted actual
   child/constructor and unchanged fork snapshot, not file metadata authorization.
   `startup_material.c:33–99` builds the actual1536-byte ABI using existing Digest;
   `qc07f-owned-runtime/server/candidate_auth.c:41–105` retains its real callback,
   image, linked-file and public-prefix checks. Its adapter's actual image/launch
   checks (`verifier_adapter_overlay.c:120–240`), verifier hardening-before-read
   (`isolated-verifier/child.c:23–40`, `isolation.c:80–127`) and material validation
   (`isolated-verifier/engine.c:44–72`) are not replaced or invoked here.

8. **Native and PUBLICDUMMY are separate classes, not a runtime fallback.**
   Header66–74 and `provisioner_process.c:37–63,199–203` select distinct exported
   process symbols and config classes; frozen handoff does likewise. Actual native
   hardening checks root/saved IDs/groups, core0, nondumpability, NNP and successful
   current/future mlock before read; partial failure is terminal in this child.
   PUBLICDUMMY excludes that helper and cannot bind native config. Recipe23–68
   links only separate dummy process/handoff objects plus real constructor/crypto/
   validators into the fixture, not native process/handoff or verifier/runtime code.

## Retained evidence checked read-only, not rerun

Fresh byte-count/SHA256 comparisons matched all59 delivery pins,33 selected inputs,
1771 dependency records (277 recorded path strings),84 artifacts,40 command logs
and five invoked-tool pins. All27 dependency files exactly match their ordered
manifest records;40 recorded argv vectors match REPORT.json. Its27 C compile logs
are empty and40 command exits are recorded0. Exit provenance is the author's
completed audited `set -euo pipefail` recipe, not a reviewer rebuild.

Actual ELF headers match HOST/ARM REL and separate HOST dummy executable roles;
retained ELF/undefined/symbol logs agree with those roles and separate dummy
exports. No complete compiler closure, native ABI or whole-program fit is inferred.
The ONE existing PUBLICDUMMY observation records exit0,270-byte scalar stdout and
empty stderr; both outputs and the recorded executed executable pin match. Its
source checks actual fork, constructor/readonly move, SCM_RIGHTS, owned wait0,
explicit bind and success-only test cleanup. Parent-created public dummy constants
do **not** prove a product parent was secret-free; the fixture does not inspect
dead child memory or exercise native hardening/future verifier use.

## Conditions and remaining real gaps

CONTRACT.md20–47 and header33–43 correctly expose, rather than implement, trusted
authorized external writer/RTSP ingress, a genuinely secret-free parent, inherited
FD/environment/capability hygiene, no aliases/signals/threads/reentry/extra fork,
no competing reaper, no earlier prepare/start anywhere and no native/media owners
before fork. Addresses/graph/routes/callbacks and exclusive namespace/writer
exclusion must remain stable for the entire specified lifetime. A fresh object
check is not proof of those global conditions.

The borrowed0700 directory and linked0600 nlink1 file must remain live/exclusive
through HOLD and future verifier consumption/retirement. No destructor/unlink or
failure retirement policy is supplied. In particular unlink-before-consumption
would violate actual backend nlink1 admission. The config owns the bound FD even
if later startup fails. Authorized producer/launch integration, parent hardening
and global hygiene, durable generation/storage/rotation, linked-file retirement,
bootstrap/main, external media recovery/backing,128KiB-compliant buffers/combined
load and native authenticated PLAY/decode/finite STOP remain unqualified.

Reviewer performed only public source/artifact/metadata reads and hashes; no
candidate import, build, program/test, negative/adversarial/fuzz/mutation case,
private material access, live process/device inspection, network/RPC or hardware
operation. Only this fresh review directory was written; no source, Git/index,
AGENTS or central-plan changes. Parent camera OFF/no-recorder and23g unexplained
prefix HOLD were supplied context, not rechecked or changed. No automatic next
work, source adoption or hardware/native queue follows this review.
