# Fixed QC07F stock GET helper

Four query modes only. No arbitrary path/request/value, module loading, GPIO/MMIO,
motor SET/MOVE/RESET/explicit STOP, FPS SET, audio, video capture or application
startup. This is a userland observation payload, not a watchdog/recovery owner.

## Delivered artifact

`evidence/delegated/20261013-stock-reference/query-helper/build01/stock-get-armv5`

- SHA256 `8b7896422c36ad773b54080a37270039149666927aa548ae46d4cdbe91ee5713`
- 34,828-byte static ELF32 little-endian ARM EABI5 soft-float executable.
- No ELF interpreter/dynamic segment. Native binary was **never executed** here.
- 45 explicit host-MOCK cases passed; logs/report/SHA256SUMS are alongside it.

Source and syscall ABI binding: [ABI.json](ABI.json). The preceding static module
review remains at `docs/qualification-plans/stock-reference-20261013/`.

## Exact CLI and fixed device selection

Run as a child of the rescue shell, never replace PID1 with this program. After
parent stages the exact hash at an admitted absolute RAM executable path, use
that path as argv0 and exactly one invocation:

```text
<absolute-staged-payload> --native-stock-get motor0-query
<absolute-staged-payload> --native-stock-get motor1-query
<absolute-staged-payload> --native-stock-get sensor-fps
<absolute-staged-payload> --native-stock-get isp-fps
```

`<absolute-staged-payload>` is deliberately not an invented existing target path.
Parent chooses/proves the RAM staging location. No helper option can change a
device path, request number, packet size or value.

| Mode | Fixed path | Request | Actual output ABI |
|---|---|---:|---:|
| motor0-query | `/dev/motor0` | `0x40046d43` | 36 bytes |
| motor1-query | `/dev/motor1` | `0x40046d43` | 36 bytes |
| sensor-fps | `/dev/isp_char` | `0x8004497b` | 4 bytes |
| isp-fps | `/dev/isp_char` | `0x8004494f` | 24 bytes |

The motor request is semantically GET despite its misleading encoded write/size4
bits. The actual packets, not `_IOC_SIZE`, control buffer allocation. A single
zero-initialized36-byte buffer accommodates every request, with boundary canaries
and zero unused-tail checks. No setter is appended.

Native entry requires real/effective UID0, Linux release exactly`3.4.35`, an
absolute argv0, exact selector and exact argc/mode. The fixed node must already
exist as a non-symlink character device. `O_RDONLY|O_CLOEXEC|O_NOFOLLOW` is used;
post-open fstat identity must equal pre-open lstat identity. **These checks do not
authenticate the loaded driver or make its open callback harmless.** Parent must
prove the node/provider and exact module hashes before invocation. Missing node,
unknown provider or absent modules is a legitimate HOLD; no fallback probing.

## Hard native eligibility

Parent's reported baseline is stock3.4.35 root rescue, read-only root/usr, no stock
app/JFFS2/card mount, default-stock watchdog currently60/nowayout0/nodeamon0 with
kernel idle feeding. Those are parent-reported admission inputs, not observations
made by this worker or verified merely by uname. This helper never changes that
watchdog and does not substitute the replacement5-second campaign budget.

Before each native invocation, parent must establish:

- Exact stock image/kernel/module hash/ABI and fixed-node provider; protected
  current owner, external bounded stop/recovery and user power recovery.
- Stock application absent, no uncontrolled startup/config/card/recording effects.
- For either motor: already-admitted module probe effects; **both axes idle, no
  queued axis, sole first-open ownership**, and no concurrent actor. Motor open
  changes open bookkeeping. **Close/process exit invokes driver STOP**, which
  can decelerate, wait or start a queued other axis. GET naming does not remove
  that shared effect. The helper rejects a returned nonidle field, but this check
  occurs after query/close and cannot retroactively prevent release effects.
- For ISP: `/dev/isp_char` and matching selected sensor/ISP state already admitted,
  or parent explicitly admits the first-open initialization. First open maps ISP
  and calls `isp2_module_init`; last close finalizes it. This is **not a pure
  passive open**. A new fd is not evidence of an already-configured sensor.
- Failed info-module dependencies do not justify loading all modules. None of
  these modes requires `ak_info_dump` or launches the stock app.

No video QUERYCAP/G_FMT mode is delivered: the query leaf does not establish safe
stock video open/attachment. No audio mode is delivered: a safe read-only open
and exact status ABI were not established. No guessed utility or ioctl is used.

## Output, errors and bounded execution

One JSON line only after successful query, close, packet checks and monotonic
deadline check. It includes path/mode/request, expected ABI byte count, exact raw
hex, explicit little-endian words, elapsed time and `physical_fps_measured:false`.
The raw packet has no private configuration or bulk kernel-memory content.

- Motor words: busy flag; software current steps; timer-frequency parameter;
  derived frequency*360/steps-per-cycle; steps-per-cycle; software travel bound;
  limit margin; index; phase count. Signed interpretation/physical orientation
  is not invented; hexadecimal words preserve the native representation.
