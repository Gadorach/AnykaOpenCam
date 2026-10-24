# Independent ordinary driver-foundation PACKAGE/LAYOUT review

2026-10-24 · RP-11/CTRL-02; PLAT-01/02/04/08.

**Narrow conditional actual PACKAGE/CRC/static-layout/paired-source-association
sufficiency. No blocking artifact or packaging-method defect found within this
scope.** N3 below qualifies one reporting field, without changing the valid
conservative bound. Parent disposition/image selection is REQUIRED before use.
NOT BOOTED; native eligibility, boot admission and parent image selection remain
false. This review is not a trust/signature, hardware-readiness or execution GO.

## Scope, records and unchanged custody

Read the complete476-line `bringup/qc07f/driver-foundation-package/package.py`,
actual NOTICES.md, CONTRACT/PACKAGE_RESULT/HANDBACK, complete PACKAGE_MANIFEST/
DELIVERY/PREPACKAGE_PROOF/HELD_PINS/ORIGINAL_GRAPH, prior full build review and
`driver-foundation-image-20261024/PARENT_BUILD_DISPOSITION.md`. Here:

- `W = work/lightbulb-camera/driver-foundation-20261024`, `B = W/build`.
- `P = work/lightbulb-camera/driver-foundation-package-20261024`.
- `D = docs/qualification-plans/driver-foundation-package-20261024`.

Independently checked all6 public delivery pins,8 local delivery pins,76 selected
held-file pins and all7 bounded original evidence graphs against current bytes/
membership. Three consumed recipe/notices/contract copies are byte-identical to
their originals. Public/local PACKAGE_MANIFEST copies agree. P/images contains
only QC6DF01.UIM; original W/images is still EMPTY. No package FAILURE exists.
Original source/object/build outputs, source/config/header/DT/archive/head.S,
selected licenses,16 saved build/inspection logs, metadata, original recipe/failure
and review/parent pins remain unchanged at this bounded scope. Counts overlap.
Author's recorded before/after checks are supported by the frozen method and
manifest; reviewer independently checked current custody, not historical execution.

Saved author invocation is ONE `python3 -I -B .../package.py`,
18:54:52.112578–18:54:52.981375UTC/exit0; zero build invocations/retries. Interpreter
flags are in DELIVERY/result text, not Python sys.argv. Full static recipe review
finds stdlib data processing/new-output writes only, no subprocess/build/target call
or old recipe import/continuation. Reviewer did NOT invoke/import that script.

| Exact record | SHA256 |
|---|---|
| Recipe28555B / consumed copy | `58d83343010936f3d449525d237d454aa873473db1ed0e6c4104926d7abc12a3` |
| P/PREPACKAGE_PROOF.json17384B | `9b9ae0d64f584c4aa89d912a415ae7fd332dd094e7c0e29949b0b266672206ba` |
| P/inputs/HELD_PINS.json14375B | `93e335d590869eb0a502c11b3f2aeb632ed3ee6b5a3157fbb19e9d9178466557` |
| Prior independent build review15992B | `592d780b157c9426293f7295d321f5a2bacb728886b4c8600f273d448ee5aeef` |
| Parent build disposition3447B | `897ea4823f774fb8522ef6fd3a4d5730cdacc5bed3936684fe53f9743fb726b9` |

## Actual final artifact and independently recomputed CRCs

`P/images/QC6DF01.UIM`: **3,403,371B**, SHA256
`d37615e86699ef5d80b159eb690ec130049c4ee023785e9d6d80fc169f543cee`.

Decoded actual64B big-endian legacy header: magic27051956, timestamp0, Linux5,
ARM2, kernel2, outer NONE0, load`0x82408000`, entry`0x82408040`, name exactly
`QC07F driver-foundation01 RAM` with zero tail. Declared payload3,403,307B equals
the entire suffix: held3,400,736B zImage followed by exact2,571B DTB, with no
additional padding/truncation/recompression or substituted baseline image.

- Header CRC32/IEEE, independently zeroing only its CRC word: **c61aedd5**.
- Entire payload CRC32/IEEE: **c09e7c76**.
- Entire UIM CRC32/BZIP2: **f8371c93**, independently computed bit-by-bit,
  non-reflected polynomial04c11db7/initFFFFFFFF/xoroutFFFFFFFF; NOT zlib IEEE.

## Pairing, not a new build or toolchain reproduction

