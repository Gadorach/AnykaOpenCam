# Ordinary author audit — before the sole HOST process

2026-10-24. Author self-audit only; not independent review or parent adoption.
Read the complete new C/header/harness/both scripts/notices and original hub,
backend/RTP headers, accepted span C/header/contract/review/parent disposition,
STREAM/CAP plans and root memory constraints. CONTRACT preceded production code.
No runtime case has been invoked at this audit boundary. Offline build01 passed;
the sole normal HOST case below is the only selected next execution.

## CPU ownership/free proof (SOURCE, including unexecuted branches)

`FRESH -> INITIALIZING -> ACTIVE -> STOPPED -> DISPOSING -> DISPOSED`, or
`INITIALIZING -> INIT_FAILED -> DISPOSING -> DISPOSED`. Self is latched before
the first malloc and never reset. A copied initialized owner has self!=its own
address and is rejected. Fresh means a genuinely new persistent = {0} object at
its final address, not memset of a used object. No caller edits/copies/reentry,
signals, extra fork, plugin pointers or concurrent aliases are admitted.

Eight loop iterations store `malloc(131072)` directly into canonical slots,
before the failure branch and next allocation. No other payload allocator,
realloc, mmap, cleanup callback or native-pointer ingress exists. Failure keeps
all successful slots; allocation counters retain actual request history. Only
explicit dispose can free; INIT_FAILED has no legal publication/acquire path,
and normal disposal requires STOPPED/no acquired bits. It latches DISPOSING,
saves one known CPU pointer, clears that canonical slot, then performs its void
free once. No numeric close retry/unknown native descriptor issue is imported.
Self/phase/generation/statistics stay latched. Caller must never use/call owner,
chunks or borrowed views after disposal. Ordinary HOST process exit is not
external/native retirement; no automatic error cleanup was added.

## Bounds/publication proof (SOURCE)

Full input shape, actual H264 enum, keyframe boolean, exact checked sum, NAL
forbidden bit/type1-or5/keyframe consistency precede state/payload mutation.
There is no Annex-B/AU/parser/decoder validity claim. Trusted valid, disjoint,
serialized caller objects are required; const/non-null is not address/alias
validation. Generation refuses at UINT64_MAX before copy/drop mutation. A last
generation of UINT64_MAX can still have exact releases and stop/drain/dispose;
it never increments again. No separate wrapping hub sequence counter exists.

Copy uses only destination/131072 and destination%131072 plus a source-local
offset; checked total<=1048576 bounds indices to0..7 before every memcpy. Each
piece is a positive minimum of source/destination remainder, <=131072, and
advances; at most16+8-1 pieces. Final destination==total is not dereferenced.
Only current NAL bytes are copied, not guard/capacity tails; views trim their
last span, so stale unused capacity cannot be read through a valid lease. No
source pointer/descriptor is retained. CAMERA_OK only follows the actual copy;
no eligible client returns NOT_READY. CPU copy never grants original source
release/END/receipt authority.

## Leases/resync/STOP proof (SOURCE unless observed later)

Each bit identifies one of four subscribers. Subscribe starts needing IDR.
Explicit resync and BUSY backlog loss clear only unacquired pending bits, mark
IDR need, and never clear an acquired consumer or change its payload/metadata.
Any acquired bit prevents copy/reuse. With unacquired backlog, replacement marks
affected clients for IDR; a now-undecodable non-IDR returns BUSY without a copy.
Only successfully copied IDR clears eligible resync bits. No partial-AU socket
drop/reconnect/deadline or recorder-admission policy is fabricated.

Publication increments one uint64 generation without wrap; its nonzero value
is also the token. The exact release key is (owner,consumer,token,generation),
accepted once while the acquired bit remains. Release clears both that acquired
and pending bit; reacquire of the same generation is impossible for that
consumer. Unsubscribe refuses held leases; resubscribe adds IDR need, not an old
pending bit. No per-client payload copies are made. Acquire emits one small
value-copy view with zero unused descriptors; exact lease validation checks
self/active phase, view owner, subscription/acquired bit, both IDs, codec,
keyframe, timestamp, bytes/count and all eight pointer/size pairs.

