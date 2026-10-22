# B08 RTSP tail — independent SOURCE review

Reviewed 2026-10-22; static pin comparison observed **03:31:17 UTC**.
Scope: **B08 / RP-08–09 / SEC-05/11 / STREAM-04/07** only.

## Decision

**SOURCE finding: the three frozen functional edits correctly replace the old
generation-mismatch input with a foreign UI-domain input, with generation otherwise
matching.** The actual rejection is the **server's pre-child routing-prefix domain
predicate**, not an executed verifier-engine domain test.

**NO-GO for an unattended HOST reproduction using the current builder/harness
unchanged.** Failure/timeout handling does not bound the lifetime of all owned
fixture processes, and the refusal oracle does not identify the rejection stage.
These are source-review findings, not observed test failures. A separately reviewed
owned-process guard and specific scalar refusal oracle are needed before execution.
No builder, compiler, test, generator, listener, image, target, UART, network,
device, private input or generated startup record was accessed/executed here.
No Git/index or central-plan changes. Station P1 was neither inspected nor worked.

## Exact public pins and historical boundary

All paths below are workspace-relative. In source anchors, `A/` means
`apps/rtsp-auth/isolated-server-candidate/`; `V/` means
`apps/rtsp-auth/isolated-verifier/`.

| Public input | SHA-256 observed |
| --- | --- |
| `evidence/delegated/20261012-rtsp-isolated-server/HALT-SEAL.json` | `6db5315f9b938c7ef5d0d19982149f984b917365e4bbb59bb999eaf387314111` |
| `tested05-to-frozen-HALT.diff` in that evidence directory | `51815a627c40a5b5202eb24789c2904b046acca0203a1327e6f3cb1d517808d4` |
| `A/public_dummy_material.c` | `4835dac5ee06ac69347c91689b2d47b199b06a7434218ea84ae0f2f652f3a43a` |
| `A/fixture.c` | `47787404e82fa997e57d329e5f622b834ece3e6da7fbeb753a9c74224074d018` |
| `A/client_test.py` | `ec0abab0d7fea922d3a981f1067bb77799fe3412cd12ba13b124c71db6111f19` |
| `A/build.py` | `e64ff1f9c824a801a1450dde7a64d95a4bdc6da96886679523c9987eccca3de5` |
| `A/candidate_auth.c` | `25fcd9c8994cdf76331ee4e9657712aea657421b56d64c9f72fba8717cc31fe8` |
| `A/verifier_adapter_overlay.c` | `a47881ae429cb4b73e8f501e0261b01a2837ef25831d22df6f0e96cf2c3abb1f` |
| `V/engine.c` | `cf298b8096c66859806c6e0c0b449592ac4081f05ad814f21fee35e97262c396` |
| `V/child.c` | `b317bac6512a3795fa7b8c82ae339366750dbf39960642d97e53d69d1d3e4ad7` |
| `V/verifier.h` | `3f4a97a12ed9acafe678e720b473c798c75b170648e6a6de076a1017ff0ad900` |

`source-review/public-source-pins01.json` contains all **35** seal-listed public
source/provenance/license pins, tested05/frozen/current hashes, additional reviewed
source/header pins and all **13** provenance-source comparisons. All frozen pins
match the seal; all 35 current files match frozen; all 13 provenance sources match.
This is an explicit public-source list, not an exhaustive current-directory or
dependency inventory. No generated startup file or image bytes were read/hashed.

Recomputing the tested05→frozen textual diff from the explicitly named public
input snapshots produces a **byte-identical** historical diff. Exactly four files
differ: README documentation, plus these three functional edits:

1. `A/client_test.py:81` passes `BAD-DOMAIN` to the PUBLICDUMMY creator instead of
   silently converting it to SHA-256 normal-domain material.
2. `A/public_dummy_material.c:22,27` accepts that mode as SHA-256 but sets the
   marker to `CAMERA_ACCESS_UI` instead of `CAMERA_ACCESS_RTSP`.
3. `A/fixture.c:240` always configures credential generation 1, replacing
   `f.bootstrap_bad?2:1`. The flag is still set at line 220, but is no longer used
   to select a generation.

