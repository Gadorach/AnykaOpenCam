# Prestarted backend v1 — independent focused SOURCE/API review

2026-10-22. RP-08 prerequisite / CORE-03–04 / STREAM-04/07.
**CORRECTION REQUIRED: R1, observed-exit IPC FD custody.** The ordinary lifecycle
delta is not independently accepted as delivered. Build custody checks pass, but
compile/archive/REL evidence does not resolve this source cleanup defect.

Exact11-file source aggregate:
`867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90`.
Author SOURCE_SEAL01 SHA256:
`9efc3a84ab6342e9ad789ac1b6448358ea8798cde0339726b78fb47dec3a23c9`.
Sole frozen41-file baseline aggregate:
`2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f`.
Aggregate recipe is sorted workspace-relative UTF-8 path + NUL + raw file SHA256,
concatenated and SHA256-hashed. Inputs/source and existing listener/app/reviews
remain unchanged. This review is **not sibling composition**.

## R1 — observer-reaped child loses its still-open parent IPC descriptor

Paths below are within `apps/rtsp-auth/prestarted-backend-candidate/`.

**Anchors:** `candidate_auth.c:235–239,242–257,107–116`;
`verifier_adapter_overlay.c:33–39,177,287–306,316–322`.

The new start success check calls `is_rv_running`, whose `reap` can consume the
real owned child's exit before explicit finish. `reap` records wait status,
sets retired/failed, clears child PID and closes/clears the pidfd — **but does not
close or clear `c->fd`**. That parent IPC FD was assigned at startup`:177`.
The explicit destructor depends on `rv_stop` to retire the IPC FD. However,
`rv_stop:289` immediately returns RV_OK for an already-retired child; only its
later loop path`:294` closes/clears the IPC FD. `rv_destroy:306` calls that same
already-retired fast path, then wipes/frees the client without another FD close.

An ordinary source-reachable sequence, not an executed test:

1. Actual `rv_start` completes READY and provisioning ACK successfully, publishing
   a real initialized client with `c->fd >= 0`.
2. Child exits between that ACK and the new start observer. No field mutation,
   malicious caller, native provider or concurrent library call is required.
3. `is_rv_running -> reap` observes the real exit and returns false. Start records
   CAMERA_NOT_READY and leaves the actual object START_FAILED (`auth:235–239`).
   The IPC FD remains open in the retired client.
4. The permitted explicit `candidate_prestarted_finish` calls the private backend
   destructor. Both stop calls return from `if(c->retired)return RV_OK` without
   signaling — but also without the IPC close. `rv_destroy` wipes/frees the only
   client record and clears `backend.child`; backend wipe and outer FINISHED then
   lose the FD ownership record while the parent description is still open.

The final status can correctly be non-OK (for example missing observed wipe);
that does **not** recover the lost IPC custody. The leak is not a claim that the
child is still running or that unknown native work was retired. The normal stop
loop and the already-reaped stop path have different FD effects. The latter's
absence of a second signal is necessary but insufficient for correct destruction.

This is an inherited retired-fast-path cleanup gap newly relevant to the added
ordinary readiness observer and explicit START_FAILED finish path, not a claim
that the entire original stop implementation was newly authored. No broad
security/parser audit or runtime reproduction was needed for this branch proof.

### Minimum fresh correction boundary

In a separately selected fresh revision, ensure the **actual owned IPC FD** gets
an explicit exactly-once disposition before any already-retired client is wiped/
freed, including retirement observed by `is_rv_running` or earlier adapter reads.
For example, consolidate terminal FD cleanup so already-retired and newly-reaped
stop paths both reach it, without a second signal/wait against a consumed child.
Clear its owned slot before the one close attempt, do not retry/reuse an error FD,
and do not turn cleanup errors into successful final status. Preserve actual wait/
wipe receipts and start/serve failure history. Keep unknown-client ownership and
mandatory sink/HOLD untouched; **do not** route RV_UNCERTAIN startup through another
finish/stop. Document/review any deliberate change to the inherited adapter body.

No source correction, fresh revision, build or test is performed/queued here.
Static re-review must trace both observed-exit and normal-stop paths through real
FD disposition and destruction, rather than equating waitpid retirement with all
FD cleanup. Do not edit this sealed candidate or old baseline/reviews in place.

## Other focused ordinary findings

The full report/contract, actual five-file delta and original contexts/APIs were
read. No further correction-required defect was identified in these bounded
areas; this is not an independent GO for the whole lifecycle while R1 remains.

