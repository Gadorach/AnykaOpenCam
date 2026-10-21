# Station76 A2b independent exact SOURCE/HOST review

2026-10-21. **GO for the exact EVENT/EVENT2 original-extent admission and real
consumer suppression/denial increment only. No mandatory scoped blocker
demonstrated.** Accepted A2a is the prerequisite, not a new acceptance granted
to an unreviewed ingress tree. Conservative denial of every non-debug event is
intentional qualification policy, **not all-firmware/event compatibility**.

**Full S4, non-event/BT/raw-proc grammars, S1/S2/S3/S5/S6, ARM/module/kernel
build, staging, native execution, attach, association and MTU remain OPEN/NO-GO.**
This review grants no hardware authorization and queues nothing.

## 1. Exact pins, provenance and immutability

Read the complete `STATION76_EVENT_A2B_CANDIDATE.md`, original event design
`STATION76_EVENT_ENTRY_REVIEW.md` §§5–6, accepted
`STATION76_INGRESS_A2A_REVIEW.md`, and A1 independent review. Inspected both full
changed source files, their exact diffs, actual root protocol/control headers,
worker/error/copy path, sole core registration, all six HOST test/shim files,
preparer/runner/sealer and sealed run history. Source references below are relative
to `apps/qualification/kernel76-station-events/source/`.

| Changed source | Bytes | SHA256 |
|---|---:|---|
|`hgic_smac/event.c`|2534|`b00e6ee84c10f676c2ad7d3afe8abb0800d37224b98445d86acdddde774a6345`|
|`hgic_smac/rx.c`|10917|`600c2bccc48fae9ee5642fa320d83541b261bf24ff8d2d492c7871f6b97e2cf0`|

Independently verified candidate-seal SHA256
`63aa6d567f8218b9dcf7c413b9a9bbe56fca742a5ca532729c8a4325ed8bf3cc`,
all606 inventoried files, all693 successful run08 inputs, every output hash and
all unmutated extraction byte/line extents. The exact68 source path sets match
accepted A2a: **66 byte-exact, only event.c/rx.c changed, no additions/removals**.
All whole-file offset0/size/hash rows match copy-baseline and source extents.
Rechecked A1 and A2a candidate seals plus A2a independent review inventory; original
tests/tools/source remain immutable. Copied A1/A2a test bodies and their two base
shims are independently byte-compared to accepted A2a. No old collector or binary
was executed; hashing HOST artifacts is not execution or a module/image check.

