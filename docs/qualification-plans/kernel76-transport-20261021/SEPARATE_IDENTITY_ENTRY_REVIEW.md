# RP-02 — separate one-shot APPLICATION / firmware-info entry review

2026-10-21. Independent **SOURCE ONLY**; parent alone owns hardware.

## Decision and concrete blockers

**Conditional SOURCE GO for the narrowly specified separate operation below.**
**Execution NO-GO today: no exact identity controller exists or has been reviewed.**
The parent explicitly selected this new scope after observed retirement; this is
not resumption of the failed trial at its next line. A source GO is **not a target
queue**. Parent must implement, freeze, host-test and independently review the
actual small controller, then perform fresh admission before use.

Concrete closure items for that controller (not a new general framework):

| ID | Required closure |
|---|---|
| I1 | Bind the exact **native-retirement private checkpoint** and its completion provenance to the original boot → postcheck → failed-trial derivation. Old pretrial `admitted_handoff()` TX and `Cfailed` alone are stale. No current-status rebase. |
| I2 | Implement only pre-RO → one state → strictly conditional one info → post-RO, immutable original HOLD, fresh one-attempt outputs, exact per-send ledger, original peer/start/UUID and bounded guard at **inherited** send/retry/receive/RPC boundaries. Never call old `main()` entrypoints. |
| I3 | Add an explicit fatal **active-endpoint warning** check. Existing `trial.HCD_DIAGNOSTIC` and `retire.TRANSITION` both miss usbcore's `Process 1 (init) called USBDEVFS_CLEAR_HALT for active endpoint 0x81`. This could otherwise admit `info` after contradictory queue evidence. Preserve full-stream/cross-record checks. |
| I4 | Demonstrate the actual controller's early-refusal/conditional-info paths, counter/checkpoint binding, fresh 600/300s coverage, <=300s operation, pre-active remaining-time floor and 10s RPC reserve with HOST fault fixtures; pin its source/helpers before use. This audit is not those controller tests. |

The old trial, metadata, postcheck and retirement `main()` functions are **NO-GO
for this new scope**. Trial/metadata/postcheck assume empty HOLD or clear it;
retirement starts at the old failed TX and performs no identity entry. Reuse only
the reviewed parsers/fixed RO transactions and locally composed guard mechanics.

Owned outputs: this file and `identity-entry-review/` only. No production edits,
commit/index changes, private inputs, target/UART/RPC/status/network operations,
builds or ARM execution. Host audit evaluates AST-selected pure parser/constants
against synthetic data, not transport imports or an executable target wrapper.

## 1. Retained outcome and exact source basis

Public `READONLY_RETIREMENT21A.md` and
`evidence/qualification/qc07f/execution-20261021/ram76-resume21a/RADIO_READONLY_RETIREMENT21A01.json`
record **06:00:08UTC read-only retirement PASS**, one refresh/860 UART bytes,
same original private boot UUID, console-only PID1 FDs0..3, a012:8400/config1/
HS480/bulk01/81/packet512/no interface driver, idle ISP, logical watchdog inactive
and clean UART. Exact original failed HOLD remained byte-identical. Parent reports
local commits `7f2b21c` (reader) and `a498082` (result), no later target TX. These
are dated public/parent facts, **not live state observed by this reviewer**.

The original trial still has **150 whole ACK records, not151**, interrupted RUN
ACK, FAILED/HOLD; RUN success text and a later healthy retirement cannot repair
it. APPLICATION/firmware-info identity remains unverified. No firmware/RUN repeat,
256 fallback, GPIO42, module/driver attach, association, network/media/peripheral,
watchdog reset, bus reset, persistent write or memory-policy change is included.

Exact reviewed reader SHA256:
`aeda5136b55d3cec920e73adcd41209f52c978b526e4ba1709908a6398caa9d1`.
`identity-entry-review/result.json` pins all26 inspected public inputs and the
audit. Current `init.c`, `radio-ram-loader.h`, `wifi-monitor.h` match retained
`source-review/entry-bind.json` build50 source pins. The existing binding records
embedded attempt50 `/init` (174248 bytes,
`8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899`)
and candidate76 image
`bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f`.
This review rehashes source/bindings; it does **not** rebuild, read firmware payload,
re-unpack images or claim a fresh independent ELF/image validation.

