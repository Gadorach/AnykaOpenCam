# Camera software foundation — master implementation plan

Created: 2026-10-11. Status: **planned work; overall implementation incomplete**.
Workspace: `/home/gadorach/Desktop/AnykaOpenCam`.
Canonical plan directory: `docs/plan/camera-foundation` in that workspace.
Imported 2026-10-12 from `/home/gadorach/.opencode/plan/camera-foundation`;
original paths/hashes/timestamps are recorded in
`docs/archive/migration-docs-20261012.json`. Files01–18 retain imported bytes.

## Objective and order

Build and qualify the shared camera product on **QC07F first**, then port that
working foundation to the two remaining camera systems. The later systems are
**T34-4G / Anyka KM01** and **CFMTA-635727-BJYVX / Fulhan FH8626V200**. “Second
and third” in this implementation sequence does not renumber the inventory:
QC07F remains inventory camera2. Keep stable internal IDs and exterior labels.

The common product must support all fitted physical sensors, shared encoders
and PTZ heads, networking, authenticated RTSP/browser video, imaging controls,
PTZ/peripherals/audio, recording/playback/motion/retention, configuration,
setup/reset/captive portal, DNS/discovery and NVR integration.

Creating/importing this plan performs no hardware operation. All-required-modules
remains false. The current checkpoint below supersedes the preserved execution
history; revalidate actual state before continuing.

## Mandatory entry point for future work

1. Read this index and [01 baseline/execution](01-BASELINE-AND-EXECUTION.md).
2. Check the latest evidence and continuation records against actual runtime;
   never act on an old PID, UART owner, address or expired recovery timer.
3. Select the first eligible unfinished task in the active phase below. Read
   that subsystem file and its dependency files before editing or testing.
4. Complete one bounded, evidence-producing increment. Preserve rollback and
   all failed observations. Distinguish built, fixture-tested, hardware-tested
   and product-accepted results.
5. At handoff, record task ID, exact source/build/input hashes, observations,
   failed gates, current runtime, rollback and the **one next executable task**.
   In implementation mode, update status here and the project's evidence/index
   records. Do not silently convert a passing fixture into a product checkmark.
6. Read subsequent user steering before this plan. Scope changes and measured
   blockers may revise tasks, but do not quietly drop requirements or start
   either later camera before the foundation gate.

This repository-local index is the standing next-step reference, linked from
`AGENTS.md` and `START_HERE.md`. The external plan remains historical import
provenance; its pointer is maintained separately, not a competing canonical plan.
The previous workspace path is a compatibility symlink for frozen evidence and
build recipes. Do not rewrite historical absolute paths merely for the rename.

## Next task

### Oct23 user steering — return to feature integration

Park diagnostic-image/build-semantics work. Query78 v3's review is complete at
narrow SOURCE/method scope; no recipe/archive/kernel/image invocation follows.
The actual guarded owned-provider → authenticated RTSP/keyframe adapter in
qc07f-owned-runtime now has narrow conditional parent SOURCE/API integration
sufficiency and HOST/ARM compile evidence, no execution. See its
[parent disposition](../../qualification-plans/rtsp-owned-runtime-20261023/PARENT_DISPOSITION.md).
Actual app startup/provisioning/backing/external-owner integration remains next. No new
clock/receipt framework is selected. Startup/provisioning/
main/external media owner/backing/recovery remain explicit gaps, not fabricated
ready flags. See the current execution ledger; earlier build selections below
are retained history and do not override this steering.

Owned-runtime source is committed locally `5b3e252`. The next feature increment,
RTSP-only operator-password→actual verifier startup-record construction, now has
narrow independent SOURCE/API sufficiency and20 HOST/ARM compilations/26 commands
PASS, never called; no real credential material was used. See its
[parent disposition](../../qualification-plans/rtsp-startup-material-20261023/PARENT_DISPOSITION.md).
Actual separate hardened provisioner→bootstrap private descriptor handoff remains
next; storage/generation authority/recovery/backing/native requirements are open.

Oct23 renewed user approval explicitly includes hardware/motor/NOR work. Actual
passive23b and operator-assisted cold23c are recorded in the
[latest hardware result](../../qualification-plans/execution-20261023/COLD23C_RESULT.md):
camera now OFF, original HOLDs and parent EOF-caller failure preserved, capture
finalized/no active owner. Confirmed ESPHome outlet now controls QC07F only, not
MS42P; OFF POST/readback works. Subsequent v3/23d and v4/23e remain FAILED/HOLD:
23d stopped before3s wait/ON readback;23e refused BEFORE ON on2 unexplained bytes.
Both owned recorders exited0/finalized; latest independent GET OFF. Separate
receive-only23f completed120s0RX/TX, not an explanation/relabel of23e. See
[23e result](../../qualification-plans/execution-20261023/COLD23E_RESULT.md).
After a separate method review/selection,23g exercised accepted ON/OFF readbacks
but retained FAILED/HOLD on1 unexplained prefix byte before the exact profile/
trailers. Outlet OFF, child wait0/final hashes verified; no10s quiet or further
power-cycle/ON/native queue. See
[23g result](../../qualification-plans/execution-20261023/COLD23G_RESULT.md).
Hardware qualification is blocked on unexplained serial data, not a permission gap.
Separate RTSP provisioner-child/readonly-FD IPC now has narrow independent SOURCE/
API/custody sufficiency,27 HOST/ARM compiles/40 commands and ONE normal PUBLICDUMMY
fork/FD-transfer/wait0/bind observation. No real credentials/native hardening or
product/main/native adoption. The actual device-free runtime prepare/prestarted-
start connection after known provisioner retirement/bind now has narrow independent
SOURCE/API/order/custody sufficiency,2 HOST/ARM compiles/2 real REL links/24 commands,
never called; see
[bootstrap disposition](../../qualification-plans/qc07f-bootstrap-20261023/PARENT_DISPOSITION.md).
One source lane implements a real public-RTSP OS randomness callback, not fixture
entropy or native qualification. Provisioner handback:
[disposition](../../qualification-plans/rtsp-provisioner-process-20261023/PARENT_DISPOSITION.md).
PTZ counter source/compile-only correction is committed locally `8ec07ff`, not
deployed. Private
RTSP file/FD custody has also received narrow SOURCE/API acceptance with compile/
one PUBLICDUMMY observation; actual cross-process provisioning remains unfinished.

### October22 unattended execution / actual cold handoff

Follow [RESUME_20261022.md](RESUME_20261022.md) and the execution ledger. User
authorizes unattended eligible work/hardware trials with watchdog recovery until
genuine interaction is required; further adversarial testing stays deferred.
After silent receive-only observation, required user OFF/ON under fresh22b catch
established actual held original U-Boot. Fixed RAM76 upload, independent RAM CRC,
monitor startup and RO idle postcheck passed11:49UTC; normal watchdog smoke passed
12:05UTC. One selected idle reset controller failed on password-text classification;
actual reset/catch and separate10s stable original U-Boot observed12:12UTC. Target
was last observed held U-Boot at12:12UTC, not Linux. No current runtime/owner/
recovery admission is inherited. Both failure HOLDs remain; no native queue.
Prior radio HOLD is preserved; source/helper/operation ownership gates still apply.

Query77 kernel/package and strict pre-memory custody return now have parent
offline/source dispositions, not native/full-app acceptance. See
[query77 disposition](../../qualification-plans/encoder-query77-build-20261022/PARENT_DISPOSITION.md)
and [return disposition](../../qualification-plans/qc07f-provider-return-20261022/PARENT_DISPOSITION.md).
The new image has never booted; its embedded station76 module is unsupported.
External failed-work retention/backing/bootstrap policy remains the next source
dependency; do not reuse old22b controllers or treat their catch as unconsumed.

The actual prestarted-backend/owned-listener composition and query77 client reviews
are now completed with narrow SOURCE/API dispositions, not runtime/native or app
acceptance. Those author/review lanes completed. See
[composition disposition](../../qualification-plans/rtsp-prestarted-owned-listener-20261022/PARENT_DISPOSITION.md)
and [query client disposition](../../qualification-plans/encoder-query77-client-20261022/PARENT_DISPOSITION.md).
Next eligible work is [offline native-method design](../../qualification-plans/encoder-query77-native-method-20261022/METHOD_FINDINGS.md):
fresh controller/HOLD profile and bounded volatile delivery/observer/recovery.
Controller design is completed; its parent disposition selects only C1 host-local
dual-HOLD/epoch/crash-sticky ledger source work in encoder-query77-custody, now
delivered AST/syntax-only but review requires R1 original-epoch constraints and
R2 failed-call historical postchecks. Fresh custody-v2 source is delivered
AST/syntax-only and conditionally SOURCE/API sufficient after focused review of
those corrections and exact read-only historical-root/private-output roles, never
migration or copied markers. V1 stays frozen/unaccepted. Next selected source is
C2a fixed-public-input validation, now independently source-sufficient with one
parent offline public-artifact call PASS (25 inputs, not native admission).
C2b stopped before code on missing public grammar. Parent offline historical
inspection supplied a privacy-reviewed symbolic exact-profile grammar; fresh
cold-context-v2 has conditional exact-profile source sufficiency plus one parent
offline observation on saved historical bytes, not native/current-state proof.
C2c clock contract and continuity feasibility remain strictly blocked; no changed
policy, watcher/inhibitor/sampler or host reconfiguration. Next separate source
prerequisite is an observational recorder-timer derivative, now conditionally
SOURCE/API sufficient after independent review, never imported/called/run. It
exposes original stored values to exact private status only, no timer/catch/lease
change or execution/adoption. Frozen source/review diff are withheld from Git for
an inherited adapter selector; fresh explicit-port source removes that default,
now conditionally SOURCE/API/selected-privacy sufficient after independent review,
never imported/called/run, no discovery/fallback or live profile adoption.
Separate original-build02 artifact-role inspection
confirms static architecture/define separation, not native ABI or release packaging.
User continuation selects fresh query78-client, now HOST/ARM compile/link PASS
and narrowly conditionally SOURCE/API sufficient after independent review, never
run. Parent separately closed the review's public build-config pin exclusion.
A proposed future image embeds that reviewed client and omits
the unpaired station record, preserving original PID1; see embedded-image contract.
Closed recipe v1 passed syntax only; independent review requires R1 final-pin/
terminal ordering and R2 primary-failure preservation. V1 stays frozen/unaccepted;
v2 correction passed syntax but re-review requires R1-A dependency inventory
coordinates; R2 changed error paths narrowly sufficient, not composite GO. Fresh
v3 R1-A source has narrow conditional review sufficiency, but later user steering
parks this diagnostic lane. Old failures remain frozen; no recipe/archive/kernel/
image invocation or native recipe/boot is selected.
Original block/parser/HOLDs
remain frozen. No
target interface, executable native recipe, capture, staging, boot or query is
selected. Staging lacks an admitted watchdog receiver and any later manual-OFF
handoff still needs review plus actual current operator readiness.

### October22 earlier steering — build and functional checklist

Current state is maintained in the [execution ledger](EXECUTION_TASKS_20261021.md)
and compact root `AGENTS.md`, not older “latest” sections below. User now requests
best-effort app builds and a [functional checklist](../../qualification-plans/rtsp-tail-20261022/OPERATIONAL_CHECKLIST.md),
not further penetration/adversarial/mutation tests. Select compile/link only via
`apps/rtsp-auth/build_app.py`: HOST/ARM build02 now PASS, no test/app/generator/
listener or target execution. [Build result](../../qualification-plans/rtsp-tail-20261022/BUILD_RESULT.md)
distinguishes libraries/companions/synthetic bench from a deployable camera app;
real platform/provider/provisioning wiring and packaging remain missing.
P1 author and fresh parent HOST results are preserved, but independent review is
provider-blocked, partial and unaccepted. No further retry/reroute is selected.
The disjoint B08 HOST harness candidate is delivered; compile-only artifacts do
not satisfy its deferred runtime checks or supply missing camera integration.
The fresh22b handoff above supersedes the earlier unknown target state; original
radio HOLD persists. Neither source lane authorizes native station/RTSP work.

### Active execution — October21, reviewed increments committed on main

User approved execution of the consolidated plan. Follow
[EXECUTION_TASKS_20261021.md](EXECUTION_TASKS_20261021.md): RP-00 baseline first,
RP-02 finite USB/radio entry and independent RP-04 memory source work. Existing Git
main now has curated commits/no remote; user requests commits and selected **local commits for
now** until a URL is supplied. Preserve the pre-existing staged index; do not commit
acquisitions/secrets/binaries or publish automatically. [Workflow](../../../GIT_WORKFLOW.md).
Fresh21a cold entry and exact RAM76 upload/CRC/monitor plus fixed read-only idle
postcheck passed04:36–37UTC Oct21. Target ON/UART connected; after the05:23UTC
radio trial, application state/retirement is unverified. Capture07:09UTC is
DATED/revalidate, not inherited authority.17 host methods and independent
closure review passed; actual trial passed preadmission/all74 fragments but final
RUN ACK was interrupted by USB-disconnect printk. Strict receipt **HOLD**, no
state/info followed. A separately reviewed06:00UTC retirement-only cached read
then passed on the same boot (no PID1 USB/watchdog FD, idle ISP/watchdog/UART),
preserving original HOLD byte-exact. No reset/retry. See active ledger,
`NATIVE_TRIAL21A_HOLD.md` and `READONLY_RETIREMENT21A.md` under qualification docs.
Separate06:43UTC APPLICATION/retained firmware identity plus full fresh pre/post
idle/FD/descriptors retirement then passed,1741B/54 sends, preserving original
HOLD. No reset/reload/module/association/network/MTU result. See
`SEPARATE_IDENTITY21A_RESULT.md` under qualification docs. Paired76 station entry
review is EXECUTION NO-GO (async guard/timeouts/recovery/parser/lifecycle gaps);
parent selected isolated A1 control/FWINFO SOURCE/HOST correction only, no target
or ARM build. See `STATION76_ENTRY_REVIEW.md`. Combined memory/product gates open.

21a capture has since **expired/finalized**: parent07:24UTC host check found no old
recorder, adapter owner or socket, and verified private final manifest/raw/events
sizes/hashes. No extra RX or TX followed the06:43 identity checkpoint. Current
target runtime/power is unqueried; no receive owner/catch recovery is inherited.
A1 local control/FWINFO, A2a original ingress/status and A2b bounded event
suppression/denial each have narrow independent SOURCE/HOST GO, with sealed
derivatives and parent reproductions. Full S4/lifecycle/ARM build/attachment remain
NO-GO. Remaining attach source plan is delivered; parent selected only its P1
outer-RX-family exclusion SOURCE/HOST increment. Subsequent TX/recovery/lifetime/
guard packets remain plans, not implementation/build/native approval. No target queue or current
receive/catch recovery authority. See `CAPTURE21A_CLOSEOUT.md` and active ledger.

P1 subsequently **paused/incomplete after a worker tool-safety error**. Partial
files are preserved but unreviewed/uncommitted, with no observed final seal. All204
accepted A1/A2a/A2b source pins still match. No retry or reroute was launched;
see `P1_TOOL_BLOCK.md`. No later packet or hardware operation is selected by it.

### Latest request — complete remaining-feature/bug plan, October21 (PLAN ONLY)

Use [REMAINING_FEATURES_AND_BUGS_GATED_PLAN_20261021.md](REMAINING_FEATURES_AND_BUGS_GATED_PLAN_20261021.md)
for the consolidated dependency order,29 defect/regression rows,21 integration
packets, E0–E4 entry/evidence gates, canonical G0–G10 and coverage of all172
existing subsystem tasks. Existing requirements/task IDs remain authoritative;
corrected SOURCE/HOST candidates are distinguished from native/product acceptance.
User requested a planning file, **not execution**. No build/target/RF/actuator/card/
reset/flash work is queued. Last actual RAM76 boot/idle proof is Oct20; the20c
08:02:56UTC protection is expired historical authority. Current power/runtime/
owner/socket/recovery are unknown without fresh admission; none was queried here.
Review the explicit threshold/privacy/AP/mechanics/storage/NVR/persistence decisions
before executing the affected packet. QC07F G8 remains the prerequisite to ports.

### Latest actual — RAM76 boot and read-only idle PASS, no further test queued

User-authorized one fresh20c RAM boot passed05:33UTC: exact upload/full CRC/monitor,
zero frame retries; UID0/PID1init/16MiB CMA, volatile mounts/no modules, ISP idle,
logical watchdog inactive and UART errors/busy0. Target ON at idle RAM76 monitor,
no shell/GUI/radio/network/media/control action. Same protected20c owner/catch/HOLD
revalidated05:34UTC through08:02UTC DATED/revalidate. See
[RAM boot result](../../qualification-plans/kernel76-integration-20261020/RAM_BOOT20C.md)
and [ledger](KERNEL76_TASKS_20261020.md). Native full-size USB/MTU1500, FPS/media/
peripheral/product/install gates remain open. Next is bounded radio-loader/USB
test-entry review, not an automatic command queue or stock retry.

