# Remaining QC07F foundation work — independent lanes and review gates

## Active stock-reference campaign2026-10-13

User authorized tests after review. Parent controls actual stock3.4.35 root rescue,
original providers, authorizedguestMTU1500; card softwareRO/unmounted, configRO,
no normalapp/selfcheck/cloud/recording. See ACTUAL_RESULTS under qualification-plans/
stock-reference-20261013. Source work: one bounded12-function light-policy REA
follow-up, one bounded stockfeed-entry audit. FixedGET helper completed45 parent
HOST mocks plusfour native GETs; no MOVE/RESET/STREAMON. NovisibleLED userreport
preserved; sensor off/on/off exposure-aware comparison pendingfeed, no more blind
GPIO/PWM. Historical13a hold/U-Boot below is not currentLinuxstate/protection proof.

## Explicit resume2026-10-13

Two read-only reference reviews completed: stock motor/lighting command+GPIO tracing
contracts and factory video cadence; parent audited startup side effects and factory
MTU/transport evidence. No implementation/build/stock boot/device commands. User
refinement and campaign: [STOCK_REFERENCE_CAMPAIGN_20261013.md](STOCK_REFERENCE_CAMPAIGN_20261013.md).
Findings: board5directGPIO/retailABI/publicdemo mismatch; delayedSTOP and no confirmed
tracing mechanism/high-levelCLI; sensor30softwarelabel and adjusted15/13 VTS; actual
factoryprofiles/FPS/MTU remain unknown. Next is exact safe stock entry/command/logging
admission, not output tests based on compiled tables.

User resumed and powered on: actual held U-Boot13:34:23UTC; no Linux handoff.
13a catch consumed, fresh protection needed before next target operation. User
requested review/strategy first. Two read-only reviews completed, incorporated in
[completion strategy](REVIEW_AND_COMPLETION_STRATEGY_20261013.md); no builds/device
commands or implementation queue restart during review. Future WIP: one parent
hardware campaign and at most two disjoint source lanes resolving actual integration
dependencies. Previous pause/results remain history; media/RTSP final checkpoints
still need code review/native qualification, all product gates remain open.

## Latest steering: PAUSED

**All lanes stopped; final closure recorded2026-10-13.** Media-v6 final SOURCE/HOST
seal returned, migration/placement forecast stillHOLD; parent review/rerun pending
only after explicit user resume. No current unit, source build/test, target job,
UART owner/socket or recovery arm. Final results/partial tails in PAUSED_STATUS.md.

RTSP isolated-server lane stopped at sealed partial checkpoint. Integration05 is
tested HOST only; current three fixture edits unbuilt/untested, parent review pending.
No queue advancement or further tests. Media-v6 final seal is now recorded above.

User requested finish active checkpoints, status/documentation and **halt work**.
No new units or automatic queue advancement. See [PAUSED_STATUS.md](PAUSED_STATUS.md).
UART i finalized/hash-verified/closed; no serial owner/socket/recovery arm remains.
UI03 finite public stage cancelled withzero transfers, no native launch. Target last
observed same75 RAMLinux/WLAN384/ISPidle/UART0/Wdoginactive. Guard-only native increment
passed; parent121 models, motion v2/profile-transition v2 independent HOST reviews
completed. Media-v6/isolated-server sealed current work and stopped;
final source receipts are unreviewed/nativeHOLD. All dated assignments below are
historical and **must not restart** unless the user explicitly resumes work.

User steering2026-10-12: split remaining tasks and delegate isolated work, with
parent review. This supplements [the canonical index](00-INDEX.md), not a new
roadmap. QC07F first; T34-4G/Fulhan ports remain deferred throughG8. All product
gates remain open. Source/host/browser fixtures are not native/product acceptance.

## Current baseline

- Native75 HTTPS authentication and owned SIGTERM lifecycle increment passed;
  warm private qualification only, not durable first-use/cold entropy/trust.
