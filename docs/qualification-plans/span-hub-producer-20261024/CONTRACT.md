# Producer custody contract — before code

2026-10-24. B07 / RP-04/09; CAP-01/05/06/07 + STREAM-01/02/03.
SOURCE candidate only; no function/test/ioctl calls, including HOST. Independent
ordinary review and parent adoption are required before use. Deferred negative,
adversarial, mutation and fuzz coverage is NOT RUN, not passed.

## Exclusive replacement and trust boundary

Derive only the complete hub C/header from accepted `span-hub-v2` (C
`bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1`, header
`310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454`). Retain
ordinary API names/guard, explicitly declare layout/API version3. Never link two
hubs/old combined RELs, cast layouts, or reuse old-layout consumers. The accepted
unchanged production span packetizer is the actual packet-wrapper destination.
No copied runtime tree, source allocation, full-NAL temporary or new backing.

Persistent true-fresh zero owner at final address, valid stable disjoint private
objects, caller serialization and no owner edits/copies, reentry, signals, later
fork or concurrency are mandatory. Readonly/writable views are separate disjoint
metadata objects. The ONLY writable chunk aliases permitted are the eight exact
canonical ranges returned by arm, and ONLY during its admitted writer lifetime.
They must never be used after commit. Retained stale pointer values do not grant
access. Checks are trusted C protocol checks, not malicious pointer validation,
physical alias/provenance, DMA exclusion, authorization or recovery proof.

## Actual state machine

The existing eight independent malloc131072 chunks remain the sole backing.
Hub ACTIVE/STOPPED phase is separate from writer IDLE/RESERVED/ARMED/HOLD.
One monotonic writer issuance counter never wraps, reuses or resets. Typed
reservation and writable lease bind exact owner, writer token and reserved NEXT
publication generation. Numeric collisions with consumer tokens confer nothing.
Distinct canonical consumer active tokens and scalar release keep v2 R1 meaning.

* Reserve requires ACTIVE, IDLE, no pending/acquired consumers, no active consumer
  tokens, a subscriber and unexhausted writer token AND publication generation.
  It returns small reservation metadata, no pointers. All refusal paths leave
  owner/output untouched. It does not drop backlog to gain custody.
* Exact RESERVED cancellation (also after STOP) exposes no pointers, returns IDLE
  and retains token/generation/issuance history. ARMED/HOLD cannot cancel.
* Arm requires exact canonical RESERVED identity and ACTIVE. Latch ARMED and the
  historical armed token BEFORE returning eight exact writable pointer/capacity
  descriptors (131072 each). There is no callback or fallible action afterward.
* Commit requires exact current ARMED lease, ACTIVE, no HOLD, canonical count,
  pointers/capacities, untouched reserved next generation, no pending/acquired/
  active consumer tokens, complete bounded metadata and an eligible consumer.
  On success publish a logical slice, increment generation EXACTLY once, install
  ordinary consumer pending/resync metadata, and ONLY THEN retire writer to IDLE.
  No memmove, memcpy, allocation or source pointer is involved in commit.
* Explicit exact-lease hold is available for actual write/ioctl/source/clock/auth
  uncertainty. Any commit refusal after recognizing the current ARMED identity
  similarly latches terminal HOLD/first error. Wrong/stale identities do not
  mutate another current writer; its ARMED ownership remains held, never retried
  as a valid result. Caller must retain/hold the actual writer on uncertainty.
  HOLD has no reset/abort/free API. Failed metadata never marks consumer complete.

While RESERVED/ARMED/HOLD, no subscription/resync/unsubscribe, copied publish,
acquire/validation/packet effects or disposal can alter/retire storage. No reader
can coexist by admission; scalar release also refuses writer-active state without
changing the consumer identity rules. STOP immediately invalidates future effects
and preserves writer identity/chunks; ARMED STOP latches HOLD. RESERVED after STOP
may only exact-cancel. Drained/dispose require IDLE, no writer error, no held or
pending consumers and zero active consumer tokens. Disposing preserves issuance,
last writer/armed identity, generation, logical slice and allocation history.
No retry/init/reset clears these fields; no native END follows CPU disposal.

