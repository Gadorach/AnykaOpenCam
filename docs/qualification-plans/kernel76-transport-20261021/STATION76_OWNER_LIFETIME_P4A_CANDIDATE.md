# Station76 P4a shared-owner lifetime prerequisite — SOURCE candidate

2026-10-22. Parent explicitly selected bounded P4a before P3; this is not P1
adoption, full P4/P5 selection, build/native authorization or source acceptance.

## Ownership / lock / retirement contract (written before implementation)

1. Derive a fresh byte copy from frozen P2 v2. Preserve accepted RX/event, licenses,
   all older candidates/reviews and P3's stopped baseline. No request/deadline
   implementation, normal data/UP/radio or status-schema expansion in P4a.
2. A single bus admission spinlock serializes permanent STOP against TX preparation,
   RX submission/dispatch and core/FWCTRL work queue admission. No spinlock is held
   across USB submit, completion handlers, synchronous work/timer joins or poison.
   An operation admitted before STOP may finish with its owners retained.
3. Allocate immutable **16 TX + 32 RX slot arrays**,2560-byte RX backing, enumerated
   by index. Lists are only scheduling hints, never lifetime/drain evidence. Retain
   every allocated URB pointer, including partial allocation, until final cleanup.
4. Reserve conservative per-slot submitter/callback owner leases **before USB
   entry**, plus a TX caller lease before preparation. These bounded leases stay
   nonzero until external return proofs; they are not purported active counts.
   Callback list movement/return-tail code never makes the owner counts zero.
5. Successful USB submit has one actual callback (possibly before submit returns);
   submit error has none. Actual callback and local submit-failure cleanup are
   separate owners of the skb/token. Never touch successful-submit skb/URB backing
   on the submitting return path after an immediate callback may have consumed it.
6. On retirement: close admission under the bus lock, wake pending controls with
   failure, synchronously join init/TX/test/detect/FWCTRL work and timers while all
   owners remain valid. Work admission uses the same permanent STOP gate.
7. Then call **usb_poison_urb on every allocated fixed slot outside all callback
   locks**, irrespective of free/used membership. Actual pinned Linux source
   guarantees callback return and permanent submit rejection even for never-used
   URBs. Never unpoison/reopen/retry this lifetime. Each poison return is the
   callback-return proof; list emptiness/counter snapshots are not.
8. Only after worker joins and all poison calls returned may lease totals be
   reconciled to zero, then core/FWCTRL and slot/backing/bus owners freed. Initial
   RX submission enumerates the32 slots once, not a callback-replenished free list.
   Exact partial allocation/probe rollback is included only because fixed ownership
   enumeration cannot otherwise be made real.
9. Any uncertain invariant retains owners and blocks retirement, **including module
   removal**; leaking allocations and returning is not safe module-text retention.
   Kernel poison/work/RTNL waits may block without a finite bound. No timeout skip,
   forced free, watchdog feed/budget extension or claim that P5 exists is allowed.
10. Preserve first-error order, strict sole FWINFO admission,500ms reply ceiling,
    existing5s later guard policy,16/32/2560 limits and128kB cap. Ordinary changed-
    unit HOST syntax only; runtime/functional/adversarial/fuzz/mutation tests remain
    unexecuted. Independent review and all later gates remain required.

## Kernel contract checked before implementation

Retained public kernel `drivers/usb/core/urb.c:724–769`: `usb_poison_urb` increments
reject permanently (including before the never-submitted early return), unlinks
and waits for USB use_count0. Its documented guarantee is that all completion
handlers have finished and the URB cannot be reused. `hcd.c:1519–1569,1634–1672`
reserves use_count before HCD enqueue and decrements it only after the driver
callback returns; error and giveback are disjoint. Poison may sleep and is called
only in process-context retirement, never while holding callback locks. Work and
module/proc return contracts will be retained with final source evidence.

## Implementation / evidence outcome

**Delivered an implemented best-effort SOURCE candidate for independent review.**
Final changed-unit HOST syntax run02 passed4/4; no runtime/functional/native result
or independent source acceptance is claimed. P3 remains unimplemented. This does
not resolve P1, full P4, P5, paired-build, native or product gates.