### Latest actual — candidate76 built/packaged, offline verification PASS

Modern increment completed: isolated Anyka PIO continuation/NAK byte-kick fix;
fresh kernel/radio builds; explicit initramfs76 replaces stale radio/RTSP and adds
the ELF DHCP bridge. QC6U76.UIM3603555B/SHA bd8a2a08… is a **RAM candidate, not
booted or flash-qualified**.524288 helper cases across optimized/sanitized builds
and3 runtime mutants,17 archive tests,7 Python -O refusals and20 independent final
artifact checks pass. Original75/preliminary76 outputs preserved; final radio
byte-equals embedded. No image/core changes from later host safety hardening.
See [results](../../qualification-plans/kernel76-integration-20261020/RESULTS.md) and
[active ledger](KERNEL76_TASKS_20261020.md). Camera remains OFF/UART connected,
no UART owner/socket/recovery arm, GUI, workers or target test queue. Next is
finite native RAM-loader/test-entry review and fresh physical/runtime admission;
native USB/MTU1500/FPS/peripheral/product gates remain OPEN. No stock retry/rebase.

### Latest user continuation — modern kernel76 source/build ACTIVE

User asks continue investigations or switch to new kernel work. All unblocked
stock source work is closed; remaining native/physical rows remain blocked. Follow
[KERNEL76_TASKS_20261020.md](KERNEL76_TASKS_20261020.md): isolated Anyka MUSB PIO
continuation/NAK MODE-preserving fix, host tests, fresh ARM build/paired radio and
package review first. Keep Linux6.18.55/kernel75 rollback. Camera remains OFF;
no UART owner/recovery arm or native test/flash/actuator/RF queue.

### Latest stock closeout — source inputs reviewed; native rows BLOCKED; user OFF

Follow [active ledger](STOCK_REFERENCE_TASKS_20261020.md) and
[reviewed next-build inputs](../../qualification-plans/stock-reference-20261013/closeout-parent01/NEXT_BUILD_INPUTS.md).
SD/L2, Wi-Fi, video-entry, motor-zero source, audio and AP/NOR decisions executed;
no native completion implied. New exact USB continuation/MODE delta is the next
focused modern patch/test hypothesis. Exact MCI group13 recovered; physical power/
pdata/clock/retirement gaps remain. Stock FPS unmeasured; current media adapter HOLD.
One fresh20b stock boot reached read-only root but garbled console/no verified shell;
no retry or resource/test command. User OFF/leave OFF, UART connected;20a/20b private
captures sealed/hash-verified, no UART owner/socket/recovery arm, GUI or workers.
No new image/install, live homing/card writes/AP broadcast or other effect queued.

### Active execution — remaining stock reference closeout

User approved execution and requested an actual active reference. Follow
[STOCK_REFERENCE_TASKS_20261020.md](STOCK_REFERENCE_TASKS_20261020.md): SD/L2 first,
Wi-Fi, video, motor zeroing source, audio, conditional AP/NOR decisions. At most two
disjoint source lanes; parent-only serialized hardware after fresh admission.
No live homing/card writes/AP broadcast/flash changes in the approved base scope.
Do not treat an old20a deadline, GUI epoch or cached runtime as live authority.

### Latest request — remaining stock investigations, PLAN FOR REVIEW ONLY

User requests a finite plan for worthwhile Wi-Fi, SD/MMC and other remaining stock
questions, including motor zeroing. See
[stock-reference closeout review draft](STOCK_REFERENCE_CLOSEOUT_PLAN_20261020.md).
No execution/boot/homing/card/RF campaign is authorized by this planning request.
Recommended source-safe priorities: USB/radio boundary differences, MCI/shared L2,
real uncapped stock cadence, motor zero/phase/STOP semantics and remaining audio
provider facts. AP/flash conditional; live homing/card writes/AP broadcasts separate.

### Latest checkpoint — GUI stopped, neutral postcheck, local close fix tested

User-authorized exact GUI SIGTERM completed; no forced kill/target signal. Native
postcheck lights/pairLOW, usedmotorcoils retired, capture/WD idle and UARTerrors0.
Closed-trap corrections pass71 guarded mocks; no native re-test or auto reopen.
No hardware queue; fresh owner/runtime/recovery/admission before another trial.
See qualification `optical-live01/GUI_CLOSE_FIX_20261020.md`. IR-cut functional
A OUT/nightIR-pass, B IN/dayIR-block recorded; motor physical direction remains open.

### Latest user scope — white-light and motor aiming for optical IR-Cut check

User reports A/B associated clicks but white light/aim obscures view. White GPIO5
native outputLOW passed and **user confirms OFF**. Added White OFF/timed ON and
existing-kernel manual±4 motor nudges; native motor metadata only, no move yet.
Updated v2 aiming GUI launched, private epoch02:47:28UTC expiry. No parent UART
during operator use. Await user movement/image feedback; see
[aiming handoff](AIM_WHITE_MOTOR_20261020.md). No homing/32-step chaining/FPS detour.

### Explicit resume — Oct19 user / fresh UTC20 cold catch

Latest actual:45s liveRTSP83decoded/errors0, fourlease renewals/ownedretirement
passed. Nativeoptical inertload/setup/OFF/dispatch/refusal passed, NO HIGHtrialyet.
FreeSimpleGUI boundedoperatortrial launched with private01:53:07Z expiry; user
Connect/Readstatus/manualA/B/IRnext. Parent noUARTcommands duringoperatorhandoff.
Physicalfilter/electricaltiming and FPS/productgates remain open. See resume.

Newer actual: RAM75 fullimage/CRC/monitor and inertroot admission passed, retained
radio RAMload/applicationff passed. One private hash-verified RAMbundle stage active,
not module/helper execution or association. Next controlledsensor-streamoff prep,
pairedstation/network, then finite realRTSP decode/retirement before opticalbuttons.

User confirmed OFF/UARTconnected, thenON under freshprivate20a catch. Originalcold
U-Boot identity/BOOTpin/neutralbaseline passed; parent fixed RAM75 upload/boot recipe
active, completion pending, no nativefeed/controls admission. Old16b expired/sealed,
not inherited. Read [resume](RESUME_20261019.md); parent alone owns hardware and no
concurrent UART commands during transfer. Optical live-feed test remains next.

### Latest user order — IR-Cut and speaker first, camera FPS next

Latest actual/user steering: speaker audible224ms and steady5s narrowly PASS.
One own U-Boot GPIO6 pulse returned neutral; no audible response, movement unproved.
Temporary GPIO setup was restored. User now requests a decoded live RTSP preview
with independent IR-Cut A/B and IR-LED controls for optical confirmation. This is
requested GUI scope, not the formerly deferred general GUI/policy work. See
[optical test plan](OPTICAL_IRCUT_LIVE_20261016.md); exact current kernel75 encoded
receipt renewal and live output release under media load still need qualification.

Reset input data is sufficient; no further retail reset-policy or GUI detours.
Use retail only as hardware reference for **our** implementation. IR-Cut pair6/7:
own RAM-only U-Boot pulse probe source unit, no native pulse yet, retain UART.
Speaker: confirmed physical SPK connection; concrete missing enable-low edge found,
own speaker8 correction source unit, no playback until UART TX/RX isolation. Then
camera timing/cadence investigation. See
[IRCUT_SPEAKER_CAMPAIGN_20261016.md](IRCUT_SPEAKER_CAMPAIGN_20261016.md).

### Oct16 user resumes tests — CTRL-08 reset input before UART handoff

**Input increment now PASS:** physically matched GPIO0 active-low taps, no reboot,
pin configuration or ENV/ROOT/JFFS change; helper closed/retired. No factory-reset/
hardware-recovery qualification. UART remains active, not sealed. Offline GUI/SSH
contract repair and parent console07 reproduction are next, no output/ADC actions.
See reset-button01/ACTUAL_RESULT01.md and dated reset plan for exact evidence/gates.

Fresh private16a sole capture/next-boot catch active02:55–05:55UTC, cameraON/UART
connected, pinned installed SSH runtime revalidated. Identify reset input without
stock SET-before-GET wrapper or factory erase. GPIO0 shared LED/reset is source-only
candidate; no press yet. Keep UART active, parent owns hardware. See
[RESET_BUTTON_TEST_20261016.md](RESET_BUTTON_TEST_20261016.md). No unrelated outputs.

### Oct16 completed increment — persistent SSH-only ROOT cold-accepted

ROOT1MiB actually flashed; default power-on (zero UARTTX/no boot interruption)
auto-started pinned key-only SSH, stock3.4.35/initPID1. Auth/pin negative tests pass.
Full8MiB post-cold byte readback equals private expected composition; all nonROOT
bytes/current password preserved. Normal app/cloud/record/homing disabled.15c RX
capture finalized/hash-verified/stopped, no UART owner/socket/arm; SSH/network left
running. No GUI/peripheral queue. See [current resume](RESUME_20261015.md) and
[installation result](../../qualification-plans/stock-reference-20261013/persistent-ssh01/INSTALL_RESULT_20261016.md).

Next source-only eligible unit: review/fix retained GUI/console schema/rounding,
contradictory receipt acceptance, UART fallback and uncertain SSH-exit ownership
issues, plus fresh PID1-init profile/manifest. Do not stage/start native controls
or infer a product gate from the earlier host tests. Fresh target protection first.

### Oct16 actual ROOT programmed/readback PASS — cold auto-start acceptance next

Authorized SSH-only ROOT `[0x200000,0x300000)` actually programmed once,1MiB full
comparison passes; seven nonROOT chunk CRCs unchanged. No other partition/ENV/SD
write. Existing password preserved. New image not booted yet, so SSH readiness is
NOT claimed. Parent coordinating one default cold start with fresh PRIVATE RX-only
owner, explicit intentionally-unarmed catch for that observation and tested
U-Boot/host-restore/manual OFF/rearm recovery. No GUI/peripheral queue. See
`resume-20261015/ROOT_PROGRAMMED15B.json` and the current dated resume.

### Explicit resume2026-10-15 — fresh catch armed; await power-on observation

**Later actual checkpoint:** readable controlled stock3.4.35/PID1 shell and RAM
SSH restored, key/pin negative tests pass. Two fresh8MiB alias backups equal; ROOT
matches authorized Oct08 password update. Earlier acquisition-based SSH candidates
invalidated; corrected live-baseline pair preserves existing passwd/ENV/JFFS,
14 tests/equal970752B and native RAM SquashFS/inert execution pass. No flash write.
U-Boot-only protected ROOT transfer/recovery selected; source unit pending parent
review/new owner rotation. Do not run unqualified pivot/Linux programmer or GUI.
See persistent-ssh01/RESUME_20261015.md and current dated evidence for source states.

User resumed, camera unplugged/UART connected. Fresh15a sole updated recorder
verified22:25UTC/TX0RX0, future hold armed through Oct16 01:25UTC. No current Linux
or SSH runtime is assumed. Observe user-controlled power-on/U-Boot hold first.
One bounded source-only boot-review closure runs independently; parent owns target.
Persistent SSH remains uninstalled; final private ROOT/backup/geometry/unmounted
programming/rollback gates still open. See [RESUME_20261015.md](RESUME_20261015.md).

### USER HALT2026-10-14 — no persistent SSH flashed

**Projectpaused. DoNOTresumeimplementation/delegation/targetopsautomatically.**
Userrequestedflashthenhaltforcredits; parentdidnotflashunreadyNOTFLASHABLEdummy/
missingprivateimage/programming/rollbackgates. NoSPI/SD/saveenv/OTPwrite. Lastcamera
originalUbootheld/noLinux; hcapturestopped/finalhashverified, **noUARTowner/socket/
arm**. UserOFFleaveOFFrequested/notobserved. Originalstockstartupstillactiveonnext
uncaughtboot. Currenthandoff [HALT_20261014.md](HALT_20261014.md) andPAUSED_STATUS.md.
Onlyresumeonnewuserrequest withfreshownership/runtime/recovery/image/range review.

### Boot safety fixes host-pass/re-review pending; fresh updated h catcher ready

5independentBLOCKs fixed(resetgaps/lifetime/staleSFread/handoff/name/prematurenotice),
newconditionalTXv1refuseschangedorqueuedRX/keybursts beforewirewrite. 34host+4private+
4bootwindowPASS; re-reviewpending/no nativeRAMprofilepass. UserconfirmedstillOFF;
gsealed/hashverified, freshh3370127/start81204056 armed04:41–07:41UTC/TX0RX0. Camera
canonlybecaughthold; noautomaticboot/upload/flash. NeedcontrolledpowerON/actual
Ubootidentity/parser/readgeometry beforeSPIplan. Sourceconsole07(164each) andGUI
(33/smoke)built, no nativequalification; crossauditpending. ROOTdummyfit970752Bnot
flashable/no privatefinalimage/backup/recoveryvalidated; NO SPIwrites. SeeACTUAL_RESULTS.

### Persistent SSH-only ROOT proposal / repeatable RAM-boot tool; power-on next

UserauthorizespersistentSSHstartup andchoosesSSH-onlytrial(noapp/cloud/record/homing),
confirmsUARTreconnectedcameraOFF. AimROOT-only[200000,300000),kernel/boot/ENV/MAC/USR/
config/SDuntouched; offlineDUMMY970752Bfit/77824Bheadroom/223pathpreservation/11tests,
notflashable/private/nativeapprovedyet. NoSPIwrites. Fresh13gsoleowner/catcharmed
03:14–06:13UTC, noLinuxruntimeinherited; awaitingusercontrolledpowerON.

scripts/qc07f_boot.py nowpackagescatch/identity+vendorCRCpin/volatileROstock-shell/
SD-freecheckedUARTloader+RAMupload/boot, publicpayloadsonly/noerase-write-saveenv.
16hosttestsPASS+nativeRPC-arm-only(noTX), independentaudit/nativeboot/uploadpending.
GUI33mock/smokePASS; console07sourceunitactive, deploymentblockedonrecovery. Parent
readliveflashgeometry/freshbackup+prepareprivateimage/recoveryBEFOREscopedROOTwrite.
See stockACTUAL_RESULTS anddocs/QC07F_REPEATABLE_BOOT_TOOL.md.

### GUI7 offline implementation; restore safe control before deployment

Userwantsambientmanualreadout+allvalidfreq/gain0..5/vol0..100/duration0..5scontrols,
tonegeneratedcamera-side(hostparametersonly). Two disjointsource/mockunitsactive;
bounded5sringrefill49KiB/512scratch andmanualIRQ-riskALS/noautopoll. Parentpublicmic
restage02:59failedSSHNoRouteToHost; PCroute180/enpunchanged,targetruntimeUNKNOWN.
NoPCM/ADC/GPIOeffectbycopycommand. **NoUARTowner/socket/arm,wiresdisconnected.**
UserOFFleaveOFF/reconnectUARTunpowered/tellparentBEFOREpowerON; freshcatch+controlled
root/SSHneededbeforeGUI/nativequalification. Noauto-recovery oroutput. Sharedcontract
PERIPHERAL_GUI_AUDIO7_INTERFACE.md; stockACTUAL_RESULTS/latestfailureevidence.

### Native gain3 trial applied/restored; await audible feedback

Tone06parent221opt+221sanPASS/byte-identical73864B. Oneannounced02:38native224ms/
digital8192trialexit0, actualcache1→3→1+HPfield8→2→8verified; oneSET3/SET1,noHPdirect
store. PAUSE/close/guard774return0/gone; SYS74186/DIR/OUT2/SYS7850/HPfe220008restored,
postnoPCM/WDowners/SSHhealthy/L2IRQ12Err0. **Askuser tone/click/nothing**; no repeat/
moregain/IRcutpulsequeued. Old04/05inaudiblenegativespreserved. UARTdisconnected/
recorderended/noarm; manualOFFleaveOFFrecovery. speaker-native06/POST_IDLE_RESULT01.

### 13f finalized; SSH only, gain3 candidate review still pending

02:30bounded13frecorderended/finalhashesverified; **noUARTowner/socket/arm** remains.
Didnotreopenunpluggedserial. Fresh02:31pinnedSSHroot3.4/idlePCM+WD/SYS74186/OUT2/
HPgainactual1PASS. Gain3nativehasnotrun; bounded06source/hostunitpendingparentreview.
OnfailureOFFleaveOFF; reconnectUARTunpowered/newowner+freshcatchbeforeboot. No new
GPIO/audio/IRcutoutput. See speaker-native06/RECORDER13F_FINAL_SSH_CONTINUES01.json.

