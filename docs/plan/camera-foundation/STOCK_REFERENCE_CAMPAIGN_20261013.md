# Stock reference investigation — GPIO, factory FPS and Wi-Fi MTU

**Execution update2026-10-13:** user authorized tests; controlled unchanged stock
root rescue is now running. See [actual module results](../../qualification-plans/stock-reference-20261013/ACTUAL_RESULTS.md).
Native stockMTU1500/2048 firmware download/finite packet+TCP integrity passed;
read-only card metadata and driver/GET/configuration data collected. GPIO5/23
readback changed/restored but user saw no visible light change. Sensor-based
off/on/off comparison is now required; feed entry is under audit, no streaming yet.
Initial review/entry-decision text below remains historical context.

User steering2026-10-13: investigate how the actual stock kernel/application drives
motor/visible/IR outputs, optionally use a root-access stock boot with GPIO tracing,
and establish factory frame-rate and Wi-Fi MTU behavior before deciding replacement
targets/fixes. **Current activity is static review/planning. No stock boot or output
command has been executed in this resumed session. Camera is held at U-Boot.**

This is an early evidence-gathering dependency for CTRL/IMG/NET/CAP/QUAL, inserted
before the next replacement-media experiment. It does not change the QC07F-first
roadmap, authorize new persistent patches, or turn family capabilities into facts.

## Existing evidence to reuse

- Exact retail kernel/app REA is already retained. See
  `docs/QC07F_RETAIL_ILLUMINATION_ROUTING.md`, including subject hashes and Evidence
  IDs. Board5 app LIGHTS selects `/sys/user-gpio/gpio-light` → GPIO5; IR selects
  `gpio-ir` → GPIO23; GPIO2 is separate. Raw setters pass values through and their
  return is not electrical feedback. Active polarity, optical channel and current
  still need correlation. GPIO6/7 IR-cut entries do not prove a fitted actuator.
- Motor/AW9523 modules and compiled routing exist; trace the selected board branch,
  ioctl/callers, phase order/timing, move/STOP and init/homing effects before choosing
  a command. Do not sweep pins or infer position from phase counts.
- Previously verified stock root access changed a root-filesystem authentication
  fragment, **not the stock kernel**. Root access and GPIO instrumentation are
  separate questions. Preserve password/provisioning privately; do not repeat
  passwords or credential hashes in new reports.
- Historical stock rescue used volatile `uart_debug=1 init=/bin/sh`; its source
  plan is `work/lightbulb-camera/password-update-20261008T152221Z/analysis/stock-rescue-experiment-plan.json`.
  That is precedent, not current image/environment/boot authority. Exact current
  image, ranges, command line, watchdog and recovery must be freshly verified.
- Qualified replacement station uses MTU384 as a workaround. The known2048-byte
  radio payload/2052-byte USB path fails on the replacement host; original256-byte
  radio loading and MTU384 have narrow native evidence. **384 is not an established
  hardware or factory limit.** Standard1500 is a candidate, not yet a measured
  factory setting. A targeted retained stock-log scalar search found no MTU result:
  `evidence/qualification/qc07f/resume-20261013/stock-MTU-retained-runtime-scalar-search01.json`.

## Factory-FPS static findings — reviewed2026-10-13

Exact shipped SC1346 REA initializes `g_fps=to_fps=30` and computes printed pixel
clock as initialVTS × initialHTS ×30. This is a derived software assumption, not an
independent clock measurement. The retained HTS1800/VTS750 table does not establish
active app selection/delivered cadence; replacement HTS1650 was a separate readback.

| Question | Established evidence / limitation |
|---|---|
| Retail sensor initialization label |30; exact module REA, not measured output |
| Factory main/sub profile FPS/bitrate/GOP and overrides |Unknown; board5 app registration/default/stored-setting precedence still needs tracing |
| Factory night/AE FPS policy |Unknown; setter/exposure machinery is recovered |
| Factory source/encoded/client frame rates |Unmeasured |
| Public reference app |1280×720/25fps/2000kbps CBR main and640×480/25fps/200kbps CBR sub; not retail factory proof |
| Replacement requirement |720p15 remains provisional; no silent revision to25/30 |