- Shared C fMP4 init/profile/timing/span and CPU-handoff/admission models pass
 26 C +2 Python +76 receipt tests. Retained720p H.264 decodes in desktop MSE;
  latest renderer reported a drop, so no zero-drop/live/FPS claim.
- On-camera media still501; RTSP is independent NOAUTH LAB. Native handoff,
  combined memory/load, normal FPS, storage/setup/peripherals/NVR are unfinished.
- Last actual runtime05:09UTC is historical.13:01 metadata found adapter/socket
  but no serial owner, expired e arm and absent final_e manifest. Parent must
  preserve/investigate ownership and establish fresh runtime/recovery before TX.
- 720p15fps is an engineering target, **not an established exact-QC07F guarantee**.
  Generic AK3918B152 documentation and V200 reference defaults do not replace
  exact retail/stock measurements. No silent performance-target change.

## Work decomposition

| Lane | Task IDs / deliverable | Independent now | Integration / real completion |
|---|---|---|---|
| 1. Protected native video | CORE/CAP/ENC, WEB-02/03/05/10, STREAM-02/03/08/09 | Shared bounded mux, source/CPU-lease interface and source-build audit | Parent-only native handoff/retirement/memory campaign, then real authorized TLS/MSE frames, expiry/slow-client cleanup and combined load |
| 2. Independent RTSP authentication | SEC-05/12, STREAM-04/07 | Bounded Digest crypto/nonce/replay core and negative host vectors | Parser/backend/producer admission review, exact native build, independent credentials, actual NVR interoperability; never label TCP as encrypted |
| 3. Imaging and performance | IMG/CAP/ENC, QUAL-02 | Exact-part/source/stock evidence audit, timing/memory experiment plan | Sensor clocks/colour/exposure/AWB/IR/focus facts, continuous capture/standard import and measured native FPS without dropped integrity checks |
| 4. Recording and motion/playback | STORE task group, WEB-09, STREAM-08 | Owned catalogue/retention plans/playback pins, then bounded masks/catalogue format | Exact owned SD/fs/capacity and atomic persistence, actual continuous/motion files, ignored areas, authorized range/playback and retention under recording load |
| 5. Network/setup/reset/config | SEC-07–10, SETUP task group, WEB-07/08 | Receipt-first transaction/rollback state, settings validation/schema fixtures | Real station reconnect/AP/clients/unique keys/captive portal, durable commit/recovery and deliberate reset preserving firmware/footage |
| 6. Peripherals/audio | CTRL/AUDIO task groups, WEB-06 | Source-supported interface/routing matrix and safe qualification sequences | Parent hardware and any missing physical measurements; actual displacement/STOP/light outputs/mic capture. Unknown speaker/IR-cut/ambient/absolute position stay unsupported |
| 7. Discovery/ONVIF/NVR | NVR task group, SEC/STREAM dependencies | Truthful capability/auth/service mapping and client test contracts | Owned Frigate/Blue Iris endpoints, real protected streams/control, discovery limits; no fake SOAP success/capabilities or unrelated-network scans |
| 8. Durable security/boot/release | SEC-02/03/06/08/11, PLAT, QUAL | Licensed reproducible recipes, secret-store/trust/rotation and fault policy design | Cold entropy/trust, durable credentials/config, exact install ranges/boot/recovery, hard-fault behavior and privacy/license review |
| 9. Product qualification | QUAL task group, G0–G8 | Coverage ledger/scalar collectors for actual requirements | Combined browser+NVR+recorder with stalled peer, soak/fault/reset/SD/network campaigns and all required modules; no acceptance from fixture counts |
| 10. Later hardware ports | T34/FUL tasks/G9/G10 | Existing acquisition/source inventory retained | **Not started until QC07F G8**; all fitted physical sensors and measured retail routing/resource limits |

## Delegations launched

### Queue round02 — continuing on real completion events

Latest queue/review state (supersedes dated running rows below):
- Motion source returned but parent HOLD: final valid callback may revoke policy
  after last policy proof; final post-callback LIVE/EVENTS authorization and
  acquire timing/loan tracking correction requested.32×18/8×16/4KiB unchanged;
  no native statistics/tuning/recording/SD acceptance or real-frame access.
