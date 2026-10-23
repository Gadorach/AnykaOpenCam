# Device-free bootstrap connection — author handback

2026-10-23 · CORE-03/04, SEC-01/03/05/11, STREAM-07.
**Source candidate delivered; HOST/retained ARMv5 compilation and real REL closure
PASS. NOT RUN. Independent ordinary review and parent adoption are required.**

## Actual feature connection

New source only: `apps/rtsp-auth/qc07f-bootstrap-candidate/bootstrap.c` (111 lines)
and `bootstrap_v1.h`. The design was written first in CONTRACT.md, after reading
the real provisioner begin/observe/bind, runtime prepare/postfork/native init and
prestarted start/finish/backend destroy/verifier adapter bodies. No frozen source
or old builder was modified/invoked. This is not a simulated transition wrapper:
its actual calls are to the frozen APIs, using the embedded actual objects.

| New API | Actual calls and custody |
| --- | --- |
| `qc07f_bootstrap_begin_v1` | One process-local guard, stable self/config/routes/bootstrap, actual `rtsp_provisioner_begin` with canonical ingress slot and embedded actual fresh prestarted owner. No prepare/verifier/native ownership first. Underlying preflight/fork failure retains moved ingress; pre-admission null/identity refusal leaves caller slot unchanged. |
| `qc07f_bootstrap_observe_v1` | Actual provisioner observer; provisioned only from its real READY (checks + own-child wait0 + closes), never receive alone. No automatic bind/prepare/start. On outer HOLD still delegates actual pending/HOLD provisioner observation to preserve own-child collection; never clears outer HOLD. |
| `qc07f_bootstrap_start_v1` | Explicit only after provisioner READY: **actual parent bind → actual owned-runtime prepare → actual same-owner prestarted start**. Stops at actual prestarted READY/start failure; no postfork bind/native init/setup/serve. |
| `qc07f_bootstrap_finish_device_free_v1` | One explicit actual `candidate_prestarted_finish`, only matching actual READY/START_FAILED. No raw backend destroy, hand reaping or backend-field retirement inference. Actual finish0 + PS_FINISHED gives FINISHED_DEVICE_FREE; otherwise sticky HOLD and full retained owner/namespace. |

The initializer contains real zero runtime, `CANDIDATE_PRESTARTED_INIT` and
`RTSP_PROVISIONER_PROCESS_INIT`, not fabricated fresh/ready surrogates. Whole
bootstrap/config/graph/routes/context addresses remain stable, with fields private
after begin and **no borrowed mutable runtime/owner getter, activation or serve
export**. Public enum is source progress, not native permission. Real caller
callbacks are configured before begin to accept the future embedded runtime
address; after prepare they use `runtime.bootstrap` for stable public userdata.
Prepare installs real runtime callback addresses, not fake entropy or no-op sinks.
The header's cfg.context convention is a trusted caller prerequisite, not a
deep check or permission flag. Provisioner child invokes no config callbacks.

Runtime prepare MUST follow bind: provisioner begin snapshots the original config,
requires material_fd==-1, and unchanged checks remain live through READY/bind.
Prepare mutates config.context/source_ready/source_guard_bind/retain_uncertain/
retain_frame_uncertain. After bind those changes are legal, before it they would
break the real provisioner checks. No preparation occurs in pending/receive-only
or READY-before-bind paths. Same actual owner subsequently starts the verifier.

## Failure and lifetime details

- Admitted ordinary failures latch the first public error; no reset/retry or auto
  startup from observe. START_FAILED remains a distinct finish-eligible custody
  phase with sticky start error. Identity/reentry refusals do not touch another
  owner. Calling APIs out of order can enter permanent HOLD; this is not a
  forgiving restart/status interface. Observe is for provisioner progress only,
  not a post-start verifier poll or general status accessor.
- Provisioner HOLD keeps ingress/socket/received rights, possible partial linked
  file, borrowed directory and still-known actual child. Its own HOLD observer
  can collect that child but not retry unknown wait/ECHILD or admit startup.
- After actual bind, **config owns the original readonly material FD**, even if
  prepare/start/finish fails. `rv_start` duplicates that FD and closes its own
  temporary references; neither backend destroy nor this glue closes the original
  config slot. No double-close, implicit close/unlink or numeric close retry.
- Actual start UNKNOWN already invokes the real retain_verifier callback and
  enters actual owner HOLD. Glue retains it without a second stop/finish attempt.
  Other actual START_FAILED paths can own a child/client and require explicit
  finish; no returning stack owner or implicit destruction is used.
- Finish invokes the actual verifier stop/wait/close/destroy policy once. UNKNOWN
  keeps actual owner/child namespace and actual caller retention sink. A known
  PS_FINISHED can still return a historical start error or normal wipe/wait0 error;
  glue conservatively stays HOLD on **any nonzero** finish, preserving that exact
  result. Only actual finish0 + actual PS_FINISHED is successful verifier-only
  FINISHED_DEVICE_FREE, not file/FD/native retirement, safe exit or product STOP.
