# B08 R1/R2 HOST harness candidate — frozen for parent review

2026-10-22. **SOURCE + standalone HOST dummy-process evidence only.**
Scope: B08 / RP-08–09 / SEC-05/11 / STREAM-04/07; no station P1 work.

## Frozen handback

Candidate: `apps/rtsp-auth/isolated-server-host-tail/` (**41 public files**).
Aggregate source pin:
`2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f`.

Exact manifest and aggregation recipe:
`harness-correction/CANDIDATE_SOURCE_SEAL01.json`.
Recipe: SHA-256 of sorted workspace-relative UTF-8 paths, each followed by NUL
and the raw 32-byte SHA-256 of its file, concatenated. The manifest covers the
candidate directory, not this report or generated test evidence.

Review-entry guard pin (`host_guard.py`):
`2c05e83ba59e35601b4b79f34e669ebac39399f6ad461b8894d336eb02651df7`.

**Parent review pending; no full-builder/RTSP testing GO claimed.** No C/ARM
compilation or generator/real RTSP fixture/server execution occurred. No network,
target/UART/device operation or private/generated-startup input access occurred.
Only Python syntax checking and the new standalone dummy-process guard tests ran.
No Git/index, AGENTS, central ledger, original candidate, shared source, completed
source-review or historical HALT edits.

## Provenance and exact delta

`harness-correction/copy_baseline01.py` exclusively byte-copied all **35** sealed
public source/provenance/license files before edits. Candidate
`SOURCE_TAIL_PROVENANCE.json` records every original/copy/hash. Of those 35 files,
**29 remain identical**; six changed: README, build.py, candidate_auth.c,
client_test.py, fixture.c and integration_test.py. New files are that provenance
record, host_guard.py, host_process.py, prefix_receipt.c, guard_dummy.py and
guard_regression.py. Licenses, verifier overlay, server implementation, original
generator, crypto configurations, ABI header and all upstream copies are unchanged.

`harness-correction/frozen-tail-to-host-harness01.diff` contains the exact six-file
delta. The source seal rechecks all 35 originals, completed review artifacts,
reviewed verifier/source-header pins and HALT seal/diff as unchanged. The copied
README clearly labels its inherited remainder as historical documentation, not a
current run procedure. Copied `PROVENANCE.json` still points to the original
reviewed V3/V2 sources, copies and code-image paths; those are intentionally not
rewritten. Existing image/dependency checks remain in the unexecuted builder.

## R1 design and ownership argument

All corrective supervision is **Linux x86_64 nonroot HOST-only**, not a native
guard, daemon or target recovery mechanism.

- `host_guard.py:87–120`: a fresh dedicated process verifies subreaper setup,
  default SIGCHLD handling and no existing children before spawning anything.
  Parent death/SIGTERM/SIGINT requests bounded cleanup, not success.
- `host_guard.py:68–83,128–162`: Popen creates a new session. The supervisor
  retains its own leader PID unreaped, verifies SID=PGID=that PID and releases a
  private pipe barrier only after the launcher reports readiness. No workload
  executes before release. Launcher PDEATHSIG is armed with expected-parent
  recheck. No PID discovery, process-status scans, fuser/pkill or unrelated signal.
- `host_guard.py:40–65`: before release, an inherited no-new-privileges seccomp
  restriction denies setsid/setpgid/unshare/setns, foreign syscall architecture
  and x32 syscall entry. It prevents group/session escape across the controlled
  workload's fork/exec descendants. This is an additional HOST lifecycle filter,
  **not a replacement/bypass** of the unchanged isolated verifier's stricter filter.
- `host_guard.py:154–243`: only WNOWAIT observes leader completion before cleanup.
  Exactly one group TERM phase and one final group SIGKILL attempt occur with
  leader PID still held unreaped, preventing PGID reuse. **No signals follow the
  final kill.** Then actual waitpid reaps the leader and adopted descendants;
  ECHILD is required, not a timeout or successful kill call.
