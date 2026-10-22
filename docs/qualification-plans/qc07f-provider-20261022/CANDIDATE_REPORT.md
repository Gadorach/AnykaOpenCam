# Preopened provider FD increment — frame custody only; full adoption BLOCKED

2026-10-22; RP-07/09 source work, RP-04/08/native/failed-next HOLD unchanged.

## Delivered, not inferred

Implemented `qc_frame_fd_take` / `qc_frame_fd_give_back` in
`apps/rtsp-auth/qc07f-provider-candidate/frame_fd_custody.c` and `.h`.
This is **actual move-only custody of one already owned frame FD**, checked
against its real existing per-open receipt. It is not a provider-ready flag,
capture adapter, full provider set, app main or native admission.

App-side footprint:4 new files,373 lines/19095 bytes; C/header142 lines, builder179,
README52. Writes are confined to this new app directory and this evidence directory.
Frozen runtime candidate/server/native sources and earlier evidence are unchanged.
No Git/index, central plan, P1, boot/helper or safeguard/settings work occurred.

The helper calls only `fcntl(F_GETFL/F_GETFD)`, `fstat` and **existing observational
QC_ISP_RECEIPT_BIND** on a trusted preopened descriptor. No descriptor acquisition,
dup, close, ARM, ACK, END, reset, I2C, watchdog operation, backing pointer or
allocation is exposed. No new UAPI or original-source overlay is applied:
`applied_source_overlays=[]` in the build report. No output/test was executed.

### Exact supported custody contract

1. Trusted bootstrap already owns the actual native75 frame **open description**
   and independently observed device/source/file tuple. It must have the exact
   pinned provider/source association and sole, serialized custody: no remaining
   dup/fork/SCM_RIGHTS alias, concurrent I/O, fd-table changes, close or signal
   reentry. This is an explicit trusted typed precondition, **not a Boolean input
   and not a fact this helper can establish**. Without that history do not invoke.
2. Fresh owner uses `QC_FRAME_FD_CUSTODY_EMPTY` (FD=-1). FD0/zero-default storage
   refuses. Input FD>=3 must be readonly, CLOEXEC and a character node matching
   the supplied actual device ID. No fixed/dynamic minor is guessed or hardcoded.
3. Existing BIND must successfully return version1/reserved0 and the exact nonzero
   expected source/file identities, stateNONE/generation0/reason0/error0/time0.
   Failed syscalls' partial replies are never interpreted. Only then ownership
   moves from the bootstrap slot to the actual stored FD; bootstrap slot becomes-1.
   Precommit failure leaves that FD with bootstrap; no close/destructor is called.
4. Give-back requires an explicit-1 destination, repeats metadata/BIND checks,
   and moves that **same FD** back. A mismatch, unexpected state or syscall failure
   latches sticky HOLD while retaining actual FD, original identity and any valid
   latest receipt/generation. No later healthy observation clears HOLD. Caller
   retains the custody record/process; metadata copying does not transfer the FD.
5. Give-back is **only custody transfer**, never permission to close/free/reset or
   evidence of DMA retirement. NONE can follow a prior successful ACK; it does not
   prove never-used, device health or global admission. BIND also does not expose
   another file's reservation, disconnected/quarantined state or alias count.

There is no long-lived **borrowed** provider mode: the FD is borrowed only for
observation until a successful move; successful take transfers sole custody.
Borrowing the bootstrap's live description while claiming independent provider
ownership would be false. `O_RDONLY` itself is **not a read-only capability**:
the examined ISP ioctl handler permits effectful commands on that description.
This helper simply does not issue/expose them; it is not a sandbox.

## Exact real roles and why a full FD-table adoption is not implemented

`N` below is immutable `evidence/delegated/20261012-native-handoff/run34/inputs/`;
`P` is `bringup/qc07f/variants/security75/mach-ak3918ev200/`;
`K` is retained `work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts/`.
These are source facts, not queries of the running RAM76 target.

