# Producer span hub — independent ordinary SOURCE review

2026-10-24; B07 / RP-04/09; CAP-01/05/06/07, STREAM-01/02/03.

**Narrow conditional SOURCE/API/bounds/ownership/order sufficiency. No blocking
ordinary defect found in the stated trusted-caller scope.** This is independent
review, NOT parent adoption, execution, native/image ABI, allocation-cap or product
GO. Parent must separately dispose this exact delivery before use/adoption or any
follow-on. No implementation, test or hardware queue is selected here.

## Scope and pins

Read all four delivered app files (complete C/header/recipe/notices), complete
CONTRACT/HANDOFF/AUTHOR_AUDIT, delivery/build manifests, command records, all22 logs,
dependency/stack metadata and literal source deltas. Compared complete accepted
v2 C/header, its parent V2_DISPOSITION and v1's independent R1 negative review;
read the actual unchanged RTP span C/header and parent disposition. Read encoder
span UAPI/helper/parent disposition as bounds context only. Targeted reads of
actual composed sender/runtime, native allocation/ioctl and memory layout users
establish NON-adoption, not a new review/correction of those disjoint units.

SHA-256, workspace-relative paths:

| Path | SHA-256 |
| --- | --- |
| `apps/rtsp-auth/span-hub-producer-candidate/span_hub.c` | `72e06177ae86c8659972ba8a59c81208302d44e0cb78f29b6114f8a4d5631f93` |
| `apps/rtsp-auth/span-hub-producer-candidate/span_hub.h` | `460320064f85d1f1f8ada1a9afd1e282c287583fdff34787ba63c7354adeed8e` |
| `apps/rtsp-auth/span-hub-producer-candidate/compile01.py` | `4d9a39da0755a5180020477a546475d17ac8b295ccb7fc92040f5241249b2175` |
| `docs/qualification-plans/span-hub-producer-20261024/DELIVERY_PINS.json` | `15fb48df2ee1694f8802c2d268efdfd8f72ad690038a6b8cb920ff34fe5e0e2a` |
| `docs/qualification-plans/span-hub-producer-20261024/runs/build01/REPORT.json` | `680b6b7bc317c76443267e6c5b1a557afe79377a84f9ee29db8d5d5d49ff561b` |
| `docs/qualification-plans/span-hub-producer-20261024/runs/build01/FROZEN_INPUTS.json` | `894f5407219d418d49e1d5f2b55798a1b2288e7be8dba6daf9c58cddc6a37d7f` |
| `docs/qualification-plans/span-hub-producer-20261024/runs/build01/SOURCE_DELTA.json` | `e3bf6d00d804a77c9757687a8af8e5a0b42e09d433bca346bb7f34207b352702` |

Fresh filesystem/hash checks matched delivery8, selected inputs9, consumed
dependencies42, tools12, report artifacts69, frozen519 across12 exact roots and
all22 command/log records. Build01 has exactly70 files including REPORT; the
complete app/author-doc delivery has78 files. Counts overlap; not coverage or a
complete compiler-execution closure. The78-file aggregate is
`6366dcecd732b8a4210f2bee7989242f028abd4ab9f262913f36862ccc242d5e`:
sort complete workspace-relative path strings, concatenate UTF-8
`path\tdecimal_bytes\tsha256\n`, then SHA-256. No original bytes were changed.

Both literal forward/inverse C/header deltas regenerated exactly and reconstructed
their opposite source exactly without applying patches. Current consumed sources/
headers match pins; dependency membership equals the four saved `.d` files. All12
tool records match retained tool identities. The five claimed unchanged function
texts (`consumer_valid`, `lose_backlog`, `input_valid`, `copy_input`, packet wrapper)
are identical to v2; this is not blanket proof of their changed call contexts.

## Ordinary source findings

References below are to the pinned new `span_hub.c` unless otherwise stated.

