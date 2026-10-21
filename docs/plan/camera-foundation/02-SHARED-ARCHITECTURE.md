# 02 — Shared architecture, device graph and control ownership

Entry: [index](00-INDEX.md). Prefix: **CORE**. Depends: BASE-01/02.
Serves all later subsystems; only QC07F hardware is active until G8.

## Existing implementation

`framework/include/camera/backend.h`, graph validation, lease pool, packet hub,
RTP helpers, fair RTSP cursors and memory accounting exist with host fixtures.
QC07F has a working streaming callback adapter. Much state in
`bringup/qc07f/userland/rtspd.c` and codec code remains global; complete platform
adapters, common lifecycle/config/event services and product APIs are unfinished.

## Tasks

- [ ] **CORE-01 — Freeze service/driver boundary.** Audit current structs and
  actual requirements before versioning the next ABI. Drivers own discovery,
  MMIO/DMA, clocks, reset, physical controls, radio and block lifecycle. Common
  services own authentication, logical producers, fan-out, settings, recording,
  UI and ONVIF. No SoC paths, physical addresses or vendor structs in public APIs.
- [ ] **CORE-02 — Complete graph and capability schema.** Preserve stable
  camera/profile/encoder/PTZ/light/radio/storage IDs, explicit association and
  resource groups, qualified concurrency limits, absence versus unsupported
  versus unqualified states, format/rate ranges and control units. Add audio
  endpoints, imaging controls, orientation, events and recording capabilities.
  Main/sub streams are profiles of a sensor; two physical sensors remain two
  independently addressable cameras even when they share an engine or head.
- [ ] **CORE-03 — Per-instance contexts.** Migrate QC-specific globals into
  adapter/camera/stream contexts in small tested steps. Shared hardware engine
  contexts arbitrate references, bandwidth, clocks and lifecycle; do not allocate
  one full engine per RTSP client or pretend two contexts imply simultaneous
  silicon operation. Allow static registration on constrained targets; dynamic
  loading is optional and must validate ABI/size/lifetime.
- [ ] **CORE-04 — Common producer lifecycle.** Define stopped, starting,
  running, draining, recovering and failed states; generation changes; demand
  refcounts for RTSP/UI/recorder; cancellation and bounded waits. First consumer
  starts one producer, last consumer releases it, recording keeps it alive.
  Deduplicate IDR requests and propagate source failure to all dependent clients.
- [ ] **CORE-05 — Control broker.** Serialize imaging/profile/PTZ/light commands
  through the owning service; expose request ID, desired state, applied state,
  generation, timestamp, result and failure reason. Kernel EBUSY guards actual
  capture leases, not whether a client happens to be connected. Pause/drain or
  use a proved safe boundary for controls; failed/cancelled operations must not
  silently commit desired/applied state. Define bounded priority for STOP.
- [ ] **CORE-06 — Time, events and diagnostics.** Common monotonic timestamps,
  explicit wall-clock validity and mapping generations; camera/encoder sequences,
  drops versus deliveries, queue occupancy, source CRC/guard status, memory high
  water, transport loss, restart reasons. Bound event queues; expose subscription
  resume/snapshot semantics. No invented lux, motor position or synchronized UTC.
- [ ] **CORE-07 — Configuration integration.** Common schema/validation and
  transaction objects from SEC, with platform capability validation. Version
  profile/control settings and resolve stale commands after disconnect/reboot.
  Keep settings persistence separate from low-level driver state.
- [ ] **CORE-08 — Two-sensor fixture before additional boards.** A mock backend
  with two sensors, main/sub profiles, shared encoder/head, delayed completion,
  failed startup and unsupported control must run the same product services.
  Exercise shared-engine admission, no ID aliasing, no cross-camera credentials
  or leases, correct UI/NVR enumeration and recorder namespace separation.
- [ ] **CORE-09 — Backend conformance kit.** Reusable tests for ABI rejection,
  lifecycle, stale tokens, EBUSY, cancellation, missing features, memory accounting,
  generation wrap/overflow, resource release and error translation. Make these
  the later T34/Fulhan adapter entry requirements.

## Completion criteria

Actual QC07F services consume the common backend, not parallel board-specific
service implementations. Two-sensor shared-resource fixtures pass, without
claiming real dual-sensor hardware support. A context can be destroyed only after
callbacks, queued operations and buffer exports no longer reference it. API
errors are stable and machine-readable, with no silent fallback for unsupported
hardware. Deliver a documented API/schema and migration notes with source pins.