- Media v6 completion lacked handoff; partial raw-reuse/forecast source unsealed,
  final report/closure requested. Native HOLD, no allocation/source operation from
  candidate build files. Existing reserve/CRC/quality/deadline ceilings preserved.
- RTSP lane advances isolated-child ↔ derived-server integration HOST only; reviewed
  original v3/child v2/qualification pack untouched. Native strict2M/readiness and
  source handoff remain separate dependencies, runtimeNOAUTH/501 unchanged.
- Profile-transition real pp/ntx/policy integration returned20/1151 each HOST only.
  Parent source HOLD: failed active metadata query must not unlock/invalidate in
  pt_close. Focused retained-context correction requested; native store/key/radio
  observer still blocked. No fixture private-content hashing or shared-core edits.
- Guard-only7s payload/source pack pending; no native UI3/producer/newRTSP runs.
  Latest post-restoration WLAN memory/zone readback retained for future candidate
  admission, no prefault/codec/sensor effect and no guaranteed future placement.
- Actual75 RAM/WLAN authorizedVertexHotspot WPA2_CCMP DHCP192.168.1.73/MTU384 restored,
  Wdoginactive/UART0/ISPidle/guards0 observed. No new candidate/service running;
  current i/02:28Oct13 proof revalidate, historicalPID/end not authority.
- Parent wrong-mode radio2048 error preserved; original measured75 restoration
  uses256. Source47 exact guardedUSBunbind/rebind/power42 coldreset revalidated
  with no modules/claims; explicit original256CRC_RUN_APPff passed before paired75
  station normal(no_bootdl1)/privateguest/DHCP. No automatic fallback/newGPIO route,
  native2048 qualification/SPI/SDdata/saveenv/AP or key/default changes.
- Audio-media v2 independent410499/410495 +C++/ARM/codecFFmpeg oracles reviewed;
  preborrow authorization/sourceclock corrected. Lane advances motion masks/low-cost
  statistics HOST candidate, not native recordings/mic/client acceptance.
- Guard source/race review independent7 realHOST cases+7 source tests PASS: hazard
  confirmed, native02 cause unproven. Existing5s/4s/45/2M/180/140 unchanged. Tiny
  guard-only native qualification source pack pending beforeUI3/producer; no new
  guardian/transport/service or claim of bounded native scheduling from HOST.
- Source-same station75 module RAM upload active sh_13258a29b001pZtMlKMekrmPM1,
  paired public pins verified, no autoload. UART exclusive until realhash/end;
  WLAN/guest/services not yet restored, no AP/newcredentials/nativeUIretry.
  Async crng init observed on restored75, no cold entropy/trust acceptance.
- SAME75 framed restoration complete37237/zero retries/full image+independent
  staging CRC/QCLcommit, protected directRAMboot observed. Current i75 Linux
  shell/publicnote+config matches/ISPidle/guards0/UART0/Wdoginactive. WLAN/private
  keys/services notrestored/inherited; failureguardreview stillnextbeforeretry.
  Original8s partialbootobserver retained, lateractual monitor/prompt evidence
  distinguishes direct boot. QualifiedBOOT-only60s protection not native5s-budget
  increase; noSPI/SDdata/saveenv writes. Revalidateactuali ownership/arm/runtime.
- Finite RTSP first-entry pack independent220/220+ABI6/C++/ARM link reviewed,
  native/PRIVATE generation never run. Actual entry waits restored75/fresh owner/
  recovery and source guard review, no concurrentUART target operations. First
  native claim limited READY15/root-origin/2M/provisioning/wipe+wait, not positive
  authentication/media/client proof. No product gate closes from HOST results.
- Finite RTSP native entry source pack reviewed; independentHOST rerun active.
  Future native operation is READY15/root-origin/2M/real provisioning/owned
  retirement only, not positive Digest/RTSP/media/NVR acceptance. PRIVATE PC tool
  not executed by worker, password stays PC-only, challenge/material hashes/logs
  forbidden. Historical output-scope deviations preserved/disclosed, final
  builder uses assigned evidence lane only. No target operations during UARTrestore.
