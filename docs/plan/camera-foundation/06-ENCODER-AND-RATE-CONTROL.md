# 06 — Hardware codec, rate control, performance and shared-engine integration

Entry: [index](00-INDEX.md). Prefix: **ENC**. Depends: CORE contracts, CAP budget;
CAP import/overlap and NET throughput run as coupled G2 work.

## Baseline

QC07F ASIC82900760/config2b522500 has measured JPEG/H.264 capability. Hardware
IDR/P with two owned reference buffers, fixedQP26/GOP30 and CAVLC works at720p.
Output limit1MiB is distinct from its2MiB guarded allocation. No silent software
entropy fallback. Public Anyka trees contain useful GPL kernel glue and API/demo
contracts, but the audited encoder core was supplied as binary libraries.

Sources: `bringup/qc07f/hwenc/`, `bringup/qc07f/userland/h264-baseline.h`,
`rtspd.c`, `docs/ANYKA_PUBLIC_ENCODER_SOURCE_AUDIT.md`, retained REA contracts.

## Tasks

- [ ] **ENC-01 — Reuse audit at each missing function.** Check licensed public
  Anyka/Hantro-compatible source for the *measured* ASIC interface, not a family
  name or object filename. Retain pins/notices and compatibility evidence.
  Use REA for missing binary semantics: reference layouts, stream limits,
  QP/rate/VBV, control/status/error bits and actual call order. Do not transplant
  EV300 AL5 handling or execute an unreviewed SDK build script.
- [ ] **ENC-02 — Per-device/per-session ownership.** Remove singleton mutable
  state where it prevents common adapters; keep real engine arbitration, fd-owned
  references and profile limits. Define IDR/reset after owner loss, source
  generation change or error. Do not reuse P references across incompatible
  contexts. Serialize shared-engine access with bounded admission/fairness.
- [ ] **ENC-03 — Imported capture input.** Implement CAP's validated mapping,
  valid extent/guard/cache contract, then eliminate the redundant input copy only
  after hardware proof. Reject unsupported fragmented mappings rather than
  treating them as contiguous. Preserve output/reference ownership until DMA
  retirement on timeout, reset, close and failed completion.
- [ ] **ENC-04 — Standard codec interface.** Decide/document the appropriate
  V4L2 encoder/mem2mem integration using existing kernel patterns, explicit
  OUTPUT raw and CAPTURE encoded queues, controls and stream/drain semantics.
  Keep private prototype ABI as a rollback until standard behavior is qualified.
  No ownership shortcut around buffer lengths, strides or generation validation.
- [ ] **ENC-05 — Bitstream correctness.** Validate SPS/PPS/profile/level,
  dimension/crop, frame_num/POC/IDR wrap, VUI colour/range/timing, emulation
  prevention and NAL/whole-AU boundaries. Support parameter changes with a new
  stream generation/IDR/decoder configuration. Compare independent decoders,
  parse syntax and correlate source/output lengths and integrity.
- [ ] **ENC-06 — Qualified rate control.** Recover QP/rate/CPB/VBV semantics,
  units, bounds and feedback. Implement a measured fixed-QP range first, then
  supported CBR/VBR/target/max bitrate/GOP/FPS controls. Host control logic must
  drive hardware encoding, not become a silent software encoder. Validate target
  bitrate, burst size, IDR cost, quality and buffer limits on static/noisy/moving
  scenes and I-frame storms. Advertise only the modes actually implemented.
- [ ] **ENC-07 — Performance accounting.** Measure sensor cadence, capture wait,
  copy/cache/CRC, encode hardware time, scheduling, network and log cost separately.
  Use monotonic trace events and actual unique delivered/decoded frames. Reuse
  allocations and avoid unnecessary opens/resets/copies; do not disable guards
  or count dropped IRQ events as output FPS. Recheck the full memory budget after
  each optimization. Preserve before/after results and rejected-frame counts.
- [ ] **ENC-08 — Profile/concurrency policies.** Implement measured resolution,
  FPS/bitrate and encoder-group admission. One encoding can feed many consumers;
  main/sub encoding may consume additional silicon/reference resources. Prevent
  one client from changing the global stream unexpectedly. Software fixture two-
  sensor scheduling must expose contention honestly before later hardware ports.
- [ ] **ENC-09 — Fault recovery and acceptance.** Test output-full, invalid input,
  missing IRQ, timeout, reference corruption, cancelled capture, active close,
  generation change and reopen without leaks. A failed AU is rejected and forces
  recovery/IDR as appropriate; no stale frame or clipped NAL enters the hub.

## Gate

Actual native720p stream targets at least15fps with repeatable pacing and useful
quality;30fps only when measured support allows it. Rate/peak AU size must fit
the qualified network and packet/recorder budget. G2 needs stable ownership and
hardware entropy encoding under combined load; fixedQP-only~2.5fps is a fallback,
not this exit. If the target cannot meet the goal, produce a measured bottleneck/
feasibility report and explicit user scope decision instead of claiming success.
