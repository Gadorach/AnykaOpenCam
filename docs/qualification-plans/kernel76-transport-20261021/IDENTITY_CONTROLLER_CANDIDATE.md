# Separate identity controller — SOURCE/HOST candidate only

2026-10-21. **Not independently reviewed; execution NO-GO.** Parent alone owns
hardware. This implementation does not queue a target operation. The original
strict trial remains **FAILED/HOLD**; the observed 06:00 read-only retirement does
not repair its interrupted 151st ACK or establish application identity.

Design basis: `SEPARATE_IDENTITY_ENTRY_REVIEW.md`, read completely, including
I1–I4 and sections 5–7. No private/native input was examined by this lane.

## Exact owned changes

- `apps/qualification/kernel76-transport/identity21a.py` — new separate controller;
  SHA256 `c3c19a12bffb56eec2044e259793f58a7243e1ae20ac6fe8cf75ee3f3229ceeb`.
- `apps/qualification/kernel76-transport/test_identity21a.py` — new HOST tests;
  SHA256 `8393989ac89f75100521bfca9fd55932122f7c076b2f56f367a6b0da913ef04b`.
- This candidate report.
- `identity-parent-host01/run_host.py`, `initial01.{json,txt}`,
  `corrected02.{json,txt}`, `final03.{json,txt}`, `final04.{json,txt}`, and
  `result.json` — authored, synthetic/public-source HOST evidence only.

No existing trial/metadata/receipt/retirement/boot helper, central document, Git
index/ref, private file, image or target was changed. No Git command was run.
The final runner rehashed all 26 public inputs of the previous independent audit:
all unchanged. Exact input hashes, argv, Python version and UTC are in each run's
JSON. This report itself is not an independent source-closure decision.

## Candidate implementation

### I1 — original ordered lineage, not a live-counter rebase

`lineage()` walks original recorder events in order from TX zero, pairing every
protected intent with exactly one equal-length completion. Unknown TX kinds,
ordinary/private TX intents, nested/missing/partial/pending completions and extra
TX reject. Cboot/Cpost/Cfailed/Cret must be whole-frame boundaries. The three
bounded length sequences are compared independently, including all three typed
refresh booleans. Arithmetic is `Cret=Cboot+2056+Rp+Rt+Rret`; UART length/order
provenance is explicitly **not historical payload proof**.

`handoff()` never calls `retire.handoff()`/its stale all-postfailure selection.
It uses the original boot/postcheck admission, saved offline failed checkpoint,
original preflight, saved failed snapshot, native
`radio-retirement21a01.private-checkpoint.json`, public retirement receipt and
matching final successful private completion event. It reparses saved rows,
compares original runtime UUID, metadata, full peer/start and exact old HOLD.
It checks the original failure remains failed. Source pins include exact
`retire21a.py` SHA256
`aeda5136b55d3cec920e73adcd41209f52c978b526e4ba1709908a6398caa9d1`.
The final HOST manifest also freezes the candidate/helpers/public source basis.
Only a later guarded parent execution may read those real private inputs.

### I2 — fixed sequence, immutable HOLD and internal wire admission

The sole sequence is cached metadata + full14 RO inventory, one state, only
strict APPLICATION allows one info, then cached metadata + full14 RO inventory.
Each round uses one prechosen nonce, literal 527-byte script/64-byte chunks,
exact echo before CR, one explicit exit, original postcheck schema and UUID.
Completed invalid metadata may receive only its already-framed shell exit;
incomplete/unknown framing receives neither exit nor CR. No post refresh exists.
At most one pre-refresh is allowed after saved returned monitor/empty-buffer
lineage and zero intervening TX, with exact `\nqc07f> ` response.

`Guard` locally subclasses the frozen retirement guard. Its per-instance hooks
remain on `session.check_stream` and `session.call`; inherited real `Boot.send`,
`received` and `wait` traverse them. The complete next-wire/phase program is
enforced inside the actual send RPC, including peer and conditional RX cursor.
Only proven zero-byte refusal retains the same pending slot for inherited retry;
exact success advances TX/index before the next stream read. Partial/ambiguous
transmission freezes permanently. Direct inherited-send bypass and foreign
actions/wires are denied. Active phases also require a terminal monitor prompt
after the latest inherited status read, not just the outer caller's earlier one.

The original lock is opened read-only/O_NOFOLLOW, exclusively flocked, and compared
by lstat/fstat identity plus exact content before/after and through all guards.
No truncate/write/chmod/clear path exists. Real/effective UID/GID must remain the
same unprivileged identity. Owner refresh cannot repin peer/start. The fixed fresh
private attempt directory is consumed before provenance admission; a refused
attempt cannot reuse it. Fresh subordinate transport and exclusive output files
never overwrite originals. Existing public output refuses before any target TX.
No failure path performs target cleanup or fallback.