## 2. What the two commands actually do

Anchors: `bringup/qc07f/initramfs/init.c:319–390,514–517,535`.

| Exact UART command | Build50 behavior |
|---|---|
| `radio state\r` (12 bytes) | Dispatch `radio_info(1)`, **one cycle**; boot ENTER query20B, cookie2. Reports BOOTLOADER/ff APPLICATION/unresolved according to reply. No payload download/RUN is called. |
| `radio info\r` (11 bytes) | Dispatch `radio_info(0)`, **one cycle**; application command43/12B, type3/interface1/cookie1. Reads firmware information; identifying response bytes remain private. |

Both check cached vendor/product/config prefixes and stoppability, resolve the
**current** `1-1/devnum` into usbfs (do not pin obsolete device number2), open USB
RW and watchdog0 RW, set/check nominal5s, claim interface0, and unconditionally
CLEAR_HALT on endpoints01 and81 before OUT. These are **active USB/endpoint-state
operations**, notwithstanding the `READONLY FWINFO` text. They are not cached RO
retirement queries or a reset-free continuation of the failed load interval.
Exclude `radio repeat`/`resetrepeat` (four-cycle variants) entirely.

`devio.c:1382–1434` warns if CLEAR_HALT/RESETEP encounters an active endpoint.
`message.c:1240–1295` requires outstanding requests normally be unlinked first;
CLEAR_HALT sends endpoint CLEAR_FEATURE, then resets the host endpoint/data toggle
on success. It is not a USB device/bus reset or firmware/rail reset. **Only after
the failed load interval ended and separate retirement plus fresh pre-admission
proved bounded known idle** is this endpoint reset source-admissible. Its success
must never be represented as preserving load-interval toggles, proving the PIO
MODE/NAK fix, measuring native toggle handling or qualifying MTU1500.

### Timing and cleanup limits — unchanged

- `init.c:352–385`: opening watchdog already starts/reloads hardware; requesting5s
  is then checked. No pre-ping between the two CLEAR_HALT calls and the OUT/IN
  exchange. One OUT has500ms timeout; up to four IN reads inherit500ms and request
  a512-byte buffer. Negative IN immediately cleans up; nonmatching successful IN
  replies can continue up to four. One `.` ping occurs after a recognized result;
  its return is unchecked. There is no second host-issued query inside this entry.
- CLEAR_HALT uses `USB_CTRL_SET_TIMEOUT=5000` (`include/linux/usb.h:1938`), **not**
  the500ms bulk timeout. Two controls plus one OUT/four IN can nominally consume
  12.5s before console/scheduling/cleanup overhead, exceeding5s. Thus the guard
  can reset a stalled query before a host prompt; source does not prove every
  successful/failed query completes within5s. Do not lengthen target timeouts,
  weaken the guard or inject keepalive/recovery to force success.
- USB bulk is synchronous (`devio.c:1139–1166,1283–1370`); timeout/interruption
  kills the URB before return, then backing/URB are freed. This is source lifecycle
  evidence, not live all-URB/DMA telemetry. PID1 executes synchronously and prints
  the monitor prompt only after returning. Query text alone precedes cleanup.
- Cleanup (`init.c:387–390`) attempts interface release, watchdog `V` write/close,
  then USB close; **all returns ignored**. Watchdog core open is exclusive and
  arms hardware; set-timeout reloads; `V` write permits close **and pings**; release
  attempts stop and may ping on failure while returning0. The linked Anyka driver
  programs keyed load/control, nominal rate46875Hz, and has no active-userspace
  periodic feeder requirement when max hardware heartbeat is0. Logical inactive,
  missing FD or prompt is not electrical-off proof. See candidate
  `drivers/watchdog/watchdog_dev.c:76–97,368–389,697–735,804–826,864–989`,
  `arch/arm/mach-ak3918ev200/watchdog.c:39–95` and prior watchdog review.
