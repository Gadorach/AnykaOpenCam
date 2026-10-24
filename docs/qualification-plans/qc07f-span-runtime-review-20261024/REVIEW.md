# QC07F span runtime — independent ordinary source review

2026-10-24. CAP-01/05/06/07, CORE-03/04, RP-04/09, B07.

**Disposition: narrow conditional SOURCE/API/order/ownership sufficiency. No
blocking ordinary defect found within the declared trusted-caller contract.**
Parent adoption remains separate. NOT RUN; no runtime/native/allocation-cap or
product GO, and no sender/runtime composition acceptance.

## Scope and exact identity

Read all nine assigned candidate files, CONTRACT/HANDOFF, both audits, build01
disposition, source deltas/diffs and build/pin records. Compared actual frozen
owned-runtime/native/import/provider/watchdog/return paths, accepted span-memory
units and independent review/parent disposition, accepted v2 hub and RTP spans.
The separate span-server, including v3, is not this candidate's object dependency
or adopted implementation.

SHA256; candidate names relative to `apps/rtsp-auth/qc07f-span-runtime-candidate/`:

| File | SHA256 |
| --- | --- |
| `owned_runtime.c` | `4557ccab74d8f4a499da91acc3626ddb5fdd28befc744817dcd3221e61d05a74` |
| `owned_runtime_v1.h` | `1cda7bcc58fc0d0b39437097171990a99053f82085cd12cfec0fb65f462d0afa` |
| `native/native_adapter.c` | `b0b97dc3946e001023bb0c00d22e0aa1d5104ec61b93b4d5770c8d7245a18f5f` |
| `native/native_adapter.h` | `57876304bc483349b89faaf3b4c82c7df840d53b3c229f202ea7ac11dae730b0` |
| `native/provider-return.inc` | `281bea5950da27f9cf7b04aa624405160cf4876ecb59a752f6adf517da480b44` |
| `build.py` | `570f762a63960848e2ddecacf0caaaa333beb63cfa9475e5912933d266b62504` |
| `build02.py` | `6b044026ac623ec4e0a4e666f3a0c6905e82cc9274e2f62d950cfc86a4abf436` |

Evidence root: `docs/qualification-plans/qc07f-span-runtime-20261024/`.
`DELIVERY.json`: `7afb1e186476015699e2493073d94ef454e2f6f482f3bbc1f6453cd6b94575e6`.
Reconstructed C/h/inc aggregate:
`d6e781d22fe67e863bf2550e7231f3b6d952b2f7abf9e3914492a278268aea33`.
Build02 report:
`748ce4bc39b479c9e18f82d342b472cc54286d9a5760d3c797cbe1147dcd0bb8`.

## Ordinary findings

1. **One actual private owner, complete replacement definitions.**
   `owned_runtime.c:7–21` selects the new complete adapter/handoff/memory types
   before including the actual native TU. Its `native`, `memory_state` and
   `runtime` are the real static objects, not shadow metadata. Frozen nested
   headers encounter legitimate already-defined guards, not fabricated skip
   macros, missing declarations, stubs or mixed old memory layouts. Source API2
   explicitly requires replacement/recompilation, not binary compatibility.

2. **Actual order and irrevocable acquisition boundary.**
   Prepare remains device-free; unchanged postfork bind requires the same READY
   config/backend/child and no peers, then actually initializes the singleton.
   Original watchdog UNKNOWN take, storage/exact139, COMMITTED provider take and
   one-shot sealed memory start remain prerequisites. The typed call at
   `owned_runtime.c:326–366` additionally checks actual callback identities,
   session/callback/history/owner conditions and absent peers before a pure
   pristine-hub forecast. It records BOTH persistent hub and sole attempt before
   any new clock/query/raw touch. LAST-fork/global ordering is a trusted bootstrap
   obligation, not proved by READY metadata. A preborrow refusal never authorizes
   freeing/exiting an earlier provider/watchdog owner.

3. **Real raw → whole-hub prefault connection, no payment shortcut.**
   `native/native_adapter.c:203–216` preserves guard → one begin → actual lifetime
   query → unpaid gate → original1728000-byte raw kind0 touch/credit → fixed gate
   → actual `nh_memory_prefault_span_hub(&memory_state, hub, begin)`. The same begin
   reaches all work; count<=60 reserves four observations within the original64.
   The accepted unchanged helper brackets all eight actual chunk touches with
   unpaid gates, checks unchanged association, and atomically pays the whole hub.
   No direct metadata-credit substitute, flat hull/chunk cast, deadline extension
   or fallible postcredit callback is added. Old flat fixed-buffers admission
   (`native_adapter.c:83–87`) immediately refuses before effects.

4. **Paid readiness checks exact actual association without pristine-state abuse.**
   `owned_runtime.c:86–145` checks recorded runtime hub/self/ACTIVE/allocation
   history8/1048576/max131072, PAID/no failure, matching ledger AND retained owner/
   count/all eight canonical ranges/credit, zero scalar hub extent, overflow-safe
   disjoint extents and exact full-interior4KiB credit. Original raw extent and
   optional paid producer extent contribute to the exact aggregate CPU credit;
   original provider/storage/exact139/watchdog/session/source checks remain.
   No unpaid forecast or generation/token/client/pristine condition runs on the
   paid hub: normal publication/acquisition metadata can change. These checks
   establish local refusal history, not malloc provenance, residency or native fit.

