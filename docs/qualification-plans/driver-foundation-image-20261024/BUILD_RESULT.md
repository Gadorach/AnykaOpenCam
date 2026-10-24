# Driver foundation01 — kernel built; verifier FAILED; NO packaged image

2026-10-24 · RP-11/CTRL-02, PLAT-01/02/04/08.
**FAILED_PRESERVED_POSTBUILD_SYMBOL_ASSERTION_NO_PACKAGE.** ONE full built-in
Kbuild compile/link completed exit0. The recipe then failed its own symbol-name
assertion before package/layout checks or UIM creation. No correction/retry.

## Actual attempt and failure

Executed once:

`python3 -u bringup/qc07f/driver-foundation-image/build.py`

The543-line/31232B fixed recipe is SHA256
`952ae8c2a0d8dd0a47b704956179892bd2fbede4830131891c9ca8288d78f52f`.
Its identical consumed copy is `W/inputs/build.py`, where
`W = work/lightbulb-camera/driver-foundation-20261024`.
No old builder/helper was imported/invoked; syntax was checked with AST only
before the actual attempt. Exact16 successful tool argv/environments/UTC/log pins
are in `W/commands.jsonl`, preserved again in BUILD_MANIFEST.json. They comprise
one DT inspection, seven tool-version commands, one olddefconfig, ONE full Kbuild
and six ELF/symbol/code inspections. No modules/install or target/test execution.

Full Kbuild: **17:24:31.568223–17:27:57.621593UTC, exit0**, selecting exactly
`-j4 zImage vmlinux` with baseline ARCH=arm/LLVM=1/LLVM_IAS=1/HOSTCC=clang/
HOSTCXX=clang++/LOCALVERSION=/V=1 and fresh source/O= paths. Generated compiler
metadata says Clang23.1.3/LLD23.1.3, neutral qualification/offline identity. The
baseline deterministic KBUILD timestamp remains Oct22; actual execution is Oct24
as the command records show, not a clock/admission assertion.

At **17:27:58.284426UTC**, build.py:429 raised:

`RuntimeError: actual linked PTZ definitions absent`

