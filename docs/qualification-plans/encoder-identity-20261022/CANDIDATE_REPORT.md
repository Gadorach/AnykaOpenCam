# Encoder per-open identity query — source delivered; driver compile BLOCKED

2026-10-22; RP-07/09 source dependency only. No provider/native/memory adoption.

## Outcome and footprint

Implemented a bounded observational addition in a **fresh exact source copy**:
`runs/build01/candidate/`. Original bringup/mainline/full76 source, build metadata,
objects/images and old RTSP/provider work remain untouched. No Git/index/central
plan/P1/boot/recorder/safeguard/provider/model change, target/network/private IO or
runtime/test campaign occurred.

New app-side files at `apps/qualification/kernel76-encoder-identity/`:

- `encoder-open-query.h`:46 lines, versioned output-only88-byte UAPI.
- `encoder-open-query.inc`:45 lines, actual locked provider query implementation.
- `abi_compile.c`:25 lines, compile-only ABI declarations, no main or fixture.
- `build.py`:217 lines, fresh overlay/compile-only recipe, no make/image/module.
- `README.md`:47 lines. Total380 lines/20608 bytes.

**HOST/ARM ABI declarations compiled PASS; actual query body/driver compile is
NOT established.** The build stopped at an exact compiler-version prerequisite:
current system Clang23.1.3 differs from retained final76 Clang23.1.1. No silent
compiler/config substitution or guard removal was performed. This is a technical
toolchain blocker, not a provider-policy rejection. No code was run on a target.

## Exact existing source and applied overlay

Retained source:
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/arch/arm/mach-ak3918ev200/`.

| Original | SHA256 |
|---|---|
| `qc07f_hwenc.c` | `e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f` |
| `qc07f_hwenc.h` | `6723c30c76a3fbd1feaf1ed9d1676a2b9b6289b352ffe7682372bf8e95123731` |

These match the retained native75 input used by v6; they are the actual built-in
encoder carried into the frozen76 source, not a module/mock substituted for it.
Exact original board/telemetry/UAPI headers, final76 Kbuild command/config/symvers,
generated/compiler metadata and kernel license are copied under the run. Header
and provider behavior were read before selecting the addition.

Applied overlay changes only:

1. Include the new UAPI header at the end of existing `qc07f_hwenc.h`.
2. Include the new static query body before existing `encoder_ioctl`.
3. Add exact new-command dispatch before the unchanged lifetime-query branch.

All pre-existing commands, structures, handlers, open/session allocation,
exhaustion check, stream reference updates, final release, reset/quarantine,
memory bounds and init sequence are unchanged. Original open/release suffix was
compared byte-for-byte after overlay generation. No broad tracing or new counter.

Exact applied patch: `runs/build01/applied-source.patch`,5646 bytes,
SHA256 `9d096489e3ba8cc44556b81feb8192b6c5f08e698c18b2718cdfedbab5aea232`.
Candidate provider SHA256:
`9bf16b1175befb091d6c61c3ac573121378e1d8c403eed21806a3e8fc80fc946`.
Candidate original-header-plus-include SHA256:
`6da117b504c15c05dd2b0a332f22f821e085f2e1549e4163ded07ede752829bd`.

## Query contract (not an authorization Boolean)

New encoder-local command `_IOR('Q',0x66,struct qc_hwenc_open_query)`, version1,
size88, aligned numeric fields only. Exact command comparison in the original
dispatcher rejects this command with ENOTTY; **the frozen76 image does not support
it**. This is source-supported expected refusal, not an observed native result.
Clients must require successful syscall, version1 and exact size; no fallback to
global `session_sequence`, fake supported flag or interpretation of partial copy.

Fields expose actual source-owned observations:

- This open description's existing numeric session identity; current reference
  owner's numeric session identity and relationNONE/THIS_OPEN/OTHER_OPEN. No file
  pointer/address, device label, payload, key or caller-asserted permission.
- Existing reference generation and the owner's codec frame number (zero if no
  owner). These are not new lease identities or proof of reference validity.
- Existing quarantine/DMA-uncertainty/guard-poison state and telemetry flags.
- Existing last-dispatch session identity, global operation sequence/result/kind
  and actual ledger-active flag, explicitly **history/ledger**, not readiness.

Session identity already increments under `enc.lock` in `encoder_open`; zero is
not assigned, and u64 maximum causes new opens to return EOVERFLOW. No namespace
reset/unload/reprobe or identity reuse is added. This is a boot-lifetime namespace,
not durable identity or a boot identifier. dup/fork shares the same open identity;
independent opens get distinct successful-open identities. Query cannot count or
revoke aliases, prove task/PID binding, exclusive custody or authorization.

Existing telemetry generation/operation counters **saturate**, set the existing
QC_STAGE_F_SATURATED flag, and are not nonwrapping operation tokens. The new query
copies existing flags so saturation/ledger/retirement error is not hidden. Codec
frame number wraps16 bits; it is not a unique generation. No attempt is made to
invent a new unique ref/job generation from these diagnostics.

## Locking, lifetime and init sequencing

The new function acquires existing `enc.lock`, reads this file's initialized
private session, checks any referenced owner's private session, copies the current
numeric snapshot and releases the mutex. Null/zero impossible-session states deny
with EBADF/EIO; they do not manufacture a session or clear failure.

VFS holds the calling file during ioctl. `encoder_release` uses the same mutex,
clears `enc.h264_owner` **before** freeing private session data, and then drops
the lock. Thus the other owner's private session cannot be freed between the
locked pointer lookup and numeric copy. No new reference/allocation is needed.
The query never stores or exports those private pointers.

Original `encoder_init` initializes mutex, allocates/installs telemetry via
`dev_set_drvdata`, then calls `misc_register`; published opens/queries therefore
have a telemetry context. Original built-in/static-DT/no-PM/no-kexec/no-forced-
unload guards remain. No query is callable as a substitute for that init sequence.

Jobs/open/final release/query all serialize on the same lock. A query waits behind
the existing synchronous operation, so it cannot observe a concurrently executing
job. `ledger_active` is merely the actual stage flag (normally0 at this boundary),
not an independent physical-idle assertion. `last_dispatch_identity` is history
and may be a closed session or an INFO/BUSY attempt. Last operation fields are
global, not the querying file's completion/result. Snapshot lock/copy latency is
not a hard time bound; only copy size is fixed.

The query only copies88 bytes to userspace. No MMIO, reset, allocation, clock,
stage/counter mutation, ownership transfer, reference clearing or capture occurs.
EFAULT returns no usable snapshot; it cannot prove or undo an operation. As with
the existing lifetime query, it remains readable for a held FD after quarantine,
without granting permission to restart/close/free anything.

## Actual compile result and retained failure

```text
python3 -B apps/qualification/kernel76-encoder-identity/build.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