| Boundary | Actual source finding |
|---|---|
| Caller object/one-shot start | `candidate_auth.h:57–97`, auth`:217–240`: macro initialization is real C zero initialization. process_owner/self/config/STARTING are claimed before private init's memset. The outer object/process claim never resets to EMPTY, including invalid config/clock/finish failures. NULL/wrong/already-consumed objects refuse; fields are private by trusted source contract, not malicious-memory enforcement. |
| Trusted ordering/config | Contract`:18–41`, header`:57–64`: no native/provider/watchdog/guardian FD, session, backing, lease or userdata may exist at this/any remaining fork. Config identity, graph/routes, context ADDRESS and callback addresses remain immutable/nonmoving through finish/HOLD; later userdata, if independently admitted, stays inside the same stable context after the last fork. No fields/queries prove those facts, and no real bootstrap satisfying them is delivered. |
| Real startup unchanged | Auth`:41–105` keeps actual rv_clock at outer entry, original graph/routing/domain/generation/RNG/rate/native-vs-HOST/image/material predicates. startup_result alone records actual rv_start status. Overlay`:105–226` retains real fork, owned pidfd/release barrier, exact READY15/profile and provision ACK. No fake callback/grant/image/credential selector or child budget change. |
| Failed start ownership | Auth`:227–234`: actual RV_UNCERTAIN from rv_start's own attempted stop becomes terminal HOLD and invokes real retain_verifier on actual stable child/config; no implicit finish or second stop follows. Other failures remain START_FAILED for one explicit finish. Neither failed startup nor an observer rejection implies actual retirement. R1 applies when the observer actually reaped the child. |
| Scoped observer | Overlay`:316–322` uses only the existing owned wait relationship, no arbitrary PID/signal/protocol effect. It rejects failed/retired/revoking/no ACK/no pidfd state, actual wait error and EINTR. A successful no-exit observation is point-in-time only, not future health/permission/external retention. R1 concerns its real reap side effect, not its Boolean admission predicate. |
| Actual serve association | Server`:424–450,632–652`: READY->SERVING consumes the real owner attempt, verifies same backend/config, initialized/nonfailed/nonrevoked/unoccupied records, exact READY/profile and owned-child observation, then uses embedded backend/config pointers. No backend copy/memset/init/rv_start/fork/exec or hidden backend destruction occurs in body/entry. Original backend polling remains actual. |
| Entry errors and reentry | A real READY owner's backend-entry rejection, bad options/producer/lifecycle/hub/clock or body failure goes terminal HOLD, not a retryable READY or implicit destruction. NULL/foreign/misphased objects refuse without starting a new lifecycle. SERVING/FINISHING phases reject callback reentry; config/callback immutability and serialization remain mandatory, not thread/signal safety. |
| Stable auth from first OPEN | Server`:33,477–478` binds auth to owner.peers[index] before actual candidate_peer_open. Every request/challenge/CHECK/effect/rate/session/close/pending comparison consistently uses that pointer. Auth`:128,136,183–198` therefore stores only stable peers in backend.peers/pending_peer, never a copied/returning server peer record. Pending/discard failure keeps the actual record/owner, not a stack copy. |
| Never-opened NULL auth | Client array is zeroed then all fd=-1 (`server:441–442`). close_client`:139` returns before touching backend/auth on such slots. Accept-side failures before binding close the local fd directly; all stored-client close/error paths bind backend/auth at`:477` first. Failed OPEN close uses null-safe candidate_peer_close; no demand/lifecycle was attached yet. Parser/control/pending/media dereferences occur only for opened/live/playing slots. |
| Serve gate/stack | Hub/client/gate remain live through all synchronous callbacks and unbind. After binding`:444`, socket-creation failure unbinds`:446`; every other returning path goes through listener_done/unbind`:613–629`. Early validation returns precede binding. The new wrapper retains no gate/client/hub pointer; source_guard_bind must clear saved references on unbind and cannot retain those stack addresses asynchronously. No other sibling implementation is adopted to assert that absent caller behavior. |
| Source HOLD / inherited cleanup | Final server`:627–628` latches observed sticky clock or non-STOPPED/demand/active common state. Wrapper`:647–651` independently keeps failed/revoked/pending/peer/source or nonzero body result on HOLD. Socket failure/early option failure is also HOLD even without reaching final source checks. Source guard, packet budgets, exact auth/effect CHECKs, source END/release/retire and copy-only sinks stay unchanged. Clean scalar common state is not native/DMA proof. |
| Explicit finish admission | Auth`:242–257` admits only READY/START_FAILED/clean SERVED, refuses SERVING/HOLD/repeated/source_hold/pending/pending_peer/peer state, and marks FINISHING before one private destructor claim. It controls only verifier ownership, never native STOP/free/close/END. With R1 noted, actual remaining unknown child stays on HOLD with original mandatory sink; start/serve/destructor errors are not converted into OK. Known retired destruction remains terminal FINISHED, not renewed EMPTY. |

Original sticky server clock is not reset by start/serve/finish. The same strict
loopback127.0.0.1 internal socket/bind/listen path, four peers, one1MiB hub,2s AU
deadline, image pins, native2MiB/separate HOST4MiB class and original128KiB/40MiB/
CMA/backing restrictions remain. No listener adoption or composition is supplied.

The unchanged public `camera/rtsp_server.h:36–42` still requires non-OK NEXT to
retire partial work internally. Stable backend/peer storage does **not** retain
returned sourcehub/client/gate stack, undo earlier source cleanup or implement
nonreturning native failed-NEXT retention. External media/process/code/backing/
namespace lifetime, native bootstrap/recovery and regular shutdown with uncertain
native owners remain explicitly BLOCKED. No fake source-ready hooks or source
cleanup permission is inferred from these source predicates.