- Retain existing host per-query6s wait. A short/unknown/late reply is refusal,
  not permission for a longer wait, another state/info, reset or completion CR.
  Outer300s is an admission/evidence envelope, **not** a target watchdog timeout.

## 3. Exact grammar, privacy and refusal

Use `postcheck.body()` for a fresh command-local body: terminal **unique** monitor
prompt after that TX, optional exact command echo, ASCII, no failure diagnostic.
`Boot.wait()` uses substring matching for this prompt; it cannot by itself prove
completion. Do not accept an old prompt, strip printk lines, join corrupted
records, search for just `APPLICATION`, or accept matching bytes from an earlier
query. At all receive/parse boundaries also scan the continuous diagnostic stream.

`radio_receipt.py:59–70` accepts **exactly five lines**: OUT20/RX16, exactly16 hex
bytes, magic/type/vif `1a2b0700`, declared16, cookie2, cmd0, statusff, checkFD,
matching statusff text and exact `RADIO STATE APPLICATION`. This is stricter than
the target's minimum-length/partial-header test. Reserved reply bytes10..14 are
not independently checked by this schema; do not invent a full-byte identity claim.

**Only after that strict acceptance**, current clean stream/TX/owner/time gates
again, may the single info command be sent. Any status RPC failure, target
refusal, BOOTLOADER/unresolved state, bad length/cookie/extra reply, warning,
unknown prompt or timeout means **no info** and no automatic subsequent target TX.
Even a completely returned negative state does not authorize reload or repair.

`radio_receipt.py:73–87` accepts **exactly four lines**: OUT12/RX36, exactly36 hex
bytes, `1a2b0301`, declared36, cookie1/cmd43, signed return24, payload tuple
version `0x02010001`, SVN22118, chip8400, CPU1, exact matching final text. It
returns only two allowlisted identity/privacy booleans. Byte9/residual identifying
payload fields are not an equality test against a publicly exported device ID.
No raw fwinfo, identifying tail, its digest, hex transcript or exception repr in
public output. Complete raw replies/nonce/UUID/counters/events/errors stay in the
existing protected private envelope outside Git; stdout/public result are
allowlisted states/booleans only. A successful identity result is separate evidence,
not acceptance of the old151-ACK receipt or permission to clear HOLD.

## 4. Fresh admission and exact counter chain (I1)

Before any target TX, parent must hold the original shared exclusive nonblocking
operation lock; no transfer, GUI, shell, second command producer, queued operation
or consumer. Original failed attempt and retirement reader host leases must be
finished. Validate exact image/source continuity and absence of other userspace
USB/watchdog owners through pinned image plus owned history; PID1-only FD sampling
cannot supply a global process/FD census. Unexplained ownership blocks active USB.

Use **`radio-retirement21a01.private-checkpoint.json`**, saved by
`retire21a.py:191–196`: `part`, image SHA, full `peer` tuple, original `start_ticks`,
exact `tx_bytes`, `rx_bytes`, full `rows`, metadata, exact `original_HOLD`, and
`original_trial_still_failed=True`. Parent alone reads/revalidates it. Reparse
saved rows with pinned postcheck schema, compare their UUID with original
`runtime21a01.private.json`, and verify metadata/receipt/recipe/completion
provenance. Do not synthesize a replacement checkpoint from live status.

Let `Rp` = original postcheck refresh, `Rt` = failed trial pre-inventory refresh,
`Rret` = completed retirement reader refresh; each is a validated exact boolean.

```text
Cfailed = Cboot + (320+Rp) + (320+Rt) + 539 + 18
        = Cboot + 1197 + Rp + Rt
Cret    = Cfailed + 859 + Rret
        = Cboot + 2056 + Rp + Rt + Rret
```

