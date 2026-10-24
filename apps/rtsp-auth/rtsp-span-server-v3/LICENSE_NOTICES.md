# Scoped derivative provenance

This v3 is an exclusive residual-R1 source replacement of the frozen
`../rtsp-span-server-v2/`, retaining the SAME exports/types/include guard.
Only process_input and serve_prestarted_body observed-uncertainty fences change;
the accepted local v2 pause_client, close_client and finish_au changes remain exact.
V1/v2 remain frozen NOT ADOPTED; this derivative awaits independent parent review.
`rtsp_span_server.c` is a focused GPL-2.0-only derivative of the actual
`../qc07f-owned-runtime/server/rtsp_server_candidate.c`, preserving its real
authentication/lifecycle/listener server body. The new header and offline recipe
are GPL-2.0-only. Original license texts remain at
`../qc07f-owned-runtime/licenses/`; no existing notice is replaced or relicensed.
Original runtime/backend/crypto headers/implementations are referenced, not copied
into a new runtime tree. See the precise forward/inverse source diff in the
qualification directory. The accepted `../span-hub-v2/` and
`../rtp-span-candidate/` retain their own provenance/notices. V1 hub is NOT used.

This source API is a separate entry, not a drop-in binary replacement. Never link
the old server unit/combined REL alongside this derivative. Original framework,
crypto, runtime, provisioner, hub and build tools remain unchanged. No acquisitions,
firmware, credentials, photographs, audio/video payloads or private evidence are
included; no license over those is granted.
