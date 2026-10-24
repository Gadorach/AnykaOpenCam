# Private writer runtime — independent ordinary review

2026-10-24. B07 / RP-04/05/09; CAP-01/05/06/07, ENC-02/03/05,
STREAM-01/02/03.

**CHANGES REQUIRED: R1, final auth-gate latency escapes source admission.**
Narrow SOURCE/API/order/ownership/lifetime sufficiency is **not established** for
this delivery. Saved compilation succeeds, but does not resolve this ordinary
ordering defect. NOT ADOPTED / NOT RUN; parent disposition remains separate.

## Scope and identity

`A` = `apps/rtsp-auth/qc07f-writer-runtime-candidate/`;
`D` = `docs/qualification-plans/qc07f-writer-runtime-20261024/`.
Read all eight A files in full, full contract/audit/both handbacks/failure and
selection records, both deliveries/verifications, and saved reports/frozen/diffs/
command/log/dependency metadata. Compared actual span-serve runtime/header
(26d7e4c), included span-runtime adapter, owned-runtime native import/watchdog/
provider take/strict return/FD custody/observations/receipt/close-once/clock and
common producer. Read accepted encoder-spans930afde, producer-hub b4cbbca and
producer-memory5ca49c8 source connections, contracts, independent reviews and parent
dispositions. Earlier sufficiency is bounded dependency context, not this review.

SHA256, names relative to A:

| File | SHA256 |
| --- | --- |
| `owned_runtime.c` | `af95dacac7f609654270201b18772de1a5ee4370f34ab7e1798fb0f0257dfaab` |
| `owned_runtime_v4.h` | `1aa528fd9a0d42ddfb8f798b2eecfb3a8361c5acd1bb07e6a3ab0e9bd5bc7879` |
| `native_adapter.inc` | `f4b270c055e8f190f96fdaba8377d0a123d24f020c549c12b78395117226fa72` |
| `native-watchdog.inc` | `e442a15885d97ab2e94d2a9c2b979de830e52c99d6b1b21f6baadb8aae4e2c35` |
| `compile01.py` | `f90e70593e16686e1bf51a96cbd5c139be84e82a221431ff52a3cf48599ee1c7` |
| `compile02.py` | `b2c31d3364cdbfc8d77539ad464e5a94f80d1e255145f82c4b6bf977954c7fd1` |
| `README.md` | `f0aa8f6ad2d16f984ee94e698135df59318ba8a1d9877c13d9344209b29c4eab` |
| `LICENSE_NOTICES.md` | `800b588cc1a5bfa861561079438d5ba8a5dec9497e03329703ddad197a21a39e` |

D/build01 report SHA256
`535a2f022f5adc01052066a5303a26f059f8b005cf30ae8852536acb5e2e881d`;
D/build02 report (both under `runs/`)
`e6a7bc6f859cf02e46f0ef4a4a7d25c33b5fefa76e1c9ce811d577b533a9ad2c`.
All182 original A/D files aggregate to
`944d6a64b6b78e77e18ed06240114d77f5aaa8c139f6247438a4f17ba2026a99`:
sort complete workspace-relative path strings; concatenate UTF-8
`path TAB decimal_bytes TAB sha256 LF`, then SHA256. Review directory excluded.

## R1 — final blocking gate is outside the last source-clock/watchdog check

**Affected source:** A/`owned_runtime.c:235–245`, especially `242–244`.
`checked_source()` first checks source time/readiness/auth/lifetime/cancellation,
then the actual active/idle watchdog. It reads source time again at242, but its
last condition checks that saved `now` and `ready(...)` **before** calling
`r->gate(r->gate_context)` at244. On gate0 it returns immediately. There is no
post-gate source-clock/session/operation-budget or active-watchdog observation.

This is a real blocking callback, not a pure Boolean. The actual baseline
`qc07f-span-serve-candidate/rtsp_span_server.c:120–132` gate calls `guarded()`
(`102–110`) and final auth checks. The unchanged actual
`qc07f-owned-runtime/server/candidate_auth.c:141–155` calls `source_ready` BEFORE
subsequent verifier RPC/query work; its final auth checks enforce their own grant/
backend timing, not the runtime's original operation/session deadlines. Those
later calls can consume positive time while still returning authentic success.
No malicious pointer, mutated clock/config, reentry, fabricated callback or
concurrency is needed. Near a source deadline, the last legitimate callback can
cross it after the last source-ready observation.

