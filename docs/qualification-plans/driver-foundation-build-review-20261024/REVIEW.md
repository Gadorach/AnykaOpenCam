# Independent ordinary driver-foundation SOURCE/BUILD review

2026-10-24 · RP-11/CTRL-02; PLAT-01/02/04/08.

**Narrow conditional existing-KERNEL/compiled-PTZ/config/DT/archive association
sufficiency. No kernel-source correction required within that scope.** The ONE
full Kbuild completed; the subsequent author verification failed. Original status
remains **FAILED_PRESERVED_POSTBUILD_SYMBOL_ASSERTION_NO_PACKAGE**. No UIM exists,
so this is NOT package/layout acceptance, image eligibility, adoption or native GO.
Parent disposition is required. N1 resolves the erroneous verification assumption
independently, not by editing or passing the failed recipe. N2 limits closure claims.

## Scope, actual inputs and custody

Read the complete543-line `bringup/qc07f/driver-foundation-image/build.py`, actual
`NOTICES.md`, CONTRACT/BUILD_RESULT/HANDBACK, complete manifest/delivery JSON,
actual235-line PTZ C, its full independent review and parent disposition, accepted
query77 disposition/manifest and relevant driver-first/supervised policy. Here:

- `W = work/lightbulb-camera/driver-foundation-20261024`; `B = W/build`.
- `F = work/lightbulb-camera/foundation-20261011T041148Z`.
- `O = F/artifacts/encoder-query77-source02/linux`.
- `M = F/analysis/encoder-query77-build02`.

Independent public-file reads/hashes/comparisons checked:

| Evidence | Actual check |
|---|---|
| Delivery | All6 payload pins; DELIVERY SHA256 `c9efd2a48f7b06320a87cedcfd825ed567b0373116e3a7e6abc41067b99cd512` |
| Build handback | All17 output pins,20 input/failure metadata pins,16 baseline/rollback input pins,4 baseline map/tool/resource-reference pins |
| Commands | All16 argv/environment/time/log/recipe records agree between commands.jsonl, manifest and FAILURE; every saved exit0/log hash matches |
| Source | Both actual complete91,270 file/link sets, content/modes and85 confined links checked against accepted map; fresh tree differs ONLY by complete PTZ C |
| Baseline preservation | All3,913 baseline build file/link records match; original75/76/query77 selected image/module/archive/DT inputs match their saved pins |
| Kbuild | Exact membership and hashes of1,612 `.cmd` files; independently reconstructed source/dependency set equals all3,903 saved paths/pins/resolved paths |
| Tools | All58 before-tool records equal baseline and current files/resolutions; all550 Clang resource records match;82 loaded HOST paths match current pins and saved loader observations, FINAL-ONLY (N2) |
| Licenses | All4 selected license pins match; complete original COPYING/LICENSES/source notices preserved by the sole-PTZ source delta |

Counts overlap, not unique coverage. No `.git`, `.scmversion` or root localversion
input exists in either source tree. Regenerated PTZ unified patch exactly matches
saved `inputs/ptz.patch`; current source, consumed reviewed copy and candidate are
byte-identical. New `.ptz.o.cmd` first-line flags equal accepted baseline after
ONLY source-root substitution; dependencies use new source/generated headers, not
old source. No `-DMODULE`. That exact compile command occurs in the successful
full Kbuild log, with exactly one PTZ CC record; thin built-in archive independently
contains `ptz.o`8580B among the12 board objects. Makefile/Kconfig select PTZ built-in.
The fresh object also byte-equals the independently reviewed earlier built-in
object; it was compiled in this Kbuild, not substituted from that earlier output.

## N1 — preserved author verification defect, not a failed kernel compile

Recipe/current consumed copy31232B both SHA256
`952ae8c2a0d8dd0a47b704956179892bd2fbede4830131891c9ca8288d78f52f`.
ONE full `make ... -j4 zImage vmlinux` ran17:24:31.568223–17:27:57.621593UTC,
exit0. Saved full log3571244B SHA256
`3aca96ba9d0c4b81b21822c495edcf70fced05580378a69de9b26d252563fd1d`
ends with compressed link/objcopy and normal make exit. Six later ELF inspections
also exited0. At17:27:58.284426UTC, recipe429 requires unsuffixed `ak_ptz_hold`
and raises `RuntimeError: actual linked PTZ definitions absent`. FAILURE25183B SHA256
`6005b4c42f1a9decc303b85abd6c7454edf26745a4fbcf6537d6f35c655b5807`
is unchanged. Recipe239–249 also collapses duplicate local names into one dictionary
entry. Merely accepting a HOLD name prefix or its last `state_show` is inadequate.

