# Author handback — encoder output spans, static reconciliation complete

2026-10-24. **B07 / RP-04/05/09; CAP-01/05/06/07 + ENC-03/05 dependency.**
SOURCE candidate with saved compile evidence only. **NOT independently reviewed,
NOT adopted, NOT executed, NOT a product/native/allocation-cap acceptance.**

## Resume boundary and delivery

The original author was cancelled at the host-restart pause before final handback.
Explicit resume selected only missing public static reconciliation, not a live
worker/run continuation. This handback finishes that author documentation boundary;
it does not retrospectively turn the interrupted work into reviewed/adopted work.

Only `HANDOFF.md`, `AUTHOR_FINAL_VERIFICATION.json` and `DELIVERY_PINS.json` are added
after resume. All eight app files, five prior top-level qualification documents,
31 build01 files and 52 build02 files are preserved. No source/header/ABI/recipe/
notice/README/contract/report/log/input/selection/disposition/diff is rewritten.
The verifier uses only local file reads, hashing, JSON/text comparison and new
exclusive-create final documents. No recipe import, compiler/help/readelf/tool
invocation, build, ELF/candidate/test/function/ioctl/callback/native execution,
network/private/device query or hardware operation occurred after resume.

Parent-supplied HEAD26d7e4c and ordinary-index hash are context, not re-queried here;
no Git/index/AGENTS/central-plan operation. Historical target evidence remains23h
OFF/finalized, **not a fresh camera observation**. No inherited runtime/recovery
lease. Query78 remains PARKED; no new image/query/clock/receipt framework.

## Actual implementation, not a standalone scatter helper

The full536-line driver derivative is
`apps/qualification/qc07f-encoder-output-spans/qc07f_hwenc.c`.
Its actual `encoder_ioctl` accepts the exact new full command under the existing
mutex, numeric calling-file session, quarantine gate, reference-owner BUSY gate
and H.264 stage begin/end (`:371–458`). Existing VFS open/release custody remains.

1. **128-byte ABI / eight fixed capacities.**
   `encoder-output-spans.h` declares only u32 fields, alignment4: version1,
   struct_bytes128, keyframe0/1, count8, width1280/height720/input_bytes1382400,
   u32 input address, eight u32-address/u32-length descriptors, output bytes/status/
   frame_num and five zero reserved words. Every capacity is exactly131072, total
   1048576. Exact command `_IOWR('Q',0x67,...)` = **`0xc0805167`**. HOST/ARM compile
   assertions cover every field offset, sizes/alignment and full new/old commands.
   Existing number0x65 lifetime/stage commands remain distinct full commands;
   stage size296/full command`0xc1285165`. Open query0x66 stays unchanged.
2. **Snapshot/preflight before hardware.**
   `encoder-output-spans.inc:12–48` checks control address before the sole full
   request `copy_from_user`, then validates all fields/reserved/output-on-input
   words and every source/control/output address range. It conservatively rejects
   u32 endpoint overflow; widened overlap arithmetic rejects intersecting output
   ranges and output overlap with full input/control memory. `access_ok` covers
   complete ranges before the real encoder body is called. No descriptor reload.
   Malformed/preflight failure preserves the prior reference owner. Ownerless P
   is ENODATA; another file's owner is BUSY through the original common gate.
3. **ONE actual hardware H.264 body.**
   `qc07f_hwenc.c:268–370` uses an explicit typed destination, not a fake pointer.
   Legacy STREAM/IDR select the contiguous kind and retain original contiguous
   capacity/pointer validation and single-copy behavior. The spans branch supplies
   validated stack descriptors; its local frame's unused contiguous output fields
   are zero, not forged to bypass old checks. Actual input copy remains the real
   existing `copy_from_user` into enc.input. Register programming, QP26/CAVLC,
   input/output/reference/aux DMA backing and hardware behavior are unchanged.
