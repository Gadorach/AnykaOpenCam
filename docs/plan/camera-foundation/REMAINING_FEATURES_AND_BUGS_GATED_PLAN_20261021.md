# Remaining features and bugs — gated completion plan

Date: **2026-10-21**. Status: **PLANNING / FOR REVIEW; NOT AN EXECUTION QUEUE**.
Target order: **QC07F foundation → G8 → T34-4G/G9 → Fulhan/G10**.
Entry: [master index](00-INDEX.md). Requirements: [product acceptance](../../CAMERA_PRODUCT_ACCEPTANCE.md).

**Subsequent user approval:** execution is active; follow
[EXECUTION_TASKS_20261021.md](EXECUTION_TASKS_20261021.md). The original planning
boundary is retained history. Effect/privacy/current-state gates remain in force.
User now requires commits on main and chose local commits until a remote is supplied;
this supersedes earlier no-commit wording, not the private-data/publication boundary.

This consolidates the remaining work from plans01–18, the stock-reference closeout,
kernel76 and retained candidate reviews. Existing subsystem IDs and G0–G10 meanings
remain authoritative. `RP-*` below are integration work packets, not replacement
task numbering. Existing successes are reused at their actual scope; no subsystem
or product gate is closed merely by writing this plan.

**This request authorizes planning only.** No new build, target query, capture,
boot, radio operation, actuator action, mount, reset, card write/deletion or flash
change is performed or queued by this file. Execution requires a subsequent user
instruction and the applicable current-state/effect gates below.

