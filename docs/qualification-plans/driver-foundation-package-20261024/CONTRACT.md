# FIRST driver-foundation packaging-only contract

2026-10-24 · RP-11/CTRL-02; PLAT-01/02/04/08. Written before packager/attempt.
Parent explicitly selected this NEW lane after independent build review and
`driver-foundation-image-20261024/PARENT_BUILD_DISPOSITION.md`. No old recipe
repair/resume or build follows. Owned roots ONLY:
`bringup/qc07f/driver-foundation-package/`, this directory, and fresh
`work/lightbulb-camera/driver-foundation-package-20261024/`.

## Fixed inputs and proof boundary

`W = work/lightbulb-camera/driver-foundation-20261024`; all W contents READ-ONLY,
including its EMPTY images directory, source/build/logs/FAILURE/manifest/metadata.
Original image app/docs likewise immutable, including parent disposition. Review
`docs/qualification-plans/driver-foundation-build-review-20261024/REVIEW.md` must
remain SHA256 `592d780b157c9426293f7295d321f5a2bacb728886b4c8600f273d448ee5aeef`.

Use only completed ONE full Kbuild outputs: vmlinux10704800B/SHA
`7323778a647aed1fc60875192d39b1ca0cadf7ae55b0ae18493d689d0cf7ea1f`,
zImage3400736B/SHA
`8e3d9c9c041a425845b0b57d2dcc09cd4b4eafa7af3a9f33d7e4e2f81ce64114`,
Image8334396B/SHA
`b3f2e07cc88e35754b7b88349d789ce3fec06b46e45ddf20174fd110eb7f8751`,
actual PTZ C SHA `a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`
and8580B ptz.o SHA
`6d06084ca2908a580764787df6b977b85315343b08a074f90d6f0fda1c0b1ae4`.
Config58848B SHA `0e503122fb7915afead493a6783545b0f3dc71e581da6886ea974b09ab034b48`;
DTB2571B SHA `d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`;
new cpio1559552B SHA `a0916c00a3a8c85495b2d6db5b34f7009c32ff302764f734b541b5a29cf37219`;
embedded GZIP802447B SHA
`ff788cd70012918cb43d7a4c8bcb534ae3a098c432196d5c93ce7632cd1d28b8`.
Other completed output pins are inherited from the exact held build manifest.

Reuse the accepted indexed95-relocation/seven-section PTZ proof ONLY by exact
unchanged source/object/vmlinux/review/parent pins, clearly labelled REUSED, not
freshly rederived. Fresh raw ELF checks keep every symbol index/STT_FILE unit;
cross-check reviewed probe/pulse/OFF/state/init/driver/sysfs pointers and eight
exact split-HOLD ranges. Never require an unused aggregate/NULL module symbol,
unique local state_show, arbitrary prefix or name-keyed-local dictionary.
Original failed predicate/status stays FAILED_PRESERVED, not a passing author seal.

## Actual new packaging checks

Python standard library only; no external command subprocess, project imports,
old code execution/copy-execution, make/compiler/linker/ELF tools/dtc, target ELF,
helpers/candidates/tests/synthetic cases, network/device/proc/private/hardware/Git.
Before the FIRST actual script invocation, read method/contract and actual indexed
public metadata. AST inspection may parse the authored script without importing it.

Parse raw ARM32 little-endian ELF program/section/symbol tables. For every nonempty
allocated file-backed kernel section, obtain LMA through actual containing PT_LOAD
and compare bytes to Image at its LMA offset; do NOT use section VMA for vectors/
stubs. Verify XZ+LE32 exact Image, compressed allocated sections vs zImage, true
ARM magic/start/end extent and exact compressed piggydata. Bind embedded GZIP to
ELF/Image and decompress to exact cpio. In-memory newc paths/member graph only,
no extraction/credential/config payload read; recheck exact two removals and all48
retained raw records including headers/bodies/order. Preserve original archive.
Config remains exactly two approved symbol differences; full source sole PTZ delta
is REUSED independent review, not another91k-source/toolchain census. DTB same,
actual board5 nodes/pin flags remain as reviewed. No userland/driver/source change.

Use unique defined GLOBAL range anchors and record real indices: kernel _text,
_end,__bss_start,__bss_stop,swapper_pg_dir; compressed _start,_edata,__bss_start,
_end,_kernel_bss_size; also exact LOCAL index11 wont_overwrite and actual .stack.
The actual raw metadata was inspected before authoring the packager. Measured RAM is
[0x81800000,0x84000000), page directory[0x81804000,0x81808000), kernel live
[0x81808000,0x82029be0). Account for appended DTB alignment and head.S334–469:
aligned50%-growth clamped32768..1048576, actual compressed BSS/4096B stack and
64KiB malloc, with explicit conservative upper bound. No self-overwrite/relocation
must be needed; staged image AND decompressor live end stay strictly below
0x82800000. Existing CMA[0x82800000,0x83800000) unchanged. Static layout fit is
not DMA/combined-load/process/allocation-cap/headroom or physical RAM qualification.

Compare old query77 legacy header FORMAT only, not its driver/source/payload pins
as new image evidence. Create ONE `images/QC6DF01.UIM` under the fresh root:
64B legacy header, timestamp0/Linux5/ARM2/kernel2/outer NONE0,
load0x82408000/entry0x82408040, literal name `QC07F driver-foundation01 RAM`,
payload exact held zImage + same appended DTB. No padding/truncation/recompression,
capacity/config/flag change or fallback. Independently reread/decode final bytes,
verify header/data IEEE CRCs, own non-reflected polynomial04c11db7 init/xorout
ffffffff whole-image CRC32/BZIP2, exact size/SHA and suffix/payload association.

## Custody, stop and handback

Bounded before/after public pins cover selected held output/source/config/header/
DT/archive/head.S/licenses, original app/docs and immediate evidence metadata/logs,
review/parent and old-format reference. Original images stays empty. No new huge
dependency framework/full source copy, host tool/library/setting install/change.
N2 remains: three FINAL-ONLY Brotli HOST libraries changed1.2.0→1.2.1; no complete
unchanged toolchain closure, pre-use82-path capture or exact query77 reproduction.
Original notices/source licenses preserved by reference; retained binary output
stays LOCAL ignored evidence, not Git/publication/stock redistribution consent.

FIRST script invocation only. Preserve any verification/package failure and STOP,
no repair/retry/package02/rebuild/check weakening. Hand back actual result, small
manifest/proof/report/delivery pins. New image is NOT BOOTED/native-eligible, not
entry selection/boot admission. Independent ordinary package/layout review and
parent image selection must precede any use. No automatic native queue.

Existing sensor reset/MCLK/radio42 power/USB enumeration/ISP allocations and
monitor watchdog-stop behavior remain baseline boot effects. Speaker DEFERRED,
backend PARKED, Wi-Fi P1 restriction/native ADC gaps unchanged. Uncertain-OFF
retention may energize coils indefinitely; no physical STOP/thermal/provider/
mechanical/zeroing/native ABI/memory acceptance. Supervised detection/manual
fallback is neither strict C1 proof nor present supervision/current ownership.
