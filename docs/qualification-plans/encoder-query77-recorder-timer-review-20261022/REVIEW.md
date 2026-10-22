# Independent focused recorder stored-timer SOURCE/API review

Task date: 2026-10-22, supplied context only; no clock/timestamp observation.
Scope: ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Ordinary selected-delta review,
not a broad inherited-code audit or penetration/adversarial/mutation campaign.

## Disposition

**Narrow conditional SOURCE/API sufficiency for the exact three selected changes.
No correction-required finding in that delta.** This does not admit execution,
installation, deployment, a new controller/profile, source-pin replacement or any
current/native/continuity/coverage claim. Parent must read and reverify this packet
before any curated local source commit. No Git/index operation occurred here.

| Artifact | Independently read/hash-checked identity |
|---|---|
| Original recorder, complete read | `work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py`, 322 lines / 25426 bytes, SHA256 `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00` |
| Candidate recorder, complete read | `apps/qualification/encoder-query77-recorder-timer/uart_capture.py`, 340 lines / 26563 bytes, SHA256 `be0fee5d7aaa55aba5616d04689e309ee9a38255376188ede074cd5be1c7392d` |
| Candidate README, complete read | 4412 bytes, SHA256 `b980eeff43e41038fae898d9953b2761adb439ae3d038df5738bf527037a98d9` |
| Author method seal | 7330 bytes, SHA256 `b4356449aa1a47c03dbdd30cbfa0282beaa9cbb17b8638a13ca0acca80b9ab25` |

Candidate source aggregate (recorder + README, sorted path/NUL/raw digest):
`33cd62fd5ec8d55e4a2fbd019d45489b87d5edf76c2850971b16f7fbf6c6b4b4`.

## Selected changes and reasoning

1. **ROOT association, candidate line13.** The fixed canonical original
   implementation path preserves the intended lexical CONTROL association
   (`workspace/work/qc-uart-live`) and original historical/output expressions in
   this copy. CONTROL/SOCKET and all their users remain unchanged. This is SOURCE
   association only: no actual directory/control/history path was opened, resolved
   or qualified for custody. Original/default PORT remains historical and untouched;
   this review does not diagnose it or infer any present adapter.
2. **Stored-value formatter, lines66–79.** Exact built-in `float` start and `int`
   duration checks short-circuit before value methods/comparisons on unsupported
   types. No bool, subclass or coercion is accepted as duration; range is120..14400.
   Negative start is refused; NaN and positive infinity reach `as_integer_ratio`,
   whose ValueError/OverflowError are replaced by fixed ValueError text with
   suppressed chaining. Negative infinity is already rejected by the sign check.
   Signed zero is not normalized. With the exact built-in types, the remaining
   methods cannot dispatch to user overrides. The finite ratio is discarded, not
   a replacement start or timer. `.hex()` represents the original stored float;
   the original integer duration is returned unchanged. Six call nodes are only
   two `type`, two fixed `ValueError`, `as_integer_ratio`, and `hex`. No new clock,
   I/O, callback, mutable cache, deadline, now/remaining, arm or capability exists.
3. **Actual status integration, lines220–225.** The single formatter caller is
   line224, under both private capture and `req.get('private_status') is True`,
   before the unchanged response. Nonprivate/default/missing/false/truthy-non-bool
   requests do not get the field. `state` is newly assigned a dict at221 on every
   status request; there is no reuse/cache and thus no prior-request field bleed.
   The unchanged `capture_status` dict copy preserves an inserted field only on
   that request. Its private-capture argument uses existing truthiness, whereas
   its RX override and the NEW field gate require exact True. No policy broadening
   is inferred from preexisting privileged status fields. The CLI still refuses
   exact private-status requests; no option/action/access route was added.

The fixed nine-field record agrees with TIMER_CONTRACT.md and README: schema,
version, start_encoding, start_hex, duration_seconds, clock_api, loop_predicate,
scope and arm_timer_role. In particular the scope is
`STORED_VALUES_ONLY_NOT_DEADLINE_OR_ADMISSION` and the arm role is
`CAPTURE_TIMER_ONLY_NO_INDEPENDENT_ARM_DEADLINE`. Names/constants are public source
metadata, not actual absolute timer values. No real or synthetic timer value or
request/response was obtained, constructed, serialized or published by this review.

On formatter refusal, the existing private handler at301–303 returns/logs fixed
`Private control request refused`, without raw start/duration/exception text.
No timer or arm assignment/retry is added. This is control-flow reasoning, not
executed refusal evidence; inherited event/response failures can still reach the
outer error/finalizer, so continued operation is not unconditionally guaranteed.

## Independent exact-delta and callsite closure

`SELECTED_DELTA.diff` is the independently computed text diff: three hunks,
+19/-1 lines. Full original and candidate, README, TIMER_CONTRACT.md and
CANDIDATE_REPORT.md were read. A fresh stdlib-only text/AST inspection—not the
author's script—parsed both source texts, copied candidate AST metadata, replaced
only ROOT with the original assignment, removed only the added helper and the
single exact status gate, and compared complete AST dumps without locations.
**Complete equality holds.** No AST/code was executed or compiled in this review.