4. **Real retirement and valid output before any scatter.**
   The unchanged hardware core (`:284–358`) performs actual reset retirement,
   refuses reset failure, executes dma_rmb, checks all original input/output/ref/
   aux guards, poll/status and nonzero byte-aligned length <=OUTPUT_LIMIT before
   destination selection. No freeing of uncertain DMA or reset retry is added.
   `encoder-output-spans.inc:51–67` copies the actual enc.output at a cumulative
   offset into successive snapshot ranges. Every copy <=131072, the last partial
   chunk copies only actual bytes, no unused tails or new full-AU allocation.
5. **Data, then metadata, then reference commit.**
   Only after real data copy success does `:418–423` populate/copy the control
   result. Only after that full copy0 does it commit h264_owner/frame_num/ref_index
   and the inherited owner-generation observation. IDR starts num0/ref0; P wraps
   num16 bits and alternates references exactly as old STREAM. Any failure after
   entering the encode body clears the new route's logical owner, even if already
   quarantined. It never clears quarantine, fault_session or DMA uncertainty.
   Final release clears a matching file pointer before freeing session data.

## Error and telemetry meaning

First partial output-copy fault stops with EFAULT; no skipped partial chunk,
retry, repair, padding or fabricated output count. Some bytes may already be
visible. A later metadata-copy fault can leave complete data and partial metadata.
**Every error makes the entire result unusable; metadata is valid only with
syscall0.** Discard partial output; a failed actual job has no logical P reference
and any future admitted encode needs IDR. No automatic retry/recovery is selected.
Preflight-only rejection is different and preserves references; errno alone does
not prove which hardware phase ran or establish DMA retirement.

One OUTPUT_COPY sample covers the entire scatter operation and reports actual
copied bytes and actual result, including the first partial-fault residual. Legacy
copy telemetry is untouched. Stage end counts delivery only on final result0.
Inherited enc.info.last_bytes is hardware-length history, **not delivery proof**;
it can remain nonzero on a later data/metadata failure. No invented completion.

`access_ok` is not page reservation, writability/VMA/physical-alias provenance,
exclusive allocation, mapping stability or a deadline. Stable ordinary CPU RAM,
mapping/alias serialization and no MMIO remain caller obligations; disjoint user
virtual ranges can still alias. The ABI offers no physical-address, DMA import,
user-MMIO or pinning interface. Concurrent unmap/remap/pixel mutation is not cured.

## Exact unchanged scope / inverse reconciliation

Baseline driver SHA256:
`9bf16b1175befb091d6c61c3ac573121378e1d8c403eed21806a3e8fc80fc946`.
It is the accepted query-preserving seven-file baseline at
`docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate/`;
retained prior source-seal identity:
`ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`.

Seven literal C delta groups and one header delta invert exactly to that baseline.
Saved forward/inverse patches regenerate exactly from current bytes, without
executing patch/build code. The entire pre-H.264 prefix (including JPEG/DMA/reset/
allocation), hardware core before scatter selection, open/lifetime/stage query
block, and release/open/init suffix compare byte-exact. Five support header/inc
files are byte-exact baseline copies in both run snapshots. No legacy command
value or query body changed. Old STREAM/IDR call sites only gain the typed
contiguous argument (and incidental indentation); their old failure quirks,
including quarantine-owner handling, are not blanket-cured. All original license/
SPDX/module notices and built-in/static-DT/no-PM/no-kexec guards remain.

## Saved compile evidence — no resumed compilation

**build01 FAILED_PRESERVED:** five commands (four0, HOST ABI compile1), no driver
compile. The author mistakenly asserted stage size304/full command`0xc1305165`.
The failed consumed ABI copy/logs remain immutable. Current ABI differs from that
copy by **only** the two documented corrections:296 /`0xc1285165`. No input-history
rebasing. Exact31-file membership/pins match BUILD02_SELECTION.json.

