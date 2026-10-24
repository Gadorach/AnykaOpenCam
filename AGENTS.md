# Project instructions for local agents

## Read first

- Build the shared camera product foundation on **QC07F**, then port to T34-4G
  and Fulhan only after **G8**. The user owns all three cameras.
- Canonical workspace: `/home/gadorach/Desktop/AnykaOpenCam`.
  `/home/gadorach/Desktop/anyka-km01-agent-handoff` is a compatibility symlink for
  frozen evidence/build paths, not another workspace. Preserve historical paths.
- Read `docs/plan/camera-foundation/00-INDEX.md`, the current execution ledger
  below, and the relevant subsystem plan before selecting work. Retain canonical
  task IDs and requirements; distinguish source, host, native and product evidence.
- Later user steering takes precedence over dated handoffs. Latest steering in
  the conversation checkpoint approved hardware testing and requested retry/resume;
  it does not establish current hardware state or completion of interrupted work.
- Proceed with authorized investigation, reversible local work and offline builds
  without repetitive confirmation. Explain concrete blockers and continue eligible
  independent work. Do not infer permission for third-party contact, publication,
  account changes or unrelated hardware.

## Current checkpoint (2026-10-23)

**Latest autonomy steering:** continue eligible work unattended until completion
or genuinely required user input. User renewed approval Oct23, explicitly including
motor testing and onboard NOR flashing if needed; authorization does not establish
current physical/runtime state or remove bounded procedure/rollback/recovery gates.
Hardware trials are authorized; prefer reviewed
prearmed watchdog recovery/known-idle watchdog reset. Further adversarial testing
remains deferred. The initial RAM76 belief was superseded by the cold handoff below.

