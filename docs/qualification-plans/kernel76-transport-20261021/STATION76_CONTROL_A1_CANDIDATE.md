# RP-02 / station76 A1 strict-control candidate

2026-10-21. **SOURCE/HOST CANDIDATE DELIVERED; independent review pending.**
**Full attach/association/MTU and hardware execution remain NO-GO.** A1 does not
close all of S4, much less S1/S2/S3/S5/S6. No command or build is queued.

Parent explicitly selected this bounded correction after `STATION76_ENTRY_REVIEW.md`;
that review itself was not authorization. This lane owns only the new
`apps/qualification/kernel76-station/` tree, this report, and
`station-control-a1-host01/`. No Git/index/central-document or original source edit;
no target/RPC/status/discovery, network, private input, firmware payload/image
read, ARM/kernel/module build, vendor Makefile, module execution, or staging.

## 1. Provenance and exact candidate

The exclusive preparer ran:

```text
python3 -B apps/qualification/kernel76-station/prepare_source.py
```

`station-control-a1-host01/copy-baseline.json` records all **67** original source
paths, full-file offset/byte extent, original SHA256, and byte-exact copied SHA256,
plus UTC, Python version, argv and preparer self-hash. Source selection is exactly
`paired_radio_source75_pins` in `prepare76-manifest.json`, SHA256
`686ad5da743ea3396962606f6349cf07f6d6c7454c84d572fbc5c4418bdd581d`, with
`station75/` stripped and paths preserved. Inputs are solely the corresponding
public text files in:

`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/station76-final/`.

No recursive build-tree copy: no generated objects/ELFs/modules/configs, firmware,
or private files. The original LICENSE/notices are retained. The 67 original
source hashes were rechecked before and after final host tests. **65 copied files
remain byte-exact; only two original files changed, plus one new pure header.**
Limited diffs are retained as `run06/utils_fwctrl.{c,h}.diff`.

Candidate paths below are relative to `apps/qualification/kernel76-station/source/`:

| File | Bytes | SHA256 |
|---|---:|---|
| `utils/fwctrl.c` | 46380 | `67e95c81ad1d044475eb521f90f95ffc43702b50d2a6d4c8e4be6d6026750243` |
| `utils/fwctrl.h` | 17157 | `e35102bd358117fb02104e17894d053abc7a31a4940953d849c02b06e6fb27fc` |
| `utils/control_receipt.h` (new) | 3984 | `3b80f917e989fd3587f61e94d9dea713bb26059d7243de5cf0c7bbaccc869073` |

Existing paired module `e667ed2a…` and QC6U76.UIM `bd8a2a08…` are unchanged and
were not opened here. **This candidate is neither a compiled paired module nor
RAM-staged/native accepted.** Any future changed module would require a new
reviewed build/staging binding; it must never inherit those existing artifact pins.

## 2. Source-supported ABI, not family-structure guessing

- Actual selected `hgic.h`: outer header8 bytes, full control header12. The inner
  CMD structure has a padding byte9: signed status is **offset10**, not9. CMD2's
  status is also offset10. Static assertions retain these layout requirements.
- Linux `HGIC_CMD_START=0`, `HGIC_WDEV_ID_STA=1`; all 15 allowed station commands
  have opcodes<=255, so the selected sender emits CMD type3, not CMD2. Request
  cookie/type/ifidx/opcode are cached before pending-list publication and compared
  to the response. CMD2 is tested as a parser branch but no new CMD2 opcode is
  enabled in the station allowlist.
- Retained public `apps/qualification/kernel76-transport/radio_receipt.py:73–87`
  independently pins FWINFO response type3/ifidx1/opcode43/status24, total36 bytes,
  and identity version `0x02010001`, SVN22118, chip `0x8400`, CPU1. No private
  response or identifying suffix was read. This is not new native observation.
- Unchanged `hgic_smac/rx.c:218–232` converts outer magic/length/cookie to CPU
  endian **before** forwarding to fwctrl; the helper uses safe unaligned native
  reads there. Opcode/status/payload remain little-endian and use unaligned-LE
  helpers. This explicitly reflects the selected path, not double conversion.
- Byte3's low nibble is ifidx; high flags are not guessed to be zero. Async
  EVENT/EVENT2 are dispatched as events and never satisfy pending CMD requests.
- Retail FWINFO is exactly24 payload bytes. The mirror's28-byte `hgic_fw_info`
  is only a destination object: no wire cast/copy to that structure. `app_version`
  stays zero; `smt_dat` is decoded at retail offset20; reserved destination bytes
  are initialized, not interpreted as a newer firmware layout.

## 3. What changed

