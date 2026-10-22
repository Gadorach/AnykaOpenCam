# ENC-02 / PLAT-01 — independent query77-client SOURCE/API review

2026-10-22. **Narrow conditional SOURCE/API query-client sufficiency, with static
compile custody verified. No correction-required ordinary defect identified in
the selected main/query/guard/cleanup flow.** This does not admit native use,
staging, a parent recipe, watchdog recovery, global idle, product memory fit or
app composition. No functional/runtime result or automatic next queue follows.

Reviewed five-file source aggregate:
`101afbb482c4562d02e58ab7c2aafed0d3cbb76a7d7b4cc36cf08e667fc80600`.
Main source: `apps/qualification/encoder-query77-client/encoder-query77-qualification.c`,
267 lines, SHA256
`d132a2eeaae170586aa437a2e7a3102027d9ff656077c5102f57dd79d95268b7`.
Exact imported UAPI SHA256:
`6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37`.
Source aggregate recipe: SHA256 of concatenated sorted workspace-relative UTF-8
path + NUL + raw file SHA256. There is no client SOURCE_SEAL file: actual authority
for recorded source/evidence pins is HANDOFF_SEAL01/OFFLINE_CLOSURE01 plus original
local build receipts, not an invented seal.

## Reviewed scope and real source association

Full candidate report/contract, actual main/imported88-byte UAPI and build/metadata
recipes were read as source. Relevant real open/ioctl/query/release/init, telemetry
initialization, TOP driver and watchdog-core APIs were inspected, not executed.
Let `K` be:

`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux`.

Frozen five-file query source aggregate remains
`ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`.
Actual K provider C/H match its pinned candidate evidence; actual query header/inc
match exact original source-file pins. Client header is byte-identical to both
original and K. Actual provider dispatches OPEN_QUERY before nonquery command
history/effects. Pinned image association and parent disposition are at
`docs/qualification-plans/encoder-query77-build-20261022/PARENT_DISPOSITION.md`.
The image SHA remains
`f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e`;
this review did not repeat its whole250,219-path kernel build verification or boot it.

Ordinary SOURCE/API/control/lifetime/error review only, not a broad device/security/
parser audit, test campaign or architecture expansion. Only fresh files in this
review directory were written. No candidate/author script/helper/build/compiler/
generator/test/guard/listener/client/default/help/program/ELF execution, target/
UART/RPC/status/network/private IO, other active-lane inspection/composition, Git/
index/central-plan/AGENTS/P1/model/provider/safeguard edit occurred. Independent
reviewer Python performs static public read/hash/receipt/ar/ELF inspection only,
without candidate imports or subprocesses.

## Main flow — actual code, not metadata as permission

Line references here are to the267-line client main source.

