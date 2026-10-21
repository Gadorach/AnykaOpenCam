# RP-02 / station76 A2a ingress candidate

2026-10-21. **SOURCE/HOST candidate delivered; independent review pending.**
**Full S4, ARM/module/kernel build, staging, attach/association/MTU and native
execution remain NO-GO.** No hardware action or build is queued.

Parent selected only A2a after narrow A1 independent GO/local commit152cee0.
This lane owns only new `apps/qualification/kernel76-station-ingress/`, this
report, and `station-ingress-a2a-host01/`. No A1/original/helper/central-doc/Git/index
edit, old collector/Makefile execution, network/discovery, target/RPC/status,
private input, firmware/image/module/stock-executable read/execution, ARM build
or staging occurred. HOST native executables contain extracted public functions
and synthetic primitives, not a kernel module or stock binary.

## 1. Provenance / exact source freeze

Read the complete A1 independent review (including §§178–206), A1 candidate and
station entry review. Exclusive preparation:

```text
python3 -B apps/qualification/kernel76-station-ingress/prepare_source.py
python3 -B apps/qualification/kernel76-station-ingress/prepare_tests.py
```

`station-ingress-a2a-host01/copy-baseline.json` freshly records **68** original A1
paths, full-file offset0/byte extents, original/copied SHA256, UTC, Python/argv,
preparer hash and A1 seal/review hashes. All68 inputs equal A1's candidate seal;
the review's three current source pins were explicitly verified before copying:

- `utils/fwctrl.c`: `67e95c81ad1d044475eb521f90f95ffc43702b50d2a6d4c8e4be6d6026750243`
- `utils/fwctrl.h`: `e35102bd358117fb02104e17894d053abc7a31a4940953d849c02b06e6fb27fc`
- `utils/control_receipt.h`: `3b80f917e989fd3587f61e94d9dea713bb26059d7243de5cf0c7bbaccc869073`

This is exactly the67-baseline-plus-receipt-header A1 source set, not a recursive
build-tree copy. **61 files remain byte-exact; seven differ; no source file was
added or removed.** LICENSE remains byte-exact (`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`);
original notices and paths are preserved. A1 source is immutable and all68
original pins were rechecked after HOST execution and during final sealing.

Paths below are relative to `apps/qualification/kernel76-station-ingress/source/`:

| Changed file | Bytes | SHA256 |
|---|---:|---|
| `utils/if_usb.c` |15757|`500a5b4149bf506256542f5f19b2b8476051ef0749e110c483ec0b5f02672a79`|
| `hgic_smac/rx.c` |10823|`c9b145f50952a8bc5dc573b845e3a443b78d2cf509d5e7be294e5c63cb83c772`|
| `utils/fwctrl.c` |46706|`9a33bb351f36438f897ebccb2da54de285da24bf8a7eebe6ec813dff09a96c83`|
| `utils/fwctrl.h` |17209|`52f8cc93aef88e85f96238cde152cc93395d17efb3600a3c366cca0751080db6`|
| `hgic_def.h` |8719|`3d13521744e834465feb47f492e567eaefe640e125c28e837f75bb44dc167571`|
| `hgic_smac/hgics.h` |3803|`969cf4264e311e7c605d8f271444fb22d32b6001854164774414f7476cb9932f`|
| `hgic_smac/core.c` |43055|`71a578f033148b2290b2b2b8de5ad540aeb7e4d348043a986615fcad07743452`|

Final `run08/*.{c,h}.diff` files retain the narrow diffs against sealed A1
(14/12/12/72/26/12/83 lines for the seven paths in sorted order). No candidate
pin is a paired module or image pin; existing modules/images remain unchanged.

## 2. Explicit RX policy, not a guessed padding ABI

1. **Status is admitted at the actual USB completion before any payload ingress.**
   Any nonzero `urb->status`, including error+positive data and error+zero, fails.
   Only known-success status0 reaches `bus.rx_packet`; no status-error payload can
   be turned into a valid control skb. Status is not added to a fabricated wire
   header or silently lost by forwarding a failed completion.
