# PTZ OFF-retain author handback — awaiting independent review

Delivered one exclusive fresh driver derivative:
`bringup/qc07f/ptz-off-retain-candidate/ptz.c`.
SHA256 `a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`.
This is authored SOURCE plus ONE actual ARM-object compile PASS, NOT ADOPTED,
NOT RUN, NOT hardware-ready. No independent reviewer was spawned by this author.

Concrete change: failed/unknown final LOW now retains the original requested
gpio_descs array, a real consumer-device reference and self-module reference in
one persistent static owner. Its sticky global HOLD refuses either axis before
GPIO acquisition/history changes, including after consumer unbind/rebind/probe.
Attempted direction calls are tracked before invocation, separately from successful
initialization. Final cleanup touches only attempted lines and requires setter0,
actual intended-provider output direction and logical LOW for every attempted
line before release. Partial initialization never claims full coils_OFF. Original
first error, completed issued count and phase survive; normal verified return
still releases descriptors and both added refs. No HOLD reset/retry/release API.

Preserved: accepted counter guard, board5/four-pin metadata, phase table, +/-4
steps/20ms and break-before-make, existing state-field meanings, unknown physical
position, no probe GPIO request/write. Added status is scalar only. No extra
allocation payload, timer, PID, watcher, supervisor, provider or backend framework.

Review inputs:

- CONTRACT.md — written before source.
- SOURCE_REASONING.md — actual retained kernel/device/module/sysfs contracts,
  derivative flow, lifetime limits and conditional direct-provider direction proof.
- SOURCE_DELTA.patch / INVERSE.patch / SOURCE_INPUTS.json — exact change,
  literal inverse to baseline,421 original/candidate/reference/tool pins.
- ADDITIONAL_INPUTS.json — later-read fixed lookup/plan reference pins.
- BUILD_RESULT.md and runs/compile01/REPORT.json + logs — actual3 commands,
  ONE C compile,394 dependency pins; empty warning log, ARMv5TE/EABI5 REL only.
- DELIVERY.json — closed file membership/pins, excluding its own recursive hash.

There were no compile failures, second attempts, source corrections after the
single compile, candidate calls or tests. No original source, image, header,
sysroot, plan, AGENTS, Git/index, private evidence or device resource was changed.
Only the two assigned fresh directories contain this author's outputs.

Remaining: real product STOP/queue semantics, finite wait/hang behavior, native
broker/provider lifecycle, observed movement/sign/limits/zeroing and independent
recovery. Provider module refs do NOT prevent explicit provider unbind, and a
consumer device ref does NOT pin devm state. The derivative avoids relying on
either claim but cannot guarantee continued operational/electrical backing.
Retention may leave coils energized indefinitely: no current/thermal/physical
deenergization, STOP or deadline guarantee. Homing remains NO-GO; no origin reset
or mechanical acceptance. Historical23h OFF gives no fresh runtime authority;
RAM75 recovery/C1 and Wi-Fi P1 review restrictions remain unchanged. Speaker
testing is deferred. Nothing is queued for target operation or another source lane.

Only next selected action: independent ordinary source/build-evidence review by
the parent, then explicit parent disposition before any use. Do not link both
baseline and derivative or treat this loose object as an installable module.
