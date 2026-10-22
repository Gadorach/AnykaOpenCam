# Selected recipe and evidence boundaries

## Input lineage

- Retained full Linux6.18.55 tree:
  `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux`.
  Read `.git/HEAD` as data: upstream pin
  `725bd2f3c81d54edccb66fead01d4c0c222e2231`, plus all retained platform overlays
  and USB76 delta. The full byte manifest, not upstream HEAD alone, defines input.
- Exact actual final output/config/headers:
  `analysis/native-usb76-package01/build` under that foundation directory.
- Original package recipe/manifest: `analysis/native-usb76-package01/`.
  Their old station rebuild/tests/CRC helper are historical only, not invoked.
- Query source seal `ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`;
  patch `9d096489e3ba8cc44556b81feb8192b6c5f08e698c18b2718cdfedbab5aea232`.
  Parent review: `docs/qualification-plans/encoder-identity-clang2313-20261022/PARENT_REVIEW.md`.

## New compiler and config

Use installed Clang/LLVM23.1.3 and integrated assembler, fresh source and `O=`.
Regenerate olddefconfig/config/compiler/generated headers; do not hand-edit
generated23.1.1 metadata. Host Kbuild generators also use Clang. Record every
top-level argv/environment, verbose expanded recipes, saved dependency/command
files, source/header/tool/resource/library hashes and actual logs. Tool closures
describe local installed binaries and do not grant redistribution licenses.

Only CC text/Clang/assembler/LLD version numbers and new qualification localversion
are automatically admitted configuration differences. Other feature/hardware
differences require a stop and parent decision, not incidental fixes.

Neutral build user/host are `qualification`/`offline`; timestamp is
`2026-10-22 00:00:00 UTC`, epoch1792627200, build version1. An explicit empty
LOCALVERSION disables SCM discovery in the audited script with AUTO unset.
No source `.git`, `.scmversion`, localversion file or parent-dirty suffix is used.

## Original RAM packaging format (not boot authority)

The retained76 legacy image has a64-byte big-endian uImage header, magic27051956,
OS5/Linux, architecture2/ARM, type2/kernel, outer compression0/NONE, timestamp0,
load `0x82408000`, entry `0x82408040`. Payload is real zImage followed immediately
by exact retained `attempt45.dtb` (SHA
`d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`).
Header/data CRCs use IEEE/zlib CRC32. Whole-image native transfer integrity uses
CRC-32/BZIP2, not a BZIP2-compressed image. Actual selected kernel compression is
XZ (with appended little-endian32-bit Image size); initramfs compression is GZIP.
The initial GZIP-kernel assumption/packager failure is preserved separately.

Retain byte-exact initramfs76 (SHA
`510e5fdc6e2a7b6ad4a52d82ad1de9381a85e143f7014a6f2cc3bdfb951e309f`),
including original qualification PID1 and original embedded station76 module.
No extraction, payload execution, monitor/provisioning change or autoload is
needed. The module's old vermagic is unsupported by the explicitly new kernel;
it is inert data unless an operator explicitly requests attachment. Do not do so.
No external module rebuild or PLAT-02 clearance belongs to this recipe.

## Per-artifact physical checks required before a new UIM

- Measured RAM/retained DT is40MiB `[0x81800000,0x84000000)`.
- Kernel `_text` is physically `0x81808000`; map virtual symbols by
  `physical = virtual - 0xc0000000 + 0x81800000`. Real Image and BSS/_end must
  remain below staged image start `0x82408000`.
- Original loader allows at most4MiB for the entire UIM. Also require its actual
  end below `0x82800000`, the existing media CMA boundary (stricter than4MiB).
- Read compressed ELF sections, BSS and stack plus source-defined64KiB malloc
  space and appended-DTB expansion scratch (at least32KiB) to bound temporary
  live staging, not merely file bytes. Do not expand staging capacity.
- Preserve DT RAM/reservations, physical offset and the separately reviewed
  volatile media argument `cma=16M@0x82800000-0x83800000`. Compiled default CMA0
  is not changed to silently activate a media policy.
- ISP6160KiB and lazy codec8256KiB remain source allocation requests, not fixed
  proven physical reservations or a combined-load fit result. No placement
  exclusion/admission/memory policy is adopted by this build.

If original format/association/live extents cannot be established without a
source change beyond the query, deliver actual ELF/build outputs and blocker,
not a guessed image. No installer, boot helper, storage/partition operation,
padding, truncation, target query or hardware trial is part of this lane.
