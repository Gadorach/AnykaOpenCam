# Explicit adapter prerequisite — contract BEFORE code

Task date: 2026-10-22, supplied context only; no clock read.
ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Own only the fresh
`apps/qualification/encoder-query77-recorder-explicit-port/` source directory and
this qualification directory. Parent owns central status and all hardware.

## Selected privacy correction

The actual reviewed recorder-timer derivative is the source baseline, not an
import wrapper: 340 lines / 26563 bytes, SHA256
`be0fee5d7aaa55aba5616d04689e309ee9a38255376188ede074cd5be1c7392d`.
Its independent review seal SHA256 is
`a0311797f796cc2632549c254ff6f68f4db7013225a93353b72b5605b6de0b79`.
Original recorder SHA256 remains
`135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00`.
The frozen baseline source and its review context diff contain an inherited
device-specific selector: keep both local/uncommitted and byte-unchanged. Never
copy that literal into new source, docs, diff, receipts or output logs. Before
the first new source write, remove its assignment in memory. No raw old-source
diff is saved. Public path/hash provenance is permitted, not private payload.

This is a full **effectful recorder**, not a pure library or admitted entrypoint.
No import, helper/formatter/validator/capture/client/CLI call, code-object execution,
test, mock, fixture, mutation, launch, installation or adoption is selected.
Only text/hash/AST inspection and in-memory HOST syntax compilation, discarding
the unexecuted code object, are selected. No serial dependency resolution.

## Exact source changes

1. Remove the module's fixed `PORT` assignment. No replacement selector/default.
2. Make `capture(duration, part, selected_port, private_capture_dir=None)` require
   `selected_port` for every API call. No default, implicit None selection,
   environment/config lookup, discovery, scan, USB enumeration or fallback.
3. Add a lexical validator and call it as the first capture statement, before
   private-directory, CONTROL, raw/log/final metadata, serial or socket effects.
   It accepts only exact built-in `str`, total length at most 512 characters,
   exact `/dev/serial/by-id/` prefix followed by one nonempty component. The
   component's explicit public grammar is ASCII letters/digits plus `_ . : + -`,
   with `.` alone and every `..` substring forbidden. Thus embedded slash,
   backslash, NUL, controls, whitespace and non-ASCII are refused. No coercion,
   trimming, normalization, path resolution, filesystem/device query or adapter
   substitution. Fixed safe ValueError text never echoes the rejected value.
4. Remove the CLI port default. In the `--capture` branch, validate the explicitly
   supplied `--port` before either private/public capture path; missing or invalid
   selection gets fixed parser error text. Argparse's absent-option None is only
   a refusal sentinel, never a capture selection. Noncapture request CLI does not
   require a port, validate a port, or add an RPC.

Validation returns no substitute; the exact accepted caller string flows unchanged
to inherited serial selection, status and metadata. Those runtime records may
contain private identifiers and must remain local/outside Git. CLI parser and
Python/module import mechanics are not sandboxed; this ordering claim concerns
the recorder's own explicit capture effects, not interpreter startup/import IO.

## Inherited boundaries remain

Keep the fixed canonical original ROOT and unchanged CONTROL/SOCKET association.
No socket move. Retain original docstring and root NOTICE provenance; invent no
SPDX/license/publication grant. Preserve all other source, including timer
formatter/exact private-status gate, all clock expressions and their order,
start/loop/UTC arm behavior, source hashing of NEW `__file__`, durations, RX/TX,
pacing/protected guards, catch, historical dependencies, observer and finalizer.
The inherited open-intent metadata resolves the already validated selection;
lexical validation itself must not resolve it or claim hardware identity.
No original helper/pin/launcher/profile changes; old source guards still reject
the new candidate. No C1 integration or executable native recipe.

Author verification will project only these explicit-port changes from the new
AST and require full baseline AST equality without executing either tree. A
privacy-safe textual delta, if supplied, must compare only against an in-memory
selector-free baseline and explicitly disclose that omission, not pretend to be
a byte-exact raw baseline diff. Recheck original/candidate/review hashes before
and after; do not edit frozen files or Git/index/central status.

No private state/control/HOLD/capture, clock/proc/dev/serial/UART/RPC/network,
current runtime/owner/catch or manual-readiness observation. Parent alone hardware.
Both FAILED/HOLDs remain untouched; the consumed catch is not renewed. Strict
continuity and all source/owner/recovery/native/product blocks remain. Stored
timer identity is not a deadline, liveness, admission or coverage grant.
Independent review is required; no automatic next operation or worker follows.
