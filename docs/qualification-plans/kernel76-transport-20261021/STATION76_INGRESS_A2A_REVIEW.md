# Station76 A2a independent SOURCE/HOST review

2026-10-21. **GO for the exact bounded CMD/CMD2 original-USB-extent/status-to-copy
and observed-failure RX-resubmission source increment only. No mandatory scoped
blocker demonstrated.** This is not all-firmware compatibility or full S4 GO.

**Full S4, S1/S2/S3/S5/S6, ARM/module/kernel build, staging, native execution,
attach, association and MTU changes remain OPEN/NO-GO. Nothing is queued.**

## Exact provenance and freeze

Read the complete `STATION76_INGRESS_A2A_CANDIDATE.md`, complete A1 independent
review and station-entry gates; inspected actual seven-file diffs, shared headers,
callers/initializers, queue/submission/quiesce/free contexts, unchanged event and
detect/init consumers, tests/shims/runner, and sealed public HOST evidence.
Paths in the source findings are relative to
`apps/qualification/kernel76-station-ingress/source/`.

| Changed file | Bytes | SHA256 |
|---|---:|---|
| `utils/if_usb.c` |15757|`500a5b4149bf506256542f5f19b2b8476051ef0749e110c483ec0b5f02672a79`|
| `hgic_smac/rx.c` |10823|`c9b145f50952a8bc5dc573b845e3a443b78d2cf509d5e7be294e5c63cb83c772`|
| `utils/fwctrl.c` |46706|`9a33bb351f36438f897ebccb2da54de285da24bf8a7eebe6ec813dff09a96c83`|
| `utils/fwctrl.h` |17209|`52f8cc93aef88e85f96238cde152cc93395d17efb3600a3c366cca0751080db6`|
| `hgic_def.h` |8719|`3d13521744e834465feb47f492e567eaefe640e125c28e837f75bb44dc167571`|
| `hgic_smac/hgics.h` |3803|`969cf4264e311e7c605d8f271444fb22d32b6001854164774414f7476cb9932f`|
| `hgic_smac/core.c` |43055|`71a578f033148b2290b2b2b8de5ad540aeb7e4d348043a986615fcad07743452`|

