# RAM75 entry after23h — offline method findings

2026-10-23; BASE-01/02/05/07, PLAT-01/06/08. **BLOCKED before controller
implementation or native selection.** Exact retained RAM75 has suitable static
staging/layout evidence, but no admitted whole-operation recovery/clock coverage.
No source implementation, helper import/call, test, build, native ELF execution,
hardware/current-state/private-evidence access, Git/index or central-doc edit here.
Only this fresh directory is authored. Parent alone owns hardware and disposition.

## Current boundary, not a lease

[COLD23H_RESULT](../execution-20261023/COLD23H_RESULT.md) reports one unchanged-v4
PASS:596RX =360-byte/19-record cold profile +236 controlled echo bytes;57TX from
three19-byte catch packets; separate10s unchanged observation, actual ON/OFF
acknowledgements/GETs, owned stop/wait0/final hashes and independent OFF/absence
postchecks. Camera ends controller OFF, no recorder/current Linux/watchdog.
23h cannot be continued: v4 always ends OFF/finalized. Earlier radio/reset/23c/d/e/g
HOLDs remain independent.23c was caller parsing misuse;23e/g extra bytes preceded
classification. Clean23h neither explains those bytes nor proves an electrical fix.
No repeated wiring/load/path question, physical request or hardware queue follows.

## Exact image and static memory baseline

Abbreviations, all workspace-relative:

- `F = work/lightbulb-camera/foundation-20261011T041148Z`
- `A = F/artifacts/native-security75-source01`; `K = A/linux`
- `B = F/analysis/native-security75-build01`; `M = K/arch/arm/mach-ak3918ev200`
- `I = work/lightbulb-camera/implementation-20261008T205404Z`

Read-only Python3.14.8 standard-library metadata inspection (`python3 -I -B -`,
two inline invocations retained in the session tool transcript) freshly hashed
selected public files, decoded the legacy header/DT tokens/newc metadata in memory,
recomputed CRCs, compared actual payload bytes and read actual ELF symbols.
No archive member was extracted/followed or binary executed. Selected exact pins
are in [PUBLIC_PINS.tsv](PUBLIC_PINS.tsv); they are evidence, not an admission API.
A separate final read/hash comparison checks those pins, not target behavior.

| Item | Fresh result |
|---|---|
| Selected image | `A/images/QC6S75.UIM`,3,574,675B, SHA256 `5e37b09c9dfda7c87efab998d2ce3484dc4ff152d6746563560ba745a7e18ec1` |
| Legacy header |64B, magic`27051956`, timestamp0, Linux/ARM/kernel/NONE `(5,2,2,0)`, payload3,574,611B; load`0x82408000`, entry`0x82408040` |
| Header/data CRC | Reflected IEEE `615cb843` / `fb8fa56f`, independently recomputed; outer NONE wraps an internally XZ-compressed ARM zImage |
| Whole-image CRC | Vendor/QCL CRC32/BZIP2 `a0db4e80`; reflected whole-image `4ce6a911` is **not** the vendor command expectation |
| Payload association | Byte-exact `B/build/arch/arm/boot/zImage`3,572,040B + `A/inputs/attempt45.dtb`2,571B; zImage magic`016f2818`, start0/end`00368148` |
| Actual RAM | DT40MiB `[0x81800000,0x84000000)`; U-Boot64MiB text is identity only |
| Staged image | `[0x82408000,0x82770b93)`; ceiling`0x82800000`,586,861B static headroom |
| Decompressed kernel+BSS | Actual `vmlinux` `_text=c0008000`, `_end=c0855bc0`; translated using config PHYS_OFFSET`81800000`/PAGE_OFFSET`c0000000`: `[0x81808000,0x82055bc0)`,3,875,904B below staging |
| Loader | `I/artifacts/uart-ram-loader-wordaligned.bin`,1,872B, SHA256`a1630c90f7f44666e941704778cbf119e5e2e13355113adca52f842bd9046ad1`, BZIP2`dd0572e1`; `[0x82001000,0x82001750)` before Linux decompression, not a retained Linux owner |
| Initramfs | Retained cpio50 equals the actual absolute config input and decompressed built-in gzip; embedded `/init` equals retained174,248B monitor SHA`8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899` |

