# CTRL-08 — resumed reset-button identification, Oct16

## Actual completion of input-identification increment —04:06UTC

**GPIO0 active-low physically observed.** Three low pulses72/214/267ms matched
the user's confirmed several brief taps, each returning high.60004ms/5907 samples,
maximum gap12.213ms; no full debounce qualification claim. GPIO0 alreadyGPIO/input/
pull-up enabled, no config/output writes, GET/MMIO agree, clean helper retirement.
Boot identity unchanged/no reboot, ENV/ROOT/JFFS fingerprints unchanged. No real
factory reset or software reset handler enabled. A direct hardware-reset recovery
function remains unproved: do not rely on the button for IRQ-hang recovery.

UART16a capture/nextboot catch remains active at this checkpoint, not sealed.
See [actual result](../../qualification-plans/stock-reference-20261013/reset-button01/ACTUAL_RESULT01.md).
Next bounded offline GUI contract repair active, parent console07 build reproduction
only; no tone/ADC/output test or GUI launched. Older pre-press statements below are
preserved chronological preparation, not current input-identification status.

User resumed testing and explicitly prioritized identifying/verifying the reset
button **before UART is sealed for SSH/peripheral testing**. User confirms camera
ON, UART connected. Do not press/hold the button until parent gives a bounded test
window. Initial scope is input observation only, not factory erasure/settings reset.

## Fresh actual preflight

- New sole private RX owner `foundation-20261016a`, PID3463065/start97848247,
  source135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00.
  Attached to the reported running system with zero target TX; next-boot hold-cli
  catch actually armed02:55UTC through05:55:40UTC. These are dated facts requiring
  revalidation before native operations, not inherited authority.
- Full raw/logs are0600 in protected outside-Git state envelope
  `/home/gadorach/.local/state/anyka-bench-ssh/reset-button-20261016T0253Z/`.
  Default status is redacted. Keep this UART owner active through button work.
- Pinned SSH freshly confirms UID0, stock3.4.35/initPID1 and exact installed ROOT
  hash. ROOT/USR RO, JFFS/SD unmounted, only WD/USB/radio modules, no stock
  camera/cloud/reset-polling application observed. No input-device records/event0.
- No userspace `/dev/akgpio` or `/dev/mem` owner found. Private pretest ENV/ROOT/
  JFFS digest baseline saved without publishing values. No GPIO or input-device
  open/register read/configuration/output has occurred yet in this campaign.

## Current source hypothesis and next gate

Retained retail `GPIO_RESET_ReadValue` first tries event0/keycode11; fallback is
GPIO0 through **SET-before-GET** shared LED/reset helper. GPIO0 is a source candidate,
not physical proof. Never call that wrapper as a read-only probe. No blind pin/
PWM drive, alternate reset lines41/42 or GPIO2 experiment is authorized here.

One disjoint static worker is reviewing exact GPIO0 mux/direction/input semantics,
pure getter ABI and kernel/U-Boot/app reset side effects. Parent alone hardware.
Next is a reviewed, bounded passive observation route with pin-state/ownership gates;
only then a short parent-coordinated press/release with physical user feedback.
No long press or real persistent reset action at this stage. Preserve all failures,
starting configuration and flash bytes. Product debounce/long/stuck/setup-reset
policy remains distinct from initial button identification.

Evidence: `evidence/qualification/qc07f/stock-reference-20261013/reset-button01/`
`HOST_PREFLIGHT01.json`, `PRIVATE_OWNER_ARM01.json`, `RUNTIME_PREFLIGHT01.json`,
`OWNERSHIP_BASELINE01.json`. No GUI, tone, ALS, motor or illumination test started.
