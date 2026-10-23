# Public RTSP OS randomness — independent ordinary review

2026-10-23 · SEC-06 / STREAM-07 · SOURCE/API/bounds and public evidence only.

**Disposition: narrow conditional sufficiency. No ordinary correction required in
this selected scope.** The recorded sole normal HOST OS callback observation is
adequate as HOST functional evidence only; it is not native entropy qualification.
This review selects no integration, execution or new work.

## Exact reviewed inputs

Read all six requested candidate files in full, CONTRACT/HANDOFF/AUTHOR_AUDIT,
source/pre-call/delivery seals, actual frozen runtime field/calls/cleanup and the
original zeroizer. Paths below are relative to the workspace; SHA-256 values are
public file/artifact pins, **not hashes of random output**.

Candidate prefix: `apps/rtsp-auth/rtsp-os-random/`.

| File | SHA-256 |
| --- | --- |
| os_random.c | `1e5706a00656ff8dc80aaf48859b66b5dca723461fa9b1a3aade7d03fc1df251` |
| os_random.h | `6d04f85c72f29fefd7e9e3a48d3afbef79284b277547c893b3b4801b07b5424e` |
| host_one_call.c | `c8d3274b8b512221ce376ae3d652deda5e43bf6d38e5cf28720567e0c68a931a` |
| compile01.py | `2c3052ab5462ab815b1f1d0f438c246ac7abded7393b5658e0c4d86a101b5010` |
| host_once.py | `b28a086a87ef198d22bb0a9dd4392e322658fd1b44d54833b4a48347d4d9f08c` |
| LICENSE_NOTICES.md | `3869c735dee3cce1f65191994d5ae9b20ae560b63e8660448e28db3b3f729de4` |
| .gitignore | `637400650411cd4eaf40681601af96eed5b72b298b06e54ed82a3d143938956a` |

Reconstructed seven-file source aggregate:
`f1a8691ee681b11e3cbb0b4c5d8535721a695840540ac80a307be226c0e3ff64`.
Definition: sorted workspace-relative path + space + file SHA-256 + newline.

Evidence prefix: `docs/qualification-plans/rtsp-os-random-20261023/`.

| Seal / record | SHA-256 |
| --- | --- |
| SOURCE_PINS.json | `86017c96b082bbb613fd5073c01c0f8d1304e93385406fe136395d9d53ba5a28` |
| PRECALL_SOURCE_PINS.json | `17c76704ceb372ae127e173a7df5f78bd3d972468a85d865cd61105294683052` |
| DELIVERY_PINS.json | `3d2201fea3e78581e5085d12ebce3a496d0c90dab0b71b3dcad9c86852b7111a` |
| runs/build01/REPORT.json | `8dab817bc9990f045a15e625524473690ef5b4494891f2331b12ff73dc2df6c4` |
| runs/host01/OBSERVATION.json | `4c34c6e1273e058a7fabe353492fe2759d507f25bae4066d15e21ce57242f372` |

Independently rehashed **548 overlapping public pin records**: source7, pre-call9,
delivery79, build inputs16, tools30, artifacts75, consumed dependencies328 (164
unique dependency paths), supplemental tools4. All sizes/hashes and recorded
resolved paths matched. All26 separate command JSON records equal their report
entries; all recorded return codes are0. This is file custody verification, not
an independent execution trace or a count of test coverage. Supplemental tool
pins remain post-call provenance, not proof those optional plugins executed.

## Ordinary source/API/bounds findings

1. **Real production path, not fixture entropy.** `os_random.c:14–25` ignores
   context and has no state/cache/allocator/selector. Null or length other than32
   returns CAMERA_INVALID before writes or OS access. A valid writable32 is
   cleared with actual `mbedtls_platform_zeroize`, followed by exactly one C API
   call `getrandom(out,32,GRND_NONBLOCK)`. Only exact32 returns CAMERA_OK; every
   other return wipes all32 and returns CAMERA_NOT_READY. No retry, fallback,
   new crypto, statistics, byte inspection or credential ingress is present.
2. **Exact API binding and real consumers.** The compile-time type assertion binds
   to `candidate_config.random_ready`, actually
   `int (*)(void *,unsigned char *,size_t)` at frozen
   `qc07f-owned-runtime/server/candidate_auth.h:26`. The only two actual calls
   are `candidate_auth.c:99` (32-byte startup probe) and `:129` (32-byte public
   RTSP session material). Probe cleanup at:105 and random cleanup at:131 call
   `rv_wipe`; peer close at:139 also wipes the encoded public session record.
   `isolated-verifier/ipc.c:19` delegates rv_wipe to the actual Mbed TLS zeroizer.
   No verifier CSPRNG seed/nonce, privacy key, TLS/HTTPS or credential RNG changes.
3. **Documented OS semantics, not target quality proof.** The pinned local Linux
   man-pages6.19 getrandom(2), dated2026-02-08, supports NONBLOCK/EAGAIN before
   urandom CRNG initialization and initialized small-read semantics. Neither
   GRND_RANDOM nor GRND_INSECURE is selected. All nonexact outcomes are refused
   regardless of errno. This establishes the selected Linux API contract, not
   entropy quality/health, an observed kernel syscall count, cold-target CRNG
   readiness, or a hard wall-time bound on kernel/libc execution.
