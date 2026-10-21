# Actual copied RTSP server adoption candidate — HOST ONLY

SEC-05 / STREAM-04 / STREAM-07, independent parent review pending. **Not native,
not NVR/product acceptance. Existing framework server remains NOAUTH LAB; native
media501, G3/G8/product gates remain unchanged, T34/Fulhan deferred.** No existing
framework/core/parser/policy/producer/HTTPS/TLS/runner/plan/target/Git or native
handoff worker files changed. Only this directory and own evidence are writable.

## Provenance / ABI / startup

`upstream/rtsp_server.c` is a byte-exact0444 copy of the reviewed existing server,
pinned in `PROVENANCE.json`. `rtsp_server_candidate.c` started from those same
bytes; every build emits an exact unified `server.diff` and added/removed counts.
The existing cursor/hub/RTP/lifecycle paths are retained and modified, not a
replacement toy server. A **distinct** `camera_rtsp_candidate_serve` entry point
and `candidate_auth.h` interface leave the existing `camera_rtsp_serve` ABI and
header untouched. Old caller options are accepted only with explicit127.0.0.1,
2000ms whole-AU budget, one1MiB hub and mandatory original lifecycle hooks.
No unauthenticated/legacy production fallback is linked or enabled.

Before creating the candidate listener, startup validates a sized source-only
configuration, explicit SHA-256 or MD5 legacy profile, nonzero boot/identity,
bounded valid source graph/exact H264 profile, canonical policy identifiers,
explicit persistent rate budgets, an RTSP-only trusted bootstrap callback,
ready CSPRNG callback and pure bounded readiness/ownership-sink callbacks.
Missing/invalid/foreign-domain credentials, RNG failure, source/role graph or
budget errors refuse startup. `host_fixture_only=1` is an environment boundary,
NOT authentication, source qualification or entropy proof. All actual authority
still requires a real cryptographic response, registered nonce and shared policy.

Bootstrap provides exact bounded username/realm, numeric account and binary
HA1 digest-equivalent secret, explicitly domainRTSP and configured algorithm.
The candidate has **no product credentials/password/defaults or userinfo URLs**;
only the separately labelled PUBLICDUMMY fixture knows its test password.
Bootstrap temporary and RNG probe are wiped with real Mbed TLS zeroization.
Native durable/private bootstrap, locked/isolated secret storage and cold entropy
remain parent dependencies. UI/admin600k PBKDF2 never enters this API.

MbedTLS3.6.7 Digest/wire/core and all existing shared parser/policy/hub/producer/
packetizer code are referenced read-only, with separate RTSP-only MD5 crypto
objects/config, never HTTPS TLS objects. Each final command/source/header/link/
compiler/runtime pin is recorded. No dependency installs, target operations,
guest/private files, real footage or existing socket/process status inspection.
Actual sockets are only owned127.0.0.1 server/client fixtures explicitly requested.

## Adopted authorization boundaries

### V3 destructive producer-release correction

Reviewed V2 source/diff/results are frozen before editing at
`evidence/delegated/20261012-rtsp-server/v2-release-snapshot/`. The producer's void
release callback may zero the entire `camera_packet` and invalidate its payload.
Validated scalar keyframe/token/generation/bounds are captured BEFORE release;
END and scalar logging use the independent validated keyframe, never released
packet fields. The normal path and immediate post-next clock-error release path
have no packet/payload access after the callback. Returned ownership does NOT
prove independent DMA stop: original checked END/lifecycle retirement still owns
that question. Clock uncertainty retains the exact original open bracket/demand,
with no manufactured END timestamp, STOP/idle/quiescence success or retry.

Validation before publication enforces keyframe0/1, H264, readable owned VCL,
bounds2..1MiB and IDR5/P1 correspondence. Exact common frame owner/id/lifecycle
namespace/current generation/RUNNING state is checked independently, including
after readiness callbacks. Producer packet token/generation are opaque export
identities; they are NOT required to equal common frame or hub CPU-lease
generations or be nonzero without an actual backend contract. Native next/release
must prove its own export/lease validity; the hub then owns its distinct copied
token/generation and deferred exact `camera_hub_lease_valid` checks are unchanged.
No invented DMA receipt fields or alias/source-qualified flags are added.

Every HOST fixture release now destructively zeroes the packet. Added real
localhost SHA-256/MD5 cases use actual forced-IDR work followed by synthetic P
NALs and check END sees the pre-release keyframe exactly, with one initial forced
IDR/no extra IDR and full1MiB byte/CRC/guard checks. Invalid keyframe2 and oversize
export are released once but never published/RTP-sent. Two clock-fault cases
cover immediate post-next and post-release branches, release exactly once and
retain the open frame/demand with END/idle0. Only those HOST clock error boundaries
are injected; normal calls use actual monotonic clock and real TCP, Mbed crypto
is never mocked. No native picture/reference decoding or stop proof is inferred.

