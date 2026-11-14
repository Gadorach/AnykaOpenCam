# Original stock firmware: offline extraction

These tools read the **three original 8 MiB flash main-array dumps**, not working
images built from the project's current native code. They do not access hardware,
program flash, boot a camera, run an extracted binary/script, or install anything.
The maps are corroborated **offline**, not a restoration/boot recipe. Chip OTP,
processor eFuses and separate modem storage are not included in these dumps.

## Run from the repository root

```sh
python3 -B firmware/tools/extract_t34_4g_km01.py
python3 -B firmware/tools/extract_qc07f.py
python3 -B firmware/tools/extract_fulhan_fh8626v200.py
```

Each command selects its own profile, verifies the entire input's size and SHA-256,
and creates a **new** `firmware/extracted/PROFILE/` directory. No overwrite,
padding, truncation, repair or hash-check bypass is supported. A failed extraction
leaves partial outputs for inspection; choose another fresh `--output` to retry.
Existing output directories (even empty ones) and output paths through symlinks
are refused. Run unprivileged, with a trusted host toolchain, not as root.

| Profile / input under `firmware/original/` | Identity | Expected SHA-256 (8,388,608 bytes each) |
| --- | --- | --- |
| `t34-4g-km01.bin` | Anyka AK3918AV100, AliOS Things; **not Ingenic T34/Linux** | `55765bfd352f3101bc71204d537d6d216e42d2dd6d209f98753c2dd5960ac71d` |
| `qc07f.bin` | Anyka AK3918EV200, Linux 3.4.35 | `42f50a345b17e2c5fa00cab2b1a29bd900b793d5bbb8f3c0d9e2b41e603b2ba3` |
| `fulhan-fh8626v200.bin` | Fulhan FH8626V200, Linux 4.9.129 | `c7eda7587953a868064bab58857139c3a6a9edfcbc9937543766146b0c813fc0` |

All wrappers also accept `--input PATH`, `--output NEW_DIRECTORY`, and
`--carve-only`. The shared entry point is equivalent:

```sh
python3 -B firmware/tools/extract.py qc07f \
  --input firmware/original/qc07f.bin \
  --output firmware/extracted/qc07f-second-copy
```

Defaults are relative to the scripts' repository layout, not the caller's working
directory. No acquisition session, `work/` path, UART helper, build tree, network,
download or package-install command is needed.

## Host requirements and actual coverage

- **All profiles:** Python 3.10+ with its standard library, including `lzma`.
  No `binwalk`, `flashrom`, compiler, target SDK or extracted executable is needed.
- **KM01 default:** host `liblz4.so.1` (Linux LZ4 shared library). The ROMFS parser
  checks the superblock and reachable inode metadata checksums, audits names and
  block tables before creating paths, and unpacks the observed vendor type-6
  compression: 4096-byte blocks, codec 1, ROMFS-relative table offsets, size-prefixed
  LZ4 and bit-31 literal-block flags. It preserves the stored compressed streams.
  This is the observed Anyka variant, not a general ROMFS extractor. The original
  two uImages and their flat payloads, three DTBs, and all nine full partitions
  are also carved. The AliOS uImage's generic Linux OS field is **not** evidence
  of a Linux kernel; `usr/app.elf` is never executed.
- **QC07F/Fulhan default:** installed host `unsquashfs` with XZ support and the
  `-lln`, `-UTC`, `-no-wildcards`, `-cat` options (verified with squashfs-tools 4.7.5).
  Exact SquashFS `bytes_used` extents are carved separately from full partitions.
  Every audited regular file is read with host `unsquashfs -cat`; directories are
  created locally. Symlinks and special/device nodes are **recorded only, never
  created or followed**. Any member beneath a symlink is refused before extraction.
  This is a safe regular-file view, **not a mountable/bootable filesystem replica**:
  original ownership, permissions, links, xattrs and times are not restored.
- **Linux kernels:** validate both uImage CRCs, preserve the original uImage and
  ARM zImage, then extract the exact valid embedded XZ stream and decode it with
  Python `lzma` (32 MiB output bound / 128 MiB decoder memory bound). The uImage
  compression field says `none`; the zImage still contains compressed kernel data.
  QC07F's valid XZ is at payload offset `0x3868`; Fulhan's is `0x3774`. Other
  signature hits are not substituted. No `vmlinux` ELF is claimed.
