# RAM76 resume21a — core diagnostic closure review

2026-10-21, independent source-only review. **CONDITIONAL SOURCE GO for exactly
one whole functional native-load/automatic RAM RUN trial, followed by the existing
state/info and metadata/idle retirement sequence.** Residual C2 from
`TRIAL_FINAL_REVIEW.md` is closed; no additional concrete source blocker was
demonstrated in this pass. This is not current hardware admission or native proof.

Owned outputs: this new report and fresh `core-closure-review/` only. No production
edit, commit/index change, RPC/status/serial/target/network operation, private-input
access, build or firmware execution occurred. All earlier reports, failures and
evidence remain unchanged and are rehashed in the new result. The parent's actual
04:37UTC RAM76 idle postcheck is historical context, not a refreshed observation
by this lane. No native metadata/radio/reset trial was run here.

## Exact source and C2 closure

Reviewed `apps/qualification/kernel76-transport/` pins:

| File | SHA256 |
|---|---|
| `trial21a.py` | `6d0ce652d3355baef2e5505e9d2a34813aa1f780289b17268ccb160c4748da9c` |
| `test_trial21a.py` | `887e94b8280e287a0f3b547c9f8f4a7d88a30cfbbd974d9b4a2e55f0993241f4` |

The new alternatives at `trial21a.py:24–25` fit the pinned candidate's actual
formats, rather than assuming that console text contains severity words:

| Candidate source under `artifacts/native-usb76-source01/linux/drivers/usb/musb/` | Match |
|---|---|
| `musb_core.c:692–694,721–723` | `bogus (host\|peripheral) RESUME (...)`, including both branches of the `%s` substitution |
| `musb_core.c:980–981` | `unhandled DISCONNECT transition (...)` |
| `musb_core.c:1001–1002` | MUSB device-prefix `: Babble` followed by its line ending/end of interval; this is immediately followed by babble recovery |

`musb_debug.h:13–18` confirms function/line-prefixed WARNING formatting. The
patterns need neither a literal “warning” nor a fixed timestamp/function line
number. LF, CRLF and end-of-buffer forms match; ordinary host-driver/enumeration
lines do not. The Babble alternative is line-terminal, not a broad match for any
mention of babble. The finite new alternatives neither suppress logs nor add any
target instrumentation/recovery.

All three original format strings were independently located as bytes, without
execution, in the manifest-hash-verified linked candidate kernel. Its inspected
configuration is `CONFIG_USB_MUSB_HOST=y`, `CONFIG_PM=n`. The RESUME IRQ handler
is not excluded merely because runtime PM is disabled. No firmware payload or
identifying bytes are exported by this check.

The supplied new full-main method checks the three previously missed source
forms before load, requiring an error, retained HOLD, no PASS and **zero radio
commands**. Independent probes additionally cover peripheral RESUME and all four
forms at six placements: before load, after load, after state, after info, after
post-metadata, and arriving after a returned post-inventory query body. All
**24 placements refuse**. Before-load cases send zero radio commands; after-load
cases never send state; after-state cases never send info. Later cases never write
PASS or clear HOLD. No active loader is interrupted/retried by these fences.

## C1/C3 and earlier fixes remain intact

Removing **only the two added regex source lines** reconstructs the previous
`TRIAL_FINAL_REVIEW.md` trial file's exact SHA256. Consequently the owner pinning,
diagnostic fences and full operation body are byte-identical to that reviewed
revision, not merely similar code:

- **C1:** `trial21a.py:114–132` pins original peer tuple and checkpoint start;
  the second owner refresh must preserve them, part, armed catch, idle TX and
  exact cumulative TX. Eight independent refresh variants (PID, UID, GID, start,
  catch, busy, part, extra TX) all retain HOLD before any radio command.
- **C3:** `clean_diagnostics():33–36` retains the entire owned interval. The
  explicit pre-load fence, mandatory before/after fences for load/state/info in
  `monitor():39–51`, and final fence at154 remain. The five original warning
  probes also refuse at the expected pre-effect/inter-query boundaries.
