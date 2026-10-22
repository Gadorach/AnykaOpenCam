# Station76 owned-control P3 — SOURCE candidate on reviewed P4a

2026-10-22. Fresh parent-selected P3 on exact reviewed P4a. No P1 integration,
generic lifetime API, full P4/P5/native or runtime/test selection.

## Local request / ref / lock contract (written first)

- Exactly one bounded heap request embeds its response/completion; no async user
  points to a caller-stack response. Static size checks bound the request and the
  skb-cb heap-pointer metadata. Original A1 wire/copy/validation bytes remain.
- Explicit refs/roles: synchronous waiter; module/device-lifetime registry;
  pending correlation list; queued UNSENT; sole ordered TX worker (queue ref
  transfers to worker); submitting return path; actual callback reservation.
  Every role mutation and refcount update is serialized by the existing receipt
  lock. A registry ref keeps the object until P4a final release, after joins/poison.
- States distinguish NEW, QUEUED_UNSENT, WORKER_PREPARING, SUBMITTING (potentially
  submitted), SUBMITTED, CALLBACK_RECONCILED, CANCELLED_UNSENT, LOCAL_FAILED and
  final RETIRED. Callback reconciliation is not proof its return tail has returned;
  only P4a poison/worker joins permit final request/private-owner release.
- Use the **actual immutable first-error lock** for both request cancellation and
  final submit CLAIM. The existing first-error recorder calls a locked request
  hook: queued UNSENT is detached/freed once; worker/USB backing is never freed
  by this hook. Other owned phases retain backing and receive sticky failure.
- Lock order: P4a bus admission → FWCTRL correlation → receipt/request, with
  permitted subsets. No receipt/request-held path takes either earlier lock. No
  callback-used lock is held across usb_submit_urb, waits, joins or poison.
- Pending list and queue publication use bus/FWCTRL/request serialization. The
  sole ordered worker claims the one queue item; no generic skb queue eviction,
  capacity retry, new producer/timer/worker or request cancellation thread exists.
- Seal one immutable absolute ktime monotonic deadline **before allocation and
  admission**, bounded by existing500ms. Check remaining budget at publication,
  worker claim, preparation/token, submit claim, response/copy and the sole wait.
  Never restart/grow it; no sleep/credit/retry loop. A sub-jiffy remaining budget
  may fail conservatively rather than request a wait longer than that budget.
- Replace selected USB1000-jiffy token wait with one nonblocking admission.
  Before USB entry reserve submitting and actual-callback request refs and token
  ownership, under admission + receipt/request locks. If error/STOP/deadline wins,
  no claim is made. If claim wins, later failure means potentially submitted;
  it is not relabeled UNSENT or a physical atomic TX-cancellation guarantee.
- Successful submit return touches only its independently referenced heap request,
  never skb/backing/reused slot. Actual callback performs skb/slot/token cleanup
  before its final request reconciliation. Submit error has NO real callback;
  the local path reconciles callback reservation, submit ref, token and skb once.
- RX uses the actual A1 association/status/copy functions. A valid signed negative
  status is latched while FWCTRL lock is held before response publication/wakeup.
  A response must also belong to a request that reached a submit claim. First
  response wins; negative/late/failed results never copy or repair terminal state.
- STOP cancels only owned UNSENT and wakes live heap waiters. P4a joins the same
  init/TX/FWCTRL producers and poisons the same48 slots before final FWCTRL release.
  Async request users cannot outlive those proofs. A role/ref discrepancy blocks
  final release with owners retained; leaking and returning is not module safety.
- Keep16TX/32RX/2560B,128KiB allocation cap and existing5s guard policy. Init/RTNL/
  poison waits remain unbounded; no2s/5s native/recovery closure is claimed. P4a
  reservation bits remain conservative owner reservations, not new request refs.

## Final outcome / evidence

**Implemented a best-effort one-request SOURCE candidate. Final HOST syntax04:
5/5 changed C units returncode0.** No independent source sufficiency, functional,
native, paired-build or product GO is asserted. The contract above was preserved
before implementation as `REQUEST_CONTRACT_BEFORE_IMPLEMENTATION.md` in the new
evidence directory; it is not retroactively described as executed proof.

Only new `apps/qualification/kernel76-station-control-owned/`, this report and
`station-control-owned-p3-source01/` were written. Initial copy was all72 reviewed
P4a source files as exact bytes. Final source has **74 files:66 unchanged P4a,
six changed imports and two new files**. All **451 frozen prior custody files**
(the338-file older union plus P4a candidate/review and their seals) and **204
accepted A1/A2a/A2b source pins** were verified. P4a review seal remains
`c677fe73b1e28691365e31383780f12b89b797bafe6baa075f6d7ae366bdc4ff`.
Stopped P3, P2 v1/v2 and all older reviews/candidates remain unchanged. No P1
source, partial review, retry/adoption/completion or substitute acceptance was used.

