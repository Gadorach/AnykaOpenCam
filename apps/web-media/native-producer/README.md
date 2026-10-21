# QC07F native producer handoff — offline BUILD candidate

Tasks **WEB-02 / STREAM-08**, with CAP/ENC dependencies. This isolated helper does
not enable media, change the encoder/kernel/entropy path, or qualify any product
gate. Parent alone owns hardware/UART, image readiness and recovery. HOST tests
use synthetic buffers/syscall replies; the ARM ELF is never executed by this build.

## CURRENT v6 — corrected placement forecast + explicit raw-reuse variant: HOLD

**PAUSED by user request.** Only the active offline v6 finalization was completed;
no automatic continuation, optimization or native launch is authorized. Final
source/ARM (NEVER RUN)/HOST closure is `run34/manifest.json`, with
`REPORT-v6.json` and `parent-review-v5-to-v6.diff` under delegated evidence.
Parent review/rerun and placement/admission remain pending; resume only on a new
user request. Parent handles shared status and any process/device shutdown.

All older v4/v5 sections below are historical, superseded explanations; v5/run28
sources/ELF/report are preserved under `parent-review-v5-snapshot01/`. **Do not
execute v5**: its non-CMA gate omitted unpaid CPU Normal placement and potential
CMA evacuation. Current v6 never adopts that acceptance result or old CLI silently.

```text
native-handoff --parent-protected --sensor-baseline exact139 --peer 192.168.1.15 --port 17891 --memory-reserve normal75-placement-one-shot --producer-storage raw-reuse-k75
```

`separate` is the only other explicit storage value, keeping the separate producer
mapping under the SAME corrected forecast. No automatic mode selection/alias
fallback or lower H/R exists. Parent's H512/R1024 remains ONE finite experiment
only, not peak proof/security ceiling/memory/native acceptance. The mode log now
reports declared CPU payload2,776,576B for raw reuse or3,825,152B for separate,
**not measured peak**, with current authoritative ELF mapping/page costs in report.

Corrected same-gate rule: `N−W >=1024+512+CPU_unpaid_Normal+M_future`, independent
of the global Available/debt gate. N is conservatively bounded by BOTH current
Normal free/free-CMA and meminfo free/CMA samples. Current HIGH already includes
boost; empty Movable contributes0. All still-unpaid CPU capacity may land in
Normal. Before exact healthy full codec backing is paid, default M_future charges
`min(current occupied CMA, unpaid codec range capacity)` because no qualified
placement bound is available for own/other movable occupants. It includes possible
own loans without adding them a second time. No known ISP subtraction follows
from requested bytes, prior addresses, eligibility or config alone. Resident credit
does not imply a CMA loan will survive subsequent coherent allocation/migration.

Forecast runs BEFORE raw/hub/producer prefault or allocation, before any first/
subsequent acquisition with old hub held, and again immediately before encoder
ioctl/DMA allocation in the actual narrow import. Failed predictions perform no
hopeful allocation, churn, reclaim/boost/sysctl manipulation or source retry.
64 bounded scalar observations cover these finite13-AU forecast points; frame,
time, CRC, memory/security and wire ceilings are unchanged and sampler footprint
is charged in current ELF RW pages. Missing/low samples remain benign safe END/
QUERY/ACK when source is known; real uncertainty retains backing/quarantine.

Raw reuse is conditional SOURCE support, not hardware proof. Pinned75 encoder
copies all1,382,400 main input bytes privately BEFORE ASIC work and reset/full
guards, then copies≤1MiB output into the user destination under the encoder mutex.
It never programs user raw/output pointers as DMA addresses or reads raw again
after output copy. Exact audit/caveats are in `RAW-REUSE-CONTRACT-v6.md` under own
evidence. Aggregate raw1,728,000B scalar CRC remains BEFORE encode; raw-valid is
revoked before the ioctl, including partial EFAULT. Postencode raw is only a NAL
loan, never a stale raw view. Copy complete NAL to onehub→source/pre CRC→release
producer LOAN→acquire hub→next full raw capture; oldhub post-lookahead CRC remains
unchanged. No free(raw), double prefault credit, extra AU store, lookahead deadlock,
full-AU TLS copy or kernel/framework/codec/guard/cache/reset/P-ref change.

