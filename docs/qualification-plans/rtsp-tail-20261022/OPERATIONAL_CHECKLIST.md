# Functional confirmation checklist — deferred, not executed

2026-10-22; user selected best-effort app builds and a written operational test
list instead of further penetration/adversarial/mutation testing. None of the
items below is a new test queue or a claim that the camera app is complete.
Keep existing authentication, bounds, isolation and ownership protections intact.
Do not conceal missing review/coverage or change model/safeguards to obtain a pass.

**Continuation:** later user steering authorizes eligible hardware work with
watchdog recovery; the build checklist still does not select app/runtime tests.
RAM76 restore/idle and normal watchdog smoke passed Oct22. A separate reset
controller failed its password-text classifier; actual reset/catch and later10s
stable held U-Boot were observed, with both failure HOLDs preserved. Last target
observation was **held U-Boot12:12UTC, not Linux**; current runtime/owner/recovery
is not inherited and no C/D item inherits an old Linux epoch.
See `../kernel76-transport-20261021/WATCHDOG_RESET22B_RESULT.md`.

Real native callback glue, frame/persistent-encoder custody and the observational
encoder query now have bounded source/compile evidence. Single-owner watchdog,
actual mandatory provider-FD integration and strict pre-memory custody return also
have narrow source dispositions; none is full lifecycle/runtime acceptance or a
deployable main. Query77's coherent kernel/package has parent offline disposition,
never booted; its embedded station76 is unsupported. The two active source lanes
address device-free backend startup and actual preopened-listener custody. External
retention investigation stopped before code on absent media bootstrap/recovery
and the serve-internal fork; it remains blocked, not implemented.
Owned listener now has narrow source/API custody sufficiency; prestarted-backend
v1 compiled but review requires terminal IPC cleanup after observer-reaped exit.
Fresh v2 correction compiled and is under focused re-review; no runtime leak is
claimed. A separate query77 qualification client is source/build work only, not
a selected hardware trial or app main. These remain separate
siblings, not an accepted composition. These artifacts do not check off outstanding
app/adoption/load gates below.

## Build-only deliverable and limitations

`apps/rtsp-auth/build_app.py` compiles and links the current RTSP integration
candidate, an isolated-verifier companion, and a HOST synthetic bench executable.
Optional ARM outputs use the retained pinned toolchain. It never launches built
programs, test suites, generators, listeners or camera operations. It does not
modify the frozen harness or invoke its independently-review-gated runtime builder.

The integration archive is **not a finished camera executable**: a real camera
entry point, admitted capture/provider wiring, provisioning and deployment package
are still missing. The link-check main is explicitly nonfunctional and never run;
the HOST bench uses synthetic source, not camera footage. New companion binaries
are not substituted for existing pinned runtime images. No missing feature is
reported supported, and no product gate is waived by a build.

## A. Build and packaging confirmation

- [x] Compile/link the declared HOST/ARM sources from pinned inputs and retain
  tool versions, headers, commands, licenses, sizes and artifact hashes:
  `build02` PASS, see `BUILD_RESULT.md`. This is not bit-reproducible release or
  runtime acceptance.
- [ ] Confirm architecture/ABI and the intended strict-vs-HOST configuration for
  each artifact. HOST fixtures must not enter a camera release.
- [ ] Add real platform entry/provider/provisioning wiring and an explicit release
  manifest before calling this a deployable camera app. No automatic startup.
- [ ] Review any new runtime image hash before updating existing embedded pins;
  do not force a newly built helper into an old qualification recipe.

## B. Ordinary app startup and shutdown (later controlled HOST operation)

- [ ] Start with the documented valid test configuration on a fresh owned localhost
  port; observe genuine ready/provisioning milestones rather than only process exit.
- [ ] Verify correct normal login using RTSP-only credentials, without logging
  credentials, generated startup records or their hashes. No UI-password reuse.
- [ ] Confirm the configured profile appears in DESCRIBE and TCP SETUP/PLAY works
  with the supported client and declared algorithm; report unsupported features.
- [ ] Exercise ordinary keepalive, PAUSE/resume, TEARDOWN, close/reconnect and
  graceful app shutdown. Observe actual owned child exits and released resources.
- [ ] Verify normal session expiry/logout behavior does not leave a stream running.
  These are functional checks, not a new fuzzing or mutation campaign.
- [ ] Start two normal clients and confirm independent sessions and continuous
  delivery, within the declared capacity. Do not infer native memory/FPS from HOST.

## C. Camera admission before any native operation

- [ ] Establish current user physical state, exact image/runtime, sole UART owner,
  no competing operation and fresh bounded capture/recovery; all old timers expired.
- [ ] Resolve outstanding station lifecycle/review/build gates before module attach.
  P1 parent HOST PASS is not the missing independent review or P2–P5 completion.
- [ ] Admit the exact verifier, source producer, process/memory budget and guarded
  single-use runtime recipe. Prefer watchdog recovery when separately admissible;
  retain manual OFF fallback and original native radio FAILED/HOLD.

## D. Actual camera operation (only after C)

- [ ] Start the paired app with the intended configuration and observe actual
  readiness, network/profile identity and no unintended cloud/AP/storage activity.
- [ ] Authenticate from an owned client; decode actual live H.264, not synthetic
  NALs. Count unique decoded/changed frames and report startup time and decode errors.
- [ ] Confirm orientation and requested supported resolution/profile. Measure real
  delivery cadence/latency; the engineering target is720p >=15 unique fps, not an
  achieved result. Report a measured shortfall instead of silently lowering quality.
- [ ] Exercise normal RTSP controls, reconnect, two clients, then clean stop.
  Check producer/encoder/verifier/socket/FD ownership actually retires on the same boot.
- [ ] Measure coupled CPU/RAM/CMA headroom on the40MiB board. CMA is included in
  RAM, not extra capacity; preserve128kB allocation and process budgets. Idle free
  memory and a successful static link are not load admission.
- [ ] Later verify the same admitted live source reaches the on-device browser.
  Real Frigate and licensed Blue Iris workflows require owned endpoints/access.

## E. Separate outstanding product confirmation

These remain separate tasks, not effects selected by this app build:

- Imaging/exposure/colour, physical PTZ/STOP, independent lights/IR-cut and scalar
  ambient/reset input; no GPIO2, automatic homing or blind output sweeps.
- Microphone scalar-only. Speaker samples generated on-camera; both UART leads
  must be isolated with alternate recovery before GPIO1/speaker effects.
- Approved expendable-card/storage namespace, durable settings/recording/playback,
  shared-L2/resource lifetime, protected AP/onboarding and settings-only reset.
- Combined-load reliability, cold entropy/trust, release/recovery and G0–G8.
  T34/Fulhan remain deferred until QC07F G8.

Extended adversarial/security/penetration/fuzz/mutation testing and another P1
review retry are **deferred, not passed**. Preserve existing results and partial
evidence; no external publication or support/account change is implied.
