# Cold-entry23 v2 — focused ordinary R1 SOURCE/API/method review

2026-10-23; BASE-01/05/07 / PLAT-06/08. **Conditional R1 and preserved-bounds
SOURCE/API/method sufficiency; no further correction required in this scope.**
Not execution admission, runtime confirmation, native acceptance or a retry queue.
Frozen v1 and its negative review remain valid and uninvoked.

## Read-only verification and pins

Read all538 v2 source lines and its entire README, CONTRACT, SOURCE_DELTA.patch
and SOURCE_PINS.json; reread the frozen v1 review from this same review session.
The earlier full v1/original-recorder/RPC/classifier/method review remains the
basis for unchanged behavior. Independently verified source text without executing
either candidate: reversing exactly the four R1 edits in memory equals frozen v1
byte-for-byte, and the independently generated unified diff exactly equals the
delivered patch. All six recorded byte-count/hash entries matched actual public
files. V1 source/docs/review and original recorder/boot/private-helper/classifier
hashes remain identical to this session's earlier review pins.

| Path | SHA256 |
|---|---|
| `apps/qualification/qc07f-cold-entry23-v2/cold_entry.py` (538 lines,28997 bytes) | `0d46fda61ced57bbbaa768d84f552b3d2d3f1bbc107f4d6a7f6f0cacf59e6bcf` |
| `apps/qualification/qc07f-cold-entry23-v2/README.md` | `6171901b7795e0b4a4d67a4e7b579cb4815b9ac0698584310d7bb276a9c4dcdb` |
| `docs/qualification-plans/qc07f-cold-entry23-v2-20261023/CONTRACT.md` | `f764a5a28e02d58bf683da5a7d1aba0fe42eb1005a26c0df2de407eb2aa1ca71` |
| `docs/qualification-plans/qc07f-cold-entry23-v2-20261023/SOURCE_DELTA.patch` | `bc75fcf7c00f348e4e16c2e9e3a10aa5a972f1c3abdf076b80fcb6c742e6b30b` |
| `docs/qualification-plans/qc07f-cold-entry23-v2-20261023/SOURCE_PINS.json` | `2f59e054ba6e0144f6254d3c5b5061b0329e5d4460f943a145a3ab91ebc11589` |
| `apps/qualification/qc07f-cold-entry23/cold_entry.py` | `03cc62101a4da20d2c6896e1f990ef13d85b4a12b427c3d1162d62e3e78b3bad` |
| `docs/qualification-plans/qc07f-cold-entry23-review-20261023/REVIEW.md` | `d899403829951a060e721cf927d293212acb81be60b8ef723efa1cdd3ea0ff78` |

## R1 disposition

- `http_power:162–183` now admits only GET/turn_on/turn_off. Both methods retain
  HTTP200/no Location,3s connection timeout and at-most4096-byte response acceptance
  via a4097-byte read. POST returns `None` after its bounded acknowledgement read;
  missing Content-Type/empty body no longer incorrectly requires a JSON entity.
  No requested or synthetic Boolean is returned as observed state. POST content
  is not state evidence, including when an acknowledgement has a nonempty body.
- GET alone retains the original JSON Content-Type, strict duplicate-key/non-JSON-
  constant handling, exact entity id, built-in Boolean and consistent ON/OFF checks.
  Numeric private endpoint/exact path/no redirects/discovery/default/fallback
  restrictions are unchanged. Moving the GET header check after the bounded read
  does not weaken its acceptance requirements.
- Initial OFF is still GET-only (391–393). Immediately before ON POST, uncertainty
  is marked (432); successful acknowledgement must be followed by an actual separate
  GET ON (433–434) before settle/cold/quiet acceptance. An ON POST/GET exception or
  wrong state enters the existing sticky failure/finally path, not another ON.
- OFF remains exactly one POST attempt (452–457) followed by independent strict
  GET OFF even after POST failure (458–463). A successful GET can establish reported
  OFF and gate owned retirement; it cannot erase an earlier POST/ON/other error.
  No retry, fake state, parser relaxation or failure-to-PASS conversion was added.

The fresh README/CONTRACT explicitly supersede v1's incorrect common GET/POST JSON
description for v2. The parent-reported HTTP200/empty/no-Content-Type OFF observation
supports this compatibility correction, not proof of ON behavior or measured rails.
Parent's AST/syntax result is cited, not rerun; no functional behavior was exercised.

## Preserved conditions and limits

Exact reverse equality preserves the earlier review's source bounds: protected
private roots and retained source/input identities; source-derived readonly historical
read; radio/instance0644 and reset0600 readonly locks; cooperative serialized bench
custody and limited-visible fuser checks; exact Popen/argv/start/peer and armed zero
RX/TX checks before one ON; exact first360-byte19-record profile plus EVERY118-byte
trailer group tied to actual1..3 completed19-byte packets; separate nominal10s quiet;
proven OFF before same-peer stop/actual owned wait0/final raw-event hashes/socket and
input postchecks. Unknown power/owner is preserved, not killed or taken over. No
old source, HOLD, classifier, timer or command profile changes.

Sufficiency remains conditional on trusted caller wiring/QC07F-only load association,
private configuration, trusted interpreter/dependency closure and maintained namespace/
client serialization. Config/metadata and parent-reported OFF/no-active-capture are
not reviewer measurements or permission. No global privileged exclusion, glitch-free
serial open, complete pre-flush capture, independent rail measurement, hard host
deadline, hang/suspend recovery or all-domain guarantee is established. Strict C1/
native-clock and staging/image/boot/MMIO/motor/NOR/media/product gates remain unchanged.
Original23c EOF-caller failure, original radio/reset HOLDs and v1 R1 stay preserved.
Parent disposition and fresh actual prerequisites must precede any operation; this
review selects no ON, cold retry, native queue or further framework.

Only this fresh REVIEW.md was written. Public text/hash/diff verification only;
no candidate/helper import, compile/call/run/test/help/default, fixture/adversarial/
mutation campaign, HTTP/serial/RPC, private/proc/dev/lock/capture/HOLD-state/material
access or hardware action occurred. No old file/AGENTS/central plan/Git/index/model/
provider/tool/safeguard change or subworker occurred.
