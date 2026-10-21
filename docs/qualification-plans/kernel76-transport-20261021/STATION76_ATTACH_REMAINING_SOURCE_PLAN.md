# Station76 remaining finite attach-only corrections — SOURCE PLAN ONLY

2026-10-21; public-source audit **08:52:32UTC**. RP-02 / NET-01–08 / K76-06,
with RP-00/01 recovery dependencies. **No implementation, HOST behavioral test,
ARM build, staging or native operation is selected or authorized by this report.**

## 1. Decision / ONE smallest next source increment

After **exact A2b independent GO**, recommend that parent select **P1: an explicit
attach-only RX outer-type deny gate**, in a fresh isolated derivative, primarily
`hgic_smac/rx.c`. Admit only CMD/CMD2/EVENT/EVENT2 through the existing exact-original-
extent path; every other outer type fails through the existing receipt latch
**before normalization, trim, consumer dispatch or state/counter mutation**.
This is smaller and independently testable before changing init/queues/watchdog.
It qualifies exclusion, not the denied wire grammars. It does **not** close full
S4, S1/S2/S3/S5/S6, UP/RF, build or hardware admission. Nothing is queued.

P2–P5 below are separately selected subsequent packets, not a combined implementation
order to execute automatically. One finite future use case only: bind the known
running APPLICATION once, obtain one strict FWINFO, register a netdev that remains
**administratively DOWN**, then known-state normal teardown under the same guard.
No profile read, UP, scan, association, IP/DHCP, MTU adjustment, data or media test.
If source cannot make this boundary explicit without guessing, stop that packet.

### Frozen accepted inputs versus conditional dependency

- A1 independent review accepts local CMD identity/copy/FWINFO only. A2a independent
  review accepts original CMD/CMD2 USB extent/status and observed-failure RX
  resubmit inhibition only. Neither is a global transactional/lifetime latch.
- A2b was a **candidate pending independent acceptance at this lane's entry**.
  Author run08 and parent's public reproduction01 report run09 matches,
  12,127,356 event + 2,806,874 ingress + 150,705 A1 assertions per optimized/sanitized
  mode and ten compiled runtime mutants caught. Those results are **cited, not run
  or independently reproduced here**. P1 depends on exact accepted pins, not counts.
  If A2b review changes/refuses its pins/policy, rebase this plan before selection.
- **Handoff update:** the newly delivered `STATION76_EVENT_A2B_REVIEW.md` now grants
  narrow exact-pin GO for those same two files. Its independently run supplement
  reports39 ordered probes/656 assertions per mode and11 compiled runtime mutants
  caught, including a protected-payload read probe. This lane read that report,
  not its executables, and records its pin in `final-seal.json`. The A2b dependency
  is now satisfied for these exact inputs; **P1 still requires separate parent
  implementation selection**. Parent's latest handoff records local commitb940ffe;
  no Git operation was performed here. No other GO or automatic queue follows.
- Current power/runtime/watchdog UNKNOWN. Last actual06:43 RAM76 APPLICATION idle
  is dated. Old21a capture/protection expired/finalized;07:24 no owner/socket/armed
  recovery was historical parent HOST evidence, not rediscovered here. No HW queue.
  Original failed151-ACK result and original HOLD stay byte-exact and FAILED.

## 2. Audit custody and exact source anchors

