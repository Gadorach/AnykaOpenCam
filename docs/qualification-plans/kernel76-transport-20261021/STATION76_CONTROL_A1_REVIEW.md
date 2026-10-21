# Station76 A1 independent SOURCE/HOST review

2026-10-21. **GO for the exact narrow local control-copy/FWINFO source increment
only. No mandatory blocker demonstrated within that scope.**

**Full S4, ARM/module build, staging, native execution, attach, association and
MTU changes remain NO-GO.** S1/S2/S3/S5/S6 are unchanged and OPEN. This is neither
an execution recipe nor authorization for the next source increment.

## Exact reviewed candidate and provenance

Inputs: `STATION76_CONTROL_A1_CANDIDATE.md`, `STATION76_ENTRY_REVIEW.md`, actual
changed source/headers, callers/ingress/event consumers, HOST shim/tests/runner,
`station-control-a1-host01/candidate-seal.json` and all run06 input/output pins.
Paths in the source discussion below are relative to
`apps/qualification/kernel76-station/source/`.

| File | SHA256 |
|---|---|
| `utils/fwctrl.c` | `67e95c81ad1d044475eb521f90f95ffc43702b50d2a6d4c8e4be6d6026750243` |
| `utils/fwctrl.h` | `e35102bd358117fb02104e17894d053abc7a31a4940953d849c02b06e6fb27fc` |
| New `utils/control_receipt.h` | `3b80f917e989fd3587f61e94d9dea713bb26059d7243de5cf0c7bbaccc869073` |

