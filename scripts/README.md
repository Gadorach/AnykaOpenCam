# Acquisition and offline inspection helpers

## Dated addition — repeatable QC07F volatile boot (2026-10-14)

`qc07f_boot.py` packages the existing sole-recorder U-Boot catch, fixed checked
stock rescue boot and SD-free checked UART RAM upload. See
[`docs/QC07F_REPEATABLE_BOOT_TOOL.md`](../docs/QC07F_REPEATABLE_BOOT_TOOL.md).
It performs no persistent flash/SD write; a selected public-code RAM payload is
logged. This later bring-up tool is separate from the preserved acquisition-phase
descriptions below. Run its hardware-free tests with
`python3 -B scripts/tests/test_qc07f_boot.py` and an owned workspace `TMPDIR` if
host `/tmp` is full. Host fixtures do not establish native boot/upload acceptance.

These helpers support the current state: the camera's flash chip has been removed and placed in a CH341 programmer, and the main processor's UART pins have been positively identified. **The flash capacity, programmer setup, UART voltage, OS port, baud rate, and actual connections still need local confirmation.** None of these programs discovers, reads, erases, writes, resets, or configures a flash programmer.

Use Python 3.8 or newer. `validate_dumps.py` and `inspect_firmware.py` use only the Python standard library. `capture_uart.py` additionally needs an already installed `pyserial`. Nothing in this directory installs packages or downloads tools.

Run the included 21-case self-test from the handoff root:

```sh
python3 scripts/tests/test_helpers.py
```

The tests generate and remove temporary binary fixtures and use a mock serial adapter. They require no camera, programmer, UART connection, or `pyserial` installation, and do not access hardware.

Keep each programmer read as a separate, immutable original. Analyse working copies. The examples below assume the shell's working directory is the handoff root, with `session_dir` set to the existing current-session directory created by `docs/ACQUISITION_RUNBOOK.md`. That directory contains `raw/`, `analysis/`, and `logs/`. These examples use POSIX shell variable syntax; adapt quoting and the Python executable to the actual host. Serial ports are examples, not discovered device settings.

## 1. Compare independent flash reads

After acquiring at least two independent reads using the separately documented manual programmer workflow:

```sh
python3 scripts/validate_dumps.py "$session_dir/raw/flash-read1.bin" "$session_dir/raw/flash-read2.bin" "$session_dir/raw/flash-read3.bin" --json-out "$session_dir/analysis/dump-validation.json"
```

Only after the exact flash part and capacity have been confirmed as **64 Mbit / 8 MiB**, add the conditional size check:

```sh
python3 scripts/validate_dumps.py "$session_dir/raw/flash-read1.bin" "$session_dir/raw/flash-read2.bin" "$session_dir/raw/flash-read3.bin" --expected-size 8388608 --json-out "$session_dir/analysis/dump-validation-8mib.json"
```

The validator reports file sizes, SHA-256 hashes, exact byte comparisons against the first file, the first differing offset where applicable, proportions of `00` and `FF`, entropy, whole-file repetition at small power-of-two periods, and repeated 4 KiB blocks. It warns if two arguments point to the same physical file.

Repeated erased blocks are common. **High entropy does not prove encryption or corruption.** Matching files show repeatability of these files; they do not prove independent acquisition, correct programmer voltage, a correct chip definition, or a correct read procedure. A whole-file uniform or trivial repeating pattern needs investigation even if every hash matches.

Exit codes:

| Code | Meaning |
|---|---|
| `0` | Separate input files are byte-identical, nontrivial, stable during analysis, and match the optional expected size. |
| `1` | Reads differ, the requested size differs, inputs are duplicate file references, inputs changed, or matching content is trivially uniform/repetitive. Read the report. |
| `2` | Invalid arguments, missing/unreadable input, or output creation error. |

All inputs are opened read-only. `--json-out` uses exclusive creation and refuses an existing file, including an input filename. It does not create missing parent directories. A report can still be written when the verdict is exit `1`.

## 2. Inspect a verified working copy

```sh
python3 scripts/inspect_firmware.py "$session_dir/raw/flash-read1.bin" --json-out "$session_dir/analysis/firmware-inspection.json"
```

This produces a metadata report and a console summary. It does **not** carve partitions, unpack filesystems, decompress payloads, execute embedded binaries, modify firmware, or access hardware.

