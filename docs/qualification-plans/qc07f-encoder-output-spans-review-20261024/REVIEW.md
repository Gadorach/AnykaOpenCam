# Independent ordinary review — encoder output spans

2026-10-24. B07 / RP-04/05/09; CAP-01/05/06/07 + ENC-03/05 dependency.

**Narrow conditional SOURCE/API/bounds/ownership/order sufficiency. No ordinary
blocking source defect found.** This is not parent adoption, runtime confirmation,
native/image ABI pairing, allocation-cap acceptance or product GO. No source or
custody blocker remains. N1 below records a reconciled aggregate-ordering detail,
not a changed file or a requested source correction.

## Scope and identities

`A` = `apps/qualification/qc07f-encoder-output-spans/`;
`D` = `docs/qualification-plans/qc07f-encoder-output-spans-20261024/`.
Read all eight A files, contract/handback/failure disposition and five JSON
selection/input/delta/verification/delivery records; inspected both saved reports,
logs, patches, dependency lists and ELF bytes. Read the actual seven-file accepted
query baseline at `docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate/`
and prior `encoder-identity-clang2313-20261022/PARENT_REVIEW.md`.

SHA256, independently matched to current bytes:

| A file | SHA256 |
|---|---|
| qc07f_hwenc.c (536 lines, 26761B) | `36bc80fadc0a16ded4525f24f805022faf22ee0ac8cb77cdfa4b390ba23e6b50` |
| qc07f_hwenc.h | `f2c651e522e7b5444a26c4c8aec05a10e483ee0e1fb41a655219ddf91d3f63e7` |
| encoder-output-spans.h | `4267d4074177468898937926b35029fbfb2b392c25ea987832c83137841cb6e6` |
| encoder-output-spans.inc | `0a49fd1ce8cf1962f747df0f05db66b7614570885a39f1515780a5f560f7a415` |
| abi_compile.c | `031c5337f231ad38ed8cb6bac7f43ee4816a3f3da72fe173193b680c8979b656` |
| build.py | `ea4391963e1f49ea40358ad2a2c0b42611c427e5a9496e1066de6b8b9eca0fc9` |
| README.md | `2f497b4a30cd3032289dee5fd6515d3c0854f7cfcbd6f8b59399a01ae0761f6e` |
| LICENSE_NOTICES.md | `6754bf952279ad4738421f6ee849e3cd7e0b717eeee22f3c06351be2b4dcaa37` |

Eight-file aggregate `1519d8d6c998ac2f4a33961d7d9d88597135a87254ed59d9f2e9716340113dc0`;
21 delivery-pin aggregate `19d75f3598eb36d917b935c1c702b95b86730a3f9aa197b8003767c6350479fe`.
Both reproduce using sorted workspace-relative UTF-8 path + NUL + raw SHA256 bytes.
Baseline driver `9bf16b1175befb091d6c61c3ac573121378e1d8c403eed21806a3e8fc80fc946`;
prior source-seal identity `ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`
also reproduces. All seven baseline pins and twelve prior seal records match.

| D evidence | SHA256 |
|---|---|
| runs/build01/REPORT.json | `2f1740eec954ee8cfb512a0e5e31ca576e2476d03bd14fa3933ab4939b17bcef` |
| runs/build02/REPORT.json | `065e38cd805575ae918464929e70f737d8a41bb880ccc6709892f445d55f6538` |
| SOURCE_DELTA.json | `7008b010a686dcd4eafefdf376711518d25cf28f6662604c6f273b9ca999cd6d` |
| BUILD02_SELECTION.json | `9d56f3ffe1b97ca37cb9cc0c60d0e5c0a94e0d0edad3c9460ccc64a8c70548e4` |
| AUTHOR_FINAL_VERIFICATION.json | `173d261ee7a8329e80a89f925a2c211624cafc46c3f72bc293c89acfc491a302` |
| DELIVERY_PINS.json | `224ea40a30a476b68fc335ce5307b5c7e31b55387d9865351a54044db675c53a` |

## Ordinary source findings

