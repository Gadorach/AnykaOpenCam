# QC07F bounded motor qualification (not yet deployed)

The user explicitly authorizes GPIO writes and attempted PTZ at the safe camera location. No homing, bypass-limit or unbounded motor action is included.

Retained retail board-version5 selects direct GPIO candidates:

- Axis0:10,11,19,20 (retail pdatac041e5d4).
- Axis1:15,14,13,24 (retail pdatac041e630).
- Mechanical axis names, orientation, limits and actual physical displacement remain unqualified. User's earlier version4 report is preserved; both branches select the same candidates, but that is not pin-level measurement.

REA `ak_motor_probe`/`motor_hw_timer_handler` and the exact4-byte read at11618 confirm phase sequence03,06,0c,09. Evidence/input hash is retained in `work/lightbulb-camera/implementation-20261008T205404Z/analysis/bounded-ptz-phase-sequence-read.json`.

`ptz.c` is now compiled/linked in candidate31/32, **not deployed or hardware-qualified yet**. Probe inspects metadata only, without GPIO requests/writes. Explicit command pulses are capped at4 phases20ms each, serialize both axes, deenergize between changes and after completion, and report only issued steps/GPIO readback, never fabricated mechanical position. Five host phase/bounds/partial-cleanup/source fixtures pass; they are not motor tests.

Before a real pulse, integrate the exact DT/command/guard, qualify GPIO cleanup after partial direction failure, and check whether watchdog/ROM recovery resets GPIO outputs or requires an armed forced-off cleanup sequence. Do not rely on a watchdog reset alone to guarantee coils deenergize. Acquire before/after replacement images for actual movement evidence and release every coil afterward. No flash or motor calibration values are persisted.

Recovery evidence collected after source29 watchdog reset: native U-Boot `md.l20170000 7` shows **all three direction and all three output-latch banks zero** at the held CLI, whereas sensor reset41 was previously configured output/high. Evidence is `analysis/source29-watchdog-gpio-reset-reference.json`. This proves register cleanup through observed reset/ROM startup, not coil current/thermal behavior or physical limits. Explicit pulse cleanup handles only successfully initialized output descriptors and verifies all four low when all were initialized. The first command remains bounded under an armed5s watchdog and recovered-U-Boot hold.
