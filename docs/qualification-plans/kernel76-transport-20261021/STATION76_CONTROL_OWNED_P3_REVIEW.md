# Station76 owned-control P3 — ordinary SOURCE/API review

2026-10-22. RP-02 / NET-01–08 / K76-06. Explicit parent-selected isolated
P3 on the narrowly reviewed P4a shared-owner contract; not a composite attach.

## Disposition

**Narrow SOURCE sufficiency for this exact one-FWINFO owned-control candidate.**
No concrete correction-required source/API defect was identified in its actual
eight-file delta and selected ordered-producer/owner-USB paths. No patch is
requested against these frozen inputs. This is ordinary static review, **not**
functional, concurrency, finite-retirement, build-pairing or native acceptance.

The conclusion requires the exact P4a topology and return route: one init caller,
one ordered TX producer, existing FWCTRL work, permanent STOP, producer joins and
all fixed-slot poison **before** request/core/USB free. It does not turn request
refs or callback reconciliation into independent retention of USB/core/module text.
No new producer, broader transport or general lifetime API inherits this finding.

**P1 remains absent/unmet and intentionally untouched.** No P1 source, partial
review or blocked-work substitute was inspected, retried, reviewed or adopted.
Full P4/P5, ordinary functional/SMP/IRQ/HCD execution, paired ARM/module/image,
native and product gates remain open. No automatic implementation, test, build,
integration or hardware queue follows.

Only this report and `station-control-owned-p3-review01/` were written. No compiler,
generator, candidate/helper/test/app execution, HOST shims/races, security-test
expansion, hardware/UART/RPC/status/network/private IO, original source/helper,
AGENTS, central-plan, Git/index, model/provider or safeguard change occurred.
There was no task/policy rejection. Parent's last hardware observation remains
held original U-Boot, with both radio and reset-failure HOLDs preserved; no current
Linux state was inferred or queried.

## Exact inputs, provenance and integrity

`S/` = `apps/qualification/kernel76-station-control-owned/source/`.
`B/` = `apps/qualification/kernel76-station-owner-lifetime/source/`.
`K/` = retained public kernel source at
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.

- Supplied **132-file** author seal verified:
  `5d8a96a78e1b6d99f7a16f259d36f0e077644f7e7378c8ecd88b58109add66eb`.
  The exact candidate/evidence/report file set matched, including the separate
  author report, not merely the listed hashes.
- Exact **74-file** source map SHA256:
  `f9865e63c55b09bdd36d6b53ae61acf8dfd4f3b340de5d3409e1a7c31204722e`.
  Format: compact sorted-key UTF-8 JSON relative source file-size/hash map.
  All72 baseline files match sealed P4a before changes: **66 unchanged, six
  changed imports and two new files**, with no hidden source deletion/addition.
- Independently reconstructed universal-newline diff exactly matches
  `station-control-owned-p3-source01/OWNED_CONTROL_DELTA.patch`.
  Raw full-file pins separately preserve original line endings.
- All **451 prior custody entries** matched the independently reconstructed exact
  union of seven prior candidate/review seals, including those seals themselves.
  All **204 accepted A1/A2a/A2b source pins** matched their accepted seals. This
  excludes P1 and supplies no missing P1 review or composite acceptance.
- P4a review seal remains
  `c677fe73b1e28691365e31383780f12b89b797bafe6baa075f6d7ae366bdc4ff`.
  The four author-preserved blocks and five additional producer/return ranges
  compare byte-exact with B/. The stop block includes final core removal; other
  ranges include all-slot disposal, RX callback/resubmit, slot reservation,
  init enqueue, core probe/publication, probe/disconnect and module-exit route.
