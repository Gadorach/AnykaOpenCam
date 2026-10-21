# 11 — PTZ, independent illumination, ambient sensing and reset input

Entry: [index](00-INDEX.md). Prefix: **CTRL**. Depends: BASE/PLAT/CORE;
product delivery after G3. Limited verified light work may unblock IMG day/night.

## Baseline

QC07F has user-reported PTZ, separate visible-white/IR lights, microphone and
reset button. Ambient sensor presence is uncertain. Electrical motor phase and
coil-off readback tests exist; physical displacement, axes/limits and absolute
position do not. Do not infer speaker, IR-cut filter, zoom or motor feedback.
Historical motor/GPIO candidates are evidence, not permission to reuse across
boards. Prior moving-scene comparisons did not prove displacement.

## Tasks

- [ ] **CTRL-01 — Retail routing inventory.** Correlate schematics/photos,
  continuity/voltage where needed, retained stock source/REA and read-only
  runtime state. Identify GPIO/expander/PWM/I2C/ADC/input ownership, polarity,
  electrical levels and shared pin/clock conflicts. Record uncertainty and
  required physical measurement; never replace it with an evaluation-board map.
- [ ] **CTRL-02 — PTZ low-level owner.** One engine per physical head with
  serialized axes/coils, bounded motion duration, phase sequence, acceleration
  if supported, timing, STOP and neutral output. Independent heads remain
  independent; shared heads appear once in the graph. Quiesce on process loss,
  command lease expiry, driver unload and invalid command. Avoid holding coils
  indefinitely unless thermal/current behavior is measured and required.
- [ ] **CTRL-03 — Mechanical qualification.** Use small reversible moves and a
  static reference to prove displacement/sign, axes, backlash and step mapping.
  Determine hard/soft limits and homing mechanism if present without blind
  long-travel homing. Measure safe duty/temperature and repeated stop. Do not
  advertise absolute coordinates/presets from guessed step counts; an open-loop
  estimate must be labelled and invalidated after power loss/slip.
- [ ] **CTRL-04 — Product PTZ contract.** Relative/continuous velocity and STOP
  with command lease, arbitration between UI/ONVIF, bounded priority stop,
  cancellation and device-generation handling. Support absolute/preset only
  after a defensible calibrated position model is qualified. Map installation
  orientation independently of raw image transforms; test intuitive pan/tilt
  direction for both hanging and upright profiles.
- [ ] **CTRL-05 — White-light control.** Verify output/driver/polarity and current
  or PWM behavior. Implement safe startup/off, validated intensity if available,
  bounded manual/auto policy and applied-state feedback. “On” means commanded
  versus electrically/optically observed as separately documented states.
- [ ] **CTRL-06 — IR-light control.** Same qualification for the separate IR
  emitter. Prove independent white/IR combinations without assuming shared bits
  or mutual exclusion. Define current/thermal limits and day/night interaction.
  Leave any unconfirmed IR-cut mechanism unadvertised and unoperated.
- [ ] **CTRL-07 — Actual light sensing.** Determine if a physical ADC/I2C/GPIO
  ambient sensor is fitted. Implement IIO/hwmon/input or appropriate small
  backend interface with measured scale, timestamps and validity. Report raw
  units until calibrated. If stock uses ISP statistics instead, label them image
  brightness, not lux or a fitted sensor; resolve the user requirement explicitly.
- [ ] **CTRL-08 — Reset button input.** Verify input/polarity, debounce, startup
  held button, short press, long press and stuck-input behavior through input
  events. Initially log events in disposable test state. SETUP implements the
  settings-reset action; button code must not erase firmware/SD directly.
  - [x] Oct16 bounded physical input/polarity increment: GPIO0 active-low, three
    matched taps, no reboot/settings/config change, input-only GET/MMIO observer.
    See `docs/qualification-plans/stock-reference-20261013/reset-button01/ACTUAL_RESULT01.md`.
    Full CTRL-08 remains open: debounce/startup-held/long/stuck and safe policy.
- [ ] **CTRL-09 — Peripheral policy/broker.** Shared owner coordinates auto/manual
  lighting and imaging, reports desired/applied/unavailable state, handles user
  override and returns to documented safe state on faults. Integrate physical
  reset, UI and ONVIF through one command path so concurrent clients cannot
  race motor/light hardware.
- [ ] **CTRL-10 — Regression matrix.** Actual displacement/stop for each axis,
  lease expiry, client loss, auth refusal, reboot/recovery, rapid contradictory
  commands, both independent lights, day/night dwell, low light validity and
  reset bounce/hold. Measure capture corruption/EMI/power/USB disturbance during
  motor and light changes. Re-run combined with recording and network control.

## Gate

G4 requires actual observed mechanics and bounded stop, independently working
fitted lights, truthful ambient data/presence and qualified reset input. A GPIO
register match is not motor movement or optical light proof. Calibration/results
belong to the board/sensor/head identity and must not leak into later ports.
