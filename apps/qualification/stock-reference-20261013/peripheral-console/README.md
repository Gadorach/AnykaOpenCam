# Peripheral console07 — camera-local tone and manual ambient

Implements `PERIPHERAL_GUI_AUDIO7_INTERFACE.md`: inert `--capabilities`, read-only
`--status`, `--tone FREQUENCY_HZ GAIN_CODE VOLUME_PERCENT DURATION_MS`, and exactly
one `--ambient` sysfs read. Samples are synthesized in camera C; host sends numbers
only. No audio upload/file/voice/hash interface or new daemon/transport.

Full integer ranges: positive frequency strictly below actual calculated DAC
Nyquist (currently1..4005 at8012), gain0..5, volume0..100, duration0..5000ms. Duration0
is no-effect success; volume0 is valid silence. Volume25 maps to peak8192. Tone
uses stereo identical-L/R16LE triangle, fades and initialized zero tails. The
original valid cache/hardware gain is restored, not normalized to1.

Logical kernel ring4096×12=49152; fixed512-byte user scratch. Four whole periods
prefill, source-counter-window refill, modulo ring placement and conservative
pre/post-write low-water checks support5s without full-waveform allocation. Exact
retail allocation is an order4/65536-byte owner; the ISR low-water zero spill can
reach53248 and is retained/documented, not assumed from a generic allocator.
See `docs/qualification-plans/stock-reference-20261013/peripheral-console07/CONTRACT.md`.

Parent must independently review/stage and revalidate original stock3.4.35/RAW0,
exclusive bank/PCM ownership, physically disconnected UART and recovery. Target
runtime is currently UNKNOWN after the parent's SSH loss; no dated owner/catch
is authority. No native execution by this worker. Uncertain active state retains
owners/SIGSTOP; no kill/repeat/automatic reboot. User OFF/leaveOFF on loss/missing
completion; no UART catch or fault-time SSH-survival claim.

Ambient is a separate externally guarded one-shot. Stock ADC conversion can wait
forever IRQ-disabled; alarm/SSH cancellation does not prove interruption. No ADC
read on status, capabilities, startup or a timer. Values0..2999 are stock-scaled,
uncalibrated, higher=darker per the interface—not lux or measured millivolts.

Host-only build from workspace root, fresh output required:

```
python3 -B apps/qualification/stock-reference-20261013/peripheral-console/build_and_test.py buildNN
```

Own workspace TMPDIR, immutable snapshots/commands/diagnostics/hashes including
failed builds. Actual CLI runs under POSIX/MMIO/raw-ring mocks in optimized and
ASan/UBSan modes. Static ARM artifact is never executed here. Older speaker-tone
and GUI/private SSH files remain outside this unit. No new workspace-wide license
grant; retained source facts/notices and pinned toolchain provenance are preserved.
