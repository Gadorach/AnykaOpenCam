# RAM76 failed native radio trial — retirement-only follow-up review

2026-10-21. Independent **SOURCE-ONLY** review; parent alone owns hardware.

## Decision

**Conditional SOURCE GO for one fixed, retirement-only read-only sequence below.**
**NO-GO to execute the existing trial, metadata or postcheck `main()` unchanged.**
They have the wrong pretrial counter handoff, require an empty HOLD, and their
success branches clear their own HOLD. No existing after-failure entry implements
the required preserved-HOLD/counter binding. Parent must seal/review the actual
small wrapper and satisfy the fresh gates below before its first byte. This is
not permission to continue the failed whole trial at `radio state`.

No target/RPC/status/serial operation, private input access, build, ARM execution,
production edit, commit or index change occurred here. Owned outputs are this
report and `failed-followup-review/` only. Current runtime/owner/catch/retirement
are **not observed by this reviewer**. No further action is queued.

### Outcome being preserved

The public `RADIO_NATIVE76_21A01_HOLD.json` records one failed native trial after
fresh pre-idle/descriptors/PID1 FDs passed: 74 payload fragments, 150 complete
16-byte ACK lines, observed RUN success constant, **failed strict whole receipt**,
no state/info and no post-load retirement check. Parent additionally supplied the
nonidentifying interrupted line:

```text
RADIO BOOT RX cmd0xusb 1-1: USB disconnect, device number 2
```

Private bytes were not accessed. **Do not repair/join/filter this line into an
ACK, accept 150 instead of 151, or rerun/resend load/RUN/fallback.** The original
trial stays FAILED/HOLD even if every later retirement observation is healthy.

## 1. Exact source facts and failure classification

Reviewed trial SHA256:
`6d0ce652d3355baef2e5505e9d2a34813aa1f780289b17268ccb160c4748da9c`.
`failed-followup-review/result.json` pins all inspected public inputs. Current
`init.c`, `radio-ram-loader.h`, `wifi-monitor.h` match their retained build50
bindings in `source-review/entry-bind.json`; this review did not inspect private
payloads, rebuild the image or repeat the earlier ELF disassembly.

| Source anchor | Consequence |
|---|---|
| `radio-ram-loader.h:20–35,118–123` | RUN success text is reachable only after synchronous command4/cookie176 returns status0 under the **target's** ACK checks. Those checks are weaker than the strict host grammar (target accepts length >=16 and does not check every header field). The supplied constant supports **success branch reached**, not an independently accepted final16-byte host ACK/application identity. |
| `radio-ram-loader.h:123,126–130` | Success text precedes cleanup. Interface release, watchdog `V` write/close and USB close are attempted; return values are ignored. No checked cleanup receipt is printed. The failure-text GPIO42 suggestion is not an executed action or permission. |
| `init.c:18–24`; loader `:25–26` | An ACK is many separate console writes, including each hex pair. Kernel printk may interleave within a record; source does not make that record atomic. This explains a possible framing loss without establishing its sole cause. |
| `init.c:480–535` | `radio_load(1)` runs synchronously in PID1. Dispatch prints the next prompt only after it returns, then resets its input length to0. Fresh original prompt is evidence of returned userspace dispatch, **not** verified syscall cleanup/HCD retirement. No deferred load/RUN retry exists in this entry. |
| `trial21a.py:135–147` | Host strict parse runs at136 before settling, state or info. Its exception preserves HOLD; no automatic repair or retirement is executed. A later prompt is not a continuation token. |

