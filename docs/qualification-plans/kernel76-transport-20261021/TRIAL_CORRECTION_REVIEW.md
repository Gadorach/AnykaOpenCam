# RAM76 resume21a — corrected metadata and whole-trial review

2026-10-21, independent source-only review. **NO-GO for the actual whole
native-load/RUN trial as currently written.** Prior B1/B2 are fixed, but three
concrete in-run admission/diagnostic gaps remain below. Current RP-00/descriptor/
owner/catch admission is still required after source closure; it does not replace
the missing in-run checks.

Only this report and fresh `trial-review/` were written. No production source,
prior report/evidence, ordinary Git index or central file was edited. No commit,
RPC/status/UART/target/network/private-input access, build or firmware execution.
Parent's actual boot/postcheck PASS is not disputed. No native trial ran here.

## Blockers — actual main exercised under in-memory fakes

| ID | Source finding | Independent host result | Narrow required correction |
|---|---|---|---|
| C1 — owner identity can be repinned immediately before load | `trial21a.py:109` calls `instance.owner(margin=1800)` again but does not compare the resulting peer/PID/start against the checkpoint/original owner. `Boot.owner()` (`scripts/qc07f_boot.py:320–343`) checks its *current* receipt and then overwrites `self.peer`; only the part is compared with previous state. `meta.state()` after that checks part/TX, not original peer/start. The returned catch-armed flag is also not rechecked there. | A Fake that replaces peer123 with456 on that second owner call still reaches **one load, state, info, PASS and cleared HOLD**. This models the real owner's repinning semantics; it is not a claim that a recorder actually changed. | Preserve the original peer/start identity and require exact equality after every owner refresh, plus the expected part/TX/catch-armed facts, before load. Reject replacement even if same-part/new receipt/counters look valid. Add a second-owner-change/lost-catch refusal fixture. No generic owner framework needed. |
| C2 — source-real HCD warnings evade the final regex | `trial21a.py:132` recognizes `usb/musb ... error/warning/timeout`, flush failures and BUG/Oops/panic, but many MUSB diagnostics contain none of those words. `musb_debug.h:13–18` emits function+line+message; KERN_WARNING is a severity, not a literal word in console text. `musb_host.c:565` emits `musb_rx_reinit 565: rx1, packet/512 ready?`; `:878` emits `broken !rx_reinit...` as ERR. | Injecting the source-shaped packet-ready warning into an owned metadata-exit response before load still reaches **one load, state, info, PASS and cleared HOLD**. The raw owned interval contains the warning, but the regex misses it. | Reject the actual candidate's warning/error forms, including function-prefixed messages and kernel WARNING splats. Keep the allowlist/denial finite and source-based; do not rely on severity words surviving console formatting. Add these concrete diagnostic fixtures. |
| C3 — even recognized warning rejection occurs after subsequent effects | The only whole-interval diagnostic check is at `trial21a.py:130–132`, after native-load, state/info and both postchecks. Metadata exit checks only return-message/prompt, not diagnostics in that response. Thus a recognized pre-load warning is known in owned RX yet does not block native-load. Warnings arriving between returned transactions likewise have no pre-next-effect fence. | A recognized synthetic `musb: warning ...` before load ultimately retains HOLD/no PASS, **but only after load, state and info have all been sent**. | Check the owned diagnostic interval immediately before native-load and after each returned effectful transaction, before the next active USB query. Retain the final check. Do not interrupt/retry an active whole loader; stop subsequent TX after its bounded return/failure. Add a recognized pre-load-warning fixture requiring zero load commands, plus an inter-query-warning fixture requiring no following query. |

All three are local wrapper fixes, not requests for new target instrumentation,
drivers, builds, a generic framework or a larger investigation. A later healthy
prompt/inventory cannot erase an observed warning/owner discontinuity. Do not run
the trial merely to see whether these cases occur on hardware.

## Corrected prior findings

- **B1 closed:** `radio_receipt.py:90–101` now requires four records, exact
  `lrwx------`, nlink1, size64, unique FD0..3 and console/ttyAK0 targets. No extra
  `total`, UID/GID/date, arbitrary `l` tokens or blank records are accepted. The
  normal line fixture matches the retained A6 configuration/source. A single
  record-terminating newline is not an extra blank record.
- **B2 closed:** `metadata21a.admitted_handoff():31–57` checks exact boot/post
  states, parts, image SHA/CRC flag; private checkpoint image/part/monitor/shell
  flags; integer/nonnegative counters/PID/start; exact boolean refresh. It checks
  the reviewed old postcheck hash, reversible derivation flag and actual new
  postcheck recipe hash against the receipt **before module import**. Fourteen
  query wires must total320 bytes; expected TX is checkpoint+320+boolean-refresh.
  Private inputs were not opened by this reviewer.
- Standalone metadata CLI accepts only `--before-native-load`; after-mode is
  removed. It is not the entrypoint for the composed trial's post-metadata pass.
- The earlier report/failure evidence remains byte-exact. New pins are recorded
  separately; nothing retrospectively rewrites the earlier NO-GO verdict.

## Remaining whole-code assessment

