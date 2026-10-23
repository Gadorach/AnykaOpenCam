# Actual owned QC07F → authenticated RTSP source candidate

Implements CORE-03/04, CAP-05/07, STREAM-01/04/07, RP-08/09 **integration parts**,
not completed tasks or a deployable app. No main, synthetic source or default app.

`owned_runtime.c` includes one fresh copy of the latest actual provider-return
generated adapter. `native/native_adapter.c` is **not** a second compiled unit.
One current prestarted-owned-listener server/auth/verifier/backend is composed
with the real common producer/hub/RTP/parser/auth code. Native frame/encoder/
watchdog FD custody, receipt/generation/CRC/known-loan checks remain real.

## Feature wiring

- `owned_runtime_v1.h`: fresh zero/stable runtime, actual prestarted owner, graph
  and config. `qc07f_owned_runtime_prepare_v1` installs immutable callback addresses
  and context references **before** the real verifier start/fork; no native init.
  Existing genuine RNG/owner/listener/verifier-retention callbacks accept the
  runtime address and can access its caller-owned `bootstrap` pointer.
- `qc07f_owned_runtime_bind_postfork_v1` requires the same actual READY backend
  and performs explicit real one-time `nh_native_init`, common producer init and
  original SPS/PPS generation **after the last fork**. Native configuration/
  adoption/backing/effects are still explicit original caller operations, never
  lazy source-ready/listener effects. Config/backend/peers are not copied/moved.
- Actual common START/demand/keyframe/frame END/last-demand STOP bridge calls the
  current guarded `nh_native_next_keyframe`; only force0/1 forwarding was added
  inside its unchanged watchdog-owned sequence. `nh_native_next` still means
  force0. Actual `nh_key` and packet remain stored through exact release. The
  original PRE_NEXT and first-frame POST_FIRST memory gates are connected too.
- RTSP receives the real borrowed single-VCL H.264 packet; the existing hub makes
  its one owned copy before release. Common END retains actual server-copied
  keyframe facts; errors are latched, not turned into outward success. This does
  not implement general multi-VCL AU support or a parallel producer stack.
- Native uncertainty cannot return into RTSP's unchanged failed-NEXT cleanup
  contract. Actual source/release/END/retire failures with retained work fence and
  enter the existing **nonreturning** native SIGSTOP quarantine. Three small
  server cleanup hooks catch clock uncertainty before client cleanup and final
  unretired work before guard unbind/listener close/stack return. Pure pre-effect
  configuration refusals remain ordinary errors. No recovery/END/close retry.

## Required real caller connections (NOT provided / NOT RUN)

1. Device-free media bootstrap with an actual continuing external process owner
   and independent all-domain recovery. Quarantine preserves stack/FDs/backing
   **only while the process survives**; SIGSTOP is not DMA stop or recovery.
   No TERM/KILL/respawn/resume policy is installed. Exit/OOM is retention failure.
2. Genuine private provisioning and qualified RNG; exact pinned verifier IMAGE/
   material descriptors, stable graph/routing/config, domain/rate/policy unchanged.
3. All forks finished before postfork bind and sole-description watchdog/provider
   setup. Caller explicitly applies original UNKNOWN-watchdog take, storage mode,
   exact139 opt-in, frame/encoder identities/adoption, one-shot memory/paid backing.
   Failed setup after custody/work cannot use returning cleanup/destructors.
4. Admitted backing/allocator design preserving 128KiB cap, 40MiB/CMA budgets and
   original gates. Inherited 1MiB resident/hub backing sites remain **unqualified**;
   this wiring gives no allocation-cap exception or memory-fit claim.
5. Real selected loopback listener created after last fork and then moved through
   `camera_rtsp_prestarted_owned_listener_serve_v1` with `&runtime.source`.
   App main, lifecycle/bootstrap error policy, release packaging and native paired
   query/guard/timing/media/load qualification are still missing. Normal stop is
   finite, not restart/reopen authority; post-memory unactivated abort is absent.

No generic ready/permission flags prove those prerequisites. Calls/callbacks/FD
table and storage are trusted serialized, stable, disjoint, nonreentrant, without
threads/signals/aliases or subsequent fork/dup/SCM. Both native HOLDs stay sticky.

## Compile only

The fresh `build.py` uses minimal retained compile/archive/ELF REL patterns; no old
builder/overlay/helper is imported or run. Read/audit before invocation. HOST and
musl ARMv5 GCC14.3.0 compile the same strict auth policy, no HOST fixture define.
Only compiler/tool version/target, compile, archive, `-nostdlib -r`, nm/readelf
commands run. No ELF/main/test/listener/payload-generator/camera is executed.
Reports/commands/input-header-output pins and failures remain in
`docs/qualification-plans/rtsp-owned-runtime-20261023/runs/`.

Exact source origins are in `INPUTS.json` beside `INTEGRATION_CONTRACT.md`.
Native copies originate from provider-return build01 actual generated source;
server copies from prestarted-owned-listener. Original GPL SPDX headers and
`licenses/` notices remain; Mbed TLS retains its original notices/license.
All previous candidates/failures remain immutable. Source/compile candidate only;
parent review, functional execution and native acceptance are separate.
