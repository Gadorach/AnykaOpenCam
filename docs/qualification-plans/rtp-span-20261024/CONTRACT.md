# RP-09 / B07 — borrowed-span H.264 one-packet candidate

Written before source, 2026-10-24. Author candidate; independent ordinary review
and parent adoption required. STREAM-01/03/05, CAP-01/05/07 remain open.

## Boundary and API

Add only `apps/rtsp-auth/rtp-span-candidate/` production C/header and local
offline evidence in this directory. Do not change the original framework or
owned-runtime consumer. Derive wire/offset semantics from GPL-2.0-only
`framework/src/rtp_h264.c`, using the actual `camera/rtp_h264.h` session and
1404-byte packet constant, not a replacement ABI.

`camera_rtp_h264_spans_packet_v1` accepts a readonly borrowed descriptor array
of 1..16 spans, each with nonnull data and size 1..131072, and an explicit total
1..1048576 equal to a checked sum. One NAL, no Annex-B prefix. Validate every
descriptor and sum, session, scalar pointers, header and offset before any output
write. Return CAMERA_INVALID on refusal, leaving packet and scalar outputs
untouched. No hostile-pointer or overlap detection promises: descriptor array,
source spans, session, packet and separate scalar outputs are trusted valid,
nonoverlapping, stable, serialized memory. Const is not an immutable-alias proof.

Header forbidden bit must be clear, type 1..23; channel <=254, payload type <=127,
last 0/1, offset < total. Single NAL <=1388 accepts offset 0 only. Larger NAL:
offset 0 becomes 1, FU-A carries at most 1386 source bytes, start at offset 1,
end at exact NAL end, RTP marker only on last+end. Single payload <=1388 and
total output <=1404. Preserve original timestamp/SSRC/sequence/network byte
order/interleaved length semantics. Return next_offset without changing session
or caller offset. Check complete descriptor shape before reading NAL header;
copy only header/current payload across spans, never reconstruct the whole NAL.
No allocations, callbacks, cached iterator, retained descriptors or native effects.

Caller supplies a real encoded lease valid for every future source read, including
all calls after partial writes. A complete owned 1404-byte pending packet survives
partial writes independently of source. Commit offset and sequence only after the
whole packet is transmitted; no ACK/network/release inference. This function
does not transfer ownership, pin backing, synchronize DMA, admit release, or
implement END/STOP/drain. Per-call borrowing is not an AU lifetime mechanism.

## Selected evidence only

Offline compile actual new C with real framework headers: HOST and retained
musl ARMv5TE soft-float, warnings as errors. Short fixed recipe, exclusive fresh
output directory, exact argv/tool/dependency/source/log/artifact hashes. No old
builder, image, diagnostic framework or full compiler-closure/native-ABI claim.
Only HOST links a standalone observation harness; no ARM program executes.

After ordinary author source/harness/runner audit, make ONE normal HOST call:
one public numeric-generated 174080-byte VCL IDR, three separate source objects
65536+65536+43008 bytes, no full-NAL object or >128KiB object. One owned 1404-byte
packet reused synchronously. Incrementally compare every payload byte with the
formula and all RTP/FU-A headers, offsets, sequence, marker and lengths. Exercise
both source boundaries in packet payloads; commit sequence/offset only after each
successful synchronous packet check. Print only scalar status/counts/bytes, no
payload or payload hashes. Failure stops, no retry/negative/adversarial matrix,
additional cases, hidden CLI probes or candidate calls. Observation is synthetic
packetization, not valid decoder media or RTSP/native/security acceptance.

## Remaining integration

Actual owned-runtime `prepare_output` still mallocs 1MiB and its memory ledger
prefaults that output and the separate 1MiB hub. `camera_hub_publish` still copies
contiguous NALs; `send_access_unit` still stores contiguous NAL pointers and calls
the old packetizer, retaining an owned packet through partial writes. None changes
here. Descriptors over an existing 1MiB allocation do not satisfy the allocation
cap. Real independently bounded backing, span-aware hub/producer/cursors, kernel
output/raw ownership, prefault ledger, source END/release/drain/external recovery
owner and native combined load/headroom remain unqualified. No clipping, smaller
codec limit, raised cap, fake readiness, TLS copy or automatic integration queue.
Parent alone owns hardware; no hardware/current-state operations in this lane.
