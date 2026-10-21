# RP-02 / NET-01–04 — paired station76 entry review

2026-10-21, source audit completed06:54UTC. **Current entry EXECUTION NO-GO.**
**Conditional SOURCE GO only for the small corrective increment in §7, followed
by independent exact-controller/module review. Nothing is queued.**

Parent alone owns hardware. This lane changed only this new report and
`station-entry-review/`. No Git, target/RPC/status, network, private inputs,
payload/image unpack, build, old collector/test binary or ARM execution occurred.

## 1. Decision / smallest next step

The exact embedded paired76 module **can attach to an already-running retail
APPLICATION without downloading firmware or repeating RUN** on its normal path.
It does not directly manipulate GPIO42, reset the USB device, clear endpoint halts,
launch WPA/DHCP/cloud, read station credentials, or deliberately initiate scanning
in module init. This is source support, not native attachment/RF-silence proof.

It is **not inert USB entry**: registering its USB driver immediately claims the
interface, allocates queues, submits32 continuously resubmitted RX URBs and queues
asynchronous FWINFO initialization. A successful module syscall/prompt alone is
not initialization completion or ownership retirement. The current `wifi attach`
guard closes after2s regardless of asynchronous completion/error; the module has
10s semaphore waits and autonomous error-triggered detect/re-init. There is no
reviewed76 station/transport controller that preserves the original nonempty HOLD,
owns the watchdog through asynchronous work, and proves final URB/work retirement.

**Do not execute current `wifi attach`, transplant75's helper, issue raw insmod,
or use `no_bootdl=1` as a safety proof.** Association and MTU1500 remain NO-GO.
Smallest next eligible task is SOURCE/HOST-only: one finite attach-only admission
controller plus narrowly corrected paired module (§7), no kernel rebuild needed
by design. This report does not authorize implementing/building that increment.
If parent elects not to change source, stop at this report; there is no presently
admissible station effect command to hand back.

### Preserved native meaning

`SEPARATE_IDENTITY21A_RESULT.md` / public `RADIO_SEPARATE_IDENTITY21A01*.json`
record06:43:20 separate strict APPLICATION + retained firmware identity + fresh
pre/post sameboot metadata/FD/full-idle retirement PASS, controller SHA
`c3c19a12bffb56eec2044e259793f58a7243e1ae20ac6fe8cf75ee3f3229ceeb`.
No module, RF association or network operation followed that observation here.
**Original151-ACK trial remains FAILED; its HOLD bytes/inode/path stay unchanged.**
These separate facts neither repair its interrupted ACK nor prove MODE/NAK/toggle
correctness. Last observed RAM76/application/UART-connected is dated, not current
admission. The old07:09UTC capture deadline is not a reusable protection lease.

## 2. Exact source / package / module binding