Independent rehash verified the seal, final run06 inputs and every inventoried
output. The manifest hash is
`686ad5da743ea3396962606f6349cf07f6d6c7454c84d572fbc5c4418bdd581d`.
Its exact67 `paired_radio_source75_pins` equal the copy-baseline paths/hashes;
all67 original full-file extents still match,65 copied files remain byte-exact,
only `utils/fwctrl.c` and `utils/fwctrl.h` differ, and only the receipt header is
added. LICENSE remains byte-exact, SHA256
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`;
existing source notices are retained. Originals were rechecked after HOST execution.
Neither originals nor candidate source/tests/runner were edited by this review.

The former paired module/image pins are not candidate build pins. No module,
kernel, firmware image, private input or stock executable was used here.

## Findings: real control path

1. **Bounds precede control-header reads.** `fwctrl.c:323–345` rejects null,
   nonpositive and over65535 ingress lengths before allocation/copy. For all
   positive delivered lengths, `control_receipt.h:20–24` checks minimum12 and
   maximum65535 before magic/length reads. The worker checks this before its type
   switch (`fwctrl.c:250–270`), including event dispatch; response handling checks
   it again before cookie lookup (`:194–199`). Allocated RX skbs are linear.
   No new control-header/minimum-size overread was found. Allocation failure and
   absent workqueue still drop/log rather than establishing a lifecycle failure
   receipt; this is not full asynchronous failure closure.
2. **Association uses cached request identity.** `send_data:280–300` caches type,
   ifidx and opcode before list publication; cookie was allocated under the same
   control lock. Replies for a not-yet-completed station request must match
   RX magic, delivered/declared length, exact CMD/CMD2 type, station ifidx1,
   opcode and full16-bit cookie. Sender allowlist remains the same15 commands.
   All Linux station opcodes are<=255, thus emitted as CMD3; no new CMD2 opcode,
   firmware/RUN/reset/maintenance effect was introduced.
3. **Selected layout/endian handling is consistent.** Actual `hgic.h:112–159`
   has outer8/control12; the inner CMD structure puts signed status at10, with
   byte9 padding. CMD2 uses u16 opcode8/status10. Static assertions guard these
   layout facts. `rx.c:218–220` already CPU-normalizes outer magic/length/cookie;
   receipt helpers use native unaligned reads for those three, LE unaligned reads
   for opcode/status/payload. Low ifidx nibble is compared, high flags/padding are
   not invented protocol constraints. The selected target and HOST are LE; HOST
   is not proof of ARM code generation. Packed existing header accesses remain
   compiler-managed; new wire multibyte reads use unaligned helpers. MAC validity
   receives a u16-aligned local copy, not an unaligned wire pointer.
4. **Copy admission is in the real call path.** `do_cmd:112–122` rechecks the
   receipt-error flag and calls the strict copy helper under `ctrl->lock` using
   that request's cached expectations. It does not merely validate an unused
   side helper. An unknown/bad association observed after first completion but
   before this locked copy prevents copying and returns `-EPROTO`. Helper
   rejection does not touch output. Pointer/length mismatches refuse before TX.
5. **Signed returns and capacity semantics are preserved deliberately.** Signed16
   negative firmware statuses are returned intact with no output copy; optional
   bounded error bytes are not interpreted. Setters/scalars requesting no output
   may return positive scalars without corresponding payload. For positive status
   with requested output, delivered payload must equal status and fit capacity;
   larger capacity permits a legitimate shorter result, leaving the tail alone.
   Zero status with optional bytes does not copy. No truncating memcpy remains.
   A payload/count/capacity rejection at this copy boundary returns `-EMSGSIZE`
   but does **not** set the persistent association-error flag. This is a local
   copy policy, not an all-failures latch, and is not a scoped acceptance defect.
6. **FWINFO succeeds only on exact retail identity.** Successful nonnegative
   FWINFO requires status24, payload24 (not the mirror structure's28), version
   `0x02010001`, SVN22118, chip8400, CPU1 and valid unicast/nonzero MAC before any
   copy. `fwctrl.c:403–426` decodes into a zero-initialized local destination and
   commits only on success; app_version/reserved fields stay zero, smt_dat uses
   retail offset20. Rejections preserve the entire caller object. Invalid MAC
   therefore cannot return24 and reach `core.c:950–975`'s default-MAC branch.
   Public `radio_receipt.py:73–87` independently supports these retained wire
   identity constants without reading device-identifying response bytes.
7. **No new private MAC printing.** Changes add neither a MAC format/log nor
   identifying test data. The unchanged core `%pM` printk still makes future
   native UART handling PRIVATE; validity is not a match against a particular
   device's private MAC, authentication, or cross-lifetime identity proof.

## Completion/replay policy — precise limits, not manufactured blockers

`fwctrl.c:200–222` implements **first completion wins while its pending record is
still present**, including a legitimate signed firmware error. A duplicate cannot
replace/leak the first skb or repair an earlier malformed first association.

Independent probes specifically reproduced:

- Valid first FWINFO, then structurally well-formed same-cookie **wrong opcode**
  or **wrong CMD/CMD2 type** before copy: first result succeeds, duplicate is
  freed, receipt_error stays clear. `resp->done` precedes per-command validation.
  This is the stated first-wins policy, **not** evidence that every conflicting
  duplicate poisons acceptance. No source change is required for narrow A1 GO.
- Negative first FWINFO (-32768 with optional bytes), then valid same-cookie
  FWINFO: negative remains exact, destination unchanged, no parser-error latch.
- Good first response, then malformed short outer frame: global frame validation
  still poisons the operation; no output copy occurs.
- A completed request replay after removal, an unknown cookie before copy, and
  a late timed-out response poison subsequent station admission. Async EVENT/
  EVENT2 are events, not replies, even if cookie matches; the modeled event-only
  response cannot complete the command.
- Cookie0..32767 are each usable once; after the final cookie the u16 counter
  stops advancing at32768 and sentinel0xffff refuses submission. There is no
  same-lifetime wrap or guessed cross-lifetime generation token.

The lock establishes a **copy-admission** boundary, not a transactional module
commit. A later error cannot retroactively erase copied output/return or undo
wrapper/core work. Publication before `skb_queue_tail` also means another error
can arrive after request admission while that request still reaches its queue.
There is no cancellation of already admitted/queued transport work, automatic
recovery inhibition, deadline ownership or full shutdown latch here. These remain
S2/S3/S5, not claims made true by first-wins or by single-thread HOST probes.

## Independent behavioral HOST evidence

Fresh review-owned command,07:30:16–07:30:37UTC:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-control-a1-review01/review_a1.py
```

This inspected review script **does not execute/copy/modify the author runner**,
rerun a collector or write its output directories. It independently extracts the
same actual functions from the frozen C source, verifies the candidate extraction
is byte-exact to run06, includes actual headers and the inspected synthetic
HOST harness, then compiles fresh native executables exclusively into
`station-control-a1-review01/run01/`. Review-owned `supplement.c` adds behavior
probes against those same real functions. No vendor Makefile/cross compiler ran.

| Independently reproduced result | Optimized | ASan+UBSan |
|---|---:|---:|
| Original candidate HOST suite |150705 assertions PASS|150705 assertions PASS|
| Additional review probes |131340 assertions PASS|131340 assertions PASS|

