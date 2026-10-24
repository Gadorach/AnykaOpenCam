# Ordinary author audit — before sole HOST observation

2026-10-24. Author inspection, **not independent review**. Read the actual original
RTP header/source, streaming/capture-memory plans and memory/backpressure rules;
read actual owned-runtime `send_access_unit`, `prepare_output` and fixed-buffer
prefault code, and actual framework hub publish/release code. Contract was written
before production source. Read back new production/header/harness/fixed scripts
before compilation/call. No negative/adversarial/mutation testing selected.

## Production reasoning

- Actual original `camera_rtp_session`, result codes and 1404-byte constant are
  included, not copied as a competing ABI. Original source/header are reference
  inputs, neither changed nor executed/linked into this observation.
- All null/count/total checks precede descriptor reads. All 1..16 descriptors are
  checked for nonnull data, nonempty size <=131072 and `SIZE_MAX-sum` overflow;
  complete exact sum, original offset/session/last checks precede first source
  header read. Header forbiddentopbit/type checks precede every output write.
- From nonempty spans, exact total and offset<total, the range-copy initial skip
  terminates at a valid descriptor. Positive payload <=total-offset bounds every
  copy. Advancing past the final descriptor happens only when remaining bytes=0;
  no dereference after loop termination. No failure branch after output begins.
- Small-NAL offset stays 0; payload=total<=1388. Long-NAL offset 0 becomes 1;
  payload=min(total-offset,1386), never 0. FU indicator retains F/NRI and type28;
  FU header type/start/end and RTP marker match the original exact expressions.
  Interleaved length=header+payload-4 fits uint16. Output<=1404, next<=total.
- Session is const and untouched, offset is a value parameter. Only the owned
  packet and the two separate scalar destinations are written. No allocation,
  full-NAL scratch, callbacks, iterator cache, FD/OS/runtime state or retained view.
  Trusted valid/stable/nonoverlap memory is a precondition, not pointer validation.

## Harness/runner reasoning

- Exactly one long normal NAL: 174080 public formula bytes, header0x65 (VCL IDR).
  Three separate writable source objects initialized once: 65536,65536,43008.
  Source object sum174080; one separate output object1404; object sum175484 bytes
  excluding descriptors/scalars/alignment/process runtime. Largest source65536.
  No whole-NAL object, heap allocation or reconstructed-NAL destination. Three
  descriptors occupy48 bytes on this HOST; compiler reports main stack368 bytes
  dynamic,bounded (not whole-process or libc stack accounting).
- Expected header18 bytes is independently constructed from fixed constants,
  packet ordinal, RFC6184 start/end and exact interleaved length. Every actual
  output header/payload byte is compared before commit. Formula is computed per
  byte; no payload log/hash. Header reconstruction is checked against formula(0).
- Arithmetic: 174079 body bytes =125*1386+829, hence126 packets and176347 wire
  bytes. Packet48 crosses65536 and packet95 crosses131072, both strictly inside
  payload. Initial sequence65490 wraps to final80 after126 commits. End, marker,
  complete offset/length and exact expected sequence are checked on each packet.
  All session fields are checked unchanged before increment by harness.
- Synchronous check then offset/sequence commit is only a HOST consumer stand-in,
  not a socket write/ACK. Failure exits immediately; there is no second case,
  fallback, help/default/probe, retry or hidden original-packetizer call.
- Fixed compile recipe has exclusive `runs/build01` creation and no executable
  invocation. Fixed runner exclusively creates `runs/host01` before verification;
  verifies selected source/dependency/tool/build/audit pins; runs the one exact
  HOST executable once with no args/inputs, bounded10s timeout and minimal locale
  environment. It records scalar stdout/stderr/returncode, checks exact expected
  scalar output, then reverifies pins. Failure/timeout stops without retry.

## Build observed before call

`python3 apps/rtsp-auth/rtp-span-candidate/compile01.py` passed3 actual C compiles
and20 compiler/linker/inspection commands, all return0. HOST GCC16.2.1; retained
musl GCC14.3.0 `-march=armv5te -marm -mfloat-abi=soft`. Both use real framework
and libc headers, `-std=c11 -Wall -Wextra -Werror -Wpedantic`. ARM is ELF32/REL,
v5TE/EABI5, not a linked/runnable target program. HOST ELF16312 bytes linked for
the selected observation only. Each production object exports the single new
function; `memcpy` is its only undefined symbol. Compiler stack reports112 HOST,
48 ARM bytes (not transitive libc stack proof). Compile logs have no diagnostics.

The small evidence set has14 selected inputs,46 deduplicated compiler dependency
paths,27 tool/link-input records and54 pre-report artifacts. Wrapper `.br_real`
and Python interpreter are additionally pinned in PRECALL_AUDIT.json; no compiler
closure or native ABI/runtime claim. Production source and build report are frozen
for the sole call. Independent ordinary review and parent adoption remain required.
