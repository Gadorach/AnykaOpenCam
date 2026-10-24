# RP-09 / B07 — shared chunk-owned one-slot CPU hub

Written before source, 2026-10-24. STREAM-01/02/03, CAP-01/05/07 increment only;
ordinary independent review and parent adoption required before any use.
Only the assigned fresh candidate and this evidence directory may change.

## Ownership and lifetime

One persistent, trusted, zero-initialized, stable-address owner is used once.
Initialization checks fresh identity/phase, then latches self/INITIALIZING before
any allocation. Eight canonical pointer slots each receive a separate direct
`malloc(131072)` assignment, immediately, before checking failure or allocating
again. Aggregate capacity is the existing 1048576-byte NAL envelope, not a raised
limit or eight views over a large object. No realloc, mmap, full-AU temporary,
per-client payload copy, allocator callback or native/descriptor adoption.
Failed init keeps every successful partial allocation in INIT_FAILED; no implicit
free, retry or reinit. Only explicit dispose admits INIT_FAILED/no publication or
STOPPED/fully drained. It latches DISPOSING and clears each canonical slot before
the corresponding known-CPU `free` (void return); ends DISPOSED without erasing
history. No automatic destructor, held-lease free, external-owner/DMA/END/ACK or
process-exit/recovery inference. No calls or dereferences after disposal.

Caller supplies valid serialized stable disjoint C objects, not hostile/plugin
pointers: owner, borrowed input metadata/descriptors/source objects, output views,
sessions, packet and scalar outputs cannot overlap each other or any hub chunk.
No mutable chunk aliases, owner copies/moves, signals, reentry, concurrency,
callbacks or extra fork. Self identity rejects copied/foreign initialized owners;
this is not arbitrary memory validation. Caller never edits owner internals.

## One VCL and four independent leases

Publish accepts the actual accepted `camera_rtp_h264_span_v1` descriptors: 1..16,
nonnull/nonempty each <=131072, exact checked sum 1..1048576. Only CAMERA_H264,
one VCL type1/non-IDR or type5/IDR (keyframe exactly agrees), forbidden bit clear.
No Annex-B, JPEG, SPS/PPS, multi-VCL/AU or generic media inference. All validation
and generation exhaustion checks precede payload/metadata changes. Copy only the
current NAL through bounded source/destination cross-chunk loops; no source
pointer is retained. CAMERA_OK means an actual single secured owned copy shared
by all eligible consumers; no eligible consumers returns NOT_READY, not success.
Original producer release/receipt/END authority remains entirely with its caller.

Subscribe requires IDR; explicit per-consumer resync drops only unacquired pending
backlog. Acquired backing is never overwritten. On occupied/acquired BUSY,
pending acquired bits/metadata remain, unacquired backlog may be dropped and
subscribers marked for IDR. Replacing unacquired backlog similarly forces IDR;
a new non-IDR that cannot be decoded returns BUSY, without payload changes.
Invalid input changes nothing. Four subscriber/pending/acquired/resync bits,
one monotonically increasing uint64 generation; token equals that nonzero
generation. Refuse at UINT64_MAX before another increment/copy, never wrap/reset.

Acquire copies only a fixed small view: owner identity, token/generation, codec,
keyframe, monotonic timestamp, total/count and <=8 const owned chunk descriptors
(trim final extent, zero unused entries). View metadata owns no allocation;
borrowing lasts until exact consumer/token/generation release, accepted once.
Unsubscribe refuses an acquired consumer. Exact lease validation checks owner,
phase/not-stopped, subscription/acquired bit, token/generation, all scalar fields
and every descriptor including unused entries. The production packet wrapper
validates the exact lease anew then calls the unchanged accepted span packetizer.
Caller must also validate before every deferred send/effect, retain its <=1404B
pending packet through partial writes, and commit sequence/offset only after the
whole packet is sent; the hub does not implement sockets/deadlines or peer ACK.
STOP rejects publish/acquire/new effects, keeps acquired pending bits and metadata,
drops only unacquired backlog; old exact releases remain accepted. Drained means
STOPPED and no acquired bits, never active-empty or fabricated native retirement.

## Bounded selected evidence

Offline actual HOST + retained musl ARMv5TE warning-as-error C compiles; compile
the unchanged accepted packetizer once per architecture and real hub+packetizer
REL links. HOST harness compile/link only; no ARM executable. Small fixed recipe,
fresh exclusive output, exact argv/tool/source/consumed-header/log/artifact pins;
no old helper, builder, generator, image/kernel or broad receipt framework.
After ordinary author audit of source/harness/runner, ONE normal HOST process:
numeric 174080B IDR in separate 65536+65536+43008 source objects, actual init,
four subscriptions, one actual copy, four acquired views; wipe only those public
synthetic sources after publish. Validate every packet build and synchronous
check, call the real accepted packetizer via the production wrapper for each
client, check all wire/body bytes incrementally with one <=1404B packet/cursor.
Only after complete synchronous checking commit cursor/sequence. Release all,
stop, observe drained and explicitly dispose known CPU chunks. Print scalar
allocation/count/byte/drain/dispose facts only. No failure/backlog/refusal/wrap
matrix, negative/adversarial/mutation/fuzz, second HOST run or fix-to-pass retry.
On any execution failure preserve and stop. Source wipe is not DMA release.

Native 1MiB output malloc/contiguous driver copy, raw ownership, original hub/
cursor/runtime and prefault ledger stay unchanged. Allocation size is not locked
residency; process2MiB, total40MiB/CMA, TLS/headroom, external ownership/recovery,
generic AU/RTCP and actual main/native integration remain open. STREAM-08 snapshot/
audio/mux/recorder admission is not implemented by four video lease bits. Camera
last controller OFF after23h and RAM75 recovery/C1 block are parent context only.
