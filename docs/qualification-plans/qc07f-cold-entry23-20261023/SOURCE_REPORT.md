# Cold-entry23 author delivery — AST/syntax only

2026-10-23; BASE-01/05/07 / PLAT-06/08. **Delivered for independent parent review;
not self-accepted, imported, called, CLI-invoked, tested or hardware-operated.**

Owned only the fresh `apps/qualification/qc07f-cold-entry23/` and this qualification
directory. CONTRACT.md was written before code. No Git commands/index/commits,
subworkers, private configuration/captures/state/HOLD reads, proc/device/serial/
socket/network/locks access, target/ELF execution or old source edits occurred.
Public source was read/hash-inspected; one isolated AST/in-memory syntax compile
command ran. No bytecode/code object was executed or written.

## Delivered source pins

| Path | SHA256 |
|---|---|
| `apps/qualification/qc07f-cold-entry23/cold_entry.py` | `03cc62101a4da20d2c6896e1f990ef13d85b4a12b427c3d1162d62e3e78b3bad` |
| `apps/qualification/qc07f-cold-entry23/README.md` | `9fb895153c4e94805efc8709246000e072c9e20c565c6314c6f08b061b7d9864` |
| `docs/qualification-plans/qc07f-cold-entry23-20261023/CONTRACT.md` | `0a72f984c6a7068a7cd548d19cf02e561d10dc35f8b380895a6a33fcb1f451dc` |

Script:534 lines /28,763 bytes. Longer than the approximate200–300-line target:
the required independent failure postchecks, exact read-only historical custody,
strict HTTP/RPC/file bounds and actual owned-child finalization remain together
in one foreground transaction instead of adding a framework or hiding them in old
effectful helpers. Two internal data-handling helpers are not separate libraries.
There are no retries, persistent daemon, clock/receipt library or image operations.

## Validation command and actual result

Executed from `/home/gadorach/Desktop/AnykaOpenCam`:

```sh
python3 -I -B -c 'import ast, hashlib; from pathlib import Path; p=Path("apps/qualification/qc07f-cold-entry23/cold_entry.py"); data=p.read_bytes(); tree=ast.parse(data, filename=str(p)); compile(tree, str(p), "exec"); print("AST_PARSE_PASS; SYNTAX_COMPILE_PASS; NO_EXECUTION"); print("lines="+str(len(data.splitlines()))+" bytes="+str(len(data))+" sha256="+hashlib.sha256(data).hexdigest())'
```

Result: exit0, `AST_PARSE_PASS; SYNTAX_COMPILE_PASS; NO_EXECUTION`, source statistics
and digest exactly as above. This does not establish runtime dependencies, HTTP
response compatibility, recorder startup/stop, correct hardware behavior, or any
functional/physical acceptance. No help/default invocation or synthetic fixture
was used. All functional checks remain NOT RUN.

`sha256sum` was separately run on the exact public input paths in the table below,
then on the three delivered paths above and the classifier CONTRACT. No private
input path was passed. The four user-supplied original source pins matched; these
original files were never edited by this author.

## Public inputs inspected/pinned

| Workspace-relative path | SHA256 |
|---|---|
| `work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py` | `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00` |
| `scripts/qc07f_boot.py` | `6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985` |
| `scripts/qc07f_private_root_transfer.py` (optional reference, first220 lines) | `cb224d3cd8b635774b7e364b07b259d90640b30de269ea07351e767c3f97fa3c` |
| `apps/qualification/encoder-query77-cold-context-v2/cold_context.py` | `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558` |
| `docs/qualification-plans/encoder-query77-cold-grammar-20261022/OBSERVED_PROFILE.md` | `daff4d31fcb034354df154fd0c5233824738150ee84ab59f9b862bf0f8c85437` |
| `docs/qualification-plans/encoder-query77-cold-context-v2-20261022/CONTRACT.md` | `de280219506efa79089797097f9b7ccf62a6adfea508a7ea178c7fb3edfe908c` |
| `docs/qualification-plans/execution-20261023/COLD_CATCH_PLAN.md` | `b7316bc9b8e0f884b66023bd041db1cc0a5f8b5d9a9f8993ecce8b64e8c83a7e` |
| `docs/qualification-plans/execution-20261023/COLD_CATCH_DISPOSITION.md` | `ce828ea634603b732675df5ced4290b7ae57474649db740e4c1314c0c58f20cb` |
| `docs/qualification-plans/execution-20261023/passive-review/REVIEW.md` | `c8839f7063b6db2cf66bbede61bf79443bdc1c426005bb231c86e5eb7ccc9441` |
| `docs/qualification-plans/execution-20261023/cold-catch-review/REVIEW.md` | `3370cf8b3ebb4b9402a6c7d5d7f72d9ca2dc4023ab31c194edebd6d2689bc1f2` |
| `docs/qualification-plans/execution-20261023/PREFLIGHT_REFUSAL.md` | `d51b9b477f30693ca99f07225b01731591f0ec7217d488217803090fdd7f4bc4` |
| `docs/qualification-plans/execution-20261023/PASSIVE23B_RESULT.md` | `4be5966fbfce4c111db83fa201754c79d782419eac8f9fd37e08c82288d2b180` |

Also consulted the current index/execution-ledger entry sections and public old
`preserved_hold.py` source for exact marker names/order, and public source search
for the exact GUI-instance lock name. No historical helper was imported/called.
Current parent steering supersedes dated operator-only/no-outlet wording; author
did not independently verify that private association or any physical report.

## Important caller repairs represented in source

- Original immutable recorder is launched directly; actual Popen retained and
  waited, not a numeric PID/orphan/fake wait. Its final manifest stores recorder
  `sys.argv`, so comparison excludes interpreter/`-I`/`-B` while `/proc` compares
  the full real child argv. No old global/function/source constant is patched.
- The original EOF classifier gets exactly the first360 bytes. All remaining
  bytes must equal the exact118-byte key/control-key software echo group times
  completed-packet-count minus1. Actual completed packets must be1..3,19B each,
  with one exact arm/trigger and only the corresponding protected TX events.
  Entire raw/event stream, separate quiet interval and final suffix remain checked.
- The one OFF attempt covers uncertain ON and parse/RX errors. GET OFF still runs
  after a failed OFF POST; the POST failure is preserved as HOLD. Proven OFF gates
  owned shutdown. Stop-ACK failure stays HOLD, but actual Popen.wait is still tried.
  Unknown peer never gets a mutating request; a safe status is drained on mismatch
  instead of breaking the original recorder through an empty disconnect.
- Every acquired input/marker has independent failure/normal postchecks before
  any description/lock release. Historical raw has one explicit narrow in-workspace
  read role; all new outputs and explicit configuration remain outside every Git
  tree. Original radio/reset modes and all failed observations remain unchanged.

## Review/operation gaps retained

No independent SOURCE acceptance yet. Parent must review strict HTTP JSON/entity
shape, trusted isolated interpreter/system serial closure, exact historical raw
provenance, current physical association/wiring, namespace/client serialization,
limited-visible fuser assumptions and all failure/retirement paths before selecting
ONE actual operation. No private input was inspected to make these assumptions true.
Private config schema mismatches or unavailable isolated pyserial must refuse, not
trigger defaults, dependency installation, discovery or policy relaxation.

Host waits/timeouts are nominal; no independent rail measurement, whole-interval
clock, host-hang recovery or all-domain power guarantee. No C1 adoption, original
HOLD relabel, boot/upload/kernel/MMIO/motor/flash action, native/product GO, or
automatic queue follows. Parent alone owns any later actual invocation.
