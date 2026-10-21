# RP-02 — exact separate identity controller review

2026-10-21. Independent **SOURCE/HOST ONLY**; parent alone owns hardware.

## Decision

**CONDITIONAL SOURCE GO for ONE separate identity operation at the exact revision
below, subject to all fresh parent gates in section5.** I1–I4 from
`SEPARATE_IDENTITY_ENTRY_REVIEW.md` are closed for this candidate. No further
concrete controller blocker was demonstrated. This closes the earlier
**unimplemented-controller** execution NO-GO only for these exact sources; it
does not make the old entrypoints admissible or supply current hardware admission.

**No target operation is queued or launched.** The original native whole151-ACK
trial remains **FAILED/HOLD**, notwithstanding the separate06:00 cached retirement
PASS. Native APPLICATION/firmware-info identity remains **UNVERIFIED here**.
Source GO does not repair that failed receipt or clear its original HOLD.

Owned outputs: this report and fresh `identity-controller-review/` only. No
production/helper edit, Git/index/ref action, private input, real target/UART/RPC/
status/network operation, build or ARM execution occurred. Actual controller
`main()`/`handoff()` were exercised only through inspected in-memory fake tests.
No prior exclusive-output collector was rerun; earlier failures/evidence remain
unchanged. Parent-reported offline private lineage PASS is not independently
observed private evidence or current owner/runtime authority in this review.

## 1. Exact source and test evidence

| File | SHA256 |
|---|---|
| `apps/qualification/kernel76-transport/identity21a.py` | `c3c19a12bffb56eec2044e259793f58a7243e1ae20ac6fe8cf75ee3f3229ceeb` |
| `apps/qualification/kernel76-transport/test_identity21a.py` | `8393989ac89f75100521bfca9fd55932122f7c076b2f56f367a6b0da913ef04b` |
| unchanged `retire21a.py` in that directory | `aeda5136b55d3cec920e73adcd41209f52c978b526e4ba1709908a6398caa9d1` |

Full public input/script pins, exact argv, Python/UTC and individual outcomes are
in `identity-controller-review/result.json`; supplied test output is in
`identity-controller-review/supplied-tests.log`. All inputs were rehashed unchanged
after the run, including the authored candidate evidence and preserved initial
fake-file-offset failure. The candidate/old helper pins match the frozen authored
HOST manifest and prior independent source basis. No image/payload/private capture
was opened or rebuilt; existing build50/image binding is retained, not repeated.

