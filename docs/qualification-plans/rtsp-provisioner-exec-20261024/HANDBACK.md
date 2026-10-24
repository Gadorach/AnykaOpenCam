# SEC-01/03/11 RP-08 — actual clean-exec source handback

2026-10-24. **SOURCE CANDIDATE DELIVERED; NOT ADOPTED, NOT RUN.** Independent
ordinary review and parent disposition mandatory before use. Only assigned fresh
app directory and this qualification directory changed. No reviewer/subworker
spawned; no central plan/AGENTS/Git/index/private/hardware changes by this lane.

Read [CONTRACT.md](CONTRACT.md) (written before code) and
[BUILD_RESULT.md](BUILD_RESULT.md). First build01:32 production HOST/ARM C
compilations, four rooted REL links, one genuine static ARM companion link,
127 logged offline commands PASS. No correction/retry/build02 or candidate call.

## Actual three-class call graph

1. Parent `qc07f_rtsp_exec_begin_v1` consumes one process-local attempt, moves
   the canonical ingress slot BEFORE fallible preflight, records original nominal
   deadline, validates public graph/config/routes/native root/sole-reaper class,
   FIFO/directory and dedicated independently pinned worker image. `pe_image_validate`
   reads ONLY public executable bytes, full SHA256 and static ELF/metadata bounds.
   Parent verifier FD/image metadata never enters the worker request. Required
   image provenance/ancestors/correct worker-role association are external facts.
2. Create actual private nonblocking seqpacket pair; encode WHOLE graph and public
   bindings; immediately record each >=16 F_DUPFD_CLOEXEC result in canonical owned
   stage0..3 (control, ingress, directory, worker image). Snapshot original config/
   routes/graph pointers/unused-owner bytes/image policy. Actual ONE fork; parent
   immediately records known PID/unreaped/possible-file before closing its ingress,
   child-control and all four stage aliases once, clearing each slot BEFORE close.
   Parent publishes ONE whole public packet only after known closes/current checks.
3. Child `pe_child_exec` does ONLY actual setup/exec, not construction: real root
   IDs/groups0/no inherited seccomp; bounded inherited memlock allowance and AS
   ceiling2MiB (only lower limits, never raise), core hard+soft0/nondump/NNP,
   ambient clear, locked NOROOT/no-setuid-fixup/no-ambient-raise/keepcaps-OFF,
   every bounding cap dropped, capset ZERO and actual verification. Kernel-header
   CAP_LAST_CAP is a fixed policy bound; unexpected newer caps/API mismatch refuses.
   Preserve restrictions or reject unsupported/locked incompatible state; no cap
   regain or CAP_IPC_LOCK/SYS_RESOURCE/KILL workaround. Map only3/4/5 and CLOEXEC
   image6, checked close_range>=7, checked stdio closes/absence, verify again,
   ONE SYS_execveat(6,"",fixed argv,NULL-only env,AT_EMPTY_PATH). Failure _exit
   nonzero, never parent logic, fexecve, PATH, shell, interpreter or retry.
4. Actual fresh `worker_entry.c:main`: exact fixed argc/argv/empty env; verify
   post-exec IDs/zero effective-permitted-inheritable/ambient/bounding/securebits/
   limits/NNP/no inherited seccomp. Verify stdio and image6 absent; actual checked
   pre-exec close_range supplies tail closure. Restore CLOEXEC3/4/5; check actual
   socket/FIFO/directory access/types/metadata and parent root peer. SO_PEERCRED is
   socketpair CREATOR parent, NOT child PID proof. The still-live actual parent
   comparison is getppid plus peer creator; direct unexpected worker invocation is
   NOT launcher/image provenance proof.
5. `pe_receive` stores ALL actually delivered SCM_RIGHTS descriptors before any
   packet/control/truncation refusal. Worker request rejects any ancillary/right,
   truncation, extra packet bytes, malformed reserved/counts/enums/noncanonical
   strings/tails. Decode1374 prefix + counted144/128/204/72 records, max7934,
   into actual local typed arrays/graph/config/routes, not a fabricated graph.
   Original start/deadline difference1..10000ms, no new budget; verify actual
   inherited inode bindings, graph/profile720p-H264/rates/routes before secrets.
   Local config projection fixes ABI3/RV_ABI1/native/logical/account1/domainRTSP,
   image.fd=-1/material_fd=-1 and NULL callbacks. It is NOT a verified image.
6. Call REAL `qc07f_rtsp_private_provisioner_harden_v1` after exec, check actual
   nondumpability/NNP/caps again and original clock, THEN FIRST credential read.
   Exact original private16B frame, username1..64/realm1..64/password1..128,
   exact EOF, one shared2048 recv/read/poll call budget; ingress slot clear/close
   once. No parent payload reader/encoder or default/secret fixture exists.
