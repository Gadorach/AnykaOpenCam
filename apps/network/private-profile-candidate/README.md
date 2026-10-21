# Private-profile crypto/store source candidate — NOT a secure native backend

SEC-07/08/09 and SETUP-04/05 dependency, offline isolated lane. **NATIVE is HOLD
before any key/secret file read.** Only explicit `PP_PUBLIC_DUMMY_BUILD` enables
HOST_PUBLICDUMMY_4M. No existing metadata/settings/network/policy/auth/vendor/kernel/
plan/runner file changed. G6/G8 remain open; T34/Fulhan deferred.

**V2 source-review correction:** v1 compared files to cached admission checkpoints,
not live authority, and could accept replayed authenticated files in an already-open
session after source state changed. V2 requires a live scalar authority getter at
private-read/crypto/callback/publication boundaries, rejects mismatches and wipes
keys instead of refreshing expectations to force success. Preserved v1 source,
686-check report/build/log evidence and failures are in
`evidence/delegated/20261012-private-profile/v1/`. Private fixture payloads remain
untouched in place and were **never read/hashed/copied for preservation**.
Wire formats384/848/80 and public metadata cap1024 remain unchanged.

## Actual implementation and separation

`private_profile.c/.h` contain a bounded SOURCE_PRIVATE profile codec, real
Mbed TLS **3.6.7 AES-256-GCM + HKDF-SHA256**, authenticated HMAC-SHA256 nonce ledger
and bundle, and anchored POSIX write-ahead replacement store. `pp_mbedtls_config.h`
is a new minimal independent config: AES/GCM/cipher/SHA256/MD/HKDF/platform memory.
Own fresh objects are compiled separately for optimized/sanitized/ARM modes from
the pinned canonical vendor, read-only. **Never link/reuse HTTPS/RTSP crypto
objects or use this allocator in their process.** Original vendor license and
source notices are preserved; dependency pin/closure are recorded. Our source is
GPL-2.0-only; no workspace-wide redistribution grant is created.

No handwritten crypto, CRC-as-auth, key creation/derivation from MAC/IP/serial/
image, public password/default PSK or fallback open AP. HKDF starts from the
caller-provided32-byte master key; IDs only bind its domain/AAD. No driver effects.

## Secret-read boundary and exact HOLDs

Supported sequence: fresh nonmovable session → actual host context checks/owned
region mlock → import **trusted pre-opened readonly regular owned0600/nlink1
exact32-byte key FD** → borrowed owned0700 store dirfd → private operations → close.
Native mode always returns HOLD, including in publicdummy builds, before fstat/
pread of the key. Non-publicdummy compilation also cannot open a session. Do not
reinitialize live storage, alias inputs/outputs/context, retain borrowed private
objects, reenter callbacks or concurrently use the global crypto allocator.

HOST checks real/effective nonroot UID/GID equality, actual bounded group set
(up to64; **not claimed empty**), actual empty effective/permitted/inheritable caps,
RLIMIT_CORE soft/hard0, actual nondumpability, NO_NEW_PRIVS and actual memlock limit
that fits owned storage and does not exceed4MiB. The source **does not raise
privileges/limits or set a caller-supplied verified/mlocked flag**. Direct kernel
`SYS_mlock/munlock` bypass sanitizer libc interceptors; actual success is required
before key reads. Checks repeat before private operations. Library only observes
hardening; the test process lowers its own limits/dumpability and drops its own
capabilities. Unprivileged inherited groups cannot be dropped; this is an explicit
HOST model limitation, not native closure evidence. Earlier group/capability HOLD
failures and zero-read guards remain preserved.

The locked owned region contains master/derived/ledger keys, plaintext/profile,
salt/nonce/tag/ciphertext buffers, GCM context and8×512-byte allocator cells.
Mbed TLS heap allocation is bounded to those locked cells. All working buffers/
GCM state/cells are zeroized after each successful operation; every failed private
operation destroys the session and zeroizes owned material before unlock. The
master persists only for its explicit owned session lifetime. Caller wipes its
original key/profile/codec buffers and any private-effect copy. Private callback
borrows expire on return. No secrets/ciphertext/salt/nonce/tag are public outputs.

