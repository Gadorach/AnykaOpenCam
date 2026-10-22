# Build-only RTSP app candidate

Current user direction (2026-10-22): focus on a best-effort normal app build and
a written functional checklist; defer further adversarial/penetration/mutation
testing. Existing authentication/isolation/bounds stay enabled. Deferred coverage
is not passed coverage, and incomplete native/product gates remain incomplete.

```sh
python3 -B apps/rtsp-auth/build_app.py fresh-name --toolchain /path/to/retained/pinned/armv5-eabi--musl--stable-2025.08-1
```

Omit `--toolchain` for HOST artifacts only. Every run refuses an existing output
directory; outputs are in `local/build/rtsp-app/<fresh-name>/`. The script reads
and verifies the frozen candidate and vendor/toolchain pins, compiles source,
creates integration archives, links companion/bench executables and inspects ELF
metadata. It does **not** run any output or invoke a test/fixture campaign, material
generator, process guard, network listener, installation or target operation.

## Artifacts

| Artifact | Meaning |
| --- | --- |
| `libcamera-rtsp-integration-host.a` | Strict integration components compiled for the workstation |
| `libcamera-rtsp-integration-arm.a` | Same components compiled for retained ARMv5 EABI/musl |
| `camera-rtsp-verifier.host` / `.arm` | Strict verifier companion builds; existing runtime image pins are **not** replaced |
| `camera-rtsp-bench.host` | Synthetic HOST bench entry point, not a camera runtime; **not executed** |
| `*-integration-linkcheck-NOT-RUN` | Link-only stub whose real server API is rooted to validate dependencies; never deploy |

The library consumer must use function-section garbage collection, as the existing
isolated-server builder does. `rtsp_digest_wire.c` also contains direct-Digest
helpers unused by this server. Pulling those sections into the isolated server
would incorrectly require in-process credential code. The build roots
`camera_rtsp_isolated_candidate_serve` and `candidate_backend_destroy` explicitly,
retaining their real call graph while dropping unrelated helpers. It does not
resolve missing symbols with dummy functions or link the Digest credential core
into the server.

`REPORT.json` retains commands, source/header/vendor/toolchain pins, compiler
versions, source snapshots, artifact sizes/hashes and failure logs. Failed output
directories are preserved; use a fresh name after a correction.

## Still missing

This is **not a deployable complete camera application**. A real platform main,
admitted producer/capture/provider wiring, private provisioning, release manifest,
native lifetime/memory proof and a deployment package are still needed. HOST dummy
source, a static ARM link and old NOAUTH/501 behavior are not protected native
video acceptance. The HOST harness remains a candidate, not runtime-reviewed merely
because its C files compile. No P1/station module is built or adopted here.

Desired normal-operation checks and remaining product confirmations:
`docs/qualification-plans/rtsp-tail-20261022/OPERATIONAL_CHECKLIST.md`.