The proposed scope otherwise remains suitably finite and source-supported:

1. `trial21a.main()` writes/flushed/fsyncs its own durable HOLD under the shared
   nonblocking operation lock **before input admission**, refuses foreign HOLD,
   validates the exact image/build50 source binding and performs initial
   checkpoint PID/start/TX/part/catch checks. PrivateBoot retains the reviewed
   source/envelope, sole-adapter-owner and bounded-recorder checks. Initial
   lifetime margin1800s and native-load wait600s are finite; this is not a claim
   that the compiled5s watchdog outlasts every USB timeout combination.
2. Fresh14-query idle inventory is parsed with the exact reviewed postcheck,
   UUID compared with the original private runtime, then a bounded metadata shell
   roundtrip checks PID1 FDs, cached VID/PID/config/speed, bulk01/81 maxpacket512
   and no bound interface driver. Same64B/25ms/echo-before-CR/<=950-byte recipe;
   no descriptor/vendor/RF request is introduced by metadata itself.
3. `metadata_roundtrip():36–66` requires owned fresh shell prompt, complete exact
   echo, one exact BEGIN, one nonce DONE-with-numeric-status line and a final
   line-anchored fresh `# ` context. It may then `exit`, confirm monitor's
   returned/reap message and only afterwards accept metadata values via the
   unchanged strict parser. A failed metadata value still retains HOLD and
   prevents the first radio load. This is normal retirement of the known
   read-only child, not hardware recovery. Incomplete/unknown context refuses
   before exit; no guessed reset/repair follows.
4. Only the whole existing `radio native-load` entry is composed:74 chunks,
   automatic CRC-enabled RAM RUN,151 exact ACKs. There is no single-fragment
   injection, firmware alteration, manual endpoint recovery,256 fallback,
   retransmit, second RUN or automatic reload path. Source-required state/info
   verification follows only a fully parsed loader body. Those queries actively
   perform USB transactions and reset endpoints **after** the native-load
   interval; they are not passive metadata or proof of no-reset toggles.
5. After identity verification, metadata and the full14-query idle inventory
   repeat. UUID is compared with the pre-inventory; modules/mounts/ISP/watchdog/
   UART/memory and PID1 console-only FDs are checked before a PASS can be written.
   C1–C3 prevent accepting the present wrapper's same-owner/no-HCD-warning claims.
   These checks are bounded lifecycle evidence, not direct all-URB/DMA retirement
   counters or a combined media memory budget.
6. Exceptions retain durable HOLD, emit no fallback/reset/repair commands and
   cannot enter another load iteration. Public output is allowlisted protocol/
   identity/metadata booleans and memory counters; raw fwinfo potentially
   identifying fields remain in private operation evidence. No module attach,
   association, network/MTU/media/card/flash/actuator effect is in this wrapper.

`monitor()` re-admits the frozen postcheck source on each call rather than
accepting arbitrary commands. The image/hash-bound monitor itself remains the
previously reviewed artifact; this review did not rebuild or execute it. No
directboot history is substituted for native76 transport evidence.

## Strict ACK/claim boundaries remain intact

Normal loader grammar still requires complete16-byte type7/vif0/declared-length16
ACKs, exact cookie/command/status/check and wire-length/progress sequence. Missing,
extra, short, wrong-value or malformed records refuse. Host probes independently
confirmed rejection of:

- the prior historical28-hex-character logged header shape;
- numeric negative/short declared actual length;
- nonzero ACK status;
- a missing ACK.

The historical truncation is not repaired or waived; prior independent
corroboration from1184 complete public historical headers remains separate
metadata evidence. A future successful full trial would establish functional
full-size loader/application-identity/idle-retirement evidence only. Both receipt
and public trial record correctly keep **direct MODE/NAK/toggle/MTU1500 proof
false**. Radio capability/product qualification may remain open.

## Host evidence and follow-up

Executed once,04:59:25–04:59:26UTC, Python3.14.7:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/trial-review/run_review.py
```

- Supplied scripts: **metadata5 + receipts5 + trial3 methods PASS**. This includes
  the handoff variants, healthy whole main, nine fault cases and foreign HOLD.
- Additional independent in-memory Fake-only injections exercised the unchanged
  actual main/helpers and confirmed C1/C2 unsafe acceptance and C3 late refusal.
  The handoff/private/OS/transport capabilities remain faked, not natively tested.
- Five additional synthetic ACK mutants all refused. No raw/private transcript
  was accessed, and no target commands were sent.
- Exact source/input pins, argv/timestamps, logs, prior-evidence preservation
  hashes and boundary outcomes: `trial-review/result.json`, `test_*.py.log` and
  the collector source. All reviewed source pins were unchanged during the run.

**Next source gate:** correct C1–C3 narrowly, add their refusal fixtures, then
re-review the fixed exact wrapper. Only after a source GO may the parent separately
admit **one** whole functional trial with fresh RP-00/runtime/descriptor/FD/owner/
catch/lifetime checks. This report authorizes no target operation or watchdog/
GPIO42 reset; failed metadata/trial remains HOLD with no blind retry or fallback.
