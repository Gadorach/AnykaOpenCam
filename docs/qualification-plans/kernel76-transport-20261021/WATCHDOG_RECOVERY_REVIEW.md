# RP-00/01 — RAM76 watchdog recovery source review

2026-10-21. **SOURCE-CONDITIONAL GO; NOT NATIVE ADMISSION OR A RESET RESULT.**
Parent alone owns hardware. This lane owns this report and `recovery-source-review/`
only. The RAM76 resume21a transfer remains active at task entry: **do not interrupt,
poll, send UART/RPC/status/progress requests, or edit its frozen recipe/helpers.**
No build, firmware execution or target operation was performed here. Prior native
results below are cited from authored documentation, not reproduced here; no raw
capture, credential, radio payload or identifying field is included in this output.

## Decision and immediate blocker

User steering prefers **camera-watchdog reset rather than routine manual power
cycling** for unattended recovery. There is a source-supported existing entry for
**one `wdt reset` from a freshly proved idle, responsive, exact76 PID1 monitor**,
after parent review and all gates below. It is **not available now during the
active transfer**, and does not rescue an unknown/hung monitor with inactive WDT.
No reset command, timer change or replacement recovery helper is queued here.

One next eligible effect, **only if recovery is actually needed**: after transfer
completion/lease retirement and fresh current-state admission, arm the existing
sole-owner next-boot catch, durably set operation HOLD, then send exactly one
`wdt reset` plus one line terminator. Observe reset and held U-Boot; **stop there**.
Do not reboot a healthy runtime merely to exercise this preference without the
parent's explicit selection of this one qualification/recovery operation.

**Current blockers:** active transfer; parent source review; fresh idle runtime,
sole owner/catch identity and lifetime; no competing operation; reset outcome and
held-U-Boot retirement have not been observed in this lane. Any missing fact keeps
native use NO-GO. Source review does not clear an earlier operation's failure.

## 1. Exact source/build binding

