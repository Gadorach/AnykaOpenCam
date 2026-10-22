# Query77 — full offline kernel build/package PASS; not booted

2026-10-22. PLAT-01/08 and ENC-02 dependency. **New explicit Linux6.18.55 /
Clang23.1.3 compiler/config/header/image baseline**, not retained76 reproduction,
external-module ABI acceptance or native query support observed on a camera.

## Actual deliverable

`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-images02/QC6EQ77.UIM`

- **3,603,307 bytes**, SHA256
  `f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e`.
- Whole-image **CRC32/BZIP2 `63ea4eac`**; header IEEE CRC32 `011cd017`, payload
  IEEE CRC32 `0d55fce6`. BZIP2 here names the CRC, not image compression.
- Real built-in kernel compile/link **PASS14:28:43–14:32:12UTC**; new package
  inspection/creation **PASS14:35:40UTC**. No built target program was executed.
- Release **`6.18.55-qc07f-encoder-query77`**, no inherited dirty suffix.
- Original format:64-byte legacy Linux/ARM/kernel uImage; timestamp0; outer
  compression NONE; load `0x82408000`, entry `0x82408040`; payload real new zImage
  plus exact retained DT45. Internal kernel compression is XZ+LE32 size;
  initramfs is GZIP. No padding/truncation, installer or boot-helper change.

Let `F = work/lightbulb-camera/foundation-20261011T041148Z`.

| Actual output under `F/analysis/encoder-query77-build02/build/` | Bytes | SHA256 |
|---|---:|---|
| `vmlinux` |10901136|`59460ed1744dda83d2762e941ea42e669fa51c977f78833db3787f416f1bfce8`|
| `arch/arm/boot/Image` |8531004|`0ed99d462759aa8d05b8ee89934abdcdfed73cdc7a6b082d03e25b301ce34eaa`|
| `arch/arm/boot/zImage` |3600672|`65bb107d9a33ca1733bfbdd8439a00b37e7c72f0f634cecfb9ce2cf924290eb8`|
| `.config` |58859|`d7b2ae8f167c8b56f442a201684eb433cc621bce45f52c98c94a87e48879b5bf`|
| `vmlinux.symvers` |345626|`806467d52330d4cdf2fd8e312aee54d24b0a1437d5eb4bd1b393011f277c4cdc`|
| `System.map` |1179543|`c0d4275455455bcdb74701dd52e44f45a53df52cd69f4665e4a159a8289fd9c5`|
| `include/generated/autoconf.h` |20061|`16095db58ef2ff2b64ef6f39e10e4b557c54a0fd42cd961f47a01c520c74c2b0`|
| `include/generated/compile.h` |164|`8c51d471baeb130ce5a5b31b20f006b9d0501433d2142a62cf3b5428f38c6a5c`|
| `usr/initramfs_inc_data` |1003683|`1a6d75f4684aa15f9c6df3bede9dcdff48967d765f938371d44ec3b9d6d44aea`|

Built-in-only modpost produces **`vmlinux.symvers`**, not a manufactured
`Module.symvers`. No `make modules`, external M= build, station/hgics build,
install or unsupported selected-module build was performed. Same exported-symbol
bytes as76 do **not** establish structure/module ABI compatibility.

## Exact source and coherent config

Full source02 is `F/artifacts/encoder-query77-source02/linux/`, copied from the
retained full76 source after tree/symlink/recipe/config/build-provenance inspection.
Upstream pin remains `725bd2f3c81d54edccb66fead01d4c0c222e2231` plus all retained
platform overlays and USB76 correction; the full91,268-entry input manifest, not
the upstream pin alone, defines it. No original source/build/recipe was edited.
SCM metadata25 entries was read/hashed separately and **not copied**;85 internal
relative source symlinks were inspected before copying. No Git operation occurred.

The only kernel source delta is the reviewed observational encoder OPEN_QUERY:
two existing files each containing the exact approved insertion sites, plus the
two exact new include files, giving91,270 candidate entries. Full byte comparison
against the frozen candidate passed. No query implementation/UAPI change here.