**Navigation:** [baseline](#1-baseline-and-evidence-boundaries) ·
[entry/effect gates](#2-mandatory-gates-for-every-work-packet) ·
[G0–G10](#3-canonical-product-gates-and-order) ·
[bug register](#4-known-bugs-limitations-and-corrected-regressions) ·
[work packets](#5-dependency-ordered-implementation-packets) ·
[priority/decisions](#6-priority-schedule-and-hard-decisions) ·
[test matrix](#7-required-test-matrix-and-stop-rules) ·
[task coverage](#8-canonical-task-coverage-and-handoff).

## 1. Baseline and evidence boundaries

### 1.1 Last actual target evidence — October20, not current October21 state

- Candidate76 Linux6.18.55 had one exact RAM upload/CRC/boot with zero frame retries.
  The corrected, independently reviewed read-only idle checker passed on target.
- UID0/PID1init, exact bootargs/release, stable private boot UUID, admitted volatile
  mounts, no loadable modules, idle ISP/guards/queues, logical watchdog inactive,
  and cached UART error/busy counters0 were observed.
- Measured board RAM remains40MiB; CMA reservation16MiB is **inside** that RAM.
  The dated idle snapshot was MemTotal33144KiB, MemFree21008KiB, CmaFree9216KiB.
  Do not add CMA free to MemFree, treat the whole16MiB as free, or use idle values
  as proof of combined-load headroom.
- Last observed runtime was the **idle RAM76 monitor**, not stock3.4 SSH, RAM75,
  an authenticated product service, working WLAN or active video.
- The Oct20 capture deadline **2026-10-20T08:02:56Z is expired historical authority**.
  Current power, runtime, adapter owner, socket/finalizer state, address and recovery
  protection have **not** been inspected for this planning request. Do not infer
  that an old process is running, stopped, finalized or safe to replace.

Native evidence: [RAM76 boot/idle result](../../qualification-plans/kernel76-integration-20261020/RAM_BOOT20C.md).
Artifact: `QC6U76.UIM`,3603555B, SHA256
`bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f`.
Keep kernel75 rollback and immutable original acquisitions. Linux remains pinned
at6.18.55/725bd2f3…; an upstream-version upgrade needs separate regression, not an
incidental rebase while debugging performance.

### 1.2 What is already learned — do not repeat discovery without a changed question

| Existing result | What remains, rather than rediscovering it |
|---|---|
| Stock MTU1500/2048 radio download and finite traffic worked | Qualify our kernel76 transport fix, normal MTU and loaded reliability |
| Exact stock SD group13 mux and board5 GPIO47 pulse recovered | Physical power/CD/WP, initialized pdata/clock and modern shared-resource/retirement implementation |
| GPIO0 active-low button taps physically correlated | Our debounce/held/stuck handling and settings-only reset policy; not a hardware hang-recovery button |
| White GPIO5, IR GPIO23, IR-cut6/7 routing established | Modern76 pairing, lifecycle/coexistence, calibrated policy and measured deadlines |
| User observed A/GPIO6 OUT/IR-pass and B/GPIO7 IN/IR-block | Broader repeatability/electrical/spectral qualification if required; no automatic LED/exposure policy implied |
| Stock microphone scalar clap response and audible5s speaker confirmed | Modern ADC/DAC/ALSA implementation, rates/quality/lifetimes and permitted product audio integration |
| Persistent stock SSH-only ROOT installation cold-accepted | Not an installed modern firmware; preserve its password, keys and non-ROOT contents |
| RAM75 finite RTSP decode and HTTPS campaign04 passed narrowly | Native76 revalidation and protected integrated media, not another library-selection exercise |
| RAM76 embeds freshly paired radio/current RTSP/ELF DHCP bridge | Actual module/network/media use and release-wide pairing enforcement, not bare vermagic matching |

Stock is a hardware reference, not an implementation to transplant. Unknown stock
FPS does not block profiling our pipeline. No full stock app/cloud/record/homing
startup or broad legacy-ABI/trace reconstruction is a prerequisite to our product.

### 1.3 Status vocabulary

`OPEN` = implementation/integration absent; `SOURCE/HOST` = bounded reviewed code
or fixtures only; `BUILD` = ARM compiled/linked; `NATIVE-NARROW` = specified target
increment only; `BLOCKED` = named prerequisite absent; `ACCEPTED` = the defined
product gate actually passed. A source correction can be closed while its native
integration remains open. Never copy an old image's acceptance flags into76.

All product gates G1–G8 remain **unaccepted**. RAM76's K76-07 is NATIVE-NARROW;
K76-06 full-size transport is GATED-NOT-EXECUTED. G0 operational admission must be
renewed for every hardware campaign even where historical baseline proofs exist.

## 2. Mandatory gates for every work packet

| Gate | Evidence required before advancing | Failure/rollback rule |
|---|---|---|
| E0 — scope/source | Named requirement, exact source/input pins, existing evidence, permitted effects, dependencies and acceptance thresholds | Missing physical fact blocks that row; do not guess GPIO/clock/ABI values or stall independent work |
| E1 — host/build | Parent source review; meaningful negative/fault tests; clean exact ARM build, config/DT/generated metadata/UAPI/module manifest | Preserve failed/current-vs-tested source differences; no assertion-disabling Python, forced module load or synthetic-success native stub |
| E2 — native admission | Fresh physical state, sole adapter/peer/PID/start/source, actual image/runtime, no competing transfer, private capture, bounded recovery, exact operation budget and owned rollback | Old deadlines/IPs/epochs are invalid. Preserve unexplained socket/logs. Reader/module open can have effects; audit before calling it passive |
| E3 — finite native result | Actual successful operation and negative cases; precise completion/retirement; same generation/clock/owner and bounded memory/latency | Unknown/late/partial execution retains HOLD and backing; no blind repeat, free/unload/kill repair or later healthy-status fault clearing |
| E4 — integration/acceptance | Same deployed producer/services under coupled load, client/fault/security tests, stable resource trends and user-visible results | A local fixture, READY/login, module load, IRQ count or one decode cannot substitute for an overall gate |

Parent alone operates hardware. At most two disjoint source lanes may run with
parent review; no concurrent UART sender or status probe during an exclusive
transfer/operator GUI epoch. Use existing small ownership/manifest mechanisms;
do not create another generic guardian, trace or ABI framework.

Every native packet records commands/tool versions/input and output pins, actual
runtime/owner/time, permitted effects, budgets, observations versus inference,
failed cases, post-state, remaining blockers and **one next task**. Private UART,
credentials, config, device IDs, acquisitions and footage stay outside Git. Local
Git is not publication permission; no commits/pushes/publication requested here.

### Explicit effect and privacy holds

- No GPIO2 experiment; it conflicts with UART. GPIO0 stays input. Speaker/GPIO1
  effects require explicit physical isolation of **both** UART signal leads and
  a qualified alternate control/recovery handoff; disconnected UART is not recovery.
- Live homing/full travel, AP broadcast/new SSID, original-card content access,
  card writes/removal/power faults, settings reset, retention deletion and flash/
  ENV/MAC/OTP actions require their specifically reviewed scope before execution.
- Current bench network scope is VertexHotspot and owned endpoints. A setup-AP
  test needs a separately agreed owned SSID/channel/client; do not impersonate
  VertexHotspot, alter routers/accounts or intercept ordinary-LAN HTTPS.
- Microphone work remains **scalar-only**: no raw voice, recordings, audio hashes
  or voice playback. Older AUDIO-07 recording instructions do not override this.
  Product audio tracks/quality acceptance remain planned but privacy-gated; obtain
  an explicit evidence policy before any live microphone-stream/recording trial.
- Default optical qualification uses ephemeral decode/scalar measurements. Any
  retained calibration imagery or recording needs an explicit private destination,
  purpose, access and retention scope. Never save existing footage to prove a pin.

## 3. Canonical product gates and order

Phase numbers are not gate numbers. Dependencies permit early source/feasibility
work, not early acceptance of later mandatory product stages.

| Gate | Exit condition | Work packets / dependencies |
|---|---|---|
| G0 | Repeatable baseline/build/evidence and current safe recovery/no transfer conflict | RP-00/01; bootstrap builds are allowed to establish G0, final release reproducibility is a later obligation |
| G1 | Usable unclipped colour/exposure, real controls, consistent orientation and truthful day/night | RP-03/06, limited RP-11 lighting; rerun after capture migration |
| G2 | Standard capture/encoder ownership, justified full memory budget, sustained native H.264 and measured transport | RP-02/04/05/06; target720p≥15 unique delivered/decoded fps,30 only if supported |
| G3 | Independently authenticated RTSP and direct on-device browser media, multi-client/slow-peer/reconnect correctness | G2 + RP-07/08/09; login or NOAUTH lab preview is insufficient |
| G4 | Actual safe PTZ/STOP, independent fitted lights, truthful ambient/reset input and qualified microphone path | G3 + RP-10/11/12; full audio tracks continue with media/store/NVR gates |
| G5 | Actual continuous and motion recording/masks, both retention modes, preserved imports and protected playback under SD faults | RP-12/13/14 + qualified source/services/security; product delivery follows streaming/peripherals |
| G6 | Real protected AP/onboarding/captive steering, durable settings, recoverable network transactions and settings-only reset | RP-08/15 + qualified store/radio/reset/UI; station-only/UART-only is not a substitute |
| G7 | Truthful DNS/discovery and actual Frigate **and** Blue Iris workflows/control | RP-16 + protected media, configuration, qualified controls and required tracks |
| G8 | G0–G7, combined soak/fault/security/resource acceptance, reproducible QC07F release/recovery and same-service dual-topology fixture | RP-17; no mandatory subsystem disabled/stubbed to pass |
| G9 | Same services on T34; both real sensors and all required fitted features qualified | G8 → RP-19/T34; no QC07F register/module assumptions |
| G10 | Fulhan acceptance plus all-three shared-service regressions | G8/G9 → RP-20/FH; only then consider `all-required-modules=true` |

G8 may be a reproducible RAM-qualified software release if persistent firmware
installation is not authorized. **Durable settings/first-use/reset still need real
proof on approved storage/test configuration.** RP-18 installation is a separate
exact-image/range/backup/recovery/authorization gate, not implicit in G8 or boot76.

### Targets to freeze before campaigns, not claims of achieved capability

| Area | Target / decision required |
|---|---|
| Video | Existing engineering goal720p≥15 unique delivered/decoded fps; factory15/30 remains unmeasured. If infeasible, present a measured ceiling and user-reviewed scope decision; never silently lower resolution/quality or use software entropy |
| Timing | Timestamp source/clock relation, startup-inclusive throughput plus `(N−1)/(t_last−t_first)`, interval distribution, duplicates/drops and stage costs. Do not benchmark the optical GUI's≤5fps selector |
| Imaging | Proposed neutral-channel deviation≤5% on a declared unclipped reference/colour space; freeze highlight clipping, noise, convergence and flicker tolerances before G1. Clipped grey/white patches cannot establish gain accuracy |
| Reliability/soak | Proposed1h then24h combined soak;≥100 meaningful lifecycle cycles; zero unexplained corruption/ownership leaks/healthy-client disconnects |
| Latency | Proposed p95 live latency<1s on declared LAN/profile; report p99/max and outliers. Propose PTZ STOP≤250ms or tighter proven bound; electrical/mechanical deadline needs measurement |
| Audio | Proposed A/V≤150ms without growing drift, subject to privacy/evidence approval and measured hardware clock; requested8k/calculated8012 is not a clock measurement |
| Capacity | Freeze bitrate/GOP/peak AU, browser+NVR+recorder/audio/motion mix, client count, protected process limits and positive CPU/RAM/CMA/log reserve before load tests |
| Clients | Declare real desktop/mobile browsers and Frigate/Blue Iris versions/endpoints; missing owned endpoints/access is a blocker, not a simulated-client pass |
| Storage/time | Identify expendable card, allowed byte/filesystem namespace, durability/reset/deletion scope, timezone/date-window semantics and invalid-UTC behavior |

## 4. Known bugs, limitations and corrected regressions

This is the defect register. Entries marked corrected are **not** fresh defect
reports; preserve them as regression obligations and do not repeat hardware effects
merely to rediscover the original observation.

| ID | Status / problem | Required closure / packet |
|---|---|---|
| B01 | Kernel76 Anyka continuation/NAK MODE fix source/host/ARM PASS; large-transfer root cause not natively proved | Finite guarded2048-payload/2052-wire test, actual lengths/status/ACK/CRC/toggles; RP-02 |
| B02 | Modern MTU384 workaround; previous full-size transfer timed out/FIFO warning | Preserve256/384 rollback, independently qualify1500 before changing default; RP-02 |
| B03 | Real TCP stalls exceeded500ms assumptions; prior2s cutoff failed after396 complete frames | Layer HCD/RF/CPU/peer evidence and bound slow consumers, not blanket timeout growth; RP-02/09 |
| B04 | Gated capture, full-frame CRC/cache/copies/reset/renewal serialize ~2fps path | Measured timeline, owned continuous queue/import/overlap; no guard deletion; RP-03/04/05 |
| B05 | Four-slot capture+codec allocation exceeds16MiB CMA; two-slot/sink fit unproved | Whole live ledger and selected-slot/starvation/retirement proof; RP-04/05 |
| B06 | Media-handoff v6 dated placement forecast5476KiB versus11548KiB needed; raw-reuse integration unadopted | Review exact tested/current source, overlay/copy/lease contract and actual native placement before allocation; RP-04/09 |
| B07 | AUs exceed128KiB allocation ceiling; protected process2MiB limits conflict with naive raw/hub placement | Bounded headers/borrowed spans, real release/drain and native TLS headroom; do not raise ceilings or copy full AU per client; RP-09 |
| B08 | Isolated RTSP integration05 tested source has three later unbuilt/untested fixtures; BAD-DOMAIN test actually covered generation mismatch | Review tail, explicit foreign-domain negative case, rebuild/retest and parent adoption; RP-08/09 |
| B09 | UI02 actually reset under watchdog; reproduced guard hazard is not measured cause | Re-admit exact minimal UI campaign, preserve failure and causal uncertainty, prove complete shutdown under native load; RP-01/09 |
| B10 | Expired GUI could not close; pre-TX refusal and delayed Tk reply defects corrected in71 HOST mocks | One fresh no-auto-actuation native expiry/Close trial on exact76 pairing; true post-TX/unknown HOLD remains sticky; RP-11 |
| B11 | Old UART1023-byte truncation; minimal BusyBox lacks id/cut/scripts and WPA rejects -f/update_config | Keep reviewed line limits/pacing, procfs identity, supported flags and ELF DHCP route; loaded serial/lifecycle tests; RP-01/02 |
| B12 | Polling UART idle counters do not prove loaded reliability or error recovery | Production serial-core/IRQ/close/termios plus codec+USB+logging qualification; RP-01/17 |
| B13 | Retail reset seek conflicts with software limits; no defensible absolute motor origin | No copied/automatic homing; unknown/manual origin, bounded relative motion; explicit mechanical proposal if needed; RP-11 |
| B14 | PTZ postfault descriptor release/coil state and global STOP/queue semantics incomplete | Preserve unknown-state ownership, revoke all queued work, verify coil-off/physical stop; RP-11 |
| B15 | GPIO2 loses UART; GPIO1 speaker conflicts with connected UART | Exclusion/interlock/recovery modes remain enforced; no sweep or simultaneous-use fiction; RP-01/11/12 |
| B16 | Light/pulse timers nominal only; physical cutoff/release/coexistence not fully measured | Pair00, holders/deadlines/duty and loaded/fault tests with independent observation; RP-11 |
| B17 | L2 whole/fraction starts can clear other requests/UART bits; private client locks insufficient | One coordinated provider, preserved UART/shared RMW and bounded IRQ/callback retirement; RP-12 |
| B18 | Legacy MCI map-count/cache/stop/free/error paths unsafe to copy; modern MMC deliberately refuses | Implement correct DMA/SG/tail and removal lifetime; read-only native host before writes; RP-12/13 |
| B19 | Family ADC OSR/divider model differs from actual retail; ADC/DAC close changes shared clocks/analogue state | Exact provider calculations, refcounted/masked resources and real stop/rate validation; RP-12 |
| B20 | Required protected AP missing; station75/76 and beacon-only prototype are not secure APs | Exact role/client/key/PN/PS/TSF contract and real authenticated client; firmware impossibility is not established; RP-02/15 |
| B21 | Stock init=/bin/sh attempt reached RO root then garbled console/no verified shell | Hold reference-only route; no auto stock retry, password guess or reset; not a blocker to modern profiling; RP-00 |
| B22 | Stale embedded radio/RTSP corrected in76; release-wide compatibility still open | Manifest enforcement, selected modules and fresh control/GUI pins, not just vermagic; RP-01/11 |
| B23 | Initial boot-postchecker HOLD/TX/parser defects corrected,151 HOST+19 review checks and native20c PASS | Keep exact cumulative TX/durable HOLD/unique positive schemas as regression; not an unresolved native failure; RP-00/17 |
| B24 | Motion post-callback authority and network/private-profile close/commit defects corrected in v2 SOURCE/HOST | Adopt corrected versions, not the older HOLD or a claim of native durability; RP-07/08/14/15 |
| B25 | Catalogue overlap/reservation defect corrected in reviewed v2; no actual writer/player/durability proof | Integrate actual segments/leases/fsync/fault handling; RP-13/14 |
| B26 | Finite prototype capacities:64 catalogue items,2 private profiles,8 receipts,1 transition episode | Bounded paging/rotation/exhaustion/restart reconciliation and subsequent-operation ownership; no silent ceiling increases; RP-08/13/15 |
| B27 | Cold entropy/trust, durable authority/high-water and valid UTC unqualified | Real bootstrap/storage/threat-model evidence; do not turn warm RAM auth into durable security; RP-08/15/17 |
| B28 | RAM76 image3603555B exceeds existing1572864B KERNEL partition | Design and measure release fit/layout; no truncation, silent repartition, assumed A/B or in-place flash; RP-18 |
| B29 | Retained desktop-browser feasibility included a reported renderer drop; native/mobile behavior unqualified | Preserve that observation, measure render versus source/network/decoder drops and actual client latency; no zero-drop claim from intact NALs alone; RP-09/17 |

## 5. Dependency-ordered implementation packets

All packets use E0–E4. An exit below is an additional concrete gate, not permission
to skip an earlier gate. Product delivery order remains G0→G1/G2→G3→G4→G5→G6→G7→G8.
Source-only CORE/security/AP/shared-L2/durability feasibility may overlap early.

### RP-00 — reconcile state and freeze admission (P0)

**Canonical:** BASE-01–07; QUAL-01. **Current:** historical narrow proofs only;
fresh October21 hardware admission BLOCKED/not attempted.

- Reconcile evidence dates, current versus tested sources and historical OPEN/PASS
  prose; latest native76 boot supersedes old “not booted,” without rewriting old
  manifests. Identify whether retired/expired captures finalized; preserve surprises.
- Freeze76 image/config/DT/initramfs/module/UAPI/recovery and75 rollback plus exact
  private transport source, credential/privacy boundaries and bounded diagnostics.
- Before hardware: user physical state, intended adapter, sole peer/PID/start,
  source/argv/lifetime, no pending operation and fresh independent recovery. Reuse
  existing tested boot transport; no general transport/guardian rewrite.
- **Exit:** one auditable current admission/rollback record with exact scope and
  stop conditions; no unsafe socket takeover or assumption of WLAN/SSH/feed.
- **Stop:** unknown operation remains HOLD; physical OFF/fresh cold handoff when
  required. GPIO0 is not hang recovery. Missing physical access blocks native rows.

### RP-01 — reproducible paired platform and production supervision (P0/P1)

**Canonical:** PLAT-01–09. **Depends:** RP-00 bootstrap admission; RP-04 budget for
new runtime services. **Current:** substantial build/basic-boot evidence, product
PID1/serial/supervision/release policy OPEN.

- Enforce one selected release manifest across kernel/config/generated metadata/
  DT/initramfs/modules/tools/firmware/calibration/schema; refuse stale paths. Keep
  the76 archive replacement and exact final radio pairing, licenses and snapshots.
- Migrate qualification-monitor startup to small explicit product service
  supervision: RO base, bounded RAM dirs/logs, deterministic dependencies, rescue
  entry, actual interpreter/syscall support, and no accidental radio/media start.
- Replace or qualify polling UART with applicable serial-core patterns: RX/TX,
  IRQ/termios/hangup/errors and modem policy; preserve paced rescue and prove loaded
  byte/error behavior. Do not spin on read-only MEM_RDY or erase errors as recovery.
- Managed DT/resource ownership for clock/reset/IRQ/timers/pinctrl/I²C/GPIO/USB and
  later MMC/audio; preserve unrelated fields and independent users on every failure.
- Integrate one progress-based watchdog owner with startup/runtime separation,
  bounded restart/backoff and reboot-loop escape; no unconditional feeder or new
  speculative guardian hierarchy. Retain DMA/coil uncertainty rather than free it.
- **Exit:** clean complete release reproduces from declared inputs; native service
  start/stop/error and loaded UART tests pass with positive resource reserve.
  UI02 re-entry must have an exact scoped lifecycle proof; its old reset remains.
- **Hold:** final storage/image layout/update safety goes through RP-18. Basic76
  monitor boot does not close PLAT-05/06 or authorize persistent installation.

### RP-02 — USB/radio/normal-MTU and AP feasibility (P0/P1)

**Canonical:** NET-01–08; K76-06. **Depends:** RP-00/01; later combined tests use
RP-04/09. **Current:** byte-kick implemented/reviewed/built, not natively qualified.

- Review the existing guarded radio RAM-loader entry before selecting any2048
  operation. Preserve no-OTA/flash/OTP and application-state/CRC/sequence checks;
  do not delete a256-byte refusal or replay RUN after uncertainty.
- Discriminate the MODE hypothesis at2048-payload/2052-wire: exact lengths/status,
  maxpacket boundaries, tail versus ZLP, ACK/CRC, normal toggles and clean retirement.
  Use owned bounded metadata, not read-clear status polling or per-packet printk.
- Review/measure NAK/WZC acknowledgements, flush/abort/reset, short/zero/error paths,
  disconnect/reconnect, RX bounds and actual credit/queue/default power policy.
- Paired station76 on the approved AP/owned host:384 baseline then explicit finite
 1500 pattern/TCP both directions; DHCP/renewal/multicast and later real media.
  Separate slow peer from HCD/RF/CPU stalls; keep recovery384 until combined pass.
- Early source feasibility lane for required secure AP: exact8400 role/index,
  beacon/TSF/TBTT, AID/peer lifetime, PTK/GTK/PN/replay/rekey, TIM/DTIM/PS and frame
  transport. Current refusals remain; no broadcast is authorized here.
- **Exit:** evidence-supported HCD outcome, native payload integrity/error/lifetime
  matrix and accepted network profile under G2/G3 load. A negative fix trial is
  retained, then narrow further in our driver; no broad stock archaeology.
- **Hold:** unknown AP firmware contract blocks G6, not source work elsewhere;
  present measured alternatives/user decision if infeasible, never “STA is enough.”

### RP-03 — measured frame-time and safe image baseline (P1)

**Canonical:** IMG-01/02/03, ENC-07; links RP-06. **Depends:** fresh exact producers
and RP-00/01; transport measured separately by RP-02.

- Freeze lighting/reference/exposure/gain/profile and use unclipped regions. Current
  purple/red bias and clipped ceiling/grey patch are not calibration measurements.
- Measure sensor completion, capture gating/wait, copies/cache/CRC/guards,
  allocation/reset/program, ASIC wait, scheduling/send/TLS independently. Preserve
  nested timing boundaries; do not sum nested ALLOC/PREPARE twice.
- Historical kernel CRC pair≈99ms and ASIC wait≈31ms are evidence of costs, not
  current76 maxima or proof that sensor/encoder silicon is limited to~2fps.
- Compare unique source/encoded/decoded frames, startup and renewals with an
  uncapped measurement consumer. Stored15/cached30 and sensor “pixel-clock” computed
  from30 are not factory cadence. Optional stock cadence remains HOLD unless a
  safe minimal route exists; it is not on the critical path to modern profiling.
- **Exit:** controlled native timeline/quality baseline, exact control units or
  explicitly blocked units, and a measured ranked bottleneck list with one next
  optimization. Freeze the15fps engineering target or submit feasibility review.

### RP-04 — complete memory/backing/retirement decision (P0/P1)

**Canonical:** CAP-01/02/03/06/07; PLAT-07; CORE-04. **Depends:** exact current
native allocation inventory, not just idle counters. **Current:** BLOCKED for
unproved combined continuous capture/codec/TLS placement.

- One live-owner ledger: CMA pages/alignment/guard extents, ISP references/table/
  statistics, capture slots/sinks, codec input/output/two refs/aux, exported/imported
  references, raw CPU buffer, producer output+hub, TLS/libc/stacks, sockets/skbuffs,
  audio, recorder, pagecache, snapshots, logs and locked mappings. Count each once.
- Four-slot+codec20.125MiB and imported variant18.125MiB exceed16MiB. Two-slot+
  import14.125MiB is arithmetic; an extra2MiB sink exceeds16MiB before other users.
  Do not remove refs/guards, assume migration/reclaim, or raise CMA to force a pass.
- Resolve v6 exact tested/current raw-reuse/overlay/copy contract and the historical
 5476-versus11548KiB placement shortfall. RAM76 idle CMA/free cannot dismiss it.
- Review two-slot enabled masks, inactive destinations, starvation/no-buffer path
  and selected-slot retirement. If real hardware needs more buffers, revise the
  design explicitly. Separate DMA capacity from per-process locked/address limits.
- **Exit:** defensible layout fits declared load plus reserve; finite native
  allocate/retain/return/stop/error tests show no alias/corruption/UAF and bounded
  repeated allocations under fragmentation. Failed retirement retains backing.
- **Stop:** unresolved placement prevents native handoff/overlap; continue source
  integration and lower-demand fixtures, not a headroom override.

### RP-05 — standard continuous capture and encoder input ownership (P1)

**Canonical:** CAP-04/05/08/09/10; ENC-02/03/04/09. **Depends:** RP-04, CORE owner.

- Implement actual media topology/format/crop/stride/plane/trailer/timestamp model,
  standard V4L2/vb2 queue/poll/mmap/events/STREAMON/OFF and encoder OUTPUT/CAPTURE
  drain semantics. A renamed private ioctl is not standard-interface support.
- Validate noncoherent DMA-BUF export/import, full physical/SG extents/alignment,
  cache transitions, FDs outliving STREAMOFF and owner destruction. No arbitrary
  user physical addresses or fragmented-as-contiguous shortcuts.
- Integrate capture one frame while encoder holds another only after separate
  tests prove ownership/retirement. Network/recorder retain encoded leases, not
  raw slots. Demonstrate actual overlap with a monotonic timeline.
- Exercise stale/duplicate/overflow tokens, cancellation/close-held, old IRQ,
  queue-empty/requeue, timeout/reset/device loss and reopened generations. Keep
  current gated path as rollback and meaningful full CRC/guard qualification.
- **Exit:** standard-interface compliance plus meaningful ARM lifecycle/import/
  fault tests and stable native overlap at the admitted memory/profile budget.
  Replace10s/128-frame diagnostic renewal only after long-lived ownership proof,
  not by disabling limits/watchdog/integrity to hide gaps.

### RP-06 — imaging, hardware codec, rate and profile quality (P1)

**Canonical:** IMG-01–09; ENC-01/05/06/07/08. **Depends:** RP-03/04/05; limited
verified lights may precede full G4. **Current:** historical gated H.264/JPEG and
WB presets, not G1/G2 completion.

- Resolve real exposure/gain/HTS/VTS, Bayer phase/order, BLC/demosaic/gains/offsets,
  gamma/CCM, YUV planes/range/matrix and applied-state timing before colour tuning.
- Bounded transactional manual exposure/WB, real readback/units/ranges and rollback;
  qualify stats, AE/AWB convergence/invalid/saturated/dark exclusion and CPU cost.
- Preserve original stock140-pair payload versus our guarded139 stream-off subset;
  no count-only overread or stream-on before output/reference owners exist.
- Day/night coordinates mechanical filter, independent IR/white and colour/AE/WB
  with hysteresis/dwell/manual precedence; raw ambient is not lux. Sensor orientation
  must be consistent in RTSP/browser/recording/snapshot/masks/PTZ, not preview-only.
- Validate SPS/PPS/profile/level/crop/VUI, multi-NAL AU boundaries, frame/POC/IDR
  wrap and discontinuities with independent decoders; fresh IDR after incompatible
  profile/generation changes. No P-reference sharing across incompatible contexts.
- Implement actual measured fixed-QP range then justified bitrate/GOP/CBR/VBR/VBV
  controls, peak IDR/AU and shared-engine scheduling. Reuse licensed matching source
  only with IP/ABI proof; no silent software entropy encoder.
- **Exit:** G1 tolerances and G2 performance/quality/bitrate fit under declared
  static/noisy/moving/light conditions.15fps remains engineering acceptance, not
  factory fact;30 only after real proof. New capture path reruns imaging regression.

### RP-07 — one common runtime, graph, producer and control owner (P1)

**Canonical:** CORE-01–09. **Depends:** RP-01 and admitted backend capabilities;
feeds all subsequent services. **Current:** framework contract/fixture primitives
and QC callbacks, not complete deployed common services.

- Version the real boundary: board drivers/adapters own registers/resources;
  shared services own users/settings/media/recording/UI/setup/NVR. Migrate globals
  into instance contexts with callback/export lifetime proof, not a second stack.
- Stable physical camera/profile/encoder/head/light/audio/radio/store IDs,
  associations, measured concurrency groups, units and distinct absent/unsupported/
  unqualified states. Main/sub are profiles, never two physical cameras.
- Integrate one demand-refcounted producer for RTSP/browser/recorder, state and
  generations, bounded cancellation/drain/recovery and deduplicated IDR requests.
- One imaging/profile/PTZ/light broker with desired/applied/result IDs, true lease
  boundaries, priority STOP and stale-command rejection. Connected client is not
  a held hardware lease. Bound event/resume queues and monotonic/UTC mappings.
- **Exit:** the actual QC services use this owner; same services run on a two-sensor
  fixture with shared encoder/head, delayed/failed operations and independent
  namespaces. Conformance kit covers ABI, EBUSY, tokens/overflow, accounting and
  cancellation; context destruction waits for callbacks and exported leases.

### RP-08 — credentials, secure services and durable settings authority (P0/P1)

**Canonical:** SEC-01–12. **Depends:** RP-07 and admitted native process/store
boundaries; durable stages need RP-12/13/15. Existing RAM75 HTTPS campaign04 is
NATIVE-NARROW; reviewed verifier/profile/transaction cores are SOURCE/HOST.

- Reuse the reviewed auth/crypto stack and ARM writev/signal-return fixes. Finish
  per-device identity, independent UI/RTSP domains and explicit ONVIF mapping,
  first-use/password rotation, sessions/CSRF/origin, throttling and all endpoint
  authorization. Do not reuse lab root credentials as product defaults.
- Cold CSPRNG readiness, generated device keys/certificates, clock-unset/trust
  onboarding/rotation and key/store threat model. Warm supplied test material is
  not cold trust; local certificates do not imply public trust or ACME permission.
- Native isolated verifier first gate: correct root/groups/locked-memory origin,
  genuine READY15/provision ACK, owned OPEN/challenge/revoke, BYE/self-wipe and
  actual wait0. Numeric denial/transport statuses, result-retire acknowledgement,
  missing public principal/rekey API and revoke lifetime fences need explicit
  integration; no fabricated identities or credential rotation semantics.
- Adopt latest corrected v2 private profiles, metadata transactions and network
  transition close rules, not superseded HOLD versions. Native bootstrap/unseal,
  protected high-water/nonce/commit authority, real fsync/torn-write/reboot proof,
  schema migration and bounded receipt/profile housekeeping remain open.
- Least privilege, device/ioctl/filesystem scopes and parser/traversal/symlink/
  replay/flood/concurrent-credential-change tests under load. Digest is auth,
  **not encryption** of RTSP/TCP; declare the LAN/privacy threat model.
- **Exit:** actual protected services/settings on the admitted store survive
  relevant faults without secret leakage, privilege escalation, stale authority
  or silent desired/applied commits. No TLS/login-only G3/G6 acceptance.

### RP-09 — protected streaming, browser media and product UI (P1)

**Canonical:** STREAM-01–09; WEB-01–10. **Depends:** G2 + RP-07/08/04; storage
playback/settings/control panels depend on their corresponding packets.

- Finish AU contract (multiple VCL/parameter NALs, IDR/generation/time/error/lease),
  real asynchronous bounded fan-out and recorder-specific drop/admission policy.
  A stalled browser/NVR must not block capture or healthy peers. Finish or retire
  partial TCP/interleaved units honestly; never release borrowed data mid-send.
- Review/adopt RTSP server v3/verifier v2 and isolated-server integration tail;
  validate the actual foreign-domain negative test before relying on it. Native
  strict parser/CSeq/session/SDP/Digest/expiry/replay and actual-client compatibility.
- Correct RTP sequence/SSRC/marker/time, RTCP mappings and loss/discontinuity/wrap.
  TCP first; UDP/multicast only when needed and actually tested, otherwise truthful
  unsupported SETUP. No misleading codec/profile/FPS announcements.
- Direct on-camera authenticated H.264 MSE/fMP4 or separately qualified alternative,
  real lookahead duration/IDR/SPS initialization and bounded borrowed spans. Respect
 128KiB allocation and2MiB/process locked constraints; no whole-AU TLS copy/host relay.
- Resolve UI02 entry/lifecycle risk on exact assets/backend. Build real responsive
  capability/health/live camera selection, freshness, status and operation API;
  mount controls/settings/playback only through qualified owners and authorization.
- Desktop **and mobile** browser tests: tabs/users/expiry/logout, hidden/disposed
  pages, blur/touch/keyboard cancellation, reconnect/profile/orientation and network
  changes. Retained desktop-video feasibility is not live/mobile product proof.
- **Exit G3:** real protected RTSP plus direct browser hardware video at the G2
  profile, concurrent healthy/slow clients, valid timestamps, independent passwords,
  bounded resources, clean revoke/stop/restart and no corruption. Bench GUI is not
  the product Web UI; fresh snapshots/audio need their own source/privacy gates.

### RP-10 — shared L2/clock/analogue resource foundation (P0 source; P2 native)

**Canonical:** PLAT-04, CTRL-07, AUDIO-01/02, STORE-01/02 supporting work.
**Depends:** selected source facts and UART preservation; no generic new framework.

- One coordinated owner for common L2 buffer allocation/assignment, request words,
  sole fraction engine/address, IRQ dispatch, cache/CPU-device transitions and
  callback/free fences; dedicated UART SRAM does not isolate shared control words.
- Device selectors4/MCI,14/ADC,9/DAC are **not fixed physical buffer IDs**. Preserve
  UART/shared bits; no global L2 init/reset while it owns console state. Initially
  serialize clients and use owned aligned whole periods/blocks where practical;
  small discovery/tail transfers still require bounded padded handling.
- Shared ADC/DAC/ADC1 reference/analogue/clock/reset resources need masked/refcounted
  ownership, bounded update waits and last-user close. Retail DAC close can affect
  active ADC; source enum/clock comments are not physical rate/voltage facts.
- Early/panic console and hardIRQ paths must remain atomic-safe; do not impose a
  sleeping client lock on UART or use a global reset to recover an audio/MCI client.
- **Exit:** provider failure/IRQ/fraction/clock tests prove safe start/rearm/stop/
  cancel, direction-correct DMA and UART coexistence. Failure retains memory and
  owner, not an old driver's unbounded wait or free-on-timeout behavior.
- **Hold:** missing physical route/pdata/clock facts get a named measurement/source
  dependency; do not enable MMC/audio refusal gates until real provider work passes.

### RP-11 — PTZ, IR-cut, white/IR, ambient/reset and bench GUI (P2)

**Canonical:** CTRL-01–10; IMG-07/08; bench GUI regression. **Depends:** G3 for
product delivery, RP-07 owner, RP-10 for ambient ADC; safe light work can support G1.

- Rebuild/pair optical and white modules against selected kernel metadata; recheck
  used UAPI/syscalls, helpers/dispatchers/toolset and explicit76 admission. Retired
 75 profiles, hashes and same dirty uname cannot authorize controls76.
- Keep GPIO6/A OUT/IR-pass,7/B IN/IR-block, pair00 release; independent23 IR and5
  white. Validate load/neutral/OFF, bounded ON/pulse/cooldown and cancellation,
  holder lifetime/reaping and release under CPU/media/fault load. Nominal4900ms
  light and10ms filter pulse are not measured electrical deadlines/hang protection.
- Preserve hard±4/20ms diagnostic nudges until explicit mechanical expansion.
  Stock direction observations are scoped to that backend/installation; modern
  issued28 steps did not establish displacement/direction/limits. Obtain actual
  reference feedback for both axes, backlash, sign, duty/thermal and STOP.
- No automatic home or inferred absolute position. Eight-phase bytes are known,
  but retail reset is contradictory and limits are not feedback. A future home/
  limit experiment needs algorithm, clearance, abort/coil-off/current/stall risk
  and independent OFF recovery review. Label/invalidate open-loop estimates.
- Product relative/continuous velocity, lease STOP/queue revocation, process loss/
  unload/fault generation and UI/ONVIF arbitration; absolute/presets only if actually
  calibrated. Postfault descriptor release must not turn UNKNOWN into OFF.
- Modern ambient ADC1 channel0: preserve fitted sensor/scalar evidence, implement
  bounded IIO/hwmon/backend conversion with timestamp/validity; manual guarded
  one-shot bench readings, not auto polling or guessed lux. ADC2 audio is separate.
- GPIO0 input: debounce, startup-held, short/long/stuck/bounce cases in disposable
  event state. Our settings-reset policy lives in RP-15, not copied retail code.
- Repaired FreeSimpleGUI expiry/Close native trial: start with no auto command,
  pre-TX denial closes, timely worker result survives delayed Tk polling, real
  post-TX/unknown/late HOLD remains. No automatic window reopen/epoch renewal.
- **Exit G4 controls:** observed mechanics/bounded STOP, independent lights/filter,
  truthful ambient/input, fault-safe one owner and loaded EMI/power/capture/USB
  coexistence; later re-run with recording. Register matches are not mechanics.

### RP-12 — modern microphone and numeric speaker playback (P2)

**Canonical:** AUDIO-01–07, with explicit playback extension `AUDIO-02-P` and
numeric bench interface `AUDIO-03-P` (additions, not a renumbering).
**Depends:** RP-10, RP-07/08 and appropriate UART isolation/recovery for playback.

- Port exact ADC2/DAC/HP/mic-bias/rate/gain/IRQ/cache/stop inputs into standard
  ALSA/ASoC: bounded owned rings/periods, negotiation, xrun/underrun, timestamps,
  IRQ plus deferred notification drain and module/stream/free lifetime.
- Retail ADC8k uses OSR256/divisors1..256; decoded400MHz/div195 calculates8012,
  not measured8000. DAC has its own search; distinguish sample/high-speed clocks,
  preserve shared PLL and qualify actual rate/drift before A/V mapping.
- Generate speaker samples **on camera from numbers**, not transferred recordings:
  gain0–5, volume0–100%, duration0–5000ms, frequency below the calculated selected
  Nyquist; validate silence/zero duration, overflow/clipping/range errors and normal
  STOP/drain. Initial physical trials remain bounded low-level public tones.
- Retain the confirmed GPIO1-LOW speaker enable in the appropriate playback route;
  do not repeat silent-route/gain/pin sweeps or drive it while UART is connected.
- Mute/enable/readback, clipping/noise metrics, truthful formats/channels and safe
  serial capture/playback admission before any duplex experiment. Never treat
  analogue close as DMA retirement or GPIO1 input-copy restoration as amp disable.
- Plan low-cost codec/container/SDP/browser/NVR compatibility, common sample/frame
  epochs, drift/discontinuities and independent consumer cancellation. Do not assume
  hardware AAC/Opus, heavy transcoding capacity or an unrequested general talkback API.
- **Exit G4 audio:** real qualified modern capture and truthful controls. Full
  stream/recording/NVR/A/V acceptance continues in G3/G5/G7. Scalar acoustic/rate/
  overrun evidence and synthetic tones only until the explicit privacy hold is
  resolved; scalar PASS alone cannot certify intelligibility/codec/recorded audio.

### RP-13 — SD/MMC, owned filesystem and imported recordings (P2)

**Canonical:** STORE-01–04; PLAT-04/08. **Depends:** RP-10 and specifically identified
expendable media/recovery. **Current:** source/build candidate refuses native probe.

- Complete exact board resource/cold-init contract: proprietary MCI IRQ/mux/clock/
  reset, physical GPIO47 rail/polarity and actual pdata, detect/WP/removal policy.
  Stock group13 SYSdc3fc0/15c0 and LOW1s/HIGH pulse do not prove electrical supply.
  GPIO42 is radio power, not the board3 SD branch to reuse.
- Implement correct mapped-SG counts, DMA sync/direction/alignment/bounce/tails,
  command/data/STOP/timeout/IRQ/callback retirement, non-remuxing failure/remove and
  repeated reads under UART/USB/audio arbitration. Host bind/load can send card
  commands/pulse power before software RO; admission must cover initialization.
- First native host read is RO on an explicitly identified test card. Then separate
  reviewed public-pattern write/flush/reread in an owned sacrificial scope. No mount
  repair, format, FSINFO correction or original-footage/recovery card assumption.
- Owned mount/storage-generation/directory service with actual fsync/rename/
  directory-sync/large-file/error semantics, exclusive writer and validated path/
  inode/mount identity. Separate RO imported-footage catalogue and documented codec/
  container/index/time/key support; unknown/encrypted formats stay honest blockers.
- **Exit:** reliable native read/write/durability contract for owned data, preserved
  originals and safe generation/removal behavior. Writer/playback work waits for
  this real store, not catalogue/index HOST tests alone.

### RP-14 — segment recording, motion/masks, retention and playback (P2/P3)

**Canonical:** STORE-05–12; WEB-09; STREAM-02/08; NVR-08 as selected.
**Depends:** RP-13, qualified producer/ownership/auth/time, RP-04 budget and privacy.

- Implement actual IDR-aligned bounded container segments and index/catalogue commit,
  valid timestamps/gaps, pending→closed publication and recovery of **owned**
  incomplete files. Metadata CRC/index PASS is not a media repair/writer result.
- Continuous/event scheduler, explicitly bounded pre/post-event coverage and
  recorder backpressure/drop policy; interrupted/no-input paths produce visible
  gaps, never stale live frames or incomplete “complete” clips.
- Adopt corrected SOURCE/HOST motion v2:32×18 supplied luma cells, eight16-vertex
  polygons,≤4KiB state, post-callback authority/acquisition/known-loan rules. Build
  and qualify a cheap real statistics source with full timing/cache/integrity/
  lease coverage before tuning. No false “motion detector” from invented cells.
- Tune actual scenes/noise/exposure/debounce/mask edges; disclose conservative
  cell masking/global-brightness suppression. Transform masks for crop/orientation/
  profile changes; inhibit/compensate PTZ/day-night effects without hiding motion.
- Both oldest-first/free-space and **active date-window** retention: reserve/
  hysteresis/timezone/invalid-UTC policy, closed-owned-only deletion, no symlink/
  mount race, open-recorder/playback lease exclusion. Inventory/preview before any
  adoption of originals; full card stops/drops visibly, never expands delete scope.
- Authenticated recording-ID catalogue/timeline/playback, real playable containers,
  seek/HTTP ranges and valid concurrent recording/pruning leases; no arbitrary paths.
  Bound64-item prototype catalogues through paging/rotation, not ceiling inflation.
- **Exit G5:** actual SD continuous **and** motion clips/masks, both retention modes,
  RO import and browser playback survive full/RO/corrupt/removal/power/clock faults
  in approved owned test media. Original footage/recovery files demonstrably intact.

### RP-15 — durable network setup, protected AP/captive portal and own reset (P3)

**Canonical:** SETUP-01–10; SEC-07–10; CTRL-08; WEB-07/08.
**Depends:** RP-02/07/08/13, AP feasibility and disposable persistent test namespace.

- Link/association/address/reachability generations, DHCP/static IPv4/DNS/hostname/
  renewal/conflict handling and explicit IPv6 policy. Local product operation must
  not require public Internet. Scan capability/disruption/cache age/error/busy and
  manual SSID fallback; failure is not an empty result or silent disruptive scan.
- Integrate corrected v2 network-profile transitions/private profiles/metadata
  store with real radio and durable authority. This is **not video-profile switching**.
  Prove receipt delivery before disruption, pending commit/verify/rollback, exact
  lost/duplicate result retrieval, interruption/reboot and bounded subsequent ops.
- Implement actual secure AP peer/key/beacon/management/EAPOL/PS contract, protected
  one-client first-use initially, role switch/retirement and return to station.
  Exclusive AP→STA can suffice; AP+STA is optional unless explicitly required.
- Per-device finite onboarding, AP-side DHCP/DNS/HTTP captive hints/steering,
  tested platform captive assistants and explicit local fallback URL. No insecure
  permanent setup backdoor or normal-LAN/HTTPS impersonation.
- Our deliberate debounced long-press settings-only reset revokes sessions,
  safely stops owners/recorder, preserves identity/calibration/firmware/footage and
  returns to protected setup. Test in disposable RAM/test settings before any
  authorized persistent transition; no retail factory-reset implementation detour.
- **Exit G6:** wrong networks/address faults/scans/racing admins/reboots/reset/power
  loss produce recoverable durable results and usable protected AP. Missing key/
  store authority, real AP or privacy scope is a visible blocker—not a UART fallback.

### RP-16 — DNS, discovery, ONVIF and two real NVRs (P3)

**Canonical:** NVR-01–11. **Depends:** protected sources/settings/controls and
actual user-supplied NVR hosts/versions/access.

- Stable durable hostname/UUID and measured DHCP hostname/FQDN/DNS-server behavior;
  bounded collision-safe mDNS/DNS-SD, withdrawal/reannouncement across address/
  interface/setup changes. No account/router/public-DNS changes by inference.
- Select the actual client-required ONVIF subset; bounded genuine WS-Discovery,
  reachable XAddr/device/network/time/capability services and XML security. Do not
  return canned successful SOAP or claim Profile S/T/G certification from fixtures.
- Truthful physical camera/main-sub profiles, media/snapshot/imaging options and
  relative/continuous PTZ/STOP through the same auth/owner path. No invented zoom,
  absolute/preset/home capability. Bounded motion/health subscriptions/revocation;
  ONVIF recording/playback is conditional, not implied by browser playback.
- Actual Frigate **and** licensed Blue Iris workflows: independent auth, video/
  required tracks, ingestion, supported controls, restart/timestamps/address
  changes, concurrent browser+camera recording and bad-auth/time-skew/XML faults.
- **Exit G7:** recorded actual-client acceptance and truthful discovery, not RTSP
  decode or SOAP mocks alone. Missing owned endpoints/version/license/access blocks
  those rows; do not provision third-party hosts/accounts without instruction.

### RP-17 — coupled qualification and QC07F release (P3)

**Canonical:** QUAL-01–10. **Depends:** G0–G7 and same deployed shared services.

- Maintain requirement→canonical task→implementation→HOST/build/native evidence→
  acceptance matrix, including corrected versions, known failures and all holds.
- Freeze release profile/threshold/client/privacy/storage matrix before campaigns;
  host sanitizer/property/fuzz/fault and clean ARM suites must cover real ownership
  edges and same-service dual-resource fixtures, not counts that restate edits.
- Native≥100 meaningful cycles: capture/codec/radio/store, held exports, cancellation,
  allocation pressure, source/IRQ/USB faults and loaded UART; no leaked backing or
  unexplained integrity/decoder/healthy-client failures.
-1h then24h admitted combined NVR+browser+recorder+audio+motion+retention+controls/
  admin soak: unique frames, p95/p99/max, CPU/RAM/CMA/residency/fd/socket/log/temp/
  card-latency trends and a positive resource reserve. Reject extra work before OOM.
- Fault/security matrix: link/address/credential changes, client stalls, USB/store/
  reset/config interruptions, wrong clocks, watchdog/service crash/restart and
  source stall/exhaustion. Safe documented outputs and honest resumable outcomes;
  closed owned footage survives and incomplete media stays unpublished.
- All endpoints protected, no secret/debug-listener leaks, notices/dependencies
  reviewed, core local operation without cloud. Release includes source/tools/
  config/DT/module/calibration/schema/migration/evidence and recovery instructions.
- **Exit G8:** all mandatory features actually accepted on QC07F, shared-service
  dual-topology fixture passes, adapter conformance kit/version frozen and explicit
  parent/user gate review. Do not substitute stub/disabled/“unsupported” for missing
  required hardware. Persistence installation is separately gated by RP-18.

### RP-18 — persistent modern image / update / rollback decision (conditional)

**Canonical:** PLAT-08/09; QUAL-08/09/10. **Depends:** selected product build and
authorized storage/boot design; not a blocker to non-installed RAM build work.

- Measure release fit across the existing8MiB map, compression/initramfs/root/module/
  configuration needs, RAM staging/decompression and working set. RAM76 cannot be
  written into the existing1.5MiB KERNEL partition. Do not truncate or silently
  repartition; no assumed second slot/true A/B or automatic modern NOR experiment.
- Select bounded compatible update/auth/key/migration/rollback and boot-loop rescue
  policy. Verify backups/current composition, preserve bench SSH/password and all
  excluded bootloader/ENV/MAC/USR/JFFS/SD/OTP scope unless separately authorized.
- **Entry to any write:** exact final image/hash/size/ranges, current backup and
  restore transport proof, affected configuration/recording scope, interrupted-write
  recovery, user physical readiness and actual specific authorization.
- **Exit if installed:** byte verification plus ordinary cold boot, auth/runtime,
  reboot/power-loss/recovery and separately recorded installed-state acceptance.
  The stock SSH-only ROOT trial is not installation approval for the modern OS.
- **Alternative:** deliver reproducible RAM-qualified G8 with honest “not installed”
  status, while durable setup/settings have independently passed on approved media.

### RP-19 — T34-4G / KM01 port after G8 (P4, DEFERRED)

**Canonical:** T34-01–09. Fresh board/sensors/RAM/flash/UART/power/recovery inventory;
matching licensed source and AliOS→Linux boot feasibility; measured RAM-only UART/
timer/watchdog/PID1 minimum; actual modem/USB/network/AP policy; both physical
sensors/ISP/capture; codec/shared-memory scheduling; fitted peripherals/audio/SD;
unchanged shared services and full dual-source qualification.

LTE→USB-Wi-Fi conversion remains a separately measured physical plan: unpowered
D+/D− continuity, supply/current/reset/enable, appropriate adapter and explicit
network selection. No assumed5V, GPIO77 or automatic modem removal. **Entry G8**;
**exit G9** requires both real sensors and fitted/required features plus QC07F
regression for portability changes. No separate platform-specific service stack.

### RP-20 — Fulhan and final all-model review (P4, DEFERRED)

**Canonical:** FH-01–11. **Entry G8/G9 by default**; a user-directed port reorder
needs an explicit gate/dependency mapping, never bypassing G8.

Fresh flash/reinstallation/UART/PoE/board/sensor inventory; licensed source/kernel/
memory strategy; minimum RAM boot/recovery; measured Ethernet/PHY/RTL8188FTV/WLAN/
AP/power behavior; both actual sensors; codec/shared-engine/capture lifetimes;
fitted controls/audio/SD; unchanged common services, full acceptance/recovery.
No Anyka addresses/modules/calibrations or unsafe dual-supply PoE assumptions.
**Exit G10:** both sensors and required features on Fulhan, then real cross-model
media/security/store/setup/reset/NVR/resource regressions on all three. Only after
the full requirement/evidence review may `all-required-modules` become true.

## 6. Priority schedule and hard decisions

| Order | Next deliverable | Parallel source lane allowed | Hard stop before native/acceptance |
|---|---|---|---|
| 0 | RP-00 current-state/evidence reconciliation and frozen release decisions | Requirements/task coverage, no target queries during this plan | Expired20c is not protection |
| 1 | RP-02 finite full-size transport entry/outcome; retain384 | RP-04 memory/retirement and RP-08 corrected-candidate adoption | No blind2048/RUN retry, source-safe owner/recovery first |
| 2 | RP-03 controlled image/timing baseline, RP-04 concrete backing choice | Early AP/key/store feasibility and CORE integration | No guessing FPS/colour/units or allocating an unfit pool |
| 3 | RP-05/06 real standard continuous capture/import/encoder/quality → G1/G2 | Reviewed auth/RTSP tail and same-service fixtures | No integrity bypass/session-limit removal to reach target |
| 4 | RP-07/08/09 protected native browser+RTSP → G3 | RP-10 shared-resource providers, scoped package/GUI source work | Resolve v6/UI02/isolated-RTSP entry and actual protected memory placement |
| 5 | RP-11/12 actual modern controls/audio → G4 | RP-13 RO MMC/resource readiness | No homing/recorded mic/electrical inference or UART-connected speaker |
| 6 | RP-13/14 actual owned recording/retention/playback → G5 | RP-15 AP/setup source implementation | Specific expendable card/write/deletion/fault authority required |
| 7 | RP-15 durable transactions/protected setup/reset → G6 | RP-16 protocol/client preparation | Real secure AP and durable authority cannot be replaced by stubs |
| 8 | RP-16 real Frigate+Blue Iris → G7 | QUAL release/fault preparation | Supply actual owned clients/versions/access |
| 9 | RP-17 combined QC07F acceptance → G8; RP-18 separately if authorized | Documentation/reproduction, no more speculative cores | Every mandatory subsystem and actual fault/load evidence required |
| 10 | RP-19 → G9, then RP-20 → G10 | Board-local source work within approved later scope | No deferred hardware port before G8 |

Resolve before the affected campaign, not by silently changing requirements:

1. Freeze profile/client/threshold criteria, including15fps engineering goal and
   justified ceiling review procedure; choose desktop/mobile/NVR versions.
2. Decide actual key/entropy/trust/clock/rollback threat model and attainable
   durable authority/storage. Ordinary AEAD/fsync fixtures are not that authority.
3. Define microphone evidence/privacy scope for codecs/streams/recording/A/V;
   current scalar-only restriction stands. Speaker numeric controls are required
   bench functionality; general duplex/talkback needs an explicit product decision.
4. Agree mechanical STOP bound, manual-origin/limits and any absolute/preset need;
   unknown positions cannot be turned into advertised coordinates.
5. Identify safe test media/measurement access, separate footage/reset/retention
   ownership scope and missing SD rail/pdata/clock facts.
6. Resolve secure AP feasibility early. If it cannot be delivered on fitted
   hardware, present measured alternatives and user scope review; keep G6 blocked.
7. Supply owned Frigate and licensed Blue Iris hosts/access; no invented endpoints.
8. Select RAM-qualified release versus a separately authorized persistent image;
   no extrapolation of the old ROOT-only authorization or silent repartition.

## 7. Required test matrix and stop rules

| Dimension | Minimum categories to cover on the selected real implementation |
|---|---|
| Lifetimes | Start/stop/reopen, held export, cancelled/late completion, old generation, callback reentrancy, wrap/exhaustion, owner loss and error-retained backing |
| Resources | Cold/warm/fragmented allocations, process locked/address limits, raw+DMA+TLS+hub/store/audio overlap, queues/log growth and loaded UART |
| Media | Unclipped colour, motion/noise/light changes, IDR/parameter changes, source stalls, multiple GOPs/NALs, unique independent decode, real timestamp/clock mapping |
| Consumers | Browser/NVR/recorder simultaneously; healthy, slow, silent, disconnected, expired/revoked, late join and lost-response result retrieval |
| Controls | Auth denial, conflicts, STOP/lease/process loss, unknown coil/light state, input bounce/held/stuck, orientation/mask/installation mapping |
| Store | RO/full/unplug/corrupt/torn-write/clock rollback, closed/pending reconciliation, playback leases, both retention modes, original-footage/recovery exclusion |
| Setup | Cold first-use, wrong network, disruptive scan, renew/address conflict, interrupted commit/reset, AP client auth/key/PS/retirement and safe return to station |
| Security | All live/playback/range/events/control/admin endpoints; cross-domain auth, replay/nonce/CSRF, malformed/oversized input, path/XML attacks, cold trust and secret leakage |
| Ports | Same services on two-sensor/shared-resource fixture, then both actual sensors per deferred board and all-three regression |

Before each test freeze input pins, effects, maximum duration/bytes/owners and
admission thresholds. Report distributions/outliers/rejections, not averages or
test counts alone. On uncertain hardware completion retain HOLD, mappings and
owner/provider lifetime; no fresh-status fault reset, forced unload, killed DMA
client or repeated actuator command. User power-OFF/recovery is a named physical
dependency, not an assumed command or reset-button capability.

## 8. Canonical task coverage and handoff

### Gate card required when a packet is later opened

Record: packet/canonical IDs and physical camera/profile; priority/dependencies;
current SOURCE/HOST/BUILD/NATIVE/ACCEPTED state; exact source/artifact/evidence pins;
parent reviewer; E0/E1 findings; selected E2 runtime/owner/recovery/expiry; explicit
allowed effects, media/privacy scope and resource/time budget; positive/negative
test cases and frozen thresholds; stop/HOLD/rollback method; actual E3 observations
and retirement; remaining E4 product gate; superseded-versus-still-open defects;
and **one next task**. Missing fields are a review HOLD, not an implicit approval.

### Coverage of the existing subsystem task list

Every canonical task remains in its original subsystem file. This grouped
crosswalk assigns all of them to completion packets; source/native/product state
must still be maintained per actual task and platform during execution.

| Canonical scope (inclusive) | Primary packets / gate |
|---|---|
| BASE-01–07 | RP-00, G0 and every E2 |
| CORE-01–09 | RP-07, G2/G3/G8 |
| PLAT-01–09 | RP-01/04/10/18, G0/G2/G8 |
| IMG-01–09 | RP-03/06/11, G1 then regression |
| CAP-01–10 | RP-04/05, G2 |
| ENC-01–09 | RP-03/05/06, G2 |
| NET-01–08 | RP-02, G2/G3/G6 |
| STREAM-01–09 | RP-09/14, G3/G5 |
| SEC-01–12 | RP-08/15/17, G3/G6/G8 |
| WEB-01–10 | RP-09/14/15, G3/G5/G6 |
| CTRL-01–10 | RP-10/11/15, G4/G6 |
| AUDIO-01–07 | RP-10/12/09/14/16, G4 plus tracks in G3/G5/G7; privacy hold |
| STORE-01–12 | RP-10/13/14, G5 |
| SETUP-01–10 | RP-15, G6 |
| NVR-01–11 | RP-16, G7 |
| QUAL-01–10 | RP-00/17/18, G8 |
| T34-01–09 | RP-19, G9 after G8 |
| FH-01–11 | RP-20, G10 after G8/G9 |

Keep current code/evidence, including failed and corrected versions, local and
uncommitted. When execution is later requested, select **one bounded packet**,
record its exact applicable gate/status and one next step. No percentage/calendar
promise is assigned to unmeasured hardware work. Do not reopen stock discovery or
launch the old continuous implementation queue simply because this plan exists.

### Planning sources / supersession rules

- Canonical detailed scope: plans01–18 beside this file; gate meanings from
  [00-INDEX](00-INDEX.md#gate-definitions); complete [qualification](16-FOUNDATION-QUALIFICATION.md).
- Current native boundary: [kernel76 ledger](KERNEL76_TASKS_20261020.md),
  [build/boot results](../../qualification-plans/kernel76-integration-20261020/RESULTS.md),
  [RAM20c result](../../qualification-plans/kernel76-integration-20261020/RAM_BOOT20C.md).
- Driver/reference findings: [stock task ledger](STOCK_REFERENCE_TASKS_20261020.md),
  [closeout inputs](../../qualification-plans/stock-reference-20261013/closeout-parent01/NEXT_BUILD_INPUTS.md),
  and its SD/audio/Wi-Fi/motor addenda. Closed source investigations are not open
  modern native features, and source decisions do not pass those features.
- Memory/performance: [cost and memory](../../qualification-plans/qc07f-performance/COST-AND-MEMORY.md),
  `apps/web-media/native-producer/README.md`, and the retained v6 integration seals.
- Corrected source adoption: `evidence/delegated/20261012-parent-review/` receipts
  for motion-v2, profile-transition-v2, private-profile-v2, network-transaction-store-v2,
  RTSP-server-v3/verifier-v2 and catalogue corrections. Prefer these later parent
  adoption records over old README/HOLD prose; do not invent native acceptance.
- [GUI Close correction](../../qualification-plans/stock-reference-20261013/optical-live01/GUI_CLOSE_FIX_20261020.md)
  is HOST-only for the repaired window. RAM76 postcheck corrections **did** pass
  native20c; neither should be inaccurately treated as an unresolved old bug.
- Later user steering/privacy/effect authority overrides older task wording. This
  consolidated plan resolves current planning interpretation, preserves originals
  and does not authorize target actions or changes to requirements by itself.
