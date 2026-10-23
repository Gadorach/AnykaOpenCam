# Separate provisioner process — author handback

2026-10-23 · SEC-01/03/05/11, STREAM-07.
**Actual separate-child feature delivered. HOST/ARM compile PASS; ONE normal
HOST PUBLICDUMMY child/SCM_RIGHTS/wait/bind case PASS. Parent independent review
is next. No native hardening, real credentials, verifier/media launch or adoption.**

## Actual bridge, not a C-slot helper renamed IPC

`apps/rtsp-auth/rtsp-provisioner-process/provisioner_process.{c,h}` forks exactly
one device-free provisioner before any secret read. Parent moves its preopened
trusted O_RDONLY/CLOEXEC/nonblocking ingress-pipe slot into a stable one-use owner
and closes its alias once after successful fork; parent never reads ingress.
Actual child native route calls the reviewed real hardening helper BEFORE input:
root/saved IDs, no supplementary groups, core0, nondumpability, NNP, current/future
mlock checks. This native route was compiled, **NOT RUN**. Original create repeats
hardening. Native partial hardening failure is terminal, not rolled back.

Child reads one versioned bounded little-endian pipe frame (16-byte header,
username1..64, realm1..64, independent RTSP password1..128 bytes, exact EOF), with
finite shared2048 read-attempt loop budget and nominal monotonic deadline. EINTR
consumes loop budget; nonblocking EAGAIN polls1ms. Clock/syscall/scheduler stalls
are not bounded by that nominal budget, and there is no native watchdog/recovery
guarantee or new clock framework. Fixed child input/material scratch <4096B;
recorded ARM child function frame3712B is not a total-stack/native load budget.

The actual reviewed `private_handoff_create` invokes the actual startup constructor
and original Digest/Mbed TLS, creates the exclusive1536-byte fixed-name file,
fsyncs, closes its writer once, owns/checks a real readonly FD. Child calls the
actual `private_handoff_move` with its actual h object, fork-COW copy of the same
original config address, and actual fresh prestarted owner. This changes ONLY
child config.material_fd. It never fabricates h fields, a config, permission token,
callback owner or ready object for the receiver.

Actual sendmsg transfers one SCM_RIGHTS descriptor with an exact1392-byte public
envelope over an owned private Unix CLOEXEC/nonblocking SOCK_SEQPACKET socketpair
created BEFORE fork. That duplicates a kernel reference; it is NOT atomic move.
Child clears its config slot before its ONE sender-reference close, closes its
socket/directory aliases, crypto-wipes its entire owned input/material scratch,
then `_exit(0)`. All failures wipe then `_exit(nonzero)`, never return into parent.
Exit0 is reachable only through that success sequence; the fixture observes owned
wait0, not direct inspection of dead child memory or an independent wipe sensor.

Parent `recvmsg(MSG_CMSG_CLOEXEC)` stores all actually delivered rights BEFORE
checking fixed packet length, truncation, exact single control/FD, public binding
and real inode metadata. Its bounded control buffer can deliver at most8 FDs; all
are represented on malformed/truncated reception (undelivered excess references
are kernel-discarded). There is no response JSON/readiness ACK treated as exit.
Actual waitpid targets only its own still-unreaped fork child, WNOHANG, one call
per observation. EINTR is pending; ECHILD/other unknown is sticky and NEVER retried.

Parent READY needs known wait0, known closes, unchanged public config/graph/routes
binding and actual fresh owner, plus rechecked regular root-owned(native)/fixture-
owned0600 nlink1 size1536 O_RDONLY CLOEXEC offset0 inode/name/directory association.
Public envelope binds ABI/class/domain/account/algorithm/generations/limits/device/
account/sensor/profile/routes and child-created inode; it contains no credentials,
HA1, credential payload lengths or credential-derived hash. New parent code reads
NO material contents, not even its prefix. Trusted actual constructor + unchanged
fork snapshot support content binding; metadata alone cannot prove those contents.
The original backend and verifier retain their own actual checks when later used.

## API / custody

| API | Actual effect and ordering |
| --- | --- |
| `rtsp_provisioner_begin` | ONE process-local attempt, real canonical ingress-slot move, preflight, private socketpair, ONE fork, known parent closes. Default symbols are `qc07f_rtsp_provisioner_*_v1`. Returns MORE or sticky HOLD, never a config FD. |
| `rtsp_provisioner_observe` | Constant bounded work per call; nonblocking receive and own-child wait observation, monotonic nominal admission limit1..10000ms. No automatic kill/close/unlink/retry. HOLD calls may collect still-known own child but can NEVER clear HOLD or activate. |
| `rtsp_provisioner_bind` | Explicit only at READY with same original unused config and same actual fresh prestarted owner; rechecks and moves received FD into real parent config.material_fd. Call immediately before caller's existing device-free prepare/start; module invokes neither. |

Object uses `RTSP_PROVISIONER_PROCESS_INIT`, stable caller storage, self-address and
single process attempt. Fresh prestarted owner is the actual existing
`CANDIDATE_PRESTARTED_INIT` object which caller will later start, not a surrogate.
For admission failure after a valid fresh begin, moved ingress remains in owner
if preflight/fork failed; caller slot stays-1. Received rights, socket queue,
possible partial fixed-name file, borrowed directory, own child and uncertainty
remain represented on failure. No reset/destructor/unlink or numeric close retry.
Close error clears canonical slot before one syscall and retains sticky uncertainty
mask, NOT a retryable FD. Nonzero child wait never admits config. Success moves FD
out of receiver owner; config then owns it even if later prepare/start fails.