| Boundary | Actual SOURCE/API finding |
|---|---|
| Explicit invocation | `:197–212`: any argc/argv other than the single exact one-trial option returns2 before authored I/O. Real/effective root and exact query77 uname release are required. Fixed sysfs nowayout0, logical inactive, identity and timeout1..60, then root0600 character-node lstat checks precede effects. Refusal codes3/4/5 remain pre-device-open refusals, not native results. |
| Metadata limitations | `:77–128`: fixed sysfs files are read-only with CLOEXEC/NOFOLLOW, one bounded read/one close and exact syntax; opened device descriptions are fstat-matched to observed dev/inode/rdev/mode/uid. These are point facts, not exact boot/image, global idle, hardware inactive, sole owner, no aliases or recovery proof. Parent FD-table/global-state serialization and actual image admission remain trusted prerequisites. |
| Signals and phase budget | `:40–65,210–220`: catchable signals are blocked before first device effect; immutable1.5s CLOCK_MONOTONIC phase starts before watchdog open. Each returned effect/clock observation is checked against original start and nonregressing last clock; no reset/renewal of that budget. These checks cannot preempt blocked open/mutex/ioctl/copy/close, guarantee scheduler latency or prevent SIGKILL/OOM/kernel death. |
| Guard first | `:218–238`: only watchdog0 device open, RW/CLOEXEC/NOFOLLOW, before encoder. Open already starts prior timeout, possibly even on a failing open without returned FD. Actual support exact options/identity/firmware0 are validated; SETTIMEOUT5 is attempted once and must return/write back5; GETTIMEOUT must return5. No keepalive ioctl/magic write/timeout renewal/second guardian/fallback path. Initial SETTIMEOUT itself restarts/pings in the actual kernel, not a passive query. |
| Encoder and exact query | `:240–247,133–153`: only encoder open, same flags; two calls of exact0x80585166 command/88-byte struct. Both require syscall0/version1/bytes88/reserved0, nonzero per-open session, NONE/owner0/frame0, faults/flags/ledger0, virgin generation/dispatch/operation/history0. All88 bytes must match across snapshots. ENOTTY/interruption/partial-copy/error refuses without retry or global sequence fallback. No INFO/JPEG/H264/STAGE/lifetime/control/REFCLEAR command exists in the main. |
| Deliberately narrow history policy | The full UAPI can expose history and other-owner relations; main intentionally admits only the fresh no-job subset. This subset is source-possible after fresh initialization, not a promise that an arbitrary boot qualifies. A clean snapshot does not establish authorization, exclusive custody, prior DMA retirement, global noninterference or a future no-job interval. |
| Normal close/disable | `:247–257,67–72`: only validated no-work sequence and successful encoder close permit one SETOPTIONS(DISABLECARD). Actual slot is cleared **before** each one-shot close; no EINTR/other close retry or reuse of error FD. Deadline/clock checks also follow disable and watchdog close. Only this normal branch reaches output/return; no general DMA-retirement conclusion follows from successful close/query. |
| Any post-open uncertainty | Every checked failure/late result once watchdog open is attempted goes to nonreturning pause with no failure output, new close/disable/query/repair/feed/reset/free or retry. Still-owned descriptors/process stay present while process survives. A failed open can have effects without returning ownership; a close-error slot is already cleared and its number is **not** retained/retryable. This is source failstop, not an external owner or current armed-recovery guarantee. |
| Disable/close uncertainty | A disable that returns late/error may already have stopped the timer; late/error close may already have released the description. Subsequent pause must not be labelled armed recovery. Known states require future parent observation/association and manual OFF fallback; no automatic rearm/recovery is supplied. |
| Output/exit | `:155–184,259–266`: two numeric lines in exact struct order, only after normal device-close/disable path and final phase check.17 fields*21 bytes <=357 <384;20-byte reverse scratch is sufficient for uint64. Signed result is validated0 before cast. No raw struct/payload/address/boot UUID/private config/digest/device label output. Numeric session identity is the explicit UAPI fact, not a physical identifier. Short/failed write returns6; writes may block after devices retired, so future parent must supply bounded volatile sink. No further device control follows output. |

There is no authored malloc/VLA/recursion, fork/dup/thread/SCM/token, alternate
device path, inherited-device fork within the program, capture/media/peripheral/
storage/network/module/job operation or readiness callback. Libc startup/syscall/
allocation behavior is not claimed absent merely because authored code is small.
Future launcher ordering must place all fork/exec before device acquisition and
exclude inherited/competing descriptions. Normal target completion still needs
observed stdout **and** exit status, not an invented Boolean or mere lack of errors.

## Actual kernel semantics supporting the conditional path

Under `K/arch/arm/mach-ak3918ev200/`:

- `qc07f_hwenc.c:45,59–79,92–93,447–459`: the real session is one8-byte CPU record;
  successful open increments actual boot-lifetime session namespace under enc.lock.
  No codec MMIO/reset/DMA allocation/job/reference acquisition occurs on open.
- `:361–385`, `encoder-open-query.inc:5–45`: exact command reaches query before
  nonquery dispatch/history. VFS file reference and enc.lock protect actual session/
  reference snapshot; zero-initialized88-byte output and bounded copy; no history/
  owner/counter/MMIO mutation by this query. Mutex acquisition/copy can block.