- **JFFS2:** full partitions are carved **only**; no JFFS2 node reconstruction,
  deleted-file recovery or writable-filesystem tree extraction is implemented.
- **KM01 LittleFS:** CONFIG is carved by default. Optional `--littlefs` also reads
  its files using an already available `littlefs-python` installation (verified
  with 0.19.0). No Python package or binary extension is bundled or auto-installed.
  The parser mounts an immutable in-memory copy with automatic formatting disabled
  and denied program/erase callbacks; attempted writes fail verification.

```sh
# Optional, only if littlefs-python is already installed in this interpreter:
python3 -B firmware/tools/extract_t34_4g_km01.py --littlefs \
  --output firmware/extracted/t34-4g-km01-with-config

# Stdlib-only carving: no liblz4/unsquashfs/LittleFS package required.
# This still validates containers and decodes the Linux kernel's embedded XZ.
python3 -B firmware/tools/extract.py qc07f --carve-only \
  --output firmware/extracted/qc07f-carvings
```

## Output layout

```text
firmware/extracted/PROFILE/
  manifest.json                 # artifact names, byte lengths, SHA-256, provenance
  partitions/                   # full original candidate partition ranges
  containers/                   # exact uImage/payload, DTB, ROMFS or SquashFS extents
  decoded/linux-kernel.bin      # Linux profiles only
  audits/                       # validated filesystem names/types/modes/block tables
  filesystems/romfs/             # KM01 regular files (55 files)
  stored-romfs/                 # KM01 original vendor-compressed streams (11 files)
  filesystems/config/            # optional KM01 LittleFS (40 files, 3 directories)
  filesystems/root/              # QC07F root regular-file view
  filesystems/usr/               # QC07F /usr regular-file view
  filesystems/app/               # Fulhan application regular-file view
```

Only applicable entries are produced. Files are written exclusively and mode 0444;
new output directories are owner-only (0700), and no executable bit is granted.
Manifest offsets are relative to the named source, with the original dump named
`original`. For decoded filesystem members there is no single linear byte mapping:
offset/length are `null`, the containing filesystem and member operation identify
the source, and output byte length/hash are always recorded. ROMFS audit tables
retain its real stored offsets. The manifest does not hash itself.

Extracted content, audits and manifests can contain secrets/device configuration.
Keep the generated `firmware/extracted/` tree local/ignored; do not treat a
permission change or extraction as sanitization. Original firmware includes
credential/private-key material; do not reuse its credentials or imply those
values are safe defaults. These tools intentionally do not print file contents.

## Profiles and provenance

`profiles.py` transcribes the session partition tables without changing boundaries:

| Profile | Map basis / preserved caveat |
| --- | --- |
| KM01 | CRC-valid saved ENV/FLAG, observed AliOS/ROMFS/LittleFS; conflicting DTB map is not used. Saved kernel read length extends past the KERNEL partition and is not used as a partition size. |
| QC07F | Compiled default bootargs and actual kernel/SquashFS/JFFS2 positions. The ENV region has contradictory plain text, not a validated standard U-Boot environment. |
| Fulhan | CRC-valid saved environment and actual kernel/JFFS2/SquashFS positions. Other bootloader default maps are not substituted. |

The source methods are consolidated/adapted from these preserved acquisition
scripts and their `RESULTS_SUMMARY.md` / `analysis/` metadata:

- `firmware/stock/km01-lte/session-20261008T001154Z/`:
  `extract_romfs.py`, `extract_littlefs.py`, `extract_squashfs.py`,
  `initial_analysis.py` (carving). The corrected ROMFS literal-block handling is
  retained, not the earlier failed attempt's behavior.
- `firmware/stock/lightbulb-camera/session-20261008T024241Z/`:
  `basic_analysis.py`, `basic_details.py`.
- `firmware/stock/camera-03/session-20261008T032326Z/`:
  `basic_analysis.py`, `basic_details.py`.

Those historical scripts are **not runtime dependencies** and need not be present
in a public checkout. The scripts here and the three input dumps suffice, subject
to the explicitly listed host dependencies. See also `docs/PROJECT_LAYOUT.md` for
the acquisition mirror/provenance distinction. No upstream parser implementation
is copied into this directory: LZ4, SquashFS and optional LittleFS are external host
dependencies and retain their respective licenses. Firmware/vendor code is not
relicensed by inclusion; no global open-source grant, vendor-source availability,
runtime compatibility or installation qualification is claimed.
