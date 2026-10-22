# RP-08 prestarted real backend — bounded SOURCE/compile candidate

2026-10-22. **Delivered a real device-free start / actual serve / explicit finish
split. HOST/ARM changed-unit compilation/archive/ELF REL PASS; NOT RUN.** Independent
source review/parent disposition is required. This is not external media retention,
an installed bootstrap/recovery policy, a composed listener API or native/app GO.

## Selection, provenance and contract first

Only fresh `apps/rtsp-auth/prestarted-backend-candidate/` and this qualification
directory were written. The previous retention block's91 selected input pins and
five output pins remain unchanged. Its early receiver-expiry wording was inherited
parent wording, not a fresh hardware fact; no current runtime/receiver/recovery
admission is inferred here. Parent alone hardware; no target query was performed.

`OWNERSHIP_CONTRACT.md` was written before copying/editing candidate code. Its
unchanged SHA256 is
`a9874ddf3f46a6019e5bfc66722650d1dc6c908b25eb4fe213ebc8d609d9c299`.
It defines device-free trusted ordering, stable storage/config/contexts, one-shot
partial-start/serve/finish/HOLD behavior and explicitly unsupported native cleanup.

The sole source baseline is frozen `apps/rtsp-auth/isolated-server-host-tail/`,
41-file aggregate
`2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f`.
All41 are byte-preserved under `baseline/` with original provenance/SPDX/runtime
licenses. Nine selected files were copied to the app, five changed and four
unchanged. New README and compile-only builder bring the authored tree to11 files,
1,639 lines/100,112B. Exact authored-source aggregate (sorted workspace-relative
UTF-8 path + NUL + raw SHA256) is
`867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90`.
No prior native/glue/provider/watchdog/query/build/review source was adopted or
regenerated. Frozen originals, public framework contracts and guard scripts stay
untouched. This new sibling has a deliberate source-API difference: it exports no
old monolithic serve or raw backend init/destroy/finish entry.

## Actual ownership and call graph

Paths below refer to `apps/rtsp-auth/prestarted-backend-candidate/` unless stated.

| Boundary | Implemented source behavior / ownership |
|---|---|
| Explicit caller object | `candidate_auth.h:57–97` defines source-only macro-initialized `candidate_prestarted`, containing one actual backend and four stable actual peer records, self-address/config/history/phase. No allocation, native owner, generic reference system or plugin ABI. Fields are private-by-source-contract, not attacker-memory protection. |
| Device-free start | `candidate_auth.c:215–240` claims the process/object once BEFORE private original init can memset. Real rv_clock -> original backend validation/RNG/rate setup -> ONE actual rv_start. All native/provider/guardian/watchdog descriptions, media leases/backing/session/userdata must still be absent; metadata cannot prove this trusted bootstrap requirement. |
| Unchanged actual startup | `candidate_auth.c:41–105` retains root-vs-HOST class, exact image/material/routing/domain/generation, real RNG and limits; only records actual rv_start result. `verifier_adapter_overlay.c:164–226` still performs original fork/pidfd/release/READY15/profile/provision ACK and failed-start stop. No alternate image, dummy config, synthetic RNG or caller-qualified grant. |
| Partial-start uncertainty | Actual RV_UNCERTAIN is retained in backend.startup_result instead of disappearing in CAMERA_INVALID. Start enters terminal HOLD and calls the mandatory real retain_verifier sink on the actual child. It never routes that already-attempted uncertain retirement through another finish/stop. Other failed starts stay START_FAILED with any real client in the stable owner until one explicit finish. |
| Current owned-child observation | Additive `is_rv_running`, `verifier_adapter_overlay.c:316–322`, uses the existing sole-owned reap/actual wait receipt; rejects failed/retired/revoking/unacknowledged/no-pidfd state and interrupted wait. No signal/protocol/filter/child change, no arbitrary PID. This is a point-in-time observation, not future liveness, media permission or retention proof. Original per-effect live policy exchanges remain mandatory. |
| Explicit prestarted serve | `rtsp_server_candidate.c:632–652` consumes READY -> SERVING once, verifies actual backend/config association, initialized/nonfailed/nonrevoked/unoccupied state, real READY15/profile and owned-child observation. It invokes the actual server body at :424–630, never a fixture. No init/rv_start/fork/exec/hidden backend finish occurs in this entry/body. |
| Actual backend use | Body :426–427 takes pointers to the embedded backend and original config, not copied/stack backend. Entry :434 performs actual backend polling. Gate :443 and client :477 receive that same backend; all existing request/effect/CHECK/rate paths and scalar stats now refer to it. No state/Boolean substitutes for any auth/source operation. |
| Stable actual auth records | `rtsp_client.auth` is a pointer (:33). Before the first peer OPEN, :477 binds it to owner.peers[index]. Every auth/rate/pending-peer call uses that actual record throughout service; no stack peer is copied/relabelled on failure. Backend peers and pending_peer therefore point only to caller-owned stable records after serve returns, even when pending crypto prevents peer close from clearing its record. |
| Serve-stack scope | Hub, network clients and capture gate remain stack-local. Existing source guard bind/unbind remains: socket-creation failure unbinds at :446; listener_done unbinds at :622. No other post-bind path returns without unbinding; nonreturning callbacks keep the stack by not returning. Caller/bridge/config stay stable; no async/reentrant callback may retain those stack pointers. Source-retention sinks retain their original copy-only contract, not a new lifetime guarantee. |
| Conservative served status | Final body :627–628 latches observed clock/common-producer nonquiescence; outer entry records result and pending/failed/revoked/peer/source state. Every serve error, even an early invalid-option refusal, is terminal HOLD; no implicit destroy or second serve. Only clean result with no unresolved records is SERVED. Such clean common state is not independent native/DMA proof. |
| Explicit verifier finish | `candidate_auth.c:242–258` accepts only READY, START_FAILED or clean SERVED; rejects pending/peer/source work and serving/terminal/repeated calls. It marks FINISHING BEFORE one private backend destructor attempt. Original stop -> actual retired/wipe/wait0 -> rv_destroy remains. The already-retired rv_stop inside rv_destroy has no second signal effect. |
| Finish result / HOLD | Actual remaining child after destructor refusal keeps backend/config/client/namespace alive on terminal HOLD; mandatory retain_verifier is the unchanged original destructor sink. Known retirement wipes/frees only original backend/client and leaves outer owner FINISHED with recorded results, not reusable EMPTY. Return preserves start/serve/destructor error, never converts failed startup into success. No repeated finish/stop on HOLD. |

