# 12 — Microphone, ALSA capture, stream synchronization and recording audio

Entry: [index](00-INDEX.md). Prefix: **AUDIO**. Depends: PLAT/CORE/STREAM and
SEC settings. Qualify QC07F's reported microphone; later boards enumerate only
their measured fitted hardware. No speaker/playback requirement is inferred.

**Dated user update2026-10-13:** user now reports a fitted speaker and microphone/
speaker ports. This supersedes uncertain speaker presence, not audio-route or
playback qualification. Mic capture nodecdev1/playbacknodecdev0 source-bound; no
PCM open/start/read/playback yet. ULN2803LVS/MX6208 proximity is not an audio net
proof; see stock reference PHYSICAL_PARTS_AND_AUDIO_20261013.md. Speaker work may
be planned from this user report, never guessed from actuator-driver markings.

## Tasks

Latestgaincomparison2026-10-14: useralsoheardnothingfrom05andrequestedcode3.
One06native224ms/digital8192trial applied/verifiedcache1→3→1andactualHPfield8→2→8,
restoringgain1beforecloseandverifyingafter. PAUSE/close/guard0/muxrestoration/postidle
PASS; **06physicalfeedbackpending**, no quality/route/productacceptance. No direct
HPwrite; exactstockSET/GETused. See speaker-native06/POST_IDLE_RESULT01.json.

Laterlevelcomparison: userheardnothingfrom04. One05tone atdigitalpeak8192vs768
(+20.5606dBrelative digital; analog1unchanged) completednativePAUSE/close/guard0/
muxrestoration, independentpostidle/SSHhealthy. **05physicalaudibilitypending**;
notacousticSPL/qualityorproductacceptance. See speaker-native05/POST_IDLE_RESULT01.json.

Latest2026-10-14speakerincrement: userUARTTX/RXphysicallydisconnected andpinnedSSH
controlqualified. Onefixedpublic224msnominal/peak768stereoDACtone04completednative
PAUSE/close/outerguard0; temporaryGPIO1SYS74mask2selectionrestoredexactly, gain1
unchanged, noGPIO2/IRcut/LEDdrive. IndependentpostnoPCM/watchdogowners/SSHhealthy.
**Audibilityawaitsuser**, notspeakerphysicalacceptance orAUDIO-07quality/AVgate.
See stockACTUAL_RESULTS andspeaker-native04/POST_IDLE_RESULT01.json.

Latest bounded evidence2026-10-13/14: controlled stock cdev1 scalar capture now
physically responds to user claps (late AC RMS2126–2646 versus fan84–87;27.6×mean),
with normal stop/close across eight runs. This supersedes the earlier no-open note
above, not AUDIO-02–07 integration/quality acceptance. No raw voice/hash retained;
gain7 clipped19/32000 clap samples. Original requested8000/cache8012 rate remains
source-calculated, not physically measured. Speaker cdev0 fixed public-tone helper
is parent-reviewed/host-reproduced only, not staged or heard. GPIO1 remux conflicts
with UART RX and is excluded; only the source-bound latch-only DAC path is proposed.
See stock reference ACTUAL_RESULTS.md and speaker-tone02/PARENT_REVIEW01.md.

Later2026-10-14 tone03 supersedes the staged-state sentence above: source-reviewed
post-close restoration of the known latent GPIO1 OUT bit fixes the input/latch
entry mismatch without remux. Parent reproduced151 optimized+151 sanitized tests;
fixed ELF RAM-staged/hashverified and GUI speaker button enabled for one user click.
No native playback or audible result yet. See speaker-tone03/PARENT_REVIEW01.md.

- [ ] **AUDIO-01 — Identify the actual capture path.** Trace microphone bias,
  analogue input, ADC/codec, clock/reset/mux, L2/DMA, interrupt and gain controls
  from compatible source, owned REA and physical evidence. Determine available
  rates/channels/sample widths and shared ISP/USB clocks; do not reuse stock
  module ABI or expose a guessed speaker device.
- [ ] **AUDIO-02 — ALSA/ASoC driver.** Implement capture-only standard controls,
  PCM negotiation, ring allocation, DMA start/stop, period IRQs, timestamps and
  overrun reporting. CPU/device cache and close/cancel/free rules follow CAP
  ownership principles. Bound capture memory; no speculative multi-megabyte
  audio queues on the40-MiB target.
- [ ] **AUDIO-03 — Gain/mute policy.** Real capture enable/mute, measured gain
  range, clipping detection, noise floor and optional processing only if useful
  and affordable. Expose these through common settings/permissions and actual
  readback. Muted means no unintended audio in RTSP/browser/recordings, not just
  silencing a UI player while capture continues undisclosed.
- [ ] **AUDIO-04 — Codec/container compatibility.** Choose measured low-cost
  encoding supported by required RTSP/NVR and browser recording paths. Qualify
  PCM/G.711 and browser-compatible codec options as needed; record licensing,
  CPU and mux limits. Avoid mandatory CPU-heavy transcode or assumed AAC/Opus
  hardware support. Keep unsupported tracks absent, not malformed silent stubs.
- [ ] **AUDIO-05 — Common timestamps and drift.** Audio samples and video frames
  share monotonic epochs; mux PTS/DTS and RTCP mapping correctly. Measure drift
  over long sessions, queue occupancy, underrun/overrun and recovery. Use bounded
  resampling/drift policy only with tested quality/CPU cost.
- [ ] **AUDIO-06 — Integrate STREAM/WEB/STORE/NVR.** Track discovery, enable/disable,
  SDP/fragment reconfiguration, per-client negotiation and synchronized playback.
  A disconnected audio consumer cannot stall video. Profile changes and source
  errors advance generations and produce clear discontinuities.
- [ ] **AUDIO-07 — Hardware acceptance.** Record real sound and silence with
  independent host decode; verify rate/channels, signal, clipping and timestamps.
  Test A/V sync, long-run drift, mute, high load, period loss, restart/close-held,
  SD recording/playback and actual NVR support. No loopback file is microphone
  qualification.

## Gate / outputs

G4 audio contribution is real microphone capture and truthful controls; full
recording/browser/NVR audio closes with their gates. Freeze intelligibility and
A/V sync tolerances before qualification; a proposed starting target is stable
sync within150ms without growing drift. Deliver driver/source provenance,
formats/rates, gain/mute behavior, memory/CPU measurements and recordings retained
privately as evidence.
