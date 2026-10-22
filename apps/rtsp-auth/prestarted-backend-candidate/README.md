# Prestarted actual verifier/backend — SOURCE/compile candidate

One stable caller-owned `candidate_prestarted` object, initialized once with
`CANDIDATE_PRESTARTED_INIT`, starts the actual validated verifier before native
descriptions/userdata/media backing exist. `camera_rtsp_prestarted_candidate_serve`
uses that same backend without init/fork or hidden finish. Four actual auth peer
records live in the owner, so pending/backend peer pointers do not escape a
returned serve stack. `candidate_prestarted_finish` is explicit and one-shot.

All config/graph/context addresses and callbacks stay stable/unchanged. Later
native userdata preparation inside that stable context is a separate uninstalled
bootstrap task, not permission from this API. START_FAILED still owns partial
startup; real rv_start uncertainty or failed/nonquiescent serving is permanent
HOLD. No destructor/stop retry on HOLD. Caller retains the actual object, child
namespace, process, callbacks and any source/backing. A pidfd does not preserve
an exited process's FDs or prove DMA retirement.

This sibling exports no old monolithic serve entry or raw init/destroy API. It
preserves the original loopback internal bind/listen, real RNG/material/image/
READY15/policy checks, strict native2MiB/HOST4MiB/pins and source guard. It is not
composed with the separate owned-listener sibling. Frozen originals are untouched.
No native/source glue/provider adoption or failed-NEXT contract change. Native
failed work still needs genuine nonreturning process retention/recovery/backing.

Contract before implementation:
`docs/qualification-plans/rtsp-prestarted-backend-20261022/OWNERSHIP_CONTRACT.md`.
No main, supervisor, allocator, media RPC, feeder/reset/END/ACK or installation.
Ordinary functional and all runtime/adversarial/fuzz/mutation checks NOT RUN.

Compile-only command (three changed real units per HOST/ARM, archives/ELF REL):

```sh
python3 -B apps/rtsp-auth/prestarted-backend-candidate/build.py build01 --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

No output execution, test/generator payload, listener/network/private/device IO.
The archive/REL intentionally leaves existing framework/IPC/libc dependencies
unresolved; it is not a full app link or native readiness/memory-fit claim.
Original GPL SPDX/runtime notices remain, no global relicensing. All writes are
confined to this fresh app directory and its assigned qualification directory.
