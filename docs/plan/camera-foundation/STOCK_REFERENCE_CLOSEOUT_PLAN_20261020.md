# Remaining worthwhile stock investigations — review draft

Date: 2026-10-20. **PLAN ONLY; awaiting user review.** No stock boot, build,
instrumentation, module load, card access, motor zeroing or other target operation
is authorized/executed by creating this document. This is a finite reference
closeout campaign, not a return to general retail-firmware reconstruction.

**Subsequent user approval:** execution is now active. The authoritative status,
priority and evidence ledger is
[STOCK_REFERENCE_TASKS_20261020.md](STOCK_REFERENCE_TASKS_20261020.md). The opening
review-draft statement above is preserved history; the excluded effects below
remain excluded, not automatically authorized by approval of the base campaign.

## Decision rule

Investigate stock only when the answer changes a named part of **our** driver,
resource ownership, performance target or safety policy. Reuse existing evidence
first. Deliver a concrete hardware contract/difference and an implementation
decision, or a precise unresolved fact. Do not build another speculative subsystem
core, reconstruct the whole stock kernel ABI, or copy retail application policy.

Enough is already known to consolidate the working modern-kernel RAM image.
These investigations run before enabling affected unfinished drivers or claiming
their performance; they are **not all prerequisites for producing that image**.

## Already answered — do not repeat without a changed question

- Stock Wi-Fi MTU1500, 2048-byte radio RAM download and finite bidirectional public
  pattern integrity were observed. MTU384 is our workaround, not a factory limit.
  The transport's four-byte tail at endpoint-maxpacket multiples is source-backed;
  a successful stock download does not justify deleting it from our implementation.
- Stock MCI recognized the card using L2 DMA, software IRQ and always-present
  detection; no hardware write-protect support was reported. Original MBR/VBR/
  invalid-zero FSINFO were preserved, not repaired. This does not establish actual
  supply/power wiring, Linux6 operation, safe removal or write/flush behavior.
- Board5 motor routing is direct GPIO, not a UART/MCU or board3 AW9523 hypothesis.
  Pin order and four-phase bytes are known. Probe/default geometry and app-selected
  eight-phase/configured geometry differ; full homing and the exact eight-phase
  table were not closed by those audits.
- Reset GPIO0 input/polarity, white GPIO5 output-LOW OFF, IR GPIO23, IR-Cut
  GPIO6/A OUT/IR-pass and GPIO7/B IN/IR-block have bounded native/user evidence.
  Speaker GPIO1 active-low enable and audible5s playback are known on stock3.4.
  Do not restart pin sweeps, factory-reset/GUI policy or speaker-presence searches.
- Stored video requests15fps and the sensor getter caches30. Neither measures
  actual stock cadence. Our gated checked pipeline has real costs: historical
  kernel CRC_PRE+POST alone averaged about99ms/frame; ASIC wait about31ms was not
  the sole bottleneck. Stock behavior is a reference, not an excuse to assume a
  15/30fps ceiling or to remove our integrity checks.

## Priorities and expected decisions

| ID | Priority | Question that is worth answering | Decision/output |
|---|---|---|---|
| SRX-01 | High | Why does stock USB/radio carry normal-sized packets while ours needs MTU384? | Exact transport/HCD delta and smallest modern-driver experiment |
| SRX-02 | High | What actual MCI board resources and shared L2 rules are needed for native SD? | Board/resource table, bounded DMA/stop contract, modern MMC implementation inputs |
| SRX-03 | High | What does stock really capture/encode/deliver, and how is its pipeline arranged? | Measured cadence/profile, buffering and phase costs; realistic performance target |
| SRX-04 | High, static first | What does motor zeroing actually do, and how do phase mode/STOP/origin interact? | Own homing/manual-origin decision, not blind execution of RESET |
| SRX-05 | Targeted | Which audio clock/bias/shared-L2 and stop facts still block our ADC/DAC port? | Concrete modern audio-provider contract, not another gain/audibility sweep |
| SRX-06 | Conditional high | Can the shipped radio firmware support the required protected setup AP? | Supported / unsupported / unresolved with exact missing client/key/role contract |
| SRX-07 | Conditional | Is a stock SPI/NOR detail actually missing for the chosen persistent image design? | Only the missing controller/boot contract; otherwise skip |

