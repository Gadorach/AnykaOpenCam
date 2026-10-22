# Author static inspection / actual callsite and dependency closure

Final candidate pin: `be0fee5d7aaa55aba5616d04689e309ee9a38255376188ede074cd5be1c7392d`,
340 lines/26563B. This is source reasoning and AST/text comparison, **not** executed
cases, independent acceptance, actual privacy/timing/capture or native evidence.

## Narrow delta and unchanged-body closure

Full original322-line source was read before TIMER_CONTRACT.md. Final text diff
has three hunks, +19/-1 lines: ROOT13, formatter66–79 plus blank separators, and
status223–224. No original source/other files were changed. An early copied
diagnostic string lost its final clause; the first static text diff detected it,
and that candidate-only transcription error was restored before final syntax/pin.
This was not a test, runtime failure or change to a frozen earlier artifact.

The final AST inspection removes only that new helper/gate and substitutes the
old ROOT assignment in an **in-memory AST metadata copy**. The result equals the
entire original AST without location attributes. This projection is not code
generation, a runtime fixture or execution. Seven old top functions are identical;
only capture contains the added gate; imports/CLI/nested functions/rest of module
are identical under the exact narrow projection. SOURCE_DELTA01.json records it.

| Original -> candidate lines | Static finding |
|---|---|
|13 ->13;14–16 ->14–16| ROOT constant preserves canonical original logical base. CONTROL/SOCKET/PORT expressions unchanged; fixed CONTROL derives to workspace/work/qc-uart-live, not apps. No path lookup/assertion of current safety by this author. |
|61–64 ->61–64| capture_status exactly unchanged, including default private recent_rx suppression and exact-True private override. New metadata is not produced here or advertised as a capability. |
|71–110 ->87–126| Capture guards, private directory/output/control checks, serial setup, source hash, start and arm initialization unchanged. `start` remains the one original `time.monotonic()` float assignment at118, duration the original argument; no reassignment/renewal added. |
|111–136 ->127–152| Existing transmit/response closures and pacing/serial methods unchanged. Loop predicate still `running and time.monotonic()-start<duration`, not a rewritten deadline test. |
|138–195 ->154–211| RX/write/redaction/buffers, last-boot transport behavior, full catch trigger/cancel patterns, SF/kernel/password/prompt handling and read-only inspection are unchanged, not newly qualified. |
|204–207 ->220–225| Original state initialization/private capability update precede added gate. Existing utc/tx_busy calls keep order and expressions. New metadata/CPU work appears only in exact private request, before unchanged response call. No timing equivalence is claimed. |
|208–247 ->226–265| Full arm action/modes/keys/prior-CLI reads/plan checks and exact response including UTC ends_by unchanged. Arm dict still no timer; no new expiry/coverage grant. |
|248–288 ->266–306| Observe/public send/protected send/private send/stop/unknown action/error and observer branches unchanged, including protected128B limit, watermark/pending checks and fixed private refusal text. |
|289–306 ->307–324| Normal end/error/finalizer, close/unlink association checks, original output finalization/hash/chmod behavior unchanged. Not a new cleanup/retirement guarantee. |
|308–322 ->326–340| CLI/options/request validation/client output unchanged. Existing private_status/protected-send CLI refusal remains. No new launch path, flag, action or installation. |

All15 `time.*` Call sites remain with the same AST order and enclosing original
expressions after projection. Arm reply AST and all constants, two original
asserts, source hash and finalizer are within that equality, not merely a selected
token count. Preserving those bodies does not qualify their existing races,
assert-based guards or broad inherited effect surface.

## New formatter closure, no I/O or new clock

Source66–79 has six Call nodes: two `type`, two fixed `ValueError`, one
`start.as_integer_ratio`, one `start.hex`. No imports added, file/process/clock/
FD/signal/sysconf/serial/socket/owner/namespace calls, callback or mutable timer
state. Strings `time.monotonic()` and the predicate are metadata, not calls.

Type guards reject subclasses/bool/mutable/arbitrary objects before value methods;
no repr/str/coercion on input. Duration must be exact int120..14400. Start must
be exact float, nonnegative and finite; as_integer_ratio rejects NaN/infinity,
with both documented error classes sanitized. Signed zero is preserved by hex,
not normalized. Finite float range gives bounded representation/ratio size; no
unbounded input-length loop, extra data buffer or global cache is introduced.
The ratio is discarded; neither it nor newly derived time is returned.

The fixed nine-field dict returns stored float hex, original integer duration and
fixed schema/API/predicate/scope/arm-role labels only. It is not a ready/auth flag,
bound/deadline, host-clock accuracy/continuity proof or status of the arm. No raw
new sample or current remaining value exists. Type checks do not authenticate
recorder/source/epoch association, and direct external helper use is not selected.
Conventional Python globals are not protected against arbitrary monkeypatching.

## Actual status callsite / privacy and refusal

The only new formatter call is source224, nested directly in status branch220
under `private_capture and req.get('private_status') is True` at223. No else or
extra body, no call on default/public/nonprivate status, and no public feature
flag. Private capture with truthy non-bool request does not enter. Existing
`capture_status` receives the old argument unchanged, so its existing RX policy
is not broadened. No formatter cache/shared state can later leak a prior value.

Formatter ValueError reaches inherited private handler301–303 (original283–285):
fixed `Private control request refused`, no raw start/duration/exception payload.
No start/duration/arm assignment or retry is added. Normal handled refusal returns
to the same capture loop; existing event/socket-handler failure can still reach
outer error/finalizer—no unconditional liveness claim. Python traceback locals
may retain values; do not export tracebacks/debug locals. No secure-erasure claim.

New absolute timer metadata is private only. The preexisting privileged response
already contains source/envelope and possibly recent RX; this increment supplies
neither a new authentication route nor independent source authority. No actual
status request/response or private value was created, opened or recorded here.

## Dependencies and source association

Imports remain argparse, datetime, fcntl, hashlib, json, os, re, select, socket,
stat, sys, time, pathlib.Path and external serial (pyserial). They were not
imported through this unit, resolved at runtime or qualified here. HOST compile
does not verify installed dependencies/permissions/hardware behavior. Only the
added helper is pure; the module's inherited capture/client/main are effectful.

Candidate117 still hashes `Path(__file__).read_bytes()` when private capture is
explicitly run. It therefore identifies the candidate bytes, not the old pin.
Immutable Boot33/136/226–231 bind the old SHA/canonical script; PrivateBoot133–138
compares captured source to that old SHA. Existing helpers do not select/admit
this copy. No spoofed hash or edits to those checks/launchers/profile occur.

The new constant does not grant permission to read the inherited historical CLI/
analysis paths or write original outputs. All future source-to-running-code,
canonical path, sole peer/PID/start/adapter/catch/raw/host namespace associations
remain parent prerequisites. C1/equal-clock guards/strict policy remain unchanged.
