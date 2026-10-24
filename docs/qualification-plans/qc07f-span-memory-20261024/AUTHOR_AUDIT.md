# Author ordinary source audit — not independent acceptance

2026-10-24. Scope: CONTRACT.md; four derivative memory units, unchanged local
reader/handoff and accepted v2 hub/packetizer. No tests/candidate functions called.
Independent ordinary review and parent adoption remain REQUIRED.

## Actual connection and history

`memory_reserve.h` includes the real `../span-hub-v2/span_hub.h`. No shadow hub
definition, flat pointer adapter or dependency on the separate unaccepted sender.
The whole-hub forecast reads the canonical actual owner fields, not a caller bool.
It requires self/ACTIVE, generation/issued/active tokens zero, all subscriber and
publication metadata zero and exact allocation history8/1048576/131072. Header
shape is compile-time fixed at8×131072. True-fresh allocation/identity is a trusted
private caller contract; no pointer-provenance/malloc/global admission proof.

Forecast builds bounded local metadata; checks all8 pointer+byte additions before
accepting each range; checks each range against all earlier ranges and every paid
scalar extent. Already credited hub/any nonempty prior hub metadata refuses. It
adds only ceil(begin/4096)..floor(end/4096) full interior pages. Division/remainder
avoids the original `a+4095` HOST64 wrap risk. No boundary, allocator padding or
double credit; byte-disjoint extents cannot contain the same complete interior
page. Checks subtraction precondition and total capacity before output/commit.

`nh_memory_credit_span_hub` is the PUBLIC METADATA HALF with mandatory trusted
prior whole touch, like the original credit_owned precondition. It calls actual
forecast then commits association/ranges/sum/paid[1] without callbacks or refusals
after mutation begins. It does not itself touch memory or give native permission.
No eight repeated scalar payments. begin/end[1] remain zero. Scalar raw/producer
credit checks all recorded hub ranges; kind1 refuses as its FIRST action, before
even reading the ledger or purported pointer. Raw/prod full-interior semantics
are retained. Paid debt cannot be erased by this module except trusted one-time
initialization, whose contract forbids reuse/reset; runtime init also refuses
initialized/attempted objects before memset.

## Actual prefault ordering

`nh_memory_prefault_span_hub` first validates runtime/callback presence/attempt,
two remaining observation slots, full actual hub shape and whole credit forecast.
This non-effectful preflight refusal changes NO ledger/runtime association. It
then retains exact forecast and latches TOUCHING before any callback, metadata
reader or chunk byte access. No clock/cancel/read/gate error after this admission
can reenter a fresh attempt. HOLD stores the original reason, returns without
changing paid/debt/ranges, never frees/mutates owner/chunks, and rejects subsequent
whole prefault, gate and scalar prefault work. Direct ledger routines still rely
on their trusted lifetime/serialization preconditions; they are not a guardian.

Original `checked()` is byte-identical: REAL now/cancel callback contract,
UINT64_MAX/regression, original unit_start and session_start, 2s/10s bounds. The
original gate body is unchanged apart from a HOLD early refusal. It still reads
real fixed meminfo→zoneinfo→meminfo with100ms sample/64 observation limits. Reader
and handoff are byte-identical. No actual reader/callback invocation occurred.

The first real gate still sees ALL unpaid hub debt; no credit before byte touches.
Each of all8 canonical chunks gets volatile SAME-byte read/write at offsets
0,4096,...,126976 and131071. Checked boundaries precede each chunk, occur after
each16 strides and after each final byte. Any partial return leaves HOLD/unpaid.
A synchronous address fault is NOT caught: an unfinished TOUCHING attempt is
likewise nonretryable and requires external handling with backing retained; no
claim of surviving arbitrary memory faults or catching signals is made.

After all touches, the final fresh actual gate still sees unpaid debt. Forecast
revalidates pristine owner and exact range association/sum; runtime compares the
retained snapshot. Actual public whole credit is called only then, under no
concurrent effects/callback mutation. If it refuses, HOLD remains unpaid; success
has only nonfallible PAID assignment/return afterward. No postcredit check can
pretend an unpaid retry. Subsequent whole calls refuse, not idempotent success.
Scalar raw/producer prefault touch/check/order is original except kind1 refusal,
HOLD fence and corrected raw/producer-only comment. No native ready flag installed.

## Debt and unchanged policy

All original meminfo/zoneinfo parsing and codec-credit/evaluate region bytes are
unchanged. The source delta records these exact region hashes. Minima change
2804→2832 and3832→3860. Actual ARM ELF inspection is unchanged; layout arithmetic
replaces the hub1028KiB cover by1056KiB, preserving separate producer1028KiB cover,
mapped image and stage. HOST still refuses; public-fixture branch removed.
ARM REL keeps unresolved `__ehdr_start`; not a final executable/layout result.

The +28KiB is conservative REQUESTED USER EXTENT page cover only, not allocator
overhead/headers/arena/fragmentation qualification.512KiB envelope,1024KiB reserve,
8256KiB codec debt/exact8454144-byte healthy credit, full unpaid CPU and occupied
CMA migration policy remain. No allocator/library/stack/socket/residency/process
2MiB/40MiB/combined-load/native cap acceptance, no CMA exclusion or larger limits.

## Explicit replacement boundary

Same export/type/header guards do NOT imply binary compatibility. Use only these
replacement memory units and the actual v2 hub/packetizer; rebuild all consumers,
never old memory objects/combined RELs or hub v1. Local unchanged handoff includes
old contiguous declarations solely to retain actual timing/header dependencies;
no nh_run/native/serve implementation or use is delivered.

Original native_adapter.c83–94 calls old kind1 flat prefault; it will now REFUSE,
not silently pay one/eight chunks. It is NOT compiled/adopted here. Original
provider-return.inc39–42 checks only old scalar ledger/runtime history; it must
gain exact new hub/attempt/range/failure history checks before future adoption.
Original ready/private-owner/headers and new sender integration likewise remain
separate work. No source payload/span export, frame acquisition, provider/native
memory init, listener, release/disposal, malloc or main invocation occurred.

Camera last controller OFF after finalized23h; native recovery/C1 and historical
failure HOLDs untouched. No native/hardware queue, negative/adversarial/mutation/
fuzz trial or function execution selected.
