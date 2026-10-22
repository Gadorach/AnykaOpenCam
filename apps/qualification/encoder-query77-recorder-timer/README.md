# Recorder timer metadata — unreviewed actual recorder derivative

**DO NOT RUN/IMPORT THIS CANDIDATE.** `uart_capture.py` is a full effectful recorder
derivative, not a pure library, installed entrypoint, native recipe or accepted
controller. Its inherited CLI can open UART/capture files/control sockets and
handle TX/catch/other actions. No capture/client/formatter/helper invocation,
installation or deployment is selected. No runnable example is supplied.

Original322-line/25426B recorder SHA256
`135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00`
is unchanged. Candidate340 lines/26563B, SHA256
`be0fee5d7aaa55aba5616d04689e309ee9a38255376188ede074cd5be1c7392d`.
Only ROOT association, one formatter and one actual status call are changed.
Pre-code contract and evidence:
`docs/qualification-plans/encoder-query77-recorder-timer-20261022/`.

## Narrow observational interface

The new nested `native_capture_timer_v1` is inserted into the existing status
response only for a private capture AND exact `private_status is True`. It is
absent on public/default/nonprivate status; no new RPC action, argv or public
capability. Existing private recent-RX handling and capability fields are unchanged.
The inherited CLI still refuses private-status requests; no new access route.

The fixed schema returns `schema`, `version`, `start_encoding`, `start_hex`,
`duration_seconds`, `clock_api`, `loop_predicate`, `scope`, `arm_timer_role`.
`start_hex` exactly represents the original **stored** built-in float `start`
using float.hex; duration is the original built-in int120..14400. No coercion,
new clock reading, integer-ns claim, deadline computation, now+remaining or renewal.
Type/subclass/range/nonfinite refusal uses fixed safe errors and the existing
private RPC refusal path; it does not assign/restart any capture/arm timer.

Predicate identity is `running and time.monotonic()-start<duration` with the
original float subtraction/comparison and loop-boundary checks. An exact stored
float is NOT an exact nanosecond sample/usable expiry bound, loop-stop deadline,
liveness, valid catch or coverage grant. No independent arm timer is introduced:
arm is bounded by capture lifetime and can be consumed earlier. The existing
derived UTC `ends_by` reply is unchanged and is not equated to a native deadline.
Absolute timer values are private and must never be placed in Git/public logs.

## Paths and source identity

ROOT is explicitly fixed to the original canonical implementation directory,
preserving CONTROL/work/qc-uart-live, historical CLI/analysis dependencies and
public-output association. No caller root selector or copied marker. The author
did not access those referenced private/control/capture files or run their readers.
This fixed path is a source association, not proof it is currently safe/correct.

The unchanged `capture_source_sha256` expression hashes the **new** __file__, not
the original. Old Boot/PrivateBoot path/source guards are unchanged and reject
this candidate until a separately reviewed fresh profile admits new source/path
association. No old pin/launcher/profile is changed; no automatic adoption.

## Evidence and remaining blocks

HOST AST/syntax only; code object discarded unexecuted, no serial dependency
import or runtime/build closure. Final static delta is +19/-1 lines/three hunks;
projecting only the selected ROOT/helper/status changes yields the complete old
AST. This is author inspection, not independent source/API/runtime acceptance.
New serialization has CPU/allocation cost; no latency/RSS/native128KiB/40MiB claim.
All inherited assertions/effect routes/weaknesses remain, not newly qualified.

Strict continuity operational BLOCK remains; no changed policy/trust assumptions
or new generic continuity packet. PID tick/namespace mapping, float/native-expiry
model, actual source/peer/catch/RXTX/capture/host identity and recovery/manual
readiness remain separate. C1 is not integrated. Both historical failures stay
failed, old catch consumed;12:12 U-Boot is not current state.

Source provenance/notice is retained from the task-created original helper;
`NOTICE.md` is pinned in the report. No explicit SPDX header existed in the copied
source; none is invented here and no global license/publication grant is made.
Independent review is required. No automatic call/test/runtime/settings/native/
physical/next-worker queue follows.
