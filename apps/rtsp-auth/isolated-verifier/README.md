# Isolated RTSP HA1 verifier adoption candidate — SOURCE / HOST ONLY

SEC-05/06/11. Parent review required. **No native backend/listener/media/producer
enablement or G3/G8 acceptance.** V3/server/core/framework/HTTPS/TLS/launcher/
vendor/credential/source-worker files are read-only and unchanged. This process
has no source/media/DMA/codec/radio/UART/device/NVR/cloud/ONVIF interface. Later
native mapping follows parent handoff retirement + source/memory readiness.

`PROVENANCE.json`/`authority/` pin byte-exact canonical HTTPS credential child,
seqpacket codec, owned-PIDFD/process supervisor, fixed native launcher and repaired
parent signal filter. SPDX/upstream notices preserved. RTSP adaptation removes
UI/admin/PBKDF2/material selectors, uses real existing MbedTLS3.6.7 in the separate
readonly RTSP crypto configuration, and retains canonical hardening/lifecycle
discipline. No TLS MD5 enablement, no UI600k/2MiB changes or dependency installs.

## Strict native boundary and explicit HOST class

### V2 parent-found launch/image/material entry correction

Reviewed verifier07 V1 source/authority/REPORT/diffs/errors are snapshotted before
editing under `evidence/delegated/20261012-rtsp-verifier/v1-snapshot/` (258 files).
The child now waits on a fixed pre-exec release byte on the **existing owned
seqpacket**, with PDEATH armed first. Parent acquires the actual immutable pidfd,
receives only the explicit PDEATH-arm marker (NOT READY/provisioning), checks its
known child is still alive, and only then releases exec. Acquisition/release/EOF
failure closes that barrier and observes the exact owned child's wait receipt;
there is no numeric-PID kill fallback. Unknown retirement retains the client.
No child can exec/harden/read material or send engine ACK while held. This is one
launch ordering fix, not a daemon/guardian/new transport phase or budget change.

Native image gate now requires root-owned regular nlink1 **nonwritable** public
read+execute inode (0555 or suitable0505), explicit immutable reviewed IMAGE bytes
pin, ELF32 little-endian ARM EABI5 softfloat/static, no interpreter/dynamic segment
or executable stack. Root0500 is not executable by the planned UID1000 and fails
before fork. Caller pre-stages a verified readonly public code sibling outside
private secret0700 scope; adapter NEVER chmods/chowns supplied image, adjusts pins
or executes with root privilege to work around permissions. Fresh build outputs
alone are finalized0555 by the recipe. HOST4MiB is a separately pinned correctly
formed x86_64 class with actual child execute permissions. ELF/hash gate is not
native ISA/kernel execution proof; existing Thumb-2 metadata caveat remains.

Startup material regular FD must be private readonly nlink1. Anonymous nlink0 is
allowed only with COMPLETE immutable write/grow/shrink/seal seals. Readonly FD
mode/stat metadata cannot prove parent closed other writers; trusted bootstrap
must freeze/close writers and retain ownership. No credential payload hash or
globally immutable regular-file claim is made. Source-native C struct endian/
padding contract remains private same-source ABI, not public wire serialization.

Native child requires nonroot identical real/effective UID/GID, no supplementary
groups, root-created owned SO_PEERCRED parent PID, preopened SOCK_SEQPACKET3 and
private startup FD4. It closes stdio and every unrelated FD, clears/verifies caps0
(no IPC_LOCK/SYS_RESOURCE), sets core0/nodump/no_new_privs/PDEATHSIG, fixes its own
MEMLOCK soft+hard to **2MiB**, locks current/future memory and installs copied
default-deny seccomp BEFORE material read. READY flags15 reflect actual checked
FD closure/nodump/lock/filter, not a supplied trusted flag. Forbidden empty-path
open/dup/ioctl/dump-enable probes must fail in kernel before dispatch. Canonical
ARM119 ordinary signal return fix is included alongside rt_sigreturn; no global
kernel/debug/driver configuration changed. No privilege before material access.

This session is UID1000 with15 supplementary groups, not a root creator. Strict
native startup therefore must **HOLD/fail closed without reading material**, even
if HOST memory happens to fit. No sudo/root/account change is inferred. Separate
compiled **HOST PUBLICDUMMY4MiB** profile still enforces real caps0/core0/nodump/
mlock/NNP/seccomp/FD closure/PDEATH, but accepts its actual same-UID creator and
does NOT claim native root-origin/groups0/budget proof. It is not an automatic
memory raise/fallback or deployable native image. HOST libc may fail4MiB too;
that is a concrete HOLD, never a fabricated functional isolation result.

Only PUBLICDUMMY fixtures create a sealed in-memory startup FD—no real credential
creation, disk persistence or plaintext password IPC. The trusted RTSP-only record
contains exact domain/profile/realm/username/algorithm/generations/independent HA1,
fixed live+track resource registry and explicit bounded rate configuration. Native
bootstrap ownership must be root; sealed FD is effectively immutable even if its
open mode is RDWR, or a private regular FD must be readonly. Exact size/mode/owner,
offset0 and seals validated before read; material read occurs ONLY after flags15.
Child keeps core HA1, wipes duplicate material/frame/scratch/result state; input
FD closes after provisioning. Parent owns and must close/wipe its private copies.
Adapter startup requires a second exact epoch/generation provisioning ACK after
actual child state creation; flags15 alone never fabricate a functional backend.
No material/username/Authorization/nonce/response/token/cookie values or generated
credential payload hashes enter logs/artifacts; permitted hashes are source/image/
compiler/runtime/scalar evidence only. Ordinary RTSP/TCP remains unencrypted.

