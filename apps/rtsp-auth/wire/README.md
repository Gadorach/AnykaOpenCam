# Bounded Digest wire adapter — SEC-05 / STREAM-04 / STREAM-07

**HOST ONLY; parent review required. Existing RTSP remains NOAUTH LAB. No native
authentication/client/NVR/headroom/entropy or product acceptance claim. SEC-05,
G3/G8/product gates stay open; T34/Fulhan deferred.** No server/parser/runner,
Digest core, TLS/vendor, root/plan/targets or Git index edits in this unit.

## Exact boundaries and compatibility

`rtsp_digest_wire.h/.c` consumes an actual fresh READY result from existing
`camera_rtsp_request_parse` or `camera_rtsp_input_next`. Request-Line method and
target remain borrowed byte-exact inputs, never Authorization-derived or
canonicalized. Parse one request, authenticate before retiring/moving receive
bytes, then individually process any pipelined remainder. Existing maxima remain
8192 request bytes, 24 headers, header name64/value1024 and Authorization1024;
wire adds9 required directives, name16 and the core's individual value limits.
Nonempty bodies are rejected, matching existing RTSP control policy; no invented
SET_PARAMETER/body effects or broad new protocol implementation.

Nine directives are mandatory exactly once: username, realm, uri, algorithm,
qop, nonce, cnonce, nc, response. Unknown directives (including opaque/userhash/
username*/method) and Basic/missing algorithm/no-qop/auth-int/sess are rejected.
Auth scheme, existing header names and directive names are ASCII case-insensitive.
Explicit SHA-256/MD5 algorithm values are case-insensitive and canonicalized
only after exact configured-profile matching; never implied MD5 or downgrade.
Hashed qop is strictly `auth`; nc/cnonce/realm/username/URI are NOT normalized.

Token or quoted-string values are accepted within the same exact bounds. Quoted
pairs unquote printable ASCII/SP once. Empty/unclosed strings, dangling escapes,
CTL/NUL/non-ASCII, invalid tokens/separators and decoded characters outside the
existing core profile fail. Core disallows quote/backslash in decoded values,
spaces in cnonce/URI and colon in username/realm; these remain compatibility
constraints, not guesses about client support. SP-only BWS/OWS matches the
frozen request parser: HTAB/folding are rejected, not silently reinterpreted.
Eight-hex nc must be nonzero; nonce exactly64 hex; response exact digest length.
URI must byte-match actual Request-Line target. Existing request parser rejects
authority userinfo/authority percent escapes; path `@` is not interpreted as
userinfo, filesystem or credential input. Exact configured URI resource routing
remains a distinct trusted policy step. No Authorization/URI/username/password/
nonce/response/token values or credential-payload hashes enter logs/receipts.

Decoded values live in a fixed owning scratch object (no heap). Field pointers
point into that object; do not copy/move/use after clear. Error and completed
scratch clearing uses real Mbed TLS zeroization. The existing core's fresh-cnonce
higher-global-nc behavior, strict replay, generation/expiry/scope/connection and
real MbedTLS3.6.7 crypto remain unchanged, in the separate RTSP-only config.
Ordinary RTSP/TCP is **unencrypted**, HA1 is a digest-equivalent independent RTSP
secret, never UI PBKDF2/admin verifier data. HTTPS600k/2MiB budgets/config untouched.

## Admission, challenge and stale behavior

Wrappers charge three distinct fixed owner buckets (global, admitted account,
admitted connection) before any Digest parsing/crypto or nonce issuance. Each
kind has its own explicit1..1024 attempts per1..60000ms fixed window. Equality
does not replenish; exact window expiry resets; regression latches all three
closed. No count wrap and no partial credit charge when one tier denies. Once
admitted, malformed/missing/bad crypto/capacity/random failures still consume
credit. This is a fixed-window burst bound, not sliding-window or target timing
qualification. Caller chooses measured limits; no product defaults provided.

Caller MUST bound peer/account admission and any aggregate request parsing/IDR
work before allocating/calling. Global bucket is shared across all adapters;
account bucket persists through disconnect/credential changes, including
unknown-username attempts, never keyed/allocated from attacker names. One stable
core/account binding per adapter; caller derives it from trusted resource routing.
Reinitializing buckets on peer churn must not reset account/global allowance.
No dynamic IP/user table, implicit unthrottled path or scheduler infrastructure.
Pure parse/format helpers are not authority or admission wrappers.

Challenge formatter emits only configured safe realm/explicit algorithm,
fresh core-issued nonce64, qop="auth", sanitized numeric CSeq, stale boolean and
Content-Length0 in a bounded512-byte RTSP401. Realm quoting is safe because core
configuration excludes quote/backslash/CTL. Never echo incoming Authorization or
target. Capacity is preflighted before charging/issuing; insufficient output is
not a partially published challenge. Issuance uses existing core CSPRNG callback.