2. Validate original transfer backing/range: nonnull `buf->data`, exact equality
   with `urb->transfer_buffer`, `transfer_buffer_length == max_pkt_len` (the
   actual qinit allocation/fill extent), positive actual length no larger than
   that extent. Negative/overrange actual lengths refuse before any data read.
   `max_pkt_len` is established before registration and shared by qinit/fill;
   the selected default is2560. This is not a DMA provenance proof.
3. **Successful zero-length URB is rejected and latched.** No retained station
   control wire evidence establishes a zero-byte message/keepalive. The candidate
   does not assume zero means FWINFO, benign empty data, or permission to retry.
4. USB passes the **untrimmed original actual length** into real `hgics_rx_data`.
   For CMD/CMD2, require outer8/full-control12 minimum, valid RX magic, no removed
   alignment prefix, and **declared length == original actual length**, before
   normalization/dereference of control fields, trimming, queueing or decoding.
   Declared<actual (including all-zero surplus), declared>actual, malformed outer
   bounds and undersized control refuse. Multiple controls are not flattened.
5. The inherited bounded four0xff prefix scan remains for unrelated data paths.
   There is no proven station-control prefix ABI, so a prefixed CMD/CMD2 is
   conservatively refused even if its post-prefix extent would match. No TX
   padding/alignment rule is applied to RX. **This strict qualification policy may
   refuse legitimate but unproved firmware padding/prefixes; all-firmware RX
   compatibility is NOT claimed.**
6. Exact CMD/CMD2 reaches the unchanged A1 association/copy helpers through the
   actual worker: expected type/ifidx1/opcode/full cookie remain correlated; retail
   FWINFO still requires status24/payload24 and version2.1.0.1/SVN22118/chip8400/
   CPU1/valid synthetic-tested unicast MAC before copy and wrapper commit.

Other control types have no newly proved original-completion padding grammar:
EVENT/EVENT2/BOOTDL/OTA retain legacy routing/trimming and are **not qualified**.
Data/aggregate consumers retain their existing semantics apart from necessary
outer null/minimum/magic/declared>=8/partial bounds failures before mutation.
Unknown types still return their old generic failure; no new format was invented.
These paths are NO-GO for qualification, not implicitly blessed by CMD equality.

## 3. Actual context/error propagation and lifetime limits

The existing `int (*rx_packet)(void *,u8 *,int)` signature is unchanged. Minimal
internal additions to the actual shared `hgic_bus` are a `void (*rx_error)(void *)`
hook and sticky `rx_failed` flag. This is not an external kernel UAPI, family
cast, new wire field, global supervisor, or exported completion ABI.

- Core's sole initializer adds `bus->rx_error = hgics_rx_error` beside the existing
  RX assignment, **after** ctrl initialization and before bus_priv publication.
  `hgics_rx_error` accepts the same real `hgics_wdev` context as `hgics_rx_data`;
  it calls the new locked `hgic_fwctrl_rx_error` wrapper around A1's original
  `hgic_fwctrl_bad_receipt`. No USB-to-unknown-family blind cast was added.
- The original A1 bad-receipt latch now also sets `ctrl->bus->rx_failed`. Thus a
  malformed/unknown association discovered later by the actual asynchronous
  worker prevents subsequent RX admission/submission after the flag is observed.
  USB completion failures invoke the core hook, setting A1's `receipt_error` under
  its existing lock, completing pending CMD/CMD2 waiters and preserving backing.
  Already-completed skbs remain owned until the original do_cmd free; no unknown
  skb is freed by the new error hook. Maintenance waiters are not newly qualified.
- Callback checks known STOP **first**: normal killed URBs under STOP are queued
  back, never delivered/resubmitted, and do not manufacture a successful command
  or a new parser error. ERR/sticky-failure instead rejects and poisons station
  acceptance. Bad completions enqueue their existing buffer once; no free/drain,
  unlink/reset/reload/query is introduced there.
- `hgic_usb_submit_rx_urb` now checks sticky failure as well as STOP/ERR. Existing
  submission failure also notifies A1 before its unchanged cancellation call.
  `hgic_usb_submit_rx_urbs` uses that same submit function, so later free-queue
  entries cannot bypass the new guard. No second submit API was introduced.