### SRX-01 — Wi-Fi / USB boundary and flow-control differences

**Reuse:** existing stock MTU1500/TCP results, exact radio firmware identity,
`hgic_usb_tx_packet` alignment/tail evidence, paired station75 source and previous
MUSB failures. Do not repeat basic association merely to reconfirm1500.

**Static comparison:** selected stock HCD/glue versus our indexed MUSB PIO path:
FIFO sizing and access width/alignment, maxpacket512 boundaries, multi-packet
actual-length handling, short/ZLP and padding rules, NAK/error/timeout handling,
toggle/reset/flush sequencing, completion/resubmission, RX/TX ownership and queues.
Also compare actual firmware-command framing, credits/aggregation and power-save
settings. Distinguish host-controller behavior from radio firmware/driver behavior.
Establish whether stock uses PIO or DMA in the relevant path; **do not assume DMA
is required** just because stock is faster or correct.

**Optional controlled runtime:** same authorized VertexHotspot and owned PC,
preallocated public-pattern traffic at named packet/USB boundaries and bounded
bidirectional bursts. Capture counts, actual lengths, errors, queue/credit state
and timings, not credential payloads. Include before/after counter deltas.
Compare equivalent modern-kernel tests only after a specific fix exists.

Stock tracing is not assumed: the earlier session had no ready ftrace/kprobe/
debugfs facility. Prefer existing counters, retained source and already-read
callback values. No whole-MMIO register dumps or destructive FIFO/status reads.
Do not make a stock instrumentation/kernel-ABI project a prerequisite; if that is
the only route, document the gap and instrument our controlled modern driver.

**Exit:** an explicit HCD-versus-radio difference and patch/test hypothesis, or
bounded evidence that the suspected difference is not responsible. Actual MTU1500
acceptance must ultimately be qualified on **our** driver.

### SRX-02 — SD/MMC board contract and shared L2, including UART/audio

**Static first:** bind the actual board5 platform/probe branch, not evaluation-
board GPIO defaults. Resolve fixed versus switched power, GPIO47/other candidate
meaning, active polarity, pinmux, parent/divider/edge, reset/gate and detect policy.
GPIO42 is already radio power on this unit: never try a different-board SD power
branch against it. Always-present stock detection does not prove a physically
non-removable slot or usable hotplug; no hardware WP support is not software RO.

Map MCI hwirq4, L2 hwirq12 and the real buffer assignment path. ADDR_MMC_SD enum4
is a device selector, **not physical buffer4**. Record common-buffer allocation,
fraction-engine serialization, UART-dedicated SRAM/shared control words, IRQ
acknowledgement, SG/map return counts, alignment/bounce behavior and cache sync
for both directions. Trace normal completion, STOP, timeout and release order.
Controller disable alone is not a proved L2/DMA/callback retirement fence.

**Passive stock receipt, where source-safe:** cached host/ios/capability metadata,
actual selected board fields, safe mux/clock/config words and existing owners.
Whitelist non-destructive reads; exclude read-clear STATUS/FIFO registers.
Loading a host driver is **not passive**: it can toggle power/remux/send card
commands before userspace can set a block device RO. Audit that path and automount/
hotplug helpers first; reuse cached observations if new loading is unnecessary.

Driver code cannot prove rail voltage or physical switch continuity. If essential
and still ambiguous, request a safe isolated low-voltage measurement/unpowered
continuity check; no probing an exposed/unisolated mains circuit or pin sweep.
If no CD signal is used, choose an honest polling/removal design rather than
inventing a CD GPIO. Separate initial prototype boot-inherited state from an
independently cold-initialized production host; review either before native use.