**Not proven / mandatory native gates:** whole-process/native2MiB mlock/stack/
heap coverage, rlimits/FD closure, default-deny filter, prevention of later
dumpability changes, root bootstrap exact readonly private FD origin, PID/parent/
SO_PEERCRED, device key unseal and rotation, cold entropy, rollback-resistant
external key/nonce/commit checkpoints and physical backing-store flush/power-loss
semantics. Mbed TLS HKDF wipes PRK/T and contexts in pinned source; this does not
prove that all library stack/transient copies are locked/cleared. HOST has no
seccomp/full FD/group closure; the test changes dumpability and verifies fail-closed
rechecks, not prevention. No native readiness flag or secure-product claim.

## Versioned fixed private formats (explicit LE, no C padding/pointers/tickets)

### Plaintext PPC1 /256 bytes — SOURCE_PRIVATE, never public schema

0 magic,4 version1,8 auth1 (WPA2 passphrase only),12 IPv4 mode1 DHCP/2 static;
16/20/24 SSID/PSK/hostname lengths;28 IPv6 policy0 only;32 address,36 prefix,
40 gateway,44/48 DNS;52..63 zero;64..95 SSID32;96..159 PSK64;160..223 hostname64;
224..255 zero. SSID1..32, passphrase8..63 printable bytes, hostname1..63 bounded
ASCII labels, unused tails zero: no empty labels, leading/trailing hyphens, trailing
dot or non-alphanumeric/hyphen label bytes. Each label and total name <=63 bytes.
Static unicast address/prefix/gateway consistency checked; gateway zero permits a
local LAN, nonzero gateway cannot equal self/network/broadcast for prefix<31.
/31 deliberately permits zero or the other same-prefix endpoint; /32 requires
zero gateway. DHCP has no static fields. IPv6/unknown auth/extensions rejected.
These are **basic syntax/arithmetic checks**, not DNS/IP allocation/conflict/route,
radio/capability/reachability or usable-WLAN qualification. SSID remains bounded
arbitrary octets; driver/regulatory/private-profile policy is external.

### Envelope PPR1 /384 bytes — SOURCE_PRIVATE

Header128 +ciphertext256. Offsets0 magic,4 version1,8 algorithm1 AES256-GCM,
12 header128,16 plaintext256,20 ciphertext256;24 scope,32 owner epoch,40 key epoch,
48 public opaque profile ID,56 global nonce revision (all u64);64 salt16,
80 nonce12;92 class HOST_PUBLICDUMMY1;96 tag16;112..127 zero. AAD is the entire
canonical header with tag bytes zeroed. HKDF binds protocol/magic/version/algorithm/
lengths/IDs/revision; salt is mandatory fresh host CSPRNG output. AES-GCM16-byte
tag verified by real `mbedtls_gcm_auth_decrypt` (constant-time library comparison).
Protected HOST class prevents interpreting fixture ciphertext as native class.

### Bundle PPB1 /848 bytes; nonce ledger PPL1 /80 bytes

Bundle: header48 (magic/version/count1..2/envelope length384/reserved zero), two
envelopes384, HMAC32 over preceding816 bytes. Ledger: magic/version, scope/owner/
key epoch and nonce revision through40 bytes, reserved zero to48, HMAC32. Separate
HKDF ledger domain key authenticates the ledger and bundle. Count/order/tails and
every retained envelope/codec validated before handle export. All files including
ciphertext/MACs are SOURCE_PRIVATE, **even when fixtures contain publicdummy data**.
Largest file848B <4096; no existing ceilings were raised. Two retained profiles
(old/new), then CAPACITY: no automatic discard/GC/reset/prune/forget extension.

## Nonce, anti-rollback and transaction proof