- Frozen query source pin:
  `ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`.
- Exact overlay patch:
  `9d096489e3ba8cc44556b81feb8192b6c5f08e698c18b2718cdfedbab5aea232`.
- Original provider:
  `e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f`.
- Actual candidate provider:
  `9bf16b1175befb091d6c61c3ac573121378e1d8c403eed21806a3e8fc80fc946`.
- Exact parent query review remains
  `../encoder-identity-clang2313-20261022/PARENT_REVIEW.md`.

Fresh olddefconfig with current selected tools has **exactly five** differences:
CC text23.1.1→23.1.3, CLANG/AS/LLD230101→230103, and chosen localversion.
No selected hardware, capability, driver, guard, warning, memory, DT, CMA or other
feature symbol changed. The actual source config remains the starting input;
generated compiler/config headers were rebuilt coherently, not relabeled old76.
`CONFIG_TOOLS_SUPPORT_RELR=y` remains unchanged in the successful run.

ARCH=arm, LLVM=1, LLVM_IAS=1, host Clang, fresh `O=`, V=1, `-j4 zImage vmlinux`.
Neutral KBUILD user/host=`qualification`/`offline`, timestamp
`2026-10-22 00:00:00 UTC`, epoch1792627200, version1. Explicit empty LOCALVERSION
and AUTO unset avoid SCM discovery in the inspected version script. No private
host label, disguised23.1.1 pairing, feature workaround or warning suppression.

## Payload custody and exclusions

Retained DTB2571B remains SHA
`d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`.
Retained initramfs2082816B remains SHA
`510e5fdc6e2a7b6ad4a52d82ad1de9381a85e143f7014a6f2cc3bdfb951e309f`.
Their source/build/package association and exact original image header/payload
were inspected before the full build. Config still selects the actual immutable
retained cpio path. Original/new embedded decompressed cpio bytes compare equal;
archive names/types/symlinks were inspected in memory without extraction.

The original174248B qualification `/init` remains SHA
`8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899`.
Qualification/root-only rescue behavior is preserved; no automatic WLAN/media/
retail/cloud startup, credentials/provisioning or monitor source/payload change.

**Embedded `/hgics.ko` is inert original station76 data,163720B/SHA
`e667ed2a1f726c137330638f6ff64eeab085c0bc3247ed2cac1919594e29ef29`. It is NOT
paired to query77; its old release/vermagic is unsupported. Never load/autoload
or claim PLAT-02 cleared from this image.** Original monitor still exposes an
explicit historical `wifi attach` command; its presence is not permission to use
it. No source guard was disabled to hide the stale module.

## Actual new static physical map

Half-open ranges from real new ELF/zImage/header and retained DT/source boot
recipe, not U-Boot's64-MiB banner:

| Extent | Physical range |
|---|---|
| Measured/DT RAM40MiB |`[0x81800000,0x84000000)`|
| Initial page directory |`[0x81804000,0x81808000)`|
| Decompressed Image bytes |`[0x81808000,0x8202ac3c)`|
| Kernel BSS |`[0x8202ac3c,0x82059bc0)`|
| Kernel live through `_end` |`[0x81808000,0x82059bc0)`|
| Entire staged uImage |`[0x82408000,0x82777b6b)`|
| Appended exact DTB |`[0x82777160,0x82777b6b)`|
| Conservative decompressor live end |`0x82790578`|
| Existing volatile media CMA |`[0x82800000,0x83800000)`|

Decompressor bound includes actual compressed ELF BSS/4096B stack,32KiB DTB
growth/workspace and original64KiB malloc space; real nonoverlap means no
self-relocation is needed. Remaining gap below CMA is457352B; entire file is
590997B below original4MiB transfer ceiling and also meets the stricter actual
staging end-before-CMA condition. No staging/decompression/RAM capacity increased.
DT reservation table is empty and unchanged. Actual Image/BSS sizes/physical
translation and compressed `_kernel_bss_size` agree.

