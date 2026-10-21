# RAM76 retirement reader — G1/G2 guard closure review

2026-10-21. **CONDITIONAL SOURCE GO for ONE preserved-HOLD retirement-only
reader**, at the exact revision below, after fresh parent admission. G1/G2 from
`RETIREMENT_READER_REVIEW.md` are closed for this revision. No further concrete
source blocker was demonstrated. Earlier NO-GO sources/reports/results remain
preserved, not rewritten as successful tests of their original revisions.

This is **not current hardware admission or native retirement evidence**. Parent
alone owns hardware. No target/RPC/status/serial operation, private input access,
build, ARM execution, production edit, commit/index change or old collector rerun
occurred here. Owned outputs are this report and `retirement-guard-closure-review/`
only. Original native trial remains **FAILED/HOLD**, regardless of any later
healthy read. No active state/info/reset action is admitted or queued.

## Exact reviewed source

| File | SHA256 |
|---|---|
| `apps/qualification/kernel76-transport/retire21a.py` | `aeda5136b55d3cec920e73adcd41209f52c978b526e4ba1709908a6398caa9d1` |
| `apps/qualification/kernel76-transport/test_retire21a.py` | `e669ea5eca0f59799c486226b7309fc24c7341c6e1c84fb2eec57b74958e7eea` |
| `trial21a.py` in that directory | `6d0ce652d3355baef2e5505e9d2a34813aa1f780289b17268ccb160c4748da9c` |
| `metadata21a.py` | `f0a0e48969184d8df441d525777bc4ae7110177eaf81016c8ab746c8338b2376` |
| `radio_receipt.py` | `4b7d9c1887572a6fc7c7138ab3881c8739a00cbf08b0d8a775ded0465af5125c` |

Full public input hashes, unchanged helper/recorder pins, prior preserved source
hashes and exact test argv are in `retirement-guard-closure-review/result.json`.
The previous NO-GO wrapper/test copies match the previous independent result's
hashes. AST comparison proves production wrapper logic **outside `Guard` is
unchanged except the new real/effective identity assertion**. Exact textual changes
are retained in `retirement-guard-closure-review/wrapper.diff`.

## G1 closed — actual inherited send/retry/receive boundaries are guarded

`retire21a.py:86–110,115–137` now:

1. Saves the **original bound** `session.check_stream` and `session.call` before
   installing hooks on **this one session instance**. The saved checker avoids
   self-recursion; no class/global transport or recorder code is patched.
2. Consequently unchanged `Boot.send():415–435` invokes the guard at its internal
   stream checks, including checks after an explicit zero-byte refusal and before
   another attempt. `Boot.wait():404–413` → `received():400–402` also invokes this
   same guarded checker. This is no longer only an outer facade.
3. Guarded stream checks apply local deadline/HOLD/original peer/start both before
   and **after** the saved checker/status read, then enforce catch/TX-idle, full
   original HCD/new-boot scan and follow-up topology checks.
4. The per-instance `call` accepts only status/send, checks local gates before and
   after the existing blocking RPC, and requires **more than10s** remaining before
   requesting a mutating send. `Guard.send` also reserves10s. The unchanged private
   transport still translates send to protected send, preserves kernel peer
   checking and conditional RX watermark, and never retries a partial write.

Independent old-shape G1 reproducers now refuse **before any simulated byte** for
deadline, catch, HCD and HOLD transitions. Independent real `Boot.status`,
`check_stream`, `send`, `wait` and `received` composition probes likewise refuse:
six injected faults (deadline/catch/HCD/HOLD/new kernel/topology), both direct send
and after one zero-byte refusal, plus six wait/receive placements. No second send
is attempted after a zero-byte refusal followed by one of those faults. Healthy
real-helper paths still work; five unchanged zero-byte refusals remain bounded,
and one partial transmission is never replayed.

**Bound interpretation remains honest:** the150s budget is enforced at these
host/RPC boundaries, with a10s existing-RPC overhead reserve. It is not an
electrically atomic target-write deadline or a guarantee against arbitrary host
scheduling/driver stalls. A missing/late/partial reply still preserves HOLD and
blocks continuation; no new transport protocol or timeout extension was introduced.

## G2 closed — incomplete topology record is retained across the cursor

`retire21a.py:91–96,120–123` reads the original interval through `settled_rx`, keeps
its last incomplete LF-delimited line, and refuses a carry longer than1024 bytes
**before installing hooks or sending anything**. The carry is prepended to all
fresh bytes for topology matching. The formerly missed `USB discon`/`nect...`
record now matches conservatively; no line is repaired into an ACK or filtered
out of a receipt. Existing full-original HCD/new-kernel checks remain intact.

Supplied fixtures deny disconnect/new-device/reset-device messages at **every
split position**. Independent tests accept a nonmatching1024-byte carry and
refuse1025, leaving the instance unhooked on refusal. This carry is only a boundary
aid: a complete transition in the new interval is also rejected, and an oversized
or otherwise contradictory initial boundary is not permission for refresh/retry.

## Previously reviewed scope and admission binding remain unchanged

- **HOLD/one attempt:** same exclusive nonblocking operation lock; exact original
  failed-operation HOLD content is retained and never written/truncated/cleared.
  A fresh private session output directory prevents another attempt after the
  first session is instantiated. Receipt/checkpoint outputs are exclusive.
