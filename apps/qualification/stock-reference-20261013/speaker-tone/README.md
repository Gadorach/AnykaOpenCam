# Tone06 — reversible HP analog code1 ->3 ->1

Sole selector: **`--native-stock-speaker-tone-gain3-uart-isolated`**, absolute argv0.
Parent alone reviews and launches it over the already-verified SSH connection.
The prior selector is rejected. Tone05's sealed source/artifact/tests, including
the parent's build02, are preserved. User heard no sound from05 and requested
one analog code3 trial. Tone06 is a source/host candidate, not a played tone.

Requires parent external admissions `SPEAKER_TONE04_PARENT_GPIO0_SYS74_LEASE_V1`
and **USER_UART_SIGNALS_DISCONNECTED** (both TX and RX physically disconnected).
No added receipt file/fd/service: helper cannot measure the disconnection or
prove exclusive kernel ownership. Minimal controlled current stock epoch has no
motor/sensor/camera providers or motor timer IRQs; parent must revalidate current
owners/IRQs, no bank0/SYS74 actors, no full app/LED timers/ADC/video and RAW0.

Initially maps fixed SYSCTRL and GPIO pages read-only for baseline checks, then
replaces them with explicit RW views. Entry requires UART mask6, full OUT0=2 and
DIR bit1 set. Preserves exact whole DIR0, all other OUT bits and all other SYS74
bits. One fixed SYS74 store clears **mask0x2 only**, selecting GPIO1 before PCM
open; mask0x4/GPIO2 remains unchanged. No SYS78, IR-cut6/7, DIR or other GPIO write.

Original AO tuple `[8000,2,16,4096,12,4096,0]`;49152-byte ring;512-byte scratch;
16 writes/8192-byte prefill. Public identical-L/R16LE tone1792 frames, nominal
224ms/1kHz, peak8192 with64-frame fades;1024 zero padding bytes. Digital samples,
clock and PCM/mux/guard primitives match05. No AEC/NR/capture or alternate pins.

HP gain uses stock SET0x40045030/GET0x80045030, exactly4 bytes. Require initial
cache1 and actual HP field8 at0800009c[22:18]. After PREPARE, before any write/
RESUME, mark intent then SET3 once; require GET3 and hardware field2. Recheck3/2
after RESUME and healthy PAUSE. Restore SET1 once **before last close**, GET1 and
field8, then field8 again after close and after GPIO/mux restoration. No direct HP
register store; only read that witness through the existing mapping. Codes are
not percentages or calibrated dB. Physical sound remains unconfirmed.

Every attempted SET3 can have side effects even on nonzero/canary failure. On
known preactive failure, verified original1 may be restored once before close.
Unknown initial gain is never normalized. Uncertain active/stop/restore/close or
post-close hardware mismatch retains remaining owners without normal return.
Return0/1/2 remains restricted to no-effect or confirmed-quiescent fully-restored
classes for the parent's outer watchdog policy; SIGSTOP is not a return code.

Healthy PAUSE -> verified HP1 restore -> last close -> optional OUT0=2 -> SYS74 restore.
Maximum stores3 (2 if OUT already2). Known unopened/preactive cancellation can
perform bounded zero-DMA rollback; unknown active/stop/close/store state never
grants repair. Unexpected DIR/other OUT/SYS changes, failed store/readback or clock
budget retain remaining owners/maps and SIGSTOP, with no retry or SIGKILL.

The2s alarm only marks cancellation, not a kernel hardIRQ timeout. SIGHUP is also
mark-only; neither establishes SSH fault survival. Parent's
independent watchdog/recovery must be reviewed before launch: serial catch is
unavailable; watchdog reset could boot the normal app. Keep SSH client alive>=75s
for uncertain DMA; on SSH loss/missing completion, user powers OFF and leaves OFF.
No fault-time SSH-survival or reset-to-held-U-Boot qualification is claimed.

Result schema **stock-speaker-tone-v6** records SYS74/GPIO snapshots, each MMIO
count/order, gain before/test/during/after, SET attempts, restored flag and raw/
encoded HP field witnesses. Kernel gain ioctls are separate from helper MMIO
stores (still maximum3). Cache readback alone is not hardware-field proof.
`physical_heard:AWAIT_USER` remains required. Restored UART mode is a logical state
restoration, not pad-voltage proof on disconnected wires.

Host-only build/test (fresh output directory required):

```
python3 -B apps/qualification/stock-reference-20261013/speaker-tone/build_and_test.py buildNN
```

Outputs/snapshots/diagnostics go only to `speaker-tone06/buildNN`; own TMPDIR.
Optimized/ASan/UBSan POSIX+MMIO mocks run the actual entry. ARM is not executed.
See `docs/qualification-plans/stock-reference-20261013/speaker-tone06/CONTRACT.md`.
No copied public-demo code or new workspace-wide license grant.