### User requests gain3; verify actual HP field and restore code1

05alsoNOsound. Userexplicitcode3retry; parentreadonlyboundHPphys0800009c valuefe220008/
encoded8=hardware1(cache1consistent). SET0x40045030 supports0–5; code3encoded2. One
source/host06unitactive: code1→3→1 viaioctl, digital8192/224msunchanged, getter+actual
fieldwitness8→2→8 andrestorationafterPAUSE/beforeclose. No gain/PCM/muxoutputyet;
parentindepreview/rebuildthenoneannouncedtrial. UARTdisconnected/manualOFFleaveOFF
recovery; noIRcut/LED/GPIO2test. See speaker-tone05/GAIN3_FOLLOWUP01.md.

### Higher digital tone05 completed; await user audible feedback

Parent173opt+173sanPASS/byte-identical69592B. Oneannouncednative02:01peak8192(+20.56dB
digital)/224ms, analog1unchanged, PAUSE/close/guard719exit0; SYS74restored186, DIR/OUT2/
SYS7850preserved; postnoPCM/WDowners/guardgone/SSHhealthy/L2IRQ8Err0. **Ask tone/click/
nothing**; no repeat/analogchange/IRcutpulsequeued. Old04inaudiblenegativepreserved.
UserUARTdisconnected/manualOFFleaveOFFrecovery. See speaker-native05/POST_IDLE_RESULT01.

### Speaker04 physically inaudible; prepare one louder digital-level comparison

Userheardnothing; preservephysicalnegativeversushealthyPCM/restore. SourceHPgain
codes0–5notpercent. Tone05changesdigitalpeak768→8192(+20.56dB), keepsanalog1/224ms/
allroutes/lifecycleunchanged; noGAINSET/sweep/maxjump. Smallisolatedsource/hostunit
active; parentindepreview/rebuildthenONEannouncedtrial+physicalfeedback. FreshSSH
minimal3.4root/noPCMowners/muxDIR/OUTbaselinePASS, UARTstilldisconnected/manualOFF
recovery. NoIRcut/othertoneoutputyet. See speaker-tone04/LEVEL_FOLLOWUP_REVIEW01.md.

### One native speaker tone completed/restored; await user audible feedback

Tone04independent171opt+171sanPASS/byte-identical69592B. Oneannounced224ms/peak768
DACtrialviaSSH01:51exit0/PAUSE/close/guardreturn0; gain1preserved, SYS7402000186→184→186,
DIR0008000e6/OUT2unchanged, SYS7850unchanged, twohelpermuxstoresonly/noGPIO2write.
PostnoPCM/WDowners/guard649gone/L2IRQ4Err0/SSHhealthy. **Physicalheard/tone-vs-click
feedbackpending**; no repeats/gainchanges/IRcutpulse queued. ActualIRcutsource6/7not2
stillneedsboundedrelease review. UserUARTdisconnected/manualOFFleaveOFFonfailure;
no autoUbootcatch claim. Evidence speaker-native04/POST_IDLE_RESULT01.json. Allworkers
stopped; keepSSHtemporarycontrolparent-owned pendingfeedback. Productgatesopen.

### UART isolation confirmed/SSH PASS; tone04 temporary GPIO1 mux source review

UserTX/RXdisconnectconfirmed; pinnedSSHreadbackstilloriginalminimalroot/noPCMowners/
L2IRQ0/noPTZproviders, SYS7402000186/SYS7850/DIR0008000e6/OUT02. NoUARTTX/GPIOdrive.
Benign8sWD/clientabortshowedFDsurvivalthenreleasebutleftoneexitedZsh606/PPid1/noFD;
parentstrictPIDgoneassertfailed/midrawquerynotretained, nofullfaultcleanupPASS.
Tone04isolatedsource/hostunit active: knownGPIO1temporarymask2 clear/restorewith
fixedtone/confirmedDMAstop-close;GPIO2notused. Parentrebuild/reviewbeforeannounced
nativebeep. Noactuatoroutputyet. UnpluggedUARTcannotcatchreset; onlossuserOFFleaveOFF,
reconnectunpowered/rearmbeforeboot. See stockACTUAL_RESULTS andSSHguardreceipt.

### SSH verified/active; coordinate UART TX/RX isolation before speaker mux test

Nativekey-onlySSH/pinnedhostidentityPASS; absent/wrongkey/password/none/pinmismatch
reject. Listener/session/shellcleanexitverified (860KiBobservedaggregateRSS), then
listener576restarted192.168.1.73:22222; twofreshSSHcommandsPASS. SecureRAM-onlykeys,
root/usrRO/noapp; UARTmuxselected/OUT2/PCM+WDuserspaceidle. Parentindependentbuild
byte-identical230912B. Awaituseradapter-sideTX/RXdisconnectconfirmation, camerastays
ON(RAMSSHlostonpoweroff), nohandlingpoweredPCB. NoGPIO1/2remux/audio/IRcutoutputyet.
OnSSHlossuserOFF/leaveOFF; reconnectUARTunpowered/revalidatecatchbeforeboot. Future
faultTCP/guard/peripheralreleaseproofstillopen. StockIRcutrouteGPIO6/7(not2), nominal
10mspair/release00; ROcurrentbothlow/SYS7850. See stockACTUAL_RESULTS/SSH_READY01.

### Recovery/root/station restored; SSH build/auth pending, keep UART attached

Userrebootcaught13fUboot00:43; freshrearmbounded02:30UTC, exactstockCRC5b03fb7e
controlled3.4.35rootPID1sh/restoredROmounts+WDonly, noapp/cloud/homing/SD/JFFS.
OriginalUSB/radioqualifiedloadthenprivateVertexHotspotWPA2CCMP/local-onlyDHCP:
actualcam192.168.1.73MTU1500, PC192.168.1.180, noDNS/default. Rootallowlist/live
securetmpfs0755ancestors+0700SSHdirverified; hostclientkeyprivateoutsideGit.
SSHserverbuild/sourceunit02pending(noSSHlistener/auth/nativepass), IRcut01source
laneactive(noGPIO2drive). KeepUARTconnecteduntilparentexplicitSSHready+coordinated
isolation; poweroffwouldloseRAMSSH. AlloldGUIhelperslost. See stockACTUAL_RESULTS.

### Serial command truncated; recover root and switch to verified SSH

Usertoneclick1199Bcommand echoedonly~1023chars, noexecutionBEGIN/native/exitreceipt;
CtrlCnoRX/prompt. GUIclosed/speakermanifestdisabled; hostguardnowblocks>1000BbeforeTX,
13testsPASS. DoNOTretry/completepartialcommand. Fresh00:38sole13fcatchrevalidated
unused/armedends02:30:36UTC. **UserpowercyclewithUARTstillconnectedneeded**; parent
catchUbootandrestoreminimalstockroot beforeWLAN/key-onlyRAMSSH. Source/buildSSH02
hostonlyactive; noSSHready/listener/network/keys yet. UserwantsexplicitSSHreadythen
UARTdisconnect; allowstemporaryreviewedspeakermuxchangeafterphysicalisolation and
asksGPIO2IRcuthypothesis. Needactualbridgepins/pulse/neutralproofbeforeIRcutdrive.
See stockACTUAL_RESULTS; allnativepreviousstate/helpertargetpaths staleafterreset.

### Tone03 reviewed/staged; user-clicked speaker test ready

Parentreproduced151opt+151san cases, byte-identical69424B tone03; strictGUIv3
receipts/12Pythontests/UIsmokePASS. Fresh00:26stockroot/noPCMowners/13fowner+recovery
revalidated; `/tmp/stock-speaker-tone3-armv5`hashverified. NoPCMopen/tone/GPIOwrite
yet. GUIreopenedwithspeakerbuttonenabled: userConnect, ONE224msquiettoneclick,
thenheard/notheardfeedbackandnativePAUSE/close/latchrestorecheck. OnefixedOUTbit1
restoreonlyafterconfirmedclose;noSYS/DIR/pinmux/gainwrites. ParentnoUARTwhileGUI
operatoractive. See speaker-tone03/PARENT_REVIEW01.md andstockACTUAL_RESULTS.

### Actual speaker entry blocked before transfer — input/latch mismatch

UserclosedGUI; parentlocked/revalidated13f runtime/recovery, thenROpreflightfound
GPIO1INPUT0 versusOUT1 (SYS74UARTmask6stillselected). DriverrestorecopiesINPUTtoOUT,
sothecandidate'slatchequalitygatefails. NOstage/PCMopen/tone/checkrelaxation.
Originalstockroot/mic3RAMdigest/idleowners verified; no rebootin13f. Optionalpdata
secondpinflag0 nowlivematches. ReopenGUIspeakerdisabled; boundedsourceonlyreview
ofexistingnonclobberrouteactive. See speaker-tone02/LIVE_ENTRY_HOLD01.md.

### Speaker candidate independently reviewed; await GUI closure for staging

Parent speaker-tone02 build04 reproduces112 optimized+112 sanitized tests and
byte-identical69196B ARM. GUI native-receipt mismatch corrected,12 Python tests;
no targetoperation duringreview. Candidate fixed224ms/peak768 DAC-only, no gain or
GPIOmuxwrites. GUIstillopen, speakerhelpernotstaged/manifestdisabled. Next user
stops/closesGUI; parentrevalidates owner/runtime/recovery before RAMstage, then
reopensforoneuserclick. Speakeracceptancepending. See speaker-tone02/PARENT_REVIEW01.md.

### User mic acoustic correlation confirmed; speaker tone remains next

ManualGUIquiet/fan lateAC_RMS84–87 versusclaps2126–2646 (27.6×mean), all8capturestopclose0:
physicalMICsoundresponseconfirmed, notonlypipeline/cachedsettings. Gain7claps
19/32000clipped≈0.0594%; gain/quality/physicalrate/AVstillopen. Source/runtimecapture
253:1 requested8k/calculated8012, no rawvoice/hash. ALSADC0dark/light54/2999/39already
confirmed. UserrequestsGUIexpectedDAC0tone; nativebackend/route review inprogress,
buttonstilldisabled—notspeakeracceptance. No competingparentUARTwhileGUIinuse.

### Fitted ambient/mic/speaker report; unsafe raw readers gated

Userconfirms2leglightcomponentonLEDboard, mic/speakerports andspeakerpresent.
Chipproximitynotrouteproof: ULN2803LVSsinkarray,MX6208motor/IRcutdriver. SourceALS
ADC1ch0sys/kernel/ain/ain0 hasIRQ-offunboundedwait; no read. SourcePCM cdev1capture/
cdev0playback has unboundedread/stop edges; noopen. ParentADChelperasm reviewed;
boundedPCMstart/ISR/AECsourceclosureactive. CTRL07/AUDIO01next, GUIexclusivehardware,
no guessedADC/pins/speakerdrive orcamera-feedresume. See parts/audio cross-check.

### User confirms motor/white/IR mapping — ambient and microphone next

CorrectedGUIwhiteGPIO5+IRGPIO23 physicallyONconfirmed byuser; IRdetectedexternal
filterlesscamera. Motoraxes/signs alreadyuserconfirmed. NarrowCTRL03/05/06 mapping
increment, notfulltravel/current/thermal/product gates. Oldnegative/blocked rows
are preservedhistory. NextCTRL07 actualambientroute/read effects +AUDIO01 exact
capture-onlystockPCMABI/retirement underboundedsource review. NoADC/micopen/start/
speaker orcamera-feedwork; parentonlyhardware, no competingGUIUARToperations.

### Physical directions confirmed / manual LED GUI correction

UserGUIfeedback: M0left-right +clockwise/-counterclockwise; M1up-down +towardcenter/
-towardedge inobservedrange. Directionmappingrecorded, notabsolute/home/limit/current
acceptance. LEDcommandsblockedbyabsentBusyBoxtr, notopticalnegatives; fixedusing
shellread,9hosttests andactualROzero-baselinesPASS. CorrectedGUIreopened noactuation;
IRilluminationawaitsuserretry, camera/feedstilldeferred. See mapping-firstcampaign.

### User manual FreeSimpleGUI bench — 2026-10-13

UserrequestedclickablePythonGUIandFreeSimpleGUI. LocalperipheralGUIlaunched;±32
motorbuttons/whiteIR5spulses/off, noGPIO2/homing/phase/fullrange. Preparationnative
queriesonly positions80/16/rate4/LED0;90helperhost+8Python+UIsmokePASS. Physical
mappingnotaccepted. Fresh13e catch23:46UTC;GUIchecksowner/runtime/recovery eachclick,
no secondserialowner. ParentdoesnotissuecompetingUARTcommandsduringoperatoruse.
Camera/feedintegrationstilldeferred. See mapping-first campaign andGUI README.

### Latest steering — finish LED/IR/motor mapping first

User explicitly prioritizes short motor tests and direction feedback, plus independent
IR detection, before more camera work. Nativefeed integration is deferred. First
announced motor0+16 nudge observedsoftware0→16/695ms/idle/STOP0/rate4restored/latch2,
no remainingmotor/watchdogowners. Physicaldirectionawaitsuser; motor1notmoved.
SourceGPIO5whiteconfirmed, GPIO23IRunverified, GPIO2excludedUARTconflict. Current
stockproviders additionallyAW9523/motor; no capture. See
[mapping-first campaign](../../qualification-plans/stock-reference-20261013/MAPPING_FIRST_20261013.md).
Next: obtain motor0 direction feedback, then one separately announced motor1 nudge.

Subsequent userreportsNOvisible/audible motor0 response for16 and64step tests,
despite GPIOlatch/IRQactivity. Stopdistanceescalation; boundedstockinit/enable source
reviewactive. SeparateM1+16actualsoftware0→16/691ms/idle/STOP0/rate4restored/latch2,
IRQ22+43/IRQ23stable134; physicalresponseawaitsuser. Softwarepositions80/16 not
coordinates. IRawaitsuserdetector; no camera/feedwork resumed.

### User-requested status / remaining-work report — 2026-10-13

See [status and remaining work](STATUS_AND_REMAINING_WORK_20261013.md). Parent
independentbuild12 confirms205core+40adapter optimized/sanitized cases and darkABA
oracles, byte-identicalHOLD ARM. Nativepointwiseclockfilter and retailIR-getter/
console-identity review remain open; no targetstaging/capture. Report is not a pause
or hardware authorization. Lastbenchcheck19:05/13d bound21:04 requiresrevalidation.

### Receiver rotation — 2026-10-13, 19:05 UTC

13c's bounded recorder expired normally and finalized/hashverified. Freshsole13d
receiver/futurebootcatch established, bounded21:04:42UTC; freshactualreadback stock
3.4.35/minimalwatchdog+sensor+camera, IRQ0/nouserdeviceowners/latch2. No capture,
lightchange orreboot. Historical13c/13a timers are not protection. Fixednativeadapter
integration remainsoffline; source/host/instance/reset review precedes launch.
See stockreference ACTUAL_RESULTS and fresh13d receipts; revalidate beforeoperations.

### Stock feed admission update — 2026-10-13, 17:06 UTC

Subsequent parent review corrected offline phase attribution/readbacks:
188optimized+188sanitized independentpasses; originalbacklogcounterexample nowzero
stalesamples. StillHOST-only/nativeHOLD. ISP/videoopen and extraFPS-helper effects
are source-resolved; enabledAUTO3DNR reference address/extent/backing remains under
boundedreview before nativeadapter/clock/reset admission. No capture/IR/FPS proof.

Later source closure resolves reference creator: separate0x384000-byte/900-page
owner, threeaddress/lengthpairs at fixed1280x960referencecapacity (not outputheight).
Parentacceptedexactadditionalownerproposalfor offlinefixednativeadapter integration;
no secondallocation/nativeexecution. Compiledcompletionclock and nativegate/reset
admission remainopen. See stockreference `feed-entry/PARENT_NATIVE_ENTRY_PROPOSAL01.md`.

Native allocation-only8MiB ION increment passed with2048 contiguousPFNs and
ordered cleanup; no payload/capture. Exact retail uncached mapper and USERPTR
prefix-validation correspondence are source-closed. Parent checked24 tuning
blocks/34 realsetter ioctl copybounds; remainingextraAE/ISP-init effects precede
native entry. A fixed finite capture candidate is offline only, stillHOLD.
No sensor table/queue/feed/IR or factory-deliveredFPS result is claimed.