The equality includes original imports, CLI, all nested closures, source hash,
duration guards, start initialization, loop predicate, UTC arm reply, catch,
newest_boot_bytes, TX/pacing/protected guards, observer and finalizer. All15
`time.*` call expressions retain lexical order and enclosing original expressions;
the two inherited asserts remain unchanged, not newly qualified. Receipt and
line mappings are in `STATIC_INSPECTION01.json`.

The original capture start is still assigned once at candidate118; the real loop
at152 remains `running and time.monotonic()-start<duration`. Float subtraction/
comparison rounding and loop-body/scheduling/stop latency remain; it is not a
rewritten deadline equality. Arm initialization120 and arm assignment263 add no
independent arm deadline. Existing UTC `ends_by` at265 still uses its original
separate wall/monotonic reads and is not a native-expiry bound. Added metadata
work does incur serialization/CPU/allocation cost; unchanged time-call source
order is not elapsed-time equivalence or total-memory qualification. No128KiB
allocation-cap,128-byte protected-TX guard,40MiB/native cap or other limit is raised.

## Source identity, custody and provenance

Candidate117 still hashes `Path(__file__).read_bytes()` for private capture: the
candidate identifies its NEW bytes, not the original SHA. Static inspection of
Boot33/123–146/226–231 confirms the old source hash and canonical original script
selection remain; PrivateBoot133–138 rejects a mismatched captured source hash.
The derivative is not silently accepted through ROOT. Old helpers/source pins,
launchers, profiles and originals are immutable and unchanged. No guard was called.
Authentic running-source/path/peer/PID-start/host-boot/namespace/adapter/capture/
RXTX/catch/ownership association remains absent, not established by source hashes.

The module is a full **effectful recorder derivative, not a pure library**. If
launched it retains UART/socket/TX/filesystem/catch/finalizer effects. Only its new
formatter has the narrow side-effect-free source body reviewed above. No import,
formatter/helper/library/CLI call, serial dependency resolution or launch occurred.

Original shebang/docstring/notices are retained. The original has no explicit
SPDX/license header. Pinned root NOTICE.md documents task-created helpers and
preserves other material's terms, not a global GPL/license/publication grant.
No invented license or private status content is introduced. Later publication
still requires separate privacy/license review. Author's early candidate-only
diagnostic transcription correction remains negative history in the frozen author
report/seal; this review neither deletes it nor labels it a runtime/test failure.

## Input custody and limits

At the initial comparison all25 author input pins and all9 sealed output pins
independently matched, as did the author method seal. Original recorder, Boot,
PrivateBoot, C1v2 and design pins
remain frozen. All6 C2c and7 feasibility sealed outputs also independently match
their seals. No transitive old input closure or URL was fetched/requalified;
prior URL pins are receipt association only, not actual host applicability.

36 fresh reader inputs (the25 +9 + author seal + baseline plan) are pinned in
`READER_INPUT_PINS01.tsv`; aggregate
`022925678d7e935f0b905fc33f724e9e1ee3cfb2b48eef3cdc4b10553e33d958`.
No input drift at first comparison. Earlier central-context drift recorded by the
author is retained, not rebased into any historical manifest. During this review,
the harness announced a parent checkpoint update; the final recheck found changes
only in AGENTS.md, the plan index and execution ledger. Bounded public rereads show
their recorder checkpoint now says candidate delivered/independent review selected,
not implementation or runtime admission. Full byte-diff attribution is not claimed.
33/36 reader inputs remain identical, including every candidate/author artifact
and frozen non-context input. Reader aggregate after is
`d570b326361557382313863fe861303e57c1883296fa8b20846d23fa7aaca2f4`;
exact old/new context pins are in the review seal. The before manifest remains
unchanged; no rebasing or edits to parent context occurred here.

Author HOST AST/syntax PASS is cited from the pinned SYNTAX01.json only, NOT rerun.
No candidate/compiler/generator/formatter/library execution, tests, mocks,
fixtures, synthetic clock histories, fuzzing or mutations were performed. Public
read/hash/JSON/diff/AST/control-flow inspection and new review-document writing
only. Dedicated read/grep/glob/patch tools and inline `python3 -B` stdlib
Path/hashlib/json/ast/copy/difflib handled public text/metadata; no candidate or
author script was run. Source diff was independently recomputed and the saved
diff compared as text; JSON receipts were parsed as data. No Git/index/AGENTS/
central-plan/source/old-review/helper/model/provider/
guard change, network, private/HOLD/control/capture, clock/current-state/proc/dev,
serial/UART/RPC/device access or process inventory/control occurred. One initial
glob of this not-yet-created review directory returned missing-path error; no
files were read there and it was not retried. No policy refusal occurred.

## Remaining non-admission boundaries

Strict C2c/continuity-feasibility operational BLOCK and all existing policy/guards
remain. Stored float identity supplies no nanosecond accuracy, native usable
expiry, remaining/liveness, coverage, continuity, source/owner association, PID
tick/namespace mapping, recovery or manual-readiness proof. No watcher, sampler,
inhibitor, host settings, C1 integration or new generic continuity packet follows.
Historical last target observation remains held original U-Boot12:12UTC; neither
that runtime nor current recorder state is inherited. Both FAILED/HOLDs and the
consumed catch are untouched; no actual HOLD/control/private state was inspected.

No automatic next worker, import/call/test, native recipe, capture/staging/boot/
query, deployment/adoption, physical request or hardware queue is authorized by
this review. Parent alone owns any future hardware/current-state boundary.
