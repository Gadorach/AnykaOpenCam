# RP-02 / K76-06 — exact monitor76 radio entry review

2026-10-21, source-only disjoint review; parent alone owns hardware. Oct21 execution
steering supersedes the older PLAN ONLY handoff only within the parent's admitted
scope. This lane performed **no target/UART/RPC/status/network/device operation,
build, firmware execution or firmware/configuration change**. Current target,
capture owner and recovery state are unknown to this reviewer; Oct20 facts expired.

## Verdict and one next operation

**SOURCE GO for the existing whole `radio native-load` entry**, conditional on
parent RP-00/runtime/recovery/lease admission and explicit admission of its entire
RAM download **including automatic RUN**. There is no safe exposed single-fragment
entry. Do not alter the256-byte rollback path or inject a raw2048 transfer.

**One next effectful operation:** after those gates, send exactly one
`radio native-load` monitor command; collect its entire finite transaction and
retirement, then the separately gated application verification below. No automatic
retry, `radio load` fallback on the same partial download, module attachment,
association, MTU change or traffic follows. A negative outcome is useful evidence.

This can qualify **functional full-size loader completion**, not directly prove
MODE/NAK/toggle behavior. **Concrete instrumentation blocker:** candidate76 exposes
no cached segment/CSR/MODE/NAK/toggle/flush counters. There is no permitted read-only
monitor query that recovers those observations. If direct packet/toggle/CSR proof
is required *before* this first functional trial, that stricter gate is NO-GO on
the current image: parent must separately admit a minimal counters-only source
increment (not made or queued here). Do not claim that a successful trial closes
NET-01/02/04/08, the full RP-02 exit, MTU1500 or G2/G3.

## 1. Compiled identity, not an assumption about mutable source