Recorder13a sealed/hashverified; failed13b rotation preservedTX0. Freshsole13c
receiver/nextbootcatch is bounded19:04:16UTC;17:06readback stock3.4.35 minimal
watchdog/sensor/camera, IRQ0/no deviceuserspaceowners/latch2. Revalidate before
operations. Historical13a deadlines below are not current protection. See
[stock actual results](../../qualification-plans/stock-reference-20261013/ACTUAL_RESULTS.md).

### Stock sensor original-byte correction — 2026-10-13

Exact retail REA checker/loader and immutable tuning-file tails establish a full
**140-pair/560-byte sensor payload**, final originalpair0x0100=1. Earlier139-pair
extraction omitted that tail by treating a payload-only length as header-inclusive.
Originals/139 guarded stream-off preparation/kernel75 remain unchanged; never
change count alone or silently enable streaming before receipt/admission. See
[corrigendum](../../qualification-plans/stock-reference-20261013/feed-entry/SENSOR_LENGTH_CORRECTION.md).
Current stockrootrescueruntime has no feed/queuedbuffers; original24MiBIONheap
id1/mask2 tuple source+live-bound. Allocation-only helper and final pack-loader
review active; fullcapture stillrequiresbacking/cache and retained-until-reset
retirement policy. No GPIO0 test ran; GPIO2 excluded afterUARTconflict/recovery.

### Recovery observed; stock video metadata collected

User reboot afterGPIO2 UARTloss caught by13a; freshhold/currentstockkernelCRC/root
rescue restored. **Current runtime is stock3.4.35 minimal root rescue** with original
watchdog/sensor/camera only; previousWLAN/config/card instances notinherited.
Source/data confirmsGPIO2 muxclear0x4 conflictsUART0 group10mask6/value6; no retry
ofGPIO2. Earlierprose gpio_value4 contradicteddecodeddata0, correction recorded.
NewpairedISP/video QUERYCAP/G_FMT/ENUM0 nativePASS, advertised1280x720YUYV/stride2560/
sizeimage1843200; cameraIRQ0 beforeafter/closeordered/noactiveframe. Bounded sensor
start/capturebuffer contracts stillunder source review beforefeed/IR A/B/A. No stock
app/homing/recording orblindGPIOs. See stockreference ACTUAL_RESULTS.

### Immediate recovery boundary — GPIO2 test / UART loss

User authorized feature-by-feature GPIO tests.5s GPIO5 white test physically confirmed
by user and readback restored. NextGPIO2 indicator test stopped UART after command
echo/ff00; no restoration receipt, user reports no visible effect. TargetstateUNKNOWN,
no further targetTX.13a sole receive owner/currentfuturecatch revalidated; request
user poweroff/wait5s/on withUARTconnected, thenobserveUboot. No inferredGPIO2restore/
cause. Preserve all failures; see stockreference ACTUAL_RESULTS and
`evidence/qualification/qc07f/stock-reference-20261013/GPIO2-UARTloss-no-visibleeffect-recovery-ready01.json`.

### Current stock-reference tests — 2026-10-13

User authorized running QC07F stock module tests. Actual unchanged stock3.4.35
root PID1 rescue now running (not held U-Boot, not replacement75); normalapp/
recording/cloud startup suppressed. Root/usr/configRO, SD softwareRO/unmounted,
original providers loaded by reviewed steps, stock watchdog60/nowayout0/nodeamon0
with kernel idle feeder.13a ownership/nextboot hold requires fresh revalidation.

See [actual results](../../qualification-plans/stock-reference-20261013/ACTUAL_RESULTS.md):
factorydriver MTU1500 before/after authorized association/DHCP, original2048
firmware download succeeds, finite DF1500-IP/TCP256KiB each direction pass.
Stockcard58.2GiB/FAT32 geometry and zero invalidFSINFO preserved, no repair.
Board5/sensorDA4D/0x1346/reserved24MiB; actualmotorGET idle4phase2400/600 vsstored
8phase2000/700; sensor cached30 vsstored main/sub15fps, ISP blockzero unconfigured.
GPIO5/23 output/input changed/restored, **user saw no visibleLED change**. Latest
user refinement requires off/on/off camera-image/IR comparison with exposure/gain
controlled; no activefeed yet. Higher-levellightpath and boundedstockfeed entry
being audited. No blindGPIO/PWM/homing/fullapp or guessedcapture commands.

### Reviewed completion strategy — 2026-10-13

Latest user refinement: investigate stock kernel/application GPIO motor/white/IR
behavior, factory video frame rate and Wi-Fi MTU as an early reference. See
[stock reference campaign](STOCK_REFERENCE_CAMPAIGN_20261013.md). Static REA/script
audit first; controlled root-access stock boot/tracing only after exact image/
instrumentation/commands/startup-write/output/recovery scope is established.
MTU384 is a replacement workaround, not an observed factory/card limit. Compare
actual factory MTU with USB framing/maxpacket/aggregation before fixing replacement.
This reference window now precedes same75 restoration/implementation. Held U-Boot
unchanged; no stock kernel/app/output/network command launched during review.

Independent static review returned: SC1346 init labels30fps but printed pixel clock
is derived from an assumed30; requested15/13 use adjustedVTS divisors, not proof of
factory delivery. Board5 motors use directGPIO10/11/19/20 and15/14/13/24; STOP can
decelerate/wait10s, not immediate physical stop. Public motor demo ABI differs from
retail; no safe high-level retail CLI or ready-to-enable GPIO logger established.
Generic gpiolib tracing misses vendor direct-MMIO writes. Stock reference remains
at static admission: safe startup isolation/command/provider/instrumentation before
output trials; factory MTU still unmeasured. Detailed citations in stock campaign.

User powered on and requested review/strategy before implementation. **Actual
U-Boot catch observed13:34:23UTC; held `anyka$`, no kernel handoff.** Fresh13a
recorder remains the receive owner; its one-shot catch is consumed, not a pending
recovery arm. Revalidate/rearm before a later target experiment. No Linux/WLAN/
private service from the prior power cycle is inherited.

Read [project review and completion strategy](REVIEW_AND_COMPLETION_STRATEGY_20261013.md)
as the concise next-work entry; previous checkpoints below are preserved history.
Priority: repeatable75 baseline → native UI/isolated verifier → evidence-backed
media memory admission/finite handoff → direct protected browser+RTSP slice →
performance/imaging → physical controls/audio/storage → recording/durable setup →
actual NVR/combined product acceptance. Shared-resource/storage/AP feasibility
investigations run early in parallel, hardware always serialized. Limit work to
one parent hardware campaign plus two disjoint source lanes; integrate existing
cores before extending the candidate backlog. This review ran no builds or target
commands; all product gates remain open. Next target action after review is fresh
protection and checked same75 RAM restoration, not booting guessed retained RAM.

### Explicit resume — 2026-10-13; supersedes the pause below

User requests continuation; camera reported OFF. Fresh `foundation-20261013a`
receive-only owner/TX0 verified and bounded17:28:33UTC U-Boot hold armed; ready
for user power-on, **no boot/catch yet observed**. See [resume boundary](RESUME_20261013.md)
and its exact ownership/arm receipt. No previous i runtime/address/timer inherited.
Next: observe actual U-Boot catch and revalidate before restoring/sealing same75
RAM baseline. Media-v6 HOLD/RTSP untested tail/nativeUI/product gates remain open.

### User-requested PAUSE — supersedes all execution history

**Final closure recorded2026-10-13:** all source workers stopped. Media-v6 sealed
offline SOURCE/HOST checkpoint; dated migration forecast stillfails (5476KiB aboveW
versus11548KiB raw-reuse requirement). Parent review/rerun pending after resume,
ARM neverexecuted/nativeHOLD. Isolated-server partial/tested-vs-untested tail remains
explicit. See [PAUSED_STATUS.md](PAUSED_STATUS.md) and final worker seals. No tasks,
tests, target operations or automatic queue advancement remain active.

RTSP isolated-server worker has stopped/sealed a **partial** checkpoint: tested
integration05 HOST47 campaigns/86761 optimized/86425 sanitized, ARM not executed;
three current fixture edits unbuilt/untested, parent review pending. No adoption/
validation now. See paused status and `evidence/delegated/20261012-rtsp-isolated-server/HALT-SEAL.json`.
Media-v6 seal is now recorded above; pause/no new work is unchanged.

**Do not continue automatically.** Latest user steering requests status, finishing
active checkpoints and halting work. See [paused status and remaining work](PAUSED_STATUS.md).
Target last observed same75 RAMLinux/authorized WPA2_CCMP WLAN384, ISP idle/guards0/
UART0/watchdoginactive; no qualification listeners. Recorder i finalized/hash-verified
and stopped, serial owner/control socket/future arm absent. **No current recovery
protection.** UI03 cancelled before target transfer/launch (zero public requests).
Guard-only native7.046s firstexec/ping/wait0/poststate passed narrowly; parent recipe
121 models passed. Motion v2 and profile-transition v2 independently reviewed HOST
only. Media-v6/isolated-server workers sealed and stopped; no native adoption.
Resume requires user request and fresh owner/runtime/recovery, never historicalPIDs.
All product gates open; noSPI/SDdata/envsave/Git/publication. Earlier “active” rows
below are preserved history, not authorization or current state.

### Current checkpoint — 2026-10-12; supersedes all history below

Motion/mask candidate returned bounded32×18/eight16-vertex polygons/4KiB state,
HOST-only. Parent SOURCE review HOLD: final source-valid callback could revoke
policy after the last policy check; require current post-callback LIVE/EVENTS
proof before tile/background/event effects and explicit acquisition timing/loan
tracking. Native luma source/lease/cache/digest/tuning remains unqualified, no real
frames/recordings/SD operations. Media-v6 worker returned no text; partial raw-reuse/
migration source is unsealed, request final report/closure and keep native HOLD.
Proof `motion-v1-postcallback-authority-hold01.json` parent-review. Guard-only
pack/profile-close fix/isolated-server integration continue; no new target job.

Profile-transition candidate returned20/1151 HOST integration checks, parent SOURCE
review HOLD: active `pt_close` treated failed metadata query as permission to unlock/
invalidate. Require current source-known terminal report and retain unresolved
context; focused correction requested, no native radio/store/key work. Guard-only
finite source pack pending after independently reviewed7+7 hazard tests; no UI3/
producer invocation. Actual memory/zone readback after WLAN restoration preserved
for future admission, not allocation permission or old h measurements. All gates
open; `profile-transition-v1-close-query-hold01.json` under parent-review.

Latest actual bench: same75 RAM Linux + authorized VertexHotspot WPA2/CCMP/DHCP
192.168.1.73/24 MTU384 restored; ISP idle/guards0/UARTerror-busy0/watchdoginactive.
No new HTTPS/RTSP/media/audio/MMC/PTZ candidate is running. i02:28:57Oct13 hold
remains dated: revalidate current owner/runtime/recovery before any operation.

Parent mistakenly selected `radio native-load`2048, already known unqualified:
first2052-byte wire transfer timed out/E110/TX4 FIFO warning. Original failure
preserved, no APP/runningFW assumption. Original75 successful acquisitions prove
`radio load`256 was the qualified path. Prior source47 raw receipts prove guarded
USB75 unbind/rebind/power42 low2s cold reset; same source/actual no modules/device/
5s guard checked, reset observed, explicit256 load/CRC/RUN then APPff passed.
Exact paired75 station(no_bootdl=1)/DHCP shim + private guest restored, credentials
never logged/hashed. No automatic2048 retry, SPI/SD-data/saveenv/AP/default changes.
Proof `same75-authorizedWLAN384-baseline-restored01.json` in native02 evidence.

Audio-media v2 independently410499 optimized/410495 sanitized +C++/ARM/codec
oracles reviewed, pre-borrow authority fixed. Native8k mic/A-V/browser/NVR open;
next independent lane bounded motion masks/statistics, not recordings/SD effects.
Guard counterexample independently7 real HOST cases+7 source tests: old pre-exec
check produced >5s without periodic pings, hazard not native02 cause. Candidate
preserves5s/4s/45/2M/180/140, guard-only finite native pack is next before UI3 or
producer. No native watchdog schedule/availability guarantee from HOST tests.

Restored75 baseline remains actualRAMLinux shell/ISPidle/cleanUART/watchdoginactive.
Asynchronous `random: crng init done` observed; not cold entropy/trust provenance.
Same qualified paired75 station/DHCP public pins reverified; station-only module
RAM staging active **sh_13258a29b001pZtMlKMekrmPM1** (no autoload). No concurrent
UART targetTX until actual hash/termination. Authorized guest restoration is a
separate existing baseline step, not AP/profile-transaction/nativeUI retry. WLAN
still not restored/assumed; private credentials remain off source/logs/images.

**RAM restoration completed/direct boot observed; current target is75 RAM Linux
shell, not loader/U-Boot.**37,237 frames/zero retries/fullSHA5e37.../BZIP2a0db4e80
and QCL commit passed; independent U-Boot entire staging CRC matched. Pinned
volatile bootargs/CMA16/system40 and qualified boot-only60s startup protection
reapplied unchanged, actual image/header/checksum/Linux monitor seen. Initial8s
observer lacked completed monitor; later actual prompt/startup magic-close observed,
original short window retained. Current public notes/config hashes match sealed75,
ISP idle/guards0/UARTerror-busy0/watchdoginactive. No WLAN/key/service state
inherited and no unchanged nativeUI02 retry. i fresh owner/bounded02:28:57Oct13
hold remains dated; revalidate before operations. No SPI/SD-data/saveenv writes.
Proof `same75-restored-direct-boot-poststate02.json` in native-ui/native02 evidence.

Finite RTSP entry source pack independently220 HOST checks each/ABI6/C++/ARM link
reviewed; no PRIVATE generation/native/ARM run. Next actual qualification stays
HOLD until restored75/runtime/fresh recovery and guard-failure review; first proof
is readiness/isolation/lifecycle only, not native positive Digest or media/NVR.
Proof `native-rtsp-verifier-pack-review-boundary01.json` under parent-review.

Finite RTSP native-entry source pack returned HOST220 each/ABI6/C++/ARM link;
parent full code/path/custody/lifecycle review done, independent rerun active.
No target-side password reader or native positive VERIFY; first future entry only
proves real strict root-origin/READY15/2M/provisioning/owned wipe+wait, under fresh
qualified recovery. Earlier worker entry01–04 output-scope errors disclosed and
preserved, final builder confined to assigned evidence lane. Current exclusive
same75 framed UART restore remains active; no verifier/device operation during it.

**Actual bench: existing RAM-only loader, exclusive pinned75 restoration active.**
NativeUI02 strict2MiB READY/client-on-READY reached, but ~5s later watchdog reset
was observed. PC client cancelled; no full TLS/login/retirement PASS. UART held
U-Boot, whole reset invalidated Linux/WLAN/oldprocess authority. Guard's first-exe
check/fork transition hazard is under offline investigation, not measured cause;
no timeout increase or unchanged UI retry. Retained75 RAM fullCRC ec3f11f1 !=
a0db4e80, refused boot. Original h capture finalized/hash-verified unchanged;
fresh i sole A25 receive owner/new bounded hold through2026-10-13T02:28:57UTC
established. Revalidate, do not inherit PID/timer.

Existing QCLDR1872B read from owned SD/BZIP2 dd0572e1 verified; known masked volatile
SDMUX3fc0->15c0 preserved other fields. No SPI/SD-data/saveenv writes. Current
QCL_READY_RAM_ONLY and **sh_13240dda10012UTA73u3Xxdnl5** sends exact existing75
3,574,675B/SHA5e37... to0x82408000..0x82770b93 via framed UART. **No other targetTX**
until actual completion/fullCRC/commit; Linux not restored/booted yet. This is
restoring qualified RAM baseline, not retrying nativeUI02 or a new image.

Private-profile v2 independently57/1220 each +NIST/RFC/C++/ARM link reviewed,
native key/unseal/checkpoint/store/isolation stillHOLD. Next offline unit ties real
private handles to reviewed network transaction adapter. Audio-media source review
requests pre-borrow authorization (previous ac_borrow occurred before policy),
no ring-starvation/source effects from denied consumers. Existing audio drivers
remain no-effect refusal. All product/native/FPS/NVR/other-port gates open.
Detailed actual failure/restoration evidence in
`evidence/qualification/qc07f/native-ui-20261012/native02/`.

Private-profile AES-GCM/HKDF/owned-store candidate returned, native unconditional
HOLD before key read. Parent SOURCE review found read/use relied on cached startup
checkpoints after possible external authority advance. Live scope/key/nonce/commit
revalidation before export/private effects is requested; no stale checkpoint
restamping, payload hashing or native key/store/radio acceptance. Crypto config/
objects remain isolated; secret fixture inventories excluded. All SEC/G6/G8 gates
open, evidence `private-profile-v1-live-checkpoint-review-hold01.json` parent-review.

