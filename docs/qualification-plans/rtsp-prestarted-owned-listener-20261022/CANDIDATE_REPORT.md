# Prestarted backend + owned listener — one SOURCE composition candidate

2026-10-22. RP-08/09, CORE-03/04, STREAM-04/07 prerequisite only.
**Delivered: actual one-backend/child + mandatory caller-owned listener source
composition. HOST/ARM compile/archive/ELF REL PASS; NOT RUN. Unreviewed; no
composition/native/whole-backend/deployable-app acceptance or next queue.**

## Source identity and provenance

`apps/rtsp-auth/prestarted-owned-listener/`:14 files,1897 lines,115799 bytes.
Source aggregate:
`09325264c224a43b5664b48123bce1d05ea1269e6f209dcd2c9967424fdcba33`.
Recipe: SHA256 of concatenated sorted workspace-relative UTF-8 path + NUL + raw
file SHA256 bytes. See `SOURCE_SEAL01.json` for individual source/evidence pins,
`BUILD_SEAL01.json` for static build custody and `METHOD.md` for exact commands.

Pre-code `COMPOSITION_CONTRACT.md`:
`07d271826d37aedaf0422878537764f78edc61a3ecfabba2b81ad6debb7ad3b9`.
Actual baselines retained here, without changing any old source/review/history:

| Input | Exact pin |
|---|---|
| Backend v2 source11 | `5a42cdc9d76628031c5380edd2e3b2c4dd06aefc6675138c8e10e1b098a28868` |
| Backend v2 source seal | `2bfa06a6b239c0c62b1003df6ee0f0ef9dd17ef0088efad00cccaf1ada7e7a8a` |
| Independent narrow R1 review seal | `b3310e3a85cc7295137fa46e3aca4aae8fb12653b7377a55ad9d433a42532292` |
| Owned-listener source47 | `febab8640027f71dfc8a1ec4aa53c26cba219048ca4bf74578050377101ce557` |
| Listener author seal | `835aeb64c0967dfe8cdef1d13d3e376a984bf3485bc09b882812b2a9be814ed0` |
| Independent narrow listener review seal | `2f39d960ba6cff2b236d215d65dea206f0005fad0fc430a621e24de25bd3db61` |

Seven v2 files are unchanged: `candidate_auth.c`, `verifier_adapter_overlay.c`,
`adapter_state.h`, `server_crypto_config.h`, `delay_pin.h`, `host_pin.h`,
`native_pin.h`. Changed: `candidate_auth.h` serve declaration only (no layouts),
actual `rtsp_server_candidate.c`, README and new offline builder. Added unchanged
`listener_custody.c`, its header with only include substitution, and explicit
`prestarted_owned_listener_v1.h`. One actual implementation, never two siblings
linked together. R1 terminal IPC history/scalar observer/unknown-start handling
and real auth/verifier protocol/policy/signals remain unchanged.

## Actual API, attempts and custody

Public source API:

```c
camera_rtsp_prestarted_owned_listener_serve_v1(owner, options, producer,
                                             &actual_listener_fd, &outcome);
```

Original actual `candidate_prestarted_start`/`candidate_prestarted_finish` remain
explicit. There is no old internal-bind/prestarted/monolithic serve export, no
fallback, init/fork/exec or hidden finish inside serve. `candidate_prestarted`
embeds one actual stable backend/child/config and four real persistent peer
records. The new API includes its actual `candidate_auth.h`, not frozen sibling
backend definitions; old scalar outcome name/layout is preserved for helper C.

| Real boundary | Source disposition |
|---|---|
| Trusted order | Actual device-free start first; ALL forks before native/provider/watchdog/backing/session/userdata ownership. Selected listener must also be created AFTER last fork. Metadata is not proof of these prerequisites or endpoint authority. No admitted bootstrap is supplied. |
| Stable config | Same config/graph/context ADDRESS/callbacks and real device-free RNG/material. Later actual userdata only inside that context; no fake readiness, source callback, provisioning or new native preparation. |
| Nested entry | First guard at server657 returns BUSY before any owner/FD/outcome touch, even NULL/different arguments. Guard spans callbacks, cleanup, close and publication. Serialized trusted C only, not thread safety. |
| No-attempt refusal | NULL required storage, invalid self/config or non-READY phase refuses untouched; old outcome is deliberately stale, not a new disposition. |
| Actual attempt | Valid READY owner + required storage sets SERVING and initializes outcome, consuming one serve attempt even if backend/options/clock/listener preflight fails. Backend stays with owner on HOLD; caller FD remains unchanged precommit. |
| Borrow/preflight | Original options/auth/source/URI/lifecycle/hub checks; actual backend association/READY15/profile/live child/pending/peers; listener fstat/F_GETFL/F_GETFD/SO_TYPE/ACCEPTCONN/PROTOCOL/getsockname. No generic ready bit. |
| Poll/recheck | Actual backend poll at449 may call policy;452–457 recheck actual association/failure/revoke/pending/peers/READY/profile/current child and clock before listener move. |
| Commit | Helper reobserves ALL actual socket attributes, compares numeric caller slot and fstat dev/inode, then private FD takes ownership/caller=-1/consumed1. No callback/effect between final checks and move stores. No alias/no-authority proof is inferred. |
| Real use | Guard bind465 only after commit; poll471/accept485 use private moved FD. Accepted-peer socket options remain distinct; no selected-listener socket/bind/listen/F_SETFL/setsockopt. |
| Stable peers | First open492–493 uses `&owner->peers[i]`, not stack-owned peer storage. All original non-open/refusal/client-close/backend pending semantics remain. |
| Cleanup | Original client/hub/source cleanup611–627 unchanged. Unbind630 while gate is live, then clear-before-one-close631. Every postcommit return consumed1; close error remains in outcome, result non-OK and owner HOLD645–648. No close/retry of former number. |
| Publication | Final owner result/phase and outcome675–677 precede reentry guard release678. Pending/revoked/failed/peer/source/clock/close errors never become retryable READY or clean SERVED. |
| Finish | Caller-only original READY/START_FAILED/clean SERVED verifier finish; never HOLD. R1 sticky close error/custody and no uncertain-start second stop are unchanged. No automatic backend destruction. |