**Optional card reads:** only a specifically identified expendable test card,
preferably with a public pattern prepared on the owned PC. Original footage and
recovery card are excluded. Suppress automount/recording and establish software RO
for disk/partitions before allowed block reads; no mount or filesystem repair.
Measure bounded single/multiblock paths if needed to resolve actual scheduling,
bus width/clock or alignment. Metadata is not a voltage/frequency measurement.

**Writes:** excluded from initial approval. A later small public-pattern write/
flush/reread on an explicitly approved sacrificial card is worthwhile only if
write-direction DMA or completion/flush semantics remain unresolved. No formatting,
power-loss/removal fault injection, old-footage writes or FSINFO repair here.
Unsafe legacy timeout/free behavior can be understood statically; it need not be
reproduced by inducing DMA faults.

**Exit:** one concrete UART/MCI/ADC L2 ownership and safe-stop table plus actual
board resource facts sufficient to implement a minimal modern host. Clearly mark
any retirement guarantee the old driver fails to provide. This is native-provider
work, not a reason to add another abstract MMC/L2 state-machine framework.

### SRX-03 — Real stock video cadence and pipeline organization

Separate compiled defaults, preserved stored settings, active requests, sensor
timing and measured results. This camera's preserved settings are not guaranteed
pristine factory defaults; do not factory-reset it to manufacture a baseline.

Trace just the selected sensor/ISP/encoder initialization and frame scheduling:
verified sensor clock/divider/HTS/VTS/exposure behavior; pending FPS application;
enabled main/sub paths; actual DNR/reference/buffer allocations; continuous versus
gated capture; capture/encode overlap; cache/copy work and IDR/P-reference reuse.
Existing139-pair controlled stream-off preparation remains distinct from the
original140-pair payload ending STREAM_ON. Admit stream ownership before use.

**Runtime only if a reviewed minimal media route is available:** original kernel,
exact providers/libs and reservations, no normal camera app/cloud/record/homing.
One daylight/unclipped main-profile run, about5s settle then10s measured, with a
reviewed whole-run bound; a separate dim/AE run only if it answers whether exposure
reduces cadence. Record resolution, actual illumination/filter position, exposure,
gain, HTS/VTS, requested FPS, bitrate/GOP units and output mode.

Measure independently:
1. completed source-buffer sequence/timestamps;
2. complete encoded pictures/access units and frame/IDR progression;
3. unique full-rate decoded frames at the owned host, plus duplicates/gaps.

Report `(N-1)/(t_last-t_first)`, throughput including startup, interval distribution
and dropped/rejected counts. IRQ totals, cached getters and SPS/VUI labels alone
are not delivered FPS. **Do not use our optical GUI's at-most5fps display selector
as the benchmark.** Use an uncapped measurement consumer with no FPS duplication/
decimation. Do not move full video over UART or add per-frame full CRC/copy work
that would materially throttle the reference measurement. Prefer low-overhead
completion/encoded metadata and bounded independent decode; no saved footage.

If safe original entry/retirement requires reopening the former broad stock ABI/
frame-witness investigation, stop and review that cost. Do not disable its refusal
gate or start the full retail app to get a number. Unknown stock FPS does not block
profiling our already-working pipeline. Stock speed is neither an assumed minimum
nor necessarily the silicon maximum; keep the15fps engineering goal separate from
any measured factory claim and review feasibility with the user.

**Exit:** useful actual cadence/profile or an explicit measurement HOLD, plus one
pipeline difference worth applying in our driver. Profile our capture/CRC/copy/
encoder/network stages on the frozen modern baseline regardless.

### SRX-04 — Motor zeroing, phase geometry and STOP semantics

This question crosses **application/library/kernel** boundaries. A function named
RESET or zero may reset only a software counter, perform a bounded seek, or drive
to a hard stop. None is assumed. Existing board5 limit inputs are compiled-1;
that is evidence to investigate open-loop homing, not proof of its implementation.