- Normal PASS requires natural leader exit 0, barrier admission, no adopted or
  signaled children, no interruption/error and actual ECHILD. Cleanup signals to
  a zombie-only group do not fabricate termination/retirement: any additional
  adopted child makes the run FAIL, including a child whose parent exited 0.
- Workload limits are bounded (builder campaigns retain 240s/180s); default TERM
  grace is 0.3s and reaping grace 2s. If final reaping is unknown, the supervisor
  writes **HOLD** and retains its ownership context without further signals,
  restart or false terminal handback. The builder must remain waiting; do **not**
  apply an external kill timeout to that owner. Kernel-uninterruptible tasks or
  uncatchable supervisor destruction are not proved recoverable. HOLD retention
  is source-reviewed here, not tested using intentionally unreapable processes.
- `host_process.py:28–104` provides direct own-pidfd courtesy TERM/actual timed
  wait, bounded partial-READY and stdout reads. There is **no local SIGKILL**:
  only the outer supervisor owns final escalation/reaping. A local timeout is
  failure, not retirement. Constructor cleanup lives inside `Server.__init__`
  (`client_test.py:69–86`), so it does not depend on successful object return.
  Live-fixture cleanup in finish raises; it cannot make normal PASS.
- HOST fixture parent identity is passed as an expected nonsecret parent PID;
  C arms PDEATHSIG with pre/post parent checks (`fixture.c:209–218`). Owner-pipe
  EOF is failure only in the HOST build (`:103–110`). Retained verifier uncertainty
  explicitly prints fixed HOLD and exits 74 in HOST, without freeing uncertain
  context or claiming retirement (`:282–294`). Its child's PDEATHSIG and the outer
  subreaper must still deliver actual waits. Strict/native retention remains HOLD.

`build.py:17–24` defaults to refusal without the exact supplied reviewed guard
hash, before output creation. That pin and the harness guard-entry environment
marker are trusted workflow acknowledgments, **not adversarial authentication
capabilities**. Both real Python campaigns use the supervisor (`:29–43,84–93`)
and require its successful retirement receipt. Output is exclusively fresh
`docs/qualification-plans/rtsp-tail-20261022/harness-correction/runs/<name>`, never
the legacy evidence directory. ARM CLI/build paths are removed. Compiler commands
and scalar/link checks retain their existing command timeouts;
the corrective supervisor's scope is campaign/fixture/verifier lifecycle.

## R2 actual predicate, not generic refusal

`candidate_auth.c:65–96` retains the same accepted prefix predicate set but splits
domain and credential-generation rejection after other ABI/routing/rate/account
checks. HOST-only observer calls occur **at the actual domain/generation branches**:
stage 1 = domain denial, 2 = generation denial, 3 = accepted prefix. Only fixed
stage and Boolean match scalars are exported, never identity/generation values,
credential bytes or startup-record hashes. No callback grants permission.

`prefix_receipt.c` delegates real fork and records attempts/actual successful
parent-side launches. It does not mock rv_start/READY/ACK or manufacture exits.
Actual wait0 and uncertainty observers remain the original reviewed overlay.

`client_test.py:355–388` requires these separate controls:

| Case | Stage | Generation/domain match | Actual launches | Source effects |
| --- | --- | --- | --- | --- |
| Valid RTSP-domain SHA-256 control | 3 | 1 / 1 | exactly 1, owned normal wait0 | zero peers/SDP/setup/play/RTP/source starts/next/BEGIN |
| BAD-DOMAIN | 1 | **1 / 0** | attempts=launched=owned exits=wait0=signaled=0 | starts/next/BEGIN/attach/CRC reads=0 |
| BAD-GENERATION | 2 | **0 / 1** | all zero | all zero |
| BAD-RNG | 3 | 1 / 1 | all zero | all zero |

