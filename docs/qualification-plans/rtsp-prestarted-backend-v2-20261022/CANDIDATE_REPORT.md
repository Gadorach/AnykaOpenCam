# Prestarted backend v2 — R1 correction candidate

2026-10-22. RP-08 / CORE-03–04 / STREAM-04/07 prerequisite only.
**Delivered: exactly-once actual terminal IPC disposition with sticky close error
and retained diagnostic custody. HOST/ARM component compile/archive/REL PASS; NOT
RUN. Independent review required; no SOURCE/native/full-app GO.**

## Defect, contract and frozen baseline

Full independent v1 review was read first. R1 is an ordinary source-reachable
custody defect, **not an observed runtime failure**: actual readiness observer
reaps the child, then START_FAILED explicit finish reaches already-retired stop/
destroy, which in v1 freed the client without closing its parent IPC description.
A final error status or known CPU exit did not dispose that actual FD.

`R1_CONTRACT.md` was written before code/copies. Unchanged SHA256:
`a8d15bb65ec1aebaf1be1d588d498484b36bc6cca0fe02df67cdbce42346db72`.
It limits the revision to terminal IPC custody, clear-before-one-close, retained
sticky errors, actual exit versus close outcome and no uncertain-start retry.

| Frozen input | Exact pin / preservation |
|---|---|
| v1 authored11-file source | `867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90`; byte-copied to baseline-v1-app/ before editing |
| v1 SOURCE_SEAL01 | `9efc3a84ab6342e9ad789ac1b6448358ea8798cde0339726b78fb47dec3a23c9`; all11 source/592 evidence pins verified |
| independent R1 review FINAL_SEAL | `d060aaa15deea1526920414f4015505f1024fa73e2f49eff6a502a90049ce323`; all1,303 reader input/six output pins verified |
| provenance copies | v1 source/build seals, report/contract and original static-finalizer failure receipt; independent REVIEW/FINAL_SEAL byte-preserved in baseline-provenance/ |

The recorded maps overlap. Build verifies1,328 unique frozen entries including
baseline/provenance copies; they are not additional independent histories. This
includes original retention91-input/five-output and original isolated-server
custody from v1/review. No original v1/build/review/retention/source was edited or
rerun. Owned-listener accepted source/review remain untouched, not inspected or
composed for this correction. No claim of a new listener review or composition.

Only fresh `apps/rtsp-auth/prestarted-backend-v2/` and this qualification directory
were written. All11 v1 app files were copied; seven are byte-unchanged. Production
delta is only verifier_adapter_overlay.c and adapter_state.h. README/builder are
the two metadata/recipe changes. Auth C/H, actual server C, all three pin headers
and server crypto configuration remain exact. Total authored tree11 files,
1,681 lines/102,455B. Source aggregate (sorted workspace-relative UTF-8 path + NUL +
raw SHA256 bytes):
`5a42cdc9d76628031c5380edd2e3b2c4dd06aefc6675138c8e10e1b098a28868`.
Original GPL SPDX, runtime licenses and provenance remain intact; no relicensing.

## Actual R1 source correction

Paths below are under `apps/rtsp-auth/prestarted-backend-v2/`.

### One actual owned IPC slot; former number never owned again

`verifier_adapter_overlay.c:25–45` adds three fields to the private actual client:
ipc_close_attempted, ipc_close_last_not_owned, ipc_close_error. calloc initializes
attempt/error history to zero. The scalar observer reports former=-1 when no
attempt occurred, not the uninitialized meaning of a numeric zero field.

The sole helper, `ipc_close_once`, first returns any stored outcome for an already
attempted close. Otherwise no owned FD needs no close. For an actual nonnegative
c->fd it saves its numeric diagnostic, **clears c->fd=-1 BEFORE close**, records
attempt/former and attempts close exactly once. The first errno is saved positive
(EIO if absent) and c->failed remains sticky. Former numbers are not aliases or
actual descriptions and are never retried/used for I/O/closed again. This includes
close errors/EINTR: fd=-1 must not be reclassified as later success.

After error the allocated client is retained for history/namespace, but it does
**not** claim the former numeric IPC FD is still actually owned. No dup, new open,
FD stealing, arbitrary PID, new signaling or wait is introduced by the helper.