Only new `apps/qualification/kernel76-station-owner-lifetime/`, this report and
`station-owner-lifetime-p4a-source01/` were written. All71 P2 v2 source files were
copied as bytes before editing. Final source has **72 files:65 exact P2 v2 files,
six changed imports and one new USB unit**. All **338 frozen v1/v2/review/P3 custody
files** and **204 accepted A1/A2a/A2b source pins** matched at copy and final checks.
No P1 source/partial review/retry/adoption, provider/model/safeguard change or
policy rerouting was used. The earlier P3 technical block remains frozen evidence,
not a policy error and not a claimed P3 completion.

LICENSE remains SHA256
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
Original notices, wire declarations, accepted A1 copy helper, original core/procfs,
**A2b rx.c/event.c** and imported `utils/if_usb.c` remain byte-exact. No license
rewriting/global grant or image/module/helper modification occurred.

Final source-map SHA256:

```text
6fc087502ac6d0fe2a1b5988b85404cc81a0d95b0908d0964cfc9fd1686c4dd7
```

Format: compact sorted-key UTF-8 JSON source file-size/hash map. Evidence includes
`copy-baseline.json`, preserved `OWNER_CONTRACT_BEFORE_IMPLEMENTATION.md` (exact
contract written above before production edits), `kernel-owner-contracts.json`,
`module-return-contracts.json`, complete `P4A_SOURCE_DELTA.patch`,
`source-inventory.json`, both syntax runs and `candidate-seal.json` (excluding
itself). Per-file pins retain provenance, commands, UTC/tool versions and outputs.

### Actual source delta / why each part is necessary

| Relative source path | Implemented P4a boundary |
|---|---|
| `utils/owner_usb.c` (new) | Fixed slot allocation, permanent admission closure, pre-submit conservative leases, real/local completion ownership, finite initial RX enumeration, full fixed-slot poison and terminal owner cleanup. |
| `hgic_smac/Makefile` | Selects new owner USB unit instead of imported if_usb.c; inherited root recipes are not invoked or reviewed ARM recipes. |
| `hgic_def.h` | One internal bus admission lock/permanent stopping state and producer-stop callback; no wire/platform ABI. |
| `hgic_smac/attach_init.c` | Serializes init/TX work admission; splits producer stop/join from final core free; publishes both callbacks only after real initialization stages. |
| `utils/fwctrl.c`, `utils/fwctrl.h` | Serializes RX/command/work admission, fails/wakes live pending waiters and joins FWCTRL work before owner drain. No request/cancellation/deadline redesign. |
| `hgic_smac/attach_status.c` | Changes fixed policy label to `owner-lifetime-p4a-source01`; **schema2 and every field remain unchanged**. No owner counter/status/tombstone expansion. |

The partial-allocation rollback and finite initial RX loop are deliberately
included: all allocated URBs/backing must remain identifiable even when a current
RX backing allocation fails or a callback requeues an earlier slot. TX/RX capacity
and per-RX backing remain exactly16/32/2560. `max_pkt` is refused unless0 as passed
by the selected core; there is no larger aggregation/backing override.

### Admission and owner proof in the selected code

`bus.owner_lock` linearizes permanent `stopping` with USB TX/slot/RX-dispatch/
resubmit and core/FWCTRL work admission. FWCTRL's selected schedule hook now
requires this lock across queue+work admission; all actual selected callers follow
that contract. Invalid RX length releases admission before taking FWCTRL lock.
No receipt/FWCTRL lock is held while acquiring the admission gate. Nesting that
does occur is admission→receipt or inherited FWCTRL→receipt, and admission→skb
queue internals; receipt helpers do not acquire either admission or FWCTRL locks.
No USB submit, parser/core callback, synchronous join or poison runs under the
admission lock. A claim that won before STOP may still reach USB entry: this is
not physical atomic TX cancellation or P3's error/deadline submit fence.