The tested05 public source hashes for those three files were respectively
`412df7559696deb576b976493ab54aa4cf98ee3bdcec9d61c10e0f2ac9f09e07`,
`508d069100201e29cd12f8dd33f55b3b6a349ac514e8989893641be49d76806e`,
and `412cab4984a10090b4d3d645454cb7b7cc68fb961023db5d0e365ae3fe46a299`.
The old BAD-DOMAIN label therefore covered **RTSP-domain generation 1 material
against generation 2 configuration**. That historical result is not relabelled.
The frozen source removes that particular generation-negative case; current
generation-mismatch regression coverage cannot be inherited from its old label.

HALT-SEAL lines 3–18, 19–43, 54–65 distinguish tested integration05 from the
unbuilt tail. Its old optimized/sanitized counts remain historical reported HOST
results only; this review neither reruns nor newly accepts them.

## Domain reachability and absence of protected effects

The static call/data chain is concrete, not a fake verifier response:

- `A/client_test.py:326–330` selects `BAD-DOMAIN` with `refused=True`.
  Lines 81–90 run the separate creator, then pass read-only, no-follow material
  and code-image descriptors into the fixture. No startup-record contents are
  read by the Python harness.
- `A/public_dummy_material.c:20–36` zero-initializes the real `rv_material` ABI;
  uses SHA-256, epoch **444**, boot generation **42**, credential generation **1**,
  the PUBLICDUMMY account/routing identities, matching TTL/rates and loopback
  live/track routes. Only the domain differs from the positive SHA-256 case.
  `framework/include/camera/access_policy.h:16–18` makes UI and RTSP distinct
  enum values (1 and 3). Public fixture HA1 computation remains real, not mocked.
- `A/fixture.c:218–221,240–255` selects SHA-256 for BAD-DOMAIN and configures the
  same epoch/boot/credential generation/IDs/rates and `127.0.0.1` route. There is
  no BAD-DOMAIN-specific manufactured backend success/failure return.
- Subject to the preceding normal image/FD/configuration prerequisites passing,
  `A/candidate_auth.c:62–71` preads only the fixed prefix and evaluates
  `prefix.domain!=CAMERA_ACCESS_RTSP` at **line 65**. C short-circuit evaluation
  rejects there; the generations at line 66 would match, rather than supplying
  the historical alternative failure. It returns its initial `CAMERA_INVALID`
  through the wipe/failure latch at line 83. Public-source equality establishes
  these constants; no generated record was inspected to make this finding.
- `rv_start` is only reached at `A/candidate_auth.c:81`. Server backend init is
  before source-guard binding, socket creation, bind/listen or READY callback
  (`A/rtsp_server_candidate.c:430–453`). The domain rejection thus has no child,
  listener, challenge, grant or source/media effect. Fixture counters/exit are
  printed at `A/fixture.c:260–269` and its ordinary failed serve exit is 2.
- `V/engine.c:49` independently contains the same RTSP-domain requirement;
  `V/child.c:32–40` calls it only after hardening/READY and a full material read.
  **This tail's BAD-DOMAIN case never reaches that child code.** Claiming child
  parsing/domain enforcement was exercised by it would be incorrect.

## Findings requiring disposition before reproduction

### R1 — HIGH: builder timeout is not owned-process-tree retirement

`A/client_test.py:87–105` spawns the fixture inside `Server.__init__`, then waits
for refusal or READY. A wait/select/read/check failure there has no enclosing
cleanup of the new process. Callers cannot run `s.finish()` on an object whose
constructor did not return. Normal `campaign` exception handling at lines 566–571
only writes scalar failure and exits.

Even after construction, `finish` lines 161–166 only sends SIGTERM to its exact
fixture handle and performs another timed wait; there is no owned hard-kill/reap
fallback. `A/fixture.c:263` intentionally waits forever on retained verifier
uncertainty; its SIGTERM handler only sets `stopped`, which that loop does not
check. The fixture is preserving uncertainty, not providing bounded recovery.