- Actual nativeUI02 got2M READY/client started then ~5s watchdog-reset failure.
  No full TLS/client/owned shutdown pass; heldUboot observed, oldLinux/arm/IDs not
  current. Offline first-exe-check race investigation, not native-cause proof.
- h finalized exact originals; fresh i sole receive/new bounded02:28:57Oct13 hold.
  Retained75 RAMCRC mismatch deniesboot. Verified existingQCLDR1872/dd0572e1 loader
  active; exactsame75 framed RAM restore **sh_13240dda10012UTA73u3Xxdnl5** exclusive
  UART, no concurrentdeviceTX/privateprovision. No kernel boot until actual full
  completion/CRC/commit; no SPI/SDdata/saveenv or new firmware/stock startup.
- Private-profile v2 independent57/1220 each realcrypto/C++/ARM link reviewed;
  nativeunseal/livecheckpoint/storage/full2Mstillblocked. Next source unit private
  handle+network metadata integration with no realradio/profile/key/device effects.
- G711/audio packet source returned but pre-borrow auth boundary HOLD: denied
  consumer could already hold capture slot before admission. Correct before real
  source effects, retaining after-borrow/per-effect/queued checks and safe returns.
- Private-profile source candidate returned HOST32/686 checks, native-before-key
  HOLD; parent found live read/use authority gap (cached nonce/commit checkpoints).
  Request live scope/key/global-nonce/scoped-commit validation around blocking
  work/export, strict basic hostname/gateway validation. Real unseal/entropy/full
  2MiB/store durability still external gates. Private payload/ciphertext/key/nonce/
  tag/checkpoint fixture hashing remains forbidden; no operator files or hardware.
- RTSP verifier v2 independently217 real-child/82 startup/18 engine checks each,
  C++/ARM link PASS. Root/current native2M READY15/owned wipe+exit stillHOLD; HOST4M
  real hardening is a separate evidence class. Lane advances to a small finite
  parent native qualification helper/material-tool source pack, no listener or
  source activation/new transport/guardian. Password stays PC-only; no material
  payload hashes. Native positive grant/per-effect/NVR/product acceptance open.
- Audio v2 independently39728 checks each +C++/ARM static/kernel-object proof PASS,
  no ARM/native execution. All seven waiting-state deadlines reviewed; backing
  remains held on unknown STOP/RELEASE, exact returns/fences required. Source
  capture probe still refuses. Next independent audio unit: small PCM/G711/RTP/
  rational timestamp/span helper, not microphone/8k/browser/A-V/client acceptance.
- Native UI entry v3 returned95 models but SOURCE review HOLD: status16KiB cap
  undersizes audited32KiB recent RX/JSON; actual UART health on every local RPC
  cannot be furnished without untracked target reads. Same-entry correction uses
  bounded status capacity plus explicit tracked phase/effect health barriers, no
  invented zeros/new observer/guardian. Preserve creation/revalidation timestamps
  separately; old arm identity/end cannot be restamped or inherited by another
  owner. No native02 staging/run from this returned source candidate.
- Isolated RTSP verifier HOST4M child218 checks/engine18 returned but parent
  ownership review HOLD: forked child could exec/read material before pidfd. Add
  canonical launch-release barrier and explicit native image/material metadata
  checks; native root-origin/groups0/2MiB remains unqualified. No caller FD/state
  is freed from a signal request or failed ownership acquisition.
- Audio capture-only source/ALSA objects returned but parent timeout HOLD: startup
  CLAIM/CONFIGURE and closing STOP/RELEASE bypassed capture-only deadline check.
  Fix all pending states with one owned STOP/fence quarantine, no retry/free.
  Probe remains unconditional no-effect; no actual mic or speaker inference.
- h actual sole receive/control peer/adapter/current-instance arm rechecked with
  5848s remaining; no targetTX or new native job. Historical runtime/image/owner
  fields below are not fresh admission. Parent alone owns target operations.
