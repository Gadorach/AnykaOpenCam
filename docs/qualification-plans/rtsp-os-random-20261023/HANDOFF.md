# Public RTSP Linux OS randomness — author handback

2026-10-23 · SEC-06 / STREAM-07 · **delivered, independent review pending**.
No source acceptance, callback adoption or native qualification is self-declared.

## Feature

`apps/rtsp-auth/rtsp-os-random/os_random.{c,h}` exports
`qc07f_rtsp_os_random_ready_v1(void *, unsigned char *, size_t)` with an exact
compile-time type binding to the actual owned-runtime `candidate_config.random_ready`.
Stateless ignored context; no config construction/registration/mutation.

Null/unsupported length returns CAMERA_INVALID untouched, before any write/call.
Trusted writable32 is cleared by actual Mbed TLS zeroize; one real
`getrandom(out,32,GRND_NONBLOCK)` call; only exact32 returns CAMERA_OK.
Every short/error returns CAMERA_NOT_READY after full32 zeroization. No loop,
retry, fallback, cached readiness, byte validation/statistics, seed, allocation,
selector or new crypto. Legitimate disjoint serialized C output is a prerequisite,
not arbitrary-pointer safety. No verifier nonce/seed or TLS/key/credential changes.

Only the runtime's existing startup probe and public RTSP session bytes are in
scope; its existing `rv_wipe` calls remain unchanged. Future caller must select
the function before provisioning snapshots/bootstrap prepare. No integration was
performed: constructor/provisioner/runtime/bootstrap and central plans are frozen.

## Actual bounded evidence

| Evidence | Result |
| --- | --- |
| HOST GCC16.2.1 callback + actual original Mbed TLS zeroizer | 2 C compiles PASS |
| Retained musl GCC14.3.0 ARMv5TE soft-float callback + zeroizer | 2 C compiles PASS |
| Distinct guarded Linux x86_64 HOST-only one-call harness | 1 C compile/link PASS |
| HOST / ARM relocatable closure | 2 REL links PASS; readelf/nm inspected |
| Exact commands | 26, all exited0; no warnings in compile logs |
| Sole actual HOST callback call | **PASS**, exit0, `HOST_OS_RANDOM_ONE_CALL_PASS NOT_NATIVE` |
| ARM executable/native/target/CRNG boot observation | **NOT RUN**; no ARM executable built |

Fixed `compile01.py` compiles the actual original
`apps/auth-web/service/vendor/mbedtls/library/platform_util.c` using the unchanged
RTSP-only `rtsp_crypto_config.h`. No stub, object substitution, old builder,
generator, archive/kernel/image build or original source edit. Production objects
have no HOST-fixture defines; only the dedicated harness has
`RTSP_OS_RANDOM_HOST_ONE_CALL`. ARM output is ELF32 little-endian ARMv5TE/EABI5
REL, not a linked native libc/syscall proof. Compiler stack estimate for callback
is16B per role, not whole-runtime fit/latency evidence or a128KiB exception.

Ordinary author audit (`AUTHOR_AUDIT.md`) and `PRECALL_SOURCE_PINS.json` were
written before the one HOST invocation. `host_once.py` created an exclusive
attempt directory, verified public source/artifact/tool/dependency pins, and ran
the fixed harness once with no inherited loader/config environment. Harness
called only the real callback once, checked CAMERA_OK and wiped all32 before
printing. It did not inspect, expose, store or hash random bytes. No extra
positive/failure/negative/adversarial/fuzz/mutation cases or rerun were selected.
Scalar observation timestamp: `2026-10-23T11:57:50.236521+00:00`.

Readelf/nm show no backend, verifier, provisioner, provider, listener or media
implementation in the harness. The general zeroize-and-free function exists in
the unchanged original zeroizer REL but is discarded from the HOST ELF; adapter
does not call it. No private material, process hardening, child/fork, socket,
listener, target program or hardware operation was performed.

## Reproducibility / pins

Source aggregate (7 candidate source/support files; definition in SOURCE_PINS):

`f1a8691ee681b11e3cbb0b4c5d8535721a695840540ac80a307be226c0e3ff64`

- `INVOCATIONS.json`: observed top-level argv/results, each invoked once.
- `runs/build01/COMMANDS.txt`, `command-*.json`, `logs/`: exact26 argv, cwd,
  fixed environment, exit status and captured compiler/inspection output.
- `runs/build01/INPUTS.json`:16 selected initial source/context/license/man-page
  pins; `host_once.py` was added afterward and is separately pre-call/source-pinned,
  not represented as a compiled unit. No compiled source changed afterward.
- `runs/build01/DEPENDENCIES.json`:328 consumed header/source records across five
  units,164 unique paths, including real HOST and musl sys/random headers.
- `runs/build01/TOOLS.json`:30 compiler/tool/link-input records (overlapping
  categories). `TOOLS_SUPPLEMENTAL.json` separately pins the interpreter,
  underlying Buildroot `.br_real` driver and potential link plugins after the
  call; this is file provenance, not a traced tool-execution claim.
- `runs/build01/REPORT.json`:75 artifact pins plus successful command records;
  it correctly remains the **pre-functional** compile/inspection report.
- `runs/host01/`: sole attempt/invocation, scalar stdout, empty stderr, exit0
  observation. Do not run the helper again. Both helpers refuse existing outputs.
- `AUTHOR_FINAL_VERIFICATION.json`: post-call file-only verification of recorded
  public inputs/artifacts/dependencies/tools and unchanged pre-call source/audit;
  no extra functional call. `DELIVERY_PINS.json` seals public handback metadata.

Generated objects, dependency/stack files under objects, RELs and HOST ELF are
excluded by this lane's own `.gitignore`; parent must not curate generated binary
artifacts. No Git command/index/commit or outside-owned-path write was performed.
Licenses stay scoped as recorded in `LICENSE_NOTICES.md`.

## Not established / next task

Linux API documented NONBLOCK readiness is **not** entropy quality/health testing.
Target Linux6.18.55 getrandom behavior, ARM syscall availability/link/runtime,
cold CRNG readiness, actual target seeding, verifier CSPRNG, TLS/keys, global
entropy qualification, SEC-06/G3/product acceptance and any native use remain open.
No Boolean/host success establishes those facts. External ingress/launch/storage,
bootstrap deployment, backing/recovery/retention and other parent blockers remain.

Camera OFF/23g unexplained-prefix HOLD is parent context only, never queried or
changed here. No serial/UART/RPC/HTTP/network/device action or target queue.
No further security matrix or build/clock framework was opened.

**One next task:** independent ordinary SOURCE/API/bounds/evidence review, then
explicit parent selection before any bootstrap integration or native use. No
second HOST call or native recipe is selected by this handback.