RTSP verifier v2 independently reviewed/passed217 real-child +82 startup-boundary
and18 engine checks each, C++/ARM child+adapter links (ARM not executed). Owned
pre-exec barrier now prevents material read before pidfd; executable image and
regular/sealed material FD rules remain strict. HOST4M is not native2M/root-origin
proof. Next isolated source increment is a finite native READY15/2M/lifecycle pack,
no RTSP listener/source enablement or target-side password reader. Native positive
Digest/effect integration/client/security/product gates remain open. Evidence
`rtsp-verifier-v2-review-adoption-boundary01.json` under parent-review.

Audio v2 independently reviewed/passed39728 optimized/sanitized checks, C++/ARM
static link and Linux6 kernel objects using sealed read-only headers; no ARM run.
All seven pending-operation deadlines now cover startup/teardown, with exact-owned
STOP or retained fence quarantine, no retry/free-on-timeout. Native probe still
refuses. AUDIO lane advances to bounded PCM/G711/timestamp/span source candidate;
real mic,8k mode, A/V/browser/NVR compatibility remain unqualified. Proof
`audio-v2-review-adoption-boundary01.json` under parent-review. Other active source
lanes continue; no new native operation or product gate closure.

Native UI entry v3 returned95 injected models but parent SOURCE review HOLD:
16KiB reply cap cannot hold real32768-byte recent-RX status/JSON expansion; fresh
target UART-health requirement on every local RPC cannot be met without hidden TX.
Correct same entry to bounded audited status capacity and actual tracked health
barriers before target effects, no absent-counter-as-zero or new observer/daemon.
Preserve evidence creation time separately from current same-instance revalidation;
never restamp dated arm/kernel/staging receipts as fresh observations. Source fix
requested, native02 not staged/run and native01 partial result unchanged. All gates
open; `ui-native-entry-v3-cap-health-provenance-hold01.json` under parent-review.

RTSP isolated-verifier and capture-only audio source candidates returned. Parent
review HOLDs: verifier must gate exec/material read on actual owned pidfd and
validate native executable/material FD metadata; audio must handle deadlines in
CLAIM/CONFIGURE and STOP/RELEASE waits, not capture only. Both fixes requested;
no native child, microphone, ADC/clock/L2 or listener enabled from fixture passes.
h receive owner/control-peer/sole adapter/arm revalidated21:18UTC with5848s remaining
to original current-instance22:56 bound, no targetTX/current runtime inference.
Revalidate exact owner/runtime/protection again before operations. Holds and source
receipts live in parent-review; native/product gates remain open.

Network transaction-store v2 independently reviewed/passed400 contexts/7510 checks
each optimized/sanitized, C++/ARM link (not executed), and original deadline
counterexample reproduction. Shared sanitized runner now30 C +2 Python +76 receipt
PASS, host-only. Public896/1024/1872-byte format ceilings unchanged. Private-profile
crypto/store source unit is next; actual key/entropy/store/radio qualification not
inferred. Evidence `network-transaction-store-v2-review-adoption01.json` under
parent-review. UI campaign boundary v2 independently63 models pass; existing
receiver/command entry binding is being implemented offline before native02.
Media memory/backing-source audit and capture-only audio/RTSP verifier lanes
continue; hardware remains parent-serialized, all product gates open.

Reset-input candidate independently reviewed/passed252584 optimized/sanitized
checks plus C++/ARM link (not executed); no reset executor or native button proof.
Peripheral lane advances to capture-only ADC2/ALSA source candidate, no device
activation. Network transaction-store896B codec/actual backend integration returned;
parent review requested fresh dispatch time after post-fsync admission callbacks.
UI parent-campaign44 models independently pass; actual entry integration/native02
still pending, lexical dot/dotdot/scope collision correction requested.

Native v5 memory candidate remains **HOLD**: non-CMA forecast omitted worst unpaid
CPU placement/CMA-loan migration. Actual supplied Normal-zone forecast is704KiB
short for v4, before v5's32KiB addition. Correct the same gate, do not allocate in
hope of reclaim/watermark changes. Source audit of temporal rawCPU-to-NAL backing
reuse is queued under unchanged kernel copy/reset/CRC contract, not permission for
aliasing or a native run. No reserve/quality/check/deadline reduction. All gates
remain open; evidence `native-handoff-v5-migration-forecast-hold01.json` and
`reset-input-review-adoption-boundary01.json` under parent-review.

PTZ v2 authority correction independently reviewed/passed11466 optimized and
sanitized checks, C++ link/run and ARM link without execution. Original unauthorized
neutralization regression retained; native routing/current/STOP remains blocked.
Queue advances to input-only deliberate-reset debounce/events, not a reset executor.
Media admission additionally reviews non-CMA allocation placement under the actual
zone watermarks; eligible CMA borrowing is not guaranteed headroom. No native
media/control/product gate closed. Proof `evidence/delegated/20261012-parent-review/ptz-owner-v2-review-adoption-boundary01.json`.

Latest completion-event advancement: RTSP v3 independently reviewed/passed36
HOST campaigns plus real RFC/backend, wipe, C++/ARM-link checks. Agent advances to
RTSP-only isolated verifier/owned bounded IPC; existing runtime staysNOAUTH/501.
PTZ owner v1 returned but parent found a pre-auth malformed-command neutralization
path. HOLD/correction requested before adoption; no motor operation or physical
STOP proof. Media memory gate, corrected UI client-on-READY campaign and network
metadata adapter continue independently. Evidence is in parent-review; no product
gate closed and no source/fixture was substituted for native qualification.

Current increment: g was actually still alive after the restart; an empty parent
IPC probe caused its JSON/BrokenPipe error and finalization. Error receipts/hashes
are preserved, not called clean shutdown. Fresh h sole owner/proper JSON IPC and
new22:56:14UTC recovery established; revalidate before any operation. Actual
running GNU build ID `dc6e03bfe80f5fcce5258b9255fb473a328d6cb8` matches sealed75
vmlinux, as does IKCONFIG. Public kernel notes are not runtime attestation.

Parent independently reviewed/reran v4 handoff42/adapter41/TCP24/receiver16, RTSP
server v2, MMC v2 (3230 checks each), serialized UI134 real-TLS checks and matched
launcher lifecycle. Actual dummy browser loaded8 modules once/in order without
overlap; full native TLS/browser entry remains separate. RTSP v3 destructive-release
correction returned, independent review/rerun next; original NOAUTH server unchanged.

Native UI01 reached strict service/child READY at unchanged2MiB and naturally
retired at180s: service/child exit0, no lineage, rootwait0, no reset and healthy
poststate. **No native client/asset/login tests ran:** parent marker parsing/echo
false-positive delayed admission past the safe window. Partial/orchestration result
retained, no automatic second launch. Known campaign RAM secret/public copies
removed only after observed retirement; PC originals/receipts preserved, empty dirs
left because rmdir is unavailable. No footage/firmware/persistent settings touched.

Next active increments: native strictMemoryReserve before guarded139/off-board
acquisition; corrected parent UI client-on-READY campaign; durable network metadata
adapter; bounded PTZ owner candidate. Non-CMA reserve uses observed Normal zone
watermarks (plus empty Movable), not a guessed DMA-zone floor. MMC provider/routing/
L2 ownership still blocked, DEVMEM disabled; no register/card/mount/write bypass.
Media501/RTSPNOAUTH and all product/G3/G8/FPS/other-port gates remain open.
Evidence: `evidence/delegated/20261012-parent-review/queue-round03-review-results01.json`
and `evidence/qualification/qc07f/native-ui-20261012/`.

Another server restart cancelled g receiver: all g PID/socket/20:49 protection
claims below are historical, NOT admission. No native candidate ran. Continue
queue and preserve interrupted logs before rebuilding fresh owner/runtime/recovery.
Isolated nativeUI candidate returned491+127 realTLSchecks/maxfullreply7491 and
unchangedstrictchild; parent staticdiff reviewed, serializedmoduleloading correction
is next because2peer/1handshake cannot be waived or inferred from serial tests.
Native off-board/exact139 guarded preparation, RTSPserver and MMC candidates
continue independently, subject to parent review. RAM-file launch stillHOLD;
all native/product/FPS/SD/client/otherport gates open, actualmedia501/RTSPNOAUTH.

Queue continues after server restart: f recording cancelled, originals preserved;
new sole g owner/recovery20:49UTC and actualLinux/ISPidle/WLAN384/UARTclean restored.
RunningIKCONFIGbyteexact75cfg22acc... verified; oldPID/timer/source cannotinherit.
Sensor HTS1650 != retained139-table1800, no writes yet; explicit guarded preparation
pending. RAM-file handoff launch HOLD: worst addedpayload14,039.5KiB>measured13972KiB
MemAvailable before overhead; bounded off-board qualification sink replaces target
file, no deadline/memory/integrity relaxation or new live publisher.
Parent independent source/host results: handoff40/adapter24, RTSPwire3489, index2262/
25FS, shared29C2Python76 receipt, player78+actualPUBLICDUMMY UI refresh/login/logout.
Native encoded/handoff/TLSmedia/SD/NVR/performance gates still OPEN, media501/RTSPNOAUTH.
Next isolated assignments already advancing: guarded139+sink candidate, RTSPserver
candidate, HTTPS/UI-assets candidate, read-onlyMMC host-port candidate. Parent
reviews then advances real native qualification within fresh protection, no blind
retry/protocol downgrade/false completion. Full ownership/remaining-lanes status:
[PARALLEL_WORK_20261012.md](PARALLEL_WORK_20261012.md).

User requests continuous queue advancement through stages, not stopping on bounded
completion. Queue round02 launched: native producer-handoff build candidate,
RTSP wire/challenge boundary, authenticated MSE player and portable recording-index
metadata storage. Disjoint ownership and review/actual-proof gates are in
[PARALLEL_WORK_20261012.md](PARALLEL_WORK_20261012.md). Parent sole hardware owner;
source ELF pending review before execution, currentf/arm rechecked15:28UTC through
18:03:42UTC. Advance on real results; explicit physical/client dependencies block
only their lane, not independent work. No failed retry/ceiling relaxation or
15fps/exact-hardware guarantee invented; all G3/G8/product gates remain open.

Delegation round completed and parent reviewed/corrected/reran all isolated
deliverables. Details/remaining independent lanes:
[PARALLEL_WORK_20261012.md](PARALLEL_WORK_20261012.md). Parent independent Digest566
optimized+566sanitized, recording15suites/4680checks, network90contexts/18667checks,
and shared28C+2Python+76 receipt PASS. Catalogue alias/reservation bug, Digestcnonce
interop restriction and stale rollback evidence corrected; old failures retained.
Performance/peripheral/audio/NVR documents reviewed. All new cores are source/
host-fixture adoption only, NOT native authserver/recording/reconnect/product proof.
Media501/RTSPNOAUTH/T34Fulhan deferral remain unchanged.

Parent preserved stale oldsocket/originale logs, established freshsole f receiver
and bounded18:03:42UTC nextboot hold, then read-only actualRAM6.18.55/WLANMTU384/
ISPqueueIRQidle/guards0/watchdoginactive/UART0 observation. Kernel release alone
doesn't seal exact75 image/source ABI or encoded retirement. Revalidate actual
owner/runtime/arm before operations. Next remains bounded native producerhandoff/
source qualification then protectedTLS/browser and independentRTSP integration,
using reviewed lane APIs, no addedguardian/transport phases or ceiling relaxation.
Proof `evidence/delegated/20261012-parent-review/review-and-adoption01.json`.

User now requests independent task lanes, delegation and parent review. Active
breakdown/ownership/review gates:
[PARALLEL_WORK_20261012.md](PARALLEL_WORK_20261012.md). Five isolated agents cover
RTSP Digest, recording catalogue/retention, network transactions, performance
evidence and peripheral/audio/NVR dependencies. Parent retains sole hardware/native
stream integration and central runner/status ownership. Results are pending review;
no new gate is passed or runtime service enabled by delegation.15fps remains a
target needing exact-board evidence, not an assumed documented guarantee.

**WEB-02/STREAM-08 bounded C init/profile/cadence and handoff increment fixture-
passed.** SPS/PPS geometry/association/level31 and supported baseline subset are
checked; C init644B/live duration0 +112B fragments +8192B borrowed spans, actualRTP
wrap/relative64DTS/stale-generation/IDR/atomic denial state. Existing producer1MiB
output +onehub1MiB handoff ordering model progresses without producer-held deadlock
or acquired-slot overwrite. Per-chunk existing web admission and exactCPUlease
checks deny expiry/revoke/domain/health/profile/STOP effects. No scheduler/driver
change or native handoff/TLS/headroom claim. Sanitized26C+2Python+76 receipt PASS;
C++→C API link/run and ARMv5 staticlinks NOT executed pass.
ActualC-repacked retained source decodes11 pixel-identical720p frames; MSE decoded
11/ended5.008/error0 but reported1 renderer drop/no VFC, not zero-drop displayproof.
Evidence `evidence/qualification/qc07f/web02-init-cadence-handoff-20261012/REPORT.json`.

Next executable task: bounded native producer-handoff integration/measurement,
then actual protected TLS/browser delivery. Preserve CRC/guards/DMA receipts,
2s AU and2MiB locking ceilings; no source/transport scaffolding detour or inferred
performance. Media remains501/RTSPNOAUTH and all G3/G8/product gates open.
**13:01UTC metadata boundary:** intendedadapter present/socket present/no serial
owner, olde10:55 arm expired, no final_e manifest. No UART open/TX/runtimeprobe.
Investigate current ownership and preserve unexplained logs/socket before a fresh
recorder/runtime/recovery boundary; old PID/timer/socket is NOT current protection.
Last actual target observation remains05:09Linux/WLANidle, not a current assertion.
All background test/fixture server jobs from this increment completed.

**WEB-01 desktop retained-source compatibility increment passed.** Actual MSE
decoded11 native1280x720 AVC1.42e01f frames, duration5.008s from actual RTP timestamps,
mediaerror0/ended; FFmpeg pixels match the original11 source frames. All12 original
AUs remain intact; finalAU supplies next duration boundary. Init643B/fragment112B
headers + borrowed spans, no re-encode/tint/whole-AU TLS copy. Both preview axes
corrected. Browser callback9/maxpresented10 does not prove all11 were presented;
other desktop/mobile and live/combined-load gates remain open. Finite loopback
fixture server closed normally. Evidence:
`evidence/qualification/qc07f/web01-retained-h264-mse-20261012/REPORT.json`.

**WEB-02/STREAM-08 shared C fragment primitive fixture-passed:** bounded112-byte
header/borrowed payload, no allocation/I/O/auth. All11 header bytes match the
actual browser fixture; sanitized22C+2Python+76 receipt cases, C++ header/ARMv5
static link pass. NOT native execution or protected media. Next executable task:
implement bounded init/SPS/profile/cadence state + producer handoff with per-effect
authorization/expiry/health/lease checks. Existing one-slot producer waits for
delivery: next-timestamp lookahead must not retain its only slot and deadlock.
Measure bounded encoded-storage/acquisition handoff; no guessed FPS/full-AU-per-
peer copies/raised ceilings. Native HTTPS increment remains passed, media still501,
RTSPNOAUTH and all G3/G8/product gates open; target remains idle75Linux/WLAN with
no authservice/viewer/RTSP running. All prior ACTIVE jobs completed history.

**Native HTTPS authentication + owned lifecycle increment PASSED on QC07F75.**
Fresh04: real full600k login, certificate/hostname verification, status/capabilities,
CSRF/logout/replay, request-discard/KDF-drain/subsequent login and owner revokeACK/
old-cookie401 pass; immediateclient interval0.000856s, elapsed102.710s. With narrow
ARM119 return fix, ownedSIGTERM produced strictchild-retirement receipt, service/
bootstrap/rootwait0, no adopted/remaining lineage, no recovery reset. Poststate
05:09:56UTC: IRQ/queue/WB owner idle, guards0, watchdoginactive, UART0, authorized
WPA2CCMP/DHCP192.168.1.73/24 MTU384 up, no authservice/listener/client/RTSP/viewer.
Exactcandidate/service/compiler/runtime/source/receipt pins:
`evidence/qualification/qc07f/auth-signal-return-20261012/parent-native-HTTPS-auth-lifecycle-qualified04.json`.
Original stdio/orchestration/shutdown failures remain preserved. All active jobs
below are completed history. Freshsolee/recovery arm remains dated, revalidate
before every target operation; no timer/PID inheritance after interruption.