* **Real backing/invariants (`17–92`, `399–415`):** successful init makes exactly
  eight canonical `malloc(131072)` requests, no new1MiB source/temporary or
  per-consumer payload. Self/layout/INITIALIZING latch precedes allocation;
  partial INIT_FAILED retains known chunks. `writer_consistent` audits phase,
  issuance/current token, next-generation relation, exposure and sticky error;
  `usable` also checks version3, phase, all chunks and reader exclusion. These
  are trusted protocol invariants, not hostile-pointer or provenance checks.
* **Exclusive reservation/exposure (`185–238`):** reserve requires ACTIVE/IDLE,
  subscribers, no pending/acquired/active consumer tokens and unexhausted writer
  issuance/publication generation. Refusals precede owner/output effects, without
  dropping backlog. Each success increments writer issuance once; exact owner/
  token/reserved-next-generation identities exclude stale/cross-writer completion.
  Exact RESERVED cancellation alone retires unexposed custody, also after STOP,
  retaining histories. Arm builds bounded metadata first and latches ARMED plus
  exposure history before outputting all eight canonical pointers/capacities.
  There is no fallible protocol operation/callback after that latch.
* **Commit/HOLD (`240–303`):** exact current ARMED identity precedes ACTIVE/next-gen,
  all-eight descriptor/count, consumer exclusion, actual result/prefix/header and
  eligible-consumer checks. Every reachable refusal after recognizing that ARMED
  owner holds terminally with first error; no partial publication, flag clearing,
  drop/retry or release. Wrong identities leave the actual writer untouched and
  retained. Explicit hold checks identity without requiring intact descriptors.
  Success increments publication generation exactly once, installs logical
  offset/length/metadata/pending/keyframe state, then sets writer IDLE LAST.
  No payload copy, allocation or fallible action follows publication start.
* **Bounds/slicing (`257–275`, `305–374`):** written1..1048576 and prefix0/3/4,
  strictly shorter than written, bound every owned-byte read. Exact declared
  prefix bytes, forbidden bit, VCL type1/5 and keyframe agreement are checked;
  UINT64_MAX timestamp refuses. Timestamp provenance/clock admission and complete
  single-VCL bitstream validity remain caller obligations, not grammar proof.
  Prefix is logical offset, never memmove/concatenation. `slice_spans` checks
  subtraction bounds before arithmetic; positive minima advance to successive
  physical boundaries, indices0..7, exact sum and at most8 entries. Prefix-shortened
  chunk0 and final exact-capacity endpoints are covered by that arithmetic;
  no terminal dereference, unused tails NULL/0. Acquire computes the canonical
  slice before issuing output/token effects. Validation reconstructs and compares
  count, every descriptor/tail and scalar metadata anew.
* **Consumer R1 and old mutation paths (`95–182`, `328–397`):** v2's distinct
  monotonic per-acquisition tokens, per-consumer canonical slot and view tag remain;
  scalar release independently checks that consumer's token/current generation.
  Only acquire sets a slot and exact release clears it. No consumer lease can
  coexist with writer custody. Subscribe/resync/unsubscribe/copied-publish/acquire/
  release/validation all exclude a retained writer before effects. With no writer,
  copied publish still actually copies normalized input, resetting offset0 only
  after copy; acquired data is never overwritten and ordinary backlog/keyframe
  resync semantics remain. STOP denies future effects, preserves exact consumer
  releases and writer identity/backing, and turns ARMED into terminal HOLD.
  RESERVED may only exact-cancel after STOP. Repeated STOP cannot clear error.
  Drained/dispose reject RESERVED/ARMED/HOLD/error or pending/acquired/active
  tokens; disposal preserves identities, both issuance histories, generation,
  offset and allocation history. No reset/abort/free route retires armed storage.
* **Actual packet path (`376–384`):** fresh full consumer lease validation precedes
  the unchanged accepted `camera_rtp_h264_spans_packet_v1`, not a stub or contiguous
  packetizer. V1 R1 is not reintroduced in validation OR scalar release. Per-send
  revalidation and whole-packet partial-write/cursor commit remain caller duties;
  packet generation is neither ACK nor source-release authority.

## Saved compile evidence — cited, never rerun