Mandatory caller preconditions are in CONTRACT.md/header, NOT inferred from flags:
trusted authorized independent RTSP ingress/external writer; actual secret-free
parent before fork; no native/provider/media/guardian/backing/listener ownership;
no prior prepare/start ANYWHERE; immutable live config/graph/routes/owner addresses;
single-threaded/no extra fork/reentry/signals/FD aliases/external reaper; trusted
inherited-FD/environment/capability policy; exclusive trusted0700 namespace and
writer exclusion through future verifier consumption/retirement. Directory FD is
BORROWED and must outlive all owner/HOLD/consumer states. File remains linked:
unlink-before-consumption would violate the actual backend's nlink1 requirement.
These conditions are deliberately exposed; no passBool claims to establish them.

## Build and sole functional observation

Read/audited real dependencies and complete new C/harness/recipe before invocation;
see AUTHOR_AUDIT.md. Original builders/helpers/generators were not executed.
Exact short compile-only recipe (refuses existing build01):

```sh
env -i PATH=/usr/bin:/bin LANG=C LC_ALL=C TZ=UTC /bin/bash apps/rtsp-auth/rtsp-provisioner-process/compile_recipe.sh
```

2026-10-23 11:22:56.902618–11:23:01.109083 UTC, exit0:

- **27 C compilations** (12 HOST native-shaped,12 retained musl ARMv5 native-shaped,
  3 separate HOST PUBLICDUMMY units), all27 compile logs empty.
- **40 successful tool commands**, including two REL combinations, one HOST-only
  dummy executable link and versions/machine/ELF/symbol inspection. No ARM/native
  executable/image linked/run; ARM EABI5/v5TE REL. HOST GCC16.2.1, ARM GCC14.3.0,
  `-Os -march=armv5te -marm -mfloat-abi=soft`. Existing parser's isolated warning
  exceptions only; no new process/handoff/test warning waived.
- Real frozen constructor/handoff, Digest/wire, graph/registry/parser and four
  Mbed TLS units compiled. Actual candidate_config/prestarted/verifier headers
  consumed, no stubs; full backend/runtime/adaptor was NOT built or executed here.
- `runs/build01/REPORT.json` records exact argv/environment,40 logs,84 artifacts,
  **1771 consumed source/header dependency records /277 unique paths**, invoked
  tool/real ARM compiler pins and source context. This is not full compiler closure,
  native ABI pairing, release packaging or whole-program memory acceptance.
- All33 selected source/context pins matched after build; original files unchanged.
  REL undefined symbols are libc/compiler support, not missing camera/crypto APIs.

Explicit `RTSP_HANDOFF_HOST_PUBLICDUMMY` changes exported process AND handoff
symbols/class, cannot bind native config, and excludes native hardening. Symbol
inspection preceded the ONE permitted call; no native hardening, verifier adapter,
runtime/provider/listener/network code was linked in that executable. Constructor
uses original reviewed HOST handoff's temporary native-shaped copy and explicit
PUBLICDUMMY account, never native account1/default credential adoption.

**ONE normal functional invocation**,11:23:43.449618–11:23:43.452860 UTC, exit0:
actual fork → child pipe read/exact EOF → original constructor/exclusive write/
fsync/known writer close/readonly move → real SCM_RIGHTS → known sender close/
owned-scratch wipe control path/child wait0 → explicit original parent config bind.
Readonly1536/inode/flags/offset/fresh owner checks passed; config remained empty
through READY until explicit bind. Test-only single close, inode-matched unlink,
directory close and empty-directory rmdir passed. Scalar stdout270B, stderr0B;
details in `runs/positive01/OBSERVATION.json`. No payload readback/hash/dump.

Dummy frame/constants were prepared in parent, clearly PUBLICDUMMY-only: this is
**NOT product secret-free-fork proof**. Native hardening, root isolation, private
operator credentials, secret-producer authorization and future verifier use are
NOT RUN. No further normal, failure, negative, adversarial, mutation or fuzz run.
Fixture failure path performs no cleanup; if retirement is unknown it explicitly
retains a live static keeper, not a stack HOLD returned as finished. None occurred.

## Pins, remaining integration, next action

- C SHA256 `d929b392110d6563b818aa00993301e07d5542afb3a2f4c37cafebe6f28b3724`.
- Header SHA256 `7208455b184a4a2ed9384b16cff1a05c17217789189ef3fa4812c397ff817bc6`.
- Build report SHA256 `49115dc14df26b9b2372a4f00a9b6e0b46b40409960ef28344108fb3bd45c283`.
- SOURCE_PINS.json is the33-input prebuild selection; DELIVERY_PINS.json pins the
  public handback and generated manifests/logs. No credential/material digest.
- New source GPL-2.0-only; original notices/Mbed TLS dual-license preserved in
  LICENSE_NOTICES.md. Objects/ELFs/generated files are local evidence, not release
  or Git-staging approval; no Git/index/central-plan/AGENTS edit or publication.

**Next: parent independent ordinary source/API/custody review before adoption.**
This library does not provide real authorized external credential ingress/launch,
global FD/capability/environment hygiene, durable generations/storage/rotation,
private linked-file retirement/start-failure disposition, actual bootstrap/main,
external media owner/recovery/backing,128KiB-compliant media buffers/combined40MiB
load, native isolation or authenticated PLAY/decode/finite STOP. No product/SEC/G3/
G8 acceptance, main, installation, hardware/native recipe, kernel/image change or
listener/media execution. No device/proc/locks/private config/captures/RPC/network
or hardware access occurred. Parent23g failure/current state were not inspected or
modified. Only the assigned app/doc directories were written.