Public receipts establish Rp=True and Rret=True. Parent previously reported
Rt=True; this reviewer did not read its private derivation. If all three are
confirmed by parent, Cret=Cboot+2059; never substitute that reported value for
the actual checked checkpoint/event chain. 151056 firmware bytes and USB reply
lengths are **not UART TX counter increments**.

Preserve/reuse the original failed snapshot/counter reconciliation in
`retire.handoff()`/`reconcile_counter()` as **offline lineage**, but adapt its event
selection: the original function compares *all* TX rows after trial preflight
with failed-trial lengths; after retirement those rows include27 additional
sends. Calling that whole helper unchanged now necessarily fails or tempts a bad
rebase. Bound each historical segment by retained recorder event ordering/RX
boundaries and saved operation/completion evidence, not timestamp filtering alone.
Expected failure segment is the retained28 completed sends when Rt=True;
retirement segment is optional1 + metadata12 + inventory14 =26/27 sends,
859/860 bytes. Require ordered protected intents and exact completed lengths,
no intervening partial/other TX event, and no TX after its saved completion.
If provenance is not sufficient, **NO-GO**, not a new baseline.

The retirement checkpoint is written just before final guard check, receipt and
`readonly_retirement_complete_original_HOLD_preserved` log (`retire:195–201`);
checkpoint existence alone is not proof that completion happened. Verify the
matching successful private completion event and public receipt, counters and
source hash. Any RX growth since its saved RX is retained/scanned; it may be
passive logs, never unexplained TX or permission to skip a contradictory interval.

Live admission must revalidate intended adapter/sole original recorder source/
canonical argv/private envelope/capabilities/part, full original kernel peer and
PID/start. Compare an owner refresh to the saved **original peer**, since
`Boot.owner()` can replace `session.peer`. Enforce unchanged real/effective parent
UID/GID and original unprivileged launch continuity. Require exact live TX=Cret,
RX>=saved RX without rollback/file disagreement, TX idle, future unconsumed
original-main-CPU catch, and fresh capture/catch coverage. No reconnect/rearm,
new recorder adoption, command-buffer flush or reset to obtain admission.

Preserve original HOLD **bytes**, shared lock inode/path and both previous public
receipts; parsed operation name alone is insufficient. Compare current HOLD with
the saved original and retain it on every success/error path. Fresh subordinate
private output directory/exclusive files consume the one attempt; do not retry
under another suffix after refusal. Missing/foreign checkpoint/HOLD/source refuses.

## 5. Smallest finite controller proposal (I2/I3)

No code wrapper is supplied by this review. A parent implementation needs only
this local composition, not production monitor/kernel/transport changes:

1. Perform section4 host admission and install the per-instance guarded transport
   before any target TX. At most one bounded passive settle and **one pre-RO CR**
   refresh, only after independently known returned own monitor/empty command
   buffer. Require the exact fresh `\nqc07f> ` response. Never flush unknown input.
2. **Before first active query**, one fixed cached metadata roundtrip, then all14
   existing RO queries in original order (duplicate UUID last). Metadata is the
   existing527-byte nonce line, 64-byte chunks/exact echo before CR, unique BEGIN/
   DONE0 and shell prompt, one explicit exit and owned return/monitor prompt.
   Require exactly PID1 console/ttyAK0 FDs0..3, no usbfs/watchdog FD; cached
   a012:8400/config1/480/bulk01+81/512/no driver. Full14-query schema must pass:
   exact kernel/bootargs/PID1/root/RAWIO/volatile mounts/empty modules/CMA16MiB,
   ISP/guards/queues idle, logical WD inactive/nowayout0/UART errors+busy0.
   Both UUID reads must equal the **original** runtime and saved retirement UUID.
   Saved06:00 facts do not substitute for this fresh pre-round.
   Cached attributes are sequential reads, not an atomic enumeration-generation
   snapshot; continuous topology/source checks bound that limitation, not remove it.
3. Revalidate owner/catch/exact TX/clean whole stream and remaining budget; send
   **one** `radio state\r`, existing6s host wait. Parse full strict APPLICATION.