HOST models exercise exact private input-copy→encoded-overlay order,13/12 path,
1MiB NAL, held-loan NEXT refusal and END/ACK/no-free(raw). Deliberate early/partial
overlap destroys input correlation and is rejected as a wrong-contract/EFAULT
backend; output EFAULT/guard/clock/source uncertainty retains raw/hub/provider/code.
These models cannot attest an arbitrary runtime kernel, and no private input-CRC
query/alias capability/codec receipt is invented. Parent must independently review
the source-supported alias contract and exact actual image association before use.

**HOLD remains concrete.** Older h receipt and newer i restored-WLAN receipt are
separate dated public observations, not authority or future admission. h: N−W4664
is already short for separate CPU3864+H/R1536 even with M0. Raw reuse genuinely
saves one padded1028KiB producer mapping, but default unknown occupied-CMA7168KiB
migration debt still fails either mode on both recorded receipts. Source-known
nonmovable ISP placement may later be reviewed using actual trustworthy evidence,
but v6 never subtracts it from a guess or silently sets M0. ONE next dependency is
parent independent v6 review/rerun plus a fresh existing read-only memory/Normal
zone/paid-allocation receipt and qualified future-migration bound BEFORE prefault,
sensor writes or codec allocation. If a safe bound requires new ABI/hardware page
probe/OOM experiment, keep HOLD. No physical entry, peak/FPS/native/encoded/media/
G3/G8 claim or historical owner/arm reuse is authorized by this candidate.

## Current v4 candidate — outbound TCP, no target RAM file

**Superseded by current v5 below:** v4/run21 and parentV4A remain completed offline
evidence. This source now requires the one-shot reserve policy; old v4 invocation
without the memory flag is rejected before any source operation.

## Current v5 — strictMemoryReserve, ONE finite experiment policy only

```text
native-handoff --parent-protected --sensor-baseline exact139 --peer 192.168.1.15 --port 17891 --memory-reserve normal75-one-shot
```

Parent approved512KiB overhead envelope+1024KiB reserve **only for one finite
qualification**, not a proved peak, product/security ceiling, new memory limit or
native acceptance. No CLI can lower these values or other ceilings. The initial
floor is recomputed from the current own immutable ELF32 LOAD page extents,
two pinned-musl1028KiB allocation capacities and nonshared ELF data/trailer staging
pages. RX file-backed code pages are counted once, raw/nonraw/static sampler/CRC
state is inside RW, and allocator padding is not credited as touched data. Build
independently validates exact sealed stripped-file end/page arithmetic and reports
the upward floor; it cannot fall below v4's13624KiB.

ONE bounded integrated checker reads only fixed public `/proc/meminfo` (4096B)
and `/proc/zoneinfo` (16384B), verifies procfs/read-only/no-follow descriptors,
uses bounded read calls and fresh meminfo→zoneinfo→meminfo. It takes min free/
available and max freeCMA from the two samples; no swap/reclaim/cache credit,
no private maps/proc/environment/kernel dumps and no daemon/IPC/guardian. Samples
must finish within100ms and every gate remains inside2s/10s checked monotonic
budgets. Blocking syscalls cannot be preempted by userspace checks; existing
qualified recovery remains mandatory. Metadata is preallocated inside the newly
charged RW extent/approved envelope, not a large/fullAU allocation.

The **actual parent-sealed layout** is Node0 Normal (spanned/present10240,
managed8286,cma4096,protection(0,0)) plus EMPTY Node0 Movable (all capacities/free0,
unboosted min/low/high/promo32,protection(0,0)). It is NOT singleDMA. Unknown zones/
nodes, populated Movable, missing/duplicate/overflow fields, geometry/protection
or base-watermark changes deny pending a fresh source/layout review. Per pinned
`mmzone.h`+`vmstat.c`, printed high already includes boost. Current Normal high
is used ONCE, capped to managed pages per `page_alloc.c`; empty Movable contributes
zero reserve, not a fictitious32-page floor. Legitimate dynamic boost is accepted
only if all sealed unboosted bases (178/222/266/310) still match; W is recomputed
from the CURRENT sample. No sysctl/watermark/boost manipulation occurs.

Both independent floors are mandatory:
`MemAvailable >= codec_unpaid + CPU_unpaid +512 +1024`, and
`min(MemFree)-max(CmaFree)-W >= unpaid_RX/nonshared_staging +512 +1024`.
The second conservatively retains the entire overhead envelope and unpaid known
nonmovable code/staging; it never substitutes Available−CMA or adds CMA to A.
Actual socket limits/source health remain unchanged. A pressure check cannot
guarantee contiguous CMA or stop an instantaneous burst/OOM.

