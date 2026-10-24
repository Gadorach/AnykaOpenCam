# User-selected supervised driver bench policy

2026-10-24 · BASE-01/05/07, PLAT-01/02/06/08; RP-02/11/12.

Latest user prioritizes stable Wi-Fi/USB/MTU1500, bounded motors/STOP/defensible
zeroing and modern microphone capture before backend expansion. Speaker testing
is DEFERRED. Source fixes and compilation do not establish target readiness.

## Explicit user decisions

User says: "I can manually intervene, but you have smart switch control and can
just as easily cut power if needed." Use the already-confirmed QC07F smart switch
for normal ON/OFF and failure cutoff while the host remains operational. User
manual intervention is the independent fallback for host failure/uncovered phases,
not an invented autonomous timer in the outlet. Do not repeat the confirmed wiring,
supply or outlet-mapping questions. Coordinate actual live-session presence;
availability stated now is not a perpetual unattended recovery lease.

In response to the explicit timing-policy question, user selected **Allow
supervised trials**: for supervised bench trials ONLY, clock-discontinuity
detection and abort/power-off recovery may replace proof that host suspension or
clock discontinuity is impossible throughout the trial. This is a scoped policy
decision, NOT evidence that strict whole-interval C1 prevention has been proved.
Earlier C1 findings and failed observations remain unchanged at their dated scope.

Keep existing timing checks/fixed original deadlines/no extension, failure records
and retention of uncertain resources. No clock reanchoring, success after a
detected discontinuity, automatic retry or safe-free inference from an errno.
Failure ends further target effects and invokes the reviewed cutoff path; unknown
cutoff requires manual intervention and explicit retained evidence/ownership.
No product physical timing/recovery guarantee, native image/ABI qualification,
unpaired-driver use, provider-lifetime shortcut or uncontrolled homing follows.

## Next concrete preparation, not execution

Prepare ONE separate paired driver image using the coherent retained Linux6.18.55/
Clang23.1.3 baseline and reviewed PTZ OFF-retention source. Do not overwrite old
75/76/query77 sources/images, load the unsupported embedded station module, invoke
parked query78 recipes, alter flash/partitions or add a new diagnostic framework.
Retain exact relevant source/config/DT/initramfs/module/image associations, licenses,
commands/hashes and rollback. Driver/payload selection must be explicit. No guessed
mic provider, GPIO mapping, driver-ready flags or new backend is selected.

Prepare/review a bounded supervised RAM-entry procedure with fresh sole UART owner,
fresh cold catch, exact original-byte accounting, reviewed fixed staging/protocol,
image CRC/bootargs/real boot completion and actual watchdog/runtime/FD state. Source
and helpers freeze before any owned operation; no concurrent UART/status polling.
No old finalized23h catch, PID or timer supplies live authority. Initial completed
scope is boot plus fixed read-only idle checks, followed by OFF/finalization, not
automatic firmware/RUN/station/association/MTU/motor/mic/media/storage actions.
Each later driver operation needs its own current phase admission and review.

No target action is selected merely by writing this policy. Parent alone owns
hardware. Driver image and supervised entry require independent review before use.
