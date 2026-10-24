# Private writer runtime v1 — R1 required, NOT ADOPTED

2026-10-24; B07 / RP-04/05/09; CAP-01/05/06/07, ENC-02/03/05, STREAM-01/02/03.
Parent read the full independent review and actual checked_source/callers.
Accept its **R1 changes-required finding**. No narrow source order/lifetime
sufficiency for this exact delivery; compile success is not source acceptance.

At owned_runtime.c235–245, the second source-time/readiness check precedes the
last blocking auth gate. On gate0, checked_source returns without post-gate source
operation/session or applicable watchdog observation. A legitimate callback can
cross those budgets even under all stated serialization/identity conditions.
Affected callers include presyscall354, outward loan378, release394 and post-END
publication430. Earlier pre-END clock charging does not cover the separate final
callback. Later refusal cannot undo an encode/publication admitted too late.

Select a fresh minimal v2 correction at qc07f-writer-runtime-v2 app/docs: charge
that final callback and any subsequent blocking watchdog observation to the SAME
original start/session/applicable guard before protected effects. No unchecked
blocking gate afterward, renewed starts, new timing framework or weakened budget.
Observed expiry/clock/watchdog failure retains current canonical armed storage
before unsafe acknowledgment/commit/unwind. No whole-interval C1 claim follows.

Freeze original eight app files, both build attempts and author documents, and
independent review. C `af95dacac7f609654270201b18772de1a5ee4370f34ab7e1798fb0f0257dfaab`;
header `1aa528fd9a0d42ddfb8f798b2eecfb3a8361c5acd1bb07e6a3ab0e9bd5bc7879`.
Review original182-file aggregate is
`944d6a64b6b78e77e18ed06240114d77f5aaa8c139f6247438a4f17ba2026a99`
(complete relative-string order; path TAB bytes TAB SHA256 LF). Parent records are
outside frozen author/review directories. No original failure is relabelled.

Build01 remains FAILED_PRESERVED: six metadata commands, zero C/REL, wrong pin-map
lookup. Parent explicitly selected one build02 correcting only lookup/output path;
source unchanged. Saved14 HOST/ARM compiles/2 rooted REL links/44 commands PASS
remain compile facts, not an R1 pass. Independent review checked source/phase/
ownership/pin/ABI/role/relocation/archive evidence read-only. No candidate/test/
negative-case execution; R1 is ordinary static reasoning, not observed runtime.

Accepted encoder spans, producer hub and writer-history memory are not revoked or
automatically adopted by this rejected runtime. Other reviewed local boundaries
are not blanket acceptance. New v2 requires independent ordinary review and parent
disposition before use. Native/frontend/borrow-end,128KiB/raw/process2MiB/kernel
DMA/CMA/TLS combined load/external recovery/backing/provisioning/main/entropy/image
pairing/strict C1 remain open. Historical23h OFF/finalized, RAM75 recovery block,
Query78 parked and all original failures remain. No execution/hardware queue.