Initial gate precedes hub allocation and **all sensor/capture operations**.
Only after it passes, owned unused raw/hub buffers are safely prefaulted and
rechecked. The one1MiB producer mapping is allocated/prefaulted before capture
inside the SAME2s producer start, then sampled again before sensor access. Full
interior pages of those three owned buffers receive credit only after volatile
touch success; boundary/allocator padding and code/sampler pages keep debt.
No borrowed/live AU is touched. Producer mapping now persists through CPU-loan
release/reuse until proven normal source stop, so its resident credit cannot
disappear after an unproved free. No second producer/fullAU copy is created.

After first complete healthy native producer return, existing lifetime query must
show exactly8454144B healthy live codec backing before its8256KiB debt is paid;
no new kernel query/receipt is invented. A gate runs there before media output,
and before EVERY subsequent acquisition while old hub backing is acquired. Paid
buffer bounds are evidence of direct prefault/continued ownership, NOT process RSS
or total high-water. Log includes remaining debt, required floors, current W/free,
timestamps and exact denial reason. No native credit follows from malloc success,
small AU size, declared capacity or HOST fixtures.

Missing/stale/low memory samples are **benign no-more-NEXT** refusals: socket
discard/close once and actual normal retire_held END→QUERY→ACK before known CPU
free/hub release when source is safe. Clock/source/reset/receipt/CRC uncertainty
still fences/retains/quarantines the SAME process. No false clean stop or automatic
retry/renewal. The revised producer loan release clears a loan, not its mapping;
normal final free occurs only after source proof. A failed prefault/malloc before
capture cannot be disguised as DMA corruption or a successful capture.

V4/run21/source/REPORT was byte-exact preserved before edits; parentV4A42/41/24/16
remains completed evidence and is not retaken for restart. Parent's h/22:56 hold,
matched75 buildID/config and memory/zone JSON are dated public evidence, not this
agent's authority. No target/host proc reading, hardware/ARM/private inputs/Git
operation occurred here; memory tests use ONLY public synthetic metadata. Parent
must revalidate actual ownership/runtime/recovery, source/ELF/shape/staging and
current floors/reserve before the one reviewed native experiment. No capture,
encoded retirement, memory peak,15fps, protected media or G3/G8 claim is made.

The current executable **does not create any target media file**, accept a camera
connection, publish media, resolve DNS or reconnect. RAM-file v1/v2/v3 sources,
reports and ELFs are preserved separately and remain **HOLD**, not alternate
launch modes. Historical RAM-file descriptions below are superseded by this section.

```text
native-handoff --parent-protected --sensor-baseline exact139 --peer 192.168.1.15 --port 17891
python3 -B apps/web-media/native-producer/receiver.py --bind 192.168.1.15 --port 17891 --peer PARENT_MEASURED_CAMERA_IPV4 --output /PARENT/OWNED/FRESH-PC-CAPTURE
```

Port17891 is the proposed fixed unprivileged qualification port, not a default:
the target requires that explicit port and exact authorized parent IPv4. Parent
must review/select/seal this endpoint before invocation. Receiver peer is explicit
parent-selected camera IPv4 in192.168.1.0/24 (not network/router/parent/broadcast),
never an inherited DHCP/recorder observation. No userinfo/path/command/clock/table
options or other destination is accepted. `--PUBLICDUMMY-localhost` receiver and
HOST-only sender builds admit **only127.0.0.1**; tests never contact the real camera/LAN.

One NONBLOCK/CLOEXEC outbound socket, one connect attempt, fixed2s connect bound,
and **10s total session starting before connect**. Each entire init/fragment has
one2s deadline shared across all≤8192-byte partial sends/EINTR/EAGAIN/poll waits;
no per-span renewal. BUSY returns cause the core to recheck exact CPU lease/clock/
source health before another real send. SO_ERROR and exact connected peer are
checked before any source operation. Source-produced13/output12/whole terminal
boundary, full raw/NAL/source-hub-pre-post CRCs,QP26/GOP30/main720/guards/receipts
and watchdog5/exact139 preparation remain unchanged. No TIMEOUT/CRC/locking/capture
ceilings or kernel/syscall-source changes.

