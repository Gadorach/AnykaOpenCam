# Best-effort RTSP app/component build — PASS, not run or deployed

2026-10-22; RP-08/09, SEC-05/11, STREAM-04/07.

The user requested a functional test list and best-effort app build, deferring
further penetration/adversarial/mutation testing. Parent performed ordinary source/
build inspection and compilation only. **No app, test, generator, listener, process
guard, verifier child or camera runtime was launched.** Existing protections remain.

## Successful build

```text
python3 -B apps/rtsp-auth/build_app.py build02 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

Completed **04:02:16–04:02:31UTC**; **88** compile/archive/link/compiler/ELF commands,
all exit0. Native HOST GCC16.2.1; retained ARMv5 EABI/musl GCC14.3.0 toolchain.
Frozen41-file candidate pin and vendor/toolchain inputs verified; build source
pins stayed unchanged. Output root: `local/build/rtsp-app/build02/`.

| Artifact | Bytes | Role |
| --- | ---: | --- |
| `libcamera-rtsp-integration-host.a` |169162| Strict HOST integration components |
| `libcamera-rtsp-integration-arm.a` |115910| Strict ARM integration components |
| `camera-rtsp-verifier.host` |103512| Strict verifier companion, not executed |
| `camera-rtsp-verifier.arm` |133012| Strict ARM companion, not executed/adopted |
| `camera-rtsp-bench.host` |116608| Synthetic HOST bench app, not executed |
| `host-integration-linkcheck-NOT-RUN` |92704| Real server API rooted for link closure; dummy main, never deploy |
| `arm-integration-linkcheck-NOT-RUN` |132128| Same link-only check for ARM, never deploy |

Full artifact hashes, source/header/toolchain pins, source snapshots, commands and
logs are retained in the local output. Safe build metadata is copied under
`app-build/`: both build reports, `BUILD_SUMMARY.json`, ELF reports and the initial
link failure. Built binaries remain local and are not committed.

ELF metadata is static inspection, not native ABI/ISA proof. Retained runtime
attributes are preserved, not normalized or treated as camera execution. The
new verifier is not substituted into existing frozen native/HOST image pins.

## Initial failure and correction

`build01` (04:00:37–04:00:41UTC) failed the first HOST archive link: an unrestricted
whole-archive link pulled unused direct-Digest wire helpers that reference the
credential core. The isolated server deliberately does not contain that core.

The corrected build uses the existing function-section garbage-collection policy,
explicitly rooting `camera_rtsp_isolated_candidate_serve` and
`candidate_backend_destroy` so their actual server dependency graph remains linked.
No undefined-symbol stubs, crypto/authentication bypass, source predicate removal
or in-process credential core was added. Static symbol inventories of the resulting
HOST/ARM server link-checks confirm the actual server entry points are present
and local HA1/provision/verify/principal-issue functions are absent. This is build
content inspection, not runtime/security qualification.

The failed report/log/source snapshot are preserved. One subsequent shell command
had an accidental space in the toolchain argument and was rejected by argparse
before creating output; the corrected command above created fresh `build02`.

## Source custody versus remaining runtime review

Parent inspected the copied-source provenance, exact six-file harness delta,
changed C predicate/receipt/fixture code, build procedure and HOST process helpers
without executing them. The predicate set remains fail-closed; added prefix
observers are HOST-conditional, not an authentication fallback. The public frozen
source snapshot is retained for reproducing this **build-only candidate**, not
adopted as a runtime service. Its separately reviewed entry/cleanup/oracle operation
and full harness campaigns remain deferred; earlier author standalone guard/syntax
results are not promoted into new parent runtime results.

No P1 candidate or partial independent-review evidence is adopted or committed
by this build. P1 parent HOST PASS remains distinct from its missing independent
acceptance. No station/module/kernel build, staging or attachment is selected.

## Honest completion boundary / one next task

**Compiled/linked, not a completed deployable camera app.** The current deliverables
are integration libraries, a strict companion and synthetic HOST entry point.
Real platform main, admitted producer/capture/provider and private provisioning
wiring, release packaging, native lifetime/memory proof and ordinary-operation
confirmation remain missing. Existing NOAUTH/media501 behavior is not replaced.
No sensor/encoder/GUI/network/storage/product acceptance follows from these outputs.

Desired operational checks are in `OPERATIONAL_CHECKLIST.md`; all runtime rows
remain pending. G3/G8, other-camera ports, full B08 runtime acceptance and original
radio FAILED/HOLD remain unchanged. Current target state is unknown/unqueried.

Next build-focused implementation: design/wire the real platform entry/provider/
provisioning integration against the existing qualified ownership interfaces,
without selecting new native effects or treating synthetic source as a camera.
