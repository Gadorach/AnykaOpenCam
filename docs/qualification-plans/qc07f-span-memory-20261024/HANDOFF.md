# QC07F span memory — compile-only handback

2026-10-24. CAP-01/05/06/07, RP-04/09, B07.
**DELIVERED SOURCE CANDIDATE / COMPILE-REL PASS / NOT RUN / NOT ADOPTED.**
Independent ordinary review and parent adoption are still required.

## Delivered feature

`apps/rtsp-auth/qc07f-span-memory/` contains ONLY four replacement memory units,
unchanged local reader/handoff include inputs, provenance and fixed offline recipe.
Real typed connection:

`actual accepted v2 hub → complete eight-range forecast → one retained attempt →
real checked()/unpaid metadata gate → all8 whole-chunk SAME-byte touches → fresh
unpaid final gate → unchanged actual ownership/ranges → one atomic whole-kind
metadata credit → recorded success`.

`nh_memory_prefault_span_hub(runtime, actual_v2_hub, original_unit_start)` is the
actual new runtime bridge. `nh_memory_span_hub_forecast` is non-effectful metadata
validation; `nh_memory_credit_span_hub` is ONLY the trusted-prior-touch metadata
half, NOT native admission. No caller bool pays debt. Scalar kind1 is refused as
the first action of both old scalar credit/prefault exports; raw/producer remain.

Ledger keeps original begin/end[3] and adds exact owner/count/eight ranges/credit.
No flat hull, one-chunk-paid-eight-times, payload export or lease creation. Checks
fixed8×131072 actual shape/history, pristine self/ACTIVE/generation/tokens/clients,
all overflow/disjointness/prior paid extents before touch/commit. Only full
INTERIOR4096-byte pages pay; no boundary/padding or duplicate-page credit.

Runtime retains exact borrowed owner/ranges before callbacks/touch. Any admitted
partial clock/cancel/read/gate failure stores sticky HOLD/unpaid, preserving all
backing; no retry/reset/free/idempotent-success path. No fallible work after
credit. Uncaught address faults leave the nonretryable in-progress attempt, not
a claimed survived fault. Trusted valid stable private caller memory, no aliases,
mutating callbacks/reentry/signals/fork/concurrency and actual lifetime retention
until separately proved real retirement remain mandatory, not proved provenance.

Conservative user-extent page cover: flat1MiB1028KiB → eight128KiB1056KiB, +28KiB.
Minima raw-reuse2832KiB/separate3860KiB; unchanged ARM ELF32 inspection, HOST layout
refusal and no fixture branch. Full-interior credit deliberately leaves boundary
debt unpaid; user-extent cover is NOT allocator header/arena/fragmentation charge.
512KiB envelope,1024KiB reserve, exact8454144 codec credit and full CPU/CMA migration
policy are unchanged. Parsers/evaluate/checked are byte-identical; reader/handoff
whole files byte-identical. See `SOURCE_DELTA.json` and both literal inverse diffs.

## Exact offline build

One invocation (fresh output refuses overwrite):

```text
/usr/bin/python3 apps/rtsp-auth/qc07f-span-memory/compile01.py
```

Result: `COMPILE_REL_INSPECT_PASS C_compiles=10 REL_links=2 commands=26 NOT_RUN`.
Five actual C units per role: replacement reserve/runtime, unchanged fixed reader,
accepted v2 hub and accepted span packetizer. No fixture define, executable, fake
ELF/clock/provider or old builder. HOST GCC16.2.1 (20260810), x86_64-pc-linux-gnu;
retained musl GCC14.3.0 Buildroot2021.11-18033-g83947c7bb6, ARMv5te/marm/soft-float.
Exact flags/argv/environment/timestamps/tool pins/logs are in
`runs/build01/REPORT.json` and individual command records, not just this summary.

- 10 C compiles (4 changed-unit compiles +6 real reader/hub/packetizer compiles).
- 2 actual rooted five-object REL links,2 readelf and2 nm inspections.
- 29 selected input pins,98 consumed source/header path records,12 retained tool
  records,26 logs,89 artifact records in the build report; all freshly rechecked.
- 15 frozen original/reference pins still unchanged. Source forward/inverse
  reconstruction and unchanged-region comparison is static bookkeeping only.

ARM output is ELF32 REL/ARMv5TE/EABI5; HOST is ELF64 REL. Actual new APIs, fixed
reader, v2 hub and packetizer are defined in each REL. ARM `__ehdr_start` remains
normally unresolved alongside libc/compiler symbols: not a final executable,
static/native ABI/layout/residency or deployability proof. `.su` records retained,
not transitive stack or process budget acceptance.

Build report SHA256:
`12327c7f7f084f85d92159323c2736e3d9683cc979255f252b7f988a7a7319d6`.
Replacement C SHA256:

- reserve: `3de7e750b5f846c1a77804113862881947074f42ed5718627efe70942337316a`
- runtime: `0e0434b5a8a80493e2582f8efa890fb634dd21543da35808ffd706c3a0a00332`

Header hashes and all delivery pins are recorded in `DELIVERY.json`/build inputs.
No function/harness/test/helper/callback/credit/prefault/native/proc/allocator/
provider/bootstrap/verifier/socket/device operation was executed. No negative,
mutation, adversarial, fuzz or normal replay trial. No subworkers.

## Open gates / one next task

**Next: independent ordinary SOURCE/API/ownership review, then parent disposition;
no runtime/adoption queue.** Read CONTRACT.md and AUTHOR_AUDIT.md with full actual
source and accepted v2 ownership, not only the build result.

Same names/type/header guards have NEW layouts. Use only these replacement memory
units and v2 hub, rebuild all actual consumers; NEVER old+new memory units or old
combined RELs/hubs. This lane does not modify native_adapter.c, owned_runtime.c,
private owner/ready headers or provider-return.inc. In particular:

- original native_adapter83–94 still passes flat hub kind1 and now MUST refuse;
  actual typed native adapter invocation remains a separate future task;
- original ready61–75 sees only old paid[1] and lacks new association/attempt
  checks; it has NOT been connected or qualified;
- original provider-return39–42 omits new hub/range/attempt/retained failure history;
  ALL new fields must participate before that return path can be adopted;
- no dependency on/adoption of either separate sender candidate, no source/frame
  lease or new app main/serve/callback/provider startup.

Native raw/output1MiB allocation sites,128KiB cap, allocator/library/stack/socket/
prefault pressure, process2MiB/40MiB/CMA/combined-load, actual backing/external owner/
recovery and native memory-fit remain UNQUALIFIED. No CMA exclusion/limit increase.
Real scalar readiness cannot be inferred from this compile result.
Camera last OFF after finalized23h; RAM75 recovery/C1 and all earlier failed HOLDs
unchanged. No private/proc/dev/capture/network/UART/HTTP/RPC/lock/hardware access,
Git/index/central-plan edits or commits. Parent alone owns adoption and hardware.