The scanner examines:

- **Legacy U-Boot uImage:** 64-byte header, declared payload extent, header CRC with the CRC field zeroed, and payload CRC. Load and entry addresses are reported as observations, not suggested boot commands.
- **Flattened device tree candidates:** magic, v17-compatible header layout, offsets, alignment, region bounds, reservation terminator, balanced node structure, property lengths/name references, and final token. It samples root `model`/`compatible` and `/chosen` boot arguments. Older headers are retained as unsupported candidates rather than assumed valid.
- **SquashFS candidates:** v4.0 superblock fields, declared filesystem extent, block size, and table offsets. This is a plausibility check, **not full filesystem validation**. Byte-swapped signatures are reported as nonstandard candidates.
- **Readable version/platform strings:** Linux, U-Boot, Anyka, AnyCloud, AK3918, and KM01 strings where they occur uncompressed.
- **Whole-file and window entropy:** descriptive measurements only; a compressed kernel may not expose a readable Linux version string.

A signature can appear inside another object. None of the reported offsets proves an MTD partition boundary. CRC success establishes consistency of an image's own data, not cryptographic authenticity, target compatibility, or bootability. A valid FDT can also be a FIT image rather than the camera's hardware device tree; inspect its node contents.

Defaults: 64 KiB entropy windows, 256 hits per binary signature, 200 interesting strings, and a 256 MiB input-size guard. Adjust explicitly when needed:

```sh
python3 scripts/inspect_firmware.py "$session_dir/raw/flash-read1.bin" --window-size 4096 --max-candidates 512 --max-string-hits 400 --json-out "$session_dir/analysis/firmware-inspection-detailed.json"
```

The input is read into memory, so keep `--max-bytes` appropriate for the host. FDT validation has bounded token/reservation loops; a limit is reported rather than silently claiming complete validation. A completed scan exits `0` even when no candidates validate, because firmware is not required to contain any of these formats. Argument/I/O errors exit `2`.

`--json-out` creates only the requested new report and refuses an existing path. Inspect the JSON for full metadata, entropy windows, diagnostics, and hit-limit flags.

Format references used for the checks:

- [U-Boot legacy image definitions](https://github.com/u-boot/u-boot/blob/master/include/image.h)
- [Devicetree specification: flattened format](https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html)
- [Linux SquashFS superblock definitions](https://github.com/torvalds/linux/blob/master/fs/squashfs/squashfs_fs.h)

## 3. Optional receive-only UART capture

The processor UART pins are identified, but this package does not know their voltage or the adapter's OS port. Confirm the signal voltage and the adapter's compatible input level locally. For this helper, connect only **camera TX → adapter RX** and common GND. Leave adapter TX, VCC, DTR and RTS physically disconnected.

After selecting the actual port and a justified baud rate, a Linux example is:

```sh
python3 scripts/capture_uart.py --port /dev/ttyUSB0 --baud 115200 --seconds 120 --log "$session_dir/logs/uart-passive-01.bin"
```

A Windows port example is `--port COM5`. `115200` is an example to test, not a measured rate for this board. Configure/open the logger before a separately controlled power-on when capturing the complete boot sequence. This helper does not power-cycle anything. With the flash physically removed, a power-on cannot produce the normal flash-firmware boot log; any observed output would instead describe the processor's present boot state. Label such a capture accurately.

The logger uses 8N1 and disables software/hardware flow control. It requests DTR and RTS inactive **before opening** the port, but adapter/driver behavior can still cause transitions on open or close. Leaving those pins physically unconnected is the reliable way to keep them away from reset/boot nets. Opening a serial port configures the host adapter; "receive-only" means no UART bytes are written and no target reset is requested.

The raw `.bin` log is preserved byte-for-byte, including non-text bytes. No device-origin escape sequences are printed to the terminal. Stop early with Ctrl-C if needed; the partial log is retained. The script requires explicit `--port`, `--baud`, `--seconds`, and `--log`, never scans other ports, never invokes a shell, never sends login/bootloader keys, and refuses to overwrite a log. If opening the port fails, an empty new log may remain and is intentionally retained.

Exit `0` means the requested timed capture completed; zero received bytes is reported but is not automatically classified as a hardware fault. Ctrl-C exits `130`; argument, dependency, serial, or file errors exit `2`.