Offline invocation (already completed; exclusive outputs refuse rerun):

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-entry-review/inspect_station.py
```

`station-entry-review/run02/binding.json`: **98 static audit checks PASS**,67
station source files match preparer pins; Python3.14.7 and LLVM23.1.1 metadata
commands/versions, argv, hashes and timestamps retained. These are source checks,
**not host behavioral tests or native qualification**. `run01/` preserves a failed
overstrong collector assertion: the module DOES still link `request_firmware`
through retained maintenance code. Corrected audit does not confuse linked code
with normal-entry reachability. No production input changed.

Paths below relative to workspace; common artifact root:
`work/lightbulb-camera/foundation-20261011T041148Z/`.

| Binding | Exact evidence |
|---|---|
| Candidate RAM image | `artifacts/native-usb76-source01/images/QC6U76.UIM`,3603555B; SHA256 `bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f` |
| Selected archive | `artifacts/native-usb76-source01/inputs/initramfs76.cpio`, SHA `510e5fdc6e2a7b6ad4a52d82ad1de9381a85e143f7014a6f2cc3bdfb951e309f` |
| Actual paired module | `artifacts/native-usb76-source01/station76-final/hgic_smac/hgics.ko`,163720B; SHA `e667ed2a1f726c137330638f6ff64eeab085c0bc3247ed2cac1919594e29ef29` |
| Final config / symvers | `analysis/native-usb76-package01/build/{.config,Module.symvers}`; SHA `9e1f37788d3bd8580f2eb031ea2a7d795f4b61e2c1f89e1b91c11f1a87a6499c` / `806467d52330d4cdf2fd8e312aee54d24b0a1437d5eb4bd1b393011f277c4cdc` |
| Kernel identity | Linux6.18.55, source commit `725bd2f3c81d54edccb66fead01d4c0c222e2231` plus sealed overlays; only declared76 kernel delta `drivers/usb/musb/musb_host.c` SHA `76cc145a3493221ee263e9f14cfc94e3c6eda25ade0f41ad7b7d2963f7388bd5` |
| Package manifest | `analysis/native-usb76-package01/package76-manifest.json` SHA `b61a73e245cbac3da4a8a1560f7908ea7020796d87b5eacc1580514f983f2d37` |

Module/source/config/symvers were freshly rehashed. Image/archive/vmlinux bytes
were deliberately **not opened** in this lane, avoiding proprietary embedded
payload reads. Their transitive binding is cited from the pinned manifest and
`evidence/qualification/qc07f/kernel76-20261020/final-verify01/result.json`:
actual final kernel embedded selected cpio; final-output module equals embedded
and bootstrap bytes. `recipe-snapshot/package.py:127–142` records clean source
copy and final-output rebuild, not a merely matching uname. Fresh whole-image
verification was not repeated or claimed here.

Final build argv includes `ARCH=arm LLVM=1`, exact final kernel `O=.../build`,
`M=.../station76-final/hgic_smac CONFIG_HGICS=m CONFIG_HGIC_USB=y CONFIG_HGIC_2G=y`.
67 source pins include Makefile, compatibility header and all retained sources;
generated `.core.o.cmd`, `.if_usb.o.cmd`, `utsrelease.h`, `autoconf.h`, mod.c are
additionally recorded. Makefile links2G+USB, not11ah/SDIO/STABR. Module metadata:
ELF32 little-endian ARM ET_REL/ARMv5TE; name `hgics`; empty external-module
`depends`; alias **only** `usb:vA012p8400...`; vermagic
`6.18.55-qc07f-prototype-dirty mod_unload ARMv5 p2v8`.
`CONFIG_MODVERSIONS` is off: no CRC-based ABI safety net; exact pairing matters.
`CONFIG_MODULE_UNLOAD=y` is capability, not unload safety evidence.

Runtime binding must additionally use the actual verified21a boot-transfer chain,
private boot UUID, module file hash/size, USB topology and source/owner checkpoint.
A future changed module is a **new** pin and volatile staging receipt, never
silently called the embedded e667… module. Current `/hgics.ko` needs no75-style
replacement: package76 already fixed the stale embedded module.

## 3. What init really does / RF boundary

Source anchors relative to `.../station76-final/` unless stated otherwise:

1. `hgic_smac/core.c:1253–1288`: module init calls `hgic_usb_init`, then installs
   netdev notifier. It ignores USB registration's return and returns0; even module
   load success does not establish a successfully bound interface.
2. `utils/if_usb.c:339–413`: alias8400; requires bus1/devpath1, HS and cfg1. Selects
   bulk endpoints from current alternate setting, allocates16 TX /32 RX slots
   (each RX buffer2560B), initializes bus/core, submits RX, then queues delay_init.
   No minimum/full allocation enforcement and no exact interface/endpoint-number/
   count/maxpacket gate here. Parent must require the known cached configuration:
   a012:8400, cfg1, HS480, bulkOUT01/IN81,512B, expected interface/altsetting and
   complete cached descriptor bytes, no competing interface driver or usbfs owner.
   Internal validation before submission is part of §7, not an external guess.
3. `core.c:933–990`: `hgics_download_fw` is an unconditional return1; no loader
   invocation. Delay_init sends `GET_FW_INFO`, accepts reported return24/chip8400,
   sets flow-control mode, then registers mac80211/procfs. Retained2.1.0.1 makes
   `soft_fc=false`. Existing gate does NOT compare exact version/SVN/CPU, reject
   multicast/all-ones MAC, or refuse zero MAC (it substitutes a default). It prints
   the real MAC via `%pM`: original UART must remain PRIVATE/redacted on export.
4. `utils/fwctrl.c:322–340`: explicitly decodes retail24-byte FWINFO, not mirror
   28-byte structure. However `:84–116,156–175` match response by cookie and trust
   status when copying payload; they do not prove minimum control header/payload
   length or expected opcode/type/ifidx at this copy. `rx.c:208–232` general outer
   header bounds do not supply those missing per-control checks. Separate strict
   monitor info PASS does not repair this independent driver parser.
5. `11bgn.c:111–190`: only one station,2.4GHz, CCMP-only, no advertised PMF/AP/
   concurrency. No country setting or config load in8400 init; `/etc/hgics.conf`
   is used by the unlinked11ah code, not this entry. Kernel mac80211
   `main.c:1607–1616` adds a default managed netdev, **not an UP operation**. No
   scan callback/start in module init was found. With no hotplug/manager/supplicant
   and interface DOWN, normal registration is not association. This is not proof
   that opaque running firmware is electrically RF silent.
6. UP is a separate effect: `core.c:95–108,159–200,281–319` open firmware device,
   set MAC/channel/power/retry settings through mac80211 callbacks. WPA starts
   scanning/authentication/reassociation, not a passive profile parser. Do not
   combine `ifconfig ... up`, WPA or DHCP with attach-only acceptance.

### No hidden reload/reset — qualified, not overstated

`utils/if_usb.c:272–282` denies boot data/write/RUN, raw firmware and OTA at TX;
only boot command0 (state/ENTER) is excepted. `utils/fwctrl.c:55–70` allows a small
station command set; maintenance/save-config/unknown commands are denied. There
is no writable `/proc/hgics/iwpriv` (`procfs.c:194–208`). Defaults if_test/if_agg/
bt_en=0: do not override them or enable test/BT/aggregation.

**`no_bootdl` is only declared/exported, never consulted by current core.** Safety
comes from the actual no-op loader and TX allowlists, not that parameter. Fwdl,
OTA and request_firmware code remain linked; do not claim they were compiled out.
No current core/USB GPIO/reset/clear-halt API reference was found, including module
undefined symbols. A GPIO42 recovery string elsewhere is not authorization.

**Autonomous recovery IS present:** TX failure schedules detect_work
(`core.c:703–724`); detect sends boot ENTER, may call a bus reinit callback if
provided (USB leaves it NULL), schedules delay_init and rearms its5s detect timer
(`:993–1017`). Initial probe does not start that timer; first TX failure can.
If previously INITED and radio_off0, delay_init can call ops_start again. Boot
write/RUN stays denied, but these automatic query/re-init/reopen effects violate
the requested no-auto-fallback contract. Error latching must replace them for
this finite trial, not merely hope no error occurs.

COUNTRY/security: country setter exists but is **denied** by the runtime command
allowlist. Kernel cfg80211 regulatory channel restrictions must still be honored;
do not enable forbidden channels or bypass signed-regdb checks. Actual location,
allowed channel/current authorized AP profile remain fresh private admission
facts, not a guessed US/CN country. WPA2-PSK/CCMP without required PMF is the narrow
retained qualification profile, not WPA3/AP/product security acceptance.

## 4. Guard, time and retirement blockers

| ID | Concrete source blocker / required closure |
|---|---|
| S1 | `bringup/qc07f/initramfs/wifi-monitor.h:12–21` opens5s WDT before finit_module(ARM nr379, no force flags), then waits2s and magic-closes unconditionally. Init work outlives syscall; guard close/operstate/prompt is not success or retirement. Do not reuse this entry. |
| S2 | USB `down_timeout(...,1000)` uses **jiffies**: `CONFIG_HZ=100` means10s, longer than guard. FWCTRL reply500ms, boot-state100ms; soft-credit while loop `core.c:611–630` and TX queue retry loops have no total deadline. Late queued TX may survive a timed-out control request. Must fail closed/cancel owned unsent work, not lengthen guard/target waits. |
| S3 | Automatic detect/re-init above must be disabled for this operation. Init/FWINFO failure currently returns from delay_init while RX URBs/module stay live; no completion/error latch for controller. |
| S4 | RX callback `if_usb.c:221–235` ignores urb->status and unconditionally resubmits (unless STOP/ERR already set); a failed/partial transfer is not a clean receipt. General ctrl parser/exact firmware/MAC gates in §3 are incomplete. |
| S5 | Existing teardown has an important correct order: STOP → kill/drain URBs **outside** qlock → core remove/work/timers → free. But no cached used/inflight/work/completion snapshot proves it; `/proc/hgics/status` exposes only some queues/TX totals. Probe allocation/partial submission/core failure paths need fault closure (e.g. post-set_intfdata RX failure frees usbdev without clearing intfdata/putting its extra dev ref). Do not native-unload a failed/unknown entry to discover whether cleanup is safe. |
| S6 | Embedded tools do not include a reviewed guarded transport runner; DHCP callback violates local-only route/DNS policy (§6). An external timeout/kill is not DMA retirement or watchdog ownership. |

Use `WATCHDOG_RECOVERY_REVIEW.md` semantics: prearmed camera WDT preferred if
needed, not routine manual cycling. Arm/validate5s before first module effect;
fresh next-boot catch before that. One watchdog owner only, feeds only on verified
owned progress under a hard monotonic operation deadline; no endless feeder that
masks a wedged worker. Keep it armed through attach, init and known-state normal
retirement. Do not close on exceptions/unknown completion. On uncertainty retain
backing/ownership/HOLD, cease effects/feeds by the sealed design and observe the
already-prearmed reset/catch. No second UART sender, retrospective `wdt reset`,
timeout increase, module unload, kill or injected reset into unknown work.

WDT APIs to pin in a future helper: `/dev/watchdog0` exact char-device identity,
exclusive RW open, `WDIOC_SETTIMEOUT=0xc0045706` requesting/returning5, exact
GETTIMEOUT/KEEPALIVE/magic-close result checks against this kernel UAPI. Magic V
pings before release; close0/inactive sysfs alone is not electrical stop proof.
An inactive WDT cannot rescue a later hang. Genuine CPU reset/U-Boot catch is
required if it fires; SoC reset is NOT radio rail/cold reset or full DMA retirement
proof and must not trigger firmware reload/attach retry. Manual OFF/leaveOFF only
if WDT/control/catch unavailable or fails/required reset domain is not covered.

## 5. Mandatory fresh parent admission — no historical counter rebase

Before any later UART byte/module/profile effect, exact controller review GO and:

- Bind original failed HOLD bytes+inode+path and identity21a **completion** TX/RX
  checkpoint to ordered native events, original boot UUID, snapshots, original
  recorder peer/PID/start/source and unprivileged UID/GID. Parent alone handles
  private lineage. Do not derive a new baseline from current status counters.
- Sole physical adapter/receive owner/one sender, no active transfer/lease/GUI or
  stale socket takeover; future unconsumed main-CPU catch and recording lifetime.
  Changed owner/recorder/source, coverage gap, reset, USB disconnect/re-enumeration,
  HCD/WARN or unexplained TX since saved identity => decline, not reconcile by TX.
- Full bounded pre-RO: exact76 boot/image chain, PID1/UID0/FDs console-only, expected
  no modules or other USB consumer, descriptors and no driver, sameboot, volatile
  mounts, ISP/encoder/L2 idle, WD inactive/nowayout0/known timeout1..60/no alias FD,
  UART errors/busy0, no network/actuator/audio/light/MMC jobs. No memory budget
  inferred from idle MemFree/CmaFree; allocate only the declared finite buffers.
- Acquire operation lock without editing original HOLD; create a separate durable
  subordinate station-operation HOLD/receipt referring to it. Neither old helper
  requiring empty HOLD nor clearing/replacing/truncating original is allowed.
- Seal exact module/controller/UAPI/source/profile and per-phase wire lengths;
  guard *every* low-level send, source/owner/deadline check before/after blocking
  RPC and immediately before TX, including nested helper sends. Preserve saved RX
  partial-line carry-in, inspect complete new log interval for MUSB core/host WARN,
  CLEAR_HALT-active-EP, Babble/RESUME/DISCONNECT, reset/topology/new kernel changes.
  No read-clear HCD probes or arbitrary refresh/status loops.

Proposed finite attach-only envelope for review, **not measured runtime values**:
<=60s from first effect including normal retirement; initial catch+capture
coverage>=360s (60+300 final reserve), each command fits remaining phase+10s RPC
overhead. Guard5s unchanged; bounded init observation<=2s with reply waits<=500ms
and TX token/credit admission shortened/nonblocking, no long target wait/retry.
If staging/pre-RO consumes coverage, revalidate **before** first effect; decline
when short. Subsequent association/transport is a new admitted bounded envelope,
not an extension of this one. Private staging time also needs an independently
calculated lifetime bound. Old07:09UTC must never be substituted for these gates.

## 6. Later association / MTU plan — dependencies, not command queue

Only after separately reviewed attach/retirement and separately reviewed network
controller with real private profile/runtime gates:

1. Use **only existing authorized VertexHotspot**, original privately supplied
   profile, no credential/hash/MAC/BSSID/UUID export. Parent checks private byte
   continuity and build-supported settings without this lane opening it. The
   retained75 config failures (`-f` unsupported, CONFIG_NO_CONFIG_WRITE rejecting
   update_config) are evidence, not instructions to repeat/edit private config.
   No new SSID, AP, hostapd, P2P, WPS, cloud or background connection manager.
2. Embedded availability comes from unchanged build50 records/package bindings:
   `/bin/busybox` ash, cat/ls/ps, ifconfig/ip/route, ping/nc, udhcpc, base64,
   sha256sum/dd/insmod/rmmod; `/bin/wpa_supplicant`, `/bin/wpa_cli`, and new ELF
   `/bin/qc07f-dhcp-script-bridge`. No target Python, iw, timeout, kill utility,
   arbitrary GNU flags or shell-script executable support assumed. Shell is a
   monitor child (`shell`); entering it is a separately accounted UART operation.
   Verify actual selected executable hashes, capability/options and volatile
   exclusive staging destination/source-size before use. No mdev/hotplug autoload.
3. Proposed parameter vocabulary only, **not admitted commands**: WPA
   `-Dnl80211 -iwlan0 -c<private-RAM-profile> -O<owned-RAM-control-dir>`; no `-f`,
   debug secret dumps, config save or unattended daemon restart. Owner tracks exact
   child PID/start/FD and control socket, not a PID-file alone. Initial scan,
   management/auth/key traffic/reassociation are RF effects, allowed only in that
   later finite phase. Bound one overall association attempt, not independent
   unbounded internal retries; require actual COMPLETED/WPA2/CCMP/private AP match.
4. Set/readback MTU384 **before UP/association** and keep baseline throughout first
   association/data trial. Account IPv6: final kernel has IPv6 enabled; UP may
   create link-local/DAD/RS/multicast traffic. Controller must explicitly suppress
   IPv6 on this trial interface before UP through reviewed volatile policy or
   separately admit that traffic. Do not claim IPv4-only just from DHCP flags.
5. DHCP bridge SHA `6c7766557c1d0fe9e83ef33a5339f3836c78a29f84e2a4a02ae64b0d2bb39ef2`
   (8984B) execs `/bin/busybox ash /bin/udhcpc.script <event>`, preserving env.
   **It is not a local-only DHCP policy.** Unchanged default script sets address,
   deletes/adds default route and rewrites `/etc/resolv.conf`. Do not invoke it
   unchanged. Small future dependency: exclusive volatile address/prefix-only
   script selected by a pinned ELF bridge, preserve existing script if substituted
   and restore exact bytes after retirement; or separately build the smallest
   local-only ELF callback. Native script execution cannot be assumed. Explicit
   udhcpc `-s<reviewed-ELF>` plus finite attempts/foreground owned lifetime required;
   no router/DNS/NTP/hostname side effects, no long lease daemon outside envelope.
6. Privately establish actual owned PC endpoint/interface/route/listener and fresh
   camera address. Do not inherit192.168.1.15/180/73. No Internet reachability,
   broadcast discovery or third-party endpoint needed. Existing
   `bringup/qc07f/userland/netbench.c` is NOT usable unchanged: literal stale host
   .15/port19562, per-call4s vs30s loop, no global connect/deadline guarantee; not
   embedded. BusyBox1.37 ping has no `-M` option/IP_MTU_DISCOVER: `ping -s1472`
   alone is NOT a DF test. nc lacks the needed owned integrity/deadline receipt.
7. After MTU384 finite integrity baseline and **separate** source/current review,
   one1500 envelope may test256KiB fixed public pattern TCP each direction with
   exact byte verification/ACK, no full-AU media buffers. One owned-host IPv4
   DF1472-payload probe (1500-IP bytes), pinned host implementation `IP_PMTUDISC_DO`
   or equivalent and actual capture/length/integrity proof, no fragmentation claim
   from size alone. Requires a corrected/staged bounded own runner and reviewed
   endpoint/host-capture method; neither exists as an admitted76 controller here.
   Record immutable pre/post HCD/radio/netdev counters plus submitted/actual wire
   lengths/status where available. Current status counters cannot prove per-URB
   lengths or MODE; a functional pass is not a direct MODE/NAK/toggle proof.
8. Restore/readback384 on known-responsive owned success before retirement. On
   unknown transfer do not send an automatic rollback/unload/reset/kill as repair;
   keep HOLD and prearmed recovery. One failed trial, zero automatic repeats or
   firmware fallback.1500 stays experimental, not the default/product policy.

## 7. Smallest corrective source increment to name, not implement here

**A — attach-only first, no profile/network/MTU in the same controller.**

- New one-shot parent controller derived from corrected identity/retirement
  lineage gates, not their existing mains or old75 Session. Preserve original
  HOLD read-only and keep separate station HOLD. New tiny volatile own supervisor
  (or equivalently reviewed existing primitive) needed for single-owner WDT
  prearm/progress/deadline through asynchronous init and known normal teardown;
  current shell/monitor alone does not provide it. Do not add a generic framework,
  guardian/kernel ABI redesign or new kernel image.
- Isolated copy of exact67 pinned station76 sources, paired build only against
  final config/symvers/generated identity. Narrow changes: first-error sticky
  latch/no detect/reinit/reopen; exact USB descriptor validation before any URB;
  strict control response/header/opcode/ifidx/cookie/payload and firmware
  version/chip/SVN/CPU validation; reject invalid MAC rather than defaulting.
  Never publish its private comparison material. Correct failed RX handling and
  partial-probe cleanup; bounded TX admission/cancel unsent control on timeout.
- Add minimal **cached** init/error/quiescence receipt (no active reset/query):
  init complete/failed, bound device generation, work/timer/pending-control/data
  queue counts and TX/RX submit/completion/inflight totals. A normal attach-only
  path stays netdev DOWN, then explicitly STOP/drain/unregisters only while known
  owned and error-free. Every free follows proven callback retirement. On stuck/
  unknown drain retain state and let prearmed guard act, not another remover.
  No broad printk/trace stack. Exact compiled init and TX gates independently
  inspected; no fwload/RUN/GPIO/reset added, original module/image preserved.
- Future HOST tests must exercise real changed paths, not count substring checks:
  bad/missing/multiple endpoints/full/partial allocation, registration/probe error,
  short/forged FWINFO/version/MAC, init delayed past2s, TX token/credit exhaustion,
  failed/short RX/completion races, late timed-out response, failure sticky with
  zero recovery TX, STOP prevents resubmission, used==0 before free, no freed
  bus_priv callbacks, WDT open/ioctl/ping/close failures, deadline expiry, no feeds
  without progress, no kill/unload on unknown, original HOLD exact, nested-send/
  owner repin/counter drift/expired catch/split diagnostics zero-TX refusals.

**B — only after A review/native retirement:** correct local-only callback and
owned finite network/transport controller as §6, with separate profile/association
and1500 gates. Future module/supervisor/callback builds and RAM staging need their
own reviewed pins/size/memory/lifetime receipts; no ARM binary was built here.

## 8. Required final acceptance / retirement receipt

Normal complete attach-only result must include: exact final module binding;
asynchronous init complete with strict retained identity; DOWN/no scan/association
or IP services; no errors/WARN/new boot/topology gap; explicit known-state STOP,
all RX/TX callbacks retired and inflight0; control/data/ack/event queues empty,
delay/detect/TX/test/fwctrl work and detect/beacon timers retired, no pending
response/USB references, no station module/interface driver/netdev/proc owner.
Normal teardown must be source-reviewed and finite before being selected—no
automatic `rmmod` following a failed/late init.

For later network trial additionally require observed owned WPA/DHCP/socket/bench
child exits/control-socket withdrawal, restored384 and declared route/DNS/IPv6
policy restoration, private staging disposition, no listener or retry worker.
Use normal shutdown only in responsive known ownership; no unknown-DMA kill/free.
Guard owner persists until all resource retirement receipts and sameboot post-RO
inventory: console-only monitor FDs/no USB or WDT alias, no modules/driver, expected
cached USB descriptors, WD magic-close/logical inactive/nowayout0 with required
stop corroboration, ISP idle/guards/IRQ/queues, UART errors/busy0 and expected
volatile mounts. Any missing item => subordinate HOLD, not another active query
or a claim that a prompt/empty process list proves USB DMA stopped.

If guard recovery occurs, record actual reset/catch/held original U-Boot and which
old authorities were invalidated, not normally retired. No RAM76/network/runtime
authority survives by assumption; original failed-trial HOLD never clears.

**Parent handback:** current station attach/association/MTU EXECUTION NO-GO.
Select SOURCE/HOST increment A if desired, then exact independent review and fresh
real admission. No command queue; no firmware/RUN repeat, GPIO42, card/original
media, AP/newSSID, peripherals/audio/motor/lights/GPIO2 or persistent write. RP-04
combined memory and RP-02/product acceptance remain HOLD/OPEN.