### Independent raw ELF proof, not name-only substitution

Parsed ELF32 little-endian ARM section/symbol/REL tables directly as data, keeping
every symbol index and STT_FILE unit. No ELF inspection tool was rerun. Relevant
object symbols are LOCAL; no GLOBAL requirement is appropriate:

| PTZ object index / section offset / bytes | Actual linked index / address / bytes |
|---|---|
|12 `.text+0` /236 probe |1586 `0xc0028eb0` /236 |
|16 `.text+0xec` /2104 pulse |1590 `0xc0028f9c` /2104 |
|18 `.text+0x924` /120 OFF |1592 `0xc00297d4` /120 |
|31 `.text+0x99c` /220 state |1605 `0xc002984c` /220 |

Linked STT_FILE index1578 is `ptz.c`. Six distinct local `state_show` entries are
1517/isp.c/`c00280ec`, **1605/ptz.c/`c002984c`**,24658/port.c/`c02dd5fc`,
25562/watchdog_dev.c/`c02f3b78`,26259/extcon.c/`c03006dc`,39930/core.c/`c052defc`.
The PTZ identity is additionally bound by actual bytes and callback roots below,
not just STT_FILE order or the saved disassembler's address annotation.

| Object section | Linked base | Bytes / declared relocations independently checked |
|---|---|---|
| `.text` | `0xc0028eb0` |2680 /78 |
| `.init.text` | `0xc068f254` |16 /2 |
| `.exit.text` | `0xc06ab36c` |12 /2 |
| `.data` | `0xc077ef90` |160 /11 |
| `.rodata` | `0xc0565528` |416 /1 |
| `.rodata.cst4` | `0xc062d248` |4 /0 |
| `.initcall6.init` | `0xc06b2ae4` |4 /1 |

All seven sections match input-object bytes after normalization of ONLY their95
declared relocation fields: ABS32 words and CALL/JUMP24 displacement bits. Branch
opcodes/other instruction bits remain equal. Independently resolved EVERY branch
target to the actual linked symbol and EVERY ABS32 to the exact mapped object
section/addend or byte-identical referenced NUL-terminated string (string pooling
allowed). No arbitrary changed bytes were masked. Normalized `.text` SHA256 is
`17494bcf16f40a846534a01dcb12c2cf0a4c2e8ab09e85fdaacf4a57c6b5d845`.

Positive roots: initcall index1607 at`c06b2ae4` points to init`c068f254`, whose
literal passes actual driver`c077ef90` to `__platform_driver_register` with NULL
module. Driver probe word is`c0028eb0`; match-table pointer is`c0565528`, containing
`anyka,qc07f-ptz-diagnostic` and its zero terminator. Probe's group literal is
`c05656b0`; group attrs pointer is`c077eff8`, containing pulse`c077f004`,
state`c077f020`,NULL. Pulse's store pointer is`c0028f9c`; state's show pointer is
**`c002984c`**, not another `state_show`. Attribute name references resolve to
`pulse`/`state`; state-code format reference exactly matches source scalar HOLD
status text. Phase bytes at`c062d248` remain03,06,0c,09. Probe contains property,
count, allocation and group calls, no GPIO request/direction/set calls.

Actual split BSS is32B starting`c07fb32c`: `.0`=lines/latch (+0), `.1`=device
(+4), `.3`=axis (+8), `.4`=attempted (+12), `.5`=initialized (+16), `.6`=issued
(+20), `.7`=phase (+24), `.8`=error (+28); all8 exact symbol sizes/ranges match.
Object `.text+8f8/904/908/90c/910/914/918/91c` literals resolve to these exact
storage words; state literals`a58..a70` read the same latch/scalars. Saved code,
independently byte-bound above, loads/tests the actual lines pointer under the
shared mutex at`text+160..180`, before refs/acquisition/effects. Uncertain cleanup
at`text+54c..5a4` stores the actual array/device and scalar history, sets retained,
bypasses array release at`520` and device/module puts at`5f0/5f8` via`5e4/5e8`.
Normal known-low/acquisition-error paths retain their respective puts. The NULL
built-in module field is optimized away: absent `.2`/unsuffixed whole-struct
symbols do NOT imply a put or dropped ownership. Actual `.1` device storage is
present. Referenced gpiolib/device/sysfs lifetime contracts are the exact unchanged
source inputs reached by the prior full PTZ review, not a new modular ABI claim.