Per-master-key **global** revision is reserved through mandatory source-owned
durable CAS before encryption. Authenticated `nonce.ledger` is an additional
write-ahead fence, never a substitute for rollback-resistant external ownership.
Nonce = mandatory OS CSPRNG prefix4 +LE global revision8; fresh salt16 also comes
from one exact20-byte getrandom call. Missing/short/error/all-zero salt/prefix is
HOLD, **no zero/deterministic fallback or automatic retry**. Distinct global
revisions make nonce bytes distinct under the same key. This proof requires the
external ledger to reserve uniquely across all stores/scopes/restarts for that
actual key and never reset; a new key epoch requires genuinely rotated key
material, not relabeling the same key. Native key/entropy/ledger proof is absent.

Separate external **scoped** committed-current CAS is required after profile file
sync. Burned nonce revisions are **not commits**. V2's mandatory `authority` getter
selects by scope/owner and returns actual current scope/owner/key epoch, GLOBAL
per-master-key nonce floor and that scope/owner's committed-current revision. It
must read live source authority, not echo selectors/options or private-store bytes.
Same master-key reference context/global reserve ledger is shared across **all**
scopes/owner labels; do not create an independent nonce counter per label. Scope/
owner commits remain distinct. Getter scalars are not native/authentication flags.

Admission expectations are pinned to the session/key. Before private key/file
reads and decrypt/use, and after blocking file/crypto/provider work, the live getter
must match **all** expected identity/checkpoint fields. Before opaque handle export
or private-effect handoff another final check is required. Unknown/regressed/
changed proof is HOLD/session-key wipe, public handle zero, no private callback.
Source values never replace cached expectations simply to pass old files. Only
successful this-session reserve/commit CAS plus exact confirming live proof can
advance expectations to the explicitly requested next revision. Any identity/key
epoch change requires closing and genuine externally qualified rotated-key unseal,
never relabeling imported old key material. Native remains unconditional HOLD.

`nonce.ledger` is this store's authenticated **last reservation from the global
sequence**, not global live authority. On a fresh explicitly admitted session it
may lag the global floor because another scope used the same key: require
`scoped_commit <= store_last_reservation <= live_global_floor`. Envelopes must be
within that store reservation and strictly ordered; newest bundle revision must
**exactly** match the scoped live/admission commit. New writes reserve
`live_global_floor+1`, never `store_last_reservation+1`. Thus another scope can
consume revision2 while this scope's current remains1; a fresh session can then
reserve3 without resetting or duplicating the nonce suffix. An already-open
session expecting global1 must HOLD when authority becomes2, even if old files
are replayed. A source-advanced scoped commit cannot accept an older current
snapshot. Explicit previous-profile lookup inside the authenticated **current**
bundle is permitted, not promotion of an old snapshot. Duplicate IDs/CAS reject.

### V2 live-authority control table

| Boundary | Required matching proof /result |
|---|---|
| Key import | Actual HOST hardening+lock, live authority before/after exact readonly FD read; no Native key read |
| Open/seed/read/put/select | Pinned scope/owner/key epoch/global floor/scoped commit; mismatch before private read is HOLD |
| File read /AEAD | Live checks before/after file read, before/after authenticated decrypt/crypto; material never published on change |
| Reserve | Global actual-key CAS(epoch,old-global,next-global) → exact live proof(next-global,old-scoped-commit) → advance only own expectation |
| Nonce/profile persistence | Live authority around write/sync/rename/readback and after fault/provider callbacks; uncertainty never selects older data |
| Commit | Scoped CAS(scope,owner,epoch,old-scoped-commit,next-global) → exact live proof(next-global,next-scoped-commit) |
| Handle/use | Complete file/auth/codec/revision checks → final live proof+hardening → zero-secret opaque handle or trusted private callback only |
| Private callback return | Recheck authority and wipe working material; failure cannot undo an already-performed caller effect |

Caller must serialize all files/calls/nonreentrant callbacks and hold actual source
authority/key/ownership protections through final publication/private use. Sampling
does not make a getter atomic with an external effect, authenticate the provider,
or prove physical rollback resistance. Changes detected *before* handoff mean
callback0; changes inside an effect can only produce post-return HOLD, not undo it.
Physical source/key/bootstrap/checkpoint trust is still a mandatory native gate.