Requested socket buffers: send4096B/receive1024B. Actual sizes are queried/logged;
admission denies send>8192B or receive>4096B, not silently raises them. Socket/skbuff/
page-table/stack/mapping overhead and resident peaks are **not** proven by those
numbers. Removing the2048KiB target file leaves an arithmetic1980.5KiB
raw-payload-only residual against the supplied baseline, not an admission PASS.
Parent still needs actual post-codec+simultaneousCPU/socket pressure and a justified
positive non-CMA reserve. Do not double-count freeCMA or infer contiguity/RSS.

`accepted_TCP_bytes` means bytes accepted into the local TCP stack, including
partial output. `complete_local_boundary_bytes` means the sender completed all
local sends for a whole init/fragment, **not remote ACK/receipt/decode**. On error
the socket closes once; no reconnect, ACK protocol, remote truncation/rollback or
resumed partial fragment. The PC preserves byte-exact `wire.bin`; a SEPARATE fresh
`complete-prefix.mp4` includes only independently validated complete boxes/samples.
The receiver stages at most12 samples under a2MiB wire cap, one connection and45s
active capture/validation deadline; bounded final reporting/cleanup follows.
Original wire/source/acquisition files are never padded/truncated/deleted. No
existing output directory/file is overwritten, listed or served.

The receiver adapts the existing independent feasibility box accounting and exact
pure Python fragment oracle; build checks the copied oracle's AST against canonical
`apps/web-media/feasibility/mux_fixture.py`. It checks init geometry/profile/track/
timescale, exact112B header/sequence/relativeDTS/duration/NAL size/type/flags and
computes full scalar BZIP2 NAL CRCs. Correlation against source console metadata
and real independent decode remain **parent tasks**, not parser or TCP proof.

On a safe sink error, the native stop path parks (does not copy/free) the one
known returned producer buffer, performs actual source END/QUERY/ACK, then frees
that backing and releases the acquired hub lease. If source/clock/CRC/stop is
uncertain, buffers/FDs stay fenced in the same SIGSTOP'd process for fresh parent
qualified recovery. No process-exit-only retirement or invented codec receipt.

All v1/v2/v3 and failed intermediate builds remain preserved. Source v3/run14
snapshot was sealed **after guarded139 tests passed, before TCP edits**. Latest
server restarts do not invalidate completed offline evidence, but invalidate any
assumption of current recorder PID/timer/runtime; parent must re-establish actual
ownership/runtime/protection ACK and exact image/source/ABI/memory readiness before
the one reviewed native operation. No hardware/private footage/credential/device/
UART/Git/install/publication operation was performed by this agent.

## Reproduce offline

