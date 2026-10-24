# Contract before source — encoded H.264 output spans

2026-10-24. B07 / RP-04/05/09; CAP-01/05/06/07 + ENC-03/05.
**Isolated SOURCE candidate, compile-only; ordinary independent review and parent
adoption required before use.** No hardware, ioctl, candidate/test/function call,
kernel/module/image build or application adoption is selected.

## Actual source connection selected

Use the seven-file accepted query-preserving source baseline at
`docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate/`.
Driver SHA256 `9bf16b1175befb091d6c61c3ac573121378e1d8c403eed21806a3e8fc80fc946`;
prior source seal identity
`ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`.
The retained 505-line original, query overlay, telemetry and DMA-integrity helpers
have been read. The real dispatch/lock/session/stage and H.264 reference/copy path
permit this bounded addition: no invented provider, pointer or hardware stub.

One actual `encode_h264` body remains. Add a typed destination kind and optional
validated, kernel-resident eight-descriptor sink. Legacy callers explicitly select
contiguous output; they retain their original capacity/pointer checks and single
copy. New callers select spans and never need a contiguous output pointer. The
register sequence, input copy, two reference allocations, output DMA allocation,
guard extents, reset retirement, status/length checks and QP/profile remain intact.

## New fixed ABI

Encoder-local command number `0x67` was absent from the inspected real encoder
UAPI and accepted open-query overlay. Preserve exact old commands 0x61..0x66,
including the two distinct full commands with number0x65. New command:
`_IOWR('Q',0x67,struct qc_hwenc_h264_spans_job)`, exactly `0xc0805167`.

All fields are fixed `__u32`, alignment4, total128 bytes, no native pointer/u64:

| Offset | Field |
|---|---|
| 0,4,8,12 | version=1, struct_bytes=128, keyframe=0/1, span_count=8 |
| 16,20,24,28 | width=1280, height=720, input_bytes=1382400, input_ptr |
| 32..95 | eight descriptors: user_address, bytes (8 bytes each) |
| 96,100,104 | output_bytes, status, frame_num (must be zero on input) |
| 108..127 | reserved[5], all zero |

Each descriptor must designate exactly131072 bytes; checked capacity is exactly
1048576 bytes. These are CPU uservirtual addresses only, never physical/DMA
addresses or imported DMA/user-MMIO interfaces. No arbitrary register control.
Old images return ENOTTY; no fallback, renaming/query77/78/88 or diagnostic client.

## Snapshot and preflight before hardware

Under the existing encoder mutex/stage and calling-file numeric session, validate
the full control address/range and snapshot the entire request exactly once with
`copy_from_user`. Validate every version/size/shape/count/reserved/output-on-input
field, nonzero addresses, no u32 endpoint overflow and `access_ok` for full source,
control and all output ranges. Reject any pair of output virtual ranges overlapping
each other, the full raw source range or full request range. Bounded loops only.
All descriptor use thereafter refers to this immutable kernel stack snapshot.
Known malformed/preflight failures do not clear an existing reference owner or
reach hardware. Other-reference-owner BUSY and ownerless-P ENODATA remain enforced.

`access_ok` is an address-domain check, NOT writable-page reservation, VMA/alias
provenance, exclusive custody, clock/deadline, fault-free or mapping-stability
proof. Caller must supply stable ordinary CPU memory, not MMIO, and serialize
aliases/mappings for the complete ioctl. Separate virtual addresses can alias;
this increment does not prove independent allocations or physical disjointness.
Concurrent unmapping/remapping or user mutation can still fault/change pixels;
no pin/import/zero-copy claim. Actual uaccess errors are terminal job failures.

## Actual encode, delivery and reference boundary

After successful real `codec_reset_retirement`, `dma_rmb`, all input/output/ref/
aux guards, poll/status success and nonzero aligned length <=OUTPUT_LIMIT, copy
actual `enc.output` bytes in descriptor order with cumulative source offset.
Each `copy_to_user` is <=131072; final partial chunk copies only valid bytes,
never unused tails. No new full-AU buffer or large allocation. First copy fault
stops immediately with EFAULT; no retry, repair, padding or fabricated count.
One OUTPUT_COPY timing sample covers the whole scatter job, with actual copied
byte count on failure and actual total on success; existing contiguous telemetry
semantics stay unchanged. Existing hardware length history is not delivery proof.

Only after data success populate output_bytes/status/frame_num in the snapshot,
then copy the complete control result. Return0 and commit h264_owner/frame_num/
ref_index (plus existing owner-generation observation) only after that copy0.
P numbering/ref selection match legacy STREAM (16-bit wrap, alternating refs).
Any failure after entering the real encode body invalidates the new route's
logical reference owner, including quarantine; quarantine/fault_session/DMA state
and retained backing are never cleared. This is at least as conservative as the
old stream route, whose quarantine failure quirk stays untouched. Final release
still clears its file pointer before freeing private numeric session data.

On EFAULT some data and/or metadata bytes may already be visible. No error return
is a valid frame/result; discard partial output and require a fresh IDR for any
future admitted encode. Even complete-looking metadata is invalid without syscall0.
No automatic retry or reset. Preflight-only errors are distinct and preserve the
prior logical reference; caller cannot infer actual job phase from errno alone.

## Compile-only evidence and excluded claims

Fresh refuse-overwrite recipe: HOST and retained musl ARM declarations/static ABI
assertions, genuine driver dependency preprocessing and NEW Clang23.1.3 whole ARM
driver object using retained actual Kbuild flags/config/headers (no make/generator/
linker/image). Save exact argv, timestamps, tool/source/dependency/log/artifact
pins and before/after old-source checks. No synthetic driver/test harness or ELF
execution. Retained generated metadata names23.1.1: paired ABI remains OPEN.
No full toolchain closure claim. Failed runs remain immutable; no retry loop.

The actual RTSP native import still constructs contiguous `QC_HWENC_H264_STREAM`
and the native adapter still calls malloc1048576. Hub v2's eight128KiB chunks and
RTP spans supply bounds context only; no source edit/adoption there. This kernel
sink alone does not connect producer/hub, change the native controller, strip
Annex-B, qualify2MiB kernel input/output/ref allocations, raw/user/process2MiB,
40MiB/CMA/combined TLS load, independent recovery, native entropy/ABI, image/source
pairing or the overall128KiB allocation cap. Query78 remains PARKED. No Git/index,
central plan, private/device/network IO or parent-only hardware action.