The uncharged final call has these direct consequences:

* Post-arm `checked_source(r,1)` at354 can return after the source2s/10s or owned
  NEXT phase budget has elapsed, then issue the real encoder ioctl at355–356.
  No intervening source-time/watchdog check protects that effect. A later refusal
  does not undo an encode admitted after its guard deadline.
* Post-disable `checked_source(r,0)` at378 can return a CPU loan at379 without
  detecting that the final gate crossed its source deadline.
* Typed release's check at394 can similarly precede `QCW_RELEASED` at398 without
  charging that final callback's elapsed time.
* Most decisively, after REAL common END, the check at430 is followed only by a
  pure paid association, commit-attempt latch and actual hub commit at431–434.
  The last auth callback can cross the source deadline and publication still
  occurs. The additional pre-END `actual=source_now(r)` at423 charges the earlier
  check's cost, but does **not** cover this separate post-END check/commit tail.

**Minimum correction:** keep the final blocking auth callback inside the SAME
original source-operation/session and applicable owned-watchdog admission before
the protected ioctl, outward loan/release acknowledgment and hub publication.
Finish blocking validation with an actual post-cost observation/refusal, without
renewing starts or adding another unchecked blocking callback afterward. Expiry/
clock/failure must retain the current canonical armed writer before unsafe unwind;
preserve exact phases, real END, first error and all original guards. No source
patch, test matrix, new timing framework or corrective attempt is selected here.

This is static ordinary source reasoning, **not an executed timing/negative case**.
It prevents acceptance even though the separate phase/ownership construction below
is otherwise coherent at the reviewed source boundaries.

## Other inspected source boundaries — not an overall acceptance

* One private native/memory/runtime TU selects complete producer-memory/hub3
  definitions through legitimate complete header guards. SOURCE_API4 installs
  actual scalar loan callbacks, not a fake camera_packet/provider success object.
  Prepare stays device-free; actual prestarted last-fork/postfork bind and original
  watchdog UNKNOWN/separate storage/exact139/COMMITTED-provider/memory setup remain.
  Real raw plus whole-eight prefault/payment is connected; paid[2] and resident
  encoded storage remain zero, conservative old memory debt remains.
* Old public next/release/held-retire/flat-buffer entries explicitly refuse before
  effects. Imported static contiguous functions have no selected caller; their
  prepare_output refuses without effects. This is verified by source reachability
  and absence in retained REL symbols, not the unused attribute alone. No selected
  malloc1MiB, raw-as-encoded alias or flattened AU route was found.
* Typed NEXT reserves persistently before guarded original raw work; continuous
  BIND/ARM/exact139/BEGIN/sensor-on/1728000-byte CRC/status/sequence<=128/ACTIVE receipt
  and sole progress ping remain. Codec input stays1382400 main bytes. Full u32-fit/
  request/raw/runtime/hub/chunk overlap checks precede canonical persistent ARM and
  the one real owned-FD shared128B/0xc0805167/eight131072 ioctl, without fallback.
  Syscall0 precedes complete immutable-input/descriptor/reserved/status/length/
  IDR0/P16-bit frame checks; reference history updates precede actual per-open/global
  observation. Bounded0/3/4-prefix/VCL1/5/forbidden-bit parsing and one BZIP2 scalar
  accumulator cover the logical NAL across chunks, without tails or temporary AU.
* `pipeline_exact`/`writer_association`/`paid_association` distinguish RESERVED,
  pre-ioctl ARMED, returned-result ARMED, LOAN, RELEASING, RELEASED and ENDED. Request
  and result/loan fields are populated before their corresponding readiness calls;
  release-attempt/phase change precedes release validation. Actual common END closes
  the bracket before ENDED validation, rather than demanding an open frame then.
  Common START/keyframe completion changes real producer work/last_ns, not the hub's
  publication generation. Hub generation stays private-last until actual commit;
  next reservation token/generation match actual canonical issuance. No ordinary
  safe-path self-refusal from these local phase/field orderings was found; R1 still
  prevents claiming valid completion admission.
