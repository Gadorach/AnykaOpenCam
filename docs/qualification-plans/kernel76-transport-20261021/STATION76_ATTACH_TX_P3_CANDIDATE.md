# Station76 P3 — owned-control/deadline SOURCE handback: BLOCKED

2026-10-22. RP-02 / NET-01–08 / K76-06, P3 / S2 section5 only.

## Outcome

**Stopped before speculative production implementation on the selected lifetime-
separation condition.** Delivered a pinned71-file P2 v2 baseline copy and ordinary
source/API design analysis. **P3 is NOT implemented, compiled or accepted.** This
is not an integrated attach, source-sufficiency, native or product GO, and is not
permission to build/stage/run the copied baseline.

Owned only new `apps/qualification/kernel76-station-attach-tx/`, this report and
`station-attach-tx-p3-source01/`. All71 source files are byte-identical to frozen
P2 v2; zero production edits/additions/deletions. LICENSE remains
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
All notices/provenance and accepted A2b `rx.c`/`event.c` remain unchanged.

Entry revalidation verified **259 frozen v1/v2/review custody files** and **204
accepted A1/A2a/A2b source pins**. Final sealing revalidates the same inputs.
V2 seal remains
`40928114e2de10bc84b0dc26301db2563f1d9a1df4273b6610a7cc7ba13317bb`;
its focused ordinary review accepts R1–R3 at static SOURCE/API scope only. That
review does not establish P3 ownership, P4 retirement or native behavior. No P1
source/partial review, review retry/adoption/completion or safeguard/model/provider
change was used; P1 remains intentionally absent/unmet.

Relative-path source-map SHA256 is unchanged from P2 v2:

```text
a23ec7ce1e9794762e086853655e286b3567db5ddb4e449c88bab25390b9c75f
```

Format: SHA256 of compact sorted-key UTF-8 JSON source file-size/hash map.
`copy-baseline.json` retains all baseline/custody/accepted pins. `source-contracts.json`
retains exact full-file pins and line excerpts/range hashes used below.
`source-inventory.json` explicitly records zero changed source and no compiler/test
execution. `candidate-seal.json` freezes this handback except itself.

## Inputs read before design

The selected section5, actual modified P2 v2 FWCTRL/request declarations, command/
response paths, USB TX/callback/quiesce/free/disconnect and selected init/work/remove
paths were read. No former P1 work was consulted.

Abbreviations:

- **S/** = frozen `apps/qualification/kernel76-station-attach-init-v2/source/`.
- **K/** = retained public
  `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.

| Actual input | Contract relevant to this stop |
|---|---|
| Plan section5:225–279 | Heap-owned request, owned-UNSENT cancellation, immutable deadlines and real submit/callback ownership are required; no stack pointer shortcut. |
| S/`utils/fwctrl.c:84–148,313–370` | Caller-stack response, separate pending-list/queue lifetimes, old queue eviction,500ms wait and no owned request cancellation. |
| S/`utils/fwctrl.c:224–255` | A1 exact association/copy contract and P2 v2 negative status chronology to preserve. |
| S/`utils/fwctrl.h:10–47`, `hgic_smac/hgics.h:61–109` | FWCTRL is embedded in the core allocation; pending receipt/queues do not own that core or USB bus. |
| S/`hgic_def.h:170–193` | `tx_packet(bus, skb)` and boolean completion consume skb without a request/owner-lease retirement contract. |
| S/`utils/if_usb.c:29–48,157–181,268–381` | USB private buffer/URB/bus lifetimes, callback list transitions, token handling and actual submit. |
| S/`utils/if_usb.c:74–121,485–518` | STOP/drain/free and core removal ordering do not account for preparing submitters or all active callbacks. |
| S/`hgic_smac/attach_init.c:268–316,426–467` | Worker ownership is joined during core removal, but core memory may be released before a later transport drain; TX callback still uses core private data. |
| K/`drivers/usb/core/urb.c:232–259,593–624,677–721` | Async completion, caller synchronization duty, exact kill/idle guarantee and its blocking wait. |
| K/`drivers/usb/core/hcd.c:1519–1569,1634–1672` | HCD returns error OR giveback, not both; USB use_count is reserved before enqueue and decremented after driver completion returns. |

These are ordinary static source deductions, not induced conditions, executed
fault/race observations or proof of a schedulable failure on the particular target.

## Concrete blocking source contract

### 1. A request ref does not retain its USB/core owners

The bus interface exposes only skb submission plus a boolean completion. It has
no serialized acquire/release contract retaining `hgic_usb`, `hgic_usb_buf`, the
URB/backing and `hgics_wdev` while preparation/submission/callbacks are live.
The FWCTRL object/locks/pending list live inside the core allocation, not the
proposed heap request. USB completion still dereferences `usbdev->bus.bus_priv`
and then the core's status/bus (`if_usb.c:270–278`, `attach_init.c:268–279`).

Giving a new request a kref/refcount would protect that request allocation only.
It would not change existing owner frees, pending-list destruction, callback code
or backing ownership. A URB reference likewise retains the URB, not arbitrary
driver-private core/USB objects. A module/device reference is not, by itself, a
proved driver-private owner-retirement contract. None is inferred as a substitute.

### 2. List emptiness is not callback retirement

The exact TX callback order is:

```text
if_usb.c:275   core tx_complete(bus_priv, skb, success)
if_usb.c:276   buf->data = NULL
if_usb.c:277   move buf from used to tx_freeq (helper takes/releases qlock)
if_usb.c:278   up(&usbdev->tx_sema)
```

`hgic_usb_quiesce` selects/kills only entries still on `used`, and returns when
that list is empty (:83–95). `hgic_usb_free` then frees the free-list buffers and
ultimately the caller frees `usbdev` (:97–121,500–517). No active-callback owner is
recorded after the callback removes its buffer from `used` but before its final
USB-owner access. A source-level overlapping execution can therefore observe an
empty list without a returned callback. Joining the TX work does not join this
USB completion. No such execution was run or claimed observed here.

Kernel `usb_kill_urb` does guarantee callback completion for an URB it actually
drains. This does **not** prove that selecting only the current `used` entries has
identified every active callback. Kernel giveback explicitly calls the driver's
completion before dropping USB `use_count`; the driver's list transition is not
the corresponding retirement event.

### 3. Preparing/submitting ownership is absent from STOP/drain

The last current STOP/error check precedes buffer dequeue/fill/actual submit
(`if_usb.c:348–363`). STOP/quiesce does not share a request submit-claim lock or
track submitters between that check and entry into USB core. `used` is itself a
driver list membership, not an accepted-submission count. `usb_kill_urb`'s idle
guarantee is not a permanent ban on a later submission after kill returns.

Disconnect drains before core remove; core remove later cancels/joins the worker
and frees the embedded FWCTRL/core; another USB drain occurs only afterward.
A worker that was already preparing can make a submission after an earlier drain
without an owner-retirement contract preserving the callback's core. Marking the
new request SUBMITTING alone would honestly label it potentially submitted but
would not make these owner frees safe.

### 4. Local failure callback is not HCD completion

The existing local USB failure path invokes `bus.tx_complete(..., skb, 0)` even
when USB core never accepted a submission (:372–380). Actual HCD submit failure
has no giveback; successful submit has exactly one. A new request cannot reuse
this boolean notification as its proof of an actual callback/URB retirement, or
drop a separately reserved USB ref/token twice on an immediate completion.

### Why this stops the selected packet

The proposed request states/refs below cannot establish safe callback backing/
owner lifetime using the current free and STOP contracts. A valid integration
needs a jointly reviewed **owner lease + no-new-submit fence + submitter/callback
retirement rendezvous before owner frees**. That changes the shared RX/TX drain/
core release ordering identified in section6/P4, not merely a request pointer in
the USB completion callback. RX and TX share the USB owner, `used` list and free
path, so changing only the one command's heap ownership does not close it.

I did not establish a safe P3-only separation within the selected minimal-hook
boundary. This is not a claim that a coupled source redesign is impossible; it
identifies the contract that must first be explicitly resolved/reviewed. Per the
task's stop clause, no speculative owner refs, reordered frees, forced cleanup,
global guardian, broad tracing or partial P4 implementation was inserted.

## Proposed request contract — DESIGN ONLY, NOT CODE

The smallest intended object is one bounded heap request, never a pointer to a
caller-stack waiter. It would own its completion, local generation/identity,
immutable monotonic deadline, strict existing A1 correlation metadata, first
response/result and precisely identified skb ownership. No wire token/opcode or
larger data buffer is needed. One request/capacity only; no unrelated-skb eviction.

| State | Required meaning / ownership, not currently implemented |
|---|---|
| QUEUED_UNSENT | Queue holds a ref and owns the command skb; waiter has its own ref. Pending association has an independently reconciled ref. |
| WORKER_PREPARING | Worker atomically claims queue ownership/ref under the same gate as cancellation/error/STOP/deadline; preparation is not submission. |
| SUBMITTING | Worker wins the serialized submit claim; independent submitter and callback reservations exist **before** USB entry. Failure after this claim is potentially submitted, not UNSENT. |
| SUBMITTED | USB accepted; callback may already have occurred before submit returned. Never overwrite an already-retired state on return. |
| RETIRED | Actual callback/local submit-failure owners have separately reconciled their refs/tokens/backing. Control reply completion alone cannot publish this. |
| CANCELLED_UNSENT | Only this still-owned unsent entry was detached under the ownership gate and its locally owned skb/queue refs retired exactly once. Worker/transport-owned backing is not freed here. |

Independent roles must include waiter, pending association, queue/worker,
submit-in-progress and real USB callback. The unresolved **owner leases** above
are additional to these request refs. One refcount total is not a receipt showing
which of those owners remain. Actual callback-before-submit-return and local
submit-failure/no-callback need disjoint owners, not post-callback increments.

Deadline intent: seal one absolute monotonic command deadline before admission,
no later than existing500ms ceiling, and use only its remaining budget at queue,
claim, token, submit and wait. Init observation stays<=2s under later P5's5s guard;
no timeout is restarted or increased. Replace the inherited1000-jiffy token wait
with one nonblocking admission and a sticky exhausted-capacity failure. No retry,
credit query, sleep/poll loop, automatic recovery or deadline-repairing late success.
These changes were **not implemented** on the unsafe owner contract.

An error/STOP/deadline serialized before submit claim must suppress future claim
and detach only locally owned UNSENT work. If submit claim won, retain owned
backing and report potentially submitted until real retirement. No physical
atomic TX-cancellation guarantee can follow from a software check before USB entry.
P2's immutable cause, R3's validated negative-status-before-wakeup chronology,
first-response wins and the accepted A1 no-error-payload-copy contract must remain.

Existing RTNL acquisition/hardware registration can block; merely adding cached
deadline checks cannot force their timely return. P5 supervision/guard and a
reviewed unknown/stuck branch remain necessary, not an excuse to free uncertain
owners or extend budgets.

## Execution and compile evidence: deliberately none for P3 code

Executed only new public copy/hash/excerpt/seal bookkeeping:

```sh
python3 -B apps/qualification/kernel76-station-attach-tx/prepare_source.py
python3 -B apps/qualification/kernel76-station-attach-tx/freeze_candidate.py
```

No new production source was written, so **no compiler or unchanged syntax suite
was rerun**. Existing P2 v2 three-unit syntax metadata remains frozen provenance,
not P3 compile/ownership evidence. No runtime/unit/functional/fault, adversarial,
penetration, fuzz or mutation tests ran. No app/test/native executable, Kbuild,
object/link/ARM/module build, staging, UART/RPC/status/network/private IO, hardware,
Git/index/central-plan/AGENTS or frozen boot/helper edit occurred. No parent native
operation was polled and no target state or original HOLD was changed/inferred.

## Required handback / incomplete gates

P3 remains **BLOCKED / not implemented**, including actual heap request refs,
states, owned cancellation, monotonic budgets, nonblocking tokens and reservation/
retirement counters. The copied stack response/queue design and1000-jiffy wait
remain source facts, not accepted solutions. No missing coverage is marked passed.

Required ordinary functional confirmation, only after a later explicitly selected
source contract/implementation/review: one successful owned FWINFO, normal token/
submission/completion accounting, timely command expiry and routine shutdown/
cancellation, first-error/negative-status/no-output behavior, late reply/completion
retaining failure, and truthful potentially-submitted versus UNSENT classification.
No test campaign or P4/P5 packet is queued by this list.

P1 remains unmet; P4 full descriptors/allocation/RX-start/callback/owner/drain/proc/
tombstone and P5 guarded controller remain separate and unselected here. No paired
build/native/network/media/memory/product or physical RF-silence gate advances.
Source/image/module/helper pins,128kB cap, budgets and original failed evidence
remain intact.

**ONE next source decision for parent:** resolve/review the bounded shared owner-
lease and STOP/submitter/callback-retirement contract needed to separate P3 from
P4, or explicitly revise packet scope. This is a technical handback, not an
automatic P4/P5 queue, P1 retry, build/adoption or native authorization.