```sh
python3 -B apps/web-media/native-producer/build.py runFRESH \
  --toolchain work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Each run refuses an existing directory and writes only a fresh directory below
`evidence/delegated/20261012-native-handoff/`. The assignment's explicit **WRITE
ONLY** rule takes precedence over its suggested `local/build/...` destination.
No dependency installation, SDK build, network, Git action or historical library
reuse occurs. Canonical framework C/header inputs are copied into the fresh
hashed snapshot before compilation;
selected retained `rtspd.c` functions are imported with an original-excerpt hash,
generated-source hash and exact import diff. Only `crc-bzip2.h`'s include spelling
is relocated; its full scalar CRC algorithm is unchanged. No JPEG source/link is
needed because JPEG, legacy capture, parser and network functions are excluded.
Runtime license notices, dependency `.d` files, tool/runtime hashes, link inputs,
commands, timestamps, stack-usage files, map and ELF metadata are retained.

## Public native CLI — parent review and explicit invocation only

```text
native-handoff --parent-protected --ram-dir /PARENT/OWNED/RAM/DIR --name FRESH.mp4 --sensor-baseline exact139
```

There is no default directory, credential, output name, capture count, frame-rate,
profile, ceiling-extension or renewal option. `--parent-protected` is an operator
assertion, **not** proof of protection. The directory must already exist, be root
owned, mode0700 and on tmpfs/ramfs; the final component cannot be a symlink. The
parent must review all path components and the RAM-file budget. One mode0600 file
is opened relative to that directory with `O_EXCL|O_NOFOLLOW`; it never replaces
an existing file. No raw file, binary stdout/UART, publisher, listener, footage
pruning or other filesystem write exists. The explicit exact139 flag is mandatory;
no flag/arbitrary name means no admission or table writes. Parent seals actual
image/source/sensor/baseline policy, never a historical register-state assumption.

Normal success is exactly **13 complete produced AUs /12 known-duration outputs**.
AU13 is released whole as the final boundary; it is not published or partially
exported. No EOF/source error creates a guessed duration. The candidate handles
one complete VCL NAL/sample only, baseline42/e0/1f,1280x720, fixed hardwareQP26,
GOP30, CAVLC, no B frames, no clipping, software encoding or resolution fallback.

## Ownership and failure policy

1. Strict ISP BIND→ARM→QUERY(ARMED)→BEGIN→QUERY(ACTIVE), then sensor-on.
2. Full1,728,000-byte scalar userspace raw CRC and exact bytes/pointer/sequence/
   status/guards; QUERY of the exact source/file/generation key remains mandatory.
3. Kernel75 lifetime query must be supported and healthy **before capture**;
   hardware STREAM ioctl must succeed, full NAL bounds/type and scalar CRC pass,
   then lifetime/source health are rechecked. No partially copied EFAULT reply is
   inspected. A successful encoded syscall is interpreted only against the sealed
   kernel75 synchronous reset/guard/copy source contract, not kernel73 behavior.
4. Publish/copy producer_n into the existing1MiB hub; compute the full scalar
   BZIP2 CRC of that copied NAL and compare it to the exact CRC already computed
   by the producer (retained as scalar userspace evidence, no new kernel query).
   On mismatch, quarantine **before** releasing producer storage. Then release producer CPU output;
   acquire hub_n. Produce n+1 in the now-available1MiB producer buffer while hub_n
   stays acquired. Recompute the full old-hub NAL CRC after the actual next()
   return and compare it to both the before/source CRCs **before any fragment
   write**. On corruption, preserve the hub backing and returned future producer
   CPU lease, skip source retirement, fence admission and require parent recovery.
   Failed next() outputs are never inspected; only the known old hub can be
   checksummed on an error return. Actual source timestamps form duration_n. Each ≤8192-byte real
   write continuation checks clock/cancel/source health and exact acquired CPU
   lease. All old leases are released **before** publishing n+1. No second hub,
   lookahead producer hold, acquired-slot overwrite or per-peer/full-AU mux copy.
5. Retire all CPU leases, sensor-off→ISP END→exact QUERY(RETIRED,END,stop_result0,
   stopped_ns nonzero)→ACK, then close only proven source/reference FDs. Expiry is
   failure in this finite candidate, never a renewal/resume permission.

Clock failure/regression, namespace/generation/sequence mismatch, allocation,
corruption, EFAULT, source loss, cancellation, bound or output failure stops
admission. Source errors quarantine instead of retrying END/reset/ACK. Uncertain
CPU backing/FDs are retained; `NH_QUARANTINE` then SIGSTOPs the **same process**
until the parent's freshly qualified bounded recovery resolves it. This is not
normal exit or retirement. No automatic restart/resume/retry occurs. On a safe
sink-only failure, only the fresh owned file is truncated to its last complete
prefix, all proven CPU leases are released, and normal source retirement is attempted
once. Prior complete outputs remain; no partial sample is accepted.

## Bounds, addresses and memory

- Hard file cap2,097,152B enforced before each complete fragment and by
  `RLIMIT_FSIZE`; init≤2048B, fragment header112B, borrowed spans≤8192B.
- Producer/whole-output bounds2s, helper session10s, kernel session10s/128 frames
  unchanged. Blocking kernel/RAM writes are not preempted by a userspace check;
  parent must supply the existing qualified outer recovery, ≤45s proposal and
  actual protection ACK/binding. No historical timer/PID is inherited.
- CPU payload budget: raw1,728,000 + producer1,048,576 + hub1,048,576 =3,825,152B;
  plus≤2,097,152B RAM file. Stack/libc/mapping/page/kernel overhead is **not zero**
  and must be measured by parent. The reported budget is arithmetic, not a peak.
- Same40MiB system RAM `[0x81800000,0x84000000)` and CMA16MiB
  `[0x82800000,0x83800000)`. ISP6,307,840B +codec8,454,144B remain requested DMA
  lower bounds; guarded1MiB output still needs its existing2MiB DMA allocation.
  No four-slot capture/import, module unload, DMA free or pool-size change.
- Physical driver source ranges remain ISP`0x20000000` and codec
  `[0x20020000,0x20020500)`; this helper never maps MMIO or accepts physical
  addresses. Only fixed32-bit CPU destination pointers cross existing ioctls.
- Root RAM diagnostic is **outside** the locked web/credential processes. It
  cannot be moved wholesale into either existing2MiB security budget. No auth,
  entropy, password600k, heap/single-allocation or MEMLOCK policy changes.

Scalar logs correlate raw/NAL CRCs, source keys/sequences, actual timestamps,
capture/encode intervals, next() while old hub is held, output edges/lengths,
codec requested-live bytes and END/QUERY/ACK outcome. CPU time, CMA/free memory,
actual heap high-water, kernel phase aggregates, post-idle/IRQ/UART/watchdog and
independent decode/correlation remain parent measurements. `hub_copy_ns` includes
publish/release/acquire, not isolated memcpy; raw/NAL CRC edges are nested inside
source work and must not be summed twice. TLS is not measured.

## Review limitations and one next native operation

The source supports ISP receipts and kernel75 codec lifetime/quarantine queries;
it does **not** supply a codec-specific per-FD END/QUERY/ACK receipt API. Never
invent one or call a clean lifetime query proof of untested encoded retirement.
Exact booted image/provider/config/source association and encoded normal
retirement are still native entry gates. ELF/static link and HOST mock receipts
do not pass them. ELF architecture attributes and linked runtime closure also
require parent review; no instruction/execution compatibility is inferred from
one compiler flag or kernel release string.

**One next parent operation**, only after independent offline review, exact image/
source/header/ELF readiness, fresh sole UART/runtime/recovery ACK and RAM budget:
invoke the sealed ELF once with the public CLI in one fresh owned RAM directory,
under the existing qualified ≤45s outer boundary. Expect13/12, intact final boundary,
source END/QUERY/ACK and clean actual post-idle, then independently verify complete
fMP4 source/CRC/timestamp/decode correlation. Preserve failure, quarantine and prior
complete file; no retry. If source association/encoded receipt semantics/protection
are unresolved, **HOLD** this invocation and report the exact dependency.

Native handoff/encoded retirement/media/FPS/G2/G3/G8 remain **OPEN**. Current media501
and RTSPNOAUTH are not changed by this isolated BUILD candidate.

## Parent-review correction v2 — preservation, CRC and generated-file accounting

V1 run09, its manifest/ELFs/logs/input snapshots and all source files were copied
byte-exact **before edits** into
`evidence/delegated/20261012-native-handoff/parent-review-v1-snapshot01/`.
Its original REPORT and all prior runs remain unchanged. A fresh source review
diff and v2 report/run provide separate evidence; v1 is not retrospectively fixed.

Production now performs two **additional full scalar NAL reads** per old sample:
copied hub versus existing producer CRC, then old hub after real lookahead versus
both earlier values. No full-NAL copy/new large allocation, guard/cache/oracle
weakening or ceiling/timer change. The extra fixed scalar table is1024B plus a
small initialization flag; bounded CRC/time records are stack metadata. Native
high-water/headroom and CRC wall/CPU costs remain unqualified. `NH_HUB_CRC` logs
only sequence/CRC/timing/checked state; no payload bytes. `integrity_ns` sums the
checked CRC wall edges, including intervening source-health/clock overhead, not
isolated CPU time. `hub_copy_ns` excludes those additional CRC edges; next() timing
also excludes the post-lookahead CRC. No15fps claim follows from these fixtures.

The corruption path fences admission and retains known old hub/future CPU leases
without release/normal END/ACK; root main retains the buffers and stops the same
process for fresh parent-qualified recovery. A nonzero next() return never grants
a usable packet: even bogus pointer/size/key outputs are ignored, no release is
attempted for an uncertain unreturned CPU lease, and native checked idle denies
unknown backing. HOST tests deliberately mutate the last old-NAL byte during
actual candidate next() calls, rather than relying on an assertion inside the mock.
Normal finite retirement now refuses an already-expired/stale receipt rather than
ACKing it as though the helper had performed normal END.

**Rollback is a transaction on the one fresh generated fMP4 file only**, not an
acquisition repair: its fd was created with O_EXCL by this invocation. Failure
truncates only that fd to the last complete init/fragment prefix; existing files,
source/acquisition reads and original evidence are never touched. `committed`
counts the complete generated-file prefix, including init. `accepted` counts all
bytes acknowledged by successful/partial write replies cumulatively, including a
tail subsequently discarded; rollback does not reduce it. Failed rollback leaves
the failed generated file and quarantines rather than claiming a complete prefix.

Typed packet bounds are checked before NAL reads/copy; overflowing pointer ranges
and producer/hub aliasing deny. Bytes exceeding1MiB deny before dereference. RTP
conversion divides nanoseconds by1,000,000 **before** multiplying by90: even the
largest permitted uint64 timestamp has a safe intermediate. Only the final32-bit
narrowing wraps intentionally, with relative duration/wrap checks in the timeline.
Near-maximum timestamp, UINT64_MAX error sentinel, clock regression/failure,
namespace/generation/sequence and early failed-next uncertain-storage tests are
HOST proofs only. Pointer numeric checks are not arbitrary address validation;
production only accepts its sealed native adapter's owned malloc-backed packet.

The parent recorder PID/recovery arm was invalidated by server restart. No current
owner/runtime/recovery is inferred or inspected here. Native entry requires fresh
sole ownership/runtime/recovery ACK plus exact75 image/provider/config/ELF/receipt
association, independent v2 review and RAM/headroom review. No native operation is
authorized to this agent; no codec-specific receipt or native/product pass is invented.

## V3 guarded exact139 baseline (RAM-file mode remains HOLD)

V2/run11/REPORT-v2 and all sources were preserved byte-exact before these edits
under `parent-review-v2-snapshot01/`. Parent's completed independentV2A result is
not invalidated or retaken because of recorder restart.

The maintained table `bringup/qc07f/initramfs/sc1346-init.h` is pinned to SHA256
`98da5eb9e522975d834d99e8c0cdfbefe75ed87dbbcdf84fd9f0fb150f9efbe9` and exactly139
ordered pairs/137 unique registers. Typed fixed-address0x30, STOP-separated reads
and one-message writes adapt the maintained monitor helper/validation at
`bringup/qc07f/initramfs/init.c` (pinned original excerpt/adaptation diff).

Sequence: existingwatchdog5 opened/set before sensor open → siliconDA4D and current
stream-off reads → source BIND/ARM/exactQUERY(ARMED) → watchdog GETTIMEOUT confirms5
→ revalidateDA4D/stream0 → exact139 ordered writes,0103 reset followed by one10ms
nanosleep → all139 scalar readbacks → re-QUERY exactARMED → existing BEGIN/queryACTIVE
→ existing owned sensor-on/capture/encode/CRC/13-12/handoff/END/QUERY/ACK path.
Preparation is inside the **same2s producer start**, not a restarted budget, and
inside the10s helper/session/outer protection. Clock/cancel/deadline checks surround
every table I2C syscall. No retry, watchdog feed/renewal, arbitrary register/clock/
GPIO option, new guardian or kernel/syscall-source change. Blocking syscalls are
not preempted by a userspace clock check; the existing qualified5s watchdog and
fresh bounded outer protection remain required.

Readback compares final values for repeated36e9/37f9, not their transient80 writes.
0103 is a reset command: strict candidate policy requires reset-complete read0
after the10ms wait, rather than comparing command1. The monitor only supplies the
reset/wait sequence; that stricter completion/readback policy is **not native
proof** and needs parent review. Any mismatch is retained without masking/relaxing.
All139 reads complete on scalar value mismatches if the bound allows; I/O/time/
clock/cancel failure stops immediately. Final table requires1280x720,HTS1800,
VTS750 andstream0.139 completed writes do not prove ready until all readbacks pass;
neither table readiness nor HOST fixtures prove capture/FPS/15fps.

On error, no BEGIN/sensor-on/NEXT occurs. The activewatchdog FD, source reservation,
sensor FD and uncertain backing are retained/fenced for parent-qualified recovery;
no automatic table rollback, reinit or source renewal. Bounded scalar records of
every pair's program/expected/actual/valid/discrepancy and timing are logged after
nh_run (outside producer timing). `written`/`read_count` count acknowledged and
deadline-verified operations; a timed-out syscall may have changed sensor state,
so failed_index/read-valid flags and quarantine remain authoritative.

RAM-file mode is **HOLD** per the independently reviewed budget:13,972KiB
MemAvailable already includesCmaFree9216KiB; codec8256 +CPU3735.5 +maxfile2048
=14039.5KiB exceeds it by67.5KiB before overhead/reserve. Never add freeCMA again
or infer a FPS/memory cause from the separate measured HTS1650 mismatch. No target
operation is permitted by this source build. A separately sealed off-board sink
candidate will remove that target-file copy, not claim that all remaining memory
or native readiness is already qualified.
