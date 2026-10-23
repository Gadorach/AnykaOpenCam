# Query78 build recipe — independent ordinary SOURCE/build-method review

Selection2026-10-22; actual review host2026-10-23. ENC-02 / PLAT-01/02/05/08,
QC07F first. **CORRECTION REQUIRED: R1 final output/custody binding and R2 primary
failure preservation. This exact recipe does not receive build-method sufficiency
or invocation approval.** No recipe/preflight/compiler/helper/transformer/test/
program/ELF or code object was called/executed, and no archive/kernel/image was made.

## Exact reviewed packet

- Six-file aggregate: `d3ac858dafa835587474bf7708ef0c5f5a17820cccbe42f5cb4619a52e29a373`.
- Author HANDOFF_SEAL01 SHA256:
  `57e8ea80d7d41c1da1e91de4aaa932ac37e60068cf14b09855d7944d2c585eeb`.
- Actual units read completely: build.py308 lines, common.py237, newc.py164,
  package.py289, plus README/.gitignore, full contracts/report and both final diffs.
- All42 selected pins match; input-list SHA256:
  `e49f04f0b95523e0bad1768739c5fd870361d7c16ff380bc4af80fbe2d0e620d`.
- Fixed parent embedded-image contract remains frozen at
  `f29ea0e41d947c44bb4ae69a3244bcac3b543a3b2db08474ec4b4e5e9db676a7`.
- Completed client REVIEW.md SHA256 actually matches
  `83f1b9ec9b1a125c9485038b0571ec5c7c5c241c0fb2b53bbbffddf5646c0f60`;
  client FINAL_SEAL matches `0f875806f421ab20ffcc7dae4e7c4198ca2b103179dade47b93ed0e7ababe7fe`.
  These are source/byte associations, not image/boot or native permission.

Aggregate recipe is sorted workspace-relative UTF-8 path + NUL + raw SHA256 digest
bytes. All originals, candidate units, old reviews and central records stay intact.

## R1 — Final observation is not qualification-pin revalidation

**Anchors:** `apps/qualification/encoder-query78-build/package.py:205–210,270–289`;
`apps/qualification/encoder-query78-build/build.py:202–203,255–282,295–303`.

Package entry checks BUILT output pins, then constructs/checks a fixed image and
records its hash in PACKAGE.json. However, `finish()` checks source/config/archive/
client/installed inputs only. It never compares the final UIM to PACKAGE.json,
never rechecks all final built artifacts against BUILT.json, and never compares
final consumed/generated closure inputs with their earlier captured rows. Copied
DTB/output identities are simply inventoried at the end. FINAL_OUTPUTS.json can
therefore observe different image/build/closure bytes without rejecting their
disagreement with the receipts used to qualify the package. A new observed hash
does not establish that the previously checked file is still the final artifact.

Also, FINAL.json is written **before** the final inventories are even evaluated.
An inventory/read/serialization failure can leave a terminal-looking FINAL.json
without a completed output seal; the later failure writer is not a substitute for
ordered completion, particularly given R2. No occurrence is claimed: this is the
ordinary source-level dependency/error-order gap, not a mutation experiment.

**Minimum fresh correction:** preserve the actual BUILT/package/closure identity
sets and explicitly compare the image, built outputs, copied DTB and captured
consumed/loaded inputs at their qualification/final boundaries. Recheck the
relevant frozen associations before image publication; compare the actual final
inventory values with the frozen qualification pins instead of adopting changed
values as new truth. Produce a completed output seal first; write an unambiguous
successful terminal receipt only after successful final comparisons, binding that
seal. Intermediate package/closure receipts must remain observations, not final
qualification. Preserve partial/failed outputs, no overwrite/delete/retry/resume.
Endpoint checks are custody evidence, not atomicity or privileged-race immunity.

## R2 — Receipt I/O can replace the original failure

**Anchors:** `apps/qualification/encoder-query78-build/build.py:70–80,299–304`;
`apps/qualification/encoder-query78-build/common.py:48–66`.

`command()` performs timestamp/log-pin/JSON writes in an unguarded `finally`.
If a command raises (including timeout/nonzero-exit refusal) and this bookkeeping
also raises, the propagated exception becomes the bookkeeping error. The outer
FAILURE.json then describes that secondary exception, not the actual operation
failure. Its own unguarded write can replace the failure again. Python normally
retains earlier exceptions in chained context, but that does not preserve the
intended primary exception or the structured receipt's original-failure identity.
This is a normal timeout/read/write failure-path problem, not an adversarial test.

