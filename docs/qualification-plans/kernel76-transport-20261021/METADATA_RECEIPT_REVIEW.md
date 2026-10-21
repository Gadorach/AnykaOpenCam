# RAM76 resume21a — metadata-before / public receipt review

2026-10-21; independent source-only lane. **NO-GO as written for the requested
strict metadata-before admission**, pending B1/B2 below. Query semantics and the
normal protocol grammar are source-supported; the nine supplied host tests pass.
This is not a native failure or permission to try an effectful operation.

Owned changes: this report and fresh `metadata-review/` only. No production source,
central document, index or build edits. No UART/RPC/status/target/network/private
capture access, firmware execution or build occurred. Parent reports resume21a
boot and fixed postcheck PASS, with no radio command yet; this reviewer inspected
their public receipts, not the live target. Prior native directboot is unrelated.
Only `--before-native-load` is reviewed for entry. The after-mode and an effectful
trial wrapper are **not approved** by this report.

## Findings requiring closure before metadata TX

### B1 — FD parser violates no-extra-lines/source-exact acceptance

`radio_receipt.py:90–99` silently discards **any number of `total N` lines in any
position**, and `.strip()` removes leading/trailing blank FD lines. A6 has
`CONFIG_DESKTOP=n`: retained `coreutils/ls.c:1025–1037` compiles the `total` output
out entirely. Inserting `total 0` after `FDS`, twice or between FD entries still
passes the present parser by direct source inspection. These are not legitimate
lines for this fixed query, and must not be silently filtered into success.

The FD regex also accepts arbitrary `l`-prefixed tokens and unconstrained middle
text rather than the actual A6 long-listing structure. For example, four lines
`ls: unexpected N -> /dev/console` for N=0..3 satisfy the regex without being FD
listings. Existing fixtures cover duplicate FD numbers, missing/extra FDs and a
watchdog FD, but not these accepted malformed/extra-line cases.

Required: exact four source-compatible long-listing records, FD numbers0..3 once
each, console/ttyAK0 targets only; reject extra/blank/`total`/malformed lines. Add
negative fixtures for these cases and a fixture representing **actual A6 output**,
not just desktop-style UID/GID/date columns. A6 has username/timestamp/color
features disabled: `ls -l` itself omits user/group fields without `-n`; see
`ls.c:511–589`. Do not change the target command merely to fit a desktop fixture.

### B2 — the before-mode does not itself enforce the complete exact21a handoff

`metadata21a.py:90–108` checks both public image hashes, the postcheck PASS state,
live recorder part/sole owner/lifetime, PID/start identity and cumulative TX. But
it omits checks already present in the fixed postcheck's own admission:

- boot receipt state `RAM76_BOOT_AND_INERT_MONITOR_OBSERVED` and receipt part;
- postcheck part;
- private boot checkpoint `image_sha256`, `part`, `monitor_observed is True`,
  `shell_started is False` and explicit integer/nonnegative cursor validation;
- exact boolean type for `one_empty_monitor_refresh` (currently coerced by `int`);
- binding of the imported `postcheck.py` to the postcheck receipt's
  `recipe_sha256` **before** executing its module and using its query table.

This is an incomplete self-contained admission gate, **not evidence that the
parent's observed boot/postcheck was wrong**. Public receipts currently have the
expected states/parts; the postcheck recipe hash matches the reviewed file.
The reviewer did not read the private checkpoint. Restore those explicit checks,
or have the parent document an equally exact frozen external admission that
supplies them; do not infer the omitted facts from filenames or mutable imports.

The correctly derived postcheck increment is **320 TX bytes for14 commands**, plus
exactly one byte iff the admitted boolean refresh is true. The inspected public
postcheck has that flag true, so this before-mode expects **boot checkpoint TX+321**.
No private counter value is exported here. Require exact equality and no other
sender; never adjust a checkpoint to make a mismatched counter pass.

## Source-supported parts of the proposed read-only roundtrip

### HOLD / owner / bounded send / completion

- `metadata21a.py:85–89` obtains the shared nonblocking exclusive lock, refuses an
  existing HOLD, then writes/flushed/fsyncs its own HOLD **before admission/TX**.
  Failures retain it. Successful public/private receipt creation and a final
  stream check precede clearing it. Failure logging does not authorize cleanup.
