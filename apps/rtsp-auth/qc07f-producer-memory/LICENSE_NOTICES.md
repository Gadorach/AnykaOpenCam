# Source provenance

SPDX-License-Identifier: GPL-2.0-only

`memory_reserve.{c,h}` are focused derivatives of the accepted same-named files
in `apps/rtsp-auth/qc07f-span-memory/`, themselves derived from
`apps/rtsp-auth/qc07f-owned-runtime/native/`. `memory_runtime.{c,h}`,
`memory_reader.c` and `handoff.h` are byte-identical to the accepted span-memory
files. Preserve original GPL-2.0-only notices and historical provenance.

Uses (does not copy/change) the separately accepted complete producer-layout
`apps/rtsp-auth/span-hub-producer-candidate/span_hub.{c,h}`, accepted
`apps/rtsp-auth/rtp-span-candidate/rtp_h264_spans.{c,h}` and actual framework
headers. Their notices/provenance remain in their directories. Exclusive source
replacement, not binary compatibility or permission to reuse old-layout consumers.
The fixed compile recipe is GPL-2.0-only. No firmware/private data/images included.
