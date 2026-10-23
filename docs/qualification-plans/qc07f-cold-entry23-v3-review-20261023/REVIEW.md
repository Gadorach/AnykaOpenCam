# Cold-entry23 v3 — focused ordinary R2 SOURCE/API/method review

2026-10-23; BASE-01/05/07 / PLAT-06/08. **Conditional bounded SOURCE/API/method
sufficiency: R2 corrected, R1 preserved; no further correction identified in this
scope.** Not runtime confirmation, hardware admission or a queued cold transaction.
V1/v2, their reviews/findings and the original23c EOF-caller failure remain frozen.

## Basis and exact public pins

Read all545 lines of actual v3 source, its README, CONTRACT, SOURCE_DELTA.patch and
SOURCE_PINS.json. This continues the same session's full v1/v2/original-recorder/
RPC/classifier/method review and focused R2 tty-source finding. Independently
verified public bytes only: remove exactly the new helper and reverse its two
callsites in memory, and the result equals frozen v2 byte-for-byte. Independently
generated unified diff exactly matches the delivered patch; all seven recorded
hash/byte-count entries match. No other source modification exists in this delta.

| Path | SHA256 |
|---|---|
| `apps/qualification/qc07f-cold-entry23-v3/cold_entry.py` (545 lines,29265 bytes) | `a03c10ee651a820d93ff7dddd4b66027b2e0fd70dd284a4eb25fe60fba111954` |
| `apps/qualification/qc07f-cold-entry23-v3/README.md` | `33d73c6ac24edc68cee28f6a14a80d757a3150e740fccb49c1e68303d5d79065` |
| `docs/qualification-plans/qc07f-cold-entry23-v3-20261023/CONTRACT.md` | `0057b63f9bcba369b873303b56aa76b4680e74f0a5d1d229ee6190175bca55f8` |
| `docs/qualification-plans/qc07f-cold-entry23-v3-20261023/SOURCE_DELTA.patch` | `a7497e7ccb7fdb641c46dc3fbc0e5f462bbf77751f439ed690ca3c41c2e2ed1c` |
| `docs/qualification-plans/qc07f-cold-entry23-v3-20261023/SOURCE_PINS.json` | `e756d7072e51931e080bdadf8c06e88af75dfdafdf1a3e7c288b26aa34ec41e6` |
| `apps/qualification/qc07f-cold-entry23-v2/cold_entry.py` | `0d46fda61ced57bbbaa768d84f552b3d2d3f1bbc107f4d6a7f6f0cacf59e6bcf` |
| `docs/qualification-plans/qc07f-cold-entry23-v2-review-20261023/REVIEW.md` | `c207c784ccc1c231d5c1a4f8d67a6bcaf70f55b54b0f986626d107c84df43317` |
| `docs/qualification-plans/qc07f-cold-entry23-v2-r2-review-20261023/REVIEW.md` | `da9d50389203779e945596ea897757d2f3c41221d1f053bf29a6894b597278ea` |

Frozen v1 source/review hashes also remain respectively `03cc62101a4da20d2c6896e1f990ef13d85b4a12b427c3d1162d62e3e78b3bad`
and `d899403829951a060e721cf927d293212acb81be60b8ef723efa1cdd3ea0ff78`.

## Focused disposition

- `tty_association:60–64` explicitly requires character-device type and retains
  filesystem device/inode, UID/GID, full mode/nlink, **rdev** and nanosecond ctime.
  Only this device-role tuple omits normal-IO mtime and irrelevant content size.
  Initial `port_meta` (376) and live `child_identity` (284) consistently use it;
  exact selected-port resolution and existing type validation remain. No baseline
  refresh, timestamp mutation, ignored mismatch or replacement-device fallback is
  introduced. The corrected check also governs the pre-stop child validation (474).
- `identity:55–57` and all Retained regular/source/private/historical-marker FD/
  path/ancestry/byte/hash/size/mtime/ctime checks remain exactly unchanged. The fix
  is neither a global metadata exception nor permission to weaken immutable inputs.
- R1 HTTP code (169–190) is byte-identical to v2: POST is bounded HTTP200/no Location
  acknowledgement returning no state; GET retains strict JSON/entity/type/state
  checks. Initial GET OFF, pre-POST ON uncertainty marking and separate actual GET
  ON (439–441), single OFF plus independent GET OFF even after error (459–470),
  and sticky failures remain intact.
- Exact source pins, private roles, read-only historical locks, actual Popen/argv/
  PID/start/peer custody, armed zero-RX/TX checks, full first360-byte19-record plus
  every118-byte trailer group/actual1..3 completed19-byte packet lineage, separate
  nominal10s quiet, proven-OFF owned stop/wait0/final hashes/socket checks and
  independent failure postchecks are unchanged by exact reverse equality.

Conditional sufficiency requires parent-maintained trusted namespace/client/bench
serialization, reviewed interpreter/dependency closure, protected private inputs,
and fresh actual UART-only wiring/QC07F-only outlet association/prerequisites.
Metadata/config assertions are not physical measurements or permission. Limited
visible fuser/cooperative locks are not privileged/global exclusion; serial-open
flush/modem-line uncertainty, nominal host timeouts, suspend/hang/failure, absent
independent rail measurement and all-domain recovery limits remain. No strict C1/
native-clock, staging/boot/image/MMIO/motor/NOR/media/product gate is waived.
Parent AST/syntax evidence is cited, not rerun; no functional result is supplied.
Parent disposition and revalidation must separately precede any ONE operation;
this review queues no ON/capture/retry and relabels no earlier failure.

Only this new REVIEW.md was written. Public text/hash/diff verification only; no
candidate/helper compile/import/call/run/test/help/default, runtime/proc/dev/private/
capture/HOLD-state/socket/HTTP/serial/lock access or hardware action occurred. No
adversarial/mutation campaign, old source/review/AGENTS/central-plan/Git/index/model/
tool/provider/safeguard change, framework or subworker occurred.