1. Before header access/copy: full minimum control header, bounded delivered size,
   RX magic, exact declared/delivered extent, expected type, station ifidx, opcode,
   and full cookie. A zero/null/negative/over65535 ingress extent is refused before
   allocation. Unknown or malformed control association poisons subsequent station
   admission and completes pending station waiters as failed.
2. First pending completion wins, including legitimate signed firmware errors.
   Duplicate same-cookie packets cannot overwrite/leak the first response or turn
   an earlier rejection/error into success. Unknown late/completed/expired-cookie
   responses poison future station admission. Cookies never wrap/reuse within one
   ctrl lifetime; exhaustion refuses before queue submission. Cross-lifetime
   correlation still requires retirement, not a fabricated wire generation.
3. `hgic_fwctrl_do_cmd` calls the real strict copy gate with cached expectations,
   not just the helper in isolation. It rechecks the parser error under the same
   lock before copying, including an error observed after completion but before
   copy. Caller buffers are untouched on parser/capacity rejection.
4. Signed16 firmware error returns remain exact negative values and never become
   copy lengths. All14 allowed non-FWINFO commands retain scalar/setter semantics:
   positive scalar with no requested output is not invented to be a payload size.
   Bounded optional negative-error/scalar bytes are ignored, not parsed as an
   undocumented diagnostic ABI. When bytes **are** requested, positive status must
   equal the delivered payload extent and fit the caller's capacity; truncating
   memcpy is removed. Null pointers with nonzero lengths are rejected before TX.
5. Successful FWINFO requires exact24/status24 and all four retained identity
   values **before** payload copy. `is_valid_ether_addr` rejects zero/multicast/
   all-ones; it receives an explicitly u16-aligned local copy rather than an
   arbitrarily aligned wire pointer. Invalid identity/MAC cannot return24 to core
   and reach its default-MAC fallback. The wrapper commits a fully decoded local
   object only on success, leaving its caller's object unchanged on failure.

No opcode was added, no firmware load/RUN/reset/recovery or auto effect was added,
and no lifecycle/timeout/core/USB source was edited. The small receipt-error flag
is not a new module supervisor, completion ABI, or full no-recovery latch. It
inhibits **subsequent** station admission, not already queued/concurrent transport
work (S2/S3). Existing MAC printk in unchanged core remains a future private-UART
handling issue; no real MAC logging was added.

## 4. Behavioral host evidence

Final command, **07:18:23–07:18:40 UTC**:

```text
python3 -B apps/qualification/kernel76-station/run_host_tests.py --run run06
```

Python3.14.7; GCC16.2.1, checked native target `x86_64-pc-linux-gnu`. No cross-build.
Real functions from `#define STR_LEN` through just before `hgic_fwctrl_init`, plus
the entire real `hgic_fwctrl_get_fwinfo`, are extracted with unique boundaries and
recorded byte/line extents, source input hashes and extracted hashes. The harness
includes actual candidate `hgic.h`, `fwctrl.h`, and `control_receipt.h`. Actual
allocation/send-command/send-data/pending-list/RX worker/response association/
do-command/copy/FWINFO-return paths execute with fake kernel primitives and
synthetic packets. No string-presence result is counted as behavioral coverage.

| Final run06 result | Evidence |
|---|---|
| Optimized `-O2` | **150705 assertions PASS**,1685 modeled sends,3 event dispatches |
| `-O1 -fsanitize=address,undefined` | Same **150705 assertions PASS**, no sanitizer error |
| Runtime mutants | **16/16 killed**, all compiled before executing |
| Original invalid identity | Extracted original accepts zero version/zero MAC; reproduced |
| Original short payload | Original real memcpy reads24 beyond a12-byte allocation; ASan heap-buffer-overflow reproduced and preserved |

Coverage includes full/truncated0..35 headers/replies, exact-size allocations at
eight unaligned offsets, oversized/28-byte FWINFO, declared-length extremes,
type/ifidx/opcode/cookie conflicts, bit mutations across version/SVN/chip/CPU,
invalid MAC classes and128 synthetic legitimate unicast prefixes, signed status
extremes for every allowed setter, bounded negative-error bytes, output pointer/
capacity refusal, canaries/no output change, actual wrapper return/object commit,
async events versus replies, duplicates, malformed-first/valid-later refusal,
good-then-unknown-before-copy refusal, timeout-late and completed-response replay,
cookie-space exhaustion, and zero-submission local admission failures.

Mutants remove minimum header/opcode/type/declared length/ifidx/cookie/sign/
version/SVN/chip/CPU/exact retail length/MAC/capacity checks, reintroduce cookie
reuse, or replace the first reply. They fail assertions or sanitizers at runtime;
compile refusal is not counted as a killed mutant.

