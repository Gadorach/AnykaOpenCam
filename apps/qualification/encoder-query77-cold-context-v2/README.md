# C2b v2 exact observed-profile classifier — unreviewed pure SOURCE candidate

`cold_context.py` (GPL-2.0-only) implements only the parent-published symbolic
19-record grammar in `docs/qualification-plans/encoder-query77-cold-grammar-20261022/OBSERVED_PROFILE.md`,
spec SHA `daff4d31fcb034354df154fd0c5233824738150ee84ab59f9b862bf0f8c85437`.
The earlier blocked C2b report/contract/input pins remain frozen: this fresh
candidate has a new public dependency; it does not rewrite the earlier finding.
Parent privately verified grammar provenance, not this author. No private file
or metadata path/digest was opened, discovered, requested or exported here.

## API and exact scope

`classify_cold_context(data)` is the sole operational API. Only exact built-in
immutable bytes, length<=65536; subclass/bytearray/view/other type or excess size
raises `ColdContextError` with a fixed `ErrorCode` and fixed message, using
`raise ... from None` to suppress displayed exception chaining. Ordinary Python
tracebacks can still retain frame locals; this is not secure erasure or permission
to export exception objects, tracebacks or debug-local dumps. The result/error
fields themselves do not include input bytes.
No input offset/profile selector/callback, file reader, CLI/main/example launcher,
import-time IO, C1/C2a integration, clock/owner/catch getter or effect interface.

The source walks exactly one private fixed19-record table from offset0. Each
record has exact literal bytes/CRLF except the final prompt, which has no EOL.
No optional records, alternate dates, variable spaces, lower/casefold, decode,
strip, regex search, last-banner/prefix discard, retry or fallback. Mandatory EOF:
any trailing byte or second context is unsupported. No whole expected transcript,
golden input/sample/fixture or replay is assembled, emitted or saved.

Returns frozen `TranscriptFacts` with four scalar fields only:

- `classification`: EXACT_OBSERVED_PROFILE_NOT_ADMISSION,
  PROPER_PREFIX_INCOMPLETE, or UNSUPPORTED_CONTEXT.
- `matched_records`: number of completely matched records, bounded0..19.
- `matched_record_bytes`: boundary after those complete records, bounded by input
  length<=65536; **not** the first differing byte, restart point or skip permission.
- `challenge_record_matched`: true only once the complete expected challenge
  record, including CRLF and required preceding records, matched. It may remain
  true for incomplete/unsupported later context and never implies acceptance or
  asks/generates a password/key response. Challenge text elsewhere is not this fact.

Empty input and proper prefixes stay incomplete, including partial final prompt;
nonprefix mismatches cannot be repaired by later matching text. The input bytes
are borrowed for the call and not retained in the result. No raw record names,
tokens/bytes, private identities, UUIDs, hashes or diagnostics in facts.

At most19 literal comparisons plus EOF, with only a suffix **shorter than one
record** copied for proper-prefix detection. No full64KiB copy/splitlines
amplification, recursion, dynamic evaluation or payload parser. Authored buffers
do not prove native128KiB/40MiB or Python RSS fit; no limit was increased.

## What exact bytes cannot establish

Caller must independently preserve/select the whole authentic after-fresh-arm/
actual-ON interval, all prior/later RX/TX, complete capture, source and sole owner,
current catch/coverage/budget and10s RX/TX quiet. A byte match cannot establish
those facts, cold rail power, genuine origin, current runtime, stable held prompt
or C1/E2/native readiness. Never discard later data to manufacture a match.

This profile is not the language of every original boot, another software date
or the later watchdog-reset trial. Identical bytes cannot authenticate physical
origin; no reset-specific acceptance branch exists. Original radio/reset HOLDs
and failed reset/password classifier stay unchanged and un-retried. Expected
challenge-before-prompt in this exact profile is not a retrospective old-trial PASS.
DRAM64MiB/SPI/FLASH/MMC/LOAD are software/status text, not memory capacity or
authority for RAM/flash/card/boot/peripheral actions. Console `serial` means generic
routing, not a device serial number.

## Delivery evidence and restrictions

Pre-code `CONTRACT.md`, pins/provenance/static callsite and grammar inspection,
syntax/method seals are in `docs/qualification-plans/encoder-query77-cold-context-v2-20261022/`.
New-text HOST AST/in-memory syntax only; no classifier/helper/code-object import/
call/execution, fixtures, synthetic traces, generator, tests or private/current/
device/serial/RPC/network/process activity. No Git/index/AGENTS/central-plan/old
source/model/provider/safeguard edit. All source licenses/notices retained, no raw
capture or new global license/publication grant. Independent grammar/API review
remains required; no automatic call/test/native/physical action/next worker queue.