| Real role | Existing user path | Actual provider/open-description/lifetime requirement |
|---|---|---|
| Frame/capture receipt | `N/rtspd-native-import.c:114–141`, native `continuous_frame`; O_RDONLY | `P/isp.c:215–243` creates a distinct `ak_isp_file`/receipt per open, retained separately from FD numbers. BIND yields source/file; ARM reserves monotonic generation and pins session; BEGIN/NEXT/END require that same owner. Dup/fork shares the same description/receipt; another open gets another file ID. No task/PID check exists in these handlers. Helper accepts only currently NONE, not an ARMED/ACTIVE handoff. |
| Sensor bus | `N/rtspd-native-import.c:112–113`, O_RDWR, `I2C_SLAVE(0x30)`; `sensor_baseline.c:66–104` | `K/drivers/i2c/i2c-dev.c:599–637` allocates a per-description anonymous client and adapter ref. `I2C_SLAVE` changes its address (403–418); dup shares that state. Typed I2C_RDWR messages specify address0x30 but still execute real transfers. Identity/register reads write the register selector first. Sensor preparation writes139 original pairs only after an actual ARMED receipt and watchdog5 proof, then re-QUERY before BEGIN. This is not a harmless FD-readiness probe; no sensor adoption is added. |
| Persistent encoder/reference owner | `N/native_adapter.c:lifetime`, `N/rtspd-native-import.c:190–195`; O_RDWR | `P/qc07f_hwenc.c:445–457` allocates a per-open numeric session. H264 references are bound to the actual `struct file *` (389–407), not FD/PID. Lifetime QUERY (364–373) reports global session_sequence/fault/live-memory state, **not this file's session ID or ownership**. Dup shares the owner; an independent open does not. Current QUERY cannot certify which description owns refs, whether this is the fresh expected open, or sole ownership. |
| Temporary encoder job FD | `N/rtspd-native-import.c:186,196,225–227`; O_RDWR per frame | It is opened even in the STREAM branch, although that branch sends the job on the persistent stream FD. IDR-only uses this temporary FD. It is closed only after successful job handling; uncertainty retains it in `encoder_hold`. Preopening one persistent temp FD, aliasing it with the stream FD, or duplicating the stream changes numeric session/final-release behavior. Do not silently collapse these roles. |
| Capture watchdog bracket | `N/rtspd-native-import.c:107–109,164–166`; O_WRONLY | `K/drivers/watchdog/watchdog_dev.c:864–913`: open is exclusive and **starts hardware**. SETTIMEOUT changes/pings it; GETTIMEOUT only reports the configured value. Successful original bracket writesV then closes; uncertainty retains actual FD. Preopening is already an effect requiring parent admission; no helper may open just to inspect it. |
| Encode watchdog bracket | `N/rtspd-native-import.c:185,230`; O_WRONLY | Same physical single-open watchdog, later nonoverlapping bracket. Both roles cannot be independently preopened. Dup produces one shared open description; closing one alias does not invoke final release, so it is not equivalent to the original V/close/new-open bracket. A continuously held FD changes recovery/stop/ping semantics and needs its own exact lifecycle design, not a descriptor substitution. |
| Raw/hub/encoded backing (not an adoptable FD here) | `N/native_adapter.c:17–28,75–91`; static raw1728000B/hub1MiB/optional producer1MiB | QC_ISP_NEXT and encoder ioctls copy through userspace spans; these are not dma-buf import/export contracts. No FD in the proposed role list attests CPU extents, cache state, placement, paid memory or reference lifetimes. No arbitrary pointer/address cast, large allocation or cap exception is added. Native75 raw-reuse/migration HOLD is not76 adoption or capacity credit. |

### Last close, task binding and uncertainty are real blockers

- `K/fs/file.c:1457–1482` dup gets/installs the **same file**; fork's `dup_fd`
  gets refs on the same file (455–462). Closing one descriptor does not prove
  final `.release`; receipt/device/inode equality cannot detect hidden aliases.
  A userspace move contract does not revoke copies or give per-task kernel access.
- `P/isp.c:1448–1463` final release may attempt STOP with reasonCLOSE, consume
  a retired receipt or retain/quarantine an uncertain session. Final release can
  destroy userspace's query route; it is not a substitute for original explicit
  sensor-off → END → exact QUERY/RETIRED/END → ACK before normal close.
- `K/fs/file_table.c:504–505` discards `.release`'s return value. Thus close0
  cannot prove the driver's stop/reset succeeded. The retained native END/QUERY/
  ACK checks remain indispensable; their earlier success, not close0, is proof.
- Encoder final release (429–443) clears H264 owner and frees per-file session;
  jobs are synchronous-retired or global-quarantined, not reset/retried by close.
  A bootstrap-held duplicate prevents that reference-owner release on an adapter
  close. Current global lifetime QUERY cannot attest sole/final release.
- Watchdog writeV sets shared allow-release and also pings (697–729); release
  (942–989) attempts stop or pings on failure and **always returns0**. The Anyka
  driver stop/set_timeout actually writes hardware (`P/watchdog.c:39–75`).
  No borrowed FD owner may independently disarm/close another recovery bracket.
