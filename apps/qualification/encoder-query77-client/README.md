# Real encoder OPEN_QUERY qualification client — compile-only candidate

ENC-02 / PLAT-01 dependency, not a camera app, stub provider or readiness service.
Read `docs/qualification-plans/encoder-query77-client-20261022/SOURCE_CONTRACT.md`.
No execution, native recipe, bootstrap, upload or boot is selected here.

`encoder-query77-qualification.c` has a real explicit main. Default invocation
returns2 before I/O; its exact one-trial opt-in is NOT hardware authority. Future
parent admission must precede any use. Only `/dev/watchdog0` and
`/dev/qc07f-encoder` can be opened as devices, once each, in the final one-shot
process after all fork/exec. Fixed sysfs metadata/uid/release checks do not prove
sole ownership, image identity, global idle, prior DMA retirement or recovery.

One initial5s watchdog configuration, no keepalive/feed/renewal, immutable1.5s
effect-phase budget. One encoder open/two88-byte queries, exact version/schema,
same nonzero session and identical virgin/no-work snapshots required. ENOTTY is
failure without identity fallback; no old76 support. No56-byte global query,
INFO/encode/capture/reset/control/refclear/media/module/network/peripheral I/O.

Any uncertain/late operation keeps all still-owned descriptions/process in a
nonreturning no-service failstop; close errors clear ownership before one close
and are never retried. Failstop is not external retention or guaranteed recovery.
Failed/late disable may already have stopped the timer. Future capture/catch and
manual OFF fallback remain mandatory. Native watchdog/app acceptance is unchanged.

Normal path closes encoder once, logically disables watchdog once, then closes
inactive watchdog once. No magic write (which would feed). Numeric fields are
written only after normal retirement, two lines in frozen struct declaration
order: version, bytes, relation, frame, quarantine, DMA-uncertain, guard-poison,
telemetry flags, session, owner, generation, last-dispatch, operations, result,
kind, ledger-active, reserved. No addresses/raw bytes/UUID/digests/device labels.

No authored malloc, threads, aliases, child, sockets, files written, payload
buffers, or caller-supplied device paths.17-field output buffer384B; query88B.
Static linking does not prove target memory-fit or bounded kernel waits.128KiB,
40MiB/CMA and2MiB child/4MiB HOST limits remain unchanged; not a verifier.

`encoder-open-query.h` is a byte-identical GPL-2.0-only import of
`apps/qualification/kernel76-encoder-identity/encoder-open-query.h`; retained kernel
copy must also match. No query implementation is copied/changed. Authored code
is GPL-2.0-only, see the repository's retained Linux GPL-2.0 license text at
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux/LICENSES/preferred/GPL-2.0`.
Toolchain/libc licenses remain separate; generated static ELFs stay local ignored.

## Offline build only (not a native-use recipe)

`compile_only.py buildNN --toolchain <retained armv5-eabi--musl--stable-2025.08-1>`
creates a fresh refused-if-existing local build directory. It invokes only compiler,
linker subtools and readelf; no built client/help/default invocation or tests run.
Both HOST and ARM are static ELFs. Full retained toolchain map, pre/actual consumed
header maps, link maps/archive input pins, command/loader logs, output seals and
timestamps are retained. Final-observed loader/library pins are distinguished
from pre-use pins. See the matching `CANDIDATE_REPORT.md` for actual results.