- Network store v2 independently400 contexts/7510 checks each, C++/ARM link and
  v1 bad-at-deadline/v2 retained-fence reproduction PASS. Shared runner30C2Python76
  receipt PASS; actual physical store/radio/secret-profile remain unqualified.
  Lane advances to private-profile crypto/owned-store source candidate, no device
  or real credential reads/writes. No format/heap/security ceiling increase.
- UI boundary v2 independently63 models pass. Source-only existing UART receiver/
  paced Linux-command entry binding is next before native02; no new owner/guardian,
  no replay of partial native01, no native campaign from model booleans.
- Reset-input independently252584 checks each/C++/ARM link pass; SOURCE/HOST only,
  no action callback, reset executor or native input proof. Peripheral lane now
  prepares capture-only ADC2/ALSA candidate with no-effect admission until mic/
  clock/L2/routing/IRQ/cache retirement are qualified. No speaker inference.
- Network metadata adapter returned actual settings/core integration; parent found
  post-fsync admission callback could consume deadline before core NEXT used its
  old timestamp. Correction/fault regressions requested, no radio/storage effects.
- UI parent recipe independently44 models pass; parent requested dot/dotdot leaf
  and private-scope/image collision rejection before actual entry integration.
- Native v5 remainsHOLD: onlyRX staging was charged to non-CMA floor, missing
  unpaid CPU worst placement and migration of future-evicted CMA loans. Supplied
  snapshot deficit704KiB for v4 (v5 adds32KiB). Correct SAME gate before allocations;
  source-audit rawCPU/input-to-NAL temporal backing reuse only if exact synchronous
  input-copy/reset/guards/output-copy contract permits. No kernel change, automatic
  aliasing, reserve/quality/CRC/deadline relaxation or native launch from mocks.
- PTZ v2 correction independently reviewed and passed14 suites/11466 checks each
  optimized/sanitized, C++ link/run and ARM static link (not executed). Original
  null-MOVE counterexample and664 V1 snapshot files retained. Parent fixed only
  the builder's relative-toolchain resolution after preserving its failed run.
  This is SOURCE/HOST adoption only: physical routing/current/displacement/STOP
  remains unqualified. Peripheral lane advances to input-only deliberate reset
  debounce/events; no reset executor or firmware/footage operation.
- Native memory review now checks the observed non-CMA placement margin separately
  from total availability. CMA eligibility is not a guaranteed allocation location;
  no watermark manipulation/reserve reduction to force admission. Media run stays
  HOLD pending reviewed memory gate, actual placement and fresh protection.
- RTSP v3 independently reviewed/reran36 campaigns (84085 optimized/103365
  sanitized checks, plus15 RFC/backend and4 startup-wipe each); C++/ARM link pass,
  ARM never run. Released packet scalars and post-BEGIN source guards are fixed.
  Next isolated increment is an RTSP-only hardened verifier/owned bounded IPC;
  actual native entropy/isolation/producer/retirement/NVR still not qualified.
- PTZ owner v1 returned5655 optimized/sanitized fixture checks but parent SOURCE
  review found malformed MOVE/STOP could neutralize/fault an existing owner before
  authorization. Candidate HOLD; correction/regressions requested and v1 retained.
  Per-effect real shared policy is necessary but does not establish physical STOP.
- Active source lanes now: media memory reserve, corrected UI parent campaign,
  RTSP isolated verifier, network transaction store and PTZ boundary correction.
  Parent retains sole hardware/integration ownership; no candidate listener or
  physical actuator is enabled from these source/host results.
- Parent h receive owner replaces error-finalized g. New hold22:56UTC; actual
  kernel GNU build ID/config matches75. Do not reuse h unless revalidated.
- V4 handoff independently fixture/build-passed. Off-board removes RAM-file debt;
  strict memory-debt/non-CMA floor implementation is the remaining launch gate.
  Parent freezes512KiB overhead/1024KiB reserve for one finite experiment only,
  not a proved peak/guarantee or an increased security ceiling. Observed Normal
  watermark high1290 pages includes boost; empty Movable is not a populated zone.
