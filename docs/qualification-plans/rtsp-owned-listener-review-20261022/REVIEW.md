# Owned-listener v1 — independent focused SOURCE/API review

2026-10-22. RP-09 / STREAM-04 / STREAM-07. **Narrow SOURCE/API custody sufficiency;
no correction-required defect identified in the selected listener delta.** This
is not composition, runtime, concurrency, native, product, installation or full-app
acceptance. No next build, integration, runtime test or hardware operation is queued.

Reviewed source: `apps/rtsp-auth/owned-listener-candidate/`, **47 files**, aggregate
`febab8640027f71dfc8a1ec4aa53c26cba219048ca4bf74578050377101ce557`.
Frozen original41-file aggregate:
`2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f`.
Aggregate recipe: SHA256 of sorted workspace-relative UTF-8 path, NUL, raw file
SHA256, concatenated. The source includes41 byte-identical imports and six new
root files; the old internal-bind server is reference-only.

## Scope and evidence limits

Read current supplied AGENTS instructions, current index/execution ledger, baseline
discipline and08 streaming plan. Review is limited to ordinary ownership/API,
lifetime, startup ordering, selected control flow and cleanup regressions. Full
contract/report, public API/custody helper, real server/delta, real unchanged backend
and verifier launch/READY/finish were inspected. No broad parser/security or
architecture audit, penetration/adversarial/fuzz/mutation/runtime test was performed.

Only fresh files in this review directory were written. The other active source
lane was not inspected. No source edits, compiler/build/generator/candidate/helper/
test/guard/listener/app execution, hardware/UART/RPC/status/network/private input,
Git/index or central-plan writes occurred. Python was used only for independent
public file/hash/receipt/archive/ELF metadata collection; it imported no candidate
module and launched no subprocess. Two initial nonexistent-path read lookups are
retained in `COLLECTION_ERRORS.md`; no policy rejection occurred. No claim about
current recorder, deadline expiry or hardware state is derived from collection
timestamps. Last inherited observation remains held original U-Boot12:12UTC,
catch consumed, both HOLDs unchanged; this review made no fresh observation.

## Focused source/API findings

Paths in this section are relative to the candidate unless stated otherwise.

| Boundary | Actual source and finding |
|---|---|
| Real source API | `owned_listener_v1.h:7–36`, `rtsp_server_owned.c:426–460,642–655`: actual caller-owned writable int slot, not an FD-number receipt or detached unused helper. Move-only disposition is explicit. Original serve declaration remains inert, not an implemented alternate entry. Trusted same-process bootstrap is still absent. |
| Original policy | Server`:429–442` keeps exact127.0.0.1, uint16 port>=1024, stop/producer/resource/lifecycle checks,2s sends, one1MiB hub, original bounded options. URI validation and hub metadata initialization now precede backend effects. `framework/src/rtsp_input.c:21–42` and `packet_hub.c:22–27` perform local validation/record initialization, not backing allocation or source startup. No allocation ceiling increase/new AU copy. |
| Borrowed validation | `listener_custody.c:11–32`: fstat/S_ISSOCK; F_GETFL/O_NONBLOCK; F_GETFD/FD_CLOEXEC; SO_TYPE=STREAM, SO_ACCEPTCONN=1, SO_PROTOCOL=TCP, including exact integer lengths; getsockname exact sockaddr_in length/AF_INET/127.0.0.1/options port. Queries only, no SET/socket/open/dup/bind/listen or fallback. Unsupported query support refuses. |
| Real roles/startup | Server`:435–455` excludes numeric collision with occupied image/material roles before startup. `frozen/candidate_auth.c:46–102` still requires its actual graph/policy/entropy/source/retention callbacks, validates distinct real regular image/material descriptions and exact pin/profile, then calls actual rv_start. These are trusted disjoint storage/ownership roles, not authority inferred from fd numbers. |
| Commit | Helper`:35–47` compares the actual caller slot to initial numeric fd, repeats every socket query after startup, compares fstat dev/inode, then assigns private owned fd, caller=-1 and consumed=1. No callback/effect intervenes between final observation/comparison and stores. Backend success is real READY plus engine ACK, not image validity or a caller Boolean. |
| Precommit failures | Server`:429–455`: all invalid option/lifecycle/hub/FD, clock, real backend-init and final requery returns precede the move. Helper refuses without writing the actual caller slot. Wrapper initializes its disposition separately. Backend cleanup may retain UNKNOWN verifier ownership; unchanged caller listener ownership does **not** establish retirement of a transient child reference. |
| Actual moved path | Server`:453–480`: private `listener` alone supplies fds[0], poll and accept. The exact diff removes the complete original internal socket/SO_REUSEADDR/bind/listen/F_SETFL sequence. Remaining peer setsockopt/close and verifier socketpair/fcntl/dup2 operations are not selected-listener configuration or internal fallback. |
| Postcommit paths | Server`:456–639`: no early return bypasses final cleanup after move. Normal stop, clock/backend/source/peer failure and poll exit reach original client/hub/source cleanup, guard unbind, one listener close attempt and actual backend finish. Every returned postcommit outcome keeps consumed=1. Closing the listener never substitutes for source/DMA/peer/verifier retirement. |
| Close exactly once | Helper`:50–56`, server`:625–639`: private owned slot is set=-1 and close_attempted=1 **before** close. errno is copied immediately on failure; no retry, second close or re-use of error FD. Final result cannot be OK when close failed; an already non-OK finish result, including UNKNOWN, is preserved. No promise the original description survives a close error. |
| Reentry/outcome | Server`:642–655`: NULL outcome returns INVALID without touching storage. A nested non-NULL call returns BUSY before initializing/touching any outcome, including the active outer outcome. This matches report`:22`; it is not a successful nested ownership operation. Active flag spans initialization, notifications, unbind, close and verifier finish; it is cleared only after final result publication. It is a serialized synchronous reentry guard, **not** a lock/thread/signal/hostile-code guarantee. |
| Sticky clock | Server`:35–38,72–75,619,642–655` never resets the original static server clock. `framework/include/camera/rtsp_clock.h:7–22` retains sticky failure and no realtime fallback. Entry guard does not reset clock/source uncertainty to enable a new session. |

