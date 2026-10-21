# Station76 A2b event-consumer entry — design review ONLY

2026-10-21. **Narrow conditional SOURCE GO for a later parent-selected event
suppression/denial increment, only after exact A2a independent review GO.**
This is a design decision, **not implementation approval or an implemented fix**.
The current event source remains unsafe. No source patch/prototype, behavioral
test, ARM/module/kernel build, staging or target operation was performed here.
**Full S4 and S1/S2/S3/S5/S6 remain OPEN; attach/native execution remain NO-GO.**

## 1. Reader boundary, provenance and conditional dependency

Read `STATION76_CONTROL_A1_REVIEW.md`, `STATION76_ENTRY_REVIEW.md` and
`STATION76_INGRESS_A2A_CANDIDATE.md`, then the actual public source. References
below are relative to `apps/qualification/kernel76-station-ingress/source/`.
That tree is the frozen **A2a CANDIDATE**, not an accepted ingress implementation.
The sealed `apps/qualification/kernel76-station/source/` is the A1 comparison.
Do not transplant this decision onto a different source pin or treat it as a
duplicate review of A2a's seven changes, HOST harness, mutants or concurrency.

Static evidence command, **08:09:13UTC**:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-event-entry-review/audit_public_source.py
```

`station-event-entry-review/reader-pins.json` records UTC, argv/cwd, Python3.14.7,
reader hash and full-file extents/hashes. Both **68-file** source sets match their
existing seals;61 paths are identical, seven differ as declared by A2a. This is
provenance checking only, not safety acceptance of those seven differences.
Final `--seal` rechecks all136 source pins and five public report/seal inputs.

| Event-path input | Candidate SHA256 | Comparison |
|---|---|---|
| `hgic_smac/event.c` | `5e480e6a21c45714386a458f69913e7d420001e7d6fc3c04b4d5e78f41f14cac` | unchanged from A1 |
| `hgic.h` | `136892a40e10d41f848d51693dc5e3911e780f4901f522f2f63dfed49189c22a` | unchanged from A1 |
| `utils/fwctrl.c` | `9a33bb351f36438f897ebccb2da54de285da24bf8a7eebe6ec813dff09a96c83` | frozen A2a candidate |
| `hgic_smac/rx.c` | `c9b145f50952a8bc5dc573b845e3a443b78d2cf509d5e7be294e5c63cb83c772` | frozen A2a candidate |
| `hgic_smac/core.c` | `71a578f033148b2290b2b2b8de5ad540aeb7e4d348043a986615fcad07743452` | frozen A2a candidate |

`public-excerpts.json` retains source-line/byte offsets, exact raw-range hashes
and text; `event-references.json` inventories relevant public symbols across the
selected source set. These are **static audit evidence, not HOST test results**.
`SOURCE_LICENSE_GPL-2.0.txt` is the unchanged18092-byte source LICENSE, SHA256
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
Existing source notices, including HUGE-IC tool attribution, are preserved.
All hashes here identify public source/documents, never firmware event contents,
private inputs, device identifiers or their digests. No redistribution grant for
stock firmware or workspace-wide license is inferred.

## 2. Actual selected route and present defects

1. `hgic_smac/Makefile:1–24` links `event.o`, `rx.o`, `core.o`, `procfs.o` and
   fwctrl/USB in the selected Linux2G/USB module. This is not a fmac event parser.
2. `core.c:1162–1164` initializes ctrl and registers the sole selected
   `ctrl.rx_event = hgics_rx_fw_event`. `:1196–1197` initializes Linux event
   queue/semaphore before `:1204–1211` publishes RX/error callbacks and bus_priv.
   `utils/fwctrl.h:37` has a **void**, skb-consuming event callback, not a status
   reply or a new asynchronous-completion protocol.
3. Candidate USB `if_usb.c:224–263` passes original actual_length to the real
   core RX callback on its admitted path. `rx.c:215,231–240` applies original
   extent equality to **CMD/CMD2 only**. EVENT/EVENT2 still allow a removed0xff
   prefix and still reach `:257`'s declared-length trim. `:285–292` forwards
   those trimmed bytes into `hgic_fwctrl_rx`.
4. `fwctrl.c:334–350` allocates/copies a linear skb of the delivered length.
   Actual ordered worker `:254–284` calls `hgic_ctrl_frame_valid` before its type
   switch; only EVENT/EVENT2 go to rx_event. The shared check requires full12,
   RX magic and declared==delivered. It **cannot restore bytes already trimmed
   from original USB actual**. Current A1/A2a event stubs do not run event.c.
5. `event.c:39–42` itself has no null/minimum/type check before header access and
   unsigned payload subtraction. Its normal worker caller supplies a bounded
   header, but that does not make the consumer safe: FWDBG_INFO18 executes
   **`printk(data)` at:48**, with no bounded string/NUL/constant-format admission.
   A full12-byte header with zero payload passes the worker and supplies a
   one-past-end format pointer. Arbitrary `%` sequences become the format; absence
   of NUL can read beyond the copied payload. This review does not invoke it.
6. Linux non-debug events, **including unknown IDs**, enter the default branch
   and then `evt_list`/`evt_sema` (`:58–64`). No event-specific payload validation
   occurs. The `>16` test allows17 queued entries and evicts an existing skb on
   subsequent arrivals; it is not strict16-entry admission. Repeated up() calls
   are not reconciled with evictions. These are queue/reader lifecycle concerns,
   not a reason to keep unknown events for this finite station qualification.

## 3. Exact source-supported header; no invented payload ABI

The authoritative header is **root `hgic.h`**, not the tools' duplicate or a
guessed byte-packed C structure. `:112–118,135–159,195–210,212–234` support:

| Offset / extent | Selected-source meaning | Proposed use |
|---|---|---|
|0..1|outer magic, RX `0x2b1a` (`hgic_def.h:21`)|LE wire check before RX normalization; native unaligned read after normalization|
|2|exact outer type EVENT4 or EVENT2=14|check before decoding event union; CMD3/CMD2=13 are not events|
|3|low ifidx nibble, high flags nibble|no proven event-source binding; do not invent an ifidx==1 event ABI|
|4..5|outer length, **total frame including header**, not payload-only|LE pre-normalization; require equality with original actual|
|6..7|outer cookie|no event-to-command correlation, generation or ownership inference|
|8|EVENT's u8 event_id|read only after full12/type admission|
|8..9|EVENT2's u16 event_id|unaligned **LE16**, preserve full16 bits|
|9 for EVENT|inner C-struct padding|not a high event-ID byte; no new zero-padding constraint|
|10..11|signed short `event.value` / `event2.value`|not a documented payload length/status; leave uninterpreted|
|12..total-1|event payload|bounded extent is total-12 **only after total>=12**; opaque, never cast/string-format/export|

The enclosing ctrl is packed, but its nested event struct is not separately
packed: the short has its inner padding. Existing A1 assertions establish ctrl12
and command-status10; a later implementation should additionally assert outer8,
EVENT/EVENT2 ID8 and both value10 using the **actual header**, rather than silently
changing packing or relying on an unrelated mirror. No compiler/layout experiment
was run here; source ABI assertions must fail the later HOST/build if unsupported.

`HDR_EVTID` selects LE16 **only for EVENT2** and otherwise takes event.event_id.
Thus it is not itself a type validator. EVENT2 ID18 can be silently suppressed just
like EVENT18: the reader explicitly supports both encodings; the setter's preference
for EVENT when ID<=255 is not a receiver canonicalization rule. EVENT2 ID0x0112
is **not18**; do not truncate it or treat the upper byte as padding. There are no
named extended event IDs in this source enum. No event header field supplies a
separate trustworthy payload length, a promised NUL, a firmware lifetime token,
or authenticated station provenance. Ignore uninterpreted fields, do not turn
them into new success constraints or identifiers in logs.

## 4. Existing consumers and allowed structured operations

**Selected Linux kernel consumer performs no structured station event operation.**
Only debug printing and generic queue/export are implemented in event.c. It does
not directly scan, set keys, TX, reopen/reset, update mac80211 connection state,
or complete a control request based on an event ID. Source names in the enum do
not establish such behavior or an allowlist:

| Existing IDs (`hgic.h:415–449`) | Meaning represented by names | Qualification disposition |
|---|---|---|
|1–7|state/channel/disconnect reason/assoc/scanning/scan done/TX bitrate|deny; no supported structured payload consumer here|
|8–17|pairing/connect/disconnect/signal/log/request-param/testmode|deny; not permission to change connection or run commands|
|18 FWDBG_INFO|firmware debug bytes|silently dispose after envelope admission; **no payload interpretation**|
|19–24|customer management/sleep fail/DHCP/connect fail/custom driver/unpair|deny; no DHCP/key/config action|
|25 BLENC_DATA,26 HWSCAN_RESULT|BLE provisioning/scan data|deny, including sample tools described below|
|27 EXCEPTION_INFO,28 DSLEEP_WAKEUP,29 STA_MIC_ERROR|exception/wakeup/security notifications|deny; no recovery or key policy inferred|
|30 ACS_DONE,31 FW_INIT_DONE,32 ROAM_CONECTED,33 MGMT_FRAME|channel selection/init/roam/management frame|deny; never substitute for FWINFO/init completion|
|0,34..65535 / any unrecognized ID|unknown or unsupported|deny without logging ID/payload|

Denial means fail-closed station-admission error, **not successful delivery**, and
is specified precisely in §5. This conservative policy can reject useful legitimate
events; all-firmware/association compatibility is not claimed. There is **no
approved structured event delivery** in the proposed smallest increment. Known
safe processing means envelope validation and owned disposal, not queue/export.

Additional reachable/export consumers explain why a bounded `%s` is insufficient:

- `procfs.c:61–82,201–204` creates read-only-mode0444 `fwevnt` and copies **the whole
  queued skb** to user space. The read ignores `count` when choosing copy length;
  it is neither a privacy filter nor a validated structured delivery API. Do not
  preserve EVENT queueing merely because printk is removed. With A2b's empty
  event-delivery allowlist there is no EVENT/EVENT2 queue output to this reader;
  fixing a general proc reader is outside the smallest event increment. Any later
  event delivery must first close count/copy, capacity, semaphore and retirement.
- `tools/test_app/hgics.c:194–220` dispatches25 to BLE provisioning and26 to relay
  scan handling; no bounds/type checks within that consumer repair the exported
  packet. `:124–168,171–184` parses TLVs without proven per-record bounds, prints
  supplied SSID/password and invokes configuration/system commands, interface
  changes and a restart script. These are excluded **source examples**, not
  linked kernel callbacks, reviewed tools or permission to run them.
- `tools/test_app/hgics_relay.c:362–419` casts payload to `hgic_bss_info[]`, floors
  length/sizeof, prints BSSID/SSID and can choose a new network through wpa_cli.
  `:237–248,317–359` additionally initiates scans or starts/stops hostapd via its
  status loop. Do not graft it into station qualification or claim its side
  effects occur directly in the Linux event callback.
- `hgic_bss_info` (`hgic.h:20–26`) has native fields and a fixed32-byte SSID without
  a promised NUL; `hgic_exception_info` (`:475–494`) uses native ints/union, and
  DHCP result (`:496–498`) uses unsigned ints. Those declarations alone establish
  neither a retained firmware payload ABI nor event-specific lengths/endian.
  Do not choose sizeof/cast/native loads as an implementation shortcut. **Drop/
  deny rather than guess** layout, alignment, string length or endian.
- The `__RTOS__` branch calls `hgics_event` even after setting debug `drop=1`.
  `core.c:1323–1352` forwards to an external registered callback, with legacy
  int parameters used for pointer/length. This is not the selected Linux path,
  and external callback effects/ABI are unproved. A later suppression must not
  leave a shared-source fallthrough forwarding debug bytes; it grants no RTOS GO.
- Bluetooth RX (`rx.c:164–198,310–314`) is a **different outer type** and can also
  populate evt_list or HCI. It is not made private/safe by an EVENT fix. Keep its
  entry unqualified/disabled by the separately reviewed selection; bt_en default0
  alone must not be advertised as a new RX-type deny gate. No overall procfs/data/
  BT/aggregate/privacy acceptance follows from this event-only decision.

## 5. Smallest explicit later increment — decision, not code

After A2a exact independent GO **and a separate parent implementation selection**:

1. **At the existing pre-trim check in `hgic_smac/rx.c`, extend exact-frame
   qualification to EVENT/EVENT2** alongside CMD/CMD2: original actual positive
   and within admitted backing; no removed0xff prefix; full12 control minimum;
   RX magic; total declared==original actual. Reuse the accepted A2a error path.
   Reject all surplus, even zero; truncated declarations; concatenated frames;
   prefixes and unsupported padding. No RX padding grammar is proven, and TX's
   alignment rule says nothing about RX events. Do not rewrite declared to actual.
2. This preserves untrimmed event actual **without adding a wire field or metadata
   ABI**: candidate already saves actual before prefix removal; equality before
   normalization/trim means a valid event reaches fwctrl with that exact extent,
   and fwctrl copies exactly it. There is no asynchronous URB pointer to retain.
   Current trim cannot be repaired inside event.c by comparing only skb->len.
   Alternative metadata plumbing is unnecessary for this reject-surplus policy.
3. **Replace `hgics_rx_fw_event`'s printing/forwarding/queue behavior with explicit
   envelope admission and disposal.** With valid live ctrl as callback precondition,
   check nonnull linear skb/full header, bounded delivered length, normalized RX
   magic/length equality and exact EVENT/EVENT2 type **before** ID reads or payload
   subtraction. The actual fwctrl allocation is linear; reject unsupported nonlinear
   direct inputs rather than inventing scatter support. Use unaligned-safe field
   reads with the endian split in §3. Do not create/dereference a payload pointer
   merely to discard bytes. Null skb is an error, not a successful empty event.
4. Exact envelope + full decoded ID18: **silently free the one owned skb**.
   Payload may be empty, unterminated, binary, percent-containing or any size
   allowed by the admitted envelope; no NUL search, format/string interpretation,
   `%.*s`, hex dump, hash, identifier/value export, generic logger or raw queue.
   No printk at all is preferred, rather than even a per-event constant log/flood.
   No event semaphore, queue eviction, command completion, TX, recovery work,
   scan/key change or init-success state. Leave cookie/ifidx/flags/value unused.
   Suppression across all their bit values asserts **no source binding** and
   authorizes no event operation; it is not a station identity receipt.
5. Any malformed envelope, wrong type at the consumer, unsupported/unknown ID,
   or known ID other than18: **deny**, free only its owned skb once, and use the
   accepted existing `hgic_fwctrl_rx_error` admission latch. No event data/ID log,
   no fallback callback/queue or response-cookie lookup. Existing latch semantics
   may complete pending CMD/CMD2 waiters **as errors** and stop later RX admission/
   submissions; this is intentional failure signaling, never a successful event
   completion or FWINFO copy. Do not broaden BOOT/OTA or opcode allowlists.
6. Once already failed, later/queued events must never clear the failure or create
   delivery/side effects; only dispose of their owned skb. A valid debug event
   does not repair an earlier malformed/unknown event. After async consumer denial,
   A2a's sticky flag blocks later observations/submissions, but **does not undo a
   USB resubmit already made before the worker ran** or cancel queued TX. Do not
   add a false synchronous success/error return contract to the void callback.

Expected production footprint: `rx.c` plus `event.c`, and only a narrowly shared
validation/assertion header if actually needed. No core registration change should
be required. Reuse the real callback and accepted A2a latch, not a generic logging
stack, new transport, speculative payload structure or global guardian. Preserve
all A1/A2a source immutably; later implementation is a **new isolated derivative**,
with fresh pins/license/provenance and independent review. A changed A2a review
outcome/pin requires rebase/review of this design before selecting implementation.

## 6. Required bounded HOST behavioral plan — NOT EXECUTED

Use public synthetic fixtures only; no private event capture, firmware binary,
device identifier, raw response or payload digest. A parent-selected future suite
must extract/pin the **real complete consumer**, actual USB completion, rx.c
ingress, fwctrl RX/worker and accepted A1 command-copy path. Register the real
`hgics_rx_fw_event` at the real ctrl callback slot, checking core's initializer
binding; do not pass by substituting the current A1 stub or A2a abort stub.
Only kernel primitives/work scheduling/USB transports are HOST fakes. Exercise
worker-before-return **and** deferred-worker ordering explicitly, not as races.

Use exact-size allocations, poisoned guards/canaries, unaligned starts0..7,
skb free/ownership accounting, seeded pending command sentinels and an existing
evt_list sentinel. Fake printk must classify format-pointer origin **before any
string read**, record only `FORMAT_FROM_PAYLOAD` / `CONSTANT`, and never forward
an untrusted format to printf/vprintf. No actual `%n` host evaluation or dereference
of header-only one-past payload. Track zero event queue/up/copy_to_user/RTOS-callback/
TX/recovery/scan/key/init-success operations; distinguish allowed frees and error
latch completions from unintended effects. Unexpected hooks must assert, not execute.

| Finite fixture group | Required real-path behavior |
|---|---|
|Safe baseline reproduction, original consumer|Exact valid EVENT/EVENT2 ID18 plus synthetic `%n`, `%p`, `%s`, terminal `%`, unterminated data and zero payload: fake printk records payload-origin pointer without dereferencing/formatting it. Retain this expected unsafe observation; never run unsafe host printf with missing arguments. Original non-debug queues/exports are shown by hooks, not by running tools.|
|Corrected debug suppression|Same cases, NUL at start/end, embedded NUL+surplus bytes, binary and maximum admitted payload: zero printk/format evaluation, raw export, queue change, up(), success completion or side-effect hook; exact single owned free; pending command/caller output/canaries untouched.|
|Extent boundaries via actual ingress|Actual0..11; actual12; selected exact larger values through2560; declared0..65535 with small fixed backing; actual>backing/negative/INT_MAX rejected before read; declared<actual with zero/nonzero surplus, declared>actual, header-only12 declared inside larger actual, two joined events, CMD/event mixtures, truncated IDs/values,0xff prefixes1..4. No trim may turn these into admitted debug.|
|Completion status dependency|For event-shaped fixtures use accepted A2a status-error+positive/zero, success-zero, STOP/ERR/sticky paths to prove real consumer is not reached. This is event integration regression, not a new independent claim of A2a safety.|
|Exact type and full event ID|EVENT18 and EVENT2 LE18 silently drop; exhaust EVENT byte IDs0..255 and EVENT2 IDs0..65535 at header-only12; include0x0112,0x1200,255/256/65535 and differing byte9 for EVENT. Only full ID18 is suppressed without a new error; all other IDs deny. Vary value/cookie/ifidx/flags without reinterpretation or logging.|
|CMD masquerade / callback defense|Matching pending cookie, command-looking payload and FW_INIT_DONE31 do not complete init/FWINFO. Direct real-consumer CMD/CMD2/unknown-type/null/short/nonlinear skb denies; through real ingress CMD remains CMD and uses only A1 association, not event semantics. Valid command regression still succeeds solely on strict command identity.|
|Malformed/unknown plus pending response|Hold valid FWINFO copy until worker denial; error latch prevents copy/commit, leaves full caller/canaries unchanged and performs only error completions. Good debug then good CMD works; unknown/malformed then good debug/CMD cannot clear failure. Test duplicate/error-then-event and queued events after failure; no event skb attaches to a pending response.|
|Bounded flood / disposal|A fixed64-event sequence with seed evt_list/semaphore state: no new event queue occupancy, eviction, up(), logging or unbounded loop; exact allocations/frees. Test direct and real ingress paths, immediate and deferred worker. Do not run a kernel queue race or claim retirement from this model.|

Run the finite suite optimized and ASan+UBSan with a recorded HOST wall-time limit
(e.g.120s per executable, no target timeouts implied), native compiler/version/
argv/UTC, exact source/headers/extraction extents and all output pins. Source ABI
assertions must use actual hgic.h; no ARM build is selected here. Run A1/A2a
regression suites only in new owned output paths after implementation selection,
never overwrite existing evidence. Public test reports contain fixed case labels,
counts and booleans, **no event content, raw/digest values or pointer addresses**.

Required runtime mutants must first compile successfully: restore `printk(data)`
(caught by the safe fake before evaluation); reintroduce bounded `%.*s` output;
remove event pre-trim equality/minimum/type checks; truncate EVENT2 ID; accept
unknown/default queue delivery; bypass persistent denial; allow a debug/unknown
event to complete a pending command. Each must fail a behavior assertion or
sanitizer, not merely compilation. Preserve baseline unsafe observations and any
collector/test failures. Test counts are not distinct races or hardware evidence.

## 7. Unchanged gates / handback

This report closes **design choice only** for one event source increment. Its
implementation, independent behavioral review and any later broader S4 acceptance
are still absent. Non-event grammars, raw proc/BT routes and other selected RX
consumers are not newly qualified. Validating an event does not prove finite TX,
asynchronous init completion, cancellation, owned work/deadline/watchdog progress,
inflight/backing/queue retirement or faulted-probe safety. Autonomous detect_work
still queries ENTER, clears queues, may reinit and schedules delay_init
(`core.c:993–1017`); the event policy must not claim it globally inhibits that.
Key-setting stays a separate mac80211 command callback, not event permission.
S1/S2/S3/S5/S6 and **full S4** remain OPEN; existing module/image pins cannot be
reused as pins for a future changed derivative. No build/attach plan is queued.

Only this new report and `station-event-entry-review/` were written. No source,
tests, helper, central document, Git/index, original module/image or A1/A2a input
was modified. No target/UART/RPC/status, network/discovery, private input,
firmware payload/stock binary, compiler/build or tool daemon was used/executed.
The audit reader is not a production implementation or C patch prototype.

Parent's **07:24UTC host-only** observation: old21a capture expired/finalized,
no old UART owner/socket/recovery arm. This lane did not rediscover/rearm/query it.
Current target power/runtime/watchdog remain **UNKNOWN**; last direct06:43 RAM76
APPLICATION-idle observation is historical. Original failed151-ACK trial/HOLD is
unchanged. Fresh physical/sole-owner/runtime/recovery admission is required for
any later parent-owned hardware work. Watchdog recovery remains preferred when
needed under its reviewed gates, **not active or requested here**. No TX/reset/
association/MTU/network/media/persistent effect is queued; RP-04/product HOLD stays.