FD table, signals, callbacks, threads and all storage must be serialized/live/
stable/disjoint, no aliases/dup/SCM/fork or callback mutation of slot/outcome.
`fstat` identity does not prove alias exclusion. Precommit error means caller
still owns listener, not that backend was disposed. Postcommit close error means
no second numeric-FD close is allowed, not retained descriptor ownership. Caller
must retain backend/config/context and error evidence on HOLD.

## Compile and static custody evidence

First build01 PASS16:19:19–16:19:29UTC. Thirty real C compiles,52 successful
compiler/tool/archive/REL commands plus six expected diff exit1 records. All30
compile logs empty. Actual15 units/platform, no old objects, main, harness,
synthetic fixture, sibling archive or executable. Strict native2MiB/HOST4MiB
image pins/profiles, auth/rates/loopback2s/one1MiB hub,128KiB allocation cap,
40MiB/CMA and inherited watchdog/retention constraints are not relaxed.

| Artifact (under runs/build01) | Bytes | SHA256 |
|---|---:|---|
| `libprestarted-owned-listener-host.a` |179300| `f6a773eaf3eee9d379e8102eb4de1f5d68cd28bdbf5d741fe5c7d80587e26c5f` |
| `prestarted-owned-listener-host-NOT-EXECUTABLE.o` |125328| `c41bfa5caa6735c675d9a446b8b7e92eba1458f85ebdad97c85c007636fe294e` |
| `libprestarted-owned-listener-arm.a` |127304| `85951bdecbc81fd3b43d0e73dfa1ded0bdeb4b1a19161f8cb05a7c1710be9cc7` |
| `prestarted-owned-listener-arm-NOT-EXECUTABLE.o` |84692| `388803d8f40fe26e86bf6e2766b0d20ace97b069a270dff300eebcd1f16df623` |

337 recorded inputs,308 consumed source/header paths and2669 frozen pins were
checked by builder before/after. Finalizer rechecks these, all copied inputs/
dependencies/licenses, actual archive members/link/maps/ET_REL/symbol/relocation
structure, tool targets/resolutions and command logs. Final seals cover source,
generated source, closure, evidence and artifacts, not independent acceptance.
Three GC-discarded `camera_rtsp_digest_close/issue/verify` names remain undefined
in both symbol tables but have no relocations. Actual referenced undefineds are
libc/libgcc; no real project call remains unresolved. No claim of static executable
link, native fit or runtime behavior follows from REL size/closure.

## Deferred checks and handback

No application/helper/child/listener/output/test/generator was run. Ordinary
functional behavior (precommit refusals, nested BUSY, commit/cleanup, child death,
IPC/close failures, full RTSP/source lifecycle) is NOT exercised. Further
penetration/adversarial/fuzz/mutation testing is deferred, not passed.

Stable backend/peer pointers do not retain the gate/client/hub stack after return.
Native failed-NEXT partial-work cleanup remains incompatible with the unchanged
public framework contract. External nonreturning retention/bootstrap/recovery,
backing/combined memory, entropy/trust/provisioning, failed-state cleanup, real
provider adoption and full-app release packaging remain BLOCKED/uncomposed.
No product main, media RPC/supervisor, allocator, END/reset/feed or query77-client
adoption is delivered. Query77 lane was not inspected. Both historical failure
HOLDs remain unchanged; last historical held U-Boot is not fresh admission.

Inherited v1 R1 correction-required review and unrelated static seal bookkeeping
failure remain copied in provenance/history; filename lookup errors are disclosed
in METHOD, not hidden as policy failures or runtime results. Current static
finalizers' undefined-name classification and ARM map pseudo-entry bookkeeping
failures are retained in `SEAL_FAILURE01.json`/`SEAL_FAILURE02.json` and original
`seal.py`/`seal02.py`; corrected data-only `seal03.py` does not change/recompile
the app. Current compile
succeeded first attempt. No Git/index/central-plan change, policy rejection,
publication, target/network/private IO or automatic reviewer/test/native/next
queue. Hand back exactly this bounded unreviewed SOURCE/compile candidate for
independent composition review by the parent.