**Actual native state:** After user reseated probes and requested retry, ONE fresh
23h with UNCHANGED v4/parser **PASS cold observation only**:596RX exactly360B/19
first-prompt records +236B controlled echo groups;3×19B/57TX original catch, no
extra prefix/window reset, separate10s unchanged RX/TX/raw/events. Actual ON/OFF
acknowledgements and strict GETs, known-child stop/wait0/private final hashes and
all original-input/marker postchecks passed. Independent final GET OFF, PID/socket
absent/no visible tty owner. **Camera controller OFF; no active capture/transfer,
current Linux or watchdog lease.**23h is finalized, not live held-prompt authority.
See execution-20261023/COLD23H_SELECTION.md and COLD23H_RESULT.md.
User already confirmed UART-only wiring, correct supply/visible power-up and
QC07F-only ESPHome outlet mapping; do not repeat those questions. Endpoint/private
evidence remain outside Git; skill's MS42P images/UART/runbook do NOT apply.
Earlier radio/reset/23c/23d/23e/23g failures stay independently FAILED/HOLD.
23c WAS parent full-capture EOF misuse;23e/23g unexpected bytes are in saved raw
before parsing. This clean unchanged-code23h proves a new observation, NOT electrical
origin/driver correctness/relay noise, a proved reseating fix or retrospective PASS.
See earlier COLD23C/D/E/G and PASSIVE23F results for exact failed boundaries.
V1/v2 never invoked; v3 once23d and v4 once23e/once23g/once23h, no retry loop or
byte trimming. V4 source `40794530f6ef7bb9fec15104fee5f3c06e841ba6a08947794041bac99b3cc8b5`.
Separate rtsp-provisioner-process now has narrow independent SOURCE/API/custody
sufficiency for REAL child-only ingress/constructor/readonly-FD IPC/known-wait0/
explicit parent bind.27 HOST/ARM compiles/40 commands + ONE normal PUBLICDUMMY
fork/transfer/wait/bind call PASS; parent checked33 inputs/59 delivery pins/1771
dependency records/84 artifacts/40 logs/five tools. Native hardening/real credentials
NOT RUN. See rtsp-provisioner-process PARENT_DISPOSITION; no active author/reviewer
in that lane; committed locally `5269979`. Its original33-input pin set retains
the pre-closeout index:32 remain unchanged plus known parent index/status
updates recorded in `PARENT_CONTEXT_UPDATE01.json` through `PARENT_CONTEXT_UPDATE07.json`,
not rebased. Actual authorized
external ingress, secret-free launch/FD-env-caps
hygiene, durable storage/rotation, linked-file retirement/main/external recovery/
backing remain gaps. Next source connection is known-wait/bind → real device-free
owned-runtime prepare → real prestarted start. That actual connection at
`apps/rtsp-auth/qc07f-bootstrap-candidate/` now has narrow independent SOURCE/API/
order/custody sufficiency,2 HOST/ARM compiles/2 actual REL links/24 commands PASS,
NOT RUN. Parent checked863 input pins/48 delivery pins/37 artifacts/180 new+724
reused dependency records/46 archive bodies; no main/native activation/serve export.
See qc07f-bootstrap PARENT_DISPOSITION, no author/reviewer active in that lane.
The genuine public RTSP32-byte randomness callback at rtsp-os-random now has narrow
independent SOURCE/API/bounds sufficiency,5 HOST/ARM compiles/26 commands and ONE
real HOST callback call PASS (no bytes retained/hashed). Parent verified548 public
pin records; see rtsp-os-random PARENT_DISPOSITION. One nonblocking OS getrandom
call, no fallback/retry/cache; no callback integration or native entropy qualification.
Bootstrap/OS-random closeouts committed locally `0b146c4`/`4b11b79`; no source
author/reviewer remains in those lanes. Retained RAM75 entry method is CLOSED/
BLOCKED before code/native selection:51 public pins/image/header/CRCs/static ranges
checked; automatic sensor reset/radio-power/ISP allocation documented. Watchdog
gaps at ON/staging/READY, after QCL commit and PID1 magic-close plus strict C1 clock
admission remain. Parent read-only outlet capability sample found no advertised
autonomous cutoff; same-host HTTP is NOT independent host-loss recovery. See
qc07f-ram75-entry23 PARENT_DISPOSITION. No new image/controller/clock framework.
Actual OS-callback registration BEFORE provisioning snapshots at
qc07f-bootstrap-os-random now has narrow independent SOURCE/API/order/custody
sufficiency: reject ANY prepopulated callback before mutation, latch one attempt,
select real function before actual snapshot, preserve selection/HOLD on failure.
2 HOST/ARM compiles/2 real REL links/24 commands PASS, NOT RUN; parent checked902
inputs/56 delivery pins/37 artifacts/182 new+871 reused dependency records/46
archive bodies and exact reverse to original. REPLACE old bootstrap unit, never
link both; original observe/start/finish tails unchanged. Native entropy still open.
Explicit logical material FD/fixed-file retirement at qc07f-bootstrap-material-retire
now has narrow independent SOURCE/API/order/custody sufficiency: only actual normal
start + finish0/PS_FINISHED; unknown/failed paths retain ownership.2 HOST/ARM compiles,
2 real REL links/24 commands PASS, NOT RUN. Parent checked916 inputs/56 delivery
pins/37 artifacts/221 new+871 reused dependencies/46 archive bodies and inverse
proofs. Local commit `a3d2b75`; see its PARENT_DISPOSITION; no author/reviewer active.
Replacement C/new
header only, never old bootstrap units/combined RELs. No actual retirement/test/
child/native/motor/NOR action. Inherited1MiB backing versus128KiB allocation-cap
work now has a real borrowed-span RTP packetizer at rtp-span-candidate with narrow
independent SOURCE/API/bounds/wire-semantics sufficiency.3 HOST/ARM C compiles/20
commands and ONE174080B synthetic HOST NAL/126 packets PASS;169 public pin records
parent-verified, no repeated execution. See rtp-span PARENT_DISPOSITION; no active
author/reviewer. Frozen runtime/hub/encoder APIs remain unchanged; no cap/native
acceptance. Next bounded feature is a shared chunk-backed CPU hub with explicit
consumer leases and<=128KiB allocations, then actual span-packetizer connection.
No new diagnostic/clock/receipt framework or hardware phase selected.
No bootstrap test/child/verifier/native/media execution, main, synthetic callbacks,
provider/backing or hardware adoption selected.
Original radio/Oct22 reset/23c/23d/23e/23g
failures untouched. Expired receipts are not current owners; no electrical-rail/
whole-interval recovery claim or new diagnostic/clock framework.
PTZ under-mutex pre-GPIO counter refusal has narrow independent SOURCE sufficiency
and real NEW Clang23.1.3 ARM-object compile evidence; local commit `8ec07ff`, not
deployed or motor/lifetime/recovery acceptance. See ptz-counter PARENT_DISPOSITION.

