# Private RTSP provisioner → bootstrap descriptor handoff

2026-10-23 · SEC-01/03/05/11, STREAM-07 · source candidate, parent review required.
Only the fresh app/doc directories in this lane may change. No native effects,
Git/index changes, main, listener, capture, watchdog, image or live config update.

## Trusted boundary, not a permission token

Use only in a separately hardened, authenticated provisioning context, BEFORE
media exists. Native is root UID/GID 0 with no supplementary groups, matching the
actual backend. A separate real hardening function sets/checks zero core limits,
nondumpability, no-new-privileges and locks current/future memory; call it before
secret ingress. Creation repeats these operations/checks, never accepts a permit
Boolean and never drops IDs/capabilities inside a media process. Partial hardening
failure is terminal to this provisioning attempt, not something to undo/retry.
The caller still implements real authenticated private ingress, authorization,
process separation/launch, environment/FD hygiene, capability policy, password
zeroization, durable generation authority and protected storage. A C function
cannot prove those conditions. No argv/env/path/command/PID/credential selector,
discovery, RNG/default password, UI/HTTPS credential reuse or network ingress.

Caller supplies an already exclusively owned private directory FD (borrowed),
current immutable config/graph/registry and bounded borrowed RTSP credentials,
plus disjoint writable material scratch and a fresh stable custody object.
Directory must be the intended trusted inode, owner root and exact mode0700.
Its ancestors/namespace are already trusted and remain serialized/quiescent:
no symlink/hardlink replacement, alternate writer/open alias, dup/SCM/mmap/fork,
threads/signals/reentry or FD reuse. Flags and metadata do NOT establish these
whole-interval prerequisites. Namespace/FD ownership lasts through final move.

## Actual one-shot feature

Build the original reviewed1536-byte native account1/720p record through the
original constructor and Digest/Mbed TLS helper, explicit SHA256 or legacy MD5.
Never log/read back/hash/manifest credentials or material. Module-fixed filename;
exclusive O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC mode0600 writer, no overwrite/recovery.
Check regular type, owner/mode/nlink1, saved dev/inode and size before/after
bounded1536-byte writes (EINTR counts toward the finite syscall budget). File
fsync is mandatory; directory fsync is deliberately NOT a durability guarantee:
this is one-start handoff, not a power-loss-safe credential store.

Move writer FD out of its owner slot BEFORE exactly one close call. Any close
error means sticky WRITER_CLOSE_UNKNOWN: no close/reopen/overwrite/unlink retry,
no readonly open, preserve partial file/state. Ordinary write/fsync/metadata
failure also preserves the created file; writer still gets just that one close.
Only known successful close of a complete checked/fsynced writer allows readonly
O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC open. Preserve the readonly slot immediately on
open, even if subsequent validation fails. Recheck directory, named inode and
opened inode, owner/mode/nlink1/exact1536 size, readonly access, CLOEXEC and offset0.
No implicit cleanup or unlink; partial files/owned FDs/uncertainty remain owned.

Final explicit one-shot move rechecks metadata/identity and the same still-unused
config, exact original address, material slot -1, and actual fresh prestarted
owner fields. Caller MUST also ensure owned-runtime prepare/start has never run,
no other object references/uses config, no media/last-fork state exists. Checks do
not prove that global ordering. Move clears custody FD before assigning config;
no duplicated ownership. Afterward bootstrap owns config.material_fd across all
start failures until independently reviewed explicit cleanup/retention. No
automatic close/overwrite, backend call, readiness or successful adoption follows.
All material scratch is crypto-zeroized on every return (including API refusal);
borrowed password is never mutated and must be separately wiped by caller.

## Compile/test separation

Default build exports native-only symbols. Explicit
RTSP_HANDOFF_HOST_PUBLICDUMMY exports DIFFERENT symbols, accepts only HOST fixture
config and nonroot matched IDs, uses a local synthetic native-shaped config solely
to call the frozen constructor, then replaces numeric account with PUBLICDUMMY.
It cannot move into native config and cannot meet native root/image/hardening
route. It is not a runtime fallback or product credential/default. Only one
normal HOST fixture call is authorized: actual constructor/write/close/readonly/
move, scalar metadata assertions and explicit test-only cleanup in a fresh private
/tmp/opencode directory. No payload readback, launch, network or negative suite.
Production constructor on real credentials and all native use remain NOT RUN.