- **Failure endpoint, not current-status rebase:** original boot/postcheck source
  derivation, pinned helpers, explicit offline failure checkpoint, saved parent
  preflight and exact recorder intent/completion lengths are reconciled. Before
  target TX the saved snapshot is compared to its exact original recorder slice.
  Formula remains `Cfailed = Cboot + 1197 + Rp + Rt`. Parent reports Rp=Rt=True,
  hence Cboot+1199 and trial delta878B/28 completed sends. These private actuals
  were **not accessed/independently verified by this reviewer**; the reader must
  recheck them and require exact current TX, not manufacture a new baseline.
- **UID/GID:** main now rejects real/effective UID or GID mismatch before opening
  any file. Parent explicitly asserts this session launched original21a through
  unchanged `start_receiver`, unprivileged, with no sudo/newgrp/user change. That
  supplies the historical launch fact conditional in the preceding review. The
  pinned launcher inherits credentials and recorder has no later credential/exec
  path. Expected `(original PID, same parent UID, same primary GID)` is therefore
  source-supported with the asserted launch continuity and live original start/
  kernel-peer gate. It is **derived**, not falsely described as a saved old tuple.
- **Fixed sequence:** one2s settle; at most one exact fresh empty monitor refresh;
  one pinned527-byte nonce-framed cached FD/descriptors shell line with explicit
  shell exit/reap; then the existing14-query/320-byte RO idle inventory. Same
  original private UUID, descriptor/FD/idle schemas and exact counters are required.
  All command-body/prompt checks remain. Completed invalid metadata can exit then
  refuse; an incomplete shell command does not receive appended exit/extra CR.
- **UART budget:** successful read is **859 bytes**, or **860** with that single
  refresh. No radio USB command/ioctl/endpoint repair, firmware load/RUN/fallback,
  watchdog action, module/network/media/peripheral/persistent-write or kill path.
  Old trial/metadata/postcheck `main()` entrypoints remain NO-GO for this purpose.
- **Evidence meaning:** a healthy result is only a separate bounded cached
  userspace/FD/idle retirement observation on the same boot. No exposed all-URB/
  DMA/MODE/NAK/toggle counters exist; logical watchdog inactivity is not electrical
  stop proof. The original interrupted final ACK remains unaccepted and immutable.

## Fresh parent gates — all still required before this one actual reader

1. Select and pin exactly the reviewed source, normal assertions enabled (no `-O`),
   no edits during execution. Actual failed trial/host lease completed; no other
   UART sender, transfer, GUI or queued operation. Parent alone owns admission.
2. Revalidate intended adapter, sole recorder source/argv/private envelope and
   original PID/start/kernel peer; same asserted original unprivileged parent
   identity and real/effective equality. Historical PID/deadline/catch are not live
   authority. Require fresh future/unconsumed original-main-CPU catch and >=300s
   capture/catch coverage at initial admission, >=150s final coverage,150s reader
   budget and its10s send reserve. No automatic catch rearm/reconnect/replay.
3. Confirm exact original failed HOLD, no unexplained lock/source/owner changes,
   fresh unused reader output paths, offline counter/snapshot proof, exact current
   derived failure-endpoint TX and no pending/unaccounted input. Original boot/
   image and full retained RX continuity must agree; missing facts are NO-GO.
4. Apply full original interval error/reset checks and the conservative carry/new
   interval topology gate. Only the one empty refresh is eligible after verified
   returned own monitor/empty input, never to flush an unknown partial command or
   suppress continuing logs. Any rejection ends this attempt with HOLD retained;
   do not retry the query/reader, repair endpoints or inject reset.
5. Retire only this read lease and record the allowlisted separate result. Never
   clear original failed trial HOLD, overwrite its receipt or claim native full
   load/application identity/PASS from these healthy cached observations.

Source GO covers at most:

```text
python3 -B apps/qualification/kernel76-transport/retire21a.py --one-readonly-retirement-preserve-HOLD21a
```

This report does not launch it. Parent must first satisfy the fresh gates and
select that one attempt; none are observed here. Later active state/info queries
or watchdog reset need separate known-idle/source/runtime admission and are not
queued. If a SoC watchdog reset is ever separately selected/observed, it is **not
cold-radio/rail recovery**, normal cleanup proof or permission to repeat load/RUN.
Combined-memory/adoption/product gates remain unchanged/HOLD as applicable.

## Reproducible HOST evidence

Executed once, completed05:55:29UTC, Python3.14.7:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/retirement-guard-closure-review/review_host.py
```

- **25 supplied HOST methods PASS**, including actual-main HOLD/counter cases,
  eight real-Boot direct/zero-refusal faults, timeout reserve and all topology
  splits. Supplied output: `retirement-guard-closure-review/supplied-tests.log`.
- **40 independent checks PASS:** old G1 reproducer transitions now block with
  zero bytes;12 real-Boot send and6 wait fault cases; healthy helper controls;
  bounded zero-byte refusals/no partial replay; carry bounds; actual-main
  no-refresh branch/HOLD unchanged; real/effective identity mismatch; unchanged
  surrounding AST, sources and earlier evidence. All simulated transport is in
  memory, with no private or target access. A helper-level healthy wait control is
  not substituted for the actual reader's fresh command/nonce/prompt schemas.
- Exact input/script hashes, argv, UTC and individual outcomes:
  `retirement-guard-closure-review/result.json`. Original helper/recorder and
  historical NO-GO source/report bytes rehash unchanged.

**Native retirement remains UNOBSERVED here; original strict trial FAILED/HOLD.**
