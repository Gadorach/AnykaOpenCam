# Clean-exec RTSP provisioner — bounded SOURCE feasibility

2026-10-24 · **SEC-01/03/11; RP-08** (SEC-05 material dependency).
Documentation only; no implementation, build, import, test, child, syscall probe,
credential operation or target observation. Caller identifies local closeout
`296511e`; this lane did not inspect Git/index or establish a current HEAD.

## Disposition

**A fresh provisioning-worker exec is source-feasible, but is not a drop-in call
to the current fork child.** The eligible next source slice is a real companion
worker entry, explicit public-value codec and separately typed exec-launch/owned-
wait/bind owner. It must use the actual constructor and private-file handoff,
not a permission wrapper. Neither companion entry/image nor that public protocol
exists in the inspected route. See [CONTRACT_DRAFT.md](CONTRACT_DRAFT.md).

This would narrow the worker's address-space/FD/argv/environment boundary before
credential input. It would **not prove that the media parent never held secrets**,
authorize an external writer, qualify native capabilities/syscalls, or deliver a
product launcher. Fresh **media-parent** exec from a separately trusted secret-
free launcher is a different missing launch boundary. Wiping parent storage or
examining fresh metadata cannot establish its earlier memory history.

## Actual source chain (paths below are workspace-relative)

Abbreviations: `P=apps/rtsp-auth/rtsp-provisioner-process/provisioner_process`;
`H=apps/rtsp-auth/rtsp-private-handoff/private_handoff`;
`M=apps/rtsp-auth/rtsp-startup-material/startup_material`;
`B=apps/rtsp-auth/qc07f-writer-bootstrap-candidate/bootstrap.c`;
`A=apps/rtsp-auth/qc07f-owned-runtime/server/verifier_adapter_overlay.c`.

| Boundary | Concrete source/order |
|---|---|
| Parent admission | `P.c:232–270`: consume one process attempt; move canonical ingress slot BEFORE fallible checks; native root real/effective/saved UID/GID0/groups0; readonly nonblocking CLOEXEC FIFO; directory0700/current UID/CLOEXEC; default SIGCHLD; public envelope and shallow config/routes snapshots; nominal monotonic budget; create private nonblocking CLOEXEC seqpacket pair; ONE fork. Parent closes ingress and child socket aliases once. No credential read. |
| Child before input | `P.c:192–204`: allocate bounded secret scratch, close parent socket; call **actual** `qc07f_rtsp_private_provisioner_harden_v1` before first header read. `H.c:13–34` checks root/saved IDs/groups, sets both core limits0, nondumpability, NNP, current/future mlock, then checks core/dump/NNP. It does NOT close all inherited FDs, clear env/caps/ambient/bounding sets, set securebits, or install seccomp. |
| Input | `P.h:7–13`, `P.c:150–175,204–210`: private four-LE-u32 header, version1, username1..64/realm1..64/password1..128, bounded concatenated payload and exact EOF; nonblocking reads/polls,2048 shared call budget and original nominal deadline; then ingress close-once. No parent payload encoder in product API. |
| Construction/file | `P.c:211–214` → `H.c:98–166`: actual hardener repeated; actual constructor; exclusive fixed-name `openat(O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600)`; exact1536B bounded write; inode/name checks; file fsync; writer slot relinquished before ONE close. Only complete+known close permits readonly NONBLOCK/NOFOLLOW/CLOEXEC reopen and offset0/name/inode checks. Scratch cleared on every return. Failure does not unlink/repair. |
| Actual crypto | `M.c:33–99`: validate native/logical config, nonzero epochs/generations, IDs, entire bounded graph, selected H2641280×720 profile, all three rate limits and normalized routes; call real `camera_rtsp_digest_ha1`; fill actual `rv_material`, native account1. No entropy callback, default credential or generation allocation. Borrowed password is not cleared by constructor. |
| Sender custody/exit | `P.c:215–229`, `H.c:169–182`: actual local handoff move into CHILD COW config using actual unused owner; one SCM_RIGHTS send of readonly FD plus public envelope; close config FD/socket/directory once; wipe entire owned ingress/material scratch; `_exit(0)` only after normal success. Any failure exits nonzero after wipe; residual child FDs retire at exit, possible linked file remains. SCM_RIGHTS duplicates an OFD reference, NOT a cross-process C move. |
| Parent receive/wait/bind | `P.c:273–358`: store every delivered right BEFORE checks, reject truncation/extra control/count; exact public envelope; readonly CLOEXEC regular0600/current UID/nlink1/1536B/offset0 inode association with same fixed name/directory; actual known child's WNOHANG wait0; repeat unchanged/metadata checks; close control once; explicit bind into SAME original unused config. No parent material read here. Unknown wait/close/nonzero exit/deadline is sticky HOLD; no kill/retry/cleanup. |
| API5 continuation | `B:112–188`: genuine OS callback selected before provisioner snapshot → real wait/bind/capture metadata → API5 prepare/start. `B:226–282,327–451`: last verifier fork precedes native setup; typed serve; separately acknowledged finish; only normal start+finish admits explicit close/unlink/directory-fsync material retirement. No automatic failure retirement. |

