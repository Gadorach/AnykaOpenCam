# C2b cold context — source contract before implementation decision

2026-10-22; BASE-01/05/07 / PLAT-06/08. Parent selected only a bounded offline
source increment toward new-controller P2. **No parser implementation is selected
unless the inspected public sources support strict complete-record context.**
This contract is written first; all prior source/design/review/results/HOLDs stay
frozen. Only the two new cold-context source/documentation paths may be written.

## Allowed hypothetical library boundary, not an implemented API

Self-contained pure library only: no CLI/main/import-time IO, filesystem/capture
reader, callbacks, syscalls, old helper import, wire generator, C1/C2a integration,
clock/catch/owner/source getter, state machine/controller or effect. A future
explicit argument could be one **immutable bytes** interval, length<=65536, with
the whole interval consumed. No prefix slicing, last-banner selection or caller
offset that hides contradictions. Caller-selected authentic capture interval
after fresh arm and actual ON is a separate provenance requirement, not proven
by bytes/type/flags. No actual input, fixtures or synthetic traces are supplied.

Any future result could expose only bounded transcript facts, offsets/counts and
fixed offline enum/reasons; never raw payload, UUID/identifier/digest/error text,
stable-held/ready/GO/admission or catch lease. Actual sole ownership, complete
capture/RX-TX lineage, user ON, live catch, clock budget and10s quiet interval
cannot be derived from text alone. No argument or transcript enum supplies them.

## Public anchors currently available — not a grammar

- Boot source fixes `BANNER = b"U-Boot 2013.10.0-AK_V2.0.03"` and searches for
  `b"\r\n\r\n" + BANNER`, then uses the **last** occurrence and substring checks
  for `board_version: 5`, `DRAM:  64 MiB` and absence of `Starting kernel`.
  Those checks do not establish full banner-line bytes/suffix, exact record order,
  allowed intervening records or safe interpretation of an entire interval.
- Recorder catch uses banner/DRAM/Net membership; it can cancel remaining bursts
  on password text, prompt, SF or kernel text. Its newest-boot helper discards
  earlier bytes. This is transport/catch source, not a full RX grammar and not
  permission for a classifier to discard an earlier banner/contradiction.
- Public reset result reports observed board5/DRAM64/held tail and password text
  **before** the held prompt in the successful cold catch. It does not publish
  exact line layout, banner suffix or board/banner order. The reset controller's
  original FAILED/HOLD and separate later recovered-to-held observation remain
  distinct. No parser relaxation or retry is authorized.

## Minimum complete context needed before code

A proposed positive parser must have independently reviewable source/evidence
support for: complete original banner line/terminators (including any date/compiler
suffix), board5 and DRAM64 record boundaries and actual permissible order; every
record/blank/echo/control-byte layout allowed from arm+ON interval start to final
line-anchored `anyka$` tail; strict CR/LF handling; password-text placement versus
late challenge; repeat/partial banner and handoff/flash-read/unknown-record
contradictions. Do **not** invent banner-before-board order, a generic wildcard
allowlist, broad strip/decode, token-anywhere positive rule or safe-looking sample.

Unknown/missing/partial context must remain unclassified/incomplete or blocked,
never positive production classification. If the public anchors cannot supply
this grammar, deliver a **concrete source block with pins, no code**, specifying
the minimum parent-prepared public redacted grammar dependency. Do not emit a stub
that could be mistaken for a qualified parser, acceptance enum or ready service.

Parent alone could later separately select private evidence inspection and publish
a privacy-reviewed layout description; this task does not open captures, expose
digests/identifiers/UUIDs/passwords, request user action or infer such authorization.
No parent inspection, physical action or operation is queued by this contract.

## Resource, evidence and preservation limits

Any later implementation must bound lines/counts/loops and avoid full-capture
split/copy amplification; keep128KiB/40MiB/native/host limits, no memory-fit claim.
Explicit fixed safe error/enum guards, not assertions. Offline facts never prove
quiet stability, current runtime or radio/rail/DMA retirement. Last12:12UTC held
U-Boot/catch/HOLD facts are historical; original radio and reset failures persist.

Only static public source reads/hashes and, if genuinely new code is justified,
HOST AST/in-memory syntax without executing/importing it. No fixtures, synthetic
traces, library/helper/generator/test/client/CLI/code-object execution, actual
private/control/HOLD/current-state/proc/dev/UART/RPC/network/device/process access,
Git/index/AGENTS/central-plan/model/provider/safeguard edit. Retain licenses and
all failures. Exact source pins before/after, static inspection and method seal
must distinguish blocked design from implemented/reviewed/runtime evidence.
Independent review precedes any later controller adoption; no next queue.
