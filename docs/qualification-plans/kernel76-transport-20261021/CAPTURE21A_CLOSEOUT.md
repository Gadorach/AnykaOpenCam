# Capture21a expiry / host-only closeout — 2026-10-21

**No current UART receive owner, control socket or armed receive/catch recovery
was observed at07:24UTC.** The old bounded21a capture expired; this is not a new
runtime proof or authorization to reset, restart capture or reuse old counters.

Parent performed one host-only metadata check after expiry:

- Exact old recorder PID/start no longer alive; intended adapter present and
  actually unowned by `fuser`; old control socket absent. No competing operation
  lock holder was found. Original failed-trial HOLD remains intentionally nonempty.
- Private final manifest plus both raw/event files verified in full for size and
  SHA256. Raw data, final argv/adapter identifiers and all private digests remain
  outside Git; no private material was exported.
- Final TX equals the separate identity completion checkpoint: no later target
  command was sent. Saved final tail has **no extra RX bytes** after that checkpoint.
  No new diagnostic/reset/topology marker was present in the recorded tail; this
  does not turn silence into power/current-runtime proof.
- No UART open/TX/RPC, target/network access, device reconfiguration, reset,
  capture renewal or owner takeover occurred during closeout.

Safe receipts under
`evidence/qualification/qc07f/execution-20261021/ram76-resume21a/`:
`CAPTURE21A_FINALIZED_HOST_METADATA01.json` and `CAPTURE21A_TAIL_AUDIT01.json`.

Last directly verified target state remains **06:43UTC RAM76/radio APPLICATION,
sameboot idle/retired, no station module or association**, with UART physically
connected at the user handoff. Current target power/runtime and watchdog state
have not been queried. Original151-ACK trial remains FAILED/HOLD; separate identity
and retirement evidence remains valid only for its recorded bounded operation.

All current work is SOURCE/HOST only. Later hardware requires fresh source,
physical/runtime/sole-owner/private-capture/armed-recovery admission; neither
the old07:09UTC bound nor the historical PID/IP is authority. Watchdog remains the
preferred separately admitted recovery when needed, not an automatic action in
this closeout. There is no armed watchdog/catch claim or target command queue.
