# Our speaker route08 candidate — source/host only

Derived by byte-copying **our** peripheral-console7, not by copying retail kernel
implementation. Route02 supplies only reviewed hardware/ABI facts. Console7,
GUI, reset, IR-cut and previous evidence are unchanged. This is not GUI-v7
compatible and must not be installed behind its buttons.

No native readiness/launch approval. UART remains connected for the parent's
IR-cut-first campaign. GPIO1 effects are forbidden until the user physically
isolates **both TX and RX** and the parent reviews current SSH/recovery/ownership.
The CLI token below is only an external assertion, never physical proof.

## Interface

- `--capabilities`: no device/runtime operation; v8 capabilities JSON.
- `/absolute/binary --status`: stock3.4.35 UID0/non-PID1 helper and FD3-closed
  checks, read-only fixed SYS/GPIO mappings. No PCM, ADC conversion or writes.
- `/absolute/binary --tone FREQ GAIN VOLUME MS --uart-isolated`: positive duration
  requires this exact token. Numeric decimal arguments only: frequency1 through
  floor((calculated_DAC_rate−1)/2), raw gain0..5, volume0..100, duration0..5000ms.
- Zero duration may omit the token; it reads metadata/RO views, does not open PCM,
  upgrade mappings, enable speaker, change gain, generate samples or write MMIO.
- No manual/automatic ADC command, GPIO command, register address, waveform/file
  input, voice/recording transport, repeat, auto-tone or startup hook.

**Parent's first candidate trial only:** 1000Hz, raw gain1, volume25% (peak8192),
224ms. This is not a scheduled command. Five-second support is host-tested only;
the first powered experiment must not substitute5s or raise gain/loudness.

## Route and lifetime

Fixed original-compatible PCM tuple `[8000,2,16,4096,12,4096,0]`, device1/source4,
49152-byte logical ring with source-backed65536 allocation, 512-byte scratch.
Our rational camera-local triangle, 64-frame fades, four-period moving window,
free-space/counter/old-prefix fences, zero guards and wrap/retirement model are
unchanged. Arbitrary faded/partial-cycle requests are not claimed zero-DC.

After PREPARE and gain verification, mark speaker-enable intent **before** one
strict four-byte `0x40045081` word1 request. On this actual retail descriptor that
drives GPIO1LOW. Require exact OUT0=0 and GPIO1 INPUT low after return and at all
subsequent userspace fences. Kernel RESUME/close may briefly drive high during
their synchronous HP transition, then restore saved LOW. Do not infer that close
disables the amp. Record HP source bits16:14 after RESUME as a diagnostic, without
adding guessed analog writes or an alternate source.

Before intent and after OUT restoration, require exact OUT0=2; never blanket-
accept zero/high. Preserve DIR exactly and require GPIO0 INPUT, retain SYS78 and
all non-GPIO1 SYS74 bits. There are only three possible direct MMIO store sites:
GPIO1 mux enter, saved OUT0=2 after confirmed close, exact saved SYS74 mux exit.
No GPIO2, direction, pull, SYS78 or direct HP register writes. Gain changes use
the existing ioctl with cache/hardware-field verification and original restore.

Healthy ordering: ordinary source-conditional PAUSE/IRQ retirement → verified
original gain → last close → verify stillLOW → saved OUT0=2 → saved UART mux.
Receipt flags/counters describe actual attempts and verified readbacks, not
continuous electrical observation. `speaker_enable_return_zero` means the strict
packet wrapper accepted zero plus intact canaries. Zero NOOP reports enable word/
bytes/attempts and all effect counters0. HP source validity is false until observed.

Enable failure (including negative/positive/canary result or wrong OUT/INPUT) may
already have effects: retain remaining owners/maps and SIGSTOP, no close/repair.
Later known-preactive failures with a verified low route can use existing bounded
zero-DMA gain/close/OUT/mux cleanup. Uncertain RESUME, progress, stop, gain restore,
close or shared-register state retains; no retry, SIGKILL, active close or repair.
INT/TERM/ALRM/HUP mark cancellation; SIGPIPE is ignored. These cannot interrupt a
stuck hardIRQ/L2 wait. Existing parent outer60s watchdog guard and FD3-closed exec
contract remain external; no watchdog, transport or new guardian is implemented.

## Offline build

From repository root, choose an unused output:

```sh
python3 apps/qualification/stock-reference-20261013/speaker-route08/build_and_test.py buildNN
```

Produces one canonical static ARMv5TE `qc-speaker-route08-armv5`, optimized and
ASan/UBSan host-mock tests, source snapshots, compiler/link/header/library hashes,
commands/versions/timestamps, object disassembly, ELF checks and SHA256SUMS under
`evidence/delegated/20261016-speaker/route08/buildNN/`. No ARM artifact is executed.
The script refuses overwrites and preserves failed attempts. Seven core functions
are checked byte-identical to our pinned console7. The model is not hardware proof.

Parent alone independently reviews/rebuilds and owns physical feedback; no GUI
integration or follow-on queue. Existing source provenance/license obligations
remain unchanged; no new project-wide license or retail redistribution grant.