**Minimum fresh correction:** preserve/re-raise the original exception/traceback;
attempt failure bookkeeping best-effort without replacing an active primary
exception. Retain secondary recording errors separately/as notes when possible.
On a previously successful path, receipt-write failure must still fail the run,
never produce successful completion. Use fresh exclusive receipt names, do not
overwrite old evidence, and acknowledge that storage failure may prevent any
durable receipt. No recovery invocation, retry, cleanup or ignored qualification
error is requested.

## Other inspected method boundaries — useful source findings, not composite GO

| Boundary and anchors | Static finding |
| --- | --- |
| Imports / first recipe-owned effects, build:8–29,83–102 | `dont_write_bytecode` is set before local imports. Local modules define constants/functions only; Path resolution/imports still perform ordinary Python startup/read activity. No claim of zero interpreter/stdlib startup I/O. `prepare` checks fixed inputs/review/source/tools/archive/config before recipe-owned mkdir/copy/tool effects. |
| Closed client binding, common:86–129 | Hard-hashed42-entry list, ordinary canonical selected files; explicit lowercase64-hex caller hash must match fixed REVIEW.md before output creation. Exact handoff57edc96b/source66f6e86f/main d3f85cd0/UAPI6052069f/ARM30140B5947009a identities remain closed. No guessed future review hash, ready Boolean, prose-PASS parser or global native authority. |
| Exact source/fresh paths, common:132–163; build:83–118,154–158,169–196 | Full91270 file/symlink manifest equality checks bytes/modes/relative link targets and set; rejects .git/.scmversion/root localversion, escaping links/special files. Directories are traversed, not claimed to be separately mode-pinned manifest entries. copytree preserves confined links and copies independent regular bodies; copied regular nlink1 checked. Fresh fixed source01/build01/images01 mkdir only, no phase/resume selector/original write/old object reuse. |
| Closed newc, newc:19–164 | <=65536-byte reads/copies, fixed source and output, ASCII canonical names/NUL/extents/hex070701/check0/zero alignment/root/nlink1/unique inode/parent-type checks. Symlink targets inspected, never followed. Exact original50-row comparison, module record removal only, fixed client insertion0555/max+1/nonoverflow/mtime0 before original trailer, derivative reparse50 rows and retained raw-record hashes/order. No extractor/mknod/host deletion/autostart/generic payload CLI. |
| Config, common:166–196; build:119–187 | Duplicate-free ALL-symbol config equality. Only LOCALVERSION78 and fixed new INITRAMFS_SOURCE differ; original23.1.3 text and230103 version fields remain. AUTO off/LOCALVERSION empty; no manual generated-field edits/feature/warning bypass. Real generated autoconf defines are compared to ALL expected symbols. Actual olddefconfig/new headers remain unexecuted future gates. |
| Tools/commands, common:199–233; build:37–81,130–132,138–193 | Original accepted installed tool/header/resource/support maps are read, not old scripts imported. One subprocess.run site; actual callsites select versions, olddefconfig, one make-j4 zImage vmlinux and ELF inspection. No modules/install/test/target ELF/default/help. Kbuild host generators are future ordinary build tools, not target-program permission. |
| .cmd/loader association, build:205–266 | source_/deps_ paths must resolve inside fresh source/build or accepted installed rows. Missing individual include/config wildcard dependencies are recorded as absent, not pinned; missing required/unresolved paths refuse. Actual fixdep source and retained .head/.fixdep command syntax support that grammar. Absolute installed loader aliases compare accepted resolved/body pins; relative generated spellings require one matching location; unresolved names refuse. Final-observed generated/discovered facts are not pre-use hashes. No whole-syscall or total-dependency proof. R1 still applies to final closure comparisons. |
| Packaging, package:20–289 | Actual bounded XZ+LE32→Image and GZIP→new archive, ELF allocated-section→Image/zImage byte reconstruction, embedded initramfs/piggy associations and exact2571B retained DTB. Actual symbol-based kernel/BSS/pgdir and decompressor stack/order checks; no copied77 success sizes/CRCs. Header64B OS5/ARM2/kernel2/compression0/time0/load82408000/entry82408040/fixed query78 label, zImage+DTB payload, exclusive regular output. No truncation/padding of payload or native operation. R1/R2 prevent overall acceptance. |

