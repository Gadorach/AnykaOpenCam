# Standalone RTSP Digest core — SEC-05 / STREAM-07

**HOST ONLY, awaiting independent parent review and integration. SEC-05 is not
finished. Existing server remains NOAUTH LAB. G3/G8/product gates remain false;
T34/Fulhan ports remain deferred.** No target operations or provisioning here.

## Profiles and credential boundary

- Prefer explicit `SHA-256`, mandatory `qop=auth`, bounded ASCII fields.
- `MD5` is an **explicit legacy interoperability profile**, not a fallback or a
  client compatibility claim. No algorithm omission, sess, Basic, auth-int or
  no-qop path. No actual NVR/client qualification has occurred.
- One account and immutable algorithm per owner core. Caller may create only a
  fixed, admitted outer set; this is not a dynamic per-user registry.
- `HA1 = H(username:realm:RTSP-password)` is a digest-equivalent secret, not a
  UI/admin password verifier. UI PBKDF2 material must never enter this API.
  Credentials, generations, account IDs, realms and scopes must be explicitly
  mapped by a trusted RTSP-only backend. No credentials/defaults are provisioned.
- Ordinary RTSP/TCP is **unencrypted**; Digest does not encrypt media, authenticate
  the server or prevent offline password guessing / active MITM manipulation.
  Strong independent RTSP secrets and a trusted/protected network are needed;
  TLS/VPN/NVR support must be separately designed and actually qualified.

Mbed TLS **3.6.7**, already vendored at
`apps/auth-web/service/vendor/mbedtls`, supplies MD5, SHA-256, constant-time
comparison and zeroization. It is read-only and retains its upstream
`Apache-2.0 OR GPL-2.0-or-later` notices/license. New code is GPL-2.0-only,
consistent with surrounding framework code. `rtsp_crypto_config.h` is a separate
RTSP-only object build, never an HTTPS config. HTTPS TLS configs/budgets are not
modified. Do not combine this crypto object set with the HTTPS object set in one
binary: same Mbed TLS symbol names with different configs require reviewed build
isolation (separate service/library linkage boundary), not accidental mixing.

## API and bounds

`framework/include/camera/rtsp_digest.h` exposes caller-owned storage:

1. Initialize once at owner startup with realm, profile, TTL and a qualified
   random callback. TTL is 1..300000ms. No wall clock is read.
2. Provision one independent account with exact binary HA1 length (32 SHA-256,
   16 MD5), nonzero account and explicit 64-bit allowed-scope mask.
3. Issue a challenge for a nonzero connection ID and exactly one authorized
   scope bit. Eight fixed nonce slots; no eviction of active challenges.
4. Verify bounded, already unquoted Digest fields against **actual request**
   method/target, exact username/realm/algorithm/qop/nonce/cnonce/nc and response.
   Registry binds nonce to connection, scope, credential generation and expiry.
5. Recheck the returned trusted-owner fact before every ongoing media effect;
   separately authenticate each subsequent RTSP command. Close invalidates only
   the named connection. Credential changes/revoke clear all nonces/grants.

The core allocates no heap and performs no logging, sockets or I/O. At most two
bounded hashes and eight registry comparisons per verify; invalid identity/scope
is rejected before hash work. Username/realm/cnonce <=64 bytes, target <=512,
method <=24 uppercase letters/underscores (including GET_PARAMETER and
SET_PARAMETER), password provisioning <=128 opaque bytes.
No Unicode normalization, charset/userhash, HTTP proxy rewriting, opaque or
Authentication-Info/rspauth support is claimed. Digest URI must byte-match the
actual request target; caller maps URI to trusted resource/scope separately.

Nonces contain 24 CSPRNG bytes and a nonwrapping public 64-bit issue serial,
hex-encoded to 64 bytes. They are opaque registry keys, **not signed nonces**.
The serial prevents reuse in one owner lifetime even in a deliberately broken
repeat-output callback fixture; it does not replace CSPRNG readiness. Random
failure publishes no nonce/state. HOST `getrandom` fixture is not target entropy
proof. Restart discards connections/grants and requires fresh target entropy.

Times come from one trusted boot-local monotonic millisecond clock. Expiry is
exclusive (`now >= expires` fails). Regression latches failure and invalidates
all nonces. Credential generations and nonce serials never wrap. Generation
exhaustion disables credentials. The owner must never edit storage or reinitialize
a live core. See the exact trusted buffer contract below; random callback must
not reenter/mutate the core. Connection IDs must not be recycled while old
state exists; always close before reuse and discard every state on restart.

`nc` is exactly eight hex digits, nonzero and strictly increasing **globally per
server nonce**, never per cnonce. A fresh bounded cnonce is allowed on every
request, provided the response authenticates that request's exact cnonce and
the global nc increases. Changing cnonce with `nc <= last_nc` still fails replay
checks even with a correct digest. Last accepted cnonce is retained only as
fixed-size bookkeeping, not an immutability rule or replay key. Bad responses
cannot advance nc or replace that bookkeeping, or establish a usable fact.
Clock ticks and expiry reclamation can still occur on rejected calls. Out-of-order
pipelined requests are intentionally rejected. Caller must retry with a new
challenge at count exhaustion; count wrap/no-qop downgrade is never allowed.

