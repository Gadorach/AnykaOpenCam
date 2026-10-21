# 17 — Deferred T34-4G / Anyka KM01 platform port

Entry: [index](00-INDEX.md). Prefix: **T34**. **Do not start hardware porting
until the QC07F shared foundation passes G8.** Earlier independent source records
may be retained, but no diversion from foundation work is scheduled here.
Inventory ID `km01-lte`, exterior labelT34-4G. Gate: **G9**.

## Existing evidence

Three matching8-MiB reads and static characterization. Stock identifies as
AliOS Things/AOS-R-3.4.3, not the QC07F Linux platform. Two fitted cameras/PTZ
reported; no replacement Linux boot or dual-source streaming observed. DT/strings
support USB-host and a possible LTE/RNDIS route, not measured accessible USB
pads, power levels or a plug-and-play Wi-Fi conversion. See
`docs/T34_4G_USB_WIFI.md`, `PROJECT_STATE.json`, `targets/INDEX.json` and its
acquisition/REA records. Never import QC07F sensor/clock/GPIO/MMIO defaults.

## Tasks after G8

- [ ] **T34-01 — Fresh hardware/current-state inventory.** Confirm board/revision,
  fitted sensors, RAM/flash/boot policy, serial wiring/level, stock runtime if
  obtainable, modem/power topology and physical recovery availability. Preserve
  all existing photos/identifiers/dumps. Read-only capture first; coordinate
  reinstallation or inaccessible measurements explicitly.
- [ ] **T34-02 — Source and boot feasibility.** Audit matching KM01 BSP/SDK/license
  releases, CPU/MMU/interrupt/clock/reset/timer/memory layout and boot image format.
  Use REA for gaps in the owned binary. Establish an exact pinned Linux6.x build
  baseline compatible with the common software; do not pretend AliOS objects or
  QC07F modules are loadable Linux drivers. Kernel baseline changes require
  controlled common/QC07F regression, not a forked service implementation.
- [ ] **T34-03 — RAM-only minimum boot/recovery.** Verify actual load/decompression/
  reserved-memory ranges and checked transfer, bootUART/timer/watchdog/PID1,
  measured RAM and recovery. No persistent writes to “see whether it boots.”
  Publish board-specific DT/resource facts and cross-build provenance.
- [ ] **T34-04 — Networking and deferred LTE/Wi-Fi side task.** Establish actual
  modem USB/RNDIS enumeration/control and local connectivity first. For the
  proposed USB-Wi-Fi conversion, measure powered-off D+/D− continuity, ground,
  VBUS/rail/current and shared reset/enable lines; modem supply is not assumed5V.
  Select an upstream-supported adapter/firmware compatible with measured host
  capability, preserve recovery and the known-good modem, then test reversible
  connection. No GPIO77 or board-label guess. Decide primary product network,
  setup AP feasibility and whether both modes are required; a hardware change
  needs an explicit physical plan, not automatic modem removal.
- [ ] **T34-05 — Both sensors and ISP.** Identify each sensor/bus/reset/MCLK,
  raw format, ISP inputs/mux/concurrency and calibration. Probe/init/capture each
  independently, then simultaneously where required. Count two physical cameras,
  not main/sub from one. Use common profile/control semantics and IMG calibration
  process, standard capture/ownership and per-sensor evidence.
- [ ] **T34-06 — Codec and memory.** Identify actual codec IP/capabilities, owned
  allocations and DMA aperture. Reuse compatible code only with matching evidence;
  do not assume QC ASIC82900760 or H.265 merely from product family. Implement the
  common encoder backend, source import/lifetimes/rate control and shared-engine
  schedule. Measure simultaneous two-sensor throughput and peak memory; never
  silently disable one camera or duplicate unfit reference pools.
- [ ] **T34-07 — Fitted peripherals/audio/storage.** Board-specific PTZ mechanics,
  independent lights, real ambient/reset/microphone hardware if present, and
  standard SD/MMC. Run the same CTRL/AUDIO/STORE tests and preserve stock footage.
  Capabilities distinguish absent hardware from unimplemented required features.
- [ ] **T34-08 — Adapter-only service integration.** Register full graph and
  resource groups; use unchanged shared UI/auth/streaming/config/recording/setup/
  discovery/ONVIF services. Integrate cellular status only through typed extensions
  if needed. Validate dual-camera UI/masks/recording/NVR stream IDs, shared-head
  commands and resource admission. No hardcoded second service stack.
- [ ] **T34-09 — Full acceptance and recovery.** Apply QUAL combined/fault/security/
  setup/reset campaigns with both actual sensors active, measured power/network
  conditions and truthful modality limits. Build and optionally install only with
  exact image/range/recovery authorization. Re-run QC07F shared-service regression
  for every portability change. Keep G9 false if either fitted sensor or required
  setup/network path is missing.

## Exit

G9 delivers a documented T34 backend/DT/calibration and reproducible release on
the common software, two real sensor streams and qualified required features.
Record USB-Wi-Fi conversion as confirmed, blocked by measurement/hardware, or
explicitly not selected—not as a guessed capability. Then proceed to Fulhan.