### I3 — original diagnostics and saved-retirement topology

Fatal scans include original HCD forms, new kernel/U-Boot and explicit
`Process ... called USBDEVFS_(CLEAR_HALT|RESETEP) for active endpoint` from original
boot checkpoint RX. The known failed disconnect remains in that original stream.
Topology scanning begins at **saved retirement RX**, with its incomplete LF record
prepended (maximum1024 bytes), not today's live cursor. All fresh data including
unattended-gap/split-record transitions is checked. Rejected inherited epoch
cursors are restored, with no accepted diagnostic baseline advance.

### I4 — finite host budget and evidence/privacy

Deadline is set once to monotonic now+300s **before first owner admission**.
Initial capture/catch coverage is at least600s; dynamic/final coverage at least300s.
State requires at least140s remaining, info at least130s, with repeated gates at
actual internal send and after blocking reads. Inherited send RPC keeps its10s
reserve. Existing per-query6s, shell6s/echo3s/command10s/exit6s bounds are retained.
Successful totals are exactly **1741/1742 UART bytes and54/55 completed sends**.

State/info use strict retained parsers and command-local unique terminal prompts.
Full raw fwinfo, identifying tail, errors, nonce, UUID and counters stay inside
private output/logs. Public output is allowlisted states/booleans, never raw
identity, its digest, transcript or exception repr. No original HOLD/result can
be promoted by this separate success. No reload/RUN/reset/module/network/media,
target watchdog changes, repair/finally traffic or broader monitor API is used.

## HOST validation and preserved initial failure

Final invocation (not a hardware invocation):

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/identity-parent-host01/run_host.py final04
```

At `2026-10-21T06:30:14.065158+00:00`, Python3.14.7:
**48 methods PASS: 25 unchanged existing +23 new**. The runner selects only the
five inspected unittest modules; it does not run old exclusive-output collectors.
Actual controller `main()` and `handoff()` run only with in-memory filesystem/
transport replacements. Real inherited Boot send/retry/receive/wait logic is used.
No private file, target/RPC/status/serial/network, build or ARM execution occurred.

New tests cover all eight historical refresh combinations; exact event boundaries,
missing/partial/interleaved/extra TX; stale/foreign/missing native checkpoint or
completion; saved metadata/UUID/peer/start/source; actual-main success both refresh
variants; wrong pre-FD/descriptors/idle/UUID (zero active queries); negative,
short/late/extra/interleaved state (zero info); malformed info/post failures;
all CLEAR_HALT/RESETEP split positions in the unattended gap and after strict state
parse before info; disconnect/enumeration/reset split positions; carry overflow;
600/300 coverage, first-owner budget consumption, 140/130 floors, blocking status
reserve/phase checks, direct internal send bypass, zero refusals/exhaustion,
partial/ambiguous freeze, RX rollback, UID/GID/start/peer changes, HOLD/inode/mode,
one-attempt outputs and public privacy.

`initial01` retains **35/36 PASS, one genuine failure**: the in-memory fake reopened
the same byte stream without resetting its file offset, causing JSONDecodeError
on attempted reuse instead of exercising the consumed-directory refusal. Real
fresh `os.open()` starts at zero. The fake was corrected to do so; no controller
check was weakened. `corrected02` passed45 methods. `final03` passed47, followed
by the stricter internal active-prompt fence and additional after-state-parse
split fixtures; `final04` is the current frozen48-method result. Earlier hashes
and logs remain untouched. Method names and outcomes are in the corresponding
TXT files; no assertion-count or native-pass claim is substituted for them.

## Concrete remaining gaps / parent handoff

1. Parent and independent reviewer must inspect the **exact frozen candidate**
   and decide I1–I4 closure. This lane grants neither SOURCE GO nor native GO.
2. Real private checkpoint/event/completion/snapshot provenance has intentionally
   **not** been opened or verified here. Any absent/mismatched field is refusal,
   not permission to create a replacement checkpoint or rebase from current TX.
3. Real current owner/canonical argv/source/sole adapter/catch lifetime/runtime
   admission and no unexplained USB/watchdog ownership are parent-only, fresh
   prerequisites. Historical capture deadlines are not inherited authority.
4. HOST fakes cannot prove electrical timing, endpoint/URB/DMA retirement,
   watchdog-off, actual application identity or target cleanup. Native behavior
   remains untested. USB state/info themselves clear endpoints and may outlast
   the nominal5s target watchdog; no timeout extension or recovery is queued.
5. Original strict trial **FAILED/HOLD**, RP-04 memory HOLD, MODE/NAK/toggle,
   MTU1500, network/media/product and all other excluded gates remain unchanged.
