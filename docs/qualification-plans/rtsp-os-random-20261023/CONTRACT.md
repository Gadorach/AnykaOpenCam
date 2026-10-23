# Public RTSP OS randomness — contract before code

2026-10-23 · SEC-06 / STREAM-07 · new isolated SOURCE candidate.
Only `apps/rtsp-auth/rtsp-os-random/` and this qualification directory are owned.
No original runtime/bootstrap/provisioner changes, registration, main or adoption.

- Export `qc07f_rtsp_os_random_ready_v1(void *, unsigned char *, size_t)`, matching
  the actual owned-runtime `candidate_config.random_ready` field. Compile-check
  that exact field type without constructing or mutating a config at runtime.
- Context is ignored, stateless: no native ownership, userdata, cached readiness,
  selector, allocation or Boolean entropy qualification. Every valid request
  makes **one** real Linux `getrandom(out, 32, GRND_NONBLOCK)` call.
- Null output or length other than32: `CAMERA_INVALID`, untouched, no syscall.
  For a trusted valid writable32-byte span: clear all32 with the existing actual
  `mbedtls_platform_zeroize`, call once, and return `CAMERA_OK` only for exactly32.
  Every error/short result returns `CAMERA_NOT_READY` after wiping the full32.
  No retry, loop, seed, fallback, cached bytes or output validation/statistics.
- No `GRND_RANDOM` or `GRND_INSECURE`: Linux's documented urandom/CRNG readiness
  semantics give EAGAIN before initialization with NONBLOCK. Initialized reads
  <=256 bytes have documented full-read/non-interruption semantics; nevertheless
  any short, EINTR, ENOSYS or other error is refused, not explained away.
  This is an OS API contract, not a measurement of entropy quality/health.
- Trusted caller supplies a legitimate, disjoint writable span and serialized
  lifetime. No arbitrary-pointer, hostile-memory or concurrent-alias guarantee.
  No128KiB exception; adapter has only scalar locals and caller's32-byte output.

## Actual scope / ordering

Owned-runtime `server/candidate_auth.c` has exactly two calls: startup32-byte
probe (line99) and per-peer32-byte public RTSP session identifier (line129).
Both are wiped by backend `rv_wipe`. Verifier seeds/nonces have a separate owned
CSPRNG, unchanged. No passwords, credentials, HA1, keys, certificates, TLS/HTTPS,
provisioning, retention callbacks, UI or crypto algorithms are changed here.

Future trusted caller chooses this function **before** provisioner begin's config
snapshot and bootstrap prepare/start. This delivery does not edit/call bootstrap,
mutate a live config, register a callback or waive any bootstrap prerequisite.
Independent ordinary review and parent selection are required for integration.

## Bounded evidence selected

Compile HOST and retained musl ARMv5 with real headers and actual original Mbed TLS
`platform_util.c` under the fixed RTSP-only crypto config; no stubs/old builders.
Fresh refuse-overwrite outputs, exact argv/tool/source/dependency/object pins,
logs, REL/readelf/nm inspection. Generated objects/ELF are not curated Git inputs.
One distinctly preprocessor-guarded HOST-only harness may call the actual callback
once, valid32, real HOST getrandom. It checks only return code, wipes output, prints
scalar PASS/NOT_NATIVE only; no output bytes/hashes/statistics. Author audits it
before this sole call. OS failure is preserved and stops the lane, with no retry.
No additional functional/negative/fuzz/adversarial/mutation cases.

No listener/socket, child/fork/verifier/provisioner/provider/backend/media/native,
hardware, network, private configuration, clock framework or target execution.
Camera OFF/23g prefix HOLD is inherited parent context, not freshly queried here.
Linux6.18.55 target getrandom/ARM syscall/cold CRNG readiness remain **NOT RUN**.
HOST success does not qualify target boot entropy, verifier seeding, long-lived
keys, global CSPRNG health, public trust or SEC-06/G3/product completion.

## Read-only references

- `docs/plan/camera-foundation/09-SECURITY-AND-CONFIGURATION.md` SEC-06 readiness.
- `apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.{c,h}` actual callsites.
- `docs/qualification-plans/rtsp-owned-runtime-20261023/PARENT_DISPOSITION.md`.
- `docs/qualification-plans/qc07f-bootstrap-20261023/CONTRACT.md` frozen ordering.
- Local `/usr/share/man/man2/getrandom.2.gz`, Linux man-pages6.19, dated2026-02-08:
  DESCRIPTION / GRND_NONBLOCK / signal interruption / return-value checking.
  Read locally only; reference is not authority for extra effects or native proof.
