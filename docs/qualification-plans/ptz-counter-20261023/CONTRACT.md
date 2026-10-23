# CTRL-02 — kernel-issued-step counter refusal

2026-10-23. During motor qualification preparation, parent ordinary inspection
found that the actual kernel `pulse_store` adds completed steps to a u32 counter
without checking overflow. The separate CLI checks it, but direct sysfs users
can bypass that userspace check. Preserve every original driver/image unchanged.

Create one fresh driver derivative with only an explicit limits include and a
counter-capacity check under the existing global actuation mutex, BEFORE changing
status flags or requesting any GPIO. Reject a request that could wrap with
`-EOVERFLOW`, unlock, and preserve all last-actuation facts. The completion count
is bounded by the validated1..4 request; the existing serialized final addition
then cannot overflow. No new control/position ABI, timer, phase, travel, retry,
homing, GPIO ownership or source/clock policy change.

Compile only this changed real driver ARM object using the retained new coherent
query77 source/generated headers and original command flags/compiler, with output
and dependency paths redirected to fresh candidate output. Do not run Kbuild,
replace old object/source, relink a kernel/module/image or execute target code.
This is NEW-baseline compile evidence, not old75 compiler/ABI reproduction or
deployable integration. Keep actual source delta/argv/tools/logs/input pins.
Independent ordinary source review precedes adoption. No new hardware action,
test/mutation/fuzz campaign, motor move, private input or GPIO operation selected.

This does NOT fix the original driver's descriptor release on failed OFF
readback, create quarantine/recovery, prove pulse timing or mechanical movement,
or qualify warm starts/counter-to-position mapping. Those gates remain explicit.
