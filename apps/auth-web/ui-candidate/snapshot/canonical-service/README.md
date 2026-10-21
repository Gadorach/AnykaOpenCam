# Canonical HTTPS authentication service — signal-return repair pending qualification

SEC-03/04/11/12, WEB-03/04. Maintained canonical source in `apps/auth-web/service`.
Native functional HTTPS campaign03 passed after the writev fix, including owner
revocation and discard/drain; SIGTERM lifecycle failed and triggered qualified
recovery. The failed ARM ELF's ordinary musl `__restore` uses syscall119, whereas
the filter allowed only rt_sigreturn173. The new ARM-only SYS_sigreturn permission
allows returning from an existing handler, not sending signals or installing new
handlers. `signal_abi_test.py` interprets the actual production filter with pinned
compiler-derived ARM syscall numbers: old filter fails the desired119 case; fixed
filter passes with signal-sending/open/process denials intact. Host test also
returns from an actual ordinary SIGTERM handler after filter installation.
This is offline/host evidence, not native shutdown acceptance. Evidence:
`evidence/qualification/qc07f/auth-signal-return-20261012/` and the retained
`auth-stdio-native-requalification-20261012/native03-functional-PASS-shutdown-FAIL/`.

The following stdio diagnosis and original qualification scope are retained;
new signal-return native effect is pending, while stdio marker repair was observed.
Original historical builds/evidence remain immutable. The parent confirmed that
the failed native ELF's musl `__stdio_write` issues ARM EABI146/writev, absent from
the earlier filter. The only new production permission is `ALLOW(SYS_writev)`
for existing owned-FD write authority; the positive direct stdout writev test
and existing denial matrix pass. Native HTTPS effect is NOT yet requalified;
no memory cause or product completion is inferred. No target/UART/guest input,
kernel/global account/debug/provider change, dependency install or publication.

## Chosen policy: peer-local grant discard, not process restart

Ordinary login disconnect, extra TLS bytes, HTTP login wait expiry and owner
credential revocation/replacement no longer call destructive `cp_cancel` or
unconditionally set service fatal. The owner immediately discards the exact
pending grant and closes its connection/anonymous session. It retains the only
job binding and expensive-work slot until the bound child RESULT arrives.

The strict child and broker are unchanged: the actual full600k KDF continues,
its local password/material/derived output is wiped, and `cw_forget` must succeed
(worker inactive) before the child sends RESULT. Only then may the parent forget
that job and admit the next. A late MATCH cannot issue access. **Logical discard
is neither CPU interruption nor process retirement.** The child remains alive
and locked; ordinary recovery uses the same process, boot/child generations,
credentials, request/command counters and throttle history. No fork/restart,
second KDF, secret queue or new syscall permission is introduced.

Unknown and known accounts have the same admission, full600k work and discard/
drain path. One outstanding job remains BUSY even after its peer vanishes; all
other valid sessions/read-only status requests remain usable. Native prior idle
cost24.804768586s remains parent evidence, not measured login latency here.
An abandoned request can monopolize verification for the remaining real KDF
duration, bounded by the **original40s hard CPU/job deadline**, not instant reuse.

The original `cp_cancel`, hard job expiry, child loss, malformed IPC, clock/entropy
failure and observed pidfd/waitpid retirement contracts remain fail-closed. A KDF
overrun/hung child or other hard fault still disables/ends the service; **automatic
hard-fault restart supervision is NOT implemented**. This is explicit degraded
availability scope, not a continuously fault-tolerant product-ready claim.

## Narrow added process APIs

`cp_discard(client, ticket)`:

- Exact internally pending `(child_generation, sequence, request_generation)`.
- Sticky grant-discard latch, preserves full original result binding/deadline.
- Running result stays WAITING/BUSY. A received MATCH is also demoted irrevocably.
- Terminal bound response becomes `CP_DISCARDED`, regardless of MATCH/NO_MATCH.
- `cp_issue_access` refuses discarded jobs; it cannot consume a running binding.
- `cp_forget` succeeds only after terminal response; no early slot reclamation.
- Owner credential mutation of an outstanding **already-discarded** job may use
  the existing bounded ACK channel. Undiscarded active mutation still follows
  the original destructive retirement contract. RPC can consume only bound
  RESULT/ACK packets; stale/forged/replayed IPC remains fatal.