LICENSE remains
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
Existing notices, wire declarations, accepted A1 helper, A2b **rx.c/event.c**,
original core/procfs, legacy if_usb.c, hgic_def.h and schema2 receipt declaration
remain byte-exact. No source-license rewrite, global grant or publication follows.

Final source-map SHA256:

```text
f9865e63c55b09bdd36d6b53ae61acf8dfd4f3b340de5d3409e1a7c31204722e
```

Format: compact sorted-key UTF-8 JSON source file-size/hash map. Evidence retains
`copy-baseline.json`, original preimplementation contract, `request-api-contracts.json`,
`OWNED_CONTROL_DELTA.patch`, `source-inventory.json`, all syntax runs and
`candidate-seal.json` (excluding itself). Exact source/build/tool/header/command/
UTC/diagnostic pins are included. P4a's source/API and module-return evidence is
also revalidated, not reexecuted.

### Actual production changes

| Relative source path | Selected change |
|---|---|
| `utils/owned_control.c`, `utils/owned_control.h` (new) | One request, role/ref/state ledger, immutable deadline, owned queue transfer/cancellation, submit/callback/local-failure handling and final registry reconciliation. |
| `utils/fwctrl.c`, `utils/fwctrl.h` | Heap-embedded receipt, exact single-request publication, remaining-budget wait/copy, response/acceptance handshake, STOP and P4a final-release hooks. Old selected eviction/TX skb queue and stack response removed. |
| `hgic_smac/attach_init.c` | First-error hook under the actual existing receipt lock, private locked accessors, and the same ordered TX worker consuming one owned request instead of generic skb queue. |
| `utils/owner_usb.c` | One nonblocking token attempt, final atomic submit claim, pre-entry callback/submitting refs and explicit real-callback versus local-submit-failure reconciliation. |
| `hgic_smac/Makefile` | Adds the one request implementation object; producer topology/selected owner transport unchanged. |
| `hgic_smac/attach_status.c` | Fixed policy label becomes `control-owned-p3-source01`; **schema2/fields unchanged**, no request-ref/progress/trace expansion. |

### One queue, exact role ownership and final free

The request is statically constrained to **<=1024 bytes**, below the128KiB cap.
The skb cb stores only a bounded copied heap-request pointer, with a static fit
check against the real skb cb. It never stores a stack response, output buffer,
caller completion or local cb-object address. The output remains synchronous
inside the init caller; async code cannot copy into a returned caller stack.

| Role | Acquire / reconcile in actual code |
|---|---|
| WAITER | Constructor's sole initial ref; released after command return/copy cleanup. Unpublished constructor failure frees only its private allocation. |
| REGISTRY | Installed under admission/receipt locks for this one local generation; retained until final P4a FWCTRL release after joins and poison. |
| PENDING | Taken before linking the heap receipt under FWCTRL lock; the waiter removes that exact list entry and ref under the same lock. RX worker borrows within that protected correlation interval. |
| QUEUE | Publication's one owned UNSENT entry; no old ctrl.txq capacity eviction or unrelated skb removal. |
| WORKER | Queue ref transfers atomically to the sole ordered TX worker. Error hooks cannot free its command. Released only after worker abort or transport returns. |
| SUBMITTER | Reserved by final submit claim before USB entry; released only on actual usb_submit_urb return. |
| CALLBACK | Reserved before USB entry. Real callback reconciles after actual skb/slot/token cleanup; submit error, which has no callback, reconciles this reservation locally. |

After private construction, every role/ref update is under `attach_lock`; queue-to-worker transfer changes
roles without an unowned interval. Normal peak is six refs (queue and worker are
mutually exclusive). The registry remains even after a successful reply, canceled
queue, waiter return or callback reconciliation. No intermediate ref drop frees
the request. It can reach final RETIRED/free only after the unchanged P4a owner
return route calls final FWCTRL release and verifies sole registry ref, no pending/
queue/worker/submitter/callback/waiter roles, no skb/response/token owner and, for
accepted submission, actual callback observation. A mismatch blocks with owners
retained; it does not invent reconciliation or return from module removal.

`CALLBACK_RECONCILED` means that the callback performed its request/token cleanup;
it is deliberately **not RETIRED or callback-return proof**. P4a's full poison
return provides that proof before registry drop. P4a conservative slot bits/totals
are unchanged, are not request refs/active counts, and never substitute for it.

