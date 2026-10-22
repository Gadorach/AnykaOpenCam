# C2b v2 independent review — narrow conditional exact-profile SOURCE/API sufficiency

2026-10-22; BASE-01/05/07 / PLAT-06/08. **No correction-required ordinary defect
found in the selected pure classifier, exact public record table or API/bounds
closure.** Conditional SOURCE/API sufficiency for this exact software profile
only—not source adoption, authenticated capture selection, stable-held/current
state, parser integration, recovery, native admission or a candidate invocation.

## Exact source and public evidence boundary

- `apps/qualification/encoder-query77-cold-context-v2/cold_context.py`:98 lines,
  4019B, SHA256
  `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558`.
- Two-file source aggregate (sorted workspace-relative UTF-8 path, NUL, raw digest):
  `48246eae4c9394d6ed91401fcecd64aea3c541f3067a4dc4342decaa91a506a7`.
- Normative public `encoder-query77-cold-grammar-20261022/OBSERVED_PROFILE.md`
  under qualification docs:7076B/SHA256
  `daff4d31fcb034354df154fd0c5233824738150ee84ab59f9b862bf0f8c85437`.
  Parent's historical layout/privacy attestation is inherited, **not independently
  authenticated from raw evidence by this review**. No private analysis/provenance,
  capture or cited COLD22B actual evidence was read or discovered.
- Read full new contract/report/README/module, normative public specification,
  inspection/callsite/provenance/syntax/method receipts, original block report and
  public subsystem context. Original block remains valid for its then-missing
  grammar; the new dependency does not retroactively change old controller results.

## Per-record grammar review

Independently decoded only literal AST table entries and compared **each individual
record** to the normative token/order production. No whole expected transcript,
input, fixture, sample, golden string or replay was constructed, concatenated,
supplied to a classifier, or emitted. Static schema comparison is not a behavior
trace/test. Exact spaces, case, stamp, punctuation, CRLF and position all match.

| Positions / source lines | Normative association / result |
| --- | --- |
|1–4 /17–20|Two leading CRLF records, complete fixed software-build BANNER plus CRLF, then one blank CRLF. Matches spec44–69.|
|5–10 /21–26|DRAM, first SPI, FLASH, MMC_INIT, MMC_HOST, second SPI; each full record ends CRLF. Exactly the specified two SPI positions.|
|11–13 /27–29|Exact generic input/output/error serial-routing records with CRLF, not device serial identifiers.|
|14–18 /30–34|LOAD then BOARD then NET then complete CHALLENGE_TEXT with CRLF, then blank CRLF. Board is after DRAM, not an invented reordered rule.|
|19 /35|Exact PROMPT without trailing EOL or other byte; EOF remains mandatory.|

All19 records and labels agree with the selected contract. Record lengths in
position order are2,2,54,2,15,30,7,37,22,30,15,15,15,25,18,39,24,2,6. The largest
is54B. These are per-record metadata, not a joined expected input. No optional
record/date/profile, variable field, echo normalization, permissive wildcard or
reset-specific alternative was introduced. Public software/status literals are
not stock binary/credential/device-identifier acquisitions.

## API, branch and resource reasoning

Source anchors refer to the exact98-line module. No classifier/helper was called.

| Area / anchors | Independent ordinary finding |
| --- | --- |
| API66–77; errors45–55 | One argument, no default/offset/profile/callback selector. Exact built-in bytes is checked before length or payload methods, rejecting subclasses/mutable/views/other objects with fixed INPUT_TYPE; size>65536 has fixed INPUT_TOO_LARGE. Both explicit raises suppress displayed chaining. No input interpolation/repr, assertion or unsafe error field. |
| Entry79; loop80 | Zero/zero/false initial offset/count/flag; exactly19 immutable literal records, no search, restart, recursion or caller offset. Each complete match alone advances offset and matched count. |
| Short remaining81–87 | Remaining length is nonnegative by the loop invariant and strictly less than the next literal. The only data-derived slice is therefore<=53B. Literal-prefix comparison returns INCOMPLETE for a proper prefix, including empty remainder; otherwise immediately UNSUPPORTED. Matched count/offset stay at the preceding complete-record boundary. |
| Full comparison88–92 | Exact built-in bytes.startswith at the current boundary; no full-input slice/copy, decode/strip/casefold, split, last-banner scan or prefix discard. A mismatch returns UNSUPPORTED immediately, so later positive text cannot repair it. On success offset increases by the matched literal length and cannot exceed input length. |
| Challenge93–94 | Unique CHALLENGE label is position17. Flag is set only after its complete literal, including CRLF and all preceding required records, matches. Partial challenge or text elsewhere cannot set it. It deliberately remains true if later context is incomplete/unsupported: that scalar is not acceptance or password occurrence anywhere. No response/key generation. |
| EOF96–98 | Exact status can be returned only after all19 records and offset==full input length. Any trailing byte/newline/second context is UNSUPPORTED, even when matched count is19 and challenge flag is true. Proper prefixes cannot reach exact status. |
| Facts58–63,87–98 | Frozen slotted result contains only classification enum, matched-record count, boundary byte count and boolean. Classifier returns bound count0..19, 0<=boundary<=input length<=65536. No raw token names/bytes, input digest, private identity/UUID, ready/GO/stable-held/catch/physical fact. Direct external dataclass construction is not classification or authority. |

