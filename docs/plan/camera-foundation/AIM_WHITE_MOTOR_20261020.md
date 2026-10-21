# White light + motor aiming — Oct20 optical bench extension

## Later handoff — stopped and neutral, no automatic restart

User could not close the expired window and explicitly authorized killing it.
Exact owned GUI stopped with SIGTERM only. Native03:01UTC postcheck verified pair/
IR/whiteLOW/fault0, used motor0 coils-off/count28, motor1 cold, all timed light jobs
gone, ISP/WD idle/UARTerrors0. Old epoch retired. Local close/expiry defects were
corrected with71 guarded fixtures; no native repaired-window test or reopen.
See `docs/qualification-plans/stock-reference-20261013/optical-live01/GUI_CLOSE_FIX_20261020.md`.
This supersedes the active-window handoff below. No queued target work remains.

## Latest optical result — user-observed A OUT / B IN mapping

User now reports A lets IR illumination reach the subject in the camera image,
consistent with the IR-cut filter retracted; B suppresses that illumination while
the IR emitters visibly remain enabled/outputting, consistent with filter inserted.
Thus **GPIO6/A = OUT / IR-pass / night filter position** and
**GPIO7/B = IN / IR-block / day filter position** on this unit. This is functional
optical observation, not a measured spectral cutoff or direct position metrology.
IR LED GPIO23 remains separate; moving the filter does not switch emitters or
implement automatic exposure/gain/colour/day-night policy.

See `IRCUT_OPTICAL_MAPPING20A01.json`. Host-only receipt review at02:47:53UTC saw
six A/five B, ten IR_ON, seven Motor0+4 and one White_ON normal completions during
the v2 window, no global HOLD. Motor receipts establish28 issued raw steps, **not
physical direction/extent**; user motor feedback and final hardware postcheck remain
pending. No UART commands were sent during this review. The02:47:28UTC admission
has expired; do not renew or query hardware while the operator window remains open.
Future GUI captions retain A/B keys and add observed filter OUT/IN roles; no live
window reload or additional actuation is implied.

User operated the first live GUI and reports distinct clicks on both IR-Cut A/B,
but white illumination/aim obscured the scene. User requested white-light and
motor controls. Clicks are associated audible feedback, **not optical filter
position/day-night mapping proof**. Old GUI exited normally; its epoch was retired.

## Actual narrowly accepted increments

- First GUI: status, IRoff, IRon, A, B, A completed normally. Postcheck: pair/IR
  latch and pads0, fault0, self-held IR job gone, ISP/WD idle, UART errors/busy0.
- Independent GPL white module owns **only GPIO5** through normal gpiolib;
  old working GPIO6/7/23 module/helper/dispatcher unchanged. Inert load/status then
  one explicit GPIO5 output LOW passed. **User confirms white LEDs are now OFF.**
  Initial input/pad readback alone was not physical light-state proof.
- White ON is a manual nominal4900ms target timer plus no-fork self-held CLI;
  no active extension. Native timed-ON/physical deadline/coexistence remains a trial.
- Existing kernel75 PTZ driver has a hard **±4-step,20ms/step** primitive. New fixed
  C transport validates both DT/sysfs identities/states and performs exactly one
  write per user click; never chains eight writes to imitate32. Native read-only
  status passed: both axes cold UNTESTED/count0. **No motor motion yet.**
- Wrong-shell motor request in the new composite dispatcher refused before action.
  Native three-component status and white-OFF composition passed; typed desktop
  bridge roundtrip0.891s. Motor directions/positions/limits remain unqualified.

## Source/native evidence

White source: `bringup/qc07f/white-control-candidate/`; parent run06 reproduced
module12020B/195c05ad... and CLI53556B/d08cb896...;23 policy+81 actual CLI mocks,
76 receipts per optimized/sanitized mode. PTZ source:
`bringup/qc07f/ptz-console-candidate/`; parent run04 reproduced57972B/6ee85b38...;
154 cases/145 receipts per optimized/sanitized mode. Neither fixture result alone
establishes native actuation, waveform timing or mechanical limits.

Desktop now has v2 receipts and an additional immutable toolset pin, White OFF /
nominal4.9s ON, Motor0/pan raw−4/+4 and Motor1/tilt raw−4/+4. Cold UNTESTED is not
mislabelled off-proof. A move needs exact4-step delta, unchanged other-axis snapshot,
same phase modulo4 and positive coil-off/readback. UNKNOWN/late/partial completion
retains global HOLD; a healthy status cannot clear it. GPIO0/1/2 and homing excluded.

Source64 guarded cases PASS (`20261016-optical-viewer/run14`), plus16 real host-Bash
framing/one-effect/direct-parent cases with synthetic helpers/getters only
(`20261020-optical-dispatch/aim01`). Real native ash status/OFF composition passed;
first native motor and white timed-ON trials still require user observation.

Native receipts under `evidence/qualification/qc07f/stock-reference-20261013/optical-live01/`:
`USER_CLICK_POSTCHECK20A01.json`, `PTZ_NATIVE_STATUS20A01.json`,
`WHITE_NATIVE_OFF20A01.json`, `AIM_NATIVE_COMPOSITE20A01.json`,
`AIM_GUI_ADMISSION20A01.json`. Detailed UART/config/boot IDs and GUI diagnostics
remain private outside Git. No frame/voice recording or persistent firmware write.

## Current operator handoff — no parent UART commands

Updated FreeSimpleGUI optical aiming window launched with fresh private v2 epoch
expiring **02:47:28UTC**, fixed actual shell165/boot/kernel/module/helper/dispatcher
pins and sole20a protected recorder. Startup sends nothing. User Connects, waits for
video (startup may take5s), Reads controls status, then tries one4-step nudge and
checks direction before more. Stop on no movement/strain/unknown state; physical
power OFF/leave OFF if control or release is unconfirmed. No automatic movement,
rearm, profile renewal or retry. Preview defaults300s; max300, stale3s disables
effects. Profile10min and recovery04:12UTC are dated, never inherited after expiry.

**Next:** await user observation/window exit, then inspect actual receipts,
coil/LED retirement and optical image behavior. Parent alone owns postcheck;
do not send UART during operator use or claim physical movement from counts.