`cp_release_access(client, owner_principal)`:

- Only an exact field-wise owner snapshot actually tracked by this client matches.
  Every fact field, canonical ID, generation, revision and snapshot serial is
  checked. No wire/HTTP victim/principal selector is added.
- Calls the policy revoker with the **internally stored snapshot**, not caller
  memory, before wiping/freeing the tracked slot. No bare policy-slot memset.
- Existing policy `DENIED` specifically means no exact active snapshot remains
  (e.g. already swept expiry), allowing conservative tracked-slot reclamation.
  INVALID/clock/ownership failures are not ignored and disable the client.
- Replayed/mutated/foreign snapshots fail; a reused slot cannot resurrect an old
  principal. Account session-generation and policy serial history never reset.
- Web logout/idle expiry/revocation and failure cleanup tear down dependent queued
  responses before release. Recycled anonymous session indices are additionally
  checked against their owner serial, not confused with an abandoned older job.

## Private launch ABI v2 and unchanged isolation

`web-service-arm /trusted/credential-child-arm` with only private FDs3..7:

| FD | Input |
|---|---|
|3|Parent-provisioned P-256/SHA256 PEM certificate, <=8192B|
|4|Matching private PEM key, <=4096B|
|5|Dummy56 + account56: LE schema1/count600000/salt16/verifier32|
|6|`web_config`, 768B, **version2**|
|7|Exclusive anonymous owner SOCK_SEQPACKET capability|

V2 replaces the first8 formerly reserved bytes with `response_ns` (1ms..35s),
the HTTP login wait budget only. Remaining24 reserved bytes must be zero.
The secret child hard deadline remains40s and cannot be changed by HTTP. Optional
`X-Login-Wait-Ms` can only shorten a particular CSRF/Origin-bound login wait to
1..configured milliseconds; longer/invalid/duplicate hints reject before work.
This tests real request expiry without weakening/faking the KDF or CPU deadline.
`runtime_ns=0` explicitly selects continuous service; nonzero1..3600s retains
finite qualification lifetime. All session/request deadline additions remain
bounded, with checked monotonic error/regression/overflow/horizon failure.

The trusted bootstrap still securely selects protected device-owned0700 tree,
owned/root-owned regular0600 single-link files and passes read-only descriptors
at offset0. Service checks metadata independently; it cannot infer directory
ancestry from an FD. Missing/provisioning/entropy/certificate/hardening failures
never expose a default DUMMY network service. OOB/pinned certificate trust mode1
does not fabricate qualified UTC/public CA trust or implement durable rotation.

Parent and child remain nonroot/equal UID/GID, scoped zero core/dumpability,
zero capabilities, mandatory locking, no-new-privileges and default-deny seccomp.
Child filter bytes are unchanged. Parent adds owned-FD writev and ARM-only ordinary
signal-frame return; no new open/device/process/signal-sending operation. Child PIDFD/execveat/FD
closure/PDEATHSIG/sole waitpid ownership are unchanged. Actual device identity,
groups/permissions and ARMv5/time64/futex/locking/syscall ABI need parent native
qualification on actual75; root debug/UART policy is untouched.

## Actual service routes and resource ceilings

Pinned Mongoose7.23 **external** Mbed TLS3.6.7, byte-identical measured full
TLS1.2+1.3 config. No builtin TLS/default RNG/dashboard/OTA/file/driver endpoint.
Real `/`, `/app.js`, `/api/session`, `/api/login`, `/api/logout`, protected
`/api/status`, `/api/capabilities`; protected media/snapshot admission returns
**501 unsupported_unqualified**, not fake video. All alternate media/WS/control/
settings paths authenticate before unsupported errors. UI/ADMIN and RTSP remain
independent; existing RTSP is **NOAUTH LAB, not wrapped**.