## Compile/evidence custody — PASS, cited not rerun

Author build01:15:24:31.139666–15:24:34.652420UTC;26 successful compiler/tool/archive/
REL commands plus five ordinary diff commands with expected exit1 =31 records.
Six real changed-unit compiles (auth/server/verifier per HOST/ARM), all empty logs.
No unchanged object reuse, stub, child/fixture generation or full-app link.

Independent `collect_static.py` reads/hashes/inspects only; no subprocess, candidate
imports, author script main, compiler or generated output execution. Receipts:

```text
python3 -B docs/qualification-plans/rtsp-prestarted-backend-review-20261022/collect_static.py before01
python3 -B docs/qualification-plans/rtsp-prestarted-backend-review-20261022/collect_static.py after01
```

Verified before/after and by final seal:

- Exact11-file source inventory/aggregate, author SOURCE_SEAL digest and its592
  evidence-file pins; BUILD_SEAL/report/contract pins. Nine selected app imports
  are exactly five changed/four unchanged; README/builder are new authored files.
- All41 original frozen files and all41 retained baseline bodies; old91-input/
  five-output retention manifests and named files unchanged **by hash only**, not
  semantic review/adoption of other siblings.
- All200 recorded input paths and independently reconstructed snapshot destinations;
  all282 consumed dependency paths, six actual .d maps and dependency byte copies;
  all168 frozen map entries. Those maps overlap, not additive histories. Author
  verified union661 matches actual combined maps/artifact/toolchain inputs.
- All31 argv/cwd/time/exit/log pins. Independently reconstructed six exact compile
  commands, archive member lists and relocatable link commands; HOST GCC16.2.1,
  ARM GCC14.3.0 ARMv5TE/soft-float, cc/cc1/as/ld/ar/nm/readelf and four retained ARM
  closure pins. Original warnings/Werror/FORTIFY/stack protection remain; only
  original compact server has its misleading-indentation exception. No public-
  fixture/test macro is enabled; HOST is compile architecture, not HOST-profile
  execution or native evidence. No old guarded builder invocation appears in this
  recipe/recorded command list; no unobserved process-audit claim is made.
- Every ordinary diff hunk's old/context lines independently checked and used to
  reconstruct the exact new file. Actual three-member archives match all six
  recorded object bodies. All six members and both linked artifacts are parsed
  directly as ELF REL, entry0/no program headers, x86-64/ARM EABI5 respectively.
  Actual start/serve/finish/observer/verifier symbols are defined; no main, old
  monolithic serve or exported raw backend init/destroy/finish. Relocated undefined
  framework/IPC/MbedTLS/libc/libgcc calls remain expected component dependencies,
  not a full app link or inert fake stub. No object/output was run.
- MbedTLS277-file aggregate remains
  `bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5`;
  GPL SPDX/runtime GCC/musl/MbedTLS notices are intact in baseline and snapshots.
- Author initial static-finalizer failure is preserved and its initial script pin
  verified. Comparing scripts shows only comments plus excluding new README/builder
  basenames from imported-file classification. Source/build snapshots are identical;
  no compiler rerun/source correction is represented by that metadata correction.
  It is neither a compiler failure nor policy rejection/reroute.

| Component | Bytes | SHA256 |
|---|---:|---|
| HOST archive |89078| `51967f98676f9de3924439c45a37775b0c5b4820dc22bcc3be7a87d8620f453d` |
| HOST REL |84568| `be6208b6e891cc06a81b5687b33eef146dc4663dc2cd6e23b306e67e9db78c4a` |
| ARM archive |58136| `ec4bca864ac117c94f91e51bd7b8d42faa1e4346c0580a0813fa8c9219d8285f` |
| ARM REL |54200| `74a556b40e8b00f9e4118a47003e1e9557ba8eb26cff503beedbe3b49f821bc5` |

Independent collections cover1,303 unique reader inputs; `FINAL_SEAL.json` freezes
before/after equality, final fresh input rehash and own output pins. Source finding
R1 is separate from successful pin/receipt checks; there is no runtime PASS/FAIL.
No collection error or policy rejection was observed. Collection limits and author
bookkeeping-error distinction are retained in `COLLECTION_NOTES.md`.

## Handback / effect boundary

Only this new review directory was written. No candidate/helper/author script main,
compiler/generator/test/guard/listener/app execution, target/UART/RPC/status/network/
private IO, Git/index, central-plan/AGENTS/P1/model/provider/safeguard work occurred.
Original frozen listener/app/reviews remain untouched and are not composed here.
Standing instructions/08 plan were read in the preceding bounded review; this
review follows current explicit source-only steering and makes no central edits.

Parent may disposition R1 and separately select a minimum fresh source correction;
none is automatically queued. Bootstrap/provider/session ordering, external failed-
work retention, source backing, native pairing/timing/memory fit, provisioning,
listener composition, full app/release packaging and G3/G8 remain open, independently
of R1. No native or concurrency behavior is claimed. Last inherited hardware
observation remains original held U-Boot12:12UTC/catch consumed; no fresh state or
expiry assertion is made and both HOLDs remain unchanged.
