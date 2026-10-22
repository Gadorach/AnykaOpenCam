# Station76 P4a — ordinary shared-owner SOURCE/API review

2026-10-22. RP-02 / NET-01–08 / K76-06; explicitly selected P4a prerequisite
before P3, not full P4/P5 or P1 integration.

## Disposition

**Narrow SOURCE sufficiency for the selected shared-owner prerequisite.** No
concrete correction-required defect was identified in the seven-file delta and
its actual selected producer/retirement/API paths. The stopped P3 design may be
resumed against this exact owner contract, subject to fresh parent selection.
This is not P3 implementation or a general-purpose lifetime/lease API acceptance.

The conclusion depends on the actual bounded topology: sole ordered core TX
producer, serialized interface probe/teardown, callback-owned RX resubmission,
permanent STOP, complete producer joins and full fixed-slot poison before owner
free. New independent producers or async P3 request users must be integrated into
that contract; the sticky counters alone do not retain memory or module text.

**Not established:** normal functional/IRQ/HCD execution, successful or finite
retirement,5s recovery, paired ARM/module/image/native acceptance, full P4 receipts/
descriptors/tombstone, P5, composite attach or product GO. P1 remains untouched,
absent/unmet. No automatic implementation, test, build, integration or native queue
follows. Parent's held-U-Boot observation and both failure HOLDs are unchanged;
this lane did not query hardware or infer Linux availability.

Only this new report and `station-owner-lifetime-p4a-review01/` were written.
Static public reading/diff/hash/metadata only; no candidate execution, compiler,
runtime/unit/functional/adversarial/penetration/fuzz/mutation test, target/UART/RPC/
status/network/private IO, original source/helper edit or Git/index/central-plan
operation. No P1 inspection/retry/review/adoption/completion or safeguard/provider/
model change occurred. No policy rejection occurred.

## Custody, selected source and exact pins

`S/` = `apps/qualification/kernel76-station-owner-lifetime/source/`.
`K/` = retained public kernel tree at
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.

- Supplied103-file author seal SHA256 verified:
  `49e1c0e6209371e4714e4fb1ff5adad4d8cb042014fe4508f2e5257e37f49490`.
  Exact candidate/evidence file set matched, not merely the listed hashes.
- Exact72-file source-map SHA256:
  `6fc087502ac6d0fe2a1b5988b85404cc81a0d95b0908d0964cfc9fd1686c4dd7`.
  Format is compact sorted-key UTF-8 JSON file-size/hash map.
- 65 imports match frozen P2v2 exactly; six changed imports and one new file.
  Independently reconstructed actual universal-newline text diff byte-matches
  `P4A_SOURCE_DELTA.patch`; exact raw source hashes independently preserve original
  line endings. No hidden source deletion/addition was found.
- 338 frozen custody entries matched the **exact union** of v1/v2 candidates,
  their two reviews and the stopped P3 seal, including the five seals themselves.
  204 accepted A1/A2a/A2b source pins matched their accepted seals. The copied
  71-file baseline equals the sealed P2v2 source set.