### V2 parent-found BEGIN boundary correction

V1 source/diff/results were byte-exact snapshotted before editing under
`evidence/delegated/20261012-rtsp-server/v1-snapshot/`; originals remain unchanged.
Candidate config ABI is now2, with mandatory `begin_contract=LOGICAL_ONLY`, live
source-guard bind/unbind and exact frame-uncertainty sink. This candidate **forbids
capture/DMA/reset/P-reference acquisition inside BEGIN/START**. Its common START
is logical bookkeeping, not a native effect qualification. Native integration
must move acquisition to next(), or separately review effect-gated native BEGIN;
it may not infer permission for internal native calls from a pre-callback check.

The bound live guard is enforced in the actual fixture START and next adapters,
immediately before their source effect counters. It does not reenter producer
callbacks. It runs owner/readiness/auth checks for playing attached demands, then
a single fresh-clock callback-free sweep catches even a peer that expired while
another peer's readiness check blocked. No cached boolean is authority.

After successful BEGIN returns, that same guard runs AGAIN immediately before
actual `p->next`, with current producer state/generation. If no valid admitted
demand remains, the candidate never calls next()/source CRC/release/hub publish/
RTP. It calls original END once on the exact original handle, with
`CAMERA_NOT_READY,keyframe=0`—never a fabricated successful P/keyframe result,
P-reference reset or automatic retry. Peer demands close/detach exactly once.

The existing common END failure rules are deliberately preserved: negative
no-capture END can close the logical bracket while leaving `needs_stop=1` and
DRAINING after detach (`stop_pending=0` is not a STOP acknowledgement). Failure
is returned and exact frame/demand/hub metadata
is retained; no implicit STOP retry, idle/quiescence or native retirement is
claimed. An injected END rejection retains an actually open frame/keyframe
work too, rather than erasing references. The copy-only sinks do not authorize
free/unload. V2 control tables show end/detach/idle/open-work/STOP uncertainty
explicitly; native parent must finish authoritative retirement/recovery.

Startup paths now destroy/wipe even partial initialization and the post-success
clock-error return. A dedicated HOST clock/socket-boundary injection proves one
actual successful real-crypto bootstrap, one full backend wipe and zero listener
effects; it does not emulate native cryptography or retire hardware. Source-guard
binding is removed before the backend/stack context expires on all bound paths.

At copied `process_input`, fresh actual Request-Line method/target borrow views
are mapped only to the frozen live/track registry (OPTIONS* scoped to this known
profile). Wire/core verification happens before SDP/DESCRIBE, SETUP/session,
PLAY, hub subscription and lifecycle attach. Names/directives/algorithm, malformed
fields, duplicate/unknown/missing/no-qop/basic/downgrade/userinfo/body constraints
are the unchanged reviewed parser/wire profile. No full request/header/username/
URI/HA1/response/nonce/session values or credential-payload hashes enter receipts.
401 is the bounded safe config-only realm/nonce/qop/algorithm/stale/CSeq formatter.
All normal existing methods are supported: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE,
GET_PARAMETER and TEARDOWN. GET_PARAMETER has **empty-body-only policy**; nonempty
bodies remain deliberately rejected. No SET_PARAMETER, UDP, ONVIF/cloud extension.

An actual verified Digest grant maps via the trusted backend to domainRTSP /
VIEWER / PROFILE / LIVE, exact device/account/profile/physical-sensor identity,
boot and credential generation. No wire or model `verified` flag manufactures a
principal. A separate nonwrapping per-backend account session counter survives
peer churn and rotation; one owner principal is reused for a current nonce so
keepalive cannot exhaust the16 policy slots. Checked millisecond→nanosecond
expiry conversion keeps one actual CLOCK_MONOTONIC domain. Sessions use ready
CSPRNG bytes, not the old CAM-per-slot token. Connection IDs never wrap/reuse.

Global/account admission survives reconnects and owner credential changes;
fixed per-peer state is bounded4. Accepted connections, receive chunks, every
verification/challenge and PLAY demand admission share explicit bounded budgets.
Unknown-user/malformed/reconnect attempts cannot allocate accounts or reset
global quota. Fixed windows allow boundary bursts and do not claim native CPU
capacity. Syntax failure or rate/clock/entropy uncertainty never invokes media.

