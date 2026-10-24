# Driver foundation01 — FIRST packaging-only PASS, NOT BOOTED

2026-10-24 · RP-11/CTRL-02; PLAT-01/02/04/08.
**ONE actual UIM created from the reviewed completed build; independent ordinary
package/layout review and parent image selection REQUIRED before use.**
No build/compiler/linker/ELF tool, old helper/recipe, target/test or hardware call.

## Actual artifact and invocation

`work/lightbulb-camera/driver-foundation-package-20261024/images/QC6DF01.UIM`

- **3,403,371B**, SHA256
  `d37615e86699ef5d80b159eb690ec130049c4ee023785e9d6d80fc169f543cee`.
- Whole-image CRC32/**BZIP2 `f8371c93`** (non-reflected04c11db7, init/xoroutffffffff).
  Header IEEE CRC32 **`c61aedd5`**; data IEEE CRC32 **`c09e7c76`**.
- Legacy64B header, timestamp0/Linux5/ARM2/kernel2/outer compression NONE0;
  load`0x82408000`, entry`0x82408040`, payload3,403,307B.
  Name exactly `QC07F driver-foundation01 RAM`.
- Payload is exact held3,400,736B zImage plus exact retained2,571B DTB.
  No new padding/truncation/recompression or ceiling/config change.
- Release **`6.18.55-qc07f-driver-foundation01`**. Image is NOT BOOTED,
  native_eligible=false, boot_admission=false, parent_image_selection=false.

FIRST and ONLY actual invocation:

`python3 -I -B bringup/qc07f/driver-foundation-package/package.py`

Actual18:54:52.112578–18:54:52.981375UTC, exit0. Python3.14.8 stdlib only. Generated
manifest's argv field is Python's executable/sys.argv (interpreter `-I -B` flags
are recorded here, not present in sys.argv). Recipe476 lines/28555B, SHA256
`58d83343010936f3d449525d237d454aa873473db1ed0e6c4104926d7abc12a3`.
No actual package/verification failure, retry, package02 or rebuild occurred.

## Fresh byte/layout validation versus reused build proof

Let `W = work/lightbulb-camera/driver-foundation-20261024` (READ-ONLY),
`P = work/lightbulb-camera/driver-foundation-package-20261024`.

**Fresh:**76 selected file pins before/after; original app/docs, immediate evidence
graphs/logs/metadata,17 completed outputs and selected source/header/license inputs.
Original W/images remains empty; original FAILURE/source/build/recipe/docs remain
unchanged. Final artifact was reread and independently decoded against encoded
header/payload expectations with both IEEE CRCs and actual suffix equality.
Only the fresh P/images directory contains this new UIM.

**Reused by exact source/object/vmlinux/review pins:** ALL95 PTZ relocation targets
across seven section associations, full sole-PTZ source delta, active board5 DTB
lookup and detailed original build custody from independent REVIEW SHA256
`592d780b157c9426293f7295d321f5a2bacb728886b4c8600f273d448ee5aeef`
and its parent build disposition. These were NOT freshly rederived in full here.
No91k-source copy/census or250k toolchain framework was repeated.

Fresh indexed raw ELF cross-checks bind STT_FILE1578 `ptz.c`, probe1586,
pulse1590, OFF1592, specific state1605 among six local state_show entries, init1579,
driver1582, initcall1607, group1589, attrs1608 and callbacks1609/1610. Actual raw
initcall→init instructions/literal/branch→driver→probe/match and attrs→specific
show/store pointers agree with the review. Eight exact4B split-HOLD symbols occupy
`[0xc07fb32c,0xc07fb34c)` in real BSS; phases remain03,06,0c,09. No prefix predicate,
unused aggregate/module symbol or name-keyed-local dictionary. Recorded `unit`
means preceding STT_FILE; it is not ownership proof for GLOBAL anchors.

All23 nonempty allocated file-backed kernel sections (8,321,352B) equal the exact
Image through their actual PT_LOAD **LMAs**. In particular vectors/stubs are not
mapped using their high runtime VMAs. All six compressed file-backed sections
(3,400,735B; the held zImage also has one existing alignment-gap byte) equal the
zImage. Exact compressed .piggydata equals the held XZ+LE32 bytes, and a complete
single XZ stream decompresses to the held8,334,396B Image with exact LE32 size.
Actual ARM zImage magic at0x24/start0/end0x33e420 agree with real file extent.
No alignment bytes were added or changed by packaging.

Embedded GZIP802447B/SHA
`ff788cd70012918cb43d7a4c8bcb534ae3a098c432196d5c93ce7632cd1d28b8`
occurs exactly once in vmlinux/Image, at file offsets0x6c2c78/0x6aac78, and
decompresses to exact new cpio1559552B/SHA
`a0916c00a3a8c85495b2d6db5b34f7009c32ff302764f734b541b5a29cf37219`.
Fresh safe in-memory newc graph inspection and raw-record comparison establish
exact hgics.ko/bin/rtspd removal and all48 retained records including trailer,
types/modes/owners/links/timestamps/body/order unchanged; only historical terminal
zero padding differs196→412B. No extraction/private/config payload or ELF execution.
Exact retained `/init`/help/wifi-attach bytes are unchanged; missing hgics.ko remains
unsupported, not P1/station adoption. No new userland/auto-launch/server/audio.

Config58848B/SHA `0e503122fb7915afead493a6783545b0f3dc71e581da6886ea974b09ab034b48`
freshly compares to baseline: ONLY LOCALVERSION and INITRAMFS_SOURCE differ, as
reviewed. PTZ built-in, SOUND disabled; memory/CMA/watchdog/console/recovery and
compiler choices unchanged. Actual complete PTZ C SHA
`a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`,
ptz.o8580B/SHA `6d06084ca2908a580764787df6b977b85315343b08a074f90d6f0fda1c0b1ae4`;
vmlinux10704800B/SHA
`7323778a647aed1fc60875192d39b1ca0cadf7ae55b0ae18493d689d0cf7ea1f`.
DTB2571B/SHA `d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`
is the exact final suffix. Actual active board5 mappings/flags0/provider3 and40MiB/
empty DT reservations are reused from the exact-pinned independent review.

## Actual static map and conservative decompressor bound

| Extent | Half-open physical range / bound |
|---|---|
| Measured/DT RAM | `[0x81800000,0x84000000)` |
| Initial page directory | `[0x81804000,0x81808000)` |
| Image file | `[0x81808000,0x81ffac3c)` |
| Kernel BSS | `[0x81ffac3c,0x82029be0)` |
| Kernel live | `[0x81808000,0x82029be0)` |
| Entire staged new UIM | `[0x82408000,0x82746e6b)` |
| Appended DTB | `[0x82746460,0x82746e6b)` |
| Conservative decompressor live end | `0x8275f878` |
| Unchanged existing CMA | `[0x82800000,0x83800000)` |

Unique defined GLOBAL kernel anchor indices are _text59734, _end59737,
__bss_start59743, __bss_stop59744, swapper_pg_dir59730. Compressed GLOBAL indices:
_start305, _edata184, __bss_start180, _end181, _kernel_bss_size187; actual
wont_overwrite is LOCAL index11/value0x204, not an invented GLOBAL requirement.

Real compressed BSS1048B and stack4096B give **unshifted** linked-address ranges
`[0x82746460,0x82746878)` and `[0x82746878,0x82747878)`. These are the manifest's
`compressed_BSS`/`compressed_stack` fields, NOT simultaneously occupied runtime
ranges overlapping the appended DTB. Actual head.S438–449/569–593 moves BSS/stack
past the aligned2576B DTB. Persistent scratch/malloc static end is0x82758288.
ATAG merge temporarily needs aligned50%-growth clamped32768..1048576, here32768B.
Conservative bound is unshifted stack end0x82747878 +32768 +65536 malloc =
**0x8275f878**, covering both that temporary growth and persistent aligned shift.
Actual source and completed .head.o.cmd retain MALLOC_SIZE65536. The real kernel
BSS size agrees with compressed _kernel_bss_size0x2efa4. Output/self-overwrite
checks establish no whole-zImage self-relocation is needed; ordinary appended-DTB
BSS/stack/GOT pointer adjustment still occurs. The manifest's relocation_needed
false refers to whole-zImage self-relocation, not denial of these adjustments.

Both staged-file and conservative-live end are strictly below0x82800000; remaining
gap after the conservative bound is **657,288B**. No RAM/staging/CMA ceiling changed.
This is STATIC layout fit, not measured memory, live stack/DMA, combined-load,
128KiB/process2MiB cap/headroom or native ABI qualification. Old query77 header was
used for FORMAT only, never substituted as new driver/image evidence.

## Preserved limits and next boundary

Original build status remains FAILED_PRESERVED_POSTBUILD_SYMBOL_ASSERTION_NO_PACKAGE;
the independent N1 resolution and NEW packaging result do not pass its failed seal.
N2 persists: three Brotli HOST libraries changed1.2.0→1.2.1 in82 FINAL-ONLY records.
No complete unchanged toolchain closure/pre-use capture/exact query77 reproduction.
Original notices/licenses preserved by reference and selected unchanged pins;
generated/retained binaries stay LOCAL, not redistribution/publication candidates.

Baseline sensor41 reset/MCLK, radio42 power cycle/USB enumeration, I2C setup, ISP
allocations and monitor unchecked watchdog magic-close remain boot effects. Boot
is not inert. No new motor/homing/radio firmware/RUN/station/network/media/audio/
server action. Speaker DEFERRED; backend PARKED; P1/native ADC gates unchanged.
Physical STOP/coil current/thermal/mechanics/origin/zeroing/provider lifetime,
memory/native readiness and strict C1 remain unqualified. User supervised timing/
manual fallback policy is not current supervision or boot admission.

Handback stops at independent ordinary package/layout review, then **parent-only
image selection** and any separately admitted supervised entry. No selection file,
controller invocation, ON/boot/flash/module load or automatic native queue here.