Candidate kernel anchors below are under
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`:

- `drivers/usb/core/hub.c:2319–2398`: the supplied `USB disconnect, device number`
  is **ordinary usbcore `dev_info`**, not the MUSB `unhandled DISCONNECT transition`
  warning. It is emitted **before** device-disable/unlink synchronization; the
  log line alone cannot certify completion of teardown. Removal disables the
  configuration and synchronizes unlinks at2349–2350.
- `drivers/usb/musb/musb_core.c:942–982`: A_HOST disconnect has an ordinary root
  disconnect path; the default/unhandled state is a different warning. Babble at
  1001–1002 and bogus RESUME remain errors; none may be suppressed as enumeration.
- `hub.c:2413–2418,4999–5003`: new-device/descriptor text is ordinary enumeration.
  Enumeration is asynchronous relative to PID1 console output, so logs after a
  prompt are possible. Kernel enumeration itself performs USB control traffic;
  read-only follow-up means **no new application-issued USB query/reset**, not
  that the autonomous kernel has emitted zero USB packets.
- `drivers/usb/core/devio.c:1139–1166,1283–1370`: USBDEVFS_BULK uses a synchronous
  completion wait; timeout/interruption kills the URB before returning. The bulk
  path frees its URB/backing before return. `:1092–1127` releases claimed interfaces
  and destroys asynchronous requests on usbfs release; `:2861–2878` handles remove.
  These are lifecycle source facts, not sampled all-URB/DMA counters on this target.

**Classification:** disconnect/re-enumeration at a firmware RUN boundary is
**compatible with a firmware USB transition**, and the generic line alone is not
an HCD failure. Neither the supplied text nor retained host `station76-final/utils/
fwdl.c:313–341,417–422` proves that this exact firmware must disconnect, that all
late enumeration completed, or that no other HCD error occurred. The host source
sends RUN and checks its result; radio-internal USB transition timing is not proved
here. Do not relabel this trial either “HCD failure proved” or “expected transition,
therefore PASS.” The strict-receipt failure is real and preserved.

## 2. What can safely be read, and what cannot

The fixed inventory in `postcheck.py:23–32,50–90` is 13 distinct monitor queries
plus duplicate boot UUID = **14 queries / 320 UART bytes**. It reads procfs/sysfs,
not radio endpoints. `isp state` is cached memory under the ISP mutex
(`bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp.c:296–315`); UART `rx_state`
is cached READ_ONCE counters (`uart.c:189–193`), not MMIO read-clear polling.
Require its complete existing schema: exact release/bootargs/PID1/root/RAWIO,
volatile mounts only, modules empty, CMA16MiB, idle ISP/guards/queues, inactive
logical watchdog/nowayout0, zero UART error/busy counters and stable valid UUID.
Memory snapshots remain snapshots, not a combined-load budget.

`metadata21a.script():60–70` is a fixed **527-byte** shell line for any32-hex nonce:
`ls -l /proc/1/fd`, cached vendor/product/config/speed and endpoint attributes, and
a driver-symlink test. No usbfs/watchdog device is opened and no ioctl is issued.
Candidate `drivers/usb/core/sysfs.c:25–43,90–91,145–178,690–704` and
`endpoint.c:37–58` show these **reads** emit cached configuration/descriptor fields,
not on-demand radio requests. Configuration **writes** would be active and are
excluded. Accept a012:8400/config1/high-speed480, bulk01/81/maxpacket512 and no bound
interface driver. USB device number2 need not survive re-enumeration: bind topology
1-1 and the expected descriptor set, not the obsolete usbfs node. The existing
script does not capture devnum or an atomic enumeration generation; require no
new topology transition within the follow-up interval, and retain this limitation.

`radio_receipt.parse_parent_fds():90–101` requires exactly four console/ttyAK0 FDs
0..3. While the owned shell runs, this samples **PID1**, excluding its usbfs and
watchdog leaks. It is not a complete global process/FD inventory. Parent must also
exclude other userspace consumers/command producers from the exact image and
continuous owned history; unknown consumers block later active queries/reset.

`wifi-monitor.h:23–37`: shell forks; child runs only BusyBox ash with fixed
PATH/HOME/TERM; parent waits for that child, retrying EINTR, then prints the return
line. It does not check shell exit status and prints that line even for other
wait errors. Therefore require **all** nonce-framed command completion, fresh shell
prompt, the one explicit exit, return line and fresh monitor prompt; the string
alone is not proof. These provide bounded owned child-return/reap evidence, not a
new global zombie/process census. Do not run any background command in the shell.

**Excluded in retirement:** every `radio ...` entry, including state/info/repeat;
USBDEVFS open/claim/CLEAR_HALT/bulk; module/driver attach, unbind or bus reset;
watchdog open/ping/timeout/reset; network/media/peripherals; firmware transfer/RUN,
GPIO42, fallback, memory release/reuse or persistent writes.

## 3. Fresh parent admission and the missing counter binding

Before first UART byte, all required:

1. Actual failed shell/trial has completed and its operation lease is retired;
   no transfer, GUI, second producer or concurrent parent command. **Host lease
   retirement is not proof of target cleanup.** That is what these RO reads assess.
   Saved owned load prompt/success constant and zero subsequent target TX must be
   validated by parent; no prompt invented from an old buffer.
2. Revalidate intended adapter, sole recorder source SHA/argv, same part/capture,
   full original peer tuple/UID/GID, original PID/start ticks and original boot/image
   binding. `Boot.owner()` can refresh `self.peer`; compare to the **original**
   handoff after refresh, not to a newly accepted replacement. Pin private envelope
   capabilities/source/path too. Reject owner/source/part changes or RX rollback.
3. Through that owner, freshly prove enabled, future, unconsumed original-main-CPU
   U-Boot catch and capture lifetime; not a consumed cold catch or radio boot text.
   Proposed finite budget: >=300s coverage at first TX, <=150s whole follow-up,
   including bounded waits, retaining >=150s fallback/evidence reserve. Parent may
   require a larger reserve. No silent rearm/reconnect/replay under this operation.
4. Hold the existing operation lock exclusively. Verify the durable nonempty HOLD
   is **this failed trial's**, preserve its bytes and receipt, and record a new
   subordinate retirement-attempt ID privately. Never truncate, substitute an empty
   lock file, overwrite original result, or let a successful RO result clear HOLD.
   Foreign/unexplained HOLD refuses. No alternate lock bypass.
5. Bind counters to actual reviewed commands, **not** to whatever current status
   happens to report. Let `Cboot` be the original private boot-checkpoint TX;
   `Rp` the original postcheck's boolean empty refresh and `Rt` the failed trial's
   own pre-inventory empty refresh. At the reported parse failure, exact source
   derivation is:

   ```text
   Cfailed = Cboot + (320 + Rp) + (320 + Rt) + 539 + 18
           = Cboot + 1197 + Rp + Rt
   ```

   The539 is `shell\r`6 + fixed nonce line527 + CR1 + `exit\r`5; native-load is
   18 UART bytes. USB data/ACK lengths are **not UART TX counters**. Parent must
   establish both actual refresh facts and all intervening TX from its retained
   private handoff/events/snapshot. This lane cannot supply their values. The
   public HOLD does not contain them. Trial only writes its final private counter
   checkpoint on its success path (`trial21a.py:167–169`), which was not reached.
   **Missing endpoint checkpoint is a concrete binding gap**, not permission to
   copy current counters into a new baseline. Require current TX exactly Cfailed,
   `tx_busy=false`, monotonically growing RX consistent with retained capture and
   the known failure boundary, and no unexplained/pending input.
6. Preserve/check the full owned interval from the original postboot checkpoint
   (including trial and late logs) for reset/new Linux/U-Boot or HCD diagnostics.
   Use continuous reset checking including boundary tails; do not move the cursor
   forward past a failure to manufacture a clean interval. Generic disconnect is
   retained as evidence, not removed from the failed receipt. All recognized HCD
   warnings/errors/panic remain disqualifying for a healthy-idle verdict. If already
   contradictory/unknown HCD ownership, this conditional healthy-retirement route
   is NO-GO; report blocker rather than inject a radio/reset rescue.

`metadata21a.admitted_handoff():31–57` computes **Cboot+320+Rp**, not Cfailed.
The old metadata CLI is before-only; `postcheck.main()` expects Cboot. Neither
can be rerun as an after-failure script. Reuse their pinned pure parsers/fixed RO
wire definitions only in the reviewed tiny wrapper, with failure-specific original
owner/counter binding and immutable HOLD. No source/recipe change during execution.

## 4. One finite retirement-only sequence (conditional, not executed)

1. Allow one bounded passive settling interval (e.g.2s) for already-arriving logs.
   Waiting is not readiness proof. Retain all bytes. If current tail is not a fresh
   terminal monitor prompt, parent must have verified the actual own returned
   monitor and empty command buffer. Only then, **at most one `\r` empty monitor
   refresh**, bounded5s, requiring one fresh terminal prompt. Never flush a partial
   command, send Ctrl-C, or repeat refresh to hide continued enumeration/late text.
2. Perform **one** fixed cached FD/descriptors shell roundtrip: `shell\r`, exact
   nonce line in bounded64-byte chunks with exact echo before CR, command CR,
   complete nonce BEGIN/DONE and fresh shell prompt, then `exit\r` and owned
   monitor return. Match `trial21a.metadata_roundtrip():54–84` framing/lifecycle.
   A completely executed, framed RO transaction may exit **before** accepting
   metadata so an invalid completed descriptor result does not strand its shell.
   After that invalid result: stop/HOLD; no inventory or active query. If echo,
   completion or prompt is partial/unknown, do **not** append `exit` or extra CR
   into an unknown pending command. Preserve ownership/HOLD and report recovery
   blocker. No timeout retry or child kill.
3. Only after valid metadata and shell retirement, perform the existing **full
   14-query inventory**, in existing order including final duplicate boot UUID.
   Require `postcheck.body()` framing/schema and `postcheck.parse()` acceptance;
   compare UUID privately with the **original pretrial runtime**, not just with
   itself. No interpretation of duplicate UUID alone as image/boot provenance.
4. At every boundary: original peer/start/source/catch retained, exact expected
   cumulative TX, `tx_busy=false`, monotonic RX, no reset or new HCD error. Ordinary
   new disconnect/reset-device/enumeration **during this follow-up** invalidates
   its settled/consistent metadata assumption: stop without retry. Do not filter
   logs out of strict query bodies or accept an early substring prompt.
   `Boot.wait()` uses substring matching for these prompts (`qc07f_boot.py:183–188,
   404–413`); caller terminal/unique-prompt and schema checks remain mandatory.
5. Check final same source/peer/start, final fresh monitor ownership, UART/idle
   inventory and full interval again; privately save exact per-command counters,
   nonce/transcripts, refresh boolean and final checkpoint. Public result may say
   **READONLY_POSTFAILURE_RETIREMENT_OBSERVED / ORIGINAL_TRIAL_FAILED_HOLD** only,
   with allowlisted booleans. It must not use the original functional-PASS filename
   or clear/truncate its HOLD. Finish/retire only this host read lease.

UART delta is **859 bytes** (539 metadata +320 inventory), or **860** if the one
empty refresh is needed. These counts assume complete admitted success, not a
license to finish sending after any earlier rejection. Per-call existing bounds
(shell6s, command10s, exit6s, inventory6s each, echo3s) fit within the proposed150s
total with ordinary overhead; enforce total deadline before every send too. A
short/unknown/late response is failure, not a longer wait/second attempt.

Positive results would support **bounded userspace/usbfs/watchdog/idle retirement
observations on the same boot**, from synchronous return plus source cleanup plus
absent PID1 FDs/cached inventory/no contradictory diagnostics. Current image has no
public all-URB/DMA/MODE/NAK/toggle counter. Do not upgrade this to electrical
watchdog-off, every-HCD-queue-empty, accepted original RUN receipt, ABI/network,
MTU1500, combined-memory or product qualification. If parent requires direct
all-URB/DMA telemetry before any later effect, this image cannot satisfy that gate.

## 5. Later choices — neither is automatically admitted/queued

### Separate single state/info verification

**Source-admissible in principle only as a new, separately reviewed/admitted
operation after the complete retirement-only sequence proves bounded known idle.**
The old whole-trial source GO does **not** supply this partial-result permission.
Require a fresh same-owner/UUID/counter checkpoint, catch/lifetime, no consumers,
valid settled descriptors, no HCD diagnostics, sticky original HOLD and explicit
parent selection of the separate scope. At most one `radio state`, then only on
complete APPLICATION schema one `radio info`, then their full sameboot metadata/
idle retirement. Neither query resends firmware/RUN, but both are **active**:
`init.c:332–390` opens usbfs/WDT, claims interface, CLEAR_HALT on1/81, and sends
ENTER20B or command43/12B. They are not retirement probes or a reset-free native
interval. Their5s watchdog/USB wait and unchecked-cleanup limits still apply.
Private identity fields remain private. Any negative/unknown result stops, no
repeat/fallback. Even successful separate identity cannot accept the original
corrupt151-ACK receipt or clear its failure HOLD.

### Watchdog recovery

**No immediate reset GO from the supplied failure facts.** If later known-idle
gates pass and parent selects recovery as actually needed, the existing
`WATCHDOG_RECOVERY_REVIEW.md` conditionally supports **one** `wdt reset`, not smoke,
Linux reboot or a firmware reload. Its entire fresh owner/catch/timeout/no-alias/
loop-budget checklist and genuine reset/held-U-Boot observation apply unchanged.
Existing14-query inventory does not read current watchdog timeout/identity or all
FD aliases: those reset-specific facts must be established by parent through
separately sealed passive admission, not assumed from logical inactive alone.
The loader's successful path requested5, but that is not a fresh timeout readback.

Watchdog opening already arms hardware; reset success intentionally keeps its FD
open without pinging (`init.c:78–105`). Do not inject it into an unknown active USB
operation. Prompt/`V`/close/inactive cannot prove electrical stop. Prefer an
already-prearmed operation guard if genuinely still active; a past guard open is
not evidence it remains armed after cleanup. If unknown/hung/control/catch gates
cannot be met, watchdog recovery is unavailable: report manual OFF/leaveOFF as
fallback, not a speculative command. No GPIO42 proposal.

An observed watchdog **SoC/main-CPU reset is NOT a cold radio/rail power cycle**.
It invalidates the old Linux execution context, not proof of normal FD/DMA cleanup
or clean radio bootloader state. Catch and hold original U-Boot; do not auto-boot,
reload/RUN, free quarantined backing, clear earlier HOLD or claim RAM76 restored.
If a radio cold-reset domain is actually required, this SoC reset does not prove it.

## 6. Host-only evidence

Executed once:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/failed-followup-review/check_host.py
```

**29 checks PASS**, Python/tool version, UTC, explicit public input hashes and
exact argv in `failed-followup-review/result.json`. The checker AST-selects only
pure parser/constants; it does not import trial/transport modules or instantiate
any private/target capability. Synthetic tests preserve rejection of interrupted
or missing RUN ACK despite DONE; distinguish ordinary disconnect from actual HCD
warnings; reject FD leaks, bad/incomplete descriptors/late logs, active WDT, UUID
change, UART errors, nonidle ISP and modules; verify fresh prompt grammar, byte
counts, source pins and unchanged inputs. Synthetic positive control is not this
device's receipt. No old exclusive-output collector was rerun. Native retirement,
application identity and reset remain **UNOBSERVED** here.
