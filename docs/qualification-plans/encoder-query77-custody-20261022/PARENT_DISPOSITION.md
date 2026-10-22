# Parent disposition — C1 v1 R1/R2 corrections required

2026-10-22; ENC-02 / BASE-01/05/07 / PLAT-01/06/08. Parent read the complete
independent review and actual validator/operation/failure paths. Freshly verified
20 reader inputs and three review outputs unchanged. Review seal SHA256:
`239b007833ca839d1c391b3594b4a654f915a0d53e856795c333e366ce4d58fd`.

Accept the **two correction-required static findings**, not C1 SOURCE/API use.
Neither finding is a runtime observation or a native authorization bypass:

- **R1:** the pure validator uses a supplied previous record instead of always
  enforcing original catch/counter baselines. Typed same-epoch records do not make
  that previous record valid. Validate its original relationships, then enforce
  original and previous constraints on the new observation, including no catch
  revival in bookkeeping mode and shrinking original/prior budget.
- **R2:** exceptional exits of admitted public operations skip the historical/
  root/source postcheck. Checking only the new private ledger does not satisfy
  the promised failed-call boundary. Attempt read-only postchecks on failure while
  retaining the primary error; record secondary failures only if safe. Poisoned
  writes must never be followed by retry/error append. Refused reentrant/terminal
  calls and not-yet-acquired constructor descriptions need no invented IO.

V1 source aggregate
`5c2d338973ab6384b504b6b52203b072e9ea04a3b9a357127638f1829b2ea286`
and all contract/syntax/inspection/review records remain frozen and unaccepted.
One parent lookup guessed CANDIDATE_REPORT.md; the actual BUILD_RESULT.md was then
read from the inventory. No candidate import/call, tests or private IO followed.

The CONTROL path conflict is separate. V1 correctly refuses its overbroad
both-roots-outside-Git contract. Parent's `C1_PATH_ROLE_AMENDMENT.md` under the
native-method docs permits proposing only exact source-bound in-place read-only
historical locking, while every new private output remains outside Git. No original
file migration, copy, replacement, mode repair or generic ignore-Git switch.

Parent selects fresh `apps/qualification/encoder-query77-custody-v2/` and matching
qualification docs for **R1/R2 plus that explicit path-role correction only**.
The revision remains source-only, no CLI/current-state/hardware/process/controller
interface. Static AST/syntax compilation is permitted; imports/library calls,
filesystem exercises and runtime tests are not selected. Actual implementation
must receive independent review; neither this disposition nor a receipt hash
establishes real epoch/source/recovery facts or E2/native admission.

Both actual historical HOLDs/private state remain unread and untouched. Query77
never booted; no staging/query/reset or physical request is selected. Original
controller/design/helper histories, source licenses, ordinary index and local-only
Git policy remain. Product/bootstrap/retention/backing/functional/native gates stay
open independently of this host bookkeeping correction.