- **Actual dispatch/custody:** `qc07f_hwenc.c:371–458` recognizes the exact full
  new command under the inherited mutex, quarantine gate, calling-file numeric
  session, stage accounting and other-file-owner BUSY gate. Snapshot failure and
  ownerless-P ENODATA never enter the encode body or clear the old owner. Open,
  release and provider publication remain unchanged. VFS keeps the calling file
  alive during ioctl; final release takes the same mutex and clears a matching
  owner before freeing session data (`:460–490`). No second provider/retained
  userspace descriptor pointer or asynchronous lifetime was introduced.
- **Fixed ABI/preflight:** the 128-byte, alignment4, all-u32 request encodes
  `0xc0805167`; eight 8-byte descriptors start at32, outputs at96/100/104 and
  five reserved words at108. `encoder-output-spans.inc:12–48` snapshots the entire
  control/descriptors once, validates every specified scalar/zero field, exact
  1280x720/1382400-byte input and eight exact131072-byte capacities. Nonzero,
  conservative u32-endpoint checks precede full-range access_ok; widened u64
  half-open comparisons cover every output pair and full input/control overlap.
  At most eight outer and 28 pair comparisons; no user count controls stack size.
- **One real codec path:** typed contiguous/spans destinations reach ONE
  `encode_h264` (`:268–370`). Old STREAM/IDR retain real contiguous pointer/capacity
  validation and the original single-copy semantics. The spans frame's zero
  contiguous-output fields are genuinely unused, not fake validation placeholders.
  The real identity/capability, prepare/allocation, input-copy, QP26/register,
  reference/aux, poll/stop/reset-retirement, dma_rmb, every original guard and
  nonzero byte-aligned length <=1MiB sequence remains ahead of scatter selection.
  Reset failure skips copying; no new reset retry/free/uncertainty clear exists.
- **Bounds/copy:** `encoder-output-spans.inc:51–67` reads actual `enc.output`
  at cumulative offset. With admitted bytes <=1048576 and fixed total capacity,
  each chunk is <=131072, reads stay within valid output and the last chunk writes
  only valid bytes, not unused tails. Linux uaccess's residual in [0,chunk] makes
  `chunk-left` the exact copied prefix; first nonzero residual returns EFAULT
  immediately. One sample reports actual cumulative copied bytes/result. No
  full-NAL allocation, variable stack array, recursion or new large stack object:
  request128B + frame32B + small typed destination/scalars; codec regs remain256B.
  This is not a measured total kernel-stack budget or finite uaccess/mutex wait.
- **Commit/failure ordering:** only successful data copy populates metadata;
  only complete control copy0 commits owner/frame_num/ref_index (`:417–424`).
  IDR num0/ref0, P 16-bit frame wrap and alternating refs match the old route.
  Every failure after entering this encode call clears the new route's logical
  reference, including quarantine, without clearing fault_session/quarantine/
  DMA uncertainty/backing. Another file cannot invalidate an owner's reference
  through this branch. Legacy quarantine-failure quirks remain, not blanket-fixed.
  Stage delivery increments only on final result0. Hardware last_bytes history,
  partial data and even complete-looking partial metadata are NOT valid delivery
  or retirement; all result fields are valid ONLY with syscall0.

Literal forward AND reverse reconstruction (seven C groups, one header group)
matches the actual baseline/candidate exactly. Both runs' forward/inverse patches
regenerate byte-exact. Pre-H.264/JPEG/DMA/reset/allocation prefix, hardware core
through final length/stamp, query block and release/open/init suffix compare
unchanged. Five support files in each consumed snapshot equal the accepted
baseline; all eight current A files equal build02's consumed copies. Old exact
commands, license notices and built-in/static-DT/no-PM/no-kexec guards survive.

## Saved compile evidence, independently inspected, not rerun

- build01 stays **FAILED_PRESERVED**: four successful metadata/dependency commands,
  one failed HOST ABI compile; no driver compile. All31 files exactly match saved
  selection membership/pins. 77/78 current input records match; the sole drift is
  A/abi_compile.c, exactly the documented stage assertions304→296 and
  `0xc1305165`→`0xc1285165`. Nine-file driver snapshots and recipe unchanged between runs.
- build02 saved **PASS / NOT RUN**, 10:02:54–10:02:56Z: twelve commands = three
  version reads, three dependency preprocesses, three C compiles (HOST ABI, musl
  ARM ABI, whole ARM driver) and three ELF metadata reads. All exits0/stderr empty.
  All603 input pins, 555 overlapping dependency records (21/19/515), exact pre/post
  dependency membership,24 log pins,five artifacts and four tool hashes/resolved
  paths match. Exact52-file membership matches the handback;20 reference pins and
  three actual userspace-connection reference pins also match.
