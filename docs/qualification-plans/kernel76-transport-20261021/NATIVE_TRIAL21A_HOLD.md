# RAM76 native transport trial21a — strict receipt HOLD

2026-10-21, parent hardware operation. **FAILED strict trial; HOLD retained.**
Source-only closure GO and commit `5edb85b` preceded actual admission. Parent
host gate at05:23UTC verified sole current owner, matching source/peer/start,
exact cumulative TX, no intervening reset/HCD diagnostic, empty operation HOLD
and future capture/catch coverage (6340s remaining at that checkpoint, dated).

Exactly one operation was launched:

```text
python3 -B apps/qualification/kernel76-transport/trial21a.py --one-whole-native-load-and-RUN21a
```

Shell `sh_15cea5441001X0ygn37rDHAm9G` completed with exit1. Fresh target idle,
same-boot, USB descriptors and PID1 console-only FD checks passed before the
only `radio native-load`. No concurrent target sender was used.

## Actual observations and failure

- All74 payload fragments completed:73×2048B (2052B wire), then1552B. The strict
  parser had matched150 complete16-byte ACK records before refusing the final
  RUN record. This is not acceptance of the required151-record whole transcript.
- A USB-disconnect printk interrupted the monitor's multi-write final ACK line:
  `RADIO BOOT RX cmd0xusb 1-1: USB disconnect, device number 2`.
- The loader's `RADIO RAM LOAD/CRC/RUN ACKNOWLEDGED; application/ABI/association
  still require verification` constant was observed, as was a monitor prompt;
  later USB enumeration logging followed the prompt. These do not prove cleanup,
  application identity or a successful strict receipt.
- Failure was `AssertionError` in `radio_receipt.ack`, reached from the final RUN
  check in `parse_loader`. Production parsing/acceptance was not weakened or rerun.
- **No `radio state`/`radio info`, post-load FD/descriptors or idle retirement
  checks were sent.** No retry, fallback256, RUN repeat, reset, module/network/
  association/media/actuator or persistent-write action followed.

Only saved private bytes were examined after completion. The original capture,
failure traceback and a fresh bounded private snapshot are retained outside Git;
no raw capture, boot UUID, identifying firmware-info bytes or private digest is
exported here. One over-specific offline diagnostic assertion initially assumed
the interrupted record began with the complete command number and failed; it was
not a target command or a production acceptance change.

Safe public receipts:
`evidence/qualification/qc07f/execution-20261021/ram76-resume21a/`
`RADIO_PARENT_PREFLIGHT21A01.json` and `RADIO_NATIVE76_21A01_HOLD.json`.

## Next gate, not an automatic queue

Independent source review is assessing a **retirement-only** bounded read-only
follow-up. It must retain the original failure/HOLD, bind exact postfailure owner/
TX/boot continuity, and must not reload firmware or silently turn interleaved ACK
text into PASS. Active USB identity queries and watchdog reset are not admitted
by this failure report. Watchdog is preferred when recovery is needed and its
separate known-state/catch gates pass; a SoC reset is not cold radio/rail cycling.

No functional whole-load/application/retirement PASS, direct MODE/NAK/toggle,
MTU1500, network, combined-memory or product acceptance is claimed.