- LICENSE remains
  `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
  Existing upstream notices, provenance and new GPL-2.0-only notices are retained.
  No source-license rewrite, global grant or publication authorization follows.
  A1 helper/wire declarations, A2b RX/event, original core/procfs, legacy if_usb,
  hgic_def and schema2 declarations remain unchanged from P4a.

| Changed/new S/path | SHA256 |
|---|---|
| `hgic_smac/Makefile` | `b7bfa29536b261738bb3518b76e63810bdbf2bf14cca3a45dfba5033bd1e3699` |
| `hgic_smac/attach_init.c` | `007546ab2e6bbefa47d93aaa9429d1ff7c865889c95e3cebc84207ed2a599d80` |
| `hgic_smac/attach_status.c` | `d955438a23dd19e5be64ccdd0135d67d103d8563433701ae0ffecfdf09d1bd25` |
| `utils/fwctrl.c` | `ce151a3fcfad957cd34005327d0bdda4e1ebc7b769144f5908aac24a9ebd154f` |
| `utils/fwctrl.h` | `0ba14d8a22adaff708b20f01927c1eeedebec74013802a1b6ac3f2e870686ffc` |
| `utils/owner_usb.c` | `36f74ce8e77f3deb147345066836e2f783ef0de5fbc1ef3d6959bc46366998aa` |
| `utils/owned_control.c` | `62576f438186b0a731e22df37106299123202351a7cf7ec25edce9c6c08b2aa7` |
| `utils/owned_control.h` | `7c70d2025f68a70e507b872245176b31aacc64a7c29669d944ebfb09a062ff61` |

Makefile:6,15–16,22–24 selects attach_init, fwctrl, owned_control and **owner_usb**.
Unselected legacy if_usb behavior is not used to claim a selected-path defect or
closure. `attach_status.c:14` changes only the fixed policy label; it does not add
transport accounting, a request progress receipt, full-P4 tombstone or final summary.

## Request, queue and ref conservation

`owned_control.h:19–35` embeds the completion/list/response in one heap allocation,
bounded statically to1024B; skb cb stores a fitting copied heap-request pointer.
No async pointer names the init caller's24B output, a stack response or completion.
`owned_control.c:90–133` constructs WAITER privately, then adds REGISTRY before
publication under owner→receipt locks. Allocation/admission failure frees only
the never-published constructor allocation.

| Role | Actual selected acquire / release |
|---|---|
| WAITER | Constructor:105–106; do_cmd's final waiter_done at fwctrl.c:156, owned_control.c:381–387. |
| REGISTRY | create:123–125; only final owner_release:398–432 after actual P4a return proofs. |
| PENDING | publish:162–163; exact list removal/ref release:372–378, under ctrl lock. RX borrows only while holding that same lock. |
| QUEUE | publish:164–166; exact UNSENT cancel:52–60, or atomic transfer to WORKER:184–187. |
| WORKER | take:184–187 transfers the queue ref, not an extra inc/dec; worker_done:217–224 follows abort or transport return. |
| SUBMITTER | claim:254 before USB; submit_result:281 only on actual submit return. |
| CALLBACK | claim:255 before USB; actual callback:307–320, or distinct no-callback submit-error reconciliation:269–274. |

Normal peak is **six refs**: QUEUE/WORKER are mutually exclusive. After private
construction all ref/role changes are receipt-lock serialized. The registry never
drops on waiter return, response, timeout, local failure or callback. Intermediate
role_put cannot free the allocation. Duplicate/missing-role/zero-ref observations
mark inconsistency; gates reject it and final release retains owners rather than
guessing free. No selected path has an unbounded ref-acquisition loop or numeric
increment capable of reaching refcount saturation. Kernel refcount WARN/leak
semantics are not themselves substituted for this bounded role/return argument.

`fwctrl.c:325–375` publishes exactly this request/receipt, not a generic capacity
queue; no old/unrelated skb is evicted and no capacity retry exists. After
publication the waiter never touches the command skb. `attach_init.c:312–327`
claims at most one request, aborts its own preparing command or hands it to
transport, then drops WORKER. No timer, thread, worker or cancellation producer
was added. STOP/error cannot free a worker-owned or USB-owned command.

## Actual cancellation/claim lock and first-error chronology

`attach_init.c:35–73` retains the real immutable first-error recorder. Its hook
executes **inside that existing receipt lock**. `owned_control.c:47–70` sets only
the first request error, detaches/frees only QUEUED_UNSENT, drops QUEUE once and
notifies; it does not take owner/ctrl locks, touch worker/URB backing, or reenter
the first-error recorder. The selected command is a private dev_alloc_skb object,
with no installed driver destructor; IRQ-disabled free uses the kernel's deferred
skb-free path, not a synchronous driver callback under the receipt lock.

The nesting where required is **owner→ctrl→receipt**, with permitted subsets.
`publish` requires owner/ctrl; `submit_claim:242–263` requires owner and acquires
the same receipt lock. `gate_locked:72–88` checks actual first cause, request error,
registry/generation, consistency and deadline. CLAIM additionally checks permanent
STOP, exact preparing phase/command and WORKER before reserving submitting/callback
refs, token ownership and potentially-submitted history. No receipt-held selected
path acquires either earlier lock. Completion signaling does not synchronously
run the waiter. No identified inverse/reentrant cycle exists in these paths.

If error/expiry wins QUEUED, only its own UNSENT entry is removed; later take has
nothing to submit. If WORKER already won, backing stays WORKER-owned and the
preparation/final gate can fail it locally. If CLAIM wins first, later error does
**not** relabel it UNSENT, free it, or erase potentially-submitted history. There
remains a claim-to-USB scheduling interval: this is software ownership ordering,
not physical atomic cancellation or proof HCD entry occurs before the deadline.

No callback-used lock spans usb_submit_urb, token admission, completion waits,
producer joins, poison or quarantine. `fwctrl.c:261–266` still observes a validated
associated signed negative firmware status while holding ctrl lock **before**
response publication/wakeup. Although the error hook may signal immediately, the
waiter must reacquire ctrl before pending removal, so it cannot overtake that
protected response publication. First error and first response are not overwritten.

## One deadline and actual kernel API semantics

`fwctrl.c:524–547` computes/seals one absolute CLOCK_MONOTONIC deadline before the
command grant and request/skb allocations. Checked u64 addition rejects overflow;
an existing deadline cannot be renewed. HGIC_CMD_TIMEOUT remains500ms. Create,
publish, take, prepare, claim, submit return, reply observation, wait-budget and
final copy admission check that same value; there is no restarted per-stage budget.

`owner_usb.c:292–298` makes **one down_trylock**, correctly mapping kernel success0
to success and failure1 to sticky-EAGAIN. The selected1000-jiffy token wait is gone;
there is no credit poll/sleep/retry. `owned_control.c:356–369` converts remaining
nanoseconds to **whole jiffies**, failing conservatively when an unnotified request
has less than one tick. At retained target HZ100, the remaining500ms budget is at
most50 ticks, safely representable in unsigned long and completion's signed long.
Already-notified success/error needs no new wait. A fresh locked gate before the
24B copy rejects late scheduling; completion return alone is not success evidence.

Actual K/ contracts read and pinned, rather than inferred from comments in S/:

- `include/linux/timekeeping.h:24–36,168–176`: monotonic reference and ktime_get_ns.
- `kernel/time/time.c:781–812`: nsecs_to_jiffies64 floors at this HZ; its generic
  unbounded-input caveat does not overflow this <=500ms selected input.
- `kernel/locking/semaphore.c:158–184,213–250`: nonblocking inverted convention
  and token return; caller does not hold the semaphore's internal lock afterward.
- `kernel/sched/completion.c:21–54,85–174`: serialized completion count/wakeup,
  uninterruptible timeout, zero on timeout/positive on completion. One sole waiter
  and no completion reinit/reuse avoid manufacturing a new successful generation.
- `include/linux/refcount.h:364–450`: inc assumes stable existing ownership;
  dec-and-test has release/acquire free ordering and warns/saturates on misuse.
  Here stable registry/role ownership supplies the prerequisite, not the counter alone.

This is **not a hard wall-clock return bound**. Allocation, preemption, USB entry,
RTNL, producer joins, poison and inconsistency quarantine can block or resume late.
No2s init or5s native/recovery completion proof is claimed and no budget/cap grows.

## Callback-before-submit-return, local failure and A1 copy

`owner_usb.c:299–318` reserves slot and both request roles before USB entry, then
submits outside locks. Actual callback:225–242 captures request/status **before**
core skb free, clears/requeues the stable slot and returns the token, then reconciles
CALLBACK. Callback may finish before usb_submit_urb returns: SUBMITTER independently
keeps the heap request live. Success return:319–326 touches only that request via
submit_result; no skb/backing/reused slot is accessed. Callback history is preserved.

Submit failure has **no callback for that attempt**. submit_result:269–274 drops
the reservation locally; owner_usb.c:328–345 alone clears/requeues acquired slot,
returns its one token and consumes the skb. Its boolean core failure notification
is not counted as actual HCD completion. Before-claim failures clear only preparing
command ownership. Real callback and local submit-error cleanup are disjoint under
the kernel contract; a contradiction marks the request inconsistent.

`K/drivers/usb/core/hcd.c:1519–1569` explicitly requires error **or** giveback, not
both, and reserves use_count before enqueue. Its giveback:1634–1676 calls driver
completion before decrementing use_count. No post-success transfer-buffer access
in the selected driver is justified by assuming deferred completion.

`fwctrl.c:239–267` retains exact pending cookie association and A1 type/interface/
opcode/status/retail validation. It additionally requires a won submit claim;
a still-UNSENT response cannot create success. `owned_control.c:14–20,265–283,
347–354` requires **reply_ready AND accepted successful submit return** for positive
notification. Thus a positive response while submission is still returning cannot
turn a later local submit failure into successful output. Negative/error cases
notify failure independently. Late accepted/callback history never clears r->error
or the global terminal failure. `fwctrl.c:129–143` repeats the fresh locked gate
before the unchanged A1 bounded copy; it clears the response owner before freeing
the skb. Negative payloads are never copied. Async response storage is heap-owned;
only the synchronous caller copies/decode-writes its own output.

## Registry free requires actual shared-owner and module return

Unchanged `attach_init.c:456–515` permanently closes admission, marks removal,
wakes controls and joins init/TX/FWCTRL/disabled work and timers before final core
free. Selected enqueues remain STOP-gated, as required by actual
`K/kernel/workqueue.c:4463–4485` no-racing-enqueue qualification. Unchanged
`owner_usb.c:348–411` then poisons **all16TX+32RX indexed slots**, regardless of
free/used-list membership, before final bus.remove/FWCTRL release. Slot reservations
remain conservative sticky flags, not P3 refs, active counts or drain receipts.

`K/drivers/usb/core/urb.c:724–770` promises callback return before poison return,
increments permanent reject even for never-used slots and waits use_count0 for
used ones. Actual HCD link rejects poisoned URBs; retained MUSB enqueue uses it.
The callback's request reconciliation therefore is **not callback-return proof**;
the full poison sweep and complete TX worker join supply the missing tails.

Only then does `fwctrl.c:452–468` call owner_release. Its
`owned_control.c:398–432` requires sole REGISTRY ref/role, correct active identity,
no command/response/token/pending owner, no discrepancy, and actual callback
observation for an accepted submission. Otherwise its never-completed wait retains
the request **and the containing disposal caller/owners**, rather than leak-and-return.
There is no newly introduced async producer outside P4a's joins/poison contract.

Actual module-text protection remains the checked normal caller chain, not a heap
reference: USB disconnect returns through device-locked removal before driver-list
unlink (`K/drivers/usb/core/driver.c:431–461`, `drivers/base/dd.c:1318–1379,
1415–1442`); USB deregistration synchronously unregisters. Probe rollback is within
the serialized probe caller, protected through driver-iterator/sysfs return routes
revalidated in the P4a kernel excerpts. `S/hgic_smac/attach_init.c:700–708`
module exit calls USB deregistration first;
`K/kernel/module/main.c:810–847` rejects non-LIVE removal and frees module only
after exit returns. Quarantine therefore prevents ordinary exit/text destruction.
Forced unload is excluded, unsupported and not authorized. These proofs do not
establish successful unload or any finite join/poison/quarantine duration.

## Evidence closeout and missing gates

Fresh evidence contains the exact actual diff, source/provenance verification,
90 pinned reader excerpts, reader input receipt, additional actual API/config
excerpts, input seal and final review/output seal. Seven P3 API excerpts and all54
previous P4a reader excerpts were checked against their actual full-file bytes.
Reader-only enumeration/schema mistakes are disclosed in `READER_ATTEMPT01.md`;
no author file or behavioral result was changed to resolve them.

Author **syntax04:5/5 returncode0**, exact final source/command/diagnostic pins,
Clang23.1.3/x86 installed Linux6.18.55-2-cachyos-lts headers and -fsyntax-only were
verified/cited, **not rerun**. Installed compiler/header dependencies were not
freshly rehashed. Known installed-header nocf/address-space warnings remain.
The earlier missing-include failure and intermediate runs are retained. Syntax
is not object/link/ARM/module ABI or runtime evidence.

Ordinary confirmation is still unexecuted: successful FWINFO, queue/worker/cancel
and STOP chronology, token exhaustion/local failure, immediate/deferred callback
ownership, reply-before-submit-return, signed-negative chronology, late waiter/
response/callback lifetime and final joins/poison/registry reconciliation. These
are open functional/SMP/IRQ/HCD gates, not observed runtime failures or queued tests.

**One concrete missing dependency for finite guarded execution is P5's owned,
bounded progress/recovery controller.** This candidate deliberately has no such
controller and retains unbounded join/poison/quarantine waits. Static P3 sufficiency
cannot replace it or authorize hardware. P1's independent review remains absent
and intentionally unaddressed; full P4 exact descriptors, complete accounting/
status/tombstone/final-summary and all-context retirement also remain unmet.

Parent may disposition this exact sealed review. No automatic test/build/new
implementation/native action follows. Fixed16/32/2560B backing,128KiB cap,
500ms/5s policies, original image/module and both hardware HOLDs stay intact.
No physical RF-silence, station/UP/network/media/combined-memory, paired-image,
persistent-install, G8/product or other-camera acceptance is established.
