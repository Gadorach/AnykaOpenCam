# 13 — SD/MMC, recording, motion masks, retention and browser playback

Entry: [index](00-INDEX.md). Prefix: **STORE**. Depends: CORE, PLAT, CAP/ENC/
STREAM and SEC/WEB. Delivery after streaming and peripherals; primary gate **G5**.

## Baseline and scope

The SD contains footage and recovery/rollback files to preserve. U-Boot read of
QCLDR.BIN does not qualify a Linux MMC driver, writable filesystem or recorder.
Filesystem/FSINFO warnings remain an investigation item. New retention applies
only to closed recorder-owned segments, never arbitrary stock footage.

## Tasks

- [ ] **STORE-01 — Read-only device/filesystem inventory.** Identify controller,
  clocks/reset/pinmux, power and card-detect/write-protect wiring. Read partition/
  filesystem metadata and existing recording catalogue; preserve diagnostics
  before repairs. Never autoformat or automatically “fix” FSINFO/allocation
  warnings. Coordinate physical card access without writing recovery/footage.
- [ ] **STORE-02 — Standard Linux MMC/block path.** Port/use compatible SDHCI/MMC
  source and qualify initialization, negotiated bus width/clock, reads, bounds,
  timeouts, card removal and concurrency. Start with read-only data integrity.
  Add writes only into explicitly owned test files/directories after observing
  correct read path and recovery. Do not substitute eval-board mux/power values.
- [ ] **STORE-03 — Mount and ownership service.** Stable storage IDs/generations,
  present/read-only/full/error state, exclusive mount lifecycle and a controlled
  recording directory fd. Reinsert/reformat-by-user changes invalidate stale
  references. Define filesystem limits, supported large files, timestamp behavior
  and fsync/rename semantics. A missing SD is not permission to fill root tmpfs.
- [ ] **STORE-04 — Read existing recordings.** Identify containers/indexes/codecs
  and timestamps with static inspection and REA where proprietary. Import
  read-only into a separate catalogue; remux on demand only if supported and
  measured. If encrypted/unsupported, report the concrete format/key dependency
  rather than offering false playback. Preserve originals and identifiers.
- [ ] **STORE-05 — Segment writer.** Reuse encoded AUs from the common hub;
  choose a browser/NVR-compatible bounded container and IDR-aligned segments.
  Define temporary/open versus closed segment names, initialization metadata,
  timestamps/time validity, duration, checksums and atomic catalogue commit.
  Flush/close policy must balance power-loss recovery, wear and latency. No full
  video decode for recording. Bound pre-event history and write buffers.
- [ ] **STORE-06 — Power-loss/restart repair.** Recover/truncate only the new
  recorder's incomplete files using proved container semantics; never present
  partial media as complete. Journal/catalogue reconciliation, duplicate IDs,
  torn metadata, bad sectors, card removal and read-only remount have explicit
  outcomes. Last-good closed segments remain playable.
- [ ] **STORE-07 — Continuous/event recording scheduler.** Admin selects constant
  or motion-event mode and qualified profiles/audio. Start/stop only the logical
  recording consumer; share encoding with RTSP/browser. Define pre/post-event
  coverage, segment handoff, storage fault/retry/backoff and explicit gap events.
  Storage overload must not corrupt/stall live capture indefinitely.
- [ ] **STORE-08 — Motion engine.** Consume a measured low-cost luma/statistics
  path with correct source leases. Implement sensitivity, noise/exposure-change
  rejection, debounce, minimum event duration and configured ignored polygons/
  areas. Define coordinate space relative to orientation/crop/profile, polygon
  validation/rasterization and changes while running. Compensate or inhibit
  motion from PTZ/day-night transitions under documented policy, not hidden drops.
- [ ] **STORE-09 — Retention engine.** Operate on inventory-confirmed closed
  recorder-owned segments under a directory capability, never follow unsafe
  symlinks or traverse paths. Exclude open/writing/playback-leased files. Implement
  oldest-first pruning with reserve/hysteresis, plus active date-window pruning
  with explicit timezone/UTC boundary semantics. Unknown/untrusted recording
  timestamps must not trigger mass deletion; use a documented conservative
  policy and notify admin. Clock jumps do not reorder chronology silently.
- [ ] **STORE-10 — Full-card behavior.** If eligible files cannot be pruned, stop
  or drop recording with a visible error and keep camera live. Test bounded
  reserve for finishing the open segment and metadata updates. Never expand
  deletion scope to stock/imported recordings or firmware/recovery to find space.
  Adoption of imported footage into retention requires explicit user selection
  and inventory preview; default is preservation.
- [ ] **STORE-11 — Playback service.** Authenticated catalogue/list/day timeline,
  recording ID selection, seek/range requests, content type/cache policy and
  browser-playable containers. Hold leases against retention while playing;
  release them on close/timeout. Validate ranges and missing files. Imported and
  newly recorded files show origin/time-validity and unsupported status honestly.
- [ ] **STORE-12 — Hardware/fault matrix.** Long recording with RTSP/UI/audio,
  constant/event modes, overlapping events/masks, low-space/date retention,
  playback while pruning, sudden power loss, full card, unplug/reinsert, corrupt
  metadata, timestamp rollback and permission/symlink attacks. Verify untouched
  stock footage/recovery file hashes and recoverable new segments afterward.

## Gate

G5 requires both recording modes, real-frame motion masks, both retention modes,
existing-footage preservation and authorized browser playback on actual SD.
Document supported imported formats separately from new recording format.
Measure capacity/write latency/wear impact and RAM/CPU under the intended
combined workload; passing a host filesystem fixture alone is insufficient.
