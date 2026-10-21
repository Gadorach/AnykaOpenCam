# Derived V3 server ↔ owned isolated RTSP verifier — SOURCE / HOST ONLY

## USER HALT checkpoint — 2026-10-13

**PARTIAL / FROZEN, no queue advancement.** Last completed `integration05`:
80875 wire/source +5886 focused checks optimized; 80608 +5817 sanitized;
36+11 campaigns each, C++ PASS, HOST memory shape and ARM server/adapter/child
static links PASS (ARM never executed). Exact TESTED source is preserved in
that run's `inputs/`; native integration/product acceptance was not performed.

Three subsequent source edits are **UNBUILT / UNTESTED** at HALT:
`public_dummy_material.c`, `fixture.c`, `client_test.py`. They correct the
historical BAD-DOMAIN-labelled fixture, which in integration05 actually tested
startup credential-generation mismatch, to an explicit PUBLICDUMMY foreign-domain
marker rejection. No generator/build/test/listener is run after HALT. Do not
mark the current source tree fully fixture-passed from integration05. This README
has also received documentation-only HALT text. Tested/current source snapshots,
exact diff and failures are sealed under the lane evidence directory.

Remaining, only if user resumes: independently review current/tested diff and
finish a fresh offline build of those last edits; parent controls all subsequent
native lifecycle/producer qualification and central plan/status updates. No
central documents, target, new unit/dependency, private generation or Git work.
The dependency chain below is deferred, **not authorization to proceed now**.

SEC-05/11, STREAM-04/07. **Independent parent review required; native HOLD.**
Only this new directory and `evidence/delegated/20261012-rtsp-isolated-server/`
are written. Shared V3, childV2, native qualification, framework, UI/HTTPS,
crypto/vendor, producer, launcher, plans/runner/pins remain read-only. No target,
UART, camera network, watchdog, ROM/photo/audio/footage/card/private material,
proc/fuser/status/bench, Git, installation, publication or cloud operation.
RTSPNOAUTH/media501, G3/G8/NVR/TLS/audio/durable first-use entropy/later ports OPEN.

## Actual adoption / custody

`upstream/` preserves byte-exact reviewed V3 server/backend/header/fixture/client
and V2 adapter authority. `PROVENANCE.json` records originals, read-only hashes
and exact reviewed V2 HOST/delay/native code IMAGE pins. Build emits explicit
V3→derived server/backend/header/fixture/client diffs and the V2 adapter overlay.
Server entry is a distinct `camera_rtsp_isolated_candidate_serve`; existing
`camera_rtsp_serve` and shared candidate ABI/source are unchanged.

New C backend holds **no RTSP credential, Digest core, HA1, username/realm,
password, local crypto verifier or camera_access principal**. It owns `rv_start`
and actual `rv_client`, not a grant copied from the finite native helper or a
caller-provided verified flag. Caller passes preopened IMAGE/startup FDs plus
PUBLIC source routing/limits. Server only reads the reviewed nonsecret348-byte
prefix ending BEFORE username; it matches device/account/sensor/profile IDs,
domainRTSP, explicit SHA256/MD5, boot/epoch/generation and budgets. No material/
Authorization/username/realm/nonce/session/token values or payload hashes enter
receipts/manifests. Public code/source/runtime/license hashes are allowed.

`server_crypto_config.h` enables SHA256 IMAGE-pin checking/zeroization only.
Separate child/bootstrap `rtsp_crypto_config.h` enables explicit RTSP SHA256/MD5;
no UI/HTTPS TLS/PBKDF2 object/config reuse. Shared wire-limit code is linked with
function-section GC; server binaries must have NO Digest HA1/provision/verify or
principal-issue symbols. Real crypto and actual RTSP policy remain in the pinned
isolated child. The separate `public_dummy_material.c` creator is never linked
into the server: PUBLICDUMMY-only fixture creator, no private mode/entropy/key
generation. Parent HOST harness owns its fresh0700 scope, creates one EXCL0600
1536-byte dummy record, closes writer and passes readonly FD. No contents/file
hash inventory. Server fixture contains no password/HA1 generation/literals.

## Narrow local adapter overlay / concrete V2 API gaps

Original V2 wire, child, signal ABI, isolation, image gate, barrier/pidfd ownership
and control implementation remain unchanged. `verifier_adapter_overlay.c` is a
controlled derived source with ONLY:

1. `is_rv_pending_result`: pending occupancy observer. V2 `WIRE_MISSING` equals
   `RV_LOST`(-101), and `WIRE_SPACE` equals `RV_UNCERTAIN`(-104). Status alone
   cannot distinguish a retired denial from transport failure. The reviewed
   adapter clears pending ONLY after bound RESULT and exact RETIRE_RESULT ACK.
   Observer zero means that transition occurred, **not authentication/liveness**.
2. Actual OWN wait status captured inside the existing successful `waitpid`
   branch; `is_rv_wait0` observes real owned exit0. BYE/self-wipe and actual wait0
   are both mandatory for normal initialized-child shutdown success. Signaled /
   nonzero known exit may release its dead client, but returns HOLD—not wipe/pass.
3. `is_rv_context_bytes`: PUBLIC allocation shape only.

No protocol extension, child edit, fabricated READY/principal/ACK, filter bypass,
new queue/transport/listener/guardian or numeric-PID signal. All signals go through
the original immutable owned pidfd. On UNKNOWN retirement, config's mandatory
copy-only `retain_verifier` sink retains allocated client and process namespace;
backend is not wiped/freed or respawned. Caller must retain provider code/backing
and original hub/demand/frame metadata until independent authoritative source
END/STOP/reset and verifier retirement. Fixture failure hold retains its actual
owner process for its parent, never pretends a finite STOP succeeded.