Public config identity/object, graph/routes, context ADDRESS and callback addresses
must remain immutable/stable through the entire owner lifetime. Real startup uses
the original device-free RNG/material/authority. Only after the final fork may an
independently admitted caller prepare native userdata *inside that same stable
context*, without replacing config/callbacks or returning fake readiness. No such
bootstrap, native preparation or source_ready implementation is supplied/executed.
All objects/calls/FD-table access remain serialized, nonreentrant and disjoint.

This object actually owns the verifier backend/client/namespace relationship; it
does **not** own a second media process, protect an exited process's FD table or
make OOM/exit impossible. A caller must retain the entire owner/config and process
on HOLD, not just copy the child pointer. Actual callback sinks and independent
external B/O/M/R lifetimes are still unavailable for a camera application.

## Preserved limits and explicit source/native block

Original internal socket/bind/listen/accept and loopback127.0.0.1, one1MiB hub,
2s AU budget and all auth/source option checks remain. No listener descriptor
adoption is added. The separate owned-listener sibling/active review was not read,
polled or changed. Both siblings modify the same real entry/header in separate
trees; later parent-selected composition and review will be required. **These are
disjoint source pieces, not a composite.**

Original real verifier IMAGE pin, startup class, READY15/profile, IPC, sole pending
slot, exact policy CHECKs, source guard and native2MiB/HOST4MiB stay unchanged.
`rv_start`, `rv_stop`, `rv_destroy`, release barrier, pidfd signaling and actual
wait receipt bodies are unchanged. Only the scoped wait observer is appended.
Framework/source producer semantics, source release/frame END/retire/retention
callbacks are not redesigned. Original server cleanup still exists.

**Native failed-NEXT/internal-retirement incompatibility remains BLOCKED.** Holding
this backend after serve error does not undo earlier source cleanup or preserve an
already-returned server stack. No native source glue is adopted, no nonreturning
native retention integration/recovery is installed and no native destructor/END/
ACK/reset/kill/resume/feed permission is inferred. Regular shutdown with uncertain
native owners is unsupported. Healthy pre-memory UNKNOWN watchdog handback stays
its separate narrow source API, not a disable/armed-recovery proof. Original native
10s/<2s/5s,128KiB/40MiB/CMA/backing HOLD remain untouched, no memory credit/exclusion.

## Compile-only evidence