Thus N1 is an unsupported optimizer/name-identity assumption in the verifier;
no source/object/linked PTZ mismatch was found. Original failed predicate/status
is NOT repaired or relabelled.

## Exact retained outputs and selection

| Actual output | Bytes | SHA256 |
|---|---:|---|
| PTZ C |7551|`a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`|
| `B/arch/arm/mach-ak3918ev200/ptz.o` |8580|`6d06084ca2908a580764787df6b977b85315343b08a074f90d6f0fda1c0b1ae4`|
| `B/vmlinux` |10704800|`7323778a647aed1fc60875192d39b1ca0cadf7ae55b0ae18493d689d0cf7ea1f`|
| `B/arch/arm/boot/zImage` |3400736|`8e3d9c9c041a425845b0b57d2dcc09cd4b4eafa7af3a9f33d7e4e2f81ce64114`|
| `B/arch/arm/boot/Image` |8334396|`b3f2e07cc88e35754b7b88349d789ce3fec06b46e45ddf20174fd110eb7f8751`|
| `B/.config` |58848|`0e503122fb7915afead493a6783545b0f3dc71e581da6886ea974b09ab034b48`|

Exactly TWO config-symbol changes: LOCALVERSION to`-qc07f-driver-foundation01`
and INITRAMFS_SOURCE to the new archive; generated autoconf has exactly these two
string changes too. Actual release`6.18.55-qc07f-driver-foundation01`; generated
compiler text Clang/LLD23.1.3; assembler/Clang/LLD230103. Actual flags unchanged,
PTZ=y, SOUND=n, no modular selections, all memory/CMA/watchdog/console/boot choices
unchanged. No audio provider/backend/query78 or station-source adoption.

Raw actual DTB2571B SHA256
`d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`
equals baseline. Independently decoded actual nodes: both okay/board5,
axis0 pins10,11,19,20; axis1 pins15,14,13,24; flags0/provider3, direct Anyka GPIO
provider okay/two cells. No DT reservations/reserved-memory; RAM40MiB unchanged.
Disabled prototype source DTS is not this DTB. Native mux/electrical validity is
still unproved. Saved dtc's three simple_bus_reg warnings remain visible.

Independently parsed newc names/types/extents/link graph before bodies; no
extraction/execution. Only allowed public regular payloads; etc empty, no unsafe
ancestors, hardlink groups or dangling applet links. Original2082816B SHA256
`510e5fdc6e2a7b6ad4a52d82ad1de9381a85e143f7014a6f2cc3bdfb951e309f`
becomes1559552B SHA256
`a0916c00a3a8c85495b2d6db5b34f7009c32ff302764f734b541b5a29cf37219`.
Exactly regular `hgics.ko` and `bin/rtspd` removed,50→48 records including trailer;
all48 retained raw records/header/body/padding/order byte-equal. Only terminal
zero padding changes196→412B. Embedded GZIP802447B SHA256
`ff788cd70012918cb43d7a4c8bcb534ae3a098c432196d5c93ce7632cd1d28b8`
decompresses exactly to the new archive; exact GZIP occurs once in vmlinux
(file`0x6c2c78`) and Image (`0x6aac78`). `/init`174248B SHA256
`8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899`
is byte-unchanged, not rebuilt/requalified here. Historical wifi-attach text does
not restore the physically absent unpaired module: station remains unsupported,
P1 restriction unchanged/not rerouted. No secrets/captures/credential digests read.

## N2 — HOST dependency delta; limited closure, not exact reproduction

All82 FINAL-ONLY loader paths were independently rederived from saved traces and
their current pins checked. Of these,79 records equal accepted query77 loader
records. THREE differ: `/usr/lib/libbrotlicommon.so.1`, `libbrotlidec.so.1`,
`libbrotlienc.so.1` now resolve to1.2.1 rather than1.2.0, respectively current
SHA256 `431ba8ac8e33307329ef575c13d2cff77262cd7e9c8f3b56a18c6fe667abf0e8`,
`d8b1e6da218c888d4396fafe35372c7cce28b4e8fa579c6f352a89316cd780c3`,
`038bd0bdea5c1888a2e83e1ddaed053779805bd2bc9b63e1b2caf19606b9fd1e`.
The58 tool and550 resource pins really do match; they are not the entire loaded
HOST closure. No pre-use identity for these82 FINAL-ONLY paths is independently
established. Do NOT claim unchanged complete compiler closure or query77 build
reproduction. This does not contradict the exact emitted PTZ object/link/source
binding above or require weaker flags, but remains an explicit provenance limit
for parent disposition. No library installation/reconfiguration was performed.

