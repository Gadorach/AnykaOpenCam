# ENC-02 / PLAT-01 — real OPEN_QUERY client, compile-only candidate

2026-10-22. **Delivered one bounded SOURCE/compile/static-link candidate for
independent review. No native admission, execution, upload, boot or next queue.**

Owned only new `apps/qualification/encoder-query77-client/` and this documentation
directory. Parent alone owns hardware. The disjoint active composition lane was
not read/changed. Git/index, central plans/AGENTS, existing source/helpers,
kernel/config/initramfs/module/images/recipes and both HOLDs were not changed.

## Actual source and contract

- Explicit real main: `apps/qualification/encoder-query77-client/encoder-query77-qualification.c`.
  Not an app main, fake provider, monitor substitute or readiness service.
- Before-code [SOURCE_CONTRACT.md](SOURCE_CONTRACT.md) records actual query77
  open/ioctl/release/init/misc name, CPU allocation versus DMA effects, real TOP
  watchdog/core and historical monitor smoke semantics, uncertainty and required
  future parent bootstrap. Read it before reviewing the client.
- Five-file source aggregate, sorted workspace-relative UTF-8 path + NUL + raw
  SHA256 digest bytes:
  **`101afbb482c4562d02e58ab7c2aafed0d3cbb76a7d7b4cc36cf08e667fc80600`**.
  Individual source pins are in [OFFLINE_CLOSURE01.json](OFFLINE_CLOSURE01.json).
- Real main source SHA256:
  `d132a2eeaae170586aa437a2e7a3102027d9ff656077c5102f57dd79d95268b7`.
- UAPI is an exact2455-byte import of frozen
  `apps/qualification/kernel76-encoder-identity/encoder-open-query.h`, SHA256
  `6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37`;
  matches real query77 kernel include. Original46-line UAPI and45-line handler
  were inspected; no implementation copied/replaced. Frozen five-file query
  source aggregate `ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`
  was revalidated, and actual kernel provider/header match frozen candidate pins.
- Authored source/build/metadata collector GPL-2.0-only; imported header retains
  GPL-2.0-only notice. Exact retained GPL-2.0 text is pinned as an input at
  query77 source `LICENSES/preferred/GPL-2.0`; no new global license grant.
  Toolchain README identifies GCC14.3.0/binutils2.43.1/musl1.2.5 and its separate
  source/license provenance. Host glibc/GCC and ARM musl/libgcc remain their own
  licenses; no compiler/library/image/binary publication or redistribution here.

### One future process, no media work

Default/refused argc/argv exits2 before authored I/O. The sole explicit one-trial
option does NOT authorize execution or prove admission. Require root/release and
fixed sysfs nowayout0/inactive/identity/current timeout1..60; stat exact char nodes
root0600, then fstat-match opened descriptions. Metadata is not authority, armed
recovery, exact image identity, global idle or no-alias proof.

After all future fork/exec, the final one-shot process opens exactly one
`/dev/watchdog0`, validates actual support/identity, configures nominal5s once and
reads back5 before the only `/dev/qc07f-encoder` open. Open itself already starts
the watchdog at the admitted prior timeout; early failure can leave unknown
effects without a returned FD. No fallback devices or competing watchdog open.
Catchable signals are blocked before effects; SIGKILL/kernel death remain outside
this retention design.

Immutable1.5s whole effect-phase monotonic budget begins before watchdog open.
Every returned effect is checked against that original budget and monotonic
nonregression; no feed, renewal, new guardian or service of failed work. Two exact
88-byte OPEN_QUERY calls require syscall0/version1/bytes88/reserved0, nonzero
same per-open session, NONE/owner0/frame0, faults/flags/ledger0, virgin global
history/generation/dispatch0 and whole-snapshot equality. This intentionally
rejects history-bearing boots instead of guessing semantics. **ENOTTY fails**,
including old76; no global sequence identity fallback. Optional56-byte global
query is omitted, not substituted for identity. No other encoder ioctl exists in
the client. Query does not prove prior DMA retirement/never-used hardware/global
idle/exclusive custody/authorization/retirement from NONE or a clean snapshot.

Only that normal no-work sequence permits encoder slot clear-before-one-close,
one logical `WDIOS_DISABLECARD` on the owned watchdog description, then watchdog
slot clear-before-one-close within the original budget. Real core/driver order
was read: disable invokes TOP stop and clears active; inactive release does not
feed. No magic write (which would feed). A close error NEVER retries or claims
that FD number still owns the description. Logical disable/close is not
electrical-stop proof.

Any uncertain ioctl/open/fstat/schema/reference/history/clock/deadline/close/
disable result after first watchdog-open attempt enters a nonreturning pause
loop, retaining all still-owned descriptions/process with no retry, disable,
close, repair, REFCLEAR, feed or output write. A close-error slot is already
cleared and explicitly not claimed retained. A failed/late disable may ALREADY
have stopped hardware; a late watchdog close may already have released it. Thus
failstop is not external retention, guaranteed reset or armed-recovery proof.
No failure diagnostic can block before failstop; absence of normal numeric output
must be treated as incomplete/failure by a future reviewed parent observer.

Two numeric lines in struct declaration order are written only after normal
device retirement. No raw payload/addresses/UUID/configuration/digests/device
identifiers. Parent must supply bounded volatile output sink; post-retirement
output failure returns6 without further device effects. No persistent file write,
malloc in authored code, thread, fork, dup, SCM/shared device FD, provider token,
capture/ENC job/reset/control/data/media/network/peripheral/module operation.

## Actual offline build — PASS, no program run