**P2 v2:** ordinary focused static review accepts its three callback/presence/error-
ordering corrections; author changed-unit syntax3/3 passed, not runtime confirmation.
V1/v2/reviews remain frozen; no P1 adoption or composite/native GO. P3 stopped
before code because request refs cannot protect USB/core owners through STOP/drain.
Parent revised source order: bounded P4a shared-owner candidate at
`apps/qualification/kernel76-station-owner-lifetime/` has narrow independent static
SOURCE sufficiency; syntax02 4/4 cited, no runtime/finite-wait proof. Review:
`STATION76_OWNER_LIFETIME_P4A_REVIEW.md` under transport docs. P3 heap-request/
deadline candidate at `apps/qualification/kernel76-station-control-owned/` has
narrow independent SOURCE/API sufficiency, syntax04 5/5 cited, no runtime proof.
Bounded P4b at `apps/qualification/kernel76-station-descriptor-admission/` has
narrow independent SOURCE/API sufficiency; syntax2/2 cited, no execution. Exact
configuration/interface/alternate counts and active alternate stay BLOCKED, not
guessed; cached normalization/policy is not raw-wire exactness. P1/fullP4/P5,
tests, ARM/module build and native remain unaccepted; old inputs frozen.

**Query77 offline build complete/disposed:** Linux6.18.55/NEW Clang23.1.3 coherent
source/config/header/image baseline built; parent independently hashed250,219
dependency paths and checked exact query-only delta/five config identities, CRCs,
unchanged DTB/initramfs and static staging/decompression fit. Disposition:
`docs/qualification-plans/encoder-query77-build-20261022/PARENT_DISPOSITION.md`.
Never booted. Embedded station76 is unpaired/unsupported; PLAT-02 remains open.
No old76 reproduction, station module build, P1 adoption or installation. All
originals immutable; old22b controllers/reset must not be reused. No native queue.