* Typed release matches every scalar/key/token/generation and the same open frame.
  END requires RELEASED/result0/exact frame/keyframe before REAL common_end0;
  publication uses private kernel result, never a caller success receipt. Successful
  commit immediately retires live reservation/writer/request/result/loan/frame
  aliases with no further fallible callback, retaining numeric monotonic history.
* Exhaustive runtime live/history predicates reach the unchanged strict pre-memory
  FD return. Failed reserve/partial arm cannot become fresh. Armed uncertainty
  reaches canonical hub HOLD then runtime first-error/fence/nonreturning quarantine,
  not cancel/retry/free. Existing low-level fault latches remain intact. Normal
  STOP requires common DRAINING/active STOP, no writer/loan/error, and actual native
  END/query/ACK/close-once/encoder close/watchdog-disable. No hub free is introduced.
  Hub borrow remains LIVE; no fabricated frontend/client-drain/borrow-end exists,
  and final cleanup intentionally does not return while borrowed.

## Saved evidence checked read-only — no rerun

Both22-record delivery maps/aggregates match. Original36 preselection files and
exact23 build01 members remain unchanged; both runs' original seven consumed app
copies match current bytes. Build01 remains FAILED_PRESERVED: six successful HOST
metadata commands, zero C/REL. Its consumed-source pin exists in baseline
dependencies, not inputs. All46 baseline compile-source records/23 distinct paths
match there. The disclosed record-versus-distinct-path mistake is bookkeeping,
not candidate execution. Historical UNCOMPILED handback was not rewritten.

Compile02 is EXACTLY the two selected line substitutions (lookup and fresh output
directory); forward/inverse selection proofs match, with no fallback/check weakening
or source repair. Both runs' eight feature diffs regenerate byte-exact.

Build02:540 inputs,272 new+320 reused dependencies,16 tools,139 artifacts,44 exit0
command/log records,99 frozen files/seven exact roots and exact140 build members
match current hashes/membership. Fourteen fresh `.d` memberships and reused
dependency/production-role exclusions match. Saved14 actual HOST/ARM production
compiles/two rooted RELs are cited, **not repeated**; compile logs are empty.
Seven real units per target rebuild every selected layout consumer, including
sensor feature macros BEFORE complete handoff include; old frontend/runtime/
memory/sensor/flat-hub objects and combined RELs are excluded.

Independent GNU-ar byte parsing matches all46 original bodies and32 selected
extractions. Raw ELF-byte parsing checks48 new/extracted/REL roles: HOST x86-64 /
ARM EABI5 ET_REL, entry0/no program headers. Both RELs retain54 roots, one actual
native/memory_state/runtime, typed callback definitions/references, real arm/ioctl/
commit/HOLD/packetizer connections and no old frontend/main export. Three raw
Digest UND names remain LOCAL/NOTYPE/size0, unreferenced; no referenced unresolved
project/crypto API. Normal libc/compiler/syscall/GOT and ARM __ehdr_start remain
REL dependencies, not final-link/native ABI/full compiler/transitive-stack/fit proof.

## Conditions, open gates and stop

Trusted fresh final-address private disjoint storage, stable config/callbacks/FDs,
serialization and no edits/copies/unauthorized aliases/reentry/signals/threads/
concurrency/later fork/FD mutation remain mandatory, not hostile-pointer or physical
alias/mapping/provenance proof. HOLD is retention, not continued write permission,
survival or independent recovery. R1 occurs within these conditions.

Overall128KiB/raw/process2MiB/kernel2MiB DMA/40MiB-CMA-TLS/combined-load, external
surviving owner/backing/recovery/provisioning/main/native ABI/entropy and strict C1
remain OPEN. Historical23h OFF/finalized is not fresh state or a lease; RAM75
recovery/C1 blocked, Query78 parked, all prior failures/HOLDs preserved.

Only public reads/hash/diff/JSON/log/raw-ELF/archive bookkeeping and this fresh
review document were performed. No recipe import/compiler/readelf/build/link rerun,
program/help/test/candidate/reader/prefault/credit/hub/malloc/callback/ioctl/provider/
verifier/child/native/media/material-retirement/device/network/UART/HTTP/RPC action;
no source/evidence/Git/index/AGENTS/central-plan/settings/tool/provider/model edits
or subworkers. Normal/negative/adversarial/mutation/fuzz execution NOT RUN, not
passed. **STOP for parent disposition of R1 before correction/use/adoption/follow-on;
no automatic next queue.**
