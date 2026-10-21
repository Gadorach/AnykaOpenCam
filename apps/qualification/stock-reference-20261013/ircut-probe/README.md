# IR-cut fixed-pair source prototype — actuation DISABLED

Own implementation, not the retail sysfs/shell pulse leaf. See
`docs/qualification-plans/stock-reference-20261013/ircut-native01/RESULTS.md`.

`pulse_sequence.h` is shared with host register fixtures. `kernel_disabled.c`
fixes admission to zero and module initialization to `-EOPNOTSUPP`, exposes no
endpoint and does no initialization MMIO. **No kernel module is produced. Do not
build, stage, load or enable this prototype on a camera.** It is not an installed
driver or approved actuation recipe. There is no Makefile or deploy helper.

The proposed future primitive accepts only GPIO6 **or** GPIO7, once, with no
duration argument. After atomic read-only idle checks: one OUT0 store, ten finite
`udelay(1000)` calls, unconditional OUT0 baseline/release store and readback,
then diagnostics and IRQ/preemption restoration. It never changes mux/direction,
pulls, timer, reset input or UART selection. A failed release is reported, not
misrepresented as electrical coil-off, and an attempted pulse cannot be retried.

The host hook model does not establish kernel buildability, physical wiring,
timing, electrical release or optical state. IRQ masking is not a hardware
one-shot: CPU/bus faults and unqualified FIQ/clock/calibration remain outside its
conditional bound. Userspace sleep/signal cleanup is deliberately not used.

Run only the offline evidence collector:

```text
python3 evidence/delegated/20261016-ircut/pulse-design01/capture.py
```

It refuses to overwrite its `run01/` evidence. New standalone files use
GPL-2.0-only; this does not change licensing of the repository or retail inputs.