**Next bounded task: WEB-01 browser/fMP4 feasibility with retained qualified native
720p H.264**, then WEB-02 protected producer integration and SEC-05 independent
RTSP auth. This is not permission to advertise media yet: service still501, RTSP
NOAUTH LAB, cold entropy/trust/UTC/durable keys/combined-load and all G3/G8 gates open.
Keep 2MiB/600k/35HTTP40CPU180 qualifications unchanged; no renewed transport phase.
Observed source AUs137–139kB exceed existing128kB single heap allocation limit:
design fragment/header + borrowed spans with bounded send chunks, not full-AU
TLS-buffer copies or limit increases. Actual RTP timestamps supply cadence, not
ffprobe's nominal25fps. Current actual camera idle; next increment is offline.

Exactstation/DHCP completed and authorized guest WPA2/CCMP/DHCP192.168.1.73/24/
MTU384 restored. Newdf6f service/dd8c launcher/unchanged8bb3 child targethashes match;
freshprivate04 newRAMscope700/4files600/echooff/pacedmetadata redaction verified.
**One coordinated fresh04 native HTTPS + owned shutdown campaign ACTIVE
sh_12e814bc2001ofNXDUWtH0uecu**. It starts client immediately on actualREADY, owns
exact-exe SIGUSR1/revoke hook and SIGTERM stop. No other UARTTX while it runs.
After realcompletion independently inspect clientPASS/revokeACK/childretirement/
service/bootstrap/rootwait0/lineageclean and actualWdogUARTISP poststate. No
complete native lifecycle claim yet; all180/35HTTP40CPU/2MiB/600k unchanged.

Normalradio restore completed/RAMRUNack/actualAPPLICATIONff proof. Exclusive
station75+DHCPbridge serial staging ACTIVE sh_12e7c53040019Pds94cAvxEUuS; no other
UARTTX until completion/targethashes. Current restored75 RAMshell, no authentication
instance or capture running. Guestrestore/exactsignal04 ELFs/private04/immediate
client/ownedSIGTERM remain next; existing security limits and product gates unchanged.

Same75 restoration completed37237frames/0retries/nativeCRC/commit and independent
U-Boot fullCRC match. Protected actualLinux/PID1/CMA16/manualISP/builtincodec boot
observed, watchdoginactive/ISP idle; radioBOOTLOADER measured. Normal retainedradio
RAM restore observation ACTIVE sh_12e7b2456001W7mIdiNCqb6WLv: no other UARTTX until
completion/RUN/application proof. Then exactstation75/privateguest/newsignal04
service+pairedlauncher/private04/immediate client and ownedSIGTERM lifecycle test.
No timeout/security/headroom fallback; native shutdown still unqualified.

Signal-return repair build/review complete: ARM-only119 permission, real actual
filter ARM old-fail/new-pass and ordinary host SIGTERM-return regression pass.
Canonical HOST491 service/38 launcher campaigns pass; strict child8bb3... unchanged.
New service341012B/df6ff632... and matched launcher54292B/dd8ceade... exact pins;
runtime/source/header/license closures sealed under
`evidence/qualification/qc07f/auth-signal-return-20261012/`.
Fresh sole e recorder/adapter/proc identity validated, actual held U-Boot/new hold
recorded. Retained75 staging CRCe1ea16d7 mismatcheda0db4e80; not booted. Existing
1872B SDloader read/CRCdd0572e1/RAM_ONLY verified. **Exclusive checked75 upload
ACTIVE `sh_12e6aad02001SyGObPzfU0vJAA`**; no other UARTTX until real completion.
Fresh private04 inputs remain PC-only; new exactpublic ELFs prepared. Next full
CRC/commit/protected75/network restore, exactnewservice/matchedlauncher/private04,
immediate client on READY and observed owned SIGTERM/child/lineage retirement.
Native repaired shutdown and complete lifecycle remain unqualified; unchanged
180s/35HTTP/40CPU/2MiB/600k, no safety fallback or new transport phase.

**Latest: campaign03 completed, functional native HTTPS PASS; SIGTERM shutdown
FAIL.** Full600k login/status/capabilities/CSRF/logout/replay/discard-drain/owner
revocation ACK and old-cookie401 passed. Signal-driven stop lacked child-retirement
receipt; service_status=-1/bootstrap20, adopted child reaped/no remaining lineage,
qualified watchdog reset held U-Boot. Both results are preserved in
`evidence/qualification/qc07f/auth-stdio-native-requalification-20261012/native03-functional-PASS-shutdown-FAIL/`.
Do not mark the complete lifecycle or G3/G8 passed. All ACTIVE jobs below have
completed; old e timer/PID/owner claims require fresh validation before target use.
Next bounded task: ARM signal-return ABI repair/regression. Static failed-service
ELF has musl `__restore` issuing119 and `__restore_rt`173; parent filter omits119
despite installing ordinary handlers. No memory/security-limit change or blind
native retry. Diagnose/test source offline first, then exact rebuilt requalification.

Campaign02 wholeclientFAIL phase26 preserved, but server exitednormal0/childretired1/
bootstrap0/lineagenochildren1. Timestamps show client started121.627s afterlaunch8s
observation, so finite180s service expired ~50s intoclient, not a demonstratedcrash.
InitialnativeTLS/login/status/cookieCSRF/logout/replay assertions hadpassed; no
discarddrain/revoke/G3/fullPASS claim. Onefresh03 distinctprivateinputs/epochs,
same180/35HTTP/40CPU/2MiB/600k, launch-and-immediate-client orchestration nowACTIVE
sh_12e54e9e3001UsCpHALQ533mtP. No otherUARTTX while its revoke/normalstop hook runs.
Waitrealcompletion and independentlyverifyclientPASS/ACK/ownservice-childlineage
exit0/watchdogUART beforeclosingincrement; no deadlineincrease/sourcefallback.

Native correctedservice READY observed! Same75networkrestore/exactb2ace-service/
fd0d-launcher/8bb3-child targethashes verified; privateunique02 inputs protected/
metadata-onlyredaction, operatorPW PC-only. Pinnedservice reports READY under
unchanged2MiBlimits (oldmissingmarkers fixed atstartup), ownbootstrapPID107. First
parentPIDregexmissedallowedashprefix; correctedmetadata only/no service retry.
**Private native HTTPS client campaign ACTIVE sh_12e491547001OdkVBCemGOKTDC**,
bounded140s with actualfull600k/login/status/logout/replay/discard-drain andown
SIGUSR1revoke-ACK. No otherUARTTX while its source-controlled revoke hook operates.
Waitrealcompletion and independentTLS/result/ownedservice-child/lineageexit0/
watchdogUART/heap checks beforeauthPASS. READY is notlogin/media/G3/productproof.

RadioRAMrestore35s completed/CRC-RUN ACK, actualAPPLICATIONff confirmed. Current
exclusiveUART station75+ELFDHCP restore staging jobsh_12e408dec001RRb9QztLWsGPvQ,
waitrealcompletion/targethashes beforeanyTX. Normalguestconfiguration/private
redaction/IP/MTU restore next, thenexactcorrectedpublicELFs/private02/nativeHTTPS.
No newtransportexperiment/securityfallback, targetsame75RAMshell/captureidle.

Restoration update: same75 checked37237frames/0retries/nativea0db4e80/commit and
independentUBootCRC match, protectedLinux/PID1/CMA16/manualISP/builtincodec boot
observed. CurrentnormalretainedradioRAMrestore observation ACTIVE
sh_12e3e8087001RmFZPVtJpEDTEX; no otherUARTTX beforeactualcompletion/RUN. Nextpaired
station75/privateguest DHCP/MTU384, exactcorrectedservice/helper/private02 then
bounded nativeHTTPS. No source/hardware/memorylimit change; e10:55holdfreshrevalidate.

Focused canonical stdio fix rebuilt: full491 service/38 launcher host campaigns
and directwritev regression pass. New ARMserviceb2aceed7...341012B/matchedlauncher
fd0d22f2...54292B, strictchild8bb3...unchanged, native2MiB limits unchanged. Current
retained75 RAM staging fullCRC mismatched8719700f (expecteda0db4e80); NOTbooted.
Freshsolee/runtime/recovery validated; retained1872BloaderCRC/read/QCL_READY passed.
**Checked same75 restoration upload ACTIVE sh_12e2cfc62001UEzx6weIGiNgpE; no other
UARTTX until real completion/fullCRC/commit.** Privateunique02 inputs PC-only,
correctedpublicELFstagecopies prepared. Next independentfullCRC/protected75boot/
normalguestrestore/exactnewELFs/private02 then bounded nativeHTTPS qualification.
No limitraise/automaticimage reuse/nativeHTTPclaim; targetcurrentlyRAM_ONLYloader.
See auth-stdio-source-integration report and activecontinuation; migrationdone.

Repository checkpoint completed: canonical root `AnykaOpenCam`, local Git/main
initialized with reviewed source/data staged, no commit/remote/push. All nine
original ROMs preserved and canonical copies/staged bytes verified;19 plans
imported, docs archived/updated, renamed-framework21C+2Python+76 cases pass.
Private credentials/keys and bulky historical `work/` remain excluded/preserved.
See `evidence/migrations/20261012-anykaopencam/README.md`. Resume the SEC/WEB work
below now; migration is not a firmware write or product acceptance.

**SEC-02/03/04/06/11/12 and WEB-03/04: resume focused native authentication
source/evidence review and repair.** The one native HTTPS attempt finished without
required markers; reported results were `bootstrap20`, `service_status0` and
owned-lineage `nochildren1`. Native authentication is **not passed**; no memory
cause is inferred. Review the
captured failure and exact promoted `apps/auth-web*` source/provenance before
selecting a bounded regression. Do not weaken isolation/password cost or start
another transport/guardian/publisher scaffolding phase.

Post-migration focused result: actual failed ARM ELF `__stdio_write` issues EABI146
(`writev`); service filter allowed `write` but omitted `writev`. The same desired
owned-stdout iovec regression fails old filter and passes the narrow canonical
fix, with prior forbidden-syscall tests intact. Evidence:
`evidence/qualification/qc07f/auth-stdio-writev-20261012/REPORT.json`.
Corrected canonical app/service+matched launcher rebuild is underway; native
marker recovery/HTTPS acceptance remain unproved. No memory fallback applied.

New receive-only sole FTDI owner `foundation-20261012e` was established after
migration, initialTX0, with a freshly recorded next-boot hold through
2026-10-12T10:55:40.140908Z. Blankline actual held U-Boot was revalidated. These are
new dated observations, not permission to reuse a PID/timer after interruption;
revalidate before each target operation. Previous no-owner text below describes
the quiescent migration boundary, not this resumed owner.

Last directly observed target state is a qualified watchdog reset followed by
**held U-Boot**, not Linux or associated WLAN. The UART recorder stopped normally
and final hashes were verified. No current UART owner/recovery protection is
inherited: establish exact sole ownership/no competing transfer, actual runtime
and fresh bounded recovery before any target operation. Historical PIDs/timers,
addresses and “active” sections below are not authority.

Preserved narrow proofs: kernel75 RAM boot and strict READY flags15; public-dummy
full600k backend match/mismatch/issuance/revoke/owned retirement passed. Kernel74
raw-receipt expiry/resume passed16 full-CRC frames; kernel73 finite network H.264
passed24 frames at about2.2fps. These are target-tested increments, not current
runtime, native HTTPS acceptance, sustained15fps or G3/G8/product completion.
Protected media, Digest, hard-fault supervision and all required product gates
remain open; T34/Fulhan ports remain deferred until G8.

Continuation: `work/lightbulb-camera/foundation-20261011T041148Z/CONTINUATION_AUTH_KERNEL75_ACTIVE.md`.
Original acquisitions and source evidence remain authoritative. The local
workspace migration is uncommitted/unpublished; it neither flashes targets nor
licenses redistribution. Preserve private provisioning outside public manifests.

### Preserved pre-import execution history — not current-state instructions

Actual75 strictcredential backendPUBLIC-DUMMY full600kmatch/issuance/mismatch/
revoke/late-deny/ownedCPUretirement/exit0 PASS, priorREADYflags15 retained/no unlocked
fallback. No realwebcredentials/cert/HTTPS provision. No activeUARTjob/transfer;
75station75WPA2CCMP/DHCP/MTU384up, publicserviceunrun/noencoderpools. Nextcompleted
trustednativeHTTPSbootstrap source/hash review, privateuniqueFD provisioning then
boundedactualHTTPSlogin/status/logout/discarddrain/TLS/lock-memory measurement.
No moreguardian/transportphases; allmedia/Digest/hardfault/G3/G8/product gatesopen.
Freshd/runtime/04:19holdbeforeoperations, historicalPIDs/timersnotauthority.

Actual75strictnativeREADY flags15/nonroot1000groups0caps0core0nodump/2MiBmlock/
filter/FDclosure/ownedCPUexit PASS, INITimpossible0. No debugpolicychange. **Explicit
public-dummy full600kmatch/mismatch/revoke diagnosticACTIVE sh_12dad186b001teDlYQNHq8uUV6,
no otherUARTTXuntilrealcompletion/review.** Publicqualificationonlynot web/default
credentials; qualified5sparentWdog/probe63s bound/freshd04:19hold, no weakerfallback/
global65sproof. ThenprivateFD realHTTPScredential/TLScampaign afterbootstrapreview.
ServicepublicELFsstaged/unrun, allG3/product/G8portsopen/no newtransportscaffolding.

Actual75 WPA2CCMP/DHCP192.168.1.73/24/MTU384 restore PASS, privateconfigno values/
hashlogs/echooff/redaction+ACK discipline. Public reviewedHTTPS341012B9f480... and
strictchild62284B8bb3... staged exacttargethash; NOT executed/no keys/PW/INIT/
HTTPSlistener. NoUARTjob/transfer; ISPidle/watchdog/UART0. Nextnativeprobecompletion/
review/pureREADY thenprivateFD HTTPSlogin/status/discard-drain qualification.
Host491!=nativeauth/headroom, allG3/product/G8portsopen; freshd04:19owner/runtime
alwaysrevalidate, no renewedguardian/transportscaffolding phases.

Independent491realHTTPS availabilityreplay/sourcegate passed, no nativeauth claim.
Normal75 retainedradioRAMCRC/RUN ACK thenAPPLICATIONff verified (notnewexperiment),
preparingalreadyqualifiedguestnetworkforHTTPS. **CurrentexclusiveUART station75+
preservedDHCP ELFstaging sh_12d8ead97001D2gfz2AmYmgVBd; waitrealcompletion/hash before
TX.** Thennormalno_bootdl1/privatepacedconfig/actualWPA2DHCP/MTU384. Native strict
credentialprobe nextauthgate; no HTTPS/secretINIT/media orproductclaim, no revived
guardian/transportphases. Freshd/runtime/04:19hold revalidate beforetargetoperation.

AUTHavailabilityrepair491realHTTPS/36full600k cycles returned, parentfocusedsource
review/noauthbypassdemonstrated; exactprincipalrevoke-before-reuse andpeerlocal
discard/KDFdrain preservefilters/throttle/gens. IndependentrealTLSparent replay
sh_12d84bff6001belQqpTfUwqI5z active(no target). Nextnative75 strictcredentialREADY
afterprobecompletion/review, thenprivateFD HTTPS; current75idle/noINIT/UI/network/
transfer. Hardfaultsupervisor/media/RTSPauth/nativeheadroom gatesopen, no newlab
transportphases. ActualUART/runtime/d04:19holdfreshvalidate beforetargetaction.

AUTHactualHTTPSservice01 source/runnablehostARM/109realTLS+600k checks returned,
parentcompleteentry reviewdone/noauthbypassdemonstrated. DeploymentHOLD forcancel-
wholeinstance and16-issuance availabilitylimits; focusedproductservice repair
underway, no weakening/securityfallback/newlabtransportphase. Native75pureREADY
probe nexttargetaftercompletion/review; actual75idle/noUARTjob/network/secretINIT.
Hostheap/KDFtiming are notnativeproof; media501/RTSPNOAUTH truthful/G3stillopen.

Actual75boot:37237frames/0retries/native+UBootCRC a0db4e80 thenprotectedLinux/PID1/
CMA16/manualISP/builtincodec/RAMshell, actualidle/guards0/watchdog/UART0 observed.
No activeUARTtransfer/job/encoderpools/network/UI/secretINIT. Nextcompletednative
credentialprobe review/stage/exacthash/strictnonrootcap0mlockfilter READY before
secretINIT; HTTPSservice continuesoffline. Rootdebugshellfilter0expected/notchild
proof; fresh d/runtime/04:19holdbeforeactions. No moretransportscaffolding, all
nativeauth/G3/product/otherportsG8 gatesopen; activeAUTH75continuationtopupdated.

