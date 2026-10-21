# 09 — Authentication, authorization, secrets and configuration transactions

Entry: [index](00-INDEX.md). Prefix: **SEC**. Depends: CORE/PLAT. SEC-01–05 are
dependencies of protected streaming G3; persistent/network/reset integration
continues through G6. This is product functionality explicitly requested.

## Baseline

Lab RTSP lacks authentication. Host-only preview is not a protected on-device
UI. Runtime WLAN secrets have private handling, but this is not a product secret
store. Lab root password/permanent UART debug were separately authorized;
neither defines the new UI/RTSP defaults. Shared configuration service is missing.

## Tasks

- [ ] **SEC-01 — Identity and authorization model.** Stable device identity,
  separate UI/admin and RTSP accounts/password domains, roles/scopes for viewing,
  control, configuration, recording access and setup. Define ONVIF credential
  mapping without merging independent UI/RTSP passwords. No public endpoint can
  invoke driver diagnostics or access arbitrary filesystem/MMIO paths.
- [ ] **SEC-02 — Reuse a small reviewed security stack.** Evaluate maintained
  TLS/HTTP/authentication libraries on ARMv5 for binary size, memory, CPU and
  licensing. Select a measured design rather than implement cryptography from
  scratch. Choose password verifier parameters using real target timing and
  memory; bound login work to prevent CPU/memory exhaustion.
- [ ] **SEC-03 — Credential lifecycle.** First-use admin provisioning, password
  changes, independent RTSP credentials, authorization checks and secure reset
  semantics. Use protocol-appropriate stored material: a salted UI password
  verifier is not automatically usable for RTSP/ONVIF digest authentication.
  Protect any digest-equivalent secrets as credentials. Never log password,
  PSK, Authorization, URL userinfo, secret payload hash or private config contents.
- [ ] **SEC-04 — Web sessions/API protection.** Random sessions from qualified
  entropy, constant-time secret comparison via library helpers, expiry,
  rotation/revocation, CSRF protection for state changes, origin policy, request
  limits and login throttling. Authenticate media, WebSocket/event streams,
  snapshots, playback/range requests and control APIs, not just HTML pages.
  Prevent path traversal, symlink escapes, untrusted file references and replayed
  transaction IDs from becoming actions on another resource.
- [ ] **SEC-05 — RTSP/ONVIF authentication.** Implement the selected interoperable
  challenge schemes with proper nonce/replay/session lifetime and algorithm
  compatibility. Do not offer Basic credentials over unprotected HTTP as a
  default. Document transport protection and local-network limits for supported
  NVR clients; do not claim TLS encryption for ordinary RTSP/TCP. Qualify actual
  clients before choosing weaker compatibility exceptions.
- [ ] **SEC-06 — TLS and clock/entropy readiness.** Per-device certificate/key
  creation, fingerprint/trust onboarding, expiry/rotation and private key
  permissions. No assumed public trust or automatic external DNS/ACME changes.
  Bootstrap without valid wall time deliberately; preserve monotonic session
  expiry and never silently weaken certificate checks. Ensure CSPRNG readiness
  before generating long-lived keys or sessions on an entropy-poor device.
- [ ] **SEC-07 — Versioned settings schema.** Typed settings for device identity,
  imaging/orientation/profiles, credentials, IP/WLAN/AP, PTZ calibration, lights,
  audio, motion masks and retention. Capability-aware validation, bounded values,
  schema migrations, defaults and desired/applied generations. Distinguish
  settings accepted, committed, applied, pending reboot and failed.
- [ ] **SEC-08 — Durable storage backend.** Identify a camera-owned writable
  configuration location and capacity on the actual flash/SD/boot layout. Start
  with disposable RAM fixtures; implement atomic replace/journal/CRC/versioning,
  fsync ordering and recovery from torn writes using the filesystem's measured
  semantics. Separate credentials/calibration/user settings from firmware, logs
  and recordings. Missing/corrupt config enters documented setup/recovery rather
  than formatting media or replacing rollback files.
- [ ] **SEC-09 — Transaction engine.** Validate a proposed configuration, prepare
  resource changes, record operation ID and rollback snapshot, commit/apply,
  publish result and retire previous state. Idempotent result retrieval after
  link disruption/reboot. Resolve concurrent clients with revision checks;
  failed migration/application keeps a known-good configuration or explicit
  recoverable pending state, never ambiguous success.
- [ ] **SEC-10 — Reset/provisioning integration.** SETUP owns the settings-reset
  workflow; SEC supplies session revocation and defaults. Preserve device
  identity/hardware calibration/recovery and footage per documented reset scope.
  “Admin defaults” means usable protected onboarding, not copying the lab root
  password into a network service. Define which installation-specific controls
  reset and which manufacturer/physical calibration remains.
- [ ] **SEC-11 — Process/filesystem isolation.** Service UIDs/groups and device
  permissions, least-privileged web/media workers, narrow privileged control
  broker and controlled writable directories. Disable unneeded product listeners
  and arbitrary command endpoints. Keep authorized debug/recovery behavior
  explicit; do not silently change the user's established UART/root access.
- [ ] **SEC-12 — Negative/security qualification.** Bad/expired credentials,
  credential-domain separation, replay, CSRF, oversized/malformed requests,
  traversal, resource floods, concurrent password change, interrupted settings
  commit and reboot. Verify no secret leakage in UART, logs, images, crash bundles
  or browser URLs. Test protection on target, not only on a host mock.

## Gate / output

G3 needs working independent RTSP/UI authentication and protected media/control;
G6 needs durable settings and recovery/reset integration. Deliver protocol and
credential policy, schema/migration fixtures and access-control matrix. Do not
mark a password form backed by an unauthenticated API as protected product UI.
