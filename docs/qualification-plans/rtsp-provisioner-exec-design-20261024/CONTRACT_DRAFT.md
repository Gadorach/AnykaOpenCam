# Draft minimal real clean-exec provisioning increment

2026-10-24 · **SEC-01/03/11; RP-08**. SOURCE design only, **NOT SELECTED FOR
IMPLEMENTATION OR EXECUTION**. Read [FEASIBILITY.md](FEASIBILITY.md) for source
anchors and blockers. This is a concrete next-source contract, not executable
pseudocode, a ready loop, permission API, product main or qualified launch policy.

## 1. Actual new source boundary required

One companion **worker program entry** must really start after exec and link only
the public decoder/graph/routes validators, actual private hardener/handoff,
actual startup-material/Digest/Mbed TLS construction and bounded transfer/wipe
logic. No verifier engine/launcher, runtime/provider/media/hub/UI/TLS listener or
test/default credential logic belongs in that worker image. Merely exporting a
callable worker function and invoking it in the old fork child does not meet this
contract. No image path/name/destination is assigned by this draft.

A separate typed **exec provisioner owner** in the still-device-free parent must
own real fork/exec launch, received rights, known-child wait and explicit bind.
It borrows the same stable config/routes/unused parent owner/directory. Preserve
old fork-only module and evidence byte-for-byte; new exports/layout, no aliases
or old-owner impersonation. Three inseparable source classes: actual entry,
explicit bounded codec, actual parent/child exec-launch owner. They are one
bounded feature, not a new process framework.

Later API5 bootstrap adoption needs an explicit new-layout derivative: replace
the embedded provisioner TYPE and its calls, and rewrite `bound_known`/material
capture to inspect the ACTUAL new owner. Do not fill old fork-owner fields from
a launch result or cast owners. Keep OS callback-before-snapshot, real wait/bind
before prepare/start, last verifier fork before native setup, both separate finish
routes and explicit normal-only logical material retirement unchanged in meaning.
Until that actual derivative is reviewed, delivering worker/exec-owner source
does not make the accepted bootstrap use it or establish whole-main hygiene.

## 2. Exact child descriptor/argv/env contract (proposed)

| At worker entry | Role |
|---|---|
| 0,1,2 | Closed, no secret or arbitrary diagnostic output channel. |
| 3 | Sole private nonblocking AF_UNIX SOCK_SEQPACKET control/reply endpoint. |
| 4 | Sole moved readonly/nonblocking credential FIFO; no reads before §4. |
| 5 | Borrowed private directory, root0700; fixed material filename only. |
| all others | Absent at entry. Executable FD6 is CLOEXEC, consumed by actual FD exec. |

Stage source FDs>=16 before fork, disjoint and collision-free; child maps only
these roles, leaves3/4/5 open across exec, marks image6 CLOEXEC, closes tail>=7
and stdio. Check every required operation; no close retry on uncertainty. Parent
keeps the real canonical ingress move and closes its postfork alias once, records
own child immediately, closes child-control/image staging aliases once. New
staging references need explicit slots and one-close accounting on ALL failures.
No hidden helper retains an unrepresented duplicate; original borrowed image and
directory remain caller-owned. Inherited native/media owner aliases are forbidden
at admission, NOT a set to close and call retired.

Actual exec uses the already verified **worker** image FD6, fixed nonsecret
single-role argv, NULL-only env and AT_EMPTY_PATH; no PATH search, shell, runtime
mode/path selector, credential args/environment or dynamic interpreter fallback.
No fexecve fallback is presumed: inspected launchers actually use SYS_execveat.
One fork, one exec attempt; failure exits nonzero, never returns to parent logic.

Worker validates FD role type/access/flags/inodes before input, confirms socket
peer creator matches its still-live parent/root native class, and restores
CLOEXEC on3/4/5 BEFORE calling existing helpers (directory check requires it).
After stdio closure a later file open may legitimately return0/1/2: own/close
those actual returned slots; don't print or assume material FDs must be>=3.
Socket-peer metadata binds the channel, not authorization of the external writer.

## 3. Minimal public snapshot protocol — no pointers or secret metadata

One versioned request over FD3, explicit little-endian integer packing, exact
length and no ancillary rights. NOT raw candidate_config/graph/stat/size_t/enums,
and NOT the old same-fork envelope reinterpreted as a request. Proposed fixed
prefix is **1374 bytes**, followed by counted records in the listed order:

| Offset / bytes | Typed public value |
|---|---|
| 0 /16 | u32 protocol_version=1, kind=REQUEST(1), total_bytes, reserved=0. New protocol namespace, not acceptance of old wire bytes. |
| 16 /16 | u64 original_started_ms, original_deadline_ms. Same boot/time namespace; deadline never reset after exec. |
| 32 /48 | u64 pairs(st_dev,st_ino) for actual inherited ingress, directory and CHILD control endpoint, in that order. |
| 80 /24 | u64 boot_generation, epoch, credential_generation. Nonzero exact parent values. |
| 104 /36 | u32 algorithm, nonce_ttl_ms, rate_window_ms, global_verifies, global_challenges, account_verifies, account_challenges, peer_verifies, peer_challenges. |
| 140 /16 | u32 device_count<=16, association_count<=16, profile_count<=8, group_count<=8. |
| 156 /192 | device_id[64], account_id[64], profile_id[64]. |
| 348 /1026 | live_uri[513], track_uri[513]. |
| 1374 /counted | devices, associations, profiles, groups below, no alignment padding. |