### Child inheritance and readiness: actual code, not CLOEXEC folklore

`frozen/verifier_adapter_overlay.c:164–225` duplicates image/material/child IPC at
>=16, makes one fork, acquires its owned pidfd and uses the existing release
barrier. The still-borrowed preopened listener **is inherited transiently** before
commit. CLOEXEC is not fork isolation. Occupied listener and image/material roles
cannot alias under the documented exact-role prerequisites; fresh allocations do
not replace an occupied parent listener slot.

Inspected `launch:105–121` has no listener poll/accept or caller callback. After
release it overwrites5/3/4 with actual image/IPC/material, requires close_range>=6,
closes0/1/2 and execveat; successful READY is after this path, not before. The
original listener's CLOEXEC status is also required. `rv_start:211–222` validates
actual READY flags15/profile/2MiB-versus4MiB field and then a separately bound
engine ACK. `candidate_backend_init:101–102` does not publish success earlier.
The snapshotted unchanged `isolated-verifier/child.c:23–40` sends READY after real
hardening, and engine ACK only after actual material/entropy/engine initialization.
Original native2MiB and separate HOST4MiB hardening/pins remain unchanged.

This only supports the documented inspected startup transient. On failed/UNKNOWN
startup do not assert child listener references retired: original
`candidate_backend_destroy/finish:105–117`, `rv_stop:287–306` and mandatory
retain_verifier ownership remain. No general provider-fork clearance, native
provider safety, external retention or adoption of the disjoint backend lane
follows. No unsupported pidfd/close_range/exec fallback was added.

### Guard lifetime and unchanged uncertainty

`capture_gate` and client/backend/options/lifecycle references remain stack/live
through all synchronous producer/lifecycle callbacks and final
`source_guard_bind(context,NULL,NULL)` (`server:459–460,606–638`). Earlier client
close, hub stop, failed-retirement marking, final source retire and sticky-clock
behavior are unchanged in the exact diff. Unbind moves ahead of listener close
and logging, **not** ahead of source/client cleanup. No server use of gate occurs
after unbind; backend finish uses no capture_gate pointer. The independently read
reference glue `apps/rtsp-auth/qc07f-runtime-candidate/rtsp_native_source.c:24–30`
clears both saved function/context on unbind; its synchronous uses`:32–48` cannot
call a cleared saved pointer. This is reference inspection, not integration of
that provider into this archive or proof about an absent bootstrap's callback.

API preconditions must prohibit retaining/using the gate after unbind or serve
return. Original sinks copy ownership metadata only and must arrange actual
external owner/backing/code lifetime. `server:139–156,538,552,584,606–622` retains
the original uncertain-peer/frame/failed-retirement behavior; listener consumed/
close fields neither hide UNKNOWN verifier finish nor make those partial-work
paths quiescent. Missing failed-NEXT/partial-work external retention and backing
are **not repaired or waived** by this review.

### Preconditions are not permissions

Contract`:5–28,54–68,70–85`, report`:15–37,104–126`, API header`:7–28` are
consistent at this narrow source scope. Actual caller slot/outcome/options/config/
graph/URI/parameter sets/producer/lifecycle/hub/backing/callback code must remain
stable, live, disjoint, trusted and serialized for the entire call/notifications.
Queries do not prove no duplicate descriptions, SCM_RIGHTS aliases, concurrent
FD-table mutation, source authority or endpoint/provisioning policy. All aliases,
signals and threads require exact owner/source control; callbacks cannot mutate
these records or operate on listener ownership. Early refusals and unsupported
nested calls are not thread-safe APIs. No metadata grants authority and no ready-
success/fake callback substitute is provided. Review establishes conditional
SOURCE/API reasoning, not that a real caller presently satisfies those conditions.

