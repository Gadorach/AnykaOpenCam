# Cold-entry23 v1 — independent ordinary SOURCE/API/method review

2026-10-23; BASE-01/05/07 / PLAT-06/08. **CORRECTION REQUIRED: R1.**
V1 remains frozen/unaccepted and must not be invoked to demonstrate this failure.
This is source review, not hardware/runtime admission or a cold-retry queue.

## Basis and exact pins

Read all534 lines of `cold_entry.py`, its entire README/CONTRACT/SOURCE_REPORT,
the complete original322-line recorder and98-line classifier, original boot RPC
and startup bodies (`qc07f_boot.py:98–282`), private-wrapper bodies
(`qc07f_private_root_transfer.py:105–142`), and the public historical
`preserved_hold.py`. Read the old passive/cold method reviews, cold plan/disposition,
preflight refusal, passive23b and cold23c results, exact grammar/classifier contract,
and current index/ledger entry sections. Public text/hash inspection only.

SHA256 independently checked:

| Path | SHA256 |
|---|---|
| `apps/qualification/qc07f-cold-entry23/cold_entry.py` | `03cc62101a4da20d2c6896e1f990ef13d85b4a12b427c3d1162d62e3e78b3bad` |
| `apps/qualification/qc07f-cold-entry23/README.md` | `9fb895153c4e94805efc8709246000e072c9e20c565c6314c6f08b061b7d9864` |
| `docs/qualification-plans/qc07f-cold-entry23-20261023/CONTRACT.md` | `0a72f984c6a7068a7cd548d19cf02e561d10dc35f8b380895a6a33fcb1f451dc` |
| `docs/qualification-plans/qc07f-cold-entry23-20261023/SOURCE_REPORT.md` | `13fabc1bfe12959dee16f0156d5320c52c2510eae8e64528dbd31e7d80aa0c6d` |
| `work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py` | `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00` |
| `scripts/qc07f_boot.py` | `6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985` |
| `scripts/qc07f_private_root_transfer.py` | `cb224d3cd8b635774b7e364b07b259d90640b30de269ea07351e767c3f97fa3c` |
| `apps/qualification/encoder-query77-cold-context-v2/cold_context.py` | `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558` |

Historical-method pins also matched the author table: COLD_CATCH_PLAN,
COLD_CATCH_DISPOSITION and both execution-20261023 review files. Cold23c result
read/hash basis: `b0c9fc66e2fc43cb604cc1b34ebf335791bfa5dd19598bb3bb65d6a61ea4a571`.
No private provenance, marker object or current device state was inspected.

## R1 — POST acknowledgement is not a GET entity/state response

**Blocking ordinary API mismatch, not a hypothetical hostile response.** Parent
supplied the concrete observation of one explicitly authorized idempotent OFF:
POST `/switch/outlet_power/turn_off` returned HTTP200, no Content-Type header and
zero body bytes; subsequent GET returned strict JSON with id/value/state OFF.
Parent also supplied read-only/body-profile context. This reviewer trusts that
reported interface fact within its limits; it did not repeat HTTP or observe ON.

`http_power` applies the same JSON/content-type/entity/Boolean contract to both
methods (`cold_entry.py:162–178`). The observed POST therefore fails at171 even
though its requested effect can already have happened. ON's caller at430 also
requires a returned Boolean and performs **no independent GET ON**. The supplied
OFF observation does not independently prove the ON response shape; regardless,
POST acknowledgement cannot substitute for that missing state observation.

The existing failure fallback is important and must remain: attempted ON is
recorded before the request (429); finally attempts one OFF (448–453), and still
GETs OFF independently after any POST error (454–459). Thus an empty successful
OFF acknowledgement is currently recorded as `OFF_POST_UNCERTAIN`; a real GET can
still prove OFF and permit owned retirement, but cannot remove that sticky error
or produce PASS (509). Safe cleanup is not composite API sufficiency. No hardware
execution is needed to establish this source/interface defect.

**Minimum fresh correction, preserving v1:**