`run06/result.json` retains argv, UTCs, Python/compiler identity, all input pins,
extraction extents, mutant definitions/results and exact commands. Each command's
stdout/stderr is preserved with hashes. `output-hashes.json` pins every generated
output except itself. Runner self-hash:
`6044bf6d847ba6fb7cd7a18fafcc65aa0d3cf4f300780b92cfebc0ab51b6eabc`.
Only synthetic MACs occur in tests/evidence; no device-identifying bytes/digests.

### Earlier observations preserved, not overwritten

- `run01`: host shim lacked the unused-header typedef `s8`; compile failure.
- `run02`: GCC's inlined constant-null negative test produced an array-bounds
  compile warning; changed that test to use a volatile null input, not suppression
  of parser bounds checks.
- `run03`: candidate optimized/sanitized behavioral PASS and original faults
  reproduced; mutant harness stopped at an unused-but-set-variable warning.
- `run04`: earlier candidate/10 mutants PASS, including aligned MAC handling.
  Later work tightened error-before-copy and
  clarified preserved scalar semantics.
- `run05`: those source corrections/expanded tests PASS with10 mutants.
- `run06`: final source pins above, final tests/16 mutants PASS. No earlier run is
  relabeled as the final source. Every run is exclusive and refuses overwrite.

## 5. Concrete remaining NO-GOs / limits

**A1 cannot claim exact original USB-transfer extent correlation.** Unchanged
`rx.c:227–232` first rejects declared length larger than actual, but then trims
the forwarded length to the header's declared length. A response with extra
trailing USB bytes can therefore lose that evidence before fwctrl sees it.
The new gate compares actual **delivered control skb** length to declared length;
it cannot recover the original USB completion length or decide a trailer/padding
ABI. Do not equate the host forged-length tests with closure of this upstream
boundary. Strict original-transfer extent acceptance remains **NO-GO**, requiring
a separately selected change/review at the untrimmed ingress, not guessed padding
rules or an out-of-scope edit here.

Likewise, unchanged `if_usb.c` does not preserve/admit `urb->status` at this gate;
partial/failed USB completion handling is still S4 OPEN. The host shim has no
real USB submission, kernel threads/interrupts, deadline, DMA, watchdog, reference
counting or work cancellation. A control cookie is association, not authentication
or a cross-reset/module-lifetime generation token. Same-lifetime reuse is prevented;
cross-lifetime stale delivery needs actual reviewed transport retirement. Non-CMD
boot/maintenance retains legacy cookie-only handling and is **not** qualified by
A1; no boot-reply ABI or safety claim was guessed.

Unchanged event consumer has a firmware-debug `printk(data)` path in `event.c`;
A1 only bounds/routes events and does not sanitize that existing consumer. Its
safety/privacy remains a concrete follow-up for full S4/attach review, not a claim
that strict CMD validation makes all event handling safe.

| Blocker | Status |
|---|---|
| S1 guard/init completion ownership | **NO-GO**, unchanged |
| S2 TX/jiffies/credit/timeout cancellation | **NO-GO**, unchanged; no queued-work cancellation claim |
| S3 autonomous detect/reinit/reopen, asynchronous init error latch | **NO-GO**, unchanged |
| S4 strict local control-copy/FWINFO candidate | Host PASS; independent review pending; original-ingress extent/URB status/event consumer remain OPEN |
| S5 partial probe, used/inflight/work teardown receipts | **NO-GO**, unchanged |
| S6 finite guarded runner and local-only DHCP policy | **NO-GO**, unchanged |

## 6. Parent handback

Review the exact three source pins, limited diffs, complete real-path harness and
run06 receipts independently **before any build/use**. Decide the remaining
ingress/async/event boundaries explicitly; this lane does not authorize widening
its source scope, building a paired module, loading one, or staging any binary.
No full S4 or attach SOURCE GO is asserted here.

`station-control-a1-host01/candidate-seal.json` rechecks final run06 input/output
hashes and inventories this report plus the complete owned source/test/script
tree. The exclusive seal script records its own hash/argv/Python/UTC; no hashes
of private data or uninspected firmware/module/image are introduced.

Latest hardware meaning is inherited history only:06:43 separate identity and
retirement PASS, original151-ACK trial FAILED/HOLD unchanged. No hardware action
occurred here.07:09 capture was dated authority and expired before final host work;
it was not queried/reset/renewed and is not permission. Parent alone owns fresh
hardware admission/recovery. No driver/firmware/RUN/reset/association/MTU/network/
media/persistent effect is queued; RP-04 combined/adoption/product HOLD unchanged.