`package75.py:65–98`, actual config/build/package manifests and
[PACKAGING_REVIEW](../kernel76-integration-20261020/PACKAGING_REVIEW.md) agree.
These are static staging/decompression extents, not combined-load/CMA/headroom,
electrical memory or app acceptance. Config CMA default is0; the historical
RAM-feature bootargs explicitly supplied16MiB at`[0x82800000,0x83800000)`.
The prior fixed volatile string was
`earlycon console=ttyAK0,115200n8 rdinit=/init loglevel=8 panic=0 cma=16M@0x82800000-0x83800000`;
DT's default includes `keep_bootcon` and is not a substitute for exact set/readback.
No boot command is selected here.

Config/build75 retains Clang/LLD23.1.1, Linux6.18.55 and upstream commit
`725bd2f3c81d54edccb66fead01d4c0c222e2231` **plus retained overlays**, not upstream
alone. Existing source/build inventories remain authoritative; this lane did not
rehash the whole tree or rebuild source-to-binary provenance. Current monitor
source audit plus embedded retained-artifact equality is not a fresh reproducible
monitor build. Later NEW23.1.3/query77 builds do not reproduce this compiler pairing.
No external module is selected; same dirty uname/vermagic would not prove pairing.
Embedded `/hgics.ko` is stale, not station75; embedded `/bin/rtspd` is stale too.
Neither is loaded/executed automatically by PID1. Query77/78 stays parked/not adopted.
This image exceeds the1.5MiB KERNEL partition: **RAM only, never flash/install**.
No persistent layout, shadow source-openness or product memory claim follows.

## Actual startup effects: read-only observation is not a write-free boot

The actual DT enables the following built-in probes; audited retained sources:

- `M/i2c.c:175–235`: controller clock/reset,125–130ms reset settle, pin27/28
  mux/pulls and adapter registration; no automatic sensor/expander transaction.
- `M/sensor-bootstrap.c:19–60`: GPIO41 active-low reset assertion, inherited-PLL
  divider/MCLK update at SYS18,30ms assert then20ms deassert settle. No sensor
  register table/stream command is automatic.
- `M/usb.c:270–333`: GPIO42 LOW2s/HIGH100ms, USB host clock/controller setup and
  enumeration. **Radio power/reset and USB traffic occur**, although no station
  module, radio firmware/RUN, association, WLAN or network application is selected.
- `M/isp.c:1509–1571`: allocates retained table/frame/reference DMA backing and
  exposes diagnostics; no capture at probe. This is not allocation-free idle or a
  native128KiB-cap/combined-media-budget qualification.
- `M/ptz.c:122–142`: validates metadata/registers diagnostic groups, no phase GPIO
  requests/writes at probe. No homing/motor actuation is automatic. GPIO provider
  requests validate existing mux fields rather than silently remuxing neighbours.
- `init.c:465–535`: mounts devtmpfs/proc/sysfs, opens/duplicates console, attempts
  startup watchdog stop, prints help, then dispatches explicit serial input only.
  It does not autostart ash, retail app, radio load, sensor stream or media.
  Config disables MTD/SPI/MMC/SOUND; no flash/card mount/write path is selected.

Thus a later parent selection must explicitly accept these **existing baseline
probe effects**, not describe the whole boot as physically inert. Requested inert
scope means no additional station/radio-load/association/RTSP/sensor-stream/motor/
GPIO-output/speaker/mic/storage activity. No startup-material/provisioner/RNG/app
candidate executes in this baseline. Do not add an image/module/firmware build.

## Watchdog/recovery phase boundaries

