# RP-09 / B07 — independent ordinary RTP-span review

2026-10-24. **Conditional narrow SOURCE/API/bounds/wire-semantics sufficiency;
no production-source correction required in this scope.** This is not parent
adoption, complete allocation-cap compliance, runtime/native RTSP, valid decoder
media, DMA/lifetime or product acceptance. Parent independent disposition remains
required; no automatic integration or hardware work follows.

## Scope and exact inputs

Read all six files in `apps/rtsp-auth/rtp-span-candidate/`, the author's CONTRACT,
HANDOFF, AUTHOR_AUDIT, PRECALL_AUDIT and delivery/build/sole-HOST evidence, actual
original RTP C/header/backend types, actual framework hub, and owned-runtime
`send_access_unit`, output allocation and prefault sites. Relevant STREAM/CAP
plans and current execution entry were read. Review effects: public read/hash,
JSON/log/dependency/link-map and ELF/symbol metadata inspection only, plus this
new review file. No candidate/helper import, build/link/call/test/fixture,
original packetizer execution, network, private/device/current-state query,
Git/index/central-plan change or subworker. Further negative/adversarial/fuzz/
mutation testing remains deferred, not passed.

SHA-256 below was independently read from actual files. `C` means
`apps/rtsp-auth/rtp-span-candidate/`; `D` means
`docs/qualification-plans/rtp-span-20261024/`; `R` means
`apps/rtsp-auth/qc07f-owned-runtime/`.

| Input | SHA-256 |
| --- | --- |
| C/rtp_h264_spans.c | `a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e` |
| C/rtp_h264_spans.h | `41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91` |
| C/host_normal.c | `eb1e237785e33669ee5ae77266d3cf7c47b64757a4ebdbf87e697f377cd566e6` |
| framework/src/rtp_h264.c | `6f0720ed4fd3c6b5e325094cb2b7422003efb1f5cae41b7011974de3fae51e1a` |
| framework/include/camera/rtp_h264.h | `730e0b95cc8599fc6ef8f148aa083db944de1a1910d632d9d3d5daeb4bdeb4ad` |
| framework/src/packet_hub.c | `7f3324ce5e180f637a2d0fb0e296f7050ebcb89b99f93918e7cf15521ebf267e` |
| R/server/rtsp_server_candidate.c | `fb47ac06be587b8bac793c272c804ece0220601e4a1d0810164295b0c4dd5ce1` |
| R/native/native_adapter.c | `1fcd2a68750c64a0f1d1b3a96f1124f6f21369b9eb19f4b89a9bcbd1774754bf` |
| D/DELIVERY_PINS.json | `c120807c6385dbb5ea376fadbc5872d3017aa89f57a84254632398e29dc1f50f` |
| D/runs/build01/REPORT.json | `41774e80a0928a114e569a0d739a4eaad119f7bd942ee1f9687790a55f406b0b` |
| D/runs/host01/OBSERVATION.json | `5ee26f1952caf3af1d5c5cc54cc9a300894961294e7ec332894be474f1844d9b` |

Read-only `jq` extraction and `sha256sum --check --quiet` succeeded for all
169 overlapping records: 17 delivery + 14 selected inputs + 46 dependency paths
+ 27 tools/link inputs + 54 pre-report artifacts + 11 precall pins. This verifies
the recorded bytes, not historical execution completeness or a compiler closure.
The anchored delivery manifest also pins both scripts, notices and author docs.

## Source/API/bounds findings

- **Admission before writes:** C lines32–46 reject null required pointers,
  count outside1..16, total outside1..1048576, any null/empty/>131072 span, sum
  overflow (`size > SIZE_MAX-sum`), sum/explicit-total mismatch, offset>=total,
  nonzero small-NAL offset, last>1, channel>254, payload type>127, forbidden NAL
  bit or type outside1..23. All descriptors are checked before reading source
  header byte0. Every refusal precedes packet and scalar writes; no refusal path
  follows the first write. Nonnull checks do not validate addresses or capacity.
- **Range proof:** exact nonempty descriptor sum and offset<total make the
  initial subtraction skip terminate inside count. Payload is positive and at
  most total-offset. Each copy consumes a positive minimum of remaining span
  extent and remaining payload, so it progresses without crossing a source
  object's extent. A final index increment may reach count only with bytes=0;
  it is never dereferenced then. Local offsets use subtraction, not unchecked
  prefix addition; next_offset=offset+payload<=total<=1048576 cannot overflow.