12:04:19UTC:5 tool commands exit0 (HOST GCC16.2.1 version+ABI compile, retained
ARM GCC14.3.0 version+ABI compile, current Clang version). The Python prerequisite
then refused Clang drift before actual driver compilation or any linker call.
`runs/build01/REPORT.json` retains the overall **FAILED; outputs preserved** result,
not a driver PASS. ABI static declarations establish88-byte layout/alignment and
offsets on both architectures and preserved old structure sizes/command distinction.
They are compile declarations, not runtime tests or proof of handler semantics.

| Actual output | Bytes | SHA256 |
|---|---:|---|
| `objects/host-abi.o` |1128| `58f6fe1028ee195c1e29d020a6c6e26348479f7c88d8eeb9de3b92ad729b51f5` |
| `objects/arm-abi.o` |940| `f63a217d0411e8a48821734f18aed9a8c239b0a81f8bd5a361c4e6e649411135` |

No real driver object, ELF REL link, full kernel/module/image or executable was
produced. No HOST kernel shim, mock provider or synthetic campaign was substituted.
All19 explicitly captured input pins still matched at final source check. The
retained final76 config and symvers pins match its package manifest. Pinned ARM
toolchain checks passed; Clang23.1.1 is required by the current recipe's retained
metadata check. Historical Clang driver bytes alone do not establish the matching
LLVM/shared-library/resource/linker closure, so they were not used as a workaround.
Source GPL/SPDX, original headers and kernel COPYING are retained unchanged.

## One next task / later separately selected pairing and native use

**Next:** establish a complete pinned Clang23.1.1 compilation closure matching
retained76 metadata, then compile this exact fresh driver overlay with its recorded
Kbuild flags to a new object only. No installation/download/toolchain replacement
or alternate-compiler/config adoption was authorized or performed here. If parent
instead selects a new compiler baseline, it must be explicit and separately pinned;
it cannot be reported as reproduction of the old compiler pairing.

After ordinary source disposition/object compile, a separately selected paired
kernel build must adopt the exact source/header into a new full checkout and retain
new config/symvers/image/provenance and rollback, never modify original76. No full
build is queued. Later parent-only native query requires fresh hardware/recovery/
image/source association, owned preopened FD/aliases, exact new command/version/
size and observed rejection on old kernel; no query is sent by this worker.

Even a valid query is not final-release/DMA retirement, normal-close proof or a
live source grant. Watchdog ownership, actual retained failed-next process/backing,
128KiB/40MiB/CMA memory admission, native2MiB/HOST4MiB child separation, trusted
provisioning and owned listener remain open; old strict child pin/loopback/radio
HOLD are untouched. No camera/main/provider adoption, deployment, G2/G3/G8 result
or penetration/adversarial/fuzz/mutation PASS is claimed.