Records: device144B = id[64], resource_group[64], u32 kind/state, u64
capabilities; association128B = camera_id[64],device_id[64]; profile204B =
id[64],camera_id[64],encoder_id[64],u32 width/height/codec; group72B = id[64],
u32 qualified_concurrent_sessions/state. Maximum **7934B** with existing counts;
one bounded packet fits8192B scratch, not a process/stack-fit qualification.
Use checked exact-length arithmetic and enum/range validation before local
typed conversion. Zero all unused fixed bytes; require termination and canonical
zero tails; preserve all actual public strings/values, no synthesized replacement
graph, default routes or normalized-away semantic difference. Parent must reject
unrepresentable/noncanonical inputs before launch rather than silently repair.
Receive the request nonblockingly under a finite call count and the ORIGINAL
nominal budget: deadline-start must be1..10000ms, no post-exec deadline reset or
time-namespace change; clock error/regression refuses. Use bounded recvmsg control
storage and represent every actually delivered unexpected right before refusal;
reject ancillary/truncation/extra bytes, exit nonzero without reading credentials.
No permissive read/recv path that silently ignores SCM_RIGHTS is a decoder.

Pinned native worker role fixes native material account1/domainRTSP/RV_ABI1/
1536B/logical-only begin contract and candidate ABI3 locally; parent checks those
same source requirements. No caller-selected account class, fixture mode, callback,
verifier FD/size/hash, capability permit, ingress-authorized Boolean, UID choice,
new epoch, entropy output, native device identity or pathname in this request.

Worker reconstructs real local arrays + camera_graph and a ZERO-initialized local
candidate_config/routes. Set only consumed constructor fields, constants and
`material_fd=-1`; its local image metadata has ABI1/native profile and **fd=-1**.
This is a constructor projection, not a verified image descriptor: current
constructor/class checks consume only ABI/profile; handoff move's FD-inequality
check still passes. Real parent verifier image FD/bytes/pin stay parent-only.
All callback/context fields remain NULL; actual constructor does not call them.
Validate full graph/selected720p H264 profile, rates, routes and local config
BEFORE secret reads. Existing constructor repeats its own validation later.

Use an actual local CANDIDATE_PRESTARTED_INIT object solely to satisfy the genuine
same-address local handoff move into that local config, never backend start or a
claim it is the parent's owner. Existing h.self/config/routes stay local and
stable through create/move. Across exec/IPC only public values and kernel FD
references cross; NEVER those C objects, addresses or constructor scratch.

Reply: ONE explicitly encoded1392B native semantic envelope matching
`provisioner_process.h:24–31` field order/widths, including six final zero bytes;
same public bindings/limits/routes/native class/material ABI/domain/account1 plus
actual created file st_dev/st_ino; exactly ONE SCM_RIGHTS readonly FD. No raw C
padding and no username/realm/HA1/credential lengths/hash. This preserves the
old checked result semantics, not a wire-ABI compatibility claim. Request and
reply are one-shot direction-specific packets; no ready/authorized/go token,
retry, second request, logging stream or new diagnostic/receipt framework.

Parent captures/retains the exact serialized public snapshot and separately its
original pointer/callback/config identity. Re-encode/compare full graph/routes/
public bindings at receive AND bind; still require trusted immutability and
exclusive serialized access. Keep real config object/owner freshness checks.
No extra launch nonce is needed for a one-use private socket and known unreaped
own child; existing boot/epoch/credential generations remain exact bindings, not
newly generated or made durable by this protocol.

## 4. Required pre-exec versus post-exec/pre-secret order

**Before fork/exec in parent:** establish external authorized writer/ingress and
genuine secret-free/device-free history, trusted exclusive namespace, stable
public state, sole reaper/default SIGCHLD. Parent validates worker source/image
provenance on an opened stable readonly root-owned/nonwritable/non-setid regular
nlink1 executable inode, exact reviewed image bytes/pin and static native ARM
ELF role. Validate ancestor/root-file trust separately; a path string, permissions
alone or the verifier's existing image pin is insufficient. No source path or
image is currently supplied by this draft. Start original nominal budget before
launch effects; public snapshot publication is bounded to one send.
Parent's own core/nondump launch prerequisites for later real verifier start
remain separate actual work: hardening the worker cannot satisfy them.