AUTH75package/config/source/CRCs reviewed; fresh74idle/drecovery+qualifiedresetheld
U-Boot/retainedSDloaderCRC passed. **CurrentexclusiveUART checked75upload
sh_12d3586ac001PN7A5tExpN0tAr; nootherTX untilrealcompletion/fullCRC/commit.75NOTbooted.**
NextretailBZIP2a0db4e80/protected75boot/strictnonrootcaps0mlockfilterREADY before
secretINIT, actualHTTPSlogin/session/API work inparallel. Activecontinuation
`work/lightbulb-camera/foundation-20261011T041148Z/CONTINUATION_AUTH_KERNEL75_ACTIVE.md`.
CurrentRAM_ONLYloader/notLinux/network/service; freshd04:19holdrevalidate. No new
guardian/transportphases, allauth/product/otherportsG8 gatesopen.

Readyreceiptadapter exact9-file sharedadoption+parenthash verification complete,
21C+2Python+76/C++/hostARMlinks pass, kernel/scalarCRC unchanged. No targetstage or
newtransportfeature; encodedrenewal pendingfutureprotectedviewing, LABNOAUTH.
Activework staysAUTH: isolated75minimalSECCOMPbuild andactualHTTPSlogin/session/API
service; awaitrealcompletion/reviewthenfreshprotectednativecredentials/TLS readiness.
Actual74normalreceipt-qualified/idle; noUARTjob/codecpools/network/service. All
product/G8otherports gatesopen, freezeguardian/publisherphases remainsinforce.

**Actual native74 receipt expiry/resume PASS:**16fullscalarCRCguardframes/28negative
checks,11179ms hold, exactRETIRED EXPIRY/r0 QUERY→ACK→newgeneration/resume→END/
QUERY/ACK, guardian+hosttimer+own exits0/postidle/UARTwatchdog0/zeroencoderDMA.
Oneboundedcampaign18.422s, evidencecap07-kernel74-native-expiry-once01. Notencoded
renewal/CLOSE/fault/import/15fps/global65s/G2/productproof. **User steering freezes
furthertransport/guardian/publisherexpansion. NextAUTH/product:** isolatedminimal
seccomp/filterkernel75 build andactual Mongoose/MbedTLS HTTPS login/session/API
withreviewedasynchronous600k credentialchild; narrowreadyreceiptadaptermerge only.
Nexthardwareafter75package/source reviewfreshUART/runtime/recovery/RAMboot for
credentialprocess/TLS readiness; no75 bootclaim/weakfallback. Current74idle/nopools/
network/service/transfer, dhold04:19UTC12Octrevalidate. OtherportsG8/allproductfalse.

User steering supersedes nestedtransportplan: freeze furtherlabtool expansion.
Freshactual74/d/recovery/idle/clientguardian hashes passed; **one bounded native
receipt expiry/resume campaign ACTIVE sh_12d175619001rSJyJH5W60ZFoK**, existing
guardian55s+independenthost60s, nootherUARTTX/encoder/network/respawn. Waitreal
completion and independentlycheck16fullCRCframes/exactRETIREDE XPIRY r0/ACK/newgen/
END/ownedexits/idle. Otherwiseconcreteblocker, no anothertransportphase. Thenactual
authentication/protectedviewing andotherQC07Fproduct milestones; G8deferralintact.

Appletfree217b bridge parentreview/independent4-entryreplay PASS, originalsource/
contractpins unchanged; missingtestreferences repairedonlyfreshcopy, failureskept.
Native df30preflight/newFIFOsmoke separatelyPASS. Nextcontinuousproofpublisher
completion/review +freshactualFD/protection binding thenboundedhardwarefreeLIVE
bridgeSMOKE; no targetjob/transfer/encoder/network, no65s/receipt/productclaim.

Actualdf30e nativeUID/tmpfs/anchoredabsence preflight0/nocreate andnewFIFO smoke
dev2inode63/atomic6B ARMRESET/ACK/actor+ownexit0/cleanup/occupiedpreflightrefusal/
stalenocreate PASS.74ISP8IRQidle/watchdog/UART0,noactivejob/transfer/encodernetwork.
Nextfinalizednewbridge exactcontract+continuous500msproofpublisher review then
boundedhardwarefreeLIVE Smoke/hosttimerACK/ownexit, afterwardexactreceiptcampaign.
No fabricatedfreshfacts/directsmoke65sproof; freshd/runtime/04:19hold beforeactions.

Nativepreflight df30e source-reviewed/12hostcases returned, independentreplaypending;
currentexclusiveUART167504BELFupload sh_12cd566cb001bF0WoKmd00IDS7 nootherTXuntil
completion/hash. Thenreplaypass+freshd/runtime/recovery nativeUID/tmpfs/absence
preflight/newFIFOsmoke, finalizednewbridgecontract/live500msproof beforeguardian
receipts. Noappletbypass/65s/productproof; current74scalar8idle/noencodernetwork.

Final bridgeactualcontract source-reviewed butliveHOLD: targetid/stat absent,
newsource nativeUID/tmpfs/absence preflight+bridgeupdate underway, nobypass.
Nativee8c11FIFOsmoke stillvalid. FullreceiptRTSP+credentialprocess hostreplays
active/no targetjob/transfer. Nextrealcompletion exactsource/hash review then
nativepreflight/newFIFOsmoke/livebridge beforeguardianreceipts. Product/65s/G8
gatesopen, freshd/recovery/runtime mandatory; kernel74seccomp0 blocker retained.

NativehardwarefreeFIFOsmoke e8c11/privateendpoint/6B ARM+RESETwrites/consumerACK/
ownexit0/cleanup/stalewriternocreate PASS; firstprompt-prefix rejection/timeout
preserved. No job/transfer/encoder/network/service now, actual74scalar8idlebaseline.
Nextreviewedactualcontractbridge liveSmoke/independenthostmonitorbinding then
guardedexactreceiptexpiry/ACK/resume beforecodecpools. FreshsoleUARTd/runtime/
04:19recovery revalidate; directsmoke isnot65s/retirement/productproof.

Transportguardian e8c11 source/protocol parentreview+independentexactARMreplay passes.
CurrentexclusiveUART ELFstaging sh_12cb178dd001dZ94ciZvQRBeLb; waitrealcompletion/
hash beforeTX. Thenfreshd/runtimeroot/endpoint/protection andhardwarefreeFIFOsmoke/
atomic helper/ACK/ownexit/cleanup. Completedguardian contractbridgefinalization
pending, no receiptguard/65sproof. Actual74scalar8idle/noencodernetwork remains;
allproduct/otherportsG8 gatesopen, dhold04:19UTC12Octfreshvalidatebeforeactions.

Restored74 zeroDMA/sensor139/retainedWB-UV/scalar8fullCRCguards/idlewatchdogUART0
requalified; fiveELFstagingcomplete,nojob/transfer/network/encoder. Oldc normalfinal
raw/events hashesverified unchanged; newsole receive-only **foundation-20261011d**
freshholdto2026-10-12T04:19:35Z/actualruntime revalidated. OldcPID/21:58armnotcurrent.
Next reviewedtransportguardian/bridge stage+hardwarefreeFIFOsmoke/inode/no-create/
ACK+hostmonitor binding thenreceiptexpiry/resume beforecodecpools. Allproduct/G8
otherports gatesopen,actualownership/runtime/recovery alwaysfreshbeforeactions.

Same74restoration completed37198frames/0retries/native+UBootCRC200b4491 andprotected
Linux/PID1/CMA16/manualISP/builtincodec boot observed. Actualidle/watchdog/UART0,
no new jobs/network. **CurrentexclusiveUART fiveELFrestaging bundle
sh_12c975b3a001QRJR8wpx5L05L1, waitrealcompletion/allhashes beforeTX.** ThenzeroDMA/
sensor139/retainedWB-UV/scalar baseline. Livebridge/transport smoke/inode/no-create/
ACK monitorqualification stillpending; no longfixture/protectionclaim yet.
Freshc ends21:58:46Z revalidate/newowner ifneeded; allproduct/otherportsG8 gatesopen.

Bridgehost25/13 returnedbutnativeactivationHOLD: guardian needs no-hardwareFIFO
smoke, activeinode publication and atomicno-create writer. Freshguardtransport/
bridgefix work underway; parent review/actualsmoke/ACK/binding mandatory,no65sclaim.
Same74restorationuploadsh_12c812395001UD04gjFW06ETfc remainsexclusive untilreal
completion beforeanyconsoleTX. Existingprotected74restore/CRC/restage remainsnext
hardwarestep; allproduct/othercameraG8 gatesopen.

Actual ownno-DMAwatchdog5→verifiedPIDFDcharalias1 resetroute producedfullreset and
nextheldU-Boot; RXlatency1.640349145s notfull65sguardproof. Freshrecovery/SDloader/
same74hash checked. **CurrentexclusiveUART same74 restorationupload
sh_12c812395001UD04gjFW06ETfc; nootherTX untilrealcompletion/fullCRC/commit.**
74notcurrentlyLinux,volatiletools gone. NextindependentCRC/protected74boot/zeroDMA/
baseline,9e75client+223badguardianrestage/realprivatebridge thenreceiptexpiry/resume.
Allproduct/othercameraG8 gatesopen; no reset-request-only or timing-math proof.

Fixedguardian223bad selectedMbedhash/boundedabort/watchdogalias-only qualifier
returned/source reviewed; independentreplaypending. CurrentexclusiveUARTupload
sh_12c797784001DzesgdXDKNysk1; waitrealcompletion/hash beforeTX. Thenfreshactual
runtime/protection/noDMAjob watchdogalias5→1 reset-only qualification, captured
nextU-Boot/elapsed, protected74restoration/staging/livebridge beforeexpiryfixture.
No request-marker/hostcap65s/nativeproof substitution; allproduct gatesremainopen.

Actual74 guardian hardwarefree ownpipecapability allpidfd/getfd/stopwait/cont/exit/
proc1 exit0 passed, ISP8IRQidle/watchdoginactive/UART0 unchanged. No targetjob or
UARTtransfer/network/service. Nextafterreviewedfixedguardiancompletion: stage/hash
explicit no-DMA ownedwatchdogdup nominal1s reset-only route, actualU-Boot hold/
elapsed underfreshprotection; protected74restoration+liveprivatebridge qualification
thenreceiptexpiry/resume. Oldguardian199def --guard prohibited; no65sproof from
cap/math/source. Otherproduct/portsG8 gatesopen; see topCONTINUATION_KERNEL74_ACTIVE.

CurrentexclusiveUART guardian-capability-only ELFuploadsh_12c49140d001G3DaW35QCbXjvL,
waitrealcompletion/hash thenhardware-freePIDFD/getfd/stop/continue/wait/proc probe.
`--guard` originalbuildHOLD for selectedlibrary hash/boundedpre-exec wait fixes;
liveFIFO/hosttimer/watchdogdupresetroute stillrequireactualqualification. Freshc
protection/runtime revalidate; no ≤65s/receipt claim. Actual74scalar8/idle/noencoder
pools/network/service baseline retained. See topCONTINUATION_KERNEL74_ACTIVE.md.

Actual74 scalar8 full1728000-byteCRCguards/seq/status/sensoron/ENDexit0 andidle/
watchdog/UART0 pass withretainedWB/UV restored. FourELF stagingcomplete, noactive
targetjob/transfer, noWLAN/RTSP/viewer/codecpools. Next explicitreceiptexpiry/resume
stillrequires suppliedbounded outerrecoverymonitor: copiedguardian beingprepared,
nativecapabilities/resetroute parentqualification needed beforefixture. Thenreceipt/
copiedqueue qualification beforecodecpools. CopiedreceiptRTSP76case/fullsuite and
credentialprocessfixes returnedpendingreview;74seccomp0 securityblocker unchanged.
Allproduct/other-camera G8 gatesopen; freshUART/recovery/runtime beforetargetactions.

Actual74 codec-init17-rowpurequery zeroDMAattempts/live/peak/flags0 andidlewatchdog/
UART0 pass, notzeroheap/electricalproof. **CurrentexclusiveUART fourELFstaging bundle
sh_12bf4d813001Iw0uqzQcqFDOkR; waitrealcompletion/allhashes beforeTX.** NextretainedWB/
UV comparisoncontrols, unchangedscalar8 CRCguards, reviewedfixedreceiptexpiry/
resume beforeanycodecpools. Freshc recovery/runtime/5s watchdog+boundedparentmonitor
required. Allproduct/othercamera gates stillopen; actual74 boot supersedes history.

Actual74 update: image37198checkedframes/0retries/fullBZIP2+independentUBootCRC
200b4491 thenprotectedLinux/PID1/CMA16/manualISP/builtincodec boot observed. Actual
idle/guards0/watchdoginactive/CmaFree9216; sensorDA4D/exact139/nativegeometry pass.
Capture/receipts/zero-codec-DMA query stillpending. **CurrentexclusiveUART small
stage-inspectELFupload sh_12bebf5be001KRaL9KZefpQ34n; waitrealcompletion beforeTX.**
Nextpurecodecquery thenfixedclient/scalar staging/native receipt+capture before
codecpools. NoWLAN/RTSP/viewer/encode yet. See topCONTINUATION_KERNEL74_ACTIVE.md;
freshc recovery/runtime revalidate. Credentialprocessclock/signalfix/SECCOMP=n and
copiedRTSPvalidation fullsuite remainopen; allproduct/othercameraG8 gatesfalse.

Actual next phase: parent-reviewed74 package/source/bounds/CRC, fresh73 idle/
rollback/UART/recovery thenwhole-systemreset heldU-Boot; existingSDloaderCRC/read
passed. **Checked74 RAMuploadACTIVE sh_12bc8add5001fdymb6D8oMw1WE; no other UARTTX
until realfulltransfer/commit completion.74 NOTbooted.** New continuation:
`work/lightbulb-camera/foundation-20261011T041148Z/CONTINUATION_KERNEL74_ACTIVE.md`.
Nextaftercompletion nativefullCRC200b4491/protectedboot/scalar+receiptqualification
beforecodecpools. Fixedclockfixture replay passes; finalCLOSEproof stillunobservable.
CopiedreceiptRTSP/credentialprocess returnedpendingparentreview, noauth/nativeclaim.
OtherportsG8/allproductgates unchanged/open. NoSPI/saveenv/SDdata writes.

Latest receipt review: original ARMlinked native fixture is execution-HOLD for
unchecked clock/deadline/regression handling; complete copied fix underway. Final
close proof also notobservableviaexistingreceiptABI (dup delaysrelease; finalclose
consumesreceipt), so do notclaimCLOSE electricalproof. Copied RTSP exactreceipt
expiry/ACK/newgeneration adapter underway; noEPERMignore/timeoutrelaxation. Native
kernel74/provider audit build andcredentialprocess continueoffline. Nexttarget
requires reviewed74package/exactstation AND correctedfixture plusfreshactual UART/
recovery/runtime. No targetaction/newboot orproductgate closure from these builds.

Later source completion: reviewed RTSP prerequisite narrowly integrated19 files,
fresh21 C+2 Python/ARMlink/C++/sanitizer/72 syntheticframe tests pass; parent exact
pins verify kernel/scalarCRC untouched. Strict hostpreview Session migration5 mocks
pass. No newtarget operation/staging/authentication. Isolated74/provider audit,
receipt fixture andcredentialprocess continueoffline; next target stillrequires
reviewedcomplete74package/exactstation/fixture andfreshUART/recovery/runtime. Other
camera ports deferredG8/allproductgatesopen; see latestcontinuation/evidenceledger.

Later2026-10-11 offline: reviewed complete resource-fix01 and scoped built-in
codec-retirement01 now queued for isolated74 source/provider/config audit/build,
with all12 STATIC4 calls retained. No74 image staged/booted or further target
operation after14:31Z. Receipt-native-fixture preparation independent/offline.
Corrected strict parser full copied host/ARM linked tests pass runParent04;
narrow shared integration authorized, still NOAUTH. Credential admission fix
342+587/real600k/ARM-link passes; isolatedprocess work underway, no productauth.
**Next target only after reviewed74 package/exactstation/receiptfixture complete:
freshsoleUART/recovery/runtime/rollback, protectedreset/checkedRAMboot, scalar/
receipt qualification beforecodec pools.** See latest continuation; otherportsG8
and allproduct gates unchanged/open. Preserve all parentrun/old defects/failures.