**build02 saved PASS, 10:02:54–10:02:56UTC Oct24:** twelve commands, all0; three
version reads, three real dependency preprocesses, HOST/retained-musl-ARM ABI
objects, one NEW Clang23.1.3 whole real ARM-driver object and three ELF metadata
inspections. All twelve saved stderr files are empty. No link/make/generator,
kernel/module/image build, synthetic driver harness or output execution occurred.
One real driver compilation total; build01 never reached it.

Static reconciliation finds all603 build02 input pins unchanged; dependency
records21 HOST ABI +19 ARM ABI +515 driver (555 overlapping records), saved
log/artifact pins and all eight current app files match their build02 consumed
copies. All78 build01 input records reconcile except the one documented current
ABI-file correction (77 unchanged); its consumed original remains exact. The four
recorded top-level tool hashes/resolved paths still match on read-only comparison;
that is not a fresh compiler invocation or full loaded-toolchain closure claim.

Whole-driver object:21668 bytes, saved ELF32 little-endian ARM EABI5 REL, entry0,
no program headers; real ioctl/open/release/init, ONE encode_h264 and actual
snapshot/scatter functions are present in saved symbol metadata. Object SHA256:
`c9a39012806ab0bc481a906b780d19970716ee0b9b49d4d792841ea658b8f40f`.
It remains local, unexecuted, excluded from narrow delivery-file pins.

| File (relative to app unless marked) | SHA256 |
|---|---|
| qc07f_hwenc.c | `36bc80fadc0a16ded4525f24f805022faf22ee0ac8cb77cdfa4b390ba23e6b50` |
| qc07f_hwenc.h | `f2c651e522e7b5444a26c4c8aec05a10e483ee0e1fb41a655219ddf91d3f63e7` |
| encoder-output-spans.h | `4267d4074177468898937926b35029fbfb2b392c25ea987832c83137841cb6e6` |
| encoder-output-spans.inc | `0a49fd1ce8cf1962f747df0f05db66b7614570885a39f1515780a5f560f7a415` |
| qualification runs/build01/REPORT.json | `2f1740eec954ee8cfb512a0e5e31ca576e2476d03bd14fa3933ab4939b17bcef` |
| qualification runs/build02/REPORT.json | `065e38cd805575ae918464929e70f737d8a41bb880ccc6709892f445d55f6538` |

Full eight-app-file/source identities, exact checks/counts and narrow public
source/docs/report delivery pins are in the two companion final JSON documents.
Binary objects and toolchain binaries are not delivered/redistributed.

## Unqualified dependencies and handback stop

Clang23.1.3 remains a distinct SOURCE-object baseline; retained generated compiler/
config metadata still names23.1.1. No native ABI/paired kernel/header/image/module
acceptance follows from these objects or static assertions. No image gains the
new ioctl; old images return ENOTTY, no fallback.

The actual composed runtime still includes the span-runtime native adapter, whose
prepare_output uses malloc1048576; its imported producer still invokes contiguous
QC_HWENC_H264_STREAM. No native producer/controller/provider/hub source adoption,
Annex-B span parsing or CPU-output lender/consumer connection was implemented.
Hub v2/RTP span bounds are context, not integration. The new kernel sink does not
prove independent allocations, the overall128KiB cap, raw/process2MiB budget,
existing2MiB guarded DMA/reference allocations,40MiB/CMA/combined TLS load, native
fit, strict C1/independent recovery, entropy/provisioning/main or product gates.
CAP/ENC import/zero-copy/bitstream/native requirements remain open. Deferred normal/
negative/adversarial/mutation/fuzz coverage is NOT passed; no normal test was run.

**Stop at finished author handback.** Independent ordinary source/evidence review
and an explicit parent disposition are required before use/adoption. Any ordinary
blocking defect requires fresh correction selection, never an in-place rewrite of
this preserved candidate. No automatic build/test/native/hardware/next-work queue.