- Sensor FPS is **cached driver `g_fps`**, including legitimate zero/unconfigured
  values. It does not prove configured or delivered factory FPS.
- ISP frame-rate block stays **uninterpreted**. No four-byte public SDK example
  layout is substituted for the recovered24-byte packet.
- `bytes` means expected ABI copy extent, not a byte count returned by ioctl.
  The reviewed kernel returns0 on complete copy and−EFAULT on short copy. A broken
  provider silently returning0 after a partial copy is not distinguishable from
  legitimate zeros; zero initialization is not proof of completeness. This is
  why exact provider binding is required.

All nonzero ioctl returns, including unexpected positive returns, fail. ENODEV
and EFAULT receive explicit error-stage labels. Close is attempted once and never
retried; a close failure produces no success JSON. There is no retry loop or
second ioctl. A stdout failure can leave a partial line; parent accepts only
complete JSON **and exit0**, not an arbitrary prefix or file existence.

SIGALRM10 seconds, SIGINT and SIGTERM use a non-restarting handler that calls
`_exit` (124/130/143). Relevant signals are explicitly unblocked. Clock failure,
backward time or elapsed>=10s fails. An alarm **cannot preempt uninterruptible
kernel execution or release/STOP during exit**. Parent's established stock60s
recovery/owner and physical recovery remain necessary; this creates no guardian.

## Resource / runtime closure

No application heap allocation, fork, shell, dynamic-library hook or environment
selection in the native code. Fixed output buffer1536 bytes and packet44 bytes.
Native compiler `.su` reports main2640 bytes; sum of all application stack frames
is2760 bytes. ELF writable LOAD memory is524 bytes (.bss340, .data20 plus linkage
sections). Both bounds are below4KiB individually; combined app-frame sum plus
ELF writable memory is3284 bytes. This is **not** a claim that the whole process,
musl startup, signal frame, libc stack or kernel allocations fit in4KiB. The static
executable itself is34,828 bytes. ISP open can allocate kernel resources outside
these application bounds and needs parent admission.

Compiler: pinned Bootlin armv5-eabi musl2025.08-1, GCC14.3.0, binutils2.43.1,
musl1.2.5, sysroot headers5.4.296. Headers are newer than target3.4.35; ioctl values
and packet sizes are pinned to retail REA, not guessed from those headers.
Build report retains exact compiler/subtool, used-header, CRT/archive and source
hashes, command lines, environment, linker map and timestamps. Archive provenance
SHA is`8cdb4ad70c6b5a66427fa3315fe3ddde1c19c90232674dec59045e04d2a36cf1`.

Static runtime inspection is under `query-helper/runtime-review01/`:

- Musl clock wrapper attempts time64 and has ENOSYS fallback to old ARM
  clock_gettime syscall263; fail-closed if unavailable.
- Musl stat wrapper has statx ENOSYS fallback, including fstat64/lstat64.
- Signal trampolines use ARM syscall119/173. This is static evidence, not native
  signal/clock acceptance on this boot.
- Aggregate ELF attributes say v5TEJ and Thumb-2. Linked musl includes runtime
  selected ARMv6/v7 atomics as well as old-kuser paths; do not infer that every
  linked instruction is ARMv5 from the application compiler flag. No runtime or
  kernel helper is patched here; parent native acceptance remains separate.

## Host verification / reproduction

```text
PYTHONDONTWRITEBYTECODE=1 python3 apps/qualification/stock-reference-20261013/build_and_mock.py buildNN
```

`buildNN` must be fresh; existing outputs are refused. Writes go only under
`evidence/delegated/20261013-stock-reference/query-helper/`. Compiler TMPDIR is
inside that output. The script builds native but **executes only** separately
compiled `-DSTOCK_QUERY_MOCK` host code. Native compilation rejects non-ARMv5.
The mock accepts only`--mock-stock-get`, rejects native selector, and mocks every
UID/kernel/device/time/signal operation; it never forwards to open/ioctl/close.

45 cases cover all four byte extents, endian interpretation, zero initialization,
short-copy EFAULT, overrun/tail guards, ENODEV/ENOTTY/positive results, identity
change, missing/regular/symlink nodes, root/kernel/selector/argc/absolute-entry
refusal, close failure, clock failure/backward/deadline, nonidle motor, signal exits
and legitimate zero sensor/ISP state. Every case checks fixed path/request,
open/ioctl/close counts and absence of success output on failure. Host mocks do
not establish native sensor state, physical FPS or open/release effects.

## License scope

New source is local qualification code; no vendor implementation or license notice
was copied/replaced. No workspace-wide license or redistribution grant is added.
Toolchain `summary.csv` identifies musl1.2.5 as MIT and GCC runtime as
GPL-3.0-with-GCC-exception; Linux headers as GPL-2.0. Exact original summary and
toolchain README are preserved byte-for-byte in `runtime-review01/`. The README
identifies upstream complete source/license locations. Complete license texts
were not bundled in this toolchain directory or fetched during this task. This
local payload handoff is not a publication or redistribution authorization.
