# Explicit-port recorder — unreviewed source-only derivative

**DO NOT IMPORT OR RUN.** This is the actual full effectful recorder, not an
import wrapper, pure library, admitted entrypoint or native recipe. No executable
example is supplied. Inherited capture/socket/filesystem/UART/TX/catch/finalizer
effects remain unqualified. Independent focused review is required.

The frozen recorder-timer source SHA256 is
`be0fee5d7aaa55aba5616d04689e309ee9a38255376188ede074cd5be1c7392d`.
Its source and review context diff contain a private selector and must stay
local/uncommitted and unchanged. This new copy had that assignment and its
defaults removed in memory before its initial write. Never copy the old literal
into source, documentation, logs or Git. The original recorder SHA256 remains
`135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00`.

## Narrow API/CLI correction

`capture(duration, part, selected_port, private_capture_dir=None)` requires an
explicit port on every call, including public and private capture. Its first
statement lexically validates that argument. Exact built-in str only, at most
512 characters, `/dev/serial/by-id/` plus one nonempty ASCII component containing
only letters, digits, `_ . : + -`. The single-dot component and every double-dot
substring are forbidden. No embedded slash, backslash, NUL, control, whitespace,
non-ASCII, coercion, trimming, normalization or substitution. Fixed errors never
echo rejected input. No environment/config lookup, scan, enumeration or fallback.

There is no module PORT constant or selected-port default. CLI `--capture`
requires an explicitly provided valid `--port` before either capture path and
before recorder-owned filesystem/control/raw/metadata/serial/socket effects.
The missing argparse option yields None only as a refusal sentinel; capture
never falls back from it. Noncapture request CLI remains independent of port
selection, with no new RPC. Python/import startup is not sandboxed by validation.

The accepted string is used unchanged. Lexical validity is not adapter identity,
existence, ownership, current state, safety or hardware admission. Inherited
open-intent path resolution is later metadata, not part of validation. Runtime
port/status/argv/event data can contain identifiers and must stay private/outside
Git. This task does not repair or admit the inherited public recorder mode.

## Preserved boundaries

Canonical original ROOT and CONTROL/SOCKET stay fixed; no socket move. Timer
formatter and its exact-private-status gate remain unchanged. Stored timer values
are not new clock samples, deadlines, usable expiry, remaining/liveness, continuity,
catch validity or coverage. RX/TX/pacing/protected guards, durations, start/loop,
UTC arm reply, historical dependencies, observer and finalizer remain inherited.
No128KiB cap or other limit is raised; no timing/memory/runtime claim is made.

The existing source-hash expression identifies the NEW file. Old helper/path/hash
guards remain unchanged and reject it pending separate reviewed source/profile
association. No installation, C1 integration, launcher/source-pin replacement,
timer/catch/lease change, original edit or host-setting change follows.

Pre-code contract and static author evidence:
`docs/qualification-plans/encoder-query77-recorder-explicit-port-20261022/`.
Only AST/text/hash inspection and in-memory unexecuted HOST syntax compilation
are selected, not imports/calls/tests/mocks/fixtures/mutations, current-state or
target IO. Parent alone hardware; both FAILED/HOLDs and consumed catch untouched.
No current runtime/owner/catch/manual readiness is inferred. All continuity,
source/ownership/recovery/native/product blocks remain.

Original docstring/provenance and root `NOTICE.md` terms are preserved. No explicit
SPDX header existed; no license/global grant or publication consent is invented.
