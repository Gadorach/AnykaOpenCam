# Scoped derivative provenance

`span_hub.c` and `span_hub.h` are the focused R1 derivative of frozen
`../span-hub-candidate/span_hub.c` and `span_hub.h`. They retain the original
GPL-2.0-only SPDX and derive one-slot pending/acquired/resync/stop semantics from
`framework/src/packet_hub.c` and `framework/include/camera/packet_hub.h`.
The unchanged accepted `../rtp-span-candidate/rtp_h264_spans.h` and production C
provide actual framework RTP/backend descriptors, results and session types.

The new small compile-only recipe is GPL-2.0-only. No observation harness or
runtime runner is included. Existing V1 names are for exclusive source replacement,
not binary ABI compatibility or permission to link both hubs/old combined RELs.
No original/accepted source is edited. This notice changes no other license and
grants no rights over acquisitions. No firmware/private data/credentials/payload
dumps are included.