LICENSE is unchanged18092 bytes, SHA256
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`;
original notices/paths remain. Review `run01/SOURCE_LICENSE_GPL-2.0.txt` preserves
it verbatim. No firmware/device payload or private-input hash was used. Exact
source/test/extraction/HOST-artifact hashes do not imply redistribution rights for
stock firmware or any workspace-wide license. Both source pins and all successful
review inputs/outputs are checked again at final seal.

## 2. Findings against event design §5

1. **Original event extent is checked before normalization, trim or publication.**
   `rx.c:215,219–243` retains the accepted A2a bounds/prefix machinery and extends
   its precise type predicate to EVENT4/EVENT2=14 beside CMD3/CMD2=13. These four
   require no removed prefix, full12 minimum and declared total==**original** actual.
   RX magic/outer bounds are checked first. Consequently `:258` cannot hide event
   surplus, zero padding, concatenation, truncation or1..4-byte0xff prefixes. No
   TX-padding rule, new wire field, event-value length or metadata ABI is invented.
   Accepted USB status/backing/range gates still precede core ingress; direct
   arbitrary-caller/DMA provenance is not newly proved by rx.c alone.
2. **Real ABI and endian split are explicit and compiled.** `event.c:30–35`
   asserts root `hgic.h` outer8/control12, EVENT/EVENT2 IDs8 and both values10.
   This preserves the unseparately-packed inner EVENT struct's padding at9.
   Actual `control_receipt.h:20–24` reads normalized outer magic/length with native
   unaligned helpers; event.c reads EVENT byte8 or unaligned **LE16 EVENT2** at8.
   Type is checked before union access. ID0x0112 is not18; byte9 is not part of an
   EVENT ID. Header layouts compile against root hgic.h, not the tools' duplicate.
   HOST and selected target are LE; there is no ARM/big-endian codegen result.
3. **Bounds precede consumer field interpretation.** `event.c:44–51` first
   observes prior ctrl/bus failure, otherwise short-circuits null skb/nonlinear
   before the shared validator. That validator rejects null data and lengths
   outside12..65535 before magic/length reads. Only afterward are exact type and
   ID read. Null skb denies without a free. No unchecked payload subtraction,
   structure cast or derived payload pointer remains in the complete consumer.
   Direct consumer65535 is a helper bound; the accepted selected USB backing cap
   remains2560, not a new larger transfer/allocation budget.
4. **Exact debug ID18 disposes without output or payload access.** `:56–61,69–71`
   branches to one owned-skb free. There is no printk (not even bounded `%.*s`),
   NUL scan, string/format evaluation, payload pointer/read/hash/export, event
   queue/up/eviction or RTOS forwarding. No command response lookup/success,
   TX, recovery, scan, key change or init state action is present. The closed
   consumer call set is validation, existing denial and disposal only; inherited
   ingress still copies the received message into its owned linear skb for work,
   which is not a consumer export or command-output copy.
5. **Unknown and other named IDs deny through the existing locked latch.**
   `:59–68` includes FW_INIT_DONE31 and all IDs except18; malformed envelope and
   wrong direct-consumer type also reach `hgic_fwctrl_rx_error`. Actual
   `fwctrl.c:188–195` acquires ctrl->lock before `:173–184` sets receipt_error to
   `-EPROTO`, bus.rx_failed to1 and completes unfinished CMD/CMD2 waiters as errors.
   The consumer then frees its skb once, never attaches it to a response or infers
   success from an event cookie. Already-completed reply backing remains owned by
   its waiter; maintenance is not silently made a station completion. Late denial
   before locked command-copy/wrapper commit prevents output; later denial cannot
   revoke a copy already committed.
6. **Already-failed consumer entry only disposes.** `event.c:44–45` precedes
   skb inspection and does not call the error helper again. A ctrl-only or bus-only
   failure stays as found; no clearing, new error completion, delivery or recovery
   is added. Independent probes retain a pre-existing non-EPROTO ctrl error and
   verify that an unreadable data pointer/oversized/nonlinear/null skb input is
   not interpreted on this branch. These are synthetic pre-read guards, not
   permission to hand the kernel invalid skb objects.
7. **Unused fields remain unused.** Cookie, ifidx/flags and both value bytes are
   not station identity, generation, payload-length or event-success constraints.
   Both EVENT18/EVENT2 LE18 suppress safely across their bits; all other IDs deny.
   No structured payload ABI or arbitrary opcode/event operation is enabled.
   Dropping even legitimate state/scan/exception/init events can limit operation;
   that documented conservative qualification policy is not a defect in this scope.
8. **Actual registration and owning route are retained.** `core.c:1162–1164`
   initializes ctrl then assigns its sole rx_event slot to the real consumer;
   `:1196–1211` initializes the Linux event queue/semaphore and publishes RX/error
   callbacks/bus_priv. Actual `fwctrl_rx:334–350` makes a linear copy; actual
   ordered worker `:254–284` validates before routing EVENT/EVENT2 to that callback.
   Event processing does not substitute for command association. The callback
   remains void. The selected Makefile links event.o; no initializer edit or
   alternate family/container cast was added.

**Preconditions and lifetime limits:** live initialized ctrl/bus, live owning core,
and a valid singly owned skb/object/allocation are required by this internal
callback. Wire validation cannot prove those pointer lifetimes. RX worker checks
precede the consumer, so malformed queued skbs can be rejected by the existing
response/error path without reaching event.c. Error-before-disposal is not global
atomic cancellation: an asynchronous worker may deny **after** USB resubmitted;
already queued TX/work, earlier copies, URBs and backing are not undone. No source
change here makes original plain status operations atomic or proves SMP/IRQ/unload
ordering. Removal, deadlines, watchdog ownership and source-lifetime handoff remain
S1/S2/S3/S5, not hidden prerequisites this narrow event GO pretends to close.

## 3. Independent behavioral HOST evidence against design §6

Fresh review-owned command, **08:49:18–08:50:03UTC**:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-event-a2b-review01/review_a2b.py --run run01
```

The inspected review script **does not import/execute/copy the author runner** or
write author/parent output directories. It independently extracts byte-preserving,
uniquely bounded real source, includes actual headers and inspected synthetic
tests/shims, and proves the four baseline C units and ten advertised mutant units
byte-exact to author run08 **before** fresh compilation. Only new review-owned
executables are executed. Parent run09 is not reused or overwritten.