`A/build.py:24–29,71,77` uses `subprocess.run(..., timeout=...)` for the Python
campaign. Its 240/180-second deadlines kill/wait that immediate Python process,
not every descendant. The fixture has no parent-death hook; its stdin EOF path
simply returns from `owner_poll` (`A/fixture.c:100–102`), leaving the serve loop
alive. A fixture orphan can therefore keep its loopback listener and verifier
alive. The verifier's real PDEATHSIG is tied to the **fixture**, not to Python
(`A/verifier_adapter_overlay.c:105–121`); it does not close this gap. Some client
inner receive loops also rely on the outer campaign timeout for a hard total
bound. These are reachable failure-path gaps, not a claim that old runs leaked.

Required: before execution, review a supervisor/cleanup contract that retains
ownership across constructor failure and campaign timeout, bounds all own
fixture/child lifetimes, and observes their actual exit. Do not use PID discovery,
unrelated process signals, broad kill patterns or elapsed time as retirement proof.
Unknown retirement remains HOLD. The unchanged builder alone is insufficient.

### R2 — MEDIUM: generic refusal is not a domain-specific test oracle

`A/client_test.py:97–101` accepts any nonzero exit with no READY initially;
`finish:136–160` strengthens that to expected fixture exit 2, scalar source
invariants and no retained verifier. `campaign:329` checks zero starts/next.
Thus a signal crash generally does **not** pass, and there is no unconditional
PASS or crypto stub. However unrelated well-formed init failures (e.g. another
prefix mismatch, invalid image metadata or unavailable child startup) can also
produce exit 2/no READY/zero source. Refused cases skip the owned-exit/wait0 checks
at lines 143–148. Their material-read scalar line is not parsed/asserted either.

The full campaign's positive SHA-256/MD5 cases help detect a globally broken
environment, but do not identify which predicate rejected BAD-DOMAIN. There is no
specific refusal-stage assertion or targeted control that would prevent the old
generation-mismatch attribution error from recurring in a future fixture edit.

Required for B08 evidence: a nonsecret stage-specific observation of the actual
prefix-domain predicate, matched generation/ABI inputs from source, zero child
launch/owned exits and zero protected effects. Include a paired otherwise-equal
valid RTSP-domain control. An oracle/control failure must yield FAIL/HOLD, not
fall back to generic refusal. Do not inspect/dump/hash generated startup records
or export credential values merely to strengthen this assertion. Child-engine
domain rejection, if desired, needs a separately scoped actual-engine case.

## Existing process/endpoint/credential controls that are sound in scope

- Creator requires explicit PUBLICDUMMY compile/CLI modes and nonroot real UID,
  validates an owned exact-0700 directory, exclusively creates mode-0600 material,
  closes the writer and wipes its local structure (`A/public_dummy_material.c:
  5–7,20–39`). There is no private mode or password in argv. Its only output file
  is the intended dummy startup record; **no stdout/stderr credential output**.
  Same-UID malicious writers are outside this trusted-bootstrap fixture model.
- Fixture rejects root (`A/fixture.c:201`); HOST backend additionally rejects UID/
  GID mismatch, root and wrong image/profile before material read
  (`A/candidate_auth.c:49–59`). The strict-native build's nonroot refusal is not
  native execution. No sudo or target operation appears in the reviewed builder.
- Fixture and server independently enforce literal `127.0.0.1`, nonprivileged
  ports and bounded buffers/deadlines/four-peer capacities. Python only reserves
  and connects its own loopback port (`A/client_test.py:70–73,171`), not existing
  listeners. Release between reserve/bind permits a local port collision: treat
  failure as failure, not authority to connect to an alternate endpoint.
- Actual verifier launch uses validated preopened IMAGE, a fresh owned fork,
  pidfd-before-release barrier, fixed argv/empty environment, closed stdio,
  privilege/capability restrictions and PDEATHSIG. Signaling is via own pidfd;
  actual `waitpid` establishes retirement (`A/verifier_adapter_overlay.c:33–44,
  105–121,164–226,287–306`). Host hardening uses separate 4MiB locked class,
  native stays 2MiB (`V/isolation.c:80–127`). No host resource availability was
  queried or assumed here. Unsupported hardening must fail, never fall back.
- Parent's material read is bounded before `username` (348-byte prefix by ABI),
  and wipes prefix/probe (`A/candidate_auth.c:20–25,64,83`). `A/pread_receipt.c:
  7–13` delegates the real pread and measures only count/max requested length.
  The fixture's `parent_secret_bytes_read=0` at line 261 is **literal text**, not
  an independent measured absence-of-all-reads proof; line 262 bounds max pread
  length but not every possible I/O API/offset. Current source supports the
  bounded-prefix claim; do not promote that literal to a general dynamic audit.