- UI source/static diff/134 realTLS/matched launcher HOST checks independently
  pass; actual dummy browser8 modules sequential/no overlap passes. Native UI01
  strict2MiB READY+natural180s owned retirement pass narrowly. Client never started
  due parent marker parser/echo false-positive; native assets/login/SIGTERM NOT
  tested. UI agent prepares source-reviewed client-on-READY orchestration using
  the existing finite watchdog recipe, no added daemon/guardian/transport phase.
- RTSP v3 fixes use of packet keyframe after destructive release; parent review
  and independent rerun next, existing server unchanged. MMC v2 exhaustion fixes
  independently3230 checks/object/link pass; native provider still refuses probe.
- Network agent advances to actual settings-store transaction metadata adapter,
  host-only. Peripheral agent advances to bounded PTZ owner candidate, host-only.
  No radio/AP/SD/coil effects or fake authentication from fixture flags.
- Known native UI01 temporary private/public copies were unlinked after observed
  normal retirement. Original PC inputs/builds and all failures retained. Empty
  owned dirs remain; no recursive deletion or touching original footage/settings.

All product gates stay open; parent alone owns every actual device operation and
integration/status changes. Next native source experiment waits for reviewed
strictMemoryReserve, current image/ABI/owner/recovery and fresh measurements.

Continuing queue after the second restart:
- g recorder cancelled by restart; do not reuse20:49 timer/PID/socket. No native
  candidate was launched. Parent is re-establishing bench protection separately.
- UI C candidate returned491 canonical +127 real-TLS asset checks, full reply
  max7491B and unchanged child; parent reviewed exact static-only diff. Serial TLS
  assets do not qualify ten-module parallel loading under2 peers/1 handshake.
  Agent now prepares serialized CSP-safe module loading without increasing caps.
- Native off-board sink/exact139 guard work and MMC host-port candidate remain
  active. RAM-file handoff launch staysHOLD under reviewed peak budget.
- RTSP authenticated server candidate remains isolated/host-only pending review.
  Native media501/RTSPNOAUTH, stored footage and firmware remain unchanged.

Latest parent review/restart boundary (supersedes dated rows below):
- Server restart cancelled the f recorder. Its socket was rechecked refused/
  ownerless and preserved; interrupted e/f raw/events remain untouched, not given
  fabricated normal-finalization receipts. New g sole receive owner/fresh hold
  through20:49:24UTC and actual Linux/ISP/WLAN384/clean UART were re-established.
- Running IKCONFIG matches sealed75 `.config` byte-exact; driver/source/lifetime
  readiness is still checked independently. Sensor is stream-off720p but actual
  HTS1650 differs from retained exact139-pair HTS1800. Mismatch preserved; guarded
  explicit preparation is pending, no inferred FPS or register defaults.
- Native handoff v2 adds real full scalar old-NAL checks before/after lookahead;
  independent parent40 handoff/24 adapter mock cases and ARM link pass, not native
  retirement. **RAM-file launch HOLD:** added worst-case payload14,039.5KiB exceeds
  measured13,972KiB MemAvailable before overhead. Off-board bounded qualification
  sink is being prepared without file retention/ceiling raises/new live service.
- RTSP wire unit independently passes3489 optimized/sanitized checks; existing
  server stillNOAUTH. Agent advances to an isolated authenticated-server candidate,
  not direct production modification.
- Recording index unit independently passes2262 checks/25 synthetic filesystem
  cases; shared29 C +2 Python +76 receipt pass. Native read-only inventory finds no
  MMC block device; current75 has MMC disabled. A read-only MMC host-port candidate
  is queued, no card mount/write/prune/driver activation.
- Player v2 actual browser found illegal native-fetch receiver despite mocks;
  v3 fixes binding, parent78 tests and real PUBLICDUMMY browser refresh/login/logout
  pass (HTTP fixture, not targetTLS). Media501/start/controls disabled remain true.
  Agent advances to bounded C HTTPS/UI-assets candidate, production originals
  unchanged. Ten modules fit existing8192 response ceiling.