1. Distinguish GET state parsing from POST acknowledgement. Keep the exact numeric
   private endpoint/path, no discovery/auth/default/fallback, HTTP200 requirement,
   no redirects/Location, connection timeout and bounded response read. A POST
   must consume/check a bounded acknowledgement body and accept the reported empty
   HTTP200 without demanding JSON Content-Type/entity fields. It must not fabricate
   `value`, parse absence as state, or return the requested state as observation.
2. After a successful single ON acknowledgement, require a **separate actual GET**
   satisfying the existing strict JSON content-type/duplicate-key/id/Boolean/state
   checks and reporting ON before proceeding with capture/quiet acceptance. Initial
   OFF remains GET-only; OFF remains one POST attempt followed by its independent
   strict GET OFF even if POST failed. Unexpected/non200/oversize/transport/parse/
   state failures remain sticky; no retry or response normalization into success.
3. Preserve pre-request ON uncertainty marking and the finally/OFF/owned-child
   behavior. Update fresh README HTTP claims (v1:52–55), source-report API claims
   and contract to make acknowledgement versus state evidence explicit. No other
   HTTP safeguard, source pin or historical helper/classifier needs relaxation.

## Remaining reviewed source bounds

No additional concrete ordinary correction was identified in these paths:

- `Retained` retains readonly NOFOLLOW/CLOEXEC/NONBLOCK descriptions, bounded bytes,
  hashes and path/FD metadata under checked ancestry. The historical854-byte read
  has a narrow source-derived in-workspace role, not a substitute modern manifest
  or authentication of current state. New config/output roots are protected and
  outside Git; marker locks are radio0644, reset0600, empty instance0644, acquired
  read-only in that order. Every retained object has an independent final check
  before descriptor/lock release, including failure exits. Neither old HOLD is
  rewritten, cleared, chmodded, migrated or adopted through C1.
- Direct exact-argv Popen retains actual wait ownership. PID/start/exe/cwd/argv,
  same PID/UID/GID peer, source/envelope/adapter and visible fuser association are
  checked before one arm and immediately before one ON, with zero RX/TX and no
  busy/inspection. Mismatched peer receives only drained status, never mutation.
  Capture leaf creation is left to the recorder. Original boot is AST-read, not
  imported/called; classifier executes exact retained public bytes, not cached pyc.
- The caller checks exactly first360 bytes/all19 records and **every** trailing
  byte against118 times (actual completed19-byte packets minus1). One arm/trigger,
  actual1..3 packet lineage, paired protected intent/completion and full RX extent
  are required; other event kinds/extra bytes refuse. This is explicit composition,
  not an EOF-classifier change or retrospective PASS for failed23c. Separate nominal
  10s raw/counter/event equality and exact final stop/capture_end suffix remain.
- Proven OFF gates same-peer stop and actual owned `wait(timeout=5)==0`, socket
  absence/visible fuser-none and final raw/event hashes. Lost stop ACK stays HOLD
  even if actual wait succeeds. Unknown power preserves the child; unknown ownership
  gets no mutating takeover or kill. Partial evidence and immutable intent remain.

These are conditional source observations, **not** acceptance of the whole v1
transaction, continuous custody or measured runtime behavior. Cooperative locks,
trusted serialized namespace/clients/system dependency closure and limited-visible
fuser cannot prove privileged/global exclusion. User UART-only wiring and QC07F-only
outlet association, config assertions and parent OFF observations are trusted caller
facts, not fresh reviewer measurements or permission derived from metadata.
Serial-open flush/modem-line limits, nominal host timing, host hang/suspend/failure
and absence of independent rail measurement remain explicit. No all-domain recovery,
C1/native-clock admission, image/boot/staging/MMIO/motor/NOR/command expansion or
native/product GO follows. Original23c error and both original HOLDs remain intact.

Only this fresh REVIEW.md was written. No candidate/helper import, compile/call,
test/fixture/fuzz/mutation, help/default invocation, HTTP/serial/RPC, private/proc/
device/lock/material/current-state access, hardware operation, Git command or
source/AGENTS/central-document change occurred; no subworker was spawned. Parent
alone may select/review a fresh correction; no automatic invocation or retry follows.
