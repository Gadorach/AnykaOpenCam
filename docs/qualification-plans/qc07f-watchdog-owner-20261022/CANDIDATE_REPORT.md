# Single-owned watchdog / actual v6 integration — candidate handback

2026-10-22. Selected watchdog-lifetime dependency of RP-07/09 and
STREAM-04/07; RP-04/08 and all product/native gates remain open.

**Outcome: actual v6 derivative plus reusable watchdog owner compiled for HOST
and ARM; archives and relocatable objects only. Not executed, reviewed/accepted
by the parent, integrated into the frozen RTSP glue, or deployable.**

## Scope and provenance

Only `apps/rtsp-auth/qc07f-watchdog-owner-candidate/` and this documentation
directory were changed. The app directory contains eight files, 658 lines,
37,533 bytes. No main, second producer, mock device, fake success callback,
background feeder, new guardian, watchdog open/dup/close, reset or retry API.

`overlay.py` applies unique exact-context replacements to the real immutable
run34 v6 `native_adapter.c` and `rtspd-native-import.c`. Both original and generated
inputs, added includes, upstream license material and the complete applied diff
are retained under `runs/build04/`. Original run34 manifest SHA256:
`8286c85c09d7f34d5bb6af13054803f68ebafad4e2e874da4b1a35aaf9e0d529`.
There is still one actual native singleton and producer; wrappers rename and call
the real original bodies, rather than replacing their effects with callbacks.

Inspected watchdog core and Anyka driver are byte-identical to their retained76
copies. Their SHA256 values are respectively
`07e4291f639506d558d9eaf0176f7b901ab132580f1800c58dc2340c70814bf2` and
`d6646a2b894f822ea486c4d24db7a49df2003a5706cf25e9908bc1fe15fe06e9`.
The build captures the inspected core, driver, UAPI and `fs/file_table.c`.
This source association is not live kernel/config/toolchain/ABI qualification.
New code is GPL-2.0-only; original notices/licenses remain, including the driver's
GPL-2.0-or-later notice. No relicensing claim is made.

## Real ownership and control boundaries

`watchdog_owner.h` documents trusted serialized, sole-open-description custody:
no aliases, concurrent descriptor-table changes, external service, reentry or
unadmitted device/backing use. Descriptor metadata cannot prove these conditions.

| Boundary | Actual candidate implementation |
| --- | --- |
| Bootstrap move | `qw_owner_take` checks expected character-device identity, O_WRONLY, CLOEXEC, FD >=3 and empty owner. It transfers the actual preopened slot and clears the source slot only after checks. State is UNKNOWN, not armed proof: opening already starts hardware. |
| Native admission | Fresh singleton init, move, original explicit exact139/storage/memory preparation. Memory entrypoints require guard presence; `nh_native_next` additionally requires initialized ledger and paid raw/hub records before lifetime open/query or producer effects. This is not complete provider/backing adoption. |
| Begin | `native_guard_begin` uses real `nh_native_now`, original `memory_state.session_start + NH_SESSION_NS`, and `qw_owner_begin`. SETTIMEOUT5, ENABLECARD and GETTIMEOUT5 must succeed under the same live phase/session deadline before ACTIVE5. |
| Capture progress | Generated `continuous_next` validates complete raw extent/pointer, guard/status, sequence, full raw CRC and keyed ACTIVE receipt before its one `qw_owner_capture_progress` call. KEEPALIVE is permitted at most once per NEXT, never from a periodic loop or Boolean qualification input. |
| Capture + encode | The two original independent open/V/close brackets are removed. Sensor baseline borrows this same actual owned FD for its existing timeout observation; no extra ping is added. Capture and encode remain in the single outer interval. |
| Post-acquisition | `nh_native_next` calls real `native_next_legacy`, including both lifetime queries, cancellation/receipt and encoded-deadline checks. Only their success and clear uncertainty/clock state permit outer logical disable. Failures zero outward packet/key and fence without releasing unreturned backing/FDs. |
| Loan release | `nh_native_release` requires logical DISABLED observation plus original packet-identity checks. It retires the CPU loan only; charged backing remains resident. |
| Normal retirement | `nh_native_retire` / `_held` begin a RETIRE interval and call real source sensor-off, END/QUERY/ACK and close-once paths. Original successful proof and clear uncertainty precede disable. Resident/parked CPU backing is freed only after successful logical disable; a failed disable retains it in the actual owner. |
| Healthy handback | `nh_native_watchdog_give_back` requires no armed receipt, provider FDs, held/resident/parked backing, uncertainty or close-unknown flag, then moves the DISABLED watchdog description to an empty recipient slot. No final close is implemented. |
| Uncertainty | `nh_native_fence` latches retirement uncertainty and watchdog HOLD. Original process SIGSTOP quarantine remains, reporting actual watchdog FD/state and parked backing separately from failed-close diagnostics. No V/close, stop retry, re-enable, recovery ping, timeout growth or HOLD-clear is performed. |