Independent verification checked every A2a candidate-seal inventory entry, every
run08 input/output pin and unmutated extraction byte/line extent; all68 full-file
A1 originals against A1's seal and copy-baseline; exact source path sets; the
candidate report's seven pin rows; and LICENSE. **68 A1 files,61 byte-exact,
seven changed, no addition/removal.** A1 candidate-seal SHA256 is
`22a45351d4573ec0bc90a8e03fa1028434276cb6cdf35409ce814476e0371318`.
LICENSE remains byte-exact,
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`;
notices remain intact. A1 tests are independently compared: `test_control.c` is
byte-exact; only the documented small bus declaration changes in `shim.h`.
`run02/provenance.json` contains all68 pairs and author-run history; seven fresh
diffs are sibling files. Originals and all recorded successful-run inputs were
rechecked afterward. No candidate/original source/test/runner/module/image was
edited or rebuilt by this review.

## Findings: actual ingress and ownership

1. **Completion admission precedes payload reads.** `utils/if_usb.c:224–262`
   checks known STOP first. Otherwise known ERR/sticky bus failure, any nonzero
   completion status (negative or positive), null backing, changed backing pointer,
   transfer extent unequal to the actual qinit/fill extent, nonpositive actual or
   actual beyond transfer extent reject before calling `bus.rx_packet`.
   qinit allocates `max_pkt_len`; init sets that value before USB registration;
   fill and admission use it unchanged (selected default2560). This is bounds/
   backing-identity admission, not proof against kernel memory corruption or DMA.
   Valid URB/context/buf/core lifetimes remain prerequisites, not wire-controlled
   pointers. No new null-context/family-casting assumption was introduced.
2. **Successful zero-length is deliberately fatal qualification policy.** It
   latches just like failed zero-length; this does not establish that legitimate
   firmware cannot emit ZLPs. No reviewed RX ZLP/padding/prefix grammar establishes
   their benignness. Source GO is for this explicit conservative policy only.
3. **Original extent survives until CMD/CMD2 admission.** `hgic_smac/rx.c:207–245`
   receives original `urb->actual_length`, bounds outer8 and the bounded prefix
   lookahead, reads magic/declared with unaligned LE helpers, then requires RX
   magic, declared>=8 and for CMD/CMD2 no prefix, full12 minimum and declared equal
   to **original actual**, not a trimmed length. Declaration mismatch, zero/nonzero
   surplus, truncation, concatenation and prefixed control reject before header
   normalization, trim, worker publication or control decode. Actual bounds were
   established by the USB callback; this is not a new arbitrary-caller API proof.
4. **Actual layout/endian conventions are preserved.** The real packed outer
   header is8, control12, CMD/CMD2 signed status starts at10; static assertions
   compile those facts. New original-wire reads are LE/unaligned. Existing
   packed outer normalization supplies native magic/length/cookie to A1's native
   unaligned checks, while opcode/status/payload remain LE. Selected HOST and target
   are LE; there is no ARM or big-endian code-generation claim.
5. **Valid exact FWINFO still traverses real A1 association and copy.** Actual
   callback → `hgics_rx_data` → `hgic_fwctrl_rx` → worker/response → `do_cmd` →
   strict helper → wrapper succeeds with expected type/ifidx1/opcode/full cookie,
   status24/payload24, version2.1.0.1/SVN22118/chip8400/CPU1 and valid synthetic
   unicast MAC. Wrapper commits a decoded local object only on success. Rejected
   ingress/association/identity/status probes leave full destination/canaries
   untouched. No real MAC, private response or firmware payload was used/logged.
6. **Rejection reaches the same real owner, under its existing lock.**
   `hgic_def.h:181–184` retains the RX signature and adds an internal typed error
   hook plus sticky bus flag, not external UAPI or an invented wire field.
   `hgics_rx_error:200–205` takes the same `hgics_wdev` context as RX and calls
   `hgic_fwctrl_rx_error:188–195`, which locks before A1's bad-receipt helper.
   `fwctrl.c:173–184` also sets `bus.rx_failed` when the async worker discovers a
   malformed/unknown association. Pending CMD/CMD2 waiters complete once; an
   already-completed skb remains its original waiter's property until `do_cmd`
   frees it. No callback free/drain, second sender or recovery scheduling appears.
7. **Observed failures prevent later RX submission/admission.**
   `if_usb.c:184–207` rejects observed STOP/ERR/rx_failed before fill/submit;
   the free-queue loop uses this same gate. Bad callback paths enqueue their buffer
   once and do not resubmit. Submit failure still enqueues/sets ERR/cancels as
   before, now notifying A1 first. STOP cancellation never delivers/resubmits and
   does not create a new receipt error, including STOP together with ERR/sticky
   failure. Backing is retained; this is not a callback-retirement receipt.
8. **Initializer/context and lifetime reachability are consistent for selected
   2G+USB.** Core's sole callback initializer sets the hook at1206, after ctrl
   initialization1162 and before bus_priv publication1211. USB verifies bus_priv
   before initial submission. USB and unlinked SDIO are the only selected-tree
   bus RX callers; rx.c is the sole fwctrl RX caller and core the sole ctrl init
   caller. Both transports zero-allocate their actual shared bus container; SDIO
   uses the same core context, not a blind fmac layout. Its abort/reinit path is
   not qualified. Normal USB quiesce retains STOP→kill/drain outside qlock→core
   remove→free ordering, with bus_priv alive through callbacks. That unchanged
   order is inspected, not concurrent/native retirement proof. Partial-probe,
   intfdata/dev-ref/allocation failures remain S5.

## First-error/receipt semantics: narrow, not S3 closure

Both new bus failure writes set1 and have no selected-tree clear. Receipt poison
is consistently `-EPROTO`, not preservation of the original USB errno. There is
no separate first-error-cause ledger. A1's association/copy latch remains locked;
clearing local USB ERR does not clear it. Later **observed** sticky failure blocks
new RX submission and new station-control receipt admission/copy.

This is not an atomic global work/submission fence: another error may arrive after
a check, and already admitted/submitted URBs or queued TX are not canceled. The
worker can discover a malformed association after a callback has resubmitted;
independent deferred-worker probes explicitly show those prior submissions.
Likewise, a late error cannot retract an already copied result or committed
wrapper/core action. A1 still uses first completion wins while its pending record
exists: structurally valid same-cookie wrong-opcode/type duplicates after the first
completion do not poison it. Unknown/late associations do. Local payload/capacity
copy rejection and legitimate firmware-negative status are not all-error sticky
transactions. Those are retained A1 semantics, not new scope defects.

Original plain STOP/ERR read/modify/write concurrency is not repaired by READ_ONCE
on rx_failed; no atomics, global cancellation or SMP/interrupt race proof is claimed.
The unchanged detect_work can still issue boot ENTER, select reinit and schedule
delay_init/reopen; a new lifetime still needs reviewed retirement. **S3 remains
OPEN.** No firmware/RUN/RESET/GPIO/query effect or new timeout was added by A2a.

## Independent HOST evidence and retained failures

Fresh review-owned command,08:08:36–08:08:51UTC:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-ingress-a2a-review01/review_a2a.py --run run02
```

This independently verifies seals and regenerates uniquely bounded, byte-preserving
actual source slices. All three baseline generated units and all eight mutant
units match author's run08 C bytes **before** fresh compilation. It does not execute
the author runner, any old binary/collector/Makefile, or write author/parent runs.
Generated native executables and outputs are exclusively review-owned.

