# C2b v2 exact observed profile — contract before code

2026-10-22; BASE-01/05/07 / PLAT-06/08. Fresh unreviewed **pure SOURCE** candidate
only. Parent separately supplied public `encoder-query77-cold-grammar-20261022/
OBSERVED_PROFILE.md`, SHA256
`daff4d31fcb034354df154fd0c5233824738150ee84ab59f9b862bf0f8c85437`.
Its private provenance was checked by parent, not read/reproduced by this author.
Original blocked C2b contract/report/pins stay frozen as valid findings for the
then-missing grammar. No old reset/parser/source or failure result is revised.

## One operational API, no context acquisition

Self-contained GPL-2.0-only library; no CLI/main/example, import-time IO, input/
capture file reader, callback, syscall, clock/catch/owner/epoch/source getter,
wire generator, sender, C1/C2a integration or effect. Sole classifier takes one
exact built-in immutable `bytes` argument, max65536. Reject subclasses, bytearray,
views/other objects and oversize with fixed safe exception codes, no raw/error
formatting or assertions. Empty bytes and every proper prefix are INCOMPLETE.

Consume the supplied interval from offset0, with mandatory EOF. No caller offsets,
prefix discard, last-banner search, normalization, decoding, strip/casefold,
optional record, variable software date or alternate/reset profile. Nonprefix
mismatch is immediately UNSUPPORTED and cannot be repaired by later positive
text. Extra bytes after the final prompt, including a newline or second boot,
are unsupported. These rules are syntactic, not authenticity or live admission.

## Exact19-record table from public specification

Per-record literals only, including their exact endings. Do not construct a whole
expected transcript, golden input, fixture, sample capture or replay string.

| Record positions | Required content/endings (symbol names refer to pinned spec) |
|---|---|
|1,2| each EOL only |
|3| full BANNER including exact software build stamp, then EOL |
|4| EOL only |
|5,6,7| DRAM, SPI, FLASH respectively, each with EOL |
|8,9,10| MMC_INIT, MMC_HOST, second SPI respectively, each with EOL |
|11,12,13| CONSOLE_IN, CONSOLE_OUT, CONSOLE_ERR respectively, each with EOL |
|14,15,16| LOAD, BOARD, NET respectively, each with EOL |
|17| complete CHALLENGE_TEXT with EOL |
|18| EOL only |
|19| PROMPT, no EOL/trailing byte; then EOF |

EOL is exactly CR LF. Spacing/case/build stamp are literal. Board follows DRAM
in this exact observed profile; SPI appears at only the two specified positions.
Generic serial console route literals are software/status text, not device serial
identifiers. DRAM64MiB text remains identity, not a replacement for measured40MiB.
SPI/FLASH/MMC/LOAD strings are not commands or permission for storage/RAM effects.

## Frozen bounded facts, not admission or retrospective acceptance

Return only a fixed classification enum and bounded scalars: number of fully
matched records, byte offset after those complete records, and whether complete
expected record17 (including EOL, at its required position) matched. No record
names, tokens, bytes, raw text, private labels/UUID/digest or exception context in
results. The offset is a **record boundary**, not necessarily the first differing
byte and not a caller-selectable restart/slice point.

Classifications: EXACT_OBSERVED_PROFILE_NOT_ADMISSION, PROPER_PREFIX_INCOMPLETE,
UNSUPPORTED_CONTEXT. Challenge flag is neither password occurrence anywhere nor
acceptance: it may remain true on incomplete/unsupported later context. Challenge
alone never exact; no response/key/password entry generated. The expected challenge
before prompt is not rejected globally as the old controller did.

The input is borrowed immutable bytes, not retained in the result. Walk at most19
records; full comparisons use bounded literal-prefix checks at the current offset.
Only a suffix shorter than one literal may be sliced for proper-prefix detection;
never copy/split the entire64KiB input or assemble an expected whole transcript.
No allocation/RAM ceiling change, native128KiB/40MiB or total Python RSS proof.

## Source/evidence boundaries

Caller must independently select the **whole authentic** after-fresh-arm/actual-ON
window, preserving all earlier/later RX/TX and contradictions. Exact bytes cannot
prove that selection, actual ON/cold rail power, complete capture, sole current
owner/source, unconsumed catch/coverage/budget or later10s quiet. No stable-held,
ready/GO, native/C1 custody or lease result. Text match cannot distinguish physical
cold boot from a different origin with identical bytes. This grammar adds no
watchdog-reset branch and must never retrospectively judge/relabel the failed
reset operation or authorize another one. Both historical failure HOLDs remain.

Author may read/hash public spec/frozen source/docs and AST/in-memory compile new
text only. No candidate/helper/library/code-object import/call/execution, fixtures,
synthetic traces, generator/test/example, private analysis/capture/current-state/
control/HOLD/proc/dev/serial/UART/RPC/network/device/process operation, Git/index/
AGENTS/central-plan/old source/provider/model/safeguard edit. Only the two fresh v2
paths may be written. Exact before/after pins, public-spec provenance, static
caller/grammar inspection and syntax/method seals required; no private provenance
paths/raw/hashes discovered or requested. Independent grammar/API review remains
required, no automatic call/test/native/physical action/next worker queue.
