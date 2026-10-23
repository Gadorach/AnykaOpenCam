# Actual owned-provider RTSP feature increment — handback

2026-10-23. **C implementation delivered; HOST/ARM compile/archive/REL PASS.
Nothing run; not independently accepted, deployable, native or product GO.**
CORE-03/04, CAP-05/07, STREAM-01/04/07, RP-08/09 integration contribution only.

## Actual feature implemented

`apps/rtsp-auth/qc07f-owned-runtime/owned_runtime.c` connects the current native
frame/encoder/watchdog-owned singleton to the real common producer lifecycle and
authenticated prestarted-owned-listener RTSP server. No second native stack,
backend/child, synthetic packet source, test fixture, monitor/client or main.

- Versioned `owned_runtime_v1.h` provides device-free **prepare before verifier
  fork**, then explicit **bind after the last fork**. The latter calls actual
  one-time `nh_native_init` in its owning TU, not a caller-ready Boolean. Immutable
  callback/config/context/graph addresses precede start; only state inside that
  same context is filled afterward. Real RNG/provisioning remain caller-owned.
- Common first/last demand, logical START, keyframe deduplication, exact frame
  END and normal STOP use real producer code. Actual RTSP force demand reaches
  the encoder job via `nh_native_next_keyframe`, **inside** the current owned
  watchdog guard → legacy checks → disable boundary. Existing force0 NEXT stays
  compatible. No direct unguarded produce call in glue or extra watchdog owner.
- Actual packet + `nh_key` stay in stable context through release; native release
  validates the actual loan. The original SPS/PPS generator supplies parameters,
  never fixture H.264. Existing PRE_NEXT and first POST_FIRST gates are connected.
  The server owns its hub copy before release and passes independent scalar
  keyframe facts to common END. Errors latch; no void release/END/retire success
  masks an error. Single-VCL output only, not complete multi-VCL STREAM-01.
- Uncertain native failed NEXT cannot return into RTSP's unchanged internal-
  retirement contract. Native/release/END/retire/clock uncertainty with retained
  work enters the **actual nonreturning quarantine** before unsafe cleanup.
  Three small server hooks catch clock uncertainty before client cleanup and
  unretired work before final unbind/listener close/stack return. No metadata
  substitute owner or false retirement. No-work configuration refusals return
  normally. Healthy pre-memory custody remains eligible only under original API.

## Exact source provenance / delta

`INPUTS.json` records87 selected public input pins and exact direct-copy origins.
`SOURCE_ORIGIN_CHECK.json` binds20 original authored-provider files to aggregate
`9ac7f363d7395fe944f2d068a4c964a5b20c77e9410e84633c07ce7f8b8a169f`, the40
actual generated files to their original build report and sealed build-report
pin, and14 composed server files to aggregate
`09325264c224a43b5664b48123bce1d05ea1269e6f209dcd2c9967424fdcba33`/source-seal
`fa9d056819662781a46307d94a10b4d3ce820f0eb24678fc0e591371b52ba95b`.
No old generator/helper was imported/run. All selected originals remain unchanged;
five generated framework headers match current actual framework bytes exactly.

Only **four copied source files** differ: native adapter C/header, native watchdog
wrapper, and actual server C. `SOURCE_DELTA.patch` records the complete133-line
comparison. The original guard/lifetime/provider/cancel/receipt/time/output/key/
known-loan ordering is unchanged except force0/1 forwarding and its input check;
quarantine is annotated `_Noreturn`, not replaced. Auth/backend/verifier/listener
custody bodies, sensor139/table, memory-policy and other native files are exact
copies. Two new feature C/header files implement wiring; no old source edited.
Original SPDX and seven copied license/notice files remain intact.

## Compilation (not behavior)

Fresh recipe audit: `BUILD_AUDIT.md`. Exact foreground invocation for final source:

```sh
python3 -B apps/rtsp-auth/qc07f-owned-runtime/build.py build02 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

`runs/build02/REPORT.json`: **62 successful commands,46 actual C compilations**,
two archives and two ELF REL objects;364 input pins,378 consumed source/header
paths,50 artifact pins. No cached/stub objects or executable target. HOST GCC16.2.1;
retained ARM musl GCC14.3.0, ARMv5TE/EABI5 soft-float. Both compile strict native
authentication, no HOST fixture define. No compiler diagnostics. Actual project
relocations resolve; only libc/compiler/linker obligations remain in ELF REL.
One definition each of feature APIs, native force/compatibility NEXT, quarantine,
prestarted start and composed serve. `main` absent. Whole-program link, runtime ABI,
memory fit and actual source behavior are **not** proved by REL inspection.

Final build interval03:20:12–23UTC is ordinary host build timing, not admission.
`build01` also passed and remains frozen; final build02 includes a bounded source
correction for gate-denied retained work/common-bridge clock uncertainty. No
failed compile, mutation test or old failure was discarded. Logs/argv/flags/tool
pins/dependency pins/output hashes are in the reports; final source/document pins
are in `SOURCE_SEAL01.json`. Generated archives/objects remain local, not Git.

## Functional checklist — ALL NOT RUN, no queued operation

- [ ] Real device-free bootstrap/continuing external media-process owner/recovery.
- [ ] Genuine RNG/private provisioned verifier startup; all forks before native.
- [ ] Actual sole watchdog/frame/encoder/provider take and admitted paid backing.
- [ ] Authenticated PLAY → common demand → guarded native capture/encode/IDR.
- [ ] Native packet/key release, owned hub copy, exact common END and last STOP.
- [ ] Failed-NEXT/release/retire/clock quarantine preserving live stack/FD/backing.
- [ ] Independent all-domain recovery and normal/failed shutdown disposition.
- [ ] Native decode, client fan-out/reconnect, load/headroom and packaging/main.

The missing **real media bootstrap + external owner/recovery + admitted backing**
are the next concrete integration prerequisites, not another clock/receipt/build
framework. SIGSTOP retains process-local owners only while that process survives;
it does not stop DMA, provide an external owner, guarantee watchdog reset or retain
an exited/OOM-killed task's FD table. No TERM/KILL/respawn/resume/feed/rearm/END/reset/
close retry on uncertainty. Original post-memory unactivated abort/failed-state
handback remain unsupported. The inherited1MiB allocation sites remain unqualified
against128KiB cap; no memory exception or claimed40MiB/CMA fit.

Source API requires trusted serialized stable/disjoint calls/storage/FD table,
no signals/threads/reentry/aliases or later fork/dup/SCM. No loopback/auth/domain/
MD5/rate/MEMLOCK/pin or2s/10s/5s change. Both native HOLDs stay sticky; last held
U-Boot observation is historical, not current runtime/owner/recovery. No camera,
ELF, default/help program, listener, test/fixture/payload generator, private input,
target/current-state/proc/dev/HOLD/control/capture/serial/UART/RPC/network operation,
Git/index/central-plan/AGENTS/query78/kernel/image work. No policy refusal/retry/
reroute or worker delegation occurred. **One source/compile handback to parent for
actual code review; no self-acceptance or automatic next queue.**
