# Kernel76 finite transport qualification candidates

Host qualification code only until independent source/current-target admission.
No module, firmware, GPIO, reset or network operation happens on import or in tests.
The lab entry uses separately retained private boot/capture tooling; this is not
a self-contained public firmware release or a product network service.

- `radio_receipt.py`: strict74-fragment/151-ACK whole-loader transcript, application
  state/retained firmware identity and source-exact A6 PID1FD parsing. Firmware-info
  identifying bytes are never returned. These parsers do not prove MODE, NAK,
  toggles, RF/network connectivity or MTU1500.
- `metadata21a.py`: exact21a handoff/source binding, read-only cached USB descriptors
  and PID1FD inventory through one owned shell roundtrip. The standalone CLI is
  before-mode only; no unreviewed after-mode remains. A6 `ls -l` omits total/uid/
  gid/date; READLINK applet is disabled but its internal symlink reader is present.
- `trial21a.py`: candidate for exactly one whole native RAM load including its
  automatic CRC-enabled RUN, separately verified application state/identity and
  independent descriptor/FD/logical watchdog/ISP/UART/boot continuity retirement.
  Entire operation retains durable HOLD until all gates pass; no retry,256 fallback,
  GPIO42 reset, module attachment, association or MTU change is implemented.

Tests (no target/RPC or private-capture access):

```sh
python3 -B apps/qualification/kernel76-transport/test_radio_receipt.py
python3 -B apps/qualification/kernel76-transport/test_metadata21a.py
python3 -B apps/qualification/kernel76-transport/test_trial21a.py
```

The16 current unittest methods contain grouped negative cases and full trial-main
in-memory fakes. Metadata admission tests cover receipt/checkpoint/source-binding
failures before import, while trial fakes exercise complete/fault/HOLD paths with
at most one load. This is HOST proof, **not native acceptance**.

Preserved failures: original FWINFO mutant changed no bytes and was corrected;
missing FD_RC formerly raised ValueError and now explicitly asserts; independent
review found overly permissive FD grammar and incomplete handoff binding despite
passing earlier tests. Both were corrected before any native metadata/load use.
The first historical corroboration assumed1185 complete logged ACKs; actual text
has1184 complete and one truncated header, retained as failure evidence. No repair
or dropped-line filtering is used to turn that old transcript into a strict PASS.

A further independent whole-trial review reproduced silent peer repinning and
missed/late HCD-warning rejection. The corrected wrapper freezes the original peer/
start across owner refresh and fences source-real diagnostics before each effect
and after each returned active transaction. New fixtures require zero radio
commands on pre-load warnings/owner loss and no next query after an inter-query
warning. These corrections remain source-review-gated before native execution.

Qualification review/evidence:
`docs/qualification-plans/kernel76-transport-20261021/`.
Execution: `docs/plan/camera-foundation/EXECUTION_TASKS_20261021.md`.
Current candidates are **not admitted for target execution** until the independent
correction/trial review and exact current owner/runtime/recovery/lease gates pass.

Watchdog is preferred for reviewed recovery, not a hidden test side effect. An
already guarded operation may reset autonomously; a deliberate `wdt reset` needs
its own known-idle/catch admission. No reset is used to clear an unknown state
without observed reset/retirement proof; cold radio/rail reset is not inferred.