Four actual production C compiles: new hub plus accepted packetizer, HOST O2 and
retained musl ARMv5te/ARM/soft-float Os; all warning-as-error logs empty. Two real
rooted REL links,17 public hub roots matching header and both link argv,22 successful
saved tool commands, no saved failure/rerun, fixtures/stubs/main/executable. No
old-layout consumer is compiled here and no old hub/combined REL is linked.

Read-only raw ELF parsing of all four objects and two RELs confirms ELF64 x86-64
HOST / ELF32 ARM EABI5, REL/entry0/no program headers,17 actual hub definitions plus
the real packetizer, no main/_start or unresolved camera symbols. Wrapper
relocations reference actual lease validation and packetizer definitions;21 HOST/
25 ARM nonempty input text sections match corresponding REL bytes exactly. Maps
name only the two production objects (ARM additionally records `LOAD linker stubs`).
An initial reviewer map assertion assumed only two LOAD records; inspecting that
ARM-generated metadata corrected the read-only assumption, not any build evidence.

Raw referenced undefined symbols are HOST `malloc/free/memcpy/__stack_chk_fail`;
ARM additionally `memset/__stack_chk_guard/_GLOBAL_OFFSET_TABLE_`. These are normal
unresolved libc/compiler/GOT REL dependencies, not native ABI/final-link acceptance.
ARM `.su` maximum168B is acquire; arm136B. Individual frames are NOT transitive
stack, allocator overhead, residency, prefault or native memory-fit proof.

## Mandatory conditions and remaining gaps

Persistent true-fresh final-address private owner; valid stable disjoint objects;
serialization; no owner edits/copies/moves, reentry/signals/later fork/concurrency
or unauthorized mutable aliases. Only arm's intended writable aliases are admitted
during its writer lifetime; quiesce on uncertainty/HOLD and never use after commit.
HOLD retention is NOT continued write permission or a bounded recovery guarantee.

The future real caller must establish supported actual syscall0/complete valid
metadata, status/frame/profile ownership, successful real producer release/exact
frame END and actual native/clock/auth guards BEFORE commit. Any uncertainty must
retain actual external owners AND armed CPU backing. This CPU API has no fake
success Boolean/callback/opaque receipt proving those external facts. Encoder UAPI
128B/0xc0805167/eight131072/total<=1MiB supplies bounds context only; any copy/control
error invalidates all metadata. No ioctl integration, native support or ACK is here.

Version3 is an EXCLUSIVE source replacement despite retained names/guard. Use only
this C/header, never old objects/combined RELs or cast old layouts. Current composed
sender still copies (`rtsp_span_server.c:534–548,718`) BEFORE actual source release
and exact END (`724–730`). Its native adapter still mallocs/prefaults1MiB and uses
the old contiguous ioctl. Current memory forecast/prefault, paid-association,
readiness/pristine/return-history/borrow-end and frontend know the old layout;
they are NOT adopted. Full rebuilding PLUS separately reviewed actual typed
connections and new-history audits are required, not merely relinking this hub.

Overall128KiB cap/raw/process2MiB, existing kernel2MiB DMA/reference allocations,
40MiB/CMA/TLS/combined load, external surviving owner/backing/recovery,
provisioning/main/native ABI/entropy and strict C1 remain open. Single-VCL source
reasoning does not complete generic AU/recorder/audio/product gates. Historical
23h OFF/finalized is not fresh physical admission; RAM75 independent recovery/C1
remain blocked, Query78 parked, v1 NOT ADOPTED and all original failures/HOLDs kept.

Reviewer performed public reads/hash/diff/log/raw-ELF-byte metadata only; no
recipe import/build/compiler/link/readelf rerun, candidate/library/helper/hub/
malloc/packetizer/test/callback/ioctl/native/private/device/network/hardware call.
Only this fresh review directory was written; no Git/index/source/evidence/AGENTS/
central-plan changes or subworkers. Deferred negative/adversarial/mutation/fuzz
coverage is NOT RUN, not passed. Parent disposition remains separate; stop here.