## Raw range inputs versus unreached package/layout gate

Independent raw ELF observations: page directory`81804000`, `_text` physical
`81808000`, BSS`[81ffac3c,82029be0)`, `_end` physical`82029be0`. All23 allocated
file-backed kernel sections (8321352B) match Image through actual PT_LOAD LMAs,
including vectors/stubs with intentionally different VMAs. Existing XZ+LE32
decompresses to the exact8334396B Image. Compressed ELF's six allocated file-backed
sections match zImage; magic/extent agree, `_start=0`, `_edata=33e420`,
`__bss_start=33e420`, `_end=33e838`, `.stack=[33e838,33f838)`,
`_kernel_bss_size=2efa4`, `wont_overwrite=204`. These are association/range INPUTS,
not a new whole-package static-fit result or physical-memory proof.

Read the frozen packaging tail as METHOD only: legacy64B IEEE-header/payload CRC,
new zImage+same DTB, load`82408000`/entry`82408040`, timestamp0/Linux5/ARM2/kernel2/
outer NONE0 and whole-image CRC32/BZIP2. Actual old query77 header/payload agrees
with that baseline format. `head.S:334–469` requires appended-DTB relocation,
aligned50%-growth workspace clamped32KiB..1MiB, BSS overlap/stack and64KiB malloc
accounting against retained40MiB map/staging split`82800000` and existing CMA
`[82800000,83800000)`. The original recipe never reached this tail. `W/images`
is EMPTY; `QC6DF01.UIM` absent. No new UIM size/hash/header/CRC/layout PASS exists.

## Separate gates and handback

Boot is NOT inert: unchanged sensor-bootstrap GPIO41 reset/MCLK, radio GPIO42
LOW2s/HIGH and USB enumeration, I2C setup and ISP coherent allocations remain.
Unchanged monitor startup mounts filesystems/opens console, attempts unchecked
watchdog magic-close, then command-waits; no newly added automatic motor/sensor
stream/firmware RUN/station/audio/server start. This review does not requalify
that inherited bootstrap. HOLD may leave coils energized indefinitely; descriptors/
device/code retention does not preserve provider binding/electrical function.
Physical STOP/current/thermal/deadline, mechanics/sign/limits/origin/zeroing,
MTU1500/mic,128KiB/process2MiB/combined-load/CMA, recovery and native ABI remain
open. Speaker DEFERRED; backend PARKED. No automatic homing or guessed limits.
Supervised detect/abort/manual backup is not strict C1 proof, present supervision
or current entry admission. Historical23h OFF is not a fresh target observation.

If parent accepts this narrow disposition, it may EXPLICITLY commission a FIRST
fresh **packaging-only** derivative over these pinned completed outputs, without
make/compiler/linker/rebuild or failed-recipe mutation/import/continuation. That
derivative must use real indexed PTZ identity/relocation proof, preserve N1/N2,
and independently establish actual new package/layout/CRC evidence. It is a
recommendation, not authorization or a queued task. Package review/parent image
selection and fresh supervised entry/sole ownership/recovery precede any effects.

Only this fresh review directory was written. No project recipe/helper import,
candidate/test/target function, compiler/linker/ELF tool/dtc rerun, packaging,
device/proc/private/UART/RPC/outlet/network/hardware, Git/index/source/evidence/seal/
central-plan/AGENTS change or subworker. Review-only inspection history remains:
one broad read/hash command timed out120s after completed source traversals;
remaining checks completed in separate read-only scopes. One extra Image reader
wrongly used VMA for vectors and failed; corrected independent inspection used
raw PT_LOAD LMAs without changing artifacts. An all82-baseline-loader-equal
inspection assertion failed; N2 above reports the actual three deltas, not PASS.
None was a build/recipe/candidate retry or an altered original failure. Stop at
parent handback; no next execution/source queue selected.
