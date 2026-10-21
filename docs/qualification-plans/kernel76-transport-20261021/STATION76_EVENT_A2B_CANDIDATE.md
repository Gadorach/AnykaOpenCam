# Station76 A2b bounded event suppression — SOURCE/HOST candidate

2026-10-21. **Implemented and HOST-tested candidate; independent exact-pin review
required before any later build/use. This report does not grant source acceptance,
full S4, ARM/module/kernel build, staging, native execution or attach GO.**
S1/S2/S3/S5/S6 and full S4 remain OPEN. No hardware/effect operation is queued.

## Selection, provenance and exact production pins

Parent selected this isolated implementation after accepted A2a independent GO
(`08949c0`) and the complete `STATION76_EVENT_ENTRY_REVIEW.md` design. That complete
design and `STATION76_INGRESS_A2A_REVIEW.md`, including its seven accepted pins and
limitations, were read. New ownership is exclusively
`apps/qualification/kernel76-station-events/`, this report and
`station-event-a2b-host01/`. No A1/A2a source/test/runner/helper or Git/index edit.

`prepare_source.py` verified both original seals, independent A2a review inventory,
the seven review pins and exact source path set, then copied all **68** accepted
A2a files byte-exact into a fresh source directory. `copy-baseline.json` records
every whole-file offset0/extent/original and copied SHA256, UTC, argv/cwd,
Python/version, preparer hash and input seal/report hashes. Final runner and seal
recheck the originals, including A1 and A2a sealed tests/tools, after HOST execution.
**66 files remain byte-exact; only these two files differ:**

| Relative to new `source/` | Bytes | SHA256 |
|---|---:|---|
|`hgic_smac/event.c`|2534|`b00e6ee84c10f676c2ad7d3afe8abb0800d37224b98445d86acdddde774a6345`|
|`hgic_smac/rx.c`|10917|`600c2bccc48fae9ee5642fa320d83541b261bf24ff8d2d492c7871f6b97e2cf0`|

`LICENSE` remains18092 bytes,
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
Original names, comments and notices are retained; no stock firmware, private
payload or redistribution grant is introduced. Hashes identify public source and
HOST artifacts, never device data, event contents or private-input digests.

## Exact policy implemented

1. **Before normalization, trim or worker publication**, existing A2a `rx.c`
   original-extent qualification now includes EVENT4/EVENT2=14 beside CMD3/CMD2=13:
   RX magic, no removed prefix, full12 minimum, declared total exactly original
   actual. Zero/nonzero surplus, concatenation, truncation and1..4-byte prefixes
   reject through the existing error path. No RX padding grammar is guessed.
2. Complete real `hgics_rx_fw_event` requires its existing live ctrl/bus owner.
   Already-failed ctrl **or** bus only disposes the owned skb. Otherwise it checks
   nonnull skb, linearity, data,12..65535 delivered length, normalized native outer
   RX magic and declared==delivered, then exact EVENT/EVENT2 type before ID access.
   Null skb denies without free. No payload pointer or payload subtraction exists.
3. Actual root `hgic.h` is compiled, not a mirror. New assertions require outer8,
   control12, both event IDs at8, both values at10. EVENT uses byte8 alone; byte9
   is unconstrained padding. EVENT2 uses unaligned LE16, so0x0112 is not18. Native
   normalized outer and still-LE inner fields are not confused. Cookie, ifidx,
   high flags and value remain uninterpreted; no source binding is invented.
4. Exact ID18 silently frees the owned skb once. Empty, unterminated, percent,
   NUL/binary and maximum admitted opaque payloads have no logging, string scan,
   payload read, hash/export, event queue/up/eviction, response association,
   success completion or RTOS forwarding. The old Linux queue/semaphore and RTOS
   fallthrough are removed from this complete function. This is a conservative
   Linux-selected qualification policy, **not RTOS ABI/build qualification**.
5. Every other ID, including FW_INIT_DONE31, and every bad consumer envelope/type
   denies via existing locked `hgic_fwctrl_rx_error`, then frees only its skb.
   Pending CMD/CMD2 may complete **as errors**. A valid debug event cannot repair
   a prior error, and no event skb attaches as a command response. The callback
   stays void. No opcode allowlist, control association or TX/lifecycle policy is
   broadened. Already resubmitted RX/queued TX or earlier committed copies cannot
   be undone; there is no global atomic cancellation claim.

## Reproducible HOST evidence

Fresh command, **08:36:29–08:37:05UTC**:

```text
python3 -B apps/qualification/kernel76-station-events/run_host_tests.py --run run08
```

Native GCC16.2.1, `x86_64-pc-linux-gnu`, Python3.14.7. Each compiler/executable has
an explicit120s HOST subprocess timeout; no target timeout is inferred. No Makefile,
old collector or pre-existing executable is run. `run08/result.json` records693
input pins,96 extraction records, exact argv/cwd/UTC/exits and mutant outcomes;
`output-hashes.json` seals generated units/executables/stdout/stderr. Source/header
pins, input snapshots, diffs and actual byte/line extraction extents are retained.
`candidate-seal.json` rechecks final successful inputs/outputs, unmutated source
slice extents and original A1/A2a/review seals, and inventories this report/evidence.

| Actual fresh executable suite | Optimized `-O2` | ASan+UBSan `-O1` |
|---|---:|---:|
|Real event consumer + actual USB/rx/fwctrl/A1-copy integration|12,127,356 assertions PASS|12,127,356 assertions PASS|
|Byte-exact A2a ingress regression body|2,806,874 assertions PASS|2,806,874 assertions PASS|
|Byte-exact A1 command regression body|150,705 assertions PASS|150,705 assertions PASS|
|Original real unsafe event consumer, safe hooks|12 payload-format calls +2 nondebug queue publications observed|same observation|

