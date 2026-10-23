# Ordinary author audit — before sole HOST invocation

2026-10-23 · not independent acceptance. Source was read after compile01; no
callback/test/backend/native program has been run at the time of this audit.

## Bounds and actual path

- `os_random.c` lines17–18 refuse null/unsupported length before writes or OS call.
  No invalid-span clearing; nonnull writable32 remains a trusted C precondition.
- Lines20–25: actual Mbed TLS full32 preclear, one getrandom call with only
  GRND_NONBLOCK, exact32 success, full32 wipe on every other return. No loop,
  fallback, flags selector, state, entropy test, allocator or credential ingress.
- Exact actual `candidate_config.random_ready` type is compile-asserted, with no
  config object or emitted runtime registration. Only original header declarations
  are included; no candidate/backend function implementation is linked.
- Fixed compile recipe executes compilers/linkers/readelf/nm only. Five C compiles
  and26 commands exited0. Both zeroizers are compiled from actual `platform_util.c`
  under `rtsp_crypto_config.h`, no stub or old builder. No HOST defines on either
  production callback/zeroizer compile. Only harness gets its distinct HOST macro.
- Readelf: ARM REL, ELF32 little-endian ARMv5TE/EABI5, no program headers/entry.
  HOST harness is x86_64 ELF. Callback compiler stack estimate16B on both roles,
  not a whole-runtime memory budget or native timing proof.
- HOST ELF global symbols: actual callback, Mbed TLS zeroizer, libc getrandom,
  stdio/startup/stack protection only; no backend/verifier/provisioner/provider/
  listener/media/native entrypoints. General `mbedtls_zeroize_and_free` is in the
  original zeroizer REL object but section-GC removes it from the HOST harness;
  callback never calls it. ARM REL's libc getrandom remains unresolved until a
  future real native link; no ARM executable is produced/run.

## Selected single helper invocation

`host_one_call.c`: dedicated macro + Linux x86_64/non-ILP32/non-ARM guards,
rejects unrelated existing fixture classes. Main has one direct callback invocation
on its legitimate32-byte stack output with ignored NULL context. It checks only
the result, wipes via actual zeroizer before reporting/exit, and emits scalar
PASS/NOT_NATIVE (or scalar refusal code). No byte inspection, hashing, statistics,
private input, config, backend, hardening, socket, fork or target operation.

`host_once.py`: fixed one ELF path, no options/selectors; exclusive `runs/host01`
marker before effects. Rechecks compile inputs/tool/link/artifact/dependency pins
before invocation. One `subprocess.run`,10s host wait bound, no retry/fallback;
empty inherited environment except fixed LC_ALL. Stores only scalar program output,
exit status and public provenance, never receives/hashes RNG output. Failure stops
with marker retained; no rerun selected. Postchecks are file hashes only, not
additional callback calls. Python's own interpreter/libc initialization is not
being characterized or counted as callback evidence.

Selected next command: `python3 apps/rtsp-auth/rtsp-os-random/host_once.py` once.
HOST OS success is not cold-target readiness/health, verifier seed/nonce/TLS/key
qualification, bootstrap adoption or any SEC-06/G3/native/product acceptance.