V2 has **no public principal tuple/role export or credential rekey API**. Backend
does not fabricate duplicate policy principals from reply kind/typed flags. Its
public graph must match startup routing; every protected effect calls child CHECK
against actual child-owned VIEWER/PROFILE/LIVE policy. Exporting a principal to
other domains/services is HOLD. Fixed-owner REVOKE permanently fences this
verifier lifetime (adapter's revoking latch); no new session/password rotation,
automatic restart/reprovision, old-generation restamping or cached TTL fallback.
Parent-managed fresh bootstrap after actual owned retirement is a separate task.
Expiry reason is not exported by V2; retry challenges are explicit INITIAL /
stale=false, never an invented stale=true hint. NVR compatibility remains HOLD.

## Exchange / effect control table

| Boundary | Actual required exchange and invariant |
|---|---|
| Startup | Exact IMAGE class/pin and FD metadata; rv_start owned fork → actual pidfd → pre-exec release → READY15/profile/budget → provision ACK. No listener before success. |
| Connection | Child OPEN assigns nonreused connection; fixed4 child peers, persistent child global/account rates plus server outer receive/churn limits. Public random session is not auth. |
| Challenge | Source-free 401 only: exact negative CHECK with impossible handle → bound CHALLENGE ACK/rate/nonce issuance → negative CHECK before each header continuation. No protected effect uses this admission path. |
| Verify | Actual borrowed full Request-Line/header/body bytes submitted; child reparses exact method/target and mandatory qop=auth/algorithm/nonce/nc/cnonce/profile policy. No Authorization-derived reconstruction. |
| Result | One pending job. Discard/drop/revoke do NOT free it. Actual RESULT + exact RETIRE_RESULT ACK required before reuse; ambiguous numeric status is never retirement proof. |
| Protected effect | Real CHECK exposes expiry solely for deny-after-cost; mandatory rv_query performs another live policy/liveness CHECK. Fresh same-boot monotonic read after both IPC calls must remain before expiry. No cached grant/Boolean permission. |
| Source BEGIN/next | BEGIN/START LOGICAL ONLY; bound live guard inside START/next and again after BEGIN. First currently allowed peer must pass final IPC immediately before acquisition/publication; earlier peer count is not authority. |
| Reply/SDP/setup/play/RTCP | Child queries/readiness/owner current before each effect and each header/body write. 401 exception is source-free and cannot SDP/subscribe/attach/capture. |
| RTP | Exact hub lease + current producer generation/state + child IPC before packet creation/header/payload and every partial continuation. Fresh send deadline after IPC/control callback cost; unchanged2000ms AU deadline. |
| Release/END | Validate and copy keyframe/token/generation/size scalars BEFORE destructive producer release. No payload/packet-field access afterward. END is exact original bracket, not DMA retirement. |
| Source uncertainty | Exact hub/demand/frame copied to sinks. Global failed retire fences cleanup detach/idle rather than assuming STOP. No END/STOP retry, free/unload or reset permission. |
| Shutdown | Original rv_stop SIGTERM courtesy/fallback → actual BYE/self-wipe + OWN wait0. UNKNOWN retains client; no signal-request-as-proof. Source retirement remains independent. |

401 negative CHECK proves only the current owned exchange, not a valid grant or
known-peer oracle: successful CHALLENGE itself binds connection/epoch/generation
and rate admission. The private serialized owner cannot close/rekey between its
ACK and continuation without this backend observing cancellation/revoke. V2
cannot expose an unauthenticated admission capability; adding one would require
separate scoped child review, not widening negative CHECK into media authority.

All authority objects/input/output/callbacks must be disjoint and callbacks
bounded/serialized with no producer reentry. Config graph/routes frozen per serve.
Same boot/time namespace follows actual rv_start fork/exec with no unshare/clock
namespace change; actual CLOCK_MONOTONIC regressions latch closed. Request crypto
deadline ≤2s; control IPC ≤1s; receive framing/request8192/Authorization1024/body4096,
challenge512/reply-header2048/packet1404, four peers, one crypto slot, one pending
reply and one1MiB legacy hub unchanged. No 2S/10S/guard12/raw-NAL CRC/web-child2M/
40M/CMA16/MAXNAL/128K allocation ceiling or producer policy changes. No producer
allocation/header ceiling increase, AU TLS copy, new driver or native listener.

## HOST tests / memory / reproduction

```sh
python3 -B apps/rtsp-auth/isolated-server-candidate/build.py <fresh-run> --toolchain /path/to/pinned/armv5-eabi--musl--stable-2025.08-1
```

Only writes `evidence/delegated/20261012-rtsp-isolated-server/runs/<fresh-run>`;
existing names refuse overwrite. Full input snapshots/diffs/exact commands/
compiler/header/runtime/link/license closure and scalar results are retained.
No recursive inventory/hash of generated fixture startup payloads. Existing
native child pin remains unchanged even when identical-source child is separately
static-linked for ARM closure: new ARM build is not substituted into runtime.

Owned127.0.0.1 ordinary plaintext RTSP TCP PUBLICDUMMY clients use independent
Python hashlib versus real Mbed in actual hardened HOST4MiB/READY15 child. ASan /
UBSan instrument server/adapter paths; actual optimized child stays fixed locked
4MiB, not an unlocked shadow-VM/native fallback. Explicit synthetic fixture flag
mandatory; fixture CLI refuses root. Strict native backend is tested only for
actual nonroot before-material refusal, never borrowing root flags from mocks.
Read observer reports only count/max prefix bytes on OWN dummy FD, never contents.

Inherited36 wire/source campaigns plus focused URI/qop/downgrade, pending busy /
discard/drain/drop/revoke, four-peer bound, partial-RTP revoke, actual child death,
source-retire failure and clock-regression campaigns use actual sockets/crypto/
owned wait/leases. Original failures remain with fresh result names. Public
synthetic1MiB NAL/IDR/P and full raw CRC/guards are NOT valid decoded footage,
hardware, combined load/FPS, NVR, producer or native authentication acceptance.

`memory_shape.c`, compiler stack-usage and public ARM scalar shape metadata bound
the declared C objects. There is one caller-provided legacy hub CPU buffer, one
provider-owned borrowed export and four1404-byte cursors—no per-peer full AU.
HOST fixture separately declares a1MiB+32 synthetic source and1MiB+32 hub; it is
NOT proof native allocation/backing exists under128K ceilings. Native parent
provider/raw backing, CMA/non-CMA placement/loans, reserve/headroom, CPU deadlines,
image residency/TLS/driver/producer live sharing still require measured closure.
Child native process stays strict2MiB; HOST4MiB and sanitizer VM are not native
budgets or RSS evidence. PUBLIC IPC latency totals/max are HOST callback costs,
not camera FPS/load, memory-fit or hard real-time guarantees. No proc/RSS reads.

**ONE next parent dependency chain:** independent source/diff/fresh HOST review;
then parent alone qualifies the strict native verifier's root-origin/2MiB/
READY15/provisioning/owned lifecycle under fresh external protection using the
already-reviewed finite guard-only pack; then separately qualifies actual native
producer handoff/memory/source retirement before adopting protected RTSP effects.
No source-worker target operation or actual protected native server is authorized
or inferred by this candidate. All native/product/other-port gates remain OPEN.