CRC source was inspected mathematically, not called: IEEE header CRC zeroes its
CRC field, payload CRC covers the payload; BZIP2 shifts left using04c11db7 with
ffffffff init/final XOR and no reflection. No test vector or CRC binary executed.
ELF/raw association skips nonallocated/NOBITS/.comment and checks nonoverlap/gaps;
required symbols must exist with a single consistent value. These future predicates
have not been observed passing on a query78 output.

Pinned head.S, compressed Makefile/linker, boot Makefile and usr inclusion source
were read directly. They support the selected4096-byte stack,64KiB malloc,
32KiB-min/1MiB-max aligned DTB-growth bound, ARM32 initramfs-size field and raw
objcopy associations. Kernel text/pgdir81808000/81804000 and end<82408000, whole
UIM<=4MiB/staged and conservative live end<82800000 remain explicit future checks.
40MiB DT/CMA82800000..83800000/native128KiB cap stay fixed. There is no new size,
RSS/startup/CMA/DMA/combined-headroom/runtime fit result.

Tool checks occur at preflight/phase stability intervals, with the immediate
top-level executable rehashed in command(). That is not a fresh check immediately
before every Kbuild child/header access. Any later call needs a trusted serialized
host/no concurrent tool/source mutation contract; delayed detection is not prevention
or privileged-race immunity. No tool or missing-feature bypass is recommended.

## Independent reader evidence and explicit limits

`STATIC01.json` verifies six source pins,11 documentation pins and42 selected pins;
all four actual final AST call/import/function-span lists exactly equal CALLSITES02.
Final build/package diffs regenerate byte-identically. SYNTAX02's author4/4
in-memory syntax compilation is **cited only**, not rerun; independent inspection
uses AST, no executable code object. Earlier01 receipts/diffs remain unchanged.
Interim build/package textual reconstructions match their01 syntax pins; no absent
interim common.py body is invented or claimed to match final source.

The complete91270-entry manifest and accepted tool-map schemas were read as
metadata, not all underlying source/tool bodies rehashed. Installed accepted rows
merge to50365 alias paths without conflicts; the11 mandatory top-level tool paths
read/hash-match their baseline. Full future preflight remains uninvoked and may
refuse on source/tool drift or an ordinary actual-build difference. This is not a
250219-path reproduction, new build admission or claim all50k inputs were rehashed.

`BASE_METADATA01.json` independently reads/hashes the saved fixed original50 raw
record extents/metadata/bodies, plus196 final zero bytes, against BASE_INPUTS01.
No candidate parser call, extraction, synthetic/derived archive or transformation.
Existing original77 ELF section/required-symbol metadata was inspected only to
check field meanings, not a new package result. Every additional reader is pinned
in INPUTS02. The public historical .config is explicitly read/hash-checked against
selected/parent closure here; previous client scope/error records stay frozen.

INPUTS02 has100 path-associated inputs. POSTCHECK01 rehashes all100 unchanged,
including source/docs/config/selected inputs/client review and inspected old
artifacts. No mutable context was rebased. No reviewer collection error or policy
refusal occurred; old failures/interim syntax history are retained as history, not
new runtime failures. FINAL_SEAL binds fresh reader and review-output pins.

GPL notices/old recipe derivation and baseline GPL license association remain;
toolchain/libc/retained payload licensing is separate, no global redistribution or
publication approval. `.gitignore` excludes local Python caches; future generated
artifacts remain local under ignored work. No Git/index operation occurred here.

## Handback

Parent should retain this candidate/review unchanged and, if selected, request a
fresh bounded source revision for R1/R2, followed by independent source review.
No broad architecture rewrite, native feature change or new test campaign is
required by these findings. No correction author/retry or execution is queued.

No invocation/archive/image integration/kernel build/current-state/hardware/
physical/manual-OFF request or automatic next operation is authorized. Monitor/
PID1/help remain unchanged; unsupported advertised wifi attach remains forbidden,
not full PLAT-02/station support. Actual bootstrap/observer/lifetime/clock/catch/
watchdog/global-idle/FD/backing/provisioning/main/native combined-load and G2/G3/G8
remain unproved. Historical12:12UTC U-Boot and prior HOLDs supply no current facts.
