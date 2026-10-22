# QC07F native producer → existing RTSP callbacks (compile-only candidate)

This is a small reusable **source wiring piece, not an app or a native GO**.
There is no main, new provider protocol, synthetic source, listener launcher or
provisioning reader/generator. RP-07/09, STREAM-04/07; RP-04/native v6 HOLD remains.

`qc07f_rtsp_native_wire` binds the actual retained v6 producer into the existing
`camera_rtsp_common_producer` and isolated-server callback types. It forwards RTSP
IDR demands to the existing real encoder keyframe job, preserves borrowed output
until the existing native release, propagates a failed native release into common
END instead of dropping its status through the void RTSP callback, and requires
the server's existing current-peer guard before START/next. No new buffer, copy,
guard process, credential, generation, hardware ABI or limit increase is added.

The included original native owner has unresolved runtime contracts. In particular
it opens its own provider FDs, contains large legacy allocations, and retains
uncertain work on non-OK next; the public RTSP contract expects a failed next to
retire its partial work internally. This piece does **not** resolve that mismatch
or supply a retention owner. Do not call it on a target, ship it or infer readiness
from successful callback wiring. The finite v6 preparation/receipt/memory/time
limits are unchanged. No full-app executable can honestly be made from these APIs
within this source-only scope. Exact blockers and smallest patch plan:
`docs/qualification-plans/rtsp-runtime-20261022/CANDIDATE_REPORT.md`.

## Build (no generated output is run)

```sh
python3 -B apps/rtsp-auth/qc07f-runtime-candidate/build.py fresh-name --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

Writes only fresh `docs/qualification-plans/rtsp-runtime-20261022/runs/<name>/`.
Produces HOST/ARM archives and ELF **relocatable objects**, no executable/main,
stub linkcheck, fixture, child build, app execution or test expansion. Sources,
licenses, exact compiler commands, dependency closure and artifact pins retained.
Artifacts are local build evidence, not release/package candidates.

## Exact upstream custody / license

- Isolated server/header/config inputs: frozen41-file public candidate, pin
  `2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f`.
- Native v6 original source: immutable
  `evidence/delegated/20261012-native-handoff/v6-final-source-snapshot01/`;
  source manifest SHA256
  `bdc87e1700b8f0a8e6c191ed0bd8869035487258636b55f4e6c8bce3640267d5`.
- Generated native import and exact inputs: immutable `run34/inputs/` in that
  evidence lane; run34 manifest SHA256
  `8286c85c09d7f34d5bb6af13054803f68ebafad4e2e874da4b1a35aaf9e0d529`.
  The builder checks snapshot/run34 agreement before copying public sources.
- New glue/build code GPL-2.0-only; existing source SPDX/license notices retained.
  Retained Mbed TLS and compiler/libc runtime licenses remain their own licenses,
  copied without a global license grant. No original source is edited.

Strict native verifier stays **2MiB**, HOST **4MiB** is separate; embedded native
image pin remains **153060 bytes / 7a8d66850197ae62dd3db4718c57ba0487ccb4fdcaac1c7819241fd0be10f084**.
The build02 child is not substituted. The128KiB allocation ceiling remains;
unmodified v6 large buffers are a **blocker**, not an exception granted here.
