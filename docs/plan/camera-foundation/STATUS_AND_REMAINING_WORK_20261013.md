# Status and remaining work — 2026-10-13

User-requested report. **The shared product remains incomplete; all product gates
remain open.** QC07F is first; T34-4G and Fulhan ports remain deferred through G8.
This report performs no hardware operation and does not request or declare a pause.

## Current bench — dated observation, not inherited authority

Last direct check19:05UTC: unchanged stockLinux3.4.35 root rescue, normal app/cloud/
recording startup bypassed; only watchdog/sensor/camera providers loaded. CameraIRQ0,
no userspace video/ISP/ION/UIO/watchdog owners, output latch0x2. No capture client
has been staged or executed; no current camera-feed/IR/FPS result exists.

Recorder13c expired normally and finalized/hash-verified. Freshsole13d receiver
and next-boot catch were established, bounded21:04:42UTC. This is not permanent
protection: revalidate owner, runtime and recovery before any later operation.
No new flash/SD-data/saveenv writes, normal-app startup or power cycle occurred.

## Stock-reference results established

- Original Wi-Fi runs MTU1500 and completes2048-byte firmware downloads; finite
  DF/TCP tests passed. ReplacementMTU384 remains a workaround, not a factory limit.
- White-light GPIO5 response was physically confirmed by the user and restored.
  GPIO2 is excluded after its exact UART mux conflict and preserved failure.
  IR emission is still unverified; GPIO23 latch changes alone do not prove it.
- Stored video settings request15fps; sensor cached30 is not delivered FPS.
  Stock native delivered/encoded/client cadence has not been measured.
- Full stock sensor initialization is140 pairs/560 bytes, final0x0100=1. Original
  139 subsets/sealed replacement preparation stay unchanged.
- An allocation-only8MiB ION test passed with2048 contiguousPFNs and known cleanup.
  No pixels were accessed. Retail uncached mapper/USERPTR correspondence is closed.
- Exact3DNR reference contract is resolved: separate0x384000-byte/900-page owner,
  three address/length pairs, fixed1280x960 reference capacity. This is not output
  geometry and the second allocation has not been hardware-tested.
- Original stock card geometry/read-only metadata collected; invalid zeroFSINFO
  preserved without repair or footage access. Motor GETs proved software idle only.

## Fixed capture candidate — source/host, not native

Real syscall adapter is linked but native entry unconditionally refuses launch.
Parent independently reran build11 as build12: **205 core +40 adapter cases per
optimized/sanitized mode**, plus dark0→textured-high→dark0 oracles. ARM bytes are
identical:82140 bytes, SHA256
`a39ad260e5bf88bbd91d0c0b29e336e963ce810de9e936d2dbde710fbffbb41b`.

The previous stale-frame phase attribution defect was independently reproduced,
corrected and retested. Phase latch readbacks, source-clock admission, full CRC/
guards, separate backing owners and retained-until-reset handling remain explicit.
Completed flat/zero observations are preserved as unconfirmed images, not IR
absence or sensor acceptance. Untouched prefill still rejects.

**Remaining code gate:** pointwise native frame/phase clock witnesses. Exact
timestamp conversion is proved; entry-only calibration cannot establish a session
error bound. The current adapter does not implement that filter. Parent review
also identified the observed trailing-NUL IR sysfs representation and requires
console/stdout device-identity reconciliation before native integration acceptance.
These are not cleared by the passing mock tests.

## Next work, in order

1. Correct the remaining retail-adapter ABI mismatches; implement bounded per-frame
   and per-phase clock witnesses, with old/ambiguous completions explicitly drained.
2. Parent code/binary review and independent tests of the complete fixed client.
   Preserve failures/seals; no guessed offsets, relaxed integrity or new services.
3. Admit an exact fresh target instance and a full-reset/power boundary while all
   owners remain retained. Stock watchdog expiry is not yet a proved full-DMA reset.
4. Run one announced finite GPIO23 level0→1→0 image comparison on the same scene,
   preserving exposure/gain/timing/flat-image evidence and qualitative confounds.
   Measure unique completed capture cadence separately from stored/cached labels.
5. Close authorized stock motor/audio/peripheral and encoded-FPS gaps through
   reviewed short tests with physical feedback; no GPIO2/homing/blind pin sweep.
6. Restore/seal replacement75 and continue the native product slice below.

## Remaining shared-product work

- **Protected live media:** native producer/handoff, authenticated browser viewing,
  independent RTSP credentials/positive native Digest, finite lifecycle and actual
  Frigate/Blue Iris interoperability. Host protocol/MSE fixtures are not deployment.
- **Memory/performance:** replacement media-v6's dated conservative forecast still
  fails5476KiB available versus11548KiB required; prove safe placement or redesign.
  Checked native encoded runs were roughly2.1–2.5fps, not720p15. Retain integrity
  checks and both axis corrections; no fit-by-cap-increase or tint compensation.
- **Durable security/setup/networking:** cold entropy/trust, durable keys/settings,
  isolated verifier integration, setup AP/reset/admin and recovery. Earlier native
  HTTPS full600k/lifecycle passes are narrow, not combined-product acceptance.
- **Physical features:** measured motor axes/travel/limits/STOP/current, microphone
  capture, illumination/IR-cut/ambient/reset behavior where actually fitted.
- **Storage/product behavior:** replacement MMC, real recording/motion/masks,
  retention/catalogue/playback, combined-load endurance and complete G3/G8 gates.

Original acquisitions/recovery artifacts/failures remain preserved. Work remains
local, uncommitted, unpublished and reversible. Detailed campaign evidence:
`docs/qualification-plans/stock-reference-20261013/ACTUAL_RESULTS.md`.