Eight fixed nonce notes retain only issued scope/connection/generation/expiry
metadata. They are NOT authentication authority. An exact, current-scope,
current-generation **known expired** nonce yields `stale=true`, even after the
core's per-effect expiry check clears its slot. Unknown, forgotten, replaced,
revoked or wrong identity/scope nonces get `stale=false`. Use `wire_close` rather
than calling core-close alone: it clears that connection's expiry notes as well
as core nonces. Parent separately revokes policy/retires subscriptions. Slot reuse can forget
old hints; conservative false is intentional. A stale hint is not password proof
(expired responses are never granted); bad/replayed requests never advance nc.
Clock/crypto/exhaustion/throttle faults do not invite retry challenges. Parent
must call challenge only on the enum returned for an eligible rejected request,
not an untrusted stale string. Old notes cannot revive credentials/sessions.

## Trusted object/alias contract

Valid typed owner/output objects and readable input bytes are caller preconditions,
not adversarial C pointer support. All writable scratch/grant/reason/output/length
objects, adapters, core, buckets and callback context must be mutually disjoint
and disjoint from all external input objects/bytes. Read-only inputs may alias.
Output clearing precedes validation and can corrupt aliased input/credentials;
no runtime partial-overlap detection or safe-rejection claim. Equal bucket
pointers are explicitly rejected. Borrowed request views remain readable until
verification finishes; serialize all calls and actual effects. Only the adapter
may write nonce notes. No live reinitialization to reset generations/admission.

## Exact parent integration proposal (not native integration)

Existing `rtsp_server.c` `process_input` (lines110–159 at this source pin) receives actual borrowed views after
`input_next`, before `input_retire`, then currently calls `request_policy` and
dispatches DESCRIBE/SETUP/PLAY. Parent should insert admitted wire verification
there, before SDP/reply transport selection/hub/source subscription, with exact
registry live/track target routing into a stable source-graph profile/sensor ID.
Live and SETUP track map to the SAME authorized profile, not separate sensor
identity. `request_policy` still owns legal Session/Transport/body/state checks.
Unsupported/different target or command must not become another profile/effect.

After real core OK, trusted backend binding maps numeric RTSP account/single
scope/connection to exact canonical `camera_access` device/account/profile/sensor
IDs. Require domainRTSP, roleVIEWER, scopePROFILE and operationLIVE—not UI/admin,
snapshot, controls or wildcard sibling-profile rights. Backend must keep the
RTSP credential generation identical in core and policy, with separate durable
HA1/domain/realm mapping. Build `camera_access_verified` ONLY after cryptographic
proof, never an incoming `verified` boolean. Its session_generation is a separate
nonwrapping boot/account issuance counter, not nc/nonce serial; reuse one owner
policy principal per current authorized connection/nonce instead of exhausting
16 session slots on every keepalive. Checked ms->ns conversion and a common
trusted monotonic/boot domain are required. Revoke/close retires owner principal.

Before each source/hub/packet/media effect recheck BOTH Digest grant and shared
policy against exact live resource, health/CPU/producer lease readiness. Expiry,
credential/policy revoke and peer teardown must release existing subscriptions;
a successful old DESCRIBE does not authorize arbitrary SETUP/profile/client or
stale media. Current scalar server logs do not print wire Authorization/target;
retain that discipline, never add raw header/URL diagnostics to receipts.

HOST fixture implements this narrow chain with real request/input parser, real
Mbed TLS Digest verification and real access-policy issue/check. Its synthetic
`camera0`/`camera0-main`/`camera0-sub` IDs reuse existing access-policy test graph
conventions, not measured retail topology. Only scalar effects are modeled;
there is no native principal installation, driver, media bytes or NVR simulation.

## Reproducible HOST/ARM link qualification

```sh
python3 -B apps/rtsp-auth/wire/build.py wire01 --toolchain /path/to/existing/armv5-eabi--musl--stable-2025.08-1
```

Use a new name each run. Outputs and immutable source-version snapshots live in
`local/validation/rtsp-wire/<run>`; existing runs refused. Every exact compiler/
version/command/timestamp and source/header/runtime/link/binary hash is in its
REPORT.json. Optimized, ASan/UBSan, C++→C linkage and pinned Bootlin ARMv5 musl
static link are qualified; ARM ELF is NEVER run. The existing read-only input
parser's compact legacy style alone gets `-Wno-misleading-indentation`; its
instrumented GCC `explicit_bzero` nullability diagnostic is retained with only
`-Wno-error=nonnull` on that frozen dependency. A HOST null-retire regression
checks the existing guard, without editing its source or removing sanitizer
instrumentation. Original failed compilation stays preserved in its fresh run. All new
code keeps `-Wall -Wextra -Werror`. No installs/network/devices/private inputs.
Real crypto config/vendor/helper are referenced read-only, pinned and unchanged.
Runtime merged ELF Thumb-2 metadata is not native ISA/ABI qualification.

Parent independently reviews all APIs/diffs/pins and reruns a fresh recipe before
adoption. Native CSPRNG, secret-memory isolation, combined budgets/media workload,
strict authenticated transport/session state and actual FFmpeg/NVR compatibility
remain open. **One parent next step:** insert the reviewed admitted parser/core/
backend-policy chain at the existing pre-dispatch boundary, including per-effect
revoke/expiry/close cleanup, without advertising native auth until real tests pass.
