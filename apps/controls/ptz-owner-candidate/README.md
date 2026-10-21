# Portable PTZ owner candidate — CTRL-02/04, bounded CTRL-03/10 prerequisites

**Isolated source/HOST candidate for parent review, not integrated/native-qualified.**
No hardware, GPIO, UART, network, private data or physical movement is used here.
No existing framework ABI, policy, kernel diagnostic, runner or plan is modified.

## Implemented boundary

- `ptz_owner.h/.c`: one caller-serialized registry entry per explicit head; finite
  bounded commands, one same-principal replacement slot, exact work/phase/STOP
  tickets,20-ms minimum cadence, initial maximum four phases and100-ms lease.
  No allocation, thread, signal, sleep or supervisor. Caller drives poll/completion
  and quiesce/dispose; provider independently enforces each absolute phase-neutral
  deadline if the caller/process disappears. This is a **provider requirement**,
  not a demonstrated hardware feature.
- `ptz_auth.h/.c`: immutable trusted physical-sensor→head associations from the
  actual CORE graph, canonical fixed-width IDs, and real shared
  `camera_access_check(...CAMERA_ACCESS_PTZ, CAMERA_ACCESS_PTZ_CONTROL...)` for
  client MOVE/STOP and immediately before each phase. Only UI/ONVIF domains;
  ONVIF must have explicit shared-policy NVR mapping. It never takes a wire
  `verified=1`, authenticates a protocol, issues a principal or copies credentials.
- Initialized storage is address-bound and cannot be shallow-copied/reinitialized.
  Head records remain tombstones; namespace must be trusted lifetime-unique
  across registry restarts. Within a registry, generation/sequence never wrap
  or regress. All phase and STOP tickets are reserved **before** effects so
  exhaustion cannot deny cleanup. One registry per actual graph/head-owner
  authority is required; two unrelated registries cannot prove global exclusion.
- There is no absolute/preset/homing/zoom/FOV/motion-feedback contract or physical
  sensor invention. A main/sub profile is not a second sensor. No unrestricted
  continuous-speed/travel option is advertised. This candidate accepts bounded
  relative **phase bursts**, not calibrated degrees or ONVIF FOV translations.

## STOP and provider fence contract

### V2 public client authority (parent correction)

`ptz_owner_move` and `ptz_owner_stop` reject null, known-storage alias and malformed
schema/ID inputs with `PTZ_INVALID`, **without provider calls, neutralization,
fault/clock latching, queued-intent mutation or ticket reservation**. This also
applies to malformed commands carrying a valid owning principal: no malformed
client command silently becomes STOP. Complete accessible typed input objects
are required; arbitrary invalid-pointer dereference protection is not promised.
Field validation precedes indexing/reservation and never uses unknown axis/count
to address arrays. Sensor-association or real policy denial returns `PTZ_DENIED`
without owner mutation. Shared policy's own trusted-time observations are separate
from owner/provider effects; `now_ns` must not be taken from a wire client.

Valid MOVE checks the actual head grant, then enforces exclusive same-principal
lease arbitration **before** observing owner time or reserving tickets. A valid
different operator is BUSY while another principal owns the head; an invalid
different account cannot clobber it merely because the head exists. An explicit
well-formed STOP with the actual head grant may stop another operator's movement;
that is intentional shared-head STOP authority, not a schema-error side effect.
Parent-trusted `quiesce` remains the explicit cleanup path for genuine internal/
parser owner failures; wire errors must not automatically invoke it. Poll,
completion, dispose and reconfigure are trusted lifecycle/provider APIs, never
unauthenticated wire STOP substitutes. Actual internal clock/completion/provider
faults retain exact-owned safety cleanup and quarantine. Independent owner-driven
expiry/revoke cleanup still works even after the client permission is gone.

V1 source/HANDOFF/run06 and earlier failures were preserved byte-exact in
`evidence/delegated/20261012-ptz-owner/v1-snapshot/` before these edits. Its HOST
tests passed but missed the parent-found malformed-client authority defect;
that old handback is not adoption authority. V2 adds active-A versus revoked/
wrong-domain/no-head-grant/alternate-B malformed-input regression coverage.

Provider submission must enqueue without blocking/reentering. Caller-owned
requests are borrowed **only for the duration of submit** and copied by the
provider. Tickets, not raw pointers, correlate later completions. Clock callbacks
are trusted, bounded monotonic observations in the same domain as caller time.
Measured callback overrun/regression latches fault and immediately requests one
priority neutral fence after the callback returns; C cannot preempt a blocking
callback. The80-ms HOST blocking-provider counterexample demonstrates why the
old kernel mutex/sleep loop cannot become a qualified asynchronous physical STOP
merely by adding logical tickets. This is not actual-kernel execution/timing.

