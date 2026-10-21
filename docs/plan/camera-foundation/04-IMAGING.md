# 04 — Exposure, white balance, colour, day/night and orientation

Entry: [index](00-INDEX.md). Prefix: **IMG**. Depends: G0 and CORE control
contracts. First active imaging target is QC07F; other-board tuning is deferred.
Primary outcome: **G1**, then regression under the final capture/encoder path.

## Baseline and current blocker

SC1346 silicon IDda4d and exact139-pair retail initialization are tested. Native
Y/U/V and idle UV64/mono0 presets work. WB snapshot396/258/498 and retained
manual413/256/310 both generated60 clean, source-correlated H.264 frames. The
manual tuple lowers blue/purple but leaves excess red. White ceiling is clipped;
about99% of the selected grey-wall patch clips in at least one channel. Those
patches cannot support accurate gain estimation. TV reflections are not a
calibrated black reference. Current exposure/gain physical units are not proved.

Relevant code/evidence: `bringup/qc07f/mainline/.../isp.c`, `isp-whitebalance.h`,
`isp-stream.h`, `bringup/qc07f/userland/isp-whitebalance*.c`,
`docs/QC07F_WHITEBALANCE69_ACTUAL_RESULTS.md`, retained SC1346 config and REA
sensor/ISP procedures under `work/lightbulb-camera/driver-re-20261008T110053Z/`.

## Tasks in order

- [ ] **IMG-01 — Trace the full imaging state.** Correlate measured sensor
  register values, line/frame timing, exposure packing and analogue/digital gain
  with matching SDK source, sensor information and REA stock setter/caller paths.
  Identify ISP black-level, Bayer order, demosaic, RGB gains/offsets, gamma, colour
  matrix, YUV range/matrix, saturation and day/night linkage. Preserve observed,
  source-supported and hypothetical values separately. Resolve the missing
  interpolation/unity divisor rather than inventing it.
- [ ] **IMG-02 — Build a controlled unclipped baseline.** Record scene lighting,
  current exposure/gain/WB/table state and histogram/clip fractions on raw pixels.
  Ask for a matte neutral/colour reference only when the scene cannot supply one.
  Capture one owned frame, preserve host CRC/SHA and release target memory. Check
  raw YUV, independent JPEG/H.264 decoding and browser rendering for range,
  plane/order or colour-metadata mismatch. Keep gamma1 and no viewer tint filter.
- [ ] **IMG-03 — Bounded manual exposure/gain controls.** Implement supported
  ranges/steps/units from evidence with clamping, readback, frame-boundary apply,
  rollback and desired/applied reporting. Respect VTS/HTS and flicker/frame-rate
  interactions. Use sensor group-hold only if its semantics are established.
  First reduce clipping with measured exposure, then re-evaluate gain/noise.
  Failed transactions must not leave mixed multi-register settings unreported.
- [ ] **IMG-04 — General manual WB and statistics.** Extend the two comparison
  presets to validated gain/offset controls and qualified ISP statistics, with
  idle/frame-boundary ownership and rollback. Preserve unrelated table bits and
  valid ranges. Characterize dark offset versus gain and avoid applying decoded
  RGB ratios directly through unknown nonlinear processing. Provide read-only
  actual/desired state, timestamps and camera ID.
- [ ] **IMG-05 — Calibrate the colour pipeline.** Test neutral patches and known
  colours at several unsaturated exposures, daylight and artificial light. Verify
  Bayer order/phase before tuning CCM or gains. Audit retained day/night matrices
  and curves; use known presets as experiments, not named accuracy guarantees.
  Record neutral error, clipping, colour consistency, shadow/noise and temporal
  stability; retain reversible profiles keyed by sensor/board/revision.
- [ ] **IMG-06 — AE/AWB control loops.** Implement bounded statistic sampling,
  startup convergence, hold/manual/auto modes, invalid-stat rejection, saturated
  and dark-region exclusion, smoothing, hysteresis and safe limits. Control
  exposure/gain/WB through CORE's owner. Avoid per-frame expensive host-like RGB
  processing on ARM926; measure CPU/latency. Mode changes and lights/PTZ must not
  create unbounded oscillation, stale control writes or cross-camera state.
- [ ] **IMG-07 — Day/night coordination.** Use CTRL's verified white/IR controls
  and real ambient data if fitted, or explicitly named image-brightness estimates.
  Define auto/manual policy, separate illumination and colour mode, hysteresis,
  dwell time and override precedence. No mechanical IR-cut assumption. Qualify
  hardware absence explicitly; do not claim natural daylight colour with an
  optical limitation hidden by arbitrary tint correction.
- [ ] **IMG-08 — Product orientation.** Represent independent horizontal and
  vertical transforms per physical camera. The upright QC07F test uses both host
  preview flips. For product output, implement a qualified sensor/ISP/encoder
  transform or measured alternative so RTSP, browser, recordings and snapshots
  agree. If raw Bayer is flipped, account for phase change and recalibrate.
  Transform motion masks, UI coordinates and PTZ direction mapping consistently.
  Preserve a zero-transform rollback and the hanging-bulb installation profile.
- [ ] **IMG-09 — Quality regression.** Repeat raw/encoded comparisons on the
  standard queue/import path, across stream/profile restart, day/night changes,
  light toggling and temperature/long-session drift. Test manual controls during
  live multi-client operation through the broker, including EBUSY/error/cancel.

## Acceptance and outputs

Before tuning, freeze the reference scenes and quantitative tolerances: neutral
channel balance over *unclipped* regions, useful shadow detail, controlled highlight
clipping, convergence time, low-light noise and no hunting/flicker. A proposed
starting target is neutral-channel deviation within5% on a controlled reference;
document exposure/colour-space conditions and revise only with justified evidence.

G1 requires usable visible-light imagery without strong tint, exposed controls
with real units/ranges, explicit night-mode capabilities and consistent orientation.
Two successful preset writes alone do not close it. Deliver versioned calibration
data, source provenance, before/after raw and decoded evidence, test settings and
rollback instructions. If optics/lighting prevent acceptance, document the
measured limitation and the physical dependency instead of fabricating a fix.
