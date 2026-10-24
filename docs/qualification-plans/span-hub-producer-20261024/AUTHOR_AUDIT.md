# Ordinary source audit — author, not independent review

Contract recorded before C/header. No function/library/test call is selected.

## Ownership and state edges

`usable` audits stable self/layout, actual eight pointers and writer phase PLUS
last canonical token/next generation/armed exposure/error consistency. Non-IDLE
requires zero pending/acquired/canonical consumer tokens. This is an invariant
audit under trusted no-edit serialization, not protection against arbitrary edits.
All writer identities and successful-allocation history remain after disposal.
No issued-counter assignment other than successful reserve/acquire exists.

Reserve has no backlog-dropping path. It checks token and generation max before
incrementing, so target generation and token are nonzero without overflow. Cancel
only changes RESERVED to IDLE; armed exposure history must precede its token.
Arm builds a small local view, then stores ARMED/exposed token BEFORE output.
Commit checks current identity, then phase/next generation, all eight pointers/
capacities/count, consumer exclusion, result shape/prefix/header and eligibility.
Recognized ARMED refusals call sticky hold; no failed validation can publish or
clear a writer. Wrong identity cannot cancel/hold/commit the current actor.

Publication assignments have no callbacks/fallible work between them. Generation
increments once; `writer_phase=IDLE` is last. Only this successful commit or exact
unexposed cancel can retire a writer; neither clears history. STOP turns ARMED to
HOLD and keeps exact identity/backing, never an abort or successful END. RESERVED
STOP may cancel; no arm after STOP. Repeated STOP cannot clear HOLD/error. Hold
does not permit further writes; caller must quiesce all writable aliases and
retain actual external owner on uncertainty. No recovery/armed retirement API is
invented; an armed stopped owner may intentionally remain retained indefinitely.

No subscription/backlog/dispose/copy path can bypass writer exclusion. Scalar
consumer release retains its exact consumer slot/token/generation meaning, with
writer exclusion added. It still permits genuine exact releases after STOP when
no writer exists. Acquire is the only active consumer-token setter; exact release
alone clears it. Issuance never resets on release/publication/STOP/disposal.
V1's shared-token R1 therefore is not reintroduced; this is ordinary source
reasoning, NOT an executed wrong-consumer case or authorization claim.

## Slice bounds and accepted packetizer connection

Result validation admits0/3/4 prefix bytes only when strictly below written total
<=1048576. Reads are only prefix bytes and the actual owned VCL header, at offsets
<=4 and <written. Type1/5/forbidden-bit/keyframe agreement is explicit. Sentinel
UINT64_MAX timestamp refuses; zero is representable and is not clock admission.
The policy remains one VCL, not a generic AU/bitstream validator.

`slice_spans` first checks `offset<total_capacity` and
`0<bytes<=total_capacity-offset`, so position+take never overflows or exceeds the
eight chunks. Every step takes a positive min of the current physical remainder
and remaining logical bytes. Each physical boundary is visited at most once:
at most eight entries, exact sum, no OOB final dereference. Prefix3/4 shortens
first range, not a memmove. Final tails are NULL/0. Logical header/first payload
map at prefix and prefix+1 even when payload later crosses physical boundaries.
Acquire derives this array before output/token effects. Lease validation freshly
reconstructs and compares every descriptor including tails, plus metadata and R1
identity. Packet wrapper remains byte-identical and rechecks before every actual
call to the unchanged accepted production span packetizer. No cached slice/token.

The sole malloc site remains init's fixed eight131072 requests. Copy_input remains
the old bounded legacy copy only, never producer commit. No source pointer is
stored in the owner. New writer/reservation/result and validation arrays are small
fixed metadata, not a full-NAL allocation or per-consumer backing.

## Real missing connection

Read accepted kernel UAPI/helper/contract/parent disposition, real composed sender
publish/source-release/exact-END tail and runtime paid-association/memory/borrow-
end code. Current sender secures its copy BEFORE release/END. This new path must
instead commit only AFTER actual successful copy + complete metadata + real
producer release/exact END + real guards, while keeping hub writer custody alive.
This lane has no ioctl/caller implementation and no fake callback or public
success flag. It cannot establish those external facts. New-layout memory,
pristine, return, native prefault, readiness and borrow-end derivatives are still
required; old consumers MUST NOT cast/dereference this layout or auto-adopt it.

No existing native1MiB malloc/old contiguous ioctl changes here; overall allocation
cap/raw/process2MiB/40MiB/CMA/TLS combined load/external backing/recovery/main/C1
remain open. No normal/negative test, generated main, driver/image invocation,
native ABI claim or hardware operation. Compile-only recipe is fixed, uses fresh
exclusive outputs, pinned retained tools, production C only and rooted REAL RELs.
