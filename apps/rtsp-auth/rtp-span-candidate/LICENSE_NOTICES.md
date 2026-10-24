# License and derivative provenance

Production `rtp_h264_spans.c` and `rtp_h264_spans.h` derive from the project's
`framework/src/rtp_h264.c` and `framework/include/camera/rtp_h264.h`, respectively.
Both originals carry SPDX `GPL-2.0-only`; that license is preserved here. The
production header includes the original real framework header and its backend
types. Packet formatting, validation limits and build-one-packet offset semantics
are adapted from that original source; the new bounded span copy does not call
or reconstruct input for the original contiguous packetizer.

New harness and fixed offline scripts are also GPL-2.0-only. This does not change
any original source or license or grant a license over unrelated acquisitions.
Original input hashes and actual consumed headers are recorded in build evidence.