The first two use otherwise-equal SHA-256/default bindings, **including the same
owned just-retired loopback port**. Only generator domain differs; frozen creator
source still supplies generation 1 in both. BAD-GENERATION separately restores
the old generation-2 configuration versus generation-1 record negative, without
misnaming it BAD-DOMAIN (`fixture.c:237–239,259`). Negative cases additionally
require exactly one 348-byte prefix pread, exit 2/no READY and no retained verifier.
A generic init failure no longer satisfies the stage oracle. An accepted domain
or child-side-only rejection cannot impersonate stage-1 pre-child rejection.

Future results include a scalar `PREFIX-CONTROL-TABLE.json`. Python does not
read/dump/hash generated records; the legitimate server-prefix and isolated-child
reads remain unchanged parts of the original architecture. No child-engine domain
coverage, credential rotation, encryption or native policy qualification is claimed.
**These C/oracle cases are implemented but UNBUILT/UNTESTED in this handback.**

## Commands and bounded observations

Python: **3.14.8**, reported GCC 16.2.1 build; no C compiler was invoked.
Commands from the workspace root, with fresh output names:

```text
python3 -B docs/qualification-plans/rtsp-tail-20261022/harness-correction/copy_baseline01.py
python3 -B docs/qualification-plans/rtsp-tail-20261022/harness-correction/syntax_check01.py syntax01
python3 -B apps/rtsp-auth/isolated-server-host-tail/guard_regression.py dummy-guard01
python3 -B docs/qualification-plans/rtsp-tail-20261022/harness-correction/syntax_check01.py syntax02
python3 -B apps/rtsp-auth/isolated-server-host-tail/guard_regression.py dummy-guard02
python3 -B docs/qualification-plans/rtsp-tail-20261022/harness-correction/seal_candidate01.py
```

Both syntax receipts report **9 Python files PASS**, using in-memory compile only
(no imports/execution/pyc). First dummy run passed 12 cases on the earlier local
TERM/KILL helper; retained as development evidence, **not the final helper pin**.
Final `dummy-guard02/REPORT.json` pins the current four guard/test files and reports
**12/12 expected outcomes**, 03:51:48–03:51:56 UTC. Each case observed owned session,
barrier, subreaper, leader reaping and ECHILD. No process discovery was used.

| Final dummy case | Observed leader exit | Adopted children actually reaped | Guard normal PASS |
| --- | --- | --- | --- |
| normal / normal fixture / inherited escape denial | 0 / 0 / 0 | 0 / 0 / 0 | yes, three controls |
| constructor failure with grandchild | 2 | 2 | no |
| partial READY / no READY | 2 / 2 | 1 / 1 | no |
| ignored SIGTERM / fixture wait timeout | 2 / 2 | 1 / 1 | no |
| late child at TERM / supervisor deadline | SIGKILL / SIGKILL | 1 / 1 | no |
| retained uncertainty | 74 | 1 | no |
| exit-0 workload with orphan | 0 | 1 | **no** |

All final cases completed in **415–918ms** wall time each on this host. Negative
outcomes are expected guard-regression successes, not normal lifecycle passes.
Session/group escape denial was also exercised after actual fork+exec. No RTSP
module, generator, fixture, code image, startup record or network operation was
used by those tests. The final C-only EOF conditional edit postdates syntax02 but
does not change any Python/dummy-test input; it is covered by the source seal only.

## Outstanding gates / one next task

**Next: parent independently review the exact 41-file seal, ownership/escape/
HOLD logic and actual C predicate/oracle delta; reproduce only standalone dummy
guards under a fresh name if desired.** This author-run evidence is not independent
review. The full builder, C/C++ links, sanitizer campaigns, original/pair/negative
oracles and actual host child hardening/filter compatibility remain unexecuted
until parent source review admits them. The supplied guard pin alone is not that
admission. Native verifier/media/memory/product/NVR/G3/G8 and station P1 gates
are unchanged. Original tested05 and HALT claims retain their historical scope.