Next active source assignments: native guarded139/off-board sink and peak-budget
review; RTSP server candidate; native HTTPS/UI candidate; MMC host-port candidate.
Parent owns actual bench, independent reviews and next native source qualification.
No fixture/loopback/build result passes G3/G5/G6/G8 or exact15fps hardware gates.

User steering: do not pause merely because a bounded task completes. Parent
reviews each result, fixes/retests when needed, then advances the next eligible
task; missing physical measurements/owned clients remain explicit blockers, not
false completion. Work continues on independent lanes while blocked lanes wait.

| Owner/session | Next isolated implementation | Write ownership / boundary | Current state |
|---|---|---|---|
| Native candidate agent `ses_ecf487f78ffeMktebViXELpWgd` | Actual source-based bounded producer/hub lookahead helper, metadata/timing, host faults and ARM build | `apps/web-media/native-producer/`, own evidence; no hardware/driver/kernel edits | Running; parent reviews ELF/source/lifetime/budget before native execution |
| RTSP agent `ses_ecf9d217affelsFHNKko4l2cRS` | Bounded Digest wire parser/challenge/admission adapter | New `rtsp_digest_wire.*`/test, `apps/rtsp-auth/wire/`; existing server stays NOAUTH | Running; parent wire/crypto/policy review and integration later |
| Player agent `ses_ecf487f10ffeAlqFRrT2Npy74g` | Local authenticated UI/MSE client and lifecycle/race tests | `apps/web-media/player/`, own evidence; no production service assets or private inputs | Running;501/unqualified states remain truthful, no fake live video |
| Storage agent `ses_ecf9d2163ffePl7ipWpV84va7g` | Portable recording-index format and anchored metadata transaction store | New `recording_index_store.*`/test, own docs/evidence; synthetic host dirs only | Running; no real footage/native SD writes/deletion |
| Parent | Actual native admission, integration reviews, source/ABI/ownership qualification | Sole UART/device owner; central runner/status only after review | Current f ownership/arm rechecked15:28UTC,9342s until18:03:42 hold end; source helper still pending |

Every helper stays inside existing CRC/guard/entropy/locking/time/memory/footage
constraints. No agent is authorized to operate hardware, read real provisioning,
enable media/RTSP auth merely from fixtures, install dependencies or publish/commit.
Round01 outcomes below remain preserved; round02 is not an acceptance claim.

Separate source ownership; no worker changes the central runner, existing runtime
services, active plans/status, private inputs, hardware or Git index.

| Owner/session | Bounded assignment | Owned write scope | State |
|---|---|---|---|
| RTSP agent `ses_ecf9d217affelsFHNKko4l2cRS` | Standalone MbedTLS-backed Digest/nonce/replay core and public offline vectors | New `rtsp_digest.h/.c`, test; `apps/rtsp-auth/`; own evidence | v2 reviewed; parent optimized+sanitized566 checks each, C++/ARM link; NOT parser/server/native/NVR integration |
| Storage agent `ses_ecf9d2163ffePl7ipWpV84va7g` | Pure owned recording catalogue, retention reservation/ack and playback pins | New `recording_catalog.h/.c`, test; own docs/evidence | v2 alias correction reviewed; parent15 suites/4680 checks, overlap/C++ tests; pure case added to shared runner, NOT filesystem deletion |
| Network agent `ses_ecf9d213dffe6hiSbWonVfY1ax` | Receipt-before-disruption/revision/confirm/rollback/cancel state | New `network_transition.h/.c`, test; own docs/evidence | v2 rollback ordering reviewed; parent90 contexts/18667 checks, C++/ARM link; pure case added to shared runner, NOT native reconnect/persistence |
| Performance agent `ses_ecf9d211affewWa3tpJ8tk0vGm` | Exact-part capability evidence, bottleneck distinctions and bounded measurement plan | `docs/qualification-plans/qc07f-performance/`; own evidence | Parent citations/captured hashes/arithmetic reviewed, two wording/origin corrections; sensor maximum not encoded-retail proof |
| Peripheral/NVR agent `ses_ecf9d2117ffea7YNRvp2xDBgnT` | Routing/presence/safety/physical dependencies and truthful client/interface matrix | `docs/qualification-plans/peripherals-audio-nvr/`; own evidence | Parent document/safety/dependency review complete; required routing/audio/client measurements remain open |
| Parent | Native stream/source integration, bench ownership and central integration | Existing live/protection/build interfaces and central plan/runner | Sole hardware owner; no concurrent UART/DMA/device operations |

