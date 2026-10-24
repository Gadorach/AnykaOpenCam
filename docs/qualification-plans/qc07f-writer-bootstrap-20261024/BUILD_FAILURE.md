# FIRST/ONLY offline attempt — FAILED_PRESERVED, STOP

Exact invocation (cwd `/home/gadorach/Desktop/AnykaOpenCam`):

```
/usr/bin/python3 apps/rtsp-auth/qc07f-writer-bootstrap-candidate/build01.py
```

UTC 2026-10-24T13:31:26.304207+00:00 through
2026-10-24T13:31:26.842575+00:00. Exit1; report written by finally:
`runs/build01/REPORT.json`, SHA256
`a68bbf16b8dbadf1cdf8026534686a4c1026c2a2f95e52a7c423f6358391fea7`.

```
KeyError: '/home/gadorach/Desktop/AnykaOpenCam/docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02/objects/host-5-frame_fd_custody.d'
```

Failure is the recipe's direct `.d` artifact-pin lookup into the old runtime
report's `artifacts` map. This is NOT a compiler/source diagnostic, source/API
acceptance, native state observation or policy refusal. The old report lacks the
requested key; do not invent an input-map fallback or weaken the check here.

**Five successful HOST compiler-metadata commands only:** version, machine,
cc1/as/ld program-name queries. **ZERO new C compiles, ZERO links, ZERO ARM
commands, ZERO candidate/function/test/child/native/hardware executions.**

Initial110 frozen file pins/12 source memberships and exact accepted source pins
were checked before the failure. Source copies, four literal forward/inverse
diffs, inverse reconstruction and three unchanged regions were saved. Some HOST
reuse records were checked (394 overlapping inputs/257 reused dependencies/eight
tools); the first original custody `.d` lookup then stopped. No claim of completed
reuse/body/role/dependency/ELF/symbol/relocation audit: later checks did NOT run.
Final in-recipe frozen/consumed checks also did NOT run.15 artifacts are recorded
in the report; report itself is the16th retained run file. Counts overlap.

No source/recipe correction, retry/build02, compiler/link continuation, weakened
check, old-builder/policy reroute or candidate execution follows. New source is
frozen as delivered, **UNCOMPILED and independently UNREVIEWED**. Existing accepted
dependency builds are prior evidence ONLY and cannot supply this bootstrap's
missing compile evidence. Handback custody metadata is separate, not a resumed
build/preflight. STOP for parent disposition; no automatic next/hardware queue.
