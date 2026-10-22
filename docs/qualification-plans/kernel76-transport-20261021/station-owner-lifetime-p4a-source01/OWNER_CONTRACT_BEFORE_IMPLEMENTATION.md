# Station76 P4a shared-owner lifetime prerequisite — SOURCE candidate in progress

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

Pending in this fresh report. No compiled, tested or accepted result is claimed
by the contract above. Final source delta, custody, syntax metadata, limitations
and deferred ordinary confirmations will be appended before sealing.