### Actual error/STOP/deadline-versus-claim serialization

The former private receipt `attach_lock` is now shared only among this module's
receipt/request objects; it is not exported as a kernel/platform ABI. The real
`attach_error_locked` writes the immutable first cause/phase and invokes the
request hook **while still holding that lock**. All first-error paths continue
through that recorder. The hook detaches/frees only `QUEUED_UNSENT` once and
signals sticky failure. It never takes FWCTRL or bus admission locks, removes
another queue item, frees worker/URB backing or retracts a completed copy.

The final USB claim holds P4a bus admission then that same receipt/request lock,
checks STOP, the real first-error, local generation, immutable deadline, worker
role and exact command identity, then records SUBMITTING/potentially-submitted,
callback/submitting refs and token ownership before releasing locks and entering
USB. Thus:

- Error/deadline wins while QUEUED: detach the owned UNSENT command/ref once;
  later worker dequeue finds no item.
- Worker preparing wins first but claim has not won: backing stays worker-owned;
  later gate failure/local abort reconciles it, not the error hook.
- STOP/error/deadline wins before final claim: no callback ref/USB claim, and only
  the local worker/transport cleanup consumes skb/token.
- Claim wins first: a later error/expiry leaves potentially-submitted ownership
  intact. It cannot be called UNSENT, removed as if never submitted, or freed by
  timeout. Accepted/callback history may subsequently update, but sticky error
  and returned failure are never cleared by late success.

Lock nesting is bus admission→FWCTRL→receipt/request where all are needed, and
the corresponding subsets. No receipt-held path takes an earlier lock. Global
error observation borrows the registry under that lock, rather than needing a new
async producer/ref. No callback-used lock spans USB submission, waits, joins or
poison. Existing P2 first-response and validated negative-status recording remains
under FWCTRL→receipt locking before publication/wakeup.

The claim-to-USB interval still exists: a claim that won before the clock boundary
or error/STOP may reach USB later. This is a serialized **software ownership
claim**, not physical atomic TX cancellation, proof of zero wire effects or a
guarantee that HCD entry itself occurs before500ms.

### Deadline, nonblocking token and reply/submit handshake

`hgic_fwctrl_attach_fwinfo` computes the single absolute CLOCK_MONOTONIC nanosecond
deadline before the command grant and all allocations. Overflow fails; a previously
sealed deadline is not replaced. The heap request receives that value once.
Remaining budget is checked at registry publication, pending/queue publication,
worker claim, preparation/token, submit claim/return, response observation, wait
and copy. There is no per-stage restart, capacity retry, credit query, polling/
sleep loop or automatic recovery. Selected transport uses one `down_trylock`;
failure is sticky `-EAGAIN`, with no1000-jiffy token wait.

The sole completion wait requests only remaining whole jiffies, failing
conservatively if an uncompleted request has less than one tick left. An already
notified request needs no extra wait. A fresh clock check protects final copy,
so scheduling delay cannot extend acceptance. This is not a hard wall-clock
return bound: allocation, USB/scheduler, RTNL, init/join and poison behavior can
still block or resume late; P5 remains absent.

Wire encoding, exact A1 envelope/identity/status/retail FWINFO validators and
24-byte copy/decode remain unchanged. The heap pending association additionally
requires that its request reached submit claim; no reply can manufacture success
for still-queued UNSENT work. A positive reply waits for **both** actual existing
A1 response publication and successful USB submit return. Otherwise a response
observed while submit was still in progress could wrongly appear successful even
if that submission later failed. Negative/failed/deadline cases wake as errors,
keep the first response owned and never copy its payload. This is ordinary request
state binding, not a new opcode, wire token, RX grammar or P1 substitute.

### Immediate callback, local failure and P4a topology

On actual callback, the request pointer/status are captured before skb free;
core skb completion, stable slot requeue and token return happen before final
request callback reconciliation. Its registry plus actual-callback ref keeps
request memory alive independently of waiter state. P4a retains the return tail.

On successful usb_submit_urb return, only separately referenced heap request
metadata is touched, never skb/backing/reused slot. If callback already reconciled,
submit return does not overwrite that history or acquire a late callback ref.
Submit error has no callback for the attempt: the local path drops callback/
submitter reservations, returns token and consumes skb exactly once. The local
boolean skb-failure notification is never counted as real HCD completion.

