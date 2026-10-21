# QC07F MMC source/build candidate — parent review handoff

**PLAT-04 / STORE-01 / STORE-02: source audit + synthetic tests + ARM Kbuild
objects/relocatable link complete. Native host implementation remains blocked;
probe unconditionally refuses before resource claims/discovery.**

This is deliberately **not a usable MMC driver or bootable image**. Unknown
retail power/CD/voltage and shared L2/DMA ownership/retirement are not guessed.
See [SOURCE_AUDIT.md](SOURCE_AUDIT.md) for controller facts, existing UART
conflicts, resource ownership, missing implementations and one parent step.

## Latest correction — v2, parent review pending

Parent found that v1 skipped zero after generation overflow and could reuse an
old event namespace. **V1 must not be adopted with that wrap behavior.**
`qc_request_ready()` now fences idle `UINT32_MAX` exhaustion before claims or
Linux work scheduling; `qc_run()` never wraps. `UINT32_MAX` remains the final
valid operation generation, active-operation/poison precedence is preserved,
and null `qc_irq_event()` arguments fail closed.

V2 evidence:
`evidence/delegated/20261012-mmc-candidate/v2-20261012T180922.619999Z/`.
Its `HANDOFF-v2.json` points to the fresh source diff, tests and ARM closure.
Optimized + ASan/UBSan tests each pass **3230 checks**, including near-max/final
generation, exhaustion with zero provider effects, stale generation1 callbacks,
in-flight-operation preservation and failed-retirement quarantine. The work
gate is tested as a pure fixture; no Linux workqueue/native execution is claimed.

`v1-preserved/` was copied **before editing**, including v1 source/handoff,
Kbuild objects/commands, logs, config and seal. Focused v2 compilation redirects
source/include/output/dependency paths while using the unchanged, hash-verified
v1 generated/public headers. Both new ARM objects and their relocatable link
pass; no 91k-file copy, Kbuild/header preparation or DT build was repeated.
Probe, resource constants, DT, Kconfig and filesystem config remain unchanged.

Focused rerun (creates a unique new output directory):

```sh
python3 bringup/qc07f/mmc-candidate/build-v2-focused.py evidence/delegated/20261012-mmc-candidate/v2-20261012T180922.619999Z
```

The v1 results below remain historical evidence; native blockers and all product
gates remain unchanged.

## Reviewable files

- `host-core.c/h`: Anyka register/command adaptation, bounded SD-only read-only
  admission/event/timeout/cancel/retirement engine, no real backend.
- `qc07f-mci.c`: dormant Linux6 MMC glue and no-effect refusal-only probe.
  Missing native set_ios/CD/IRQ/DMA/remove integration is explicit, not faked.
- `Kconfig` / `Makefile`: built-in candidate, default off; no module loading.
- `candidate.dtsi`: disabled node, one-bit/400kHz candidate limits, no outputs.
- `filesystems-candidate.config`: **separate unused** FAT/EXT4/block candidate.
  Host object build leaves MMC_BLOCK and EXT4 off, and disables power sequencers.
- `test-host-core.c`: synthetic callbacks, not target/model qualification.
- `build-offline.py`: fresh copied Linux scratch, standalone host fixture and
  Kbuild object proof; never installs/packages/boots a kernel or module.
- `seal-existing-build.py`: finishes pending object linkage/closure after a
  restart without repeating completed fixtures/compilation.

## Final evidence and failures retained

Workspace-relative final build:
`evidence/delegated/20261012-mmc-candidate/build-20261012T174515.810242Z/`.

Final link/closure seal:
`seal-20261012T175950.282234Z/REPORT.json` inside that build.

- Optimized + ASan/UBSan host tests: **3190 checks each**, exhaustive **2,105,344
  descriptor cases**, 17 lifecycle modes plus APP/busy/IRQ/clock tests.
- Linux6.18.55 pinned header compilation, ARM926/ARMv5, `KCFLAGS=-Werror`:
  both objects pass. Final relocatable link contains dormant worker and core.
- Disabled DT compiles without warnings and round-trips `status="disabled"`.
- **542 actual ARM header dependencies** pinned in `arm-header-closure.json`;
  actual compiler commands/dependencies retained in Kbuild `.cmd` files.
- **91,256 copied source-file identities** match unchanged original inputs;
  only isolated scratch MMC Makefile/Kconfig receive integration additions.
  Config reverse/diff proof leaves video/CPU/recovery/security options unchanged.
- Tool binaries/versions and every invoked build command, cwd, time, logs and
  input/object/output SHA256 are in build/seal reports. No target binary ran.
- ARM combined object SHA256:
  `4ce97df141e122efa9a0226b0ee2c77a0c5711e423b4f739aa6d10e88c4549de`.
- Object-only config SHA256:
  `fa67247c3161f1ec787727d8e49b2283144afe18888e14e4155d494e105d9118`.

First attempt `build-20261012T173139.673219Z` preserved its rejection of upstream
internal symlinks after fixture success. Second attempt
`build-20261012T173308.818428Z` completed object proof with dormant-worker/DT-cell
warnings; those logs and its earlier candidate remain preserved. Current build
completed fixtures/Kbuild before the harness restart interrupted sealing;
its original `REPORT.json` stays INCOMPLETE. The fresh final seal references that
immutable report and completes **only** the remaining link/DT/source closure.
Do not treat the harness cancellation as a failed compile or rewrite old logs.

## Reproduction (offline only)

From the workspace root:

```sh
python3 bringup/qc07f/mmc-candidate/build-offline.py
```

Creates a uniquely named new evidence directory and refuses overwrites. It
copies the public pinned Linux tree, preserving audited internal symlinks, not
hard-linking/mutating any active source or build. It keeps block/filesystems off
for this object proof. No SDK `auto_build`, install, full image or target helper.

After an interruption **only if** the report records successful final Kbuild
objects and no subsequent commands, resume the pending seal with:

```sh
python3 bringup/qc07f/mmc-candidate/seal-existing-build.py evidence/delegated/20261012-mmc-candidate/build-20261012T174515.810242Z
```

This creates another new seal, never overwrites old evidence. It verifies the
input candidate source hashes before linking.

## License and scope

GPL Anyka original copyright/notices retained in evidence `upstream/akmci` paths
(`drivers/mmc/host/akmci.c`, `include/plat-anyka/akmci.h`) and original `COPYING`.
New derivative core and candidate files are GPL-2.0-only. Other upstream sources
keep their own licenses. REA contracts were comparison evidence only, not copied
as implementation or assigned a source license. No workspace-wide license.

No target/card/SD/register/UART/network/driver/mount/format/flash/SPI/OTP/reset
operation, private provisioning, ROM/photo/footage access or publication.
No canonical plan/status/runner, active helpers/DT/config/image/build, framework,
authentication/audio/storage code, or Git state was changed. Parent review and
all native/product gates remain open. Full vmlinux/module runtime linkage, ABI,
memory footprint and combined-load performance are **not** proved by this link.
