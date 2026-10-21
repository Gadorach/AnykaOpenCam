# QC07F project review and completion strategy

Date:2026-10-13. **Review/planning, not a qualification pass.** User requested this
review before continuing implementation. No new build, hardware experiment or
service launch is part of this review. Existing task IDs and gates remain canonical.

## Executive assessment

**User refinement:** perform the [stock reference investigation](STOCK_REFERENCE_CAMPAIGN_20261013.md)
early, before assuming motor/light polarity, factory FPS or Wi-Fi MTU. Reuse exact
retail REA and a controlled root-access stock boot with bounded GPIO tracing where
supported. Root access was previously a rootfs change, not kernel instrumentation.
Suppress unreviewed startup recording/config writes/cloud/actuator effects. Factory
configured values and measured physical/frame/network behavior remain distinct.

Static reviews now confirm why this step matters: the sensor's30 label/pixel-clock
print is not measured factory cadence; board5 uses direct motor GPIO and a potentially
10s decelerating STOP, while the public demo has an incompatible ioctl ABI. Generic
GPIO tracing does not cover the vendor MMIO path and stock tracing availability is
unproven. Factory MTU remains a required runtime scalar. These findings refine R0/R1
admission; they do not authorize guessing a command or enabling normal app startup.

The project has a substantial RAM-tested Linux/capture/codec/network/security
baseline and many useful host-tested shared components. It does **not yet have an
integrated camera product**. The critical work is now native integration, measured
resource ownership and feature acceptance, rather than adding more isolated cores.

There is no defensible completion percentage or fixed completion date: uncertain
memory placement, useful native frame rate, AP capability and retail peripheral/
storage routing can change the implementation effort materially. All product gates
remain open; QC07F stays first, with T34-4G and Fulhan deferred through genuine G8.

### Actual bench now

At13:34:23UTC, after the user powered on, the fresh13a owner recorded the stock
U-Boot banner and a held `anyka$` prompt. No kernel handoff was observed. **Current
state is held U-Boot, not last night's RAM Linux/WLAN.** The catch consumed its
one-shot arm. The receive recorder is running on FTDI A25M86CW with a17:28:33UTC
end bound, which is not proof of a currently armed future-boot catch.

Evidence: `evidence/qualification/qc07f/resume-20261013/poweron-observation-133424021361.json`.
Keep the camera powered and UART connected during review. Before later target
operations, revalidate owner/adapter/actual runtime and arm fresh bounded recovery.
Do not infer available RAM from U-Boot's64MiB string; retain the reviewed40MiB
system/16MiB CMA layout until a separately evidenced decision.

## 1. Evidence-based state

| Area | Established evidence | Missing product result |
|---|---|---|
| Acquisition/recovery/platform | All three stock acquisitions preserved; QC07F exact75 RAM image repeatedly restored/booted with CRC/source/config evidence | Production startup, release packaging, installed/reboot-tested system; upstream-based local port is not an upstream-merged port |
| Capture/H.264/imaging | Actual720p hardware capture/encode and earlier finite lab streams; raw receipt and integrity increments | Qualified encoded renewal/retirement/shared handoff, sustained useful frame rate, colour/day-night calibration and combined load |
| Networking | Actual retained radio256-byte load/APPff, paired station75, WPA2/CCMP/DHCP atMTU384 in prior runtime |2048-byte/full-size USB path, normalMTU1500, robust network lifecycle and actual AP/APSTA/scan capabilities |
| Web authentication | Earlier native HTTPS campaign04 full600k/session/CSRF/revocation/discard-drain/owned shutdown passed | Durable credentials, cold entropy/trust/UTC/rotation, new serialized UI native client/browser acceptance and protected live media |
| New UI/guard | Serialized desktop fixture and TLS assets tested on host; nativeUI01 READY/expiry only; corrected7s guard-only native increment passed; parent recipe121 models pass | UI02 reset remains failed; UI03 was cancelled before target transfer; new full native UI lifecycle is untested |
| RTSP authentication | Independent credential/Digest cores, isolated verifier and first-entry pack parent-reviewed on host; derived isolated-server worker integration snapshot tested | Parent review of integration/adapter changes; three untested fixture edits; strict native2MiB/root-origin lifecycle, positive authentication and protected native producer integration |
| Native browser mux/handoff | Retained hardware H.264 MSE feasibility and shared borrowed-span/fMP4 host fixtures; v6 sealed worker checkpoint | Parent v6 review, safe migration/placement admission, native13-AU/12-output correlation and stop, direct TLS/browser video |
| MMC/recording | MMC command source/object candidate refuses hardware probe; catalogue/index cores host-reviewed; motion v2 parent-reviewed | Native MMC/shared L2/cache/IRQ provider, filesystem/old-footage inventory, actual writer/scheduler, motion source, retention/playback and faults |
| Settings/network transactions | Metadata/private-profile/profile-transition cores parent-reviewed on host | Real protected key/bootstrap/live authority, backing-store durability, native radio/observer integration, first-use/setup/reset workflow |
| PTZ/lights/reset | Bounded PTZ owner and deliberate reset-input logic host-reviewed; historical diagnostic evidence retained | Retail routing/current, physical movement/STOP/limits, independent lighting effects, real button input. Ambient presence unknown; no invented speaker/IR-cut/position feedback |
| Audio | Capture-only candidate and bounded PCM/G.711 helper host/object reviewed | Actual microphone path/clock/bias/L2/IRQ/cache retirement; measured8k mode, native audio client compatibility/A-V synchronization |
| Discovery/NVR/release | Requirement and capability contracts/plans | Actual mDNS/DNS-SD/ONVIF subset, owned Frigate/Blue Iris tests, full product soak/fault/release/install evidence |