7. REAL unchanged private handoff create repeats actual hardener → actual startup
   constructor → Digest HA1 → maintained Mbed TLS SHA256/explicit legacy MD5 →
   exclusive fixed-name0600 file/write1536/fsync/writer close/readonly reopen and
   metadata checks. Caller-owned password/payload/header scratch clears immediately
   on helper return, including failure. Encode explicit1392-byte public semantic
   reply (no raw struct padding), then REAL handoff_move into the ACTUAL local
   config using ACTUAL local CANDIDATE_PRESTARTED_INIT. Nothing impersonates parent
   owner/COW state. ONE SCM_RIGHTS send, sender FD/control/directory close-once,
   full owned secret scratch wipe, _exit0 ONLY on known success. File opens may
   return0/1/2 after stdio closure; actual slots are handled normally.
8. Parent observe stores received rights before validating exact reply, then
   re-encodes/compares full graph/routes/public snapshot and original exact config/
   callback/context/graph-address/unused-owner/image identity. Readonly CLOEXEC
   NONBLOCK root0600/nlink1/1536B/offset0 FD must match actual fixed filename in
   same private directory and reported device/inode. No material payload read.
   Actual OWN child WNOHANG wait0 + exact reply + unchanged inputs/metadata + all
   known closes → READY. At explicit `qc07f_rtsp_exec_bind_v1`, repeat full public/
   identity/metadata/clock checks, then MOVE the actual received slot into original
   unused cfg.material_fd. No auto prepare/start, forged h/old owner fields/casts,
   native permission Boolean or kernel/media operation.

## Failure custody and actual limits

Before fork, any failure retains moved ingress, actual recorded duplicates/pair
and borrowed directory/image/config/owner namespace. Successful fork records known
child before effects. Postfork alias closes all have separate sticky mask bits;
unknown close clears its canonical number BEFORE attempt, never retries it.
HOLD preserves actual remaining slots, queue/received refs, possibly linked file,
child/dir/image borrowed namespace/external root. No rollback/kill/unlink/reset/
destructor. HOLD observation can collect ONLY still-known actual unreaped child;
unknown wait/ECHILD is sticky and never numerically retried. Exact response alone,
metadata alone or child exit alone cannot permit bind or cleanup.

Controlled worker failures wipe owned scratch and exit nonzero; residual child
refs retire at exit, possible file remains linked. No guaranteed wiping on crash/
SIGKILL/physical memory or while fsync blocks. No secret payload/hash/readback/debug
output; no live credentials used in tooling. Parent public image hashing is NOT
material hashing. Actual shared syscall budget/original nominal clock admission
does not provide finite syscalls/fsync/suspend/host-loss recovery or strict C1.

Zero caps UID0 retains owner-DAC authority. This is NOT hostile-code filesystem/
syscall isolation or prevention of future arbitrary root-owned file access. No
new seccomp filter and no weaker verifier/authority/parent profiles. Existing
inherited filters are unsupported/fail closed. Native behavior remains uncalled.

## Explicit remaining gaps / no next execution

* Independently reviewed trusted root-owned correct worker image/policy and stable
  inode/ancestor provenance: NOT supplied. Built user-owned ELF is NOT eligible.
* Native caps/securebits/exec/close_range/syscalls/static ARM instructions/libc/
  locking/stack/2MiB and <=128KiB whole-process allocation/load fit: OPEN, not
  established by object sizes/LOAD sums. New source itself has no heap requests.
* Media parent's all-domain secret-free history/clean launch is NOT proved by
  this worker exec. External writer authorization/ingress exclusivity and no
  threads/native/media/aliases/mutations/reapers are trusted launch preconditions.
* Durable authenticated generations/storage/rotation/main/release/entropy and
  surviving backing/external recovery/C1 remain OPEN. File fsync is not durable
  credential storage or erasure. Keep nlink1 for future actual backend checks.
* API5 accepted bootstrap still embeds OLD fork-only provisioner TYPE and calls.
  No adoption here: future reviewed fresh typed derivative must change actual
  ownership inspection/calls, never fill/cast old owners. Child wait is not
  native/backing/material retirement or whole-process exit permission.

No normal/negative/adversarial/mutation/fuzz test or helper/default-help run. No
worker/function/fork/exec/UID/GID/caps/securebits/seccomp/env/FD/hardener/constructor/
credential/material/retirement runtime operation was executed, including HOST.
23h OFF remains historical, not a fresh lease; RAM75 recovery/C1 blocked, Query78
parked, all failures/HOLDs preserved. No next queue/self-GO/hardware phase.