- PrivateBoot/Boot imports contain definitions/constants only; module entrypoints
  are guarded. No test import opens the UART/socket or runs a build recipe.
  `owner(margin=1200)` checks recorder source/envelope, socket peer/UID/PID/start,
  sole adapter owner, lifetime and inspection exclusion. `state()` requires exact
  part/TX, no busy TX; checkpoint RX starts reset detection. `fixed()` requires
  fresh response bytes, reset-rejecting waits and exact cumulative TX afterwards.
- A known owned monitor enters only `shell\r`; an optional single CR refresh is
  admitted only when the already-observed monitor prompt is present but not at
  the end. It is not a stock-shell discovery or blind recovery attempt.
- The fixed nonce-bearing command is ASCII, one line, <=950 bytes, with no CR/LF/
  NUL supplied by the caller. It is sent in <=64-byte chunks,25ms between chunks;
  complete exact echo (apart from CR/LF wrapping) is required **before** the final
  CR. No CR is sent after echo mismatch/timeout. No retry of an executed command.
  Ash's no-editing input buffer is1024 (`ash.c:3004`), distinct from the monitor's
 256-byte buffer, which receives only `shell` here; ash itself reads `exit`.
- Framing requires unique exact nonce BEGIN/DONE0 lines, `FDS`, unique `FD_RC:0`,
  exact ordered/value-matched attribute pairs and the sole trailing `# ` prompt.
  Extra/duplicate attribute keys, warnings, wrong endpoint values or nonzero
  aggregate status refuse. B1 is the exception in the FD subparser.
- Only after parsed metadata succeeds does `exit\r` run. Retained
  `wifi-monitor.h:23–37` forks, executes `/bin/busybox ash` with only PATH/HOME/TERM,
  waits for that child via wait4 (retrying EINTR), then prints the exact returned
  message; the monitor subsequently prints its prompt. This is source-backed
  return/reap evidence, not an independently checked child exit-status receipt.
  Failure before that point intentionally leaves HOLD/possibly the shell active;
  do not send speculative `exit`, reset or other repair commands.

### Exact A6 artifact, prompt, applets and paths

Offline `metadata-review/collect.py` verified in memory:

- retained A6 BusyBox **378104 bytes**, SHA256
  `3f7e15eae5f1c03a0af78ce45a341aaa48ad0eb0a00850a0914512e830b7c3a1`,
  equals selected76 cpio `/bin/busybox` and the retained A6 build receipt;
- selected cpio matches its package pin and the final build's embedded archive;
  `/init` equals retained attempt50; no `/etc/profile`, `/.profile` or
  `/root/.profile` entries;
- ASH/ASH_TEST/LS/CAT/TEST enabled; READLINK applet and FEATURE_EDITING disabled.
  Disabled READLINK does **not** remove `ls -l`'s internal
  `xmalloc_readlink_or_warn()` (`ls.c:492–497,606–607`).

Root default PS1 is exactly `# ` (`ash.c:2314–2319`). Invocation is non-login,
with no `ENV`; profile startup is not selected (`ash.c:14750–14776`). No use of the
absent `readlink` applet, `id`, `cut`, a shell profile or injected credentials.

Read-only source anchors in the pinned76 tree:

| Query | Source and interpretation |
|---|---|
| `/proc/1/fd` via `ls -l` | `fs/proc/fd.c:174–189,204–215`: fd symlinks resolve existing file paths, size64. PID1 monitor opens console (ttyAK0 fallback), dup2s to0/1/2 and retains original3 (`init.c:472–474`). This is PID1-only inventory, not an all-process owner census. |
| `1-1/idVendor,idProduct` | `drivers/usb/core/sysfs.c:690–704`: cached descriptor LE16 values, `%04x`; require a012/8400. |
| `1-1/bConfigurationValue` | `sysfs.c:25–43,90–110`: show reads cached actconfig under lock; require `1`. Although the attribute also has a write handler, `cat` never invokes it. |
| `1-1/speed` | `sysfs.c:145–179`: cached high-speed maps to `480`. No new descriptor/vendor request. |
| `1-1:1.0/ep_01` and `ep_81` | `message.c:1300–1310`, `endpoint.c:150–170`: endpoint devices are children of the interface, named `ep_%02x`. Correct path includes the interface, not just device1-1. |
| endpoint address/type/maxpacket | `endpoint.c:37–58`: cached endpoint descriptor; `%02x` address01/81, `%02x` attributes02 (bulk), `%04x` maxpacket0200 (512). No endpoint reset/USBDEVFS_BULK. |
| interface `/driver`, `test -L` | driver core creates a `driver` symlink (`drivers/base/dd.c:494–508`); BusyBox test uses `lstat` for FILSYM (`coreutils/test.c:699–711`). Presence fails aggregate status. Prior successful endpoint cats establish the known interface path. |