Every guarded reply/body/packet/partial-send continuation rechecks real Digest,
shared policy, current generation, explicit peer cancellation and pure readiness.
RTP additionally rechecks the exact acquired hub CPU lease and original producer
RUNNING state immediately before each effect. Callback/crypto latency is followed
by fresh monotonic reads and callback-free final auth/policy check; send/expiry
deadlines are rechecked without widening2000ms. No hidden blocking-helper lease
extension or no-auth side door. Callback contracts are bounded and serialized;
this candidate cannot preempt a stuck native helper and claims no hard-fault proof.

Mixed interleaved RTCP requires current authorization. One bounded pending
GET_PARAMETER/OPTIONS reply waits for a complete RTP packet boundary. PAUSE and
TEARDOWN wait for the current AU's unchanged absolute deadline, then release CPU
lease before unsubscribe/detach/reply; no reply is inserted into partial data.
Pipelined ready control already in receive storage is processed after pending
reply completion. Expiry, rotation/revoke, owner cancel or deadline failure before
a continuation closes/discards the buffered partial packet, releases its exact
CPU lease once, then unsubscribes/detaches the exact demand once. Source release/
frame end still precede consumer handoff; all producer work remains caller-owned.

Unexpected release/detach/retirement uncertainty returns failure, preserves fixed
hub/demand metadata through mandatory `retain_uncertain`, and skips false retry/
STOP/free/unload claims. Backing/lifecycle must remain retained by the caller until
separate authoritative retirement/recovery. That copy-only notification is not a
DMA/CPU retirement receipt or permission to recycle memory. No driver/source
unsafe unload is implemented. Trusted object nonoverlap, callback lifetime, no
reentry and source graph freeze are required; malicious C-pointer alias support
is not claimed. One serialized serve per process (copied static clock) only.

## Fixtures / reproducibility / qualification limits

```sh
python3 -B apps/rtsp-auth/server-candidate/build.py candidate01 --toolchain /path/to/existing/armv5-eabi--musl--stable-2025.08-1
```

Use a new output name each run; existing outputs refused under
`local/validation/rtsp-server-candidate/<run>`. Versioned immutable source copies,
exact diff, commands/versions/source/header/runtime/library/binary hashes and
scalar results/failures are preserved. Optimized and ASan/UBSan run real owned
loopback PUBLICDUMMY clients with independent Python hashlib vs real Mbed TLS,
plus real producer/common-lifecycle/hub code. Public RFC2617/7616 vectors are
checked in both actual Mbed C and independent client code, never printed or
payload-hashed. C++→C and optional pinned ARMv5 static linking are separate;
ARM ELF is NEVER executed. Pinned runtime merged Thumb-2 metadata is not native
ISA/ABI qualification. The existing input-parser GCC nonnull diagnostic remains
only a dependency warning; copied server compact style gets its existing-style
warning exception, new backend/tests retain strict `-Wall -Wextra -Werror`.

Synthetic source is explicit PUBLICDUMMY1MiB NAL bytes with16-byte guards,
full-source CRC-BZIP2 before each producer return and independent client full
NAL byte/CRC reassembly. Synthetic SPS/PPS/NAL are **not valid decoded footage**,
software encoding, native media, hardware capabilities or performance evidence.
Graph nodes remain PRESENT_UNQUALIFIED with0 qualified sessions. HOST counters
exercise owned attach/start/frame/release/end/detach/idle and retirement, not
electrical/DMA/source proof. One1MiB hub and original1404-byte per-peer packet
cursors are unchanged; no whole-AU per-peer copies or HTTPS2MiB budget change.

`host_send_chunk` is0 for ordinary actual sends; the deterministic retirement
fixture explicitly chooses17 bytes per real localhost RTP `send`. It changes no
crypto result, socket syscall return or2000ms budget. A trusted owner cancel is
armed to fire after the first real positive write, forcing a partial packet;
the next guarded continuation must close/discard, release once and detach/idle
once. This is HOST instrumentation, not a proposed production performance knob.

Real localhost tests cover no/bad credentials, duplicate parameters, both
algorithms, fresh cnonce/global-nc replay, no SDP/source before auth, sessions,
normal methods, fragments/pipelines, mixed RTCP, owner rotation/revoke/cancel,
rate/churn, stalls/expiry and blocking-readiness expiry. All public receipts are
scalar; no private camera, keys, ROM/photo/SD data, proc/fuser, arbitrary endpoint
or NVR is accessed. Child shutdown is via exact owned process/pipe, not PID scans.

**ONE parent native integration task:** review the copied diff/backend ABI and
independently rerun, then adapt this admission/per-effect/retirement chain to the
parent-owned real RTSP server with independently provisioned RTSP HA1, qualified
target entropy, stable policy/source/lease owner mapping and exact native combined
load/revoke/expiry/slow-peer evidence. Do not advertise native authentication or
close G3/G8/NVR/client acceptance from this candidate's HOST results.