The owner state progression is EMPTY -> UNKNOWN -> ENABLING -> ACTIVE5 ->
DISABLED, with a new admitted begin from DISABLED or a healthy custody give-back.
Failure latches HOLD. Neither state names nor metadata prove electrical state.

Core `watchdog_dev.c:784-825` supports SETOPTIONS disable/enable and timeout
operations. SETTIMEOUT can itself ping; the Anyka `set_timeout` also reloads
active/running hardware. These are acknowledged begin effects, not an additional
unconditional progress feeder. Core open is single-open/start; close may stop or
ping depending on release/nowayout semantics. Retaining one actual description
avoids competing opens but does not establish current nowayout or recovery policy.

## Deadlines and uncertainty limits

Nominal watchdog timeout remains 5s. Source phase remains strictly <2s and the
original session deadline remains 10s, with no renewed session or growth.
`qw_owner_check` reads CLOCK_MONOTONIC before and after descriptor inspection;
every control uses checks before and after the syscall. Unknown/backward clock,
expired phase/session or bad descriptor latches HOLD before another control.
Generated `producer_begin_ns` uses the earlier outer owner start, so memory
forecast/capture preparation include initial lifetime/admission overhead. The
original inner encoded-duration check also remains.

These userspace pre/postchecks cannot preempt a blocking ioctl or prove finite
native scheduling/driver latency. A late/error result can follow a real effect.
In particular, DISABLECARD may have stopped the watchdog before a subsequent
failure: retained FD + HOLD **does not guarantee armed recovery**. Successful
disable/driver return0 is logical acknowledgment only, not electrical stop or
DMA retirement. No sysfs inactive observation is substituted for those proofs.
The local SIGSTOP path is not the missing common RTSP failed-next process owner.

`source-close-once.inc` is only for non-watchdog provider descriptors after the
original job/retirement boundary. A pre-close guard failure retains the actual
slot; otherwise the slot is cleared before the one Linux close attempt. On close
error, `last_close_attempt` is explicitly NOT owned and `close_unknown` is sticky.
There is no numeric-FD retry or claim that an error preserved that description.
Other still-owned descriptors/backing and the watchdog remain retained.

## Build evidence and preserved failures

Final command (compile tools and pure overlay generation only):