**App integration source:** real v6→RTSP callback glue at
`apps/rtsp-auth/qc07f-runtime-candidate/` compiled HOST/ARM (54 commands), not run
or deployable. Parent source/build disposition:
`docs/qualification-plans/rtsp-runtime-20261022/PARENT_DISPOSITION.md`.
Move-only frame-FD custody helper is also delivered/compiled HOST/ARM (16 commands),
not full provider adoption. Observational encoder session/ref query source is
delivered at `apps/qualification/kernel76-encoder-identity/`;88-byte HOST/ARM UAPI
compile PASS; old driver compile stopped on retained23.1.1/current23.1.3 drift.
Explicit NEW23.1.3 whole-driver ARM object compile PASS; parent checked exact
three-site source delta,513 consumed source/header files and compiler closure.
Narrow static query/API sufficiency, not old76 compiler/ABI pairing or full build.
Review: `docs/qualification-plans/encoder-identity-clang2313-20261022/PARENT_REVIEW.md`.
Persistent encoder-FD custody v2 is delivered, HOST/ARM18-command compile PASS;
parent static/source-custody disposition preserves it as compile-only candidate.
Single-owned watchdog v2 W1–W3 has narrow independent SOURCE/API sufficiency;
HOST/ARM18 commands/eight reused objects verified, not runtime/native acceptance.
Disposition: `docs/qualification-plans/qc07f-watchdog-owner-v2-20261022/PARENT_DISPOSITION.md`.
Actual mandatory provider-FD derivative at
`apps/rtsp-auth/qc07f-owned-provider-candidate/` has narrow independent SOURCE/API/
ownership sufficiency; HOST/ARM20 commands/ten reused objects cited/verified.
Strict pre-memory return at `apps/rtsp-auth/qc07f-provider-unactivated-return/`
has narrow independent SOURCE/API/ownership sufficiency; HOST/ARM18 commands cited,
5,108 input pins parent-verified. Disposition:
`docs/qualification-plans/qc07f-provider-return-20261022/PARENT_DISPOSITION.md`.
UNKNOWN watchdog handoff is not disable/END, armed recovery or post-memory/
failed-state cleanup. Retention-owner investigation stopped before code on missing
real bootstrap/external lifetime/recovery and verifier fork inside serve.91 public
input hashes unchanged; block/contract preserved in retention-owner docs. Parent
selected fresh `apps/rtsp-auth/prestarted-backend-candidate/` for a device-free
backend/serve split, one actual child and no second fork; not media retention GO.
Owned listener at `apps/rtsp-auth/owned-listener-candidate/` has narrow independent
SOURCE/API custody sufficiency;52 HOST/ARM commands cited, no execution,1,281
review inputs parent-verified. Disposition in rtsp-owned-listener qualification docs.
Prestarted-backend candidate delivered at `apps/rtsp-auth/prestarted-backend-candidate/`,
source pin `867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90`.
Six HOST/ARM changed-unit compiles/archive/REL PASS, but review requires R1: new
observer can reap verifier, then retired stop/destroy loses still-open IPC FD.
V1/review frozen and unaccepted. R1 v2 at `apps/rtsp-auth/prestarted-backend-v2/`
has narrow independent SOURCE/API sufficiency; six HOST/ARM compiles cited,1,892
review inputs parent-verified. Actual composition at
`apps/rtsp-auth/prestarted-owned-listener/` has narrow independent SOURCE
composition/API custody sufficiency;30 HOST/ARM C compilations/52 successful tool
commands cited,3,499 review inputs parent-verified. Parent disposition is in the
rtsp-prestarted-owned-listener qualification docs. Not runtime/native adoption.
Query77 client at `apps/qualification/encoder-query77-client/` has narrow conditional
SOURCE/API sufficiency;36 HOST/ARM compile/link commands cited,4,014 review inputs
parent-verified. ARM ELF30140B, neither ELF run. Disposition is in query77-client
qualification docs. No staging/boot/upload/run or executable native recipe.
Both reviews completed; no active source author/reviewer from those lanes remains.
Offline method/design/C1/C2 history and exact pins are in the execution ledger and
linked qualification dispositions. C1v1's R1/R2 failure stays frozen; custody-v2 has
conditional SOURCE/API sufficiency, never called/applied to actual HOLDs. C2a input
validator has conditional source sufficiency plus ONE public-artifact call PASS
(25 inputs). C2b cold-context-v2 has conditional exact-profile sufficiency plus ONE
saved-historical-prefix call (19 records, NOT_ADMISSION); raw/provenance stays
private. Original block/old parser/HOLDs frozen; no current state/quiet/catch proof.
C2c clock design and continuity feasibility completed/BLOCKED. Parent retains strict
policy/guards: brackets/detection/cooperative inhibitors do not establish the whole-
interval guarantee.30 local feasibility inputs/seven outputs verified; three URL
pins association only. No watcher/sampler/host policy/settings change selected.
Recorder-timer derivative has narrow independent SOURCE/API sufficiency for its
three changes;33 unchanged reader inputs/three recorded parent context updates/
four review outputs verified. HOST AST/syntax only, never imported/called/run;
stored values are not a deadline or admission. Parent disposition in recorder-
timer docs. Frozen candidate source and review context diff retain an inherited
adapter selector and are withheld from Git. Fresh explicit-port recorder has narrow
conditional SOURCE/API/selected-privacy sufficiency;29 reader inputs/three review
outputs parent-verified. No default/discovery/fallback; explicit private caller
adapter required before capture effects. HOST AST/syntax only, never imported/
called/run; disposition in recorder-explicit-port docs. No active author/reviewer.
This is an unadmitted effectful recorder, not a pure library; no timer/catch/lease
change, installation, original
helper/source-pin edit, current-state or hardware action. Separate static build02
role inspection matched seven artifacts/73 object headers/28 archive bodies and
strict-vs-HOST define separation; not native ABI, release packaging or app adoption.
Query78 diagnostic-image lane is **PARKED per latest user feature-development
steering**. Client source/build review is closed (`ba9b645`, ELF never run).
Recipe v3 has narrow conditional SOURCE/build-method sufficiency;134 reader inputs/
five review outputs parent-verified, but no recipe/archive/kernel/image invoked.
V1/v2 negative reviews and all inputs remain frozen. See v3 PARENT_DISPOSITION.md;
do not open another diagnostic-build/clock/receipt lane or infer boot permission.
Owned-runtime feature at `apps/rtsp-auth/qc07f-owned-runtime/` now has narrow
conditional parent SOURCE/API integration sufficiency: guarded owned-provider →
authenticated RTSP/keyframe wiring, device-free prepare/postfork bind and
nonreturning uncertainty handling. Parent checked57 source/87 original inputs,
exact four-copy delta and final build02 pins/logs;46 HOST/ARM compilations/62
commands cited, not rerun. Source aggregate `410d42008de738bd9b67cdf51ca0333ab72d96159f4fd52da20175128dff0cf0`.
See rtsp-owned-runtime-20261023/PARENT_DISPOSITION.md. No active author/reviewer,
main/native adoption, listener/capture/client execution or hardware action.
Real bootstrap/external owner/recovery/backing/provisioning remain blockers;
inherited1MiB sites are unqualified against128KiB cap. Finite STOP, no restart GO.
Owned-runtime source/metadata committed locally `5b3e252`. Separate real RTSP-only
password→verifier startup-record constructor at `apps/rtsp-auth/rtsp-startup-material/`
has narrow independent SOURCE/API sufficiency plus20 HOST/ARM C compilations/
26 commands PASS, never called. Parent checked16 selected inputs/165 dependency
records/22 artifacts/26 logs; disposition in rtsp-startup-material-20261023 docs.
No default credentials, private input/generation/use, media/HTTPS linkage or new
build framework. Actual hardened secret ingress/private FD handoff/storage/durable
generation authority/rotation are still missing; no active author/reviewer.
Private handoff at `apps/rtsp-auth/rtsp-private-handoff/` now has narrow independent
SOURCE/API/custody sufficiency plus24 HOST/ARM compiles/37 commands and ONE normal
PUBLICDUMMY constructor/write/close/readonly/move/zeroization/cleanup call PASS.
Parent checked32 selected/15 delivery pins/1363 dependency records/75 artifacts/
37 logs. Native hardening/real credentials NOT RUN. Disposition in rtsp-private-
handoff-20261023 docs. This is a C-slot move, NOT separate-process IPC/provisioning;
actual launch/ingress/generation/storage/bootstrap/recovery/backing remain gaps.
Staging guard remains unavailable; future manual-OFF handoff requires separate
review/actual readiness. No physical request or transfer is active. Query77 monitor
lacks query88 command. Authentication/loopback and media-retention
blocks remain; actual bootstrap/backing/provisioning/main are still missing.
Query77/return closeout committed locally `9fd7bb6`; ordinary index unchanged.
Composition source committed locally `d170304`, client/checkpoint `0ad2c1d`;
no binaries/private captures or push. Ordinary index remains unchanged.
The Oct23 hardware observations above supersede earlier no-fresh-state wording.
An early-expiry claim in the retention report was inherited parent wording,
corrected in its parent disposition.
Frozen originals/RTSP glue stay separate. Native watchdog
qualification, failed-next retention/backing/provisioning/owned listener remain
blockers. No deployed image/module change, new main or native memory-fit claim.