4. **Bounds/preconditions.** Only the caller's32-byte span is touched; no adapter
   buffer/allocation or128KiB exception. Trusted legitimate, disjoint writable
   storage and serialized lifetime are essential: no arbitrary-pointer, alias,
   concurrent-mutation or hostile-memory safety claim. Compiler stack records say
  16B for the callback in each role, not whole-app/native memory-fit evidence.

Frozen context SHA-256 pins:

- `apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.c`:
  `cb27b35e2d8730b8d19f231d339e06cbe4c97fbe0aca7826fe3469d8479825a7`.
- Same directory `candidate_auth.h`:
  `94b6ed560677b7db4119eded9726ab01085c2e0eb392bdad29e961ebcd2ed643`.
- `apps/rtsp-auth/isolated-verifier/ipc.c`:
  `f42968ebf2fa7a361dfc1db31599a061df7f5c7de718a3eac4b0e2e2a4becc01`.
- `apps/auth-web/service/vendor/mbedtls/library/platform_util.c`:
  `83051fddcd37171831ff5706613dc3d336b2e53c42e6515f2f14ac0f8aa649f1`.
- `apps/rtsp-auth/rtsp_crypto_config.h`:
  `7cfb2b4eacf9379bd945a2faa9a95bc228c131bdf956a07e3953fdba6d7f818f`.

## Cited build and sole HOST observation

Recorded26 commands comprise five C compiles (HOST callback/zeroizer, ARM
callback/zeroizer, HOST harness), two REL links, one HOST harness link and18
compiler/ELF inspection commands. Five compile logs and two REL logs are empty;
recorded HOST GCC16.2.1 / retained musl ARM GCC14.3.0 commands use actual headers
and original `platform_util.c` with the unchanged RTSP-only crypto config.
Production units have no HOST-fixture define; only the harness gets its distinct
`RTSP_OS_RANDOM_HOST_ONE_CALL` guard. Its source also requires Linux x86_64,
excludes ILP32/ARM and rejects the named unrelated fixture classes.

Raw ELF header reads independently agree with sealed readelf logs: HOST ELF64
x86_64 REL and ARM ELF32 little-endian EABI5 REL have no program headers/entry;
ARM attributes say v5TE. The sole executable is HOST x86_64 PIE, not ARM.

| `runs/build01/` artifact | SHA-256 |
| --- | --- |
| host-random-NOT-EXECUTABLE.o | `9197253cfd9bb6720e17a0fc3b5b23f53aa7afb94738a6ad9cae1dcb15aad50d` |
| arm-random-NOT-EXECUTABLE.o | `52bf2049cf4544e7e406098c6aedb080a32cf33661df22b7d1f526f2b016981b` |
| HOST-OS-RANDOM-ONE-CALL-NOT-NATIVE | `58e0f05035ce0b6fc17bb82ce7680ba0b5938916251c43c51610ff005a9e71ab` |

Sealed nm/link-map evidence matches callback + actual zeroizer + libc/startup:
no backend/verifier/provisioner/provider/listener/media implementation in the
harness. The original zeroize-and-free function exists in the RELs but is
discarded from the HOST executable; callback does not allocate/free. ARM getrandom
remains unresolved in REL: this is not an ARM libc/syscall link/runtime proof.

`host_once.py` is read-only with respect to support code/inputs; it writes only
its fixed evidence output. Exclusive attempt-directory creation precedes effects;
one fixed subprocess call, no options/retry/fallback, a10s host wait and explicit
`LC_ALL=C` environment with no inherited loader/config selectors. That is **not**
an inherited-FD audit, secret-free launch proof, sandbox or native hardening claim.
The harness calls the callback once on32 stack bytes, checks only CAMERA_OK,
wipes32 before reporting, and emits no bytes/hash/statistics. Pre-call audit/pins
predate the recorded attempt. File records support one selected observation;
they are not a system-wide history of OS randomness calls.

Cited author observation: `2026-10-23T11:57:50.236521+00:00`, exit0, stdout exactly
`HOST_OS_RANDOM_ONE_CALL_PASS NOT_NATIVE` plus newline, stderr empty. No random
output was retained or hashed by this path. Review made **zero** callback calls.

## Conditions and exclusions

Future explicit parent selection must choose this callback **before** the
provisioner config snapshot and bootstrap prepare/start, while preserving frozen
config/address/lifetime prerequisites. No automatic registration, live-config
mutation, integration or native adoption is approved here.

Native hardening, ARM syscall/runtime, cold CRNG, target boot/verifier seeding,
TLS/keys, entropy quality, SEC-06/G3/G8/product acceptance remain NOT RUN/unproved.
Existing bootstrap/backing/recovery/provisioning and128KiB-cap blockers remain.
No negative/adversarial/mutation/fuzz/statistical test or second HOST case was
performed or selected. No candidate import, helper/build/ELF execution, private
data, actual HOLD/capture, proc/dev, network/serial/RPC/HTTP or hardware access;
no Git/index/AGENTS/central-plan/source/provider/model/tool/safeguard change.
Only this fresh review document was written. All old code/31 failures/23g prefix
HOLD and camera-OFF context are unchanged, inherited rather than freshly queried;
there is no hardware queue and no new-work selection.
