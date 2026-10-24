# Precompile ordinary source audit — NOT RUN

2026-10-24. Contract was written before the four fresh derivatives. Baseline C
pins match the assigned runtime/native/sender hashes. Source text comparisons,
not candidate calls, produced SOURCE_PROOF.json and literal forward/inverse diffs.

## One owner / actual composed callgraph

`prepare` installs the original callbacks plus persistent `span_sink={r,
retain_native_span}`. Postfork bind and actual provider/watchdog/memory setup stay
in their original order. `memory_span_hub` still records the one borrow before
the unchanged native fixed-span transaction. New typed `span_serve(r,...)`
checks actual identities then passes ONLY `r->owner`, `&r->source`, `r->span_hub`
and `&r->span_sink` to the real v3-derived server. The generic old server export
is removed, not an alternative entry with caller-selected source/sink.

Runtime TU directly includes the accepted span-runtime native adapter C unchanged.
Its imports define the sole static `native`, `memory_state`, `raw`, source clock,
provider and watchdog owner. No second native TU or borrowed source tree is linked.
New runtime header selects complete accepted adapter/memory/handoff definitions
before original imports. New runtime and sender are recompiled; actual reserve,
memory runtime, reader, v2 hub, RTP spans and unchanged sensor are recompiled with
complete selected definitions. Old layout consumers are excluded from reuse.

Actual producer path remains capture gate → common BEGIN → native NEXT → shared
copy → native CPU release → common exact END → per-consumer span cursors. Twelve
sender functions are byte-identical, including pause/finish_au/process_input,
send_access_unit/publish/source_validate/capture gate/retain frame+span and checked
retire/begin/end. Guarded's sole added denial routing retains actual unknown
backend/IPC custody before rollback; safe auth/policy denial is still false.
Close_client adds backend-unknown fences before entry and after the actual peer
close callback; all original v3 fences/detach acknowledgment logic remain in order.
No auth/packetizer/token/sequence/deadline/copy/release/END behavior is replaced.

## Actual normal acknowledgment, not fresh STOPPED

Last detach puts the common producer into DRAINING. Its real STOP dispatch calls
`idle_source`: check actual STOP work/frame/loan and live paid canonical hub/raw/
optional-output association, capture output **integer** begin/end, latch native
retire attempt1, call unchanged `nh_native_retire`, then set real `r->retired=1`
and attempt2 only on actual OK. Native COMMITTED refusal is unchanged and cannot
produce normal retirement. Native path still observes lifetime, performs sensor
off/END/query/ACK, closes source roles once and sets QP_CLOSED, then logically
disables its actual owned watchdog and frees its own known CPU resident output.
The bridge then observes actual post-clock and completes that exact STOP work.

Native retirement may occur before the final server tail while the hub is ACTIVE;
the integer output snapshot is needed because final validation occurs AFTER the
unchanged native free. Later readiness refuses retired state before paid-hub or
output access. No retry/restart is introduced. A post-ACK fault retains current
native/watchdog/hub state, not an invented prior output pointer.

The real sender tail first retains sticky/native/backend/retirement uncertainty,
then closes actual clients, STOPs/drains the hub and checks all four canonical
tokens, successfully retires the common producer, and requires STOPPED/no demand/
active work. New checks require all real client fd/setup/playing/demand/lease/
pending/deferred/auth/backend cleanup clear, valid observed server clock and exact
local gate association. The private normal end verifies exact still-bound gate,
same serving owner/backend/config, no pending/peers/failure/revoke/HOLD, complete
producer drain/no frame/clock fault/attached consumers, and ALL actual native
retirement predicates. These include staged/adopted/legacy FDs absent, no armed
receipt/CPU loan/resident/parked/close uncertainty, actual QW_DISABLED with FD>=3
and QW_RETIRE, and retained matching retired/END receipt. It performs NO query,
clock read, IO, callback, disable/close/END/giveback or retry.

Final paid association admits ONLY STOPPED, still compares the SAME live hub to
BOTH paid owners/all eight canonical integer ranges/interior-page credit and exact
raw/optional-output aggregate. The snapshot was established before actual native
free, not fabricated after it. There is no pristine forecast at the paid boundary,
and ready() still accepts ACTIVE only, not STOPPED.

## Terminal pointer/history and refusal policy

After all actual checks, one serialized no-fail/no-callback transition records
numeric owner, clears `span_hub`, `ledger.hub.owner`, `span_retained.owner`, and
sets terminal relinquished. Counts/ranges/credit/payment/attempt/failure/native
history persist. No dangling owner/output is converted back to a pointer, read,
compared or dereferenced by terminal audit; only integer arithmetic is used.
Final cleanup returns only when real native retirement AND complete pointer-free
terminal scalar history validate, never on terminal flag alone. Gate unbind and
one listener close retain the original order/outcome rules.

Every added runtime scalar/live/terminal field participates in
`runtime_span_history_empty`, already consumed by the unchanged strict pre-memory
return include. Thus even post-relinquishment cannot masquerade as pre-memory.
All prior range/paid/HOLD checks and original four-FD return tail stay unchanged.
Terminal ready/setup/next/release checks precede any old hub/output access. Native
memory gate already refuses QP_CLOSED through `native_guard_present` →
`provider_shape` BEFORE ledger/clock/reader/credit work; native NEXT also refuses
that shape before actual guard/query/output. No native adapter edit is necessary.
Private memory_state is not exposed for direct gate reuse; all histories remain
initialized/one-use. No new allocation/session or lender/destructor API follows.

Real typed sink unconditionally calls actual retain/fence/quarantine and cannot
return; v3's real cleanup/SIGSTOP fallback remains. Old config.retain_uncertain is
not invoked with a newhub cast/NULL fakehub. Wrapper finalcleanup handles even
ordinary pre-move options/backend refusal after borrowed/native setup, preserving
whatever owner phase/outcome/listener move actually occurred before retention.
Known preborrow invalid identity can return. BUSY-first remains no-touch under the
mandatory no-reentry contract. No external process survival/recovery is supplied.

## Offline evidence selection only

`build01.py` is a fixed standard-library compiler/metadata orchestrator, not an
application/helper import or old recipe invocation. One exclusive build01 output,
no overwrite/retry loop. Eight actual C units per HOST/retained-musl-ARM role,
production defines (no HOST_PUBLIC_ONLY/stub/fixture), exact archive-role/define/
dependency pin checks, exclude all old runtime/server/memory/sensor/flat-hub units,
and rooted actual REL closure. Three raw Digest UND names must remain
LOCAL/NOTYPE/size0 with zero retained references; project relocation failures stop.
No final main/ELF/image/native ABI or transitive stack/memory-fit claim.

All native cap/load/allocator/stack/socket/TLS/process2MiB/40MiB/CMA, external
recovery/backing/provisioning/main/entropy/image ABI/strict C1 gates remain open.
Independent ordinary review and parent adoption BEFORE use; no execution queue.
