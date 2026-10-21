# Live optical IR-Cut test — 2026-10-16

Latest user steering: no audible response to the single GPIO6 pulse; use a live
camera feed, add RTSP decoding and independent IR-Cut/IR-LED buttons to the Python
tool. This is now requested GUI work, superseding the earlier GUI deferral only
for this optical test. Full frame-rate tuning still follows it.

## Actual hardware checkpoint

- Own RAM U-Boot probe STATUS, explicit neutral setup and separate inert
  calibration passed. One user-admitted GPIO6 pulse used a nominal10ms finite
  call-free interval, then read back OUT/IN pair00. This proves the primitive's
  latch/release behavior, **not filter movement, coil current or measured width**.
- User heard nothing. Do not infer failure/success or escalate duration, pulse
  count, polarity, gain or pins from that silence. No GPIO7 pulse has occurred.
- Explicit restore passed: original full mux/direction/latch/pulls/clocks restored,
  pair pads00, GPIO0 remains input. No retained bridge output or Linux feed.
- At06:06UTC fresh read-only owner validation found the original held U-Boot
  prompt, protected16b capture with about8515s remaining. This is dated evidence,
  not authority for a later boot. UART is connected; speaker effects prohibited.
- Evidence: `ircut-native01/SINGLE_GPIO6_NATIVE01.json` and
  `ircut-native01/OPTICAL_HANDOFF01.json` beneath
  `evidence/qualification/qc07f/stock-reference-20261013/`.

## Selected shortest implementation path

Use our RAM-only kernel75 + our capture/hardware-H.264/RTSP producer, not the
stock metadata or finite-thumbnail adapter. The current stock adapter has a
specific phase-witness admission HOLD and is not an RTSP implementation. No full
stock app, cloud, recording or homing is allowed.

The historical hardware RTSP path decoded real720p frames, but the exact current
kernel75 receipt-aware continuous encoded RTSP renewal remains unqualified.
Embedded initramfs media/radio components are stale: explicitly pin/stage reviewed
RAM replacements. One controlled RAM boot, not a reboot for each UI button.
Persistent SSH-only ROOT and every flash partition remain untouched.

## Isolated offline increments

1. **Desktop:** dedicated optical entry beside the existing FreeSimpleGUI tool,
   FFmpeg/TCP decoding, bounded latest-frame display, explicit endpoint/connect,
   stale/end/failure indication; no recording or startup effects. Existing audio/
   motor GUI and its source epoch remain unchanged. A/B labels until optical proof.
2. **Target:** small exact-kernel75 GPL endpoint, inert load/status, exclusive
   GPIO6/7 ownership, explicit neutral setup, fixed10ms A/B pulses with kernel-owned
   release, sticky release fault, cooldown and separate GPIO23 IR-LED timer<=5s.
   Preserve other GPIO bits through existing gpiolib driver locking. No arbitrary
   MMIO writes from a live desktop process. Wrong mux/ownership is refusal, not
   automatic pin stealing. Module/source/build review precedes native effects.
3. **Parent integration:** bind desktop requests to the fresh boot/generation,
   reviewed helper identity, finite recovery admission and serialized operation
   lease. Unknown completion blocks retry; no UART/SSH fallback, auto-recovery or
   actuation on connection. Video decoding alone does not admit GPIO controls.

## Native admission sequence — not yet executed

1. Revalidate sole private UART owner, actual runtime, remaining recovery, exact
   RAM image/config/module/startup identities, memory ranges and source readiness.
   The IR-Cut GO code has already executed at0x82001000 with I-cache enabled.
   Do not replace it with the different QCL loader and assume cache coherence.
   Before using the existing same-page loader, coordinate a fresh user OFF/ON
   under a freshly verified catch (or independently review an explicit cache
   maintenance path). No automatic reboot or unreviewed relocation workaround.
2. Controlled RAM boot only after review. Check PID1/minimal startup, GPIO0 input,
   UART untouched, idle capture and no collateral media/peripheral owner. Check
   pair/IR mux, latch/pad and timer state before endpoint setup; no pin sweep.
3. Establish one finite decoded RTSP baseline with no output effects, plus clean
   receipt retirement/renewal and idle ownership. Do not call a listener live video.
4. Load/status/setup endpoint separately; verify neutral release, client-loss and
   timer behavior before admitting individual UI effects under media load.
5. User clicks one A or B at a time while viewing. IR illumination is a separate
   bounded action. Compare the same static scene under controlled IR lighting;
   account for exposure, gain, white balance and ISP day/night processing before
   assigning day/night labels. Readbacks are not optical evidence.

No microphone/raw voice, snapshots, file recordings, firmware/ENV/SD/OTP writes,
speaker/UART multiplexing, reset policy or full frame-rate campaign in this unit.

**Next executable task:** parent review/reproduce the two offline candidates;
then select one finite native feed-only baseline if fresh target admission passes.

## Parent offline checkpoint06:20UTC

Current own RTSP daemon rebuilt359520B;76 sanitized receipt-adapter cases PASS,
artifact/source manifests in `evidence/delegated/20261016-optical-media-parent/run01`.
No native execution. Desktop optical candidate independently passed39 guarded
mock tests (`20261016-optical-viewer/run02`), including IR-Cut while independent
timed IR illumination is ON. Default control transport remains unavailable.

Actual FFmpeg/TCP against our **localhost synthetic** RTSP fixture passed after
preserved build/timebase failures:25 decoded preview frames,5s expiry/readiness
revocation, own decoder/server reaped. `20261016-optical-viewer/loopback05/RESULT.json`.
Removed manufactured FPS duplicates and fixed a PNG encoder timebase mismatch;
no ignored decoder errors or native camera/video qualification from this result.

## Parent controls review / transport integration

Exact75 module independently reproduced at
`evidence/delegated/20261016-optical-controls/parent-build01/`: shared-policy model,
eight mocked local-Python transport cases and exact external-module build pass.
Module15444B/SHA b839958867cfbf89877a17cfc72a448db12b3703f13bf3528faaa720a6a3a61e,
matching the delegated final artifact. Source uses normal GPIO reservations and
the existing chip's per-bit lock, not live whole-bank MMIO stores. Load/status is
inert; setup explicit. IRQ-disabled nominal10ms A/B pulse and target4900ms IR timer
are source candidates, not native media-load/timing/physical proof.

Practical integration gap found: the RAM image has no Python. The candidate
`optical.py` performs local ioctls only; copying it to the desktop does not control
the camera. A bounded static ARM C helper is now a separate source unit, not a new
TCP service. Its short-lived IR fd holder must preserve the kernel-owned timer
through one-shot command exit without extending/retrying illumination.

Parent `peripheral-gui/optical_uart.py` prepares the explicit existing-recorder
transport with generation/boot/helper/module pins, private epoch, shared lock and
durable unknown-completion HOLD. It remains **pre-TX disabled** until the C helper
envelope is reviewed; no profile/GUI control admission exists.44 guarded fixtures
pass in `20261016-optical-viewer/run04`, including the transport mocks. An initial
unguarded development run failed writing test temporary files on full system
`/tmp`; tests now use the evidence run's owned TMPDIR. No cleanup of unrelated files.

Remaining next step: review/reproduce native C helper, complete strict envelope
translation, then coordinate one fresh cold catch for the already-reviewed RAM
kernel75/feed-only baseline. No further output/pulse or image boot has occurred.