Unchanged source7551B SHA
`a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`,
actual PTZ object8580B SHA
`6d06084ca2908a580764787df6b977b85315343b08a074f90d6f0fda1c0b1ae4`,
and vmlinux10704800B SHA
`7323778a647aed1fc60875192d39b1ca0cadf7ae55b0ae18493d689d0cf7ea1f`
bind this package to the prior reviewed completed Kbuild. Its95 relocation targets/
seven PTZ section associations and complete sole-PTZ source delta are explicitly
REUSED through unchanged pins, not recomputed or another91k census here.

Fresh raw indexed ELF checks agree with all22 recorded PTZ identity/HOLD/register
rows and all six distinct state_show entries. Actual initcall→init→driver→probe/
match and group→attrs→pulse_store/state_show pointers agree; specific state remains
LOCAL index1605/addressc002984c, not a name-keyed last entry. Eight split4B HOLD
fields remain real BSS`[c07fb32c,c07fb34c)`. No unused aggregate/GLOBAL/prefix test.

Independently reconstructed all23 allocated file-backed kernel section/PT_LOAD
LMA associations:8,321,352B match exact8,334,396B Image, including vectors/stubs
whose VMAs differ from LMAs. Remaining13,044B image gaps are zero. All six
compressed associations match3,400,735B of the held3,400,736B zImage; its ONE
existing zero alignment gap is `[3400691,3400692)`, not packaging-added padding.
Compressed .piggydata equals the held XZ+LE32 input; a single complete XZ stream
with no unused suffix decompresses to exact Image and its LE32 length agrees.
ARM magic/start/end agree with actual zImage extent. Held zImage SHA is
`8e3d9c9c041a425845b0b57d2dcc09cd4b4eafa7af3a9f33d7e4e2f81ce64114`.

Embedded802447B GZIP SHA
`ff788cd70012918cb43d7a4c8bcb534ae3a098c432196d5c93ce7632cd1d28b8`
occurs once in vmlinux/Image at6c2c78/6aac78 and decompresses to exact1559552B
cpio SHA`a0916c00a3a8c85495b2d6db5b34f7009c32ff302764f734b541b5a29cf37219`.
Independent safe newc graph/header parsing precedes body comparison: exactly
regular hgics.ko/bin/rtspd removed,50→48 records including trailer; ALL48 retained
raw records/headers/bodies/padding/order identical. Only terminal zeros196→412B.
No extraction, unsafe ancestors/links/hardlink groups or private/config payload;
etc empty. `/init`174248B SHA
`8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899`
unchanged. No new userland or autolaunch. Wi-Fi attach remains unsupported without
the removed unpaired module; no station/P1 adoption or restriction reroute.

Fresh config comparison confirms ONLY LOCALVERSION and INITRAMFS_SOURCE changes
against accepted query77. Actual release6.18.55-qc07f-driver-foundation01,
Clang/LLD23.1.3 identities230103, PTZ=y, SOUND=n, no modular selections; all other
memory/CMA/watchdog/console/boot choices unchanged. Exact appended DTB SHA
`d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`
reuses prior raw decoding:40MiB/no reservations, active board5 axes10,11,19,20 /
15,14,13,24, flags0/direct provider3. Native mux/electrical validity is not inferred.

## Independently derived static map and bound

| Range / limit | Half-open physical interval or upper bound |
|---|---|
| Retained measured/DT RAM | `[81800000,84000000)` |
| Page directory | `[81804000,81808000)` |
| Decompressed Image | `[81808000,81ffac3c)` |
| Kernel BSS | `[81ffac3c,82029be0)` |
| Kernel live | `[81808000,82029be0)` |
| Entire staged UIM | `[82408000,82746e6b)` |
| Exact appended DTB file | `[82746460,82746e6b)` |
| Compressed BSS, UNSHIFTED linked model | `[82746460,82746878)` |
| Compressed stack, UNSHIFTED linked model | `[82746878,82747878)` |
| Conservative decompressor live end | **8275f878** |
| Existing unchanged CMA | `[82800000,83800000)` |

All addresses above hexadecimal. Raw unique defined GLOBAL anchor indices:
kernel _text59734/_end59737/__bss_start59743/__bss_stop59744/swapper_pg_dir59730;
compressed _start305/_edata184/__bss_start180/_end181/_kernel_bss_size187.
Actual wont_overwrite is LOCAL index11/value204. Compressed BSS1048B and4096B
stack agree with sections; _kernel_bss_size2efa4 equals real kernel BSS. No whole
zImage self-relocation is required: BSS correction to decompressed-size check is0,
Image end and kernel live end both precede physical wont_overwrite82408244.
This does NOT mean appended-DTB BSS/stack/GOT adjustment is omitted.