Source acceptance/compile history is in
`docs/qualification-plans/qc07f-writer-bootstrap-20261024/PARENT_DISPOSITION.md:9–47,91–110`
and `docs/qualification-plans/rtsp-provisioner-process-20261023/PARENT_DISPOSITION.md:9–23,47–65`.
Those are **SOURCE/compile accepted, NOT RUN** for this native composition. The
historical single PUBLICDUMMY fork case is not clean-parent/native proof and was
not repeated here. The later verifier backend DOES read a public material prefix
(`candidate_auth.c:66–75`, `A:230–237`); it does not read username/realm/HA1.

## What crosses fork today, and what exec actually needs

`candidate_auth.h:17–36` (under `qc07f-owned-runtime/server`) includes graph pointer,
opaque context and eight callback pointers. `P.c:256–257` copies pointer VALUES,
not pointed-to graph/context storage; `P.c:111–118` is not a deep graph snapshot.
Fork inherits the entire address space and all open FDs, not just this API's fields.
The no-earlier-secret/native/media/aliases invariant remains global and trusted
(`P.h:33–43`; provisioner CONTRACT:20–47). CLOEXEC does nothing without exec.

| Public item now inherited | Worker need / parent-only distinction |
|---|---|
| boot_generation, epoch, credential_generation; algorithm, TTL/window and six rates; device/account/profile IDs | Actual constructor inputs; copy VALUES exactly. Native account1, RTSP domain, material ABI/size and logical begin class are fixed source requirements, not authorization tokens. |
| graph pointer plus up to16 devices/16 associations/8 profiles/8 groups | Constructor validates the WHOLE graph before selecting profile/sensor. Send bounded value records and reconstruct local arrays/pointers. Sending only sensor/profile or a fabricated minimal graph would silently remove this input equivalence. Counts are bounds, not hardware qualification. |
| live/track registry | Constructor rebuilds normalized routes (`M.c:66–72`); explicit bounded route bytes, no private endpoints or credential userinfo. No registry addresses cross exec. |
| `cfg.image` verifier ABI/profile/FD/bytes/image SHA256 | Constructor/handoff only consume ABI/native profile and FD inequality. Real verifier FD/bytes/pin/executability are **parent** backend/launcher checks (`candidate_auth.c:59–63`, `A:138–177`). Worker needs no verifier FD or verifier executable bytes. It needs its OWN independently reviewed worker image. |
| context; random/source/owner/listener/retain callbacks; unused owner address | Never called for construction/transfer. Do not serialize function/heap/runtime addresses. Worker uses a genuine local empty prestarted object ONLY for the existing local handoff move; parent retains/rechecks its real unused owner separately. No COW config or owner move survives exec. |
| directory FD/inode; ingress FD; socket endpoints; start/deadline | Pass only designated child roles through exec, validate exact public inode/type/flags bindings, same monotonic time domain/original budget. Parent keeps original directory and own socket endpoint/known fork result. Filesystem st_dev/st_ino are not sensor/encoder device identities. |
| native provider/frame/encoder/watchdog FDs, DMA/leases/media backing/listener | Forbidden BEFORE either fork under existing contracts; NOT needed by worker. Closing inherited aliases cannot masquerade as DMA retirement or repair an illegal launch order. No device discovery or native identity acquisition belongs here. |

Graph definitions/validation:
`framework/include/camera/backend.h:24–45`, `framework/src/backend.c:26–57`.
They are value records behind pointers, not a portable in-memory wire ABI.

## Existing exec/image/isolation facts and their limits

* `A:120–177,187–237` actually stages child FDs>=16, arms parent-death/release
  barrier, drops verifier to UID/GID1000, zeroes caps, core0/nodump/NNP, remaps
  IPC3/material4/image5, closes the tail and stdio, and calls
  `SYS_execveat(image,"",fixed_argv,empty_env,AT_EMPTY_PATH)`. This is not fexecve
  or a worker launcher. Its verifier image check requires pinned readonly nlink1
  native ARM ELF32 ET_EXEC/EABI5/softfloat/no PT_INTERP/PT_DYNAMIC/executable stack;
  root-owned public r-x permits the existing nonroot verifier. Backend additionally
  requires exact0555 and its fixed verifier pin. Do not relabel that ELF worker.
* `isolated-verifier/authority/launch.c:49–156,194–214` has a real component-safe
  public image path walk, ELF/pin validation, copied code inode and FD exec;
  `authority/credential_process.c:157–164,188–209` stages collision-free fixed FDs
  and execs an opened image with empty env. Neither implements this RTSP ingress
  worker; weaker final-component-only path checks are not a replacement for image
  provenance and ancestor/namespace trust. No executable destination is chosen.
