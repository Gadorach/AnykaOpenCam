# Authenticated span server v3 — residual R1 correction delivered, NOT RUN

2026-10-24. RP-09/B07; bounded STREAM-01/02/03/07 and CAP-01/05/07.
**Await independent ordinary review and parent disposition BEFORE source use.**
No self-adoption, function/program/test execution, functional/native GO or next queue.

Only assigned `apps/rtsp-auth/rtsp-span-server-v3/` and this qualification directory
were written. [CONTRACT.md](CONTRACT.md) preceded source changes. V1/v2 apps/docs/
reviews remain NOT ADOPTED and byte/membership frozen; no Git/index/AGENTS/central
plan edit, subworker, disjoint native-runtime read/adoption or hardware action.

## Exact derivative and bounded corrections

SAME exports/types/include guard: exclusive SOURCE replacement for v2, never link
old server units/hubs/combined RELs alongside it. Accepted span-hub-v2 and
rtp-span-candidate are unchanged. Only process_input and serve_prestarted_body
change; all C bytes outside those functions match v2. Header noncomment bytes
match exactly. V2 pause_client/close_client/finish_au, including R2 acknowledgment
ordering, are byte-identical. No auth/readiness/Digest/clock-limit/deadline/cursor/
owned-listener/backend/common-producer API or implementation change.

| New file | SHA-256 |
| --- | --- |
| rtsp_span_server.c | `73c0f5886a54ca8fc064b27c72b7c877180730502673e8a6ee106f085a7304d0` |
| rtsp_span_server_v1.h | `3b81af3ff9e7478ec19b3ab7f91414bdffa74cea231c8fc9d6b180a5f63f5333` |
| build.py | `ec057cb3c0d0bd955ef14d6636e6a68b8ca2bc59253f89876e6c87d576eaf20c` |
| LICENSE_NOTICES.md | `bb24cf0278b76124b00fc71d0a79e759f7cfc3ff06bd375396a6430c12c87c00` |

Frozen v2 C/header pins matched the supplied
`8bebe38b59a2690921074bb1fd348c2504503716433b0d416925a7e2592cc020` /
`ae1fcbe656a5ded4fba5b06c7d5114ee9652f9287187df66c65c3b2b8333a7b1`.
The full independent v2 review and v1 review/contracts were read; old local
acceptance does not substitute for the wider caller audit.

Actual v3 C locations:

* PLAY guard-false rollback290–295 and attach-failure rollback296–302 now inspect
  sticky server_clock.failed, client lifecycle_error/lease_uncertain and fresh
  MAX BEFORE actual unsubscribe. The second check is AFTER the real callback.
  Uncertainty nonreturns through original retain_span with actual demand/index;
  known-safe ordinary denial keeps actual unsubscribe/error-return policy.
* Open-frame per-playing guard-false687–691 checks those conditions BEFORE
  unsubscribe/playing/deferred-close changes. Actual retain_frame receives the
  real source with packet/native owner/frame bracket live. No pause/detach/STOP,
  source release or END occurs on that uncertain path.
* Post-next669–674 removes the old release-on-observed-MAX. Actual flags and
  sticky/fresh MAX retain actual frame/process without packet/native retirement.
* After actual precopy gate701, flags/sticky/fresh MAX are fenced703–705 BEFORE
  publish. The same checks709–711 gate the sole ordinary producer release712,
  including skipped-copy and ordinary-denial paths. This directly closes the
  gate-observes-MAX -> release -> later-retain ordering defect.
* Known-safe success remains real shared copy706 -> producer release712 -> exact
  END718. Safe ordinary auth/admission denials preserve current result handling,
  not retain-all. No packet metadata/payload read after release. A fault first
  observed afterward retains current state, not reconstructed earlier ownership.

[AUTHOR_CONTROL_FLOW_AUDIT.md](AUTHOR_CONTROL_FLOW_AUDIT.md) inventories ALL four
real unsubscribe sites, the sole remaining producer release, exact CPU release,
demand flags, playing/deferred/local clears, END/retire callers and cleanup tails.
No additional concrete bypass within the bounded observed-fault claim was found;
this is author static inspection, not independent sufficiency or runtime proof.

retain_span/retain_frame are unchanged actual nonreturning destinations: source
HOLD, actual typed sink/frame notification, existing native cleanup if needed,
repeated SIGSTOP fallback. No copied-token owner, NULL old callback, cast, new
external owner, close/free/exit/abort/unwind/END retry. Whole actual process/stack
remains retained under the sink contract; survival/recovery is not implemented.