The type guard makes both startswith calls ordinary built-in byte operations, not
caller callbacks. At most19 record comparisons and the final EOF decision are
needed; only the short suffix branch copies input-derived bytes. No whole input
split/copy or assembled expected transcript. The result does not retain the input.
Ordinary Python traceback locals can still retain arguments after an exception,
as the README explicitly warns: fixed error fields/from-None are not secure
erasure or permission to export exception objects/debug locals.

## Callsite/declaration closure

AST independently finds exactly17 Call nodes, matching CALLSITE_CLOSURE's scoped
inventory: dataclass declaration58; type53/74; super/base initializer55 (two nodes);
ColdContextError75/77; len76/81(two)/91/97; bytes-prefix operations85/88;
TranscriptFacts87/89/98. Only dataclasses and enum are imported. The sole top-level
function is classify_cold_context; no authored import-time IO, CLI/main/example,
callback registration, file/capture/current-state/owner/clock/source/catch getter,
sender, C1/C2a/Boot/recorder import or controller integration appears.

Declaration decorators, Enum/dataclass/Exception internals and generated methods
were not executed or recursively runtime-audited. Author HOST AST/in-memory syntax
PASS is cited, not rerun; reviewer parsed text AST only, no compile/code object.
GPL-2.0-only notice remains; no global license/publication grant is inferred.

## Pins, drift and collection method

`INPUT_PINS01.tsv` seals29 explicit public reader inputs. Snapshot19:21:01UTC is
after substantive reading, before this report—not a pre-use/build/admission seal.
STATIC_CHECK01 records comparisons and per-record/callsite metadata; FINAL_SEAL01
records final input/output hashes. No transitive kernel/build reproduction claim.

- All18 author input rows compared:15 noncontext match; three central checkpoint
  documents changed after author freeze. Exact original-block/author-v2/reviewer
  context pins are retained, without rebasing either earlier manifest. Harness
  steering describes parent's delivered-v2/active-review checkpoint; full historical
  byte-diff attribution is not claimed.
- Original blocked method seal and all five sealed output pins match. Original
  eleven-input manifest has its same eight noncontext inputs unchanged and the
  three separately recorded contextual changes. The frozen no-code block/syntax
  NOT_APPLICABLE finding is not re-labelled as a source PASS.
- All ten current output pins match (two source files/eight docs, excluding method
  seal itself), as do final source/syntax pin, normative spec and source spec-hash
  metadata. Metadata equality is file identity, not a disk lookup/future freeze or
  proof of actual executed/captured source. Syntax and callsite receipts are not
  functional/native evidence.
- Public read/hash/JSON/AST/per-record comparison only. No import/call/eval/compile
  or candidate/helper/code-object execution, synthetic trace, fixture/generator/
  test/example, process inventory/control or private FS operation. Collection/tool
  errors and policy refusals: none; no retry/reroute. Only this new review directory
  is written; source/old review/design/AGENTS/central plans/Git/index/model/provider/
  helpers/safeguards remain untouched.

## Limits and handoff

The exact software profile can reject future variants; no full original-boot
language or independent raw-layout provenance proof follows. A literal match
cannot authenticate interval origin, actual ON/cold rails, full earlier/later
RX/TX/capture, current source/sole owner, fresh unconsumed catch, immutable clocks/
budgets or the later10s quiet interval. All those remain separate parent gates;
no slicing/discarding later bytes may be justified by this result. DRAM64MiB text
does not replace measured40MiB, and SPI/FLASH/MMC/LOAD strings authorize no effects.

No revised judgment/retry of the distinct failed reset/password controller, no
cleared HOLD, cold radio/rail/DMA retirement, recovery/manual readiness, 4MiB Python
RSS/native128KiB fit, runtime/durability result, current target or product PASS.
Last12:12UTC held U-Boot/consumed catch remain historical only; both actual HOLDs
stay untouched. Parent alone owns hardware/private provenance. **No source adoption,
candidate import/call, integration, worker/test/native/physical or automatic queue.**