This is **static boot-layout fit only**, not live combined memory/DMA/headroom
admission. Compiled CMA default0 is unchanged; future selected media still needs
the separately reviewed volatile16MiB CMA argument. ISP6160KiB/codec8256KiB
requests,128KiB app allocation cap, original backing/FD retention/quarantine and
RP-04 HOLD remain. No fixed-ISP exclusion or new media policy was adopted.

## Evidence, failures and acceptance boundary

`F/analysis/encoder-query77-build02/` retains exact commands/environment/UTC,
unfiltered verbose logs, saved `.cmd` recipe/dependency files, installed
Clang/LLVM/resource/system-header hashes, actual loaded host ELF paths/hashes,
full input/source/build file maps, ELF/symbol/disassembly inspection and receipts.
These are local generated evidence, not Git-stage candidates. Host loader traces
are separate files so capability-probe stderr is not changed. Compiler paths,
linked libclang/LLVM and host generator closure are explicit, not wrapper-only.
This is not syscall-level tracing or an independently reproduced clean build.

The actual provider object is19584B, SHA
`f5491b1c915257c76de7a7708bb5a92f767c0d3c1684e6e45485e163d7ec85d6`.
Final evidence includes1,612 saved Kbuild command files,3,903 dependency-map
source/header paths,3,913 build-file entries and109 path-associated loaded host
ELF entries (aliases are not unique binaries). Full installed resource map has550
entries and the system-header/GCC-support map46,508 files. The actual new compiler
metadata reads `clang version 23.1.3, LLD 23.1.3` and neutral user/host labels.
Supplementary loader transferring-control names cover pathless core utilities
and real host generators, with explicit final hashes; host link support/Python
source metadata is separately inventoried. Precompile-pinned inputs and final
observed-path inventories are distinguished rather than claiming every final
support-file hash was captured before its first use.
The supplementary map resolves all81 observed transferring-control names
(aliases included), with zero unresolved programs, and3,261 additional host
support/Python source files. Host Clang generator links also used installed GNU
ld2.47, explicitly versioned/pinned; the ARM kernel/decompressor use LLD23.1.3.
Standard Kbuild host generators (including CRC tables and certificate/build
metadata) are distinguished from target firmware/generator payload execution.

An initial diagnostic text search matched three **quoted recipes** containing
conditional `warning:`/`error:` strings for atomic-header checksum validation,
not emitted diagnostics. Unfiltered logs retain them and their successful exit;
no compiler error/warning was suppressed. This observation is not a warning/test
acceptance campaign.

`METHOD_CORRECTION01.md` and `METHOD_NOTES.md` preserve author-method failures:
run01's refused tracing-polluted capability probe; metadata collector permission/
deduplication interruptions; Python helper-name collision; initial packager's
incorrect GZIP-kernel assumption. Fresh corrected helper/output names preserve
all old receipts/partial outputs. The actual successful kernel was compiled only
once, without any kernel-source/config correction or resumed partial compilation.

Only host kernel/config/image-format tools and standard host generators ran.
No unit/runtime/security/fuzz/mutation or unchanged-UAPI test suite, target
program/query, UART/RPC/status/network/private I/O, service/listener, boot, camera,
installer/storage operation, P1/module adoption, Git/index/stage/commit or central
plan/AGENTS/provider/model/safeguard change occurred. Parent alone owns hardware;
original radio HOLD/reset-failure HOLD and kernel75/retained76 remain untouched.

**One next dependency:** parent ordinary offline review/admission of this exact
new source/compiler/config/header/image and static layout closure. Only a later
separately selected parent hardware/recovery/sole-owner admission may boot/query
it. No boot/native/full-app/module ABI/G2/G3/G8/product acceptance follows here.

Full source retains Linux COPYING/LICENSES and per-file GPL/SPDX/notices. Authored
builders are GPL-2.0-only. Installed compiler/library metadata does not relicense
or redistribute tool binaries; proprietary retained payloads/generated images
stay local ignored, with publication/license review still required.