The provider owns safe routing/level/current/duty knowledge and break-before-make.
`PTZ_NEUTRAL` is **not** a0000 coil mask or whole-register write. Readiness bits
declare obligations (measured routing/STOP, bounded phase and independent async
STOP); they are neither authentication nor hardware evidence. Missing obligations
fail closed with no phase or forced-off output. A typed flag alone never sets
`native_qualified`, position-known or calibrated capability. Callback≤1ms and
STOP≤250ms declarations are candidate admission caps, not measured capabilities.

Expiry/revoke/STOP/dispose/process-loss notification drops queued intent and
requests neutralization of **only exact already-owned work**. Cleanup retains
that ownership even after its session expires; it cannot authorize another phase.
A normal exact STOP ACK fences/retirements **all** earlier effects of that work,
including uncertain queued submissions, and forbids later outputs/IRQs. Only
then may replacement work run or an owner dispose/release its head slot. No fake
STOP callback is emitted for a command cancelled before any provider work exists.

Uncertain request/timeout/callback failure keeps work quarantined and storage
alive, sends at most one reserved priority STOP request, and denies more motion.
An eventual exact normal STOP ACK can retire uncertainty, but the fault remains
latched. A definitive STOP failure or contradictory electrical-neutral receipt
is terminal: later alleged ACKs cannot free it. No retry/force-free/reset/reinit
shortcut exists. Parent must retain provider context, policy, binding, registry
and owner until known normal retirement. Whole-process memory loss cannot be
handled by a portable in-process object: the future hardware provider must bound
its last step independently and preserve enough lifecycle evidence externally.

Desired intent, applied provider-policy ACK, separately reported electrical
observation and physical position are distinct. ACK without electrical validity
does not manufacture coil-current proof. Position stays UNKNOWN, with explicit
uncalibrated/slip/reboot reason; phase counts are not position. Installation
pan/tilt sign changes only at idle with newer generation; both preview transforms
remain untouched, with no postdecode compensation.

## Standard-controls mapping for later parent integration

| Operation | This candidate | Required before any native/UI/ONVIF claim |
|---|---|---|
| Relative move | One axis, direction±1,1–4 phases, finite lease; explicit associated sensor and shared head | Measured routing/current, displacement/sign/local limits/duty; calibrated supported units |
| Continuous intent | No indefinite command/speed API; caller may request another bounded burst only under fresh per-effect permission | Measured accepted velocity/duty and STOP, truthful bounded options; no automatic renewal here |
| STOP | Authorized client PTZ_CONTROL path; internal expiry/revoke/dispose cleanup is exact-owned fence only | Independent interruptible provider stop, normal retirement ACK and electrical/mechanical stop measurements |
| Status | Desired/applied/observed/retained/fault/UNKNOWN position and generation | Genuine observation/timestamps and non-native evidence labels; never infer physical moving/idle from a policy ticket |
| Absolute/preset/zoom/autotracking | No operation | Not advertised; no guessed homing/coordinate/FOV model |

No light/audio/reset control or new grants are included. Their later owners must
be separately scoped. The old `camera_control_ops.ptz_velocity/ptz_stop` functions
are synchronous and cannot alone satisfy this asynchronous fence ABI. Future
adapter must implement it rather than treating `camera_control_stop` return as
electrical proof. Parent owns that integration and the real CORE/policy graph.

## Evidence/build

`build.py RUN --toolchain EXISTING_PINNED_ROOT` creates a fresh directory under
`evidence/delegated/20261012-ptz-owner/`, copies exact controlled source/header
inputs, preserves licenses, compiles only those copies and seals commands,
versions, hashes, compiler/header/link/runtime closure, costs and HOST failures.
Optimized and ASan/UBSan executable tests use real copied shared policy/backend
cores plus PUBLICDUMMY synthetic providers. C++ links/runs the C API; pinned
Bootlin ARMv5 musl links are inspected only, **never executed**. The read-only
original GPL diagnostic supplies the03/06/0c/09 pattern and blocking-counterexample
source citation (`ptz.c:19,48–101`); no extracted firmware executes.

One next parent hardware dependency: **C1→C2 exact retail routing/off-current and
independent asynchronous STOP/retirement qualification**, under fresh sole
ownership/runtime/bounded recovery, static reference and safe local clearance.
Historical PID/timer/register snapshots cannot admit it; no blind homing or
unmeasured outputs. CTRL tasks/G4/G8 and native movement remain open regardless
of HOST counts or ARM link success. See the already-reviewed MATRIX/C2/C3 for
physical duties, limits and preservation requirements; this is not another matrix.