**Child before exec:** perform only needed FD remap/close and setup operations,
core soft+hard0/NNP/nondump protection, bounded memlock-limit admission, and a
real child-only capability/exec-regain transition. Root IDs/groups0 remain needed
by current helpers. Required outcome is zero effective/permitted/inheritable/
ambient caps, no retained bounding authority, and locked no-root-regain policy
before exec. Implementation must resolve native securebits/ambient/bounding APIs
and setup privilege while still available, clear keep-caps policy appropriately,
verify actual results and fail closed; no metadata flag can replace those calls.
No later CAP_IPC_LOCK/SYS_RESOURCE/KILL or UID0 privilege reacquisition workaround.
These new source operations/policy have NOT been implemented or qualified.

Owner-DAC file operations do not inherently need capabilities while IDs remain
root; mlock must succeed under an admitted inherited allowance and actual footprint
without privilege bypass. Respect existing native2MiB ceiling and128KiB allocation
cap; reject unavailable/insufficient allowance, do not raise a hard ceiling or
claim the graph packet size proves process fit. No global host/media policy edits.

**Fresh worker before FIRST credential byte:** validate entry/roles/channel and
the one public request; reconstruct/validate local public state and exact resource
bindings; verify actual identity/caps/no-regain state after exec; run the REAL
private provisioner hardener so core0/nondump/NNP/mlock are observed in THIS image.
No dynamic-loader execution under a contaminated env; no reliance on pre-exec
mlock/nondump surviving exec. Existing incompatible seccomp profiles must not
already be inherited. No filter weakening/fallback is permitted.

Current actual hardener is not a syscall sandbox. If the required isolation claim
includes preventing later exec/dumpability restoration or arbitrary root-DAC
opens, a reviewed WORKER-ONLY restriction is a separate concrete missing source
part of this launch slice, not satisfied by `rv_harden`. It must permit the real
repeated helper calls plus constructor's openat/stat/fcntl/lseek/write/fsync/close
and SCM_RIGHTS sendmsg, while restricting the claimed authority. No runnable filter
or native syscall list is selected here; do not advertise such enforcement until
that source/policy is actually delivered/reviewed. A clean-exec FD/env boundary
alone may be delivered only with this narrower claim explicitly retained.

**Then only:** original bounded private frame+exact EOF → ingress close-once →
actual hardener/create/constructor/write/fsync/writer-close/readonly checks → actual
local handoff move → ONE response+SCM_RIGHTS → sender FD/control/directory closes
once → all owned secret scratch wipe → exit0. On EVERY failure wipe owned secret
storage and exit nonzero without unlink/repair/retry; possible file persists.
Parent never reads credentials or material payload. Normal result means actual
source path completed, not measured wipe of dead memory or an fsync hang bound.

## 5. Parent result and lifetime invariants (non-negotiable)

* Owner records actual fork result, all canonical FD slots, possible file from
  launch onward, sticky launch/close/wait/clock errors. No arbitrary PID input,
  competing reaper or unknown-wait retry. WNOHANG known-child observation only.
* Store ALL delivered rights before validating exact reply/control/truncation/
  count, as current receiver does. Require original bindings plus actual readonly
  CLOEXEC regular root0600/nlink1/1536B/offset0 FD and fixed-name same-directory
  dev/inode checks. Directory/ancestors and all writers remain externally trusted.
* Only exact reply AND known own-child wait0 AND all known parent closes AND
  unchanged original public/config/owner state admit READY then explicit bind.
  Child reporting success/hardening, EOF, exec-success inference or inode metadata
  alone is insufficient. Material FD is moved into original unused parent config,
  never copied or forged as a private handoff object. No auto prepare/start.
* Partial request send/exec/image/hardening/constructor/protocol/timeout/nonzero
  exit/unknown-close/unknown-wait failures permanently HOLD. Observe may collect
  still-known child only. Retain received refs, partial named file, directory,
  owner and all external lifetimes; no implicit close/unlink/kill/reset/destructor.
  Nominal deadline is not a finite syscall/fsync/scheduling/recovery guarantee.
* Keep file linked for real backend nlink1 checks; parent owns material after bind
  even on later prepare/start failure. Only genuine normal API5 start and separately
  acknowledged normal finish may use reviewed logical close→name check→unlink→
  directory-fsync retirement. Never use child exit as backing/native retirement,
  secure erasure, durable storage or whole-process exit permission.

## 6. Whole-product boundary still blocked

Fresh worker exec cannot prove parent memory secret-free before fork, remove a
parent credential mapping, authenticate external ingress, create a safe media
parent/main, allocate generations durably, qualify OS randomness/crypto entropy,
or supply surviving native backing/external recovery/C1. A fresh MEDIA-PARENT
exec and all-domain launch/FD/secret-routing authority require their own actual
source chain and external facts, not an extra Boolean in this protocol.

No implementation/build/test/normal or negative runtime case is queued by this
draft; adversarial/mutation/fuzz work remains deferred, NOT PASSED. Native syscall/
libc/image ABI, lock footprint and all policy outcomes require future separately
authorized qualification. Stop here; preserve old fork-only route and failures.
