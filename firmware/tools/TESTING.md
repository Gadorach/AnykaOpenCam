# Offline verification — 2026-11-14

Verified against the actual original `raw/flash-read1.bin` in each preserved
2026-10-08 acquisition mirror, with the three pinned whole-image hashes from
`profiles.py`. Outputs were fresh, private directories outside the repository;
no firmware executable, hardware, programmer, flashing script or build was run.
No packages were installed and no network operation was performed.

## Final source results

| Profile | Original partition metadata matches | Prior exact container/kernel matches | Regular files extracted |
| --- | ---: | ---: | ---: |
| T34-4G / KM01 | 9 / 9 | 3 | 55 ROMFS |
| QC07F | 7 / 7 | 4 | 201 across root + usr |
| Fulhan FH8626V200 | 7 / 7 | 3 | 124 application |

- All three compatibility wrappers passed both normal extraction and
  `--carve-only` against unchanged real inputs.
- All 23 full partition byte lengths/hashes matched the original session's
  `analysis/partition-map.json` (KM01) or `analysis/basic-details.json` (Linux).
- Prior container matches cover KM01's two uImages and ROMFS, QC07F's uImage,
  two SquashFS images and decoded kernel, and Fulhan's uImage, SquashFS image
  and decoded kernel.
- All 55 KM01 ROMFS file lengths/hashes matched the original
  `analysis/extracted-file-inventory.json`, including the 11 vendor-compressed
  files. Original stored compressed streams are preserved by the tool.
- Optional `--littlefs` passed using the already retained host
  `littlefs-python==0.19.0` dependency. All 40 CONFIG file lengths/hashes matched
  the prior inventory; no program/erase callback was invoked. That dependency
  was not copied into these tools and is optional for public users.
- The Linux regular-file views are new extractions from the verified original
  SquashFS containers, **not** comparisons against an earlier full-tree inventory
  (the original sessions only listed/read selected files).
- Every listed output artifact was reread and its byte length/SHA-256 checked
  against the new manifest: 84 KM01, 216 QC07F and 137 Fulhan entries, excluding
  each manifest itself. Optional LittleFS output was also rechecked.
- Existing-output refusal passed for all six wrapper/mode combinations without
  changing the existing manifest. Original input hashes were unchanged afterward.
- All newly created output directories were 0700, regular files 0444, and no
  symlink/device node was materialized. Python source AST parsing passed.
- Host SquashFS tool: `unsquashfs 4.7.5 (2026/03/01)`; KM01 decoding used host
  `liblz4.so.1`. No extracted file was executed.

## Preserved earlier finding

Initial real extraction and metadata comparisons passed, but an additional output
mode check **failed**: `Path.mkdir(parents=True, mode=0o700)` did not apply 0700
to intermediate directories. Their enclosing fresh extraction root was already
0700, but the nested-directory claim was too strong. The source was corrected to
create each missing component explicitly with mode 0700, without changing existing
directories. The complete final verification above used fresh destinations and
passed. Earlier partial/test outputs were retained, not overwritten or relabelled.

## Not covered

No JFFS2 reconstruction, generic filesystem/parser certification, adversarial/fuzz
testing, target execution, native-code image build, boot/restore/install check,
runtime compatibility, credential-policy assessment or electrical acquisition
validation is claimed. See `README.md` for exact extraction limits and commands.