## External obligations BEFORE commit — not CPU receipts

Arm does NOT call an ioctl. A future real caller must perform the exact supported
kernel copy, observe actual syscall0 with COMPLETE metadata, validate actual
status/frame numbering/profile/source ownership, successfully perform actual
producer release and exact common frame END, and satisfy all real native,
clock/auth/admission guards BEFORE calling commit. Writable aliases must then be
quiescent and cease on successful commit. No public success Boolean, callback,
opaque receipt or token is accepted as proof of these external facts. This API
cannot itself establish syscall0, release/ACK/END, mapping stability or recovery.
On any uncertainty retain actual native/source ownership AND armed CPU backing;
no general abort/discard, no complete-looking partial result, no fallback/retry.

The accepted kernel UAPI (`qc07f-encoder-output-spans`, commit930afde) is128 bytes,
command0xc0805167, eight fixed131072 CPU destinations, <=1048576 valid bytes. It
copies only actual output, no unused tails/FIFO. Any data/control partial-copy
error invalidates ALL metadata even when apparently complete. access_ok is not
page pinning/exclusivity. This lane does not invoke/adopt/change that driver.

## Logical NAL slice and real packet path

Commit metadata is codec H264, keyframe0/1, monotonic timestamp, actual written
total1..1048576, leading-prefix length0/3/4. Bounded canonical-byte reads check
exact declared Annex-B prefix (00 00 01 / 00 00 00 01) and a remaining VCL header,
forbidden bit clear, type1/5 exactly agreeing with keyframe. No general multi-NAL
AU or complete bitstream grammar claim. No unreported padding/trimming. Stored
data_offset is prefix size; logical bytes are written-prefix. No contiguous view.
Copied legacy publish remains normalized input at offset0; reset offset only
after actual successful copy. Ordinary no-writer drop/keyframe behavior remains.

Acquire and lease validation derive canonical packed const spans from logical
offset/length over the eight physical boundaries. Nonempty entries <=131072,
exact sum, no overflow, empty tail entries NULL/0. Slice bounds are checked
before pointer arithmetic. Actual packet wrapper rechecks consumer owner/tag/
active token/generation/full slice before EVERY call into the unchanged accepted
packetizer. Caller still checks before every send and preserves complete-packet
partial-write/cursor-commit rules. CPU commit is not source END or packet ACK.

## Non-adoption and evidence scope

Current `qc07f-span-serve-candidate` still uses copied publish before its actual
producer release/END; its memory paid-association, pristine checks, prefault,
return-history, borrow-end and front server know only old layout. They MUST NOT
dereference/cast/adopt this replacement automatically. Current native producer
still mallocs1MiB and uses contiguous ioctl. Future separate derivatives must
connect real encoder destinations/release/exact END and update every layout user.

Fixed small recipe: four actual production HOST/retained musl ARMv5te C compiles,
two real rooted hub+accepted packetizer REL links and ELF metadata only. No main,
fixtures/stubs, executable, tests, old builder, kernel/module/image or hardware.
Exact argv/dependencies/tools/logs/artifacts plus forward/inverse deltas and
frozen input membership retained. Refuse output reuse; preserve any failed run.

Overall128KiB cap, raw/process2MiB, kernel2MiB buffers,40MiB/CMA/TLS combined load,
external recovery/backing/provisioning/main/native ABI/entropy and strict C1 stay
open. Camera last23h OFF/finalized is historical, not current admission. RAM75
independent recovery/C1 blocked; v1 hub NOT ADOPTED; all old failures untouched.
No next lane/queue, clock/receipt/diagnostic framework or hardware phase follows.