- **Wire semantics:** compared with original C lines10–25, the small-NAL path
  retains offset0, copies the NAL header/body and carries <=1388 bytes. Long-NAL
  initial offset0 becomes1; FU-A source body <=1386, indicator `(h&0xe0)|28`,
  type `h&31`, start iff offset1, end iff exact remaining payload, marker iff
  last&&end. RTP v2, channel, payload type, supplied timestamp, SSRC and sequence
  are preserved; integer fields use network order. Interleaved length is
  header+payload-4 (<=1400), complete packet<=1404. Interior long-NAL offsets
  retain original acceptance; this API does not prove prior transmission or
  cursor continuity. Single/last0/refusal semantics here are SOURCE conclusions,
  not executed coverage.
- **Mutation/ownership:** actual framework session/constant/result ABI is used.
  Session and caller's value offset remain unchanged; no sequence advance,
  callback, allocator, full-AU temporary/copy, cached iterator or retained state.
  Only packet prefix and separate length/next outputs are produced on OK; bytes
  beyond packet_bytes are unspecified. INVALID preserves previous outputs, which
  must not be mistaken for a newly valid result. The valid/stable/serialized/
  disjoint descriptor/source/session/output/scalar contract is essential, not
  hostile-pointer, alias, source-object allocation, DMA/cache or lease proof.
  Every future call still needs a valid real source lease and coherent NAL.
  An owned pending packet survives partial writes independently; it grants no
  whole-AU release/END/drain permission. Caller commits offset/sequence only after
  complete packet transmission, without inferring peer ACK.
- **License:** scoped GPL-2.0-only markings and actual original source/header
  attribution are retained; no unrelated-source license grant is asserted.

## Retained build and one HOST observation — not rerun

The frozen recipe/report contain exactly three C compiles and20 return0 tool
commands: production HOST GCC16.2.1 and musl GCC14.3.0 ARMv5TE soft-float, plus
HOST-only normal harness and its link/inspection. Actual dependency files use
real framework/libc headers. The three compile logs are empty. Direct read-only
`readelf -h -A`/`nm -g` agree with retained metadata: HOST production ELF64 REL,
ARM ELF32 REL/EABI5/v5TE; each exports only the new function and has only `memcpy`
undefined, no main or allocator. HOST executable alone has main. Link argv,
trace/map and symbols show only harness/new packetizer project objects, not the
original packetizer. Stack reports112 HOST/48 ARM production and368 bounded HOST
main bytes are local compiler-frame evidence, not transitive/native stack proof.
Wrapper `.br_real`/Python supplemental pins and sampled link inputs are not a
complete compiler-execution closure. No ARM executable was linked or run.

The audited fixed runner has one exclusive output directory, one exact HOST
invocation,10s timeout, pin checks before/after and no retry/additional cases.
Saved attempt/invocation/observation record ONE normal process at03:14:10Z,
exit0, empty stderr, exact scalar stdout. The harness's sole174080-byte synthetic
case uses three distinct65536/65536/43008-byte source objects and one1404-byte
packet, not views over or reconstruction of a full-NAL object. Formula checks
cover every source body byte and all18 wire-header bytes for126 packets;
174079=125*1386+829, wire bytes176347, two within-payload span crossings, final
offset174080 and sequence65490 wrapping to80. Header reconstruction checks0x65.
There are no sockets/partial writes/decoder checks and no small-NAL, last0 or
refusal runtime coverage. This is public formula packetization, not footage,
valid decoder media, native RTSP or actual backing-cap qualification. Review
made zero candidate calls; recorded history is not an external execution trace.

## Conditions, documentation note and integration gaps

Maintain the memory/lease/commit preconditions above and exact scoped pins. No
ordinary source correction was found. **Nonblocking provenance wording only:**
HANDOFF line11 calls `a3d2b75` “User-reported”; supplied context identifies it as
PARENT-reported. This review does not verify that commit or retirement execution;
parent should correct contextual disposition, without changing reviewed source.

Actual runtime remains unchanged: native_adapter lines28–29 malloc/prefault1MiB
output and line91 accounts1MiB hub; framework hub line68 copies contiguous NALs;
server cursor lines284–288 holds contiguous pointers, line351 calls the old
packetizer, lines366–368 commit only after a complete pending packet. This new
function does not remove those allocations or integrate span backing. Small views
over existing1MiB objects would not meet the128KiB allocation cap. Independently
bounded producer/hub backing, span-aware packet/lease/cursor adoption, kernel/raw
ownership and DMA/cache synchronization, prefault ledger, source retirement/drain/
external recovery, combined TLS/load/headroom, multi-VCL/RTCP and product gates
remain open. No limit reduction, clipping, cap raise or fabricated source is
accepted. Camera OFF after23h and RAM75 recovery/C1 BLOCKED are parent context,
not fresh observations here; no hardware/native queue is selected.
