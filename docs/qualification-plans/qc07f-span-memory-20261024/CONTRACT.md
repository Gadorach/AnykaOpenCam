# QC07F span memory — contract before implementation

2026-10-24. CAP-01/05/06/07, RP-04/09, B07. SOURCE/compile-only candidate.
Only this directory and `apps/rtsp-auth/qc07f-span-memory/` are owned by this lane.
No function/test/program/native/proc/device execution, no new server dependency.

## Typed transaction

- Replace the original owned-runtime memory reserve/runtime units, not the native
  adapter. Keep scalar raw(0)/producer(2) APIs; refuse scalar hub(1) before any
  buffer dereference. Retain begin/end[3]; add eight explicit canonical hub ranges,
  count and borrowed actual v2 hub association. No flat hull or repeated credit.
- Read the accepted v2 type directly: self identity, ACTIVE, pristine generation,
  issued/active tokens, subscribers/pending/acquired, metadata zero; eight
  allocation calls, 1048576 successful requested bytes, maximum131072; all eight
  canonical nonnull chunks, fixed131072-byte extents. This is trusted in-process
  source history, NOT malloc/provenance/fork/global-admission proof.
- Pure metadata preflight checks every pointer+length overflow, mutual byte overlap,
  existing paid raw/producer/hub overlap, full INTERIOR4096-byte page credit and
  remaining capacity before touching any backing or changing the ledger. Boundary
  pages/padding never pay debt; disjoint byte ranges cannot share interior pages.
- Runtime retains the exact borrowed owner and ranges, then latches ONE attempt
  before clock/cancel/real metadata gates or touches. Whole unpaid hub debt remains
  in the pre-memory gate. Use original checked() callback/time contract and fixed
  meminfo→zoneinfo→meminfo reader. Same unit/session start, 2s/10s, 100ms samples,
  64 observations. No C1 whole-interval guarantee.
- Volatile SAME-byte read/write at each4096-byte stride plus final byte of EACH
  of all eight chunks, with checked clock/cancel boundaries. Final fresh unpaid
  gate, unchanged full owner/range association and forecast, then ONE whole-kind
  metadata credit and success record; no fallible work after commit.
- Any admitted attempt failure is persistent HOLD/unpaid, with original borrowed
  backing retained; no retry/reset/free/idempotent double credit. Trusted address
  faults are not caught. Metadata preflight refusal leaves ledger untouched and
  is not an admitted touch attempt. Direct public credit is ONLY the metadata half
  with trusted prior actual whole touch, not permission or a caller-ready boolean.

## Lifetime/layout/policy

Caller provides truly fresh persistent runtime storage, valid stable disjoint
objects/private writable chunks, no aliases, reentry/signals/fork/concurrency or
callbacks mutating hub/ledger/config. Hold exact owner/chunks through all outcomes
until separately proved real retirement. Module adds no guardian or disposal API.
New fields change layouts, despite retained export/type/header guards. Recompile
all consumers; never link old/new memory units or old combined RELs together.
Old provider-return history checks omit these new fields and MUST be revised in a
future actual adoption, as must native_adapter/ready/private ownership headers.

Old flat1MiB requested extent can cover257 pages=1028KiB; eight128KiB requested
extents can cover8×33 pages=1056KiB: +28KiB user-extent page cover. Minima become
2832KiB raw-reuse /3860KiB separate. Actual ELF32 ARM layout inspection remains;
ELF64 HOST refuses layout. No public-fixture branch. These are NOT allocator
header/arena/fragmentation charges or qualified overhead. Preserve512KiB envelope,
1024KiB reserve, full CPU/CMA migration debt and codec8454144-byte policy.
Allocator/library/stack/socket pressure, native raw/output/cap, process2MiB and
40MiB/combined-load fit remain UNQUALIFIED. No CMA exclusion or cap increase.

## Evidence boundary

Compile changed units plus unchanged real fixed reader, accepted v2 hub and actual
accepted packetizer, HOST and retained musl ARMv5te; offline REL/readelf/nm only.
No fixture define, function call, synthetic harness or executable. Pin exact
commands/tools/consumed inputs/logs/artifacts and forward/inverse source diffs.
Independent ordinary review and parent adoption required before any use.
Camera last OFF after finalized23h; RAM75 recovery/C1 blocked; no native queue.