- All selected-tree callers/initializers were inspected, including unlinked SDIO:
  USB receive and SDIO interrupt are the only bus RX callers; core probe is the
  only RX callback initializer; rx.c is the sole actual fwctrl RX caller; core
  probe is the sole ctrl initializer. SDIO allocates its bus context with kzalloc
  and uses the same core context, but its abort/reinit/data grammar is **not**
  qualified. Selected Makefile links2G+USB; no fmac/external-family layout or ABI
  compatibility is claimed from the retained references in the root Makefile.
  `context-audit.json` records all matching selected-tree references, not tests.
- Existing USB probe checks nonnull bus_priv before initial submission; core hook
  is installed before that. Inspected qinit/deq/enq, submit/failure, TX completion,
  quiesce, probe-failure, disconnect and free paths: normal quiesce sets STOP and
  kills/drains outside qlock before core remove; bus_priv remains live through
  callbacks. None of their ordering/backing rules were changed. Existing partial
  allocation/probe dev-ref/intfdata failures remain S5, not fixed or host-proved.

This is a **copy-admission/RX-resubmission** latch, not an all-errors transactional
or global-new-work latch. An error observed after an admitted copy cannot undo it;
an already admitted/submitted URB or queued TX is not canceled by this increment.
Checks and flags are not a new atomic submission fence; HOST ordered injections
are not concurrent race proof. A1's local copy/payload/identity rejection semantics
(not every one is a persistent association error) remain unchanged. No timeout,
credit, watchdog, detect/reinit/reopen or network policy was changed. The new flag
has no clear operation in the selected source; even clearing USB status in HOST
does not clear A1's receipt latch. **S3 still remains OPEN**: existing autonomous
detect/query/reinit and new-lifetime initialization are not inhibited globally.

## 4. Behavioral HOST evidence / retained failures

Final command **07:55:36–07:55:49UTC**, Python3.14.7 / GCC16.2.1,
native target `x86_64-pc-linux-gnu`:

```text
python3 -B apps/qualification/kernel76-station-ingress/run_host_tests.py --run run08
```

| Real extracted-path evidence | Optimized `-O2` | ASan+UBSan `-O1` |
|---|---:|---:|
| Actual USB callback → core ingress → A1 worker/association/do_cmd/copy/wrapper |2,806,874 assertions PASS|2,806,874 assertions PASS|
| Byte-exact A1 semantic regression test body against derivative fwctrl/header |150,705 assertions PASS|150,705 assertions PASS|
| Original A1 USB/rx/fwctrl functions accept surplus after trim |Reproduced|Reproduced|

Ingress run:68,448 rejected fixtures,68,470 modeled command sends,22 fake USB
submissions; these are synthetic counts, not wire transactions or distinct
concurrency scenarios. All normal sanitized executions exit0 without diagnostics.
A1 regression preserves1685 modeled sends/3 stub events; its test body is
byte-exact to the sealed A1 input. Only the shim's small bus declaration was
extended, with original/copied pins in `test-copy.json`.

The original reproduction invokes A1's actual USB callback → actual rx.c trim →
strict A1 copy with a36-byte FWINFO inside a40-byte all-zero-surplus completion.
It returns24/copied payload in both modes. Candidate rejects that original
extent, changes no caller/canary bytes, latches error and submits no new RX URB.

Coverage includes status-error+positive/zero (including USB cancellation statuses
outside STOP), success-zero, actual lengths0..35 and surplus37..2560, all65536
declared values, exact short8..11 CMD/CMD2, unaligned offsets0..7 with exact-size
allocations, original full2560 backing/control, oversized/negative/INT_MAX actual,
bad transfer lengths/backing/null, prefix refusal, STOP/ERR skips, good exact
FWINFO/wrapper, wrong type/ifidx/opcode/cookie/identity/MAC through the real path,
error-then-valid and good-then-error-before-copy, clearing USB ERR without clearing
the receipt latch, asynchronous association failure, direct sticky submit refusal,
existing submission-failure cancellation and pending CMD/CMD2 waiter completion.
Canaries and complete test allocations remain unchanged on rejection; LE
normalization/no-mutation checks are not proof of ARM/big-endian code generation.

