# Cold-entry23 v4 — focused ordinary SOURCE/API/method review

2026-10-23; BASE-01/05/07 / PLAT-06/08. **Narrow conditional SOURCE/API/method
sufficiency; no blocking ordinary defect identified in this delta.** This is not
runtime confirmation, current-state authentication, recovery coverage or native GO.
No invocation or hardware operation is selected by this review.

## Basis and exact public pins

Read full v4 source (565 lines), README, CONTRACT, delivered delta and pin manifest;
full v3 source/review/contract/parent disposition and README; inherited v1/v2
contracts; relevant original recorder arm/burst/status/stop/finalization source and
full classifier. Independently matched all five delivered byte-count/hash entries,
the actual unified v3→v4 diff against SOURCE_DELTA.patch, all14 non-main helper
bodies byte-for-byte, and all non-main top-level AST. AST parsing only; no candidate
or helper import, compilation, call or execution.

Paths below are workspace-relative; v4 metadata lives under
`docs/qualification-plans/qc07f-cold-entry23-v4-20261023/`.

| Input | SHA256 |
|---|---|
| `apps/qualification/qc07f-cold-entry23-v4/cold_entry.py` (30486 bytes) | `40794530f6ef7bb9fec15104fee5f3c06e841ba6a08947794041bac99b3cc8b5` |
| `apps/qualification/qc07f-cold-entry23-v4/README.md` | `ad0708cb8bee9268ed566f740dce38bf8183a0f1923be366a6a5ffab0ceaebdd` |
| v4 `CONTRACT.md` | `ab821b80736ff6bea7cdb3ee9815734593d4f4858e3918b6e6811406663d7d87` |
| v4 `SOURCE_DELTA.patch` | `2fe18e59a97b99abace53ebfef75dd0f29316db691e8310b1933e5f06645144a` |
| v4 `SOURCE_PINS.json` | `0cb5a81655fe05201a211a39b7b93783814aede6ebbddbf6a423fc6c6662eaba` |
| `apps/qualification/qc07f-cold-entry23-v3/cold_entry.py` | `a03c10ee651a820d93ff7dddd4b66027b2e0fd70dd284a4eb25fe60fba111954` |
| `docs/qualification-plans/qc07f-cold-entry23-v3-review-20261023/REVIEW.md` | `15edfe16eac8f654d0254f5bbbfaed52d02fcbe8eccd2b85401735fd081bfe69` |
| `docs/qualification-plans/qc07f-cold-entry23-v3-20261023/CONTRACT.md` | `0057b63f9bcba369b873303b56aa76b4680e74f0a5d1d229ee6190175bca55f8` |
| `docs/qualification-plans/qc07f-cold-entry23-v3-20261023/PARENT_DISPOSITION.md` | `69a120a3ab3b7ae5000b1fe18578f32006def5e85ae77bf23c9cfca431cfe84c` |
| `work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py` | `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00` |
| `scripts/qc07f_boot.py` (hashed; not executed) | `6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985` |
| `apps/qualification/encoder-query77-cold-context-v2/cold_context.py` | `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558` |

## Focused findings

- Lines444–453 mark ON uncertain before the single POST, acknowledge only its
  successful return, then move the existing nominal3s wait before the single strict
  GET. No ON GET retry/polling, synthetic state or weakened HTTP helper is added.
  A failed POST goes directly to cleanup; no cold observation is accepted without
  a successful strict GET reporting actual Boolean true.
- Lines270–272,401–404,450–452 and483–486 retain null until a strict GET returns,
  then store its actual Boolean before the expected-state check. POST acknowledgement
  Booleans remain separate and are not state evidence. These new fields contain
  no endpoint, response body, selector, credentials, capture bytes or digest.
- Lines467–489 snapshot the main exception's phase before OFF cleanup changes the
  last phase. One OFF POST remains followed by one GET even after POST uncertainty;
  the added nominal0.25s wait precedes that GET. OFF_POST_UNCERTAIN and
  OFF_UNCERTAIN_MANUAL_OFF_REQUIRED retain cleanup failures, including parse errors;
  neither a later OFF readback nor capture finalization clears an earlier error.
  `failure_phase` describes the main exception, not every cleanup failure or an
  exact trace of non-power steps; cleanup errors must also be read.
- Exact diff confirms no other behavioral change: immutable input/marker checks,
  typed tty association, actual Popen/PID/start/argv/peer custody, pre-ON arm/status
  guards, first360-byte19-record classification plus every118-byte trailer group
  for each additional completed19-byte packet, separate10s quiet, proved-OFF
  stop/actual wait0/final hashes and individual failure postchecks remain intact.

## Conditions and boundaries

Parent must separately select any ONE fresh23e transaction and revalidate current
serialized UART-only/sole-owner prerequisites, protected inputs/output, exact
source/interpreter/serial association and initial GET OFF. The already confirmed
QC07F-only outlet mapping and visible power-up are caller facts, not questions to
repeat or independent rail measurements. Trusted namespace/client/dependency
closure and limited visible fuser/cooperative-lock scope remain explicit conditions.
Nominal sleeps/timeouts and recorder duration do not establish hard deadlines,
host-hang/suspend recovery, privileged/global exclusion or whole-interval coverage.
Uncertain OFF retains manual-OFF handling; no automatic retry follows.

V3 foundation-20261023d remains FAILED/HOLD: supplied history reports initial OFF,
ON attempted, OFF attempted/read back, finalized owned child exit0, but0RX/0TX and
an arm-to-stop interval0.19894351s before its3s wait. Its generic exception cannot
locate the failed step. Async dispatch racing immediate ON GET remains a hypothesis;
0RX is neither a boot-failure diagnosis nor proof of absent power. Frozen23c/v3
failures and all old HOLDs stay unchanged. Last independent OFF/no-recorder state
is supplied context, not a fresh observation by this reviewer. No C1/clock-policy,
image/loader/staging, motor/NOR/media, native-watchdog or product gate is waived.

Only this fresh REVIEW.md was written. Public source/docs read/hash/diff/AST work
only; no private/proc/dev/HOLD-state/capture/network/HTTP/serial/RPC/lock access,
CLI/help/default/build/test, hardware effect, source/AGENTS/central-plan/Git/index
change, subworker, adversarial campaign or tool/provider/safeguard change occurred.