**Newly compiled runtime mutants:16/16 caught at runtime** (sanitized builds).

The candidate suite reports1685 modeled sends/3 event dispatches per mode. The
supplement count includes131072 simple assertions covering all65536 signed16
values; it is not131340 distinct scenarios or evidence of timing/concurrency.
All16 mutant compile commands succeeded; each execution then failed an assertion
or sanitizer, not compilation. Missing-minimum-header and missing-retail-length
mutants produce ASan overreads. First-response-replacement fails the first-data
assertion (and sanitizer leak handling can determine exit status). Original real
code again accepted zero version/zero MAC and reproduced a short-payload ASan
heap-buffer-overflow. Those intentional failing runs are retained, not repaired.

Compiler: GCC16.2.1, native `x86_64-pc-linux-gnu`; Python3.14.7.
`run01/result.json` records command argv/cwd/UTC/exits/compiler identity, exact
source/script/header/input pins, extraction extents and mutant results. Each
command's stdout/stderr is retained; `run01/output-hashes.json` pins generated
outputs. Both candidate modes and both supplemental modes exited0 with no
sanitizer diagnostic. All inspected input hashes were checked again after tests.
Final exclusive `python3 -B docs/qualification-plans/kernel76-transport-20261021/station-control-a1-review01/final_seal.py`
rechecks those pins and records this report plus all review-owned files in
`station-control-a1-review01/review-seal.json` (excluding that seal itself).

The shim uses exact requested allocations and canaries, fake send/event hooks,
single-thread lock assertions and deterministic completions. It does not exercise
kernel allocators, skb reference counting, real queue locking, USB/DMA, real ARM
unaligned execution, interrupts, concurrent threads, elapsed deadlines, watchdog
feeding or teardown. Ordered injections before copy are not real race testing.

## Unclosed boundaries and smallest next eligible source step

- **Original URB extent remains unproved.** `rx.c:227–232` rejects declared>actual
  (except legacy boot), then trims actual to declared before fwctrl. Trailing USB
  bytes can disappear before the new equality check. A1 compares delivered control
  skb length only; it cannot validate original USB completion length or padding.
- **Failed/partial completions and resubmission remain unqualified.**
  `if_usb.c:221–235` ignores `urb->status`, delivers positive actual_length and
  resubmits. Neither strict FWINFO nor the HOST tests supplies that missing status
  receipt or callback retirement.
- **Event dispatcher bounds do not make consumers safe.** Unchanged
  `event.c:39–48` computes event payload after a now-bounded header but executes
  `printk(data)` for debug events, without bounded/NUL/constant-format admission.
  A header-only debug event is not made safe by A1. The HOST event stub does not
  execute this consumer; its safety/privacy is a concrete full-S4 blocker.
- Non-CMD maintenance retains cookie-only association, asynchronous recovery and
  core initialization/teardown remain unchanged. Native concurrent behavior and
  all S1/S2/S3/S5/S6 acceptance stay OPEN/NO-GO.

**Smallest next source increment to select, not execute here:** preserve the
untrimmed USB actual extent plus completion status through the control ingress,
and fail closed on failed/partial/surplus control completions under an explicit
reviewed RX-padding policy (do not infer RX grammar from TX padding). HOST tests
should invoke the actual changed callback/ingress with declared<actual,
declared>actual, status-error+positive-length and status-error+zero-length cases,
proving no successful FWINFO/copy from those inputs. This is a focused ingress
correction, not a kernel rebuild or generic supervisory framework. The existing
debug-event consumer separately needs bounded constant-format handling or explicit
rejection before full S4 GO; neither change is silently included in this review.

## Hardware/ownership handback

Only this new review report and `station-control-a1-review01/` were written.
No source/test/central-doc/Git/index edit, private input, network/discovery,
target/RPC/status/UART, ARM/module/kernel build or binary staging occurred.

Parent's07:24UTC host-only finalization says the old21a capture expired/finalized,
with no old owner/socket/recovery arm; this review did not rediscover or renew it.
Last direct target observation remains dated06:43 RAM76 APPLICATION idle.
Current power/runtime/watchdog are unqueried. Original failed151-ACK trial/HOLD
remain unchanged; A1 does not clear or reinterpret them. Parent alone must obtain
fresh physical/owner/runtime/recovery admission for any later hardware operation.
No command is queued. RP-04 combined/adoption/product HOLD is unchanged.