Follow the selected self-check/zeroing caller into the relevant motor operations:
phase4 versus actual phase8 table, steps/cycle and travel conversions, origin and
centre/backoff assignment, speed/ramp/dwell, axis order/queueing, STOP/deceleration,
timeout/retry, cancellation and final coil state. Determine what establishes home:
limit switch, other feedback, deliberate hard-stop overtravel, elapsed steps/time
or merely assigning a counter. Check which settings/presets it would persist.
Do not infer the unknown eight-phase bytes from a familiar generic half-step table.

Produce an **own host model with mocked motor outputs** for the command sequence
if useful; never execute original firmware binaries on the host. Distinguish
compiled2400/600/default phase4 from app/stored2000/700/phase8 and doubled totals.
Those numbers are not safe measured travel. Retail STOP can decelerate/wait and
start a queued other axis: no assumption that returning0 means immediate coil-off.
Do not reuse stock timer assignments in our kernel's different timer allocation.

**Initial scope is static/read-only only. No live zeroing.** Current modern GUI
remains capped at one±4-step request per click; do not chain requests into homing
or bypass the cap. If stock zeroing is blind hard-stop travel, the recommended
first-build policy is manual reference/unknown absolute position, not copying it.

**Separate possible live proposal:** only after the algorithm, actual phase/
geometry/rate, bounds, other-axis queue state, abort/coil release and side effects
are understood. Bring a new one-axis experiment to the user, with physical
clearance, stall/current/thermal risk and an independent power-OFF recovery plan.
If its hard-stop behavior cannot be justified safely, do not run it. No full app
self-check, bypass-limit command or unreviewed full-range motion in this plan.

**Exit:** zeroing semantics and go/no-go for our own homing design. Mechanical
origin/limits require physical qualification, not a successful software reset.

### SRX-05 — Only the remaining audio-provider facts

Reuse scalar microphone/clap evidence, known nodes/gains and the successful
GPIO1-low speaker route/5s refill. Do not repeat speaker-presence checks or gain
sweeps. Resolve only the active ADC2/DAC/HP/mic-bias and shared reference/clock
sequence, actual parent/divider/OSR, L2 allocation/IRQ/cache and stop/drain/close
fences needed for modern capture **and playback**. Reuse SRX-02's shared L2 table.
Explain calculated8012 versus requested8000 without calling it measured clock.

Prefer retained exact code and source-safe cached/resource observations. If an
active scalar sample is essential, propose it separately, one bounded mode with
reviewed recovery: no raw voice, recordings, audio hashes or voice playback.
Speaker/GPIO1 effects remain denied while both UART signals are connected; a
separate physical isolation/control/recovery handoff is required before any such
test. ADC1 ambient is not ADC2 audio; no conversion/polling just for this audit.

**Exit:** real provider implementation inputs; remaining ALSA notification/lifetime/
playback integration is our coding/qualification work, not more stock archaeology.

### SRX-06 / SRX-07 — Conditional questions, not extra campaigns by default

**AP:** if protected wireless setup remains required, inspect advertised interface
types and the selected stock firmware/driver's role, beacon, client-table, key and
power-save support. Family capability strings or AP advertisement are not an
authenticated usable AP. Existing AP prototype is beacon/probe-only and explicitly
rejects clients/keys; do not promote it by removing those refusals. Deliver a
specific go/no-go/unresolved decision. No hostapd, role change, channel switch,
new SSID or RF beacon trial in initial approval. Any later AP trial needs its own
approved owned SSID/channel/endpoints; the current VertexHotspot-only restriction
does not authorize broadcasting a new SSID or impersonating the existing AP.

**SPI/NOR:** reuse the verified8MiB acquisitions, actual partition/alias map and
preserved current ROOT composition. Do only a source/read-only controller/boot
audit if a concrete missing fact blocks the chosen flash/config/update design.
No new dump merely to reconfirm size, and no erase/program/saveenv/OTP/MAC/account
change. Kernel image fit/boot packaging is our engineering task, not a reason to
experiment on flash. Persistent installation stays separately scoped/approved.

