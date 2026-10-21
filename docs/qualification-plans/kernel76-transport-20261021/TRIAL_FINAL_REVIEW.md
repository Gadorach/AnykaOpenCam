# RAM76 resume21a — final source re-review

2026-10-21. **NO-GO for the exact current whole functional trial: residual C2.**
C1 and C3 are corrected; the previously identified host-side C2 strings are
covered, but three additional **compiled MUSB-core diagnostics** still evade the
denylist and permit a false PASS/cleared HOLD in the full in-memory main.

Scope: this new report and fresh `final-review/` only. No production edits,
Git/index/commit changes, RPC/status/target/network/private-input access, build or
firmware execution. Original `TRIAL_CORRECTION_REVIEW.md`, metadata review and all
their failure/evidence files were preserved byte-exact and rehashed. No target
operation occurred, and the parent's observed boot/postcheck PASS is not disputed.

## Corrections verified

- **C1 closed:** `trial21a.py:112–130` freezes the first peer tuple/checkpoint
  start, requires identical tuple/start after the second owner refresh, then exact
  part/catch-armed/no-busy facts and existing cumulative TX check. Changed PID,
  changed UID in the tuple, lost catch, foreign part or busy refresh all refuse
  before any radio command in the independent in-memory probes.
- **C3 closed:** `clean_diagnostics()` scans the entire owned interval;
  `monitor():37–49` requires its start for every active native-load/state/info
  command and fences before and after each returned transaction. There is also
  an explicit pre-load fence and the final fence. Named pre-load warnings now
  yield zero radio commands; a warning after load prevents state, and a warning
  after state prevents info. No active loader is interrupted or retried.
- **Named C2 forms closed:** packet-ready, broken rx_reinit, bogus ep0, missing
  URB, DMA-busy, RX interrupt, active suspend, flush, kernel WARNING/BUG/Oops/panic
  and the original USB error/warning/timeout forms are matched. No warning is
  suppressed or normalized into success. B1/B2 fixes remain intact; metadata is
  standalone before-only.

## Concrete remaining C2 blocker

`HCD_DIAGNOSTIC` at `trial21a.py:20–24` still requires literal error/warning/timeout
for most MUSB-prefixed messages. The following candidate-core messages contain
none of those words and none of its other alternatives:

| Compiled source | Public source-shaped console form missed |
|---|---|
| `musb_core.c:667–695` | `musb_handle_intr_resume 692: bogus host RESUME (a_host)` |
| `musb_core.c:942–983` | `musb_handle_intr_disconnect 980: unhandled DISCONNECT transition (a_wait_bcon)` |
| `musb_core.c:990–1002` | `musb-hdrc musb-hdrc.0.auto: Babble` |

The first two are `WARNING()`; `musb_debug.h:13–18` prints function+line+message,
not the literal word “warning.” The third is a host-active `dev_err()` immediately
followed by the driver's babble recovery path. These are not normal enumeration
lines and must not be treated as a clean transport interval.

All three format strings were independently found **as bytes, without execution**
in the manifest-pinned candidate76 linked kernel. Its hash matched the package
manifest. The candidate is `USB_MUSB_HOST=y`, `CONFIG_PM=n`; the findings do not
depend on excluded gadget or PM code. IRQ RESUME handling is not the same as the
excluded runtime-PM callbacks.

Each was separately injected into the existing host Fake's complete owned
pre-load metadata-exit response. The actual production main/helpers/denylist were
unchanged. For **each** injected diagnostic the actual main still:

1. sent one native-load followed by state and info;
2. accepted post-inventory and wrote PASS;
3. cleared its HOLD.

The owned raw interval contained the diagnostic. This is a reproduced acceptance
gap, **not an observation that hardware emitted these messages**. Fresh E2
owner/runtime/descriptor/FD/catch checks cannot repair a denylist that later
accepts an HCD error. The current public
`no_new_HCD_error_FIFO_warning_observed_in_owned_interval` claim would be false
for these cases.

**Narrow closure:** add these three source-backed core forms to the finite
diagnostic predicate and full-main fixtures requiring HOLD, no PASS and **zero
radio commands** when seen before load; retain inter-query/no-next-command and
final fences. A small pattern for each source format is sufficient. No blanket
kernel-log suppression, new driver/image, instrumentation, generic framework or
additional target experiment is requested. Do not invoke any recovery to test
these cases on hardware.

## Host tests, evidence and unchanged limits

Executed once with Python3.14.7; exact UTC timestamps/argv/input pins/output hashes
are recorded in `final-review/result.json` and the three `test_*.py.log` files:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/final-review/review.py
```

- Supplied **16 methods PASS**: metadata5, receipts5, trial6.
- Independent full-main Fake-only probes confirm C1/C3/named-C2 refusals, plus
  the three remaining C2 false acceptances. All inputs/file/OS/transport/private
  admission capabilities stay in memory; the public postcheck module imported
  by the test has definitions only. Healthy flow still performs one load.
- Synthetic historical28-hex header truncation, missing ACK, negative/short
  numeric length and nonzero ACK status all refuse. No ACK is reconstructed,
  dropped or silently repaired. The complete151-ACK/74-chunk grammar is retained.
- Reviewed production/helper sources were unchanged throughout the run. Prior
  reports and failures are retained, not overwritten by a corrected verdict.

No other new blocker was demonstrated in this pass. The positive source review
of the fixed metadata shell lifecycle, image binding, full native-load/RUN entry,
strict application identity and bounded retirement checks remains as described
in the earlier reports. It does **not** override residual C2 or missing fresh
hardware gates.

After that source closure/re-review, the candidate operation remains narrowly
**one whole functional native-load with automatic CRC-enabled RAM RUN**, then
the existing state/info and strict metadata/idle retirement sequence. Direct
MODE/NAK/toggle/MTU1500 proof stays false; radio/product capabilities may remain
unqualified. Failures remain HOLD: no retry,256 fallback, RUN repeat, module/
network/flash/card/media/actuator effect or watchdog/GPIO reset is authorized by
this review. Actual E2/owner/catch admission belongs to the parent, not this lane.