The actual TX producer is the selected single ordered core TX worker; normal
data/UP/AP/BT/tool/maintenance effects remain refused. Its full function return
is established by the producer-stop callback's `cancel_work_sync`, not by a
counter decremented within `tx_packet`. Initial RX submission is synchronous in
probe and executes at most32 initial indexed attempts, not a requeueable-list
drain. Kernel driver-core device locks serialize this probe with disconnect for
that interface. Every asynchronous RX resubmitter belongs to an actual URB
callback, which is retained by the slot's kernel poison rendezvous.

Slot `submit_reserved`/`callback_reserved` bits and their totals are conservative
**owner reservations**, not current submitter, inflight, active-callback or accepted
submission counts. Each slot contributes at most one to each total, so arbitrary
RX resubmissions cannot overflow them. Both reservations are made under admission
before USB entry. A TX caller reservation precedes its preparation. They are
deliberately retained even after a failed/no-callback submission or completed URB
until stronger external return proofs exist; callback tails and submit returns
never reduce them to0. They are not exported as progress/retirement status.

Successful submit returns without further skb/slot/URB access on the submitting
path: an immediate callback may already have freed the skb, returned the token
and moved the slot. On submit error, USB/HCD guarantees no callback for that
attempt, and the local owner alone returns the slot/token and reports skb failure.
The local boolean failure notification is explicitly not a real HCD completion
or a callback-return proof. RX follows the same successful-return/no-touch and
local-failure/no-callback split. No synthetic successful response/ACK is created.

On retirement the permanent gate closes first. Core stop marks teardown, preserves
earlier first-error and fails pending init, wakes only live list-owned controls
with failure, joins FWCTRL/init/TX/test/detect work and synchronous timers while
core/FWCTRL/USB backing remains valid. A pre-STOP RX dispatch may finish, but its
FWCTRL queue admission sees STOP and cannot race a later enqueue after the join.
The waiter's existing500ms ceiling is not increased; a validated first negative
response is not replaced or copied as payload.

The final poison sweep visits **every allocated fixed slot**, including ones
already moved to free lists and a partially allocated/unlisted current slot.
`usb_poison_urb` is invoked outside all callback locks. Its permanent reject prevents
later reuse; kernel use_count waits past the actual driver's callback return.
No unpoison/reopen path exists. Only after every poison returns and producer joins
have returned are slot/caller leases cleared, core/FWCTRL released and stable
backing/URBs/USB owner freed. Lists never decide which URBs to join or free.

The code validates allocation/poison and reserved-slot reconciliation and requires
TX skb ownership to have been retired (slot data NULL). An impossible/uncertain
condition enters an uncompleted wait with all owners retained. It does **not**
free, return, synthesize completion, time out or feed a watchdog. This can block
disconnect/module exit indefinitely; it is intentionally not a successful normal
unload or a substitute for P5 bounded recovery.

### Module text is not retained by leaking allocations

The source return argument uses actual synchronous APIs, not a private reference
count or list/counter snapshot:

- `usb_poison_urb` returns after driver callback code/return tails have completed,
  including slots no longer on a used list; USB use_count is decremented after
  `urb->complete` returns in HCD giveback.
- `cancel_work_sync` guarantees no pending/executing work if racing enqueues are
  excluded. The permanent common admission gate supplies that exclusion; timer
  callbacks are synchronously deleted and never rearmed by this profile.
- Normal module exit calls `usb_deregister` synchronously. Actual source route:
  USB `driver.c:1115–1123` → base `driver.c:270–278` → `bus.c:781–797` →
  `dd.c:1419–1442`, whose locked detach calls device removal before unlinking the
  driver/device association. USB `driver.c:431–461` calls this disconnect before
  unbind returns. Blocking poison/quarantine therefore prevents normal driver/
  module removal from returning; there is no leak-and-return branch.
- The inherited module exit then unregisters its netdev notifier under RTNL
  (`net/core/dev.c:2016–2037`; notifier calls require RTNL at:2228–2243) and removes
  module-owned proc entries. Existing proc rundown waits callers/open releases
  (`fs/proc/inode.c:255–273`). No new proc/device-private reader is introduced.