- Byte-only ELF inspection confirms HOST ELF64 x86-64 REL and two ELF32 ARM EABI5
  REL objects, entry0/no program headers. Both ABI objects contain the same eight
  exact old/new command words. Whole-driver21668B SHA256
  `c9a39012806ab0bc481a906b780d19970716ee0b9b49d4d792841ea658b8f40f`;
  actual defined ioctl/open/release/init, snapshot/scatter/retirement and ONE
  encode_h264 symbols are present. This is not execution or independently rebuilt
  object evidence.
- Real board Makefile selects `qc07f_hwenc.o` via
  CONFIG_QC07F_HWENC_RETIREMENT_BUILTIN=y. Saved driver argv matches the actual
  `.qc07f_hwenc.o.cmd` flags after the documented path/dependency/output changes;
  no guard bypass added. Retained generated/config metadata says23.1.1, while saved
  compiler version is explicit NEW Clang23.1.3. Exact source/app matching and input
  pins do NOT establish all loaded compiler dependencies, full kernel/module/image
  build or native ABI pairing. Build timestamps are not whole-interval clock authority.

## N1 — nonblocking aggregate ordering precision, reconciled

AUTHOR_FINAL_VERIFICATION's protected96 corpus reproduces its recorded
`b88eb1a37b9988bbb810d8e97258be4d04361f19c167ff8859921b5136bf3368`
using Python Path component ordering. Lexicographically sorting complete relative
path strings instead yields
`f1fb90612417029c1b33f782c6cca0e597335040808df1fc8907d4ba3619dc14`.
For example, Path ordering places source/ children before source.patch, whereas
complete-string ordering places source.patch first. The initial static check
assumed complete-string ordering and stopped; this was an ordering-assumption
mismatch, not candidate execution or an individual pin failure. The author's
method label does not specify this distinction; use Path ordering to reproduce
that historical rollup. Membership is exactly8 A +5 prior D top-level +31 build01
+52 build02 files, excluding the three resumed final documents. All checked
individual pins match. No source fix or frozen-evidence rewrite is requested.

## Conditions, remaining gates and stop

Stable ordinary CPU RAM and complete-ioctl mapping/alias serialization remain
caller obligations. access_ok is NOT page pinning, writability, VMA provenance,
physical disjointness or MMIO exclusion; distinct virtual ranges may alias. This
is a CPU-copy ABI, not DMA import/user-MMIO/zero-copy or exclusive allocation proof.
No automatic retry/fallback is allowed; old images remain ENOTTY. Real producer
adoption must reject every error/partial result and separately admit recovery/IDR.

Actual composed runtime still imports the native adapter with malloc1048576 and
the old contiguous STREAM ioctl. No userspace producer/hub/provider connection,
Annex-B span parsing, allocation-cap qualification, raw/process2MiB, existing
2MiB DMA/reference backing, CMA/40MiB/combined load, native entropy, independent
recovery, image/source pairing, main or product gate is closed here. Normal and
negative/adversarial/mutation/fuzz execution remain NOT RUN/deferred, not passed.

Only public static reads/hash/JSON/text/diff/log/ELF-byte metadata inspection was
performed; no recipe import, compiler/readelf/build/link/test/helper/candidate/
ioctl/callback/native/hardware execution, Git/index/private/device/network access,
subagents or settings changes. Only this fresh review directory is authored.
All99 existing A/D files remain unchanged across the review write, with aggregate
`a143809530a3d8181698930d9ac6a4183408ed67d44b3a5de4d9d91e42ec8419`
(complete relative-string ordering, then path/NUL/raw hash), for parent closeout
comparison. The corresponding Python Path-ordered all99 digest is
`deec02a8d54d16d782e65c496377eabb2f8775e8bd566c1e48546770f1a32f06`.

No fresh physical-state observation or owner/recovery lease: historical23h OFF/
finalized only, RAM75 recovery/C1 blocked and all prior HOLDs unchanged. Query78
remains PARKED. **STOP: separate parent disposition before adoption, correction
or use; no further work/build/test/native/hardware phase selected.**
