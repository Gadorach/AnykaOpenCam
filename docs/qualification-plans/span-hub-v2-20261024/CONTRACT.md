# RP-09 / B07 — span hub R1 consumer lease correction

Written before v2 source, 2026-10-24. Only fresh
`apps/rtsp-auth/span-hub-v2/` and this evidence directory may change.
STREAM-01/02/03, CAP-01/05/07: one focused SOURCE correction, not new features,
tests, runtime adoption or native/cap acceptance. Independent ordinary review
and separate parent disposition are required before source adoption.

## Frozen inputs and exact minimum change

Start from the complete frozen V1 `span_hub.c` and `span_hub.h`. Preserve all
original candidate/evidence/review bytes. The independent review's R1 is an
ordinary ownership defect: a shared publication-generation token does not bind
a view or scalar release to its acquiring consumer. V1's ONE normal HOST PASS
used correct cursors only; it is not an R1 failure test or v2 runtime evidence.

Add one hub-wide uint64 issued-token counter, four canonical active-token slots,
and a consumer tag in the view. Keep publication generation separate. On acquire,
after owner/consumer/output/phase/acquired/pending checks, refuse an exhausted
issued-token counter before any output, mask or canonical-token mutation.
Otherwise form the same fixed small borrowed view, tagged with the actual
consumer and counter+1; commit that issued token and that consumer's canonical
active token only on actual successful acquisition. Tokens are nonzero, distinct
for every acquisition in this one stable owner lifetime, never wrapped or reused.
No callback or refusal follows token commitment.

View validation must check owner, actual consumer tag, subscribed/acquired bit,
nonzero token equal to that consumer's canonical active token, publication
generation, all existing metadata and every pointer/size including unused spans.
Scalar release must compare its supplied token to that consumer's canonical
active token, in addition to generation and existing owner/consumer/held checks;
merely tagging views is insufficient. Exact successful release first clears the
canonical active token, then clears acquired/pending bits. Another consumer's
unchanged view/token cannot validate or release this consumer's held lease.

STOP invalidates new effects and retains held tokens and old exact releases.
Subscribe/unsubscribe/resync/backlog/publication/STOP do not clear or reassign
held canonical tokens. Unsubscribe still refuses a held consumer. Generation
exhaustion still refuses before copy/drop mutation; token exhaustion refuses
before acquisition effects. The issued-token counter never decreases/resets
after release, unsubscribe, publication, STOP or disposal. It is not the
publication generation, random material, authorization or a cross-owner/process/
camera namespace. Trusted one-use true-fresh stable-address ownership and caller
serialization/disjoint-memory/no-alias/no-reentry rules remain mandatory.

Canonical-token invariant: initially all slots are zero; only successful acquire
sets one, in the same serialized operation setting its acquired bit; only exact
release clears one, before clearing that bit. No other path writes token slots.
Thus no acquired bits implies all canonical tokens are zero. Existing
STOPPED/drained or unpublished INIT_FAILED disposal remains valid under that
invariant, without a new fake permission flag or unrelated disposal policy.

## Preserved production paths and replacement boundary

Keep allocation, shape checks, chunk copying, publication, STOP, known-CPU
free/drain and packet-wrapper bodies byte-identical wherever possible. Eight
separate allocations each <=128KiB still back the inherited 1MiB envelope; no
full-NAL/per-client copy or raised cap. The actual accepted unchanged
`rtp-span-candidate/rtp_h264_spans.c` remains the production dependency. Producer
receipt/source/DMA/END ownership and deferred-send/packet-commit obligations
remain unchanged; no runtime/hub/encoder policy edit or native adoption.

The existing V1 export/type/guard names are retained ONLY for explicit source
replacement. New owner/view layouts are NOT V1 binary ABI compatible. Compile
all consumers against this replacement header; never link both hub versions,
old combined RELs, or include both headers in one translation unit/guard context.
Do not copy V1 host_normal.c or host_once.py into v2.

## Selected evidence only

Use a short fixed fresh-output/refuse-overwrite compile recipe: four warning-as-
error production C compiles (v2 hub and actual accepted span packetizer, each
HOST and retained musl ARMv5TE), two real production relocatable links, and
compiler/readelf/nm metadata. Record exact argv, logs, tool/input/dependency/
artifact hashes and exact forward/reverse source diffs. No executable or harness
is built or run; no old recipe, hidden HOST program, functional/negative/wrong-
consumer/refusal/mutation/fuzz test or existing HOST rerun. Compilation/linking
is not native ABI, loadability, transitive stack or complete memory qualification.
On policy refusal stop and report without retry/reroute.

No candidate production call, RNG/provisioner/verifier/device/hardware/private/
credential/retirement operation, socket/fork/thread or unrelated file change is
selected. No Git/index/AGENTS/central-plan edits or subworkers. Parent context
remains last OFF after finalized23h, blocked RAM75 recovery/C1 and no hardware
queue; this lane makes no new physical-state observation.
