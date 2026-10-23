# Passive-only caller profile — ordinary source/method review

2026-10-23 (task context). BASE-01/05/07. **Conditional method sufficiency for
the narrow historical0644 read-only role; NOT live capture admission.** No marker
mode correction or recorder source change is required for that distinction.
Parent independently disposes and selects any operation; this review queues none.

## Inspected basis

Read both execution-20261023 parent documents, the complete353-line explicit-port
recorder, its ADAPTER_CONTRACT/parent disposition/independent review, historical
`ram76-resume22b/preserved_hold.py`, and C1v2's actual root/marker acquisition and
postchecks (custody.py:304–398,478–499,548–565), plus its correction contract and
disposition. Public text/hash inspection only; no import/call/build/test, private
evidence/state/control/proc/dev/lock/serial/RPC/network/credential access.

SHA256 checked with `sha256sum` (workspace-relative paths):

| Input | SHA256 |
|---|---|
| `apps/qualification/encoder-query77-recorder-explicit-port/uart_capture.py` | `c8433e225675bf16ca153d3b2c700cb4d3ab80dd70083d4e4a7bc19dffc19445` |
| `apps/qualification/encoder-query77-custody-v2/custody.py` | `d60176e2d2725460a22c82fee6ce1068a82a07a976daddb7287c52daf3ade16f` |
| `evidence/qualification/qc07f/execution-20261022/ram76-resume22b/preserved_hold.py` | `ad19370f769384cf9513e2a8288b97fc2836f05a0764e148b28537924ef0d9e6` |
| `docs/qualification-plans/execution-20261023/CURRENT_STATE_PLAN.md` | `6fe919b70f28ec706a5f514fe9a01ba326f2233a407ce4a5cadc61285715a768` |
| `docs/qualification-plans/execution-20261023/PREFLIGHT_REFUSAL.md` | `d51b9b477f30693ca99f07225b01731591f0ec7217d488217803090fdd7f4bc4` |

The reported preflight refusal and host observations are parent evidence, not
independently reproduced facts. No inline caller implementation was supplied for
line-by-line review; conclusions below concern its specified method only.

## Findings and minimum conditions

1. **0644 is not a write permission or intrinsically a blocker here.** For the
   exact original radio and empty GUI-instance files, owned regular/nlink1/0644
   beneath the exact owned0700 canonical control root is consistent with the
   specified nonsecret logical-marker role. Require trusted root/current-UID,
   non-group/world-writable nonsymlink ancestry and serialized namespace/mount
   custody. It is not a general0644/private-data exception. Pin the exact instance
   filename too; no discovery, aliases, copied markers or alternate roots.
2. **Retain actual historical objects, not merely equivalent JSON.** Read-only
   NOFOLLOW/CLOEXEC (prefer NONBLOCK plus pre-open regular-file checks) and
   nonblocking exclusive flock are suitable on the local Linux file role. Lock
   radio then reset, and exclude the GUI instance with its exact existing lock;
   retain all descriptions in the parent through child completion/cleanup. Verify
   pre-open/path/FD device+inode association, owner/mode/nlink, bounded exact logical
   records and raw-byte baselines; instance must remain empty. Recheck each acquired
   file and ancestry independently on success/failure before releasing locks. Never
   write/chmod/truncate/clear/replace historical files. Reset remains exact0600.
3. **C1 has a different, unchanged contract.** custody.py:484–492 explicitly
   requires0600 for BOTH historical HOLDs, and `_regular`/postchecks retain that
   requirement. Historical preserved_hold.py:27–49 did not require0600 on the
   original radio file; it opened it r+ and compared bytes while imposing0600 on
   the subordinate. That explains a source distinction, not runtime provenance
   proof or permission to reuse that effectful helper. Do not call/adopt/relax C1,
   borrow its clock/epoch/lease claims, or treat this profile as its qualification.
4. **RX-only is an orchestration restriction, not a recorder capability fence.**
   recorder:123–136 requests1152008N1/no flow control, DTR/RTS false while closed,
   exclusive open, then creates a0600 control socket. With initial arm=None,
   burst=[] and inspecting=False, RX alone selects no transmit path. Nevertheless
   socket handlers still permit protected TX/catch; locks do not gate those RPCs.
   Parent must exclude ALL clients/GUI/controllers throughout, including same-UID
   ones, not merely promise not to invoke the request CLI. Some RPCs (e.g. status)
   leave no control event; clean logs cannot prove no RPC. If trusted serialization
   cannot be maintained, this unchanged recorder is **blocked** for this profile.
5. **No privileged owner or electrical proof is inferred.** Negative unprivileged
   fuser/visible-FD checks, absent historical PID/socket and cooperative locks plus
   exclusive open support only that trusted serialized environment, not global
   exclusion of uncooperative/privileged tty users. Limited root visibility alone
   need not defeat this conditional passive method; unexplained owners or inability
   to establish bench custody do. Parent must freshly establish the exact historical
   by-id→device association (the source validator is lexical only), no competing
   operation, and retained UART-only wiring with no reset/boot/adapter-power leads.
   DTR/RTS requests do not prove glitch-free opening; initial input flush loses
   earlier bytes. This is protocol-RX-only, not zero electrical/host-side effect.
   If retained wiring cannot be established, block before open; no metadata or broad
   hardware approval substitutes for it. No new waveform measurement is claimed.
6. **One bounded observation, no escalation.** After parent disposition and current
   prerequisites, the specified scope is one explicit120-second full-private capture,
   new foundation part, fresh0700 directory outside Git, all new regular outputs
   (including console/errors/phase record)0600. Existing socket conflict refuses;
   no takeover/cleanup of unfamiliar artifacts. Parent waits without UART/RPC/status
   polling/helper edits. The monotonic loop duration is nominal, not a hard120-second
   wall-clock deadline including setup/suspend/cleanup or any recovery coverage.
   Require normal exit, opened/capture_end evidence, zero TX/redaction, no arm/TX/
   stop/control-error events, matching final raw/event pins and unchanged original
   files. These are postflight classifications, not prevention or proof of no RPC.
   Preserve partial/error outputs if setup/finalization fails; never manufacture a
   final manifest or retry to obtain PASS. Silence/flush gaps mean UNKNOWN, never
   OFF, held U-Boot, healthy Linux, recovery, or permission for a newline.

**Boundary:** The mode distinction is technically sufficient only under those
explicit environmental/caller assumptions. Their present satisfaction is not
established by this source review. Both FAILED/HOLDs and strict native-clock gates
remain untouched. No catch/TX/boot/reset/power/motion/NOR/storage effect or automatic
retry follows. Only this fresh REVIEW.md was written; no framework, source,
central-plan, Git/index, model/provider or safeguard change; no subworkers.
