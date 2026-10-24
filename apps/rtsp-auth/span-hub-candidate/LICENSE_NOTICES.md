# Scoped derivative provenance

`span_hub.c` and `span_hub.h` adapt the one-slot publication, per-consumer
pending/acquired bits, keyframe resync and stop/drain semantics of
`framework/src/packet_hub.c` and `framework/include/camera/packet_hub.h`.
Those originals carry SPDX `GPL-2.0-only`, preserved in these derivatives.
New independent chunk ownership and the packet wrapper use the real accepted
`../rtp-span-candidate/rtp_h264_spans.h` descriptors and production packetizer,
which include the actual framework RTP/backend enums, results and session types.
No original or accepted source/header is replaced or edited.

The new observation harness and small offline scripts are also GPL-2.0-only.
This notice changes no other license and grants no rights over acquisitions.
No firmware, private device data, credentials or payload dumps are included.
