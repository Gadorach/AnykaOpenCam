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
- `retire21a.py`: separate postfailure **retirement-only candidate**, with
  conditional source GO. Reconciles original boot/postcheck/trial TX from saved completion
  events and snapshot; reads cached metadata plus full idle inventory while
  preserving the exact original nonempty failed-trial HOLD. No radio or reset
  command, retry or successful-trial reinterpretation is part of this reader.
- `identity21a.py`: separate one-state/conditionally-one-info candidate after the
  native cached retirement, pending independent controller review. Exact historical
  event boundaries/native checkpoint/completion, immutable HOLD, internal wire/TX/
  phase ledger, active-endpoint warnings and saved-retirement RX topology checks
  gate pre-RO → state → conditional info → post-RO. No reload/RUN or reset path.

Tests (no target/RPC or private-capture access):

```sh
python3 -B apps/qualification/kernel76-transport/test_radio_receipt.py
python3 -B apps/qualification/kernel76-transport/test_metadata21a.py
python3 -B apps/qualification/kernel76-transport/test_trial21a.py
python3 -B apps/qualification/kernel76-transport/test_retire21a.py
python3 -B apps/qualification/kernel76-transport/test_identity21a.py
```

The48 current unittest methods contain grouped negative cases and full trial/reader-main
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
warning. Further MUSB-core bogus RESUME/unhandled DISCONNECT/Babble cases now
also require zero radio commands before load. Independent closure review returned
conditional source GO; current hardware admission remains mandatory.

Qualification review/evidence:
`docs/qualification-plans/kernel76-transport-20261021/`.
Execution: `docs/plan/camera-foundation/EXECUTION_TASKS_20261021.md`.
`CORE_DIAGNOSTIC_CLOSURE_REVIEW.md` closes the preserved source NO-GOs for the
pinned corrected candidate. Exactly one whole functional load/RUN plus state/info
and retirement is conditionally source-approved, **not yet natively accepted**.
That one trial ran and FAILED strict receipt at an interleaved final RUN ACK;
original HOLD remains. No rerun is authorized. See `NATIVE_TRIAL21A_HOLD.md`.
`FAILED_TRIAL_FOLLOWUP_REVIEW.md` conditionally supports one retirement-only
sequence, not the old entrypoints; the new reader's implementation review and
fresh current owner/runtime/recovery/counter gates must pass before use. See
`RETIREMENT_GUARD_CLOSURE_REVIEW.md` for the exact approved revision/scope.

Retirement HOST failures retained in the parent host record: an anchored new-boot
regex missed a message joined after a prompt (finite substring matching fixed it);
a counter-test fixture tried to deepcopy a module (excluded from the copy). Neither
ran on target or changed original trial acceptance.

The reader's first independent implementation review found nested `Boot.send`
checks bypassing the outer guard and a topology record split across its RX boundary.
The NO-GO revision is preserved under `retirement-parent-correction01/before/`.
Correction hooks only this session's checker/RPC boundaries (saved unwrapped methods
avoid recursion), reserves the existing RPC timeout before send, and scans the
carry-in incomplete line. Tests now exercise the real inherited send/retry method
and every split of disconnect/new/reset-device records. Independent closure review
returned conditional SOURCE GO (25 supplied methods +40 independent checks); this
is not current hardware admission or original-trial clearance.

The one cached retirement subsequently passed natively with the original failed
HOLD preserved. Application identity remains unverified. The separate identity
candidate's author reports48 HOST methods PASS; independent exact-controller
review remains pending. Parent offline native-checkpoint/event lineage matches;
that is not a live owner/runtime admission. A parent30s HOST reproduction timeout
is retained; the longer bounded run then passed all48 methods. No target controller ran.

Watchdog is preferred for reviewed recovery, not a hidden test side effect. An
already guarded operation may reset autonomously; a deliberate `wdt reset` needs
its own known-idle/catch admission. No reset is used to clear an unknown state
without observed reset/retirement proof; cold radio/rail reset is not inferred.