| Boundary | Source-supported protection and gap |
|---|---|
| Future ON/catch, held U-Boot, BOOT pin and468 loader-word stores | No source-proven autonomous recovery guard. Catch is host/recorder dependent, not a power cutoff. **Earliest uncovered interval starts at future ON**; even after a good cold catch, the first native-extension BOOT read/staging remains uncovered. |
| GO/READY | `uart_ram_loader.c:48–52` refuses an already-enabled TOP watchdog; READY/first-byte busy-wait is explicitly **unarmed**. Prearming TOP before GO is incompatible, not a ready-made solution. |
| QCL first byte to commit | First byte (except ESC cancellation) arms nominal15s; complete received frames feed it, including NAK/duplicate paths. Header/frame waits/halt loops then rely on that timer. `stop()` disables it **before** COMMIT_OK/return (`:91`). No fresh effect/retry is authorized by this support. |
| Independent image CRC/bootargs | QCL timer is already stopped. The independent full-image vendor CRC, environment set/readback and return-to-CLI gap are unguarded until the separate boot-start stores. |
| Historical boot-start guard | Fixed SYS`e4=552aea54`, then `e8=aa000003` represents nominal60s at46,875Hz. This is boot-transition-only source precedent, not permission for blind MMIO/pet/reset/rearm here. Fresh current catch/recovery admission must precede any such reset-capable action. |
| Kernel handoff | `M/watchdog.c:97–129` detects inherited control bit0; core can feed it. Actual config HANDLE_BOOT_ENABLED=y/OPEN_TIMEOUT=0 means pre-userspace feeding can continue without finite userspace-open deadline (`watchdog_dev.c:65–126,1091–1107`), not a bounded host-loss/PID1-stall guarantee. No all-domain recovery proof. |
| PID1/idle | `init.c:292–299` opens watchdog, writes`V`, closes it, prints success **without checking write/close returns**. Healthy stop removes timer; message alone cannot prove it. Idle loop never owns/feeds a guard. Startup failure may leave recovery unknown, not safely stopped. |
| Known-idle reset/smoke | Separate explicit monitor commands with own effects; cannot guard earlier staging/boot. No reset selected. A watchdog reset needs a fresh unconsumed catch or may return to persistent boot policy; no automatic retail-app/homing fallback is acceptable. |

Same-host HTTP outlet control has demonstrated one ON/OFF + actual GETs. It is
**not independent host-hang/suspend recovery**, a rail measurement or a watchdog.
Future failure cleanup may attempt its one bounded OFF/readback, but Python
`finally`, host timeouts/sleeps and a recorder reserve do not run through host loss.

## Unchanged admission requirements; no executable phase contract selected

1. A future complete foreground transaction needs its **own** confirmed OFF,
   exact sole adapter/owner, original recorder source/peer/interpreter/dependencies,
   protected fresh outputs, actual owned `Popen` retained through wait, zeroRX/TX
   before and after original arm, exact whole cold/trailer accounting and separate
   quiet interval. Preserve all bytes/events; no trim/window-reset/reanchor/retry.
   No historical receipt/PID/catch/23h quiet interval supplies live authority.
2. BOOT callback pin requires original `[0,0x60000)` read into zeroed/zero-CRC-checked
   scratch `[0x82408000,0x82468000)` and exact fresh read/BZIP2`452a21fe`, not stock
   KERNEL/ROOT/private dump acquisition. Required source-neutral read schema from
   controller-design SOURCE_ANCHORS/cold-gate precedent: SYS74`02000006`, SYS78/7c0,
   GPIO DIR`20170000`/OUT`2017000c`0, INPUT`20170018 &008000c0=0`, timers
   SYS`c8/d0`0; GPIO0 stays input/UART mux preserved. Mismatch means HOLD, **not**
   old RAM75 recipe's pair DIR/mux repair stores. QCL additionally checks TOP
   control`080000e8` bit0 clear. Readback requirements are not new MMIO commands.