| Fresh native HOST result | `-O2` | ASan+UBSan `-O1` |
|---|---:|---:|
| Candidate real USB→ingress→A1 control/copy/wrapper suite |2,806,874 PASS|2,806,874 PASS|
| Byte-exact A1 semantic regression body |150,705 PASS|150,705 PASS|
| Independent supplemental assertions |2,688,972 PASS|2,688,972 PASS|
| Original actual A1 USB→trim→copy accepts36-byte FWINFO in40-byte completion |reproduced|reproduced|

All normal tests exit0 with empty sanitizer diagnostics. The candidate reports
68,448 refused fixtures/68,470 modeled sends/22 fake submissions. The supplement
reports65,597 ordered fixtures/65,592 modeled sends/16 fake submissions; its large
count principally repeats all65,536 CMD2 declarations, not distinct concurrency
scenarios. Added probes cover genuinely deferred worker errors before copy,
conflicting first-wins duplicates, late errors after copy, full wrapper preservation,
INT_MIN/INT_MAX/status/transfer extremes, exact sizeof8/12 boundaries, CMD2's real
internal send_data/worker/copy path, STOP precedence with invalid payload pointer,
and completed/pending waiter backing/completion ownership. Natural allowlisted
`do_cmd` emits CMD, so the supplemental CMD2 internal probe is not misreported as
a new public CMD2 opcode or TX permission.

**Eight of eight freshly compiled sanitized mutants caught at runtime**, all
compile exit0/runtime exit2: missing original equality, full-control minimum,
completion-status check, known ERR, known STOP, sticky submit guard, transfer range,
backing identity. An early worker can refuse a malformed short reply without that
being sufficient: the minimum/range mutants are caught by their forbidden RX
resubmission. No compile failure counts as detection.

Review `run01` is retained as a **HOST fixture failure**, not a candidate failure:
the hand-built CMD2 request declared receipt cookie11 but left wire cookie0. Strict
association correctly refused it. `run01-fixture-before/` preserves exact script/
supplement inputs. Setting the synthetic request's wire cookie11 and adding an
exclusive `--run` selector produced run02; production source never changed.
Author's run01–05 extraction/fixture/unused-variable/surviving-range-mutant failures
remain preserved; independent history agrees with its report. In particular the
earlier range mutant survived until an exact-declared oversize fixture isolated it;
final run08 and fresh review now detect it. Parent run09 was not reused or written.

The first final-seal inventory attempt also stopped before producing a seal: a
retained non-UTF8 comment in public `tools/test_app/hgics.c` defeated its text-only
reference scan. `final_seal_utf8_before.py` preserves that script; the corrected
scan escapes undecodable bytes without changing source or hash checks. This was
an evidence-inventory error, not a C behavioral or candidate failure.

Compiler GCC16.2.1, native `x86_64-pc-linux-gnu`, Python3.14.7.
`run02/result.json` records argv/cwd/UTC/exits/compiler identity,611 input pins,
66 extraction records and mutant results; stdout/stderr and `output-hashes.json`
retain outputs. Actual bus/USB buffer structures and functions are extracted;
real protocol/control headers are included. URB, kernel primitives, reduced core
context and unrelated consumers are synthetic. Exact-sized allocations/canaries
exercise bounds, not real2560-byte kernel allocations, queue locks, DMA/refcounts,
elapsed waits, ARM unaligned execution, threads, watchdog or teardown. The event
stub is not real consumer coverage. Final `review-seal.json` inventories this
report and review-owned evidence and rechecks frozen successful-run inputs.

## Remaining boundary / smallest next source step

The next small **source-only boundary to select separately** is real EVENT/EVENT2
consumer admission: reject or safely bound firmware-debug content and use constant
format handling, with exact header/payload/consumer HOST probes. Unchanged
`hgic_smac/event.c:39–48` still executes **`printk(data)`** without bounded NUL/
constant-format admission. A2a neither fixes nor qualifies it. No implementation is
authorized by this review; a disjoint event design review can define that step.

EVENT/BOOTDL/OTA/data/aggregate grammar, abort/reinit, S1 init/guard ownership,
S2 finite TX/deadlines/cancel-unsent, S3 recovery inhibition/init failure receipt,
S5 partial-probe/concurrency/inflight retirement and S6 finite runner/local-only
DHCP remain OPEN. No source GO here substitutes for any of them. Existing
module/image pins are not builds of this derivative; no ARM build/staging or
native attach is admitted.

## Ownership / hardware handback

Wrote only this new report and `station-ingress-a2a-review01/`. No production,
central-doc, Git/index, candidate source/test/runner, private input, stock binary, firmware
payload, target/RPC/status/UART, network/discovery, capture, ARM/module/kernel build,
staging, reset, association or effect operation occurred.

Historical parent07:24 host observation: old21a expired/finalized, no owner/socket/
armed catch; not rediscovered here. Current target power/runtime remain UNKNOWN;
last direct06:43 RAM76 APPLICATION idle is dated. Original failed151-ACK trial and
HOLD remain unchanged. Parent alone needs fresh physical/sole-owner/runtime/
recovery admission for later hardware. No command is queued. RP-04 combined/
adoption/product HOLD remains unchanged.