```sh
python3 -B apps/rtsp-auth/prestarted-backend-candidate/build.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Build01 ran **15:24:31.139666–15:24:34.652420UTC**.26 compiler/tool/archive/REL/ELF
commands returned0; five separate ordinary static delta commands returned1 for
their expected differences (31 recorded commands total, not31 compilation tests).
Six real changed-unit compilations: candidate_auth, rtsp_server_candidate,
verifier_adapter_overlay on HOST and ARM. All six compile logs are empty. No
unchanged framework/IPC/UAPI/test source was compiled and no object was reused.

HOST GCC16.2.1/x86_64-pc-linux-gnu; retained pinned ARM GCC14.3.0/ARMv5TE/soft-float.
Original warnings/Werror/FORTIFY/stack protection retained; only compact legacy
server keeps its existing misleading-indentation exception. Both builds use the
strict/default backend class, not an invented HOST4MiB-to-native fallback. Sources
preserve the separate HOST-only branch/pins; no fixture/payload generator is built.

`runs/build01/REPORT.json` records200 input paths,282 consumed source/header paths
and168 frozen selected pins (overlapping sources, not additive histories), all
commands/logs/environment, actual cc/cc1/as/ld/ar/nm/readelf identities, four retained
ARM closure pins, copied dependency bytes, baseline/deltas, source before/after and
artifacts. Vendor277-file pinned tree is only read for retained header/license
closure; no crypto source rebuild/child pin update. Runtime notices are retained.

| Component artifact | Bytes | SHA256 |
|---|---:|---|
| HOST archive |89078| `51967f98676f9de3924439c45a37775b0c5b4820dc22bcc3be7a87d8620f453d` |
| HOST REL |84568| `be6208b6e891cc06a81b5687b33eef146dc4663dc2cd6e23b306e67e9db78c4a` |
| ARM archive |58136| `ec4bca864ac117c94f91e51bd7b8d42faa1e4346c0580a0813fa8c9219d8285f` |
| ARM REL |54200| `74a556b40e8b00f9e4118a47003e1e9557ba8eb26cff503beedbe3b49f821bc5` |

These contain only the three changed real units. Existing framework, IPC, Mbed TLS,
libc/libgcc symbols intentionally remain unresolved and are inventoried. No stubs,
final executable/full app link or native memory-fit/driver pairing is claimed.
Symbol inventories show actual new start/serve/finish and original verifier
operations, no exported old monolithic serve/raw backend init/destroy/finish.
No output was run. Initial static finalization stopped on a bookkeeping mismatch:
the new README/builder shared original basenames and were mistakenly counted as
changed imports. `SEAL_FAILURE01.json` and `seal-initial01.py` retain the failure.
Only that metadata classification was corrected; authored/compiled source and
build01 stayed unchanged, no repeat compile. `SOURCE_SEAL01.json`/`BUILD_SEAL01.json` seal inputs/source and
static metadata/artifacts only; no functional or independent acceptance.

## Functional checklist — NOT RUN, no automatic queue

- [ ] Real installed device-free bootstrap and correct all-forks-before-native order.
- [ ] Actual pinned verifier startup/READY/provision with genuine private authority.
- [ ] Later native preparation in unchanged stable config context, real source guard.
- [ ] One actual backend/child throughout serving; actual peer/pending lifetime.
- [ ] Normal clean serve return and one owned verifier finish/wipe/exit0.
- [ ] Partial-start/serve/finish HOLD with actual stable owner/process custody.
- [ ] Native nonreturning failed-work lifetime/recovery and admitted backing.
- [ ] Explicit sibling composition and all native/runtime/product gates.

No runtime, ordinary functional, penetration/adversarial/fuzz/mutation tests,
candidate app/verifier/listener execution, generator payload, source readiness
callback execution, hardware/UART/RPC/status/network/private IO, Git/index/central
plan/AGENTS/P1 work or model/provider/safeguard configuration change. No policy
rejection/retry/reroute or failed build occurred; the ordinary static-finalizer
correction above is distinct from a tool/policy rejection. No kernel/module/image/install,
native source adoption, media RPC/supervisor/main or automatic next selection.
Last reported original held U-Boot12:12UTC remains historical; no current target/
owner/recovery is inherited, and both original radio/reset HOLDs stay untouched.

**One bounded handback:** independent ordinary SOURCE/ownership review of this
exact candidate and its compile-only custody, then parent disposition. No automatic
review worker, composition, additional implementation/build/test or native queue.