### Real paths and terminal results — static trace, NOT execution

| Existing real path | v2 actual disposition |
|---|---|
| Observer/earlier adapter reap -> retired stop | Reap/observer bodies are unchanged and preserve actual wait/wipe/retired/child0/pidfd facts. rv_stop:304 reaches ipc_close_once immediately: no clock/signal/poll/wait on consumed child. It closes the still-owned IPC or reports its stored outcome. |
| Normal rv_stop loop -> new actual reap | At :309 successful owned reap reaches the same helper. Original signal/poll/attempt/time budgets and all error/unknown-wait branches stay exact. |
| Startup failure before release | At :239 original barrier IPC close is replaced by helper. Its result is intentionally not substituted for actual child retirement: original rv_stop still runs for that real child. Once exit is observed, helper returns any earlier sticky close error, so startup cannot hide it via fd=-1. |
| Close success -> original backend destruction | First stop returns OK with actual child retired and IPC disposition successful. Backend still checks actual wipe/own wait0. rv_destroy's unchanged body calls stop again: retired helper returns stored OK without second close/signal/wait, then original wipe/free/null is permitted. Final start/wipe/wait errors remain non-OK as in v1. |
| Close failure -> original backend destruction | Stop returns RV_UNCERTAIN, c->fd=-1 and explicit error/history stay in actual client. Backend's unchanged :107–116 destructor refuses wipe/free, invokes mandatory retain_verifier on actual client/config and returns non-OK. Unchanged explicit finish :242–258 sees remaining client and enters HOLD, not FINISHED. |
| Later stop/destroy after failed close | Already-retired stop returns the same stored non-OK outcome, no new close/signal/wait. rv_destroy refuses to wipe/free/null the record. fd=-1 cannot erase the error. Outer HOLD still forbids a second finish; this describes sticky internal behavior, not permission to retry. |
| Actual unknown startup stop | Unchanged rv_start returns RV_UNCERTAIN; unchanged candidate_prestarted_start:227–234 records it and enters original HOLD/sink. No new finish/retry route, even if child later exits. A close-failed but actually reaped startup can now correctly retain that same non-OK status with known exit plus failed IPC outcome. |

`rv_retired` at :322 is still actual known CPU-exit evidence, not IPC disposition.
The additive `is_rv_ipc_close_outcome` at :338–345 / adapter_state.h:17–23 reports
only attempt, former-NOT-owned and saved errno. It returns RV_UNCERTAIN on saved
close failure even when the child is actually retired. No mutation/clock/syscall/
payload query or retirement grant; trusted caller output must be stable/disjoint.
An unattempted snapshot's OK means successful metadata observation only, never
closed IPC/child retirement. Known child exit and failed IPC cleanup are therefore
distinct retained facts, not a fabricated unknown wait or still-owned former FD.

This is not a broad FD cleanup rewrite. Unpublished startup constructor/fork-failure
resource handling, ancillary-FD disposal and pidfd cleanup are outside R1 and
unchanged; no new sufficiency claim is made for those paths. Normal verifier-only
shutdown signaling remains the existing implementation, not native/media authority.

## Preserved behavior / open boundaries

Actual reap/observer/wait, barrier/child payload/pins/filter, signal and policy paths
remain unchanged except the explicit IPC-close substitutions above. rv_destroy's
body is byte-identical. The backend start/serve/finish/destructor and actual stable
peer/config/one-shot ownership are byte-identical to v1. No serve init/fork/hidden
finish, ready-Boolean grant, copied backend, fake RNG or device preparation is added.

Real start still requires device-free trusted bootstrap ordering, stable immutable
config/graph/context address/callbacks and genuine auth material/RNG. Those facts
are not proved by fstat, this source API or a pidfd. One source-only backend owner
is not an external media process owner or installed bootstrap/recovery policy.
Unexpected OOM/process exit retains no userspace FD table and cannot be called safe.

