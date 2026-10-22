# Prestarted backend v2 — independent focused R1 re-review

2026-10-22. RP-08 / CORE-03–04 / STREAM-04/07 prerequisite.
**Narrow R1 SOURCE/API sufficiency:** the exact v2 correction resolves the
previously identified observer-reaped-client IPC custody gap at this source scope.
No additional correction-required R1 ordinary defect was identified. This is not
runtime, concurrent behavior, whole-backend qualification, composition, native or
full-app/product acceptance. V1 and its correction-required review remain frozen.

Exact11-file source aggregate:
`5a42cdc9d76628031c5380edd2e3b2c4dd06aefc6675138c8e10e1b098a28868`.
Author SOURCE_SEAL01 SHA256:
`2bfa06a6b239c0c62b1003df6ee0f0ef9dd17ef0088efad00cccaf1ada7e7a8a`.
Baseline v1 source aggregate:
`867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90`.
Baseline independent R1 review seal SHA256:
`d060aaa15deea1526920414f4015505f1024fa73e2f49eff6a502a90049ce323`.
Source aggregate recipe: SHA256 of concatenated sorted workspace-relative UTF-8
path + NUL + raw file SHA256. Author reports, contracts and seals were read fully
or parsed as full static pin/receipt maps, without executing their code.

## Scope and provenance

Only fresh files in this review directory were written. The full R1 contract and
candidate report, actual two-production-file delta, real stop/start/reap/destroy/
observer and unchanged backend caller/finish contexts were inspected. V1 source,
builds/reviews and their failure remain intact. Auth C/H, server, three image pin
headers and crypto config are byte-identical to v1. Only adapter C/scalar header
change in production; README/builder account for the other two changed files.
No unrelated sibling is semantically reviewed/adopted, and no listener composition
is performed. Historical preservation maps are hash-only custody checks.

This is ordinary SOURCE/API review of R1, not a broad security/parser/cleanup or
architecture audit. There was no candidate/author-helper/script-main import or
execution, compiler/generator/test/guard/listener/executable/app/output execution,
penetration/adversarial/fuzz/mutation/runtime campaign or target/UART/RPC/status/
network/private IO. Independent Python collection performs public reads/hashes,
diff/receipt checks and direct ar/ELF metadata parsing only; no subprocesses.
No original source/AGENTS/Git/index/central-plan/P1/model/provider/safeguard edit.
No collection error or policy rejection occurred; limits are retained in
`COLLECTION_NOTES.md`. Earlier standing08-plan context applies; no central-plan
work or new native admission is selected by this correction review.

## Actual R1 close/disposition reasoning

Paths/lines below are within `apps/rtsp-auth/prestarted-backend-v2/`.

| Boundary | Actual code / focused finding |
|---|---|
| Actual slot and history | `verifier_adapter_overlay.c:25–45`: actual private client adds attempted/former-NOT-owned/error fields. Real calloc`:191` initializes history; actual IPC assignment remains`:192`. Helper saves actual c->fd, clears c->fd=-1 **before** close, then records attempted/former and makes one close call. FD0 is admitted as a real owned FD. This is not a detached numerical receipt. |
| Sticky first close error | Helper`:36–44` checks attempted **before** checking fd<0. A failed first close records positive errno (EIO fallback), marks failed and returns RV_UNCERTAIN. No later helper call overwrites it or treats fd=-1 as success; no second close/retry, new open/dup or I/O through the former number. Under serialized FD-table/nonreentrant caller preconditions the record remains the same actual client. |
| Already-reaped route | `rv_stop:304` immediately returns ipc_close_once for actual retired state, rather than skipping IPC disposal. No clock, signal, poll or wait occurs on this path. The original readiness observer/reap can still consume the actual exit; it need not itself close IPC or invent future liveness. This directly closes R1's former normal START_FAILED->explicit finish ownership gap. |
| Newly-reaped route | `rv_stop:305–318`, particularly`:309`: original stop loop performs the same actual signal/receive/wait/poll/budgets; successful owned reap now reaches the same helper. Unknown wait/error/time-budget branches remain non-OK, not artificial retirement or retry permission. Actual wait status/retired/wipe observations are unchanged. |
| Pre-release startup failure | `rv_start:238–240`: original early barrier close now uses the helper. Its direct return is deliberately not treated as child retirement; original actual rv_stop still runs. While the child is live, clearing/closing a parent IPC slot is not wait/exit proof. On eventual actual reap, the same helper returns any earlier saved close error, so the fd=-1 slot cannot erase that error. If actual wait remains unknown, original non-OK/HOLD still applies. |
| Destruction after success | Unchanged `rv_destroy:320–321` calls rv_stop before wipe/free/null. After successful IPC disposition and actual reap its second already-retired call returns stored OK without a second close/signal/clock/wait/poll. Real wait/wipe observations are not fabricated; unchanged backend destructor still checks them before publishing normal initialized success. |
| Destruction after close error | The same unchanged rv_destroy returns the stored non-OK immediately and cannot wipe/free/null the client. Unchanged auth destructor`:107–116` preserves actual child/client/config on non-OK stop, invokes mandatory retain_verifier and returns non-OK. Explicit finish`:242–258` sees the remaining actual client and enters HOLD, not FINISHED; repeated outer finish is refused. No outward OK or claim of retaining the former numeric description follows. |
| Diagnostic API | Overlay`:338–345`, `adapter_state.h:17–23`: scalar observer reads only actual attempted/former/error and does no syscall/control/payload/clock work. Requires live stable client and writable stable disjoint output under existing serialization. NULL refuses. No-attempt reports attempted0/former=-1/error0; RV_OK then means only a metadata observation, **not** proof of closed IPC/retired child/readiness. Recorded error remains RV_UNCERTAIN even when child exit is known. |
| Private layout | Actual allocator uses sizeof(*c), destruction wipes sizeof(**p), and `is_rv_context_bytes:329` returns sizeof the actual expanded private type. Overlay and real header consumers are freshly compiled. rv_client remains opaque outside the overlay; no public binary/plugin ABI or old-object layout compatibility is claimed. |