Host check totals are recorded in [PAUSED_STATUS.md](PAUSED_STATUS.md) and parent
receipts. They are not feature completion metrics. Current shared regression is
30 C +2 Python +76 receipt cases, not proof of a deployed common service.

## 2. Findings that change execution priorities

### Native media has a concrete admission blocker

Sealed v6 removes one padded1028KiB producer mapping by explicitly reusing raw CPU
storage after the exact kernel75 input copy. This is source-supported and host-modeled,
not native-correctness evidence. The recorded i snapshot has5476KiB above the Normal
zone reserve watermark; raw reuse conservatively requires11548KiB while possible
CMA migration remains unqualified—a6072KiB deficit. This is a **forecast HOLD**, not
an observed OOM or proof the physical design can never fit.

More precisely, dated global availability14756KiB exceeds the v6 raw-reuse global
requirement12636KiB; the failure is Normal-placement forecasting. With2844KiB CPU
debt plus512KiB overhead and1024KiB reserve, allowable future migration would be
at most1096KiB. Security75 requests6160KiB probe-time ISP coherent backing. **If**
same-instance allocator/source/lifetime evidence proves all of that is occupied
nonmigratable CMA, remaining unknown occupancy could be1008KiB and the illustrative
Normal requirement5388KiB—only88KiB below the dated5476KiB allowance. This is a
hypothesis to investigate, not permission to subtract requested sizes. V6 currently
makes no such subtraction. Contiguity, later forecast points, transient overhead
and native high-water remain separate tests. A thin hypothetical pass is not a
product memory budget for simultaneous TLS, RTSP, recording and audio.

Resolution order: audit exact allocation ownership/placement and release lifetimes;
obtain a defensible fresh bound for genuinely movable CMA occupants; recalculate
before any prefault/sensor/codec activity. Do not count freeCMA twice, assume ISP
occupants nonmovable from an address alone, reduce reserve, manipulate watermarks,
or allocate merely hoping reclaim succeeds. If proof cannot establish fit, make
one scoped backing/allocation redesign with explicit ABI/lifetime review and
regressions. Do not continue adding services to a failing budget.

### Functional security and new UI acceptance must be separated

Campaign04 proved earlier native auth/lifecycle, not the newer UI image or cold
provisioning. UI02 reset was real. HOST reproduced a plausible fork/pre-exec guard
hazard; the later native guard-only result validates one corrected normal sequence,
not its combined-load schedulability or the exact cause of UI02. Use the corrected
sequence for a **new, bounded, matched UI campaign** with source-bound receipts.
Unused PC UI03 credentials are not current target state; validate their lifecycle/
trust before use and keep them private. No repeated infrastructure redesign.

### Three hardware dependencies cut across multiple features

1. **Shared L2/DMA/cache/IRQ ownership** affects MMC/audio/UART and concurrent video.
   Establish one measured ownership model and standard driver adapters; no subsystem
   privately resets shared hardware to make its own test pass.
2. **Persistent storage + device-key/entropy authority** blocks real settings,
   first-use/reset, private profiles and recording. Host fsync/AEAD proofs do not
   establish physical persistence or rollback-resistant checkpoints. Define the
   attainable threat/rollback model explicitly; lacking an authority is a blocker,
   not permission to call cached counters rollback-resistant.
3. **Actual AP support** is essential to required setup/reset. Perform focused
   source feasibility early, then bounded native qualification when resources are
   ready. If the fitted radio cannot support it, document the precise limitation
   and bring the hardware/requirement decision to the user; station-only is not
   completion and a captive-portal UI cannot compensate for a missing AP.

