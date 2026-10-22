# QC07F actual native producer / RTSP source wiring — COMPILE ONLY

2026-10-22; RP-07/09, STREAM-04/07; RP-04/08 dependencies **not satisfied**.

## Outcome

Implemented one real reusable source wiring function, **not a camera app**:
`apps/rtsp-auth/qc07f-runtime-candidate/rtsp_native_source.c` and `.h`.
No stub main, synthetic media, dummy success, alternate framework, new provider
protocol, listener launcher, secret generator or provisioning file reader exists.
Native memory/adoption HOLD is unchanged. This source candidate needs parent
review; compilation is not behavioral/source acceptance.

Handwritten app-side footprint:4 fresh files,446 lines/23601 bytes (C/header142
lines, compile-only builder248, README56). All writes are confined to the two
assigned fresh directories. The two retained build trees additionally contain
public source/license snapshots, logs, metadata, objects and archives; no existing
source/helper/vendor/plan or Git/index was changed.

The existing APIs cannot honestly support the requested complete executable yet.
In particular, preopened owned **provider** descriptor adoption is absent, while
the retained server is explicitly loopback-only. Neither predicate is bypassed.

## What the new source actually connects

`qc07f_rtsp_native_wire(output, lifecycle, bridge, producer, config)` uses the
existing `camera_rtsp_producer`, `camera_rtsp_common_producer`,
`camera_rtsp_lifecycle` and `candidate_config` structures; no second owner model.

- Requires a fresh initialized/stopped/keyframe-capable common producer and the
  original native owner's initialized FD=-1/no-loan state. It does **not** call
  native initialization, reset someone else's owner, or treat static FD0 defaults
  as initialized. One wire per process lifetime; no automatic retry/recovery.
- Installs actual common attach/detach/begin/retire callbacks. START only checks
  existing preparation prerequisites plus the server-bound current-peer gate;
  START itself performs no allocation, capture, sensor write or device query.
- `next` requires those prerequisites and that bound gate, then follows the real
  v6 `nh_native_next` lifetime → capture/encode → lifetime/cancellation/time checks.
  Unlike old `nh_native_next(..., key)`, it forwards the actual RTSP force-IDR flag
  to retained `produce_h264_frame` and its real hardware keyframe job. It does not
  manufacture an IDR, restamp generations or turn a failure into a packet.
- Output is the existing borrowed native `camera_packet`; no AU copy/allocation
  is introduced by the glue. `expected` is the original native scalar/pointer
  ownership record. Native release remains the actual release implementation.
- Native release has an `int` result, but RTSP release is `void`. The adapter
  latches failure into the existing native fence; common frame END then receives
  failure, not an invented successful completion. No payload/packet access occurs
  after release. Failed acquisition also fences existing backing/FDs, not retries.
- Installs `config.source_guard_bind` using the **existing** server gate, not a
  new guardian/process. Unbind never clears native uncertainty. Actual retained
  H.264 SPS/PPS generation supplies parameter bytes, not the synthetic fixture.

Return0 from wire means **callback assignment only**. Caller still owns config,
producer/event callbacks, retained metadata, all backing and code lifetimes.
There is no complete platform main, `source_ready` implementation or retention
owner in this candidate. In particular, the failed-next contract below is not
resolved by this glue and prevents native use.

## Exact provenance; originals never edited

| Input | Authority / exact pin |
|---|---|
| Frozen isolated-server public source | `apps/rtsp-auth/isolated-server-host-tail/`,41-file pin `2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f` |
| Native v6 source snapshot | `evidence/delegated/20261012-native-handoff/v6-final-source-snapshot01/manifest.json`, SHA256 `bdc87e1700b8f0a8e6c191ed0bd8869035487258636b55f4e6c8bce3640267d5` |
| Actual generated native input/adapter | `evidence/delegated/20261012-native-handoff/run34/inputs/`; run34 manifest SHA256 `8286c85c09d7f34d5bb6af13054803f68ebafad4e2e874da4b1a35aaf9e0d529` |
| Original native adapter | v6 `native_adapter.c`, SHA256 `33f65bb1b40a3fbe2b7e756e0c8e5a3a933a762cbb13737cdb16a9be1b5f0f93` |
| Generated native rtspd excerpt | `run34/inputs/rtspd-native-import.c`, SHA256 `ed40ac73b71f2dadd0cf72d0c69ec1020236b431d9a70d7f7ab67b7ed0a79e52` |