- **B1/B2:** `metadata21a.py`, `radio_receipt.py`, both corresponding tests,
  `postcheck.py` and Boot/PrivateBoot retain their prior final-review hashes.
  Exact A6 four-FD grammar, before-import postcheck hash/handoff binding, typed
  checkpoint/refresh facts and the320-byte/14-query TX derivation remain enforced.
  Standalone metadata still admits before-only, not an independent after-mode.
- The strict151-ACK/74-chunk loader parser is unchanged. Five independent mutants
  (historical28-hex truncation, missing ACK, negative/short numeric lengths and
  nonzero status) all refuse. No ACK is repaired, dropped or reconstructed.

## Whole-operation assessment and limits

The composed main still takes the nonblocking shared operation lock and durably
writes its own HOLD before admission; foreign HOLD refuses untouched. It checks
the image/build binding, fresh same-owner idle inventory/boot UUID, and bounded
read-only metadata shell transaction before the only load command. Exact echo,
nonce BEGIN/DONE framing, known shell exit/monitor reap and strict metadata parsing
remain; an invalid completed metadata transaction cannot admit radio commands.

The sole effectful entry remains **one** `radio native-load`: full74 chunks and
automatic CRC-enabled RAM RUN. Complete strict loader grammar precedes one state
and one identity query. Those queries deliberately reset endpoints **after** the
native-load interval. Post-metadata and idle inventory must again pass, same boot,
console-only PID1 FDs, empty modules, volatile mounts, ISP idle, watchdog inactive,
UART clean and whole-interval diagnostics before PASS/HOLD clearance. Exceptions
have no download/RUN repeat,256 fallback, endpoint repair or reset path.

Existing bounds remain material: the5s target watchdog can expire on a slow path;
600s host wait is finite, not proof that every USB timeout combination fits5s.
Source cleanup plus synchronous return, PID1 FD absence and idle checks provide
bounded retirement evidence, **not direct all-URB/DMA counters**. The diagnostic
gate checks observed console evidence; it does not instrument every internal HCD
condition. These previously documented limits are not expanded into new claims.

Before using this source GO, the parent must independently establish **fresh
current owner/catch/lifetime, no competing lease/sender, correct runtime and exact
handoff/TX, descriptors/FDs and retirement admission**. Dated PID/deadline/IP or
04:37UTC facts do not supply that gate. Do not adjust checkpoint counters to fit
unaccounted commands or reuse this fixed handoff after a separate metadata run.

Even a native PASS would establish functional full-size download/application
identity/idle retirement only. **Direct MODE/NAK/toggle/MTU1500 proof stays false;
RP-02/product and RP-04 combined-memory gates remain open/HOLD as applicable.**
No association/network/module/media/card/flash/AP/actuator action, GPIO42 reset,
watchdog reset or automatic recovery/retry is admitted by this review. An uncertain
or failed operation stays HOLD; later healthy status cannot erase it.

## Reproducible host evidence

Executed once, **05:20:02–05:20:03UTC**, Python3.14.7:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/core-closure-review/review.py
```

- Supplied discovery suite: **17 methods PASS** (metadata5, receipts5, trial7).
- Independent unchanged actual-main probes: one healthy complete flow loads once;
  **24 core placements +13 owner/original-warning fault probes refuse**.
- Five additional strict-ACK mutants refuse; reviewed sources and prior evidence
  rehash unchanged before/after. All trial files/OS/transport/private-admission
  capabilities in these probes are replaced by the reviewed in-memory harness.
- Exact argv, UTC, tool version, source/prior pins, compiled-format booleans and
  outcomes: `core-closure-review/result.json`; supplied test output:
  `core-closure-review/unittest-discover.log`; collector: `review.py` in that folder.

This source-only GO closes the earlier residual-C2 NO-GO for the pinned corrected
revision; it does not rewrite the preserved historical failures or initiate work
on the target.
