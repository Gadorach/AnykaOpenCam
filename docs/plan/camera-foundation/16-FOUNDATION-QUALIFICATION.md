# 16 — QC07F foundation acceptance, release package and port readiness

Entry: [index](00-INDEX.md). Prefix: **QUAL**. Depends: G0–G7. Primary gate
**G8**, the prerequisite for either remaining hardware port.

## Definition of a working foundation

One common service implementation runs on QC07F using qualified backend APIs:
correct/useful imaging, native hardware streaming at accepted rate, authenticated
RTSP and on-device browser, safe PTZ and fitted peripherals/audio, SD recording/
motion/masks/retention/playback, durable settings and recoverable setup/reset/
network transitions, discovery and actual NVR integration. Capability-aware
dual-sensor/shared-engine fixtures run the same service code. No required feature
is a stub or turned off to fit tests. Unfitted hardware is explicitly absent;
unimplemented required hardware is a blocker, not “unsupported” completion.

## Tasks

- [ ] **QUAL-01 — Requirements/evidence matrix.** Map every user requirement to
  implementation path, task IDs, host tests, actual-target evidence and acceptance
  status. Resolve stale inventory/docs, lingering prototype-only paths and each
  prior failure. A failed experiment remains visible after its regression passes.
- [ ] **QUAL-02 — Freeze release profiles and thresholds.** Select measured
  native resolution/FPS, bitrate/quality/GOP, max clients, audio, memory reserves,
  control latency, reconnect deadline and supported browsers/NVR versions. Use
  the targets below as starting criteria, not retrospective claims. Any change
  to a requirement or hardware ceiling needs a documented decision.
- [ ] **QUAL-03 — Automated host/cross-build suite.** Sanitizers, meaningful
  property/fault tests for ownership/protocol/config/retention, parser fuzzing
  with bounded resources, deterministic media/orientation fixtures and clean
  ARM builds. ABI validation and two-sensor shared-resource conformance tests.
  Do not spend effort on tests that only restate simple reversible edits.
- [ ] **QUAL-04 — Native driver/lifecycle campaign.** Repeated capture/codec/
  radio/storage starts/stops, held exports, allocation pressure, errors and
  removal. Confirm guards/CRC, reference lifetime, exact module compatibility,
  no unexpected IRQs and stable allocated resource counts. Exercise UART while
  the full workload runs; track real dropped/error events.
- [ ] **QUAL-05 — Combined product soak.** Run NVR+browser+camera recorder+audio,
  motion masks, retention, controlled light/PTZ operations and admin polling.
  Record unique frames, pacing/latency distributions, corruption/drop/reconnect
  causes, bitrates, CPU/RAM/CMA/heap/fd/socket/log/temperature trends, card write
  latency and usable free space. No unbounded logs or frame dumps.
- [ ] **QUAL-06 — Fault/recovery matrix.** Slow/silent consumers, link loss and
  address changes, USB/radio errors, wrong credentials, app crash, watchdog,
  config commit/reset interruption, SD full/removal/power loss, invalid clock,
  source stalls and memory exhaustion. Motors/lights reach documented safe
  state, closed footage survives, incomplete data is not served as complete,
  pending operations return an honest final result after reconnection.
- [ ] **QUAL-07 — Protection and installation-state checks.** Verify every media,
  playback, control and admin endpoint's auth policy, independent password
  domains, setup permissions, private secret storage, no log leakage and no
  accidental listeners. Keep authorized debug access policy explicit. Review
  dependency notices and compatibility, not just a passing web login.
- [ ] **QUAL-08 — Reproducible foundation release.** Source pins/patches/licenses,
  build recipe/environment versions, config/DT/module manifest, calibration,
  service/config schemas and migrations, acceptance results, known limitations,
  recovery artifacts and usage guide. Verify image/working-set fit and boot
  layout. No cloud dependency for core local operation. Keep original acquisitions
  and exact stock rollback images separate and immutable.
- [ ] **QUAL-09 — Persistent deployment decision.** Produce exact proposed image,
  affected partitions/ranges, backup verification, configuration/recording scope,
  interruption/recovery method and update policy. Perform persistent installation
  only under actual user authorization for that experiment. Reboot/power-loss
  qualify the installed image separately from RAM evidence. If authorization or
  a necessary persistent driver is missing, report it; do not imply “installed.”
- [ ] **QUAL-10 — Foundation port gate review.** Check all required subsystems,
  not just video. G8 can be a reproducible RAM-qualified software release if
  persistent install is not authorized, but first-setup/reset/durable settings
  need actual proof on their authorized storage/test configuration. Freeze the
  adapter conformance kit and shared service version, then start T34 only.

## Proposed quantitative campaign targets

Freeze or explicitly revise before execution:

| Area | Proposed target / interpretation |
|---|---|
| Video | Native720p QC07F at least15 delivered/decoded unique fps under accepted profile;30 only if supported; no silent scaling/software fallback |
| Soak | Initial1h, then24h full combined workload; extend only for failures/unresolved risks or changed implementation |
| Reliability | No unexplained decoder corruption, source-integrity violation, leaked ownership or spontaneous healthy-client disconnect |
| Lifecycle | At least100 start/stop/reopen cycles for final buffer path, including meaningful held/failed/cancelled cases |
| Live latency | Proposed p95 below1s on the qualified LAN/profile; report p99/max and all outliers, not average alone |
| PTZ STOP | Propose command/lease stop bounded to250ms or a tighter proven hardware limit; measure physical/electrical stop and agree final safe bound |
| A/V | Proposed sync within150ms with no unbounded drift |
| Memory | Measured stable high-water and explicit reserve, no OOM under admitted load; reject additional clients/features before unsafe exhaustion |
| Recording | Complete closed segments recover/play after faults; no deletion of originals/recovery; pending files/catalogue reconciled |
| Setup/reset | Recoverable wrong-network/scan/disconnect/reset/power-loss paths, actual AP and result retrieval, no lost footage |

These are release goals, not established platform capabilities. Do not quietly
weaken them to get a green report. A fixed2s send cutoff or successful15-minute
prototype test cannot by itself satisfy the combined workload gate.

## G8 handoff

Deliver the matrix, measurements, build/image manifest, recovery and a clean
current-state record. Begin [T34](17-DEFERRED-T34-4G.md) only after G8 review;
retain QC07F as the regression reference while later adapters evolve.
