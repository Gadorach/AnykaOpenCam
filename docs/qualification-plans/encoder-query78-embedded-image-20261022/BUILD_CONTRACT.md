# Next qualification image — embedded query client, offline selection

2026-10-22; ENC-02 / PLAT-01/02/05/08. User requested continuation. Parent
selects a bounded build-side prerequisite, not native execution or a product image.
All75/76/77 images/modules/source/configuration and original failures stay frozen.

## Purpose and first selected packet

Query77 lacks a built-in query88 command/client. The investigated UART-to-shell
file transfer has no admitted watchdog-owning staging receiver. A fresh image can
carry a reviewed client at boot, avoiding that particular post-boot file-writing
interval. **Embedding alone does not admit client launch or solve clock/recovery.**

First selected packet is fresh `apps/qualification/encoder-query78-client/` and
matching qualification docs: the exact reviewed query77 client with only its
release and explicit trial-argument literals changed to query78. Original query77
rejects another release; silently reusing it would produce an incoherent package.
Same query88 UAPI and all watchdog/query/budget/retention/close/output guards remain.
HOST/ARM compile/static-link/ELF inspection is selected, neither executable run.
Independent source/build review precedes integration of this fresh client.

## Deliberately small proposed image delta

After that review, a separately inspected fresh build recipe may:

1. Copy the full exact query77 Linux6.18.55 source baseline into a fresh source
   root without SCM metadata, not modify the old tree or silently reapply overlays.
   No kernel driver source change is proposed. Keep Clang/LLD23.1.3 and regenerated
   coherent config/generated headers in a new output directory.
2. Construct a fresh newc archive from the pinned initramfs76 used by query77,
   **without extracting or executing entries on the host**. Remove only the
   unpaired `hgics.ko` record and add only the independently reviewed ARM query78
   client as `bin/encoder-query78-qualification`, root-owned regular executable.
   All other original records, including PID1, stay byte-identical. Do not replace
   the original archive or remove any host file. Preserve original records/hashes
   as input evidence and record every addition/removal explicitly.
3. Change only `CONFIG_LOCALVERSION` to `-qc07f-encoder-query78` and
   `CONFIG_INITRAMFS_SOURCE` to the fresh archive relative to the coherent query77
   config. Compiler/feature, memory/CMA, watchdog and compression choices must not
   drift. Regenerate/review config before compiling builtins; no modules/install.
4. Reuse the exact retained DTB and original RAM-only legacy image format. Recompute
   all image/header/data CRCs, actual Image/zImage/initramfs association, kernel
   symbols and conservative decompressor stack/malloc/DTB-growth extents. No old
   query77 size, CRC or free-space figure may be copied as a result for query78.

This contract does not yet admit a packager invocation or full kernel build;
the actual fresh recipe and its closed input identities still require parent
inspection. No new monitor command, process supervisor, shell transaction or launch
protocol is selected. PID1/help/shell therefore remain the old source/bytes.

Parent's separate read-only archive inspection is recorded in `BASE_INPUTS01.json`:
all50 records match the retained query77 inventory, no regular-file hard-link
groups, and symlink bodies/record padding/trailer were inspected without extraction
or entry execution. Whole archive length/hash matched before/after. This is
original-input structure evidence, not a transformed archive, new client admission
or future parser/packager execution result.

## Stale module boundary, not full PLAT-02 acceptance

The original archive inventory identifies exactly one `hgics.ko` regular record,
163720B, unpaired with query77. Removal from a fresh archive prevents that embedded
module from being loaded from its original pathname. It is not station support,
a paired module build, P1 acceptance or global release-manifest completion.

The retained monitor still advertises `wifi attach`. Its actual helper opens the
module read-only and returns on a missing file before acquiring its watchdog or
calling finit_module. The command remains unsupported/forbidden; absence of the
embedded file is not blanket permission for this command or an externally supplied
replacement. Document this preserved misleading help rather than claim a new
monitor/qualified radio. No radio action or module build/load/autoload is selected.

## Archive/build evidence and resource limits

Inspect archive path spelling, duplicates, fields, types, link targets, hard-link
relationships, extents/alignment/trailer and zero padding before copying records.
Never instantiate archive device nodes/symlinks on the host. Refuse ambiguous
records/aliases instead of normalizing unsafe names. A fixed transformation must
prove exact original record retention, exact one removal/one addition and exact
client body hash. No permissive generic extractor/payload selector or overwrite.

Fresh exclusive output directories/receipts preserve failures, partial outputs and
actual commands/tools/source/header/linker dependencies. Never rerun an old package
entrypoint whose modules/tests/CRC checker has effects beyond this selection.
Kbuild host generators are build tools, not permission to run target executables.
All target ELF/images/retained payloads stay local and excluded from Git.

Measured40MiB RAM, original4MiB transfer maximum and stricter staged/decompressor
end below `0x82800000` remain. Preserve CMA `[0x82800000,0x83800000)`, original
decompressed placement and128KiB native allocation cap. An archive-size reduction
or static link is not a measured headroom/DMA/combined-load fit. Source licensing
and retained payload provenance remain unchanged; no blanket redistribution grant.

## Native boundary remains closed

Neither reviewed bytes nor a new image establish actual target/owner/capture/catch,
clock continuity, global idle, source association, watchdog recovery or manual-OFF
readiness. Client nominal5s guard begins only at its own future watchdog open;
late disable/output/failstop may have no running timer. Embedding does not cover
kernel boot, Linux idle, process startup or post-query output. Numeric output AND
actual child exit/sameboot postflight still need a separately reviewed method.

No source-pin replacement, live recorder adoption, UART/RPC/status, native capture,
staging/boot/query/reset, persistent write or physical request is selected. Both
FAILED/HOLDs stay untouched;12:12UTC held original U-Boot is historical only.
QC07F remains first; product/bootstrap/backing/retention/provisioning/main and
G2/G3/G8 remain open independently of this offline qualification image work.
