# Actual RTSP password → startup material feature

2026-10-23; SEC-01/03/05, STREAM-07. Source + HOST/ARM compile only.
Selected after reviewed owned-runtime integration (`5b3e252`); diagnostic-image,
clock/receipt infrastructure and adversarial tests stay parked/deferred.

Implement a small real provisioning-side constructor for the existing1536-byte
`rv_material`, bound to the actual owned-runtime `candidate_config` and exact
RTSP resource registry. Use the existing `camera_rtsp_digest_ha1` and vendored
Mbed TLS, not new cryptography, random qualification passwords or UI credentials.
No new secret schema, main, filesystem writer, daemon, network API or defaults.

Only a trusted separately hardened provisioning process may call it, before
media startup. It receives an explicitly supplied RTSP username/realm/password
as bounded borrowed secret spans. It never generates, reads, logs, hashes for
evidence, persists or transfers real operator material during this work. The
existing helper computes protocol-required secret HA1, not a public digest.
Password is never copied to output; output is credential-equivalent SECRET.
Caller separately owns authentication/authorization, secret-input hardening,
zeroization and actual private output-descriptor handoff. It must not link or
invoke this constructor in the media process or mutate a running backend/config.

Bind public device/account/sensor/profile, nonzero generations, algorithm,
nonce/rate policy and exact live/track URIs without defaults. Native account1
and RTSP domain are fixed by the existing actual backend. Preserve explicit
SHA256/legacy-MD5 selection; never silently downgrade or enable MD5 in HTTPS.
Reject HOST fixture configuration, malformed bindings/limits and unsupported
profile; success is material construction only, not backend/entropy readiness.

All input/output spans and typed objects have trusted valid lifetime and are
serialized/disjoint. Clear the entire fresh output before validation and on
failure, including padding; crypto-library wipe provides explicit destruction.
No allocation, IO, clocks, fork, media init, hardware, tests or secret execution.
Compile this separate bootstrap object and real dependencies HOST/retained ARM;
do not rebase frozen owned-runtime or run any resulting object. Keep all original
source/evidence immutable and keep generated objects out of Git. Independent
ordinary source review remains required before adopting this new component.