Offline command run once at04:29:05UTC:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/recovery-source-review/inspect_recovery.py
```

Result: **offline binding PASS**. Python3.14.7, LLVM23.1.1; exact input/output pins,
tool versions and disassembly argv in `recovery-source-review/binding.json`.
Outputs are exclusively created: a rerun must preserve them/use a fresh reviewed
output location. This is a static artifact inspection, not a host fixture or native
watchdog test. The collector opens only named source/build/binding inputs; it does
not use the RP-02 collector or read its local/private firmware inputs.
`recovery-source-review/validation.json` separately records syntax/pin checks and
successful rerun/`-O` refusals with all original evidence bytes unchanged.

- `/init` is the unchanged attempt50 monitor,174248 bytes, SHA256
  `8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899`.
  It and current `init.c` match the earlier image↔cpio↔build50 source binding in
  `source-review/entry-bind.json`; `QC07F_PERIPHERAL_QUALIFICATION=1` was compiled.
  This review rehashes that exact ELF/source and disassembles watchdog text only;
  it does not rerun the radio loader/payload audit.
- Final candidate76 `vmlinux` rehash matches package manifest pin
  `24df07ee09121eeff50ab01a5aa002a67418e5593d955f79836447673d427b14`.
  Image pin remains `bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f`;
  image construction relies on the existing final package verification, not a
  rebuild or fresh full kernel unpack here.
- Candidate `linux/arch/arm/mach-ak3918ev200/watchdog.c` equals the reviewed
  `bringup/qc07f/mainline/.../watchdog.c` overlay, SHA256
  `d6646a2b894f822ea486c4d24db7a49df2003a5706cf25e9908bc1fe15fe06e9`.
  Retained `.watchdog.o.cmd` names that exact source path. Candidate core sources
  and object are pinned separately. Path metadata is not a fresh rebuild proof;
  selected driver text is also present in the manifest-pinned linked kernel.
- Final config: built-in `WATCHDOG_AK3918EV200`, core and sysfs enabled;
  NOWAYOUT disabled; HANDLE_BOOT_ENABLED enabled; OPEN_TIMEOUT=0; no hrtimer
  pretimeout. Retained candidate `inputs/attempt45.dtb` watchdog node is **okay**,
  resource `[0x080000e4,0x080000ec)`, nominal rate46875Hz. The driver's introductory
  comment about a disabled prototype DT does **not** describe this retained DTB.

Selected ARM text corroborates:

| Address | Static finding |
|---|---|
| monitor `0x4802c..0x48058` | Nonzero reset argument chooses1, smoke chooses5 |
| `0x4805c..0x480cc` | Stoppability gate, RW open, timeout ioctl/error/exact-value comparison |
| `0x480d0..0x480ec` | Reset branch prints arm text, loops sleeping1; no ping/close branch |
| `0x481a4..0x48204` | Error/smoke cleanup writesV, closes, checks syscall return only |
| kernel `0xc001ef50..0xc001ef94` | Start programs keyed load and control enable/reset |
| `0xc001ef98..0xc001efcc` | Stop writes keyed disable and maximal reload |
| `0xc001efd0..0xc001f050` | Timeout bounds/store and active-or-HW-running rearm |
| `0xc001f054..0xc001f090` | Separate restart callback loads **one tick**, not one second |

`kernel-watchdog.disasm` also locates core open/ioctl/release. This is selected
symbol text, **not exhaustive core disassembly**: objdump stops the ioctl body at
an ARM jump table and renders two multiply instructions `<unknown>`. Detailed core
semantics below are source-backed, not claimed fully decoded from those snippets.

## 2. What the monitor/driver/core actually do

Anchors: `bringup/qc07f/initramfs/init.c`; candidate kernel under
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.

1. **Entry (`init.c:66–105,492–495`).** Exact dispatcher text `wdt reset` calls
   `watchdog_test(1)`. It requires a successful nowayout read whose first character
   is0; this is a policy check, not device ownership or hardware readiness proof.
   It opens `/dev/watchdog0` RW: opening is already effectful. Then
   `WDIOC_SETTIMEOUT` (`0xc0045706`) requests1 and must return nonnegative and write
   back exactly1. Only then it prints:

   ```text
   WATCHDOG RESET ARMED: nominal1s; fd remains open, no keepalive
   ```

   PID1 then loops forever in1s nanosleeps, intentionally retaining the FD with
   **no later keepalive or magic close**. It does not call Linux reboot, a shell
   reboot, GPIO reset or the driver's restart hook. No subsequent monitor prompt
   is expected on this successful path. A blocked arm-message UART write does not
   cancel an already-programmed watchdog; reset can precede complete marker RX.
2. **Open/core (`watchdog_dev.c:248–277,864–919`).** Single-open gate rejects a
   competing owner; successful open starts/reloads hardware at the current timeout
   before the1s ioctl. Do not infer “no effect” from a failed/missing arm marker:
   an open may already have armed the prior timeout. Startup configured60s is not
   the deliberate reset timeout; require the current logical timeout to be known.
3. **Timeout/core (`watchdog_dev.c:368–389,804–826`).** Core checks validity, calls
   driver set_timeout, then pings and copies `wdd->timeout` back. Driver set_timeout
   (`watchdog.c:67–75`) stores1 and restarts if active/HW-running. The ioctl can
   therefore rearm twice; nominal timing begins from the last actual reload, not
   from host TX/marker time. There is no independent electrical timeout readback.
4. **Hardware (`watchdog.c:39–65,109–128`).** Start/ping writes
   `0x55000000 | (timeout × rate)` atSYS+e4, then `0xaa000003` atSYS+e8 under raw
   spinlock/IRQ-save. At retained DT rate,1s uses46875 ticks (`0x5500b71b`),5s
   uses234375 (`0x55039387`). Min1/max357 seconds follow the24-bit counter.
   These are **calculated programming values, not measured oscillator/reset times**.
   Stop writes `0xaa000000` control then `0x55ffffff` reload. Restart callback
   (`:77–83`) loads1 tick (~21.3µs nominal); it is not used by `wdt reset` and is
   not an alternative admitted command.
5. **No permanent automatic feeder on this active entry.** Driver leaves both
   min/max hardware-heartbeat fields zero. Core worker condition
   (`watchdog_dev.c:76–97,129–141`) is false for an active userspace watchdog with
   no max heartbeat; active PID1's no-feed path is not routinely rescued by a
   timer worker. In contrast inherited running hardware with logical inactive
   state **can** be fed by core under HANDLE_BOOT_ENABLED/OPEN_TIMEOUT=0 until the
   startup open/stop. “Kernel worker exists” neither disproves this entry nor
   proves every later hang resets.
6. **Errors and magic close.** Refused nowayout/open return to monitor. A failed
   timeout request goes to writeV/close; smoke uses5s, sleeps2, pings, sleeps2,
   checks timeout5, then the same cleanup. The reset success branch does none of
   this cleanup. Core writeV first enables release **and pings** (`:697–735`);
   release (`:942–989`) attempts stop, or pings on failure, and **returns0 even if
   hardware did not stop**. Thus `WATCHDOG MAGIC CLOSE COMPLETED`, a prompt, sysfs
   inactive or `close()==0` alone is not electrical proof or failure clearance.

No direct GPIO, SPI, flash, card or radio command occurs in `watchdog_test`.
That is not a proof that reboot has no peripheral/boot-policy consequences.

## 3. Three different recovery cases — do not interchange them

| Case | Preferred action and limits |
|---|---|
| Existing operation already prearmed its reviewed5s guard before risky work | Let that guard expire if progress stops, retaining ownership/backing/HOLD and the prearmed next-boot catch. **No second sender, `wdt reset`, FD stealing, timeout shortening, kill/unload, or rescue query while it is active.**5s is nominal since last feed, not a total operation deadline. A still-feeding but unhealthy path might never expire. |
| Responsive known idle exact76 monitor, no active operation | After all gates below, one deliberate `wdt reset` uses the existing1s entry. This is a controlled reset qualification/recovery, not the5s guard or smoke. |
| Unknown/hung monitor, inactive/unknown watchdog, or USB/DMA/actuator ownership unknown | Cannot retrofit a prearm by waiting, hoping inactive WDT expires, injecting monitor text, or assuming one healthy reply means safe. Existing already-armed recovery may still reset; otherwise watchdog/control recovery is unavailable and manual **OFF/leave OFF** is fallback. No speculative reset command. |

The QCL transfer's15s loader watchdog and60s boot/startup watchdog are **other
phases**, not this5s runtime guard. Their frozen recipe must not be changed now.
The RP-02 review already covers radio guard timing/cleanup hazards; this lane does
not duplicate its loader/RUN audit or change those timeouts.
An operation that returns an error may already have attempted magic-close and
disarmed its5s guard; it is not necessarily waiting to reboot. Use the known-idle
reset route only after its independent ownership/idle gates, otherwise treat
watchdog recovery as unavailable. Do not manufacture an armed guard from its
earlier successful open or from the existence of `/dev/watchdog0`.

## 4. Parent admission checklist — all required before any reset TX

1. **No active lease:** actual resume21a completion and full helper/transfer lease
   retirement, followed only by its already-approved fixed RO postcheck. A delayed
   tool response or presumed elapsed time is not retirement. If failure leaves
   unknown in-flight ownership, use its existing guard, not this idle-only entry.
2. **Fresh runtime:** bind exact candidate76 boot/image to genuine current monitor,
   PID1init/UID0/capability, expected volatile mounts/no persistent mounts, modules
   and FD owners, boot continuity and returned clean command ownership. Read-only
   cached ISP guards/queues/frame state must show idle; no live camera/encoder/L2/
   USB bulk or asynchronous work, USB consumers/station driver/network workload,
   actuator/light/audio jobs, GUI or staged command producer. Confirm UART counters
   and empty monitor input history. Enumerated idle USB is not an active transfer.
   Unknown subsystem ownership is **NO-GO**, not permission to run active probes.
3. **Watchdog admission:** exact TOP watchdog identity/device, current `nowayout=0`,
   logical inactive and known logical timeout in1..60, no FD/alias owner or queued
   operation. These are required source/ownership facts, **not electrical proof**.
   Use only parent-reviewed bounded readers after transfer retirement. Do not add
   `wdt smoke` or a device open to passive preflight or take a watchdog from another
   process. If current timeout is unknown/>60, this bounded recipe is not admitted.
4. **Fresh host ownership:** intended adapter and source/argv/peer/PID/start identity
   revalidated by parent after exclusive transfer; exactly one receive owner and
   permitted sender, no stale socket takeover, no other UART TX. Current21a PID,
  07:09UTC limit and earlier catch state are **dated facts**, not authority here.
5. **Arm BEFORE TX:** through that existing owner, confirm a **future**, enabled,
   unconsumed one-shot catch for the original main-CPU U-Boot autoboot sequence,
   not merely receive capture, consumed cold-boot catch or radio `HGIC UBOOT` text.
   Its matcher/stop-TX behavior must handle reset boundaries, and catch must not
   silently expire at reset. No new catch implementation or helper edit is admitted.
   Host catch interrupt bytes are the only automatic post-command TX allowed.
6. **Lifetime/budget:** record absolute monotonic/UTC deadlines and the exact one
   command before sending. Proposed conservative bounds in §5 require **at least
  120s remaining** on receive ownership **and** catch at TX, plus any larger
   parent-proven fallback/recording reserve. This minimum is not an assertion that
   a human can always intervene in120s. If finite coverage cannot be maintained,
   decline rather than start an unattended reset with expiring protection.
7. **Durable HOLD/loop escape:** parent records a new reset operation ID, selected
   reason, old boot binding, command, catch arm receipt, lifetime and one-attempt
   budget **before** any byte. Preserve all prior failures; retain backing and
   exclusive owner. No tool retry/reconnect may replay this text. No queued command
   may be delivered after reset. The budget is **one reset, zero automatic reboot/
   firmware reload/RUN/effect retries**. Parent review is required before native use.

## 5. Finite observation, reset proof and retirement

These are **proposed acceptance/host observation limits**, not measured hardware
deadlines or changes to driver/catch timers. Parent must accept/seal them first:

| Relative to first reset-command TX `t0` | Limit / interpretation |
|---|---|
| `t0+5s` | Expected reset-arm line or already-observed genuine reset; otherwise missing/unknown-arm HOLD, **never resend**. Keep original catch/receive coverage. |
| `t0+20s` | Require fresh main-CPU original U-Boot startup/boot sequence, intercepted autoboot and complete held `anyka$` prompt. Missing catch/reset by this bound fails watchdog recovery; fallback OFF/leaveOFF, not a longer guessed timeout or a second reset. |
| Through at least10s after held prompt (latest `t0+30s`) | Bounded passive observation of stable held U-Boot: no second reset/banner, autoboot/kernel/stock SSH startup, loader execution or unexplained TX. If needed, only one separately parent-reviewed harmless U-Boot prompt/identity check **after** stable catch; not an automatic part of this source GO. |
| Remaining coverage (minimum90s at latest success point) | Evidence/owner handoff or named manual fallback reserve. No routine progress polling loop or unbounded extension/retry. A required longer reserve increases the pre-TX minimum. |

**Positive criteria, all required:**

- Same owned receive sequence brackets the single TX and fresh arm marker (when
  fully delivered), then a genuine **main-CPU** reset/U-Boot boot sequence. No
  receive-buffer replay, old banner, isolated prompt, disconnect, silence, network
  loss or marker alone is reset proof. If marker is truncated/missing but real
  reset/catch is observed, record recovered reset with incomplete arming receipt;
  do not fabricate a full monitor-command PASS or measured1s interval.
- Actual catch consumes its one-shot, autoboot is stopped and original U-Boot is
  stably held for10s. Any repeat reset is an immediate **loop-budget failure**:
  preserve HOLD and use OFF fallback; do not replenish reset/reboot attempts.
  Do not assume a held prompt disabled inherited watchdog hardware. This passive
  interval corroborates short reset-loop absence, not unlimited WDT-off proof.
- Retire old Linux control epochs/PID/FD authority **as invalidated by observed
  CPU reset**, not as normally closed or successfully cleaned up. Retire the host
  reset command/lease and its queued-send authority; retain only the known capture
  owner. Record catch consumed/unarmed; explicitly rearm before any later separately
  approved effect. Do not silently inherit a future catch from this operation.
- Because entry required idle, this proves one idle CPU reset/catch boundary,
  **not full-DMA, active-USB, L2, optical-pulse or motor-safe reset qualification**.
  Unknown hardware backing/ownership from an earlier failed active operation
  remains quarantined until its own reset-domain/retirement proof. No memory reuse,
  free/unload, DMA restart or HOLD clearance follows from a later good prompt alone.
- Parent may close this reset's HOLD only with full transcript/deadline/retirement
  review. Earlier operation failure remains a preserved failed result, optionally
  annotated “recovered to held U-Boot,” never converted to success. If any required
  retirement fact remains unknown, HOLD persists even though a reset was seen.

**Negative criteria:** refusal/open/ioctl/magic-close error, returned Linux prompt
instead of reset, partial/late response, timeout, missing/expired catch, extra TX,
wrong banner, boot escape, repeated reset or unknown owner is HOLD. Keep sole
capture/evidence; do not `kill`, free, reload, run firmware or send rescue commands
into an uncertain active runtime. Manual **OFF/leave OFF** is used only when the
watchdog/control/catch path is unavailable, fails this bounded recovery, or cannot
provide the required reset domain. If no operator is available, report that
physical blocker; do not manufacture recovery or continue an effect queue.

## 6. Electrical scope, historical evidence and remaining product gates

`watchdog_dev.c:578–586` sysfs `state` reports `watchdog_active()`, not control-register
readback; `nowayout` is a software policy bit. This driver supplies no hardware
time-left/status/reset-cause observation proving pin timing, rail reset or a
distinct electrical reset cause. Genuine U-Boot after no-feed is native functional
reset evidence; host chunk timing is not measured oscillator tolerance.

- `docs/DEVELOPMENT_OBJECTIVES.md:53` records Oct09 Linux6.18.55 basic watchdog
  start/timeout/ping/stop/**expiry reset** PASS. It is historical, not today's
  candidate76 proof or arbitrary hang coverage.
- `docs/FOUNDATION_PLAN_PROGRESS_20261011.md:364–368` records an exact74 **no-DMA**
  holder5→stopped holder/proc-FD alias→request1 experiment: actual reset and next
  U-Boot held, host RX-request→U-Boot1.640349145s. That demonstrates a prior
  modern TOP-watchdog1s recovery route, **not this monitor76 entry, electrical
  timing, full65s guard acceptance, hung-core recovery or active-DMA retirement**.
- Oct20 candidate76 boot/RO idle PASS in `kernel76-integration-20261020/RESULTS.md`
  and its postcheck review corroborate startup `V`/close and logical inactive WDT.
  They did **not** perform deliberate76 expiry/reset; timestamps/catch are expired
  authority. Candidate76 reset acceptance is OPEN until parent observes this trial.

The CPU/SoC watchdog is **not a cold power cycle** of the radio, sensor, bridge or
other rails. Radio firmware/application/download state can survive or be unknown;
no automatic `radio load`, `radio native-load`, RUN, GPIO42 reset, bus/endpoint
reset or station attachment follows. No claim that reset restores a clean radio
bootloader, clears partial firmware, electrically turns off actuators, preserves
motor origin or returns to a previously admitted image. Main-CPU reset also
invalidates the RAM76 execution context; held U-Boot is **not Linux76 restored**.

An inactive watchdog cannot help a later hard hang by itself. A successfully
prearmed counter may work despite scheduler/IRQ stalls, but this review does not
prove watchdog clock/reset-domain survival in every bus/power failure. Progress-
based single-owner production supervision, startup/runtime separation, bounded
restart/backoff and reboot-loop escape remain **PLAT-06 / RP-01 OPEN**. This one
review closes no E2/E3/G0/G2/G8 product gate and authorizes no new firmware build,
reload, persistent write, card access, AP broadcast, GPIO42 or actuator effect.

**Handback:** parent review first; leave active transfer uninterrupted. On actual
completion, reconcile current gates and choose at most the one idle reset above
if required. Prefer the already-prearmed guard when its operation is active;
manual OFF remains fallback for unavailable/failed watchdog recovery only.
