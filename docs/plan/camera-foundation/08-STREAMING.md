# 08 — Shared media hub, RTSP/RTP, consumer isolation and reconnect

Entry: [index](00-INDEX.md). Prefix: **STREAM**. Depends: CORE, CAP/ENC/NET for
production performance; coordinate SEC and WEB before **G3**.

## Baseline

Shared RFC6184 single-NAL/FU-A, packet hub and fair nonblocking packet cursors
are implemented. One1MiB hub slot and~1404-byte cursor per peer avoid four large
AU copies. Current producer waits for delivery before producing again. Up to
four lab clients, explicit2s whole-AU budget and optional TCP coalescing work in
finite tests. SPS/PPS and source checks are retained. RTSP is still NOAUTH LAB.
Strict host receiver rejects incomplete AUs before decoding, but is not the
on-device browser service. Current API assumes one VCL NAL per produced frame.

## Tasks

- [ ] **STREAM-01 — Complete AU contract.** Represent parameter sets, one or
  multiple VCL NALs, keyframe/IDR, profile generation, capture/encode timestamps,
  size and error status without assuming one frame equals one NAL. Bound NAL/AU
  counts and sizes. Producer release invalidates data only after owned hub copy
  or an explicitly shared encoded lease is secured.
- [ ] **STREAM-02 — Asynchronous bounded fan-out.** Decouple source/encode from
  network and recorder delivery using a measured encoded pool, not unbounded
  queues or a full-sized duplicate per consumer. Apply per-consumer deadlines,
  retained leases, cancellation and keyframe resync. Release all old references
  on disconnect/generation changes. Recorder admission/drops are explicit, not
  silently inherited from live-preview drop policy. Keep the memory design in
  CAP current, including socket and mux buffers.
- [ ] **STREAM-03 — Backpressure correctness.** Slow RTSP/browser/NVR/storage
  must not stall capture or overwrite data held by another consumer. Do not
  abandon a partially written interleaved packet and continue on that TCP
  connection; close or use a validated complete-unit policy. If dropping P-frame
  chains, wait for IDR and supply current parameters. No stale frame marked live.
- [ ] **STREAM-04 — RTSP state/parser hardening.** Bounded request/body parsing,
  mixed interleaved control traffic, fragmentation/pipelining, case-insensitive
  headers where required, valid sessions/CSeq, legal SETUP/PLAY/PAUSE/TEARDOWN,
  keepalive and cleanup. Associate paths with stable profiles. Integrate SEC
  authentication before public media/control. Reject malformed/oversized input
  without heap growth, cross-client state or leaking credentials.
- [ ] **STREAM-05 — RTP/RTCP/time.** Use per-session sequence/SSRC, correct marker
  semantics, timestamps from actual capture cadence and wrap-safe arithmetic.
  Implement required RTCP sender/receiver reports and validated NTP mapping when
  wall time exists. Parameter changes restart decoder config coherently. Test
  loss/jitter and timestamp discontinuities; do not stamp all frames at nominal
  FPS while actual output drifts.
- [ ] **STREAM-06 — Transport compatibility.** Qualify TCP interleaving first.
  Determine actual NVR/browser bridge requirements before enabling UDP/multicast;
  if added, implement bounded ports, RTCP, loss/reorder and session lifetime.
  Advertise only qualified transports; unsupported SETUP returns a proper error.
- [ ] **STREAM-07 — Authenticated fan-out.** Independent RTSP credentials from
  UI; challenge/replay/logout/password-change handling from SEC. One user's
  session does not authorize another client's stream or arbitrary profile change.
  Rate-limit authentication and IDR request floods within CPU/memory budget.
- [ ] **STREAM-08 — Snapshot/audio/mux integration.** Fresh snapshot service
  shares qualified source ownership and encodes explicitly if needed. Add AUDIO
  timestamps/SDP/tracks without blocking video. Recorder and WEB muxing consume
  the same encoded AU stream. Empty, stale and source-failed states are explicit.
- [ ] **STREAM-09 — Acceptance tests.** Native decode with independent clients,
  exact unit/sequence/CRC correlation, multiple GOPs/profile changes, late joins,
  silent readers, abrupt close, bad credentials, parser faults, source timeout,
  producer restart, network outage and memory-pressure admission. Retain
  incomplete wire evidence while keeping decoder input complete. Count unique
  decoded frames, not just socket writes or a process that stays alive.

## Gate

G3 requires authenticated multi-client RTSP plus WEB on-device live video at G2's
qualified profile, bounded latency/queue memory, truthful freshness and stable
reconnect. Freeze tested client count and normal/stall latency bounds before
campaigns. Minimum combined target: NVR+browser+recorder active, with an extra
stalled client unable to stop them. Final long-soak/fault gate is QUAL, not the
existing15-minute lab result.