## Execution sequence if approved

1. **Freeze evidence/current modern rollback** and an answered/unanswered checklist.
   Read-only source work only: at most two disjoint lanes, Wi-Fi/USB/AP and MCI/L2/
   audio; parent motor/FPS selection/review. Reuse reports before opening new code.
2. **One minimal stock baseline/resource window**, only after fresh actual owner,
   runtime/image/provider/startup/recovery admission. Original3.4 modules belong
   only in original3.4, never loaded into6.18. Preserve the password-updated SSH-only
   ROOT; no additional persistent patch. Inspect hotplug/autoload/mount side effects
   before providers. Kernel idle-feeder60s is not our modern5s guard; GPIO0 button
   is not hang recovery. Physical OFF/fresh private UART catch is retained.
3. **Benign Wi-Fi boundary comparison** on the authorized AP/owned host, only for
   a named unresolved difference; no repeat of the entire completed test matrix.
4. **Separate finite stock media window if admitted.** Do not combine it with
   motor/audio/card stress or weaken retirement safeguards to obtain FPS.
5. **Optional card/homing/AP effects require another review** after source findings
   and any physical/card dependency. They are not silently queued by steps1–4.
6. **Close the reference campaign and build our coherent image.** Keep known-good
   gated capture and recovery as rollback; package exact paired modules/DT/init/
   tools, and leave unfinished MMC/audio features explicitly disabled until native
   provider implementation is ready. No implicit upstream rebase while diagnosing.

Current/expired20a PIDs,04:12UTC capture deadline, GUI epochs and IP addresses are
historical evidence, not authority for any proposed stock session. This review
has not queried or changed the target. No automatic execution on approval of only
part of the plan; each selected increment still needs its exact runtime gates.

## Evidence and stop rules

Each work packet produces: question; reused source/evidence pins; added reads or
commands and side effects; exact tool versions/timestamps; observations versus
inference; implementation decision; unresolved facts; and **one next task**.
Keep UART/config/images/IDs/keys and acquired data private; local Git is not
publication authorization. Original acquisitions, failed tests and unsafe old
error paths remain preserved. No card footage/recovery-file access or repair.

Bound source expansion to the named selected call chain. If a new unanswered
function is necessary, name the dependency before expanding; no whole-SDK/kernel
reconstruction. Bound trace buffers/counters and account for overflow/overhead;
no per-step/per-packet printk. Missing stock trace support is a stopping boundary,
not an excuse for an instrumentation framework. Never poll a read-clear register
as a passive observer, force-unload unknown DMA, free retained backing after a
timeout, or repeat an uncertain effect. Preserve HOLD and use reviewed recovery.

### Review recommendation

Approve **source closeout and source-safe stock resource/network observations**
first; admit the minimal FPS measurement only when its entry/stop path passes
review. Keep real homing, AP broadcasting, card writes/removal/power-loss tests and
speaker effects excluded until their separate proposals. No further stock tests
of already-confirmed lights, reset input or IR-Cut mapping are recommended.

Reference anchors: `STOCK_REFERENCE_CAMPAIGN_20261013.md`;
`docs/qualification-plans/stock-reference-20261013/ACTUAL_RESULTS.md:948–1029`;
`bringup/qc07f/mmc-candidate/SOURCE_AUDIT.md`;
`bringup/qc07f/audio-candidate/SOURCE_AUDIT.md`;
`docs/qualification-plans/stock-reference-20261013/motor-map-source01/CONTRACT.md`;
`motor-map-source02/CONTRACT.md` in the same directory;
`work/lightbulb-camera/foundation-20261011T041148Z/analysis/radio-ap-prototype01-report.md`;
`docs/qualification-plans/qc07f-performance/COST-AND-MEMORY.md`.