The production wrapper checks that lease on EVERY call and directly invokes
the real accepted `camera_rtp_h264_spans_packet_v1`, not the old contiguous
packetizer. Deferred send/effect still needs a fresh caller check under the
same serialization, plus owned pending-packet retention through partial writes;
neither source lease nor packet building implies peer ACK. STOP immediately
denies effects/acquire/publish, sets pending=acquired, preserves exact old release
metadata, and permits release to drain. Drained is only STOPPED/no acquired bits,
not ACTIVE/empty or native END. Release-after-STOP is source reasoning only in
this selected normal case, which releases all four before STOP.

## Actual build inspection; not candidate execution

`/usr/bin/python3 -B apps/rtsp-auth/span-hub-candidate/compile01.py` passed once.
Five warning-as-error C compiles: HOST hub+accepted spans+harness; retained musl
ARMv5TE soft-float hub+accepted spans. Two actual production REL links resolve
the new hub's packetizer reference to the unchanged accepted span object. HOST
alone has an executable/main; neither production REL has main or old packetizer.
ARM REL is ELF32/EABI5/v5TE. Undefined libc/stack-protector/GOT references are
expected; no final ARM executable/ABI or native link/runtime claim.

All33 commands returned0; five compile logs empty. Read-only postbuild check
verified19 selected input,53 consumed dependency,30 tool/link and90 artifact pins;
see BUILD_INSPECTION.json. Actual tool versions: HOST GCC16.2.1, ARM GCC14.3.0.
Local compiler frames: hub acquire224 HOST/176 ARM, publish80/40, wrapper64/40;
accepted spans112/48; HOST main432 dynamic,bounded. Not transitive/native stack
acceptance. Tool and sampled link/header pins are not full compiler closure.

## Sole normal harness and runner audit

The harness has exactly one NAL case and no CLI, listener/fork, device/file I/O,
native inputs, credentials or negative/mutation/fuzz matrix. Three distinct
65536/65536/43008 static source objects hold public formula bytes. One init,
four subscriptions, one actual publish, volatile zero stores to original
synthetics, then four acquisitions check shared chunk identity/trimmed extents.
Actual hub allocation counters and eight nonnull/distinct slots are checked.
Each cursor owns one1404-byte packet, not another NAL. All four leases stay
held through fair packet iteration. Before every wrapper call and synchronous
consumption check, exact lease validity is required. The wrapper itself repeats
validation immediately before the real accepted builder.

Every RTP/interleaved/FU-A header byte and payload byte is independently checked
against fixed numeric formulas. Offset/sequence commit follows the complete
check. One174080-byte NAL means126 packets/client,504 total,696316 checked body
bytes and705388 wire bytes. Four independent channel/SSRC/sequence values, no
new wrap case. No whole-NAL reconstruction or payload/hash output. Release all,
STOP, require drained, explicit dispose; no owner/view/chunk use after dispose.
Any check failure returns immediately without hidden cleanup or retry.

`host_once.py` creates an exclusive host01 attempt before admission checks,
verifies all selected build/dependency/tool/artifact and audit pins, invokes
ONLY the fixed HOST executable once with10s timeout, records exact stdout/
stderr/exit, checks scalar normal expectations, and verifies pins afterward.
It never invokes ARM, old helpers, additional cases, or deletes outputs. A failed
or timed-out attempt stays preserved and must not be rerun/fixed-to-pass.
Python files were AST-parsed without importing/calling them for that check.

## Explicit open boundaries

All init failure, foreign/copy, invalid input, no eligible, non-IDR, backlog,
resync, held-unsubscribe, stale/duplicate release, stop-held, partial-init dispose,
exhaustion/refusal branches remain SOURCE-only, not selected runtime coverage.
Further adversarial/negative/mutation/fuzz work remains deferred. Source wipe is
not real producer/DMA release. Original native1MiB output/contiguous copy/raw,
prefault/residency, process2MiB,40MiB/CMA/TLS/headroom, external retention/recovery,
generic AU/STREAM-08/main remain gaps. No frozen/runtime/provider/framework edits,
hardware, current-state/private reads, Git/index or central-plan changes occurred.
