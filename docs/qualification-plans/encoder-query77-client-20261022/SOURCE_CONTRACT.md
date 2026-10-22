# ENC-02 / PLAT-01 — query77 qualification client contract (before code)

2026-10-22. SOURCE/offline compile only. No native recipe, upload, boot or execution
is selected. Parent alone owns hardware. Last recorded target observation was held
original U-Boot at12:12UTC, not current state; consumed catch and both HOLDs remain.

## Inspected real implementation

Let `K = work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux`.

- `K/arch/arm/mach-ak3918ev200/qc07f_hwenc.c:447–459`: one successful open
  allocates an8-byte zeroed session, takes `enc.lock`, refuses quarantine/poison/
  exhausted session namespace, increments the boot-lifetime sequence and installs
  private data. No codec reset, MMIO, DMA allocation, job or reference acquisition.
- `:361–385` dispatches the exact88-byte OPEN_QUERY before other commands. Frozen
  `encoder-open-query.inc:5–45` locks, snapshots actual calling-file identity,
  owner relation and telemetry, and performs one bounded userspace copy. No state
  mutation/hardware effect. Mutex acquisition/copy/open/release are NOT bounded.
- `qc07f_hwenc.c:431–445`: release clears matching H264 owner/history, then frees
  only session private data. For this sole open, which issues ONLY queries and
  observes NONE, no matching-owner branch is expected under trusted no-other-work
  admission. A surprising relation/error is retained, not repaired/REFCLEARed.
  No DMA backing is reclaimed by this normal query-only release.
- `:480–506`: init reads two syscon registers, reserves/maps codec MMIO, registers
  a platform DMA device/mask and allocates CPU telemetry; publishes misc device
  `qc07f-encoder`, mode0600, dynamic minor. Init does not start/power/reset the
  engine or allocate codec DMA pools. No supported remove/unload/reprobe exists.
  Client uses only `/dev/qc07f-encoder`, never INFO/JPEG/H264/STAGE/control ioctls.
- Original56-byte global query is optional and deliberately omitted. A clean
  OPEN_QUERY does NOT expose live allocation totals or prove prior DMA retirement,
  global idle, no aliases, never-used hardware, authorization or safe cleanup.

Reviewed frozen query source aggregate:
`ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`.
Use an exact byte import of `apps/qualification/kernel76-encoder-identity/encoder-open-query.h`;
do not import the implementation or whole kernel. Require syscall0/version1/
bytes88/reserved0; ENOTTY is failure, including old76, with no identity fallback.

## Real watchdog effects and retirement order

Inspected `K/arch/arm/mach-ak3918ev200/watchdog.c:32–129` and
`K/drivers/watchdog/watchdog_dev.c:76–320,444–587,750–989`, plus previous source
`bringup/qc07f/initramfs/init.c:66–105`, public
`WATCHDOG_RECOVERY_REVIEW.md`, `WATCHDOG_SMOKE22B_PLAN.md` and
`WATCHDOG_RESET22B_RESULT.md` in kernel76 transport docs. The public recorded
normal5s smoke/sameboot/FD postflight PASS12:05UTC is historical, not query77
qualification or current authority. No private receipt/device/host `/proc` read.

- The core enforces single-open and **open already starts the current timeout**.
  TOP start writes the keyed reload/control registers; timeout setting restarts
  active hardware and core pings again. Thus configuring5s is an initial arm,
  not a passive query. Exact support is SETTIMEOUT|KEEPALIVEPING|MAGICCLOSE,
  identity `AK3918EV200 TOP watchdog`, firmware version0. Driver heartbeat fields
  remain0: no routine core feeder for this active userspace session.
- Before the watchdog open, read only fixed sysfs identity/nowayout/state/timeout
  and require exact driver/nowayout0/logical inactive/current timeout1..60. These
  metadata checks and uid/release/device metadata are NOT hardware authority,
  ownership admission or electrical readback. Race exclusion belongs to parent.
- Start one immutable **1.5s whole effect-phase monotonic budget immediately before
  the only watchdog open**; configure nominal5s once, get support/timeout, then
  open encoder and query twice. No feeds, writes, enable retry, timeout renewal,
  guardian, second watchdog path, fork/dup/thread/SCM/provider/token are present.
- Normal path requires both exact validated snapshots to be identical, nonzero
  session, NONE/owner0/frame0, all fault/telemetry/ledger flags0, and virgin
  history/generation/dispatch0. This intentionally selects a fresh no-job boot,
  not a general history reader. Even these facts do not establish global idle.
- Within the original budget, clear encoder slot **before one close**. No close
  retry, including EINTR. Error means number no longer claimed owned, not proof
  the description survived or release completed. Keep actual watchdog/process.
- Only after validated no-work path and successful encoder close, within budget,
  one `WDIOC_SETOPTIONS(WDIOS_DISABLECARD)` performs logical stop. This avoids
  magic-write's extra feed. Core stop calls TOP stop and clears active on success;
  subsequent core release of inactive description does not ping/restart. Clear
  watchdog slot before its one close. No claim of electrical stop from either.
- Any error/late result/clock regression once watchdog open is attempted enters
  nonreturning failstop; keep all still-owned descriptions and process, do not
  retry, close, disable, free, query for repair or service failed work. Catchable
  signals are blocked before effects (SIGKILL/kernel death remain unavoidable).
  No failure diagnostic writes can block before failstop. ENOTTY is retained
  failure, never a successful/unsupported identity. A failed open may already
  have effects without returning an FD; failstop cannot invent ownership.
- A failed/late disable may ALREADY have stopped hardware. A watchdog close error
  may ALREADY have released it. Failstop then makes **no armed-recovery promise**.
  Process retention is not independent external retention proof. No deliberate
  reset/fault trial or automatic retry is included.

## Mandatory unimplemented future parent admission

A separately selected/reviewed parent recipe must freshly prove exact boot/image
association (release `6.18.55-qc07f-encoder-query77`, image SHA
`f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e`),
fresh sole UART/capture/process ownership, no active provider/jobs/DMA or unknown
aliases, global idle, watchdog association/no existing owner, exact nodes and
no concurrent nowayout changes. The process must be launched after all fork/exec,
with no inherited device description, bounded nonpersistent output sink, adequate
stack/backing, independent lifetime/capture and fresh unconsumed original-U-Boot
catch. Parent must provide manual OFF fallback if guard/catch cannot be proved or
disable uncertainty loses recovery. argv opt-in is NOT such proof. No new ready
Boolean or token substitutes for these missing facts. None is implemented here.

No module load/autoload: embedded station76 is unpaired/UNSUPPORTED. Frozen75/76/
77/config/initramfs/images/module/recipes stay immutable. No monitor command exists
for this query. This ELF removes only a future client dependency, not the native
bootstrap/control-handback gate. Preserve128KiB allocation cap,40MiB/CMA and2MiB
child/4MiB HOST limits; this helper is not a verifier or full app/memory-fit claim.

## Source/build boundary

Minimal real explicit qualification main; exact one-trial argv required, default
refused before I/O. Fixed small stack/structs, no authored malloc or payload
buffers; numeric struct facts only after normal device retirement. No addresses,
device identifiers, UUIDs, raw payloads, private configuration or digests output.
HOST and retained pinned musl ARM compile/static link/ELF inspection ONLY. No
program/test/helper payload/generator/listener/child/camera execution, kernel/module
build, native staging, Git/index/central-plan edits or other lane inspection.
