# Span hub v1 — parent R1 disposition, NOT ADOPTED

2026-10-24; RP-09/B07, STREAM-01/02/03, CAP-01/05/07.
Parent read the independent `span-hub-review-20261024/REVIEW.md`, complete v1
source/header and normal harness. Accept its **R1 correction-required finding**;
do not adopt v1's ownership/API or treat its normal HOST pass as lease sufficiency.

V1 uses publication generation as every consumer's token. View validation and
scalar release check a supplied consumer's acquired bit, but do not bind that
consumer to a distinct acquisition identity. An unchanged view/token from one
cursor can therefore validate/release another cursor's held lease. This can make
backing appear overwriteable/drained while the other cursor still holds it.
No hostile pointer, race or executed adversarial case is required for this
ordinary source consequence. Adding only a view consumer tag would leave scalar
release uncorrected.

Preserve exact v1 C `40c6d84c1e78c29a78384bee4dc1f435a53032f31655e122949921275a5c2289`
and header `8c45f279a09dce01b53d928e2fa024663093f6955c469d0b71bb18ba72947c47`, all
source/evidence and independent review. Its five HOST/ARM compiles, two REL links,
33 commands and ONE normal174080B/four-correct-cursor/504-packet observation remain
their actual facts, not an R1 test or full ownership pass. No runtime adoption,
negative test, second HOST run or changed historical result.

Parent separately selected a minimal fresh v2 correction: distinct monotonic
acquisition identities, canonical per-consumer active tokens and consumer-tagged
views, checked independently by release and deferred effect validation. V2 was
compiled/linked only; independent review and parent closeout remain required.
No native/runtime/cap acceptance, source END/DMA retirement, hardware or main
integration follows. Camera last controller OFF after23h; RAM75 block unchanged.

This parent document is deliberately outside frozen v1/v2 source/evidence
directories; their membership and sealed original pins remain untouched.
