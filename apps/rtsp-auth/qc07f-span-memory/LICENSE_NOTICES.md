# Source provenance

SPDX-License-Identifier: GPL-2.0-only

`memory_reserve.{c,h}` and `memory_runtime.{c,h}` are focused derivatives of
`apps/rtsp-auth/qc07f-owned-runtime/native/` with the same filenames.
`memory_reader.c` and `handoff.h` are byte-identical local include/build inputs
from that directory. Preserve those files' GPL-2.0-only notices.

Uses (does not copy/change) the separately reviewed
`apps/rtsp-auth/span-hub-v2/span_hub.{c,h}` and
`apps/rtsp-auth/rtp-span-candidate/rtp_h264_spans.{c,h}`, and the actual framework
headers. Their original notices and provenance remain in their directories.
The compile recipe is GPL-2.0-only. No firmware, private data or images included.
