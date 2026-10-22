# Idle watchdog reset22b — controller FAILED/HOLD; actual recovery observed

2026-10-22. PLAT-06 / RP-00–01; original radio failure remains unchanged.

## Actual result, not a relabelled PASS

One `reset22b.py --one-idle-reset-stop-at-uboot` from locally committed source
`cfa39ea` completed with **FAILED/HOLD**. Source line81 rejects any occurrence of
`Please enter password:`. The complete nominal1s watchdog-arm message and one
fresh genuine main-CPU U-Boot banner were captured, then that check stopped the
controller. No reset was resent, no rescue text was sent, and no reload followed.
The frozen controller, plan, private error and new subordinate HOLD are retained.

Subsequent separately bounded **receive-only reconciliation**, under the same
sole recorder/source/start and both held operation locks, established:

- Original board5/DRAM64 U-Boot and actual held `anyka$` tail; no kernel handoff.
- One10B reset command and three19B audited automatic catch packets; private TX
  intents/completions and cumulative counter agree. No other manual TX.
- Catch consumed/unarmed; a later target effect needs a fresh reviewed arm and
  new phase admission. No operation/transfer remains active.
- At12:12:25UTC, an additional10-second passive interval had no RX/TX changes,
  repeat banner or lost held prompt. Old Linux runtime/boot/FD authority is
  invalidated by the observed CPU reset, **not** proved normally closed.
- The same password text also exists **before the held prompt in the successful
  original22b cold catch**. This points to an overbroad transcript classifier, not
  evidence that a held prompt was absent. No parser was relaxed or trial retried.

The later stability observation does not retroactively satisfy the original
controller's uninterrupted timing/acceptance sequence. Keep its result FAILED/
HOLD, alongside the separate fact **recovered to observed stable held U-Boot**.
Neither controller HOLD nor original radio HOLD is cleared by this document.

## Evidence and current boundary

Public scalar receipts in `execution-20261022/ram76-resume22b/`:

- `WATCHDOG_SMOKE22B01.json`: normal5s smoke/sameboot postflight PASS12:05UTC.
- `WATCHDOG_RESET22B_RECONCILE01.json`: failed-trial receive-only reconciliation.
- `WATCHDOG_RESET22B_STABILITY01.json`: later10s stable held-prompt observation.

Private capture/intent/error/boot UUID/raw transcript remain in the protected
unattended envelope outside Git; no private digest or device label is exported.
The subordinate marker remains
`{"state":"HOLD","operation":"one_idle_watchdog_reset22b_stop_at_uboot"}`.
Existing22b boot/postcheck/normal-smoke controllers must **not** be reused now:
they name the invalidated Linux epoch or unconsumed-catch assumptions.

No radio rail cold cycle, active-DMA retirement, electrical timeout calibration,
full watchdog fault policy or G8 acceptance is claimed. Parent-only future native
work requires a separately reviewed recovery handoff preserving this failed HOLD;
no automatic reboot/RAM restore is queued. Source/offline builds continue.
