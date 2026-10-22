# Independent explicit-port recorder SOURCE/API and selected privacy review

Task date2026-10-22 is supplied context, not a clock observation.
ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Ordinary focused selected-delta review,
not an inherited-code security audit or runtime/adversarial campaign.

## Outcome

**Narrow conditional SOURCE/API and selected privacy-delta sufficiency. No
correction-required ordinary defect identified in the selected changes.**
Parent must read/reverify this packet before any curated local source commit.
No execution, installation, deployment, source/profile adoption, hardware/current
state, full privacy/publication clearance or automatic next operation follows.

| Artifact | Independently checked identity |
|---|---|
| Fresh recorder | `apps/qualification/encoder-query77-recorder-explicit-port/uart_capture.py`, 353 lines/27283 bytes, SHA256 `c8433e225675bf16ca153d3b2c700cb4d3ab80dd70083d4e4a7bc19dffc19445` |
| Fresh README | 3885 bytes, SHA256 `539f2fcbf792febbc167697e19291b231b7dda5ded740e253c5cb20949c5fbed` |
| Frozen timer baseline | 340 lines/26563 bytes, SHA256 `be0fee5d7aaa55aba5616d04689e309ee9a38255376188ede074cd5be1c7392d` |
| Frozen original recorder | 25426 bytes, SHA256 `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00` |
| Author method seal | 5424 bytes, SHA256 `22e39cdd1b0710901eb3cae4dbe1fe51fa99d31d3b5581017abaee3cb4106bbc` |
| Frozen earlier review seal | 7442 bytes, SHA256 `a0311797f796cc2632549c254ff6f68f4db7013225a93353b72b5605b6de0b79` |

Two-file fresh-source aggregate:
`c99c724b53fe3fe2862e97c4c9a33574b8563b8b6a3a19a17e72301b6b8a23ef`.
Aggregation is SHA256 of sorted workspace-relative UTF-8 path + NUL + raw digest.

## Exact API/ordering review

1. **No selected adapter default.** The fresh module has no PORT assignment or
   PORT Name nodes. The signature at97 is
   `capture(duration,part,selected_port,private_capture_dir=None)`: selected_port
   is required; its only trailing default is for private_capture_dir. Missing
   selection therefore fails Python argument binding before the function body;
   explicit None reaches the validator and is refused. There is no replacement
   fixed selector, environment/config lookup, discovery, enumeration or fallback.
2. **Validator,86–95.** Exact built-in str is checked before `len`, slicing,
   membership or methods. The boolean short-circuit excludes arbitrary objects,
   subclasses and coercion before their value methods can be used. Total string
   length must be1..512, not a filesystem NAME_MAX assertion. The exact public
   prefix `/dev/serial/by-id/` is required; the remaining nonempty single component
   is checked by `re.fullmatch(r'[A-Za-z0-9_.:+-]+',component)`. Empty component,
   single dot, any double-dot substring, slash/backslash, whitespace, NUL/control
   and non-ASCII characters are refused by the explicit checks/ASCII grammar.
   No trimming, case conversion, normalization, path/FS resolution, device query
   or substitution occurs. Every authored refusal is fixed ValueError text,
   with no input repr/value/path interpolated. Calls are confined to type, len,
   startswith, re.fullmatch and fixed ValueError construction. The helper neither
   returns a substitute nor modifies the supplied string.
3. **First capture statement,98.** `_checked_selected_port(selected_port)` is
   before even inherited part-name handling at99, private directory handling at
   106–108, CONTROL operations at109–112, path/file setup at113–121, serial setup
   at123, socket creation127 and source/clock/capture setup128 onward. Thus invalid
   selection cannot enter any recorder-owned capture-body effect through this API.
   No actual refusal/effect was run to establish this static ordering conclusion.
4. **Capture CLI,338–346.** The port option is exactly
   `parser.add_argument('--port')`, with no chosen default. Argparse's absent-option
   None is a refusal sentinel, not a fallback. Inside `if args.capture`, the first
   statement is the validation Try at340–341, before the private/public branch.
   It catches ValueError and calls parser.error with fixed safe text. The private
   capture at343 and public capture at346 both pass unchanged `args.port` as the
   third argument; capture then revalidates it. No new path can bypass either
   required selection or the capture-body check.
5. **Request CLI,347–353.** Complete else-branch AST equals the baseline. No port
   requirement/validation, new RPC, option-driven discovery or other request-side
   behavior was introduced. The only validator callers are98 and340; the only
   in-module capture callers are343 and346.

Ordering concerns recorder-owned capture-body actions **after module setup**.
Python imports/argparse/interpreter startup are not sandboxed, dependency-closed
or claimed IO-free. Fixed lexical-validation diagnostics do not constitute a new
guarantee for every inherited argparse/startup/runtime failure.

## Verbatim flow, privacy and provenance

The accepted original string still supplies serial selection at124, inherited
open-intent metadata at125, public-mode announcement137 and status adapter232.
No selected_port assignment/replacement is added. The inherited later metadata
`Path(selected_port).resolve()` remains, but is not a validator operation or an
identity/sole-owner proof. A lexically accepted path may be absent, aliased or
otherwise inadmissible; no VCC/electrical/device existence/identity/ownership/
source association is established. No selected adapter was supplied here.

The full340-line baseline was read/parsed in memory for the selected comparison;
its private assignment and full AST were never printed or copied into new files.
The original322-line source and frozen earlier raw diff were hash-only inputs.
The private baseline literal was extracted only in memory for a bounded absence
check against all10 author artifacts (nine outputs plus seal), and separately
against the new review outputs at sealing. No matched value, sample or
identifier-derived digest is published; pins are whole-artifact source hashes.