- Wire Digest material is sent only over owned plaintext loopback as required
  by this test; **Digest is not encryption**. Server logs inspected here contain
  scalar counters/errors, not Authorization/challenges/session values. The client
  catches runtime exceptions as scalar failure types, checks forbidden output
  prefixes and requires creator stdout/stderr empty (`A/client_test.py:81–83,
  157–160,566–571`). This is source-supported output discipline, not newly observed
  runtime leak freedom; prefix scanning is not a complete secret-leak detector.
- Builder uses fresh exclusive outputs, public source/dependency/image pins,
  compile/link closure and before/after source checks (`A/build.py:15–29,47–64,
  98–115`). Source snapshots are taken before fixture generation; no generated
  startup-record hashing is requested. Code-image hashes are not credential hashes.

## Alias, status and error distinctions

`framework/include/camera/rtsp_digest_wire.h:13–16` and `V/verifier.h:20` show all
the overlapping namespaces: wire INVALID/BUSY = -100; MISSING/LOST = -101;
THROTTLED/BAD = -102; CLOCK/DENIED = -103; SPACE/UNCERTAIN = -104. They cannot
identify transport, policy, retirement or credential-domain cause by number alone.

The recomputed `source-review/adapter-overlay01.diff` confirms only actual owned
wait-status capture and three scoped observers were added to the pinned V2 adapter;
no launch/IPC/hardening implementation was replaced. `rv_complete` retains pending
on transport/ACK failure and clears it only after exact RESULT + RETIRE_RESULT ACK
(`A/verifier_adapter_overlay.c:260–277`). `A/candidate_auth.c:164–180` consults that
pending observer instead of equating -101 with transport failure. Zero pending is
**retirement of a result, not permission**; only RV_OK + token + fresh child CHECK
can grant (`:180–183`). The observer also avoids an infinite pending retry based
solely on RV_MORE. Current source does not manufacture a passed child result.

Actual wait0 is captured only from successful own-child waitpid and tested with
WIFEXITED/WEXITSTATUS (`A/verifier_adapter_overlay.c:35–37,315`;
`A/wait_receipt.c:6–10`). Backend destroy requires both BYE/wipe and real wait0
for normally initialized success (`A/candidate_auth.c:85–94`). Known signaled/
nonzero exit can release dead owned context but does not become a clean success;
uncertain retirement retains context. These guarantees do not supervise the Python
harness's direct fixture child and therefore do not cure R1.

The negative CHECK used only for source-free 401 accepts RV_BAD as a denial,
not a media grant (`A/candidate_auth.c:136–152`); child challenge still validates
connection/rate/generation. Error reasons are not separately exported; stale=true,
public principal export and password-rekey/rotation are not proved. REVOKE fences
the lifetime (`A/candidate_auth.c:185–190`; overlay `:253–258,279–285`), not a
credential-change implementation. BAD-DOMAIN fails before these wire statuses;
its generic serve error is not an RV-domain-rejection receipt.

## Evidence and limits

Only new files in this report's directory and `source-review/` were written.
Static collection command, Python version, timestamp and collector hash are in
`source-review/public-source-pins01.json`. `collect_public_pins01.py` imports only
standard-library modules; it never imports project builders, runs project code,
enumerates run outputs or reads generated records. Diff outputs and JSON use
exclusive creation. Historical source/HALT files were read-only and preserved.

This review establishes neither a fresh HOST PASS nor full source/security audit,
native root/groups0/READY15/2MiB operation, producer/DMA/retirement, cold entropy,
native media, memory fit, private provisioning, actual NVR compatibility, adoption,
G3/G8 or product GO. B08 remains open pending guarded reproduction and parent
disposition. No old result or blocked station lane is completed by this document.

**One next task:** parent selects a fresh, isolated **RTSP HOST harness/entry
safety correction and review** for R1/R2 (owned timeout/constructor cleanup plus
domain-specific scalar oracle and valid-domain control), preserving all frozen
tail/HALT evidence; only after that review consider a fresh nonroot localhost-only
builder reproduction. No native work is selected.