The script only lists PID1 FDs and cats these allowlisted cached scalars; it never
reads serial/manufacturer/product, firmware-info device fields or credentials.
No watchdog/usbfs FD, module, firmware, RF or network command is opened/executed.

## Public radio receipt grammar — independently corroborated, not native76 proof

`radio_receipt.py:9–56` requires complete command-body lines, exact cookie/command
sequence and no unclassified nonblank lines; exactly74 chunks,151056 payload
bytes,151348 wire bytes and **151 ACKs** (ENTER+GET_SC+74 command/data pairs+RUN).
Each logged ACK must report actual length16 and contain16 header bytes, RX magic,
type7, vif/flags0, declared length16, matching cookie, status0 and checkFD. Command
byte is exact except the deliberately unconstrained raw-data ACK command, as in
retained stock semantics. Firmware RUN is cookie176; no explicit ZLP is expected.
CRC-enabled RUN is functional acknowledgement, **not numeric CRC readback,
MODE/NAK/toggle observation, MTU1500, association or a whole-product gate**.

Independence from the synthetic fixture was checked against **public historical**
retained256 metadata, not private captures. `ack-corroboration02.json` records
1184 complete historical ACK headers independently matching that shape and
cookie/command/status/check semantics. One additional historical logged header
is truncated (28 rather than32 hex characters), despite reporting length16.
It is preserved/reported, **not repaired or counted as a valid strict ACK**;
public text and public hex agree on the truncation. This historical transcript
would not pass the new full-transcript strict parser. It nevertheless provides
1184 complete independent shape observations, plus a separate public APPLICATIONff
response with the same16-byte/type7/vif0/length16 framing. No raw headers or device
fields were copied into the owned evidence. Source `hgic.h:112–118,219` plus
`hgic_def.h:94–101` independently describes the8+8-byte BOOTDL response layout.

Application-state parsing requires the five exact lines and statusff.
Firmware-info parsing requires36 bytes, type3/vif1, cookie1/cmd43, return24 and
version/SVN/chip/CPU allowlist. Only identity/privacy booleans are returned;
remaining potentially identifying bytes stay unreturned. These are pure parsers,
not authorization to issue their active USB queries. Before any future effectful
trial, a separately reviewed wrapper still needs durable HOLD, sole ownership,
full finite transcript, independent identity/retirement gates and failure policy.

## Tests, retained failures, and bounds on this verdict

Executed once at04:45:38UTC with Python3.14.7:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/metadata-review/collect.py
```

The collector ran only the two admitted host unit scripts with `-B -v`:
**metadata4/4 and receipts5/5 PASS**. Exact argv, timestamps, reviewed-source pins,
logs and artifact/config facts are in `metadata-review/result.json` and the two
`test_*.py.log` files. B1/B2 are not exercised by those passing tests. No native
production operation was run by this lane.

Preserve the parent-reported initial failures: the original firmware-info mutant
was a no-op fixture, corrected to mutate byte12; missing `FD_RC:0` formerly raised
ValueError, now explicitly asserts before lookup. They were host-test/fixture
failures, not native device failures. This lane did not invent or overwrite their
earlier logs. Its own first independent historical corroboration also failed on
the falsely assumed1185 complete logged headers; the initial collector remains
unchanged, with the failure recorded in
`metadata-review/ack-corroboration-initial-failure.txt`. The corrected02 collector
exports only the1184 complete-header observations and the one truncation count.

**Failure remains HOLD.** Nothing here authorizes watchdog/reset/GPIO42, firmware
load/RUN, RF, endpoint reset, freeing uncertain owners/DMA, flash/card/AP/media/
actuator actions, or blind retry when metadata fails. Any later recovery must
come from the parent's separately reviewed/current admission and actual reset
proof, not this source review or a subsequent healthy prompt.
