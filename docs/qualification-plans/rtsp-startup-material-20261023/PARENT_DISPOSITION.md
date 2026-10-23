# RTSP startup material — source increment closed

2026-10-23; SEC-01/03/05, STREAM-07. Accept **narrow conditional SOURCE/API
sufficiency** after the separate ordinary review. No correction required in this
bounded constructor. This is not a running provisioner, media adoption or G3 GO.

Delivered `apps/rtsp-auth/rtsp-startup-material/startup_material.c`/header: the
operator-supplied independent RTSP username/realm/password goes through the real
existing Digest/Mbed TLS helper into the actual1536-byte native verifier startup
record. Device/account/sensor/profile, generations, explicit algorithm/rate/TTL
policy and exact resource URIs bind to the same current owned-runtime public
config. No default credential, qualification password, UI secret reuse, silent
MD5 fallback, new crypto or media/HTTPS linkage is introduced.

Secret output is cleared before validation and on crypto failure, including
padding/tails. Borrowed password is never copied to output; caller still owns its
zeroization. All credential material, including HA1, remains secret and must never
enter public logs/hashes. Construction does not prove supplied credential-domain
independence, authorization, bootstrap generation authority or entropy readiness.
Trusted valid/disjoint/serialized objects and immutable config/routes are required.

## Evidence

- Constructor C SHA256: `5eb23e473750c0b3ae0daecd472c2091ec1a6e8287001c65b9d1634fdd3aceae`.
- Header SHA256: `76fb100e06c6b7508b6011400bf35734d273615fd26a3aa8cc57cd2150630206`.
- Independent review SHA256: `c8f2a1d9964dba8e496fb4a247dbacf0338cbc9ad233ee03a46c4cdb54555a8e`.
- Build01 report SHA256: `c7e06a428e0306a4daf6dd0a4ff469bdb4355c22765710c322b28f6ffe4ec6e4`.

HOST/retained ARMv5TE soft-float compile and ELF REL combination passed:20 actual
C compilations plus six link/metadata commands; no diagnostics. Real constructor,
Digest/wire/graph/registry/parser and crypto units were compiled, no stubs/main.
Parent checked review hash,16 selected input pins,10 compile source pins,165
consumed dependency records,22 artifacts and26 command logs. Frozen owned-runtime
source/document pins remain unchanged. Reviewer cited compile evidence without
rerunning it; review scope/limitations are retained unchanged in its REVIEW.md.

No new build framework/script was created. Actual public argv/tool/environment/
source/header/output pins and logs are in build01's report. Generated objects
remain local. No candidate/library/test/fixture/executable was called. No real
operator credential, secret fixture or startup material was read, generated,
hashed, persisted or transferred during this work; only source was compiled.

## Remaining feature boundary

The function belongs ONLY to a separately hardened authenticated provisioning
context before media starts. Real secret ingress, private output-FD writer/handoff,
storage, durable generations, account authorization and live password rotation
remain absent. It must not run in media/UI/HTTPS or mutate a running config.
The existing backend still requires independently admitted exact image/material
FDs and real RNG/launch policy. This component is a startup-record constructor,
not that missing private descriptor pipeline or an installed provisioner.

App bootstrap/external media owner/recovery,128KiB-compliant backing and40MiB/CMA
admission, main/packaging and native authenticated PLAY/decode/STOP remain blocked.
Next feature connection is the real separate provisioning-to-bootstrap private
descriptor handoff; do not manufacture readiness or reopen diagnostic-build/clock/
receipt infrastructure. Further adversarial testing remains deferred. No hardware
state queried/changed; both native HOLDs intact and no native queue. No worker
remains active. Curate only source/notices/public metadata locally, ordinary index
unchanged; no binaries/private material or push.
