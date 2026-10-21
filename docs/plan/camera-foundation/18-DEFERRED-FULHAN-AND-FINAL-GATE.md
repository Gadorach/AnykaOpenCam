# 18 — Deferred Fulhan FH8626V200 port and final all-model acceptance

Entry: [index](00-INDEX.md). Prefix: **FH**. **Last platform in the plan.**
Depends: shared foundationG8 and the default T34 sequenceG9. A user-directed
change between deferred platforms may reorder them, but never move either ahead
of the QC07F foundation. Final gate: **G10**.

Inventory ID `camera-03`, exterior label `CFMTA-635727-BJYVX`, user-reported
dual-camera Wi-Fi/PoE/PTZ system with FH8626V200 and RTL8188FTV. Read
`targets/camera-03.json`, original acquisition and authorized U-Boot access
records. Historical off-board flash status is not a current observation.

## Existing evidence

Three matching8-MiB original reads; stockLinux4.9.129, identified BSP/board image,
environment/partition analysis and earlier separately authorized boot-access
work. Full filesystem/runtime/sensor/PoE/Wi-Fi qualification is incomplete.
No replacement-kernel boot or both-sensor stream has been observed. Fulhan codec
and memory architecture must not be presumed equivalent to Anyka.

## Tasks after foundation and T34 gates

- [ ] **FH-01 — Confirm retail/current hardware.** Revalidate board/revision,
  flash reinstallation, UART map/voltage, CPU/RAM, boot policy/recovery, fitted
  sensors, network interfaces and PoE power path. Collect stock receive-only
  boot/runtime evidence where possible. Preserve original images and prior
  access-update provenance; do not overwrite an unknown current array.
- [ ] **FH-02 — Pinned source audit.** Obtain matching licensed Fulhan kernel/
  ISP/codec/sensor/network/audio/MMC source subsets or full tree as appropriate.
  Use REA for owned binary gaps. Record source/ABI/license evidence and decide
  Linux6.x platform port strategy. Old4.9 modules and matching symbol names alone
  do not establish compatibility. No generic Anyka register reuse.
- [ ] **FH-03 — Minimal RAM Linux and recovery.** Qualify CPU/MMU, interrupt/
  timer/clock/reset/pinctrl, UART/watchdog/PID1, actual memory/reserved ranges
  and checked image load/decompression. Keep stock SPI and footage intact.
  Build exact module/config manifests and obtain direct boot evidence before
  peripherals/media integration.
- [ ] **FH-04 — Ethernet/Wi-Fi/power.** Qualify measured MAC/PHY/network path and
  RTL8188FTV USB transport/firmware with an appropriate upstream driver where
  feasible. Test normalMTU, bidirectional integrity, DHCP/static, link recovery
  and setup AP. PoE is a measured board/power capability, not a software checkbox:
  verify actual delivery/stability and avoid unsafe dual-supply assumptions.
  Define interface preference/failover as an explicit SETUP policy if required.
- [ ] **FH-05 — Both fitted sensors.** Identify sensor types/interfaces and ISP
  topology; qualify resets/clocks/init/raw formats/calibration independently and
  both active. Standard capture/vb2/export/lifetime tests from CAP, with correct
  generation/IRQ/cache behavior for this platform. Main/sub profiles never stand
  in for a second fitted sensor.
- [ ] **FH-06 — Codec/shared-engine adapter.** Discover exact hardware compression
  capabilities, reference/output layouts, rate/VBV, import/retirement and memory
  limits. Implement common backend contracts and profile admission/scheduling;
  measure simultaneous two-sensor video and concurrent recording/browser/NVR.
  No silent software fallback, unqualified H.265 claim or hidden sensor disable.
- [ ] **FH-07 — Peripherals/audio/SD.** Route and mechanically qualify PTZ,
  lights, reset and any ambient sensor/microphone actually fitted. Port standard
  audio/block drivers and the platform settings location, preserving stock
  recordings. Use common services and qualification procedures, replacing only
  board-specific adapters/calibration/DT.
- [ ] **FH-08 — Full product integration.** Use the same protected UI/RTSP,
  recording/masks/retention/playback, network transactions/setup/reset, DNS and
  ONVIF/NVR services. Validate both camera IDs, shared heads/encoders, timestamp
  mapping and storage namespaces. Document actual simultaneous limits rather
  than showing an attractive but false two-camera capability page.
- [ ] **FH-09 — Platform acceptance/release.** Run the full QUAL campaign on
  the actual device, including both sensors, Ethernet/Wi-Fi transitions if
  supported, PoE stability, card faults and settings recovery. Produce source/
  calibration/build/evidence/recovery package. Any persistent install remains
  separately image/range/recovery-authorized.
- [ ] **FH-10 — Final cross-model regression.** Re-run shared-service conformance,
  security, media, recording/retention, setup/reset, discovery/NVR and resource
  tests on QC07F, T34 and Fulhan after all portability changes. A later adapter
  must not regress QC's40-MiB budget or conflate physical sensors/profiles.
- [ ] **FH-11 — All-required-modules completion review.** Requirements matrix
  contains working implementations and real evidence for every fitted/required
  subsystem on every target. Resolve blockers, publish local compatibility and
  deployment/recovery instructions, and leave unsupported optional features
  explicitly named. Only set the overall completion flag after G10 actually
  passes; builds, mocks and one good platform are insufficient.

## Final deliverables

One shared product codebase, three platform backends with explicit capability
graphs, reproducible pinned builds and dependency/license inventory, measured
calibration and resource limits, protected interfaces, actual multi-sensor/NVR/
recording/setup tests, preserved original footage/images and per-target recovery.
Document RAM-qualified versus persistently installed status separately for each
camera. No automatic publishing, third-party uploads or account changes are
part of this plan.