Requested15 uses divisor1428, requested13 uses1250; other values use request×100.
With initialVTS750, these produce integer VTS1575 and1800. **Conditionally**, if
initial cadence is30fps and clock/HTS unchanged, physical cadence is about14.286
and12.5fps respectively. Pending VTS is applied by exposure update. The software
label/getter alone does not prove requested rate was physically applied.

Parent inspected these retained REA outputs (not compilable/original C source):
- `work/lightbulb-camera/driver-re-20261008T110053Z/contracts/pseudocode/sensor-sc1346-ko/021-sc1346_init.c.txt`,
  evidence `ev_3f9abe347433b87acd1d492c4869c22082eb2f00c5d4a02b7b56024bf1fba23c`.
- Same directory `011-sc1346_set_fps.c.txt`, evidence
  `ev_2dd140a22adc32c84da1f6266b1c271d3780f072814aa9ecf022d893080d8a68`.
- `contracts/pseudocode/libakstreamenc-so-remaining/181-VideoStream_Enc_set_fps.c.txt`
  copies caller fields, not measured cadence; evidence
  `ev_62c0b126805357d400077edec2cfd430d8049cfafc4b6b61be95edb2771cb0fe`.

Stock sensor getter ioctl0x8004497b returns a four-byte software label. No verified
existing shell utility or bounded app capture invocation was established; don't
invent a command from the ioctl/function name. Proposed initial measurement once
the source/stop path is admitted:5s settle +10s capture,20s whole-run limit;
daylight main first, separate low-light run, substream only where needed. Report
`(N−1)/(t_last−t_first)`, wall throughput, source/encoded/decoded unique frame counts,
duplicates/gaps independently. This is a proposed campaign, not executed evidence.

## GPIO/motor and instrumentation findings — independent static review

Exact board5 selects direct GPIO, not the board3 AW9523 motor branch:

| Retail device | Compiled phase-pin order | Compiled travel field |
|---|---|---|
| `/dev/motor0` |10,11,19,20 |2400 |
| `/dev/motor1` |15,14,13,24 |600 |

Limit inputs are−1; runtime app configuration can override travel/speed/phase count.
Four-phase bytes03/06/0c/09 are recovered; the actual effective eight-phase sequence
is not established by this review. Neither compiled travel nor lack of limit inputs
proves physical limits or axis orientation. The timer handler writes four pins
sequentially, with acceleration/deceleration altering update cadence.

Retail STOP0x40046d42 may set a flag or request a deceleration tail and wait up to10s;
its zero return is not physical STOP proof. Completion can start a queued other
motor under the mutex policy. Keep other axes unqueued; establish actual coil-off.
Exclude bypass-limit0x40046d41, RESET/homing0x40046d60, cruise0x40046d62 and full-range
0x40046d63 from initial bounded trials. The public motor demo is **not compatible**:
it uses `/dev/ak-motorN` or `/dev/akmotorN` and ioctls11–16, unlike retail `/dev/motorN`
and the recovered0x40046d.. ABI. No verified safe retail high-level CLI was found.

GPIO5/23 illumination paths go through vendor `ak_gpio_set`, which can change mux/
pull/latch/direction. Motor phases use `ak_gpio_setpin` → `g_ak39_gpio_setpin`, a
direct bank-word RMW path. Generic gpiolib tracing would miss those writes. GPIO2
indicator handler is unresolved; KEYLED can touch GPIO0 and is not a substitute.

Retained retail symbols export GPIO functions but do not establish kprobe/ftrace
interception. Public family defconfigs disable KPROBES/FTRACE/IKCONFIG; these are
not the extracted retail config. Equal `__kprobes_text_start/end` addresses and
an exported kallsyms lookup are not proof of a usable GPIO logger. Root access
does not itself create a logging hook.