RFC rationale (public source verified read-only):
[RFC7616 section3.4](https://www.rfc-editor.org/rfc/rfc7616#section-3.4) defines cnonce as client-
supplied opaque request data and nc as the number of requests using **the server
nonce**. [Section3.4.1](https://www.rfc-editor.org/rfc/rfc7616#section-3.4.1)
binds the request's cnonce in the response. Section3.4.2 explicitly names
cnonce-prime for `-sess` A1, whereas non-sess SHA-256/MD5 A1 does not use cnonce.
Nothing in these non-sess rules requires cnonce immutability. V1's additional
restriction was removed at parent review; this is interoperable request binding,
not relaxed replay state. Actual FFmpeg/NVR compatibility is still unqualified.

### Exact trusted C buffer / alias contract

This is **not an adversarial-pointer API**. There is no runtime overlap detection
or safe-rejection promise for aliased outputs/owner storage. Caller supplies
valid typed writable objects and readable input objects/bytes for all declared
lengths throughout each serialized call. The header repeats these preconditions.

- For owner-taking calls, every external input object and payload must be
  outside the **entire** owner struct, including credential and nonce fields.
  `check` also mutates the owner by ticking/reclaiming expired nonce state.
- Writable output `out[32]` (both stateless helpers), `nonce[64]` (`issue`) and
  `grant` (`verify`) must be disjoint from all input objects and payloads, any
  live owner struct and random-callback context. In particular, grant cannot
  overlap `fields`, any field's bytes, actual method/target, or credential data.
- Read-only inputs **may alias each other**: actual URI may share `fields.uri`,
  and nonce/cnonce may share a readable external buffer. Positive tests cover
  these cases for both algorithms; they are not output-alias rejection tests.
- HA1/response helpers zero their 32-byte output **before** input validation.
  In-place password/HA1 hashing or HA1-to-response output is unsupported.
- `init` clears the whole owner before reading realm. `provision` clears nonce
  state and old HA1/username before copying input. Never pass `s->realm`,
  `s->username`, `s->ha1` or nonce-backed input to these mutating calls: first
  obtain separate trusted copies. Never reinitialize an active credential core.
- `issue` clears output nonce; `verify` clears output grant **before** owner/
  request validation. An output pointing into owner, credentials or request
  inputs could destroy them even when the call returns failure; such a call
  violates the preconditions and is not covered by malformed-field tests.
- Random-callback context remains valid and separate until destroy. Callback
  must not reenter/mutate owner/input/output objects or retain its scratch buffer.

Zeroizing an error output is not evidence of safe arbitrary alias handling. The
trusted parser/backend adapter must own distinct scratch/output/credential/core
objects; supporting malicious C pointers would require a separately reviewed
API design, not guessed pointer comparisons added to this bounded correction.

## Reproducible offline build

From `/home/gadorach/Desktop/AnykaOpenCam`:

```sh
python3 -B apps/rtsp-auth/build.py host01
python3 -B apps/rtsp-auth/build.py arm01 --toolchain /path/to/existing/armv5-eabi--musl--stable-2025.08-1
```

Use a **new** name every run. Outputs live only under
`local/validation/rtsp-auth/<run>`; the script refuses existing paths. No installs,
dependency downloads, hardware access or ARM execution. The second command
requires existing pinned Bootlin ARMv5 musl GCC14.3.0; component SHA256 pins match
the existing authentication build's public pins. No historical build cache used.

`REPORT.json` records exact commands, timestamps, compiler/runtime closure,
selected sources and transitive header hashes, pinned vendor source tree and
licenses, link archives/objects, binary hashes, compiler stack-usage estimates
and scalar test results. It
compiles optimized HOST, ASan+UBSan HOST, links/runs a C++->C API fixture, and
optionally statically links ARMv5 and inspects ELF metadata. No credential payload
or hashes are emitted. Source/binary hashes include public test code only; no
separate generated/public credential-payload hash is ever recorded.

ARM link metadata caveat: newly compiled core/crypto objects report ARMv5TE;
the pinned static runtime's merged ELF advertises v5TEJ/Thumb-2. This is preserved
as metadata, not a claim of native ISA/ABI qualification. No ARM ELF is executed.

Public RFC2617 section3.5 MD5 and RFC7616 section3.9.1 SHA-256 vectors qualify the
stateless core formula; PUBLICDUMMY fixtures independently formulate requests
with one-shot Mbed TLS, not the core helper. Tests cover identity/method/URI/
profile/qop/scope/connection binding, wrong secret, every response byte, malformed
fields, replay/cnonce switching/nc wrap, random failure, capacity/reclamation,
expiry, revocation/generations, owner-counter exhaustion, clock regression,
per-client isolation and public C++ linkage. Scalar logs never print auth fields.

## Remaining integration boundary

**One next parent task:** independently review the diff and rerun fresh tests,
then adapt the existing bounded RTSP parser/challenge/backend boundary to this
core **before any SDP, SETUP/PLAY, hub subscription, snapshot or media effect**.
The wire parser must reject duplicate/missing/unsupported directives, correctly
unquote fields without logging them, and use actual method/target from Request-
Line. Keep an admitted fixed account/connection set and enforce aggregate and
per-peer/account attempt/challenge/IDR rate limits before these calls; fixed-cost
hashing is not a CPU flood/throttle policy. Map verified facts into the existing
shared access policy (no UI/RTSP domain confusion), check policy and credential
generation on every effect, and tear down expired/revoked subscriptions.

Challenge formatting/status/stale policy, backend persistence/provisioning,
process secret isolation/locking/crash-dump prevention, target CSPRNG readiness,
native memory/CPU/media-load measurement, parser fuzzing and actual SHA-256/MD5
NVR interoperability are **not implemented/qualified here**. Do not advertise
authentication or weaken HTTPS600k/2MiB budgets from these host tests.