## Independent build-receipt / archive inspection

Author build01 **52 recorded HOST/ARM commands,30 translation-unit compilations,
all exit0**,15:14:45.777413–15:14:55.491248UTC, is cited and statically verified,
**not rerun**. `STATIC_before01.json` / `STATIC_after01.json` retain exact command,
log, input and artifact pins plus independent archive/REL parsing. Collector:

```text
python3 -B docs/qualification-plans/rtsp-owned-listener-review-20261022/collect_static.py before01
python3 -B docs/qualification-plans/rtsp-owned-listener-review-20261022/collect_static.py after01
```

- Verified author FINAL_SEAL's642 files and evidence aggregate,47 source inventory,
  original41-file before/after maps and each imported body, exact regenerated
  SERVER_DELTA.patch, report/contract/builder/source and original notices.
- Verified441 explicit public inputs **and each stored snapshot**,332 prior closure
  entries and293 fresh closure entries; rebuilt the latter map from all30 actual
  dependency files. These maps overlap (author reverified union695); do not sum.
- Reconstructed all30 compile argv from the prior build02 command receipt plus
  the exact source/include/output relocation. Each platform compiles actual changed
  server/custody plus13 unchanged real dependencies, **not** the frozen server.
  All original strict flags remain; no public-fixture/test macros, fixture or
  synthetic source is compiled. HOST here is a compilation architecture, not a
  claim to have exercised the separate public HOST4MiB verifier profile.
- HOST GCC16.2.1 and retained ARMv5 musl GCC14.3.0 version/target/find-tool logs,
  source/header/compiler/wrapper/runtime/compiler-subprogram/binutils pins match.
  Source recipe invokes no old builder: only literal AST pin extraction and prior
  JSON receipt reading. The52 recorded commands contain compiler/archive/ELF tools
  only. No guarded runtime-builder invocation appears in this recipe/receipt;
  this is not a process-audit assertion about unobserved activity.
- Parsed actual archive bytes without extraction/execution:15 unique members per
  platform match each recorded compiled object body exactly. Parsed actual ELF
  headers/sections/symbols/relocations for all30 members and both linked RELs.
  RELs are x86-64/ARM EABI5 respectively, entry0, no program headers, real owned
  serve/auth/verifier/helper linkage, no original serve or main export. Actual
  relocated undefined calls contain only libc/libgcc dependencies, no unresolved
  project calls. Unreferenced direct-Digest symbol-table leftovers from discarded
  sections match author reporting, not credential stubs. Residual fork/dup2/
  socketpair/peer setsockopt are real unchanged paths, not listener fallback.
- GPL SPDX, original MbedTLS3.6.7 source/license pin
  `bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5`,
  and runtime GCC/musl/MbedTLS notices remain present/byte-identical. No license
  removal or global license grant.

| Artifact | Bytes | SHA256 |
|---|---:|---|
| HOST archive |174568| `1fdd6a4c0c9f01367654738a0a0233aa93ee46ae6b109dcb739f78e59180fe4a` |
| HOST REL |119904| `c11019da664f8c87697da5f86b84ec00a4d1cff5ecad78d46637249941d2a4e5` |
| ARM archive |119668| `f6f3038665d8702ee6d54cc56d6168ab9341de28ab2b255b971708f190a986ca` |
| ARM REL |77996| `64fadbfc041fdc87da23c1f05152309fe8967b356b9bbbfb9474909ede833e0c` |

The two independent collections cover1,281 unique reader inputs each; final seal
verifies equality of before/after input maps and a final fresh rehash. Own output
pins and all reader input pins are frozen in `FINAL_SEAL.json`. This is a hash
evidence seal, not a signed attestation or runtime proof.

## Disposition / next boundary

No minimum corrective revision is required for this exact bounded listener
custody delta under its stated trusted serialized SOURCE/API preconditions.
Keep the candidate and original sources frozen; parent may disposition this
review separately. Do not combine/link with another server/backend implementation
that defines the same dependency symbols, or infer composition from separate lane
results. Ordinary functional/native qualification is deferred, not marked passed.

Real bootstrap/endpoint admission, provisioning, actual source-ready/entropy/owner
callbacks, backend composition without an unsafe live-provider fork, external
failed-NEXT/partial-work/process retention, backing/code/namespace ownership,
admitted memory fit, native paired platform/provider, release packaging, full app,
G3 and G8 remain open. Query77 has never booted and its embedded station pairing
remains unsupported. Both original HOLDs stay untouched. There is **no automatic
build/test/integration/native queue** from this result.