These are bounded-profile **author source arguments**, not runtime or all-context
independent lifetime acceptance. Forced module unload is neither supported nor
authorized. Kernel USB unbind may disable the interface and deliver real error
callbacks before invoking driver disconnect; such observed errors remain sticky,
not relabeled successful normal shutdown.

## Commands / compile-only evidence

```sh
python3 -B apps/qualification/kernel76-station-owner-lifetime/prepare_source.py
python3 -B apps/qualification/kernel76-station-owner-lifetime/check_syntax.py syntax01
python3 -B apps/qualification/kernel76-station-owner-lifetime/check_syntax.py syntax02
python3 -B apps/qualification/kernel76-station-owner-lifetime/freeze_candidate.py inventory
python3 -B apps/qualification/kernel76-station-owner-lifetime/freeze_candidate.py seal
```

`syntax01` preserved: core/status/FWCTRL returned0; new owner USB failed because
its skb/netdevice declaration includes were missing. Corrected by explicit
`linux/skbuff.h`/`linux/netdevice.h` includes, with no compiler/safeguard/permission
change. **Final syntax02:4/4 changed C units returncode0**, matching the final72-file
source map and including changed declarations through dependencies. Clang23.1.3,
HOST x86 installed Linux6.18.55-2-cachyos-lts headers, actual `-fsyntax-only`, implicit
function/incompatible-pointer errors enabled. All argv/UTC/compiler/source/header/
diagnostic pins retained. Installed-header nocf/address-space warnings remain;
this is not warning-free or paired ARM/config/module ABI evidence.

No runtime/unit/functional, adversarial/penetration/fuzz/mutation tests or unchanged
suites ran. No app/test/native executable, Kbuild/object/link/ARM/module/full image
build, staging, target/UART/RPC/status/network/private IO, watchdog/reset, Git/index/
AGENTS/central-plan or frozen boot/helper edit occurred. Hardware/parent operations
and original failed HOLD are not queried, changed or inferred by this source lane.

## Remaining gates / ordinary confirmation deferred

- Independent exact-pin review of this P4a implementation and source return/lock
  arguments remains required. No author source/native/product GO is asserted.
- Ordinary functional confirmation is unexecuted: successful one-shot init/DOWN;
  normal stop/join/poison return; genuine callback-before-submit-return and deferred
  completion ownership; submit error versus callback ownership; exact partial-slot
  cleanup and fixed initial RX behavior; callback/free-list movement versus poison;
  blocking/uncertain retirement retaining both owners and module text. This list
  selects no test campaign and marks none of the deferred coverage passed.
- **P1 remains unmet/absent. P3 remains unimplemented:** caller-stack receipt,
  queue eviction/cancellation, absolute deadlines and error-versus-submit fences
  are not repaired by this packet. Inherited USB token wait is still1000 jiffies
  (10s at target HZ100);500ms reply ceiling is unchanged. STOP/join/poison/RTNL
  can block. Nothing here proves init observation<=2s or successful5s recovery.
- **Full P4 remains open:** exact descriptors/endpoint/interface admission, complete
  accounting/receipts/tombstone/final summary, independent full teardown closure
  and IRQ/HCD/DMA/native qualification. Fixed-slot rollback and RX enumeration are
  included narrowly as inseparable owner prerequisites, not full P4 acceptance.
- **P5 remains absent:** actual bounded watchdog/controller/progress/source-generation/
  recovery/disarm contract. Reservation counters, status reads, blocked callers or
  poison attempts do not authorize watchdog feeds. No timeouts/capacities/budgets
  were increased and no generic guardian/reference/trace framework was introduced.
- No physical RF-silence, station/network/media/combined-memory/product, paired
  image/module/ABI, persistent-install or other-camera gate advances. Old evidence,
  images/modules/helpers,128kB allocation cap and all license/provenance pins stay.

**ONE next eligible action:** parent independent SOURCE review of this exact P4a
candidate and its limitations. No automatic P3/P4/P5 implementation, tests, build,
integration, staging or native operation is queued by this handback.