If instrumentation is needed, the review target is the actual vendor GPIO write
path and relevant mux/direction paths, preserving original writes/order/returns.
Proposed bounded logger: selected-pin filtering, preallocated256×32B records,
overflow count, IRQ-safe allocation-free recording after the original write using
already available values, drain after the trial. Do not add MMIO reads or per-step
printk, alter timer writes, or move motion into a logging worker. Exact ABI/hook
mechanism and timing overhead must be reviewed/qualified before a stock trial.

Retained authorities: `contracts/board-wiring-candidates.json`; kernel board5
`112-ak3918_init_machine`; motor `001-motor_stop`, `005-motor_hw_timer_handler`,
`009-motor_move.part.8`, `010-ak_motor_probe`, `011-ak_motor_ioctl` contracts;
kernel GPIO `005-g_ak39_gpio_setpin`.
Review sessions: `ses_eca7e895dffeKDbFQstApxsWfq` (motor/light/trace) and
`ses_eca7e894fffeyirs0hI4r6l6to` (FPS). These are source reviews, not native acceptance.

## Controlled stock startup details

Preserved scripts show `/etc/inittab` → `rcS` → `rc.local` → `camera.sh setup` and
`service.sh start` → `anyka_ipc.sh start`. They are evidence, not a recipe to execute:

- `rc.local` mounts `/etc/jffs2`, loads modules and starts application services.
- `camera.sh` loads MMC/watchdog/camera/sensors/AW9523/motor/USB-network components.
- `anyka_ipc.sh` mounts the footage card, may use card debug inputs and redirect logs
  to SD, starts the application, and touches persistent error state. Its `stop`
  branch explicitly does not stop the service. Do not rely on that as bounded STOP.
- Retained evidence reports normal stock recording. Stock app auto-start may also
  change lights, run homing or bring up networking; determine exact behavior first.

Use a verified minimal root/rescue session with automatic app/recording/network
startup suppressed. Preserve calibration/config inputs without exposing credentials;
use read-only access and owned RAM copies where needed. Launch only reviewed
components. If representative factory behavior requires broader startup, isolate
storage/network/actuator effects explicitly before doing so. No arbitrary original
boot-script execution, existing-footage write, cloud contact or new flash patch.

## Reference campaign sequence and outputs

### R0 — Static contracts and instrumentability

Trace actual board5 motor/light handlers and stock video/network setup. Record
exact image/module/config/library hashes and offsets from existing evidence.
Establish which built-in commands are genuinely present and what they change;
if no safe command is verified, do not invent a CLI from a function name.

Inspect the stock kernel configuration and available tracing/exported symbols:
ftrace/kprobe/dynamic-debug/debugfs cannot be assumed on Linux3.4.35. Determine
whether the stock code uses generic GPIO APIs, Anyka direct-register functions,
an I2C expander, timer/PWM or userspace MMIO. A generic gpiolib logger could miss
the actual path. If extra instrumentation is necessary, prefer a small reversible
RAM-only, exact-ABI approach. Root privileges alone do not establish module ABI.
An instrumented kernel rebuild must preserve the retail drivers/config/memory
reservations and be separately sealed/recoverable; it is not a cosmetic patch.

### R1 — Protected stock baseline, output commands disabled

Freshly validate UART owner, held U-Boot, exact current stock image/root access,
volatile boot policy and bounded recovery. Capture the stock boot and read-only
board/sensor/radio/module configuration in a fresh evidence scope. Do not start
the main app until its unavoidable outputs/writes/network effects are understood.
Read runtime GPIO configuration/latches through proven passive paths only;
some stock helpers named GET/"read" perform SET first.

### R2 — GPIO/motor/white-light/IR correlation

1. Establish stopped outputs and actual physical clearance/reference; log idle
   mux/direction/latch and any expander state without stealing ownership.
2. Trigger one audited existing control path at a time: bounded move on one axis,
   STOP, then the other axis; white-light off/on/off and IR off/on/off separately.