Successful test sanitizer stderr is empty. The event suite reports732,940 fixture
invocations and3 modeled command sends; these are repeated finite boundary cases,
not distinct races or hardware evidence. A1/A2a test bodies and both copied base
shims are byte-exact; their unrelated A1 event stub/A2a abort stub **are not A2b
consumer coverage**. The separate event suite registers the **real complete**
consumer, verifies the sole core initializer binding, and extracts actual USB
completion, ingress, fwctrl worker/association and A1 copy/wrapper functions.
Kernel primitives, reduced core container, unrelated data consumers, scheduling
and USB transport are HOST fakes. No complete module or ARM build is represented.

Coverage includes:

- Direct consumer plus real transport/worker path; immediate worker execution
  inside fake queue_work and genuinely deferred worker execution after callback
  resubmission are deterministic separate orderings, not claimed thread races.
- Exhaust EVENT0..255 and EVENT2 LE0..65535, including0x0112/0x1200/255/256/65535;
  only full18 suppresses. Exhaust unused16-bit cookie/value and byte fields without
  new bindings. Direct/transport declaration0..65535 and transport actual0..65535
  with small backings outside admitted USB bounds; exact frame sizes through2560,
  direct full65535, alignments0..7, negative/INT_MIN/INT_MAX/transfer extremes,
  truncated controls, surplus/concatenation/prefixes, bad magic, null/nonlinear/
  oversized direct inputs and every non-event outer type at the direct consumer.
- Error statuses negative/positive with zero/positive lengths, known STOP/ERR,
  null/changed backing and persistent failure. STOP precedence stays unchanged.
  Candidate events never log; inherited USB STOP and do_cmd failure diagnostics
  are separately asserted constant-origin calls, never event-ID/payload output.
- Seeded pending command/cookie, existing event queue and semaphore sentinel;
  exact free accounting, live-allocation registry and canaries. Good debug then
  good CMD succeeds. Unknown then later debug/CMD remains failed, with future
  command TX refused. A valid FWINFO response is held before copy while a later
  worker denial poisons commit: full caller buffer/canaries stay unchanged.
  Command-looking event bytes/FW_INIT_DONE do not grant command or init success.
-64-event direct, per-callback immediate/deferred and fully queued deferred floods
  leave the seeded event queue/semaphore unchanged and free all64 skbs. No event
  capacity/CPU/memory bounds are increased by production changes.

The fake printk first compares integer address ranges of registered live skb
storage (including the header-only one-past payload address); it **never reads the
format, accesses varargs or forwards to printf/vprintf**. Only fixed
FORMAT_FROM_PAYLOAD/CONSTANT labels/counts are output. The original consumer's `%n`,
`%p`, `%s`, terminal-percent, unterminated and header-only calls are safely observed
without evaluating any format. No original proc reader/tool example is executed.
Queue/up, frees and pending completions are instrumented. Export/RTOS tripwires
abort if reached. `consumer-call-audit.json` checks the complete selected consumer's
closed call set: only validation, existing denial and disposal calls; no logger,
queue/export, TX/recovery/scan/key/init entry exists. Synthetic zero-effect counters
are not represented as instrumentation of the unchanged whole driver/lifecycle.

**10/10 mutants compile exit0 and fail runtime assertions exit2**, never counted
from compiler rejection: payload-derived printk, bounded `%.*s`, missing event
pre-trim equality/minimum/type qualification, missing consumer exact type,
EVENT2 truncation, unknown/default queue restoration, persistent-denial bypass
and event-success completion of pending command. Each source unit, compile/run
argv and diagnostic is retained. No mutant evaluates untrusted formats.

## Preserved HOST failures, limitations and unchanged gates

`run01` retained the unused test-variable compile failure; its pre-correction
runner/tests were copied to `input-before/` before editing. `run02` exposed the
fixture's incorrect assumption that an immediate unknown-event denial still
resubmits RX. `run03` and `run04` exposed test accounting of inherited constant
USB STOP and do_cmd failure logs as if they were event output. Those expectations
were narrowed explicitly, not production logging added. `run05` passed the event
suite optimized/sanitized, then refused the unchanged original baseline's unused
`data_len` warning. Only that baseline reproduction receives
`-Wno-unused-but-set-variable`. `run06/07` passed before extra closed-call/type/
queued-flood and live-allocation/persistent-command probes; final run08 passes all.
Initial failed results/generated units/logs and later run input snapshots remain.
Production source did not change after the initial two-file implementation.

This is not all-firmware compatibility: every non-debug event is deliberately
unsupported. No guessed payload struct or authentication/provenance token exists.
No generic procfs/BT/raw tool fix, BOOT/OTA/data/aggregate grammar qualification,
native parameter/regulatory/recovery/timeout/TX edit, global work cancellation,
SMP/IRQ lifetime proof, actual DMA/refcount/retirement or watchdog progress claim.
Unchanged detect_work may still query/reinit/schedule delay_init. Full S4 and
S1/S2/S3/S5/S6 remain OPEN. Exact independent review is the next required source
step; no ARM/module/kernel build, staging, native test or attach GO is granted.

No target/UART/RPC/status/network/discovery, private input, firmware payload,
stock binary, staging, persistent write or Git operation occurred. Parent's old
21a capture expired/finalized;07:24 no owner/socket/armed recovery is historical,
not rediscovered here. Current target power/runtime remain UNKNOWN; last06:43
RAM76 APPLICATION idle is dated. Original failed151-ACK trial/HOLD is unchanged.
Fresh physical/sole-owner/runtime/recovery admission remains parent-owned before
any later hardware work. No reset/association/MTU/media effect is queued;
RP-04 combined/adoption/product HOLD remains unchanged.