4. Only upon strict APPLICATION and clean current gates, send **one**
   `radio info\r`, existing6s host wait; parse strict retained identity. No optional
   repeat/flush/settle-query, no reset toggle helper beyond these target entries.
5. On identity success only, one complete cached metadata + full14-query idle
   retirement round, same schemas/UUID. Smallest proposal has **no post refresh**:
   strict info already supplied a fresh terminal prompt. If that prompt is lost,
   refuse rather than add CR to force it. No extra active USB query for cleanup.
6. Fresh final original owner/start/source/catch, exact TX, terminal owned prompt,
   full original diagnostic/new-boot checks plus whole post-retirement topology
   interval; coverage>=300s; HOLD bytes unchanged; exclusive private checkpoint/
   completion and separate allowlisted public receipt. Retire only this new host
   lease. Never touch original HOLD or either historical result.

Reuse `retire.Guard`'s saved **unwrapped** per-instance `check_stream`/`call`
hooks, not only its outer `send()` facade. Internal `Boot.send` retries after a
proven zero-byte refusal and `Boot.wait/received` must still traverse these
guards. A partial/ambiguous write is never replayed; zero-byte refusal is not a
license for a second command execution. Preserve10s send/RPC reserve and gates
both before and after blocking status/RPC; reject all other RPC actions.

Required local additions/binding, not provided by unchanged Guard:

- Deadline<=300s, initial coverage>=600s/final>=300s, dynamic remaining coverage
  and pre-active floor below. Exact expected TX/phase/allowed next wire must also
  be enforced at internal send attempts (not only before an outer call); do not
  expose `trial.monitor`'s broader `radio native-load` allowlist as this scope.
- Full HCD/new-Linux/U-Boot/reset scan starts at **original boot checkpoint RX**.
  Retain the known original failed disconnect as historical evidence, without
  removing it or treating it as a fresh transition. **Topology scanning starts at
  saved completed retirement checkpoint RX**, not today's `current.rx_bytes`:
  otherwise a transition in the unattended gap would be silently skipped.
  Prepend the incomplete LF record at that saved boundary (<=1024B; larger refuses)
  and scan all fresh bytes. Any later disconnect/new/reset-device/enumeration,
  including split records, rejects settled descriptor ownership; no retry.
- Include the explicit active-endpoint warning gate I3 in the **full original**
  diagnostic scan as well as new bytes; generic printk severity is not text in
  this record. Preserve existing HCD fatal patterns. An observed active queue
  contradicts retirement even if a later command prints APPLICATION and a prompt.
- Retain peer/start/HOLD/TX-idle/exact expected TX/monotonic RX at every relevant
  boundary, with no baseline/cursor advance past a rejected interval. A fresh
  owner call must not silently repin a replacement.

Stop on every refusal; no `finally` target cleanup/reset/info. The already reviewed
exception for a **fully framed executed cached shell command** allows its one
explicit exit before rejecting completed invalid metadata, so the shell is not
stranded solely by a bad descriptor value. An incomplete/unknown echo/command/
prompt must **not** receive exit or CR. No inventory/active query after rejected
pre-metadata; no automatic retirement/repair traffic after unknown active-query
completion or parser/transport failure. Preserve uncertainty/HOLD and report it.

## 6. UART ledger and budget (I4)

One cached round = `shell\r`6 + line527 + CR1 + `exit\r`5 + inventory320
= **859 UART bytes /26 completed sends**, plus at most one refresh byte/send.
Define new-operation refresh flags `Rpre`, `Rpost` independently from old Rret.

```text
Cbefore_state = Cret + 859 + Rpre
Cafter_state  = Cbefore_state + 12
Cafter_info   = Cafter_state + 11             # only strict APPLICATION permits
Cfinal        = Cret + 1741 + Rpre + Rpost
```

Smallest proposal fixes Rpost=False: **1741/1742B and54/55 sends** on complete
success. The broader two859/860-round arithmetic is1741..1743B, but a second
refresh is **not** admitted by this minimal proposal. Failure counters stop at
the exact actually completed prefix; these formulas never authorize finishing
the ledger after refusal. Source/schema changes to the527-byte line or14-query
table require a new derivation/review, not a rounded estimate.