Strict native2MiB/HOST4MiB, loopback/internal listener, exact auth/policy/source
guard/checks/budgets,128KiB/40MiB/CMA and native10s/<2s/5s stay unchanged. No native
failed-NEXT retirement/retention/backing fix, regular uncertain native shutdown,
source glue/provider adoption, listener composition, provisioning/main, new
framework/media RPC/supervisor, feed/rearm/reset/END/ACK/native kill/resume or memory
credit/exclusion. Native/bootstrap/recovery/backing/product gates remain BLOCKED.

## Compile-only record / artifact custody

```sh
python3 -B apps/rtsp-auth/prestarted-backend-v2/build.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Build01 **15:51:31.068143–15:51:35.427033UTC PASS**:26 compiler/tool/archive/REL/ELF
commands returned0; four separate ordinary delta commands returned expected1
(30 records total). Six fresh real C compilations: changed adapter plus auth and
server consumers of changed adapter_state.h per HOST/ARM. No object reuse or
unchanged framework/crypto/UAPI suite builds. All six compile logs are empty.

HOST GCC16.2.1/x86_64-pc-linux-gnu; retained pinned ARM GCC14.3.0/ARMv5TE/soft-float.
Same warnings/Werror/FORTIFY/stack protection; only inherited compact server has
the existing misleading-indentation exception. Strict/default backend source
class, no fixture/test macro, native/HOST-budget/pin substitution or new child.

REPORT.json retains204 input paths,282 actual consumed source/header paths with
byte copies,1,328 frozen pins, exact commands/cwd/time/logs/environment, actual
cc/cc1/as/ld/ar/nm/readelf identity plus four retained ARM closure pins and vendor
277-file aggregate. The two existing recorded compiler symlinks are verified by
unchanged target bytes/resolution, not treated as new source symlink authority.
All recorded inputs/frozen/closure pins are rechecked after build. No original
builder/guard/test/helper script, fixture/provisioning generator or output is run.

| Component artifact | Bytes | SHA256 |
|---|---:|---|
| HOST archive |89914| `4b76d28ba280c5be6600c7d818fe81f792a6ad38797f6e25268ca082501b0e74` |
| HOST REL |85368| `70d0c7af98ab779f453608571a45110088ac11c3e59d702ebbe7680a44548cdf` |
| ARM archive |58640| `6345b7b8995f16820005e8756cc5d307381209865c9b8f0c498f8bc866d572c4` |
| ARM REL |54664| `90527c7f983c1d01e3a7b7af819c35a3828feb97773cda28c400627b9c01d565` |

Fresh unchanged-body auth/server object bytes happen to match v1, but are actual
new compilations, not reused evidence. Archives/REL contain only these three
components and leave normal framework/IPC/MbedTLS/libc/libgcc dependencies unresolved;
no stubs/final executable/full-app/native link or memory-fit/ABI-pairing claim.
Static finalizer/seals verify bytes/receipts/scope only; no runtime acceptance.
V1's original static-finalizer failure remains preserved; this v2 increment had
no build failure, policy rejection/retry/reroute or runtime result.

## Functional confirmation — deferred / NOT RUN

- [ ] Observed-exit START_FAILED finish closes actual IPC exactly once.
- [ ] Normal/newly-reaped verifier retirement has the same IPC disposition.
- [ ] Close-error history persists through later internal stop/destroy observations;
  actual retired receipt stays distinct, former number never reused, client retained.
- [ ] Pre-release partial-start error is retained without new uncertain-start retry.
- [ ] Normal real wipe/wait0/explicit finish, unchanged real auth/source operation.

None is a test queue. No runtime/ordinary functional/adversarial/penetration/fuzz/
mutation/helper/listener execution, UART/RPC/status/network/private/target IO,
Git/index/central-plan/AGENTS/P1/model/provider/safeguard work, kernel/module/image/
installation or automatic next work. Parent alone hardware. Historical last
original held U-Boot12:12UTC is not fresh state/receiver/recovery admission; both
original radio/reset HOLDs remain untouched. Accepted listener source/review and
all frozen originals stay separate; no source/native/full-app GO.

**One bounded handback:** independent ordinary review of this exact R1 correction,
sticky close/error/retained-client branch and compile custody, then parent
disposition. No automatic reviewer, composition, build/test or native queue.