| Fresh executable result | Optimized `-O2` | ASan+UBSan `-O1` |
|---|---:|---:|
|Actual event consumer + USB/rx/fwctrl/strict-copy suite|12,127,356 assertions PASS|12,127,356 assertions PASS|
|Accepted A2a ingress regression body|2,806,874 PASS|2,806,874 PASS|
|Accepted A1 command regression body|150,705 PASS|150,705 PASS|
|Independent supplement|656 assertions /39 ordered probes PASS|656 assertions /39 ordered probes PASS|
|Original real unsafe consumer, safe hooks|12 payload-format calls /2 nondebug queue publications observed|same observation|

Every positive compile/test exits0 with **empty stderr**, including sanitized
tests. GCC16.2.1, native `x86_64-pc-linux-gnu`, Python3.14.7; each subprocess has
an explicit120s HOST limit. No Makefile/cross compiler/module/kernel build ran.
`run01/result.json` records UTC/argv/cwd/exits,1407 input pins and110 extraction
records; stdout/stderr/generated units and `output-hashes.json` retain outputs.
`provenance.json` retains68 source comparisons, closed-call set and run01–08
author history. Final `review-seal.json` inventories report/evidence and rechecks
the successful-run inputs and outputs, excluding itself.

### Real coverage and instrumentation limits

- The event suite uses the complete real consumer through the real callback slot,
  actual USB completion → rx.c → fwctrl RX/worker; strict A1 association/copy/
  wrapper functions are present. Core initializer binding is independently checked
  in source, not represented as a complete probe/module-init execution. A1's
  inherited event stub and A2a's abort stub in their separate regression bodies
  **are not real event-consumer coverage**.
- EVENT0..255 and EVENT2 LE0..65535 are exhausted, including0x0112/0x1200,255/256/
  65535 and named31. Header-only/debug maximum sizes, unaligned starts0..7,
  declarations and transport actual0..65535, short/null/nonlinear/oversize/bad
  magic/type/backing/status, negative/extreme lengths, surplus/concatenation and
  prefixes exercise direct and actual transport paths. Cookie/value fields cover
  all65536 values with varying byte3/EVENT padding; these are correlated fixture
  patterns, **not an exhaustive Cartesian product** of all header fields.
- Payload cases include synthetic `%n`, `%p`, `%s`, terminal `%`, unterminated,
  empty, embedded NUL/tail and binary bytes. `event_shim.h:23–35` classifies integer
  address ranges of registered live skb payload storage **before any format read**,
  including the header-only one-past pointer. It never dereferences fmt, calls
  strlen, accesses varargs, or forwards to printf/vprintf. Original unsafe calls
  are safely observed, **never actually evaluated as formats**. Output labels/
  counts contain no event bytes, payload digests, device identifiers or pointer
  values. Ordinary test printf uses trusted constant formats only.
- Debug suppression leaves seeded event queue/semaphore/pending command/canaries
  unchanged and frees the owned skb. Unsupported IDs error-complete rather than
  deliver; no event skb becomes a command response. Good debug+genuine CMD can
  succeed; unknown then later debug/CMD cannot clear failure or allow new command
  TX. A good response queued before event denial cannot commit caller bytes.
-64-event direct, immediate, per-callback deferred and fully queued deferred
  sequences keep the seeded queue/semaphore intact and free all owned events.
  These are deterministic worker **orderings**, not threads/races. The author event
  suite reports732,940 fixture invocations and3 modeled command sends; its12M
  assertions are repeated boundary/invariant checks, not12M independent scenarios.
- Queue/up/free/completion hooks are concrete. Export/RTOS hooks abort if reached;
  the complete consumer's independently checked closed call set establishes no
  TX/recovery/scan/key/init entry. Some zero-effect counters are scaffolding, not
  instrumentation of all unchanged driver or external-callback code. No whole-
  driver privacy/security/lifecycle claim follows from their zero values.

### Additional independent probes and mutation sensitivity

Review `supplement.c` adds **39 ordered probes**,656 assertions per mode:

1. Put the actual full12-byte header at the last readable bytes of a HOST mapping,
   with all payload pages `PROT_NONE`, for header-only,13 and65535-byte declared
   envelopes and selected debug/31/extended IDs. Suppression/denial succeeds
   without any payload access. This changes synthetic memory access permissions,
   not production allocation/capacity policy; it is not kernel/DMA evidence.
2. Exact-size0..11-byte consumer allocations plus null data, oversize/UINT_MAX and
   nonlinear invalid-data sentinels verify pre-read refusal. A review-only free
   wrapper asserts the existing error latch is set **before** the event free.
3. Prior ctrl-only non-EPROTO or bus-only failure disposes unreadable/oversize/
   nonlinear skb data and null skb without a new error or completion; flags stay
   unchanged. Fixture resets are explicit HOST setup, not production clear APIs.