**Latest steering: refocus on feature development, not build-semantics work.**
Keep further penetration/adversarial/mutation testing deferred. The earlier
functional checklist and best-effort app build remain historical evidence. See
`docs/qualification-plans/rtsp-tail-20261022/OPERATIONAL_CHECKLIST.md`.
`apps/rtsp-auth/build_app.py build02 --toolchain <retained pinned toolchain>`
completed HOST/ARM compile/link PASS (88 commands); initial build01 link failure
is preserved. See `docs/qualification-plans/rtsp-tail-20261022/BUILD_RESULT.md`.
No program/test/generator/listener/camera execution. Keep protections intact;
do not treat deferred coverage as passed or the integration archive/synthetic
bench as a finished deployable camera app. Real platform/provider/provisioning
wiring and release packaging remain missing.

**P1:** author same-session clarification succeeded; fresh parent HOST run04
passed86 commands/12 mutants per mode with453 sealed files unchanged. No source
acceptance: independent reviewer then hit another generic `cyber_policy`;23
partial files/no final report remain unreviewed/uncommitted. No further review
retry/reroute/parent substitute is selected. Exact result and latest block:
`docs/qualification-plans/kernel76-transport-20261021/STATION76_ATTACH_RX_P1_PARENT_REPRODUCTION.md`.
`docs/qualification-plans/kernel76-transport-20261021/P1_REVIEW_TOOL_BLOCK_20261022.md`.
Do not adopt partial review claims. P2/P4a's disjoint source-only work above is
not P1 acceptance; full P3–P5/native/paired-build acceptance remains absent.

