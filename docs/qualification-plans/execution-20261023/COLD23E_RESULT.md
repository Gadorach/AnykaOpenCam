# Cold23e v4 — pre-ON UART prerequisite FAILED/HOLD

2026-10-23; BASE-01/05/07 / PLAT-06/08. One fresh v4 invocation was selected
after independent ordinary source review. Initial controller GET reported OFF.
The recorder received2 unexpected bytes; the transaction refused before ON.
Actual fields: ON_attempted=false, OFF_attempted=false, OFF_readback_proved=true
(initial GET), capture_finalized=true, owned_child_returncode=0. No UART TX.

Private final raw/event hashes matched. One arm was recorded, but no banner/
catch triggered and no power-changing request was sent. Saved event timestamps
place the2-byte RX event about13.8ms after open and49.7ms BEFORE the arm event.
That establishes saved event order, not a physical noise/stale-buffer diagnosis.
The bytes are not explained by the expected cold profile and are NOT dropped,
normalized, echoed, published or accepted as harmless. Source/guard unchanged.

The new failure-phase field is coarse: it retained initial_GET because that was
the last power phase before subsequent pre-ON checks, not because GET failed.
The recorded initial_GET_value=false was accepted. Separate saved RX/event
evidence localizes this refusal to pre-ON UART admission. V4's moved ON wait has
NOT yet been exercised; no claim that asynchronous-dispatch hypothesis is fixed.

Fresh independent GET after exit again reported OFF. Recorded child task/socket
absent, no visible tty owner. No electrical rail or Linux state claimed. Both
original radio/reset HOLDs and23c/23d/23e failures remain independently preserved.

Parent separately selects ONE nominal120s receive-only observation as fresh23f,
with exact original pinned recorder, explicit already-associated UART and the
mapped outlet left OFF. This is investigation, NOT a cold-catch retry. Hold the
same read-only historical locks/immutable inputs, fresh private0700/0600 output,
actual owned child wait/final hashes/postchecks; no RPC, arm, TX or power change.
No filtering/flush-after-open/quiet-prefix discard/guard change is selected.
Any RX remains evidence, not native admission. Further hardware needs disposition
of these unexplained bytes; no ON/boot/motor/flash/native effect is queued.