- `nh_native_release` only relinquishes the exact CPU output loan; it is not DMA
  STOP. `nh_native_retire_held` explicitly retains backing until original normal
  retirement, while `nh_native_quarantine` retains real process/FD/backing on
  uncertainty. This helper owns none of that failed-next pipeline and does not
  replace it with a receipt/metadata sink. **Failed-next remains BLOCKED.**

## Provenance / offline compile result

Native run34 manifest SHA256:
`8286c85c09d7f34d5bb6af13054803f68ebafad4e2e874da4b1a35aaf9e0d529`.
Actual retained receipt header equals the implementation header byte-for-byte,
SHA256 `887e32a3a9201812be3bd882826d2bc88a518b613a11793c3da862a39458dc39`.
Implementation pins checked before building:

- `P/isp.c`: `7d0ef80561df4ef058be1deb6698001c35066fab5533f492dec97d2b447bbd50`
- `P/qc07f_hwenc.c`: `e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f`
- `P/capture-retirement.c`: `22bd93661c0a80613dbfa224b8f1987ceafe0990ba06b1a4727a7431a0d18c54`

The exact I2C/watchdog/VFS source snapshots and all other input/output/compiler/
header pins are in `runs/build01/REPORT.json`. Originals are immutable; only the
new helper is compiled, not patched kernel/native/server sources. Existing SPDX
and GCC/musl notices are retained; no global license grant or private payload use.

```text
python3 -B apps/rtsp-auth/qc07f-provider-candidate/build.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

**PASS11:51:26UTC**;16 ordinary compiler/archive/relocatable-link/ELF commands,
one C translation unit per HOST/ARM.29 source/provenance/license inputs unchanged,
95 compiler-header closure paths retained. No output/test/synthetic campaign,
executable/listener/native action, private IO or target/UART/RPC/status/network/
boot/recorder IO. No final executable or kernel is built; no runtime acceptance.

| Artifact | Bytes | Role |
|---|---:|---|
| `libqc07f-frame-fd-custody-host.a` |3930| HOST helper archive |
| `libqc07f-frame-fd-custody-arm.a` |3172| ARMv5 musl helper archive |
| `host-frame-fd-custody-NOT-EXECUTABLE.o` |3552| ELF REL; not an app |
| `arm-frame-fd-custody-NOT-EXECUTABLE.o` |2724| ELF32 ARM EABI5 REL, entry0/no program headers |

Only the two real custody functions are exported. Undefined dependencies are
ordinary fcntl/fstat/ioctl, stack checking and ARM memset/GOT support—no open,
close, dup, malloc, provider stub or unknown kernel symbol. ARM stack metadata:
observe32B, take264B, give_back240B (static compiler estimates, not measured peak).
Archives/objects are local generated evidence, not source/deployable artifacts.

## Smallest next real patch / full adoption disposition

**Full owned-provider adoption: SOURCE BLOCKED**, not solved by this helper.
Concrete next kernel/source increment: in a **fresh encoder derivative**, add one
versioned, fixed-size **observational per-open ownership query** beside the current
global lifetime query. Under existing `enc.lock`, export the actual numeric
`file->private_data` session identity and whether `enc.h264_owner==file`, alongside
the existing quarantine/reference lifetime facts. No pointers/physical addresses,
new allocation, reset, reference clear, job execution or guessed deployed ioctl
number. Bind the eventual userspace take to that actual observed tuple instead
of interpreting global session_sequence as this FD's identity. Keep current
global lifetime ABI unchanged. That kernel/source increment requires review and
ordinary build only; it is not a selected target operation or an exclusive-alias
proof. Trusted sole-description transfer still remains a precondition.

Separately, before integrating all roles, the adapter must replace both internal
watchdog open/V/close brackets with one explicitly admitted ownership/lifetime
contract preserving5s recovery semantics (not two preopened aliases), and reconcile
temporary-versus-stream encoder lifetimes. No kernel patch can make close0 a DMA
receipt. Real backing/placement under128KiB and40MiB/CMA gates, failed-next retained
process/backing ownership, trusted provisioning/current readiness and owned listener
remain necessary. No main, native2MiB/HOST4MiB substitution, child pin replacement,
loopback policy change, old HOLD clearance or camera/deployment/G3/G8 acceptance.
Further penetration/adversarial/fuzz/mutation tests remain deferred, not passed.
