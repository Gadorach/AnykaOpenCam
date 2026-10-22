# Owned loopback listener v1 — SOURCE/compile candidate delivered

2026-10-22; RP-09 / STREAM-04 / STREAM-07. **Not run, independently accepted,
native-qualified, installed or deployable.** One bounded source increment only.

## Actual implementation and ownership

`apps/rtsp-auth/owned-listener-candidate/rtsp_server_owned.c` is a real derivative
of the frozen server, not a detached helper/new main. Public source entry:
`camera_rtsp_owned_listener_serve_v1(options, producer, config, &actual_fd, &outcome)`.
Read [CONTRACT.md](CONTRACT.md) and `owned_listener_v1.h` before any use.

| Boundary | Implemented disposition |
|---|---|
| Policy | Only original validated `127.0.0.1` and exact options.port>=1024;2s send, one1MiB hub, real lifecycle, source guards and strict auth retained. No public bind/new option/mode. Metadata never grants authority. Trusted bootstrap create/bind/listen policy is external and not delivered. |
| Borrowed preflight | fstat S_ISSOCK; F_GETFL nonblocking; F_GETFD CLOEXEC; SO_TYPE STREAM; SO_ACCEPTCONN1; SO_PROTOCOL TCP; getsockname exact AF_INET/loopback/port and returned lengths. Queries only, no SET/open/socket/dup/setsockopt/bind/listen on selected descriptor. Missing old APIs fail closed. |
| Startup | URI/lifecycle/hub metadata validations moved ahead of real backend startup. Hub initialization only initializes the local hub record, not backing. Original candidate_backend_init/rv_start/real image pin/READY flags15/profile/engine ACK are unchanged. No dummy backend/callback/child fallback. |
| Commit | After real verifier readiness, requery actual caller descriptor and compare numeric slot plus fstat device/inode; no callback/effect between final query and move. Private owned slot takes fd; actual caller int becomes -1; outcome.consumed becomes1. Exact process-FD-table/source review remains mandatory; queries do not prove no aliases. |
| Actual listener path | Existing poll and accept use only private moved `listener`; original internal socket/bind/listen/F_SETFL sequence is removed, not left as a fallback. All packet/auth/grant/source/owner/lifecycle paths are unchanged. Accepted peer socket options/close behavior remain original and are not selected-listener operations. |
| Before commit errors | Bad options, nonlistener/bad flags/endpoint, clock, real backend startup or final revalidation errors leave actual caller listener slot unchanged. No listen call is attempted. Original verifier retention may still report UNKNOWN; caller listener ownership is not proof the transient child reference retired. |
| After commit errors/return | All paths reach original client/hub/source cleanup, synchronous guard unbind while its stack context is live, clear-private-slot-before-one-close, then original real verifier cleanup. outcome.consumed remains1 on every such return; close_attempted and close_errno are explicit. Close error prevents successful final status; no second close/retry, no claim that numeric fd still denotes the same description. |
| Callback/storage lifetime | Required stable disjoint caller slot/outcome/options/config/graph/URI/producer/lifecycle/hub/backing/code. Gate remains stack-live through callbacks and unbind; no callback may retain the gate afterward. Entry flag rejects synchronous reentry and stays set through verifier cleanup. Nested refusal returns BUSY without touching any outcome (avoids clobbering active caller); it is not a supported ownership transfer. Null outcome returns INVALID. Flag is not a thread lock; caller must serialize threads/signals/table. Sticky clock is never reset. |

## Actual child inheritance inspection

Read frozen `candidate_auth.c:41–117` and
`verifier_adapter_overlay.c:68–225`, not just header claims. rv_start forks once
with caller listener still borrowed. The child can transiently inherit it while
waiting at the original release barrier: CLOEXEC is **not** fork isolation. There
is no listener poll/accept/callback path in inspected launch code. Image/material/
IPC dup targets overwrite3/4/5; required close_range closes>=6;0/1/2 are closed;
execveat precedes READY/engine ACK. Commit therefore occurs only after real child
readiness, never on an image/query-only flag. Unsupported close_range/pidfd/exec
fails original startup; no relaxation/fallback supplied. On uncertain startup,
original child ownership sink remains mandatory and no child-listener-retirement
claim is made. This is not safe for an arbitrary live provider fork: the separate
prestarted-backend/native-retention investigations remain disjoint and unadopted.

## Exact provenance and delta

- Frozen original41-file pin, verified before and after:
  `2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f`.
  Original source, guarded builder/harness and all licenses are untouched.
- `frozen/` imports those41 files byte-for-byte. Its original internal-bind server
  is inert reference: not compiled or exported. The old header declaration alone
  does not implement an entry point. Do not combine this derivative with another
  server/backend library defining the same real dependency symbols.
- Six authored root files (real server derivative, small custody C/header,
  versioned source API header, compile-only builder, README). Whole47-file source
  aggregate is `febab8640027f71dfc8a1ec4aa53c26cba219048ca4bf74578050377101ce557`.
  Recipe: SHA256(sorted(workspace-relative UTF8 path + NUL + raw file SHA256)).
