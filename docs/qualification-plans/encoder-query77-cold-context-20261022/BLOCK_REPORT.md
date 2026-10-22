# C2b blocked before code — missing complete public cold-record grammar

2026-10-22; BASE-01/05/07 / PLAT-06/08. **Outcome: bounded SOURCE investigation /
concrete design block, not a classifier implementation or SOURCE/API acceptance.**
The permitted no-code branch was selected. This report does not diagnose a new
target failure, weaken an old parser or clear either original HOLD.

## Why no positive parser can honestly be delivered from these inputs

The inspected public files describe tokens and transport heuristics, but not the
complete accepted language for an immutable arm+ON-to-held capture interval. A
strict parser must reject unclassified records; without the legitimate complete
record grammar it cannot distinguish those records from arbitrary/echoed text.
Allowing unknown lines around required tokens or inventing line order would be
the permissive production classifier that this task expressly excludes.

| Pinned public anchor | What it supports | What it does **not** establish |
|---|---|---|
| `scripts/qc07f_boot.py:32,556–579` | Version byte constant; `caught` finds last `CRLF CRLF + BANNER`, then searches suffix for board/DRAM fragments and kernel text absence; uses recent tail plus host0.1s quiet predicate | Full banner line/suffix/terminator; unique genuine line/cardinality; complete interval grammar; actual board/banner order; new10s stability or present runtime |
| `Boot.marker_seen:183–188` | Existing line-oriented prompt regex allows trailing space/tab/CR; other markers often use substring matching | New exact full-interval prompt/tail grammar, echo disambiguation or strict CR/LF policy. A prompt match somewhere is not full-context proof |
| `Boot.wait/check_stream:361–413` | Existing epoch reset/partial-banner rejection and source-defined loader-record handling | Grammar for initial original cold boot. These inspect an already established epoch or QCL protocol; neither supplies the missing original boot records |
| `Boot.command/wait_handoff:437–470` | U-Boot echo-before-CR and specific handoff rejection/Starting-kernel checks | A cold transcript grammar. Command response patterns/echo cannot be silently reused as permitted startup records |
| Pinned `uart_capture.py:18–22,149–181` | Last-banner buffer trimming; banner/DRAM/Net membership triggers; password/prompt/SF/kernel text can cancel pending catch bursts | Safe trimming by a new classifier; full line syntax/order; absence of earlier/repeated boots or unknown RX. Buffering/triggering is not a positive record parser |
| Recorder `:204–247` | Arm/profile/catch behavior and existing prior-CLI acquisition dependency | Actual present arm/ON/sole owner, complete RX/TX lineage, current unconsumed catch or authority to read the referenced historical raw file. No referenced raw was opened |
| `WATCHDOG_RESET22B_RESULT.md:5–32` | Historical reset controller FAILED/HOLD; separate observed original board5/DRAM64/held tail; password text also preceded held prompt in successful original cold catch; later10s receive-only stability | Exact line layout/spacing/order, complete banner suffix, full pre-prompt/control/echo context. It cannot retroactively pass the failed controller or provide current quiet/catch proof |
| Controller design `IMPLEMENTATION_CONTRACT.md:166,177,187–210` | New P2 requires complete context, unique banner, no unclassified/repeat/partial/handoff contradictions; password observation separate from positive held context | An already implemented/reviewed grammar. It explicitly leaves exact surrounding banner grammar to independent review |

In particular, the **board record order is unknown here**. `Boot.caught` searches
its chosen suffix after the last banner prefix; that search-window decision is
not independent evidence that the real original board record must follow the
banner. The public result gives board5/DRAM64 scalars, not an ordered transcript.
No banner-before-board rule (or opposite rule) is invented or inferred from it.

The literal `BANNER` is not specified as a complete line by these sources. Whether
an actual line includes additional date/compiler/build text remains unresolved;
no suffix, wildcard or standalone-line acceptance was fabricated. The same is
true of exact record boundaries/cardinalities for board/DRAM and other output.

## Whole-interval and password boundaries retained

- A future parser must consume **all** caller-provided bytes, bounded<=64KiB,
  without discarding a prefix, selecting the last boot, accepting a caller offset
  that removes contradictions, replacing invalid bytes, or broad whitespace strip.
  Exact newline/CR handling and max line/count bounds require the missing grammar.
- Last-banner slicing in the two old helpers is historical transport behavior,
  not a reusable proof that earlier bytes are harmless. Prior/partial/repeated
  banners or unknown records cannot be hidden to manufacture a positive result.