Only this new report and `station-attach-remaining-review/` are owned/written here.
New reader `audit_public_source.py` was executed once with Python3.14.7:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-attach-remaining-review/audit_public_source.py
```

`reader-pins.json` records argv/cwd/UTC/version/reader hash, full-file public input
hashes and all **204 source pins**. Each exact68-file A1/A2a/A2b path set and file
extent/hash matched its own existing candidate seal and was rechecked after reading.
`public-excerpts.json` retains exact line/byte ranges, text and range hashes for
the findings below. Static seal equality is **not a behavioral safety test**.
An initial attempted read of nonexistent `utils/bootdl.c` is disclosed in the
reader receipt; actual selected definitions were found in `utils/fwdl.c`.

Abbreviations used below:

- **S/** = `apps/qualification/kernel76-station-events/source/` (frozen candidate).
- **K/** = `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.
- **B/** = `work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/build/`.

| Exact public input | SHA256 / interpretation |
|---|---|
| S/`hgic_smac/rx.c` | `600c2bccc48fae9ee5642fa320d83541b261bf24ff8d2d492c7871f6b97e2cf0` — conditional A2b |
| S/`hgic_smac/event.c` | `b00e6ee84c10f676c2ad7d3afe8abb0800d37224b98445d86acdddde774a6345` — conditional A2b |
| S/`hgic_smac/core.c` | `71a578f033148b2290b2b2b8de5ad540aeb7e4d348043a986615fcad07743452` — unchanged A2a |
| S/`utils/if_usb.c` | `500a5b4149bf506256542f5f19b2b8476051ef0749e110c483ec0b5f02672a79` — unchanged A2a |
| S/`utils/fwctrl.c` | `9a33bb351f36438f897ebccb2da54de285da24bf8a7eebe6ec813dff09a96c83` — unchanged A2a |
| S/`LICENSE` | `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643` — retained GPL text/notices |
| B/`.config` | `9e1f37788d3bd8580f2eb031ea2a7d795f4b61e2c1f89e1b91c11f1a87a6499c` |
| B/`Module.symvers` | `806467d52330d4cdf2fd8e312aee54d24b0a1437d5eb4bd1b393011f277c4cdc` |

Config read directly: HZ100; built-in cfg80211/mac80211; BT unset; IPv6 enabled;
MODULE_UNLOAD enabled, MODVERSIONS unset; WATCHDOG/core/sysfs and AK3918EV200 built
in; NOWAYOUT unset; HANDLE_BOOT_ENABLED enabled, OPEN_TIMEOUT0. No configuration
change/new kernel image is needed or proposed. No binary/image/ELF/firmware payload,
private config/capture, real MAC or private-input digest was opened/copied/exported.
Public source constants and public-source hashes are not device data.

## 3. P1: RX exclusion boundary — actual remaining reachability

Actual caller chain: S/`if_usb.c:224–263` → registered `bus.rx_packet`
(S/`core.c:1204–1211`) → S/`rx.c:207–325`. A2b makes types3/13/4/14 exact12+
and declared==original USB actual, no prefix. It **does not restrict other types**.
S/`hgic.h:212–234` is the authoritative outer enum; no opcode/type is invented.

| Current outer type / route | Existing behavior even in a proposed DOWN trial | Attach-only disposition |
|---|---|---|
| CMD3 / CMD2=13 | `rx.c:286–293` → fwctrl worker/response; A1 station association/copy | Admit exact existing envelope only; not new CMD2 TX permission |
| EVENT4 / EVENT2=14 | Same ingress → actual A2b consumer; only full ID18 silently owned-disposed, every other ID denies | Retain conditional A2b exactly; no structured delivery/source binding guessed |
| BOOTDL7 | `rx.c:242,253,258,290–292` bypasses declared>actual rule, trims, enters fwctrl | Deny before any legacy parser; do not infer a boot reply/padding grammar |
| OTA12 | `rx.c:291–292` enters fwctrl; worker explicitly accepts BOOT/OTA and non-CMD pending association remains cookie-only (`fwctrl.c:211–224,268–274,299–301`) | Deny, not a maintenance qualification |
| ACK1 (TX-ACK/block ACK) | `rx.c:260–273` updates last_rx before START check, can manipulate ack/trans queues if START ever becomes set | Deny before last_rx or ACK access; DOWN is not a grammar gate |
| FRM=2 / AGGFRM18 | `rx.c:274–285` updates last_rx then START/if_test check; consumers `:90–162` allocate/deliver to mac80211 | Deny, not silently accept/drop as proof of validity |
| SOFTFC11 | `rx.c:306–310` unconditionally changes txwnd and completes credit waiter, independent of DOWN | Deny before credit mutation/completion |
| BLUETOOTH19 | `rx.c:311–315` checks INITED, **not START or bt_en**; `:164–198` HCI conditional, otherwise whole skb queued to evt_list/up | Deny. `CONFIG_BT=n` and bt_en0 do NOT disable the raw Linux queue fallback |
| TEST2=10 | `rx.c:294–305` updates last_rx/test count even if if_test0; if_test3 reads fixed test extent | Deny; no test grammar/content probing |
| FIRMWARE5, NLMSG6, TEST8, FRM2=9, BOOTDL_DATA15, IFBR16, BEACON17,0,20..255 | Current default logs type/length/cookie and returns-1 (`:317–319`); A2a callback only promotes -EPROTO/sticky failure, not every-1 | Deny through the common sticky receipt path, no value/payload log |

The last row's `FRM2` is enum name `HGIC_HDR_TYPE_FRM2=9`, distinct from FRM=2.
Neither is admitted.
Wire-type labels are public protocol constants, not data receipts.

P1 must place a positive type allowlist after safe outer8 access but before any
prefix acceptance/normalization/trim/dispatch side effect. Existing allowed frames
still undergo all A2a/A2b exact checks; a prefixed allowed type must not evade it.
It need not dereference forbidden payloads or validate their inner structures.
Use existing `hgics_rx_error`/`hgic_fwctrl_rx_error`; no extra logger, metadata ABI,
new response opcode, actual-device identifier or queue. Preserve STOP precedence
and borrowed USB backing ownership: ingress does not free that buffer.

**Profile meaning:** this is a deliberately attach-only derivative with a sealed,
explicit source/build policy, not a guessed runtime default or `no_bootdl=1` promise.
P1 may hard-exclude types in that derivative without building a general profile
framework/switch. Any selectable later broader mode is a new review; no default
may silently select it. All exact68 A2b inputs remain immutable in their old tree.
Proposed sole owner: new `apps/qualification/kernel76-station-attach-rx/` including
its source/tests/runner and new evidence; mainly `rx.c` only. File names are scope
proposals, not created implementations or authority to copy/edit them now.

**Future real-path HOST requirement:** actual USB completion → ingress → fwctrl
worker → actual event consumer → A1 copy/wrapper, not a replacement RX predicate.
Exhaust all256 type bytes with outer8 and full12/minimal exact frames, before/after
INITED/START/if_test states; forbidden types with minimal backing must produce zero
consumer/last_rx/credit/ACK/mac80211/HCI/event queue/up/export hooks and one existing
denial, no borrowed-buffer free or output copy. Test lengths0..11, exact12/larger,
all16-bit declared lengths, surplus/concatenation,0xff prefixes, unaligned starts,
STOP/error/sticky callbacks and pending command before copy. Keep valid FWINFO and
debug18 paths plus first-wins/late-error limitations unchanged. Seed queue/credit/
last_rx sentinels and live skb accounting; use safe fake-printk as A2b did. Test
immediate/deferred workers separately, not as concurrency proof. Restore BOOT,
SOFTFC, BT and generic default acceptance as compiled runtime mutants; each must
be caught by behavior, not substrings or compiler failure. Parent reproduction
and independent exact-pin review required; no tests were executed in this lane.

**P1 exit remains narrow:** non-event grammar qualification and general BT/rawproc
repair OPEN; full S4 still NO-GO. If legitimate non-control traffic appears later,
one attach fails/HOLD; do not guess padding/ABI or relax the gate to force PASS.

## 4. P2: explicit attach-only effects + first-error/init lifetime (S3)

Proposed subsequent owner: fresh derivative of accepted P1; mainly
`hgic_smac/{core.c,hgics.h}`, `utils/{fwctrl.c,fwctrl.h}`, `hgic_def.h`, with minimal
USB admission-hook touch if required. Do not concurrently edit P1/P3/P4 trees.

### DOWN/start boundary — useful but insufficient alone

K/`net/mac80211/main.c:1595–1620` registers wiphy and creates a default managed
netdev, not UP. K/`iface.c:457–472` ndo_open reaches do_open; `:1419–1425` calls
drv_start before `:1501` drv_add_interface. Current S/`core.c:95–107` immediately
calls hardware start, sets radio_off0, sends DEV_OPEN, sets START and wakes queues;
the hardware2G start stub itself is no-op (`11bgn.c:208–210`). Reject start **before
all of those effects** for attach-only. Do not return success with radio still off.
Netdev DOWN is administrative flags/IFF_UP-clear, not merely operstate=down.

This refusal bounds the ordinary first UP path (must propagate failure; never set
open_count/running), **not all possible driver calls or physical RF emission**:
add_interface sends SET_MAC (`core.c:159–200`); config sets channel/power/retries
(`:281–319`); station/key/TXQ/AP/beacon callbacks also issue commands. Reconfigure
and direct callers must be audited, not assumed unreachable from one start guard.
Detect can call start directly (`:967–970`). Controller must never try UP to see
whether refusal works natively. Running opaque radio firmware can emit autonomously;
no source-only audit establishes electrical RF silence. If literal zero RF is a
required admission condition, it remains a separate unproved physical blocker.

Small qualification boundary: allow **only the existing GET_FW_INFO station
command** in this attach-only lifetime, once, from init; no new opcode. Root
`hgic.h:279` declares43; actual sender derives wire ID via existing HGIC_CMD_START,
not a new literal guessed on wire. Existing15-command station allowlist is too
broad for this use case. Enforce at actual command admission AND direct send_data/
USB TX boundaries, so retained maintenance/BT/AP callers cannot bypass it. USB's
current allowlist (`if_usb.c:298–308`) admits FRM/FRM2/AGGFRM/CMD/CMD2/SOFTFC and
BOOT command0; attach-only must deny non-owned/non-init commands and **BOOT ENTER**
too. An RX CMD2 allowance does not require new CMD2 TX. Init normal path requires
no DEV_OPEN/CLOSE or SET_MAC, including DOWN unregister cleanup; test that claim.
If real unregister legitimately requires an excluded command, stop/review exact
source before widening, never invent a synthetic success response.

### Remove recovery/retry, record terminal init

- `core.c:703–724` schedules detect on TX failure and clears BUS_ERROR on later
  success. `:993–1035` detects via boot ENTER, optional bus reinit, delay_init and
  timer rearm. Replace these for attach-only with first-error notification only;
  disable every trigger/work/timer/reopen branch, not merely a null USB reinit
  pointer or unused DISABLE_REINIT enum. `probe_post:1044–1048` may schedule exactly
  one init, never restart it. Retain loader no-op `:933–940`; no firmware/RUN/reset.
- One per-owned-module/device-lifetime first-error cause/phase, immutable once set;
  generation/identity binding is **local cached identity**, not a new wire token,
  authentication or cross-reset proof. Integrate USB submit/completion errors,
  invalid RX, FWCTRL allocation/no-wq, command allocation/timeout/firmware-negative/
  copy-policy errors, descriptor/probe/register errors. Accepted A1 first-wins and
  copy results stay what they were: a later error cannot retract them. No late
  success clears firsterror or turns a failed init into success.
- Explicit cached init NOT_STARTED/RUNNING/SUCCEEDED/FAILED plus start/done counts;
  only strict FWINFO and actual successful hw/wiphy/netdev/proc registration, still
  DOWN/errorfree, can publish success. EVENT FW_INIT_DONE never does. Current
  `delay_init:950–954` returns on failure without reporting lifecycle completion;
  `:977–984` can still queue test work after failed core_init. Remove that ambiguity
  and exclude if_test/if_agg/bt/dack overrides, don't silently coerce bad parameters.
- `hgics_init:1270–1288` ignores usb_register and notifier return values; propagate
  exact errors and unwind only initialized stages. usb_register success with no
  bound probe is NOT init success; finite controller must require the exact bound
  generation. Failures are cached/retained, no automatic unload/retry as a remedy.

Future HOST tests must run actual start/callback/command/send_data/TX and init/
detect/error paths with side-effect hooks: forbidden UP/add/config/key/beacon,
direct boot/OTA/BT/data sends, invalid overrides, first failure then successful TX/
valid FWINFO, duplicate init scheduling, FWINFO negative/malformed/no response,
core registration/proc errors, usb_register/notifier/no-device failures. Assert
zero recovery query/reinit/reopen/timer-rearm/retry and exactly one terminal init
receipt; test errors before/after init-success publication without claiming undo.
Runtime mutants restoring detect scheduling, error-clear, allowed DEV_OPEN or early
start side effects must compile then fail assertions. A1/A2a/accepted A2b/P1 regress.

## 5. P3: finite control ownership/deadlines (S2), no stack-waiter shortcut

Proposed owner: next isolated derivative, primarily `utils/{fwctrl.c,fwctrl.h,
if_usb.c}`, `hgic_smac/{core.c,hgics.h}` and minimal shared internal declarations.
P2 excludes data/test/soft-credit paths, but their absence must be enforced at the
real queue worker/transport, not an assumption about initialization timing.

Actual blockers: `if_usb.c:325` down_timeout1000 **jiffies =10s at HZ100**;
`hgic_def.h:45` command reply500ms; `fwctrl.c:313–331` can evict a queued skb,
publish/wait/time out/delete its stack response while queued TX survives;
`core.c:611–641,843–887` soft-credit/retry loops lack one total deadline. Boot
normal100ms queries are not needed in attach-only and must not become retries.

Define/seal absolute monotonic phase deadline before first effect, use remaining
budget at every queue admission/dequeue/token/submit/wait, never restart a per-step
budget on churn. Keep500ms reply ceiling, bounded init observation<=2s, guard5s
unchanged; prefer nonblocking USB token admission for this one control request,
fail on exhausted16 slots instead of10s wait. No blanket target timeout increases.
No credit polls or retry sleep/continue loops in this profile; fail once if a path
would need them. Before submit, recheck generation/STOP/firsterror/deadline under
the same serialized ownership transition; READ_ONCE alone is not that fence.

Required ownership design (review/test, not implementation snippets):

1. Distinguish **admitted/queued UNSENT**, worker-owned/preparing, submit-in-progress,
   accepted/submitted, completion-retired, and locally canceled/failed. Queue
   admission is not USB submission; control completion is not URB retirement.
2. Deadline/firsterror atomically removes only its **owned unsent** entry, reconciles
   waiter/queue references once, suppresses later dequeue/submit. No eviction of
   some other skb as queue-capacity handling; bounded capacity failure is sticky.
3. `hgic_fwctrl_do_cmd:95` has a stack receipt; current send_data removes pd_list
   before return. **Never attach a raw pointer to that stack receipt to an async
   skb/URB** to enable cancellation. Any asynchronous request state must have its
   own bounded refcounted allocation (or equally proved lifetime), independent of
   the stack waiter, and explicit waiter/queue/worker/USB references. Allocation
   failure is a first failure, not fallback to unsafe stack ownership. Sender must
   not free a request/command skb still owned by worker or transport.
4. If the worker has won the cancellation race and entered USB submission, report
   that fact as potentially submitted; do not label it unsent or undo it. Accepted
   URBs keep backing until callback/drain. Late response/completion cannot repair a
   deadline or retroactively prove transactional no-effect. No callback free of
   stack waiter, duplicate completion/free/refdrop or use of waiter after return.

Future HOST real-path tests: deterministic deadline before admission, after queue,
after dequeue, before submit, inside submit, after accepted submit, immediate and
late callback/response, waiter returns before late callback; exact500ms edge and
expired-before-wait; token0/exhaustion, STOP/error concurrency orderings, credit0
and queue capacity. Inject callback **before usb_submit_urb returns** as well as
deferred completion. Model submit failure separately: real USB success has one
callback; submit failure has none, so only the local failure owner cleans it.
Check conservation of every request/skb/token/ref and no post-deadline UNSENT TX;
submitted timeout remains failed/owned until retirement. Use barriers/threads for
new locking when feasible plus deterministic orderings; single-thread shim counts
are not SMP/IRQ proof. Mutants must expose stack lifetime/ref, missing cancel and
late-success acceptance at runtime under optimized and ASan+UBSan builds.

## 6. P4: exact allocation/probe and normal retirement (S5)

Proposed next derivative owner: mainly `utils/if_usb.c`, `hgic_smac/{core.c,hgics.h,
procfs.c,procfs.h}`, `utils/{fwctrl.c,fwctrl.h}` and minimal bus receipt declarations.
Keep16 TX/32 RX/2560B RX backing, not increased counts/ceilings. Public-source
facts requiring fault closure:

- `if_usb.c:365–409`: partial topology gate but no exact interface/alt/endpoint
  count, unique OUT01/IN81, bulk512 HS configuration validation before allocation/
  submission. Use source-supported expected descriptors + later private current
  descriptor continuity, not guessed interface numbers. Reject missing/duplicate/
  wrong endpoints/maxpacket before TX padding's ep_out_size division/alignment use.
- `qinit:124–155`: partial counts accepted at `probe:415–419`; RX backing allocation
  failure at:140–144 frees buf **without usb_free_urb**. Require exactly16+32 and
  rollback every successful prior allocation, including the current unlisted URB.
  `probe:421` ignores core-probe return; require both exact return and valid context.
- `core_probe:1140–1218`: early hghw mismatch can call core_remove before works/
  timers/ctrl are initialized; later fwctrl workqueue failure is not propagated.
  Track initialized stages so cleanup never touches an uninitialized primitive.
- `if_usb.c:427–448`: after get_dev/set_intfdata, initial RX failure drains/removes/
  frees without clear-intfdata/put_dev; stale pointer/ref leak. Distinguish device
  reference actually acquired, intfdata actually published, callback context live.
  Every path clears publication before invalidating its object and puts exactly
  once. Do not infer core lifetime merely from nonnull bus_priv after a failed probe.
- `submit_rx_urbs:210–221` drains rx_freeq while completions can resubmit/requeue;
  initialization must submit the finite initial32 slots once, not loop over an
  immediately replenished freeq forever. Separate list-used from HCD-inflight.

**Required ordering:** serialize STOP/no-new-TX/RX/worker admission → hold valid
context/URB references → kill/drain submitted URBs **outside qlock** → cancel/join
init/TX/detect/test/fwctrl work and timers with no requeue → unregister DOWN hw/
remove proc access and pending waiters → purge only locally owned queues → free
backing/core/bus and drop dev refs. Review exact ordering dependencies (especially
workers waiting on controls); cancellation must wake/error only live owned waiters,
never make shutdown depend on already-disabled RX. Existing USB quiesce before
core_remove (`:451–468`) is valuable but not proof that concurrent worker submit
cannot race the drain. Unlink under qlock (`:62–72`) is not synchronous retirement.
K/`drivers/usb/core/urb.c:678–721` guarantees idle after kill returns, forbids holding
callback locks, and contains an **unbounded wait** at:718. A blocked kill/cancel is
not permission to force free; prearmed watchdog may expire instead.

**Counter/lifetime requirement:** reserve inflight ownership and callback refs
BEFORE entering submit; callback can run before submit returns. Submit-error
rollback and successful immediate callback must be disjoint owners; no increment
after callback leading to phantom inflight, double decrement, token return or ref
drop. Keep submitters-in-progress separately from submitted-not-completed and
callbacks-active: an empty used list/inflight0 alone does not prove callbacks have
returned. Resubmission creates another owned interval; STOP fences that transition.
RX/TX submit-attempt/accepted/failed/completed/cancel totals reconcile per generation,
not from current status rebasing. Overflow/negative/impossible totals fail closed.

**Small cached receipt, not broad tracing/new kernel ABI:** extend/reuse the
module's read-only qualification status machinery with fixed bounded scalar fields:
module/source identity + operation/device generation; firsterror/cause/phase; init
started/terminal; allocated TX/RX; used/free/inflight/active-callback/submitter totals;
pending controls/queued-unsent/worker-owned/submitted controls; all ctrl RX/TX,
four data, trans/ACK/event and OTA queue lengths; delay/detect/TX/test/fwctrl work
pending/running and detect/beacon timer states; STOP/admission-closed and refs.
No receipt read issues a USB query, retries init or formats MAC/payload/addresses.
Current status (`procfs.c:9–42`) is insufficient and unsynchronized; define lock
order/coherent snapshot, not independently sampled zeros. Status must exist for
init failure and survive bus teardown as a **module-owned fixed cached tombstone**,
not retain dangling hg pointers. No arbitrary logging/trace ring or global guardian.

Keep scope small: no raw `fwevnt` endpoint creation in attach-only; no iwpriv writer,
BT/HCI/tool or event export. P1/A2b exclusion is not a general fix to `fwevnt`'s
ignored count (`procfs.c:61–82`). Existing opens/seq private-data need rundown:
K/`fs/proc/inode.c:255–273` waits active callers and closes registered openers; remove
access and finish that rendezvous before freeing referenced objects. Never hold
a status FD open across module removal. There must be no use-after-free reader.

Normal unload needs a terminal receipt **after** device drain but **before** module
text disappears. Smallest candidate design: module-owned cached tombstone and one
bounded fixed-format final summary at successful module exit tail, plus exact
delete_module success and sameboot postinventory, consumed privately by parent.
No post-unload proc read, concurrent UART sender or inferred heap pointer. Source
review must prove that tail follows every reference/work retirement and all
registration unwind. If final receipt cannot be delivered/verified in budget,
normal result is HOLD; do not call an early snapshot final. No new kernel ABI or
new kernel image is proposed; this is a private qualification module/helper schema
which itself requires independent review before implementation/native selection.

Future HOST tests must invoke complete actual qinit/probe/core-probe/disconnect/
quiesce/free/control-release paths with allocation/ref/list/work/timer/USB fakes.
Fault **each** buf/URB/RX allocation index (16TX/32RX), core/hghw/wq/conf/proc/hw/
notifier/USB registration and each initial RX submit index; pre/post-intfdata/devref
failures; absent/duplicate/wrong endpoint/alt/config/speed. Test immediate completion,
submit failure/no callback, callback reentry with STOP, worker preparing during
STOP, queued work/timer trying to rearm after cancel, read racing proc rundown,
waiter racing release, and a modeled never-returning kill/cancel. Track exact once
free/ref/token conservation and no access to dead objects; stuck retirement must
never reach free or normal terminal receipt. Required compiled runtime mutants:
partial allocation accepted, missing free_urb/put_dev/clear-intfdata, free before
drain, kill under callback lock, post-return inflight increment, resubmit after
STOP, stale proc/hg pointer, omitted work/timer retirement. HOST simulation cannot
prove real IRQ/HCD/DMA behavior; leave that explicitly E3, not fixture closure.

## 7. P5: one-shot OWN volatile C helper + small parent controller (S1/S6)

Only after P1–P4 independently accepted. New proposed own helper/controller tree
`apps/qualification/kernel76-station-attach-runner/`; no edits to sealed identity/
retirement/recorder/monitor/boot helpers. Use their reviewed **gates**, not existing
mains requiring empty HOLD or dated counters. No generic service/guardian, kernel
ABI project, PID1 replacement or new image. Module packet and helper/controller
schemas must be jointly reviewed before any ARM build selection.

`bringup/qc07f/initramfs/wifi-monitor.h:12–21` arms5 then finit_module, waits2s and
unconditionally V/closes: **forbidden for this operation**. A shell timeout, prompt,
operstate or module syscall success cannot own asynchronous initialization/drain.

Small helper contract to design/test:

1. Fixed one operation, sealed exact module/controller/source pins, explicit
   attach-only policy; no arbitrary exec, profile parser, network callback or
   parameter override. Volatile exclusive staging with finite size/lifetime; no
   touching embedded `/hgics.ko` or original image. Precheck exact runtime/USB/FD/
   module identity and no existing owner before effects. No target Python assumed.
2. Sole helper opens exact `/dev/watchdog0` RW, checks device identity/nowayout and
   UAPI/support, requests **5** with WDIOC_SETTIMEOUT and requires exact returned5
   plus GETTIMEOUT5 **before finit_module**. Open itself arms hardware, so partial
   setup after open is not a zero-effect refusal. Audit each arm failure explicitly;
   if armed state is uncertain do not invoke module or routinely magic-close.
3. Keep the same sole WD FD through async init and known normal teardown. Small
   parent/supervisor and one child for blocking finit_module/delete_module may be
   needed; no child WD alias/inherited FD (close in child/CLOEXEC as appropriate),
   no second feeder, FD theft or accidental magic V. Supervision remains possible
   if syscall worker blocks; worker identity/PID/start and ownership are retained,
   never kill it to manufacture completion. One module syscall each, no EINTR/
   lost-reply automatic replay; reconcile ambiguous side effects as unknown.
4. **Feed only verified OWNED phase progress** from coherent module receipts and
   actual syscall milestones, for the same generation, errorfree and within fixed
   subdeadline. Each progress edge accepted at most once with explicit budget;
   identical snapshots, generic RX resubmit counts, BUSY churn, debug floods,
   polling, live process, UART responsiveness or parent heartbeats do not count.
   No endless feeder/thread/timer. Receipt access itself may block: no independent
   keepalive during an unverified read. A blocked single syscall/drain must allow
  5s expiry, not stretch it with artificial progress. Feeds cease before hard
   deadline; late callbacks/healthy status do not restart feeds or clear failure.
5. **<=60s total effect phase** from WD open (including module/init/normal retirement),
   initial capture+future catch coverage **>=360s**, final reserve **>=300s**.
   Keep init observation<=2s and command<=500ms; table exact fixed phase bounds,
   monotonic checks and UART/RPC10s overhead reserve in the later controller design.
   Reserve is outside effect time, not permission to feed300s. Staging/pre-RO has
   separately calculated bounded time/coverage; recheck before first effect.
6. Normal delete_module only from exact responsive SUCCEEDED/errorfree/owned/DOWN
   state with coherent preteardown receipt, no outstanding profile or user jobs.
   Kernel normal teardown must then prove P4 STOP/drain/work/ref retirement, not
   force-unload or use a timeout to skip it. If its kill/cancel wait blocks, guard
   expiry is the recovery path, not forced free. Do not normally unload a failed,
   late or unknown init to discover whether it is safe.
7. After verified normal retirement/final summary/syscall and bounded sameboot
   post-RO/no modules/driver/proc/netdev/ref/WD alias, only the known normal branch
   may writeV/close and corroborate stop. All fields/results are checked. WDT
   close0, sysfs inactive or a prompt alone is not electrical stop proof. Final
   receipt/disarm/postchecks must fit the guard/progress budget; otherwise decline
   design admission rather than extend target waits or add an endless feeder.
8. On uncertainty/error: stop new commands AND feeds; retain module/request/backing,
   subordinate HOLD and sole owner; no kill/unload/reopen/repair query/retroactive
   `wdt reset`. Observe the **already armed genuine main-CPU reset + U-Boot catch**
   through the existing receive owner. Helper retains WD FD until reset where
   possible; unexpected exit/nonmagic close is not intentional retirement (core
   may ping once). No repeated recovery attempts. Manual OFF fallback only if
   watchdog/control/catch unavailable/fails or reset domain insufficient, not
   first-choice routine recovery. CPU reset is NOT cold radio/rail reset, normal
   URB/DMA retirement, permission to reload/RUN, or clearance of either HOLD.

Actual watchdog support: overlay and K/`arch/arm/mach-ak3918ev200/watchdog.c:39–75`
program keyed TOP counter; UAPI `include/uapi/linux/watchdog.h:24–31`; core
`watchdog_dev.c:864–919` single open/start, `:804–826` set/ping/get, `:697–735` V
also pings, `:942–989` release returns0 even if stop fails. `:76–97` excludes active
userspace auto-feeding when max_hw_heartbeat0 (driver leaves it0), but inherited
logical-inactive HW may be fed under selected config. This supports prearming,
not every-hang/reset-domain proof. Preserve `WATCHDOG_RECOVERY_REVIEW.md` limits;
no reset qualification or recovery was attempted here.

Future HOST real helper/controller tests: compile actual own helper with syscall/
clock/receipt/FD fakes, not a Python-only mirror. Open/support/set/get/ping/V/close
and short/EINTR/error paths; child spawn/fd alias/exit/wait ambiguity; delayed init
past2s; blocked finit/drain/receipt read; unchanged/generic-RX/flood/replayed/wrong-
generation progress; firsterror concurrent with progress; hard60s/coverage359s/
final299s boundaries; late good result; worker still alive on timeout. Assert no
unload/kill/new effects/V/continued feeds on unknown; exact module call count and
WD FD lifetime. Parent nested-send/owner/partial-line diagnostics guards must cover
every low-level TX before/after blocking RPC, original HOLD path/inode/bytes and
separate subordinate HOLD. No real private input required: synthetic lineage only.
Missing/expired catch/source change/unexplained TX/topology/WARN produce zero new
effect commands. Runtime mutants restoring fixed2s close, BUSY-based feeds, clock
restart, hidden retry, unknown-state unload and child FD inheritance must be caught.

## 8. Packet order and build/native gates — not self-authorization

| Stage | Required review/evidence exit | Explicitly still forbidden |
|---|---|---|
| E0 now | This bounded plan, exact public anchors/pins; conditional A2b dependency | Implement/test/build/stage/target queue |
| P1 next, only if parent selects after A2b GO | RX-only derivative; real-path optimized/sanitized tests + runtime mutants, parent reproduction + independent exact review | Full S4/S1–S6/build/attach claims |
| P2 then P3 then P4 | Each fresh derivative of last accepted tree; changed-file ownership/seals/license, real fault tests/regressions and independent review before next selection | Parallel overlapping source edits or clearing old failures |
| P5 | Exact bounded helper + parent controller source/HOST review, module receipt compatibility; compile/runtime negatives; no private source fixtures | ARM build/staging/HW until separately selected |
| Later E1 paired ARM build | Separate parent build selection; frozen whole-source/helper pins, all earlier independent GO; exact retained6.18.55 config/Module.symvers/generated headers/UAPI/compiler recipe and build argv; inspect selected2G+USB object/callback gates/undefined refs, helper syscalls/FD handling, size/memory manifest | New kernel/image, force-load, treating vermagic-only as pairing, using old embedded pin |
| Later E2 native admission | Parent alone: fresh physical/sole owner/peer/PID-start/UID/source, exact boot/module/USB/runtime/HOLD/TX lineage, no competing work, unconsumed future catch and360s coverage, sealed single-use recipe | Old PID/IP/deadline/06:43 runtime or07:09 capture authority; automatic native trial |
| Later E3 | At most ONE finite DOWN attach/init/normal-retirement envelope; actual cached coherent receipts, final callback/work/ref closure + sameboot post-RO, or preserved failed/HOLD/reset-catch outcome | Retry/reload/RUN, failed-init automatic unload, physical RF-silence/transport qualification from module load |
| E4 later | Separate RP-02 network/transport/media/coupled/security/product acceptance | Declaring G2/G3/G8/full network product complete from this attach slice |

Future changed module **must have a new full hash/size/generation pin different
from embedded `e667ed2a1f726c137330638f6ff64eeab085c0bc3247ed2cac1919594e29ef29`**.
Original image/module and A1/A2a/A2b evidence remain intact. A newly compiled module
is never called the embedded paired module. Kernel/config/symvers stay pinned;
MODVERSIONS=n makes exact pairing necessary, not sufficient. No binary was opened,
built or staged for this report. Build success and HOST counts are not native GO.

Fresh E2 must also preserve original identity completion event lineage and original
failed HOLD without status-counter rebasing, validate current idle FD/USB cached
descriptors/volatile mounts/ISP/watchdog/UART and private diagnostics interval,
including split carry-in lines, HCD/WARN/active-EP CLEAR_HALT/Babble/RESUME/
DISCONNECT/new kernel/reset/topology gaps. No raw UART response/digest is public.
Subordinate attach HOLD does not replace/truncate/clear original HOLD.

## 9. All remaining NO-GOs / privacy and product boundary

- **S1/S6:** own5s prearmed progress helper/controller not implemented/reviewed;
  current fixed2s wifi attach guard forbidden. **S2:** total deadline/owned-unsent
  cancellation/lifetime unclosed. **S3:** autonomous recovery/reopen/firsterror/
  terminal init receipt unclosed. **S5:** full allocation/partial probe/submit-
  callback concurrency/URB/work/timer/proc/ref retirement unclosed.
- **Full S4 remains OPEN:** accepted narrow A1/A2a/A2b are not all RX/
  maintenance grammars or privacy/lifecycle proof. P1 would exclude unsupported
  BOOT/OTA/BT/data/ACK/softcredit/test paths for attach-only, not validate them.
  Event tails remain opaque: ID18 no read/log/hash/export, all others deny; no
  event source binding/authentication or structured payload ABI guessed. General
  rawproc count/copy/semaphore/lifetime/tool/BT routes remain unqualified outside
  explicitly excluded attach profile; no later broader mode inherits acceptance.
- No GPIO42/device power, bus/endpoint reset, firmware write/RUN/reload/fallback,
  watchdog timeout growth, generic kernel ABI/image changes or active probing to
  learn current state in this lane. Existing GPIO1 speaker/UART isolation, GPIO2,
  flash/ENV/OTP/card/audio/media/light/motor restrictions stand; no extra effects.
- Optional address/prefix-only DHCP callback, explicit IPv6 UP traffic policy,
  profile/UP/association, routes/DNS, authorized host endpoint and MTU384→1500 are
  **later separate RP-02 work**, not required to complete this DOWN slice. Default
  DHCP script's route/DNS effects remain forbidden for local-only future use.
  No AP/newSSID/Internet/discovery/cloud/security-regulatory bypass authorization.
- No cold entropy/trust, idle-memory-as-budget, RP-04 combined memory/adoption,
  media/FPS/coexistence or G8/product/other-camera acceptance claim.

**Handback:** select only P1 SOURCE/HOST after exact A2b GO if desired, then review
its actual behavior. All other packets remain plans awaiting separate selection.
This lane ran static public-text inventory/excerpt reading only, no host tests,
compiler/build, Git/index edit, target/RPC/status/UART, network/discovery, private
input, firmware/stock ELF read, capture renewal or reset. Current hardware remains
unknown/unprotected, original151-ACK FAILED/HOLD unchanged, no HW command queued.