Outer monotonic budget **<=300s**, initial fresh capture/catch coverage **>=600s**,
final **>=300s**,10s reserve before each mutating RPC (and recheck after blocking
calls). Set the single deadline no later than the first live owner admission;
it includes subsequent admission work, settling/refresh and both RO rounds.
No reset of it at the active query. Retain existing shell6s/echo3s/command10s/
exit6s/inventory6s each/state6s/info6s bounds and the absolute deadline. No timeout
extension or change to target watchdog5s/USB500ms/controls5000ms.

**Before first active query require >=140s outer time remaining**, a conservative
implementation of the requested **>=130s** floor. The nominal remaining waits
are queries12s + metadata25s + interchunk sleeps0.2s + inventory84s =121.2s;
adding10s reserve already reaches131.2s before other overhead, so130s alone is
not a worst-case completion guarantee. Check again before info with enough time
for its6s plus full post-round plus reserve (>=130s conservative). Refuse before
sending if the selected fixed schedule no longer fits. Both complete rounds'
nominal waits are218.4s plus queries12s, leaving margin within300s; RPC/status/
host stalls can consume it and cause refusal rather than guaranteed completion.
Bounds are host/RPC-boundary checks, not electrically atomic write deadlines.

## 7. Evidence, minimum candidate regressions, and meaning

Executed exactly once:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/identity-entry-review/audit_source.py
```

**78 SOURCE-audit checks PASS**; exact argv/Python/UTC/script and input hashes in
`identity-entry-review/result.json`. Pure APPLICATION/fwinfo mutations, unique/
terminal prompts, private-field suppression, exact wire counts/all historical
boolean formulas, source bindings, complete public retirement receipt and
split-record patterns were checked. Positive fixtures are synthetic. Two passing
reproducers explicitly demonstrate **existing diagnostic patterns miss I3**;
they do not declare it fixed. No private Cret/peer/UUID/event proof was accessed.
No old collector/test or prior exclusive-output generator was rerun.

Parent candidate regressions must exercise the **actual controller** with fake
transport: valid no-refresh/one-refresh success; stale Cfailed/pretrial/current-TX
rebase; foreign/stale/partial checkpoint or absent completion event; historical
segment mismatch/partial TX; changed UUID/peer/start/HOLD/source; pre-round FD/
descriptor/idle failure (zero state/info); negative/malformed/extra/interleaved/
late state (zero info); info refusal; post-round failure; I3 at every split and
between state/info; unattended-gap topology/reset; exhausted600/300 coverage,
pre-active/info floor and10s RPC reserve after a blocking read; real inherited
send/zero-refusal/wait paths; immutable HOLD on success and all failures; exclusive
one-attempt outputs; private-only raw replies. A parser fixture alone is not this
composition proof. No hardware use before parent source closure and fresh gates.

A successful later result may claim only **separate strict APPLICATION + retained
firmware-info identity and bounded sameboot FD/cached-descriptor/idle retirement,
ORIGINAL_TRIAL_FAILED_HOLD retained**. It does not prove all URB/DMA queues empty,
electrical watchdog-off, original strict RUN ACK, MODE/NAK/toggle behavior,
MTU1500/association/network/media, combined-memory adoption, RP-02 completion or
product acceptance. Memory remains a snapshot, not a load budget; RP-04 HOLD
unchanged.

Watchdog preference remains **separate recovery only when needed and separately
admitted**, not queued here. If an already-prearmed query guard stops progress,
retain ownership and the prearmed catch; do not inject another sender/reset while
USB may be active. If it returns uncertain cleanup, a past open is not proof the
guard remains armed. A later deliberate watchdog reset needs its independent
known-state gates; SoC reset is not cold radio/rail reset or permission to reload,
clear HOLD, free uncertain backing or auto-boot. No recovery operation is launched.