Only this offline build command was selected (NOT a native-use recipe):

```text
python3 -B apps/qualification/encoder-query77-client/compile_only.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Fresh output `apps/qualification/encoder-query77-client/build01/` is ignored and
retained locally. Build started16:10:15UTC, tools16:10:17–19UTC, final unchanged
input check PASS16:10:20UTC. **36 tool commands exit0**: two compiler/version,
two readelf/version,24 compiler-support location queries, two dependency-only
preprocesses, two real C compiles, two static executable links and two ELF
inspections. No warning suppression/guard relaxation, failed build, test suite,
generator payload or HOST/ARM executable/default/help invocation occurred.

Common flags: `-std=c11 -Wall -Wextra -Werror -Wpedantic -fno-common
-fno-strict-aliasing -ffunction-sections -fdata-sections -fstack-protector-strong
-fstack-usage -Wframe-larger-than=8192 -Wstack-usage=8192`, exact local include and
workspace file-prefix map. HOST GCC16.2.1 uses `-O2`; retained ARM GCC14.3.0 uses
`-Os -march=armv5te -marm -mfloat-abi=soft`. Both links use `-static -no-pie`,
section GC, real link map and input trace. Full argv/environment/log/UTC are in
`build01/REPORT.json`; no old guarded builder or imported build module executed.

| Actual static ELF under `build01/` | Bytes | SHA256 |
|---|---:|---|
| `arm/encoder-query77-qualification` |30,140|`87a40f291600fee1296d96030726b9db825956e274e0db2a7ee5ea18f6a26ac6`|
| `host/encoder-query77-qualification` |906,640|`d743ad0b8e10cd210aa5729b1b588215a34cb34796bcbcfc9541965e2729b2a3`|

ARM ELF32 little-endian EXEC/EABI5/soft-float, ARMv5TE attributes; HOST ELF64 EXEC.
No PT_INTERP/NEEDED/dynamic section, no writable executable LOAD; stack segments
RW, not executable. Actual PT_LOAD memory sums ARM14,468B/HOST833,165B, excluding
page rounding, live stacks/libc startup/kernel/VFS/slab/other allocations. Largest
authored compiler stack frames ARM1,496B/HOST1,504B; fixed query structs88B each,
output384B, local numeric scratch20B. No VLA/recursion/payload or authored heap.
These are static facts, NOT process RSS/heap/stack maxima, runtime allocation or
combined native memory fit. Static HOST glibc may include unused general libc
machinery; no claim that libc startup never allocates or executes syscalls.
128KiB allocation cap,40MiB/CMA, native2MiB child/HOST4MiB limits remain unchanged;
helper is not a verifier and no limit was raised to build it.

### Evidence and method limitations

- `PRE_INPUTS.json` pins full retained toolchain map before use, including headers,
  wrapper/real compiler, linker/assembler/readelf, crt, libc/libgcc archives and
  symlink identities; four retained expected ARM toolchain pins matched.
- `INPUTS.json` contains **3,710 path-associated inputs**, all checked unchanged.
  Exact HOST119/ARM37 consumed source/header paths are preprocessed/pinned/copied
  before C compile; actual `-MD` dependencies match preprocessed sets. Archive/
  crt/linker inputs and link maps retain actual static-link closure. Host support
  discovered from link trace is final-observed, not falsely all pre-use-pinned.
- `OUTPUT_SEAL.json` pins every original build output/log/snapshot. The separate
  read-only metadata collector `seal_offline.py` verified that seal and all3,710
  inputs without invoking compiler/client/old helper/subprocess.
- Initial builder's loader parser records only2 absolute-path plugin entries,
  not complete shared compiler dependencies. That receipt remains intact;
  [OFFLINE_CLOSURE01.json](OFFLINE_CLOSURE01.json) supplements it from existing
  explicit loader init/transfer records, giving **58 path-associated compiler/
  tool/shared-ELF pins** and13 transfer-name spellings, zero unresolved. Aliases
  are not unique binaries. No rebuild, ldd, private `/proc` or runtime/client
  tracing was used. These loader pins are final-observed rather than all pre-use.
- Static metadata collector directly parsed already-built ELF program headers and
  compiler `.su` records, no ELF execution. This is authored provenance/integrity
  evidence, not independent reproduction, syscall-level trace or source acceptance.

## Remaining blocking native boundary — unchanged

Query77 release `6.18.55-qc07f-encoder-query77` / exact image
`f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e`
is an offline accepted baseline, NEVER booted. Actual monitor lacks the88-byte
query command; this ELF is not staged/embedded. Embedded station76 remains
unpaired/UNSUPPORTED; no load/autoload. No PLAT-02/full app/provider/watchdog/
native memory/G2/G3/G8 acceptance follows.

Last recorded target observation was original held U-Boot12:12UTC, not current
state. No fresh target/recorder/expiry/owner state was queried, no current runtime
or recovery inherited, no catch rearmed, no HOLD changed. Future use requires a
separately selected reviewed parent recipe proving exact boot/image, sole UART/
process and no inherited/competing device owners, no active providers/jobs/global
idle/prior uncertain DMA, real watchdog/recovery association, independently
maintained capture/lifetime and fresh unconsumed original-U-Boot catch/manual OFF
fallback. The client cannot establish or replace these preconditions, bootstrap
or control handback; no arbitrary ready flag is offered instead.

**One next eligible action:** independent ordinary SOURCE/API/compile-custody
review of this exact candidate. No native recipe, program/test execution,
upload/install/boot/query, automatic retry or next hardware operation is queued.