* `isolated-verifier/isolation.c:80–127` is a **nonroot verifier** hardener:
  closes FDs>=5, then seccomp denies openat/dup/ioctl/prctl; filter:24–77 also lacks
  exec, sendmsg and file-creation/fsync operations. It cannot be reused for a root
  constructor needing directory5 and SCM_RIGHTS. `authority/parent_sandbox.c:12–16,
  26–113` forbids open/socket/exec/dup/clone/ioctl, and accepts only selected fcntl
  commands. An inherited filter cannot be loosened by exec. Run the launch BEFORE
  incompatible final parent sandboxing; never weaken either existing profile.
* Actual private provisioner hardener `H.c:25–34` is compatible with file creation
  precisely because it installs **no seccomp/capability drop**. Root UID/GID0/no
  groups are required by both handoff and parent metadata checks. Constructor
  crypto itself needs no root privilege; owned0700 directory traversal,0600 file
  create/write/fsync/readonly reopen can use owner DAC with zero capabilities.
  Existing APIs would reject a newly invented nonroot provisioner account.
  It hardens ONLY its calling child, not the media parent. `A:183–185` separately
  requires parent core0/nondumpability before verifier start; the API5 bootstrap
  does not establish that launch prerequisite by hardening its provisioner.
* Zeroing capset while retaining UID0 is not by itself an exec-regain policy.
  No securebits/ambient/bounding-set handling appears in these inspected launch
  paths. A new child-only launch primitive must resolve those BEFORE exec while
  it still has necessary setup privilege; post-exec worker must verify zero
  effective/permitted/inheritable/ambient capability sets and the intended locked
  no-root-regain/bounding state before secrets. NNP is not a substitute. Do not
  copy the authority launcher's retained CAP_KILL or verifier UID1000 as policy.
* Expected Linux exec behavior (not qualified here): mappings/mlock state and
  dumpability cannot be assumed to survive unchanged. Core0/NNP and a sufficient
  bounded memlock allowance must precede exec; real `H` hardening must run AGAIN
  after exec and before secret reads, including mlock and nondump recheck. Static
  reviewed code avoids an unpinned dynamic loader; empty env passed at exec, not
  clearing `environ` after loader startup. Native syscall/libc/ELF support and
  actual lock footprint are OPEN, including close_range/execveat/prctl/cap APIs.

The current helper alone is not a hostile-code syscall sandbox and does not
prevent its own later dumpability changes. Any stronger enforced worker policy
needs a separately reviewed compatible worker-only restriction, including its
repeat-hardener and file syscalls; no such policy/filter is implemented or selected
here. Do not claim READY15 or reuse the verifier flag word for this worker.

## Exact remaining blockers / stop boundary

1. No real companion entry/worker image/public snapshot codec or typed exec owner;
   no caller supplies a reviewed root-owned stable worker ELF/inode/provenance.
2. Child-only securebits/cap/bounding/ambient transition and post-exec verification
   need real source and available native setup authority; no runtime policy/account
   changes are inferred. No proven2MiB lock/process-fit budget or new ceiling.
3. Authorized independent RTSP writer, authenticated domain/generation allocation,
   ingress exclusivity, private namespace/ancestors and no writer aliases remain
   external facts. FD metadata/SCM peer identity cannot establish them.
4. Product media-parent clean launch/all-domain secret history, callbacks/external
   retention root/backing/recovery, store/rotation and release packaging remain
   absent. Worker exec cannot supply any of them. Bootstrap must eventually use
   the NEW typed launch owner, not forge old `rtsp_provisioner_process` fields.
5. Native entropy/trust/storage, C1, syscall/image ABI and combined-load memory
   qualification remain open. `rtsp-os-random/os_random.c:14–25` is one nonblocking
   public32B getrandom callback, not durable credential authority or cold-entropy
   acceptance. File fsync is not a power-loss-safe credential store.

Task sources: `09-SECURITY-AND-CONFIGURATION.md:16–31,56–62,75–79` and
`REMAINING_FEATURES_AND_BUGS_GATED_PLAN_20261021.md:402–429`, under
`docs/plan/camera-foundation/`; shared process/owner order also follows
`02-SHARED-ARCHITECTURE.md:16–43,64–71`.

**Stop after this report.** No implementation/calls/native queue selected; normal,
negative, adversarial, mutation and fuzz execution NOT RUN. Old fork-only route,
all failed evidence/HOLDs and dispositions remain unchanged.23h OFF is historical,
not fresh state; RAM75 recovery/C1 blocked, Query78 parked. No input-pin framework:
the targeted paths/function/line anchors above describe inspected public source,
not a new hash seal, independent review or OS-behavior claim.
