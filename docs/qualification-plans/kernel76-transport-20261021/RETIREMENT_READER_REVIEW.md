# RAM76 preserved-HOLD retirement reader — independent implementation review

2026-10-21. **NO-GO for the current reader revision.** No target action is admitted
by this review. The retirement-only operation remains a reasonable conditional
source scope, but two concrete guard implementation gaps below need correction
and re-review before this particular wrapper is used. Parent alone owns hardware.

Owned outputs: this report and new `retirement-reader-review/` only. No production
edit, commit/index change, target/RPC/status/serial access, private input access,
build, ARM execution or old exclusive-output collector rerun occurred here. All
reviewed inputs remain unchanged. The original native trial remains FAILED/HOLD;
no new result accepts its151-ACK receipt or admits active identity/reset actions.

## Reviewed revision and evidence

| File | SHA256 |
|---|---|
| `apps/qualification/kernel76-transport/retire21a.py` | `daa04cd5e5bb8da38a0e87b4975d3c5c49a727ecb4529a9607d64301bbe9b275` |
| `apps/qualification/kernel76-transport/test_retire21a.py` | `15986c1c2447d767116b39a86f363cfdcabc8d6d5a0d4df799e071d0a7259b1a` |

Executed once, Python3.14.7, completed05:44:43UTC:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/retirement-reader-review/review_host.py
```

`retirement-reader-review/result.json` records exact argv, UTC, Python version,
all public input/script hashes, **22 supplied HOST methods PASS**, four independent
actual-`Boot.send` gate-bypass probes and the topology-tail probe. Supplied output
is preserved in `supplied-tests.log`. Socket construction, subprocess run/Popen
and unmocked private-path access were denied during the supplied suite. Probe
transports/files/clocks are entirely in memory. No collector opens a private
checkpoint/snapshot or creates a real transport. Synthetic wire transmission below
means appending a byte to a Python list, **never hardware TX**.

The reported earlier regex-anchor and deepcopy fixture failures remain historical
parent evidence; this review neither deletes them nor treats the corrected host
suite as native evidence.

## G1 — blocking: nested send checks bypass the retirement guard

Anchors: `retire21a.py:91–116`; `scripts/qc07f_boot.py:404–435`.

`Guard.send()` calls the full guard once at107, then invokes the **session-bound**
`self.session.send()` at108. The inherited `Boot.send()` subsequently calls
**session** `check_stream()` at419, not `Guard.check_stream()`. That base checker
tracks reset/transfer state; it does not apply this retirement wrapper's deadline,
catch, HOLD, HCD diagnostic or topology checks. Its up-to-five compare-before-send
zero-byte-refusal iterations also stay inside the unguarded session method.

Consequently a new gate failure received/discovered by the inner status check can
be followed by another fixed reader byte. The conditional-send RX watermark does
not repair this: the inner status can consume the new diagnostic and submit its
**updated** watermark, so the recorder can legitimately accept that send. A later
guard check fails, preserving HOLD, but it is too late to enforce the stated
no-further-TX boundary. This does not mean any radio command is sent or a healthy
receipt is falsely accepted; it means the proposed one-attempt guard is bypassed
at the actual sending method's own checked boundaries.

Independent probes use the unchanged real `Guard.send` **and inherited real
`Boot.send`**, with only state/RPC return values and clock replaced in memory:

| Gate changes at the inner `Boot.send` status check | Observed simulation |
|---|---|
| Clock advances past150s to151s | One byte transmitted after deadline; next guard check refuses |
| Catch becomes false | One byte transmitted; next guard check refuses |
| New actual-format MUSB Babble diagnostic appears | One byte transmitted; next guard check refuses |
| Held-file content changes | One byte transmitted; next guard check refuses |

These are injected gate transitions, not claims that all occurred on this camera.
In particular a genuine catch consumption normally accompanies reset text, which
the base checker may independently reject. **Deadline/HCD cases alone demonstrate
the blocker** without assuming an unreviewed catch action or competing lock user.
A status call/scheduling delay near the150s edge is sufficient for the deadline
issue; no second sender is needed for a kernel diagnostic.

There is a related budget detail: `Guard.check_stream()` checks time **before**
the underlying status call at95/98, not again after that potentially blocking call.
`Guard.wait()` caps its requested wait but delegates to session-bound receive/status
methods, so this is not a hard150s cap on nested transport activity. An outer
post-wait check is useful rejection, not proof no byte was sent after a changed
gate. The initial300s/final150s coverage reserve does not implement the missing
per-send checks.

**Small bounded correction needed:** make the existing reader's actual internal
stream-check/send/retry boundaries pass through the retirement gate, preserving
the existing protected conditional-send and zero-partial-replay behavior. Keep a
saved unwrapped checker if using a per-instance adapter; avoid recursion or
globally patching shared helpers. Recheck the deadline after blocking state reads
and before requesting a mutating send; reserve the existing finite RPC overhead.
This asks for a tiny wrapper correction, not a new recorder/kernel/transport
framework or an electrically atomic deadline guarantee. Merely checking once
again **after** `session.send()` does not close G1.

Regression must exercise real `Boot.send`, including a zero-byte refusal followed
by an intervening HCD/deadline failure, and require **zero subsequent simulated
transmitted bytes**. The current supplied main fixture replaces `Session.send`
(`test_retire21a.py:57–62`), so its13 main fault cases do not cover this composition.

## G2 — blocking for the stated topology guarantee: scan loses boundary tail

Anchors: `retire21a.py:100–103,134–145`.

The original-postboot HCD/new-kernel scan uses the full original interval; that
is good and does not have this particular new cursor problem. However the
topology scan starts **exactly** at `settled_rx`, captured from current RX before
the2s wait. No carry-in tail or complete-record-boundary requirement is applied.

Reproduced entirely in memory:

```text
bytes already present at settled_rx: usb 1-1: USB discon
bytes arriving afterward:           nect, device number 3\n
```

`TRANSITION` matches the full disconnect line, but not the suffix after the cursor.
Two successive real `Guard.check_stream()` calls accept it. Thus a transition
completing after that boundary can escape the claimed follow-up transition gate.
An initial partial line is permitted by the current wrapper; its optional empty
refresh does not establish that the previous boundary was complete.

**Small bounded correction needed:** retain/scan the carry-in incomplete line
(or fail closed unless the actual captured boundary is a proven complete
record/prompt). Attribute a crossing transition conservatively to the follow-up;
do not discard/filter it to obtain acceptance. Add split-position fixtures. No
extra target command, second metadata attempt or log-suppression policy is needed.
If parent instead changes the intended interval to start after a separately
validated settled boundary, that change needs explicit source review and honest
receipt wording; the current output claim is not justified by this implementation.

## Source-positive findings retained

These are source findings, not observed target results:

- **HOLD:** main takes the same nonblocking lock, requires the exact failed-operation
  JSON object, retains its read content, never writes/truncates it, and checks it
  repeatedly. Neither healthy nor failed reader paths clear original HOLD. The
  original writer emits ordinary LF/ASCII; text-mode comparison is appropriate to
  that exact known file, not a general arbitrary-byte file authentication scheme.
- **One attempt:** `PrivateBoot` requires a fresh private output directory and
  `Boot.__init__` creates it exclusively. Once instantiated, it blocks a second
  attempt without deleting evidence. New private/public receipts are exclusive.
  Failures before instantiation have sent no target bytes.
- **Counter proof:** `handoff():58–81` pins trial/meta/receipt, invokes
  `meta.admitted_handoff()` for original boot/postcheck binding, reads the explicit
  offline failure checkpoint and saved parent preflight/events, then compares the
  saved snapshot to the corresponding actual recorder slice **before target TX**.
  `reconcile_counter():40–55` enforces:

  ```text
  Cfailed = Cboot + 1197 + Rp + Rt
  ```

  It compares complete intent and completion length lists, rejects other TX-event
  kinds, and requires the known recipe counts/lengths. All four boolean variants
  and supplied negative mutants pass their HOST expectations. For parent-reported
  Rp=Rt=True this is Cboot+1199, with878 bytes/28 completed sends after the original
  postcheck checkpoint. Actual private values were **not** verified by this lane.
  Protected recorder events contain lengths, not TX payloads: this is source/
  ownership/counter reconciliation, not independent cryptographic proof of every
  transmitted command's content. Do not claim it reconstructs or accepts an ACK.
- **No status rebase:** current TX must match that derived failure endpoint;
  saved snapshot length/end and actual raw slice are compared. It is not merely
  copying current status into a new checkpoint. The original missing success-path
  checkpoint is addressed by the explicitly named offline-derived failure record.
- **Scope:** only one optional empty refresh, one existing fixed cached metadata
  roundtrip and14-query idle inventory are called. There is no radio state/info,
  firmware load/RUN, usbfs ioctl, watchdog action, fallback, repair or kill entry.
  The fixed shell line uses cached proc/sysfs data and no background commands.
- **Return/schema:** existing metadata nonce/echo/completion and shell-exit/reap
  framing remain; completed invalid metadata exits then refuses before inventory,
  while partial command completion does not append `exit`. Inventory uses the
  pinned body/schema parsers, original private UUID and final duplicate UUID.
  Successful UART delta remains859 bytes, or860 with the one refresh.
- **Continuity and result limits:** original full postboot interval is retained
  for HCD/new-kernel checks. The corrected finite substring boot-marker regex
  catches kernel text immediately joined after a prompt. Final owner refresh is
  compared to the expected peer/start; receipt preserves original failure and
  explicitly denies all-URB/DMA/electrical-watchdog/original-RUN-ACK proof.

## Original UID/GID derivation — conditional source support, not invented history

The old boot checkpoint did **not** save UID/GID. This report does not invent an
old tuple or obtain credentials from private data/current status.

Independently inspected pinned recorder
`work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py`, SHA256
`135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00`:

- Entire322-line source has no host credential-changing syscall, subprocess,
  host `exec` or re-exec path. UART boot text/credential-provisioning names are
  target protocol features, **not host process credential changes**; protected
  capture rejects its target credential-redaction path anyway (`:269–280`).
- `scripts/qc07f_boot.py:start_receiver():226–244` hash-checks that exact recorder,
  then starts the Python script with `Popen`, stdin/stdout/stderr and
  `start_new_session=True`. No `user`, `group`, `extra_groups`, `preexec_fn`, shell
  or privilege-transition option exists. This initial launch is an exec as normal;
  there is no later recorder exec/credential path. New session changes session
  identity, not UID/GID. Actual Popen keyword names are in the result artifact.
- Initial receiver admission checks original child PID and UID against launcher
  `os.getuid()` at260, and stores PID/start. Subsequent owner checks enforce that
  UID and exact PID/start. Socket peer checks use kernel SO_PEERCRED; protected
  calls preserve expected peer before any mutating request.

**Assessment:** deriving expected peer as `(original PID, same unprivileged
parent UID, same primary GID)` is source-supported **conditional on the parent's
asserted actual original unprivileged same-identity launch**. Require real/effective
UID/GID equality and unchanged invoking identity, same original PID/start, pinned
argv/source, and exact current kernel peer comparison at fresh parent admission.
Do not generalize this to a sudo/setgid/newgrp/different-user launch: source alone
does not prove historical invocation credentials. If the parent cannot establish
that launch fact, identity remains a concrete NO-GO; current credentials alone
must not be described as a saved historical tuple. Given that explicit supplied
launch fact and these gates, missing saved UID/GID is **not an additional mandatory
code blocker** in this review. No live credential facts were queried here.

## Handback and re-entry criteria

1. Keep target untouched and original failed trial/HOLD intact. Correct only the
   bounded wrapper/test gaps G1/G2 in a separately owned source increment; preserve
   this NO-GO revision/evidence. No target retry or private-byte reconstruction.
2. Re-review the exact corrected revision and real nested send/retry composition.
   A future SOURCE GO would still require parent fresh original owner/source/peer/
   start/counters, no competing lease/sender, failed-HOLD identity, current catch/
   lifetime, original sameboot/snapshot continuity and fixed one-attempt admission.
3. At most the retirement-only859/860-byte reader could then be admitted. A healthy
   result remains a separate bounded cached retirement observation, never acceptance
   or clearance of the original native151-ACK trial.
4. No active state/info query or watchdog reset is queued/admitted here. They need
   separate later known-idle/source/runtime gates. SoC watchdog reset, if separately
   selected and genuinely observed, is not cold radio/rail recovery or proof of
   normal cleanup, and never licenses firmware/RUN replay.