- `qc07f_hwenc.c:431–445`: release clears owner/history only if THIS file is actual
  H264 owner, then frees CPU private session. In the trusted sole query-only/no-job
  path with validated NONE that owner branch is not entered. This does not make
  query/close a general source/DMA cleanup API or evidence about prior hardware.
- `:480–506`, `stage-telemetry.h:24–61`: init installs zeroed telemetry before misc
  publication; coverage initializes supported masks, not operations/flags/owner
  history. Fresh zero-history subset is therefore possible. Init maps/reserves
  codec MMIO/registers DMA device/mask/reads syscon but does not start codec/DMA
  pools. No supported unload/reprobe exists; client adds neither.
- `watchdog.c:32–129`: actual TOP identity/options/version0, keyed start/ping/stop,
  set-timeout range/store/restart, nowayout policy and boot hardware-running flag
  remain as pinned. Logical state alone cannot rule out inherited running hardware.

Under `K/drivers/watchdog/watchdog_dev.c`:

- `:76–142,248–320,368–391,864–919`: core single-open and actual start at prior
  timeout; active/HW-running bits, timeout set and worker policy are real effects.
  TOP leaves min/max hardware-heartbeat fields0; the ordinary active-user timer
  predicate then requests no routine worker feeder. This is not a claim to have
  proved current inherited/queued work or every native recovery condition.
- `:750–850`: GETSUPPORT exact copy; SETTIMEOUT calls driver, pings, copies back;
  GETTIMEOUT copy; DISABLECARD calls stop. No implicit permission flag substitutes.
- `:292–320,942–989`: successful TOP stop clears HW-running/active under core lock;
  subsequent inactive release sets err0 and does not execute release's error ping.
  `:697–735` shows why avoiding magic write matters: any nonempty write feeds.
  No electrical-stop/time calibration is proved by logical return values.

Historical init monitor smoke/reset source and public source-bound watchdog review
were read; actual query77 watchdog/core bodies match retained binding pins. Old
normal5s smoke and held-U-Boot reset observations are **not** query77 qualification,
fresh ownership, current timer/catch coverage or native admission. No private
receipt/process/recorder state was read.

## Build / closure / static artifacts — PASS, cited not rerun

Author build01 started16:10:15.294839UTC, ended16:10:20.992155UTC.36 recorded tool
commands all exit0: two compiler/version, two readelf/version,24 support-location
queries, two dependency preprocesses, two actual C compiles, two static links and
two ELF inspections. No executable/default/help/test or imported builder invocation
appears in reviewed recipe/records. This is not a process audit of unobserved work.

Independent static collection commands (no compiler/subprocess/candidate import):

```text
python3 -B docs/qualification-plans/encoder-query77-client-review-20261022/collect_static.py before03
python3 -B docs/qualification-plans/encoder-query77-client-review-20261022/collect_static.py after03
```

Verified:

- Five actual source files, exact UAPI/kernel association, HANDOFF documentation/
  source/closure/report/output-seal pins;3,710 INPUTS,3,572 PRE_INPUTS subset,
  all252 original build outputs. Includes exact source/header/toolchain/archive/
  CRT/linker inputs and supplied public kernel/source/license/image custody.
- Full retained ARM compiler/wrapper/binutils/runtime/header/symlink map and four
  retained expected pins. Exact HOST119/ARM37 preprocessed dependency sets match
  actual .d maps and each copied dependency body. Compile/link argv reconstructed
  independently; strict warnings/Werror/Wpedantic/stack limits and optimization/
  ARMv5TE soft-float/static/no-pie/GC/map/trace flags agree. No object reuse/stub.
- Actual link trace/LOAD paths all pinned; ARM synthetic `LOAD linker stubs` is
  a generated BFD label, not an omitted filesystem input. Parsed real archives and
  verified map-named members exist (415 HOST/42 ARM selected members). CRT/libc/
  libgcc/support and compiler cc1/collect2/as/ld paths/logs remain associated.
  HOST support discovered from link traces is final-observed, not falsely all
  pre-use-pinned; retained ARM inventory and preprocessed compile inputs have
  their distinct recorded capture boundaries.