### Documentation and candidate promotion need tightening

The master index contains extensive prepend-only history; framework README and
target inventory include dated “current” fields. Consult original receipts when
they disagree. This dated review becomes the concise planning entry, keeping old
bytes/history available. For each candidate record one tested source/ELF/ABI plus
its adoption state. Isolated-server `integration05` does not cover its three later
fixture edits. Prefer targeted fixes in existing components and controlled adoption
over more parallel derivative services or another reporting/guardian framework.

## 3. Ordered delivery strategy

| Stage / canonical tasks | Deliverable and dependencies | Exit evidence / stop condition |
|---|---|---|
| **A. Restore and freeze a repeatable bench** BASE-01–06, PLAT-01/02/08 | Fresh catch/protection, exact75 RAM restore, checked boot/source/ABI, original256 radio/paired75 station/authorized guest; one active source manifest | Actual boot/idle/UART/watchdog/network receipts. Any pin/CRC/ownership mismatch stops entry; no guessed retained image or2048 replay |
| **B. Close security/control-plane integration** SEC-03–06/11/12, WEB-03–05/10, STREAM-04/07 | Fresh serialized UI assets+login/session/CSRF/revoke/logout/shutdown campaign; separately strict native verifier READY15/2MiB/retirement; review/build RTSP tail and local overlay | Real pinned TLS assets and desktop/mobile entry, native positive/negative auth and actual child/service cleanup. Keep media501 until a qualified source exists |
| **C. Admit and qualify the shared producer** CAP/ENC, PLAT-07, STREAM-01/02/08 | Resolve v6 budget/source alias contract first. Finite13 produced/12 output TCP measurement with full raw/NAL/old-hub CRCs, true timestamps and normal END/QUERY/ACK | Native integrity/source correlation + independent decode + measured memory before/after allocation/idle. Unknown lifetime or failing forecast is HOLD, not a longer timeout |
| **D. First end-to-end protected viewing** WEB-01/02/05, STREAM-02–09, SEC-04/05 | Same admitted producer/hub feeds authenticated RTSP and direct on-device HTTPS fMP4/MSE, bounded borrowed spans and fair consumers | Real fresh frames, both preview axes, per-effect auth/expiry/revoke/slow-reader cleanup, desktop/mobile browser plus independent RTSP client; no PC video bridge |
| **E. Performance/imaging and transport closure** IMG, CAP, ENC, NET-01–05/08, QUAL-02 | Measure capture/codec/CRC/copy/TLS/IPC/USB costs; optimize actual bottleneck with unchanged integrity; qualify cadence/colour/day-night/bitrate/rate control and MTU path | Sustained unique decoded720p15 engineering target, latency and measured reserve under declared load. Functional low-FPS demo does not close G2/G3; infeasibility requires explicit reviewed decision |
| **F. Physical controls/audio and storage foundation** CTRL, AUDIO-01–03, PLAT-04, STORE-01–04 | Routing/bias/current/clock/shared-L2 qualification; physical bounded PTZ STOP and independent lights/input; capture-only audio; read-only MMC/inventory before owned writes | Actual electrical/physical effects, DMA/IRQ/lease retirement, native PCM and card bounds/removal. No speaker/ambient/absolute feedback assumptions; never format existing SD |
| **G. Recording and persistent setup** STORE-05–12, SEC-07–10, SETUP, WEB-07–09 | One encoded segment writer + catalogue; native luma source + masks/events; continuous/event scheduling, retention and authorized playback. Actual config/key store + rollback/WLAN/AP/captive portal/deliberate settings reset | Owned closed segments decode/recover; originals untouched; interruption/full-card/removal; settings survive reboot; wrong WLAN rolls back; reset preserves footage/firmware and returns protected setup |
| **H. Discovery/NVR and product acceptance** NVR-01–11, AUDIO-04–06, QUAL-01–10 | Truthful mDNS/DNS-SD/ONVIF/profile/control/time/RTCP behavior, measured audio format/browser choice, actual Frigate and Blue Iris versions; combined service packaging | Native NVR+browser+recorder+audio/control fault matrix, staged1h/24h soak and lifecycle campaign, reproducible release/recovery. Persistent installation separately scoped to exact image/ranges/boot policy/authorization |

Stages are dependency-driven, not blanket subsystem locks. Storage/shared-resource
and AP feasibility investigations should run alongside B–E to expose blockers early;
their hardware experiments remain serialized and cannot compromise media recovery.
G4–G7 are not closed before the preceding streaming foundation is accepted. G8
requires all mandatory features; only then begin the two deferred hardware ports.

### First implementation batch after this review

