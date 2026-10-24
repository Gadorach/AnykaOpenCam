# Span hub v2 — parent R1 source closeout, NOT RUN

2026-10-24; RP-09/B07, STREAM-01/02/03, CAP-01/05/07.
Accept **narrow conditional SOURCE/API/ownership sufficiency** after independent
ordinary review. R1 is corrected in this replacement; no blocking ordinary
defect found. This does not adopt v1 or qualify runtime/native/allocation limits.

Acquisition now issues distinct monotonic nonzero tokens, separately from
publication generation, storing each in the actual consumer's canonical slot.
View validation checks both consumer tag and active token; scalar release
independently checks that consumer's token/current generation and clears the
token before acquired/pending bits. Refusals/exhaustion precede output/state
effects; tokens never wrap/reset/reuse. Another consumer's unchanged view/token
therefore cannot validate or release this lease. This is a static source finding,
not an executed wrong-cursor test or security authorization across owners.

Under mandatory serialization/true-fresh trusted storage, acquire alone sets
token/held bit and exact release alone clears them. Publish refuses held bits;
STOP preserves tokens/backing and permits old exact releases while denying new
effects. Thus drained implies no active tokens; explicit known-CPU disposal does
not free held backing. All allocation/copy/publication/packet-wrapper/STOP/drain/
dispose paths remain byte-identical to v1. Eight independent128KiB requests and
one shared copied NAL, not a per-client AU allocation or native ownership claim.

## Exact evidence

- C: `bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1`.
- Header: `310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454`.
- Review: `01876c2ff2276e3ed7696627d8569338ebbeac55a66f505a6160a8c8a684152e`.

Parent read complete v1/v2 source/header, exact deltas, recipe, contract/handback
and both independent reviews. Freshly checked222 v1 records plus33 logs,282 v2
records (including20 logs/113 frozen files), exact frozen directory membership,
regenerated forward/inverse diffs and unchanged C regions. See the parent JSON.
Counts overlap; not a complete compiler-execution closure or independent runtime
trace. Original source/evidence/review directories retain every sealed byte.

Author4 HOST/ARM production compiles,2 real hub→accepted-packetizer REL links and
20 commands PASS are cited, not rerun. HOST/ARM outputs are REL, not executables;
malloc/free/libc/compiler symbols remain unresolved. No final ARM/native ABI,
transitive-stack, locked/resident memory or combined-load proof.

**V2 has zero function/program/test calls.** Frozen v1's ONE174080B/four-correct-
cursor/504-packet HOST PASS remains its normal observation only; v1 remains NOT
ADOPTED for R1. No repeated HOST case, negative/adversarial/mutation/fuzz run or
historical relabeling. Further deferred coverage is not passed.

V2 retains old API/type/guard names but changes layouts: use only this replacement
header/C and recompile consumers; never both hub versions or old combined RELs.
Stable valid disjoint objects, no mutable aliases/copies/reentry/signals/fork/
concurrency, per-effect lease checks and whole-packet cursor commit remain caller
requirements. CPU copy/dispose is not source-release/DMA/END/ACK/recovery authority.

Actual server/runtime adoption, native contiguous1MiB output/raw backing, chunk
prefault/ledger accounting,2MiB process/40MiB/CMA/TLS/headroom, external retention/
recovery/main and generic AU/RTCP/audio/recorder integration remain open. The next
feature connection must reconcile real hub backing and these accounting/lifetime
contracts before changing native startup; no hidden allocation exception or
metadata-ready flag is accepted. No new diagnostic/build/clock framework selected.
No source author/reviewer active. Camera last controller OFF after finalized23h;
RAM75 recovery/C1 block and all old failures unchanged, no hardware phase queued.

Curated local Git retains v1's correction-required history alongside v2 source,
not runtime adoption. Ignore run trees/objects/ELFs; preserve literal diffs if Git
flags context-space whitespace. Parent records stay outside frozen directories.
No push, private data or publication.
