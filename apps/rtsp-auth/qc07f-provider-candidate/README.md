# Preopened frame FD custody — source/compile candidate, not a provider

One bounded real helper, using the **existing** native75 receipt UAPI. No camera
main, provider-ready flag, capture/configuration operation, allocator or guardian.
Frozen runtime glue/server/evidence are not changed, included, rebuilt or adopted.

`qc_frame_fd_take` moves a preopened frame FD from an explicit owning bootstrap
slot into caller-owned static custody storage after metadata and observational
BIND checks. The bootstrap supplies the actual independently observed device/
source/file tuple for the exact pinned kernel. It must exclusively own the open
description with no dup/fork/SCM_RIGHTS alias or concurrent use. This is a trusted
typed precondition, **not something fstat/BIND proves**. Without that real custody
history the helper is inapplicable; it cannot certify an arbitrary FD as safe.

`qc_frame_fd_give_back` only transfers the same FD back, after reobservation. It
never closes, stops, ACKs, frees backing or proves retirement. Unexpected receipt
state/identity/syscall failure latches HOLD while retaining the actual owned FD
and known identity/observation. No destructor, retry, recovery or HOLD-clear API.
Caller must retain the storage/process on HOLD; this helper does not create a
failed-next retention owner for the camera pipeline.

The accepted observation is current NONE/gen0, **not never-used/global-available**:
BIND after a prior successful ACK may also return NONE. No device numbers are
hardcoded. FD0/zero-default owner, missing expected tuple and non-CLOEXEC/non-readonly
inputs refuse. No descriptor is duplicated. Adoption is in-process ownership
transfer, not proof that kernel task ownership exists (the examined UAPI has none).

Full sensor/encoder/watchdog/backing adoption remains **blocked**. In particular,
watchdog preopen starts hardware, and the two per-frame watchdog open roles cannot
be preopened independently on the single-open core. Encoder lifetime query is a
global snapshot, not this FD's session/reference ownership. Exact source findings,
roles, close behavior and necessary next source change are in
`docs/qualification-plans/qc07f-provider-20261022/CANDIDATE_REPORT.md`.

## Build only

```sh
python3 -B apps/rtsp-auth/qc07f-provider-candidate/build.py fresh-name --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

Writes only fresh `docs/qualification-plans/qc07f-provider-20261022/runs/<name>/`.
HOST/ARM compile, archives and ELF REL link only. No executable/main is emitted;
no output/test/synthetic campaign/listener/provider is run. Actual implementation
and native input snapshots/pins, UAPI equality, dependency/toolchain pins and
licenses are retained. No original source overlay is applied: `applied_source_overlays=[]`.
This source API is not a binary descriptor-transfer format or native acceptance.

New code is GPL-2.0-only; original UAPI/implementation notices and retained compiler/
musl license notices remain intact. No credential material is used or generated.
The128KiB allocation limit,40MiB/CMA memory gates, native2MiB/HOST4MiB child split,
strict image pin, loopback-only server, failed-next/backing/radio HOLD and G3/G8
remain unchanged. A passed compile does not establish safe target invocation.