The independently recomputed textual comparison agrees exactly with pinned
`PRIVACY_SAFE_DELTA.diff`: four zero-context hunks, +16/-3 lines. **It is not a raw
byte-exact baseline diff.** The old assignment is replaced in memory by an
explicit omission marker before comparison, and the diff header discloses that
fact. The real baseline AST is used only in memory for full equivalence. Neither
the frozen source nor its earlier raw diff is redacted, rewritten or resealed.
Both remain local/uncommitted and must stay excluded from curated Git.

This is only absence of the selected inherited full literal and review of its
replacement API/privacy surface, not a broad privacy/license/publication grant.
Inherited historical references, caller-provided runtime selection, status,
open-intent data and final argv may still contain identifiers; runtime records
must remain private/outside Git. This increment does not repair or qualify the
inherited public capture mode. No actual path, private file or control socket was
opened. Canonical ROOT/CONTROL/SOCKET source association is not filesystem custody.

Original shebang/docstring and root NOTICE provenance remain. No explicit SPDX
header existed in the original; no invented SPDX/GPL/global license or publication
consent is added. Full module remains an **effectful recorder**, not a pure library:
if launched, inherited filesystem/socket/UART/TX/catch/observer/finalizer effects
remain. No candidate, helper, validator, timer formatter, serial module, CLI,
library or inspected function was imported, called or executed in this review.

## Independent selected-delta closure

Full new contract/report/README and actual candidate were read. Independent
stdlib-only source inspection restored only the old PORT assignment, capture
defaults and CLI port-option expression in a metadata copy of the new AST, and
removed only the new validator definition, first capture check and CLI capture
validation Try. **Complete baseline AST equality holds** without location fields.
No AST/code-object compilation or execution occurred. The author inspection
script was not run; author HOST AST/syntax PASS is cited only from pinned
STATIC_SYNTAX01.json, not reproduced as compile/runtime evidence.

This equality retains canonical ROOT/CONTROL/SOCKET, original imports, all
unselected capture body/serial settings, source hash, RX/TX/pacing/protected128B
checks, catch/newest_boot_bytes, historical guards, observer/finalizer and request
CLI. All15 time expressions preserve lexical order and original enclosing
expressions; two inherited asserts remain, not newly qualified. No128KiB
allocation cap,40MiB/native cap or timing/memory bound is raised or proven.

Timer formatter65–78 and capture_status60–63 are AST-identical to the baseline.
Status still builds fresh state232 and adds timer metadata only for private capture
AND exact private_status True at234–235, with no previous-state cache. Start129,
loop163 and UTC arm reply276 are unchanged except line positions. Stored float hex
and integer duration are not a fresh clock, nanosecond accuracy, remaining value,
usable native expiry, coverage, source/owner or continuity proof. Float subtraction,
loop/scheduling latency and serialization overhead remain unqualified; no separate
arm deadline is introduced. Strict C2c/continuity block remains.

The private source-hash expression at128 still hashes new __file__ bytes. Pinned
Boot/PrivateBoot sources retain the old hash/path guards and are unchanged from
the prior review; they do not admit this copy. No original pin, guard, launcher,
profile, helper or source was edited/called, and no runtime adoption follows.

## Inputs, drift and preserved negative history

The author manifest is parsed using its actual **path/bytes/sha256 header**, not
the earlier manifest's column order. All14 entries were independently inspected:
10 frozen inputs match; four context paths changed since the author snapshot
(AGENTS, index, execution ledger, resume). All9 author output pins, author seal,
source aggregate, original/baseline pins and all4 frozen earlier review outputs
plus its seal match. Existing manifests remain immutable, with no rebasing.

The harness announced parent checkpoint consolidation. Bounded context reads show
the fresh candidate delivered/independent review active and preserved limitations;
no full byte-diff attribution or actual current state is claimed. Exact author
before/current context pins and aggregates are in STATIC_INSPECTION01.json.
The new29-input reader snapshot includes the14 selected inputs,9 outputs, author
seal, platform plan, old-client sources and two additional earlier review outputs.
Its aggregate is
`4db63c1bdb0faae95e7c436dadd954db38d98b88db08be44e61d764c4aa32953`.
The final review seal records the reader window separately.

Author's initial wrong AST main-statement index and missing fresh-directory glob
are retained as historical collection/inspection errors; the author reports no
candidate repair or execution. Parent's separately reported fixed-column
FileNotFoundError and subsequent header-aware pin check are also preserved, not
rerun or relabelled as candidate failures. No new collection error or policy
refusal occurred in this review. No retry/reroute/provider/model/guard change.

Dedicated public read/grep and new-document patch tools, plus inline `python3 -B`
stdlib Path/csv/hashlib/json/ast/copy/difflib, handled source text and metadata only.
No inspected source/script, compiler/generator/CLI/example/help, code object,
test/mock/fixture/synthetic clock history, adversarial/fuzz/mutation/penetration
or runtime campaign was executed. No clock/current-state/proc/dev/private
capture/analysis/HOLD/CONTROL/serial/UART/RPC/network/device or process inventory/
control access; no Git/index/central-plan/AGENTS/source/old-review/helper/
provider/model/safeguard edit. Writes are confined to this new review directory.

## Non-admission handback

No watcher/sampler/inhibitor/host setting, C1 integration, timer/catch/lease change,
quarantine/retention/provider/module/kernel change or executable native recipe is
selected. PID/tick/namespace, usable native-bound, actual source/peer/owner/capture/
catch/RXTX/host association, recovery and manual readiness remain missing.
Both FAILED/HOLDs and consumed catch remain untouched. Last target observation is
historical original held U-Boot12:12UTC, not inherited current runtime or protection.
Parent alone owns hardware. No import/call/test/adoption/deployment/capture/staging/
boot/query/physical request or automatic next worker/operation is authorized here.