Read actual head.S334–469/569–593/634–638, atags_to_fdt.c130–217 and libfdt
fdt_rw.c426–506. Actual .head.o.cmd fixes TEXT_OFFSET8000/MALLOC_SIZE65536.
head.S temporarily moves stack by aligned50%-growth clamped32KiB..1MiB: here
**32768B**. It RESTORES that temporary shift before adding aligned current DTB
size to the persistent stack/BSS/GOT arrangement. Therefore these shifts are not
summed. For original-size DTB, aligned shift2576B gives model persistent malloc
end82758288. For a folded DTB of aligned size D≤32768, persistent end is at most
82747878+D+65536≤**8275f878**; temporary workspace/stack is also below that bound.
Stage end and this conservative end remain strictly below82800000, leaving
**657,288B** after the latter. No layout/capacity/config limit was raised.

**N3 — reporting precision, not a fit failure:** manifest
`actual_persistent_scratch_end=82758288` is an original-DTB-size MODEL, not an
observed or universal runtime end. atags_to_fdt calls fdt_open_into(total_space),
may change properties and then fdt_pack; head.S rereads totalsize afterward.
DTB size/shift can consequently differ from2571/2576. libfdt bounds it by the
allocated32768B workspace; the conservative bound above still covers it. Parent
must retain this qualification rather than claim an observed fixed persistent
end. Fixed RAM/entry/bootargs/ATAG policy remains a separate entry-admission input;
this is not validation of arbitrary bootloader ATAG contents or full stack use.

Additional source bytes were checked against the accepted full-source map, not
a new source census (atags/fdt_rw additionally match captured dependencies):
atags_to_fdt.c5572B SHA`3052214e2f76ee1e3bc5226b73a2515185f20ef7e0ea279a004d8931d764a193`;
scripts/dtc/libfdt/fdt_rw.c12462B SHA`c11358cc70404fe78ecfbc4eb8485f59fd356af8dea1fc1bf4bdfdad943bc061`;
compressed/Makefile4673B SHA`60ccf9f39b11568cb21f994e9e062cb2cca046cbf60bfd0542bd38e5b3e53685`.

## Preserved failures, conditions and parent handback

Original build recipe remains FAILED_PRESERVED_POSTBUILD_SYMBOL_ASSERTION_NO_PACKAGE,
FAILURE SHA`6005b4c42f1a9decc303b85abd6c7454edf26745a4fbcf6537d6f35c655b5807`.
N1's failed unsuffixed-HOLD assertion is not relabelled by this NEW package result.
N2 remains: three FINAL-ONLY HOST Brotli libraries1.2.0→1.2.1; no unchanged complete
compiler closure/pre-use capture/exact query77 reproduction. No generic toolchain
closure or rebuild was attempted. Original and retained-source license obligations
remain; generated UIM stays LOCAL, not publication/redistribution consent.

Static fit does not prove physical RAM, native ABI, DMA/CMA/combined load, full
stack/headroom,128KiB or process2MiB caps. RAM-only image exceeds1.5MiB NOR KERNEL;
no flash/install/repartition. Boot remains effectful: sensor41 reset/MCLK, radio42
LOW2s/HIGH/USB enumeration, I2C/ISP allocations and unchanged monitor unchecked
startup watchdog magic-close. No new automatic motor/homing/firmware RUN/station/
network/audio/server action; native ADC incomplete. Physical STOP/current/thermal,
mechanics/limits/origin/zeroing, provider lifecycle/recovery/MTU/mic remain open;
uncertain-OFF HOLD may leave coils energized. Prior failures/HOLDs stay unchanged.

Speaker DEFERRED, backend PARKED. Entry24-v2 source review/local1282626 is not
an invocation or current lease. Supervised detect/abort/manual backup is not strict
C1 proof, present supervision, fresh ownership or execution admission. Parent
disposition/image selection and fresh supervised entry/recovery precede any use.
No self-selection, flag promotion or next queue follows this review.

Only this fresh review directory was written. Independent inline data readers
only; no recipe/helper/project import, make/compiler/linker/ELF tool/dtc rerun,
packager/candidate/test/target execution, device/proc/private/UART/RPC/outlet/
network/hardware, Git/index/source/old evidence/report/seal/AGENTS/central-plan
change or subworker. No reviewer reader failure occurred in this package-review
scope. Stop at parent handback.