Unassigned portions are not forgotten: motion detection/storage I/O, actual AP/
secret persistence, peripheral drivers and ONVIF/client integration follow their
listed dependencies and may get separately bounded assignments after these reviews.

## Parent review outcome

Evidence: `evidence/delegated/20261012-parent-review/review-and-adoption01.json`.
Shared integration passes **28 C +2 Python +76 receipt cases** with sanitizers.
Digest crypto stays in its separate RTSP-only build/config; HTTPS/vendor config,
600k/2MiB, source CRC/guards and runtime501/NOAUTH are unchanged.

Actual parent findings corrected before adoption:
1. Catalogue output/plan overlap could partially reserve; v1 counterexample retained,
   v2 rejects safely before mutation and parent reran explicit overlap regressions.
2. Digest's pinned cnonce unnecessarily rejected valid fresh client nonces; v2
   accepts a correctly authenticated new cnonce with higher global nc, still denying
   equal/lower counts and not advancing failed-request state.
3. Older rollback-positive evidence could survive newer delivered link loss;
   v2 tracks exact rollback observations, refuses stale restoration and keeps live
   work until latest proof/explicit retirement acknowledgment.
4. Clang found an uninitialized negative-test work object; parent initialized that
   fixture, preserving the failed compile and leaving runtime behavior unchanged.

All five assignments are complete in their bounded scope; **no product subsystem
is complete**. No agents operated hardware or enabled native services. Follow the
native integration/measurement path below each lane; do not merge everything into
the live image merely because independent fixtures pass.

Parent bench recovery boundary: stale owned IPC socket preserved by rename after
refusal/no-owner checks; original e logs remain untouched/unfinalized. Fresh f
receive-only recorder/sole adapter ownership and bounded hold18:03:42UTC established.
Read-only current6.18.55 RAM shell, ISP idle/guards0, watchdoginactive/UART0 and
WLAN192.168.1.73/24 MTU384 revalidated. Exact source/encoded receipt readiness still
needs the next experiment. These are dated observations, not inherited timers.

## Mandatory parent review before adoption

1. Inspect actual new/modified files and dependency/source/license pins, not just
   the agent summary. Preserve unrelated/staged/user files and all failed runs.
2. Check API compatibility, bounded memory/time/state, fixed-ID conventions,
   namespace/generation/replay/clock/partial-result correctness and secret handling.
3. Verify no crypto/locking/integrity/timeout downgrade or fabricated hardware
   capability. No helper grants authority merely by a model `verified` flag.
4. Independently rerun focused positives/negatives with sanitizers, add only
   reviewed cases to the parent-owned runner and rerun shared regressions.
5. Review build ABI/C++/ARM source/runtime closure; link proof is not execution.
6. Label each deliverable **source / fixture / browser / native / product**.
   Keep current501/NOAUTH until their native integration is genuinely qualified.
7. Reject or send targeted corrections back to the same agent. Record reviewer,
   commands, counts, source hashes, failures, adoption scope and next real test.

No target/UART/network-camera/guest-secret/SD/SPI/motor/light/audio operations by
subagents. No real credentials, payload hashes, device-data publication, dependency
installation, commits/remotes/push or unrelated-device access. Hardware stays
serialized with fresh actual ownership/runtime/recovery; no guardian/transport
scaffolding expansion is authorized by this split.