### R1 path table — static source trace only, NOT executed

| Actual path | Owned IPC / return / remaining custody |
|---|---|
| Successful startup then observer consumes child exit, START_FAILED, explicit finish; close succeeds | Already-retired stop clears/closes actual IPC once. Second stop inside destroy returns stored OK, no duplicate effect. Actual client can be wiped/freed only through original destructor; original start/wipe/wait0 failures remain non-OK and outer history is not reset. |
| Same observed-exit path; close fails | c->fd=-1, attempted1, former diagnostic and first error remain in actual allocated client. Stop/destruction non-OK -> mandatory real sink -> backend/client/config/namespace retained -> outer HOLD. No error-FD retry or ownership resurrection. |
| Normal stop obtains a new actual reap; close succeeds/fails | Same helper/disposition as observed-exit path. A new successful close does not manufacture BYE/wipe/exit0; a failed close cannot be hidden by the child's known exit or a later retired stop observation. |
| Pre-release close attempt succeeds/fails before child exit | Original stop still must establish real child retirement. After actual reap the attempted outcome is reused, not closed again; an earlier error remains non-OK. Missing wait proof and failed IPC disposition are separate facts. |
| RV_UNCERTAIN from rv_start's already-attempted stop | Unchanged `candidate_prestarted_start:227–234` enters terminal HOLD and the actual retain_verifier sink immediately. No explicit finish/retry/second stop is selected, even if the child later exits. Known-exit plus failed-IPC can now share this return code; it must not be relabelled an unknown CPU wait or forced into success. |
| Internal observation after a recorded failed close | Helper/retired stop returns stored non-OK without another close; rv_destroy refuses free/null. This describes idempotent error handling, **not authorization to call stop/destroy again on outer HOLD**. |

`rv_retired:322`, actual owned wait status, `rv_wipe_observed:325` and
`is_rv_wait0:330` remain independent of IPC outcome. Thus a known CPU exit can
coexist with failed IPC disposition. The retained object owns its diagnostic/
namespace history, not the former description: Linux close error does not prove
that number still denotes the original open file description. No FD retry or
fake FD-retention claim was found in the selected correction.

## Exact preserved behavior / remaining limits

Independent byte comparisons confirm clock/reap/owned-signal/barrier/launch/image
and RPC/policy/submit/complete/query bodies unchanged; rv_destroy/retirement/
existing observers/test tail unchanged. rv_start differs only at its one existing
pre-release IPC close; rv_stop differs only at already-retired/newly-retired IPC
routes. No signal choice, numeric-PID fallback, poll/attempt/time-budget, release/
READY15/profile/provisioning ACK, child image pin or per-effect authorization is
replaced. R1 adds only private history/helper, its three callsites and the scalar
observer/declaration (plus required include).

Auth C/H/server and their source API/lifecycle are byte-identical to reviewed v1:
process/self claim before private memset; real one-shot device-free initialization;
stable immutable config/graph/context ADDRESS/callbacks; actual stable auth peers
from first OPEN; serve uses same embedded backend with no init/fork/hidden finish;
guard stack lifetime ends at unbind; failed/nonquiescent serve is terminal HOLD;
explicit verifier-only finish does not stop native owners. Caller serialization,
no aliases/reentry/movement/reset and genuinely device-free all-forks-before-native
bootstrap ordering remain trusted source prerequisites, not facts proved by fields,
fstat or pidfd. No installed caller/readiness/bootstrap is supplied by this review.

Unpublished constructor/fork-failure resources, ancillary FD and pidfd cleanup
remain outside R1 and unchanged; no broad cleanup qualification is claimed or new
cleanup refactor demanded here. Existing non-OK NEXT/internal partial-work retirement
contract is unchanged. Stable backend/peer metadata cannot retain a returned source
hub/client/gate stack or solve native failed-NEXT/nonreturning external retention.
No new native END/ACK/reset/feed/kill/resume, source-memory credit/exclusion or
permission follows from child exit/IPC disposal. Unexpected process/OOM exit does
not retain its FD table. Functional/native/post-memory/external-process ownership,
backing/code/namespace lifetime, provisioning, real main and G3/G8 remain open.