3. Trace driver entry arguments, selected board branch/pin, phase values, requested
   delay/PWM and STOP completion, plus final register state. Keep unrelated GPIO
   bits intact. Do not call homing, absolute presets or unsupported IR-cut paths.
4. Correlate trace with user-visible motion, direction/optical observations and
   measured current/polarity where required. A GPIO write proves software intent;
   physical STOP/light emission needs independent observation.

Tracing must be bounded and rate-limited: fixed buffer/counters and selected events,
not per-phase unbounded UART printk. Verify trace loss/overhead; GPIO readback and
debug timestamps can perturb motor timing. An unknown stop state ends the experiment
through the qualified recovery path, not repeated output toggling.

### R3 — Factory profile and actual video cadence

Record separately: compile-time defaults, stored profile selection, active runtime
sensor/encoder requests, sensor clock/HTS/VTS, exposure/night auto-FPS policy,
resolution/bitrate/GOP/codec settings, and observed frame delivery. Main/sub profiles
must not be confused with physical cameras or sensor maximum capability.

Measure a finite daylight/unclipped stock run, then a distinct low-light policy
run if safe/available: source frame counters/timestamps and independently received
complete unique frames over measured elapsed time. SPS/VUI/RTSP advertised FPS
alone is not throughput. Use fresh owned off-board capture, not existing SD footage.
Report capture FPS, encoder output FPS and client decoded FPS separately. Use this
to refine feasible requirements;15fps remains a provisional engineering target
until evidence and an explicit requirement decision support changing it.

Compare factory cost/ownership/copy/scheduling with replacement measurements,
including the historical99.201ms/frame CRC_PRE+POST cost. Learn from stock without
removing required integrity/retirement checks just to match a reported rate.

### R4 — Factory MTU, packetization and transport behavior

In a source-audited stock session, record runtime interface name and MTU before
and after normal association, along with firmware/version and driver settings.
Read `/sys/class/net/<verified-interface>/mtu` when present; use the actual stock
interface tooling otherwise. Record scalar MTU/configuration only, not PSKs,
Authorization, private identifiers or wholesale configuration dumps.

Correlate MTU with driver maximum packet size, USB endpoint maxpacket, command/data
headers, aggregation, short/zero-length packet rules and actual transfer lengths.
These are different quantities: an Ethernet MTU does not equal a firmware-download
fragment or a USB bulk maximum. Compare stock `otg-hs`/HGICS behavior against the
replacement MUSB host at64/512-byte boundaries and multipacket lengths.

Any network test uses only authorized VertexHotspot/owned bench endpoints, with
finite transfers and data-integrity checks. Confirm bidirectional delivery,
retransmissions, errors and host timing. A factory1500 setting, if observed, is
evidence of the intended interface contract—not proof the replacement1500 path works.

### R5 — Apply evidence to one replacement fix at a time

- GPIO: map measured polarity/sequencing/timing/STOP into the existing shared owner
  and platform adapter; qualify physical effect and interference.
- Video: choose measured profiles, cost/ownership improvements and exposure policy;
  preserve both preview transforms and avoid tint compensation.
- Network: reproduce the exact framing/FIFO/toggle/short-packet failure, implement
  the narrow verified fix, then qualify progressive packet sizes and factory MTU
  under video/TLS/control traffic. Keep the named384 recovery profile until the
  replacement large-packet path passes. No blind global timeout or MTU increase.

Outputs are a stock-reference matrix, exact command/trace provenance, observed
GPIO/FPS/MTU results and an explicit measured-vs-inferred column. Fold these into
the completion strategy before further speculative hardware assumptions.

## Entry decision

Static review is underway. **No extra power cycle or stock application launch is
requested yet.** Kernel instrumentation availability, safe command exposure and
startup side effects determine the next exact bounded experiment. User optical/
motion observations and any necessary physical measurements will be requested at
that point. Parent alone controls UART/device operations and recovery.