- Independently resolved existing loader init/initialize/transfer records to all58
  supplementary path-associated pins and13 transfer spellings. Original2-plugin
  LOADED_TOOL_ELF_FINAL receipt remains intact/incomplete. These are final-observed
  alias paths, **not58 unique binaries or all pre-use support pins**. No ldd, compiler,
  private proc read, runtime trace, rebuild or false pre-use assertion was used.
- Direct actual ELF parsing: HOST ELF64 EXEC/x86-64; ARM ELF32 EXEC/EABI5/soft-float,
  pinned readelf ARMv5TE attributes. No PT_INTERP/dynamic section/NEEDED, no WX LOAD,
  GNU stack RW/nonexecuting, real nonempty main/entry in executable segment and no
  strong undefined symbol. These files really are executables, but were never run.
  Object ELF/main and .su records also inspected; no executable was substituted
  with an inert archive or fake provider.
- PT_LOAD memory sums match original closure: ARM14,468B/HOST833,165B. Actual authored
  .su records are static, largest frame ARM1,496B/HOST1,504B. These exclude page
  rounding/libc startup/live call depth/kernel/VFS/slab/other allocations and are
  **not** total stack/RSS/syscall/native/combined memory-fit evidence. HOST906,640B
  static glibc is not a verifier budget proof. No128KiB/40MiB/CMA/native2MiB/HOST4MiB
  limit is raised; helper is neither a verifier nor finished camera main.
- GPL-2.0-only authored/import notices and retained kernel GPL text/toolchain license
  provenance remain pinned. Separate GCC/binutils/musl/glibc licenses are not a
  global license grant. No binary/toolchain/kernel redistribution/publication.

| Actual local static executable | Bytes | SHA256 |
|---|---:|---|
| ARM |30140| `87a40f291600fee1296d96030726b9db825956e274e0db2a7ee5ea18f6a26ac6` |
| HOST |906640| `d743ad0b8e10cd210aa5729b1b588215a34cb34796bcbcfc9541965e2729b2a3` |

Two reviewer bookkeeping errors are preserved, not candidate failures or policy
rejections: before01 assumed imported query H/inc lived in a candidate-evidence
map rather than their frozen source map; before02 treated ARM's synthetic linker
stubs label as a file. `COLLECTION_ERROR01.md`, `COLLECTION_ERROR02.md` and both
failed STATIC receipts retain hashes/tracebacks/reversible reviewer-only changes.
Successful before03/after03 plus final fresh rehash cover4,014 path-associated reader
input records; aliases are not unique physical files. `FINAL_SEAL.json` freezes
these input/own-output pins including errors. No original source/build was changed.

## Disposition / mandatory missing native boundary

No minimum corrective source revision is required for this exact conditional
query-only main at the reviewed scope. Parent may separately disposition this
source candidate. It remains **not staged/embedded/native-admitted**, with no
monitor88-byte query command, reviewed bootstrap/control handback or native recipe.

Future selection must independently establish exact boot/image, sole capture/UART/
process and FD table/no inherited aliases, no active provider/jobs/unknown DMA,
global idle, real watchdog/recovery association/no competing owner/nowayout changes,
adequate source/stack/backing and bounded volatile output sink, independently
maintained owner/lifetime/capture, fresh unconsumed original-U-Boot catch and manual
OFF fallback. None is implemented/proved by argv/root/release/sysfs/lstat or query
facts. Failstop after uncertain disable can lack a running watchdog; it is not an
external retainer/recovery policy. No current hardware admission follows.

Query77 frozen image has NEVER booted; embedded station76 remains UNPAIRED/
UNSUPPORTED, no load/autoload. Both original radio/reset HOLDs remain untouched.
Held original U-Boot12:12UTC/catch consumed is historical only; no fresh state,
owner/catch/expiry assertion is made. Native staging/query/boot, new image/module,
post-memory/source/provider/external retention/backing/provisioning/main/PLAT-02/
G2/G3/G8 and ordinary functional qualification remain open. **No hardware/program/
test/new recipe/native query/implementation or automatic next queue is selected.**