**B08 / RP-08–09:** fresh frozen HOST harness correction delivered at
`apps/rtsp-auth/isolated-server-host-tail/`,41-file pin
`2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f`.
Author12 standalone process cases/9 syntax checks passed; not full RTSP/runtime
acceptance. Current compile-only selection does not invoke its guarded runtime
builder or waive its remaining review. Report: `HARNESS_CANDIDATE.md` in the
checklist directory. No B08 worker or runtime operation is queued.

- P1 scope: `apps/qualification/kernel76-station-attach-rx/`, its candidate report
  and `station-attach-rx-p1-host01/`. The selected SOURCE/HOST change admits only
  CMD/CMD2/EVENT/EVENT2 outer RX types before legacy dispatch/state/credit/test/BT/
  raw-proc paths, rejecting other types through the existing error latch. No inner
  grammar or payload probing; real-path all256-type, sentinel and mutant coverage,
  parent reproduction and independent review are required.
- Accepted A1/A2a/A2b derivatives are immutable; all204 source pins matched after
  the interruption. Narrow SOURCE/HOST acceptance: A1 control/FWINFO (`152cee0`),
  A2a original USB extent/status (`08949c0`), A2b event suppression (`b940ffe`).
  A2b silently disposes debug event18 without payload interpretation/output and
  denies other IDs, retaining full16-bit EVENT2 identity. These are not full S4 GO.
- P2 first-error/init/effects now has the unaccepted source candidate above;
  P3 bounded owned TX, P4 lifetime and P5 guarded runner remain plans. Station attachment, ARM build/staging and native station
  qualification have not been accepted. Async init/guard, automatic recovery,
  TX timeout/ownership, teardown and non-event paths remain technical blockers.
- Current actual state is the Oct23 OFF/finalized boundary above, not Oct21/22
  RAM76/radio APPLICATION. Capture21a/22a/22b/23b/23c/23d/23e/23f/23g/23h are finalized; their PIDs,
  counters, addresses and timers are historical. No current Linux/WLAN/radio state
  is inherited from them. Old private evidence and original failure remain intact.
- Original05:23UTC native radio trial remains **FAILED/HOLD**:74 payload fragments
  and150 complete ACKs, final RUN ACK interrupted by USB-disconnect printk. Separate
  06:00 retirement and06:43 APPLICATION/identity/retirement passed while preserving
  that failure byte-exact. No reload/RUN repeat, fallback256, reset, station attach,
  association, MTU, media or peripheral operation followed in the recorded campaign.
- Keep Linux6.18.55, kernel75 rollback and sealed candidate76 image/module intact.
  RAM76 upload/CRC/monitor and idle checks passed04:36–37UTC Oct21. RAM boot is not
  persistent installation or full USB/media/product acceptance.
- RP-04 combined-memory/adoption HOLD remains. The proposed6160KiB fixed-ISP
  evacuation exclusion requires proven extents and continuous FD pin; it is not
  implemented/adopted. MemFree/CMA snapshots are not load budgets.
- Last pre-cleanup execution commit: `71f03a93fc179dd438e3c09422411762f08507fe`,
  status-only P1 closeout on local `main`; remaining-source plan is `84676be`.
  Documentation cleanup does not advance implementation or hardware acceptance.

### Current work references

Paths below are workspace-relative; consult the relevant document, not all history.