Offline collector `source-review/inspect_entry.py` ran once at03:52:11UTC:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/source-review/inspect_entry.py
```

Results in `source-review/entry-bind.json`; selected ARM text only in
`source-review/monitor50-selected.disasm`. Python3.14.7, LLVM23.1.1. Output creation
is exclusive; a rerun must preserve these outputs/use a separately admitted fresh
output directory, not overwrite them. No extracted ELF/blob was written.

- Candidate image rehash matches final package pin
  `bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f`.
- Final build `analysis/native-usb76-package01/build/usr/initramfs_inc_data`
  decompresses exactly to the manifest-pinned selected cpio76. Its `init` equals
  both attempt50 cpio `init` and retained `artifacts/init-monitor-attempt50`.
- **Embedded `/init`:174248 bytes, SHA256
  `8c8e1a5e083914c8fd2fce7bbcfe10a4065691f49a8a5ea026a4ff12c20bb899`.**
  Matches `mainline-build-attempt50.json`, not a rebuilt monitor.
- The single successful `net50-compile-init` record in retained
  `logs/commands.jsonl` starts2026-10-09T19:50:21.956338Z. Its exact argv and source
  input comparisons are in `entry-bind.json`; all five current C/header files
  match recorded build50 input hashes. In particular:

| Source | SHA256 |
|---|---|
| `bringup/qc07f/initramfs/init.c` | `b3dd4ee9689bde4bead4c7a3c862284b198b879d4470b920366c1503814598aa` |
| `bringup/qc07f/initramfs/radio-ram-loader.h` | `91f8f9a4a1628b270664555dcc73e81c59c1a85ef33b08d8596946ae5dfd8196` |

Final image↔kernel/archive construction also relies on the already-completed
package/final-verifier proof in `kernel76-integration-20261020/RESULTS.md` and
`evidence/qualification/qc07f/kernel76-20261020/final-verify01/result.json`.
This lane did not rebuild/unpack the compressed kernel from scratch. The Oct20
boot receipt ties that image to the **historical** boot, not today's runtime.

Bounded disassembly independently confirms the important branches:

| ARM address | Finding |
|---|---|
| `0x477dc..0x47800` | Dispatch passes0 for load,1 for native-load to the same `radio_load` |
| `0x48e1c..0x48e94` | Exact151056 length and full reflected raw CRC residue check |
| `0x49020..0x49034` | Native2048 versus rollback256 chunk limit |
| `0x49078..0x49094`, `0x49b24..0x49b38` | Native skips both data and command endpoint-reset calls |
| `0x490b8..0x490f8` | Copy exact chunk plus four zero transport-tail bytes for512 multiples |
| `0x49134..0x49184` |1000ms data bulk call, exact wire-length comparison; ZLP test uses **wire** length |
| `0x4924c..0x49268` | Only after all data: command4, base20000000, size OR60000000 |
| `0x492c8..0x49328` | Interface release, watchdog magic-write/close, USB close; results not checked |
| `0x49cd0..0x49e1c` | At most four ACK reads, magic/cookie/command/check/status gates |

## 2. Exact local/private firmware admission

The input is **already embedded in `/init`**, selected at monitor build time by
`radio-payload.h:4–5` `.incbin` of retained local
`work/lightbulb-camera/implementation-20261008T205404Z/artifacts/retail-radio-payload36.bin`.
There is no runtime firmware filename, host upload, config or network dependency.
Changing that local file today cannot change the booted monitor.

The collector compared in memory only; `entry-bind.json` exports **booleans only**:

1. Embedded symbol interval equals that exact local payload artifact.
2. Payload and original local `hgics.bin` each match the retained private
   `analysis/radio-payload36-preparation.json` pins; no pin values exported.
3. Payload equals the entire original input slice beginning at its3072-byte
   declared header offset, exactly151056 bytes. Header chip8400, RAM base20000000,
   AES disabled and CRC enabled all match. Raw reflected CRC residue is zero.

All passed. No blob, raw config, device identifier or firmware digest is copied
into this report/evidence. These are local input/provenance checks, not current
radio identity. The loader itself enforces length/residue but **not a cryptographic
payload hash**; its exactness depends on the image/init binding above. A different
same-length zero-residue image would not be refused by CRC alone.

## 3. Existing finite entry and acceptance grammar

Anchors below are in `bringup/qc07f/initramfs/radio-ram-loader.h` unless qualified.

1. **Admission (`:50–70`):** native flag set;151056-byte/residue gate; sysfs
   onboard1-1 vendor/product a012:8400, configured1; stoppable watchdog; dynamic
   usbfs device from bus001/devnum2..127 (`init.c:319–330`); open watchdog RW,
   request/confirm timeout5; claim interface0. Prefix sysfs tests are not exact
   inventory/owner validation. Parent must independently require the expected
   endpoint descriptors/maxpacket512, empty modules/no bound station driver, no
   USB/watchdog consumers, correct boot/UID/idle resources and no competing sender.
2. **State/security (`:71–75`):** command0 ENTER cookie100, keyed boot template,
   response status must0 (BOOT); statusff/application is a refusal, **not reload
   permission**. Command1 GET_SC cookie101 must return status0. The helper does
   not inspect a returned security/capability value; no bypass/override is added.
3. **Data (`:77–117`):** cookie102 upward; command2 WRITE_MEM at20000000+offset,
   declared size `chunk+1024`, command OUT exactly20 bytes/500ms. Require its
   matched status0 ACK. Native then emits the exact raw data with transport tail,
   timeout1000ms, and requires exact returned wire length. Require another matched
   status0 raw-data ACK using the same cookie, then advance offset/cookie and ping
   watchdog. No retransmit/fragment resend exists.
4. **CRC/RUN (`:118–123`):** once offset151056, one command4 at20000000 with
   `151056|0x60000000 = 0x60024e10`, cookie176. Require matched command4/status0.
   Header definitions `station76-final/utils/fwdl.h:18–20` call bit30 CRC_CHK,
   bit29 **SP_CFG**, bit31 AES_DEC (off). `fwdl.c:313–341` uses these same flags.
   Do not inflate the loader comment's “SPI-config” into a separately audited SPI
   operation: bit29's name is SP_CFG; MCU internal implementation is not proved
   here. No host/main-CPU SPI/MTD/OTA/OTP operation or boot register-write opcode
   exists in this entry. The flag is retained unchanged, not a new firmware change.
   Radio ACK reports CRC-enabled RUN success; there is **no separate numeric CRC
   readback or cryptographic radio measurement**.
5. **Return (`:126–130`):** release interface0; write watchdog magicV and close;
   close USB. Prompt means PID1 returned; it is not a checked cleanup receipt.

Native success expectations (derive, do not mistake for measured packet traces):

| Item | Expected |
|---|---|
| Payload chunks |74:73×2048, last1552 at offset149504 |
| Raw data wire lengths |73×2052 (`0x804`), last1552 (`0x610`);151348 total |
| Data cookies |102..175, one command ACK and one data ACK each |
| All command transactions |77:ENTER, GET_SC,74 WRITE_MEM, RUN |
| Matched ACK count |151 total (77 command +74 raw-data); normal reply16 bytes |
| At maxpacket512, nominal data packet sizes | Full chunk512,512,512,512,4; last512,512,512,16 |
| Explicit ZLP calls |**Zero** for this image in either entry;2052 is not512-aligned |

Rollback `radio load` is unchanged:591 data fragments (590×256 +16), endpoint1
and81 CLEAR_HALT before every command and before raw data. It is a separately
admitted recovery profile only **after known clean/cold entry**, never an automatic
continuation from an uncertain native download. No candidate76 rollback trial ran.

ACK parser (`:20–35`) reads at most4×512 bytes/500ms, returns immediately on a
negative bulk result; skips short/wrong-magic/wrong-cookie packets; requires
bytes0..1=1a2b, cookie bytes6..7, check byte15=fd; command ACK must also match byte8.
Raw-data ACK deliberately accepts any command byte. Matching byte9 must0 at each
loader stage. It does not validate all header fields/declared reply lengths/type.
Parent should require the expected normal16-byte grammar and no unexplained
discarded packet; do not loosen the compiled parser or count mismatches as success.

## 4. Application identity and retirement — separate gates

After full success and returned command ownership, admit at most one existing
`radio state` and one `radio info` verification, in sequence; no module/association.
Both are active USB queries, **not passive status reads**, and both deliberately
CLEAR_HALT on1/81 before querying (`init.c:332–390`). Their resets happen **after**
the no-reset native-load interval and must not be misreported as within it.

- `radio state`:20-byte keyed ENTER, cookie2; require complete matched16-byte
  response, statusff and `RADIO STATE APPLICATION`. This alone is application
  *state*, not firmware/ABI identity.
- `radio info`:12-byte application command43, cookie1; expected36-byte reply,
  magic/type3/cookie1/cmd43 and return/payload length24. Independently parse retail
  payload at byte12:version02010001, SVN22118, chip8400, CPU1 (retained ABI fixture
  `test_retail_fwinfo_layout.py:7–14`). Monitor only checks a subset; printed
  “COOKIE/CMD MATCH” is not a full schema/identity acceptance. Full RX includes
  identifying fields: retain privately, export only allowlisted identity booleans.
- In the same owned boot, require PID1 monitor returned, no open usbfs/watchdog FD
  owners or new modules, watchdog inactive/nowayout0, unchanged volatile mount
  inventory, ISP idle/guards/queues zero, clean cached UART error/busy counters and
  no new HCD error/FIFO-warning evidence. Use parent reviewed bounded procfs/sysfs
  readers; no MMIO register polling. Do not query concurrently with the loader.
- There is no public “all URBs retired” counter. Synchronous bulk return plus
  source cleanup/usbfs close is bounded lifecycle evidence, not direct inspection
  of every HCD queue; a contradictory/unknown owner or warning remains HOLD.

## 5. Bounds, error paths and HOLD are not equivalent to a returned prompt

`radio_bulk` is synchronous USBDEVFS_BULK. Candidate source
`linux/drivers/usb/core/devio.c:1130–1166,1283–1370` waits for completion, kills the
URB on timeout/interruption, then frees it; return is negative status or successful
actual length. On errors partial `actual_length` is **not exposed** to the monitor.
Short nonnegative OUT becomes-EIO; no match after four ACK packets becomes-ETIMEDOUT;
matching bad command/check becomes-EIO; nonzero radio status stops the transaction.
Failure prevents later RUN except when uncertainty is the RUN exchange itself.

All loader paths attempt ordinary cleanup; release/magic-write/close/ping return
values are ignored and some pre-transfer failures are silent. There is **no sticky
target HOLD**, unique operation generation or target-side total deadline. Parent
must set durable operation HOLD **before TX**, keep sole ownership and bounded
private capture, and clear it only after the complete transcript/identity/retirement
gates. Late/unknown/malformed/short/error/reset/disconnect, missing final prompt,
cleanup uncertainty or unexpected TX is HOLD. Never resend RUN/download, clear
endpoints manually, reset GPIO42, kill/free an unknown operation, or infer success
from a later healthy prompt. The failure text suggests GPIO42 cold reset but the
loader does **not** perform it and this review does not admit it. Recovery is the
parent-reviewed user power-OFF/capture path; GPIO0 is not reliable recovery.

Nominal protocol timeout ceilings are not a safety deadline: one accepted data
cycle can consume up to500+4×500+1000+4×500=5500ms; the first ping also follows
ENTER and GET_SC (another5000ms). The5s watchdog can therefore reset on a slow but
otherwise parseable path; it is not guaranteed to outlast all timeout combinations.
No timeout/feeding change is proposed. All74 cycles plus entry/RUN have414.5s of
configured USB wait budgets **before** UART/scheduling/cleanup; watchdog may cut
this much shorter. Parent needs adequate fresh capture lifetime and its own finite
transaction deadline/reserves, not a guessed5s whole-loader timeout or blind retry.

## 6. Discrimination available now, and exactly what is missing

`source-review/transport-source-pins.json` records exact candidate HCD/usbfs/glue/
flag-source pins and selected nonprivate build-option facts (no private config).
The HCD hash matches the package manifest. Source anchors under
`artifacts/native-usb76-source01/`:

- `linux/drivers/usb/musb/musb_host.c:167–178,1278–1279,1479–1482`: candidate
  low-byte1 kick at NAK retry/continuation; no direct native CSR measurement.
- `:1234–1281,1379–1433,1454–1482`: cached IRQ CSR, successful PIO segment/toggle
  update, actual-length accounting and bounded maxpacket continuation exist
  internally. Nothing exports those values as cached monitor counters.
- `linux/arch/arm/mach-ak3918ev200/usb.c:28–43,143–171`: interrupt registers are
  read-clear; software toggle shadows are internal. No ad hoc MMIO/status poll.
- USB_MON/debugfs/dynamic-debug/TRACEPOINTS/TRACING are not enabled; FTRACE=y alone
  does not enable runtime tracing. `musb_trace.c:13–24` routes musb_dbg into trace,
  not ordinary per-packet console output. `usbfs_snoop` exists but defaults off,
  logs traffic/payload and is **not an admitted substitute**; do not enable it.

Available without a new image: expected submitted payload/wire lengths, successful
actual length or negative errno, complete short ACK headers/cookies/status, exact
offset progression, CRC-enabled RUN ACK, application state/ABI and bounded owner/
resource retirement, plus ordinary HCD warnings. This is enough to compare a
real functional result against the preserved offset0/2052 timeout. It cannot prove
the exact DATA0/1 sequence, that NAK retry actually occurred, WZC acknowledgement,
which segment partially transferred on failure, or physical MODE retention.
Report those as **unobserved**, not “normal toggles verified” or a sole-cause fix.

No host fixture tests were rerun or promoted to native evidence. The offline
identity/private-input checks passed; native loader/USB/MTU acceptance remains OPEN.