This is an **author verification assumption failure**, not a reported compiler or
linker failure: the recipe required an exact unsuffixed `ak_ptz_hold` symbol. Saved
object and linked symbols instead contain `ak_ptz_hold.0`, `.1`, `.3`, `.4`, `.5`,
`.6`, `.7`, `.8`. The exact failing predicate remains unchanged in the frozen
recipe; no relaxed assertion, alternate packager or continuation was used.
Additionally, saved `nm` contains SIX distinct local `state_show` definitions;
the recipe's name-keyed dictionary cannot identify PTZ's one without context.
The saved objdump includes all same-name functions (PTZ's at0xc002984c), not a
unique name-only code binding. Independent review must resolve these evidence
method defects; this report does NOT convert the failed check to PASS.

`W/FAILURE.json` retains traceback/command records; all files and logs remain.
**`W/images/` is empty.** No QC6DF01.UIM exists: no actual UIM size/SHA/CRCs/entry,
decompressor/DTB workspace or whole-package static-fit acceptance. Intended
load0x82408000/entry0x82408040 remain CONTRACT values only. No bootable handback.

## Actual preserved build outputs (not a packaged/qualified image)

All paths below are under `W/build/`; full role pins are in BUILD_MANIFEST.json.

| Output | Bytes | SHA256 |
|---|---:|---|
| `vmlinux` |10704800|`7323778a647aed1fc60875192d39b1ca0cadf7ae55b0ae18493d689d0cf7ea1f`|
| `arch/arm/boot/zImage` |3400736|`8e3d9c9c041a425845b0b57d2dcc09cd4b4eafa7af3a9f33d7e4e2f81ce64114`|
| `arch/arm/boot/Image` |8334396|`b3f2e07cc88e35754b7b88349d789ce3fec06b46e45ddf20174fd110eb7f8751`|
| `arch/arm/mach-ak3918ev200/ptz.o` |8580|`6d06084ca2908a580764787df6b977b85315343b08a074f90d6f0fda1c0b1ae4`|
| `.config` |58848|`0e503122fb7915afead493a6783545b0f3dc71e581da6886ea974b09ab034b48`|
| `usr/initramfs_inc_data` |802447|`ff788cd70012918cb43d7a4c8bcb534ae3a098c432196d5c93ce7632cd1d28b8`|

Release is `6.18.55-qc07f-driver-foundation01`. The real complete PTZ C is7551B,
SHA `a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`.
Its actual Kbuild .cmd names the NEW full source and new generated-header roots;
expanded command flags exactly equal accepted baseline after only the source-root
substitution, with no MODULE define. Actual ptz.o is byte-identical to the prior
independently reviewed built-in changed-object compile; it was freshly compiled,
NOT copied. The board built-in.a command includes ptz.o. Linked symbols/code for
probe/pulse and the split HOLD storage are saved. No modular/native ABI claim.

Raw saved symbols translate `_text` to0x81808000, `_end` to0x82029be0, BSS to
[0x81ffac3c,0x82029be0), and initial page directory to0x81804000. These are raw
ELF observations only; the failed recipe did not reach its static-layout proof.
Do not infer complete decompressor/staging/CMA fit or a physical-memory guarantee.

## Exact source/config/archive custody

- Full accepted baseline source91,270 file/link records and baseline build3,913
  records matched before copying. Fresh copy matched all records, then ONLY
  `arch/arm/mach-ak3918ev200/ptz.c` was replaced byte-exact with reviewed source.
  Original baseline PTZ was5114B/SHA
  `c70829527a939bf3533ec0270fba1f8db4186fb3dc61ff1ba506e1900cda50b6`.
  Entire fresh/original source sets and original build were checked unchanged
  again in read-only failure-handback collection. No source/config repair.
- Exactly TWO config differences: unique LOCALVERSION and new INITRAMFS_SOURCE.
  All compiler identities/flags, CMA/memory, watchdog, console/recovery/boot and
  SOUND-disabled choices preserved. No backend/media/query78/new station code.
- Original DTB2571B/SHA
  `d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`
  is unchanged. Actual board DTB has both PTZ nodes okay/board5/flags0, direct
  provider3, axis0 pins10,11,19,20 and axis1 pins15,14,13,24; no DTS activation or
  GPIO output was added. Prototype source DTS status is not the actual DTB.
- Original archive2082816B/SHA
  `510e5fdc6e2a7b6ad4a52d82ad1de9381a85e143f7014a6f2cc3bdfb951e309f`.
  Fresh `W/inputs/initramfs-driver-foundation01.cpio`1559552B/SHA
  `a0916c00a3a8c85495b2d6db5b34f7009c32ff302764f734b541b5a29cf37219`.
  Exactly hgics.ko and bin/rtspd removed:50→48 records INCLUDING trailer. Every
  retained raw record/header/type/mode/uid/gid/inode/nlink/mtime/body/order matches;
  only terminal zero padding realigned. In-memory path/type/link graph was checked
  before body handling; no extraction, symlink following, credential/config
  payload read/export or target execution. etc is empty in the selected archive.
- Actual new built-in GZIP decompresses to the fresh archive exactly. `/init`
  remains174248B/SHA
  `8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899`.
  Monitor/init/help/wifi-attach bytes unchanged. Missing physical module prevents
  that historical path loading the removed unpaired file; entry unsupported,
  not station adoption or full PLAT-02 closure. No new/rebuilt userspace/server.

Postfailure collection only reads/hashes public metadata and writes fresh report
files: it never imports/calls the recipe or resumes its package phase. It retains
1,612 .cmd pins,3,903 consumed source/header dependency paths,82 actually observed
loaded HOST ELF paths (FINAL-ONLY),58 pre/post unchanged host-tool pins and the
baseline Clang-resource association. Counts overlap, not unique coverage claims.
Full COPYING/LICENSES/per-file notices stayed unchanged in the complete source;
selected license pins are recorded. Original75/76/query77 images/modules and
selected rollback inputs match their preattempt pins. No Git/index operation.

## Unchanged limits / handback gate

Baseline boot still has sensor41 reset/MCLK, radio42 LOW2s/HIGH/USB enumeration,
I2C controller setup and ISP coherent allocations; boot is not physically inert.
Retained monitor mounts filesystems/console and attempts startup watchdog
magic-close without checking its write/close results, then waits for commands.
No automatic motor pulse/homing, sensor stream, firmware/RUN, station/network,
audio or server start. Speaker DEFERRED; Wi-Fi P1/provider restriction unchanged;
modern ADC provider incomplete. No READY, native STOP/current/thermal/zeroing,
strict C1 or128KiB/combined-load acceptance. Uncertain-OFF retention can leave coils
energized indefinitely and does not pin provider binding/electrical function.

ONE attempted build is now frozen **FAILED/unpackaged**. Parent may commission
independent ordinary review of this failure and existing outputs; no repair,
packaging continuation, second build, staging/boot/flash or hardware queue is
selected by this handback. Supervised timing/manual fallback policy is not such
an invocation. All new code/proof needs review and parent disposition before use.
