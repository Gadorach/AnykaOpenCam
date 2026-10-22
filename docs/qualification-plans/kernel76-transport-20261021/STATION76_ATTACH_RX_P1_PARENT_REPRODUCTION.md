# P1 parent HOST reproduction — PASS, independent review tool-blocked

2026-10-22; RP-02 / NET-01–08 / K76-06. This is defensive RX exclusion testing,
not a native station test or full S4/lifecycle/product acceptance.

**Subsequent review interruption:** the independent reviewer returned generic
`cyber_policy`, no final report or successful handback. Partial review evidence is
preserved but not adopted. See `P1_REVIEW_TOOL_BLOCK_20261022.md`. The parent HOST
PASS below does not satisfy that missing gate; no source acceptance follows.

## Exact invocation and custody

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-attach-rx-p1-parent01/reproduce.py --run run04
```

Fresh execution: **03:37:55–03:40:56UTC**, Python3.14.8, native GCC16.2.1,
`x86_64-pc-linux-gnu`. The shell operation completed normally. No earlier compiler,
test executable or collector output was reused as the fresh result.

Candidate seal SHA256:
`59fe2c873945ac20e3742298d4827cf5ce445b8dd28c1ab768a869973c878c1f`.
All **453** sealed candidate files matched before/after reproduction. No candidate
source, tests, runner, report, author evidence or seal was changed.

The parent wrapper records one explicit textual replacement in the collector:
`E = DOC / 'station-attach-rx-p1-host01'` becomes
`E = DOC / 'station-attach-rx-p1-parent01'`. It copies the public baseline metadata
byte-exactly to that new output root. This prevents appending parent artifacts to
the sealed author directory. No extraction, instrumentation, test, mutation,
compiler flag, timeout, core-dump policy or production source change is made.
The relocated collector is **not claimed byte-identical** to the original; its
original/transformed hashes and exact replacement are in `invocation.json`.
The collector's `runner_sha256` still identifies its original file, as disclosed
by the wrapper. Generated include/output paths naturally differ between runs.

## Observed results

| Actual newly executed HOST result | Optimized | ASan+UBSan |
| --- | ---: | ---: |
| P1 invariant assertions (main + eight declaration shards) | 531,997,726 | 531,997,726 |
| P1 ordered fixture invocations | 16,624,970 | 16,624,970 |
| P1 setup modeled command sends | 1,008 | 1,008 |
| A2b event/ingress regression assertions | 12,127,356 | 12,127,356 |
| A2a ingress regression assertions | 2,806,874 | 2,806,874 |
| A1 control regression assertions | 150,705 | 150,705 |
| Distinct compiled runtime mutants caught | 12 | 12 |

Counts represent repeated invariant checks, not half a billion distinct cases or
concurrency schedules. Setup modeled command sends are not after-input effects.
The original A2b safe-hook observation again reports eleven legacy routes plus
the BOOT outer8 bypass; unsafe downstream payload/printf behavior is not executed.

All positive compilation/execution commands exited0 with empty stderr. All twelve
mutant designs compiled in both modes and failed at runtime (24 executions).
Restoring the unsigned length comparison trips the protected short-input read
probe; other mutants fail assertions. Core dumps are disabled. A compilation
failure is not counted as a caught mutant. Each subprocess retains its120s bound.

There are **86** command records, **1,446** input pins and **117** extraction records.
Parent rechecked every recorded input and output hash. Author run03 and parent
run04 input inventories agree except for the path of the byte-identical baseline
metadata copy. Positive test stdout matches the author run, including regression
counts; sanitizer/address diagnostics are not required to be byte-identical.

## Evidence and limits

`station-attach-rx-p1-parent01/` contains:

- `reproduce.py`, `invocation.json`, `completion.json`, `copy-baseline.json`;
- `run04/result.json`, command journal, generated units, logs and output hashes;
- `parent-summary.json`, with verified counts and comparison scope.

Original run01's real signed-length overread, run02's incomplete timeout and the
provider rejection remain preserved in author evidence and the candidate report.
The same-session clarification subsequently completed normally with no safeguard
change; the original flag's internal cause is not established by test success.

These are deterministic synthetic HOST executions of extracted real paths with
recorded test hooks/stubs, not complete production translation units, SMP/IRQ/
USB/HCD/DMA, teardown, normal network functionality or target proof. Accepted
A1/A2a/A2b remain unchanged. The ordinary Git index remains unchanged.

**Next:** resolve the independent-review provider restriction before completing
that missing exact-pin review/source-acceptance gate. P2–P5, ARM/module builds, staging, native attach,
UP/association/MTU/media and product gates remain unselected/unaccepted. No target,
UART/RPC, private input, network, capture renewal or reset operation occurred.
Current hardware state remains unknown; original151-ACK native FAILED/HOLD stays.