## One compile/REL-only recipe, no candidate execution

Command: `python3 apps/rtsp-auth/rtsp-span-server-v3/build.py`.
Fresh exclusive `build01/`, no overwrite or retry loop. **PASS:** six real
production C compiles (server, accepted hub v2, accepted span; each HOST + retained
musl GCC14.3.0 ARMv5TE/marm/soft-float), two actual rooted REL links, 26 exit0 tool
commands. All six compile and two link logs empty. No prior failed v3 build.

Reused SAME21 of23 reviewed original runtime archive bodies per target, excluding
old server member9 and old packet_hub member17. All46 actual archive bodies matched
retained objects;42 selected bodies matched both extraction and v2 build01. Real
production defines/roles preserved, no HOST fixture/stub/whole archive/old REL.

V2 recipe's18 actual API roots and actual retained-relocation closure checks are
unchanged. Actual ELF-byte inspection additionally confirms nonempty real GLOBAL
FUNC roots/live APIs, no main/old server/old hub export, entry0/no program headers.
All three raw Digest UND names remain explicit LOCAL/NOTYPE/size0 with ZERO actual
retained relocation references. HOST2786/ARM2093 relocations inspected; no project
undefined relocation, while appropriate libc/compiler/linker references remain.

| REL (NOT executable) | Bytes | SHA-256 |
| --- | ---: | --- |
| HOST ELF64/x86-64 |259576| `a2bfabd0b9ae9a36f25648b7463ee5a387ae95721cb1e85a04dcfd0681f61bab` |
| ARM ELF32/ARM |172948| `a3853b7b491067f5ea7773242d88b198d92f58dd1de92f6c5799f27d731314db` |

Evidence:

* `build01/REPORT.json` SHA256
  `b5abd45f47ce98596d4f9005ad6536e32e9906c840e6e2f3951337ee3df53e51`:
  exact argv/cwd/environment/timestamps/consumed inputs/tools/support/log/artifact
  pins.203 inputs,251 new dependencies,363 reused dependencies,98 artifacts,
  26 command logs rehashed; dependency memberships checked exactly. Counts overlap,
  not whole tool-execution/native ABI/stack closure evidence.
* `FROZEN_INPUTS.json`:332 original files, exact membership AND bytes checked
  before/after build, covering frozen v1/v2 apps/docs/reviews and accepted hub/span.
* `AUTHOR_SOURCE_PROOF.json`: all C bytes outside the two functions identical;
  selected accepted cleanup/guard/retention functions exact; header comments only.
* `SOURCE_DIFF_PROOF.json`: all eight saved C/header/recipe/notice forward/inverse
  diffs regenerate and apply exactly in memory, restoring original bytes.
* `AUTHOR_BUILD_PIN_VERIFICATION.json`, `AUTHOR_ELF_ARCHIVE_PROOF.json`: targeted
  actual pins/memberships/archive bodies/ELF symbols and relocations, no execution.
* `SOURCE_INSPECTION_NOTE.md`: two failed author metadata inspections preserved
  (function extraction regex and link selector counting readelf -r); no source
  change or compile rerun for either. They are not relabelled passing executions.

## Remaining trusted conditions and stop

No candidate/helper import, C function/program/ELF/test/harness/callback, negative/
adversarial/mutation/fuzz case, backend/auth/provisioner/verifier/RNG/credential/
provider/encoder/media/native/network/UART/HTTP/RPC/hardware invocation occurred.
Only fresh offline recipe/stdlib evidence orchestration and allowed compiler/
linker/ar/nm/readelf children were used. Deferred coverage remains NOT RUN.

Trusted stable disjoint storage, true-fresh hub after last fork, no aliases/
reentry/signals/concurrency/FD mutation, synchronous metadata callbacks, actual
producer/callback ownership and explicit whole-process sink/lifetime remain caller
requirements. Added checks are only observed-fault fences, NOT whole-interval C1
prevention. Native use stays BLOCKED: original flat kind1/paid[1] wiring, producer
1MiB output/raw backing,128KiB cap,2MiB process/combined-load accounting, external
ownership/recovery/provisioning/main and native ABI/image qualification remain
open. The disjoint native-span-runtime candidate is NOT a dependency or adopted.

Parent's camera OFF/finalized23h and RAM75 C1/recovery block are inherited context,
not newly observed hardware facts. Original failures/HOLDs and all earlier failed
builds are untouched. Stop for independent ordinary review then parent disposition;
no source use, commit/publication, execution or automatic next task selected.