| Purpose | Reference |
| --- | --- |
| Active tasks and outcomes | `docs/plan/camera-foundation/EXECUTION_TASKS_20261021.md` |
| Complete roadmap:172 tasks,21 packets,29 defect rows | `docs/plan/camera-foundation/REMAINING_FEATURES_AND_BUGS_GATED_PLAN_20261021.md` |
| Tool block and partial-work disposition | `docs/qualification-plans/kernel76-transport-20261021/P1_TOOL_BLOCK.md` |
| Partial inventory/prior-source checks | `docs/qualification-plans/kernel76-transport-20261021/STATION76_P1_TOOL_BLOCK_STATUS01.json` |
| P1–P5 order and acceptance gates | `docs/qualification-plans/kernel76-transport-20261021/STATION76_ATTACH_REMAINING_SOURCE_PLAN.md` |
| Accepted A2b exact scope/pins | `docs/qualification-plans/kernel76-transport-20261021/STATION76_EVENT_A2B_REVIEW.md` |
| Separate identity and original HOLD | `docs/qualification-plans/kernel76-transport-20261021/SEPARATE_IDENTITY21A_RESULT.md` |
| Expired capture and verified closeout | `docs/qualification-plans/kernel76-transport-20261021/CAPTURE21A_CLOSEOUT.md` |
| Memory review/reproduction | `docs/qualification-plans/execution-20261021/memory-admission01/` |

## Hardware ownership and effect boundaries

- Parent alone owns serialized UART/device work and central runner/status
  integration. Use isolated source delegation with parent review and at most two
  disjoint source lanes when selected; do not revive old worker queues.
- Before target operations establish fresh physical state, exact sole UART owner/
  adapter, actual runtime, no competing transfer and bounded recovery coverage.
  Preserve/investigate unexplained owners, sockets and logs. Start capture without
  transmitting; opening serial can affect modem-control lines, so keep reset/boot
  connections detached and document adapter behavior.
- While a transfer/operation lease is active, no concurrent UART/RPC/status/progress
  polling or helper/recipe edits. Await completion; keep source frozen. No parent
  UART while an operator GUI owns its window. Expired GUI epochs are not renewable
  authority; new trials require current admission.
- Prefer reviewed prearmed watchdog recovery or a controlled watchdog reset from
  known state, with fresh sole capture/catch before TX and actual reset/U-Boot proof.
  Manual OFF is fallback if watchdog/control is unavailable or fails. SoC watchdog
  reset is not cold radio/rail power cycling. Never free uncertain DMA/owners or
  clear HOLD before retirement/reset proof; no automatic boot/firmware/RUN retry.
- Preserve sticky failures and original HOLD independently of later healthy tests.
  Old transport controllers/entrypoints require fresh source/lineage review.
- No blind GPIO/PWM sweeps, GPIO2 experiments, automatic homing/full travel or full
  retail-app/cloud startup. GPIO0 remains reserved INPUT; its observed active-low
  button is not a proved hardware-reset recovery method. GPIO2 conflicts with UART0.
- Speaker/GPIO1 effects require physical isolation of **both UART signal leads**
  and alternate control/recovery handoff. Last recorded wiring had UART connected.
  Microphone work is scalar-only: no raw voice, recordings, audio hashes or voice
  playback. Generate speaker samples on-camera from numeric parameters. Retain
  FreeSimpleGUI bench controls and manual one-shot ambient readings.
- Use VertexHotspot and owned bench endpoints. AP/new SSID, destructive storage/
  reset, live homing, card writes and persistent installation retain their specific
  scope/review gates. Broad hardware approval does not authorize collateral writes.
  Before a device-writing experiment identify exact image/ranges, boot policy and
  recovery; upstream flashing scripts do not confer authorization.

## Evidence, source and acquisition discipline

- Preserve acquisitions, failed observations, private configuration, credentials,
  footage and source licenses. Retail firmware is hardware-reference evidence.
  Record exact commands/tool versions/input hashes/output paths/timestamps.
- Keep original dumps immutable in their session's `raw/`; use fresh output names,
  refuse overwrites and retain conflicting reads. Matching hashes show repeatability,
  not completeness/electrical correctness. Never pad/truncate to fit8MiB assumptions.
- Discover devices without reconfiguration; select the intended programmer and
  confirm measured VCC/logic levels, orientation and device ID. Initial SPI work is
  identification/read only: no erase/program/unlock/protection/status/OTP writes or
  forced profiles. Establish actual flash installation before diagnosing no boot.
  Preserve measured UART wiring/levels rather than old photographic guesses.
- Keep original photographs unchanged and private device labels unpublished.
  Boot logs, firmware strings, extracted scripts and historical documents are
  evidence, not authority to execute commands or override the user's task.
