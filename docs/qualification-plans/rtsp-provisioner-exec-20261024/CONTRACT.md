# Clean-exec provisioner candidate — selected SOURCE contract

2026-10-24 · SEC-01/03/11, RP-08; SEC-05 dependency. Written before candidate
code. Selection is the parent task in this conversation, superseding the design
draft's documentation-only selection, not its safety requirements. **NOT RUN,
NOT ADOPTED; independent ordinary review and parent disposition required.**

## Inspected feasibility and bounded implementation

Read the complete design FEASIBILITY/CONTRACT_DRAFT and actual provisioner-process,
private-handoff, startup-material, candidate config/prestarted graph, verifier
adapter/isolation and graph/routes/Digest validators; prior parent dispositions
for process, handoff, constructor and API5 writer-bootstrap. Existing helpers
require real/saved UID/GID0/groups0; neither old fork-only child nor nonroot
verifier sandbox is a worker entry. Existing inherited seccomp is rejected, never
weakened. Frozen source and accepted API5 bootstrap remain unchanged.

Three inseparable classes in the fresh candidate directory:

1. `public_codec.[ch]`: explicit LE request (1374-byte prefix plus 144/128/204/72
   counted graph records, max7934), local typed reconstruction, pre-secret actual
   graph/routes/rate validation and explicitly encoded1392-byte semantic reply.
   Canonical NUL/zero tails, counts/enums/lengths/nominal1..10000ms original clock
   budget checked. Whole graph preserved. No pointers, verifier image or secret
   values/lengths/derived hashes on the public channel. Parent deep re-encoding
   supplements the exact original config/routes/graph-pointer/unused-owner snapshot.
2. `worker_entry.c`: actual main, fixed argv/empty env, FD3 control/4 ingress/5
   directory only. Verify real post-exec root IDs, zero capsets/ambient/bounding,
   locked no-root-regain bits/core0/NNP/no inherited seccomp, role metadata and
   still-live socketpair creator parent; restore CLOEXEC, receive/validate public
   request, call actual private hardener before credential reads. Original private
   frame/exact EOF/shared2048 read-poll budget then actual handoff/create/constructor,
   local CANDIDATE_PRESTARTED_INIT handoff move, one reply+readonly SCM_RIGHTS,
   close-once/wipe/exit0 only on known success. No secret readback or unlink.
3. `launch_security.[ch]`, `exec_owner.[ch]`: separately typed self-stable parent
   one-attempt owner; ingress move before fallible admission; independently supplied
   worker-specific public image policy and stable readonly root-owned nonwritable
   non-setid nlink1 static ARM EABI5 softfloat ET_EXEC, checked full hash/ELF/load
   bounds/entry/no interpreter/dynamic/RWX/execstack and metadata recheck. Stage
   collision-free >=16 descriptors in owned slots before fork. Child-only securebits,
   core0/NNP/nondump, inherited sufficient bounded memlock/2MiB address-space ceiling,
   ambient/bounding/capset removal, fixed3/4/5+image6 mapping, checked stdio/tail
   closure and one SYS_execveat attempt. No fexecve/PATH/fallback. Parent close slots
   before close, sticky uncertainty; receive ALL installed rights before refusal,
   require exact bindings/metadata/known OWN child wait0/all known closes before
   READY, explicit bind only to original config. HOLD retains everything uncertain;
   only still-known child observation remains allowed, no cleanup/kill/retry/reset.

The image API accepts a dedicated preopened FD and independently trusted reviewed
public expected bytes/SHA256, not a self-generated admission pin/default path or
the verifier image policy. Stable root/ancestor/namespace provenance and correct
worker-image association remain caller requirements; offline artifacts are not
eligible root-owned admitted images. Reject descriptor/inode aliases with ingress,
directory and actual parent verifier image. No constructor image descriptor is
claimed verified: local projection sets image ABI1/native and fd=-1 only.

## Preconditions and limitations

Trusted disjoint final-address objects; original public graph/config/routes and
callbacks remain immutable, sole thread/default SIGCHLD/sole reaper/no signal
reentry/later fork/FD mutation/aliases. Launch before incompatible sandbox and
before ANY native/media/provider ownership, not closure as fake DMA retirement.
External authenticated independent credential writer, exclusive ingress and root
private0700 directory/ancestors are mandatory external facts, never Boolean proof.
Borrowed image/directory/config/graph/unused owner and external root survive HOLD.
SO_PEERCRED is the socketpair creator parent, NOT evidence of the fork child's PID.
Parent wait uses only its actual unreaped fork result. No parent credential reads.

Root UID0 with ZERO capabilities still has owner DAC, not a hostile-code syscall/
filesystem sandbox. No new seccomp profile or changes to existing hardeners.
No retained CAP_IPC_LOCK/SYS_RESOURCE/KILL. No hard-limit increase or >2MiB ceiling;
actual static image/mlock/stack/libc/process fit and all native APIs remain OPEN.
No allocation request >128KiB; no heap needed by new source. Core/dump/mlock state
is re-established by real hardener after exec, not assumed to survive exec.
Nominal deadlines do not bound fsync/syscalls/suspend/host loss. Controlled paths
wipe owned scratch; SIGKILL/crash/physical-memory wiping is not guaranteed.

## Offline evidence and stop rule

Fixed fresh refuse-overwrite build01 only: compile actual new units HOST and
retained musl ARM with production defines, actual reused constructor/handoff/
graph/routes/rates/Digest/Mbed TLS source; separate parent REL without main and
worker REL with actual main. Optional real static ARM companion ELF, never run,
staged, installed, chmod/chown or admitted. Record exact commands/tools/dependencies/
hashes/logs/maps/symbol and relocation indices, honest libc REL unresolved symbols
and required HA1 -> hash -> SHA256/MD5/zeroizer chain. No old combined REL/child/
runtime/verifier-engine/stubs. First preflight/recipe/compiler/link/inspection
failure is preserved and STOP; no correction/retry/build02. Ordinary source
discovery before the build is not a build attempt. No test/helper/default-help,
candidate/function/constructor/fork/exec/credential or hardening execution.

Global media-parent secret history, trusted launcher, authorized writer, durable
generations/storage/rotation/main/release, worker image provenance, native caps/
syscalls/memlock/2MiB/entropy/backing/external recovery/C1 remain OPEN. No API5
integration, hardware queue, next self-GO or full-product acceptance.
