# Build01 — FAILED_PRESERVED before compilation

2026-10-24T11:51:47.337862+00:00 to 11:51:47.804148+00:00.
Invocation: `python3 apps/rtsp-auth/qc07f-writer-runtime-candidate/compile01.py`.
Python exited1; six offline metadata commands exited0. **ZERO C compilations,
ZERO links, ZERO candidate/library/helper/function/test executions.** No ARM
compiler command reached. No program or final executable exists.

`runs/build01/REPORT.json` preserves the failure, exact six argv/cwd/times/log
pins,127 input pins,8 tool records,22 artifact records and consumed seven app
files. No output was overwritten, source changed or automatic rerun selected.

The recipe failed in its archive-role audit, before extracting any member:

```text
compile01.py:155
source = Path(c['argv'][c['argv'].index('-c') + 1]);keep(source, old['inputs'][str(source)])
KeyError: '/home/gadorach/Desktop/AnykaOpenCam/docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02/source/app/owned_runtime.c'
```

The saved baseline compile argv names its consumed copy under `source/app/`.
That path is in baseline `dependencies`, NOT `inputs` or `artifacts`. The original
workspace source is in `inputs`. Read-only follow-up verified both the consumed
copy and original have11809B/SHA256
`5e21fcb2559911b79ffbd7732b951b8d8931a84e2e33f03a52f39d09de0dbd43`.
This is an AUTHOR RECIPE LOOKUP ERROR, not a candidate/compiler failure, missing
source pin, baseline drift, native ABI result or demonstrated REL infeasibility.

The six successful commands were HOST `cc --version`, `cc -dumpmachine`,
`cc -print-prog-name=cc1/as/ld`, and `ar t` on the retained HOST archive. The recipe
stopped at the FIRST archive object, itself an excluded old-runtime role. No
selected dependency/body audit completed; no extracted-member or relocation
closure claim follows. New source therefore remains **UNCOMPILED / UNREVIEWED /
NOT ADOPTED / NOT RUN**. Intended14-compilation/two-REL profile is NOT evidence.

The recipe/current seven app files remain identical to the failed attempt's
consumed copies. Its failure is immutable; do not silently fix it in place or
rewrite this result. A future independently selected fresh correction can use the
actual baseline dependency pin for the consumed copy while retaining strict role/
layout/tool/body checks. No such correction, second recipe, build or use is
selected by this handback. Ordinary independent SOURCE review and parent
disposition remain required before use; any subsequent compile selection is
separate. All other gates/HOLDs and hardware boundaries remain unchanged.