The new translation unit includes the byte-exact original native adapter and its
controlled generated source in the same translation unit, retaining its actual
singleton, rather than substituting mock callbacks. Build snapshots preserve
the exact copied source and existing import diff. The v6 source snapshot entries
are checked against run34 and their manifest before compilation. Current common
headers/sources and frozen server inputs are separately captured/pinned; their
compile compatibility is **not** native75→76 ABI or memory adoption.

Original GPL SPDX/notices and runtime licenses are retained. Mbed TLS3.6.7 tree
pin `bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5`,
GCC runtime exceptions and musl notices remain their respective licenses.
No device material, payload, credentials, footage or payload hash was read.

## Offline build result

```text
python3 -B apps/rtsp-auth/qc07f-runtime-candidate/build.py build02 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

**PASS11:39:27–11:39:38UTC**,54 compiler/archive/relocatable-link/ELF commands,
19 translation units per platform. HOST GCC16.2.1, pinned ARMv5 musl GCC14.3.0.
Output: `runs/build02/` in this directory. All410 public input pins unchanged;
356 source/header dependency paths retained. No test, fixture, generator, child,
listener, app, guard or camera executable was launched. No private input, target,
UART/RPC/network/status/boot/recorder IO or Git/index operation occurred.

| Output | Bytes | Meaning |
|---|---:|---|
| `libqc07f-native-rtsp-wiring-host.a` |256374| Actual native/glue/server integration archive; not runtime-qualified |
| `libqc07f-native-rtsp-wiring-arm.a` |179478| Same for retained ARMv5 musl; not run/adopted |
| `qc07f-native-rtsp-wiring-host-NOT-EXECUTABLE.o` |187208| Rooted ELF REL, not an executable/main |
| `qc07f-native-rtsp-wiring-arm-NOT-EXECUTABLE.o` |126924| Rooted ELF32 ARM EABI5 REL, entry0/no program headers |

The rooted relocatable link retains both `qc07f_rtsp_native_wire` and the real
`camera_rtsp_isolated_candidate_serve`/backend destroy call graph. There are **no
project symbols referenced by unresolved relocations**; ordinary libc/libgcc
dependencies remain because this is not a final executable link. Unreferenced
undefined symbol-table names from discarded direct-Digest sections are explicitly
distinguished in the report, not satisfied with stubs or an in-process credential
core. No verifier child is rebuilt or substituted.

Static allocated section totals are HOST1857403B / ARM1830756B, including original
raw BSS; these exclude external hub backing, final ELF/libc/stack/heap/child and
kernel costs and are **not RSS, a load budget or proof of fit**. Old1MiB native
allocation sites remain untouched and are incompatible with product adoption
under the128KiB per-allocation cap. No cap is lifted or exception granted.
The native child stays2MiB; separate HOST4MiB is not borrowed. Frozen native image
pin remains153060B / `7a8d66850197ae62dd3db4718c57ba0487ccb4fdcaac1c7819241fd0be10f084`.

Earlier build01 also compiled/relocatable-linked, but predates the final FD0/init
refusal and explicit relocation inventory. It is preserved, **not** the final
source pin. Full commands, logs, source snapshots, licenses and artifact hashes:
`runs/build02/REPORT.json`. Generated archives/objects are local evidence; do not
stage them as source/release artifacts. No final static executable link is claimed.

## Blocking APIs/contracts before a real main is honest

| Missing contract | Exact existing boundary / smallest necessary change |
|---|---|
| **Owned provider-FD adoption** | v6 `native_adapter.h` exposes init/config/next/release/retire, not FD adoption. `native_adapter.c:lifetime` and generated `continuous_next`/`produce_h264_frame` internally open encoder, frame, I2C and watchdog nodes. Their `void *` is ignored, not a public provider handle. A bootstrap FD number cannot be cast into a native context or treated as a receipt. Add reviewed ownership transfer/retirement for each real FD role; replace those internal opens only in a fresh derivative. Do not assume dup/fork preserves independent ownership or close semantics. |
| **Failed-next retirement/retention** | `framework/include/camera/rtsp_server.h:36–42` requires a non-OK next to own no returned packet and retire its partial work internally. v6 deliberately retains CPU/FD/DMA uncertainty until parent recovery; its original `nh_run`/main owns that HOLD. A `camera_producer` frame END or STOP request does not retire it. Define a real owned retention handoff before allowing error return/service exit; preserve original loan/backing/receipt identities and no END/STOP retry. This adapter only latches failure, not that handoff. |
| **Admitted backing and allocator contract** | v6 raw1728000B + hub1MiB + optional producer1MiB, conservative placement/migration ledger and source75-only raw-reuse proof remain HOLD. `nh_native_memory_fixed_buffers` takes an owned CPU span, not proof of allocation/adoption/FD pin. Real preowned hub/export backing must have proved extents, lifetime, placement and source associations. Replace forbidden large allocations with an admitted borrowed backing contract, not arbitrary pointers/addresses, fabricated capacity or a larger ceiling. The original outer `nh_run` memory-stage schedule is not replaced by callback assignment. |
| **Finite qualification lifetime versus ordinary runtime** | Native memory runtime is10s/64 observations; sensor exact139 preparation is one attempt, capture sequence bound128, and normal END/QUERY/ACK is strict/no renewal/retry. Reattaching after STOP cannot silently reset the sensor report, native singleton, memory ledger or refs. Need a reviewed bounded normal-session/restart policy with actual retirement, not remove timers/counts. No such extension is implemented. |
| **Trusted provisioning/bootstrap** | `candidate_config`/`rv_start` already require preopened image and material FDs; there is no platform bootstrap here. Native candidate requires root/groups0, exact0555 root-owned native IMAGE FD/pin and readonly1536B0600 regular material FD, nlink1, position0; trusted bootstrap must quiesce writers, not merely check mode. Need independent RTSP domain/routing/generations/rate policy and cold entropy from the real authority. No credentials, private reads/hashes, public fixture material or build02 image substitution are permitted. |
| **Current source/retention callbacks** | `candidate_config.source_ready`, `random_ready`, `retain_uncertain`, `retain_frame_uncertain`, `retain_verifier`, optional owner_poll and producer events need a real retained owner. `nh_native_cancelled` includes a device receipt query, not a universally pure readiness callback. Metadata copying alone does not retain FDs, namespace, buffers/code or retire a child. No dummy always-ready/success callbacks are supplied. |
| **Usable network entry** | Frozen server `rtsp_server_candidate.c:430` requires bind127.0.0.1,2s,one1MiB hub and lifecycle; line452 labels HOST_ONLY. It creates its listener after real verifier startup. It has no preopened-listener API. A camera LAN main cannot bypass this with a string/default-address edit. A separately reviewed explicit owned endpoint/listener contract is needed; authentication and bounded send checks remain intact. |
| **Product routing/package/dependencies** | `bringup/qc07f/userland/rtspd.c:653–672` wires the old real producer to NOAUTH lab `camera_rtsp_serve`, with malloc1MiB and no product bootstrap. Its config/graph/SDP are not an authenticated runtime main. Need actual device/profile/URI bindings, ownership/error disposition, supervision and release manifest; modern76 provider/kernel/source association and original radio HOLD remain independent. No alias of main/sub into extra sensors or claimed qualified capability. |

## One next task (SOURCE ONLY)

Specify and implement **one preopened owned native provider adoption increment**
in a fresh derivative: enumerate the actual sensor/frame/encoder-stream/temporary
encoder/watchdog FD roles from the retained functions, pin their source/runtime
ABI and define transfer versus borrowed lifetime/close/uncertainty rules. Require
every prerequisite before any open/ioctl/capture/allocation effect; missing roles
must fail before effects. Preserve v6 receipts, memory gates, no retry and retained
HOLD. If exact ownership/backing facts are absent, parent must establish those
facts before implementation can safely proceed; this is not a target command.

Only after that contract and the other listed dependencies are resolved can an
actual main consume mandatory owned bootstrap/provider descriptors and call the
existing strict server. No native launch, camera/deployment/G2/G3/G8, protected
browser/RTSP, normal-operation or further-camera acceptance follows here. Deferred
adversarial/fuzz/mutation work remains deferred, not passed.