Latest2026-10-11T14:35Z: actual73 held/shadow/queue48/11105ms expiry/fresh48/close-
held/querypurity pass. Exactcodec25 loaded; twofinite12-AU source-correlated clean
distinct decoded720p runs/serviceexit0/idle pass, superseding supervisor failures
preserved. No RTSP/viewer/capture active; codec8454144B remains/CmaFree1712kB.
No four-slot+codec or unsafeunload. Freshc UART owner/recovery21:58:46Z revalidate.
Independentresource03/parser reviews HOLD with concrete fixes underway offline;
credential-worker prototype independently underway. **Next on real resource-fix
completion: review complete candidate and narrowSTATIC4rebase, freshimage/modules
build thenprotectednative receipt expiry/close/resume qualification.** NoEPERM
ignore/TTLrelaxation/productgate acceptance; otherports stilldeferredG8.

Latest2026-10-11T14:17Z: actual73 heldslot0/2 scalar CRC guards andCPU-shadow EIO
negatives passed; copiedqueue48 (three16-frame sessions/48unique tokens) andquery
fault/observerpurity passed. Notstandardcapture/overlap/15fps/G2. Oldb ownerwas
interrupted; byteexactRX/eventcopiespreserved/originalsunchanged. Freshsoleowner
foundation-20261011c/liveholdrecoveryuntil21:58:46Z revalidated. **Currentbounded
held-expiryexperiment mustcompletebeforefurtherTX; thenfreshqueue48recovery and
close-held, beforecodec25.** See latest top of CONTINUATION_KERNEL73_ACTIVE.md;
resource03candidate06/strictparsercopies need parentreviewbeforeadoption.

Latest2026-10-11T12:16Z: actual73network/scalar24fullframeCRCguardoracle andone
hostrawSHA/CRCmatchpass; repeatpre49.705/post49.496ms versus72_55.798/56.232ms,
modest10–12%not15fps. Oldrecorderendednormally12:11/hashverified;expiredarmunused.
Freshreceive-onlyownerfoundation-20261011b/recoveryuntil20:13:22Z andactual73idle/
network/watchdog/UARTverified. **Next: boundedscalarheld/queue/shadownegative73
fixturesbeforecodec25allocation; reviewresource03/parsercopiesbeforeadoption.**
Allproductgatesopen/otherportsdeferredG8; do notmarksubsystemcompletefromhosttests.

Supersedingactual2026-10-11T09:06Z:73checked37103frames/0retries/fullnativeCRC/
independentU-BootCRC/boot/PID1/CMA16/sensor139pairs/firstguardedSTATIC4capture/
RAMradioRUN passed. Station73moduletransferactive sh_12a3789e10018qe3oyhyp4ySbq,
noUARTconsoleTXuntilcompletion. Networking/query/fullscalaroracleA_B stillpending;
no nativeCRC/performanceacceptancefromboot. One next: exactnetworkrestorationthen
unchangedscalar8frameoracle/allguards/querytimings. Lifetime02actualkerneladoption
blocked/manualresourceproviderintegrationoffline; not genericmodelcompletion.

Latest2026-10-11T08:47Z: reviewedSTATIC4kernel-onlyoverlayfrozen/built73; exact
station73/codec25 built,samesafeconfig/symvers/ranges/scalaruseroracles preserved.
Fresh72idle/logs/privatefile/recovery/preflight thenfullresetheldU-Boot/read-only
SDloaderCRCdd0572e1. **Checked73RAMuploadactive sh_12a24d2e0001RE1siqKgp4feRF;
NO UARTconsoleTX until realtransfercompletes.73notbooted.** Newcontinuation
`work/lightbulb-camera/foundation-20261011T041148Z/CONTINUATION_KERNEL73_ACTIVE.md`.
One nextaftercompletion: fullnativeCRC5deb2eec/protected73boot/unchangedscalar
8frameCRCguardoracle/querypurity/stageA_B; no speed/productclaimfrombuild. Per-open
deferredlifetimeandstrictparserintegrationprototypes continueoffline; noEPERM/TTL
relaxations orpartialpipelineacceptance. Othercameraports stilldeferredG8.

Latestdirect2026-10-11T08:11Z supersedesuploadhistory: actual72/station72/network/
codec24 queryprotocol/fault/observerpurity andgatedCRC/stagespassed. a24short20+
reconnect10received/sourcecorrelated/decodedclean, butnormal10sTTL50requestfailed
28complete/10.734s/TCP EOF; checkedretirementrefusal/lifecycleFAILED/nonzeroexit,
PID116absent/noRTSP/viewer. CapturequeueIRQidle/watchdoginactive/UART0 observed,
notformalreceipt. **Next: reviewstatic4CRCsourceintegration + retainedper-open/
deferredresource lifetimeprototype beforeadoption; thenprotectednewimage native
A/Bwithallchecksintact.** No blinda24respawn/EPERMignore/expiryrelaxation.
SourceARC/buffermemory/performance/security/storage/peripheral/allQC gatesopen.

Supersedingexecution2026-10-11T05:58Z: checked70transfer/nativeCRC andactual70
boot/PID1/CMA/sensor139pairs/guardedcapture/radioRAMRUN passed. Station71WPA2CCMP/
DHCP/MTU384 restored. ActualmanualWB idle/fault/IRQ/heldqueueCRC/EBUSY/restore
passed afterfixture02 fixedomittedcaller-sensorstart (failed01 preserved).
Small380/256/380 gainstep improvedblue-poorwarm-litreferences withoutclipping;
retained413/256/310+UV64 restored withfresh8frames. G1 stillopen. NoRTSP/viewer
currently; codec22built/notloaded. **Nexttarget: reviewed/fixed boundedpublic
entropyprobe5s, then unchangedKDFprobe35s, actualresources/readiness, notsecurity
acceptance.** Parentblockedunrunwrapper on independentPIDownershipcounterexamples;
fix/regressionsinprogress. Independenttelemetrykernel71review, produceradapter,
QPconstructorREA andhostfixtureintegration continue. Currentcontinuation'snew
topsection supersedes activeupload history. Othercamera ports staydeferred.

Newexecution2026-10-11T06:34Z: fixedreviewedpublicprobe wrapperactualtarget passed;
late-idleOSRNG32bytes/onecall/40.5us andpublicPBKDF2 costs10k0.413s/50k2.068s/
100k4.121s observedunder35s bound. Notcoldboot/PSA/TLS/workfactoracceptance.
600k~25s estimate needsactualstrongcost andmedialoadqualification; do notweaken
passwordcosttomeetproposedlatency. Strongpublicfixture beingprepared. a23build/
16hostcases passed butindependentreview blockedstaging onframe-tokennamespace
collision andlegacyuncertaintyforgotten; fixes/newa24required. Telemetryfix01
reviewed, controlled71overlayrebasedelegated; no71runtimeclaim. SEC-01policyhost
passed; actualauth andrevocationteardown remainopen. Publicsettings storage
primitive delegatedhost-only, noSD/SPIpermissionassumptions. **Nexttargetafter
fixreviews: hash-gatedstrongcost ornewa24 finitehealthy/retirement qualification**;
serializeandpreservefailures, thenprotectedtelemetry71 boot. Allproductgatesopen.

Superseding2026-10-11T07:14Z: actual600kpublicPBKDF2 completed24.804768586s,
zeroization/CPU/RSS/childretirement validated; notselectedlogin/workfactorpolicy.
Kernel71compilefailed Linuxcurrentmacrocollision; privatefieldrename+codecfix
compiledfresh72/codec24, correcteda24and17hostcasespass. **Protectedchecked72
RAMuploadnowactive; NO UARTconsoleTX until realtransfercompletes.** Currenttarget
RAM_ONLYloader, not72booted; network/viewer/RTSPoffline. Newcontinuation
`work/lightbulb-camera/foundation-20261011T041148Z/CONTINUATION_KERNEL72_ACTIVE.md`.
Nextaftercompletion: fullnativeCRC63a88874/protected72boot/querypurity/gatedCRC/
stageownership measurements, thenfinitea24source/retirementtest. Formalper-FD
retirementreceipt prototype offline; do notusebareEPERM/ownerNULLasproof.
Publicsettingsstorehost855checks registered, full18-casehostsuite09running.
Otherports/allproductacceptancegates remainunchanged/open.

Execution update2026-10-11: BASE-01 current-state/recovery validation passed and
BASE-04 repository pointer is installed. IMG-01 REA exposure packing/clamp and
gain divisor are corroborated; physical units/Bayer/CCM remainopen. IMG-02 has
verified current mixed-light raw captures; IMG-03 raw-code reduction/readback/
restore and fault fixtures pass, not full AE/physical-unit acceptance. Camera
restored toexposure744, gain/VTS/flip/lights unchanged. User authorized parallel
scopes: manualWB (IMG-04), commonproducer/control lifecycle (CORE-04/05), AP
feasibility (NET-06). Parent owns all hardware and integration. **Next: review
manualWB output/tests, freeze nextRAMkernel/exactmodules, qualify actual controls**;
then integrate reviewed commonprimitive/audit. Do not mark G1 complete.
New2026-10-11T05:17Z: reviewed kernel70 and exactstation71/codec22 built; audited
watchdogreset heldU-Boot, freshlyread existingSDloader/nativeCRC passed. **Checked
kernel70 RAMuploadactive; noUARTconsoleTX until realuploadcompletion.** Kernel70
notbooted. Nextaftercompletion: nativefullimageCRC, protectedboot, actualidle/
IRQ/queue manualWBfixtures. New continuation
`work/lightbulb-camera/foundation-20261011T041148Z/CONTINUATION_KERNEL70_ACTIVE.md`.
ProducerSTOPchurn/staleappliedevidence bugs reproduced and fixed withpositive
hostregressions; actualadapterintegrationpending. BASE-07 realdecodejob passed
without automaticacceptance. APprototype/capturememory/security/ratecontrol and
REAilluminationcorroboration continueoffline; otherports stilldeferred.
Current evidence/coordination: workspace `docs/FOUNDATION_PLAN_PROGRESS_20261011.md`.
The original initial-task paragraph below is preserved entry history.

**BASE-01 → IMG-01/02.** Revalidate sole UART/recovery ownership and actual
QC07F runtime, preserve the kernel69/station70/codec21/a22 baseline, then resolve
source-backed exposure/gain units and collect an unclipped neutral-reference
capture. Current ceiling/wall highlights are clipped, so do not calculate a
final white balance from them. Keep gamma1 and both requested preview flips.

Independent offline work after BASE-01: CORE-01/02 contract audit and CAP-01
memory/lifetime design. Independent tasks can be interleaved, but hardware
experiments sharing camera/UART/DMA ownership must be serialized.

## Dependency-ordered phases and subsystem files

| Phase | Work and task prefixes | Dependency / exit gate |
|---|---|---|
| 0 | [01 Baseline, evidence and execution](01-BASELINE-AND-EXECUTION.md), BASE | Reliable recovery and truthful current-state ledger: G0 |
| 1 | [02 Shared architecture/control broker](02-SHARED-ARCHITECTURE.md), CORE; [03 Platform/build/recovery](03-PLATFORM-BUILD-RECOVERY.md), PLAT | Small common runtime and stable driver contracts, on QC07F |
| 2 | [04 Imaging/exposure/colour/orientation](04-IMAGING.md), IMG | Usable, measured imaging: G1; continue tuning during later capture changes |
| 3 | [05 Capture/memory/V4L2/DMA-BUF](05-CAPTURE-AND-MEMORY.md), CAP; [06 Codec/rate/performance](06-ENCODER-AND-RATE-CONTROL.md), ENC; [07 USB/radio transport](07-USB-AND-RADIO.md), NET | Native owned-buffer hardware stream within measured resource budget: G2 |
| 4 | [08 Streaming/fan-out/protocols](08-STREAMING.md), STREAM; [09 Security/config core](09-SECURITY-AND-CONFIGURATION.md), SEC; [10 On-device Web UI/browser media](10-WEB-UI-AND-BROWSER-MEDIA.md), WEB | Protected RTSP and browser streaming acceptance: G3 |
| 5 | [11 PTZ/lights/sensors/reset input](11-PTZ-AND-PERIPHERALS.md), CTRL; [12 Audio](12-AUDIO.md), AUDIO | Qualified peripheral owners and real microphone capture: G4 |
| 6 | [13 SD/recording/motion/retention/playback](13-STORAGE-RECORDING-PLAYBACK.md), STORE | Recording and playback preserve existing footage: G5 |
| 7 | [14 Network transactions/setup AP/reset](14-NETWORK-SETUP-AND-RESET.md), SETUP; finish SEC/WEB configuration integration | Recoverable first-use/network/reset product: G6 |
| 8 | [15 DNS/discovery/ONVIF/NVR](15-DISCOVERY-AND-NVR.md), NVR | Real supported Frigate/Blue Iris integration: G7 |
| 9 | [16 QC07F foundation qualification/release](16-FOUNDATION-QUALIFICATION.md), QUAL | Shared software foundation accepted on QC07F: **G8** |
| 10 — deferred | [17 T34-4G port](17-DEFERRED-T34-4G.md), T34 | Start only after G8; both fitted sensors and shared product qualified: G9 |
| 11 — deferred | [18 Fulhan port and final cross-model gate](18-DEFERRED-FULHAN-AND-FINAL-GATE.md), FH | Start after G8/G9; both sensors and all-model regression: G10 |

The dependencies are not a requirement to finish every detail of a file before
opening the next. CORE, SEC's authentication/config schemas and WEB's transport
design must exist before protected streaming acceptance. Read-only AP capability
research under NET is an early feasibility task; full setup/AP/reset integration
stays in phase7. Bounded light routing needed for day/night evidence may be done
under CTRL before G1 closes; full peripheral product delivery stays after G3.

## Gate definitions

- **G0:** repeatable baseline/recovery/build/evidence; no active transfer conflict.
- **G1:** corrected colour/exposure at qualified lighting conditions, explicit
  orientation and day/night capability, no claimed lux/filter hardware by inference.
- **G2:** standard capture/encoder ownership path, justified DMA/user budget,
  sustained useful native H.264 performance and measured transport behavior.
  Target at least15fps native720p on QC07F; qualify30fps only if supported. A
  measured hardware ceiling requires explicit scope review, not redefining2.5fps
  as completion. No unqualified resolution drop or software entropy fallback.
- **G3:** authenticated RTSP and on-device browser media with isolated slow-peer
  handling, valid timestamps, bounded reconnects, and multi-client acceptance.
- **G4:** actual safe PTZ displacement/stop, independently controlled fitted
  lights, truthful light data, reset input and qualified microphone path.
- **G5:** continuous/motion recording, masks, retention, existing-footage import
  and authorized browser playback under real SD fault scenarios.
- **G6:** first setup, AP/captive steering, configuration persistence,
  transactional network switching and reset with settings-only scope.
- **G7:** truthful DNS/service discovery and tested NVR playback/control.
- **G8:** full QC07F combined workload, restart/power-loss/security/resource
  qualification; reproducible deliverable and recovery. No required subsystem
  replaced with a stub or disabled to pass. Host fixture proves dual topology.
- **G9:** T34-4G runs the same shared services, both real sensors active and all
  fitted/required features qualified within truthful concurrency limits.
- **G10:** Fulhan does likewise; re-run common regressions on all three after
  port-driven changes. Only then consider all-required-modules true.

Persistent SPI installation is a separate, exact-image/address/recovery action,
not an implicit consequence of any gate. RAM qualification can satisfy a software
foundation gate without silently installing firmware.

## Tracking conventions

Every task starts unchecked here. Existing proof is summarized in each file,
not discarded or counted twice. Use these states during execution:
`planned → implementing → fixture-passed → target-passed → accepted`, or
`blocked(dependency, evidence, next experiment)`. A task spanning multiple
platforms is not accepted globally from QC07F evidence.

No percentage or calendar estimate is assigned to unmeasured driver work.
Progress is demonstrated by passing gates, removed blockers and retained
artifacts. Test thresholds in this plan are **proposed release criteria**,
not descriptions of tests already run; freeze numerical targets before the
corresponding qualification campaign.

## Requirement cross-reference

| Requirement | Primary plans |
|---|---|
| Three camera systems, both sensors on dual targets, common services | 02, 05, 06, 16–18 |
| Correct live colour, exposure, orientation, daylight/night | 04, 11 |
| Authenticated RTSP/browser/live PTZ; separate passwords | 08–11 |
| White and IR independent controls, real ambient data, microphone | 11–12 |
| IP/admin/WLAN settings and disruptive-scan result retrieval | 09–10, 14 |
| Settings reset, setup AP and captive portal | 09, 11, 14 |
| SD playback and preserving original footage | 13 |
| Continuous/motion recording, ignore areas, oldest/date retention | 13 |
| DNS advertising/registration, ONVIF, Frigate and Blue Iris | 15 |
| Reproducible upstream-based builds, evidence, rollback | 01, 03, 16 |
| T34 LTE-to-USB-Wi-Fi feasibility side task | 17, after foundation |