Full600k library verification; qualified nonblocking OS getrandom/PSA startup,
256-bit cookie and separate CSRF, Secure/HttpOnly/SameSite=Strict/__Host-/Path=/,
library constant-time comparison/zeroization, exact Host/Origin, expiry/revocation
before effects. No secret/Authorization/Cookie/key/material-hash logs/argv/env/URL
or production images. Uniform public admitted/discarded/drained event booleans
contain no account, ticket, token, material or match status.

Two admitted TLS peers, one handshake, five-second absolute handshake/header
completion;2s send drain;8KiB/24 headers,512B URI,4KiB body/login128B,16KiB receive,
8KiB response. Socket send/recv requested8KiB (kernel accounting not inferred).
16 accepts/s,32 requests/s, dominant >=1s account/global login spacing;8 web
sessions and16 **concurrent tracked principal** slots with exact release, not a
16-login lifetime cap. One active KDF, no per-peer encoder/frame buffer. Explicit
heap wrapper bounds1MiB total/128KiB individual/1024 slots; excludes child,
libc-internal allocations/overhead, stacks and kernel buffers. No native2MiB
headroom or15fps claim from host/static file sizes.

## Finite reproduction and review

From the workspace:

```
export AUTH_WEB_ARM_TOOLCHAIN=/path/to/existing/armv5-eabi--musl--stable-2025.08-1
python3 apps/auth-web/service/build.py runNEW
python3 apps/auth-web/service/host_https_test.py runNEW hostNEW
python3 apps/auth-web/service/finalize.py runNEW hostNEW handoffNEW
```

Builders derive the project root from their canonical location; no old workspace
absolute path or ignored historical source/library cache is used. `--toolchain`
may instead select the existing read-only toolchain explicitly. `--output-root`
selects a directory under project `local/`; default output is
`local/build/auth-web/service/runNEW/`. Tests/finalizers accept an absolute run
directory or workspace-relative local path as well as default run names.
Both HOST and ARM full TLS libraries are rebuilt from canonical vendored source,
validated against `apps/auth-web/dependency-pins.json`, with compiler/header/
runtime/library hashes and failures preserved. No upstream scripts/install run.
Fresh output names refuse overwrite. The9-unit service/5-unit strict child
compound,600k,35HTTP/40CPU,CFG768v2,crypto/IPC/policy/vendor/config/license bytes
are unchanged; only parent stdio writev and ARM ordinary signal-frame return
permissions have changed production behaviour. Every build additionally runs the
offline compiler-derived ARM filter ABI regression. To reproduce its old-filter
counterexample or fixed result separately, use `signal_abi_test.py` with a fresh
run name and the same explicit toolchain; `--expect-denied` is for pre-fix source
only, not a way to bypass the desired regression on current source.

Host-only PUBLICDUMMY files are fresh0700/0600; host service compile-time loopback
restriction unchanged. Finite180s campaign budget is based on real measured
~1s host600k cost: real old17th-login exhaustion/abort failure, new36 full600k
sequential login/logout/expiry/replay, known/unknown/extra-TLS/request-expiry
discard, dependent session preservation, pending revocation/replacement and
usable subsequent login without manual restart. Separate strict process tests
exercise every release-field mutation, valid source alias, stale snapshot after
reuse, swept expiry, running/received-MATCH discard, BUSY until actual drain,
and original hard timeout with observed own-child retirement. Old availability
faults are reconstructed as explicitly labelled canonical-source HOST models,
not old ELF execution or historical source input. HOST crypto/cancellation test
conditions remain unchanged. Proc/fuser inspection is not performed; resource
evidence uses owned service exit metrics without post-exec VmRSS claims. No mock KDF or
iteration weakening is linked into service/child/process tests.

See fresh analysis report/handoff for actual results, exact hashes, build/runtime/
license closure, retained failed observations and narrow adoption patch. Shared
adoption/target deployment requires parent review. Native memory/locked/socket
headroom in24MiB competing non-CMA, full600k/TLS under media load, genuine browser
video, RTSP authentication, durable secret provisioning, cold entropy/trust and
hard-fault automatic supervision remain OPEN. Other ports deferG8; all product
flags remain false. No host proof is a booted native authentication claim.