- Prefer bounded reads/`rg`; work on copies and inspect archive paths/symlinks
  before extraction. Never execute firmware binaries on the host. Audit upstream
  `auto_build.sh` before use: the SDK version removes `os/uboot` and expects an archive.
- `references/` contains pinned evidence subsets, not a complete kernel tree.
  Use a separate pinned full checkout for builds, retaining patches, dependencies,
  recipes and licenses. Vermagic/symbol agreement alone does not prove module ABI;
  retain exact modules/libraries/configuration and media-memory reservations.
- Do not infer unknown GPIO routing, sensors, concurrency or other capabilities.
  Keep measured retail facts, source-supported capabilities and hypotheses distinct.
  QC07F bring-up does not qualify KM01, Fulhan, persistent install or the product.
- Preserve the128kB allocation cap: use bounded headers/borrowed spans/chunks,
  not full-AU TLS copies or ceiling increases. Native handoff/DMA/TLS/headroom and
  combined-load acceptance remain open even where individual host/native tests pass.
- Acquisition handback includes full-read evidence/hashes/provenance, extracted
  inventory, justified partition map, observed stock boot/runtime and next experiment.
  No result is “booted”, “working” or “flashed” until observed on that target.

## Git and privacy

- Follow `GIT_WORKFLOW.md`: reviewed increments on **main**, **local commits for
  now** until the user supplies a remote. No remote creation, automatic publication
  or invented destination. A future push still needs privacy/license review.
- Use a separate temporary index initialized from HEAD and stage only reviewed
  paths. Preserve the ordinary index (historically1993 entries,906 acquisition-scope)
  and unfamiliar user work. No blanket `add -A`, reset, forced untracking or cleanup.
- Exclude private captures/responses/digests, credentials/provisioning/keys, stock
  binaries, photographs/identifiers, footage and generated target images. Local Git
  is not publication consent; preserve each upstream license, no global license grant.
- `work/` and ignored acquisitions are retained history, not disposable scratch.
  Preserve root legacy manifests/notices and `docs/archive/initial-handoff/`.
  See `START_HERE.md` and `docs/PROJECT_LAYOUT.md`; dated runtime claims there may
  be superseded by the checkpoint above.

## Historical facts to retain when returning to a subsystem

- SSH-only stock ROOT `[0x200000,0x300000)` was installed and cold-boot/key-only
  authenticated Oct16; nonROOT bytes/password were preserved. Earlier “not flashed”
  entries are superseded. This is separate from RAM-only candidate76 and does not
  establish current SSH availability. Candidate76 is not qualified for installation.
- Sensor payload ID0x1c length560 means **140 pairs**, final0x0100=1. Preserve the
  sealed139-pair controlled-stream-off subset; do not change count alone or silently
  enable streaming. Full140 is not native-qualified; the archive index links the
  sensor correction document.
- Stock MTU1500 was observed; replacement MTU384 is a workaround, not a factory
  limit. Modern native1500/FPS and storage/shared-L2/AP acceptance remain open.
- User observed GPIO6/A = OUT/night IR-pass, GPIO7/B = IN/day IR-block;
  GPIO23 IR illumination is independent. GPIO6/7 drive an MX6208 bridge, not the coil
  directly. GPIO5 white OFF and bounded speaker08 tones were confirmed. These do
  not prove electrical timing, general audio ranges, motor direction/limits or homing.
- Kernel75 backend/HTTPS lifecycle and finite decoded RTSP have bounded native
  passes; host-reviewed RTSP Digest/catalogue/network cores are not native product
  integration. Preserve original failed runs. G3/G8, entropy/trust, durable settings,
  combined media/memory and other product gates remain open.

## History and maintenance

The complete pre-cleanup file is preserved **byte-for-byte** at
`docs/archive/agent-handoffs/20261021-pre-cleanup.md` (historical evidence, not live
instructions). `docs/archive/agent-handoffs/README.md` indexes subsystem references
and records its hash. Read only the relevant history when needed.

Keep this file as a concise entry point: replace the current checkpoint on state
changes and put detailed chronology, exact pins/results and superseded instructions
in linked ledgers/evidence. Do not prepend another accumulating “latest” history.