- Exact server diff: `runs/build01/SERVER_DELTA.patch`. No protocol/parser/media/
  grant/retention redesign. Changes are source entry/reentry, query/commit startup
  ordering, listener selection and close/disposition cleanup only.
- Actual prior command provenance: `local/build/rtsp-app/build02/REPORT.json` and
  `apps/rtsp-auth/build_app.py`; native callback metadata reviewed in
  `qc07f-runtime-candidate/rtsp_native_source.h/.c` and its candidate report. No
  runtime glue/provider/query77 image adoption or integration is performed.
- Original GPL SPDX/license notices and Mbed TLS3.6.7 tree pin
  `bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5`
  and runtime GCC/musl notices are retained. No global license grant.

## Compile-only evidence

```text
python3 -B apps/rtsp-auth/owned-listener-candidate/build.py build01
```

**PASS15:14:45.777413–15:14:55.491248UTC**,52 compiler/archive/ELF/tool commands,
all exit0;30 actual C translation-unit compilations (15 per platform: changed real
server/custody and13 unchanged real dependencies). HOST GCC16.2.1; retained
ARMv5 musl GCC14.3.0, exact prior recorded command flags and compiler pins. Both
are original strict integration conditional selections; no HOST-public fixture
macro, harness, synthetic source or auth material is compiled/generated here.

441 explicit public input snapshots unchanged;332 prior source/header dependency
paths rehashed before/after;293 consumed source/header paths recorded in the fresh
build closure. Counts overlap and must not be summed. Compiler/cc1/as/ld/ar/nm/
readelf hashes and actual commands/timestamps are retained. The builder parses
only literal old ARM/vendor pins via AST: it imports or executes **no old builder**.
All originals plus exact imported41-file copies revalidated after compilation.
`FINAL_SEAL.json` seals the47-file source, this report/contract, exact diff,
snapshots, logs and artifacts; it rehashes all enumerated public compile inputs
and verifies original/imported frozen custody again. This author seal is not an
independent review or behavioral test.

| Artifact | Bytes | SHA256 |
|---|---:|---|
| `libowned-loopback-listener-host.a` |174568| `1fdd6a4c0c9f01367654738a0a0233aa93ee46ae6b109dcb739f78e59180fe4a` |
| `owned-loopback-listener-host-NOT-EXECUTABLE.o` |119904| `c11019da664f8c87697da5f86b84ec00a4d1cff5ecad78d46637249941d2a4e5` |
| `libowned-loopback-listener-arm.a` |119668| `f6f3038665d8702ee6d54cc56d6168ab9341de28ab2b255b971708f190a986ca` |
| `owned-loopback-listener-arm-NOT-EXECUTABLE.o` |77996| `64fadbfc041fdc87da23c1f05152309fe8967b356b9bbbfb9474909ede833e0c` |

Outputs live under this assigned docs directory's `runs/build01/`; generated
objects/archives are local evidence, not source/release packaging. ELF REL has
entry0/no program headers. Rooted real call graph includes owned serve/auth/
verifier; no original internal-bind serve export, new main or executable. Actual
unresolved relocations are libc/libgcc, not project calls; unused direct-Digest
symbol-table leftovers from discarded sections are separately recorded rather
than satisfied by credential stubs. `setsockopt`, `fork`, `dup2`, `socketpair`,
pread and fcntl remain for unchanged peer/verifier code, not an internal listener
fallback. No application/test/generator/guard/listener/child/camera was run.

## Remaining boundaries / independent review handback

This candidate is not a trusted bootstrap or endpoint/provisioning authority.
Caller identity/exact source, no aliases and serialized table must be established
before use. Independent review of source/API/partial-failure/close semantics is
next; **no runtime/native operation is queued**. It is not accepted merely because
it compiled. No adversarial/fuzz/mutation/runtime tests were selected or passed.

Real source_ready/randomness/owner callbacks and provisioning remain mandatory;
strict native child2MiB/HOST4MiB, exact image pins, guards, loopback and allocation
ceiling are unchanged. No real endpoint, private input/current network/hardware/
UART/RPC/status/recorder read, Git/index/central-plan/AGENTS/helper/original-source
write occurred. Both assigned directories alone contain this lane's writes.

External failed-next retention and stable process/backing/code/namespace retention
remain **unresolved**, not solved by stack metadata or this outcome record. No
source/provider/retained-verifier/peer cleanup contract is changed; original sinks
must copy metadata only and honor real owner lifetime. No current provider/real
camera main can safely adopt this candidate yet. Native/backing/memory fit, query77
pairing (never booted, embedded station unsupported), provisioning, packaging,
ordinary-operation qualification, G3/G8 and all other product gates remain open.
Last inherited target observation is held original U-Boot12:12UTC, not current
Linux authority; no fresh target observation here. Both HOLDs remain untouched.