4. Simultaneous pending CMD/CMD2, already-completed reply backing and maintenance
   records verify one error completion per unfinished station waiter, preservation
   of completed backing, no maintenance completion and no repeat completion from
   already-failed later events. Existing queue/semaphore sentinels stay intact.
5. Genuine FWINFO then unknown EVENT2 before wrapper return, in immediate and
   deferred worker modes, rejects the **whole fw_info wrapper commit** unchanged.
   Exactly one prior RX resubmit is visible in immediate mode, two in deferred
   mode: rejection cannot retroactively cancel them.

**Ten advertised mutants independently compile exit0 and fail behavioral
assertions exit2.** They restore payload-format/bounded-string output, remove
event original-equality/minimum/type qualification or consumer type, truncate
EVENT2 IDs, restore unknown queue delivery, bypass persistent denial, or grant
event-success completion. Removing ingress minimum/type can still be caught by
forbidden RX resubmission even when the downstream worker rejects the short skb;
downstream rejection alone is not a passing ingress admission policy.

**One additional review mutant compiles exit0 and fails at runtime exit1:** adding
a volatile one-byte payload read to otherwise valid debug disposal. The protected
payload mapping triggers an ASan-reported read fault in the mutated consumer.
This confirms the new no-payload-read probe is sensitive, not just a source grep.
Its deliberate failing diagnostic is preserved as synthetic HOST evidence; no
untrusted format/varargs or target data was executed. **11/11 compiled runtime
mutants caught; no compile failure counted as detection.**

## 4. Preserved failures and remaining source boundary

Author run01–08 results and input snapshots remain untouched and their seal pins
match. History shows run01 unused-variable compile refusal, run02 incorrect
immediate-denial/resubmit expectation, run03/04 inherited constant-log accounting
failures, run05 original consumer's unused-but-set warning, then successful run06–08
with increasing coverage. Only the original safe baseline receives the explicit
unused-but-set warning exception. These are preserved HOST fixture/reproduction
failures, not silently discarded native failures or evidence that source changed
after freeze. This review's first run passes; intentional mutant failures remain.

This increment removes the unsafe event-format/export/default-queue behavior and
adds the selected exact event-envelope policy. It does **not** repair or qualify
raw procfs reader count/copy/semaphore retirement, Bluetooth's separate evt_list/
HCI route, ACK/data/aggregate/soft-credit/test/BOOT/OTA consumers or unknown outer
types. No command allowlist or event payload ABI is expanded. RTOS fallthrough
is absent in the changed consumer, but no RTOS build/external callback ABI is
qualified. Every non-debug event deliberately denies, including useful legitimate
notifications; broader functional compatibility remains unproved.

**Next source gap to select separately, not implement/queue here:** remaining
outer-RX-family admission/consumer policy for full S4 and S2/S3 finite queued-TX/
autonomous recovery admission. Existing detect_work still queries ENTER, can
clear queues/reinit and schedule delay_init/reopen. This event latch does not
globally inhibit those paths or prove async-init success, finite deadlines,
cancel-unsent ownership, guarded teardown or S5 partial-probe/inflight retirement.
The disjoint parent-owned remaining-attach plan can order those boundaries; this
review does not replace it or authorize a broad logging/supervisor rewrite.
Full S4/S1/S2/S3/S5/S6 and build/staging/native attach remain NO-GO. Existing
module/image pins are not builds of this newly reviewed derivative.

## 5. Ownership and hardware handback

Only this new report and `station-event-a2b-review01/` were written. No source,
tests, helper, original/candidate runner/sealer, central document, Git/index,
module or image was modified. No target/UART/RPC/status, network/discovery,
private input, firmware/stock binary, ARM/kernel/module build, staging or tool
daemon execution occurred. All executed C is freshly compiled review-owned
synthetic HOST code. A1/A2a/A2b inputs remain frozen and hash-matching.

Historical parent07:24 host-only observation:21a expired/finalized, no UART owner/
socket/recovery arm; this review did not rediscover or renew it. Current target
power/runtime/watchdog remain **UNKNOWN**, last direct06:43 RAM76 APPLICATION
idle is dated. Original151-ACK native trial remains **FAILED/HOLD unchanged**.
Parent alone requires fresh physical/sole-owner/runtime/recovery gates before
any later hardware operation. Watchdog recovery stays preferred when separately
needed/admitted, not active/reset here. No TX/reset/association/MTU/network/media/
persistent effect is queued; RP-04 combined/adoption/product HOLD is unchanged.