```sh
python3 -B apps/rtsp-auth/qc07f-watchdog-owner-candidate/build.py build04 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Final build ran 12:54:10.544728–12:54:13.570092 UTC. All 26 recorded commands
returned0: compiler version/target, six C units per platform, deterministic
archive, relocatable link and ELF/symbol inspection. HOST GCC16.2.1 targets
`x86_64-pc-linux-gnu`; retained pinned ARM GCC14.3.0 targets
`arm-buildroot-linux-musleabi`, ARMv5TE/soft-float. Full argv/environment/tool
hashes, logs, maps, stack-usage files and dependency closure are in
`runs/build04/REPORT.json` and adjacent files. Legacy compact units retain the
original misleading-indentation exception; standalone owner has none. Compilation
uses Wall/Wextra/Werror, stack protector and FORTIFY; compile logs have no warnings.
Relocatable outputs still require libc/runtime dependencies and are not runnable
camera executables. No executable, listener, output object/archive, test, synthetic
bench, kernel/module/image or native operation was run/built beyond this scope.

| Final artifact | Bytes | SHA256 |
| --- | ---: | --- |
| HOST archive | 89086 | `9869c80f1b4c1a97b563e3bebb0d3c8715d7b2754a70fe272e5b1f66935680c7` |
| HOST REL | 80920 | `400cbcfb96e8c90028f2013fa05968132ae9e8b5d9d5144d0577c9cf5925304d` |
| ARM archive | 60916 | `29848aafe3a2e1b1a9991e5cd666aec1d9ffc4059a868e7e1abfc14c24cd654e` |
| ARM REL | 53896 | `38e8907d45b423d67426b94ec35381a755e4050837971415bc481df69797e8ff` |

Applied diff: 30,953 bytes,
`c7eb5c52c5bd0cd4d79e9ede7a387831a8001163cd1da5393dc4162e7e63a506`.
Generated adapter: 10,470 bytes,
`a32d2cb5c48ec96c9c32106f967c8ac75d7ac7b6e045ec86ae45ab5a6fa34c7c`.
Generated producer: 16,399 bytes,
`a609c60448cd633772a67be9770320dec7a93745e20c258784c77045252f8bd6`.

All earlier runs remain separate and unchanged:

- `build01`: stopped before any compiler command on nonunique frozen overlay
  context `{c->retirement_uncertain=1;return CAMERA_IO_ERROR;}`. Corrected by matching
  the entire `capture_failed` definition, not weakening input pins or uniqueness.
- `build02`: initial HOST/ARM pass; superseded, not final source.
- `build03`: HOST/ARM pass after earlier outer deadline alignment, guard-presence
  gates and fresh admission before output allocation; superseded.
- `build04`: final pass, additionally defers resident/parked CPU frees until disable
  success. Earlier passing artifacts are not substituted for these final pins.

Final read-only metadata/hash verification found no mismatches: 276 captured
input paths, 234 consumed source/header paths, 28 generated source/header files,
44 prior frozen source/evidence pins, all eight live app files versus captured
build04 inputs, 26 command log pins and four final artifact pins. Original inputs,
prior frozen pins, sensor139 implementation/table and all three memory-policy C
implementations are unchanged. `SOURCE_SEAL01.json` pins this report, every
candidate source file and the retained run evidence; it is not review acceptance.

## Open gates and one next parent dependency

No complete mandatory provider-FD adoption or admitted backing is implemented.
Actual sensor/frame/encoder opens and old large buffers remain in the real v6
derivative. The product128KiB cap is unchanged and not satisfied by preserving
1MiB allocations; there is no allocation-cap exception, new memory-fit claim,
40MiB/CMA forecast waiver, or strict native2MiB/HOST4MiB/loopback policy change.
Sensor table139, radio restrictions and existing failure evidence remain intact.

Frozen RTSP callback glue includes the original producer internals; it has NOT
been rebased onto this derivative. Persistent encoder/frame custody helpers and
observational query/compiler candidates remain separate, unchanged increments.
Failed-next retention/process lifetime, backing, provisioning, owned listener,
full source review and native guard qualification still block a usable app.
No full provider/app/G2/G3/G8 or paired76 ABI/build acceptance is claimed.

**Next dependency: parent focused source review of the exact build04 owner/overlay
and failure/retirement boundaries before selecting any further integration.**
No hardware/full build/test queue is selected. Parent alone owns hardware. Last
reported target state is held original U-Boot with original radio and failed-reset
controller HOLDs preserved; this source-only work made no fresh observation or
target/UART/RPC/status/network/private IO. No Git/index/central-plan/P1, model,
provider or safeguard changes were performed.