Executed once:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/identity-controller-review/review_host.py
```

- **48 supplied HOST methods PASS**, reproducing25 existing +23 new methods.
  These are methods, not48 native trials or a count of all subfixtures.
- **62 independent checks PASS**, including28 real inherited Boot/protected-call
  composition probes: two healthy controls and26 faults (13 fault classes both
  before first send and after one proven zero-byte refusal). The final private
  RPC capability, raw stream, HOLD, clock and process identity were all memory
  fakes; real `Boot.status/check_stream/send/received/wait` and real
  `PrivateBoot.protected_call` translation were retained. Healthy calls became
  private status/protected send with original peer and conditional RX watermark.
  Deadline/coverage/HOLD/TX/RX/activeEP/HCD/topology/new-kernel/reset/busy/catch/peer
  faults froze without a later send or completed program slot. A helper healthy
  wait is not substituted for the controller's stricter body/nonce schemas.
- Additional independent checks cover non-timestamp cumulative lineage, zero-size
  frame refusal and source-manifest membership. External sockets, subprocess
  execution, default private-path access and default process-tick lookup were
  explicitly denied during tests; nested inspected fixtures supplied the fakes.
  No test needing actual private admission was run.

## 2. I1 closed — native-retirement boundary and full failed lineage

`identity21a.py:50–101,115–158,329–342` implements the necessary new binding, not
the stale `retire.handoff()`/all-events-after-preflight selector:

1. Pinned original `meta.admitted_handoff()` supplies original boot/image/postcheck
   provenance and the pinned14-query source. Its returned pretrial TX is **not**
   adopted as current TX. The original failed checkpoint/preflight/snapshot,
   native-retirement checkpoint, public retirement receipt and final successful
   private completion event are independently required by the new handoff.
2. Recorder TX is walked in original event order from zero. Protected intent and
   equal positive-length completion must alternate; unknown/ordinary/private TX
   kinds, nested/unmatched/pending/partial/extra TX reject. Cboot/Cpost/Cfailed/
   Cret must be whole-frame boundaries. Exact length sequences are compared for
   each postboot segment; no timestamp-only filtering or live-status rebase.
3. Typed refresh booleans Rp/Rt/Rret give:

   ```text
   Cpost   = Cboot + 320 + Rp
   Cfailed = Cboot + 1197 + Rp + Rt
   Cret    = Cboot + 2056 + Rp + Rt + Rret
   ```

   The saved native-retirement TX and successful completion TX must equal Cret;
   completion RX must equal saved checkpoint RX. The completion must occur once
   and be the retirement log's last event. An unfinished checkpoint alone refuses.
4. Failed and retirement records must retain original part/image/start identity;
   original recorder PID and full saved retirement peer must match the original
   unprivileged launch identity. Saved rows are reparsed, UUID compared with
   original runtime, and metadata matched to the frozen public retirement receipt.
   The known failure receipt remains failed with no earlier state/info query.
5. Saved failed snapshot length/boundaries/DONE/monitor/no-state-info witnesses are
   checked, then its **exact bytes** compared with the current original recorder
   slice before any target TX. The fresh owner must report exact TX=Cret and
   RX>=saved retirement RX. Later passive bytes are scanned, not ignored.

This proves source-enforced **length/order/counter lineage**, not historical TX
payload equality or private provenance by itself. Parent still must admit the
actual protected files/continuous capture/source/history. No reviewer read private
Cret/peer/UUID/events. A mismatch refuses; do not regenerate a checkpoint from
status, discard events, or adjust historical flags to make it pass.

## 3. I2/I3 closed — fixed operation, sticky HOLD, real internal guards

### Sequence and framing

`identity21a.py:265–306,343–373` permits only:

1. At most one exact empty **pre-round** monitor refresh, after saved returned
   own monitor/empty-buffer lineage and zero intervening TX. Require exact fresh
   `\nqc07f> `; not a flush or recovery attempt. No post refresh exists.
2. Fixed527-byte cached FD/descriptors shell transaction plus full14-query idle
   inventory **before state**. Exact nonce BEGIN/DONE/prompt, full command echo
   before CR, one explicit exit with unique returned monitor prompt, then all
   original inventory schemas/duplicate UUID equal to original runtime.
3. Exactly one `radio state\r` with strict command-local body and retained
   five-line APPLICATION parser. **Only** its strict success sets application
   permission for exactly one `radio info\r`; info must pass the retained exact
   four-line/36-byte/chip-version-SVN-CPU parser.
4. Only identity success enables the full fixed cached FD/descriptors +14-query
   post-retirement round, same original UUID/idle schemas. Any state/status/
   parser/transport failure stops without info or automatic target cleanup.

The sole completed-invalid-metadata exception is preserved: once the executed
cached shell command has complete nonce/prompt framing, its already-planned one
exit occurs **before** metadata parsing. Invalid FD/descriptor then stops; partial
echo/unknown completion/timeout receives neither exit nor CR. No `finally` target
cleanup, native-load/RUN/fallback/reset or broad monitor allowlist is reachable.

### HOLD and one attempt

`identity21a.py:104–113,309–330,374–395` opens the original shared lock read-only
with NOFOLLOW, takes the same nonblocking exclusive flock and compares fstat/lstat
regular-file single-link identity plus exact original content. It cannot write,
truncate/chmod/replace/clear the HOLD. Native checkpoint original-HOLD bytes must
match; UID/effective UID/GID/effective GID remain the same unprivileged identity.
Content/inode/path identity are checked repeatedly through guards and on failure.

The fixed fresh private attempt directory is created **before handoff/admission**;
refusal consumes it. Its fresh transport child is not a retry suffix. Existing
public output refuses before target TX; private checkpoint/error/public output
files are exclusive. No automatic second attempt is possible under this entry.
The supplied test's old fake-offset failure remains preserved and is not called a
controller native failure or silently erased.

### Actual inherited boundaries and topology

`identity21a.py:161–262` locally subclasses the exact frozen retirement Guard;
its saved unwrapped instance checker/call avoid recursion and remain installed
on this **one session**. Thus real Boot internal checks, zero-refusal retry path,
wait and receive traverse the new guard, not merely an outer facade.

- A pending exact wire and phase/index are required **inside the actual send
  RPC**, together with original peer, conditional RX cursor and exact expected TX.
  Only full acknowledged completion advances TX/index. A proven zero-byte refusal
  retains that slot; partial/ambiguous transmission freezes without replay. Direct
  inherited-send bypass cannot create pending permission. No source change to the
  transport/recorder was needed.
- Status/RPC gates run before and after blocking calls. State/coverage/owner/HOLD/
  monotonic RX/exact TX and diagnostics are checked on the returned status before
  internal send. Latest active-phase status must still end at the monitor prompt.
  Owner refresh cannot silently repin original peer/start. Only status/send RPC
  actions are accepted; protected-call keeps the original peer fence and protected
  status/send conversion.
- Full-original HCD/new-Linux/U-Boot and explicit **active-endpoint warning** scans
  start at original boot checkpoint RX. `Process ... called
  USBDEVFS_(CLEAR_HALT|RESETEP) for active endpoint` now rejects even without a
  literal severity word. This closes I3; an active queue cannot be excused by a
  later APPLICATION/prompt.
- Topology starts at **saved native-retirement RX**, not new owner status RX,
  retaining an incomplete LF boundary record (<=1024B). Thus unattended-gap and
  split disconnect/enumeration/reset-device records reject before active entry.
  Known original failed disconnect remains historical evidence in the full
  original stream; it is not filtered into an accepted ACK or counted as a fresh
  settled-interval transition. Rejected inherited epoch cursors are restored and
  the guard freezes; no clean cursor is manufactured after a contradiction.

## 4. I4 closed — retained bounds, ledger and privacy

`identity21a.py:173–193,218–244,334–336,359–388` enforces one monotonic300s deadline
set **before first owner call**, fresh initial coverage>=600s, dynamic/final
coverage>=300s, >=140s remaining at state and>=130s at info, and >10s remaining
before each mutating RPC. Actual internal send checks repeat the active floor
after status reads/zero refusals. No deadline reset at state/info. Final owner
refresh and guarded stream checks precede result creation.

Exact successful UART ledger:

```text
Cbefore_state = Cret + 859 + Rpre
Cafter_state  = Cbefore_state + 12
Cafter_info   = Cafter_state + 11
Cfinal        = Cret + 1741 + Rpre          (Rpre is one validated refresh flag)
completed sends = 54 + Rpre
```

Each cached round is539 metadata +320 inventory =859 bytes/26 sends. Success
asserts program exhaustion and total1741/1742B. A failure stops at the completed
prefix; these totals never authorize sending the rest of a failed program.

No target5s watchdog,500ms bulk or5000ms CLEAR_HALT timeout change. Host query6s,
shell6s/echo3s/command10s/exit6s bounds are retained under the outer deadline.
As in the design review, two controls plus bulk can exceed nominal watchdog5s;
neither this wrapper nor HOST tests prove successful query completion/electrical
stop. Limits are checks at host/RPC boundaries, **not** an electrically atomic
write deadline or immunity to arbitrary host/driver/filesystem scheduling stalls.
Late/short/unknown responses refuse, never extend time or cause another query.

Raw state/fwinfo are logged only through protected private session logging.
Identifying tail/nonce/UUID/counters/error text are not returned publicly; retained
parser returns two allowlisted identity/privacy booleans. Public result is a
separate state/boolean receipt and explicitly denies MODE/NAK/toggle/MTU1500/product
acceptance. Checkpoints and traceback stay under the protected fresh attempt.
Normal stdout is allowlisted; outer error output contains no exception repr.

## 5. Fresh parent admission — mandatory, not performed here

Source GO covers at most this **one** entry, after all gates, not its launch now:

```text
python3 -B apps/qualification/kernel76-transport/identity21a.py --one-separate-identity-preserve-HOLD21a
```

1. Pin exactly the reviewed candidate/tests, freeze manifest and immutable helper/
   source basis; assertions enabled (no `-O`), no edits during use. The authored
   HOST manifest is a source hash check, **not** an independent GO/admission token.
   Old main entrypoints remain forbidden for this purpose.
2. Actual original failed trial and retirement reader completed; no current lease,
   transfer/GUI/shell/second sender/queued effect or other USB/watchdog consumer.
   Admit image/source plus continuous owned history. PID1-only FDs cannot prove
   global no-consumer ownership; unexplained consumers/queue/reset/HCD/topology
   facts mean NO-GO. Cached sequential descriptors are not an atomic generation
   census or direct all-URB/DMA telemetry.
3. Revalidate actual protected native-retirement checkpoint/completion/receipt,
   full original failed counter/snapshot chain and original HOLD bytes/inode;
   fresh unused attempt outputs. Parent's earlier offline check is dated; exact
   current TX must still equal derived saved Cret, with monotonic retained RX/no
   unaccounted pending input. Never rebase from live counters or consume a new
   effect merely to discover the runtime.
4. Fresh intended adapter/canonical recorder source+argv/private envelope/sole
   owner/original full peer/PID/start, original part/image/boot identity and future
   **unconsumed main-CPU U-Boot catch**. Require >=600s initial capture/catch
   coverage, <=300s total budget and>=300s dynamic/final coverage. Do not infer
   any of these from the old07:09UTC deadline, PID or this report's completion time.
   **If fresh coverage is short, decline.** No weakened floor, rearm/reconnect,
   recorder adoption, silent renewal or recovery action under this operation.
5. Fresh pre-round FD/descriptors/full14 inventory must pass before any active
   query; both UUID reads equal original saved UUID. Strict APPLICATION alone
   admits the one info. Any refusal is sticky; no firmware/RUN repeat,256 fallback,
   toggle repair beyond the two selected target entries, module/association/
   network/media/peripheral/bus reset/watchdog action or persistent write.
6. Complete identity plus full post-round/final source/owner/counter/idle/coverage
   gates may produce **only separate identity+bounded sameboot retirement** with
   original trial FAILED/HOLD byte-exact retained. Preserve all raw/error evidence
   privately and retire only this new host lease. No original receipt promotion,
   memory adoption or RP-02/product completion claim.

Endpoint CLEAR_HALT remains admissible **only after** the failed load interval
ended, prior separate retirement was actually observed and this fresh pre-round
passes. It resets endpoint halt/data-toggle state; identity success is not
reset-free load-interval, MODE/NAK/toggle or MTU1500 qualification.

Watchdog preference remains separate recovery **when actually needed**, with its
own known-state/current-owner/catch gates, not queued here. While a query may be
active, retain ownership/prearmed catch/HOLD; do not inject a second reset sender
or assume a past guard open is still armed after cleanup. SoC watchdog reset is
not cold-radio/rail reset or permission to reload/free uncertain backing/clear
HOLD/auto-boot. RP-04 combined/adoption HOLD and all excluded product gates remain
unchanged.
