# RP-09 / B07 — chunk-backed shared CPU hub handback

2026-10-24. **Author candidate delivered; five HOST/ARM compiles, two real
production REL links and ONE normal HOST process PASS.** Independent ordinary
review and parent adoption remain required. No full runtime/native/cap/product
acceptance; no further process/test invocation is selected.

## Source/API and ownership

Only fresh `apps/rtsp-auth/span-hub-candidate/` and
`docs/qualification-plans/span-hub-20261024/` were written. No Git/index/AGENTS,
central-plan, original framework/runtime/provider, accepted packetizer, builder,
seal, private/current-state or hardware changes. GPL-2.0-only derivative notices
are preserved. CONTRACT was written before source; detailed ordinary source/
harness/runner reasoning is in AUTHOR_AUDIT.md, not an independent review.

Production `span_hub.c/h` implements actual independent backing and one copied
slot, not descriptors over an existing1MiB allocation. One stable one-use owner
contains self/phase, eight canonical malloc pointers, fixed metadata, four-client
bitmasks, monotonic generation and allocation-request history. Each malloc asks
exactly131072 bytes; aggregate capacity is the inherited1048576-byte envelope.
Actual borrowed accepted span descriptors are fully shape/sum/header-checked
before mutation; only one H264 VCL type1 or5 with matching keyframe is admitted.
The bounded cross-chunk loop copies only current NAL bytes once for all clients;
no source pointer survives. Successful copy secures only the CPU copy; producer
release/receipt/END still belongs to the actual original caller contract.

Subscribe/explicit resync require IDR. Pending-but-unacquired backlog can be
dropped with resync; any acquired bit prevents overwrite/reuse. BUSY may change
drop/resync bits, never acquired payload/metadata. No eligible consumer returns
NOT_READY rather than pretending a copy occurred. Invalid source leaves state
unchanged. This one-VCL API/result policy is NOT a drop-in replacement for the
old JPEG/multi-slot/OK-without-copy hub.

Acquire returns a fixed small const-chunk view. Token equals the nonzero uint64
publication generation; generation refuses at UINT64_MAX before copy or further
increment, never resets/wraps/reuses. Exact owner/consumer/token/generation release
is accepted once. Exact lease checking also covers phase, subscribed/acquired
bit, view owner, codec/keyframe/time/bytes/count and all eight pointer/size pairs.
The production `camera_span_hub_packet_v1` rechecks it before directly calling
the actual unchanged accepted `camera_rtp_h264_spans_packet_v1`. Deferred sends
still require caller lease checks and owned pending packet/whole-packet commit.
No socket, callback, source-release, peer-ACK or clock/deadline policy is added.

STOP denies new effects/publish/acquire, sets pending=acquired, retains backing
and exact old releases; drained means only STOPPED/no acquired bits. Unsubscribe
refuses acquired consumers. Initialization latches one attempt before malloc;
failure retains all canonical partial known CPU allocations in INIT_FAILED.
Explicit disposal only permits that unpublished failure state or STOPPED/drained,
clearing each canonical slot before its known-CPU void free. No auto-destructor,
retry/reinit, fake ownership booleans or native/FD/DMA adoption. Self rejects
copied initialized owners; valid trusted serialized disjoint C objects/no aliases/
signals/reentry/extra fork/post-disposal use are mandatory, not plugin security.

## Actual commands and offline build facts

From `/home/gadorach/Desktop/AnykaOpenCam`:

```text
/usr/bin/python3 -B apps/rtsp-auth/span-hub-candidate/compile01.py
/usr/bin/python3 -B apps/rtsp-auth/span-hub-candidate/host_once.py
```

Each command above ran once, in that order, with read-only build inspection and
the recorded ordinary author audit between them. No source correction or failed
build/run/retry occurred. Precompile refinements added small-metadata assertions
and scoped compiler TMPDIR/tool pinning; all existed before build01. Neither old
helper/builder nor any other candidate was invoked. The sole process invocation
argv/environment/timeout is in `runs/host01/INVOCATION.json`.

`runs/build01/REPORT.json` records all33 exact argv/cwd/environment/returncode/log
records and90 artifacts. Five warning-as-error C compiles: HOST hub, accepted
spans and normal harness; retained musl ARMv5TE soft-float hub and accepted spans.
Actual source headers are used. HOST GCC16.2.1; ARM GCC14.3.0. Both real production
REL links combine hub+accepted span objects; nm shows the actual packetizer
reference resolved, no production main or old packetizer. ARM output is only
ELF32 REL/EABI5/v5TE, never an executable. HOST executable links the actual REL.
All33 tool commands returned0; all five compile logs empty. HOST/ARM object and
REL readelf/nm outputs and link traces/maps are retained. No candidate execution
was hidden in the build. Compiler temporary files were scoped to build01/tmp.