- LICENSE remains
  `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
  Inherited notices/provenance and new GPL-2.0-only file notice remain retained;
  no global license grant or publication authorization is inferred.

| Changed/new S/path | SHA256 |
|---|---|
| `hgic_def.h` | `0609a727a8b3dc85d8fd9329825e4878dcd066364e0671cbc0c2d34fed978b48` |
| `hgic_smac/Makefile` | `5f81d8601b7e5ff0b8ac8071a67b03175d6baf9b29dc7b2d65433fa824c4fcfa` |
| `hgic_smac/attach_init.c` | `48635884e300d5702914af03cfe6f22f9c619c893942491e5705a54fc6466bcf` |
| `hgic_smac/attach_status.c` | `ba940df79dd7c9e29180e635851c8d3227a709b495ee56cb8fff3a9f2e29e9fc` |
| `utils/fwctrl.c` | `f98c49e46fd676bbe5c691a51553627c2c990e8e281e0da892899f47bfeb0cf5` |
| `utils/fwctrl.h` | `43be59f0879dec5e6a745f98d192702e1824ca69f9ce4627b1e69750a241b1f7` |
| `utils/owner_usb.c` | `a7a482522919327132babe6c9302452922ceee21dde827397f0dbd813f2ceab6` |

`S/hgic_smac/Makefile:21–25` selects **owner_usb.o**, not legacy if_usb.o, for
Linux2G/USB. Legacy `utils/if_usb.c` is immutable provenance only; its old used-list
drain is not treated as active behavior. Original core/procfs and A2b RX/event
files are also unchanged. `attach_status.c:14` changes only the policy label;
schema2 and fields remain unchanged, not a new owner-count/tombstone receipt.

Evidence: `public-pin-verification.json`, `actual-p2v2-p4a-diff.patch`,
`public-excerpts.json`, `reader-receipt.json` and final `review-seal.json` in the
owned directory. They retain commands/UTC/Python, exact public input hashes,
line/text/range hashes and output pins. All11 author API/source excerpt entries
were verified against actual full-file bytes; additional kernel return contracts
below were read and pinned independently. Static integrity is not behavioral proof.

## Admission, lock order and producer return

`hgic_def.h:171–198` adds the common owner lock/permanent stopping flag and splits
owner_stop from final remove. `owner_usb.c:60–72` closes STOP under that lock;
there is no reopen/unpoison path. `:156–176,198–205,248–258,293–307` serializes RX
submit/dispatch and TX preparation/slot claims against STOP. A pre-STOP claim may
reach USB later; this is retained-owner admission, **not** P3's deadline/error
versus actual-submit cancellation fence.

Core TX dequeue (:298–304), selected FWCTRL scheduling (:315–324), and init work
admission (:416–439) in `attach_init.c` use that same gate. `fwctrl.c:349–370`
serializes queue plus schedule admission; `:392–421` serializes RX copy/queue/work
admission. Its invalid-length branch releases owner_lock before taking ctrl->lock.
The selected schedule caller holds owner_lock, as required by the new assertion.
No selected FWCTRL RX/TX/init work enqueue can win after STOP and race a completed
join. Compiled legacy AP helpers can queue/rearm, but the selected attach core
does not wire their beacon callbacks or admit AP/start; its timers are initialized
to the non-rearming refusal callbacks. This is not acceptance of broader AP code.

Observed nesting is owner→receipt, FWCTRL→receipt and owner→skb-queue internals;
no selected receipt/FWCTRL-held path acquires owner_lock. USB submit, RX/core/real
completion calls, semaphore waits, poison and synchronous joins execute outside
these admission/callback locks. `dev_alloc_skb` in the locked RX path uses its
atomic allocation contract. No concrete direct lock cycle/self-join was found in
the actual process-context disposal entrypoints.

`owner_usb.c:329–351` closes admission, calls owner_stop while owners remain valid,
then poisons slots. `attach_owner_stop:442–469` marks removal/teardown, fails pending
init, wakes controls and joins FWCTRL/init/detect/test/TX work and both timers.
It releases all locks before waits. Only final `attach_core_remove:475–500`, called
after the poison sweep, unregisters DOWN hardware, releases FWCTRL/queues and frees
the core. Early unpublished core-probe failure uses the same initialized-stage
stop/remove split (:585–588), before any USB submission/publication exists.

Kernel `K/kernel/workqueue.c:4463–4485` guarantees cancel_work_sync returns with no
pending/executing work **provided racing enqueues are excluded**. The permanent
common gate supplies that condition for these selected work producers. A pre-STOP
RX dispatch may still finish while joins run, but its FWCTRL enqueue rechecks STOP;
the callback/core owner is retained until poison return. Queues with locally owned
leftovers are purged only after that callback rendezvous.

## Stable slots, preparing submitters and completion ownership

`owner_usb.c:15–47,112–153` owns immutable arrays of **16 TX and32 RX slots**;
each successfully allocated URB pointer is retained in its index even if the
current2560-byte RX allocation fails. List insertion and rx_allocated follow actual
backing success; allocation failure never loses that unlisted URB. The zeroed
remaining slots are safely skipped/freed as NULL. Probe requires all16/32 slots
(:441–446). No variable capacity/backing override exists (:497–503), and initial
RX submission enumerates at most32 indexed attempts (:458–463), not a replenished
free-list drain. Callback resubmissions are separate from that finite initial loop.
These narrow allocation changes do not qualify exact descriptors or load budgets.

Reservations are made before USB entry (`:85–101,167–168,257,300–307`). Each slot
sets its sticky submit/callback bits once, bounding both totals at48 despite RX
resubmission. **These are conservative reservation flags, not active counts, refs,
accepted-submission totals or retirement evidence.** Their sufficiency here comes
from the real external rendezvous and retained allocation structure:

| Producer / return tail | Actual owner-return proof before free |
|---|---|
| Sole TX worker, including preparation before slot assignment and submit return | owner_stop joins the full core TX work function, not an in-function counter decrement. |
| Initial RX submit loop and probe rollback | Same-interface probe/unbind is device-lock serialized; rollback runs in its own probe after a submit call returns. No concurrent disconnect frees that initial caller's objects. |
| RX resubmit, including a claim paused before usb_fill/submit | It executes inside that slot's retained actual callback; poisoning the already-submitted URB waits past its callback return, and permanent reject denies a later resubmit. |
| Callback already moved to a free list | Full fixed-slot poison ignores scheduling-list membership and still waits for kernel use_count return. |

TX success returns immediately at:307–311 without touching skb/slot/URB backing
that an immediate callback may have consumed. Actual TX completion alone consumes
the successful attempt's skb, clears data, requeues the slot and returns its token
(:224–237). Submit error has **no callback for that attempt**; :313–326 alone clears/
returns locally acquired slot/token and reports/frees the failed skb. Its boolean
failure notification is not counted as HCD completion. RX success/error split
(:174–188) likewise avoids post-success backing access and owns local submit-error
cleanup. Kernel/core caller tails may still execute; full worker join or callback
poison return, not this immediate-return style alone, protects them.

## Actual Linux USB return contract, not list/count inference

`K/drivers/usb/core/urb.c:751–769` permanently increments reject **before** the
never-used URB dev/ep early return. Used URBs are unlinked and wait for use_count0;
the API explicitly promises all completion handlers finished/no reuse. The new
transport calls this on every allocated TX and RX index, including free-list and
partial/unlisted slots, outside locks. No callback frees the URB itself.

The permanent reject is operational: `hcd.c:1130–1139` rejects link with-EPERM;
the retained MUSB enqueue actually calls that helper under its lock
(`drivers/usb/musb/musb_host.c:2156–2172`). `include/linux/usb.h:1734–1748` does not
reset reject during bulk fill. `hcd.c:1519–1569` takes USB use_count before HCD
enqueue and explicitly requires error **or** giveback, not both. Actual giveback
calls driver completion at:1661, then decrements use_count at:1665 and wakes poison
waiters at:1673–1674. Therefore callback list movement/token return is not mistaken
for completed callback return. HCD's own later URB put uses its retained URB ref,
not freed driver slot/core data.

`owner_usb.c:355–381` reconciles allocation/poison counts, sticky slot totals and
retired TX skb ownership only **after** worker joins and all poison calls returned.
It never treats an empty used list as drain proof. Only then does it clear the
reservations/caller flag and call final core remove, free every URB/RX backing and
permit the outer USB owner to be freed (:382–391,471–486). An inconsistent state
does not skip a wait or guess-free: it quarantines with all remaining owners live.

Critical API input hashes (full remaining input map is in reader-receipt.json):

| K/path | SHA256 |
|---|---|
| `drivers/usb/core/urb.c` | `6b5fbce916aa73103514a0e62089b9bff1d9b4effa19c6f2c8ce857981ffd333` |
| `drivers/usb/core/hcd.c` | `eba343195364f777f68b94278eb5bf1333d07b018d0552b8d449fc35fcfda868` |
| `kernel/workqueue.c` | `ba5bac7c5cfdf618728aee426b1bac1bc90e310fe4ec7bb6a9a08a723a7d054a` |
| `kernel/module/main.c` | `12db96fa1f6ed05d85bdf78711a54d5685f102ed385fcc28dcf15cb16c2a639b` |
| `lib/klist.c` | `138ea7c3d203a6592955ef4f4c74b179264b387eb9dd0333c51e77bfb95f2ba2` |

## STOP wake, quarantine and module/proc return

`fwctrl.c:440–458` holds ctrl->lock while visiting live pending-list waiters, sets
only previously absent receipt_error to-ESHUTDOWN and only completes !done waiters.
The waiter's removal uses the same lock (:379–381); no dead stack receipt is used.
Validated signed negative status remains latched before response completion
(:249–254); the immutable global first cause wins in do_cmd (:125–142). STOP does
not replace a completed response skb or copy negative payload, and init failure
cannot turn into success. P3's independent request lifetime is still unimplemented.

`hgic_usb_quarantine:74–82` is a nonreturning uncompleted wait, not leak-and-return.
It retains allocations and the disposal caller. Normal module text retention also
requires the **actual caller return route**, which was checked:

1. For a bound device, USB unbind calls driver disconnect before returning
   (`K/drivers/usb/core/driver.c:431–461`). Base device release calls device_remove
   **before** unlinking the device from the driver's list (`drivers/base/dd.c:
   1318–1379`). `driver_detach:1415–1442` and `bus_remove_driver` (:780–797 in bus.c)
   synchronously wait through that route. Thus a blocked concurrent unbind still
   prevents ordinary driver/module exit from completing; a dangling allocation is
   not being used as a substitute for this rendezvous.
2. A probe has not yet reached driver_bound/list insertion (`dd.c:718–768,458–468`).
   Normal automatic probe holds the bus-driver klist iterator across the callback
   (`dd.c:1034–1041,1110–1111`; `bus.c:448–465`); bus removal's klist_remove waits
   that iterator (`lib/klist.c:234–259,304–318,368–407`). During initial module load,
   delete_module also refuses a non-LIVE module (`kernel/module/main.c:810–815`).
   Manual bind is synchronous in its sysfs store (`bus.c:258–275`); removal first
   removes bind/unbind files (:651–655,788–794), whose sysfs/kernfs rundown waits
   active callbacks (`drivers/base/driver.c:197–201`, `fs/sysfs/file.c:511–516`,
   `fs/kernfs/dir.c:489–525,1491–1511`). These are actual normal entrypoint return
   protections, not a claim that arbitrary new callers inherit a module ref.
3. Selected module exit calls USB deregistration before notifier/proc removal
   (`S/hgic_smac/attach_init.c:686–692`; `owner_usb.c:511–514`). Actual USB deregister
   calls driver_unregister synchronously (`driver.c:1115–1123`). Kernel ordinary
   delete_module calls mod->exit at:834–835 and only later free_module at:847.
   Blocked disposal/quarantine therefore prevents normal module-text destruction.
4. After USB retirement, notifier unregister is RTNL synchronized (`net/core/dev.c:
   2016–2037,2228–2244`). Status remains module-owned scalars, not core-backed seq
   private data. proc_remove reaches proc_entry_rundown (`fs/proc/generic.c:
   793,813–816`); that waits active module callbacks/open releases (`fs/proc/inode.c:
   255–273`) before normal exit may return.

Forced unload is excluded and not authorized. USB unbind can terminate URBs before
entering disconnect; genuine callback errors then remain sticky rather than being
rewritten as successful retirement. No new direct internal deadlock was found, but
quarantine intentionally never returns, and poison/work/RTNL waits have **no finite
completion proof**. The inherited1000-jiffy TX semaphore wait is still a separate
budget (10s at target HZ100), not the500ms command ceiling or a5s recovery promise.
Blocking/uncertain retirement is not permission for forced free, feed, retry or a
larger guard. This review does not certify successful normal unload.

## Compile metadata, reader closeout and next boundary

Author final **syntax02:4/4 returncode0** metadata and all diagnostics were verified
under the103-file seal. The72-file input map exactly matches this source. Actual
commands use Clang23.1.3, HOST x86 installed Linux6.18.55-2-cachyos-lts headers,
`-fsyntax-only` and implicit-function/incompatible-pointer errors. They are cited,
**not rerun**; external compiler/header installation hashes were not freshly
validated. The preserved syntax01 missing-include failure remains in custody.
Neither syntax run establishes objects, link/module/ARM ABI or execution behavior.

The static reader first stopped on a reviewer metadata-key mismatch: this author's
receipt uses `host_kernel_inputs`, not older P2's `kernel_inputs`. Its partial diff
and excerpts are preserved byte-exact; `READER_ATTEMPT01.md` records the failure and
initial reader pin. The reader-only correction revalidated those outputs and
completed public integrity collection, with no author modification/compiler run or
policy rejection. Finalizer revalidates all reader inputs and exclusively seals
this report/evidence; original candidates/reviews and both hardware HOLDs stay.

**ONE next ordinary source task:** parent may explicitly resume the isolated P3
one-request ownership/deadline **design** using this exact join-before-poison/free
contract. It must still define waiter/pending/queue/worker/submitting/callback
request refs, owned-UNSENT cancellation and atomic STOP/error/deadline submit
admission, preserving actual callback-versus-local-failure distinction. Any new
producer must participate in owner_stop/join before final free; do not treat the
current sticky reservation flags as a generic acquire/release API. No P3 code,
test, integration, build or target operation is queued by this recommendation.

P1, P3 implementation, full P4 descriptors/accounting/tombstone/final summary, P5,
ordinary functional/IRQ/HCD/native, paired ARM/module/image, RF-silence and product
acceptance all remain unmet. No payload/RX grammar review or old blocked-work
acceptance was substituted for this narrowly selected owner prerequisite.