3. Source transports are references only: `Boot.command` requires echo before CR,
   <=64B text with25ms interchunk gap; protected send<=128B, QCL<=96B payload and
   15ms postframe pacing. For this exact image:37,237 frames, final19B;4,021,535B
   binary TX including header/frame headers/QEND, excluding text/catch. Fresh
   unique ACK/DONE/COMMIT/held-prompt and **independent full3,574,675B RAM CRC** are
   distinct completions. Inherited `planned=2294.22s` excludes earlier word staging
   and is not a measured complete-phase cap. V4's120s child is inadequate.
4. Retain original immutable deadline/earlier-only caps and30s transition +120s
   recovery observation reserve. Strict C1/no-suspend/no-clock-discontinuity policy
   stays unadmitted ([clock contract](../encoder-query77-clock-contract-20261022/CONTRACT.md),
   [parent block](../encoder-query77-continuity-feasibility-20261022/PARENT_DISPOSITION.md)).
   Brackets, equal endpoints, elapsed sleeps, detection, cooperative inhibitors or
   a fresh record do not prove prevention throughout the interval. No C1 guard or
   policy is waived merely because this image previously booted or v4 observed23h.
5. `Boot.caught` banner-tail search is not v4 whole-context admission; `Boot.arm`
   cannot infer a consumed catch still exists. Its five not_sent/four ACK attempts
   are not admitted retries. `PrivateBoot.command` is ROOT-only and excludes Linux
   boot/idle; no subclass/global patch/import/owner bypass makes it a RAM75 profile.
   Historical HOLDs/marker bytes/source pins must remain read-only and independently
   postchecked on normal and failed exit, not replaced/cleared to reuse a class.
6. Actual Starting-kernel transition alone is not Linux completion. Future fixed
   RO idle evidence must bind new boot/release/cmdline, PID1/UID, mounts/modules,
   memory/CMA/ISP ownership and UART errors, plus actual watchdog state and process/
   FD ownership, with before/after sameboot identity and classified asynchronous
   output. Monitor lacks a full process/FD inventory primitive; any needed shell
   route needs separate fixed read-only source review, not arbitrary commands.
   Do not import old shell command framing/refreshes as generic acceptance.
7. Any unknown/failure stops subsequent TX; no second ON/catch/upload/boot/reset.
   End OFF/readback **before** same-peer owned stop/actual wait0/final hashes and
   PID/socket/fuser absence checks. If OFF is unknown, retain recorder/evidence,
   report manual-OFF need; do not kill/unlink/claim cleanup. No outside timeout or
   second status/UART poller during the owned transfer. Successful finalization is
   not an independent power-failure recovery mechanism.

## Precise block and one next task

**No source/controller selection is justified now.** Minimum recovery precondition
is a real, separately reviewed path covering future ON through final OFF,
including unarmed U-Boot/staging/READY, commit-to-boot and post-magic-close idle.
The smallest nonimplementation option later is an actually present operator with
independent physical OFF control for those gaps; unattended authorization is not
that readiness, and no operator is claimed present/requested here. An autonomous
alternative must actually cover those same gaps; this lane does not invent one.
Even actual manual readiness would **not resolve the separate strict C1 clock
block**: existing evidence does not establish its whole-interval property. Only
real sufficient evidence or a separately explicit parent/user policy decision
could change admission; neither is supplied or selected here. More receipts or
another clock/watchdog framework is not the next task.

**One next eligible OFFLINE FEATURE proposal, parent selection still required:**
connect accepted `qc07f_rtsp_os_random_ready_v1` to the actual bootstrap config
**before** provisioner snapshots (`bootstrap.c:19–31`), preserving unchanged
snapshot/bind order. Actual runtime requires `random_ready`; real consumers request
32B (`candidate_auth.c:99,122–131`). The callback ignores context, refuses errors/
short results, never retries/falls back/caches. Bounded source-only caller
registration can remove this concrete feature-wiring gap without executing RNG,
credentials/provisioner/bootstrap/listener/media/native code, adding a main, or
claiming native entropy/recovery. See the existing OS-random parent disposition;
do not mutate a live/snapshotted config or synthesize readiness. This is a proposal,
not an automatic worker/controller/hardware queue.
