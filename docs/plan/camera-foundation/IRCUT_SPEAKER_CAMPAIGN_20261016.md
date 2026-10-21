# User priority — IR-Cut, speaker, then camera framerate

## Latest user steering — optical live-feed test, not audible inference

Own U-Boot STATUS, neutral setup and inert calibration passed. One admitted GPIO6
nominal10ms pulse returned pair latch/pads00, but user heard nothing; filter
movement remains UNCONFIRMED. Original mux/direction/pulls/clocks restored afterward.
User requests RTSP decoding and separate IR-Cut/IR-LED buttons in Python to observe
the optical result. This narrowly reopens GUI work for the requested optical test;
no repeat/escalation or full FPS tuning. See
[OPTICAL_IRCUT_LIVE_20261016.md](OPTICAL_IRCUT_LIVE_20261016.md).
At06:06UTC sole private16b owner/held U-Boot was revalidated, not Linux/SSH/feed.
Offline viewer and exact-kernel75 controls are separate source lanes; parent alone
owns hardware. No new native operation from build/model success alone.

## Latest actual — speaker audible/refill PASS; IR-Cut inert probe stage

Own enable-low speaker8 reproduced9294opt+san and passed two224ms tones (second
explicitly requested; user confirmed same clear tone), then user-approved5s at
same1000Hz/gain1/25%.40060 frames/176128 submitted bytes/40 periods/39 refills,
user reports steady5s, normal gain/close/OUT-before-mux restoration and no owners.
Only this tuple qualifies; no louder sweep/all-range/product audio claim.
See speaker-route08/ACTUAL_RESULTS_20261016.md and public native receipts.

User then powered OFF/reconnected UART; fresh private16b owner3513114/start98765723,
source135a694b..., verified05:28 through08:28UTC; userON caught originalU-Boot.
No current Linux/SSH runtime, old RAM helpers not inherited. Own2176B IR-Cut probe
parent run05 reproduced147opt+san/codeSHAeecf1e6c..., fixedrange82001000..82001880
inside reviewed native GO page. STATUS-only staging active; no GPIO pulse yet.
Retail build lacks optionalbdinfo (failed attempt preserved before code execution);
admission uses actual64MiB banner, pinned originalBOOTCRC and prior nativeGO page.
No kernel reconstruction, firmware write or GPIO1 change in this IR-Cut phase.

## Physical handoff update

User disconnected BOTH UART TX/RX signal leads and confirmed physical isolation.
16a capture sealed/hashverified04:59UTC; no owner/socket/catch remains, so no U-Boot
reboot/probe while isolated. Reorder **within the first pair**: corrected speaker
trial via SSH now; IRcut U-Boot STATUS/calibration requires a later brief UART
reconnection. FPS still follows IRcut/speaker, not a competing research lane.

Speaker08 independent9294 opt/san/source build and RAM stage, fresh minimal runtime
and WD identity, and benign FD3-closed child/status/guard retirement passed05:01UTC.
User ready to listen; one1000Hz/rawgain1/25%/224ms enable-low trial active. Native
completion/physical audibility not yet known; no automatic repeat/escalation.
Existing60s watchdog/manualOFF recovery retained, no active cleanup on unknown DMA.

Latest user steering supersedes reset-policy/GUI detours: use retail behavior only
as evidence of the hardware mechanism; implement our own solution. Do not copy or
reconstruct the retail kernel as a goal. **Order: IR-Cut and speaker first, FPS next.**
GPIO0 input identification is complete; no further factory-reset investigation.

## IR-Cut first

- Existing reference evidence: board5 paired **GPIO6/7**, one direction asserted,
  nominal10ms then both low. MX6208 probable lens-coil H-bridge; day/night direction
  and optical movement still unproved. GPIO2 is excluded, not an IR-Cut candidate.
  User clarifies the hardware layer: GPIO6/7 are expected logic controls into the
  identified MX6208-class H-bridge, **not direct coil-power GPIOs**. The bridge's
  differential outputs drive the red/black two-wire lead that the user confirms
  goes directly into the lens assembly through an unlabelled connector.10/01 select
  opposite bridge polarities,00 is expected release/high-Z; exact AIN/BIN routing
  and direction-to-filter mapping remain to verify. No direct GPIO coil drive.
- Our own pulse core82 optimized/sanitized fixtures passes; disabled kernel adapter
  was not built/loaded. Do not divert into reconstructing retail module ABI.
- One bounded alternative source unit is building our **standalone RAM-only U-Boot
  probe**: no Linux scheduling/module-layout dependency, neutral/no-output default,
  deenergized calibration and finite instruction-only pulse interval with IRQ/FIQ
  masked, unconditional pair release before diagnostics. No source approval/native
  pulse yet. Parent reviews status/calibration first; no blind timer/mux experiment.
- Keep UART connected for this: GPIO6/7 do not share UART0 GPIO1/2. No speaker,
  motor, illumination or camera actuation concurrent with the single pulse. Treat
  pulse/latch release, actual electrical release, click/movement and optical filter
  identification as separate evidence. Optical check may accompany the later feed.

## Speaker second, concrete missing edge

- User confirms physical speaker connected to mainboard **SPK** connector; prior
  audible retail operation unknown (not original owner/no cloud access). No claim
  that speaker is missing; actual amplifier net/electrical polarity still unmeasured.
- Bounded reference audit found `ak_ao_enable_speaker(nonzero)` → ioctl40045081
  four-byte word1 → **actual retail GPIO1 LOW**. Earlier silent224ms tests kept high.
  No extra gain/pin sweep is justified before testing this missing enable edge.
- Separate own speaker-route08 source unit adds enable-low/readback after PREPARE
  before RESUME, preserves the low route, and restores saved high latch only after
  normal stop/close, then UART mux. Source/host only until parent review/build.
- First corrected-route trial planned **1000Hz/raw gain1/volume25%(peak8192)/224ms**,
  so the intended change is enable polarity, not duration/loudness escalation.
 5s/ring-wrap testing follows only if the audible route is physically supported.
- UART is **still connected**. No GPIO1 mux/output/playback until parent verifies
  SSH/recovery and user physically isolates BOTH UART TX/RX signal leads. Closing
  the port or unplugging USB alone is not physical isolation. Leave wiring unchanged
  until parent says the IR-Cut phase is ready for the SSH handoff.

## Camera framerate afterward

Investigate selected sensor timing versus actual captured/encoded cadence and the
reference hardware setup, with our minimal controlled feed/producer. No full stock
application/cloud/recording/homing startup; no unrelated retail policy archaeology.
Do not confuse stored/cached FPS with measured frame cadence or copy family defaults.

## Current parent facts / one next task

Persistent SSH-only ROOT is installed/cold-verified; original3.4.35/initPID1,
ROOT/USRRO/JFFS+SD unmounted, only original WD/USB/radio, no stock app. GPIO0 active-low
input confirmed with no settings change, remains INPUT. Private16a UART/nextboot
catch dated through05:55UTC, not current authority without revalidation.
Parent alone hardware. Current code/source units are IR-Cut standalone probe and
speaker-enable correction; GUI repair is retained offline, not a current deployment
task. No new Root/ENV/SD/OTP write. Next executable task: independently review the
IR-Cut inert probe, validate current pair/resource state, then coordinated U-Boot
status/calibration before admitting any pulse. Do not seal/disconnect UART yet.
