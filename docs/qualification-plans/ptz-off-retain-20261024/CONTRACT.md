# RP-11 / B14 / CTRL-02 — retain uncertain PTZ output ownership

2026-10-24. Source-only author contract, written before the derivative. Baseline:
`bringup/qc07f/ptz-counter-candidate/ptz.c`, SHA256
`b4a30cc783e7af5b2b0cb8eb53dc34ff24c80f159cd3c8bfe59db9abb11d24a0`.
Baseline and every original input remain immutable. Only this documentation
directory and `bringup/qc07f/ptz-off-retain-candidate/` are author-owned.

## Defect and bounded correction

The old OFF path releases actual descriptors after failed/unknown LOW readback.
It also excludes a direction-output call that failed after possibly affecting
hardware, since its initialized bit is set only after success. A false status
flag after release does not retain ownership.

Under the existing global actuation mutex, reject either axis on a persistent
global retained-owner latch BEFORE history changes or GPIO acquisition. Preserve
the old counter-capacity guard byte-for-byte. Before descriptor acquisition or
any driver output attempt, obtain an actual self-module reference and consumer
device reference; handle either refusal without GPIO effects. A built-in
THIS_MODULE=NULL is the legitimate non-unloadable case, not a simulated pin.

Set an attempted bit BEFORE each fallible direction-output call. Keep successful
initialization distinct. Cleanup writes logical LOW only to attempted lines,
never reconfigures untouched ASIS lines, and checks actual output direction and
logical LOW on every attempted line. Capture the first cleanup error without
overwriting an earlier actuation error. Check the modern integer setter result;
any cleanup setter/direction/read failure or mismatch retains the original
gpio_descs array, its requested descriptors, the consumer device reference and
self-module reference in a single bounded static owner, independent of devm PTZ
storage. No retry/release/reset/force-free API exists. Retain scalar original
axis, attempted/initialized masks, first error, actual completed issued count and
phase. HOLD persists across consumer unbind/rebind/new probe; no remove callback
can clear it. No pointers or device identifiers enter status output.

Only full successful four-line initialization AND all four final direction/LOW
checks may set coils_off_verified. A verified attempted subset can be released
without claiming the unattempted lines OFF. A wrong-sized ASIS acquisition has
no direction/output attempt and can be released under the inspected provider
contract; acquisition is not claimed generally effect-free for arbitrary GPIO
providers. Normal verified completion releases the array and both added refs.
All logging/access precedes the final normal module_put; sysfs active-reference
drain protects the callback while normal removal waits.

## Preserved constraints and limits

Keep +/-4 steps, 20ms phase sleep, 1--1.5ms break-before-make, board5/four-GPIO
metadata, phase bytes, phase advancement, issued-step meaning, mutex, original
sysfs fields and physical_position_known=0. First errors and completed steps are
not rolled back. Probe/status perform no GPIO requests/output writes. Added
scalar HOLD fields do not constitute STOP acknowledgement or present coil safety.
No homing, zero assignment, new pin map, motion ceiling, timer, watchdog, broker,
PID, provider or backend framework. Stock MOTOR closeout forbids transplanting
contradictory open-loop homing; no automatic physical origin is available.

The retained Linux6.18.55 source supports actual gpiod-request provider module and
gpio_device references, consumer get_device, self try_module_get and sysfs active
drain. These protect descriptor storage/code and callback lifetime, NOT continued
provider binding or electrical function. Explicit provider unbind can still run
gpiochip_remove, numb its chip pointer, and release provider devres. No claim that
consumer/module references prevent that removal, suspend, rail loss, forced unload
or privileged writes. Native ownership/broker/provider-lifecycle constraints and
independent reset/power-cut recovery remain unqualified. Retention can be indefinite
and is not thermal safety, coil deenergization, a physical deadline, bounded STOP,
mechanical stop/displacement or product acceptance. No new HOLD cleanup action is
selected. Historical23h OFF is not fresh target state; RAM75 recovery/C1 and Wi-Fi
P1 review restrictions remain unchanged. Speaker testing remains deferred.

## Evidence allowed

Read actual retained kernel/provider/device/module/sysfs contracts. ONE fresh
actual changed-driver ARM object compile, using the ptz-counter saved argv with
only source/output/dependency paths redirected, NEW Clang23.1.3 and coherent
query77 source/prepared headers. No old builder/helper invocation. Keep actual
argv/tool/input/dependency/license pins, logs, literal diff/inverse evidence and
ELF metadata. Stop on pairing mismatch or first compile failure; no flag repair,
retry or build02. No tests, fixtures, target or HOST candidate-function execution,
module/image linking, native adoption, UART, device, network or Git/index action.
Independent ordinary review and parent disposition are required before use.