The actual ordered core TX work, init waiter, existing FWCTRL RX work and actual
USB callback are the only asynchronous users. No timer, thread, worker or producer
was introduced. Full P4a producer-stop/final-core-remove, fixed-slot dispose/poison,
RX callback and conservative-reservation function blocks were compared byte-for-
byte against the reviewed source and are recorded in `preserved_p4a_blocks`.
The new final request release is inside final FWCTRL release, called **after**
that unchanged join/poison route. Request refs alone are not owner/module-text
retention; blocking discrepancy handling relies on that real caller return route,
not leak-and-return. Forced unload is unsupported/not authorized.

## Compile-only evidence / commands

```sh
python3 -B apps/qualification/kernel76-station-control-owned/prepare_source.py
python3 -B apps/qualification/kernel76-station-control-owned/check_syntax.py syntax01
python3 -B apps/qualification/kernel76-station-control-owned/check_syntax.py syntax02
python3 -B apps/qualification/kernel76-station-control-owned/check_syntax.py syntax03
python3 -B apps/qualification/kernel76-station-control-owned/check_syntax.py syntax04
python3 -B apps/qualification/kernel76-station-control-owned/freeze_candidate.py inventory
python3 -B apps/qualification/kernel76-station-control-owned/freeze_candidate.py seal
```

Syntax01: four units returned0, new request unit failed on missing netdevice
declaration include for dev_kfree_skb_any; failure/log/input pins preserved. Adding
the explicit include fixed it without compiler/permission/safeguard change.
Syntax02/03:5/5 at their intermediate pins. **Final syntax04:5/5 returncode0**, exact
final74-file input map, after sealing deadline before command grant and adding
the final accepted-submission/callback invariant. Actual `-fsyntax-only`, Clang
23.1.3/HOST x86 installed Linux6.18.55-2-cachyos-lts headers, implicit-function and
incompatible-pointer errors enabled. Known installed-header nocf/address-space
warnings remain; not warning-free or paired ARM/module ABI evidence.

API pins/excerpts include actual down_trylock inverted return convention,
CLOCK_MONOTONIC ktime_get_ns, whole-jiffy conversion, completion timeout and
refcount acquisition/release contracts. Old reviewed P4a USB/HCD/work/proc/module
return contracts are unchanged/revalidated. Static checks and compile metadata
are not functional/SMP/IRQ/HCD/DMA proof.

No unchanged suites, shims, runtime/unit/functional/adversarial/penetration/fuzz/
mutation tests, app/native executable, Kbuild/object/link/ARM/module/image build,
staging or target use occurred. No UART/RPC/status/network/private IO, parent
hardware polling, settings/model/provider/safeguard changes, Git/index/central-plan/
AGENTS or old code/helper edit occurred. Old hardware HOLDs/state are neither
queried, cleared nor inferred by this lane.

## Incomplete gates / ordinary functional checklist (NOT RUN)

- Independent exact-pin P3 source review is required before adoption or another
  implementation/build selection. No author SOURCE/native/product GO is asserted.
- Ordinary confirmation remains unexecuted: successful sole FWINFO output;
  queue→worker ref transfer; owned queued cancellation and no late UNSENT submit;
  worker preparation expiry/STOP; token exhaustion/local submit failure cleanup;
  actual immediate and deferred callback/submit return ownership; reply-before-
  submit-return handshake; negative-status chronology/no error-payload copy;
  waiter timeout before late response/completion; final role/token/registry
  reconciliation through normal P4a joins/poison; blocking discrepancy retaining
  owners/module text. No test campaign is queued or deferred coverage marked PASS.
- **P1 remains absent/unmet**. This A2b/P2/P4a-derived source must not be described
  as a composite/native attach candidate. All frozen RX/event bytes stay exact.
- Full P4 descriptor/admission, complete transport accounting/status/tombstone/
  final summary and independent all-context/IRQ/HCD/native retirement qualification
  remain open. P3's local phase/ref ledger does not supply those receipts.
- P5 bounded controller/watchdog/progress/source-generation/recovery/disarm remains
  absent. Init/RTNL/poison and blocked ownership reconciliation remain unbounded;
  no2s/5s completion/recovery proof or permission to feed/extend a watchdog follows.
  Claim-won potentially-submitted outcomes are retained failed, never forced free.
- No capacity/backing/ceiling or500ms/5s budget increase, normal data/UP/radio,
  physical RF-silence, network/media/combined-memory, paired ABI/image/module,
  persistent install, product or other-camera acceptance.48 URBs/2560B and128KiB
  cap remain intact; no generic guardian/lifetime API or trace framework was added.

**ONE next eligible action:** parent independent ordinary SOURCE review of the
exact frozen owned-control candidate, preservation of the P4a return contract and
the stated limitations. No automatic test/build/P4/P5/native queue follows.