5. **Failure/history precede unsafe unwind and custody return.**
   Every failure of the admitted typed setup enters existing nonreturning
   retain → latch first runtime error → fence → actual quarantine before outward
   unwind. Runtime borrow survives even failure before helper admission; true
   partial/raw payment and helper retained/HOLD/paid histories are never erased.
   `native/provider-return.inc:23–51` explicitly checks both runtime fields,
   ledger hub owner/count/credit/every range, retained owner/count/credit/every
   range, FRESH attempt and zero failure, plus all original conditions. Zeros are
   elementwise, not padding comparisons. Original four-FD transfer tail and
   UNKNOWN-not-DISABLED semantics remain unchanged.

6. **Borrowed lifetime deliberately outlives normal native END.**
   `span_history_present`, `retired_actual`, `retention_needed` and `native_owns`
   retain new history, including a pre-touch borrow and history after native END.
   No native CPU free receives the borrowed hub/chunks. Original capture loans,
   keys/keyframes, encoder/source FDs, release, watchdog and END bodies remain.
   **Final cleanup intentionally cannot return while the borrow exists; there is
   no external relinquishment API.** This is an explicit retained-lifetime gap,
   NOT clean server STOP return, finite product cleanup or restart acceptance.

## Read-only evidence assessment

- Fresh hashes matched20 delivery and67 frozen records; original57-file directory
  and candidate nine-file membership were exact. Regenerated both literal diffs
  at top level and in both build contexts; verified all five recorded unchanged
  byte regions and the source aggregate.
- Build02 matched207 inputs,278 new+320 reused dependency-path records,122 artifacts,
  16 resolved tool records and42 logs:985 overlapping checks, not distinct files or
  full compiler closure. Fourteen actual `.d` files reproduce exact new membership;
  reused dependency membership/46 original object roles and production defines
  match the baseline. Real runtime include paths resolve to the new local bodies
  and frozen native imports; no old memory definition header is consumed.
- Build01 remains FAILED_PRESERVED: six metadata commands, zero C/REL, old sensor
  layout-consumer reuse refused. Its199 overlapping pin/log records matched.
  Build02 excludes that object and recompiles the unchanged real sensor C with
  COMPLETE accepted handoff `-include`, feature macros first. All layout-consumer
  reuse refusals remain; no check weakening or fabricated guard.
- Cited author evidence, not rerun:14 HOST/ARM C compiles,2 rooted REL links,
  42 exit0 commands, 04:57:52–58Z. Seven actual units per role,32 extracted archive
  bodies independently byte-matched to archive members and original objects.
  Old runtime/memory/sensor/server/flat hub objects are excluded; no fixture,
  synthetic callback, old builder or separate sender object is selected.
- Independent read-only nm/readelf outputs match saved metadata. All48 build02
  ELF artifacts are ET_REL:14 fresh,32 reused,2 combined; correct HOST/ARM roles,
  ARMv5TE/EABI5. All29 explicit roots exist once, as do the three private owners.
  Actual typed runtime relocations reach the real whole-hub helper in both roles.
  No main/serve export or old hub publish is selected. Raw Digest close/issue/verify
  UND names remain truthfully present but have no surviving referencing relocation;
  no referenced project symbol is unresolved. libc/compiler/syscall symbols and
  ARM `__ehdr_start` are expected unresolved REL dependencies, not final-link proof.

Reviewer bookkeeping initially assumed whole-function region boundaries and
unresolved `.d` spelling; read-only exact-byte slices and resolved include paths
matched the retained evidence. No candidate source/build was changed or rerun.

## Mandatory conditions and open gates

True-fresh persistent valid stable disjoint private storage and actual v2 hub
allocation history; caller serialization, no mutable aliases/copies/reentry/
signals/concurrency or later fork; all runtime/config/backend/hub/chunks survive
every admitted outcome. Rebuild EVERY layout consumer; exclusively replace the
runtime/header/included native/return and accepted memory/v2 hub units. Never link
old units, separately compile the included adapter, or reuse old combined RELs.

External surviving media owner/all-domain recovery/backing/relinquishment,
bootstrap/provisioning/main, remaining1MiB producer/raw cap obligations,
allocator/library/stack/socket/process2MiB/40MiB/CMA/TLS/combined-load fit, native
entropy/syscalls/image ABI and strict C1 remain open. REL and `.su` evidence prove
neither native ABI nor transitive stack/full compiler closure/final image.

No build/import/test/program/helper/reader/prefault/credit/callback/native operation
was invoked in this review; only public reads/hashes/diffs/log/ELF/archive metadata
inspection and this fresh review document. No negative/adversarial/mutation/fuzz
matrix, subworker, Git/index/source/AGENTS/central-plan/private/device/network/
hardware action. Last OFF/finalized23h and RAM75 C1/recovery block remain historical,
not fresh state; prior failed HOLDs remain unchanged. **Parent disposition only;
no further work, execution, adoption or hardware queue is selected.**
