# 01 — Baseline, evidence, task selection and execution discipline

Entry point: [master index](00-INDEX.md). Task prefix: **BASE**. Gate: **G0**.

## Last observed baseline, not a current connection assertion

- QC07F: Linux6.18.55, pinned commit
  `725bd2f3c81d54edccb66fead01d4c0c222e2231`; RAM kernel69, station70, codec21,
  RTSPa22. Current source remains a platform bring-up port, not mainline-merged.
- Native720p H.264 constrained-baseline/CAVLC/QP26/GOP30, gated capture and
  approximately2.46fps. Custom copied queue works separately from encoding.
- Last900s preview:2216 decoded frames, zero reconnects/partial-AU rejections/
  decoder-corruption errors; bounded viewer stopped and capture idled.
- Earlier a20 long single-client run failed at frame397 with2s send cutoff.
  Three90-frame clients survived a deliberately stalled fourth peer in a finite
  a20 test. Neither success erases failures nor establishes normal frame rate.
- WB manual413/256/310 reduced purple versus396/258/498, but red bias/clipping
  remain. UVgain64 explicitly enabled. Both preview flips applied, gamma1;
  native RTSP orientation is not thereby corrected.
- WPA2/CCMP/DHCP worked at192.168.1.73 with MTU384. Those addresses, host endpoints
  and PIDs are historical; discover/validate before use.
- UART private246-byte burst preceded lost console replies; cause was not
  register-measured. User power-cycle recovered U-Boot. New private64-byte/25ms
  pacing works; Anyka receive-error acknowledgement is implemented but fault
  recovery itself was not qualified by normal traffic.
- All three cameras have three matching independent8-MiB original flash reads.
  Other two systems have no observed replacement-kernel dual-sensor qualification.

Authoritative evidence, relative to workspace:
`docs/QC07F_WHITEBALANCE69_ACTUAL_RESULTS.md`,
`docs/QC07F_UART_PRIVATE_BURST_RECOVERY.md`,
`docs/QC07F_SEND_CUTOFF_AND_WHITEBALANCE.md`,
`docs/CAMERA_PRODUCT_ACCEPTANCE.md`, and
`work/lightbulb-camera/implementation-20261008T205404Z/analysis/`.
The continuation file `CONTINUATION_WHITEBALANCE68_ACTIVE.md` contains newer69
sections above old history; do not interpret its filename as actual kernel68.
Some target/index fields lag evidence; reconcile before changing state.

## Tasks

- [ ] **BASE-01 — Receive-only current-state preflight.** Inventory intended
  adapters/serial owners, recorder socket and source revision, last RX, current
  network link and processes. Confirm no binary upload or other exclusive
  operation. An old recovery deadline from October10 is not live authorization
  or protection on October11. Start or rotate only the intended recorder with
  sole ownership and exact adapter selection; verify private pacing in the
  running owner, not merely the edited script.
- [ ] **BASE-02 — Preserve rollback baseline.** Verify complete kernel69 image,
  exact config/symvers, station70/codec21/a22, source snapshots and recovery
  loader hashes. Record actual memory map, volatile bootargs and recovery path.
  Retain previous known-good gated/capture/codec versions. Check SD recovery
  file by read/hash rather than recreating it without need.
- [ ] **BASE-03 — Normalize truthful state.** Reconcile `targets/INDEX.json`,
  target state and current continuation with direct evidence. Separate host
  viewer running, RTSP listener available, capture active, network associated,
  hardware-tested and product-accepted states. Keep unfinished flags false.
- [ ] **BASE-04 — Add the standing plan pointer in implementation mode.** Link
  the local project instructions/status index to this master plan; use task IDs
  in future handoffs. Do not modify upstream/vendored instructions to carry the
  project plan. Changes to this plan need dated rationale and dependency impact.
- [ ] **BASE-05 — Bound diagnostics.** Every hardware experiment names input
  hashes, exact commands/ranges, stop conditions, resource budget and rollback.
  Preserve raw observations with timestamps, command/tool versions and separate
  transformations. Use fresh output names; never overwrite mismatches. Keep at
  most one large diagnostic RAM file; preserve/hash it on host before reclaiming
  its owned target copy. Bound routine logs and viewer lifetimes.
- [ ] **BASE-06 — Private provisioning contract.** Unify paced public/private
  sending, keep privacy windows bounded, forbid credential echo/values in logs,
  require nonsecret completion plus actual association/address validation.
  Handle partial write/disconnect/rotation without replaying secret fragments.
  Clean private scratch according to explicit retention needs after verified
  restoration; never put credentials in source, firmware or routine manifests.
- [ ] **BASE-07 — Evidence-to-status tooling.** Add a small report collector that
  retains failures, checks expected counts/decoding/source integrity, associates
  measurements with source/target/generation, and refuses a passed status when
  evidence is absent or observation ended before the target command completed.
  Qualification runs must fail on decoder errors even if FFmpeg exits0.

## Operating constraints retained for every subsystem

Use the user's ownership/authorization for reversible local work and bounded
target experiments; avoid repetitive approval for already authorized work.
Do not infer a new persistent firmware-writing operation from a build recipe.
Before such an experiment identify image, affected ranges, boot policy and
recovery, and use actual session authorization. Never run upstream build scripts
without auditing them; never execute extracted target firmware on the host.

Preserve original dumps, photos, logs and stock footage. SDK values are source
facts, not measured board wiring. REA is the binary reverse-engineering tool;
record exact binaries/hashes/evidence IDs. Local-drive scratch is available when
temporary storage is full. No uploads/publication/third-party device access.
No flash erase/program/unlock/OTP/eFuse operations as part of acquisition.

## Gate / handoff

G0 closes when current ownership/recovery is known, baseline artifacts reproduce,
private transport is verified, logs/resources are bounded, and the next imaging
experiment can be executed without guessing current state. If a physical
measurement/power-cycle/reinstallation is required, state that concrete blocker
and continue independent offline tasks. Follow [04 imaging](04-IMAGING.md).
