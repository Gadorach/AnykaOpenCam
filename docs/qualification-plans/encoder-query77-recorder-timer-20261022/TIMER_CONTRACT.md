# Recorder native timer observation — contract BEFORE code

2026-10-22 supplied task date; no current clock read.
ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Own only fresh recorder-timer source and
matching docs directories. Full original322-line recorder was read before this
contract. Original25426B/SHA256
`135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00`
and all other source/design/reviews/HOLDs remain immutable.

## Selected actual derivative, not a wrapper or pure service

Copy the actual recorder into the new source directory; never import the original.
It retains its inherited executable CLI/UART/socket/TX/catch/filesystem/finalizer
surface. **Not pure, installed, selected for execution, safe-to-run or admitted.**
No source operation, candidate/helper/formatter/library/CLI import/call/execution,
fixtures, synthetic traces, mocks/tests or code-object execution. Only static
public read/hash/AST-equivalence and new-text HOST AST/in-memory syntax compile.
No clocks/get_clock_info/sysconf, private/HOLD/control/capture/current state/proc/
dev/serial/UART/RPC/network/device/process launch or Git/index/old-source/AGENTS/
central-plan/model/provider/safeguard actions. Parent alone hardware.

## Exact allowed source delta

1. ROOT becomes the fixed canonical original implementation directory
   `/home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z`.
   CONTROL expression is unchanged, still that directory's parents[1]/qc-uart-live
   (canonical workspace/work/qc-uart-live). No root selector, symlink discovery,
   copied markers, Boot import or original path write by the author. This prevents
   copying the module under apps from relocating all historical dependencies and
   public outputs. __file__ source hashing still hashes the **new** candidate file.
2. Add a small pure formatter for original stored start/duration values. It may
   use only exact built-in type/range checks and finite float representation
   methods; no file/process/clock/signal/FD/getter or callback.
3. In the existing status branch ONLY, add nested `native_capture_timer_v1` only
   when `private_capture` AND `req.get('private_status') is True`. No capability
   advertisement, public/default/nonprivate exposure or new RPC action/CLI option.

No other authored behavior change. In particular preserve original start102,
while136, all existing time/UTC call sites and their relative order, duration
parsing/guard, source hash101, capture_status61–64, status fields205–207, arm245–247,
catch/trigger/cancellation/SF/prior-CLI guards, protected128B/pacing/serial settings,
all writers/observer/actions/exception/finalizer/CLI bodies. Existing asserts and
inherited weaknesses are preserved, not newly qualified or broadly repaired.

## Exact representation and fixed schema

Nested schema fields are fixed:

- `schema`: `qc07f.recorder.native_capture_timer`, `version`:1.
- `start_encoding`: `python.float.hex`, `start_hex`: exact `.hex()` of the
  **stored** built-in float start, not a new sample or normalized clock value.
- `duration_seconds`: unchanged original integer duration120..14400.
- `clock_api`: `time.monotonic()`.
- `loop_predicate`: `running and time.monotonic()-start<duration` (source identity).
- `scope`: `STORED_VALUES_ONLY_NOT_DEADLINE_OR_ADMISSION`.
- `arm_timer_role`: `CAPTURE_TIMER_ONLY_NO_INDEPENDENT_ARM_DEADLINE`.

Formatter rejects type/subclass mismatch, duration bool/float/noncanonical range,
negative/nonfinite start with fixed ValueError text, never raw input/errors.
Signed zero may be represented exactly as stored; finite nonnegative float range
otherwise has no invented uptime ceiling. Exact type checks precede all value
methods, no coercion. as_integer_ratio may validate finite float (its ratio need
not be returned); nonfinite ValueError/OverflowError are sanitized with from None.
Existing private RPC exception path returns fixed `Private control request refused`
and leaves capture timer/arm untouched. No validation at capture start is added.
Serialization has bounded small authored objects, not total Python/native RSS fit.

## What is expressly NOT exposed/proved

No computed deadline, now+remaining, integer nanoseconds, freshly sampled time,
current remaining/liveness, simultaneous M/B/W tuple, catch validity, coverage
grant or ready/PASS field. Original float subtraction/comparison rounding and
loop-body/stop latency remain for later modeling. Raw stored float identity is
not raw integer-ns clock accuracy. No separate arm timer exists in arm dict;
arm lifetime is bounded by capture timer only and actual consumption can end it
earlier. Existing derived UTC ends_by reply and its call order stay unchanged,
not replaced or equated to a native deadline.

Absolute host timer values remain private. Source field names/constants/public
source pins are permitted in Git, not actual values or responses. Existing private
status also exposes other privileged information; do not broaden its existing
access policy or mistake truthy non-bools for this new gate. New CPU serialization
cost is not measured timing or a latency guarantee.

## Source association and handback

The unchanged capture source hash expression uses candidate __file__, not the old
SHA. Existing Boot/PrivateBoot pinned source/path checks must reject this derivative
until a separately reviewed fresh profile/source association is explicitly selected.
No old pin/helper/launcher/deployment change, automatic selection or invocation.
No C1 integration. Full parent authentic PID/start/peer/adapter/host boot/namespace/
RXTX/catch/capture/source association remains missing. Strict continuity remains
operationally BLOCKED, with no different trust assumptions accepted; no new generic
continuity packet. PID tick mapping, float/error/native-expiry model, source
interface adoption/recovery and current manual readiness remain separate blocks.

Deliver source provenance/licenses, exact before/after pins, narrow actual text/
AST delta and callsite closure, new-unit syntax receipt, unrun functional checklist
and source/method seal. Independent review required; one unreviewed candidate,
no automatic implementation/runtime/host settings/native/physical/next queue.
