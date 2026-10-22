# Parent offline disposition — query77 build/package, not native admission

2026-10-22; PLAT-01/08, ENC-02. **Accept this exact new qualification artifact as
an offline source/build/package baseline.** This is not an independently repeated
clean build, boot result, station-module ABI pairing or product acceptance.

Parent read all six authored recipe/helper files, the build result, manifest and
both method/failure notes. A separately authored read-only verifier did not import
or run the candidate builder/packagers. `PARENT_VERIFICATION01.json` records
250,219 distinct dependency paths hashed, plus the verifier's own pin. It checked:

- Full91,268-entry retained source and91,270-entry new source maps and exact file
  sets,25 retained SCM-data entries,4,761 retained build entries,3,913 new build
  entries and581 prior query/package custody entries unchanged.
- Exactly two modified/two added kernel files, each byte-identical to the already
  reviewed query candidate. No station/P1 or other driver change. The actual
  provider `.cmd` names the new source and generated-header roots, not old76.
- Exactly five config differences: compiler text, Clang/assembler/LLD230101 to
  230103 and the explicit localversion. All other symbols, including RELR support,
  memory/CMA and compression choices, match the old starting configuration.
- Actual generated compile metadata says Clang/LLD23.1.3 with neutral user/host;
  release is `6.18.55-qc07f-encoder-query77`, with no inherited dirty suffix.
- 1,612 Kbuild command files,3,903 consumed source/header paths,109 loaded host ELF
  path records,550 resource entries,46,508 system header/GCC-support files and
  3,261 supplementary support files match their captured pins. Final-only support
  inventories are not misrepresented as all captured before first use.
- All40 recorded successful top-level commands/log pins; exactly one full compile
  invocation, selecting `zImage vmlinux`, no modules/install target. Authored build
  PASS is metadata/integrity-verified, **not** rerun by parent.
- Legacy image length/header/data CRC, independent non-reflected full-image CRC,
  exact new zImage+retained DTB payload, XZ+LE32-to-Image association, byte-identical
  original/new decompressed initramfs and real ELF/zImage/stack/BSS symbols.

## Frozen artifact and static physical limits

`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-images02/QC6EQ77.UIM`

3,603,307B; SHA256
`f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e`.
CRC32/BZIP2 `63ea4eac`, header IEEE `011cd017`, payload IEEE `0d55fce6`.

Real kernel live `[0x81808000,0x82059bc0)`, staged image
`[0x82408000,0x82777b6b)`, conservative decompressor live end `0x82790578`.
Actual compressed `head.S:334–469` accounts for appended DTB/workspace, stack,
64KiB malloc and overlap checks. The retained40MiB map and existing CMA start
`0x82800000` leave457,352B after this conservative decompressor bound. No transfer,
RAM, CMA or allocation ceiling increased. This is static layout fit, not combined
media memory/headroom/DMA-placement admission or current physical state.

## Failure preservation and remaining gates

The original capability-probe refusal, metadata collector interruptions, helper
name collision and incorrect GZIP-kernel assumption are preserved. Corrections
changed host evidence methods only; no feature/warning/ownership protection was
disabled. `package.py` remains the failed historical main; only `package02.py`
produced the accepted artifact. No original75/76 source/image/module was replaced.

DTB/initramfs/qualification monitor are unchanged. Therefore the embedded original
station76 module and historical `wifi attach` entry remain **unpaired/unsupported**
in query77; never load/autoload it. PLAT-02 is not cleared. This is RAM-only and
larger than the existing1.5MiB KERNEL partition: no persistent installation,
partition changes or installer selection. Generated images/binaries and proprietary
retained payloads stay local, not curated Git/publication candidates.

No native operation is queued. The last target observation was held original
U-Boot at12:12UTC, not Linux;22b's catch was consumed and its dated coverage cannot
be inherited. A future boot/query needs a **new reviewed recipe**, fresh sole
ownership/physical-state/recovery admission and preservation of both failure HOLDs.
Do not reuse the old22b controllers or retry their failed reset. Full watchdog/
query/provider qualification, backing, external retention, provisioning/listener,
ordinary functional coverage, PLAT-01 reproduction and G2/G3/G8 remain open.