- Private handles must never be used to initialize native elsewhere and then
  reuse this finish. No earlier native/media ownership ANYWHERE is a mandatory
  trusted precondition, not inferred from fresh/backend fields. All callbacks
  stay device-free and honor no-reentry/lifetime constraints. Borrowed directory,
  linked nlink1 file and writer exclusion survive all states including HOLD and
  verifier consumption/finish. Later linked-file retirement policy is still absent.

## Offline build evidence (no functional invocation)

Exact fixed recipe:

```sh
env -i PATH=/usr/bin:/bin LANG=C LC_ALL=C TZ=UTC /bin/bash apps/rtsp-auth/qc07f-bootstrap-candidate/compile_recipe.sh
```

It refuses an existing `runs/build01`. INVOCATION.json retains actual argv/cwd/
timestamps/exit. Build ran **2026-10-23 11:43:36.316742–11:43:36.882426 UTC**, exit0:

- **2 changed actual-unit C compiles, 2 real integration REL links, 24 successful
  tool commands**, strict -Wall/-Wextra/-Werror with no glue warning waiver.
  Both compile logs empty. HOST GCC16.2.1, retained musl ARM GCC14.3.0;
  ARM `-Os -march=armv5te -marm -mfloat-abi=soft`, ELF32 ARM EABI5/v5TE REL.
- Audited existing runtime build02's364 input pins/378 dependency paths and actual
  archive bodies:23 members per architecture, all46 byte-equal their recorded
  object artifacts and native-shaped compiler commands.80 live source/header/
  snapshot matches. New links reuse those archives plus provisioner objects
  **0,1,2,3,9** (process/handoff/constructor/Digest/MD5) per architecture, all10
  pinned and their724 original dependency records subsequently rechecked.
- Before linking, recipe additionally runs actual read-only ar member, readelf
  member/header/attribute and nm symbol inspection. No PUBLICDUMMY/fixture/fallback
  defines in selected objects. No PUBLICDUMMY executable, whole provisioner REL,
  duplicate graph/wire/parser/SHA/constant-time/zeroize objects or fake providers.
- **863 pre/post-build input pins unchanged**,180 new actual consumed source/header
  records (145 unique path strings),37 build artifact/log/manifest files pinned in
  REPORT.json (report itself separately pinned by delivery manifest). Counts overlap.
  Selected-source/tool/archive audit is BUILD_AUDIT.json; exact argv, start/pass
  markers and all24 logs are retained. ARM glue function frames16..40B are compiler
  observations only, not whole stack/native memory-fit proof.
- Both RELs define all ten selected bootstrap/provisioner/prepare/start/finish APIs
  exactly once; no main/PUBLICDUMMY symbols or unresolved project/crypto API.
  Libc/compiler/final-link support remain undefined, including ARM __ehdr_start;
  **these are NOT executables or a release link**. Whole runtime archives retain
  inherited native/serve implementations, not newly callable glue activation APIs.
- Initial read-only inline archive audit failed its member-count assertion because
  its reader skipped GNU ar long-name entries. No build/candidate ran, no evidence
  files had been written. Read-only header inspection identified the /offset table;
  corrected reader compared all46 members. This tooling mistake is preserved in
  BUILD_AUDIT.json, not a successful first audit or a candidate failure test.

Report SHA256: `86ed8b9404388550593df89b2e86fee76473675eb0474011beebe4fd5098181f`.
C SHA256: `d67d1f4c752b3809ed11f2f5862b9296ba7c57098df62f3cbe29c46dabf79eb4`.
Header SHA256: `4bbd0bbc39fec24e0acc3d2146bf05e86c29be155bc462b6fc6eecb48306c863`.

No program/test/fixture/generator/listener/verifier/provisioner child/native
hardening/credential read or creation was invoked. Compiler/linker/readelf/nm/ar
and public-source/hash metadata work only. No adverse/negative/mutation/fuzz/
penetration matrix. No old builders, hardware, proc/dev/private/network or Git/index
access. Central plans/AGENTS were not changed. Only assigned source/docs were written.
All inherited HOLDs and parent camera state are untouched, not freshly inspected.

## Limits and next action

**Next: independent ordinary source/API/order/custody review, then parent adoption.**
No candidate call is selected. No external authorized writer/ingress/launch, actual
secret-free parent or global inherited-FD/environment/capability hygiene proof is
supplied by callback pointers or metadata. Qualified entropy, parent hardening,
native image identity, durable generation/storage/rotation, linked-file failure/
retirement policy, real external owner/recovery/backing, main and packaging remain
gaps. Nominal budgets are not kernel-stall bounds or all-domain recovery/C1/clock
waivers. Static glue adds no heap/media buffer, no128KiB cap exception, and does
not fix/qualify inherited1MiB sites, combined40MiB load or native ABI/timing.
No provider adoption, native/PLAY/decode/finite STOP, SEC/G3/G8, deployment or
finished product claim. New source GPL-2.0-only; original dependency notices stay.
Generated objects/REL are local evidence, not Git/publication approval. No commit
was made; parent alone curates any later reviewed local commit.