- Password text before a later actual held prompt is an independently retained
  observation, neither automatic failure **nor** positive held proof. Residual/
  late challenge, prompt-like echo, missing/partial context must not become held.
  Exact password/prompt/echo/control-byte placement is not publicly specified.
- `Starting kernel` and `SF:` source stop heuristics do not give a full ordered
  grammar or an independently reviewed distinction among all possible startup,
  flash-read and command-echo records. No permissive SF exception is guessed.
- A byte string can never establish actual ON, current clock/budget/catch, sole
  owner, complete capture, audited TX lineage or10s RX/TX quiet. Even a future
  complete offline-context classification must not be named stable-held/GO or
  treated as live admission. Silence/text absence is not stability evidence.

No token-counter stub is emitted: it would not meet the positive-context contract
and could be mistaken for the selected P2 prerequisite. The block is the delivered
result; there is no API/classification enum that a controller can adopt.

## Smallest precise missing dependency — parent public grammar packet

A later **separately selected** parent source/evidence task could prepare a public,
privacy-reviewed layout specification from appropriately authorized original cold
evidence (or exact relevant original bootloader source). This report does not
open or request that private evidence. Minimum content for source progress:

1. Complete non-identifying banner record syntax, including any actual suffix and
   exact line terminator; provenance tied to original boot identity/public source
   family, not an invented sample or guessed compiler/date grammar.
2. Actual permissible board5/banner/DRAM64 order and cardinalities, including all
   legitimate pre-banner records. Explicitly distinguish implementation search
   windows from observed original record order; do not drop earlier context.
3. Finite record/blank/separator/echo/control-byte inventory covering the **entire**
   allowed interval through the final held prompt. Narrow bounded public field
   rules for any redacted variable record, or leave it UNKNOWN/blocked; no wildcard
   arbitrary-line allowance. Resolve line count/length and partial-record bounds.
4. Exact early-password-to-held layout and tail alternatives: how complete held
   prompt differs from outstanding challenge, split/partial prompt or echo. No
   password value, response text, RX/TX payload, private identifier or UUID.
5. Explicit contradiction/truncation rules for repeated/new/partial banner,
   kernel/flash-read handoff and any unexplained output. Review against the whole
   public grammar, not replayed/synthetic traces in this deferred-test task.

The public packet must not contain captures/raw logs, private capture paths/
digests, boot UUIDs, device labels/MACs/serials, credentials or private errors.
Parent keeps private provenance/owner/TX evidence outside Git. A reviewable
grammar is not admission: authentic complete interval selection and every live
lease/physical/quiet requirement still belong to a separately reviewed parent
controller and fresh actual evidence. If privacy prevents a strict public rule,
keep that record/context unclassified rather than silently accepting it.

No user power/serial action is needed or requested for this source block; no
private inspection, implementation worker, test or native task is automatically
queued. Independent review of a later grammar/candidate precedes adoption.

## Evidence, immutable inputs and disposition

- `SOURCE_CONTRACT.md` was created first. `INPUT_PINS01.tsv` records11 public
  source/document inputs, after substantive reading and that contract, before
  this block report. It is not a pre-use/native/build seal. Both primitive source
  SHA pins match the original public design manifest.
- No raw/private capture, historical CLI file, current recorder/status, actual
  HOLD/control path, proc/dev/network/UART/device was accessed. No old helper/
  controller/candidate import, call, compile or retry. No fixtures/synthetic traces,
  tests or code objects. Zero new Python units: syntax is NOT_APPLICABLE, not PASS.
- Original design/helper/source licenses and notices remain unchanged. Only short
  already-public token names/source anchors are documented; no implementation or
  acquisition payload is copied, no new global license/publication grant.
- Source pin equality establishes inspected-file identity, not runtime ownership,
  genuine boot/capture authenticity, full build closure or hardware retirement.
  No memory/CMA/allocation ceiling,128KiB/40MiB/native/host limit changed.
- Original radio and reset HOLDs stay failed and unread here. Reset's separate
  historical recovery/stability remains separate, no retrospective PASS/reset
  retry or restored Linux77. The parent-reported C2a offline public-input result
  does not fill any transcript grammar/capture/stability dependency.

**Handback: C2b implementation blocked on the specific public grammar packet
above. Deliver docs/pins only; no source API, admission or next operation.**