Read-only BUILD_INSPECTION.json verifies19 input,53 consumed dependency,30 tool/
link and90 artifact pin records; PRECALL_AUDIT.json adds12 explicit audit pins.
These are overlapping records, not full compiler closure or native ABI proof.
Local compiler stack frames include hub acquire224/176 HOST/ARM, publish80/40,
packet wrapper64/40, accepted packetizer112/48, HOST main432 dynamic,bounded.
Not transitive/native stack qualification. No other candidate program or ARM code ran.

## ONE normal HOST observation, not runtime integration

Actual process: 2026-10-24 03:34:37.737–03:34:38.048 UTC, exit0, empty stderr.
One numeric synthetic174080B IDR NAL from three separate65536/65536/43008B
objects; not decoder syntax, footage, credentials, private input or native media.
Eight actual malloc results were canonical/non-null/distinct; instrumentation
recorded eight calls,1048576 successful requested bytes,131072 maximum request.
Four subscriptions, ONE actual publish, volatile wipe of originals, then four
acquired views all point to the SAME two owned chunks, trimmed131072+43008B.
No full-NAL reconstruction or copy per consumer. Each cursor owns one1404B packet.

Every packet build and synchronous consumption check had a fresh lease check;
the production wrapper also checked before the real accepted builder call.
All interleaved/RTP/FU-A header and body bytes were checked incrementally against
independent formulas; offset and per-client sequence commit only followed each
complete check.126 packets/client,504 total,696316 checked body bytes,705388
wire bytes; each complete source NAL is174080B (FU-A bodies omit its one header).
Four releases, STOP, drained=1 and explicit dispose=1 completed normally. No
owner/view/chunk use followed disposal. HOST sizeof owner152B, view184B,
cursor1632B; no allocator-overhead or locked/resident-budget claim.

The measured HOST layout is1048576B requested backing +152B owner =1048728B,
before allocator overhead. Four1632B cursors add6528B total (already including
the184B view and1404B packet each; do not count those twice). The174080B of
separate synthetic sources is an additional HOST harness obligation, not a
qualified native output design. Libraries/stacks/stdio/heap overhead and page
residency are not measured by these sizeof/request counters.

Exact scalar stdout is retained in `runs/host01/stdout.txt`; attempt, invocation
and observation JSONs retain the one-case/no-retry boundary. The runner checked
pins both before and after. There were no payload logs/hashes, sockets, partial
writes, decoder, listener, fork, device I/O or cleanup effects on external files.
Source wipe is evidence for this owned CPU copy, NOT real DMA/source release.
The actual HUB -> packetizer connection is observed only in this HOST harness;
the original server/runtime has NOT adopted it. Do not repeat the HOST process.

## Source-only and still-open gates

Failed init/partial dispose, invalid/foreign/copied owner, no eligible, non-IDR,
resync/backlog/BUSY, held-unsubscribe, stale/duplicate release, stopped-held old
release, generation exhaustion and refusals were not executed. Their reasoning
is SOURCE-only. No extra negative/adversarial/mutation/fuzz matrix was selected.

Malloc success/128KiB requests do not qualify residency or a whole app cap. The
normal case touches only the two current-NAL chunks, not a1MiB prefault. Native
1MiB output malloc, driver contiguous STREAM copy/capacity contract, raw buffer
ownership, old hub/cursors, prefault ledger, process2MiB and combined40MiB/CMA/
TLS/socket/allocator/headroom obligations remain untouched/open. Actual provider/
source retirement/DMA cache/END/drain/external retention/recovery, generic multi-
VCL AU/parameter sets/profile generation/RTCP, STREAM-08 audio/snapshot/recorder
policy and main/native product adoption remain gaps. Do not raise the cap, split
views over the old allocation, clip NALs or label this full runtime integration.

Camera last controller OFF after23h and unchanged RAM75 recovery/C1 blocks are
parent context, not new observations. No native queue/recovery lease/HOLD change.
Next permissible handback action is ordinary independent source/evidence review
and parent disposition, without rerunning the normal process or adopting runtime.

## Delivery anchors

| File under candidate directory | SHA-256 |
| --- | --- |
| span_hub.c | `40c6d84c1e78c29a78384bee4dc1f435a53032f31655e122949921275a5c2289` |
| span_hub.h | `8c45f279a09dce01b53d928e2fa024663093f6955c469d0b71bb18ba72947c47` |
| host_normal.c | `7b5ac823f1cad92181f7d870a92d306f855c19cb339052e35f39a5b8390e685f` |

Accepted packetizer C/header pins remain
`a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e` /
`41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91`.
DELIVERY_PINS.json anchors all new public source/docs plus build/sole-HOST reports;
ignored runs retain exact local output evidence, not publication/release approval.