## Build receipt and artifact custody — independently checked, not rerun

Author build01:15:51:31.068143–15:51:35.427033UTC. **Six fresh real component/header-
consumer compilations**,26 compiler/tool/archive/REL/ELF commands exit0; four
ordinary diff commands exit1 for expected differences =30 records. Empty compile
logs. No compiler/test/generator/helper/output was run by this review.

Independent static commands, exclusively producing new receipts here:

```text
python3 -B docs/qualification-plans/rtsp-prestarted-backend-v2-review-20261022/collect_static.py before01
python3 -B docs/qualification-plans/rtsp-prestarted-backend-v2-review-20261022/collect_static.py after01
```

- Verified exact11-file source inventory/aggregate, author SOURCE_SEAL digest,
 570 evidence files, BUILD_SEAL/report/contract pins and all source snapshots.
  Baseline11 v1 app bodies/provenance match source and independent prior review
  seals, their selected maps and copies; v1/R1/retention history remains frozen.
- Verified204 explicit input paths and independently reconstructed snapshot
  destinations,282 consumed source/header paths and each dependency byte copy,
  all six actual .d maps,1,328 frozen pins, toolchain and artifact maps. Counts
  overlap and are not additive histories. Author verified union1,377 matches the
  actual combined maps. Two recorded compiler aliases retain exact resolved
  target paths and bytes, not new source-symlink authority.
- Reconstructed all six compile argv and both three-member archive/REL commands.
  HOST GCC16.2.1 and retained ARMv5TE soft-float GCC14.3.0 logs/tool identities,
  cc/cc1/as/ld/ar/nm/readelf and four ARM closure pins agree. Werror/FORTIFY/stack
  protection and strict/default class remain; only inherited compact server has
  its misleading-indentation exception. No public-fixture/test macro/stub or
  guarded old builder invocation appears in reviewed recipe/command records.
  This is receipt/source inspection, not an audit of unobserved process activity.
- Reconstructed each recorded diff from original/context lines and exact new
  bytes. Seven production inputs remain byte-unchanged. Actual fresh auth/server
  object bytes match v1 on both platforms, while command receipts show new
  compilations; this equality is not object-reuse evidence. Adapter objects differ.
- Parsed actual archives without extraction/execution: exactly three unique members
  per platform, matching six recorded object bodies. All members and both linked
  artifacts parse as ELF REL, entry0/no program headers, x86-64/ARM EABI5. Actual
  start/serve/finish/close-outcome/size/verifier symbols are present; no main/old
  monolithic serve or exported raw backend init/destroy/finish. Actual relocated
  undefined framework/IPC/MbedTLS/libc/libgcc calls are normal component dependencies,
  not a full-app link, fake readiness stub or native ABI/memory-fit result.
- Original GPL/runtime GCC/musl/MbedTLS notices match retained license snapshots.
  MbedTLS277-file aggregate stays
  `bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5`.
  Image pins, native2MiB/separate HOST4MiB, strict loopback/auth/source/check/rate
  policies, one1MiB hub/2s send budget,128KiB/40MiB/CMA/native timing constraints
  remain unchanged. No license removal or global relicensing.

| Component | Bytes | SHA256 |
|---|---:|---|
| HOST archive |89914| `4b76d28ba280c5be6600c7d818fe81f792a6ad38797f6e25268ca082501b0e74` |
| HOST REL |85368| `70d0c7af98ab779f453608571a45110088ac11c3e59d702ebbe7680a44548cdf` |
| ARM archive |58640| `6345b7b8995f16820005e8756cc5d307381209865c9b8f0c498f8bc866d572c4` |
| ARM REL |54664| `90527c7f983c1d01e3a7b7af819c35a3828feb97773cda28c400627b9c01d565` |

Independent collections cover1,892 unique reader inputs. `FINAL_SEAL.json` verifies
before/after equality and final fresh rehash, and freezes reader/own-output pins.
This is a hash evidence seal, not a runtime observation or signed attestation.

## Disposition / handback

No minimum fresh R1 corrective patch is required at this bounded SOURCE/API scope.
Parent may separately disposition this exact correction; original v1/R1 failure
evidence is not erased or retroactively accepted. This result supplies no listener
composition, full backend/native lifecycle or deployable app approval, and no
automatic review retry/implementation/build/test/native queue.

Last inherited hardware observation remains historical held original U-Boot12:12UTC,
catch consumed; no fresh target/receiver/recovery or expiry assertion is made.
Both original radio/reset HOLDs remain untouched. All functional/native/post-memory/
external-retention/backing/provisioning/main/G3/G8 gates remain deferred/open,
not silently passed by source/API sufficiency or author compile receipts.
