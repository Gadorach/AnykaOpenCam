# RP-09 / B07 — span-aware packetizer author handback

**Delivered:** bounded production span packet builder, HOST/ARM compile PASS and
ONE normal synthetic HOST observation PASS. **Not independently reviewed/adopted;
allocation-cap integration, native and product acceptance remain OPEN.**

All new files are confined to `apps/rtsp-auth/rtp-span-candidate/` and this
qualification directory. No existing framework/runtime/bootstrap/source/seal,
central plan, AGENTS, Git/index or hardware was changed. Parent context says
camera OFF after finalized23h and RAM75 recovery/C1 BLOCKED; this lane made no
current-state query. User-reported prior retirement commit `a3d2b75` is context,
not adoption/execution by this lane.

## Actual API and borrowing contract

`rtp_h264_spans.h` exports `camera_rtp_h264_spans_packet_v1` with the real
`camera_rtp_session` and `CAMERA_RTP_PACKET_BYTES` from the original framework.
Inputs are session, timestamp, borrowed `camera_rtp_h264_span_v1 {data,size}`
array/count, exact total, last and offset; outputs are owned packet, packet_bytes
and next_offset. Source/header provenance remains GPL-2.0-only; see the local
`LICENSE_NOTICES.md`. The original contiguous builder is neither called nor linked.

- Count1..16, each span1..131072 bytes, explicit total1..1048576 equal to checked
  sum. Entire descriptor shape checked before NAL header read or output mutation.
  Original session/header/last/offset admission and single-NAL/FU-A wire semantics
  preserved by source inspection. All refusals precede packet/scalar writes.
- No allocator, full-AU temporary, cached iterator, callback or retained pointer.
  Per call only NAL header plus current payload is read/copied; payload<=1388
  single or1386 FU-A, output<=1404. Session/caller offset unchanged; no sequence
  advance. Caller commits next offset/sequence only after full packet transmission.
- Array/source/session/output/scalars are trusted valid, stable, serialized and
  nonoverlapping memory; no hostile-pointer validation, DMA synchronization or
  immutable-alias proof. An owned pending packet survives partial writes without
  source reads, but **every future packet read still needs the actual source
  lease**. Nothing here authorizes source release, ACK inference, END or drain.

See [CONTRACT.md](CONTRACT.md) and ordinary author-only
[AUTHOR_AUDIT.md](AUTHOR_AUDIT.md). No blanket runtime equivalence claim: only the
single long normal case below executed. Small NAL/last0/refusal combinations were
not executed; no negative/adversarial/fuzz/mutation/penetration sweep selected.

## Actual build and sole HOST result

Fixed compile invocation (completed once, not a request to rerun):

```text
python3 apps/rtsp-auth/rtp-span-candidate/compile01.py
```

`runs/build01/REPORT.json`:3 actual C compilations and20 successful tool commands.
Production C compiled once HOST GCC16.2.1 and once retained musl GCC14.3.0 ARMv5TE
soft-float, using real framework/libc headers, C11 and warnings as errors. Harness
compiled/linked HOST only. Original packetizer was a pinned reference, not compiled.

| Artifact | Size | Role |
| --- | ---: | --- |
| `objects/host-spans.o` |2392B| Actual new production HOST C object |
| `objects/arm-spans.o` |1664B| Actual new production ARM ELF32 REL/EABI5 v5TE object |
| `HOST-NORMAL-NOT-NATIVE` |16312B| Standalone HOST observation executable |

Both production objects export only the new builder and reference only `memcpy`.
Compiler stack usage112 HOST/48 ARM bytes is static frame evidence, not libc/
transitive stack or native ABI proof. No ARM program was linked or executed.

After source/harness/runner audit recorded in `PRECALL_AUDIT.json`, the ONLY
candidate execution was the fixed HOST observation, at2026-10-24T03:14:10Z:

```text
python3 apps/rtsp-auth/rtp-span-candidate/host_once.py
HOST_NORMAL_PASS packets=126 nal_bytes=174080 payload_bytes=174079 wire_bytes=176347 boundary_crossings=2 final_offset=174080 final_sequence=80 NOT_NATIVE
```

One public numeric-generated VCL IDR header0x65 plus formula body; not footage or
decoder-qualified slice syntax. Three separate source objects65536+65536+43008
=174080 bytes; largest65536, no full-NAL allocation/object. One separate1404-byte
packet, so source+packet object sizes sum175484 before descriptors/scalars/padding/
process runtime; not a whole-process allocation or locked-memory acceptance claim.
Three descriptors48 bytes on this HOST; main's compiler frame368 bounded bytes.

Every emitted header and body byte checked incrementally from fixed protocol
expectations/formula, no full reconstructed NAL or second >128KiB buffer. Both
source boundaries crossed inside packet payloads.126 packets cover174079 body
bytes (125*1386+829) plus the FU-reconstructed NAL header. Every marker/end/length/
offset/session value checked before synchronous harness offset/sequence commit;
sequence65490 wraps to80. Output bounded1404 per packet. No sockets or partial-write
behavior executed, no ACK assertion. Exit0, empty stderr, exact scalar stdout;
source/build/tool/input pins verified before and after. No failure/retry/second run.
Payload bytes were neither logged nor hashed. No other candidate was called.

## Evidence and remaining work

- `runs/build01/`: exact per-command argv/environment/cwd/returncode/logs, real
  dependency `.d` files, stack reports, objects, HOST ELF and link map/trace;
 14 selected source/reference inputs,46 deduplicated compiler dependency paths,
 27 tool/link-input records,54 pre-report artifacts. Not a whole compiler closure.
- `PRECALL_AUDIT.json`: audited source/contract/report pins and supplemental actual
  ARM `.br_real`/Python interpreter pins (supplement sampled before HOST call,
  not a retrospective complete compiler-execution trace).
- `runs/host01/`: exclusive attempt, exact single invocation, scalar outputs and
  `OBSERVATION.json`. Fixed directories refuse overwrite. Do not rerun the harness.
- `DELIVERY_PINS.json`: compact public delivery/build/observation pointers and
  hashes. Generated objects/ELF/logs remain local ignored evidence, not source
  delivery for Git/publication. No Git operation was performed.

The actual owned-runtime still has `malloc(1048576)` output and a separate1MiB
copied hub, fixed-buffer prefault/accounting, contiguous `camera_packet` and
`au_cursor.nal` representation, and calls the original contiguous packetizer.
**This addition changes none of them.** Slicing their existing large backing into
small views would not satisfy B07. No lowered codec ceiling, clipping, cap increase,
fake provider readiness, whole-AU TLS copy or release-policy change is made.

Remaining real feature work: independently bounded output/hub backing (and kernel
output/raw backing obligations), span-aware producer/hub/consumer lease API and
pending cursor adoption, actual prefault ledger integration, complete source
END/release/drain/external-owner/recovery lifetime, native DMA/cache safety and
combined load/TLS/headroom. Multi-VCL AU semantics/RTCP/browser streaming remain
separate open requirements. No listener, encoder/provider/bootstrap/provisioner,
credential, file-retirement, OS RNG or hardware execution occurred.

**Next eligible task: parent independent ordinary review of this exact candidate
and evidence, without another functional run.** Parent adoption/any later bounded
integration must be separately selected; no automatic integration/native queue.