**Eight runtime mutants caught, every one compiled successfully first:** remove
original-extent equality, full-control minimum, URB-status check, known-ERR skip,
known-STOP skip, sticky submit guard, transfer actual-range check, or backing
identity check. Each sanitized mutant exits2 on a behavioral assertion; no compile
failure is counted. No new callback recovery TX exists; tests count command
admission/submits and error hooks perform no scheduling or transport call.

Extraction records exact public source hashes, byte/line extents, unique boundaries
and generated-unit hashes. Real `hgic_bus`/USB-buffer structures, real callback,
real enq/submit gate, actual rx.c final function and A1 fwctrl functions execute.
Only primitives/context scaffolding and unrelated data/event consumers are fake.
The original reproduction uses the derivative bus structure's additional unused
fields solely as HOST scaffolding; original functions are not rewritten.

`run08/result.json` records argv/cwd/UTC/compiler/Python/input pins/extractions/
mutants; stdout/stderr and `output-hashes.json` preserve every command/output.
Runner SHA256: `7717b6e8f395830beed53613c5bd7ea067cce2be80f516d9bbb1bfde3170b0f4`.

Earlier exclusive runs are preserved, never overwritten:

- `run01`: extraction boundary failed on retained CRLF; no compilation.
- `run02`/`run03`: test-fixture failure; the synthetic sender wrote the cookie into
  a prefixed packet's raw offset6, corrupting the test header. Diagnostic retained;
  fixed fixture cookie offset, not candidate source.
- `run04`: candidate ingress and A1 optimized/sanitized PASS; original repro compile
  refused an unused HOST variable. Removed that unused shim variable.
- `run05`: candidate/A1/original reproduction PASS; range-removal mutant survived
  because the first fixture also failed downstream extent admission. Preserved;
  added exact-declared oversized fixture to isolate range behavior.
- `run06`: five mutants/runtime suites PASS; `run07`: six mutants plus exact short
  header/multiwaiter fixtures PASS; final `run08`: eight mutants/suites PASS.

No candidate source changed after the initial seven-file patch. Later changes
were HOST extraction/fixture coverage only. After final seal, source/tests/runner
are frozen for parent/independent review, not edited during review.

## 5. Explicit remaining NO-GOs / handback

The event consumer is untouched: `event.c` still has unsafe firmware-debug
**`printk(data)`** without bounded/NUL/constant-format admission. Ingress HOST
event callback aborts if reached; the separate A1 regression uses its inherited
stub. **Neither executes or qualifies the real event consumer. Full S4 stays OPEN.**

S1 init/guard ownership, S2 finite TX/timeouts/queued-work cancellation, S3
autonomous recovery/init failure lifecycle, S5 partial-probe/inflight/backing/work
retirement and S6 reviewed finite runner/local-only DHCP remain **NO-GO**. There
is no actual USB/kernel allocator/queue concurrency/DMA/refcount/deadline/teardown/
watchdog result, no ARM/module build, and no native qualification. Unknown
data/aggregate/maintenance grammar is not a reason to invent a format here.

Parent should independently review these exact seven pins, limited diffs, original
reproduction, extracted real-path harness, all run08 inputs/outputs and eight
compiled runtime mutants. `seal_candidate.py` rechecks all final pins, all68 A1
inputs and LICENSE, then exclusively inventories this report/owned tree/evidence
in `station-ingress-a2a-host01/candidate-seal.json`. This handback grants **no
native GO**, does not close full S4, and must not inherit existing module/image
pins for a future new build.

Hardware history only: old21a capture expired/finalized with no UART owner/socket/
recovery arm at parent's07:24 host observation; not rediscovered/queried here.
Current power/runtime remain UNKNOWN; last direct observation06:43 RAM76
APPLICATION idle is dated. Original failed151-ACK trial/HOLD remains unchanged.
Fresh physical/sole-owner/runtime/recovery admission is mandatory before later
parent-owned hardware work. No UART/RPC/status/firmware/RUN/reset/GPIO/network/
association/MTU/media/persistent effect is queued. RP-04 combined/adoption/product
HOLD remains unchanged.
