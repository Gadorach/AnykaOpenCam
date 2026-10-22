# Author static inspection — exact grammar, not independent acceptance

Source: `apps/qualification/encoder-query77-cold-context-v2/cold_context.py`,
98 lines, SHA256 `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558`.
Normative spec pin is in PROVENANCE.md and source line11. This is ordinary static
inspection/reasoning only; no classifier invocation, fixtures, synthetic traces,
generators, tests, imports or execution. No private provenance/capture was read.

## Literal/order inspection

Public spec lines44–69 was read alongside source lines16–36. All spaces, case,
punctuation, software build stamp and CRLFs match as written. No full expected
transcript was assembled or supplied to a comparison function.

| Position | Source line / symbolic record | Public production |
|---|---|---|
|1–2|17–18 / LEADING_EOL_1, LEADING_EOL_2|EOL EOL|
|3|19 / BANNER|complete exact BANNER + EOL|
|4|20 / BANNER_SEPARATOR|EOL|
|5–7|21–23 / DRAM, SPI_FIRST, FLASH|DRAM EOL SPI EOL FLASH EOL|
|8–10|24–26 / MMC_INIT, MMC_HOST, SPI_SECOND|MMC_INIT EOL MMC_HOST EOL SPI EOL|
|11–13|27–29 / CONSOLE_IN, CONSOLE_OUT, CONSOLE_ERR|each corresponding token + EOL|
|14–16|30–32 / LOAD, BOARD, NET|each corresponding token + EOL|
|17|33 / CHALLENGE|CHALLENGE_TEXT + EOL|
|18|34 / PROMPT_SEPARATOR|EOL|
|19|35 / PROMPT|PROMPT, no EOL|

The repeated SPI literals appear only in the two mandated positions; board is
after DRAM/LOAD. Literal CRLF is part of each nonfinal record, including blank
records and challenge. The final prompt excludes whitespace/newline. There is
no optional record, alternative software date or reset-profile branch.

## Branch and bound reasoning

- Lines74–77 guard exact built-in `bytes` before length/byte methods. Subclasses,
  views, mutable arrays and other objects cannot dispatch their payload methods;
  oversize has its own fixed code. No assert or payload interpolation.
- At line79 offset/count/flag start at zero/zero/false. Each iteration begins at
  the last full-record boundary; only full success advances offset/count. There
  are exactly19 fixed records, no input-dependent search, restart or recursion.
- At lines81–87 remaining bytes are shorter than one literal. Only this short
  suffix is sliced; the largest literal is54 bytes, so a copied suffix is at most
  53 bytes. Exact prefix of that literal is INCOMPLETE, including zero remaining
  bytes. Otherwise it is UNSUPPORTED. Count/offset exclude a partial record.
- At lines88–90 a full-length nonmatching record returns UNSUPPORTED immediately.
  `bytes.startswith(literal, offset)` compares at the current boundary without
  slicing the full input. It does not search for a later banner/token to repair it.
- Lines91–94 advance only after successful complete record matching. Only the
  unique CHALLENGE table entry sets the flag, so record17 must match with CRLF
  and all required predecessors. Partial challenge or occurrence elsewhere is
  not this fact. The flag can remain true on later incomplete/unsupported context;
  classification, not that flag, distinguishes the complete production.
- Lines96–98 require offset equal to full input length after all19 records. A
  trailing byte, newline, echo or another boot makes classification UNSUPPORTED;
  it does not silently consume/discard the tail. No other positive return exists.
- Empty input and any proper production prefix return INCOMPLETE at the first
  unavailable/partial record; exact EOF can only occur after the final prompt.
  These are source deductions, not executed cases or measured runtime results.

Results contain only enum/int/int/bool in frozen slotted TranscriptFacts. Returned
counts are0..19 and byte boundary is no larger than supplied length<=65536; no
raw record, label, digest or payload escapes in result fields. Record labels are
private grammar metadata, not diagnostics. External direct construction of the
data classes is not classifier validation; guarantees apply to classifier returns.
Conventional Python globals/frozen dataclasses are not a security boundary against
caller monkey-patching or bypass; no application integration is supplied.

Error fields are fixed enum/message values; from None suppresses displayed chains.
Ordinary Python traceback locals may retain arguments, so this is neither secure
erasure nor authorization to publish traceback objects/debug-local dumps. No input
bytes are deliberately assigned to result/error fields or logged.

## Scope findings

No file/capture getter, current source/owner/clock/catch query, callback, serial,
RPC/network, generator, command, response/key, device interface or old-helper
import appears. No whole transcript is joined/concatenated or serialized. The
only input-derived copy is the short suffix above. This does not measure total
Python allocation/RSS or prove native128KiB/40MiB fit; limits remain unchanged.

An exact byte match is NOT admission, origin/authentic-window proof, stable hold,
quiet interval, current catch/owner/budget or cold radio/rail/DMA retirement. All
earlier/later RX/TX and live requirements remain parent gates. The original reset
failure is not reclassified or retried; no grammar branch for that trial is added.

**Author conclusion:** source implements the selected public exact production by
static inspection. This is one unreviewed candidate, not independent grammar/API
sufficiency, runtime behavior confirmation, controller adoption or native GO.
