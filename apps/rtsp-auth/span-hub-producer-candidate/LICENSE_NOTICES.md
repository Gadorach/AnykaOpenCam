# Scoped derivative provenance

`span_hub.c` and `span_hub.h` derive from frozen GPL-2.0-only
`../span-hub-v2/span_hub.c` and `span_hub.h`. Original one-slot pending/acquired,
resync/STOP semantics derive from `framework/src/packet_hub.c` and
`framework/include/camera/packet_hub.h`. SPDX is retained. The unchanged
`../rtp-span-candidate/rtp_h264_spans.c/.h` supply the actual accepted production
packetizer and framework descriptors/types. No upstream source is changed.

New focused source and compile-only recipe are GPL-2.0-only. This notice grants
no new rights over other sources, acquisitions or private data. No firmware,
credentials, footage, captures, harness, test runner or native controller included.
Version3 layout/API is an exclusive source replacement retaining legacy names;
it is NOT binary compatibility, old-layout reuse or dual-link permission.