## Child custody / bounded IPC / server-side adapter

Canonical explicit endian seqpacket codec adapts to80-byte header +8192-byte max
request metadata payload, rejects truncation/unknown version/ancillary and closes
unexpected SCM_RIGHTS. No full-AU buffering. Same epoch/sequence/connection/scope/
generation/deadline bind every command/result. Opaque child-issued peer IDs and
grant handles live on the private descriptor, not public bearer/client flags.

Child holds one fixed RTSP account/profile, four admitted peer buckets, persistent
global/account counters, eight random server nonces/global nc replay, grant and
real shared-policy state. Peer churn cannot reset account/global budgets. Actual
raw Request-Line and Authorization are parsed by existing bounded parser/wire,
URI mapped to exact live/track (OPTIONS* scoped), actual Mbed Digest precedes a
trusted VIEWER/PROFILE/LIVE/domainRTSP policy fact. HA1 is not UI PBKDF2; SHA-256
and explicit MD5 legacy only, no Basic/qopless/sess/implicit MD5/userinfo fallback.
No arbitrary PID/command/path/account/secret-read selector is supplied by a client.

One outstanding verify/result slot is authoritative in BOTH child and adapter.
Parent abandon/discard immediately demotes any success but stays BUSY; no second
crypto slot until real completion/scratch wipe and exact bound RETIRE_RESULT ACK.
Malformed outer requests consume a child-owned parse admission as well as the
additional wire/crypto admission on valid frames; no uncharged malformed/churn
path is provided. Fixed owner revoke demotes adapter effects immediately even
while a result is pending; drain remains BUSY and the exact control is sent only
after result retirement. A failed owner RPC remains fail-closed, never re-enables.
Separate HOST delay binary performs actual crypto after bounded200ms delay to
prove discard/drain, not a match hook or native timeout extension. Fixed-account
owner REVOKE invalidates core/policy/current handles and advances nonwrapping
generation; no arbitrary account selector or dynamic user registry.

Every deferred stream continuation must call `rv_query` before effects. Child
rechecks exact peer/grant/resource/generation/core nonce expiry and shared policy;
adapter verifies bound reply, fresh local monotonic expiry and owned child liveness.
No cached verified/TTL permissive fallback. Rejection/hardfault invalidates use;
producer/hub/CPU lease retirement remains separate and entirely parent-owned.

Adapter accepts preopened image/material FDs only, authenticates IMAGE size/hash
against an explicit trusted reviewed pin (not a credential payload hash), stages
FDs high to avoid dup alias and execs fixed argv/empty env. Sole default-SIGCHLD
wait owner prevents PID reuse before pidfd acquisition; later signals target ONLY
immutable owned kernel pidfd, not numeric PID or network selector. Parent death
kills child. Normal SIGTERM is a courtesy: child actually wipes/self-checks state,
closes and sends bounded BYE; parent must observe owned waitpid retirement. SIGKILL
fallback retires address space, not a claimed observed user-space wipe. No automatic
restart. Unknown retirement retains client/ownership; no free-on-SIGTERM-request.
HOST tests exercise actual kernel parent-death kill via an owned creator process,
duplicated OWN pidfd test hook and sole subreaper wait lineage. That test-only
pidfd duplicate is excluded from production ABI; no secret IPC FD is exported.
All C calls/FD/object lifetimes serialized/disjoint; no hostile-pointer guarantee.
The RTSP child filter is a narrower canonical subset: no clone/clone3/tgkill
worker-thread/process permission, because exactly one synchronous Digest job is
used. No UI-thread policy or source authority file is modified.

## Reproducible tests/build and native HOLD

Fresh outputs only under `local/validation/rtsp-verifier/<new>`; overwrite refused.
ARMv5 soft-float static build links with existing pinned read-only toolchain and
NEVER executes ARM ELF. Compiler/header/library/runtime/source pins, notice copies,
adaptation diffs and original failures are retained. Actual HOST hardening tests
must not bypass seccomp/mlock or silently relabel the native2MiB class. Sanitizers
cannot lock their giant shadow VM into4MiB: use real hardened optimized child in
sanitized adapter tests, plus separate ASan/UBSan pure engine/codec qualification;
no claimed sanitizer hardening success from an unlocked child.

Owned public IPC tests only; no proc/fuser/current hardware status, real private
configuration/keys/ROM/photos/footage/SD/guest/radio/network camera/NVR. Native
root creator/groups0/cold entropy/static2MiB/private bootstrap and actual syscall/
signal/hardfault readiness stay **HOLD until parent directly qualifies them**.
Do not assume the whole3.8MiB RTSP producer fits a2MiB locked verifier: separate
minimal process and secret namespace are mandatory. No new guardian architecture.

**ONE parent dependency:** independently review IPC/secret/isolation/lifecycle
diff and prove native strict2MiB/rootcreator/groups0/READY15 + owned retirement
on the separately prepared native image BEFORE integrating the private query
adapter into the existing per-effect/source/CPU-lease chain. Product media501,
RTSPNOAUTH/native/G3G8 remain unchanged until parent proof.