1. Complete the source audit for a bounded **stock reference** window from held
   U-Boot: motor/light command tracing, factory video cadence and actual Wi-Fi MTU.
   Qualify startup/output/logging scope and fresh recovery before any stock boot.
   Then restore/seal the75 RAM baseline for replacement implementation.
2. Parent-review sealed v6 and RTSP tested-vs-untested diffs; rerun only the reviewed
   new candidate/test tail, not every historical suite because the session resumed.
3. Run the small native UI/control-plane campaign independently of video memory.
4. Resolve the media placement bound. If HOLD persists, return the exact deficit
   and required allocator/provider change before scheduling capture.
5. In independent source review, produce concrete shared-resource and AP feasibility
   decisions—not new speculative feature cores. Promote one integration slice at a time.

Stock MTU384 is not established;384 is the replacement workaround. If factory
runtime confirms1500, reproduce its packetization/USB behavior and fix the actual
replacement path before changing the qualified default. Existing footage/config
must be protected during stock reference work; see the dedicated campaign above.

## 4. Execution controls that keep the strategy deliverable

- Limit active work to **one parent-owned hardware campaign and at most two
  independent source lanes**. Delegate disjoint integration/review work with precise
  source ownership; parent reviews before adoption. No worker operates hardware.
- Prefer finishing a vertical slice over branching into another unconnected core.
  Source work only advances a named next integration or removes a measured blocker.
- Every campaign has one question, exact inputs/source pins, bounded resources,
  observables, stop conditions and a recovery path. Freeze a passed increment and
  move on; repeat only for changed code, failure or unresolved acceptance coverage.
- Keep raw failures and corrected receipts. Logs count as evidence, not executable
  instructions. Completion requires real effect, current policy, source integrity
  and actual retirement—not accepted requests, signals or PID existence alone.
- Never trade away full integrity/ownership checks, native2MiB locking, CPU/time
  budgets or explicit reserve to pass a feature. Measure cost first; change a design
only with evidence and a documented decision.

### 15fps requires a measured pipeline change

`docs/qualification-plans/qc07f-performance/COST-AND-MEMORY.md` records historical
kernel73 CRC_PRE+POST averages49.705+49.496=99.201ms/frame, already longer than
15fps's66.667ms period. Encoder checks/copies/wait add serialized cost. The current
pipeline cannot simply be relabelled15fps by changing timestamp, GOP or scheduling
constants. Retain full integrity checks; evaluate evidence-supported ownership,
copy and scheduling improvements. If those cannot meet the target under real load,
present the measured limit for an explicit user decision. Do not infer the chip's
maximum from this diagnostic pipeline or promise15fps before measuring it.

### Sustained product lifecycles exceed the finite fixture examples

Reviewed primitives have deliberate finite bounds:64 catalogue index entries,
two retained private profiles, eight metadata receipts and one transition episode
per adapter context. They demonstrate bounded correctness, not indefinite recording
or repeated product reconfiguration. Define capacity handling, restart reconciliation,
safe catalogue paging/rotation and subsequent transaction ownership as part of G/G8
delivery. No widening deletion scope, arbitrary pruning, credential reuse or silent
ceiling increases to handle exhaustion.
- Keep everything local/uncommitted/unpublished. Persistent writes require a
  specific image/ranges/storage scope and recovery, not inference from a build.

## 5. External inputs to obtain when their stages become eligible

- Access for identified retail routing/bias/current/STOP measurements, with a
  concrete measurement procedure before asking for physical manipulation.
- Owned SD/card access or a separate authorized test medium if read-only inventory
  and disposable filesystem-fault tests cannot safely share the existing footage card.
- Owned Frigate and licensed Blue Iris hosts/versions and desktop/mobile browsers
  for real integration. Do not scan unrelated networks or substitute mocks.
- An explicit decision if measured15fps, required AP or durable trust/rollback
  authority proves infeasible on the fitted hardware. No silent requirement reduction.

## Review provenance

Canonical requirement: `docs/CAMERA_PRODUCT_ACCEPTANCE.md`; subsystem plans01–16.
Historical checkpoint: `PAUSED_STATUS.md`; latest actual state: `RESUME_20261013.md`.
Public report identity checks: `evidence/qualification/qc07f/resume-20261013/state-review-inputs01.json`.
Independent read-only reviews completed in sessions `ses_eca865f0fffenu5r6b2FV5tU3b`
(media/memory/performance) and `ses_eca865f04ffeXdVnSwrNPYFUtG` (hardware/product
dependencies). Their findings are incorporated above. They performed no code
changes, tests or target operations. This strategy creates
no new acceptance claim and leaves all product gates open.