Store names are constants, never profileID/path interpolation: `profile.current`,
`profile.pending`, `nonce.ledger`, `nonce.pending`. Only current/ledger are accepted
at admission; pending/foreign entries mean HOLD. Directory owned0700, files owned
regular0600/nlink1, O_NOFOLLOW, pre/post-open fstat and exact lengths. Writer:
exclusive fresh pending → bounded write → file fsync → close → directory fsync →
rename fixed current → directory fsync → exact constant-time readback. Global nonce
reservation precedes nonce ledger persistence; profile commit checkpoint follows
profile persistence. Any ambiguity destroys keys, preserves files and requires
explicit source-owner reconciliation; no pending promotion, repair/defaults,
foreign overwrite/unlink/format or public digest. Caller serializes all files/
callbacks across processes; not a sandbox against hostile same-UID writers.

Public API returns only status or authenticated **opaque nonsecret handles** with
scope/epochs/ID/revision. `public_id` must be an owner-minted allowlist, not plaintext
or secret-payload-hash integers. No source profile data returned by public query.
`pp_store_use_private` is for an INTERNAL trusted private effect owner only,
never HTTP/events/cookies. No existing network metadata adapter is edited/wired.

## Reproduce / artifact exclusions / results

```sh
python3 apps/network/private-profile-candidate/qualify.py
```

Fresh `local/validation/private-profile/publicdummy-*` outputs. Separate own
config/crypto objects for every mode; pinned canonical vendor277-file tree verified.
Optimized and ASAN/UBSAN v2: **57 cases /1,220 checks each PASS**,54 retained private
fixture directories per execution. Published NIST AES256-GCM and RFC5869 vectors,
actual tag/ciphertext/AAD/scope/algorithm/extension failures (outer test HMAC
reauthenticated to exercise actual GCM), strict codec, external checkpoint rollback,
old namespace, replay/CAS/capacity,12 syscall fault cases, entropy/commit HOLD,
session/negative zeroization, native-before-key-read and actual host readiness/
wrong-FD/dumpability/memlock failure paths. New tests cover serialized second-session
update while the first stays open, restored old authenticated files before handle/
use/put, live getter failure/identity/epoch/floor/commit change/regression, changes
at AEAD/readback/final check, zero private callbacks on pre-handoff failure, shared
master-global floor across two scoped commits, and strict basic syntax failures.
The fork model contains PUBLICDUMMY data only and explicitly retires the inherited
child copy before a new session; it is not a native bootstrap/secret-fork recipe.
C++→C link/run PASS. ARMv5 static link /
readelf PASS, **NOT EXECUTED**. Non-publicdummy native-HOLD translation unit compiled.

Reports/logs contain scalar results/counts/check stages only, no key/profile/tag/
nonce/ciphertext contents or hashes. **All fixture directories and crypto/key/
checkpoint/readback payload files are excluded from manifest content/hash inventory**;
no recursive output walk. Allowed hashes: source/config/header/tool/compiler/
library/ELF/object/scalar-log files. Published vectors/publicdummy values exist only
in explicitly guarded test source, never product constants or native inputs.
Current source snapshots are retained in build `inputs/`; all earlier errors and
HOLDs retained without payload hashing. No operator inputs, actual key creation/
unseal, target/UART/proc/network/SD/SPI/mount/GPIO/install/Git/publication effects.

Evidence: `evidence/delegated/20261012-private-profile/v2/MANIFEST.json`.
Root MANIFEST remains historical v1; its686-check run did not cover live authority.

**ONE next parent dependency:** qualify the source-owned per-device key/bootstrap
and rollback-resistant nonce/commit checkpoint authority **on a measured dedicated
owned backing store**, with cold-entropy and actual hardened-context evidence
before enabling native unseal. Read-only store readiness must precede enrollment/
writes: retained75 MMC disabled/probe refusal is not card failure/absence evidence.
Radio/AP/client and physical key/store durability remain unqualified; no native
crypto/tag/host fsync test closes SEC/G6/G8. Main video/UI qualification is independent.
